#ifndef MSTL_CORE_NUMERIC_RATIO_HPP__
#define MSTL_CORE_NUMERIC_RATIO_HPP__

/**
 * @file ratio.hpp
 * @brief MSTL比率计算
 *
 * 此文件提供了编译期比率的实现，
 * 支持比率的各种运算，包括加减乘除、比较、化简等操作。
 */

#include "static_numeric.hpp"
MSTL_BEGIN_NAMESPACE__

/**
 * @defgroup RatioClass 比率
 * @brief 比率的主类和运算操作
 * @{
 */

/**
 * @struct ratio
 * @brief 比率类模板
 * @tparam Numerator 分子
 * @tparam Denominator 分母，默认为1
 *
 * 表示编译期比率，自动进行约分，确保分子分母是最简形式。
 * 分母总是正的，符号由分子表示。
 */
template <intmax_t Numerator, intmax_t Denominator = 1>
struct ratio {
    static_assert(Denominator != 0, "denominator cannot be zero");  ///< 分母不能为0
    static_assert(
        Numerator > numeric_traits<intmax_t>::min() &&
        Denominator > numeric_traits<intmax_t>::min(),
        "out of range");  ///< 值必须在范围内

    static constexpr intmax_t num =
        Numerator * static_sign<Denominator>::value /
            static_gcd<Numerator, Denominator>::value;  ///< 约分后的分子
    static constexpr intmax_t den =
        static_abs<Denominator>::value /
            static_gcd<Numerator, Denominator>::value;  ///< 约分后的分母

    using type = ratio<num, den>;  ///< 自身的类型
};

template <intmax_t Numerator, intmax_t Denominator>
constexpr intmax_t ratio<Numerator, Denominator>::num;

template <intmax_t Numerator, intmax_t Denominator>
constexpr intmax_t ratio<Numerator, Denominator>::den;


/**
 * @struct is_ratio
 * @brief 检查类型是否为ratio
 * @tparam Ratio 要检查的类型
 */
template <typename Ratio>
struct is_ratio : false_type {};

/// @cond
template <intmax_t Numerator, intmax_t Denominator>
struct is_ratio<ratio<Numerator, Denominator>> : true_type {};
/// @endcond

#ifdef MSTL_STANDARD_14__
/**
 * @var is_ratio_v
 * @brief is_ratio的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_ratio_v = is_ratio<T>::value;
#endif


/// @cond
MSTL_BEGIN_INNER__

/**
 * @struct __ratio_multiply_impl
 * @brief 比率乘法的内部实现
 * @tparam ratio1 第一个比率
 * @tparam ratio2 第二个比率
 *
 * 实现比率乘法：(a/b) * (c/d) = (a*c)/(b*d)，并进行约分。
 */
template <typename ratio1, typename ratio2>
struct __ratio_multiply_impl {
private:
    static const intmax_t gcd1 = static_gcd<ratio1::num, ratio2::den>::value;  ///< 分子1和分母2的最大公约数
    static const intmax_t gcd2 = static_gcd<ratio2::num, ratio1::den>::value;  ///< 分子2和分母1的最大公约数

public:
    using type = ratio<
        safe_multiply<(ratio1::num / gcd1), (ratio2::num / gcd2)>::value,
        safe_multiply<(ratio1::den / gcd2), (ratio2::den / gcd1)>::value>;

    static constexpr intmax_t num = type::num;  ///< 乘积的分子
    static constexpr intmax_t den = type::den;  ///< 乘积的分母
};

template <typename ratio1, typename ratio2>
constexpr intmax_t __ratio_multiply_impl<ratio1, ratio2>::num;

template <typename ratio1, typename ratio2>
constexpr intmax_t __ratio_multiply_impl<ratio1, ratio2>::den;

MSTL_END_INNER__
/// @endcond

/**
 * @typedef ratio_multiply
 * @brief 比率乘法类型别名
 * @tparam ratio1 第一个比率
 * @tparam ratio2 第二个比率
 */
template <typename ratio1, typename ratio2>
using ratio_multiply = typename _INNER __ratio_multiply_impl<ratio1, ratio2>::type;

/// @cond
MSTL_BEGIN_INNER__

