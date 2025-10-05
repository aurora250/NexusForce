#ifndef MSTL_STRING_HPP__
#define MSTL_STRING_HPP__
#include "basic_string.hpp"
MSTL_BEGIN_NAMESPACE__

using string = basic_string<char>;
using bstring = basic_string<byte_t>;
using wstring = basic_string<wchar_t>;
#ifdef MSTL_VERSION_20__
using u8string = basic_string<char8_t>;
#endif
using u16string = basic_string<char16_t>;
using u32string = basic_string<char32_t>;


template <typename CharT, typename Traits, typename Alloc>
struct hash<basic_string<CharT, Traits, Alloc>> {
    MSTL_NODISCARD MSTL_CONSTEXPR20 size_t operator ()(const basic_string<CharT, Traits, Alloc>& str) const noexcept {
        return _INNER FNV_string_hash(str.data(), str.size());
    }
};


#ifdef MSTL_VERSION_17__
MSTL_BEGIN_LITERALS__

MSTL_NODISCARD MSTL_CONSTEXPR20 string operator ""_s(const char* str, size_t len) noexcept {
    return {str, len};
}
MSTL_NODISCARD MSTL_CONSTEXPR20 wstring operator ""_s(const wchar_t* str, size_t len) noexcept {
    return {str, len};
}
#ifdef MSTL_VERSION_20__
MSTL_NODISCARD MSTL_CONSTEXPR20 u8string operator ""_s(const char8_t* str, size_t len) noexcept {
    return {str, len};
}
#endif // MSTL_VERSION_20__
MSTL_NODISCARD MSTL_CONSTEXPR20 u16string operator ""_s(const char16_t* str, size_t len) noexcept {
    return {str, len};
}
MSTL_NODISCARD MSTL_CONSTEXPR20 u32string operator ""_s(const char32_t* str, size_t len) noexcept {
    return {str, len};
}

MSTL_END_LITERALS__
#endif // MSTL_VERSION_17__


MSTL_API string address_string(const void* p);


MSTL_BEGIN_INNER__

MSTL_CONSTEXPR20 void __char_to_utf8(string& result, const uint32_t cp) noexcept {
    if (cp > 0x10FFFF || is_high_surrogate(cp) || is_low_surrogate(cp)) {
        result.append("\xEF\xBF\xBD", 3);
        return;
    }

    if (cp <= 0x7F) {
        result.push_back(static_cast<char>(cp));
    } else if (cp <= 0x7FF) {
        const char bytes[2] = {
            static_cast<char>(0xC0 | (cp >> 6)),
            static_cast<char>(0x80 | (cp & 0x3F))
        };
        result.append(bytes, 2);
    } else if (cp <= 0xFFFF) {
        const char bytes[3] = {
            static_cast<char>(0xE0 | (cp >> 12)),
            static_cast<char>(0x80 | ((cp >> 6) & 0x3F)),
            static_cast<char>(0x80 | (cp & 0x3F))
        };
        result.append(bytes, 3);
    } else {
        const char bytes[4] = {
            static_cast<char>(0xF0 | (cp >> 18)),
            static_cast<char>(0x80 | ((cp >> 12) & 0x3F)),
            static_cast<char>(0x80 | ((cp >> 6) & 0x3F)),
            static_cast<char>(0x80 | (cp & 0x3F))
        };
        result.append(bytes, 4);
    }
}

MSTL_CONSTEXPR20 void __chars_to_utf8_optimized(string& result, const auto* str, const size_t len) {
    result.reserve(result.size() + len);

    size_t i = 0;
    while (i < len && static_cast<uint32_t>(str[i]) <= 0x7F) {
        result.push_back(static_cast<char>(str[i]));
        ++i;
    }
    for (; i < len; ++i) {
        __char_to_utf8(result, static_cast<uint32_t>(str[i]));
    }
}

MSTL_END_INNER__


