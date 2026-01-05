#ifndef HTTP_SESSION_HPP
#define HTTP_SESSION_HPP

#include <vector>
#include <memory>
#include <string_view>
#include <chrono>
#include "ouroboros/http/io_context.hpp"
#include "ouroboros/http/task.hpp"

namespace ouroboros::http
{

    class http_session : public task
    {
    public:
        // ソケットの所有権を server から引き継ぐ
        http_session(io_context &ctx, unique_socket socket);
        ~http_session();

        // 処理開始 (最初の Read を発行)
        void start();

    private:
        // task インターフェースの実装 (IO完了時に呼ばれる)
        void complete(int result) override;

        // 受信リクエストの発行
        void submit_recv();
        // 送信リクエストの発行
        void submit_send(std::string_view data);
        // タイムアウト監視の発行
        void submit_timeout();

        // 内部状態
        enum class state
        {
            reading,
            writing,
            closed
        };

        io_context &ctx_;
        unique_socket socket_;
        state current_state_ = state::closed;

        // 受信バッファ (簡易実装: 固定サイズ)
        // 本格的な実装では要件定義にある通りリングバッファやプールを使用します
        std::vector<char> buffer_;

        // タイムアウト管理用
        __kernel_timespec ts_;
        int pending_ops_ = 0; // 実行中の非同期操作数 (0になったら削除)
        std::chrono::steady_clock::time_point last_activity_;
    };

}
#endif // HTTP_SESSION_HPP
