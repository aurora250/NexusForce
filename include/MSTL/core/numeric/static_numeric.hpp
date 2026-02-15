#ifndef MSTL_CORE_NUMERIC_STATIC_NUMERIC_HPP__
#define MSTL_CORE_NUMERIC_STATIC_NUMERIC_HPP__

/**
 * @file static_numeric.hpp
 * @brief MSTL静态数值计算
 *
 * 此文件提供了编译期数值计算的工具，包括符号、绝对值、最大公约数、
 * 大整数算术运算和整数解析等功能。所有计算都在编译期完成。
 */

#include "MSTL/core/typeinfo/type_traits.hpp"
#include "MSTL/core/memory/bit.hpp"
#include "MSTL/core/numeric/numeric_traits.hpp"
MSTL_BEGIN_NAMESPACE__

/**
 * @defgroup StaticArithmetic 静态算术运算
 * @brief 编译期的基本算术运算
 * @{
 */

/**
 * @struct static_sign
 * @brief 计算整数的符号
 * @tparam Numerator 分子值
 *
 * 返回整数的符号：负数返回-1，非负数返回1。
 */
template <intmax_t Numerator>
struct static_sign : integral_constant<intmax_t, (Numerator < 0) ? -1 : 1> {};

/**
 * @struct static_abs
 * @brief 计算整数的绝对值
 * @tparam Value 整数值
 *
 * 返回整数的绝对值，编译期计算。
 */
template <intmax_t Value>
struct static_abs : integral_constant<intmax_t, Value * static_sign<Value>::value> {};

/**
 * @struct static_gcd
 * @brief 计算两个整数的最大公约数
 * @tparam A 第一个整数
 * @tparam B 第二个整数
 *
 * 使用欧几里得算法递归计算最大公约数。
 * 基础情况：当B为0时，返回A的绝对值。
 */
template <intmax_t A, intmax_t B>
struct static_gcd : static_gcd<B, (A % B)> {};

/// @cond
template <intmax_t A>
struct static_gcd<A, 0> : integral_constant<intmax_t, static_abs<A>::value> {};
template <intmax_t B>
struct static_gcd<0, B> : integral_constant<intmax_t, static_abs<B>::value> {};
/// @endcond


/**
 * @struct safe_multiply
 * @brief 安全的编译期乘法，检查溢出
 * @tparam A 第一个乘数
 * @tparam B 第二个乘数
 *
 * 在编译期检查乘法是否会导致溢出，如果会溢出则触发静态断言。
 */
template <intmax_t A, intmax_t B>
struct safe_multiply {
private:
    static constexpr uintmax_t half_range = static_cast<uintmax_t>(1) << (sizeof(intmax_t) * 4);

    static const uintmax_t a_low = static_abs<A>::value % half_range;
    static const uintmax_t a_high = static_abs<A>::value / half_range;
    static const uintmax_t b_low = static_abs<B>::value % half_range;
    static const uintmax_t b_high = static_abs<B>::value / half_range;

    static_assert(a_high == 0 || b_high == 0, "overflow in multiplication");
    static_assert(a_low * b_high + b_low * a_high < (half_range >> 1), "overflow in multiplication");
    static_assert(b_low * a_low <= numeric_traits<intmax_t>::max(), "overflow in multiplication");
    static_assert((a_low * b_high + b_low * a_high) * half_range <= numeric_traits<intmax_t>::max() -  b_low * a_low, "overflow in multiplication");

public:
    static const intmax_t value = A * B;
};

/**
 * @struct big_less
 * @brief 比较两个由高位和低位部分组成的无符号大整数
 * @tparam High1 第一个整数的高位
 * @tparam Low1 第一个整数的低位
 * @tparam High2 第二个整数的高位
 * @tparam Low2 第二个整数的低位
 *
 * 比较两个大整数的大小，返回true如果第一个小于第二个。
 */
template <uintmax_t High1, uintmax_t Low1, uintmax_t High2, uintmax_t Low2>
struct big_less : integral_constant<bool, (High1 < High2 || (High1 == High2 && Low1 < Low2))> {};

/**
 * @struct big_add
 * @brief 无符号大整数加法
 * @tparam High1 第一个加数的高位
 * @tparam Low1 第一个加数的低位
 * @tparam High2 第二个加数的高位
 * @tparam Low2 第二个加数的低位
 *
 * 计算两个大整数的和，考虑低位相加产生的进位。
 */
template <uintmax_t High1, uintmax_t Low1, uintmax_t High2, uintmax_t Low2>
struct big_add {
    static constexpr uintmax_t result_low = Low1 + Low2;  ///< 加法结果的低位
    static constexpr uintmax_t result_high = (High1 + High2 + (Low1 + Low2 < Low1));  ///< 加法结果的高位，包含进位
};

