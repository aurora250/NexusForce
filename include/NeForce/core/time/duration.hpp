#ifndef NEFORCE_CORE_TIME_DURATION_HPP__
#define NEFORCE_CORE_TIME_DURATION_HPP__

/**
 * @file duration.hpp
 * @brief 持续时间类型
 *
 * 此文件提供了持续时间类型及相关操作，支持不同时间单位的表示和转换。
 */

#include "NeForce/core/numeric/math.hpp"
#include "NeForce/core/numeric/ratio.hpp"
NEFORCE_BEGIN_NAMESPACE__

template <typename Rep, typename Period = ratio<1>> struct duration;

template <typename Clock, typename Dur> struct time_point;

/// @cond
NEFORCE_BEGIN_INNER__

/**
 * @struct __duration_common_type
 * @brief 持续时间公共类型计算辅助模板
 * @tparam CommonT 公共数值类型
 * @tparam Period1 第一个持续时间单位比例
 * @tparam Period2 第二个持续时间单位比例
 * @tparam Dummy SFINAE参数
 *
 * 计算两个持续时间类型的公共类型。
 */
template <typename CommonT, typename Period1, typename Period2, typename Dummy = void> struct __duration_common_type {};

template <typename CommonT, typename Period1, typename Period2>
struct __duration_common_type<CommonT, Period1, Period2, void_t<typename CommonT::type>> {
private:
    using gcd_numerator = static_gcd<Period1::num, Period2::num>;
    using gcd_denominator = static_gcd<Period1::den, Period2::den>;
    using common_rep = typename CommonT::type;
    using result_ratio = ratio<gcd_numerator::value, (Period1::den / gcd_denominator::value) * Period2::den>;

public:
    using type = duration<common_rep, typename result_ratio::type>;
};

NEFORCE_END_INNER__
/// @endcond

template <typename Rep1, typename Period1, typename Rep2, typename Period2>
struct common_type<duration<Rep1, Period1>, duration<Rep2, Period2>>
: inner::__duration_common_type<common_type<Rep1, Rep2>, typename Period1::type, typename Period2::type> {};

template <typename Rep, typename Period> struct common_type<duration<Rep, Period>, duration<Rep, Period>> {
    using type = duration<common_type_t<Rep>, typename Period::type>;
};

template <typename Rep, typename Period> struct common_type<duration<Rep, Period>> {
    using type = duration<common_type_t<Rep>, typename Period::type>;
};

/// @cond
NEFORCE_BEGIN_INNER__

template <typename ToDur, typename ConvFactor, typename CommonRep, bool NumIsOne = false, bool DenIsOne = false>
struct __duration_cast_impl {
    template <typename Rep, typename Period> static constexpr ToDur __cast(const duration<Rep, Period>& value) {
        return ToDur(static_cast<typename ToDur::rep>(static_cast<CommonRep>(value.count()) *
                                                      static_cast<CommonRep>(ConvFactor::num) /
                                                      static_cast<CommonRep>(ConvFactor::den)));
    }
};

template <typename ToDur, typename ConvFactor, typename CommonRep>
struct __duration_cast_impl<ToDur, ConvFactor, CommonRep, true, true> {
    template <typename Rep, typename Period> static constexpr ToDur __cast(const duration<Rep, Period>& value) {
        return ToDur(static_cast<typename ToDur::rep>(value.count()));
    }
};

template <typename ToDur, typename ConvFactor, typename CommonRep>
struct __duration_cast_impl<ToDur, ConvFactor, CommonRep, true, false> {
    template <typename Rep, typename Period> static constexpr ToDur __cast(const duration<Rep, Period>& value) {
        return ToDur(static_cast<typename ToDur::rep>(static_cast<CommonRep>(value.count()) /
                                                      static_cast<CommonRep>(ConvFactor::den)));
    }
};

