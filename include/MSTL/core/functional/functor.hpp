#ifndef MSTL_FUNCTOR_HPP__
#define MSTL_FUNCTOR_HPP__
#include "../typeinfo/type_traits.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename Arg, typename Result>
struct MSTL_FUNC_ADAPTER_DEPRECATE unary_function {
	using argument_type = Arg;
	using result_type	= Result;
};

template <typename Arg1, typename Arg2, typename Result>
struct MSTL_FUNC_ADAPTER_DEPRECATE binary_function {
	using first_argument_type	= Arg1;
	using second_argument_type	= Arg2;
	using result_type			= Result;
};


template <typename T = void>
struct plus {
	using first_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= T;
	using second_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= T;
	using result_type MSTL_FUNC_ADAPTER_DEPRECATE			= T;

	MSTL_NODISCARD constexpr T operator()(const T& x, const T& y) const
		noexcept(noexcept(_MSTL declcopy<T>(x + y))) {
		return x + y;
	}
};

template <>
struct plus<void> {
    using is_transparent = void;

	template <typename T1, typename T2>
	MSTL_NODISCARD constexpr auto operator()(T1&& x, T2&& y) const
	noexcept(noexcept(static_cast<T1&&>(x) + static_cast<T2&&>(y)))
	-> decltype(static_cast<T1&&>(x) + static_cast<T2&&>(y)) {
		return static_cast<T1&&>(x) + static_cast<T2&&>(y);
	}
};

template <typename T = void>
struct minus {
	using first_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= T;
	using second_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= T;
	using result_type MSTL_FUNC_ADAPTER_DEPRECATE			= T;

	MSTL_NODISCARD constexpr T operator()(const T& x, const T& y) const
		noexcept(noexcept(_MSTL declcopy<T>(x - y))) {
		return x - y;
	}
};

template <>
struct minus<void> {
    using is_transparent = void;

	template <typename T1, typename T2>
	MSTL_NODISCARD constexpr auto operator()(T1&& x, T2&& y) const
	noexcept(noexcept(static_cast<T1&&>(x) - static_cast<T2&&>(y)))
	-> decltype(static_cast<T1&&>(x) - static_cast<T2&&>(y)) {
		return static_cast<T1&&>(x) - static_cast<T2&&>(y);
	}
};

template <typename T = void>
struct multiplies {
	using first_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= T;
	using second_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= T;
	using result_type MSTL_FUNC_ADAPTER_DEPRECATE			= T;

	MSTL_NODISCARD constexpr T operator()(const T& x, const T& y) const
		noexcept(noexcept(_MSTL declcopy<T>(x * y))) {
		return x * y;
	}
};

template <>
struct multiplies<void> {
    using is_transparent = void;

	template <typename T1, typename T2>
	MSTL_NODISCARD constexpr auto operator()(T1&& x, T2&& y) const
	noexcept(noexcept(static_cast<T1&&>(x) * static_cast<T2&&>(y)))
	-> decltype(static_cast<T1&&>(x) * static_cast<T2&&>(y)) {
		return static_cast<T1&&>(x) * static_cast<T2&&>(y);
	}
};

template <typename T = void>
struct divides {
	using first_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= T;
	using second_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= T;
	using result_type MSTL_FUNC_ADAPTER_DEPRECATE			= T;

	MSTL_NODISCARD constexpr T operator()(const T& x, const T& y) const
		noexcept(noexcept(_MSTL declcopy<T>(x / y))) {
		return x / y;
	}
};

template <>
struct divides<void> {
    using is_transparent = void;

	template <typename T1, typename T2>
	MSTL_NODISCARD constexpr auto operator()(T1&& x, T2&& y) const
	noexcept(noexcept(static_cast<T1&&>(x) / static_cast<T2&&>(y)))
	-> decltype(static_cast<T1&&>(x) / static_cast<T2&&>(y)) {
		return static_cast<T1&&>(x) / static_cast<T2&&>(y);
	}
};

