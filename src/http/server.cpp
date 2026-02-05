#include "ouroboros/http/server.hpp"
#include "ouroboros/http/http_session.hpp"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <iostream>
#include <cstring>
#include <expected>

namespace ouroboros::http
{
    std::expected<server, std::error_code> server::create(io_context &ctx, uint16_t port) {
        // 1. ソケット作成
        int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0); // ノンブロッキング重要
        if (fd < 0) {
            return std::unexpected(error_code::socket_creation_failed);
        }
        unique_socket server_sock(fd);

        // 2. SO_REUSEADDR / SO_REUSEPORT (要件 3.3)
        int opt = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            return std::unexpected(error_code::socket_option_failed);
        }
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
            return std::unexpected(error_code::socket_option_failed);
        }

        // 3. Bind
        struct sockaddr_in addr
        {};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);

        if (::bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            return std::unexpected(error_code::bind_failed);
        }

        return server(ctx, port, std::move(server_sock));
    }

    server::server(io_context &ctx, uint16_t port, unique_socket socket)
        : ctx_(ctx), server_socket_(std::move(socket)), port_(port) {}

    std::expected<void, std::error_code> server::start() {
        // 4. Listen
        if (::listen(server_socket_.native_handle(), SOMAXCONN) < 0) {
            return std::unexpected(error_code::listen_failed);
        }
        std::cout << "Server listening on port " << port_ << std::endl;

        // 最初の Accept リクエストを発行
        submit_accept();
        return {}; // Success
    }

    void server::submit_accept() {
        // SQEを取得
        auto *sqe = ctx_.get_sqe();
        if (!sqe) {
            std::cerr << "SQE full!" << std::endl;
            return;
        }

        client_len_ = sizeof(client_addr_);

        // IORING_OP_ACCEPT を手動設定 (liburing_prep_accept 相当)
        sqe->opcode = IORING_OP_ACCEPT;
        sqe->fd = server_socket_.native_handle();
        sqe->addr = (uint64_t)&client_addr_;
        sqe->addr2 = (uint64_t)&client_len_;
        sqe->flags = 0; // SOCK_NONBLOCKなどを指定可能

        // 完了時のコールバックとして自分自身(server)を登録
        sqe->user_data = (uint64_t)this;

        // カーネルへ送信 (バッチ対応)
        ctx_.submit();
    }

    // Accept完了時に呼ばれる (イベントループから)
    void server::complete(int result, uint32_t flags) {
        (void)flags; // このコンテキストではフラグは未使用
        if (result < 0) {
            std::cerr << "Accept failed: " << -result << std::endl;
        } else {
            int client_fd = result;
            unique_socket client_sock(client_fd);

            std::cout << "New Connection! FD: " << client_fd << std::endl;

            // Sessionを作成し、start() を呼ぶ
            // Sessionは通信終了時に delete this するので、ここではポインタを渡して放置する (Fire & Forget)
            auto *session = new http_session(*this, ctx_, std::move(client_sock));
            session->start();
        }

        submit_accept();
    }

    void server::load_routes(const std::vector<route_entry> &routes) {
        for (const auto &entry : routes) {
            routes_[entry.method][entry.path] = entry.handler;
        }
    }

    std::optional<handler_function> server::find_handler(method method, const std::string &path) const {
        if (auto it_method = routes_.find(method); it_method != routes_.end()) {
            const auto &path_map = it_method->second;
            if (auto it_path = path_map.find(path); it_path != path_map.end()) {
                return it_path->second;
            }
        }
        return std::nullopt;
    }


}