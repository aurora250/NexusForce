#ifndef NEFORCE_CORE_ASYNC_THREAD_EXIT_NOTIFIER_HPP__
#define NEFORCE_CORE_ASYNC_THREAD_EXIT_NOTIFIER_HPP__
#include "NeForce/core/async/mutex.hpp"
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
    using callback_t = void (*)(void*); ///< 回调函数类型
    callback_t callback_;               ///< 线程退出时的回调函数
    void* user_data_;                   ///< 回调函数的用户数据
    thread_exit_listener* next_;        ///< 监听器链表下一节点
    void* chain_;                       ///< 所属 thread_exit_notifier 指针
};

/**
 * @class thread_exit_notifier
 * @brief 线程退出通知器
 *
 * 利用 thread_local 对象的析构函数在线程退出时通知所有注册的监听器。
 * 每个线程拥有独立的 thread_exit_notifier 实例。
 */
class thread_exit_notifier {
private:
    thread_exit_listener* tail_{nullptr};

private:
    thread_exit_notifier() noexcept = default;

    ~thread_exit_notifier() {
        lock<mutex> guard(mutex_inst());
        for (auto* ptr = tail_; ptr != nullptr; ptr = ptr->next_) {
            ptr->chain_ = nullptr;
            ptr->callback_(ptr->user_data_);
        }
    }

    static thread_exit_notifier& instance() noexcept {
        thread_local thread_exit_notifier notifier;
        return notifier;
    }

    static mutex& mutex_inst() {
        static mutex mtx;
        return mtx;
    }

public:
    thread_exit_notifier(const thread_exit_notifier&) = delete;
    thread_exit_notifier& operator=(const thread_exit_notifier&) = delete;

    /**
     * @brief 注册线程退出监听器
     * @param listener 要注册的监听器
     */
    static void subscribe(thread_exit_listener* listener) {
        auto& tls = instance();
        lock<mutex> guard(mutex_inst());
        listener->next_ = tls.tail_;
        listener->chain_ = &tls;
        tls.tail_ = listener;
    }

    /**
     * @brief 注销线程退出监听器
     * @param listener 要注销的监听器
     */
    static void unsubscribe(thread_exit_listener* listener) {
        lock<mutex> guard(mutex_inst());
        if (listener->chain_ == nullptr) {
            return;
        }
        auto& tls = *static_cast<thread_exit_notifier*>(listener->chain_);
        listener->chain_ = nullptr;
        thread_exit_listener** prev = &tls.tail_;
        for (auto* ptr = tls.tail_; ptr != nullptr; ptr = ptr->next_) {
            if (ptr == listener) {
                *prev = ptr->next_;
                break;
            }
            prev = &ptr->next_;
        }
    }
};

/** @} */ // ThreadExit

/** @} */ // AsyncComponents

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_THREAD_EXIT_NOTIFIER_HPP__
