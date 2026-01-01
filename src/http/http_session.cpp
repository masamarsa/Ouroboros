#include "ouroboros/http/http_session.hpp"
#include <iostream>
#include <cstring>

namespace ouroboros::http
{

    http_session::http_session(io_context &ctx, unique_socket socket)
        : ctx_(ctx), socket_(std::move(socket)), buffer_(4096) { // 4KB buffer
    }

    http_session::~http_session() {
        std::cout << "Session closed. FD: " << socket_.native_handle() << std::endl;
    }

    void http_session::start() {
        submit_recv();
    }

    void http_session::submit_recv() {
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

    void http_session::complete(int result) {
        if (result <= 0) {
            delete this;
            return;
        }

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
            delete this;
        }
    }

}