/**
 * @struct big_sub
 * @brief 无符号大整数减法
 * @tparam High1 被减数的高位
 * @tparam Low1 被减数的低位
 * @tparam High2 减数的高位
 * @tparam Low2 减数的低位
 *
 * 计算两个大整数的差，要求被减数不小于减数。
 */
template <uintmax_t High1, uintmax_t Low1, uintmax_t High2, uintmax_t Low2>
struct big_sub {
    static_assert(!big_less<High1, Low1, High2, Low2>::value, "Internal library error");
    static constexpr uintmax_t result_low = Low1 - Low2;  ///< 减法结果的低位
    static constexpr uintmax_t result_high = (High1 - High2 - (Low1 < Low2));  ///< 减法结果的高位，考虑借位
};

/**
 * @struct big_mul
 * @brief 无符号大整数乘法
 * @tparam X 第一个乘数
 * @tparam Y 第二个乘数
 *
 * 使用分治策略计算两个大整数的乘积，防止溢出。
 */
template <uintmax_t X, uintmax_t Y>
struct big_mul {
private:
    static constexpr uintmax_t half_range = static_cast<uintmax_t>(1) << (sizeof(intmax_t) * 4);
    static constexpr uintmax_t x_low = X % half_range;
    static constexpr uintmax_t x_high = X / half_range;
    static constexpr uintmax_t y_low = Y % half_range;
    static constexpr uintmax_t y_high = Y / half_range;
    static constexpr uintmax_t low_product = x_low * y_low;  ///< 低位乘积
    static constexpr uintmax_t cross_product1 = x_low * y_high;  ///< 交叉乘积1
    static constexpr uintmax_t cross_product2 = x_high * y_low;  ///< 交叉乘积2
    static constexpr uintmax_t cross_sum = cross_product1 + cross_product2;  ///< 交叉和
    static constexpr uintmax_t cross_sum_low = cross_sum * half_range;  ///< 交叉和的低位部分
    static constexpr uintmax_t cross_sum_high = cross_sum / half_range + ((cross_sum < cross_product1) ? half_range : 0);  ///< 交叉和的高位部分
    using result_type = big_add<cross_sum_high, cross_sum_low, x_high * y_high, low_product>;  ///< 最终结果类型
public:
    static constexpr uintmax_t result_high = result_type::result_high;  ///< 乘法结果的高位
    static constexpr uintmax_t result_low = result_type::result_low;    ///< 乘法结果的低位
};

/// @cond
MSTL_BEGIN_INNER__
/**
 * @struct __big_div_impl
 * @brief 大整数除法的内部实现
 * @tparam NumHigh 被除数的高位
 * @tparam NumLow 被除数的低位
 * @tparam Den 除数
 *
 * 使用长除法算法实现大整数除法，返回商和余数。
 */
template <uintmax_t NumHigh, uintmax_t NumLow, uintmax_t Den>
struct __big_div_impl {
private:
    static_assert(Den >= (static_cast<uintmax_t>(1) << (sizeof(intmax_t) * 8 - 1)), "Internal library error");
    static_assert(NumHigh < Den, "Internal library error");
    static constexpr uintmax_t half_range = static_cast<uintmax_t>(1) << (sizeof(intmax_t) * 4);
    static constexpr uintmax_t den_high = Den / half_range;
    static constexpr uintmax_t den_low = Den % half_range;

    static constexpr uintmax_t quot_high1 = NumHigh / den_high;
    static constexpr uintmax_t rem_high1 = NumHigh % den_high;
    static constexpr uintmax_t temp1 = quot_high1 * den_low;
    static constexpr uintmax_t rem_high2 = rem_high1 * half_range + NumLow / half_range;
    static constexpr uintmax_t rem_high3 = rem_high2 + Den;
    static constexpr uintmax_t rem_high_final = ((rem_high2 < temp1) ? ((rem_high3 >= Den) && (rem_high3 < temp1)) ? (rem_high3 + Den) : rem_high3 : rem_high2) - temp1;
    static constexpr uintmax_t quot_high_final = quot_high1 - ((rem_high2 < temp1) ? ((rem_high3 >= Den) && (rem_high3 < temp1)) ? 2 : 1 : 0);
    static constexpr uintmax_t quot_low1 = rem_high_final / den_high;
    static constexpr uintmax_t rem_low1 = rem_high_final % den_high;
    static constexpr uintmax_t temp2 = quot_low1 * den_low;
    static constexpr uintmax_t rem_low2 = rem_low1 * half_range + NumLow % half_range;
    static constexpr uintmax_t rem_low3 = rem_low2 + Den;
    static constexpr uintmax_t rem_low_final = ((rem_low2 < temp2) ? ((rem_low3 >= Den) && (rem_low3 < temp2)) ? (rem_low3 + Den) : rem_low3 : rem_low2) - temp2;
    static constexpr uintmax_t quot_low_final = quot_low1 - ((rem_low2 < temp2) ? ((rem_low3 >= Den) && (rem_low3 < temp2)) ? 2 : 1 : 0);

public:
    static constexpr uintmax_t quotient = quot_high_final * half_range + quot_low_final;  ///< 商
    static constexpr uintmax_t remainder = rem_low_final;  ///< 余数

private:
    using product = big_mul<quotient, Den>;
    using sum = big_add<product::result_high, product::result_low, 0, remainder>;
    static_assert(sum::result_high == NumHigh && sum::result_low == NumLow, "Internal library error");
};
MSTL_END_INNER__
/// @endcond

