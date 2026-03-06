#ifndef NEFORCE_CORE_ASYNC_LATCH_HPP__
#define NEFORCE_CORE_ASYNC_LATCH_HPP__

/**
 * @file latch.hpp
 * @brief 闩锁实现
 *
 * 此文件提供了闩锁的实现，用于线程同步。
 * 闩锁是一种线程协调机制，允许一个或多个线程等待，直到计数器减为零。
 */

#include "atomic_base.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Latches 闩锁
 * @brief 线程同步闩锁原语
 * @{
 */

/**
 * @class latch
 * @brief 闩锁类
 *
 * 闩锁是一种同步原语，用于协调多个线程的执行。
 * 线程可以等待闩锁的计数器减为零，或者减少计数器并等待。
 * 闩锁是一次性的，计数器减为零后不能被重置。
 */
class latch {
private:
    alignas(alignof(platform_wait_t)) platform_wait_t counter_;   ///< 计数器值

public:
    /**
     * @brief 获取闩锁的最大计数值
     * @return 最大可能的计数值
     */
    static constexpr platform_wait_t max() noexcept {
        return numeric_traits<platform_wait_t>::max();
    }

    /**
     * @brief 构造函数
     * @param expected 初始计数值
     *
     * 创建闩锁并设置初始计数值。计数值必须为非负。
     */
    constexpr explicit latch(const platform_wait_t expected) noexcept
    : counter_(expected) {
        NEFORCE_CONSTEXPR_ASSERT(expected >= 0);
    }

    ~latch() = default;  ///< 析构函数
    latch(const latch&) = delete;  ///< 禁止拷贝构造
    latch& operator =(const latch&) = delete;  ///< 禁止拷贝赋值

    /**
     * @brief 减少计数器
     * @param update 减少的数量，默认为1
     *
     * 原子地减少闩锁的计数器。如果计数器减为零，则通知所有等待线程。
     *
     * @note 如果计数器变为负值，行为未定义
     */
    NEFORCE_ALWAYS_INLINE void count_down(const platform_wait_t update = 1) {
        auto const old_value = _NEFORCE atomic_fetch_sub(&counter_, update, memory_order_release);
        if (old_value == update) {
	        _NEFORCE atomic_notify_address(&counter_, true);
        }
    }

    /**
     * @brief 尝试等待
     * @return 计数器是否为零
     *
     * 非阻塞地检查闩锁的计数器是否为零。
     */
    NEFORCE_ALWAYS_INLINE bool try_wait() const noexcept {
        return _NEFORCE atomic_load(&counter_, memory_order_acquire) == 0;
    }

    /**
     * @brief 等待计数器为零
     *
     * 阻塞当前线程，直到闩锁的计数器减为零。
     *
     * @note 如果计数器已经为零，则立即返回
     */
    NEFORCE_ALWAYS_INLINE void wait() const noexcept {
        auto const predicate = [this] { return this->try_wait(); };
        _NEFORCE atomic_wait_address(&counter_, predicate);
    }

    /**
     * @brief 减少计数器并等待
     * @param update 减少的数量，默认为1
     *
     * 原子地减少计数器，然后等待计数器变为零。
     * 这是一个组合操作，等价于先调用count_down()再调用wait()。
     *
     * @note 如果计数器变为负值，行为未定义
     */
    NEFORCE_ALWAYS_INLINE void arrive_and_wait(const platform_wait_t update = 1) noexcept {
        count_down(update);
        wait();
    }
};

/** @} */ // Latches

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_LATCH_HPP__
