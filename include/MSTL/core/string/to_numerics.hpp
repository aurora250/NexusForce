#ifndef MSTL_CORE_STRING_TO_NUMERICS_HPP__
#define MSTL_CORE_STRING_TO_NUMERICS_HPP__
#include "../string/cstring.hpp"
#include "../numeric/math.hpp"
#include "../config/exception.hpp"
#include "../config/undef_cmacro.hpp"
MSTL_BEGIN_NAMESPACE__

MSTL_BEGIN_INNER__

template <typename T, enable_if_t<is_signed_v<T>, int> = 0>
constexpr T str_to_ints(const char* str, char** endptr, int base) {
    if (str == nullptr) {
        if (endptr) *endptr = const_cast<char*>(str);
        return 0;
    }

    const char* p = str;
    while (is_space(*p)) p++;
    const char* start_conversion = p;

    int sign = 1;
    if (*p == '+') {
        p++;
    } else if (*p == '-') {
        sign = -1;
        p++;
    }

    if (base != 0 && (base < 2 || base > 36)) {
        if (endptr) *endptr = const_cast<char*>(start_conversion);
        return 0;
    }

    if (base == 0) {
        if (*p == '0') {
            if (*(p + 1) == 'x' || *(p + 1) == 'X') {
                base = 16;
                p += 2;
            } else {
                base = 8;
            }
        } else {
            base = 10;
        }
    }

    if (base == 16 && *p == '0' && (*(p + 1) == 'x' || *(p + 1) == 'X')) {
        p += 2;
    }

    const T cutoff = numeric_limits<T>::min() / base;
    const T cutlim = numeric_limits<T>::min() % base;
    T result = 0;
    bool any_converted = false;
    bool overflow = false;

    while (*p) {
        int digit = 0;
        if (*p >= '0' && *p <= '9') {
            digit = *p - '0';
        } else if (*p >= 'a' && *p <= 'z') {
            digit = *p - 'a' + 10;
        } else if (*p >= 'A' && *p <= 'Z') {
            digit = *p - 'A' + 10;
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
        p++;
    }

    if (endptr) {
        if (any_converted) {
            *endptr = const_cast<char*>(p);
        } else {
            *endptr = const_cast<char*>(start_conversion);
        }
    }

    if (!any_converted) return 0;
    if (overflow)
        return (sign > 0) ? numeric_limits<T>::max() : numeric_limits<T>::min();

    if (sign > 0) {
        if (result == numeric_limits<T>::min())
            return numeric_limits<T>::max();
        return -result;
    }
    return result;
}

template <typename T, enable_if_t<is_unsigned_v<T>, int> = 0>
constexpr T str_to_uints(const char* str, char** endptr, int base) {
    if (str == nullptr) {
        if (endptr) *endptr = const_cast<char*>(str);
        return 0;
    }

    const char* p = str;
    while (is_space(*p)) p++;
    const char* start_conversion = p;

    int sign = 1;
    if (*p == '+') {
        p++;
    } else if (*p == '-') {
        sign = -1;
        p++;
    }

    if (base != 0 && (base < 2 || base > 36)) {
        if (endptr) *endptr = const_cast<char*>(start_conversion);
        return 0;
    }

    if (base == 0) {
        if (*p == '0') {
            if (*(p + 1) == 'x' || *(p + 1) == 'X') {
                base = 16;
                p += 2;
            } else {
                base = 8;
            }
        } else {
            base = 10;
        }
    }

    if (base == 16 && *p == '0' && (*(p + 1) == 'x' || *(p + 1) == 'X')) {
        p += 2;
    }

    const T cutoff = numeric_limits<T>::max() / base;
    const T cutlim = numeric_limits<T>::max() % base;
    T result = 0;
    bool any_converted = false;
    bool overflow = false;

    while (*p) {
        unsigned int digit = 0;
        if (*p >= '0' && *p <= '9') {
            digit = *p - '0';
        } else if (*p >= 'a' && *p <= 'z') {
            digit = *p - 'a' + 10;
        } else if (*p >= 'A' && *p <= 'Z') {
            digit = *p - 'A' + 10;
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
        p++;
    }

    if (endptr) {
        if (any_converted) {
            *endptr = const_cast<char*>(p);
        } else {
            *endptr = const_cast<char*>(start_conversion);
        }
    }

    if (!any_converted) return 0;
    if (overflow) return numeric_limits<T>::max();

    if (sign < 0)
        return static_cast<T>(-static_cast<make_signed_t<T>>(result));
    return result;
}

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

template <typename T>
constexpr T str_to_floats(const char* str, char** endptr) {
    if (str == nullptr) {
        if (endptr) *endptr = const_cast<char*>(str);
        return static_cast<T>(0);
    }

    const char* p = str;
    while (_MSTL is_space(*p)) p++;
    const char* start_conversion = p;

    int sign = 1;
    if (*p == '+') {
        p++;
    } else if (*p == '-') {
        sign = -1;
        p++;
    }

    if (_MSTL to_lowercase(*p) == 'i') {
        if (_MSTL string_n_compare_ignore_case(p, "inf", 3) == 0) {
            p += 3;
            if (_MSTL string_n_compare_ignore_case(p, "inity", 5) == 0) {
                p += 5;
            }
            if (endptr) *endptr = const_cast<char*>(p);
            return sign * numeric_limits<T>::max();
        }
    } else if (_MSTL to_lowercase(*p) == 'n') {
        if (_MSTL string_n_compare_ignore_case(p, "nan", 3) == 0) {
            p += 3;
            if (*p == '(') {
                while (*p != '\0' && *p != ')') p++;
                if (*p == ')') p++;
            }
            if (endptr) *endptr = const_cast<char*>(p);
            return numeric_limits<T>::quiet_nan();
        }
    }

    T significand = 0;
    int exponent = 0;
    int digits_count = 0;
    bool has_digits = false;

    while (*p >= '0' && *p <= '9') {
        has_digits = true;
        if (digits_count < numeric_limits<T>::digits10) {
            significand = significand * 10 + (*p - '0');
        } else {
            exponent++;
        }
        digits_count++;
        p++;
    }

    if (*p == '.') {
        p++;
        while (*p >= '0' && *p <= '9') {
            has_digits = true;
            if (digits_count < numeric_limits<T>::digits10) {
                significand = significand * 10 + (*p - '0');
                exponent--;
            }
            digits_count++;
            p++;
        }
    }

    if (!has_digits) {
        if (endptr) *endptr = const_cast<char*>(start_conversion);
        return static_cast<T>(0);
    }

    if (*p == 'e' || *p == 'E') {
        p++;
        int exp_sign = 1;
        if (*p == '+') {
            p++;
        } else if (*p == '-') {
            exp_sign = -1;
            p++;
        }

        if (*p >= '0' && *p <= '9') {
            int exp_val = 0;
            while (*p >= '0' && *p <= '9') {
                if (exp_val < 10000) {
                    exp_val = exp_val * 10 + (*p - '0');
                }
                p++;
            }
            exponent += exp_sign * exp_val;
        } else {
            p--;
            if (*p == '+' || *p == '-') p--;
        }
    }

    T result = significand;

    if (exponent != 0) {
        if (exponent > 400) {
            if (endptr) *endptr = const_cast<char*>(p);
            return sign * numeric_limits<T>::max();
        } else if (exponent < -400) {
            if (endptr) *endptr = const_cast<char*>(p);
            return T(0);
        } else {
            result *= fast_pow10<T>(exponent);
        }
    }

    result *= sign;
    const T inf = numeric_limits<T>::infinity();
    if (inf == result || -inf == result) {
        result = sign * numeric_limits<T>::max();
    }

    if (endptr) *endptr = const_cast<char*>(p);
    return result;
}

MSTL_END_INNER__


constexpr int64_t strtoll(const char* str, char** endptr, const int base) {
    return _INNER str_to_ints<int64_t>(str, endptr, base);
}
constexpr long strtol(const char* str, char** endptr, const int base) {
    return _INNER str_to_ints<long>(str, endptr, base);
}

constexpr uint64_t strtoull(const char* str, char** endptr, const int base) {
    return _INNER str_to_uints<uint64_t>(str, endptr, base);
}
constexpr unsigned long strtoul(const char* str, char** endptr, const int base) {
    return _INNER str_to_uints<unsigned long>(str, endptr, base);
}

constexpr float strtof(const char* str, char** endptr) {
	return _INNER str_to_floats<float>(str, endptr);
}
constexpr double strtod(const char* str, char** endptr) {
	return _INNER str_to_floats<double>(str, endptr);
}
constexpr long double strtold(const char* str, char** endptr) {
	return _INNER str_to_floats<long double>(str, endptr);
}


MSTL_NODISCARD constexpr float32_t to_float32(const char* str, size_t* idx = nullptr) {
    char* endptr = nullptr;
    const float32_t num = _MSTL strtof(str, &endptr);
    if(str == endptr) throw_exception(typecast_exception("Convert from string failed."));

    if (idx) *idx = static_cast<size_t>(endptr - str);
    return num;
}

MSTL_NODISCARD constexpr float64_t to_float64(const char* str, size_t* idx = nullptr) {
    char* endptr = nullptr;
    const float64_t num = _MSTL strtod(str, &endptr);
    if(str == endptr) throw_exception(typecast_exception("Convert from string failed."));

    if (idx) *idx = static_cast<size_t>(endptr - str);
    return num;
}

MSTL_NODISCARD constexpr decimal_t to_decimal(const char* str, size_t* idx = nullptr) {
    char* endptr = nullptr;
    const decimal_t num = _MSTL strtold(str, &endptr);
    if(str == endptr) throw_exception(typecast_exception("Convert from string failed."));

    if (idx) *idx = static_cast<size_t>(endptr - str);
    return num;
}

MSTL_NODISCARD constexpr int64_t to_int64(const char* str, size_t* idx = nullptr, const int base = 10) {
    char* endptr = nullptr;
    const int64_t num = _MSTL strtoll(str, &endptr, base);
    if(str == endptr) throw_exception(typecast_exception("Convert from string failed."));

    if (idx) *idx = static_cast<size_t>(endptr - str);
    return num;
}

MSTL_NODISCARD constexpr uint64_t to_uint64(const char* str, size_t* idx = nullptr, const int base = 10) {
    char* endptr = nullptr;
    const uint64_t num = _MSTL strtoull(str, &endptr, base);
    if(str == endptr) throw_exception(typecast_exception("Convert from string failed."));

    if (idx) *idx = static_cast<size_t>(endptr - str);
    return num;
}

MSTL_NODISCARD constexpr int32_t to_int32(const char* str, size_t* idx = nullptr, const int base = 10) {
    char* endptr = nullptr;
    const int32_t num = _MSTL strtol(str, &endptr, base);
    if(str == endptr) throw_exception(typecast_exception("Convert from string failed."));

    if (idx) *idx = static_cast<size_t>(endptr - str);
    return num;
}

MSTL_NODISCARD constexpr uint32_t to_uint32(const char* str, size_t* idx = nullptr, const int base = 10) {
    char* endptr = nullptr;
    const uint32_t num = _MSTL strtoul(str, &endptr, base);
    if(str == endptr) throw_exception(typecast_exception("Convert from string failed."));

    if (idx) *idx = static_cast<size_t>(endptr - str);
    return num;
}

MSTL_NODISCARD constexpr int16_t to_int16(const char* str, size_t* idx = nullptr, const int base = 10) {
    return static_cast<int16_t>(to_int32(str, idx, base));
}

MSTL_NODISCARD constexpr uint16_t to_uint16(const char* str, size_t* idx = nullptr, const int base = 10) {
    return static_cast<int16_t>(to_uint32(str, idx, base));
}

MSTL_NODISCARD constexpr int8_t to_int8(const char* str, size_t* idx = nullptr, const int base = 10) {
    return static_cast<int8_t>(to_int32(str, idx, base));
}

MSTL_NODISCARD constexpr uint8_t to_uint8(const char* str, size_t* idx = nullptr, const int base = 10) {
    return static_cast<uint8_t>(to_uint32(str, idx, base));
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_STRING_TO_NUMERICS_HPP__