/**
 * @struct big_div
 * @brief 大整数除法
 * @tparam NumHigh 被除数的高位
 * @tparam NumLow 被除数的低位
 * @tparam Den 除数
 *
 * 使用位移优化的大整数除法，提高计算效率。
 */
template <uintmax_t NumHigh, uintmax_t NumLow, uintmax_t Den>
struct big_div {
private:
    static_assert(Den != 0, "Internal library error");
    static_assert(sizeof (uintmax_t) == sizeof (unsigned long long), "clzll is unsafe on your platform.");
    static constexpr int leading_zeros = _MSTL countl_zero(Den);  ///< 除数前导零个数
    static constexpr int complement_shift = sizeof(uintmax_t) * 8 - leading_zeros;  ///< 补码位移
    static constexpr int actual_shift = (leading_zeros != 0) ? complement_shift : 0;  ///< 实际位移
    static constexpr uintmax_t shift_factor1 = static_cast<uintmax_t>(1) << leading_zeros;  ///< 位移因子1
    static constexpr uintmax_t shift_factor2 = static_cast<uintmax_t>(1) << actual_shift;  ///< 位移因子2
    static constexpr uintmax_t scaled_den = Den * shift_factor1;  ///< 缩放后的除数
    static constexpr uintmax_t scaled_num_low = NumLow * shift_factor1;  ///< 缩放后的被除数低位
    static constexpr uintmax_t num_high_shifted = (NumHigh % Den) * shift_factor1;  ///< 缩放后的被除数高位
    static constexpr uintmax_t num_low_high = (leading_zeros != 0) ? (NumLow / shift_factor2) : 0;  ///< 被除数低位的高位部分
    static constexpr uintmax_t scaled_num_high = num_high_shifted + num_low_high;  ///< 缩放后的被除数高位
    using division_result = _INNER __big_div_impl<scaled_num_high, scaled_num_low, scaled_den>;  ///< 除法结果

public:
    static constexpr uintmax_t quotient_high = NumHigh / Den;  ///< 商的高位
    static constexpr uintmax_t quotient_low = division_result::quotient;  ///< 商的低位
    static constexpr uintmax_t remainder = division_result::remainder / shift_factor1;  ///< 余数

private:
    using product_low = big_mul<quotient_low, Den>;
    using product_high = big_mul<quotient_high, Den>;
    using total_sum = big_add<product_low::result_high, product_low::result_low, product_high::result_low, remainder>;

    static_assert(product_high::result_high == 0, "Internal library error");
    static_assert(total_sum::result_high >= product_low::result_high, "Internal library error");

    static_assert(total_sum::result_high == NumHigh && total_sum::result_low == NumLow, "Internal library error");
    static_assert(remainder < Den, "Internal library error");
};

/** @} */ // StaticArithmetic

/**
 * @defgroup StaticCharDigit 静态字符数字操作
 * @brief 字符和数字值之间的操作
 * @{
 */

/**
 * @struct static_char_digit
 * @brief 将字符转换为指定进制下的数字值
 * @tparam Base 进制
 * @tparam Digit 字符数字
 *
 * 将字符'0'-'9'、'a'-'f'、'A'-'F'转换为对应进制的数值。
 * 包含is_valid成员表示转换是否有效。
 */
template <uint32_t Base, char Digit>
struct static_char_digit;

template <uint32_t Base>
struct static_char_digit<Base, '0'> : uint32_constant<0> {
    using is_valid = true_type;
};

template <uint32_t Base>
struct static_char_digit<Base, '1'> : uint32_constant<1> {
    using is_valid = true_type;
};

