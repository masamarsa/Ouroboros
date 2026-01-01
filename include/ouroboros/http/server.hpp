#ifndef SERVER_HPP
#define SERVER_HPP

#include "ouroboros/http/io_context.hpp"
#include "ouroboros/http/task.hpp"
#include <netinet/in.h>

namespace ouroboros::http
{

    class server : public task
    {
    public:
        server(io_context &ctx, uint16_t port);
        ~server();

        // サーバー起動 (Bind -> Listen -> 最初のAccept発行)
        void start();

    private:
        // task インターフェースの実装: Accept完了時に呼ばれる
        void complete(int result) override;

        // 次のAcceptリクエストを発行するヘルパー
        void submit_accept();

        io_context &ctx_;
        unique_socket server_socket_;
        uint16_t port_;

        // Accept用のバッファ (接続元アドレス情報)
        struct sockaddr_in client_addr_;
        socklen_t client_len_;
    };

}

#endif // SERVER_HPP