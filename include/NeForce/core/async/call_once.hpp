#ifndef NEFORCE_CORE_ASYNC_CALL_ONCE_HPP__
#define NEFORCE_CORE_ASYNC_CALL_ONCE_HPP__

/**
 * @file call_once.hpp
 * @brief 单次调用
 *
 * 此文件提供了单次调用的实现，确保某个函数在多个线程中只被执行一次。
 */

#include "NeForce/core/async/atomic.hpp"
#include "NeForce/core/functional/invoke.hpp"
NEFORCE_BEGIN_NAMESPACE__

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
    atomic<uint32_t> state_{0};

    template <typename Callable, typename... Args>
    friend void call_once(once_flag& flag, Callable&& func, Args&&... args);

public:
    once_flag() noexcept = default;
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
 */
template <typename Callable, typename... Args> void call_once(once_flag& flag, Callable&& func, Args&&... args) {
    if (flag.state_.load(memory_order_acquire) == 2) {
        return;
    }

    uint32_t spin_count = 0;
    while (true) {
        const uint32_t state = flag.state_.load(memory_order_acquire);
        if (state == 2) {
            return;
        }

        if (state == 0) {
            uint32_t expected = 0;
            if (flag.state_.compare_exchange_strong(expected, 1, memory_order_acq_rel, memory_order_relaxed)) {
                try {
                    _NEFORCE invoke<Callable, Args...>(_NEFORCE forward<Callable>(func),
                                                       _NEFORCE forward<Args>(args)...);
                    flag.state_.store(2, memory_order_release);
                    return;
                } catch (...) {
                    flag.state_.store(0, memory_order_release);
                    throw;
                }
            }
            spin_count = 0;
            continue;
        }

        if (spin_count < 10) {
            for (uint32_t i = 0; i < (1u << spin_count); ++i) {
                this_thread::relax();
            }
            ++spin_count;
        } else {
            this_thread::yield();
        }
    }
}

/** @} */ // CallOnce

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_CALL_ONCE_HPP__