template <typename ToDur, typename ConvFactor, typename CommonRep>
struct __duration_cast_impl<ToDur, ConvFactor, CommonRep, false, true> {
    template <typename Rep, typename Period> static constexpr ToDur __cast(const duration<Rep, Period>& value) {
        return ToDur(static_cast<typename ToDur::rep>(static_cast<CommonRep>(value.count()) *
                                                      static_cast<CommonRep>(ConvFactor::num)));
    }
};

NEFORCE_END_INNER__
/// @endcond

/**
 * @defgroup Duration 持续时间
 * @brief 持续时间类型及其辅助函数
 * @{
 */

/**
 * @struct is_duration
 * @brief 检查是否为持续时间类型
 * @tparam T 要检查的类型
 */
template <typename T> struct is_duration : false_type {};

template <typename Rep, typename Period> struct is_duration<duration<Rep, Period>> : true_type {};

/**
 * @var is_duration_v
 * @brief is_duration的便捷变量模板
 */
template <typename T> NEFORCE_INLINE17 constexpr bool is_duration_v = is_duration<T>::value;


/**
 * @brief 持续时间类型转换
 * @tparam ToDur 目标持续时间类型
 * @tparam Rep 源数值类型
 * @tparam Period 源时间单位比例
 * @param value 源持续时间
 * @return 转换后的持续时间
 *
 * 将一种持续时间类型转换为另一种，考虑时间单位的转换。
 */
template <typename ToDur, typename Rep, typename Period, enable_if_t<is_duration<ToDur>::value, int> = 0>
constexpr ToDur time_cast(const duration<Rep, Period>& value) {
    using to_period = typename ToDur::period;
    using to_rep = typename ToDur::rep;
    using conversion_factor = ratio_divide<Period, to_period>;
    using common_rep = common_type_t<to_rep, Rep, intmax_t>;
    using duration_caster = inner::__duration_cast_impl<ToDur, conversion_factor, common_rep,
                                                        conversion_factor::num == 1, conversion_factor::den == 1>;
    return duration_caster::__cast(value);
}


using nanoseconds = duration<int64_t, nano>;      ///< 纳秒持续时间
using microseconds = duration<int64_t, micro>;    ///< 微秒持续时间
using milliseconds = duration<int64_t, milli>;    ///< 毫秒持续时间
using seconds = duration<int64_t>;                ///< 秒持续时间
using minutes = duration<int64_t, ratio<60>>;     ///< 分钟持续时间
using hours = duration<int64_t, ratio<3600>>;     ///< 小时持续时间
using days = duration<int64_t, ratio<86400>>;     ///< 天持续时间
using weeks = duration<int64_t, ratio<604800>>;   ///< 周持续时间
using years = duration<int64_t, ratio<31556952>>; ///< 年持续时间（天文年）
using months = duration<int64_t, ratio<2629746>>; ///< 月持续时间（平均月）


/**
 * @class duration
 * @brief 持续时间类模板
 * @tparam Rep 表示时间的数值类型
 * @tparam Period 时间单位比例
 *
 * 表示一个时间间隔，支持不同时间单位的转换和运算。
 */
