cmake_minimum_required(VERSION 3.20)

# プロジェクト定義
project(ouroboros VERSION 0.1.0 LANGUAGES CXX)

# C++23 標準の強制 [cite: 5]
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# コンパイルオプション (高品質なコードのための警告設定)
add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)

# ヘッダーファイルのパス設定
include_directories(include)

# --- ライブラリ定義 (ouroboros_http) ---
# 将来的には io_context 等もここに追加されます
add_library(ouroboros_http
    src/http/unique_socket.cpp
    src/http/io_context.cpp
    src/http/server.cpp
    src/http/http_session.cpp
)

# --- 実行ファイル定義 (Server) ---
add_executable(ouroboros_server
    src/main.cpp
)

# ライブラリのリンク
target_link_libraries(ouroboros_server PRIVATE ouroboros_http)