#ifndef MSTL_CORE_NUMERIC_MATH_HPP__
#define MSTL_CORE_NUMERIC_MATH_HPP__
#include "../config/exception.hpp"
#include "numeric_limits.hpp"
MSTL_BEGIN_NAMESPACE__

MSTL_BEGIN_CONSTANTS__

MSTL_INLINE17 constexpr decimal_t EULER = 2.718281828459045L;
MSTL_INLINE17 constexpr decimal_t PI = 3.141592653589793L;  // radian
MSTL_INLINE17 constexpr decimal_t PHI = 1.618033988749895L;
MSTL_INLINE17 constexpr decimal_t SEMI_CIRCLE = 180.0;  // angular
MSTL_INLINE17 constexpr decimal_t CIRCLE = 360.0;
MSTL_INLINE17 constexpr decimal_t EPSILON = 1e-15L;

MSTL_INLINE17 constexpr uint32_t TAYLOR_CONVERGENCE = 10000U;
MSTL_INLINE17 constexpr decimal_t PRECISE_TOLERANCE = TAYLOR_CONVERGENCE * EPSILON;
MSTL_INLINE17 constexpr decimal_t LOW_PRECISE_TOLERANCE = TAYLOR_CONVERGENCE * PRECISE_TOLERANCE;

MSTL_INLINE17 constexpr uint16_t FIBONACCI_COUNT = 50;
MSTL_INLINE17 constexpr uint64_t FIBONACCI_LIST[FIBONACCI_COUNT] = {
	0,			1,			1,			2,			3,
	5,			8,			13,			21,			34,
	55,			89,			144,		233,		377,
	610,		987,		1597,		2584,		4181,
	6765,		10946,		17711,		28657,		46368,
	75025,		121393,		196418,		317811,		514229,
	832040,		1346269,	2178309,	3524578,	5702887,
	9227465,	14930352,	24157817,	39088169,	63245986,
	102334155,	165580141,	267914296,	433494437,	701408733,
	1134903170, 1836311903, 2971215073, 4807526976, 7778742049
};

MSTL_END_CONSTANTS__


MSTL_PURE_FUNCTION constexpr uint64_t fibonacci(const uint16_t n) {
	if (n < _CONSTANTS FIBONACCI_COUNT) return _CONSTANTS FIBONACCI_LIST[n];
	return fibonacci(n - 1) + fibonacci(n - 2);
}
MSTL_PURE_FUNCTION constexpr uint64_t leonardo(const uint16_t n) {
	return 2 * fibonacci(n + 1) - 1;
}

MSTL_PURE_FUNCTION constexpr decimal_t angular2radian(const decimal_t angular) noexcept {
	return angular * _CONSTANTS PI / _CONSTANTS SEMI_CIRCLE;
}
MSTL_PURE_FUNCTION constexpr decimal_t radian2angular(const decimal_t radian) noexcept {
	return radian * (_CONSTANTS SEMI_CIRCLE / _CONSTANTS PI);
}

template <typename T, enable_if_t<is_signed<T>::value, int> = 0>
MSTL_CONST_FUNCTION constexpr T opposite(const T& x) noexcept {
	return -x;
}

template <typename T, enable_if_t<is_signed<T>::value, int> = 0>
MSTL_CONST_FUNCTION constexpr T absolute(const T& x) noexcept {
	return x > T(0) ? x : -x;
}
template <typename T, enable_if_t<is_unsigned<T>::value, int> = 0>
MSTL_CONST_FUNCTION constexpr T absolute(const T& x) noexcept {
	return x;
}


template <typename T>
MSTL_CONST_FUNCTION constexpr decltype(auto) sum(const T& x) noexcept {
	return x;
}

template <typename First, typename... Rests, enable_if_t<(sizeof...(Rests) > 0), int> = 0>
MSTL_CONST_FUNCTION constexpr decltype(auto) sum(First first, Rests... args) noexcept {
	return first + _MSTL sum(args...);
}

template <typename... Args, enable_if_t<(sizeof...(Args) > 0), int> = 0>
MSTL_CONST_FUNCTION constexpr decimal_t average(Args... args) noexcept {
	return _MSTL sum(args...) * 1.0 / sizeof...(Args);
}