template <typename Rep, typename Period> struct duration {
    static_assert(!is_duration_v<Rep>, "rep cannot be a duration");
    static_assert(is_ratio_v<Period>, "period must be a specialization of ratio");
    static_assert(Period::num > 0, "period must be positive");

private:
    /**
     * @brief 时间单位除法辅助
     * @tparam R1 第一个时间单位比例
     * @tparam R2 第二个时间单位比例
     * @tparam Gcd1 分子最大公约数
     * @tparam Gcd2 分母最大公约数
     */
    template <typename R1, typename R2, intmax_t Gcd1 = _NEFORCE gcd(R1::num, R2::num),
              intmax_t Gcd2 = _NEFORCE gcd(R1::den, R2::den)>
    using divide = ratio<(R1::num / Gcd1) * (R2::den / Gcd2), (R1::den / Gcd2) * (R2::num / Gcd1)>;

    /**
     * @brief 检查时间单位是否和谐（分母为1）
     * @tparam Period2 要检查的时间单位比例
     */
    template <typename Period2> using is_harmonic = bool_constant<divide<Period2, Period>::den == 1>;

public:
    using rep = Rep;                      ///< 数值类型
    using period = typename Period::type; ///< 时间单位比例类型

private:
    rep rep_ = _NEFORCE initialize<rep>(); ///< 内部存储的数值

public:
    /**
     * @brief 默认构造函数
     */
    constexpr duration() = default;

    /**
     * @brief 复制构造函数
     */
    duration(const duration&) = default;

    /**
     * @brief 复制赋值运算符
     */
    constexpr duration& operator=(const duration&) = default;

    /**
     * @brief 从数值构造
     * @tparam Rep2 源数值类型
     * @param rep2 时间数值
     *
     * 从指定数值构造持续时间，支持隐式或显式转换。
     */
    template <typename Rep2, typename = enable_if_t<is_convertible_v<const Rep2&, rep>>>
    constexpr explicit duration(const Rep2& rep2) :
    rep_(static_cast<rep>(rep2)) {}

    /**
     * @brief 从其他持续时间构造
     * @tparam Rep2 源数值类型
     * @tparam Period2 源时间单位比例
     * @tparam Dummy SFINAF参数
     * @param dur 源持续时间
     *
     * 从另一种持续时间类型构造，支持时间单位转换。
     */
    template <typename Rep2, typename Period2,
              typename Dummy = enable_if_t<is_convertible_v<const Rep2&, rep> &&
                                           (is_floating_point_v<rep> ||
                                            (is_harmonic<Period2>::value && !is_floating_point_v<Rep2>) )>>
    constexpr duration(const duration<Rep2, Period2>& dur) :
    rep_(_NEFORCE time_cast<duration>(dur).count()) {}

    /**
     * @brief 从其他时间点构造
     * @tparam Clock 时钟类型
     * @tparam Dur 持续时间类型
     * @tparam Dummy SFINAF参数
     *
     * 从时间点构造，支持时间单位转换。
     */
    template <typename Clock, typename Dur, typename Dummy = enable_if_t<is_duration_v<Dur>>>
    explicit constexpr duration(time_point<Clock, Dur> tp);


    /**
     * @brief 获取计数值
     * @return 存储的时间数值
     */
    constexpr rep count() const noexcept { return rep_; }

    /**
     * @brief 一元正号运算符
     * @return 正持续时间
     */
    constexpr duration<common_type_t<rep>, period> operator+() const {
        return duration<common_type_t<rep>, period>(rep_);
    }

    /**
     * @brief 一元负号运算符
     * @return 负持续时间
     */
    constexpr duration<common_type_t<rep>, period> operator-() const {
        return duration<common_type_t<rep>, period>(-rep_);
    }

    /**
     * @brief 前置自增运算符
     * @return 自增后的持续时间引用
     */
    constexpr duration& operator++() {
        ++rep_;
        return *this;
    }

    /**
     * @brief 后置自增运算符
     * @return 自增前的持续时间
     */
    constexpr duration operator++(int) { return duration(rep_++); }

    /**
     * @brief 前置自减运算符
     * @return 自减后的持续时间引用
     */
    constexpr duration& operator--() {
        --rep_;
        return *this;
    }

    /**
     * @brief 后置自减运算符
     * @return 自减前的持续时间
     */
    constexpr duration operator--(int) { return duration(rep_--); }

    /**
     * @brief 加法赋值运算符
     * @param dur 要加的持续时间
     * @return 当前对象的引用
     */
    constexpr duration& operator+=(const duration& dur) {
        rep_ += dur.count();
        return *this;
    }

    /**
     * @brief 减法赋值运算符
     * @param dur 要减的持续时间
     * @return 当前对象的引用
     */
    constexpr duration& operator-=(const duration& dur) {
        rep_ -= dur.count();
        return *this;
    }

    /**
     * @brief 乘法赋值运算符
     * @param rhs 乘数
     * @return 当前对象的引用
     */
    constexpr duration& operator*=(const rep& rhs) {
        rep_ *= rhs;
        return *this;
    }

    /**
     * @brief 除法赋值运算符
     * @param rhs 除数
     * @return 当前对象的引用
     */
    constexpr duration& operator/=(const rep& rhs) {
        rep_ /= rhs;
        return *this;
    }

    /**
     * @brief 取模赋值运算符
     * @tparam U 数值类型
     * @param rhs 模数
     * @return 当前对象的引用
     */
    template <typename U = rep, enable_if_t<!is_floating_point_v<U>, int> = 0>
    constexpr duration& operator%=(const rep& rhs) {
        rep_ %= rhs;
        return *this;
    }

    /**
     * @brief 持续时间取模赋值运算符
     * @tparam U 数值类型
     * @param rhs 模数持续时间
     * @return 当前对象的引用
     */
    template <typename U = rep, enable_if_t<!is_floating_point_v<U>, int> = 0>
    constexpr duration& operator%=(const duration& rhs) {
        rep_ %= rhs.count();
        return *this;
    }


    /**
     * @brief 获取零持续时间
     * @return 零持续时间
     */
    static constexpr duration zero() noexcept { return duration(rep(0)); }

    /**
     * @brief 获取最小持续时间
     * @return 最小持续时间
     */
    static constexpr duration min() noexcept { return duration(numeric_traits<Rep>::lowest()); }

    /**
     * @brief 获取最大持续时间
     * @return 最大持续时间
     */
    static constexpr duration max() noexcept { return duration(numeric_traits<Rep>::max()); }

    NEFORCE_ALWAYS_INLINE duration<Rep, nano> to_nano() const { return _NEFORCE time_cast<nanoseconds>(*this); }
    NEFORCE_ALWAYS_INLINE duration<Rep, micro> to_micro() const { return _NEFORCE time_cast<microseconds>(*this); }
    NEFORCE_ALWAYS_INLINE duration<Rep, milli> to_milli() const { return _NEFORCE time_cast<milliseconds>(*this); }
    NEFORCE_ALWAYS_INLINE duration<Rep> to_sec() const { return _NEFORCE time_cast<seconds>(*this); }
    NEFORCE_ALWAYS_INLINE duration<Rep, ratio<60>> to_minu() const { return _NEFORCE time_cast<minutes>(*this); }
    NEFORCE_ALWAYS_INLINE duration<Rep, ratio<3600>> to_hour() const { return _NEFORCE time_cast<hours>(*this); }
    NEFORCE_ALWAYS_INLINE duration<Rep, ratio<86400>> to_day() const { return _NEFORCE time_cast<days>(*this); }
    NEFORCE_ALWAYS_INLINE duration<Rep, ratio<604800>> to_week() const { return _NEFORCE time_cast<weeks>(*this); }
    NEFORCE_ALWAYS_INLINE duration<Rep, ratio<31556952>> to_year() const { return _NEFORCE time_cast<years>(*this); }
    NEFORCE_ALWAYS_INLINE duration<Rep, ratio<2629746>> to_month() const { return _NEFORCE time_cast<months>(*this); }
};