MSTL_CONSTEXPR20 string string_to_utf8(const wchar_t* str) {
    if (!str) return {};

    string result;
    const size_t len = char_traits<wchar_t>::length(str);
    if (len == 0) return result;

    if constexpr (sizeof(wchar_t) == 2) {
        for (size_t i = 0; i < len; ++i) {
            const auto c1 = static_cast<uint32_t>(str[i]);

            if (_INNER is_high_surrogate(c1)) {
                if (i + 1 < len) {
                    const auto c2 = static_cast<uint32_t>(str[i + 1]);
                    if (_INNER is_low_surrogate(c2)) {
                        _INNER __char_to_utf8(result, _INNER combine_surrogates(c1, c2));
                        ++i;
                        continue;
                    }
                }
                _INNER __char_to_utf8(result, 0xFFFD);
            } else if (_INNER is_low_surrogate(c1)) {
                _INNER __char_to_utf8(result, 0xFFFD);
            } else {
                _INNER __char_to_utf8(result, c1);
            }
        }
    } else {
        _INNER __chars_to_utf8_optimized(result, str, len);
    }
    return result;
}

#ifdef MSTL_VERSION_20__
MSTL_CONSTEXPR20 string string_to_utf8(const char8_t* str) {
    if (!str) return {};

    string result;
    const char8_t* p = str;
    while (*p) ++p;
    const size_t len = p - str;
    result.reserve(len);
    for (const char8_t* ptr = str; *ptr; ++ptr) {
        result.push_back(static_cast<char>(*ptr));
    }
    return result;
}
#endif


MSTL_CONSTEXPR20 string string_to_utf8(const char16_t* str) {
    if (!str) return {};
    string result;
    for (size_t i = 0; str[i] != u'\0'; ++i) {
        if (i == 0 && str[i] == 0xFEFF) { // skip BOM
            continue;
        }

        const auto c1 = static_cast<uint32_t>(str[i]);
        if (_INNER is_high_surrogate(c1)) {
            if (str[i + 1] != u'\0') {
                const auto c2 = static_cast<uint32_t>(str[i + 1]);
                if (_INNER is_low_surrogate(c2)) {
                    _INNER __char_to_utf8(result, _INNER combine_surrogates(c1, c2));
                    ++i;
                    continue;
                }
            }
            _INNER __char_to_utf8(result, 0xFFFD);
        } else if (_INNER is_low_surrogate(c1)) {
            _INNER __char_to_utf8(result, 0xFFFD);
        } else {
            _INNER __char_to_utf8(result, c1);
        }
    }
    return result;
}

MSTL_CONSTEXPR20 string string_to_utf8(const char32_t* str) {
    if (!str) return {};
    string result;
    const size_t len = char_traits<char32_t>::length(str);
    _INNER __chars_to_utf8_optimized(result, str, len);
    return result;
}


MSTL_CONSTEXPR20 string escape_string(const string& str) {
    string result;
    result.reserve(str.length() + str.length() / 4);

    for (const char c : str) {
        switch (c) {
            case '\"':
                result += "\\\"";
            break;
            case '\'':
                result += "\\\'";
            break;
            case '\\':
                result += "\\\\";
            break;
            case '\b':
                result += "\\b";
            break;
            case '\f':
                result += "\\f";
            break;
            case '\n':
                result += "\\n";
            break;
            case '\r':
                result += "\\r";
            break;
            case '\t':
                result += "\\t";
            break;
            case '\v':
                result += "\\v";
            break;
            default:
                if (static_cast<byte_t>(c) < 0x20) {
                    result += "\\u";
                    constexpr char hex[] = "0123456789abcdef";
                    result += "00";
                    result += hex[(c >> 4) & 0x0F];
                    result += hex[c & 0x0F];
                } else {
                    result += c;
                }
            break;
        }
    }
    return result;
}


MSTL_BEGIN_INNER__

#ifndef MSTL_DATA_BUS_WIDTH_64__
template <typename CharT, typename UT, enable_if_t<(sizeof(UT) > 4), int> = 0>
constexpr void __uint_to_buff_aux(CharT* riter, UT& ux) noexcept {
    while (ux > 0xFFFFFFFFU) {
        auto chunk = static_cast<unsigned long>(ux % 1000000000);
        ux /= 1000000000;
        for (int idx = 0; idx != 9; ++idx) {
            *--riter = static_cast<CharT>('0' + chunk % 10);
            chunk /= 10;
        }
    }
}
template <typename CharT, typename UT, enable_if_t<sizeof(UT) <= 4, int> = 0>
constexpr void __uint_to_buff_aux(CharT*, UT&) noexcept {}
#endif // MSTL_DATA_BUS_WIDTH_64__

