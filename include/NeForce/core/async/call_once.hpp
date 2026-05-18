#ifndef NEFORCE_CORE_ASYNC_CALL_ONCE_HPP__
#define NEFORCE_CORE_ASYNC_CALL_ONCE_HPP__

/**
 * @file call_once.hpp
 * @brief 单次调用
 *
 * 此文件提供了单次调用的实现，确保某个函数在多个线程中只被执行一次。
 */

#include "NeForce/core/async/atomic_futex.hpp"
#include "NeForce/core/functional/invoke.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup AsyncComponents 异步组件
 * @brief 异步编程相关组件
 * @{
 */

/**
 * @defgroup CallOnce 单次调用
 * @brief 确保函数在多线程环境中只执行一次
 * @{
 */

/**
 * @class once_flag
 * @brief 一次性调用标志类
 *
 * 用于配合call_once函数，确保某个函数在多个线程中只被执行一次。
 * 每个once_flag实例对应一个需要只执行一次的函数。
 *
 * 状态标志：0=未执行，1=执行中，2=已执行
 *
 * @note 不可拷贝、不可移动
 */
class once_flag {
private:
    atomic_futex<> state_{0};

    template <typename Callable, typename... Args>
    friend void call_once(once_flag& flag, Callable&& func, Args&&... args);

public:
    constexpr once_flag() noexcept = default;
    once_flag(const once_flag&) = delete;
    once_flag& operator=(const once_flag&) = delete;
    once_flag(once_flag&&) = delete;
    once_flag& operator=(once_flag&&) = delete;
};

/**
 * @brief 单次调用函数
 * @tparam Callable 可调用类型
 * @tparam Args 参数类型
 * @param flag 一次性调用标志
 * @param func 要执行的函数
 * @param args 函数的参数
 *
 * 确保函数在多线程环境中只被执行一次。
 * 如果已经有线程正在执行该函数，其他线程将等待直到执行完成。
 * 如果函数已成功执行，后续调用将立即返回。
 *
 * @note 如果函数抛出异常，则视为未执行，后续线程将尝试重新执行
 * @warning 对 call_once 递归调用会导致死锁
 */
template <typename Callable, typename... Args>
void call_once(once_flag& flag, Callable&& func, Args&&... args) {
    if (flag.state_.load(memory_order_acquire) == 2) {
        return;
    }

    while (true) {
        const uint32_t state = flag.state_.load(memory_order_acquire);
        if (state == 2) {
            return;
        }

        if (state == 0) {
            uint32_t expected = 0;
            if (flag.state_.compare_exchange_strong(expected, 1, memory_order_acquire, memory_order_relaxed)) {
                try {
                    _NEFORCE invoke<Callable, Args...>(_NEFORCE forward<Callable>(func),
                                                       _NEFORCE forward<Args>(args)...);
                    flag.state_.store_notify_all(2, memory_order_release);
                    return;
                } catch (...) {
                    flag.state_.store_notify_all(0, memory_order_release);
                    throw;
                }
            }
            continue;
        }

        if (flag.state_.load_when_not_equal(1, memory_order_acquire) == 2) {
            return;
        }
    }
}

/** @} */ // CallOnce

/** @} */ // AsyncComponents

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_CALL_ONCE_HPP__
