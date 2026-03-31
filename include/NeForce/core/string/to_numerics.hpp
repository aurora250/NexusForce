#ifndef NEFORCE_CORE_STRING_TO_NUMERICS_HPP__
#define NEFORCE_CORE_STRING_TO_NUMERICS_HPP__

/**
 * @file to_numerics.hpp
 * @brief 字符串到数值的转换函数
 *
 * 此文件提供了将字符串转换为各种数值类型的函数。
 * 支持整数、浮点数的转换，包括进制转换和错误处理。
 */

#include "NeForce/core/exception/exception.hpp"
#include "NeForce/core/numeric/math.hpp"
#include "NeForce/core/string/string_view.hpp"
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
NEFORCE_BEGIN_NAMESPACE__

/// @cond
NEFORCE_BEGIN_INNER__

/**
 * @brief 将字符串转换为有符号整数
 * @tparam T 有符号整数类型
 * @param sv 要转换的字符串视图
 * @param endptr 指向转换结束位置的指针
 * @param base 进制基数（0表示自动检测）
 * @return 转换后的有符号整数
 *
 * 支持自动检测进制：0x或0X前缀表示十六进制，0前缀表示八进制，否则十进制。
 * 支持空格跳过、正负号处理、溢出检测。
 */
template <typename T>
constexpr enable_if_t<is_signed_v<T>, T>
str_to_ints(const string_view sv, char** endptr, int base) {
    using UT = make_unsigned_t<T>;

    const char* start = sv.data();
    const size_t len = sv.size();
    const char* end = start + len;

    if (len == 0) {
        if (endptr) *endptr = const_cast<char*>(start);
        return 0;
    }

    const char* p = start;
    while (p != end && is_space(*p)) ++p;
    const char* start_conversion = p;

    int sign = 1;
    if (p != end && *p == '+') {
        ++p;
    } else if (p != end && *p == '-') {
        sign = -1;
        ++p;
    }

    if (base != 0 && (base < 2 || base > 36)) {
        if (endptr) *endptr = const_cast<char*>(start_conversion);
        return 0;
    }

    if (base == 0) {
        if (p != end && *p == '0') {
            if (p + 1 != end && (*(p + 1) == 'x' || *(p + 1) == 'X')) {
                base = 16;
                p += 2;
            } else {
                base = 8;
            }
        } else {
            base = 10;
        }
    } else if (base == 16 && p + 1 < end && *p == '0' && (*(p + 1) == 'x' || *(p + 1) == 'X')) {
        p += 2;
    }

    const UT umax = static_cast<UT>(numeric_traits<T>::max());
    const UT umin_abs = static_cast<UT>(numeric_traits<T>::max()) + static_cast<UT>(1);
    const UT limit = (sign > 0) ? umax : umin_abs;
    const UT cutoff = limit / static_cast<UT>(base);
    const UT cutlim = limit % static_cast<UT>(base);

    UT result = 0;
    bool any_converted = false;
    bool overflow = false;

    while (p != end) {
        uint32_t digit = 0;
        const char c = *p;
        if (c >= '0' && c <= '9') {
            digit = static_cast<uint32_t>(c - '0');
        } else if (c >= 'a' && c <= 'z') {
            digit = static_cast<uint32_t>(c - 'a' + 10);
        } else if (c >= 'A' && c <= 'Z') {
            digit = static_cast<uint32_t>(c - 'A' + 10);
        } else {
            break;
        }
        if (digit >= static_cast<uint32_t>(base)) break;

        any_converted = true;
        if (!overflow) {
            if (result > cutoff || (result == cutoff && static_cast<UT>(digit) > cutlim)) {
                overflow = true;
            } else {
                result = result * static_cast<UT>(base) + static_cast<UT>(digit);
            }
        }
        ++p;
    }

    if (endptr) {
        *endptr = any_converted ? const_cast<char*>(p) : const_cast<char*>(start_conversion);
    }

    if (!any_converted) {
        return 0;
    }

    if (overflow)
        return (sign > 0) ? numeric_traits<T>::max() : numeric_traits<T>::min();

    if (sign > 0) {
        return static_cast<T>(result);
    } else {
        if (result == umin_abs) {
            return numeric_traits<T>::min();
        }
        return static_cast<T>(~result + static_cast<UT>(1));
    }
}