template <typename CharT, typename UT, enable_if_t<is_unsigned_v<UT>, int> = 0>
MSTL_NODISCARD constexpr CharT* __uint_to_buff(CharT* riter, UT ux) noexcept {
#ifdef MSTL_DATA_BUS_WIDTH_64__
    UT holder = ux;
#else
    _INNER __uint_to_buff_aux(riter, ux);
    auto holder = static_cast<unsigned long>(ux);
#endif
    do {
        *--riter = static_cast<CharT>('0' + holder % static_cast<UT>(10));
        holder /= static_cast<UT>(10);
    } while (holder != static_cast<UT>(0));
    return riter;
}

template <typename CharT, typename T, enable_if_t<is_integral<T>::value, int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 basic_string<CharT> __int_to_string(const T x) {
    CharT buffer[21];
    CharT* const buffer_end = buffer + 21;
    CharT* rnext = buffer_end;
    using UT = make_unsigned_t<T>;
    const auto unsigned_x = static_cast<UT>(x);
    if (x < 0) {
        rnext = _INNER __uint_to_buff(rnext, static_cast<UT>(0 - unsigned_x));
        *--rnext = '-';
    }
    else 
        rnext = _INNER __uint_to_buff(rnext, unsigned_x);
    return basic_string<CharT>(rnext, buffer_end);
}

template <typename CharT, typename T,
    enable_if_t<conjunction_v<is_integral<T>, is_unsigned<T>>, int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 basic_string<CharT> __uint_to_string(T x) {
    CharT buffer[21];
    CharT* const buffer_end = buffer + 21;
    CharT* const rnext = _INNER __uint_to_buff(buffer_end, x);
    return basic_string<CharT>(rnext, buffer_end);
}

MSTL_CONSTEXPR20 string __uint_to_string_base(uint64_t value, int base, bool uppercase) {
    if (value == 0) {
        return "0";
    }
    string result;
    const auto digits_lower = "0123456789abcdef";
    const auto digits_upper = "0123456789ABCDEF";
    const char* digits = uppercase ? digits_upper : digits_lower;
    while (value > 0) {
        const uint64_t remainder = value % base;
        value /= base;
        result.push_back(digits[remainder]);
    }
    result.reverse();
    return result;
}

template <typename T, enable_if_t<
    disjunction_v<conjunction<is_standard_integral<T>, is_signed<T>>, is_same<T, signed char>>, int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 string __int_to_string_dispatch(const T x) {
    return _INNER __int_to_string<char>(x);
}
template <typename T, enable_if_t<
    disjunction_v<conjunction<is_standard_integral<T>, is_unsigned<T>>, is_same<T, unsigned char>>, int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 string __int_to_string_dispatch(const T x) {
    return _INNER __uint_to_string<char>(x);
}


template <typename CharT, typename T, enable_if_t<is_floating_point<T>::value, int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 basic_string<CharT> __float_to_string_with_precision(
    T x, int precision = 6, bool force_scientific = false, bool force_fixed = false) {
    if (x == numeric_limits<T>::quiet_nan()) return basic_string<CharT>{"nan"};
    constexpr T inf = numeric_limits<T>::infinity();
    if (x == inf || x == -inf) {
        return (x < 0) ? basic_string<CharT>{"-inf"} : basic_string<CharT>{"inf"};
    }

    basic_string<CharT> result;

    if (x < 0) {
        result += '-';
        x = -x;
    }

    if (precision < 0) precision = 0;

    bool use_scientific = false;
    if (force_scientific) {
        use_scientific = true;
    } else if (force_fixed) {
        use_scientific = false;
    } else {
        use_scientific = (x >= 1e6 || (x > 0 && x < 1e-4));
    }

    if (use_scientific) {
        int exponent = 0;

        if (x == 0) {
            exponent = 0;
        } else {
            if (x >= 1) {
                while (x >= 10) {
                    x /= 10;
                    ++exponent;
                }
            } else {
                while (x < 1) {
                    x *= 10;
                    --exponent;
                }
            }
        }

        auto integer_part = static_cast<uint64_t>(x);
        T fractional_part = x - integer_part;

        result += _INNER __uint_to_string<CharT>(integer_part);

        if (precision > 0) {
            result += '.';
            for (int i = 0; i < precision; ++i) {
                fractional_part *= 10;
                auto digit = static_cast<int>(fractional_part);
                result += static_cast<CharT>('0' + digit);
                fractional_part -= digit;
            }
        }

        result += 'e';
        if (exponent >= 0) {
            result += '+';
        } else {
            result += '-';
            exponent = -exponent;
        }

        if (exponent < 10) {
            result += '0';
        }
        result += _INNER __uint_to_string<CharT>(static_cast<uint64_t>(exponent));

    } else {
        auto integer_part = static_cast<uint64_t>(x);
        T fractional_part = x - integer_part;

        result += _INNER __uint_to_string<CharT>(integer_part);

        if (precision > 0) {
            result += '.';
            for (int i = 0; i < precision; ++i) {
                fractional_part *= 10;
                auto digit = static_cast<int>(fractional_part);
                result += static_cast<CharT>('0' + digit);
                fractional_part -= digit;
            }
        }
    }

    return result;
}

