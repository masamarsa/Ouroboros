# ターゲット名が実際のファイルやディレクトリ名と被っても実行されるように宣言
.PHONY: all init build run clean

# デフォルトターゲット: make とだけ打った時に実行される順序
all: init build run

# make init: ビルドディレクトリの作成とCMake設定
init:
	cmake -S . -B build -G Ninja

# make build: ビルド実行
build:
	cmake --build build

# make run: 実行
run:
	./build/ouroboros_server

# (おまけ) make clean: ビルド環境を削除してリセットしたい場合に使用
clean:
	rm -rf build
