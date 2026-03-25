#ifndef NEFORCE_CORE_TIME_CLOCKS_HPP__
#define NEFORCE_CORE_TIME_CLOCKS_HPP__

/**
 * @file clocks.hpp
 * @brief 时钟类型
 *
 * 此文件提供了系统时钟、稳定时钟等时钟类型的定义和实现。
 */

#include "NeForce/core/time/time_point.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Clocks 时钟
 * @brief 时钟类型及相关操作
 * @{
 */

struct NEFORCE_API steady_clock;


/**
 * @struct system_clock
 * @brief 系统时钟
 *
 * 表示系统范围的实时时钟，系统时钟的时间点可以转换为日历时间。
 */
struct NEFORCE_API system_clock {
    using duration = nanoseconds;   ///< 持续时间类型
    using rep = duration::rep;            ///< 数值类型
    using period = duration::period;      ///< 时间单位比例
    using time_point = _NEFORCE time_point<system_clock>; ///< 时间点类型

    static_assert(duration::min() < duration::zero(), "a clock's minimum duration cannot be less than its epoch");

    /**
     * @brief 时钟稳定性标识
     *
     * 系统时钟不是稳定的，可能受系统时间调整影响。
     */
    static constexpr bool is_steady = false;

    /**
     * @brief 获取当前时间点
     * @return 当前系统时间的时间点
     *
     * 返回系统当前时间的时间点表示。
     */
    static time_point now() noexcept;

    /**
     * @brief 将时间点转换为时间值
     * @param time_point_value 要转换的时间点
     * @return 对应的时间值
     */
    static seconds to_seconds(const time_point& time_point_value) noexcept {
        return time_point_value.since_epoch().to_sec();
    }

    /**
     * @brief 从时间值创建时间点
     * @param time_value 要转换的时间值
     * @return 对应的时间点
     */
    static time_point from_seconds(const seconds time_value) noexcept {
        using TP = _NEFORCE time_point<system_clock, seconds>;
        return time_cast<duration>(TP(time_value));
    }
};


/**
 * @struct steady_clock
 * @brief 稳定时钟
 *
 * 表示单调递增的时钟，不受系统时间调整影响。
 * 稳定时钟的时间点仅用于测量时间间隔。
 */
struct NEFORCE_API steady_clock {
    using duration = nanoseconds;   ///< 持续时间类型
    using rep = duration::rep;            ///< 数值类型
    using period = duration::period;      ///< 时间单位比例
    using time_point = _NEFORCE time_point<steady_clock>; ///< 时间点类型

    /**
     * @brief 时钟稳定性标识
     *
     * 稳定时钟是单调递增的，不受系统时间调整影响。
     */
    static constexpr bool is_steady = true;

    /**
     * @brief 获取当前时间点
     * @return 当前稳定时钟的时间点
     */
    static time_point now() noexcept;

    /**
     * @brief 将稳定时钟转为系统时钟
     * @param tp 稳定时钟时间点
     * @return 系统时钟时间点
     */
    template <typename Dur>
    static system_clock::time_point
    to_system(const _NEFORCE time_point<steady_clock, Dur>& tp) {
        const auto steady_now = steady_clock::now();
        const auto sys_now = system_clock::now();

        const auto diff = tp - steady_now;
        return sys_now + _NEFORCE time_cast<system_clock::duration>(diff);
    }
};

/**
 * @brief 将绝对时间戳转换为相对延迟毫秒数
 *
 * @param sec 绝对时间戳的秒部分
 * @param nsec 绝对时间戳的纳秒部分，取值范围为
 * @param is_monotonic 是否使用单调时钟
 * @return 相对延迟时间
*
 * 此函数接收一个绝对时间戳和时钟类型标志，
 * 计算从当前时刻到该时间点之间的时间差，并以毫秒形式返回。
 * 返回的毫秒数会被限制在 0 到 2^32-2 的范围内，
 * 适用于需要有限范围内延迟值的定时器或调度场景。
 *
 * @note 输入支持纳秒精度，但输出仅保留毫秒精度
 */
milliseconds NEFORCE_API relative_time(int64_t sec, int64_t nsec, bool is_monotonic = false) noexcept;


/**
 * @struct is_clock
 * @brief 检查是否为时钟类型
 * @tparam T 要检查的类型
 */
template <typename T>
struct is_clock;

template <>
struct is_clock<system_clock> : true_type {};

template <>
struct is_clock<steady_clock> : true_type {};

/**
 * @var is_clock_v
 * @brief is_clock的便捷变量模板
 */
template <typename T>
NEFORCE_INLINE17 constexpr bool is_clock_v = is_clock<T>::value;

/** @} */ // Clocks

NEFORCE_BEGIN_THIS_THREAD__

/**
 * @defgroup Thread 线程
 * @brief 线程管理和相关操作
 * @{
 */

/**
 * @brief 使当前线程睡眠直到指定时间点
 * @tparam Clock 时钟类型
 * @tparam Dur 持续时间类型
 * @param time 要睡眠到的时间点
 *
 * 跨平台的线程睡眠函数，支持稳定时钟和非稳定时钟。
 *  - 对于稳定时钟，计算一次睡眠时间；
 *  - 对于非稳定时钟，循环检查直到达到目标时间。
 */
template <typename Clock, typename Dur>
void sleep_until(const time_point<Clock, Dur>& time) {
    auto current = Clock::now();
    if (Clock::is_steady) {
        if (current < time) {
            this_thread::sleep_for(time - current);
        }
        return;
    }
    while (current < time) {
        this_thread::sleep_for(time - current);
        current = Clock::now();
    }
}

/** @} */ // Thread

NEFORCE_END_THIS_THREAD__

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_TIME_CLOCKS_HPP__
