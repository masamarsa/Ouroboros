#pragma once

#include "ouroboros/http/io_context.hpp"
#include "ouroboros/http/error.hpp"
#include "ouroboros/http/task.hpp"
#include "ouroboros/http/member_binder.hpp"
#include "ouroboros/http/type_definitions.hpp"
#include <netinet/in.h>
#include <expected>
#include <map>
#include <vector>
#include <string>
#include <optional>

namespace ouroboros::http
{
    class server : public task
    {
    public:
        // Factory function for safe creation
        [[nodiscard]] static std::expected<server, std::error_code> create(io_context &ctx, uint16_t port);
        ~server() = default;

        // Prohibit copying, allow moving
        server(const server &) = delete;
        server &operator=(const server &) = delete;
        server(server &&) = default;
        server &operator=(server &&) = default;

        // サーバー起動 (Bind -> Listen -> 最初のAccept発行)
        [[nodiscard]] std::expected<void, std::error_code> start();

        // Load routes into the routing table
        void load_routes(const std::vector<route_entry> &routes);

        // Find a handler for a given method and path
        std::optional<handler_function> find_handler(method method, const std::string &path) const;

    private:
        // Private constructor, called by create()
        server(io_context &ctx, uint16_t port, unique_socket socket);

        // task インターフェースの実装: Accept完了時に呼ばれる
        void complete(int result, uint32_t flags) override;

        // 次のAcceptリクエストを発行するヘルパー
        void submit_accept();

        io_context &ctx_;
        unique_socket server_socket_;
        uint16_t port_;

        // Accept用のバッファ (接続元アドレス情報)
        struct sockaddr_in client_addr_;
        socklen_t client_len_;

        // Routing table
        std::map<method, std::map<std::string, handler_function>> routes_;
    };
}