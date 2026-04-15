#ifndef NEFORCE_CORE_NUMERIC_NUMERIC_TYPES_HPP__
#define NEFORCE_CORE_NUMERIC_NUMERIC_TYPES_HPP__

/**
 * @file numeric_types.hpp
 * @brief 数值类型特性检查
 *
 * 此文件提供了浮点数特性检查函数实现，
 * 包括符号位检测、特殊值判断、正规性检查等功能。
 */

#include "NeForce/core/numeric/numeric_traits.hpp"
#include "NeForce/core/typeinfo/type_traits.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup NumericTypeChecks 数值类型检查
 * @brief 浮点数特性检查和特殊值判断
 * @{
 */

/**
 * @brief 获取浮点数的符号位
 * @tparam T 浮点数类型
 * @param x 待检查的浮点数
 * @return 如果x为负数或负零则返回true，否则返回false
 *
 * 通过检查浮点数的二进制表示中的符号位来判断其符号。
 *
 * @note 此函数区分正零和负零。
 */
template <typename T>
NEFORCE_CONST_FUNCTION constexpr bool signbit(const T x) noexcept {
    static_assert(is_floating_point_v<T>, "floating point required");
    using UInt = make_integer_t<sizeof(T), false>;

    const UInt bits = *reinterpret_cast<const UInt*>(&x);
    constexpr UInt sign_mask = static_cast<UInt>(1) << (8 * sizeof(UInt) - 1);
    return (bits & sign_mask) != 0;
}


/**
 * @brief 检查浮点数是否为NaN
 * @tparam T 浮点数类型
 * @param x 待检查的浮点数
 * @return 如果x是NaN则返回true，否则返回false
 *
 * NaN的特性：NaN != NaN 总是成立。
 */
template <typename T>
NEFORCE_CONST_FUNCTION constexpr bool is_nan(const T x) noexcept {
    static_assert(is_floating_point_v<T>, "floating point required");
    return x != x;
}

/**
 * @brief 检查浮点数是否为正无穷大
 * @tparam T 浮点数类型
 * @param x 待检查的浮点数
 * @return 如果x是正无穷大则返回true，否则返回false
 */
template <typename T>
NEFORCE_CONST_FUNCTION constexpr bool is_pos_infinity(const T x) noexcept {
    static_assert(is_floating_point_v<T>, "floating point required");
    return x == numeric_traits<T>::infinity();
}

/**
 * @brief 检查浮点数是否为负无穷大
 * @tparam T 浮点数类型
 * @param x 待检查的浮点数
 * @return 如果x是负无穷大则返回true，否则返回false
 */
template <typename T>
NEFORCE_CONST_FUNCTION constexpr bool is_neg_infinity(const T x) noexcept {
    static_assert(is_floating_point_v<T>, "floating point required");
    return x == -numeric_traits<T>::infinity();
}

/**
 * @brief 检查浮点数是否为无穷大
 * @tparam T 浮点数类型
 * @param x 待检查的浮点数
 * @return 如果x是无穷大则返回true，否则返回false
 * @note 此函数不区分正无穷和负无穷
 */
template <typename T>
NEFORCE_CONST_FUNCTION constexpr bool is_infinity(const T x) noexcept {
    return _NEFORCE is_pos_infinity(x) || _NEFORCE is_neg_infinity(x);
}

/**
 * @brief 检查浮点数是否为有限值
 * @tparam T 浮点数类型
 * @param x 待检查的浮点数
 * @return 如果x是有限值则返回true，否则返回false
 *
 * 有限值包括：正常数、负零、正零、次正规数、正规数。
 */
template <typename T>
NEFORCE_CONST_FUNCTION constexpr bool is_finite(const T x) noexcept {
    return !_NEFORCE is_infinity(x) && !_NEFORCE is_nan(x);
}

/**
 * @brief 检查浮点数是否为正规数
 * @tparam T 浮点数类型
 * @param x 待检查的浮点数
 * @return 如果x是正规数则返回true，否则返回false
 *
 * 正规数：绝对值大于等于最小正正规数的有限非零浮点数。
 * 不包括：零、次正规数、无穷大、NaN。
 */
template <typename T>
NEFORCE_CONST_FUNCTION constexpr bool is_normal(const T x) noexcept {
    if (!_NEFORCE is_finite(x)) {
        return false;
    }
    if (x == 0) {
        return false;
    }
    const T abs = x < 0 ? -x : x;
    return abs >= numeric_traits<T>::min();
}

/**
 * @brief 检查浮点数是否为次正规数
 * @tparam T 浮点数类型
 * @param x 待检查的浮点数
 * @return 如果x是次正规数则返回true，否则返回false
 *
 * 次正规数（非正规数）：绝对值小于最小正正规数的非零浮点数。
 * 用于表示接近零的非常小的数值。
 */
template <typename T>
NEFORCE_CONST_FUNCTION constexpr bool is_subnormal(const T x) noexcept {
    if (!_NEFORCE is_finite(x)) {
        return false;
    }
    if (x == 0) {
        return false;
    }
    const T abs = x < 0 ? -x : x;
    return abs < numeric_traits<T>::min();
}

/**
 * @brief 检查浮点数是否为正数
 * @tparam T 浮点数类型
 * @param x 待检查的浮点数
 * @return 如果x是正数则返回true，否则返回false
 *
 * 包括：正正规数、正次正规数、正无穷大、正零。
 *
 * @note 此函数区分正零和负零。
 */
template <typename T>
NEFORCE_CONST_FUNCTION constexpr bool is_positive(const T x) noexcept {
    return x > 0 || (x == 0 && !_NEFORCE signbit(x));
}

/**
 * @brief 检查浮点数是否为负数
 * @tparam T 浮点数类型
 * @param x 待检查的浮点数
 * @return 如果x是负数则返回true，否则返回false
 *
 * 包括：负正规数、负次正规数、负无穷大、负零。
 *
 * @note 此函数区分正零和负零。
 */
template <typename T>
NEFORCE_CONST_FUNCTION constexpr bool is_negative(const T x) noexcept {
    return x < 0 || (x == 0 && _NEFORCE signbit(x));
}

/** @} */ // NumericTypeChecks

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_NUMERIC_NUMERIC_TYPES_HPP__