template <typename T = void>
struct modulus {
	using first_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= T;
	using second_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= T;
	using result_type MSTL_FUNC_ADAPTER_DEPRECATE			= T;

	MSTL_NODISCARD constexpr T operator()(const T& x, const T& y) const
	noexcept(noexcept(_MSTL declcopy<T>(x % y))) {
		return x % y;
	}
};

template <>
struct modulus<void> {
    using is_transparent = void;

	template <typename T1, typename T2>
	MSTL_NODISCARD constexpr auto operator()(T1&& x, T2&& y) const
	noexcept(noexcept(static_cast<T1&&>(x) % static_cast<T2&&>(y)))
	-> decltype(static_cast<T1&&>(x) % static_cast<T2&&>(y)) {
		return static_cast<T1&&>(x) % static_cast<T2&&>(y);
	}
};

template <typename T = void>
struct negate {
	using argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= T;
	using result_type MSTL_FUNC_ADAPTER_DEPRECATE	= T;

	MSTL_NODISCARD constexpr T operator()(const T& x) const
		noexcept(noexcept(_MSTL declcopy<T>(-x))) {
		return -x;
	}
};

template <>
struct negate<void> {
    using is_transparent = void;

	template <typename T1>
	MSTL_NODISCARD constexpr auto operator()(T1&& x) const
	noexcept(noexcept(-static_cast<T1&&>(x)))
	-> decltype(-static_cast<T1&&>(x)) {
		return -static_cast<T1&&>(x);
	}
};


template <typename T = void>
struct equal_to {
	using first_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= T;
	using second_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= T;
	using result_type MSTL_FUNC_ADAPTER_DEPRECATE			= bool;

	MSTL_NODISCARD constexpr bool operator()(const T& x, const T& y) const
		noexcept(noexcept(_MSTL declcopy<bool>(x == y))) {
		return x == y;
	}
};

template <>
struct equal_to<void> {
    using is_transparent = void;

	template <typename T1, typename T2>
	MSTL_NODISCARD constexpr auto operator()(T1&& x, T2&& y) const
	noexcept(noexcept(static_cast<T1&&>(x) == static_cast<T2&&>(y)))
	-> decltype(static_cast<T1&&>(x) == static_cast<T2&&>(y)) {
		return static_cast<T1&&>(x) == static_cast<T2&&>(y);
	}
};

template <typename T = void>
struct not_equal_to {
	using first_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= T;
	using second_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= T;
	using result_type MSTL_FUNC_ADAPTER_DEPRECATE			= bool;

	MSTL_NODISCARD constexpr bool operator()(const T& x, const T& y) const
		noexcept(noexcept(_MSTL declcopy<bool>(x != y))) {
		return x != y;
	}
};

template <>
struct not_equal_to<void> {
    using is_transparent = void;

	template <typename T1, typename T2>
	MSTL_NODISCARD constexpr auto operator()(T1&& x, T2&& y) const
	noexcept(noexcept(static_cast<T1&&>(x) != static_cast<T2&&>(y)))
	-> decltype(static_cast<T1&&>(x) != static_cast<T2&&>(y)) {
		return static_cast<T1&&>(x) != static_cast<T2&&>(y);
	}
};

template <typename T = void>
struct greater {
	using first_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= T;
	using second_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= T;
	using result_type MSTL_FUNC_ADAPTER_DEPRECATE			= bool;

	MSTL_NODISCARD constexpr bool operator()(const T& x, const T& y) const
		noexcept(noexcept(_MSTL declcopy<bool>(x > y))) {
		return x > y;
	}
};

template <>
struct greater<void> {
    using is_transparent = void;

	template <typename T1, typename T2>
	MSTL_NODISCARD constexpr auto operator()(T1&& x, T2&& y) const
	noexcept(noexcept(static_cast<T1&&>(x) > static_cast<T2&&>(y)))
	-> decltype(static_cast<T1&&>(x) > static_cast<T2&&>(y)) {
		return static_cast<T1&&>(x) > static_cast<T2&&>(y);
	}
};

