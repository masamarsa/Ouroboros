cmake_minimum_required(VERSION 3.20)

# プロジェクト定義
project(ouroboros VERSION 0.1.0 LANGUAGES CXX)

# C++23 標準の強制 [cite: 5]
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ヘッダーファイルのパス設定
include_directories(include)

# --- ライブラリ定義 (ouroboros_http) ---
# 将来的には io_context 等もここに追加されます
add_library(ouroboros_http
    src/http/unique_socket.cpp
    src/http/io_context.cpp
    src/http/server.cpp
    src/http/error.cpp
    src/http/http_session.cpp
)

# コンパイルオプション (高品質なコードのための警告設定)
# ターゲットに直接設定することで、FetchContentで取得した外部ライブラリに影響を与えないようにする
target_compile_options(ouroboros_http PUBLIC -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)

# --- 実行ファイル定義 (Server) ---
add_executable(ouroboros_server
    src/main.cpp
)

# ライブラリのリンク
target_link_libraries(ouroboros_server PRIVATE ouroboros_http)

# --- Unit Testing (Google Test) ---

# CTestを有効化
enable_testing()

# --- 3rd Party (Google Test) ---
# FetchContentモジュールをインクルード
include(FetchContent)

# FetchContentの挙動に関するポリシーを設定し、タイムスタンプの警告を抑制
cmake_policy(SET CMP0135 NEW)

# Google Testをダウンロードするように設定
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/v1.14.0.zip
)

# Google Testをビルドプロセスで利用可能にする
FetchContent_MakeAvailable(googletest)

# テスト用の実行ファイルを追加 (新しいテストファイルはここに追加)
# add_executable(ouroboros_tests
#     tests/unique_socket_test.cpp
#     tests/buffer_pool_test.cpp
# )
# target_link_libraries(ouroboros_tests PRIVATE gtest_main ouroboros_http)
# 
# # CTestがテストを自動検出できるようにする
# include(GoogleTest)
# gtest_discover_tests(ouroboros_tests)