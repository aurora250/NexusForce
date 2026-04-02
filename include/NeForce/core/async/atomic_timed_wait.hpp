#ifndef NEFORCE_CORE_ASYNC_ATOMIC_TIMED_WAIT_HPP__
#define NEFORCE_CORE_ASYNC_ATOMIC_TIMED_WAIT_HPP__

/**
 * @file atomic_timed_wait.hpp
 * @brief 带超时的原子等待机制
 *
 * 此文件扩展了原子等待功能，支持带超时的等待操作。
 */

#include "NeForce/core/async/atomic_wait.hpp"
#include "NeForce/core/time/clocks.hpp"
NEFORCE_BEGIN_NAMESPACE__

/// @cond
NEFORCE_BEGIN_INNER__

using wait_clock_t = steady_clock;

/**
 * @brief 将任意时钟的时间点转换为等待时钟的时间点
 * @tparam Clock 源时钟类型
 * @tparam Dur 持续时间类型
 * @param time_point 源时间点
 * @return 等待时钟的时间点
 *
 * 将任意时钟的时间点转换为稳定时钟的时间点，使用向上取整确保不提前超时。
 */
template <typename Clock, typename Dur>
wait_clock_t::time_point to_wait_clock(const time_point<Clock, Dur>& time_point) noexcept {
    const typename Clock::time_point clock_entry = Clock::now();
    const wait_clock_t::time_point wait_entry = wait_clock_t::now();
    const auto delta = time_point - clock_entry;
    return wait_entry + ceil<wait_clock_t::duration>(delta);
}

/**
 * @brief 将等待时钟的时间点转换为等待时钟的时间点
 * @tparam Dur 持续时间类型
 * @param time_point 等待时钟的时间点
 * @return 等待时钟的时间点
 *
 * 对于已经是等待时钟的时间点，直接进行向上取整。
 */
template <typename Dur>
wait_clock_t::time_point to_wait_clock(const time_point<wait_clock_t, Dur>& time_point) noexcept {
    return ceil<wait_clock_t::duration>(time_point);
}

/**
 * @brief 平台特定的定时等待实现
 * @tparam Dur 持续时间类型
 * @param addr 等待地址
 * @param old 期望的值
 * @param timeout 超时时间点
 * @return 等待是否成功
 *
 * 定时等待实现，处理超时逻辑和时钟转换。
 */
