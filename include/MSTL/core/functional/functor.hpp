#ifndef MSTL_CORE_FUNCTIONAL_FUNCTOR_HPP__
#define MSTL_CORE_FUNCTIONAL_FUNCTOR_HPP__

/**
 * @file functor.hpp
 * @brief MSTL仿函数
 *
 * 此文件提供了各种仿函数的实现，包括算术运算、比较运算、选择运算等。
 */

#include "../typeinfo/type_traits.hpp"
MSTL_BEGIN_NAMESPACE__

/**
 * @defgroup LegacyFunctionAdapters 旧式函数适配器
 * @brief 为兼容性提供的旧式函数适配器基类
 * @deprecated 已弃用，使用lambda表达式代替之
 * @{
 */

/**
 * @struct unary_function
 * @brief 一元函数适配器基类
 * @tparam Arg 参数类型
 * @tparam Result 返回值类型
 * @deprecated 已弃用
 *
 * 提供标准的一元函数类型定义，用于旧式函数适配器。
 */
template <typename Arg, typename Result>
struct MSTL_FUNC_ADAPTER_DEPRECATE unary_function {
	using argument_type = Arg;     ///< 参数类型
	using result_type   = Result;  ///< 返回值类型
};

/**
 * @struct binary_function
 * @brief 二元函数适配器基类
 * @tparam Arg1 第一个参数类型
 * @tparam Arg2 第二个参数类型
 * @tparam Result 返回值类型
 * @deprecated 已弃用
 *
 * 提供标准的二元函数类型定义，用于旧式函数适配器。
 */
template <typename Arg1, typename Arg2, typename Result>
struct MSTL_FUNC_ADAPTER_DEPRECATE binary_function {
	using first_argument_type  = Arg1;    ///< 第一个参数类型
	using second_argument_type = Arg2;    ///< 第二个参数类型
	using result_type          = Result;  ///< 返回值类型
};

/** @} */ // LegacyFunctionAdapters

/**
 * @defgroup ArithmeticFunctors 算术运算仿函数
 * @brief 实现基本算术运算的函数对象
 * @{
 */

/**
 * @struct plus
 * @brief 加法运算仿函数
 * @tparam T 操作数类型
 *
 * 执行加法运算：x + y。
 *
 * @note void特化支持透明操作，可以接受不同类型的参数。
 */
template <typename T = void>
struct plus {
	MSTL_NODISCARD constexpr T operator ()(const T& x, const T& y) const
		noexcept(noexcept(_MSTL declcopy<T>(x + y))) {
		return x + y;
	}
};

/**
 * @brief plus的void特化，支持透明操作
 */
template <>
struct plus<void> {
    using is_transparent = void;

	template <typename T1, typename T2>
	MSTL_NODISCARD constexpr auto operator ()(T1&& x, T2&& y) const
	noexcept(noexcept(static_cast<T1&&>(x) + static_cast<T2&&>(y)))
	-> decltype(static_cast<T1&&>(x) + static_cast<T2&&>(y)) {
		return static_cast<T1&&>(x) + static_cast<T2&&>(y);
	}
};


/**
 * @struct minus
 * @brief 减法运算仿函数
 * @tparam T 操作数类型
 *
 * 执行减法运算：x - y。
 */
template <typename T = void>
struct minus {
	MSTL_NODISCARD constexpr T operator ()(const T& x, const T& y) const
		noexcept(noexcept(_MSTL declcopy<T>(x - y))) {
		return x - y;
	}
};

/**
 * @brief minus的void特化，支持透明操作
 */
template <>
struct minus<void> {
    using is_transparent = void;

	template <typename T1, typename T2>
	MSTL_NODISCARD constexpr auto operator ()(T1&& x, T2&& y) const
	noexcept(noexcept(static_cast<T1&&>(x) - static_cast<T2&&>(y)))
	-> decltype(static_cast<T1&&>(x) - static_cast<T2&&>(y)) {
		return static_cast<T1&&>(x) - static_cast<T2&&>(y);
	}
};

/**
 * @struct multiplies
 * @brief 乘法运算仿函数
 * @tparam T 操作数类型
 *
 * 执行乘法运算：x * y。
 */
template <typename T = void>
struct multiplies {
	MSTL_NODISCARD constexpr T operator ()(const T& x, const T& y) const
		noexcept(noexcept(_MSTL declcopy<T>(x * y))) {
		return x * y;
	}
};

/**
 * @brief multiplies的void特化，支持透明操作
 */
template <>
struct multiplies<void> {
    using is_transparent = void;

	template <typename T1, typename T2>
	MSTL_NODISCARD constexpr auto operator ()(T1&& x, T2&& y) const
	noexcept(noexcept(static_cast<T1&&>(x) * static_cast<T2&&>(y)))
	-> decltype(static_cast<T1&&>(x) * static_cast<T2&&>(y)) {
		return static_cast<T1&&>(x) * static_cast<T2&&>(y);
	}
};

/**
 * @struct divides
 * @brief 除法运算仿函数
 * @tparam T 操作数类型
 *
 * 执行除法运算：x / y。
 */