/// @cond
NEFORCE_BEGIN_INNER__

/**
 * @brief 公共数值类型辅助
 * @tparam Rep1 第一个数值类型
 * @tparam Rep2 第二个数值类型
 */
template <typename Rep1, typename Rep2, typename CommonRep = common_type_t<Rep1, Rep2>>
using __common_rep_t = enable_if_t<is_convertible_v<const Rep2&, CommonRep>, CommonRep>;

NEFORCE_END_INNER__
/// @endcond


/**
 * @brief 加法运算符
 * @tparam Rep1 第一个数值类型
 * @tparam Period1 第一个时间单位比例
 * @tparam Rep2 第二个数值类型
 * @tparam Period2 第二个时间单位比例
 * @param lhs 左操作数
 * @param rhs 右操作数
 * @return 两个持续时间的和
 */
template <typename Rep1, typename Period1, typename Rep2, typename Period2>
constexpr common_type_t<duration<Rep1, Period1>, duration<Rep2, Period2>>
operator+(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs) {
    using duration1 = duration<Rep1, Period1>;
    using duration2 = duration<Rep2, Period2>;
    using common_duration = common_type_t<duration1, duration2>;
    return common_duration(common_duration(lhs).count() + common_duration(rhs).count());
}

/**
 * @brief 减法运算符
 * @tparam Rep1 第一个数值类型
 * @tparam Period1 第一个时间单位比例
 * @tparam Rep2 第二个数值类型
 * @tparam Period2 第二个时间单位比例
 * @param lhs 左操作数
 * @param rhs 右操作数
 * @return 两个持续时间的差
 */