template <typename T, enable_if_t<is_arithmetic<T>::value, int> = 0>
constexpr int sign(const T& value) {
	constexpr T zero = T(0);
	if (value > zero) return 1;
	if (value < zero) return -1;
	return 0;
}

template <typename T, enable_if_t<is_unsigned<T>::value, int> = 0>
MSTL_CONST_FUNCTION constexpr T gcd(const T& m, const T& n) noexcept {
	while (n != 0) {
		T t = m % n;
		m = n;
		n = t;
	}
	return m;
}

template <typename T, enable_if_t<is_unsigned<T>::value, int> = 0>
MSTL_CONST_FUNCTION constexpr T lcm(const T& m, const T& n) noexcept {
	return m * n / _MSTL gcd(m, n);
}

template <typename T, enable_if_t<is_signed<T>::value, int> = 0>
MSTL_CONST_FUNCTION constexpr T gcd(const T& m, const T& n) noexcept {
	T x = _MSTL absolute(m), y = _MSTL absolute(n);
	constexpr T zero = T(0);
	while (y != zero) {
		T t = x % y;
		x = y;
		y = t;
	}
	return x;
}

template <typename T, enable_if_t<is_signed<T>::value, int> = 0>
MSTL_CONST_FUNCTION constexpr T lcm(const T& m, const T& n) noexcept {
	return m * n / _MSTL gcd(m, n);
}


MSTL_CONST_FUNCTION constexpr decimal_t float_mod(const decimal_t x, const decimal_t y) {
	if (y == 0) throw_exception(math_exception("zero can not be dividend."));
	const decimal_t result = x - static_cast<int>(x / y) * y;
	return result;
}


template <typename T>
MSTL_CONST_FUNCTION constexpr T square(const T& x) noexcept { return x * x; }

template <typename T>
MSTL_CONST_FUNCTION constexpr T cube(const T& x) noexcept { return _MSTL square(x) * x; }

template <typename T, typename UT, enable_if_t<conjunction_v<is_arithmetic<T>, is_integral<UT>>, int> = 0>
MSTL_CONST_FUNCTION constexpr T power(const T& x, UT n) noexcept {
	constexpr UT zero = UT(0);
	constexpr UT one = UT(1);
	constexpr UT two = UT(2);

	if (n == zero) return 1;
	T result(1);
	T base = x;
	while (n > zero) {
		if (n % two == one)
			result *= base;
		base *= base;
		n /= two;
	}
	return result;
}

MSTL_PURE_FUNCTION constexpr decimal_t exponential(const uint32_t n) noexcept {
	return power(_CONSTANTS EULER, n);
}


MSTL_PURE_FUNCTION constexpr decimal_t logarithm_e(const decimal_t x) noexcept {
	uint32_t N = _CONSTANTS TAYLOR_CONVERGENCE;
	const decimal_t a = (x - 1) / (x + 1);
	const decimal_t a_sqrt = a * a;
	decimal_t nk = 2 * N + 1;
	decimal_t y = 1.0 / nk;
	while (N--) {
		nk -= 2;
		y = 1.0 / nk + a_sqrt * y;
	}
	return 2.0 * a * y;
}

MSTL_PURE_FUNCTION constexpr decimal_t logarithm(const decimal_t x, const uint32_t base) noexcept {
	return logarithm_e(x) / logarithm_e(base);
}

MSTL_PURE_FUNCTION constexpr decimal_t logarithm_2(const decimal_t x) noexcept {
	return logarithm(x, 2);
}

MSTL_PURE_FUNCTION constexpr decimal_t logarithm_10(const decimal_t x) noexcept {
	return logarithm(x, 10);
}


MSTL_CONST_FUNCTION constexpr uint64_t logarithm_2_integer(uint64_t x) noexcept {
	uint64_t k = 0;
	for (; x > 1; x >>= 1) ++k;
	return k;
}


MSTL_PURE_FUNCTION constexpr decimal_t
square_root(const decimal_t x, const decimal_t precise = _CONSTANTS PRECISE_TOLERANCE) noexcept {
	decimal_t t = 0.0;
	decimal_t result = x;
	while (absolute(result - t) > precise) {
		t = result;
		result = 0.5 * (t + x / t);
	}
	return result;
}

