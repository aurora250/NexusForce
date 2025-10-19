#ifndef MSTL_RATIO_HPP__
#define MSTL_RATIO_HPP__
#include "numeric_limits.hpp"
MSTL_BEGIN_NAMESPACE__

template <intmax_t numerator>
struct static_sign : integral_constant<intmax_t, (numerator < 0) ? -1 : 1> {};

template <intmax_t value>
struct static_abs : integral_constant<intmax_t, value * static_sign<value>::value> {};

template <intmax_t a, intmax_t b>
struct static_gcd : static_gcd<b, (a % b)> {};
template <intmax_t a>
struct static_gcd<a, 0> : integral_constant<intmax_t, static_abs<a>::value> {};
template <intmax_t b>
struct static_gcd<0, b> : integral_constant<intmax_t, static_abs<b>::value> {};


template <intmax_t a, intmax_t b>
struct safe_multiply {
private:
    static constexpr uintmax_t half_range = static_cast<uintmax_t>(1) << (sizeof(intmax_t) * 4);

    static const uintmax_t a_low = static_abs<a>::value % half_range;
    static const uintmax_t a_high = static_abs<a>::value / half_range;
    static const uintmax_t b_low = static_abs<b>::value % half_range;
    static const uintmax_t b_high = static_abs<b>::value / half_range;

    static_assert(a_high == 0 || b_high == 0, "overflow in multiplication");
    static_assert(a_low * b_high + b_low * a_high < (half_range >> 1), "overflow in multiplication");
    static_assert(b_low * a_low <= numeric_limits<intmax_t>::max(), "overflow in multiplication");
    static_assert((a_low * b_high + b_low * a_high) * half_range <= numeric_limits<intmax_t>::max() -  b_low * a_low, "overflow in multiplication");

public:
    static const intmax_t value = a * b;
};

template <uintmax_t high1, uintmax_t low1, uintmax_t high2, uintmax_t low2>
struct big_less : integral_constant<bool, (high1 < high2 || (high1 == high2 && low1 < low2))> {};

template <uintmax_t high1, uintmax_t low1, uintmax_t high2, uintmax_t low2>
struct big_add {
    static constexpr uintmax_t result_low = low1 + low2;
    static constexpr uintmax_t result_high = (high1 + high2 + (low1 + low2 < low1));
};

template <uintmax_t high1, uintmax_t low1, uintmax_t high2, uintmax_t low2>
struct big_sub {
    static_assert(!big_less<high1, low1, high2, low2>::value, "Internal library error");
    static constexpr uintmax_t result_low = low1 - low2;
    static constexpr uintmax_t result_high = (high1 - high2 - (low1 < low2));
};

template <uintmax_t x, uintmax_t y>
struct big_mul {
private:
    static constexpr uintmax_t half_range = static_cast<uintmax_t>(1) << (sizeof(intmax_t) * 4);
    static constexpr uintmax_t x_low = x % half_range;
    static constexpr uintmax_t x_high = x / half_range;
    static constexpr uintmax_t y_low = y % half_range;
    static constexpr uintmax_t y_high = y / half_range;
    static constexpr uintmax_t low_product = x_low * y_low;
    static constexpr uintmax_t cross_product1 = x_low * y_high;
    static constexpr uintmax_t cross_product2 = x_high * y_low;
    static constexpr uintmax_t cross_sum = cross_product1 + cross_product2;
    static constexpr uintmax_t cross_sum_low = cross_sum * half_range;
    static constexpr uintmax_t cross_sum_high = cross_sum / half_range + ((cross_sum < cross_product1) ? half_range : 0);
    typedef big_add<cross_sum_high, cross_sum_low, x_high * y_high, low_product> result_type;
public:
    static constexpr uintmax_t result_high = result_type::result_high;
    static constexpr uintmax_t result_low = result_type::result_low;
};

