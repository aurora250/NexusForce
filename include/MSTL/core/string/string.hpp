#ifndef MSTL_CORE_STRING_STRING_HPP__
#define MSTL_CORE_STRING_STRING_HPP__
#include "MSTL/core/string/basic_string.hpp"
MSTL_BEGIN_NAMESPACE__

using string    = basic_string<char>;
using bstring   = basic_string<byte_t>;
using wstring   = basic_string<wchar_t>;
#ifdef MSTL_STANDARD_20__
using u8string  = basic_string<char8_t>;
#endif
using u16string = basic_string<char16_t>;
using u32string = basic_string<char32_t>;


MSTL_BEGIN_LITERALS__
MSTL_NODISCARD MSTL_CONSTEXPR20 string operator ""_s(const char* str, size_t len) noexcept {
    return {str, len};
}
MSTL_NODISCARD MSTL_CONSTEXPR20 wstring operator ""_s(const wchar_t* str, size_t len) noexcept {
    return {str, len};
}
#ifdef MSTL_STANDARD_20__
MSTL_NODISCARD MSTL_CONSTEXPR20 u8string operator ""_s(const char8_t* str, size_t len) noexcept {
    return {str, len};
}
#endif // MSTL_STANDARD_20__
MSTL_NODISCARD MSTL_CONSTEXPR20 u16string operator ""_s(const char16_t* str, size_t len) noexcept {
    return {str, len};
}
MSTL_NODISCARD MSTL_CONSTEXPR20 u32string operator ""_s(const char32_t* str, size_t len) noexcept {
    return {str, len};
}
MSTL_END_LITERALS__


MSTL_CONSTEXPR20 string escape(const string_view str) {
    string result;
    result.reserve(str.length() + str.length() / 4);

    for (const char c : str) {
        switch (c) {
            case '\"': {
                result += "\\\"";
                break;
            }
            case '\'': {
                result += "\\\'";
                break;
            }
            case '\\': {
                result += "\\\\";
                break;
            }
            case '\b': {
                result += "\\b";
                break;
            }
            case '\f': {
                result += "\\f";
                break;
            }
            case '\n': {
                result += "\\n";
                break;
            }
            case '\r': {
                result += "\\r";
                break;
            }
            case '\t': {
                result += "\\t";
                break;
            }
            case '\v': {
                result += "\\v";
                break;
            }
            default: {
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
    }
    return result;
}

MSTL_CONSTEXPR20 string escape(const string& str) {
    return escape(str.view());
}

MSTL_CONSTEXPR20 string escape(const char* str) {
    return escape(string_view{str});
}


template <typename CharT>
MSTL_CONSTEXPR20 bool getline(const basic_string_view<CharT> data, size_t& pos,
    basic_string<CharT>& str, CharT delim = static_cast<CharT>('\n')) {
    str.clear();
    bool has_read = false;
    while (pos < data.size()) {
        has_read = true;
        const CharT c = data[pos++];
        if (c == delim) break;
        str.push_back(c);
    }
    return has_read;
}

template <typename CharT>
MSTL_CONSTEXPR20 bool getline(const basic_string<CharT> data, size_t& pos,
    basic_string<CharT>& str, CharT delim = static_cast<CharT>('\n')) {
    str.clear();
    bool has_read = false;
    while (pos < data.size()) {
        has_read = true;
        const CharT c = data[pos++];
        if (c == delim) break;
        str.push_back(c);
    }
    return has_read;
}

template <typename CharT, typename Pred>
MSTL_CONSTEXPR20 bool getline(const basic_string_view<CharT> data, size_t& pos,
    basic_string<CharT>& str, Pred split = [](const CharT c) {
        return c == static_cast<CharT>('\n');
    }) {
    str.clear();
    bool has_read = false;
    while (pos < data.size()) {
        has_read = true;
        const CharT c = data[pos++];
        if (split(c)) break;
        str.push_back(c);
    }
    return has_read;
}

template <typename CharT, typename Pred>
MSTL_CONSTEXPR20 bool getline(const basic_string<CharT> data, size_t& pos,
    basic_string<CharT>& str, Pred split = [](const CharT c) {
        return c == static_cast<CharT>('\n');
    }) {
    str.clear();
    bool has_read = false;
    while (pos < data.size()) {
        has_read = true;
        const CharT c = data[pos++];
        if (split(c)) break;
        str.push_back(c);
    }
    return has_read;
}


MSTL_BEGIN_INNER__
#ifdef MSTL_DATA_BUS_WIDTH_64__
MSTL_INLINE17 constexpr uintptr_t ADDRESS_MASK = 0xF000000000000000ULL;
MSTL_INLINE17 constexpr int ADDRESS_SHIFT = 60;
#else
MSTL_INLINE17 constexpr uintptr_t ADDRESS_MASK = 0xF0000000UL;
MSTL_INLINE17 constexpr int ADDRESS_SHIFT = 28;
#endif
MSTL_END_INNER__

MSTL_NODISCARD MSTL_CONSTEXPR20 string address_string(const void* p) {
    if (p == nullptr) return {"nullptr"};
    
    const uintptr_t addr_val = reinterpret_cast<uintptr_t>(p);
    constexpr size_t hex_digit_count = sizeof(void*) * 2;
    constexpr char hex_digits[] = "0123456789abcdef";
    uintptr_t mask = _INNER ADDRESS_MASK;
    int shift = _INNER ADDRESS_SHIFT;

    string result{"0x"};
    result.reserve(2 + hex_digit_count);

    for (size_t i = 0; i < hex_digit_count; ++i) {
        const byte_t digit = static_cast<byte_t>((addr_val & mask) >> shift);
        result += hex_digits[digit];
        mask >>= 4;
        shift -= 4;
    }
    return result;
}

MSTL_END_NAMESPACE__
#endif // MSTL_STRING_HPP__