template <typename Rep1, typename Period1, typename Rep2, typename Period2>
constexpr common_type_t<duration<Rep1, Period1>, duration<Rep2, Period2>>
operator-(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs) {
    using duration1 = duration<Rep1, Period1>;
    using duration2 = duration<Rep2, Period2>;
    using common_duration = common_type_t<duration1, duration2>;
    return common_duration(common_duration(lhs).count() - common_duration(rhs).count());
}

/**
 * @brief 乘法运算符（持续时间 * 标量）
 * @tparam Rep1 数值类型
 * @tparam Period 时间单位比例
 * @tparam Rep2 标量类型
 * @param value 持续时间
 * @param scalar 标量
 * @return 持续时间乘以标量的结果
 */
template <typename Rep1, typename Period, typename Rep2>
constexpr duration<inner::__common_rep_t<Rep1, Rep2>, Period> operator*(const duration<Rep1, Period>& value,
                                                                        const Rep2& scalar) {
    using common_duration = duration<common_type_t<Rep1, Rep2>, Period>;
    return common_duration(common_duration(value).count() * scalar);
}

/**
 * @brief 乘法运算符（标量 * 持续时间）
 * @tparam Rep1 标量类型
 * @tparam Rep2 数值类型
 * @tparam Period 时间单位比例
 * @param scalar 标量
 * @param value 持续时间
 * @return 标量乘以持续时间的结果
 */
template <typename Rep1, typename Rep2, typename Period>
constexpr duration<inner::__common_rep_t<Rep2, Rep1>, Period> operator*(const Rep1& scalar,
                                                                        const duration<Rep2, Period>& value) {
    return value * scalar;
}

/**
 * @brief 除法运算符（持续时间 / 标量）
 * @tparam Rep1 数值类型
 * @tparam Period 时间单位比例
 * @tparam Rep2 标量类型
 * @param value 持续时间
 * @param scalar 标量
 * @return 持续时间除以标量的结果
 */
template <typename Rep1, typename Period, typename Rep2>
constexpr duration<inner::__common_rep_t<Rep1, enable_if_t<!is_duration_v<Rep2>, Rep2>>, Period>
operator/(const duration<Rep1, Period>& value, const Rep2& scalar) {
    using common_duration = duration<common_type_t<Rep1, Rep2>, Period>;
    return common_duration(common_duration(value).count() / scalar);
}

/**
 * @brief 除法运算符（持续时间 / 持续时间）
 * @tparam Rep1 第一个数值类型
 * @tparam Period1 第一个时间单位比例
 * @tparam Rep2 第二个数值类型
 * @tparam Period2 第二个时间单位比例
 * @param lhs 左操作数
 * @param rhs 右操作数
 * @return 两个持续时间的比值
 */
template <typename Rep1, typename Period1, typename Rep2, typename Period2>
constexpr common_type_t<Rep1, Rep2> operator/(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs) {
    using duration1 = duration<Rep1, Period1>;
    using duration2 = duration<Rep2, Period2>;
    using common_duration = common_type_t<duration1, duration2>;
    return common_duration(lhs).count() / common_duration(rhs).count();
}