template <typename T = void>
struct divides {
	MSTL_NODISCARD constexpr T operator ()(const T& x, const T& y) const
		noexcept(noexcept(_MSTL declcopy<T>(x / y))) {
		return x / y;
	}
};

/**
 * @brief divides的void特化，支持透明操作
 */
template <>
struct divides<void> {
    using is_transparent = void;

	template <typename T1, typename T2>
	MSTL_NODISCARD constexpr auto operator ()(T1&& x, T2&& y) const
	noexcept(noexcept(static_cast<T1&&>(x) / static_cast<T2&&>(y)))
	-> decltype(static_cast<T1&&>(x) / static_cast<T2&&>(y)) {
		return static_cast<T1&&>(x) / static_cast<T2&&>(y);
	}
};

/**
 * @struct modulus
 * @brief 取模运算仿函数
 * @tparam T 操作数类型
 *
 * 执行取模运算：x % y。
 */
template <typename T = void>
struct modulus {
	MSTL_NODISCARD constexpr T operator ()(const T& x, const T& y) const
	noexcept(noexcept(_MSTL declcopy<T>(x % y))) {
		return x % y;
	}
};

/**
 * @brief modulus的void特化，支持透明操作
 */
template <>
struct modulus<void> {
    using is_transparent = void;

	template <typename T1, typename T2>
	MSTL_NODISCARD constexpr auto operator ()(T1&& x, T2&& y) const
	noexcept(noexcept(static_cast<T1&&>(x) % static_cast<T2&&>(y)))
	-> decltype(static_cast<T1&&>(x) % static_cast<T2&&>(y)) {
		return static_cast<T1&&>(x) % static_cast<T2&&>(y);
	}
};

/**
 * @struct negate
 * @brief 取负运算仿函数
 * @tparam T 操作数类型
 *
 * 执行取负运算：-x。
 */
template <typename T = void>
struct negate {
	MSTL_NODISCARD constexpr T operator ()(const T& x) const
		noexcept(noexcept(_MSTL declcopy<T>(-x))) {
		return -x;
	}
};

/**
 * @brief negate的void特化，支持透明操作
 */
template <>
struct negate<void> {
    using is_transparent = void;

	template <typename T1>
	MSTL_NODISCARD constexpr auto operator ()(T1&& x) const
	noexcept(noexcept(-static_cast<T1&&>(x)))
	-> decltype(-static_cast<T1&&>(x)) {
		return -static_cast<T1&&>(x);
	}
};

/** @} */ // ArithmeticFunctors

/**
 * @defgroup ComparisonFunctors 比较运算仿函数
 * @brief 实现各种比较运算的函数对象
 * @{
 */

/**
 * @struct equal_to
 * @brief 相等比较仿函数
 * @tparam T 操作数类型
 *
 * 执行相等比较：x == y。
 */
template <typename T = void>
struct equal_to {
	MSTL_NODISCARD constexpr bool operator ()(const T& x, const T& y) const
		noexcept(noexcept(_MSTL declcopy<bool>(x == y))) {
		return x == y;
	}
};

/**
 * @brief equal_to的void特化，支持透明操作
 */
template <>
struct equal_to<void> {
    using is_transparent = void;

	template <typename T1, typename T2>
	MSTL_NODISCARD constexpr auto operator ()(T1&& x, T2&& y) const
	noexcept(noexcept(static_cast<T1&&>(x) == static_cast<T2&&>(y)))
	-> decltype(static_cast<T1&&>(x) == static_cast<T2&&>(y)) {
		return static_cast<T1&&>(x) == static_cast<T2&&>(y);
	}
};

/**
 * @struct not_equal_to
 * @brief 不等比较仿函数
 * @tparam T 操作数类型
 *
 * 执行不等比较：x != y。
 */
template <typename T = void>
struct not_equal_to {
	MSTL_NODISCARD constexpr bool operator ()(const T& x, const T& y) const
		noexcept(noexcept(_MSTL declcopy<bool>(x != y))) {
		return x != y;
	}
};

/**
 * @brief not_equal_to的void特化，支持透明操作
 */
template <>
struct not_equal_to<void> {
    using is_transparent = void;

	template <typename T1, typename T2>
	MSTL_NODISCARD constexpr auto operator ()(T1&& x, T2&& y) const
	noexcept(noexcept(static_cast<T1&&>(x) != static_cast<T2&&>(y)))
	-> decltype(static_cast<T1&&>(x) != static_cast<T2&&>(y)) {
		return static_cast<T1&&>(x) != static_cast<T2&&>(y);
	}
};

/**
 * @struct greater
 * @brief 大于比较仿函数
 * @tparam T 操作数类型
 *
 * 执行大于比较：x > y。
 */
template <typename T = void>
struct greater {
	MSTL_NODISCARD constexpr bool operator ()(const T& x, const T& y) const
		noexcept(noexcept(_MSTL declcopy<bool>(x > y))) {
		return x > y;
	}
};

/**
 * @brief greater的void特化，支持透明操作
 */