template <typename Dur>
bool __platform_wait_until_impl(const platform_wait_t* addr, const platform_wait_t old,
                                const time_point<wait_clock_t, Dur>& timeout) noexcept {
    const bool has_timeout = (timeout != wait_clock_t::time_point::max());

    if (!has_timeout) {
        futex_wait(const_cast<void*>(static_cast<const void*>(addr)), old);
        return true;
    }

    const auto now = wait_clock_t::now();
    if (timeout <= now) {
        return false;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    const auto dur = timeout - now;
    const auto sec = time_cast<seconds>(dur);
    const auto ns = time_cast<nanoseconds>(dur - sec);
#else
    const auto sys_timeout = steady_clock::to_system<Dur>(timeout);
    const auto sys_now = system_clock::now();
    if (sys_timeout <= sys_now) {
        return false;
    }
    const auto sys_dur = sys_timeout - sys_now;
    const auto sec = time_cast<seconds>(sys_dur);
    const auto ns = time_cast<nanoseconds>(sys_dur - sec);
#endif

    return _NEFORCE futex_wait_until(const_cast<void*>(static_cast<const void*>(addr)), old, true, sec.count(),
                                     ns.count(), true);
}

template <typename Clock, typename Dur>
enable_if_t<is_same_v<wait_clock_t, Clock>, bool>
__platform_wait_until_dispatch(const platform_wait_t* addr, platform_wait_t old,
                               const time_point<Clock, Dur>& timeout) {
    return inner::__platform_wait_until_impl(addr, old, timeout);
}

template <typename Clock, typename Dur>
enable_if_t<!is_same_v<wait_clock_t, Clock>, bool>
__platform_wait_until_dispatch(const platform_wait_t* addr, platform_wait_t old,
                               const time_point<Clock, Dur>& timeout) {
    if (!inner::__platform_wait_until_impl(addr, old, inner::to_wait_clock(timeout))) {
        if (Clock::now() < timeout) {
            return true;
        }
    }
    return false;
}

NEFORCE_END_INNER__
/// @endcond

/**
 * @defgroup Futex FUTEX
 * @brief FUTEX同步原语的跨平台封装
 * @{
 */

/**
 * @brief FUTEX定时等待函数
 * @tparam Clock 时钟类型
 * @tparam Dur 持续时间类型
 * @param addr 等待地址
 * @param old 期望的值
 * @param timeout 超时时间点
 * @return 等待是否成功
 *
 * 公共的FUTEX定时等待接口，支持任意时钟类型。
 */
template <typename Clock, typename Dur>
bool futex_wait_until(const platform_wait_t* addr, platform_wait_t old, const time_point<Clock, Dur>& timeout) {
    return inner::__platform_wait_until_dispatch(addr, old, timeout);
}

/** @} */ // Futex

/**
 * @defgroup AtomicOperations 原子操作
 * @brief 原子变量的操作
 * @{
 */

/**
 * @struct timed_backoff_spin_policy
 * @brief 带退避策略的定时自旋策略
 *
 * 根据已等待的时间动态调整自旋策略：
 * - 短时间等待：快速自旋
 * - 中等时间等待：让出CPU
 * - 长时间等待：睡眠等待
 */
struct timed_backoff_spin_policy {
    inner::wait_clock_t::time_point deadline;
    inner::wait_clock_t::time_point start_time;

    template <typename Clock, typename Dur>
    timed_backoff_spin_policy(time_point<Clock, Dur> deadline_time = Clock::time_point::max(),
                              time_point<Clock, Dur> start_time_point = Clock::now()) noexcept :
    deadline(inner::to_wait_clock(deadline_time)),
    start_time(inner::to_wait_clock(start_time_point)) {}

    bool operator()() const noexcept {
        const auto now = inner::wait_clock_t::now();
        if (deadline <= now) {
            return false;
        }

        const auto elapsed = now - start_time;
        if (elapsed > 128_ms) {
            this_thread::sleep_for(64_ms);
        } else if (elapsed > 64_us) {
            this_thread::sleep_for(elapsed / 2);
        } else if (elapsed > 4_us) {
            this_thread::yield();
        } else {
            return false;
        }
        return true;
    }
};

/// @cond
NEFORCE_BEGIN_INNER__

/**
 * @struct timed_waiter_pool
 * @brief 定时等待器池
 *
 * 扩展基本的等待器池，添加定时等待功能。
 */
struct timed_waiter_pool : waiter_pool_base {
    /**
     * @brief 执行定时等待
     * @tparam Clock 时钟类型
     * @tparam Dur 持续时间类型
     * @param addr 等待地址
     * @param old 期望的值
     * @param timeout 超时时间点
     * @return 等待是否成功
     */
    template <typename Clock, typename Dur>
    bool do_wait_until(platform_wait_t* addr, platform_wait_t old, const time_point<Clock, Dur>& timeout) {
        return _NEFORCE futex_wait_until(addr, old, timeout);
    }
};

/**
 * @struct timed_waiter
 * @brief 定时等待器模板
 * @tparam EntersWait 是否进入等待状态的标签
 *
 * 支持超时等待的等待器，根据EntersWait标签决定是否更新等待计数。
 */
template <typename EntersWait>
struct timed_waiter : waiter_base<timed_waiter_pool> {
    using base_type = waiter_base<timed_waiter_pool>;

private:
    template <bool Wait = EntersWait::value, enable_if_t<Wait, int> = 0>
    NEFORCE_ALWAYS_INLINE void enter() const noexcept {
        waiter_.waiter_enter_wait();
    }
    template <bool Wait = EntersWait::value, enable_if_t<!Wait, int> = 0>
    NEFORCE_ALWAYS_INLINE void enter() const noexcept {}

    template <bool Wait = EntersWait::value, enable_if_t<Wait, int> = 0>
    NEFORCE_ALWAYS_INLINE void leave() const noexcept {
        waiter_.waiter_leave_wait();
    }
    template <bool Wait = EntersWait::value, enable_if_t<!Wait, int> = 0>
    NEFORCE_ALWAYS_INLINE void leave() const noexcept {}

public:
    /**
     * @brief 构造函数
     * @tparam T 地址类型
     * @param addr 原子变量地址
     */
    template <typename T>
    explicit timed_waiter(const T* addr) noexcept :
    base_type(addr) {
        enter();
    }

    /**
     * @brief 析构函数
     */
    ~timed_waiter() { leave(); }

    /**
     * @brief 执行带值的定时等待
     * @tparam T 值类型
     * @tparam Func 获取当前值的函数类型
     * @tparam Clock 时钟类型
     * @tparam Dur 持续时间类型
     * @param old 期望的值
     * @param func 获取当前值的函数
     * @param timeout 超时时间点
     * @return 是否在超时前条件满足
     */
    template <typename T, typename Func, typename Clock, typename Dur>
    bool waiter_do_wait_until_v(T old, Func func, const time_point<Clock, Dur>& timeout) noexcept {
        platform_wait_t value;
        if (base_type::waiter_do_spin(old, _NEFORCE move(func), value, timed_backoff_spin_policy(timeout))) {
            return true;
        }
        return base_type::waiter_.do_wait_until(base_type::addr_, value, timeout);
    }

    /**
     * @brief 执行带自定义谓词的定时等待
     * @tparam Pred 谓词类型
     * @tparam Clock 时钟类型
     * @tparam Dur 持续时间类型
     * @param pred 等待条件谓词
     * @param value 当前值
     * @param timeout 超时时间点
     * @return 是否在超时前条件满足
     */
    template <typename Pred, typename Clock, typename Dur>
    bool waiter_do_wait_until(Pred pred, platform_wait_t value, const time_point<Clock, Dur>& timeout) noexcept {
        for (auto now = Clock::now(); now < timeout; now = Clock::now()) {
            if (base_type::waiter_.do_wait_until(base_type::addr_, value, timeout) && pred()) {
                return true;
            }
            if (base_type::waiter_do_spin(pred, value, timed_backoff_spin_policy(timeout, now))) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief 执行带自定义谓词的定时等待
     * @tparam Pred 谓词类型
     * @tparam Clock 时钟类型
     * @tparam Dur 持续时间类型
     * @param pred 等待条件谓词
     * @param timeout 超时时间点
     * @return 是否在超时前条件满足
     */
    template <typename Pred, typename Clock, typename Dur>
    bool waiter_do_wait_until(Pred pred, const time_point<Clock, Dur>& timeout) noexcept {
        platform_wait_t value;
        if (this->waiter_do_spin(pred, value, timed_backoff_spin_policy(timeout))) {
            return true;
        }
        return this->waiter_do_wait_until(pred, value, timeout);
    }

    /**
     * @brief 执行带值的定时等待
     * @tparam T 值类型
     * @tparam Func 获取当前值的函数类型
     * @tparam Rep 时间表示类型
     * @tparam Period 时间单位比例
     * @param old 期望的值
     * @param func 获取当前值的函数
     * @param rt 相对超时时间
     * @return 是否在超时前条件满足
     */
    template <typename T, typename Func, typename Rep, typename Period>
    bool waiter_do_wait_for_v(T old, Func func, const duration<Rep, Period>& rt) noexcept {
        platform_wait_t value;
        if (base_type::waiter_do_spin_v(old, _NEFORCE move(func), value)) {
            return true;
        }
        if (!rt.count()) {
            return false;
        }
        auto rtc = ceil<wait_clock_t::duration>(rt);
        return base_type::waiter_.do_wait_until(base_type::addr_, value, steady_clock::now() + rtc);
    }

    /**
     * @brief 执行带自定义谓词的定时等待
     * @tparam Pred 谓词类型
     * @tparam Rep 时间表示类型
     * @tparam Period 时间单位比例
     * @param pred 等待条件谓词
     * @param rt 相对超时时间
     * @return 是否在超时前条件满足
     */
    template <typename Pred, typename Rep, typename Period>
    bool waiter_do_wait_for(Pred pred, const duration<Rep, Period>& rt) noexcept {
        platform_wait_t value;
        if (base_type::waiter_do_spin(pred, value)) {
            return true;
        }
        if (!rt.count()) {
            return false;
        }
        auto rtc = ceil<wait_clock_t::duration>(rt);
        return this->waiter_do_wait_until(pred, value, steady_clock::now() + rtc);
    }
};

/// 进入等待的定时等待器类型
using enters_timed_wait = timed_waiter<true_type>;

/// 裸定时等待器类型，不更新等待计数
using bare_timed_wait = timed_waiter<false_type>;

NEFORCE_END_INNER__
/// @endcond

/**
 * @brief 基于值的原子定时等待（绝对时间）
 * @tparam T 值类型
 * @tparam Func 获取当前值的函数类型
 * @tparam Clock 时钟类型
 * @tparam Dur 持续时间类型
 * @param addr 原子变量地址
 * @param old 期望的值
 * @param func 获取当前值的函数
 * @param timeout 超时时间点
 * @return 是否在超时前条件满足
 *
 * 等待直到addr处的值不等于old或超时。
 */
template <typename T, typename Func, typename Clock, typename Dur>
bool atomic_wait_address_until_v(const T* addr, T&& old, Func&& func, const time_point<Clock, Dur>& timeout) noexcept {
    inner::enters_timed_wait waiter{addr};
    return waiter.waiter_do_wait_until_v(old, func, timeout);
}

/**
 * @brief 基于谓词的原子定时等待（绝对时间）
 * @tparam T 地址类型
 * @tparam Pred 谓词类型
 * @tparam Clock 时钟类型
 * @tparam Dur 持续时间类型
 * @param addr 原子变量地址
 * @param pred 等待条件谓词
 * @param timeout 超时时间点
 * @return 是否在超时前条件满足
 *
 * 等待直到pred()返回true或超时。
 */
template <typename T, typename Pred, typename Clock, typename Dur>
bool atomic_wait_address_until(const T* addr, Pred pred, const time_point<Clock, Dur>& timeout) noexcept {
    inner::enters_timed_wait waiter{addr};
    return waiter.waiter_do_wait_until(pred, timeout);
}

/**
 * @brief 基于谓词的原子定时等待（绝对时间）
 * @tparam Pred 谓词类型
 * @tparam Clock 时钟类型
 * @tparam Dur 持续时间类型
 * @param addr 平台等待类型地址
 * @param pred 等待条件谓词
 * @param timeout 超时时间点
 * @return 是否在超时前条件满足
 *
 * 针对平台等待类型的特化版本，使用裸等待器。
 */
template <typename Pred, typename Clock, typename Dur>
bool atomic_wait_address_until(const platform_wait_t* addr, Pred pred, const time_point<Clock, Dur>& timeout) noexcept {
    inner::bare_timed_wait waiter{addr};
    return waiter.waiter_do_wait_until(pred, timeout);
}

/**
 * @brief 基于值的原子定时等待（相对时间）
 * @tparam T 值类型
 * @tparam Func 获取当前值的函数类型
 * @tparam Rep 时间表示类型
 * @tparam Period 时间单位比例
 * @param addr 原子变量地址
 * @param old 期望的值
 * @param func 获取当前值的函数
 * @param rt 相对超时时间
 * @return 是否在超时前条件满足
 *
 * 等待直到addr处的值不等于old或经过指定时间。
 */
template <typename T, typename Func, typename Rep, typename Period>
bool atomic_wait_address_for_v(const T* addr, T&& old, Func&& func, const duration<Rep, Period>& rt) noexcept {
    inner::enters_timed_wait waiter{addr};
    return waiter.waiter_do_wait_for_v(old, func, rt);
}

/**
 * @brief 基于谓词的原子定时等待（相对时间）
 * @tparam T 地址类型
 * @tparam Pred 谓词类型
 * @tparam Rep 时间表示类型
 * @tparam Period 时间单位比例
 * @param addr 原子变量地址
 * @param pred 等待条件谓词
 * @param rt 相对超时时间
 * @return 是否在超时前条件满足
 *
 * 等待直到pred()返回true或经过指定时间。
 */
template <typename T, typename Pred, typename Rep, typename Period>
bool atomic_wait_address_for(const T* addr, Pred pred, const duration<Rep, Period>& rt) noexcept {
    inner::enters_timed_wait waiter{addr};
    return waiter.waiter_do_wait_for(pred, rt);
}

/**
 * @brief 基于谓词的原子定时等待（相对时间）
 * @tparam Pred 谓词类型
 * @tparam Rep 时间表示类型
 * @tparam Period 时间单位比例
 * @param addr 平台等待类型地址
 * @param pred 等待条件谓词
 * @param rt 相对超时时间
 * @return 是否在超时前条件满足
 *
 * 针对平台等待类型的特化版本，使用裸等待器。
 */
template <typename Pred, typename Rep, typename Period>
bool atomic_wait_address_for(const platform_wait_t* addr, Pred pred, const duration<Rep, Period>& rt) noexcept {
    inner::bare_timed_wait waiter{addr};
    return waiter.waiter_do_wait_for(pred, rt);
}

/** @} */ // AtomicOperations

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_ATOMIC_TIMED_WAIT_HPP__