/**
 * @brief 取模运算符（持续时间 % 标量）
 * @tparam Rep1 数值类型
 * @tparam Period 时间单位比例
 * @tparam Rep2 标量类型
 * @param value 持续时间
 * @param scalar 标量
 * @return 持续时间对标量取模的结果
 */
template <typename Rep1, typename Period, typename Rep2>
constexpr duration<inner::__common_rep_t<Rep1, enable_if_t<!is_duration_v<Rep2>, Rep2>>, Period>
operator%(const duration<Rep1, Period>& value, const Rep2& scalar) {
    using common_duration = duration<common_type_t<Rep1, Rep2>, Period>;
    return common_duration(common_duration(value).count() % scalar);
}

/**
 * @brief 取模运算符（持续时间 % 持续时间）
 * @tparam Rep1 第一个数值类型
 * @tparam Period1 第一个时间单位比例
 * @tparam Rep2 第二个数值类型
 * @tparam Period2 第二个时间单位比例
 * @param lhs 左操作数
 * @param rhs 右操作数
 * @return 左持续时间对右持续时间取模的结果
 */
template <typename Rep1, typename Period1, typename Rep2, typename Period2>
constexpr common_type_t<duration<Rep1, Period1>, duration<Rep2, Period2>>
operator%(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs) {
    using duration1 = duration<Rep1, Period1>;
    using duration2 = duration<Rep2, Period2>;
    using common_duration = common_type_t<duration1, duration2>;
    return common_duration(common_duration(lhs).count() % common_duration(rhs).count());
}

/**
 * @brief 等于比较运算符
 * @tparam Rep1 第一个数值类型
 * @tparam Period1 第一个时间单位比例
 * @tparam Rep2 第二个数值类型
 * @tparam Period2 第二个时间单位比例
 * @param lhs 左操作数
 * @param rhs 右操作数
 * @return 两个持续时间是否相等
 */
template <typename Rep1, typename Period1, typename Rep2, typename Period2>
constexpr bool operator==(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs) {
    using duration1 = duration<Rep1, Period1>;
    using duration2 = duration<Rep2, Period2>;
    using common_duration = common_type_t<duration1, duration2>;
    return common_duration(lhs).count() == common_duration(rhs).count();
}

/**
 * @brief 不等于比较运算符
 * @tparam Rep1 第一个数值类型
 * @tparam Period1 第一个时间单位比例
 * @tparam Rep2 第二个数值类型
 * @tparam Period2 第二个时间单位比例
 * @param lhs 左操作数
 * @param rhs 右操作数
 * @return 两个持续时间是否不相等
 */
template <typename Rep1, typename Period1, typename Rep2, typename Period2>
constexpr bool operator!=(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs) {
    return !(lhs == rhs);
}

/**
 * @brief 小于比较运算符
 * @tparam Rep1 第一个数值类型
 * @tparam Period1 第一个时间单位比例
 * @tparam Rep2 第二个数值类型
 * @tparam Period2 第二个时间单位比例
 * @param lhs 左操作数
 * @param rhs 右操作数
 * @return 左持续时间是否小于右持续时间
 */
template <typename Rep1, typename Period1, typename Rep2, typename Period2>
constexpr bool operator<(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs) {
    using duration1 = duration<Rep1, Period1>;
    using duration2 = duration<Rep2, Period2>;
    using common_duration = common_type_t<duration1, duration2>;
    return common_duration(lhs).count() < common_duration(rhs).count();
}

/**
 * @brief 小于等于比较运算符
 * @tparam Rep1 第一个数值类型
 * @tparam Period1 第一个时间单位比例
 * @tparam Rep2 第二个数值类型
 * @tparam Period2 第二个时间单位比例
 * @param lhs 左操作数
 * @param rhs 右操作数
 * @return 左持续时间是否小于等于右持续时间
 */