/**
 * @brief 将字符串转换为无符号整数
 * @tparam T 无符号整数类型
 * @param sv 要转换的字符串视图
 * @param endptr 指向转换结束位置的指针
 * @param base 进制基数（0表示自动检测）
 * @return 转换后的无符号整数
 *
 * 支持自动检测进制：0x或0X前缀表示十六进制，0前缀表示八进制，否则十进制。
 * 支持空格跳过、正负号处理、溢出检测。负数会按照C标准转换为最大值。
 */
template <typename T>
constexpr enable_if_t<is_unsigned_v<T>, T>
str_to_uints(const string_view sv, char** endptr, int base) {
    const char* start = sv.data();
    const size_t len = sv.size();
    const char* end = start + len;

    if (len == 0) {
        if (endptr) *endptr = const_cast<char*>(start);
        return 0;
    }

    const char* p = start;
    while (p != end && is_space(*p)) ++p;
    const char* start_conversion = p;

    int sign = 1;
    if (p != end && *p == '+') {
        ++p;
    } else if (p != end && *p == '-') {
        sign = -1;
        ++p;
    }

    if (base != 0 && (base < 2 || base > 36)) {
        if (endptr) *endptr = const_cast<char*>(start_conversion);
        return 0;
    }

    if (base == 0) {
        if (p != end && *p == '0') {
            if (p + 1 != end && (*(p + 1) == 'x' || *(p + 1) == 'X')) {
                base = 16;
                p += 2;
            } else {
                base = 8;
            }
        } else {
            base = 10;
        }
    }

    if (base == 16 && p + 1 < end && *p == '0' && (*(p + 1) == 'x' || *(p + 1) == 'X')) {
        p += 2;
    }

    const T cutoff = numeric_traits<T>::max() / base;
    const T cutlim = numeric_traits<T>::max() % base;
    T result = 0;
    bool any_converted = false;
    bool overflow = false;

    while (p != end) {
        unsigned int digit = 0;
        const char c = *p;
        if (c >= '0' && c <= '9') {
            digit = c - '0';
        } else if (c >= 'a' && c <= 'z') {
            digit = c - 'a' + 10;
        } else if (c >= 'A' && c <= 'Z') {
            digit = c - 'A' + 10;
        } else {
            break;
        }
        if (digit >= static_cast<unsigned int>(base)) break;

        any_converted = true;
        if (!overflow) {
            if (result > cutoff || (result == cutoff && digit > cutlim)) {
                overflow = true;
            } else {
                result = result * base + digit;
            }
        }
        ++p;
    }

    if (endptr) {
        *endptr = any_converted ? const_cast<char*>(p) : const_cast<char*>(start_conversion);
    }

    if (!any_converted) return 0;
    if (overflow) return numeric_traits<T>::max();

    if (sign < 0) {
        // for unsigned, negative sign yields two's complement wrap,
        // but we follow C standard: strtoul("-1", ...) returns ULLONG_MAX.
        // So just cast via signed negation then to unsigned.
        return static_cast<T>(-static_cast<make_signed_t<T>>(result));
    }
    return result;
}

/**
 * @brief 快速计算10的幂
 * @tparam T 浮点数类型
 * @param exp 指数
 * @return 10的exp次幂
 *
 * 使用预计算表加速常用指数的计算，超出范围时使用通用幂函数。
 */
template <typename T>
NEFORCE_CONST_FUNCTION constexpr T fast_pow10(int exp) {
    constexpr T pow10_table[] = {
        1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9,
        1e10, 1e11, 1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19,
        1e20, 1e21, 1e22, 1e23, 1e24, 1e25, 1e26, 1e27, 1e28, 1e29,
        1e30, 1e31, 1e32
    };
    constexpr T neg_pow10_table[] = {
        1e0, 1e-1, 1e-2, 1e-3, 1e-4, 1e-5, 1e-6, 1e-7, 1e-8, 1e-9,
        1e-10, 1e-11, 1e-12, 1e-13, 1e-14, 1e-15, 1e-16, 1e-17, 1e-18, 1e-19,
        1e-20, 1e-21, 1e-22, 1e-23, 1e-24, 1e-25, 1e-26, 1e-27, 1e-28, 1e-29,
        1e-30, 1e-31, 1e-32
    };
    constexpr int max_table_exp = 32;

    if (exp >= 0 && exp <= max_table_exp) {
        return pow10_table[exp];
    }
    if (exp < 0 && -exp <= max_table_exp) {
        return neg_pow10_table[-exp];
    }
    return static_cast<T>(power(10.0, exp));
}

