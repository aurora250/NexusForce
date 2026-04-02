#ifndef NEFORCE_CORE_TIME_TIME_POINT_HPP__
#define NEFORCE_CORE_TIME_TIME_POINT_HPP__

/**
 * @file time_point.hpp
 * @brief 时间点类型
 *
 * 此文件提供了时间点类型及相关操作，用于表示特定时钟的时间点。
 */

#include "NeForce/core/time/duration.hpp"
NEFORCE_BEGIN_NAMESPACE__

template <typename Clock, typename Dur = typename Clock::duration> struct time_point;

/// @cond
NEFORCE_BEGIN_INNER__

/**
 * @struct __timepoint_common_type
 * @brief 时间点公共类型计算辅助模板
 * @tparam CommonT 公共持续时间类型
 * @tparam Clock 时钟类型
 * @tparam Dummy SFINAE参数
 *
 * 计算两个时间点类型的公共类型。
 */
template <typename CommonT, typename Clock, typename Dummy = void> struct __timepoint_common_type {};

template <typename CommonT, typename Clock>
struct __timepoint_common_type<CommonT, Clock, void_t<typename CommonT::type>> {
    using type = time_point<Clock, typename CommonT::type>;
};

NEFORCE_END_INNER__
/// @endcond

template <typename Clock, typename Dur1, typename Dur2>
struct common_type<time_point<Clock, Dur1>, time_point<Clock, Dur2>>
: inner::__timepoint_common_type<common_type<Dur1, Dur2>, Clock> {};

template <typename Clock, typename Dur> struct common_type<time_point<Clock, Dur>, time_point<Clock, Dur>> {
    using type = time_point<Clock, Dur>;
};

template <typename Clock, typename Dur> struct common_type<time_point<Clock, Dur>> {
    using type = time_point<Clock, Dur>;
};

/**
 * @defgroup TimePoint 时间点
 * @brief 时间点类型及相关操作
 * @{
 */

/**
 * @brief 时间点类型转换
 * @tparam ToDur 目标持续时间类型
 * @tparam Clock 时钟类型
 * @tparam Dur 源持续时间类型
 * @param time_point_value 源时间点
 * @return 转换后的时间点
 *
 * 将一种持续时间类型的时间点转换为另一种持续时间类型的时间点。
 */
template <typename ToDur, typename Clock, typename Dur, enable_if_t<is_duration_v<ToDur>, int> = 0>
constexpr time_point<Clock, ToDur> time_cast(const time_point<Clock, Dur>& time_point_value) {
    return time_point<Clock, ToDur>(_NEFORCE time_cast<ToDur>(time_point_value.since_epoch()));
}

/**
 * @class time_point
 * @brief 时间点类模板
 * @tparam Clock 时钟类型
 * @tparam Dur 持续时间类型
 *
 * 表示特定时钟的一个时间点，支持时间算术运算和比较。
 */
