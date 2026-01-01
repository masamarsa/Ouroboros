#ifndef UNIQUE_SOCKET_HPP
#define UNIQUE_SOCKET_HPP

#include <utility>
#include <unistd.h>

namespace ouroboros::http
{
    class unique_socket
    {
    private:
        int fd_;
    protected:
    public:
        // デフォルトコンストラクタ：無効なFD（-1）で初期化
        constexpr unique_socket() noexcept : fd_(-1) {}
        // FDによる明示的な初期化
        explicit constexpr unique_socket(int fd) noexcept : fd_(fd) {}
        ~unique_socket() {
            if (fd_ >= 0) ::close(fd_);
        }
        // コピー禁止（所有権の唯一性を保証）
        unique_socket(const unique_socket &) = delete;
        unique_socket &operator=(const unique_socket &) = delete;
        // ムーブコンストラクタ
        constexpr unique_socket(unique_socket &&other) noexcept : fd_(std::exchange(other.fd_, -1)) {}
        // ムーブ代入演算子
        constexpr unique_socket &operator=(unique_socket &&other) noexcept {
            if (this != &other) {
                if (fd_ >= 0) ::close(fd_);
                // 所有権を移動し、元のオブジェクトは無効化
                fd_ = std::exchange(other.fd_, -1);
            }
            return *this;
        }
        // 生のFDへのアクセス
        [[nodiscard]] constexpr int native_handle() const noexcept {
            return fd_;
        }
        // 有効性チェック
        explicit constexpr operator bool() const noexcept {
            return fd_ >= 0;
        }
    };
} // namespace ouroboros::http

#endif // UNIQUE_SOCKET_HPP