/**
 * @struct __ratio_divide_impl
 * @brief 比率除法的内部实现
 * @tparam ratio1 被除数比率
 * @tparam ratio2 除数比率
 *
 * 实现比率除法：(a/b) / (c/d) = (a*d)/(b*c)。
 */
template <typename ratio1, typename ratio2>
struct __ratio_divide_impl {
    static_assert(ratio2::num != 0, "division by 0");

    using type = typename __ratio_multiply_impl<
        ratio1, ratio<ratio2::den, ratio2::num>>::type;  ///< 乘以倒数

    static constexpr intmax_t num = type::num;  ///< 商的分子
    static constexpr intmax_t den = type::den;  ///< 商的分母
};

template <typename ratio1, typename ratio2>
constexpr intmax_t __ratio_divide_impl<ratio1, ratio2>::num;

template <typename ratio1, typename ratio2>
constexpr intmax_t __ratio_divide_impl<ratio1, ratio2>::den;

MSTL_END_INNER__
/// @endcond

/**
 * @typedef ratio_divide
 * @brief 比率除法类型别名
 * @tparam ratio1 被除数比率
 * @tparam ratio2 除数比率
 */
template <typename ratio1, typename ratio2>
using ratio_divide = typename _INNER __ratio_divide_impl<ratio1, ratio2>::type;


/**
 * @struct ratio_equal
 * @brief 检查两个比率是否相等
 * @tparam ratio1 第一个比率
 * @tparam ratio2 第二个比率
 */
template <typename ratio1, typename ratio2>
struct ratio_equal : bool_constant<
    ratio1::num == ratio2::num && ratio1::den == ratio2::den> {};

/**
 * @struct ratio_not_equal
 * @brief 检查两个比率是否不相等
 * @tparam ratio1 第一个比率
 * @tparam ratio2 第二个比率
 */
template <typename ratio1, typename ratio2>
struct ratio_not_equal : bool_constant<!ratio_equal<ratio1, ratio2>::value> {};

/// @cond
MSTL_BEGIN_INNER__

/**
 * @struct __ratio_less_impl_base
 * @brief 比率小于比较的基础实现
 * @tparam ratio1 第一个比率
 * @tparam ratio2 第二个比率
 * @tparam left_product ratio1::num * ratio2::den 的乘积
 * @tparam right_product ratio2::num * ratio1::den 的乘积
 *
 * 通过比较交叉乘积来判断大小，避免浮点数比较。
 */
template <typename ratio1, typename ratio2,
    typename left_product = big_mul<ratio1::num, ratio2::den>,
    typename right_product = big_mul<ratio2::num, ratio1::den>>
struct __ratio_less_impl_base : bool_constant<big_less<
    left_product::result_high, left_product::result_low,
    right_product::result_high, right_product::result_low
>::value> {};

/**
 * @struct __ratio_less_impl
 * @brief 比率小于比较的内部实现
 * @tparam ratio1 第一个比率
 * @tparam ratio2 第二个比率
 * @tparam has_zero_or_different_sign 是否包含0或符号不同
 * @tparam both_negative 是否都为负数
 *
 * 处理特殊情况（包含0、符号不同、都为负数）。
 */
template <typename ratio1, typename ratio2,
    bool has_zero_or_different_sign = (
        ratio1::num == 0 || ratio2::num == 0 ||
        (static_sign<ratio1::num>::value != static_sign<ratio2::num>::value)),
    bool both_negative = (
        static_sign<ratio1::num>::value == -1 &&
        static_sign<ratio2::num>::value == -1)>
struct __ratio_less_impl : __ratio_less_impl_base<ratio1, ratio2>::type {};

template <typename ratio1, typename ratio2>
struct __ratio_less_impl<ratio1, ratio2, true, false> : bool_constant<ratio1::num < ratio2::num> {};

template <typename ratio1, typename ratio2>
struct __ratio_less_impl<ratio1, ratio2, false, true>
    : __ratio_less_impl_base<ratio<-ratio2::num, ratio2::den>, ratio<-ratio1::num, ratio1::den>>::type {};

MSTL_END_INNER__
/// @endcond