template <typename CharT, typename T, enable_if_t<is_floating_point<T>::value, int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 basic_string<CharT> __float_to_string(T x) {
    return _INNER __float_to_string_with_precision<CharT>(x, 6, false, false);
}

template <typename T, enable_if_t<is_floating_point<T>::value, int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 string __to_string_with_precision(T x, int precision, bool scientific = false) {
    return _INNER __float_to_string_with_precision<char>(x, precision, scientific, scientific);
}

template <typename T, enable_if_t<is_floating_point<T>::value, int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 string __to_string_general(T x, int precision = 6) {
    return _INNER __float_to_string_with_precision<char>(x, precision, false, false);
}

template <typename T, enable_if_t<is_floating_point<T>::value, int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 string __to_string_fixed(T x, int precision = 6) {
    return _INNER __float_to_string_with_precision<char>(x, precision, false, true);
}

template <typename T, enable_if_t<is_floating_point<T>::value, int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 string __to_string_scientific(T x, int precision = 6) {
    return _INNER __float_to_string_with_precision<char>(x, precision, true, false);
}


MSTL_NODISCARD MSTL_CONSTEXPR20 string __string_to_string_dispatch(const char* x) {
    return {x};
}
MSTL_NODISCARD inline string __string_to_string_dispatch(const signed char* x) {
    return {reinterpret_cast<const char*>(x)};
}
MSTL_NODISCARD inline string __string_to_string_dispatch(const unsigned char* x) {
    return {reinterpret_cast<const char*>(x)};
}
MSTL_NODISCARD MSTL_CONSTEXPR20 string __string_to_string_dispatch(const wchar_t* x) {
    return string_to_utf8(x);
}
#ifdef MSTL_VERSION_20__
MSTL_NODISCARD MSTL_CONSTEXPR20 string __string_to_string_dispatch(const char8_t* x) {
    return string_to_utf8(x);
}
#endif
MSTL_NODISCARD MSTL_CONSTEXPR20 string __string_to_string_dispatch(const char16_t* x) {
    return string_to_utf8(x);
}
MSTL_NODISCARD MSTL_CONSTEXPR20 string __string_to_string_dispatch(const char32_t* x) {
    return string_to_utf8(x);
}

MSTL_END_INNER__

template <typename T, enable_if_t<is_character_v<T>, int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string(const T* x) {
    return _INNER __string_to_string_dispatch(x);
}
template <typename CharT>
MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string(const basic_string_view<CharT> x) {
    return _INNER __string_to_string_dispatch(x.data());
}
template <typename CharT>
MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string(const basic_string<CharT>& x) {
    return _INNER __string_to_string_dispatch(x);
}
template <>
MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string<byte_t>(const bstring& x) {
    return string(x.begin(), x.end());
}

MSTL_NODISCARD MSTL_CONSTEXPR20 bstring to_bstring(const string& x) {
    return bstring(x.begin(), x.end());
}

MSTL_END_NAMESPACE__
#endif // MSTL_STRING_HPP__