/**
 * @brief 将字符串转换为浮点数
 * @tparam T 浮点数类型
 * @param sv 要转换的字符串视图
 * @param endptr 指向转换结束位置的指针
 * @return 转换后的浮点数
 *
 * 支持：正负号、小数点、科学计数法(e/E)、inf/infinity、nan。
 * 处理精度损失和溢出情况。
 */
template <typename T>
constexpr enable_if_t<is_floating_point_v<T>, T>
str_to_floats(const string_view sv, char** endptr) {
    const char* start = sv.data();
    const size_t len = sv.size();
    const char* end = start + len;

    if (len == 0) {
        if (endptr) *endptr = const_cast<char*>(start);
        return static_cast<T>(0);
    }

    const char* p = start;
    while (p != end && is_space(*p)) ++p;
    const char* start_conversion = p;

    int sign = 1;
    if (p != end && *p == '+') {
        ++p;
    } else if (p != end && *p == '-') {
        sign = -1;
        ++p;
    }

    const char* p_start = p;

    if (p != end && (p[0] == 'i' || p[0] == 'I')) {
        if (p + 3 <= end) {
            const char c1 = p[1], c2 = p[2];
            if ((c1 == 'n' || c1 == 'N') && (c2 == 'f' || c2 == 'F')) {
                const bool terminated = (p + 3 == end) || !is_alpha_or_digit(p[3]);
                if (terminated) {
                    p += 3;
                    if (endptr) *endptr = const_cast<char*>(p);
                    const T inf_val = numeric_traits<T>::infinity();
                    return (sign < 0) ? -inf_val : inf_val;
                }
            }
        }
    }

    if (p != end && (p[0] == 'n' || p[0] == 'N')) {
        if (p + 3 <= end) {
            const char c1 = p[1], c2 = p[2];
            if ((c1 == 'a' || c1 == 'A') && (c2 == 'n' || c2 == 'N')) {
                const bool terminated = (p + 3 == end) || !is_alpha_or_digit(p[3]);
                if (terminated) {
                    p += 3;
                    if (p != end && *p == '(') {
                        ++p;
                        while (p != end && *p != ')') ++p;
                        if (p != end && *p == ')') ++p;
                    }
                    if (endptr) *endptr = const_cast<char*>(p);
                    return numeric_traits<T>::quiet_nan();
                }
            }
        }
    }

    p = p_start;

    T significand = 0;
    int exponent = 0;
    int digits_count = 0;
    bool has_digits = false;

    while (p != end && *p >= '0' && *p <= '9') {
        has_digits = true;
        if (digits_count < numeric_traits<T>::digits10) {
            significand = significand * static_cast<T>(10) + static_cast<T>(*p - '0');
        } else {
            exponent++;
        }
        digits_count++;
        ++p;
    }

    if (p != end && *p == '.') {
        ++p;
        while (p != end && *p >= '0' && *p <= '9') {
            has_digits = true;
            if (digits_count < numeric_traits<T>::digits10) {
                significand = significand * static_cast<T>(10) + static_cast<T>(*p - '0');
                exponent--;
            }
            digits_count++;
            ++p;
        }
    }

    if (!has_digits) {
        if (endptr) *endptr = const_cast<char*>(start_conversion);
        return static_cast<T>(0);
    }

    if (p != end && (*p == 'e' || *p == 'E')) {
        const char* e_pos = p;
        ++p;

        int exp_sign = 1;
        if (p != end && *p == '+') {
            ++p;
        } else if (p != end && *p == '-') {
            exp_sign = -1;
            ++p;
        }

        if (p != end && *p >= '0' && *p <= '9') {
            int exp_val = 0;
            while (p != end && *p >= '0' && *p <= '9') {
                if (exp_val < 100000) {
                    exp_val = exp_val * 10 + (*p - '0');
                }
                ++p;
            }
            exponent += exp_sign * exp_val;
        } else {
            p = e_pos;
        }
    }

    T result = significand;

    if (exponent != 0) {
        constexpr int max_exp = numeric_traits<T>::max_exponent10 + 50;
        constexpr int min_exp = numeric_traits<T>::min_exponent10 - 50;

        if (exponent > max_exp) {
            if (endptr) *endptr = const_cast<char*>(p);
            return (sign > 0) ? numeric_traits<T>::infinity() : -numeric_traits<T>::infinity();
        } else if (exponent < min_exp) {
            if (endptr) *endptr = const_cast<char*>(p);
            return static_cast<T>(0);
        } else {
            result *= fast_pow10<T>(exponent);
        }
    }

    const T inf = numeric_traits<T>::infinity();
    if (result == inf || result == -inf) {
        if (endptr) *endptr = const_cast<char*>(p);
        return (sign > 0) ? inf : -inf;
    }

    result = (sign > 0) ? result : -result;

    if (endptr) *endptr = const_cast<char*>(p);
    return result;
}