/// @cond
MSTL_BEGIN_INNER__
/**
 * @struct __static_char_digit_aux
 * @brief 静态字符数字转换的辅助结构体
 * @tparam Base 进制
 * @tparam Value 数字值
 *
 * 检查数字值是否小于进制基数，确保有效性。
 */
template <uint32_t Base, uint32_t Value>
struct __static_char_digit_aux : uint32_constant<Value> {
    static_assert(Base > Value, "Invalid digit for given base");
    using is_valid = true_type;
};
MSTL_END_INNER__
/// @endcond

template <uint32_t Base>
struct static_char_digit<Base, '2'> : _INNER __static_char_digit_aux<Base, 2> {};

template <uint32_t Base>
struct static_char_digit<Base, '3'> : _INNER __static_char_digit_aux<Base, 3> {};

template <uint32_t Base>
struct static_char_digit<Base, '4'> : _INNER __static_char_digit_aux<Base, 4> {};

template <uint32_t Base>
struct static_char_digit<Base, '5'> : _INNER __static_char_digit_aux<Base, 5> {};

template <uint32_t Base>
struct static_char_digit<Base, '6'> : _INNER __static_char_digit_aux<Base, 6> {};

template <uint32_t Base>
struct static_char_digit<Base, '7'> : _INNER __static_char_digit_aux<Base, 7> {};

template <uint32_t Base>
struct static_char_digit<Base, '8'> : _INNER __static_char_digit_aux<Base, 8> {};

template <uint32_t Base>
struct static_char_digit<Base, '9'> : _INNER __static_char_digit_aux<Base, 9> {};

template <uint32_t Base>
struct static_char_digit<Base, 'a'> : _INNER __static_char_digit_aux<Base, 0xa> {};

template <uint32_t Base>
struct static_char_digit<Base, 'A'> : _INNER __static_char_digit_aux<Base, 0xa> {};

template <uint32_t Base>
struct static_char_digit<Base, 'b'> : _INNER __static_char_digit_aux<Base, 0xb> {};

template <uint32_t Base>
struct static_char_digit<Base, 'B'> : _INNER __static_char_digit_aux<Base, 0xb> {};

template <uint32_t Base>
struct static_char_digit<Base, 'c'> : _INNER __static_char_digit_aux<Base, 0xc> {};

template <uint32_t Base>
struct static_char_digit<Base, 'C'> : _INNER __static_char_digit_aux<Base, 0xc> {};

template <uint32_t Base>
struct static_char_digit<Base, 'd'> : _INNER __static_char_digit_aux<Base, 0xd> {};

template <uint32_t Base>
struct static_char_digit<Base, 'D'> : _INNER __static_char_digit_aux<Base, 0xd> {};

template <uint32_t Base>
struct static_char_digit<Base, 'e'> : _INNER __static_char_digit_aux<Base, 0xe> {};

template <uint32_t Base>
struct static_char_digit<Base, 'E'> : _INNER __static_char_digit_aux<Base, 0xe> {};

template <uint32_t Base>
struct static_char_digit<Base, 'f'> : _INNER __static_char_digit_aux<Base, 0xf> {};

template <uint32_t Base>
struct static_char_digit<Base, 'F'> : _INNER __static_char_digit_aux<Base, 0xf> {};

template <uint32_t Base>
struct static_char_digit<Base, '\''> : uint32_constant<0> {
    using is_valid = false_type;
};


/// @cond
MSTL_BEGIN_INNER__

/**
 * @struct __power_helper
 * @brief 幂计算的辅助结构体
 * @tparam Base 进制基数
 * @tparam ThisDigit 当前字符
 * @tparam RestDigits 剩余字符
 *
 * 递归计算字符串表示的数值的幂。
 */
template <uint32_t Base, char ThisDigit, char... RestDigits>
struct __power_helper {
private:
    using next_power = typename __power_helper<Base, RestDigits...>::type;
    using current_digit = static_char_digit<Base, ThisDigit>;

public:
    using type = uint64_constant<next_power::value * (current_digit::is_valid::value ? Base : 1ULL)>;
};

template <uint32_t Base, char Digit>
struct __power_helper<Base, Digit> {
private:
    using current_digit = static_char_digit<Base, Digit>;

public:
    using type = uint64_constant<current_digit::is_valid::value>;
};

MSTL_END_INNER__
/// @endcond

/**
 * @struct static_power
 * @brief 计算字符串表示的数值的幂
 * @tparam Base 进制
 * @tparam Digits 数字字符序列
 *
 * 计算每个数字位置的权重，即基数的幂。
 */
