#ifndef NEFORCE_CORE_ASYNC_THREAD_HOOK_HPP__
#define NEFORCE_CORE_ASYNC_THREAD_HOOK_HPP__
#include "NeForce/core/config/c++config.hpp"
NEFORCE_BEGIN_NAMESPACE__

class NEFORCE_API thread;


struct NEFORCE_API thread_hook {
    enum class point {
        before_create,   // 线程创建前（启动线程的上下文中）
        after_create,    // 线程创建成功后（启动线程的上下文中）
        thread_start,    // 新线程开始执行用户代码前（新线程上下文中）
        thread_end,      // 用户代码执行完毕后（新线程上下文中）
        before_destroy   // 线程对象析构前（析构线程的上下文中）
    };

    using callback_t = void(*)(point point, const thread* th);

    static void add_hook(callback_t hook);
    static void remove_hook(callback_t hook);
    static void invoke(point point, const thread* th = nullptr);
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_THREAD_HOOK_HPP__