/**
 * @struct ratio_less
 * @brief 检查第一个比率是否小于第二个比率
 * @tparam ratio1 第一个比率
 * @tparam ratio2 第二个比率
 */
template <typename ratio1, typename ratio2>
struct ratio_less : _INNER __ratio_less_impl<ratio1, ratio2>::type {};

/**
 * @struct ratio_less_equal
 * @brief 检查第一个比率是否小于等于第二个比率
 * @tparam ratio1 第一个比率
 * @tparam ratio2 第二个比率
 */
template <typename ratio1, typename ratio2>
struct ratio_less_equal : bool_constant<!ratio_less<ratio2, ratio1>::value> {};

/**
 * @struct ratio_greater
 * @brief 检查第一个比率是否大于第二个比率
 * @tparam ratio1 第一个比率
 * @tparam ratio2 第二个比率
 */
template <typename ratio1, typename ratio2>
struct ratio_greater : bool_constant<ratio_less<ratio2, ratio1>::value> {};

/**
 * @struct ratio_greater_equal
 * @brief 检查第一个比率是否大于等于第二个比率
 * @tparam ratio1 第一个比率
 * @tparam ratio2 第二个比率
 */
template <typename ratio1, typename ratio2>
struct ratio_greater_equal : bool_constant<!ratio_less<ratio1, ratio2>::value> {};


#ifdef MSTL_STANDARD_14__

/**
 * @var ratio_equal_v
 * @brief ratio_equal的便捷变量模板
 */
template <typename ratio1, typename ratio2>
MSTL_INLINE17 constexpr bool ratio_equal_v = ratio_equal<ratio1, ratio2>::value;

/**
 * @var ratio_not_equal_v
 * @brief ratio_not_equal的便捷变量模板
 */
template <typename ratio1, typename ratio2>
MSTL_INLINE17 constexpr bool ratio_not_equal_v = ratio_not_equal<ratio1, ratio2>::value;

/**
 * @var ratio_less_v
 * @brief ratio_less的便捷变量模板
 */
template <typename ratio1, typename ratio2>
MSTL_INLINE17 constexpr bool ratio_less_v = ratio_less<ratio1, ratio2>::value;

/**
 * @var ratio_less_equal_v
 * @brief ratio_less_equal的便捷变量模板
 */
template <typename ratio1, typename ratio2>
MSTL_INLINE17 constexpr bool ratio_less_equal_v = ratio_less_equal<ratio1, ratio2>::value;

/**
 * @var ratio_greater_v
 * @brief ratio_greater的便捷变量模板
 */
template <typename ratio1, typename ratio2>
MSTL_INLINE17 constexpr bool ratio_greater_v = ratio_greater<ratio1, ratio2>::value;

/**
 * @var ratio_greater_equal_v
 * @brief ratio_greater_equal的便捷变量模板
 */
template <typename ratio1, typename ratio2>
MSTL_INLINE17 constexpr bool ratio_greater_equal_v = ratio_greater_equal<ratio1, ratio2>::value;
#endif


/// @cond
MSTL_BEGIN_INNER__

/**
 * @struct __ratio_add_impl
 * @brief 比率加法的内部实现
 * @tparam ratio1 第一个比率
 * @tparam ratio2 第二个比率
 * @tparam ratio1_non_negative 第一个比率是否非负
 * @tparam ratio2_non_negative 第二个比率是否非负
 * @tparam abs_ratio1_less_than_abs_ratio2 第一个比率的绝对值是否小于第二个
 *
 * 实现比率加法：(a/b) + (c/d) = (a*d + b*c)/(b*d)，并进行约分。
 */
template <typename ratio1, typename ratio2,
    bool ratio1_non_negative = (ratio1::num >= 0),
    bool ratio2_non_negative = (ratio2::num >= 0),
    bool abs_ratio1_less_than_abs_ratio2 = ratio_less<
        ratio<static_abs<ratio1::num>::value, ratio1::den>,
        ratio<static_abs<ratio2::num>::value, ratio2::den>
    >::value>
struct __ratio_add_impl {
private:
    using negative_result = typename __ratio_add_impl<
        ratio<-ratio1::num, ratio1::den>,
        ratio<-ratio2::num, ratio2::den>>::type;

public:
    using type = ratio<-negative_result::num, negative_result::den>;  ///< 两个负数相加的结果
};