NEFORCE_END_INNER__
/// @endcond

/**
 * @defgroup StringNumerics 字符串数值转换
 * @brief 字符串与数值之间的转换功能
 * @{
 */

/**
 * @brief 将字符串转换为32位浮点数
 * @param sv 要转换的字符串视图
 * @param idx 可选参数，存储转换结束位置索引
 * @return 转换后的32位浮点数
 * @throws typecast_exception 转换失败时
 */
NEFORCE_NODISCARD constexpr float32_t
to_float32(const string_view sv, size_t* idx = nullptr) {
    char* endptr = nullptr;
    const float32_t num = inner::str_to_floats<float>(sv, &endptr);
    if (sv.data() == endptr) {
        NEFORCE_THROW_EXCEPTION(typecast_exception("Convert from string failed."));
    }
    if (idx) *idx = static_cast<size_t>(endptr - sv.data());
    return num;
}

/**
 * @brief 将字符串转换为64位浮点数
 * @param sv 要转换的字符串视图
 * @param idx 可选参数，存储转换结束位置索引
 * @return 转换后的64位浮点数
 * @throws typecast_exception 转换失败时
 */
NEFORCE_NODISCARD constexpr float64_t
to_float64(const string_view sv, size_t* idx = nullptr) {
    char* endptr = nullptr;
    const float64_t num = inner::str_to_floats<double>(sv, &endptr);
    if (sv.data() == endptr) {
        NEFORCE_THROW_EXCEPTION(typecast_exception("Convert from string failed."));
    }
    if (idx) *idx = static_cast<size_t>(endptr - sv.data());
    return num;
}

/**
 * @brief 将字符串转换为decimal浮点数
 * @param sv 要转换的字符串视图
 * @param idx 可选参数，存储转换结束位置索引
 * @return 转换后的十进制浮点数
 * @throws typecast_exception 转换失败时
 */
NEFORCE_NODISCARD constexpr decimal_t
to_decimal(const string_view sv, size_t* idx = nullptr) {
    char* endptr = nullptr;
    const decimal_t num = inner::str_to_floats<long double>(sv, &endptr);
    if (sv.data() == endptr) {
        NEFORCE_THROW_EXCEPTION(typecast_exception("Convert from string failed."));
    }
    if (idx) *idx = static_cast<size_t>(endptr - sv.data());
    return num;
}

/**
 * @brief 将字符串转换为64位有符号整数
 * @param sv 要转换的字符串视图
 * @param idx 可选参数，存储转换结束位置索引
 * @param base 进制基数（0表示自动检测）
 * @return 转换后的64位有符号整数
 * @throws typecast_exception 转换失败时
 */
NEFORCE_NODISCARD constexpr int64_t
to_int64(const string_view sv, size_t* idx = nullptr, const int base = 10) {
    char* endptr = nullptr;
    const int64_t num = inner::str_to_ints<int64_t>(sv, &endptr, base);
    if (sv.data() == endptr) {
        NEFORCE_THROW_EXCEPTION(typecast_exception("Convert from string failed."));
    }
    if (idx) *idx = static_cast<size_t>(endptr - sv.data());
    return num;
}

/**
 * @brief 将字符串转换为64位无符号整数
 * @param sv 要转换的字符串视图
 * @param idx 可选参数，存储转换结束位置索引
 * @param base 进制基数（0表示自动检测）
 * @return 转换后的64位无符号整数
 * @throws typecast_exception 转换失败时
 */
NEFORCE_NODISCARD constexpr uint64_t
to_uint64(const string_view sv, size_t* idx = nullptr, const int base = 10) {
    char* endptr = nullptr;
    const uint64_t num = inner::str_to_uints<uint64_t>(sv, &endptr, base);
    if (sv.data() == endptr) {
        NEFORCE_THROW_EXCEPTION(typecast_exception("Convert from string failed."));
    }
    if (idx) *idx = static_cast<size_t>(endptr - sv.data());
    return num;
}

/**
 * @brief 将字符串转换为32位有符号整数
 * @param sv 要转换的字符串视图
 * @param idx 可选参数，存储转换结束位置索引
 * @param base 进制基数（0表示自动检测）
 * @return 转换后的32位有符号整数
 * @throws typecast_exception 转换失败时
 */