MSTL_PURE_FUNCTION constexpr decimal_t
cube_root(const decimal_t x, const decimal_t precise = _CONSTANTS PRECISE_TOLERANCE) noexcept {
	decimal_t t = 0.0;
	decimal_t result = x;
	while (absolute(result - t) > precise) {
		t = result;
		result = (2 * t + x / (t * t)) / 3;
	}
	return result;
}

MSTL_CONST_FUNCTION constexpr uint64_t factorial(const uint32_t n) noexcept {
	uint64_t h = 1;
	for (uint32_t i = 1; i <= n; i++)
		h *= i;
	return h;
}


// a bit down to the nearest digit, > 0 operates on decimal places, and 0 on integer places.
MSTL_CONST_FUNCTION constexpr decimal_t floor_bit(const decimal_t x, const uint32_t bit) noexcept {
	const decimal_t times = power(10.0, bit);
	const auto int_part = x * times;
	if (x < 0 && x * times * 10 / 10.0 != int_part)
		return (int_part - 1) / times;
	return int_part / times;
}

MSTL_CONST_FUNCTION constexpr decimal_t ceil_bit(const decimal_t x, const uint32_t bit) noexcept {
	const decimal_t times = power(10.0, bit);
	const auto int_part = x * times;
	if (x > 0 && x * times * 10 / 10.0 != int_part)
		return (int_part + 1) / times;
	return int_part / times;
}

MSTL_CONST_FUNCTION constexpr decimal_t round_bit(const decimal_t x, const uint32_t bit) noexcept {
	return x < 0 ? ceil_bit(x - 0.5 / power(10.0, bit), bit)
		: floor_bit(x + 0.5 / power(10.0, bit), bit);
}

MSTL_CONST_FUNCTION constexpr decimal_t truncate_bit(const decimal_t x, const uint32_t bit) noexcept {
	return x < 0 ? ceil_bit(x, bit) : floor_bit(x, bit);
}


MSTL_CONST_FUNCTION constexpr decimal_t floor(const decimal_t x) noexcept {
    return (x >= 0) ? static_cast<int64_t>(x) : static_cast<int64_t>(x - 1);
}

MSTL_CONST_FUNCTION constexpr decimal_t floor(const decimal_t x, const uint32_t bit) noexcept {
    const decimal_t factor = power(10.0, bit);
    return floor(x * factor) / factor;
}

MSTL_CONST_FUNCTION constexpr decimal_t ceil(const decimal_t x) noexcept {
    return (x >= 0) ? static_cast<int64_t>(x + 1) : static_cast<int64_t>(x);
}

MSTL_CONST_FUNCTION constexpr decimal_t ceil(const decimal_t x, const uint32_t bit) noexcept {
    const decimal_t factor = power(10.0, bit);
    return ceil(x * factor) / factor;
}

MSTL_CONST_FUNCTION constexpr decimal_t round(const decimal_t x) noexcept {
    return (x >= 0) ? floor(x + 0.5) : ceil(x - 0.5);
}

MSTL_CONST_FUNCTION constexpr decimal_t round(const decimal_t x, const uint32_t bit) noexcept {
    const decimal_t factor = power(10.0, bit);
    return round(x * factor) / factor;
}

MSTL_CONST_FUNCTION constexpr decimal_t truncate(const decimal_t x, const int bit) noexcept {
	return x < 0 ? ceil(x, bit) : floor(x, bit);
}

MSTL_CONST_FUNCTION constexpr decimal_t truncate(const decimal_t x) noexcept {
    return truncate(x, 0);
}


MSTL_CONST_FUNCTION constexpr bool around_multiple(const decimal_t x, const decimal_t axis,
	const decimal_t toler = _CONSTANTS PRECISE_TOLERANCE) {
	if (absolute(axis) < _CONSTANTS PRECISE_TOLERANCE)
	    throw_exception(math_exception("Axis Cannot be 0"));

	const decimal_t multi = _MSTL round(x / axis) * axis;
	return absolute(x - multi) < toler;
}

MSTL_CONST_FUNCTION constexpr bool around_pi(const decimal_t x,
	const decimal_t toler = _CONSTANTS LOW_PRECISE_TOLERANCE) {
	return around_multiple(x, _CONSTANTS PI, toler);
}

