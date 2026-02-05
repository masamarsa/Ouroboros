#pragma once

#include <string>
#include <functional>
#include <vector>
#include <map>

namespace ouroboros::http
{

    // 型安全なHTTPメソッド処理を保証するためのenumクラス
    enum class method
    {
        GET,
        POST,
        PUT,
        DELETE,
        PATCH,
        HEAD,
        OPTIONS
    };

    // リクエストクラス（プレースホルダ）
    // 実際のHTTPリクエストデータに基づき、セッションによって構築される
    class request
    {
    public:
        // 現在はセッションがオブジェクトを容易に構築できるよう、メンバはpublic
        // より堅牢な実装では、ビルダーパターンやフレンドクラスの使用が考えられる
        http::method method;
        std::string path;
        std::map<std::string, std::string> headers;
        std::string body;
    };

    // レスポンスクラス（プレースホルダ）
    // ハンドラはこのクラスを用いてレスポンスを構築する
    // その後、セッションがこのクラスの内容を基に最終的なHTTPレスポンスを生成する
    class response
    {
    public:
        response() : status_code_(200) {}

        void set_body(const std::string &body) {
            body_ = body;
        }

        void set_header(const std::string &name, const std::string &value) {
            headers_[name] = value;
        }

        void set_status_code(int code) {
            status_code_ = code;
        }

        // セッションが最終的なHTTPレスポンス文字列を構築するための内部アクセサ
        int status_code() const {
            return status_code_;
        }
        const std::string &body() const {
            return body_;
        }
        const std::map<std::string, std::string> &headers() const {
            return headers_;
        }

    private:
        int status_code_;
        std::string body_;
        std::map<std::string, std::string> headers_;
    };

    // 全てのHTTPハンドラのシグネチャを定義
    using handler_function = std::function<void(const request &, response &)>;

    // ルーティングテーブル内の一つのルート（経路）を表現する構造体
    struct route_entry
    {
        http::method method;
        std::string path;
        handler_function handler;
    };

} // namespace ouroboros::http
