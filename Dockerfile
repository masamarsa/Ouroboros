# -----------------------------------------------------------------------------
# Stage 1: Development Environment (Target for VS Code / Dev Containers)
# -----------------------------------------------------------------------------
FROM debian:trixie-slim AS dev

# 環境変数の設定
ENV DEBIAN_FRONTEND=noninteractive
ENV CC=gcc
ENV CXX=g++

# 必要なパッケージのインストール
# linux-libc-dev: io_uring用ヘッダー (<linux/io_uring.h>)
# gdb: デバッグ用
# cmake, ninja-build: ビルドシステム
RUN apt-get update && apt-get upgrade -y && apt-get install -y --no-install-recommends build-essential cmake ninja-build gdb git curl ca-certificates linux-libc-dev libssl-dev less procps && apt-get clean && rm -rf /var/lib/apt/lists/*

# 開発用プロンプト設定 (視認性向上)
RUN echo 'PS1="\u@ouroboros-dev:\w\$ "' >> /root/.bashrc

WORKDIR /workspace

# デフォルトコマンド (コンテナ維持用)
CMD ["/bin/bash"]

# -----------------------------------------------------------------------------
# Stage 2: Static Builder (For Production)
# -----------------------------------------------------------------------------
FROM dev AS builder

WORKDIR /build
COPY . .

# 静的リンクでビルド (Distrolessで動かすため)
# ※ CMakeLists.txtの設定に依存しますが、ここではフラグ例を示します
RUN cmake -S . -B build_release -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_EXE_LINKER_FLAGS="-static" \
    && cmake --build build_release

# -----------------------------------------------------------------------------
# Stage 3: Production Release (Distroless)
# -----------------------------------------------------------------------------
# 静的バイナリ用Distroless (glibc不要)
FROM gcr.io/distroless/static-debian12 AS release

WORKDIR /app
COPY --from=builder /build/build_release/ouroboros_server /app/ouroboros_server

ENTRYPOINT ["/app/ouroboros_server"]