/**
 * @brief __ratio_add_impl的特化，处理两个非负数相加
 */
template <typename ratio1, typename ratio2, bool abs_less>
struct __ratio_add_impl<ratio1, ratio2, true, true, abs_less> {
private:
    static constexpr uintmax_t gcd_val = static_gcd<ratio1::den, ratio2::den>::value;  ///< 分母的最大公约数
    static constexpr uintmax_t den2_scaled = ratio2::den / gcd_val;  ///< 缩放后的分母2

    using denominator_product = big_mul<ratio1::den, den2_scaled>;   ///< 分母乘积
    using numerator1_scaled = big_mul<ratio1::num, ratio2::den / gcd_val>;  ///< 缩放后的分子1
    using numerator2_scaled = big_mul<ratio2::num, ratio1::den / gcd_val>;  ///< 缩放后的分子2
    using numerator_sum = big_add<
        numerator1_scaled::result_high,
        numerator1_scaled::result_low,
        numerator2_scaled::result_high,
        numerator2_scaled::result_low>;  ///< 分子和

    static_assert(
        numerator_sum::result_high >= numerator1_scaled::result_high,
        "Internal library error");

    using numerator_gcd_reduced = big_div<
        numerator_sum::result_high,
        numerator_sum::result_low,
        gcd_val>;  ///< 用gcd约分分子
    static constexpr uintmax_t gcd_val2 = static_gcd<
        numerator_gcd_reduced::remainder,
        gcd_val>::value;  ///< 进一步的最大公约数
    using numerator_final = big_div<
        numerator_sum::result_high,
        numerator_sum::result_low,
        gcd_val2>;  ///< 最终分子

    static_assert(numerator_final::remainder == 0, "Internal library error");
    static_assert(
        numerator_final::quotient_high == 0 &&
        numerator_final::quotient_low <= numeric_traits<intmax_t>::max(),
        "overflow in addition");

    using denominator_final = big_mul<ratio1::den / gcd_val2, den2_scaled>;  ///< 最终分母

    static_assert(
        denominator_final::result_high == 0 &&
        denominator_final::result_low <= numeric_traits<intmax_t>::max(),
        "overflow in addition");
public:
    using type = ratio<numerator_final::quotient_low, denominator_final::result_low>;
};

/**
 * @brief __ratio_add_impl的特化，处理负数+正数且绝对值较小的情况
 */
template <typename ratio1, typename ratio2>
struct __ratio_add_impl<ratio1, ratio2, false, true, true> :
    __ratio_add_impl<ratio2, ratio1> {};

/**
 * @brief __ratio_add_impl的特化，处理正数+负数且绝对值较大的情况
 */
template <typename ratio1, typename ratio2>
struct __ratio_add_impl<ratio1, ratio2, true, false, false> {
private:
    static constexpr uintmax_t gcd_val = static_gcd<ratio1::den, ratio2::den>::value;
    static constexpr uintmax_t den2_scaled = ratio2::den / gcd_val;

    using denominator_product = big_mul<ratio1::den, den2_scaled>;
    using numerator1_scaled = big_mul<ratio1::num, ratio2::den / gcd_val>;
    using numerator2_scaled = big_mul<-ratio2::num, ratio1::den / gcd_val>;

    using numerator_diff = big_sub<
        numerator1_scaled::result_high,
        numerator1_scaled::result_low,
        numerator2_scaled::result_high,
        numerator2_scaled::result_low>;

    using numerator_gcd_reduced = big_div<
        numerator_diff::result_high,
        numerator_diff::result_low,
        gcd_val>;
    static constexpr uintmax_t gcd_val2 = static_gcd<
        numerator_gcd_reduced::remainder,
        gcd_val>::value;
    using numerator_final = big_div<
        numerator_diff::result_high,
        numerator_diff::result_low,
        gcd_val2>;

    static_assert(numerator_final::remainder == 0, "Internal library error");
    static_assert(
        numerator_final::quotient_high == 0 &&
        numerator_final::quotient_low <= numeric_traits<intmax_t>::max(),
        "overflow in addition");

