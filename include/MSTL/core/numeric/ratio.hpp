#ifndef MSTL_CORE_NUMERIC_RATIO_HPP__
#define MSTL_CORE_NUMERIC_RATIO_HPP__
#include "static_numeric.hpp"
MSTL_BEGIN_NAMESPACE__

template <intmax_t Numerator, intmax_t Denominator = 1>
struct ratio {
    static_assert(Denominator != 0, "denominator cannot be zero");
    static_assert(Numerator > numeric_limits<intmax_t>::min() && Denominator > numeric_limits<intmax_t>::min(), "out of range");

    static constexpr intmax_t num = Numerator * static_sign<Denominator>::value / static_gcd<Numerator, Denominator>::value;
    static constexpr intmax_t den = static_abs<Denominator>::value / static_gcd<Numerator, Denominator>::value;

    using type = ratio<num, den>;
};

template <intmax_t Numerator, intmax_t Denominator>
constexpr intmax_t ratio<Numerator, Denominator>::num;

template <intmax_t Numerator, intmax_t Denominator>
constexpr intmax_t ratio<Numerator, Denominator>::den;


template <typename>
struct is_ratio : false_type {};

template <intmax_t Numerator, intmax_t Denominator>
struct is_ratio<ratio<Numerator, Denominator>> : true_type {};

template <typename T>
MSTL_INLINE17 constexpr bool is_ratio_v = is_ratio<T>::value;


MSTL_BEGIN_INNER__
template <typename ratio1, typename ratio2>
struct __ratio_multiply_impl {
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
constexpr intmax_t __ratio_multiply_impl<ratio1, ratio2>::num;
template <typename ratio1, typename ratio2>
constexpr intmax_t __ratio_multiply_impl<ratio1, ratio2>::den;
MSTL_END_INNER__

template <typename ratio1, typename ratio2>
using ratio_multiply = typename _INNER __ratio_multiply_impl<ratio1, ratio2>::type;


MSTL_BEGIN_INNER__
template <typename ratio1, typename ratio2>
struct __ratio_divide_impl {
    static_assert(ratio2::num != 0, "division by 0");

    using type = typename __ratio_multiply_impl<ratio1, ratio<ratio2::den, ratio2::num>>::type;

    static constexpr intmax_t num = type::num;
    static constexpr intmax_t den = type::den;
};
template <typename ratio1, typename ratio2>
constexpr intmax_t __ratio_divide_impl<ratio1, ratio2>::num;
template <typename ratio1, typename ratio2>
constexpr intmax_t __ratio_divide_impl<ratio1, ratio2>::den;
MSTL_END_INNER__

template<typename ratio1, typename ratio2>
using ratio_divide = typename _INNER __ratio_divide_impl<ratio1, ratio2>::type;


template <typename ratio1, typename ratio2>
struct ratio_equal : integral_constant<bool, ratio1::num == ratio2::num && ratio1::den == ratio2::den> {};

template <typename ratio1, typename ratio2>
struct ratio_not_equal : integral_constant<bool, !ratio_equal<ratio1, ratio2>::value> {};


MSTL_BEGIN_INNER__
template <typename ratio1, typename ratio2,
    typename left_product = big_mul<ratio1::num, ratio2::den>,
    typename right_product = big_mul<ratio2::num, ratio1::den>>
struct __ratio_less_impl_base : integral_constant<bool, big_less<
    left_product::result_high, left_product::result_low, right_product::result_high, right_product::result_low>::value
> {};

template <typename ratio1, typename ratio2,
    bool has_zero_or_different_sign = (ratio1::num == 0 || ratio2::num == 0 || (static_sign<ratio1::num>::value != static_sign<ratio2::num>::value)),
    bool both_negative = (static_sign<ratio1::num>::value == -1 && static_sign<ratio2::num>::value == -1)>
struct __ratio_less_impl : __ratio_less_impl_base<ratio1, ratio2>::type {};

template <typename ratio1, typename ratio2>
struct __ratio_less_impl<ratio1, ratio2, true, false> : integral_constant<bool, ratio1::num < ratio2::num> {};

template <typename ratio1, typename ratio2>
struct __ratio_less_impl<ratio1, ratio2, false, true>
    : __ratio_less_impl_base<ratio<-ratio2::num, ratio2::den>, ratio<-ratio1::num, ratio1::den>>::type {};
MSTL_END_INNER__


template <typename ratio1, typename ratio2>
struct ratio_less : _INNER __ratio_less_impl<ratio1, ratio2>::type {};

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
struct __ratio_add_impl {
private:
    using negative_result = typename __ratio_add_impl<
        ratio<-ratio1::num, ratio1::den>,
        ratio<-ratio2::num, ratio2::den>>
    ::type;
public:
    using type = ratio<-negative_result::num, negative_result::den>;
};

template <typename ratio1, typename ratio2, bool abs_less>
struct __ratio_add_impl<ratio1, ratio2, true, true, abs_less> {
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
struct __ratio_add_impl<ratio1, ratio2, false, true, true> : __ratio_add_impl<ratio2, ratio1> {};

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
    using type = typename __ratio_add_impl<ratio1, ratio2>::type;
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
#endif // MSTL_CORE_NUMERIC_RATIO_HPP__