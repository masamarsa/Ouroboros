#include "ouroboros/http/server.hpp"
#include "ouroboros/http/http_session.hpp"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <iostream>
#include <cstring>
#include <stdexcept>

namespace ouroboros::http
{

    server::server(io_context &ctx, uint16_t port)
        : ctx_(ctx), port_(port) {

        // 1. ソケット作成
        int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0); // ノンブロッキング重要
        if (fd < 0) throw std::runtime_error("socket failed");
        server_socket_ = unique_socket(fd);

        // 2. SO_REUSEADDR / SO_REUSEPORT (要件 3.3)
        int opt = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

        // 3. Bind
        struct sockaddr_in addr
        {};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);

        if (::bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            throw std::runtime_error("bind failed");
        }
    }

    server::~server() = default;

    void server::start() {
        // 4. Listen
        if (::listen(server_socket_.native_handle(), SOMAXCONN) < 0) {
            throw std::runtime_error("listen failed");
        }
        std::cout << "Server listening on port " << port_ << std::endl;

        // 最初の Accept リクエストを発行
        submit_accept();
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

        // カーネルへ送信
        ctx_.submit_request();
    }

    // Accept完了時に呼ばれる (イベントループから)
    void server::complete(int result) {
        if (result < 0) {
            std::cerr << "Accept failed: " << -result << std::endl;
        } else {
            int client_fd = result;
            unique_socket client_sock(client_fd);

            std::cout << "New Connection! FD: " << client_fd << std::endl;

            // --- 変更点 ---
            // Sessionを作成し、start() を呼ぶ
            // Sessionは通信終了時に delete this するので、ここではポインタを渡して放置する (Fire & Forget)
            auto *session = new http_session(ctx_, std::move(client_sock));
            session->start();
            // --------------
        }

        submit_accept();
    }

}