    using denominator_final = big_mul<ratio1::den / gcd_val2, den2_scaled>;

    static_assert(
        denominator_final::result_high == 0 &&
        denominator_final::result_low <= numeric_traits<intmax_t>::max(),
        "overflow in addition");
public:
    using type = ratio<numerator_final::quotient_low, denominator_final::result_low>;
};

/**
 * @struct ratio_add
 * @brief 比率加法包装结构
 * @tparam ratio1 第一个比率
 * @tparam ratio2 第二个比率
 */
template <typename ratio1, typename ratio2>
struct ratio_add {
    using type = typename __ratio_add_impl<ratio1, ratio2>::type;  ///< 和的类型
    static constexpr intmax_t num = type::num;  ///< 和的分子
    static constexpr intmax_t den = type::den;  ///< 和的分母
};

template <typename ratio1, typename ratio2>
constexpr intmax_t ratio_add<ratio1, ratio2>::num;

template <typename ratio1, typename ratio2>
constexpr intmax_t ratio_add<ratio1, ratio2>::den;

MSTL_END_INNER__
/// @endcond

/**
 * @typedef ratio_add
 * @brief 比率加法类型别名
 * @tparam ratio1 第一个比率
 * @tparam ratio2 第二个比率
 */
template <typename ratio1, typename ratio2>
using ratio_add = typename _INNER ratio_add<ratio1, ratio2>::type;

/// @cond
MSTL_BEGIN_INNER__

/**
 * @struct ratio_subtract
 * @brief 比率减法包装结构
 * @tparam ratio1 被减数比率
 * @tparam ratio2 减数比率
 *
 * 通过加法实现减法：a - b = a + (-b)
 */
template <typename ratio1, typename ratio2>
struct ratio_subtract {
    using type = typename ratio_add<ratio1, ratio<-ratio2::num, ratio2::den>>::type;  ///< 差的类型

    static constexpr intmax_t num = type::num;  ///< 差的分子
    static constexpr intmax_t den = type::den;  ///< 差的分母
};

template <typename ratio1, typename ratio2>
constexpr intmax_t ratio_subtract<ratio1, ratio2>::num;

template <typename ratio1, typename ratio2>
constexpr intmax_t ratio_subtract<ratio1, ratio2>::den;

MSTL_END_INNER__
/// @endcond

/**
 * @typedef ratio_subtract
 * @brief 比率减法类型别名
 * @tparam ratio1 被减数比率
 * @tparam ratio2 减数比率
 */
template <typename ratio1, typename ratio2>
using ratio_subtract = typename _INNER ratio_subtract<ratio1, ratio2>::type;

/** @} */ // RatioClass

/**
 * @defgroup SIUnits SI单位
 * @brief 国际单位制（SI）的常用前缀
 * @{
 */

using atto  = ratio<1, 1000000000000000000>;  ///< 阿托（10^-18）
using femto = ratio<1,    1000000000000000>;  ///< 飞（10^-15）
using pico  = ratio<1,       1000000000000>;  ///< 皮（10^-12）
using nano  = ratio<1,          1000000000>;  ///< 纳（10^-9）
using micro = ratio<1,             1000000>;  ///< 微（10^-6）
using milli = ratio<1,                1000>;  ///< 毫（10^-3）
using centi = ratio<1,                 100>;  ///< 厘（10^-2）
using deci  = ratio<1,                  10>;  ///< 分（10^-1）
using deca  = ratio<10,                  1>;  ///< 十（10^1）
using hecto = ratio<100,                 1>;  ///< 百（10^2）
using kilo  = ratio<1000,                1>;  ///< 千（10^3）
using mega  = ratio<1000000,             1>;  ///< 兆（10^6）
using giga  = ratio<1000000000,          1>;  ///< 吉（10^9）
using tera  = ratio<1000000000000,       1>;  ///< 太（10^12）
using peta  = ratio<1000000000000000,    1>;  ///< 拍（10^15）
using exa   = ratio<1000000000000000000, 1>;  ///< 艾（10^18）

/** @} */ // SIUnits

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_NUMERIC_RATIO_HPP__