template <>
struct greater<void> {
    using is_transparent = void;

	template <typename T1, typename T2>
	MSTL_NODISCARD constexpr auto operator ()(T1&& x, T2&& y) const
	noexcept(noexcept(static_cast<T1&&>(x) > static_cast<T2&&>(y)))
	-> decltype(static_cast<T1&&>(x) > static_cast<T2&&>(y)) {
		return static_cast<T1&&>(x) > static_cast<T2&&>(y);
	}
};

/**
 * @struct less
 * @brief 小于比较仿函数
 * @tparam T 操作数类型
 *
 * 执行小于比较：x < y。
 */
template <typename T = void>
struct less {
	MSTL_NODISCARD constexpr bool operator ()(const T& x, const T& y) const
		noexcept(noexcept(_MSTL declcopy<bool>(x < y))) {
		return x < y;
	}
};

/**
 * @brief less的void特化，支持透明操作
 */
template <>
struct less<void> {
    using is_transparent = void;

	template <typename T1, typename T2>
	MSTL_NODISCARD constexpr auto operator ()(T1&& x, T2&& y) const
	noexcept(noexcept(static_cast<T1&&>(x) < static_cast<T2&&>(y)))
	-> decltype(static_cast<T1&&>(x) < static_cast<T2&&>(y)) {
		return static_cast<T1&&>(x) < static_cast<T2&&>(y);
	}
};

/**
 * @struct greater_equal
 * @brief 大于等于比较仿函数
 * @tparam T 操作数类型
 *
 * 执行大于等于比较：x >= y。
 */
template <typename T = void>
struct greater_equal {
	MSTL_NODISCARD constexpr bool operator ()(const T& x, const T& y) const
		noexcept(noexcept(_MSTL declcopy<bool>(x >= y))) {
		return x >= y;
	}
};

/**
 * @brief greater_equal的void特化，支持透明操作
 */
template <>
struct greater_equal<void> {
    using is_transparent = void;

	template <typename T1, typename T2>
	MSTL_NODISCARD constexpr auto operator ()(T1&& x, T2&& y) const
	noexcept(noexcept(static_cast<T1&&>(x) >= static_cast<T2&&>(y)))
	-> decltype(static_cast<T1&&>(x) >= static_cast<T2&&>(y)) {
		return static_cast<T1&&>(x) >= static_cast<T2&&>(y);
	}
};

/**
 * @struct less_equal
 * @brief 小于等于比较仿函数
 * @tparam T 操作数类型
 *
 * 执行小于等于比较：x <= y。
 */
template <typename T = void>
struct less_equal {
	MSTL_NODISCARD constexpr bool operator ()(const T& x, const T& y) const
		noexcept(noexcept(_MSTL declcopy<bool>(x <= y))) {
		return x <= y;
	}
};

/**
 * @brief less_equal的void特化，支持透明操作
 */
template <>
struct less_equal<void> {
    using is_transparent = void;

	template <typename T1, typename T2>
	MSTL_NODISCARD constexpr auto operator ()(T1&& x, T2&& y) const
	noexcept(noexcept(static_cast<T1&&>(x) <= static_cast<T2&&>(y)))
	-> decltype(static_cast<T1&&>(x) <= static_cast<T2&&>(y)) {
		return static_cast<T1&&>(x) <= static_cast<T2&&>(y);
	}
};

/** @} */ // ComparisonFunctors

/**
 * @defgroup SelectionFunctors 选择运算仿函数
 * @brief 从复杂类型中选择特定部分的函数对象
 * @{
 */

/**
 * @struct identity
 * @brief 恒等仿函数
 * @tparam T 参数类型
 *
 * 返回输入参数本身。
 */
template <typename T>
struct identity {
	template <typename U = T>
	MSTL_NODISCARD constexpr U&& operator ()(U&& x) const noexcept {
		return _MSTL forward<U>(x);
	}
};


/**
 * @struct select1st
 * @brief 选择pair的第一个元素
 * @tparam Pair pair类型
 *
 * 从pair对象中选择并返回第一个元素。
 */
template <typename Pair>
struct select1st {
#ifdef MSTL_STANDARD_20__
	static_assert(is_pair_v<Pair>, "select1st requires pair type.");
#endif // MSTL_STANDARD_20__

	MSTL_NODISCARD constexpr const typename Pair::first_type&
		operator ()(const Pair& x) const noexcept {
		return x.first;
	}
};

/**
 * @struct select2nd
 * @brief 选择pair的第二个元素
 * @tparam Pair pair类型
 *
 * 从pair对象中选择并返回第二个元素。
 */
template <typename Pair>
struct select2nd {
#ifdef MSTL_STANDARD_20__
	static_assert(is_pair_v<Pair>, "select2nd requires pair type.");
#endif // MSTL_STANDARD_20__

	MSTL_NODISCARD constexpr const typename Pair::second_type&
		operator ()(const Pair& x) const noexcept {
		return x.second;
	}
};

/** @} */ // SelectionFunctors

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_FUNCTIONAL_FUNCTOR_HPP__