template <uint32_t Base, char... Digits>
struct static_power : _INNER __power_helper<Base, Digits...>::type {};

template <uint32_t Base>
struct static_power<Base> : uint64_constant<0> {};


/// @cond
MSTL_BEGIN_INNER__

/**
 * @struct __number_aux
 * @brief 数值转换的辅助结构体
 * @tparam Base 进制
 * @tparam ThisPower 当前位置的权重
 * @tparam ThisDigit 当前字符
 * @tparam RestDigits 剩余字符
 *
 * 递归将字符串表示的数值转换为编译期数值。
 */
template <uint32_t Base, uint64_t ThisPower, char ThisDigit, char... RestDigits>
struct __number_aux {
private:
    using digit_value = static_char_digit<Base, ThisDigit>;
    using next_number = __number_aux<Base,
        digit_value::is_valid::value ? ThisPower / Base : ThisPower,
        RestDigits...
    >;

public:
    using type = uint64_constant<ThisPower * digit_value::value + next_number::type::value>;

    static_assert(
        (type::value / ThisPower) == digit_value::value,
        "Integer literal does not fit in unsigned long long"
    );
};

template <uint32_t Base, uint64_t ThisPower, char ThisDigit, char... RestDigits>
struct __number_aux<Base, ThisPower, '\'', ThisDigit, RestDigits...>
    : __number_aux<Base, ThisPower, ThisDigit, RestDigits...> {};

template <uint32_t Base, char Digit>
struct __number_aux<Base, 1ULL, Digit> {
    using type = uint64_constant<static_char_digit<Base, Digit>::value>;
};

MSTL_END_INNER__
/// @endcond

/**
 * @struct static_number
 * @brief 将字符串表示的数值转换为编译期数值
 * @tparam Base 进制
 * @tparam Digits 数字字符序列
 *
 * 将给定进制的数字字符串转换为编译期数值。
 */
template <uint32_t Base, char... Digits>
struct static_number :
    _INNER __number_aux<Base, static_power<Base, Digits...>::value, Digits...>::type {};

template <uint32_t Base>
struct static_number<Base> : uint64_constant<0> {};


/**
 * @struct static_parse_int
 * @brief 根据前缀自动识别进制并解析整数
 * @tparam Digits 数字字符序列，可能包含前缀
 *
 * 根据整数字面量语法自动识别进制：
 * - 0b或0B开头：二进制
 * - 0x或0X开头：十六进制
 * - 0开头：八进制
 * - 其他：十进制
 */
template <char... Digits>
struct static_parse_int;

template <char... Digits>
struct static_parse_int<'0', 'b', Digits...> : static_number<2U, Digits...>::type {};

template <char... Digits>
struct static_parse_int<'0', 'B', Digits...> : static_number<2U, Digits...>::type {};

template <char... Digits>
struct static_parse_int<'0', 'x', Digits...> : static_number<16U, Digits...>::type {};

template <char... Digits>
struct static_parse_int<'0', 'X', Digits...> : static_number<16U, Digits...>::type {};

template <char... Digits>
struct static_parse_int<'0', Digits...> : static_number<8U, Digits...>::type {};

template <char... Digits>
struct static_parse_int : static_number<10U, Digits...>::type {};


/// @cond
MSTL_BEGIN_INNER__

/**
 * @struct __select_int_base
 * @brief 选择合适整数类型的辅助结构体
 * @tparam Value 数值
 * @tparam IntTypes 候选整数类型列表
 *
 * 递归遍历候选类型列表，选择第一个能容纳该数值的类型。
 */
template <uint64_t Value, typename... IntTypes>
struct __select_int_base;

template <uint64_t Value, typename IntType, typename... RestIntTypes>
struct __select_int_base<Value, IntType, RestIntTypes...>
    : conditional_t<
        (Value <= numeric_traits<IntType>::max()),
        integral_constant<IntType, static_cast<IntType>(Value)>,
        __select_int_base<Value, RestIntTypes...>
    > {};

template <uint64_t Value>
struct __select_int_base<Value> {};

MSTL_END_INNER__
/// @endcond

/**
 * @typedef static_select_int_t
 * @brief 根据数值选择最合适整数类型的类型别名
 * @tparam Digits 数字字符序列
 *
 * 从unsigned数值中选择第一个能容纳解析出的数值的类型。
 */
template <char... Digits>
using static_select_int_t = typename _INNER __select_int_base<
    static_parse_int<Digits...>::value,
    unsigned char,
    unsigned short,
    unsigned int,
    unsigned long,
    unsigned long long
>::type;

/** @} */ // StaticCharDigit

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_NUMERIC_STATIC_NUMERIC_HPP__