MSTL_BEGIN_INNER__
template <uintmax_t num_high, uintmax_t num_low, uintmax_t den>
struct big_div_impl {
private:
    static_assert(den >= (static_cast<uintmax_t>(1) << (sizeof(intmax_t) * 8 - 1)), "Internal library error");
    static_assert(num_high < den, "Internal library error");
    static constexpr uintmax_t half_range = static_cast<uintmax_t>(1) << (sizeof(intmax_t) * 4);
    static constexpr uintmax_t den_high = den / half_range;
    static constexpr uintmax_t den_low = den % half_range;

    static constexpr uintmax_t quot_high1 = num_high / den_high;
    static constexpr uintmax_t rem_high1 = num_high % den_high;
    static constexpr uintmax_t temp1 = quot_high1 * den_low;
    static constexpr uintmax_t rem_high2 = rem_high1 * half_range + num_low / half_range;
    static constexpr uintmax_t rem_high3 = rem_high2 + den;
    static constexpr uintmax_t rem_high_final = ((rem_high2 < temp1) ? ((rem_high3 >= den) && (rem_high3 < temp1)) ? (rem_high3 + den) : rem_high3 : rem_high2) - temp1;
    static constexpr uintmax_t quot_high_final = quot_high1 - ((rem_high2 < temp1) ? ((rem_high3 >= den) && (rem_high3 < temp1)) ? 2 : 1 : 0);
    static constexpr uintmax_t quot_low1 = rem_high_final / den_high;
    static constexpr uintmax_t rem_low1 = rem_high_final % den_high;
    static constexpr uintmax_t temp2 = quot_low1 * den_low;
    static constexpr uintmax_t rem_low2 = rem_low1 * half_range + num_low % half_range;
    static constexpr uintmax_t rem_low3 = rem_low2 + den;
    static constexpr uintmax_t rem_low_final = ((rem_low2 < temp2) ? ((rem_low3 >= den) && (rem_low3 < temp2)) ? (rem_low3 + den) : rem_low3 : rem_low2) - temp2;
    static constexpr uintmax_t quot_low_final = quot_low1 - ((rem_low2 < temp2) ? ((rem_low3 >= den) && (rem_low3 < temp2)) ? 2 : 1 : 0);

public:
    static constexpr uintmax_t quotient = quot_high_final * half_range + quot_low_final;
    static constexpr uintmax_t remainder = rem_low_final;

private:
    using product = big_mul<quotient, den>;
    using sum = big_add<product::result_high, product::result_low, 0, remainder>;
    static_assert(sum::result_high == num_high && sum::result_low == num_low, "Internal library error");
};
MSTL_END_INNER__

template <uintmax_t num_high, uintmax_t num_low, uintmax_t den>
struct big_div {
private:
    static_assert(den != 0, "Internal library error");
    static_assert(sizeof (uintmax_t) == sizeof (unsigned long long), "__builtin_clzll is unsafe on your platform.");
    static constexpr int leading_zeros = __builtin_clzll(den);
    static constexpr int complement_shift = sizeof(uintmax_t) * 8 - leading_zeros;
    static constexpr int actual_shift = (leading_zeros != 0) ? complement_shift : 0;
    static constexpr uintmax_t shift_factor1 = static_cast<uintmax_t>(1) << leading_zeros;
    static constexpr uintmax_t shift_factor2 = static_cast<uintmax_t>(1) << actual_shift;
    static constexpr uintmax_t scaled_den = den * shift_factor1;
    static constexpr uintmax_t scaled_num_low = num_low * shift_factor1;
    static constexpr uintmax_t num_high_shifted = (num_high % den) * shift_factor1;
    static constexpr uintmax_t num_low_high = (leading_zeros != 0) ? (num_low / shift_factor2) : 0;
    static constexpr uintmax_t scaled_num_high = num_high_shifted + num_low_high;
    using division_result = _INNER big_div_impl<scaled_num_high, scaled_num_low, scaled_den>;

public:
    static constexpr uintmax_t quotient_high = num_high / den;
    static constexpr uintmax_t quotient_low = division_result::quotient;
    static constexpr uintmax_t remainder = division_result::remainder / shift_factor1;

private:
    using product_low = big_mul<quotient_low, den>;
    using product_high = big_mul<quotient_high, den>;
    using total_sum = big_add<product_low::result_high, product_low::result_low, product_high::result_low, remainder>;