template <typename Rep1, typename Period1, typename Rep2, typename Period2>
constexpr bool operator<=(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs) {
    return !(rhs < lhs);
}

/**
 * @brief 大于比较运算符
 * @tparam Rep1 第一个数值类型
 * @tparam Period1 第一个时间单位比例
 * @tparam Rep2 第二个数值类型
 * @tparam Period2 第二个时间单位比例
 * @param lhs 左操作数
 * @param rhs 右操作数
 * @return 左持续时间是否大于右持续时间
 */
template <typename Rep1, typename Period1, typename Rep2, typename Period2>
constexpr bool operator>(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs) {
    return rhs < lhs;
}

/**
 * @brief 大于等于比较运算符
 * @tparam Rep1 第一个数值类型
 * @tparam Period1 第一个时间单位比例
 * @tparam Rep2 第二个数值类型
 * @tparam Period2 第二个时间单位比例
 * @param lhs 左操作数
 * @param rhs 右操作数
 * @return 左持续时间是否大于等于右持续时间
 */
template <typename Rep1, typename Period1, typename Rep2, typename Period2>
constexpr bool operator>=(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs) {
    return !(lhs < rhs);
}


/**
 * @brief 向下取整持续时间转换
 * @tparam ToDur 目标持续时间类型
 * @tparam Rep 源数值类型
 * @tparam Period 源时间单位比例
 * @param dur 源持续时间
 * @return 向下取整转换后的持续时间
 */
template <typename ToDur, typename Rep, typename Period>
NEFORCE_NODISCARD constexpr enable_if_t<is_duration_v<ToDur>, ToDur> floor(const duration<Rep, Period>& dur) {
    auto to = time_cast<ToDur>(dur);
    if (to > dur) {
        return to - ToDur{1};
    }
    return to;
}

/**
 * @brief 向上取整持续时间转换
 * @tparam ToDur 目标持续时间类型
 * @tparam Rep 源数值类型
 * @tparam Period 源时间单位比例
 * @param dur 源持续时间
 * @return 向上取整转换后的持续时间
 */
template <typename ToDur, typename Rep, typename Period>
NEFORCE_NODISCARD constexpr enable_if_t<is_duration_v<ToDur>, ToDur> ceil(const duration<Rep, Period>& dur) {
    auto to = time_cast<ToDur>(dur);
    if (to < dur) {
        return to + ToDur{1};
    }
    return to;
}

/** @} */ // Duration

/// @cond
NEFORCE_BEGIN_INNER__

/**
 * @brief 检查字面量是否溢出
 * @tparam Dur 持续时间类型
 * @tparam Digits 数字字符序列
 * @return 转换后的持续时间
 */
template <typename Dur, char... Digits> constexpr Dur __check_overflow() noexcept {
    using value_type = static_parse_int<Digits...>;
    constexpr typename Dur::rep rep = value_type::value;
    static_assert(rep >= 0 && rep == value_type::value, "literal value cannot be represented by duration type");
    return Dur(rep);
}

NEFORCE_END_INNER__
/// @endcond

/**
 * @namespace literals
 * @brief 自定义字面量命名空间
 */
NEFORCE_BEGIN_LITERALS__

/**
 * @defgroup UserLiterals 字面量
 * @brief 用户定义字面量支持
 * @{
 */

/**
 * @brief 小时字面量（浮点版本）
 * @param hours 小时数（浮点）
 * @return 小时持续时间
 */
constexpr duration<decimal_t, ratio<3600, 1>> operator""_h(const decimal_t hours) noexcept {
    return duration<decimal_t, ratio<3600, 1>>{hours};
}

/**
 * @brief 小时字面量（整型版本）
 * @tparam Digits 数字字符序列
 * @return 小时持续时间
 */
template <char... Digits> constexpr hours operator""_h() noexcept {
    return inner::__check_overflow<hours, Digits...>();
}

/**
 * @brief 分钟字面量（浮点版本）
 * @param mins 分钟数（浮点）
 * @return 分钟持续时间
 */
