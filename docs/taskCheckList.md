# 🐍 Ouroboros Project: Task Progress Checklist

**目標:** C++23 / Linux (io_uring) を用いた、依存関係ゼロ・超高性能Webサーバーライブラリの構築

## 🏗 Phase 0: 開発環境構築 (Infrastructure)

* [x] **Docker環境の構築**
  * [x] ベースイメージ: Debian Trixie (Testing) for GCC 14+
  * [x] マルチステージビルド (Dev / Builder / Distroless Release)
* [x] **VS Code Dev Containers 設定**
  * [x] `devcontainer.json` の整備 (C++23, CMake, GDBサポート)
  * [x] 特権設定 (`cap_add: SYS_PTRACE`, `IPC_LOCK`)
* [x] **ビルドシステム**
  * [x] CMake + Ninja 構成
  * [x] コンパイルオプション (C++23強制, Wall)

## 🔗 Phase 1: コアネットワーク層 (Core Networking)

* [x] **RAII ラッパー (`unique_socket`)**
  * [x] ファイル記述子の自動管理 (close)
  * [x] ムーブセマンティクスの実装
* [x] **io_uring 基盤 (`io_context`)**
  * [x] `io_uring_setup` システムコールのラップ (liburing不使用)
  * [x] リングバッファ (SQ/CQ) の `mmap` 実装
  * [x] SQE取得 (`get_sqe`) と送信 (`submit_request`)
* [x] **イベントループ (`io_context::run`)**
  * [x] `Task` インターフェース (抽象クラス) の定義
  * [x] CQE (完了キュー) のポーリングとコールバック実行処理
* [x] **サーバー基本機能 (`server`)**
  * [x] Socket作成, Bind, Listen (SO_REUSEPORT対応)
  * [x] 非同期 Accept ループの実装

## 📡 Phase 2: セッション管理 & HTTP通信 (Basic HTTP)

* [x] **セッションクラス (`http_session`)**
  * [x] 接続ごとのライフサイクル管理 (簡易実装: `delete this`)
  * [x] 非同期 Read / Write の実装
* [x] **疎通確認**
  * [x] 固定レスポンスによるエコーバック (Hello, io_uring!)
* [x] **【完了】ライフサイクル管理の強化**
  * [x] タイムアウト処理 (接続がアイドル状態の時の切断)
  * [x] HTTP Keep-Alive 対応
* [x] **【完了】エラーハンドリングの強化**
  * [x] `std::expected` の適用 (サーバー初期化処理)

## 🚀 Phase 3: ゼロコピー & メモリ管理 (Zero-Copy Architecture)

要件定義書 3.2に基づき、現在の `std::vector` バッファをカーネル管理バッファへ移行します。

* [ ] **Buffer Ring の基盤実装**
  * [ ] `buffer_pool` クラスの作成
  * [ ] `io_context` での `IORING_OP_PROVIDE_BUFFERS` によるバッファ登録
* [ ] **Buffer Ring の利用**
  * [ ] `http_session` で `IOSQE_BUFFER_SELECT` を使用してデータを受信
  * [ ] 使用済みバッファの返却処理
* [ ] **【Next】HTTPパーサー (State Machine)**
  * [ ] アロケーションなしの解析ロジック
  * [ ] `request` オブジェクトへの `std::string_view` マッピング

## 🛣 Phase 4: ルーティング & API (Routing & API)

* [ ] **ルーターの実装**
  * [ ] パスルーティング (Trie木 または 正規表現)
  * [ ] パスパラメータの抽出 (`/users/:id`)
* [ ] **ユーザーAPIの整備**
  * [ ] `server.get(path, handler)` インターフェース
  * [ ] ハンドラのConcept定義 (`std::invocable`)

## ⚡ Phase 5: マルチスレッド & 最適化 (Thread-per-Core)

* [ ] **マルチスレッド対応**
  * [ ] CPUコア数分のスレッド起動
  * [ ] 各スレッドへの `io_context` 配置 (Shared-nothing)
* [ ] **ベンチマーク & チューニング**
  * [ ] `wrk` / `ab` による負荷テスト
  * [ ] メモリリークチェック (Valgrind / ASan)
  * [ ] `IORING_OP_RECV_MULTISHOT` の組み込み
