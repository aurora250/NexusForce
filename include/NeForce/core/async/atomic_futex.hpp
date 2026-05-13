#ifndef NEFORCE_CORE_ASYNC_ATOMIC_FUTEX_HPP__
#define NEFORCE_CORE_ASYNC_ATOMIC_FUTEX_HPP__

/**
 * @file atomic_futex.hpp
 * @brief 原子快速用户态互斥锁实现
 *
 * 此文件提供了基于futex的原子快速用户态互斥锁实现。
 * 结合原子操作和futex系统调用，提供高效的等待/通知机制。
 */

#include "NeForce/core/async/atomic.hpp"
#include "NeForce/core/time/clocks.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup AsyncComponents 异步组件
 * @brief 异步编程相关组件
 * @{
 */

/**
 * @defgroup Futex FUTEX
 * @brief FUTEX同步原语的跨平台封装
 * @{
 */

/**
 * @class atomic_futex
 * @brief 原子快速用户态互斥锁类模板
 * @tparam WaiterBit 等待者标志位，默认为0x80000000
 *
 * 基于FUTEX实现的用户态互斥锁，
 * 提供高效的线程同步机制，减少不必要的上下文切换。
 */
template <uint32_t WaiterBit = 0x80000000>
class atomic_futex {
    atomic<uint32_t> data_; ///< 原子数据存储

    /**
     * @brief 加载并测试直到超时（系统时钟）
     * @param assumed 假设的值
     * @param operand 操作数
     * @param equal 是否等待相等
     * @param mo 内存顺序
     * @param has_timeout 是否有超时
     * @param sec 秒数
     * @param ns 纳秒数
     * @return 加载的值
     * @note 使用系统时钟进行超时控制
     */
    uint32_t load_and_test_until(uint32_t assumed, const uint32_t operand, const bool equal, const memory_order mo,
                                 const bool has_timeout, const int64_t sec, const int64_t ns) {
        for (;;) {
            data_.fetch_or(WaiterBit, memory_order_relaxed);
            const bool ret = futex_wait_until(&data_, assumed | WaiterBit, has_timeout, sec, ns, false);
            assumed = load(mo);
            if (!ret || ((operand == assumed) == equal)) {
                return assumed;
            }
        }
    }

    /**
     * @brief 加载并测试直到超时（单调时钟）
     * @param assumed 假设的值
     * @param operand 操作数
     * @param equal 是否等待相等
     * @param mo 内存顺序
     * @param has_timeout 是否有超时
     * @param sec 秒数
     * @param ns 纳秒数
     * @return 加载的值
     * @note 使用单调时钟进行超时控制，不受系统时间调整影响
     */
    uint32_t load_and_test_until_steady(uint32_t assumed, const uint32_t operand, const bool equal,
                                        const memory_order mo, const bool has_timeout, const int64_t sec,
                                        const int64_t ns) {
        for (;;) {
            data_.fetch_or(WaiterBit, memory_order_relaxed);
            const bool ret = futex_wait_until(&data_, assumed | WaiterBit, has_timeout, sec, ns, true);
            assumed = load(mo);
            if (!ret || ((operand == assumed) == equal)) {
                return assumed;
            }
        }
    }

    /**
     * @brief 加载并测试
     * @param assumed 假设的值
     * @param operand 操作数
     * @param equal 是否等待相等
     * @param mo 内存顺序
     * @return 加载的值
     */
    uint32_t load_and_test(const uint32_t assumed, const uint32_t operand, const bool equal, const memory_order mo) {
        return load_and_test_until(assumed, operand, equal, mo, false, 0, 0);
    }

    /**
     * @brief 加载并测试直到超时（系统时钟）
     * @tparam Dur 持续时间类型
     * @param assumed 假设的值
     * @param operand 操作数
     * @param equal 是否等待相等
     * @param mo 内存顺序
     * @param atime 绝对超时时间点
     * @return 加载的值
     */
    template <typename Dur>
    uint32_t load_and_test_until_impl(uint32_t assumed, uint32_t operand, bool equal, memory_order mo,
                                      const time_point<system_clock, Dur>& atime) {
        auto sec = atime.to_sec();
        const auto ns = nanoseconds(atime - sec).count();
        return this->load_and_test_until(assumed, operand, equal, mo, true, sec.since_epoch().count(), ns);
    }

    /**
     * @brief 加载并测试直到超时（单调时钟）
     * @tparam Dur 持续时间类型
     * @param assumed 假设的值
     * @param operand 操作数
     * @param equal 是否等待相等
     * @param mo 内存顺序
     * @param atime 绝对超时时间点
     * @return 加载的值
     */
    template <typename Dur>
    uint32_t load_and_test_until_impl(uint32_t assumed, uint32_t operand, bool equal, memory_order mo,
                                      const time_point<steady_clock, Dur>& atime) {
        auto sec = atime.to_sec();
        const auto ns = nanoseconds(atime - sec).count();
        return this->load_and_test_until_steady(assumed, operand, equal, mo, true, sec.since_epoch().count(), ns);
    }

public:
    /**
     * @brief 构造函数
     * @param data 初始数据值
     */
    explicit atomic_futex(const uint32_t data) :
    data_(data) {}

    /**
     * @brief 原子加载数据
     * @param mo 内存顺序
     * @return 加载的数据值（清除等待者标志位）
     *
     * @note 清除等待者标志位，只返回实际数据
     */
    NEFORCE_NODISCARD NEFORCE_ALWAYS_INLINE uint32_t load(const memory_order mo) const {
        return data_.load(mo) & ~WaiterBit;
    }

    /**
     * @brief 等待直到值不等于指定值
     * @param value 期望不相等的值
     * @param mo 内存顺序
     * @return 当前值（当不等于指定值时）
     *
     * 阻塞当前线程，直到数据值不等于指定的值。
     */
    NEFORCE_ALWAYS_INLINE uint32_t load_when_not_equal(const uint32_t value, const memory_order mo) {
        const uint32_t old = load(mo);
        if ((old & ~WaiterBit) != value) {
            return (old & ~WaiterBit);
        }
        return load_and_test(old, value, false, mo);
    }

    /**
     * @brief 等待直到值等于指定值
     * @param value 期望相等的值
     * @param mo 内存顺序
     *
     * 阻塞当前线程，直到数据值等于指定的值。
     */
    NEFORCE_ALWAYS_INLINE void load_when_equal(const uint32_t value, const memory_order mo) {
        const uint32_t old = load(mo);
        if ((old & ~WaiterBit) == value) {
            return;
        }
        load_and_test(old, value, true, mo);
    }

    /**
     * @brief 在指定时间内等待值等于指定值
     * @tparam Rep 时间表示类型
     * @tparam Period 时间单位比例
     * @param value 期望相等的值
     * @param mo 内存顺序
     * @param rtime 相对超时时间
     * @return 是否在超时前等到相等
     */
    template <typename Rep, typename Period>
    NEFORCE_ALWAYS_INLINE bool load_when_equal_for(const uint32_t value, const memory_order mo,
                                                   const duration<Rep, Period>& rtime) {
        const auto atime = steady_clock::now() + ceil<steady_clock::duration>(rtime);
        return this->load_when_equal_until(value, mo, atime);
    }

    /**
     * @brief 在指定时间点前等待值等于指定值（通用时钟）
     * @tparam Clock 时钟类型
     * @tparam Dur 持续时间类型
     * @param value 期望相等的值
     * @param mo 内存顺序
     * @param atime 绝对超时时间点
     * @return 是否在超时前等到相等
     *
     * @note 将其他时钟转换为单调时钟进行处理
     */
    template <typename Clock, typename Dur>
    NEFORCE_ALWAYS_INLINE bool load_when_equal_until(const uint32_t value, const memory_order mo,
                                                     const time_point<Clock, Dur>& atime) {
        auto now = Clock::now();
        do {
            const auto s_atime = steady_clock::now() + ceil<steady_clock::duration>(atime - now);
            if (this->load_when_equal_until(value, mo, s_atime)) {
                return true;
            }
            now = Clock::now();
        } while (now < atime);
        return false;
    }

    /**
     * @brief 在指定时间点前等待值等于指定值（系统时钟）
     * @tparam Dur 持续时间类型
     * @param value 期望相等的值
     * @param mo 内存顺序
     * @param atime 绝对超时时间点
     * @return 是否在超时前等到相等
     */
    template <typename Dur>
    NEFORCE_ALWAYS_INLINE bool load_when_equal_until(const uint32_t value, const memory_order mo,
                                                     const time_point<system_clock, Dur>& atime) {
        uint32_t old = load(mo);
        if ((old & ~WaiterBit) == value) {
            return true;
        }
        old = this->load_and_test_until_impl(old, value, true, mo, atime);
        return (old & ~WaiterBit) == value;
    }

    /**
     * @brief 在指定时间点前等待值等于指定值（单调时钟）
     * @tparam Dur 持续时间类型
     * @param value 期望相等的值
     * @param mo 内存顺序
     * @param atime 绝对超时时间点（单调时钟）
     * @return 是否在超时前等到相等
     */
    template <typename Dur>
    NEFORCE_ALWAYS_INLINE bool load_when_equal_until(const uint32_t value, const memory_order mo,
                                                     const time_point<steady_clock, Dur>& atime) {
        uint32_t old = load(mo);
        if ((old & ~WaiterBit) == value) {
            return true;
        }
        old = this->load_and_test_until_impl(old, value, true, mo, atime);
        return (old & ~WaiterBit) == value;
    }

    /**
     * @brief 存储新值并通知所有等待线程
     * @param value 要存储的值
     * @param mo 内存顺序
     *
     * 原子地存储新值，如果有等待线程，则唤醒所有等待线程。
     */
    NEFORCE_ALWAYS_INLINE void store_notify_all(const uint32_t value, const memory_order mo) noexcept {
        const auto futex = static_cast<uint32_t*>(static_cast<void*>(&data_));
        if (data_.exchange(value, mo) & WaiterBit) {
            _NEFORCE futex_notify(futex, true);
        }
    }
};

/** @} */ // Futex

/** @} */ // AsyncComponents

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_ATOMIC_FUTEX_HPP__
