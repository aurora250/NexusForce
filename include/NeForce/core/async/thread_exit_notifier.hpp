#ifndef NEFORCE_CORE_ASYNC_THREAD_EXIT_NOTIFIER_HPP__
#define NEFORCE_CORE_ASYNC_THREAD_EXIT_NOTIFIER_HPP__
#include "NeForce/core/async/thread_exit_register.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup AsyncComponents 异步组件
 * @brief 异步编程相关组件
 * @{
 */

/**
 * @defgroup ThreadExit 线程退出回调
 * @brief 线程退出时的回调管理
 * @{
 */

/**
 * @struct thread_exit_listener
 * @brief 线程退出监听器
 *
 * 用于在线程退出时自动清理该线程的隐式生产者。
 */
struct thread_exit_listener {
    thread_exit_callback_t callback; ///< 线程退出时的回调函数
    void* user_data;                 ///< 回调函数的用户数据
    thread_exit_listener* next;      ///< 监听器链表下一节点
    void* chain;                     ///< 所属 thread_exit_notifier 指针
};

/**
 * @class thread_exit_notifier
 * @brief 线程退出通知器
 *
 * 利用 thread_local 对象的析构函数在线程退出时通知所有注册的监听器。
 * 每个线程拥有独立的 thread_exit_notifier 实例。
 */
class NEFORCE_API thread_exit_notifier {
private:
    thread_exit_listener* tail_{nullptr};

private:
    thread_exit_notifier() noexcept = default;
    ~thread_exit_notifier();

    static thread_exit_notifier& instance() noexcept;

public:
    thread_exit_notifier(const thread_exit_notifier&) = delete;
    thread_exit_notifier& operator=(const thread_exit_notifier&) = delete;

    /**
     * @brief 注册线程退出监听器
     * @param listener 要注册的监听器
     */
    static void subscribe(thread_exit_listener* listener);

    /**
     * @brief 注销线程退出监听器
     * @param listener 要注销的监听器
     */
    static void unsubscribe(thread_exit_listener* listener);
};

/** @} */ // ThreadExit

/** @} */ // AsyncComponents

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_THREAD_EXIT_NOTIFIER_HPP__
