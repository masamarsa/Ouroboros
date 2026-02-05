#ifndef HTTP_SESSION_HPP
#define HTTP_SESSION_HPP

#include <vector>
#include <memory>
#include <string_view>
#include <chrono>
#include "ouroboros/http/io_context.hpp"
#include "ouroboros/http/task.hpp"
#include "ouroboros/http/member_binder.hpp"

namespace ouroboros::http
{
    class server; // Forward-declaration

    class http_session : public task
    {
    public:
        // Constructor now accepts a reference to the server to access the routing table
        http_session(server &svr, io_context &ctx, unique_socket socket);
        ~http_session();

        // 処理開始 (最初の Read を発行)
        void start();

    private:
        // task インターフェースの実装 (IO完了時に呼ばれる)
        void complete(int result, uint32_t flags) override;

        // Main logic for request processing
        void handle_request();

        // Low-level IO operations
        void submit_recv();
        void submit_send(std::string_view data);
        void handle_read(int result, uint32_t flags);
        void handle_write(int result);

        // 内部状態
        enum class state
        {
            reading,
            writing,
            closed
        };

        server &server_; // Reference to the server instance
        io_context &ctx_;
        unique_socket socket_;
        state current_state_ = state::closed;
        std::vector<char> buffer_;

        // タイムアウト管理用
        __kernel_timespec ts_;
        int pending_ops_ = 0; // 実行中の非同期操作数 (0になったらセッションを削除)
        bool keep_alive_ = false;
    };

}

#endif // HTTP_SESSION_HPP