NEFORCE_NODISCARD constexpr int32_t
to_int32(const string_view sv, size_t* idx = nullptr, const int base = 10) {
    char* endptr = nullptr;
    const int32_t num = inner::str_to_ints<int>(sv, &endptr, base);
    if (sv.data() == endptr) {
        NEFORCE_THROW_EXCEPTION(typecast_exception("Convert from string failed."));
    }
    if (idx) *idx = static_cast<size_t>(endptr - sv.data());
    return num;
}

/**
 * @brief 将字符串转换为32位无符号整数
 * @param sv 要转换的字符串视图
 * @param idx 可选参数，存储转换结束位置索引
 * @param base 进制基数（0表示自动检测）
 * @return 转换后的32位无符号整数
 * @throws typecast_exception 转换失败时
 */
NEFORCE_NODISCARD constexpr uint32_t
to_uint32(const string_view sv, size_t* idx = nullptr, const int base = 10) {
    char* endptr = nullptr;
    const uint32_t num = inner::str_to_uints<uint32_t>(sv, &endptr, base);
    if (sv.data() == endptr) {
        NEFORCE_THROW_EXCEPTION(typecast_exception("Convert from string failed."));
    }
    if (idx) *idx = static_cast<size_t>(endptr - sv.data());
    return num;
}

/**
 * @brief 将字符串转换为16位有符号整数
 * @param sv 要转换的字符串视图
 * @param idx 可选参数，存储转换结束位置索引
 * @param base 进制基数（0表示自动检测）
 * @return 转换后的16位有符号整数
 * @throws typecast_exception 转换失败时
 */
NEFORCE_NODISCARD constexpr int16_t
to_int16(const string_view sv, size_t* idx = nullptr, const int base = 10) {
    const int32_t val = to_int32(sv, idx, base);
    if (val > static_cast<int32_t>(numeric_traits<int16_t>::max()) ||
        val < static_cast<int32_t>(numeric_traits<int16_t>::min())) {
        NEFORCE_THROW_EXCEPTION(typecast_exception("Value out of int16_t range."));
    }
    return static_cast<int16_t>(val);
}

/**
 * @brief 将字符串转换为16位无符号整数
 * @param sv 要转换的字符串视图
 * @param idx 可选参数，存储转换结束位置索引
 * @param base 进制基数（0表示自动检测）
 * @return 转换后的16位无符号整数
 * @throws typecast_exception 转换失败时
 */
NEFORCE_NODISCARD constexpr uint16_t
to_uint16(const string_view sv, size_t* idx = nullptr, const int base = 10) {
    const uint32_t val = to_uint32(sv, idx, base);
    if (val > static_cast<uint32_t>(numeric_traits<uint16_t>::max())) {
        NEFORCE_THROW_EXCEPTION(typecast_exception("Value out of uint16_t range."));
    }
    return static_cast<uint16_t>(val);
}

/**
 * @brief 将字符串转换为8位有符号整数
 * @param sv 要转换的字符串视图
 * @param idx 可选参数，存储转换结束位置索引
 * @param base 进制基数（0表示自动检测）
 * @return 转换后的8位有符号整数
 * @throws typecast_exception 转换失败时
 */
NEFORCE_NODISCARD constexpr int8_t
to_int8(const string_view sv, size_t* idx = nullptr, const int base = 10) {
    const int32_t val = to_int32(sv, idx, base);
    if (val > static_cast<int32_t>(numeric_traits<int8_t>::max()) ||
        val < static_cast<int32_t>(numeric_traits<int8_t>::min())) {
        NEFORCE_THROW_EXCEPTION(typecast_exception("Value out of int8_t range."));
    }
    return static_cast<int8_t>(val);
}

/**
 * @brief 将字符串转换为8位无符号整数
 * @param sv 要转换的字符串视图
 * @param idx 可选参数，存储转换结束位置索引
 * @param base 进制基数（0表示自动检测）
 * @return 转换后的8位无符号整数
 * @throws typecast_exception 转换失败时
 */
NEFORCE_NODISCARD constexpr uint8_t
to_uint8(const string_view sv, size_t* idx = nullptr, const int base = 10) {
    const uint32_t val = to_uint32(sv, idx, base);
    if (val > static_cast<uint32_t>(numeric_traits<uint8_t>::max())) {
        NEFORCE_THROW_EXCEPTION(typecast_exception("Value out of uint8_t range."));
    }
    return static_cast<uint8_t>(val);
}

/** @} */ // StringNumerics

NEFORCE_END_NAMESPACE__

#endif // NEFORCE_CORE_STRING_TO_NUMERICS_HPP__
