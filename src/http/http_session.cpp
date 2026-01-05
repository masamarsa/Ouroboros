#include "ouroboros/http/http_session.hpp"
#include <iostream>
#include <cstring>
#include <linux/time_types.h>

namespace ouroboros::http
{

    http_session::http_session(io_context &ctx, unique_socket socket)
        : ctx_(ctx), socket_(std::move(socket)), buffer_(4096) { // 4KB buffer
        last_activity_ = std::chrono::steady_clock::now();
    }

    http_session::~http_session() {
        std::cout << "Session closed. FD: " << socket_.native_handle() << std::endl;
    }

    void http_session::start() {
        submit_recv();
        submit_timeout();
    }

    void http_session::submit_recv() {
        pending_ops_++;
        current_state_ = state::reading;

        auto *sqe = ctx_.get_sqe();
        if (!sqe) return;

        sqe->opcode = IORING_OP_RECV;
        sqe->fd = socket_.native_handle();
        sqe->addr = (uint64_t)buffer_.data();

        // 【修正1】 size_t (64bit) -> __u32 (32bit) への明示的キャスト
        sqe->len = static_cast<uint32_t>(buffer_.size());

        sqe->flags = 0;
        sqe->user_data = (uint64_t)this;

        ctx_.submit_request();
    }

    void http_session::submit_send(std::string_view data) {
        pending_ops_++;
        current_state_ = state::writing;

        auto *sqe = ctx_.get_sqe();
        if (!sqe) return;

        sqe->opcode = IORING_OP_SEND;
        sqe->fd = socket_.native_handle();
        sqe->addr = (uint64_t)data.data();

        // 【修正2】 size_t (64bit) -> __u32 (32bit) への明示的キャスト
        sqe->len = static_cast<uint32_t>(data.size());

        sqe->flags = 0;
        sqe->user_data = (uint64_t)this;

        ctx_.submit_request();
    }

    void http_session::submit_timeout() {
        pending_ops_++;
        // 5秒のタイムアウトを設定
        ts_.tv_sec = 5;
        ts_.tv_nsec = 0;

        auto *sqe = ctx_.get_sqe();
        if (!sqe) return;

        ctx_.prepare_timeout(sqe, &ts_, this);
        ctx_.submit_request();
    }

    void http_session::complete(int result) {
        pending_ops_--;

        // タイムアウト発生時の処理 (-ETIME = -62)
        if (result == -ETIME) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_activity_).count();

            if (elapsed < 5) {
                // 最近アクティビティがあった場合は、タイムアウトを再設定して継続
                submit_timeout();
                return;
            } else {
                std::cout << "Session timed out." << std::endl;
                // ソケットを閉じて他のPending操作(Read/Write)をキャンセルさせる
                socket_ = unique_socket();
            }
        }

        // エラーまたは切断、あるいはタイムアウト確定時
        if (result <= 0) {
            // 他に実行中の操作がなければ削除
            if (pending_ops_ == 0) {
                delete this;
            } else {
                // まだPendingがある場合は、ソケットを閉じてキャンセルを促す
                // (既に閉じている場合は何もしない)
                if (socket_) socket_ = unique_socket();
            }
            return;
        }

        // 正常なRead/Write完了時はアクティビティ時刻を更新
        last_activity_ = std::chrono::steady_clock::now();

        if (current_state_ == state::reading) {
            std::cout << "Received " << result << " bytes." << std::endl;

            // 【修正3】 int (signed) -> size_t (unsigned) への明示的キャスト
            // 直前の if (result <= 0) チェックにより、ここでの result は正の値であることが保証されています
            std::string_view received_data(buffer_.data(), static_cast<size_t>(result));

            std::cout << "--- Request ---\n" << received_data << "\n---------------" << std::endl;

            // ... (レスポンス定義はそのまま)
            static const char response[] =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: 16\r\n"
                "Connection: close\r\n"
                "\r\n"
                "Hello, io_uring!";

            submit_send(response);

        } else if (current_state_ == state::writing) {
            std::cout << "Response sent." << std::endl;
            // レスポンス送信後は切断する仕様 (Connection: close)
            // ソケットを閉じて、残っているタイムアウト等の完了を待つ
            socket_ = unique_socket();
            if (pending_ops_ == 0) delete this;
        }
    }

}