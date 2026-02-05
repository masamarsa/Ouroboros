#ifndef OUROBOROS_HTTP_ERROR_HPP
#define OUROBOROS_HTTP_ERROR_HPP

#include <system_error>

namespace ouroboros::http
{
    // プロジェクト固有のエラーコードを定義
    enum class error_code
    {
        success = 0, // 成功
        socket_creation_failed,
        socket_option_failed,
        bind_failed,
        listen_failed,
    };

    // カスタムエラーカテゴリを取得するための関数宣言
    const std::error_category &ouroboros_category() noexcept;

    // error_code から std::error_code を生成するためのインライン関数
    inline std::error_code make_error_code(error_code e) noexcept {
        return { static_cast<int>(e), ouroboros_category() };
    }
}

namespace std
{
    // std::error_code との連携を有効にする
    template <>
    struct is_error_code_enum<ouroboros::http::error_code> : true_type
    {};
}

#endif // OUROboros_HTTP_ERROR_HPP