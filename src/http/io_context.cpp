#include "ouroboros/http/io_context.hpp"
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdexcept>
#include <cstring> // for memset
#include <chrono>
#include <thread>
#include <iostream>
#include <signal.h> // 追加: シグナル関連の定義

// 追加: 環境によって _NSIG が定義されていない場合のフォールバック
// Linuxカーネルの標準的なシグナル数は64
#ifndef _NSIG
#define _NSIG 64
#endif

// システムコールラッパー
static int io_uring_setup_syscall(unsigned entries, struct io_uring_params *p) {
    return (int)syscall(__NR_io_uring_setup, entries, p);
}

static int io_uring_enter_syscall(int fd, unsigned to_submit, unsigned min_complete,
    unsigned flags, sigset_t *sig) {
    // _NSIG / 8 = 8 bytes (64 bits) を渡すのがカーネルの期待値
    return (int)syscall(__NR_io_uring_enter, fd, to_submit, min_complete, flags, sig, _NSIG / 8);
}

namespace ouroboros::http
{

    io_context::io_context(unsigned entries) {
        std::memset(&params_, 0, sizeof(params_));

        // 1. io_uring インスタンスの作成
        int fd = io_uring_setup_syscall(entries, &params_);
        if (fd < 0) {
            // エラー詳細を出力するとデバッグしやすいです
            std::perror("io_uring_setup_syscall failed");
            throw std::runtime_error("io_uring_setup failed");
        }
        ring_fd_ = unique_socket(fd); // RAII管理へ

        // 2. メモリマッピングの実行
        try {
            setup_memory_mapping();
        } catch (...) {
            // 例外発生時はデストラクタが呼ばれないため、ここで手動クリーンアップが必要だが、
            // unique_socket が fd は閉じてくれる。
            // mmap したメモリの解放ロジックが必要だが、ここでは簡略化のため省略
            throw;
        }
    }

    io_context::~io_context() {
        // mmap 解除
        if (sqes_ptr_) munmap(sqes_ptr_, sqes_mmap_sz_);
        if (sq_ptr_) munmap(sq_ptr_, sq_mmap_sz_);
        if (cq_ptr_) munmap(cq_ptr_, cq_mmap_sz_);
        // ring_fd_ は unique_socket により自動クローズ
    }

    void io_context::setup_memory_mapping() {
        // --- Submission Queue (SQ) Mapping ---
        sq_mmap_sz_ = params_.sq_off.array + params_.sq_entries * sizeof(__u32);

        sq_ptr_ = mmap(0, sq_mmap_sz_, PROT_READ | PROT_WRITE,
            MAP_SHARED | MAP_POPULATE, ring_fd_.native_handle(), IORING_OFF_SQ_RING);
        if (sq_ptr_ == MAP_FAILED) throw std::runtime_error("mmap SQ failed");

        // ポインタの接続
        sq_.head = (uint32_t *)((char *)sq_ptr_ + params_.sq_off.head);
        sq_.tail = (uint32_t *)((char *)sq_ptr_ + params_.sq_off.tail);
        sq_.ring_mask = (uint32_t *)((char *)sq_ptr_ + params_.sq_off.ring_mask);
        sq_.ring_entries = (uint32_t *)((char *)sq_ptr_ + params_.sq_off.ring_entries);
        sq_.flags = (uint32_t *)((char *)sq_ptr_ + params_.sq_off.flags);
        sq_.array = (uint32_t *)((char *)sq_ptr_ + params_.sq_off.array);

        // --- SQ Entries (SQEs) Mapping ---
        // 実際にリクエスト構造体を書き込む場所
        sqes_mmap_sz_ = params_.sq_entries * sizeof(struct io_uring_sqe);
        sqes_ptr_ = mmap(0, sqes_mmap_sz_, PROT_READ | PROT_WRITE,
            MAP_SHARED | MAP_POPULATE, ring_fd_.native_handle(), IORING_OFF_SQES);
        if (sqes_ptr_ == MAP_FAILED) throw std::runtime_error("mmap SQEs failed");

        // --- Completion Queue (CQ) Mapping ---
        cq_mmap_sz_ = params_.cq_off.cqes + params_.cq_entries * sizeof(struct io_uring_cqe);

        cq_ptr_ = mmap(0, cq_mmap_sz_, PROT_READ | PROT_WRITE,
            MAP_SHARED | MAP_POPULATE, ring_fd_.native_handle(), IORING_OFF_CQ_RING);
        if (cq_ptr_ == MAP_FAILED) throw std::runtime_error("mmap CQ failed");

        cq_.head = (uint32_t *)((char *)cq_ptr_ + params_.cq_off.head);
        cq_.tail = (uint32_t *)((char *)cq_ptr_ + params_.cq_off.tail);
        cq_.ring_mask = (uint32_t *)((char *)cq_ptr_ + params_.cq_off.ring_mask);
        cq_.ring_entries = (uint32_t *)((char *)cq_ptr_ + params_.cq_off.ring_entries);
    }

