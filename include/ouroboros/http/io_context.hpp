#ifndef IO_CONTEXT_HPP
#define IO_CONTEXT_HPP

#include <atomic>
#include <cstdint>
#include <cstring>
#include <linux/io_uring.h> // カーネルヘッダー
#include <linux/time_types.h>
#include "ouroboros/http/unique_socket.hpp"
#include "ouroboros/http/task.hpp"

namespace ouroboros::http
{
    // liburingを使わずにリングバッファを管理するための構造体
    struct io_uring_queue
    {
        uint32_t *head;
        uint32_t *tail;
        uint32_t *ring_mask;
        uint32_t *ring_entries;
        uint32_t *flags;
        uint32_t *array; // SQのみ使用 (index array)
    };
    class io_context
    {
    public:
        // コンストラクタ: io_uring_setup システムコールを発行し、リングをmmapする
        explicit io_context(unsigned entries = 4096);
        ~io_context();
        // コピー禁止 (リソースへのポインタを持つため)
        io_context(const io_context &) = delete;
        io_context &operator=(const io_context &) = delete;
        // イベントループの開始 (ブロッキング)
        void run();
        void process_completions();
        // SQE (Submission Queue Entry) を取得する
        // 取得できない場合 (Full) は nullptr を返す
        [[nodiscard]] io_uring_sqe *get_sqe() noexcept;
        // キューに入れたリクエストをカーネルに送信する
        void submit_request();

        // タイムアウトを設定する (SQEの準備)
        // 注意: ts は submit_request() が完了するまで(正確にはカーネルが読み込むまで)有効である必要があります。
        // そのため、ts はスタック変数ではなく、http_session などの永続的なオブジェクトの一部として管理してください。
        inline void prepare_timeout(io_uring_sqe *sqe, __kernel_timespec *ts, task *task) noexcept {
            std::memset(sqe, 0, sizeof(*sqe));
            sqe->opcode = IORING_OP_TIMEOUT;
            sqe->fd = -1;
            sqe->off = 0; // count=0: 指定時間が経過したら発火 (相対時間)
            sqe->addr = reinterpret_cast<uintptr_t>(ts);
            sqe->len = 1;
            sqe->user_data = reinterpret_cast<uintptr_t>(task);
        }

    private:
        unique_socket ring_fd_; // io_uring のファイル記述子
        // マッピングされたメモリ領域へのポインタ
        void *sq_ptr_ = nullptr;
        void *cq_ptr_ = nullptr;
        void *sqes_ptr_ = nullptr; // SQE配列自体
        size_t sq_mmap_sz_ = 0;
        size_t cq_mmap_sz_ = 0;
        size_t sqes_mmap_sz_ = 0;
        // リングバッファの制御構造体
        io_uring_queue sq_;
        io_uring_queue cq_;
        struct io_uring_params params_;
        // 内部ヘルパー: mmap のセットアップ
        void setup_memory_mapping();
    };
} // namespace ouroboros::http
#endif // IO_CONTEXT_HPP