MSTL_CONST_FUNCTION constexpr bool around_zero(const decimal_t x,
	const decimal_t toler = _CONSTANTS PRECISE_TOLERANCE) {
	return absolute(x) < toler;
}


MSTL_CONST_FUNCTION constexpr decimal_t remainder(const decimal_t x, const decimal_t y) noexcept {
    return x - y * _MSTL round(x / y);
}

MSTL_CONST_FUNCTION constexpr decimal_t float_part(const decimal_t x) noexcept {
	return x - static_cast<int64_t>(x);
}

MSTL_CONST_FUNCTION constexpr decimal_t float_apart(decimal_t x, int64_t* int_ptr) noexcept {
	*int_ptr = static_cast<int64_t>(x);
	x -= *int_ptr;
	return x;
}


MSTL_PURE_FUNCTION constexpr decimal_t sine(decimal_t x) noexcept {
    decimal_t sign = 1.0;
    if (x < 0) {
        sign = -1.0;
        x = -x;
    }
     constexpr decimal_t twoPi = 2 * _CONSTANTS PI;
    x = x - twoPi * floor(x / twoPi);

    if (x > _CONSTANTS PI) {
        x -= _CONSTANTS PI;
        sign = -sign;
    }
    if (x > _CONSTANTS PI / 2) {
        x = _CONSTANTS PI - x;
    }

    decimal_t i = 1;
    int32_t neg = 1;
    decimal_t sum = 0;
    decimal_t idx = x;
    decimal_t fac = 1;
    decimal_t taylor = x;
    do {
        fac = fac * (i + 1) * (i + 2);
        idx *= x * x;
        neg = -neg;
        sum = idx / fac * neg;
        taylor += sum;
        i += 2;
    } while (absolute(sum) > _CONSTANTS EPSILON);
    const auto fin = sign * taylor;
    return around_zero(fin) ? 0 : fin;
}

MSTL_PURE_FUNCTION constexpr decimal_t cosine(const decimal_t x) noexcept {
	return sine(_CONSTANTS PI / 2.0 - x);
}

MSTL_PURE_FUNCTION constexpr decimal_t tangent(const decimal_t x) {
    const decimal_t multiple = (2 * _MSTL round((2 * x - _CONSTANTS PI)
        / (2 * _CONSTANTS PI)) + 1) * (_CONSTANTS PI / 2);
    if (absolute(x - multiple) < _CONSTANTS LOW_PRECISE_TOLERANCE) {
        throw_exception(math_exception("Tangent Range Exceeded"));
    }
	return sine(x) / cosine(x);
}

MSTL_PURE_FUNCTION constexpr decimal_t cotangent(const decimal_t x) {
	return 1 / tangent(x);
}

MSTL_BEGIN_INNER__
MSTL_PURE_FUNCTION constexpr decimal_t __arctangent_taylor(const decimal_t x) noexcept {
    const decimal_t x_sq = x * x;
    decimal_t term = x;
    decimal_t sum = x;
    decimal_t n = 1.0;
    while (absolute(term) > _CONSTANTS EPSILON) {
        term *= -x_sq;
        n += 2.0;
        sum += term / n;
    }
    return sum;
}
MSTL_END_INNER__

MSTL_PURE_FUNCTION constexpr decimal_t arctangent(const decimal_t x) noexcept {
    if (absolute(x) > 1) {
        const decimal_t sign = x > 0 ? 1.0 : -1.0;
        return sign * (_CONSTANTS PI / 2 - _INNER __arctangent_taylor(1 / absolute(x)));
    }
	return _INNER __arctangent_taylor(x);
}

MSTL_PURE_FUNCTION constexpr decimal_t arcsine(const decimal_t x) {
    if (x > 1 || x < -1) throw_exception(math_exception("Arcsine Range Exceeded"));
    return arctangent(x / square_root(1 - x * x));
}

MSTL_PURE_FUNCTION constexpr decimal_t arccosine(const decimal_t x) {
    if (x > 1 || x < -1) throw_exception(math_exception("Arccosine Range Exceeded"));
    return _CONSTANTS PI / 2.0 - arcsine(x);
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_NUMERIC_MATH_HPP__