constexpr duration<decimal_t, ratio<60, 1>> operator""_min(const decimal_t mins) noexcept {
    return duration<decimal_t, ratio<60, 1>>{mins};
}

/**
 * @brief 分钟字面量（整型版本）
 * @tparam Digits 数字字符序列
 * @return 分钟持续时间
 */
template <char... Digits> constexpr minutes operator""_min() noexcept {
    return inner::__check_overflow<minutes, Digits...>();
}

/**
 * @brief 秒字面量（浮点版本）
 * @param secs 秒数（浮点）
 * @return 秒持续时间
 */
constexpr duration<decimal_t> operator""_s(const decimal_t secs) noexcept { return duration<decimal_t>{secs}; }

/**
 * @brief 秒字面量（整型版本）
 * @tparam Digits 数字字符序列
 * @return 秒持续时间
 */
template <char... Digits> constexpr seconds operator""_s() noexcept {
    return inner::__check_overflow<seconds, Digits...>();
}

/**
 * @brief 毫秒字面量（浮点版本）
 * @param msecs 毫秒数（浮点）
 * @return 毫秒持续时间
 */
constexpr duration<decimal_t, milli> operator""_ms(const decimal_t msecs) noexcept {
    return duration<decimal_t, milli>{msecs};
}

/**
 * @brief 毫秒字面量（整型版本）
 * @tparam Digits 数字字符序列
 * @return 毫秒持续时间
 */
template <char... Digits> constexpr milliseconds operator""_ms() noexcept {
    return inner::__check_overflow<milliseconds, Digits...>();
}

/**
 * @brief 微秒字面量（浮点版本）
 * @param usecs 微秒数（浮点）
 * @return 微秒持续时间
 */
constexpr duration<decimal_t, micro> operator""_us(const decimal_t usecs) noexcept {
    return duration<decimal_t, micro>{usecs};
}

/**
 * @brief 微秒字面量（整型版本）
 * @tparam Digits 数字字符序列
 * @return 微秒持续时间
 */
template <char... Digits> constexpr microseconds operator""_us() noexcept {
    return inner::__check_overflow<microseconds, Digits...>();
}

/**
 * @brief 纳秒字面量（浮点版本）
 * @param nsecs 纳秒数（浮点）
 * @return 纳秒持续时间
 */
constexpr duration<decimal_t, nano> operator""_ns(const decimal_t nsecs) noexcept {
    return duration<decimal_t, nano>{nsecs};
}

/**
 * @brief 纳秒字面量（整型版本）
 * @tparam Digits 数字字符序列
 * @return 纳秒持续时间
 */
template <char... Digits> constexpr nanoseconds operator""_ns() noexcept {
    return inner::__check_overflow<nanoseconds, Digits...>();
}

/** @} */ // UserLiterals

NEFORCE_END_LITERALS__

/// @cond
NEFORCE_BEGIN_INNER__

void NEFORCE_API sleep_for_aux(ssize_t s, ssize_t ns);

NEFORCE_END_INNER__
/// @endcond

NEFORCE_BEGIN_THIS_THREAD__

/**
 * @defgroup Thread 线程
 * @brief 线程管理和相关操作
 * @{
 */

/**
 * @brief 使当前线程睡眠指定时间
 * @tparam Rep 数值类型
 * @tparam Period 时间单位比例
 * @param time 要睡眠的时间
 */
template <typename Rep, typename Period> void sleep_for(const duration<Rep, Period> time) {
    if (time <= time.zero()) {
        return;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    const nanoseconds ns = time.to_nano();
    inner::sleep_for_aux(0, ns.count());
#elif defined(NEFORCE_PLATFORM_LINUX)
    const seconds s = time.to_sec();
    const nanoseconds ns(time - s);
    inner::sleep_for_aux(s.count(), ns.count());
#endif
}

/** @} */ // Thread

NEFORCE_END_THIS_THREAD__

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_TIME_DURATION_HPP__