    static_assert(product_high::result_high == 0, "Internal library error");
    static_assert(total_sum::result_high >= product_low::result_high, "Internal library error");

    static_assert(total_sum::result_high == num_high && total_sum::result_low == num_low, "Internal library error");
    static_assert(remainder < den, "Internal library error");
};


template <intmax_t numerator, intmax_t denominator = 1>
struct ratio {
    static_assert(denominator != 0, "denominator cannot be zero");
    static_assert(numerator > numeric_limits<intmax_t>::min() && denominator > numeric_limits<intmax_t>::min(), "out of range");

    static constexpr intmax_t num = numerator * static_sign<denominator>::value / static_gcd<numerator, denominator>::value;
    static constexpr intmax_t den = static_abs<denominator>::value / static_gcd<numerator, denominator>::value;

    using type = ratio<num, den>;
};

template <intmax_t numerator, intmax_t denominator>
constexpr intmax_t ratio<numerator, denominator>::num;

template <intmax_t numerator, intmax_t denominator>
constexpr intmax_t ratio<numerator, denominator>::den;


MSTL_BEGIN_INNER__
template <typename ratio1, typename ratio2>
struct ratio_multiply_impl {
private:
    static const intmax_t gcd1 = static_gcd<ratio1::num, ratio2::den>::value;
    static const intmax_t gcd2 = static_gcd<ratio2::num, ratio1::den>::value;

public:
    using type = ratio<
        safe_multiply<(ratio1::num / gcd1), (ratio2::num / gcd2)>::value,
        safe_multiply<(ratio1::den / gcd2), (ratio2::den / gcd1)>::value>;

    static constexpr intmax_t num = type::num;
    static constexpr intmax_t den = type::den;
};
template <typename ratio1, typename ratio2>
constexpr intmax_t ratio_multiply_impl<ratio1, ratio2>::num;
template <typename ratio1, typename ratio2>
constexpr intmax_t ratio_multiply_impl<ratio1, ratio2>::den;
MSTL_END_INNER__

template <typename ratio1, typename ratio2>
using ratio_multiply = typename _INNER ratio_multiply_impl<ratio1, ratio2>::type;


MSTL_BEGIN_INNER__
template <typename ratio1, typename ratio2>
struct ratio_divide_impl {
    static_assert(ratio2::num != 0, "division by 0");

    using type = typename ratio_multiply_impl<ratio1, ratio<ratio2::den, ratio2::num>>::type;

    static constexpr intmax_t num = type::num;
    static constexpr intmax_t den = type::den;
};
template <typename ratio1, typename ratio2>
constexpr intmax_t ratio_divide_impl<ratio1, ratio2>::num;
template <typename ratio1, typename ratio2>
constexpr intmax_t ratio_divide_impl<ratio1, ratio2>::den;
MSTL_END_INNER__

template<typename ratio1, typename ratio2>
using ratio_divide = typename _INNER ratio_divide_impl<ratio1, ratio2>::type;


template <typename ratio1, typename ratio2>
struct ratio_equal : integral_constant<bool, ratio1::num == ratio2::num && ratio1::den == ratio2::den> {};

template <typename ratio1, typename ratio2>
struct ratio_not_equal : integral_constant<bool, !ratio_equal<ratio1, ratio2>::value> {};


MSTL_BEGIN_INNER__
template <typename ratio1, typename ratio2,
    typename left_product = big_mul<ratio1::num, ratio2::den>,
    typename right_product = big_mul<ratio2::num, ratio1::den>>
struct ratio_less_impl_base : integral_constant<bool, big_less<left_product::result_high, left_product::result_low, right_product::result_high, right_product::result_low>::value> {};

template <typename ratio1, typename ratio2,
    bool has_zero_or_different_sign = (ratio1::num == 0 || ratio2::num == 0 || (static_sign<ratio1::num>::value != static_sign<ratio2::num>::value)),
    bool both_negative = (static_sign<ratio1::num>::value == -1 && static_sign<ratio2::num>::value == -1)>
