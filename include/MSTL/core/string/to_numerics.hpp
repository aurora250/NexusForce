#ifndef MSTL_CORE_STRING_TO_NUMERICS_HPP__
#define MSTL_CORE_STRING_TO_NUMERICS_HPP__

/**
 * @file to_numerics.hpp
 * @brief 字符串到数值的转换函数
 *
 * 此文件提供了将字符串转换为各种数值类型的函数。
 * 支持整数、浮点数的转换，包括进制转换和错误处理。
 */

#include "MSTL/core/exception/exception.hpp"
#include "MSTL/core/numeric/math.hpp"
#include "MSTL/core/string/string_view.hpp"
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
MSTL_BEGIN_NAMESPACE__

/// @cond
MSTL_BEGIN_INNER__

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
template <typename T, enable_if_t<is_signed_v<T>, int> = 0>
constexpr T str_to_ints(const string_view sv, char** endptr, int base) {
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

    const T cutoff = numeric_traits<T>::min() / base;
    const T cutlim = numeric_traits<T>::min() % base;
    T result = 0;
    bool any_converted = false;
    bool overflow = false;

    while (p != end) {
        int digit = 0;
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
        if (digit >= base) break;

        any_converted = true;
        if (!overflow) {
            if (result < cutoff || (result == cutoff && digit > -cutlim)) {
                overflow = true;
            } else {
                result = result * base - digit;
            }
        }
        ++p;
    }

    if (endptr) {
        *endptr = any_converted ? const_cast<char*>(p) : const_cast<char*>(start_conversion);
    }

    if (!any_converted) return 0;
    if (overflow)
        return (sign > 0) ? numeric_traits<T>::max() : numeric_traits<T>::min();

    if (sign > 0) {
        if (result == numeric_traits<T>::min())
            return numeric_traits<T>::max();
        return -result;
    }
    return result;
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
template <typename T, enable_if_t<is_unsigned_v<T>, int> = 0>
constexpr T str_to_uints(const string_view sv, char** endptr, int base) {
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
MSTL_CONST_FUNCTION constexpr T fast_pow10(int exp) {
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
    return static_cast<T>(_MSTL power(10.0, exp));
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
constexpr T str_to_floats(const string_view sv, char** endptr) {
    const char* start = sv.data();
    const size_t len = sv.size();
    const char* end = start + len;

    if (len == 0) {
        if (endptr) *endptr = const_cast<char*>(start);
        return static_cast<T>(0);
    }

    const char* p = start;
    while (p != end && _MSTL is_space(*p)) ++p;
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
                if (p + 3 == end || !is_alpha_or_digit(p[3])) {
                    p += 3;
                    T inf_val = numeric_traits<T>::infinity();
                    if (endptr) *endptr = const_cast<char*>(p);
                    return (sign < 0) ? -inf_val : inf_val;
                }
            }
        }
    }

    if (p != end && (p[0] == 'n' || p[0] == 'N')) {
        if (p + 3 <= end) {
            const char c1 = p[1], c2 = p[2];
            if ((c1 == 'a' || c1 == 'A') && (c2 == 'n' || c2 == 'N')) {
                if (p + 3 == end || !is_alpha_or_digit(p[3])) {
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
            significand = significand * 10 + (*p - '0');
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
                significand = significand * 10 + (*p - '0');
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
                if (exp_val < 10000) {
                    exp_val = exp_val * 10 + (*p - '0');
                }
                ++p;
            }
            exponent += exp_sign * exp_val;
        } else {
            --p;
            if (p != start_conversion && (p[-1] == '+' || p[-1] == '-')) --p;
        }
    }

    T result = significand;

    if (exponent != 0) {
        if (exponent > 400) {
            if (endptr) *endptr = const_cast<char*>(p);
            return sign * numeric_traits<T>::max();
        } else if (exponent < -400) {
            if (endptr) *endptr = const_cast<char*>(p);
            return T(0);
        } else {
            result *= fast_pow10<T>(exponent);
        }
    }

    result *= sign;
    const T inf = numeric_traits<T>::infinity();
    if (result == inf || result == -inf) {
        result = sign * numeric_traits<T>::max();
    }

    if (endptr) *endptr = const_cast<char*>(p);
    return result;
}

MSTL_END_INNER__
/// @endcond

/**
 * @defgroup StringNumerics 字符串数值转换
 * @brief 字符串与数值之间的转换功能
 * @{
 */

/**
 * @brief 将字符串转换为64位有符号整数
 * @param sv 要转换的字符串视图
 * @param endptr 指向转换结束位置的指针
 * @param base 进制基数（0表示自动检测）
 * @return 转换后的64位有符号整数
 */
constexpr int64_t strtoll(const string_view sv, char** endptr, const int base) {
    return _INNER str_to_ints<int64_t>(sv, endptr, base);
}

/**
 * @brief 将字符串转换为长整型有符号整数
 * @param sv 要转换的字符串视图
 * @param endptr 指向转换结束位置的指针
 * @param base 进制基数（0表示自动检测）
 * @return 转换后的长整型有符号整数
 */
constexpr long strtol(const string_view sv, char** endptr, const int base) {
    return _INNER str_to_ints<long>(sv, endptr, base);
}

/**
 * @brief 将字符串转换为64位无符号整数
 * @param sv 要转换的字符串视图
 * @param endptr 指向转换结束位置的指针
 * @param base 进制基数（0表示自动检测）
 * @return 转换后的64位无符号整数
 */
constexpr uint64_t strtoull(const string_view sv, char** endptr, const int base) {
    return _INNER str_to_uints<uint64_t>(sv, endptr, base);
}

/**
 * @brief 将字符串转换为无符号长整型
 * @param sv 要转换的字符串视图
 * @param endptr 指向转换结束位置的指针
 * @param base 进制基数（0表示自动检测）
 * @return 转换后的无符号长整型
 */
constexpr unsigned long strtoul(const string_view sv, char** endptr, const int base) {
    return _INNER str_to_uints<unsigned long>(sv, endptr, base);
}

/**
 * @brief 将字符串转换为单精度浮点数
 * @param sv 要转换的字符串视图
 * @param endptr 指向转换结束位置的指针
 * @return 转换后的单精度浮点数
 */
constexpr float strtof(const string_view sv, char** endptr) {
    return _INNER str_to_floats<float>(sv, endptr);
}

/**
 * @brief 将字符串转换为双精度浮点数
 * @param sv 要转换的字符串视图
 * @param endptr 指向转换结束位置的指针
 * @return 转换后的双精度浮点数
 */
constexpr double strtod(const string_view sv, char** endptr) {
    return _INNER str_to_floats<double>(sv, endptr);
}

/**
 * @brief 将字符串转换为长双精度浮点数
 * @param sv 要转换的字符串视图
 * @param endptr 指向转换结束位置的指针
 * @return 转换后的长双精度浮点数
 */
constexpr long double strtold(const string_view sv, char** endptr) {
    return _INNER str_to_floats<long double>(sv, endptr);
}

/**
 * @brief 将字符串转换为32位浮点数
 * @param sv 要转换的字符串视图
 * @param idx 可选参数，存储转换结束位置索引
 * @return 转换后的32位浮点数
 * @throws typecast_exception 转换失败时
 */
MSTL_NODISCARD constexpr float32_t
to_float32(const string_view sv, size_t* idx = nullptr) {
    char* endptr = nullptr;
    const float32_t num = _MSTL strtof(sv, &endptr);
    if (sv.data() == endptr) {
        throw_exception(typecast_exception("Convert from string failed."));
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
MSTL_NODISCARD constexpr float64_t
to_float64(const string_view sv, size_t* idx = nullptr) {
    char* endptr = nullptr;
    const float64_t num = _MSTL strtod(sv, &endptr);
    if (sv.data() == endptr) {
        throw_exception(typecast_exception("Convert from string failed."));
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
MSTL_NODISCARD constexpr decimal_t
to_decimal(const string_view sv, size_t* idx = nullptr) {
    char* endptr = nullptr;
    const decimal_t num = _MSTL strtold(sv, &endptr);
    if (sv.data() == endptr) {
        throw_exception(typecast_exception("Convert from string failed."));
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
MSTL_NODISCARD constexpr int64_t
to_int64(const string_view sv, size_t* idx = nullptr, const int base = 10) {
    char* endptr = nullptr;
    const int64_t num = _MSTL strtoll(sv, &endptr, base);
    if (sv.data() == endptr) {
        throw_exception(typecast_exception("Convert from string failed."));
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
MSTL_NODISCARD constexpr uint64_t
to_uint64(const string_view sv, size_t* idx = nullptr, const int base = 10) {
    char* endptr = nullptr;
    const uint64_t num = _MSTL strtoull(sv, &endptr, base);
    if (sv.data() == endptr) {
        throw_exception(typecast_exception("Convert from string failed."));
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
MSTL_NODISCARD constexpr int32_t
to_int32(const string_view sv, size_t* idx = nullptr, const int base = 10) {
    char* endptr = nullptr;
    const int32_t num = _MSTL strtol(sv, &endptr, base);
    if (sv.data() == endptr) {
        throw_exception(typecast_exception("Convert from string failed."));
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
MSTL_NODISCARD constexpr uint32_t
to_uint32(const string_view sv, size_t* idx = nullptr, const int base = 10) {
    char* endptr = nullptr;
    const uint32_t num = _MSTL strtoul(sv, &endptr, base);
    if (sv.data() == endptr) {
        throw_exception(typecast_exception("Convert from string failed."));
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
MSTL_NODISCARD constexpr int16_t
to_int16(const string_view sv, size_t* idx = nullptr, const int base = 10) {
    return static_cast<int16_t>(to_int32(sv, idx, base));
}

/**
 * @brief 将字符串转换为16位无符号整数
 * @param sv 要转换的字符串视图
 * @param idx 可选参数，存储转换结束位置索引
 * @param base 进制基数（0表示自动检测）
 * @return 转换后的16位无符号整数
 * @throws typecast_exception 转换失败时
 */
MSTL_NODISCARD constexpr uint16_t
to_uint16(const string_view sv, size_t* idx = nullptr, const int base = 10) {
    return static_cast<uint16_t>(to_uint32(sv, idx, base));
}

/**
 * @brief 将字符串转换为8位有符号整数
 * @param sv 要转换的字符串视图
 * @param idx 可选参数，存储转换结束位置索引
 * @param base 进制基数（0表示自动检测）
 * @return 转换后的8位有符号整数
 * @throws typecast_exception 转换失败时
 */
MSTL_NODISCARD constexpr int8_t
to_int8(const string_view sv, size_t* idx = nullptr, const int base = 10) {
    return static_cast<int8_t>(to_int32(sv, idx, base));
}

/**
 * @brief 将字符串转换为8位无符号整数
 * @param sv 要转换的字符串视图
 * @param idx 可选参数，存储转换结束位置索引
 * @param base 进制基数（0表示自动检测）
 * @return 转换后的8位无符号整数
 * @throws typecast_exception 转换失败时
 */
MSTL_NODISCARD constexpr uint8_t
to_uint8(const string_view sv, size_t* idx = nullptr, const int base = 10) {
    return static_cast<uint8_t>(to_uint32(sv, idx, base));
}

/** @} */ // StringNumerics

MSTL_END_NAMESPACE__

#endif // MSTL_CORE_STRING_TO_NUMERICS_HPP__
