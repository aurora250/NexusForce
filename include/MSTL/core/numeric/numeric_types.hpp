#ifndef MSTL_CORE_NUMERIC_NUMERIC_TYPES_HPP__
#define MSTL_CORE_NUMERIC_NUMERIC_TYPES_HPP__
#include "math.hpp"
#include "numeric_limits.hpp"
MSTL_BEGIN_NAMESPACE__


template <typename T, enable_if_t<is_floating_point_v<T>, int> = 0>
MSTL_CONST_FUNCTION constexpr bool signbit(const T x) noexcept {
	using UInt = make_integer_t<sizeof(T), false>;

	const UInt bits = *reinterpret_cast<const UInt*>(&x);
	constexpr UInt sign_mask = static_cast<UInt>(1) << (8 * sizeof(UInt) - 1);
	return (bits & sign_mask) != 0;
}


template <typename T, enable_if_t<is_floating_point_v<T>, int> = 0>
MSTL_CONST_FUNCTION constexpr bool is_nan(const T x) noexcept {
	return x != x;
}

template <typename T, enable_if_t<is_floating_point_v<T>, int> = 0>
MSTL_CONST_FUNCTION constexpr bool is_infinity(const T x) noexcept {
	return x == numeric_limits<T>::infinity();
}

template <typename T, enable_if_t<is_floating_point_v<T>, int> = 0>
MSTL_CONST_FUNCTION constexpr bool is_pos_infinity(const T x) noexcept {
	return _MSTL is_infinity(x);
}

template <typename T, enable_if_t<is_floating_point_v<T>, int> = 0>
MSTL_CONST_FUNCTION constexpr bool is_neg_infinity(const T x) noexcept {
	return x == -numeric_limits<T>::infinity();
}

template <typename T, enable_if_t<is_floating_point_v<T>, int> = 0>
MSTL_CONST_FUNCTION constexpr bool is_finite(const T x) noexcept {
	return !_MSTL is_infinity(x) && !_MSTL is_nan(x);
}

template <typename T, enable_if_t<is_floating_point_v<T>, int> = 0>
MSTL_CONST_FUNCTION constexpr bool is_normal(const T x) noexcept {
	if (!_MSTL is_finite(x)) return false;
	if (x == 0) return false;
	return _MSTL absolute(x) >= numeric_limits<T>::min();
}

template <typename T, enable_if_t<is_floating_point_v<T>, int> = 0>
MSTL_CONST_FUNCTION constexpr bool is_subnormal(const T x) noexcept {
	if (!_MSTL is_finite(x)) return false;
	if (x == 0) return false;
	return _MSTL absolute(x) < numeric_limits<T>::min();
}

template <typename T, enable_if_t<is_floating_point_v<T>, int> = 0>
MSTL_CONST_FUNCTION constexpr bool is_positive(const T x) noexcept {
	return x > 0 || (x == 0 && !_MSTL signbit(x));
}

template <typename T, enable_if_t<is_floating_point_v<T>, int> = 0>
MSTL_CONST_FUNCTION constexpr bool is_negative(const T x) noexcept {
	return x < 0 || (x == 0 && _MSTL signbit(x));
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_NUMERIC_NUMERIC_TYPES_HPP__