struct ratio_less_impl : ratio_less_impl_base<ratio1, ratio2>::type {};

template <typename ratio1, typename ratio2>
struct ratio_less_impl<ratio1, ratio2, true, false> : integral_constant<bool, ratio1::num < ratio2::num> {};

template <typename ratio1, typename ratio2>
struct ratio_less_impl<ratio1, ratio2, false, true>
    : ratio_less_impl_base<ratio<-ratio2::num, ratio2::den>, ratio<-ratio1::num, ratio1::den>>::type {};
MSTL_END_INNER__


template <typename ratio1, typename ratio2>
struct ratio_less : _INNER ratio_less_impl<ratio1, ratio2>::type {};

template <typename ratio1, typename ratio2>
struct ratio_less_equal : integral_constant<bool, !ratio_less<ratio2, ratio1>::value> {};


template <typename ratio1, typename ratio2>
struct ratio_greater : integral_constant<bool, ratio_less<ratio2, ratio1>::value> {};

template <typename ratio1, typename ratio2>
struct ratio_greater_equal : integral_constant<bool, !ratio_less<ratio1, ratio2>::value> {};


template <typename ratio1, typename ratio2>
MSTL_INLINE17 constexpr bool ratio_equal_v = ratio_equal<ratio1, ratio2>::value;
template <typename ratio1, typename ratio2>
MSTL_INLINE17 constexpr bool ratio_not_equal_v = ratio_not_equal<ratio1, ratio2>::value;
template <typename ratio1, typename ratio2>
MSTL_INLINE17 constexpr bool ratio_less_v = ratio_less<ratio1, ratio2>::value;
template <typename ratio1, typename ratio2>
MSTL_INLINE17 constexpr bool ratio_less_equal_v = ratio_less_equal<ratio1, ratio2>::value;
template <typename ratio1, typename ratio2>
MSTL_INLINE17 constexpr bool ratio_greater_v = ratio_greater<ratio1, ratio2>::value;
template <typename ratio1, typename ratio2>
MSTL_INLINE17 constexpr bool ratio_greater_equal_v = ratio_greater_equal<ratio1, ratio2>::value;


MSTL_BEGIN_INNER__

template <typename ratio1, typename ratio2,
    bool ratio1_non_negative = (ratio1::num >= 0),
    bool ratio2_non_negative = (ratio2::num >= 0),
    bool abs_ratio1_less_than_abs_ratio2 = ratio_less<
        ratio<static_abs<ratio1::num>::value, ratio1::den>,
        ratio<static_abs<ratio2::num>::value, ratio2::den>
    >::value>
struct ratio_add_impl {
private:
    using negative_result = typename ratio_add_impl<
        ratio<-ratio1::num, ratio1::den>,
        ratio<-ratio2::num, ratio2::den>>
    ::type;
public:
    using type = ratio<-negative_result::num, negative_result::den>;
};

template <typename ratio1, typename ratio2, bool abs_less>
struct ratio_add_impl<ratio1, ratio2, true, true, abs_less> {
private:
    static constexpr uintmax_t gcd_val = static_gcd<ratio1::den, ratio2::den>::value;
    static constexpr uintmax_t den2_scaled = ratio2::den / gcd_val;
    using denominator_product = big_mul<ratio1::den, den2_scaled>;
    using numerator1_scaled = big_mul<ratio1::num, ratio2::den / gcd_val>;
    using numerator2_scaled = big_mul<ratio2::num, ratio1::den / gcd_val>;
    using numerator_sum = big_add<
        numerator1_scaled::result_high,
        numerator1_scaled::result_low,
        numerator2_scaled::result_high,
        numerator2_scaled::result_low>;
    static_assert(numerator_sum::result_high >= numerator1_scaled::result_high, "Internal library error");
    using numerator_gcd_reduced = big_div<numerator_sum::result_high, numerator_sum::result_low, gcd_val>;
    static constexpr uintmax_t gcd_val2 = static_gcd<numerator_gcd_reduced::remainder, gcd_val>::value;
    using numerator_final = big_div<numerator_sum::result_high, numerator_sum::result_low, gcd_val2>;
    static_assert(numerator_final::remainder == 0, "Internal library error");
    static_assert(numerator_final::quotient_high == 0 &&
        numerator_final::quotient_low <= numeric_limits<intmax_t>::max(), "overflow in addition");
    using denominator_final = big_mul<ratio1::den / gcd_val2, den2_scaled>;
    static_assert(denominator_final::result_high == 0 &&
        denominator_final::result_low <= numeric_limits<intmax_t>::max(), "overflow in addition");
public:
    using type = ratio<numerator_final::quotient_low, denominator_final::result_low>;
};