    io_uring_sqe *io_context::get_sqe() noexcept {
        uint32_t head = std::atomic_load_explicit((std::atomic<uint32_t>*)sq_.head, std::memory_order_acquire);
        uint32_t tail = *sq_.tail; // User only writes tail

        if (tail - head >= *sq_.ring_entries) {
            return nullptr; // Ring is full
        }

        struct io_uring_sqe *sqes = (struct io_uring_sqe *)sqes_ptr_;
        struct io_uring_sqe *sqe = &sqes[tail & *sq_.ring_mask];

        // SQEをクリアしておく
        std::memset(sqe, 0, sizeof(*sqe));

        return sqe;
    }

    void io_context::submit_request() {
        // Tailを更新してカーネルに通知
        uint32_t tail = *sq_.tail;
        uint32_t index = tail & *sq_.ring_mask;

        sq_.array[index] = index; // 単純なマッピング
        tail++;

        std::atomic_store_explicit((std::atomic<uint32_t>*)sq_.tail, tail, std::memory_order_release);

        // システムコールでカーネルを起こす
        // io_uring_enter(fd, to_submit, min_complete, flags, sig)
        io_uring_enter_syscall(ring_fd_.native_handle(), 1, 0, 0, nullptr);
    }

    void io_context::process_completions() {
        unsigned head;
        head = std::atomic_load_explicit((std::atomic<uint32_t>*)cq_.head, std::memory_order_acquire);

        while (head != *cq_.tail) {
            // 【修正前】
            // struct io_uring_cqe* cqes = (struct io_uring_cqe*)cq_ptr_;

            // 【修正後】 cq_off.cqes オフセットを足して正しい先頭アドレスを取得する
            struct io_uring_cqe *cqes = (struct io_uring_cqe *)((char *)cq_ptr_ + params_.cq_off.cqes);

            struct io_uring_cqe *cqe = &cqes[head & *cq_.ring_mask];

            if (cqe->user_data) {
                auto *t = reinterpret_cast<task *>(cqe->user_data);
                t->complete(cqe->res);
            }

            head++;
        }

        std::atomic_store_explicit((std::atomic<uint32_t>*)cq_.head, head, std::memory_order_release);
    }

    void io_context::run() {
        std::cout << "io_context: Event loop running..." << std::endl;

        while (true) {
            // CQEを処理
            process_completions();
            // ここで少し待機しないとCPU 100%になる (ビジーループ)
            // 本番では io_uring_enter で待機するが、今は簡易的に wait
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            // io_uring_enter(fd, 0, 1, IORING_ENTER_GETEVENTS) で1つ以上の完了を待つのが定石
            io_uring_enter_syscall(ring_fd_.native_handle(), 0, 1, IORING_ENTER_GETEVENTS, nullptr);
        }
    }

} // namespace ouroboros::http