template <typename Clock, typename Dur> struct time_point {
    static_assert(is_duration_v<Dur>, "duration must be a specialization of duration");

    using clock_type = Clock;                      ///< 时钟类型
    using duration_type = Dur;                     ///< 持续时间类型
    using rep = typename duration_type::rep;       ///< 数值类型
    using period = typename duration_type::period; ///< 时间单位比例

private:
    duration_type value_; ///< 持续时间

public:
    /**
     * @brief 默认构造函数
     */
    constexpr time_point() :
    value_(duration_type::zero()) {}

    /**
     * @brief 从持续时间构造
     * @param dur 自纪元以来的持续时间
     */
    constexpr explicit time_point(const duration_type& dur) :
    value_(dur) {}

    /**
     * @brief 从其他时间点构造
     * @tparam Dur2 源持续时间类型
     * @param value 源时间点
     */
    template <typename Dur2, typename = enable_if_t<is_convertible_v<Dur2, duration_type>>>
    constexpr time_point(const time_point<clock_type, Dur2>& value) :
    value_(value.value_) {}


    /**
     * @brief 前置自增运算符
     * @return 自增后的时间点引用
     */
    constexpr time_point& operator++() {
        ++value_;
        return *this;
    }

    /**
     * @brief 后置自增运算符
     * @return 自增前的时间点
     */
    constexpr time_point operator++(int) { return time_point{value_++}; }

    /**
     * @brief 前置自减运算符
     * @return 自减后的时间点引用
     */
    constexpr time_point& operator--() {
        --value_;
        return *this;
    }

    /**
     * @brief 后置自减运算符
     * @return 自减前的时间点
     */
    constexpr time_point operator--(int) { return time_point{value_--}; }


    /**
     * @brief 加法赋值运算符
     * @param dur 要加的持续时间
     * @return 当前对象的引用
     */
    constexpr time_point& operator+=(const duration_type& dur) {
        value_ += dur;
        return *this;
    }

    /**
     * @brief 减法赋值运算符
     * @param dur 要减的持续时间
     * @return 当前对象的引用
     */
    constexpr time_point& operator-=(const duration_type& dur) {
        value_ -= dur;
        return *this;
    }

    /**
     * @brief 获取自纪元以来的时间
     * @return 自纪元以来的持续时间
     */
    constexpr duration_type since_epoch() const noexcept { return value_; }

    /**
     * @brief 获取最小时间点
     * @return 最小时间点
     */
    static constexpr time_point min() noexcept { return time_point(duration_type::min()); }

    /**
     * @brief 获取最大时间点
     * @return 最大时间点
     */
    static constexpr time_point max() noexcept { return time_point(duration_type::max()); }

    NEFORCE_ALWAYS_INLINE time_point<Clock, nanoseconds> to_nano() const {
        return _NEFORCE time_cast<nanoseconds>(*this);
    }
    NEFORCE_ALWAYS_INLINE time_point<Clock, microseconds> to_micro() const {
        return _NEFORCE time_cast<microseconds>(*this);
    }
    NEFORCE_ALWAYS_INLINE time_point<Clock, milliseconds> to_milli() const {
        return _NEFORCE time_cast<milliseconds>(*this);
    }
    NEFORCE_ALWAYS_INLINE time_point<Clock, seconds> to_sec() const { return _NEFORCE time_cast<seconds>(*this); }
    NEFORCE_ALWAYS_INLINE time_point<Clock, minutes> to_minu() const { return _NEFORCE time_cast<minutes>(*this); }
    NEFORCE_ALWAYS_INLINE time_point<Clock, hours> to_hour() const { return _NEFORCE time_cast<hours>(*this); }
    NEFORCE_ALWAYS_INLINE time_point<Clock, days> to_day() const { return _NEFORCE time_cast<days>(*this); }
    NEFORCE_ALWAYS_INLINE time_point<Clock, weeks> to_week() const { return _NEFORCE time_cast<weeks>(*this); }
    NEFORCE_ALWAYS_INLINE time_point<Clock, years> to_year() const { return _NEFORCE time_cast<years>(*this); }
    NEFORCE_ALWAYS_INLINE time_point<Clock, months> to_month() const { return _NEFORCE time_cast<months>(*this); }
};

template <typename Rep, typename Period>
template <typename Clock, typename Dur, typename Dummy>
constexpr duration<Rep, Period>::duration(time_point<Clock, Dur> tp) :
duration(_NEFORCE time_cast<Dur>(tp.since_epoch())) {}

/**
 * @brief 加法运算符（时间点 + 持续时间）
 * @tparam Clock 时钟类型
 * @tparam Dur1 时间点持续时间类型
 * @tparam Rep2 持续时间数值类型
 * @tparam Period2 持续时间时间单位比例
 * @param lhs 时间点
 * @param rhs 持续时间
 * @return 加上持续时间后的时间点
 */
template <typename Clock, typename Dur1, typename Rep2, typename Period2>
constexpr time_point<Clock, common_type_t<Dur1, duration<Rep2, Period2>>>
operator+(const time_point<Clock, Dur1>& lhs, const duration<Rep2, Period2>& rhs) {
    using duration2 = duration<Rep2, Period2>;
    using common_duration = common_type_t<Dur1, duration2>;
    using result_time_point = time_point<Clock, common_duration>;
    return result_time_point(lhs.since_epoch() + rhs);
}

/**
 * @brief 加法运算符（持续时间 + 时间点）
 * @tparam Rep1 持续时间数值类型
 * @tparam Period1 持续时间时间单位比例
 * @tparam Clock 时钟类型
 * @tparam Dur2 时间点持续时间类型
 * @param lhs 持续时间
 * @param rhs 时间点
 * @return 持续时间加上时间点后的时间点
 */
template <typename Rep1, typename Period1, typename Clock, typename Dur2>
constexpr time_point<Clock, common_type_t<duration<Rep1, Period1>, Dur2>>
operator+(const duration<Rep1, Period1>& lhs, const time_point<Clock, Dur2>& rhs) {
    using duration1 = duration<Rep1, Period1>;
    using common_duration = common_type_t<duration1, Dur2>;
    using result_time_point = time_point<Clock, common_duration>;
    return result_time_point(rhs.since_epoch() + lhs);
}

/**
 * @brief 减法运算符（时间点 - 持续时间）
 * @tparam Clock 时钟类型
 * @tparam Dur1 时间点持续时间类型
 * @tparam Rep2 持续时间数值类型
 * @tparam Period2 持续时间时间单位比例
 * @param lhs 时间点
 * @param rhs 持续时间
 * @return 减去持续时间后的时间点
 */
