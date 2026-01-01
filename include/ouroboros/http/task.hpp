#ifndef TASK_HPP
#define TASK_HPP

namespace ouroboros::http
{
    // IO完了時に呼び出される汎用インターフェース
    // io_uring_sqe.user_data にはこのクラスへのポインタを格納する
    struct task
    {
        virtual ~task() = default;
        // result: システムコールの戻り値 (例: recvなら受信バイト数、acceptならFD)
        virtual void complete(int result) = 0;
    };
}

#endif // TASK_HPP