template <typename T = void>
struct less {
	using first_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= T;
	using second_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= T;
	using result_type MSTL_FUNC_ADAPTER_DEPRECATE			= bool;

	MSTL_NODISCARD constexpr bool operator()(const T& x, const T& y) const
		noexcept(noexcept(_MSTL declcopy<bool>(x < y))) {
		return x < y;
	}
};

template <>
struct less<void> {
    using is_transparent = void;

	template <typename T1, typename T2>
	MSTL_NODISCARD constexpr auto operator()(T1&& x, T2&& y) const
	noexcept(noexcept(static_cast<T1&&>(x) < static_cast<T2&&>(y)))
	-> decltype(static_cast<T1&&>(x) < static_cast<T2&&>(y)) {
		return static_cast<T1&&>(x) < static_cast<T2&&>(y);
	}
};

template <typename T = void>
struct greater_equal {
	using first_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= T;
	using second_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= T;
	using result_type MSTL_FUNC_ADAPTER_DEPRECATE			= bool;

	MSTL_NODISCARD constexpr bool operator()(const T& x, const T& y) const
		noexcept(noexcept(_MSTL declcopy<bool>(x >= y))) {
		return x >= y;
	}
};

template <>
struct greater_equal<void> {
    using is_transparent = void;

	template <typename T1, typename T2>
	MSTL_NODISCARD constexpr auto operator()(T1&& x, T2&& y) const
	noexcept(noexcept(static_cast<T1&&>(x) >= static_cast<T2&&>(y)))
	-> decltype(static_cast<T1&&>(x) >= static_cast<T2&&>(y)) {
		return static_cast<T1&&>(x) >= static_cast<T2&&>(y);
	}
};

template <typename T = void>
struct less_equal {
	using first_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= T;
	using second_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= T;
	using result_type MSTL_FUNC_ADAPTER_DEPRECATE			= bool;

	MSTL_NODISCARD constexpr bool operator()(const T& x, const T& y) const
		noexcept(noexcept(_MSTL declcopy<bool>(x <= y))) {
		return x <= y;
	}
};

template <>
struct less_equal<void> {
    using is_transparent = void;

	template <typename T1, typename T2>
	MSTL_NODISCARD constexpr auto operator()(T1&& x, T2&& y) const
	noexcept(noexcept(static_cast<T1&&>(x) <= static_cast<T2&&>(y)))
	-> decltype(static_cast<T1&&>(x) <= static_cast<T2&&>(y)) {
		return static_cast<T1&&>(x) <= static_cast<T2&&>(y);
	}
};


template <typename T>
struct identity {
	template <typename U = T>
	MSTL_NODISCARD constexpr U&& operator()(U&& x) const noexcept {
		return _MSTL forward<U>(x);
	}
};


template <typename Pair>
struct select1st {
#ifdef MSTL_STANDARD_20__
	static_assert(is_pair_v<Pair>, "select1st requires pair type.");
#endif // MSTL_STANDARD_20__

	using argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= Pair;
	using result_type MSTL_FUNC_ADAPTER_DEPRECATE	= typename Pair::first_type;

	MSTL_NODISCARD constexpr const typename Pair::first_type&
		operator()(const Pair& x) const noexcept {
		return x.first;
	}
};

template <typename Pair>
struct select2nd {
#ifdef MSTL_STANDARD_20__
	static_assert(is_pair_v<Pair>, "select2nd requires pair type.");
#endif // MSTL_STANDARD_20__

	using argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= Pair;
	using result_type MSTL_FUNC_ADAPTER_DEPRECATE	= typename Pair::second_type;

	MSTL_NODISCARD constexpr const typename Pair::second_type&
		operator()(const Pair& x) const noexcept {
		return x.second;
	}
};

MSTL_END_NAMESPACE__
#endif // MSTL_FUNCTOR_HPP__
