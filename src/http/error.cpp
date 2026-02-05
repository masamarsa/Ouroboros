#include "ouroboros/http/error.hpp"
#include <string>

namespace ouroboros::http
{
    // カスタムエラーカテゴリの実装
    class ouroboros_error_category_impl : public std::error_category
    {
    public:
        const char *name() const noexcept override {
            return "ouroboros.http";
        }

        std::string message(int condition) const override {
            switch (static_cast<error_code>(condition)) {
            case error_code::socket_creation_failed: return "Socket creation failed";
            case error_code::socket_option_failed:   return "Setting socket option failed";
            case error_code::bind_failed:            return "Socket bind failed";
            case error_code::listen_failed:          return "Socket listen failed";
            default:                           return "Unknown Ouroboros error";
            }
        }
    };

    const std::error_category &ouroboros_category() noexcept {
        static ouroboros_error_category_impl instance;
        return instance;
    }
}