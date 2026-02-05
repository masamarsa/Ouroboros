#ifndef MEMBER_BINDER_HPP
#define MEMBER_BINDER_HPP

#include "type_definitions.hpp" // 新しいヘッダーファイルをインクルード

namespace ouroboros::http
{

    /**
     * @brief クラスインスタンスのメンバ関数を handler_function に束縛（バインド）するためのヘルパー関数
     *
     * これにより、開発者は自身のコントローラ風クラスのメソッドをHTTPハンドラとして使用でき、
     * コードの整理がしやすくなります。
     *
     * @tparam F メンバ関数ポインタの型 (例: void (MyClass::*)(const request&, response&))
     * @tparam C クラスインスタンスのポインタの型 (例: MyClass*)
     * @param function メンバ関数へのポインタ
     * @param instance オブジェクトインスタンスへのポインタ
     * @return メンバ関数呼び出しをラップする、handler_function と互換性のある std::function
     */
    template<typename F, typename C>
    auto bind_member(F function, C *instance) {
        return [=](const request &req, response &res)
            {
                (instance->*function)(req, res);
            };
    }

} // namespace ouroboros::http
#endif // MEMBER_BINDER_HPP