template <typename ratio1, typename ratio2>
struct ratio_add_impl<ratio1, ratio2, false, true, true> : ratio_add_impl<ratio2, ratio1> {};

template <typename ratio1, typename ratio2>
struct ratio_add_impl<ratio1, ratio2, true, false, false> {
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
    using numerator_gcd_reduced = big_div<numerator_diff::result_high, numerator_diff::result_low, gcd_val>;
    static constexpr uintmax_t gcd_val2 = static_gcd<numerator_gcd_reduced::remainder, gcd_val>::value;
    using numerator_final = big_div<numerator_diff::result_high, numerator_diff::result_low, gcd_val2>;
    static_assert(numerator_final::remainder == 0, "Internal library error");
    static_assert(numerator_final::quotient_high == 0 &&
        numerator_final::quotient_low <= numeric_limits<intmax_t>::max(), "overflow in addition");
    using denominator_final = big_mul<ratio1::den / gcd_val2, den2_scaled>;
    static_assert(denominator_final::result_high == 0 &&
        denominator_final::result_low <= numeric_limits<intmax_t>::max(), "overflow in addition");
public:
    using type = ratio<numerator_final::quotient_low, denominator_final::result_low>;
};

template <typename ratio1, typename ratio2>
struct ratio_add {
    using type = typename ratio_add_impl<ratio1, ratio2>::type;
    static constexpr intmax_t num = type::num;
    static constexpr intmax_t den = type::den;
};
template <typename ratio1, typename ratio2>
constexpr intmax_t ratio_add<ratio1, ratio2>::num;
template <typename ratio1, typename ratio2>
constexpr intmax_t ratio_add<ratio1, ratio2>::den;

MSTL_END_INNER__

template <typename ratio1, typename ratio2>
using ratio_add = typename _INNER ratio_add<ratio1, ratio2>::type;


MSTL_BEGIN_INNER__
template <typename ratio1, typename ratio2>
struct ratio_subtract {
    using type = typename ratio_add<ratio1, ratio<-ratio2::num, ratio2::den>>::type;

    static constexpr intmax_t num = type::num;
    static constexpr intmax_t den = type::den;
};
template <typename ratio1, typename ratio2>
constexpr intmax_t ratio_subtract<ratio1, ratio2>::num;
template <typename ratio1, typename ratio2>
constexpr intmax_t ratio_subtract<ratio1, ratio2>::den;
MSTL_END_INNER__

template <typename ratio1, typename ratio2>
using ratio_subtract = typename _INNER ratio_subtract<ratio1, ratio2>::type;


using atto  = ratio<1, 1000000000000000000>;
using femto = ratio<1,    1000000000000000>;
using pico  = ratio<1,       1000000000000>;
using nano  = ratio<1,          1000000000>;
using micro = ratio<1,             1000000>;
using milli = ratio<1,                1000>;
using centi = ratio<1,                 100>;
using deci  = ratio<1,                  10>;
using deca  = ratio<10,                  1>;
using hecto = ratio<100,                 1>;
using kilo  = ratio<1000,                1>;
using mega  = ratio<1000000,             1>;
using giga  = ratio<1000000000,          1>;
using tera  = ratio<1000000000000,       1>;
using peta  = ratio<1000000000000000,    1>;
using exa   = ratio<1000000000000000000, 1>;

MSTL_END_NAMESPACE__
#endif // MSTL_RATIO_HPP__