template <typename Clock, typename Dur1, typename Rep2, typename Period2>
constexpr time_point<Clock, common_type_t<Dur1, duration<Rep2, Period2>>>
operator-(const time_point<Clock, Dur1>& lhs, const duration<Rep2, Period2>& rhs) {
    using duration2 = duration<Rep2, Period2>;
    using common_duration = common_type_t<Dur1, duration2>;
    using result_time_point = time_point<Clock, common_duration>;
    return result_time_point(lhs.since_epoch() - rhs);
}

/**
 * @brief 减法运算符（时间点 - 时间点）
 * @tparam Clock 时钟类型
 * @tparam Dur1 第一个时间点持续时间类型
 * @tparam Dur2 第二个时间点持续时间类型
 * @param lhs 左时间点
 * @param rhs 右时间点
 * @return 两个时间点之间的时间间隔
 */
template <typename Clock, typename Dur1, typename Dur2>
constexpr common_type_t<Dur1, Dur2> operator-(const time_point<Clock, Dur1>& lhs, const time_point<Clock, Dur2>& rhs) {
    return lhs.since_epoch() - rhs.since_epoch();
}

/**
 * @brief 等于比较运算符
 * @tparam Clock 时钟类型
 * @tparam Dur1 第一个时间点持续时间类型
 * @tparam Dur2 第二个时间点持续时间类型
 * @param lhs 左时间点
 * @param rhs 右时间点
 * @return 两个时间点是否相等
 */
template <typename Clock, typename Dur1, typename Dur2>
constexpr bool operator==(const time_point<Clock, Dur1>& lhs, const time_point<Clock, Dur2>& rhs) {
    return lhs.since_epoch() == rhs.since_epoch();
}

/**
 * @brief 不等于比较运算符
 * @tparam Clock 时钟类型
 * @tparam Dur1 第一个时间点持续时间类型
 * @tparam Dur2 第二个时间点持续时间类型
 * @param lhs 左时间点
 * @param rhs 右时间点
 * @return 两个时间点是否不相等
 */
template <typename Clock, typename Dur1, typename Dur2>
constexpr bool operator!=(const time_point<Clock, Dur1>& lhs, const time_point<Clock, Dur2>& rhs) {
    return !(lhs == rhs);
}

/**
 * @brief 小于比较运算符
 * @tparam Clock 时钟类型
 * @tparam Dur1 第一个时间点持续时间类型
 * @tparam Dur2 第二个时间点持续时间类型
 * @param lhs 左时间点
 * @param rhs 右时间点
 * @return 左时间点是否早于右时间点
 */
template <typename Clock, typename Dur1, typename Dur2>
constexpr bool operator<(const time_point<Clock, Dur1>& lhs, const time_point<Clock, Dur2>& rhs) {
    return lhs.since_epoch() < rhs.since_epoch();
}

/**
 * @brief 小于等于比较运算符
 * @tparam Clock 时钟类型
 * @tparam Dur1 第一个时间点持续时间类型
 * @tparam Dur2 第二个时间点持续时间类型
 * @param lhs 左时间点
 * @param rhs 右时间点
 * @return 左时间点是否不晚于右时间点
 */
template <typename Clock, typename Dur1, typename Dur2>
constexpr bool operator<=(const time_point<Clock, Dur1>& lhs, const time_point<Clock, Dur2>& rhs) {
    return !(rhs < lhs);
}

/**
 * @brief 大于比较运算符
 * @tparam Clock 时钟类型
 * @tparam Dur1 第一个时间点持续时间类型
 * @tparam Dur2 第二个时间点持续时间类型
 * @param lhs 左时间点
 * @param rhs 右时间点
 * @return 左时间点是否晚于右时间点
 */
template <typename Clock, typename Dur1, typename Dur2>
constexpr bool operator>(const time_point<Clock, Dur1>& lhs, const time_point<Clock, Dur2>& rhs) {
    return rhs < lhs;
}

/**
 * @brief 大于等于比较运算符
 * @tparam Clock 时钟类型
 * @tparam Dur1 第一个时间点持续时间类型
 * @tparam Dur2 第二个时间点持续时间类型
 * @param lhs 左时间点
 * @param rhs 右时间点
 * @return 左时间点是否不早于右时间点
 */
template <typename Clock, typename Dur1, typename Dur2>
constexpr bool operator>=(const time_point<Clock, Dur1>& lhs, const time_point<Clock, Dur2>& rhs) {
    return !(lhs < rhs);
}

/** @} */ // TimePoint

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_TIME_TIME_POINT_HPP__
