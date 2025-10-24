#ifndef MSTL_STRING_HPP__
#define MSTL_STRING_HPP__
#include "cstring.hpp"
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


MSTL_CONSTEXPR20 string escape(const string_view str) {
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
    if (!has_read) {
        return false;
    }
    return true;
}

template <typename CharT>
MSTL_CONSTEXPR20 bool getline(const basic_string<CharT>& data, size_t& pos,
    basic_string<CharT>& str, CharT delim = static_cast<CharT>('\n')) {
    str.clear();
    bool has_read = false;
    while (pos < data.size()) {
        has_read = true;
        const CharT c = data[pos++];
        if (c == delim) break;
        str.push_back(c);
    }
    if (!has_read) {
        return false;
    }
    return true;
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
    constexpr size_t hex_digit_count = POINTER_SIZE * 2;
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


template <typename T>
struct istringify;

template <typename T, typename P = package_t<T>, enable_if_t<is_packaged_v<T> && is_base_of_v<istringify<P>, P>, int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string(const T& value);


MSTL_BEGIN_INNER__

template <typename T>
MSTL_ALWAYS_INLINE MSTL_CONSTEXPR20 void __append_utf8_char_aux(T&) {}
template <>
MSTL_CONSTEXPR20 void __append_utf8_char_aux<string>(string& result) {
    result.append("\xEF\xBF\xBD", 3);
}
#ifdef MSTL_VERSION_20__
template <>
MSTL_CONSTEXPR20 void __append_utf8_char_aux<u8string>(u8string& result) {
    result.append(u8"\xEF\xBF\xBD", 3);
}
#endif

template <typename T>
MSTL_CONSTEXPR20 void append_utf8_char(basic_string<T>& result, uint32_t cp) {
    if (cp > 0x10FFFF || _MSTL is_high_surrogate(cp) || _MSTL is_low_surrogate(cp)) {
        _INNER __append_utf8_char_aux(result);
        return;
    }

    if (cp <= 0x7F) {
        result.push_back(static_cast<T>(cp));
    } else if (cp <= 0x7FF) {
        result.push_back(static_cast<T>(0xC0 | (cp >> 6)));
        result.push_back(static_cast<T>(0x80 | (cp & 0x3F)));
    } else if (cp <= 0xFFFF) {
        result.push_back(static_cast<T>(0xE0 | (cp >> 12)));
        result.push_back(static_cast<T>(0x80 | ((cp >> 6) & 0x3F)));
        result.push_back(static_cast<T>(0x80 | (cp & 0x3F)));
    } else {
        result.push_back(static_cast<T>(0xF0 | (cp >> 18)));
        result.push_back(static_cast<T>(0x80 | ((cp >> 12) & 0x3F)));
        result.push_back(static_cast<T>(0x80 | ((cp >> 6) & 0x3F)));
        result.push_back(static_cast<T>(0x80 | (cp & 0x3F)));
    }
}

constexpr bool decode_utf8_char(const byte_t* data, size_t& i, const size_t len, uint32_t& cp) noexcept {
    if (i >= len) {
        cp = 0xFFFD;
        return false;
    }

    const byte_t b1 = data[i++];
    if ((b1 & 0x80) == 0) {
        cp = b1;
        return true;
    } else if ((b1 & 0xE0) == 0xC0) {
        if (i >= len) return false;
        const byte_t b2 = data[i++];
        if ((b2 & 0xC0) != 0x80) return false;
        cp = ((b1 & 0x1F) << 6) | (b2 & 0x3F);
        return cp >= 0x80;
    } else if ((b1 & 0xF0) == 0xE0) {
        if (i + 1 >= len) return false;
        const byte_t b2 = data[i++];
        const byte_t b3 = data[i++];
        if ((b2 & 0xC0) != 0x80 || (b3 & 0xC0) != 0x80) return false;
        cp = ((b1 & 0x0F) << 12) | ((b2 & 0x3F) << 6) | (b3 & 0x3F);
        return cp >= 0x800 && !(cp >= 0xD800 && cp <= 0xDFFF);
    } else if ((b1 & 0xF8) == 0xF0) {
        if (i + 2 >= len) return false;
        const byte_t b2 = data[i++];
        const byte_t b3 = data[i++];
        const byte_t b4 = data[i++];
        if ((b2 & 0xC0) != 0x80 || (b3 & 0xC0) != 0x80 || (b4 & 0xC0) != 0x80) return false;
        cp = ((b1 & 0x07) << 18) | ((b2 & 0x3F) << 12) | ((b3 & 0x3F) << 6) | (b4 & 0x3F);
        return cp >= 0x10000 && cp <= 0x10FFFF;
    }

    cp = 0xFFFD;
    return false;
}

template <typename T>
constexpr bool get_utf16_codepoint(const T* obj, size_t i, const size_t len, uint32_t& cp, size_t& chars_consumed) {
    const auto c1 = static_cast<uint32_t>(obj[i]);
    chars_consumed = 1;

    if (_MSTL is_high_surrogate(c1)) {
        if (i + 1 < len) {
            const auto c2 = static_cast<uint32_t>(obj[i + 1]);
            if (_MSTL is_low_surrogate(c2)) {
                cp = _MSTL combine_surrogates(c1, c2);
                chars_consumed = 2;
                return true;
            }
        }
        cp = 0xFFFD;
        return true;
    } else if (_MSTL is_low_surrogate(c1)) {
        cp = 0xFFFD;
        return true;
    }

    cp = c1;
    return true;
}

template <typename T>
constexpr bool handle_utf16_surrogate_pair(
    const T* obj, size_t& i, const size_t len, uint32_t& cp) noexcept {
    const auto c1 = static_cast<uint32_t>(obj[i]);
    if (_MSTL is_high_surrogate(c1)) {
        if (i + 1 < len) {
            const auto c2 = static_cast<uint32_t>(obj[i + 1]);
            if (_MSTL is_low_surrogate(c2)) {
                cp = _MSTL combine_surrogates(c1, c2);
                i += 2;
                return true;
            }
        }
        cp = 0xFFFD;
        i += 1;
        return true;
    } else if (_MSTL is_low_surrogate(c1)) {
        cp = 0xFFFD;
        i += 1;
        return true;
    }
    cp = c1;
    i += 1;
    return true;
}

constexpr bool is_valid_unicode_codepoint(const uint32_t cp) noexcept {
    return cp <= 0x10FFFF && !_MSTL is_high_surrogate(cp) && !_MSTL is_low_surrogate(cp);
}

template <typename T>
MSTL_CONSTEXPR20 void codepoint_to_utf16(basic_string<T>& result, uint32_t cp) {
    if (!_INNER is_valid_unicode_codepoint(cp)) {
        result.push_back(0xFFFD);
        return;
    }

    if (cp <= 0xFFFF) {
        if (cp >= 0xD800 && cp <= 0xDFFF) {
            result.push_back(0xFFFD);
        } else {
            result.push_back(static_cast<T>(cp));
        }
    } else {
        const uint32_t adjusted = cp - 0x10000;
        const auto high_surrogate = static_cast<T>((adjusted >> 10) + 0xD800);
        const auto low_surrogate = static_cast<T>((adjusted & 0x3FF) + 0xDC00);
        result.push_back(high_surrogate);
        result.push_back(low_surrogate);
    }
}

template <typename T>
MSTL_CONSTEXPR20 void codepoint_to_wchar(basic_string<T>& result, uint32_t cp) {
    if (!_INNER is_valid_unicode_codepoint(cp)) {
        result.push_back(0xFFFD);
        return;
    }

#ifdef MSTL_PLATFORM_WINDOWS__
    if (cp <= 0xFFFF) {
        result.push_back(static_cast<wchar_t>(cp));
    } else {
        const uint32_t adjusted = cp - 0x10000;
        const wchar_t high_surrogate = static_cast<wchar_t>((adjusted >> 10) + 0xD800);
        const wchar_t low_surrogate = static_cast<wchar_t>((adjusted & 0x3FF) + 0xDC00);
        result.push_back(high_surrogate);
        result.push_back(low_surrogate);
    }
#elif defined(MSTL_PLATFORM_LINUX__)
    result.push_back(static_cast<wchar_t>(cp));
#endif
}

template <typename T, typename U>
MSTL_CONSTEXPR20 void append_ascii_chars(basic_string<T>& result, const U* str, size_t len) {
    result.reserve(result.size() + len);
    for (size_t i = 0; i < len; ++i) {
        result.push_back(static_cast<T>(static_cast<byte_t>(str[i])));
    }
}

MSTL_END_INNER__


template <typename T, typename CharT>
struct icharacter : icommon<T> {
    static_assert(is_character_v<CharT>, "icharacter can only be used with characters");
public:
    using base_type = icommon<T>;
    using self = icharacter<T, CharT>;
    using child_type = T;
    using value_type = CharT;

private:
    static constexpr child_type* to_template(const self* o) noexcept {
        return base_type::to_template(o);
    }

public:
    MSTL_CONSTEXPR20 ~icharacter() = default;

    constexpr bool is_space() const
    noexcept(noexcept(self::to_template(this)->is_space())) {
        return self::to_template(this)->is_space();
    }
    constexpr bool is_alpha() const
    noexcept(noexcept(self::to_template(this)->is_alpha())) {
        return self::to_template(this)->is_alpha();
    }
    constexpr bool is_digit() const
    noexcept(noexcept(self::to_template(this)->is_digit())) {
        return self::to_template(this)->is_digit();
    }
    constexpr bool is_xdigit() const
    noexcept(noexcept(self::to_template(this)->is_xdigit())) {
        return self::to_template(this)->is_xdigit();
    }
    constexpr bool is_alpha_or_digit() const
    noexcept(noexcept(self::to_template(this)->is_alpha_or_digit())) {
        return self::to_template(this)->is_alpha_or_digit();
    }
    constexpr bool is_digit_or_alpha() const
    noexcept(noexcept(self::to_template(this)->is_digit_or_alpha())) {
        return self::to_template(this)->is_digit_or_alpha();
    }

    constexpr void to_lowercase()
    noexcept(noexcept(self::to_template(this)->to_lowercase())) {
        return self::to_template(this)->to_lowercase();
    }
    constexpr void to_uppercase()
    noexcept(noexcept(self::to_template(this)->to_uppercase())) {
        return self::to_template(this)->to_uppercase();
    }

    static constexpr child_type to_lowercase(const self& obj)
    noexcept(noexcept(child_type(static_cast<const child_type&>(obj)).to_lowercase())) {
        return child_type(static_cast<const child_type&>(obj)).to_lowercase();
    }
    static constexpr child_type to_uppercase(const self& obj)
    noexcept(noexcept(child_type(static_cast<const child_type&>(obj)).to_uppercase())) {
        return child_type(static_cast<const child_type&>(obj)).to_uppercase();
    }

    static MSTL_CONSTEXPR20 string to_string(const basic_string_view<value_type>& obj) {
        return child_type::to_string(obj);
    }
    static MSTL_CONSTEXPR20 wstring to_wstring(const basic_string_view<value_type>& obj) {
        return child_type::to_wstring(obj);
    }
#ifdef MSTL_VERSION_20__
    static MSTL_CONSTEXPR20 u8string to_u8string(const basic_string_view<value_type>& obj) {
        return child_type::to_u8string(obj);
    }
#endif
    static MSTL_CONSTEXPR20 u16string to_u16string(const basic_string_view<value_type>& obj) {
        return child_type::to_u16string(obj);
    }
    static MSTL_CONSTEXPR20 u32string to_u32string(const basic_string_view<value_type>& obj) {
        return child_type::to_u32string(obj);
    }
};

template <typename T, typename CharT, enable_if_t<is_base_of_v<icharacter<T, CharT>, T>, int> = 0>
constexpr bool is_space(const T& obj)
noexcept(noexcept(obj.to_space())) {
    return obj.is_space();
}
template <typename T, typename CharT, enable_if_t<is_base_of_v<icharacter<T, CharT>, T>, int> = 0>
constexpr bool is_alpha(const T& obj)
noexcept(noexcept(obj.is_alpha())) {
    return obj.is_alpha();
}
template <typename T, typename CharT, enable_if_t<is_base_of_v<icharacter<T, CharT>, T>, int> = 0>
constexpr bool is_digit(const T& obj)
noexcept(noexcept(obj.is_digit())) {
    return obj.is_digit();
}
template <typename T, typename CharT, enable_if_t<is_base_of_v<icharacter<T, CharT>, T>, int> = 0>
constexpr bool is_xdigit(const T& obj)
noexcept(noexcept(obj.is_xdigit())) {
    return obj.is_xdigit();
}
template <typename T, typename CharT, enable_if_t<is_base_of_v<icharacter<T, CharT>, T>, int> = 0>
constexpr bool is_alpha_or_digit(const T& obj)
noexcept(noexcept(obj.is_alpha_or_digit())) {
    return obj.is_alpha_or_digit();
}
template <typename T, typename CharT, enable_if_t<is_base_of_v<icharacter<T, CharT>, T>, int> = 0>
constexpr bool is_digit_or_alpha(const T& obj)
noexcept(noexcept(obj.is_digit_or_alpha())) {
    return obj.is_digit_or_alpha();
}

template <typename T, typename CharT, enable_if_t<is_base_of_v<icharacter<T, CharT>, T>, int> = 0>
constexpr T to_lowercase(const T& obj)
noexcept(noexcept(icharacter<T, CharT>::to_lowercase(obj))) {
    return icharacter<T, CharT>::to_lowercase(obj);
}
template <typename T, typename CharT, enable_if_t<is_base_of_v<icharacter<T, CharT>, T>, int> = 0>
constexpr T to_uppercase(const T& obj)
noexcept(noexcept(icharacter<T, CharT>::to_uppercase(obj))) {
    return icharacter<T, CharT>::to_uppercase(obj);
}


template <typename CharT, enable_if_t<is_standard_character_v<CharT>, int> = 0>
MSTL_CONSTEXPR20 string to_string(const CharT& x) {
    return icharacter<package_t<CharT>, CharT>::to_string(basic_string<CharT>(1, x).view());
}
template <typename CharT, enable_if_t<is_standard_character_v<CharT>, int> = 0>
MSTL_CONSTEXPR20 string to_string(const CharT* x) {
    return icharacter<package_t<CharT>, CharT>::to_string(basic_string_view<CharT>(x));
}
template <typename CharT, enable_if_t<is_standard_character_v<CharT>, int> = 0>
MSTL_CONSTEXPR20 string to_string(const basic_string_view<CharT> x) {
    return icharacter<package_t<CharT>, CharT>::to_string(x);
}
template <typename CharT, enable_if_t<is_standard_character_v<CharT>, int> = 0>
MSTL_CONSTEXPR20 string to_string(const basic_string<CharT>& x) {
    return icharacter<package_t<CharT>, CharT>::to_string(x.view());
}
template <typename CharT, enable_if_t<is_standard_character_v<CharT>, int> = 0>
MSTL_CONSTEXPR20 string to_string(basic_string<CharT>&& x) {
    return icharacter<package_t<CharT>, CharT>::to_string(x.view());
}
template <>
MSTL_CONSTEXPR20 string to_string<char>(string&& x) {
    return _MSTL move(x);
}

template <typename CharT, enable_if_t<is_standard_character_v<CharT>, int> = 0>
MSTL_CONSTEXPR20 wstring to_wstring(const CharT& x) {
    return icharacter<package_t<CharT>, CharT>::to_wstring(basic_string<CharT>(1, x).view());
}
template <typename CharT, enable_if_t<is_standard_character_v<CharT>, int> = 0>
MSTL_CONSTEXPR20 wstring to_wstring(const CharT* x) {
    return icharacter<package_t<CharT>, CharT>::to_wstring(basic_string_view<CharT>(x));
}
template <typename CharT, enable_if_t<is_standard_character_v<CharT>, int> = 0>
MSTL_CONSTEXPR20 wstring to_wstring(const basic_string_view<CharT> x) {
    return icharacter<package_t<CharT>, CharT>::to_wstring(x);
}
template <typename CharT, enable_if_t<is_standard_character_v<CharT>, int> = 0>
MSTL_CONSTEXPR20 wstring to_wstring(const basic_string<CharT>& x) {
    return icharacter<package_t<CharT>, CharT>::to_wstring(x.view());
}
template <typename CharT, enable_if_t<is_standard_character_v<CharT>, int> = 0>
MSTL_CONSTEXPR20 wstring to_wstring(basic_string<CharT>&& x) {
    return icharacter<package_t<CharT>, CharT>::to_wstring(x.view());
}
template <>
MSTL_CONSTEXPR20 wstring to_wstring<wchar_t>(wstring&& x) {
    return _MSTL move(x);
}

#ifdef MSTL_VERSION_20__
template <typename CharT, enable_if_t<is_standard_character_v<CharT>, int> = 0>
MSTL_CONSTEXPR20 u8string to_u8string(const CharT& x) {
    return icharacter<package_t<CharT>, CharT>::to_u8string(basic_string<CharT>(1, x).view());
}
template <typename CharT, enable_if_t<is_standard_character_v<CharT>, int> = 0>
MSTL_CONSTEXPR20 u8string to_u8string(const CharT* x) {
    return icharacter<package_t<CharT>, CharT>::to_u8string(basic_string_view<CharT>(x));
}
template <typename CharT, enable_if_t<is_standard_character_v<CharT>, int> = 0>
MSTL_CONSTEXPR20 u8string to_u8string(const basic_string_view<CharT> x) {
    return icharacter<package_t<CharT>, CharT>::to_u8string(x);
}
template <typename CharT, enable_if_t<is_standard_character_v<CharT>, int> = 0>
MSTL_CONSTEXPR20 u8string to_u8string(const basic_string<CharT>& x) {
    return icharacter<package_t<CharT>, CharT>::to_u8string(x.view());
}
template <typename CharT, enable_if_t<is_standard_character_v<CharT>, int> = 0>
MSTL_CONSTEXPR20 u8string to_u8string(basic_string<CharT>&& x) {
    return icharacter<package_t<CharT>, CharT>::to_u8string(x.view());
}
template <>
MSTL_CONSTEXPR20 u8string to_u8string<char8_t>(u8string&& x) {
    return _MSTL move(x);
}
#endif

template <typename CharT, enable_if_t<is_standard_character_v<CharT>, int> = 0>
MSTL_CONSTEXPR20 u16string to_u16string(const CharT& x) {
    return icharacter<package_t<CharT>, CharT>::to_u16string(basic_string<CharT>(1, x).view());
}
template <typename CharT, enable_if_t<is_standard_character_v<CharT>, int> = 0>
MSTL_CONSTEXPR20 u16string to_u16string(const CharT* x) {
    return icharacter<package_t<CharT>, CharT>::to_u16string(basic_string_view<CharT>(x));
}
template <typename CharT, enable_if_t<is_standard_character_v<CharT>, int> = 0>
MSTL_CONSTEXPR20 u16string to_u16string(const basic_string_view<CharT> x) {
    return icharacter<package_t<CharT>, CharT>::to_u16string(x);
}
template <typename CharT, enable_if_t<is_standard_character_v<CharT>, int> = 0>
MSTL_CONSTEXPR20 u16string to_u16string(const basic_string<CharT>& x) {
    return icharacter<package_t<CharT>, CharT>::to_u16string(x.view());
}
template <typename CharT, enable_if_t<is_standard_character_v<CharT>, int> = 0>
MSTL_CONSTEXPR20 u16string to_u16string(basic_string<CharT>&& x) {
    return icharacter<package_t<CharT>, CharT>::to_u16string(x.view());
}
template <>
MSTL_CONSTEXPR20 u16string to_u16string<char16_t>(u16string&& x) {
    return _MSTL move(x);
}

template <typename CharT, enable_if_t<is_standard_character_v<CharT>, int> = 0>
MSTL_CONSTEXPR20 u32string to_u32string(const CharT& x) {
    return icharacter<package_t<CharT>, CharT>::to_u32string(basic_string<CharT>(1, x).view());
}
template <typename CharT, enable_if_t<is_standard_character_v<CharT>, int> = 0>
MSTL_CONSTEXPR20 u32string to_u32string(const CharT* x) {
    return icharacter<package_t<CharT>, CharT>::to_u32string(basic_string_view<CharT>(x));
}
template <typename CharT, enable_if_t<is_standard_character_v<CharT>, int> = 0>
MSTL_CONSTEXPR20 u32string to_u32string(const basic_string_view<CharT> x) {
    return icharacter<package_t<CharT>, CharT>::to_u32string(x);
}
template <typename CharT, enable_if_t<is_standard_character_v<CharT>, int> = 0>
MSTL_CONSTEXPR20 u32string to_u32string(const basic_string<CharT>& x) {
    return icharacter<package_t<CharT>, CharT>::to_u32string(x.view());
}
template <typename CharT, enable_if_t<is_standard_character_v<CharT>, int> = 0>
MSTL_CONSTEXPR20 u32string to_u32string(basic_string<CharT>&& x) {
    return icharacter<package_t<CharT>, CharT>::to_u32string(x.view());
}
template <>
MSTL_CONSTEXPR20 u32string to_u32string<char32_t>(u32string&& x) {
    return _MSTL move(x);
}


struct character : icharacter<character, char> {
    using value_type = char;
    using self = character;
    using base = icharacter<character, char>;

private:
    value_type value_ = _MSTL initialize<value_type>();

public:
    constexpr character () = default;
    constexpr character (const self&) noexcept = default;
    constexpr character (const value_type& val) noexcept : value_(val) {}
    constexpr character & operator=(const self&) noexcept = default;
    constexpr character & operator=(const value_type& other) noexcept { value_ = other; return *this; }

    constexpr character (self&& other) noexcept : value_(other. value_) {
        other. value_ = _MSTL initialize<value_type>();
    }
    constexpr character (value_type&& other) noexcept : value_(other) {}

    constexpr self& operator=(self&& other) noexcept {
        if (this != &other) {
            value_ = other. value_;
            other. value_ = 0;
        }
        return *this;
    }
    constexpr self& operator=(value_type&& other) noexcept {
        value_ = other; return *this;
    }

    MSTL_CONSTEXPR20 ~character () = default;

    MSTL_NODISCARD constexpr explicit operator bool() const noexcept {
        return value_ != _MSTL initialize<value_type>();
    }
    MSTL_NODISCARD constexpr operator value_type() const noexcept { return value_; }
    MSTL_NODISCARD constexpr value_type value() const noexcept { return value_; }
    static constexpr size_t bytes() noexcept { return sizeof(value_type); }
    static constexpr size_t bits() noexcept { return sizeof(value_type) * 8; }

    MSTL_NODISCARD constexpr size_t to_hash() const noexcept {
        return _MSTL hash<value_type>()(value_);
    }

    constexpr void swap(self& other) noexcept {
        _MSTL swap(value_, other.value_);
    }

    constexpr bool operator ==(const self& other) const noexcept { return value_ == other.value_; }
    constexpr bool operator !=(const self& other) const noexcept { return value_ != other.value_; }
    constexpr bool operator <(const self& other) const noexcept { return value_ < other.value_; }
    constexpr bool operator <=(const self& other) const noexcept { return value_ <= other.value_; }
    constexpr bool operator >(const self& other) const noexcept { return value_ > other.value_; }
    constexpr bool operator >=(const self& other) const noexcept { return value_ >= other.value_; }

    constexpr bool is_space() const noexcept { return _MSTL is_space(value_); }
    constexpr bool is_alpha() const noexcept { return _MSTL is_alpha(value_); }
    constexpr bool is_digit() const noexcept { return _MSTL is_digit(value_); }
    constexpr bool is_xdigit() const noexcept { return _MSTL is_xdigit(value_); }
    constexpr bool is_alpha_or_digit() const noexcept { return _MSTL is_alpha_or_digit(value_); }
    constexpr bool is_digit_or_alpha() const noexcept { return _MSTL is_digit_or_alpha(value_); }

    constexpr void to_lowercase() noexcept { value_ = _MSTL to_lowercase(value_); }
    constexpr void to_uppercase() noexcept { value_ = _MSTL to_uppercase(value_); }


    static MSTL_CONSTEXPR20 string to_string(const basic_string_view<value_type>& obj) {
        return string{obj};
    }

    static MSTL_CONSTEXPR20 wstring to_wstring(const basic_string_view<value_type>& obj) {
        if (obj.empty()) return {};
        wstring result;

        const auto* data = reinterpret_cast<const byte_t*>(obj.data());
        size_t i = 0;
        const size_t len = obj.size();
        result.reserve(len);

        while (i < len) {
            uint32_t cp;
            if (_INNER decode_utf8_char(data, i, len, cp)) {
                _INNER codepoint_to_wchar(result, cp);
            } else {
                _INNER codepoint_to_wchar(result, 0xFFFD);
            }
        }
        return result;
    }

#ifdef MSTL_VERSION_20__
    static MSTL_CONSTEXPR20 u8string to_u8string(const basic_string_view<value_type>& obj) {
        if (obj.empty()) return {};
        u8string result;
        result.reserve(obj.size());
        for (const char c : obj) {
            result.push_back(static_cast<char8_t>(static_cast<unsigned char>(c)));
        }
        return result;
    }
#endif

    static MSTL_CONSTEXPR20 u16string to_u16string(const basic_string_view<value_type>& obj) {
        if (obj.empty()) return {};
        u16string result;

        const auto* data = reinterpret_cast<const byte_t*>(obj.data());
        size_t i = 0;
        const size_t len = obj.size();
        result.reserve(len * 2);

        while (i < len) {
            uint32_t cp;
            if (_INNER decode_utf8_char(data, i, len, cp)) {
                _INNER codepoint_to_utf16(result, cp);
            } else {
                result.push_back(0xFFFD);
            }
        }
        return result;
    }

    static MSTL_CONSTEXPR20 u32string to_u32string(const basic_string_view<value_type>& obj) {
        if (obj.empty()) return {};
        u32string result;

        const auto* data = reinterpret_cast<const byte_t*>(obj.data());
        size_t i = 0;
        const size_t len = obj.size();
        result.reserve(len);

        while (i < len) {
            uint32_t cp;
            if (_INNER decode_utf8_char(data, i, len, cp)) {
                result.push_back(static_cast<char32_t>(cp));
            } else {
                result.push_back(0xFFFD);
            }
        }
        return result;
    }
};

template <>
struct package_base<char> {
    using type = character;
};
template <>
struct unpackage_base<character> {
    using type = char;
};


struct wcharacter : icharacter<wcharacter, wchar_t> {
    using value_type = wchar_t;
    using self = wcharacter;
    using base = icharacter<wcharacter, wchar_t>;

private:
    value_type value_ = _MSTL initialize<value_type>();

public:
    constexpr wcharacter () = default;
    constexpr wcharacter (const self&) noexcept = default;
    constexpr wcharacter (const value_type& val) noexcept : value_(val) {}
    constexpr wcharacter & operator=(const self&) noexcept = default;
    constexpr wcharacter & operator=(const value_type& other) noexcept { value_ = other; return *this; }

    constexpr wcharacter (self&& other) noexcept : value_(other. value_) {
        other. value_ = _MSTL initialize<value_type>();
    }
    constexpr wcharacter (value_type&& other) noexcept : value_(other) {}

    constexpr self& operator=(self&& other) noexcept {
        if (this != &other) {
            value_ = other. value_;
            other. value_ = 0;
        }
        return *this;
    }
    constexpr self& operator=(value_type&& other) noexcept {
        value_ = other; return *this;
    }

    MSTL_CONSTEXPR20 ~wcharacter () = default;

    MSTL_NODISCARD constexpr explicit operator bool() const noexcept {
        return value_ != _MSTL initialize<value_type>();
    }
    MSTL_NODISCARD constexpr operator value_type() const noexcept { return value_; }
    MSTL_NODISCARD constexpr value_type value() const noexcept { return value_; }
    static constexpr size_t bytes() noexcept { return sizeof(value_type); }
    static constexpr size_t bits() noexcept { return sizeof(value_type) * 8; }

    MSTL_NODISCARD constexpr size_t to_hash() const noexcept {
        return _MSTL hash<value_type>()(value_);
    }

    constexpr void swap(self& other) noexcept {
        _MSTL swap(value_, other.value_);
    }

    constexpr bool operator ==(const self& other) const noexcept { return value_ == other.value_; }
    constexpr bool operator !=(const self& other) const noexcept { return value_ != other.value_; }
    constexpr bool operator <(const self& other) const noexcept { return value_ < other.value_; }
    constexpr bool operator <=(const self& other) const noexcept { return value_ <= other.value_; }
    constexpr bool operator >(const self& other) const noexcept { return value_ > other.value_; }
    constexpr bool operator >=(const self& other) const noexcept { return value_ >= other.value_; }

    constexpr bool is_space() const noexcept { return _MSTL is_space(value_); }
    constexpr bool is_alpha() const noexcept { return _MSTL is_alpha(value_); }
    constexpr bool is_digit() const noexcept { return _MSTL is_digit(value_); }
    constexpr bool is_xdigit() const noexcept { return _MSTL is_xdigit(value_); }
    constexpr bool is_alpha_or_digit() const noexcept { return _MSTL is_alpha_or_digit(value_); }
    constexpr bool is_digit_or_alpha() const noexcept { return _MSTL is_digit_or_alpha(value_); }

    constexpr void to_lowercase() noexcept { value_ = _MSTL to_lowercase(value_); }
    constexpr void to_uppercase() noexcept { value_ = _MSTL to_uppercase(value_); }


    static MSTL_CONSTEXPR20 string to_string(const basic_string_view<value_type>& obj) {
        if (obj.empty()) return {};
        string result;

#ifdef MSTL_PLATFORM_WINDOWS__
        for (size_t i = 0; i < obj.size(); ) {
            uint32_t cp;
            size_t chars_consumed;
            if (_INNER get_utf16_codepoint(obj.data(), i, obj.size(), cp, chars_consumed)) {
                _INNER append_utf8_char(result, cp);
                i += chars_consumed;
            } else {
                _INNER append_utf8_char(result, 0xFFFD);
                i++;
            }
        }
#elif defined(MSTL_PLATFORM_LINUX__)
        for (size_t i = 0; i < obj.size(); ++i) {
            _INNER append_utf8_char(result, obj[i]);
        }
#endif
        return result;
    }

    static MSTL_CONSTEXPR20 wstring to_wstring(const basic_string_view<value_type>& obj) {
        return wstring{obj};
    }

#ifdef MSTL_VERSION_20__
    static MSTL_CONSTEXPR20 u8string to_u8string(const basic_string_view<value_type>& obj) {
        if (obj.empty()) return {};
        u8string result;

#ifdef MSTL_PLATFORM_WINDOWS__
        for (size_t i = 0; i < obj.size(); ) {
            uint32_t cp;
            size_t chars_consumed;
            if (_INNER get_utf16_codepoint(obj.data(), i, obj.size(), cp, chars_consumed)) {
                _INNER append_utf8_char(result, cp);
                i += chars_consumed;
            } else {
                _INNER append_utf8_char(result, 0xFFFD);
                i++;
            }
        }
#elif defined(MSTL_PLATFORM_LINUX__)
        for (size_t i = 0; i < obj.size(); ++i) {
            _INNER append_utf8_char(result, obj[i]);
        }
#endif
        return result;
    }
#endif

    static MSTL_CONSTEXPR20 u16string to_u16string(const basic_string_view<value_type>& obj) {
        if (obj.empty()) return {};
        u16string result;

#ifdef MSTL_PLATFORM_WINDOWS__
        result.reserve(obj.size());
        for (size_t i = 0; i < obj.size(); ++i) {
            result.push_back(static_cast<char16_t>(static_cast<uint16_t>(obj[i])));
        }
#elif defined(MSTL_PLATFORM_LINUX__)
        result.reserve(obj.size() * 2);
        for (size_t i = 0; i < obj.size(); ++i) {
            _INNER codepoint_to_utf16(result, obj[i]);
        }
#endif
        return result;
    }

    static MSTL_CONSTEXPR20 u32string to_u32string(const basic_string_view<value_type>& obj) {
        if (obj.empty()) return {};
        u32string result;
        result.reserve(obj.size());

#ifdef MSTL_PLATFORM_WINDOWS__
        for (size_t i = 0; i < obj.size(); ) {
            uint32_t cp;
            size_t chars_consumed;
            if (_INNER get_utf16_codepoint(obj.data(), i, obj.size(), cp, chars_consumed)) {
                result.push_back(static_cast<char32_t>(cp));
                i += chars_consumed;
            } else {
                result.push_back(0xFFFD);
                i++;
            }
        }
#elif defined(MSTL_PLATFORM_LINUX__)
        for (size_t i = 0; i < obj.size(); ++i) {
            result.push_back(static_cast<char32_t>(obj[i]));
        }
#endif
        return result;
    }
};

template <>
struct package_base<wchar_t> {
    using type = wcharacter;
};
template <>
struct unpackage_base<wcharacter> {
    using type = wchar_t;
};


#ifdef MSTL_VERSION_20__

struct u8character : icharacter<u8character, char8_t> {
    using value_type = char8_t;
    using self = u8character;
    using base = icharacter<u8character, char8_t>;

private:
    value_type value_ = _MSTL initialize<value_type>();

public:
    constexpr u8character () = default;
    constexpr u8character (const self&) noexcept = default;
    constexpr u8character (const value_type& val) noexcept : value_(val) {}
    constexpr u8character & operator=(const self&) noexcept = default;
    constexpr u8character & operator=(const value_type& other) noexcept { value_ = other; return *this; }

    constexpr u8character (self&& other) noexcept : value_(other. value_) {
        other. value_ = _MSTL initialize<value_type>();
    }
    constexpr u8character (value_type&& other) noexcept : value_(other) {}

    constexpr self& operator=(self&& other) noexcept {
        if (this != &other) {
            value_ = other. value_;
            other. value_ = 0;
        }
        return *this;
    }
    constexpr self& operator=(value_type&& other) noexcept {
        value_ = other; return *this;
    }

    MSTL_CONSTEXPR20 ~u8character () = default;

    MSTL_NODISCARD constexpr explicit operator bool() const noexcept {
        return value_ != _MSTL initialize<value_type>();
    }
    MSTL_NODISCARD constexpr operator value_type() const noexcept { return value_; }
    MSTL_NODISCARD constexpr value_type value() const noexcept { return value_; }
    static constexpr size_t bytes() noexcept { return sizeof(value_type); }
    static constexpr size_t bits() noexcept { return sizeof(value_type) * 8; }

    MSTL_NODISCARD constexpr size_t to_hash() const noexcept {
        return _MSTL hash<value_type>()(value_);
    }

    constexpr void swap(self& other) noexcept {
        _MSTL swap(value_, other.value_);
    }

    constexpr bool operator ==(const self& other) const noexcept { return value_ == other.value_; }
    constexpr bool operator !=(const self& other) const noexcept { return value_ != other.value_; }
    constexpr bool operator <(const self& other) const noexcept { return value_ < other.value_; }
    constexpr bool operator <=(const self& other) const noexcept { return value_ <= other.value_; }
    constexpr bool operator >(const self& other) const noexcept { return value_ > other.value_; }
    constexpr bool operator >=(const self& other) const noexcept { return value_ >= other.value_; }

    constexpr bool is_space() const noexcept { return _MSTL is_space(value_); }
    constexpr bool is_alpha() const noexcept { return _MSTL is_alpha(value_); }
    constexpr bool is_digit() const noexcept { return _MSTL is_digit(value_); }
    constexpr bool is_xdigit() const noexcept { return _MSTL is_xdigit(value_); }
    constexpr bool is_alpha_or_digit() const noexcept { return _MSTL is_alpha_or_digit(value_); }
    constexpr bool is_digit_or_alpha() const noexcept { return _MSTL is_digit_or_alpha(value_); }

    constexpr void to_lowercase() noexcept { value_ = _MSTL to_lowercase(value_); }
    constexpr void to_uppercase() noexcept { value_ = _MSTL to_uppercase(value_); }


    static MSTL_CONSTEXPR20 string to_string(const basic_string_view<value_type>& obj) {
        if (obj.empty()) return {};
        string result;
        _INNER append_ascii_chars(result, obj.data(), obj.size());
        return result;
    }

    static MSTL_CONSTEXPR20 wstring to_wstring(const basic_string_view<value_type>& obj) {
        if (obj.empty()) return {};
        wstring result;
        const size_t len = obj.size();
        result.reserve(len);

        size_t i = 0;
        while (i < len) {
            const auto data = reinterpret_cast<const byte_t*>(obj.data());
            uint32_t cp;
            if (_INNER decode_utf8_char(data, i, len, cp)) {
                _INNER codepoint_to_wchar(result, cp);
            } else {
                result.push_back(0xFFFD);
            }
        }
        return result;
    }

    static MSTL_CONSTEXPR20 u8string to_u8string(const basic_string_view<value_type>& obj) {
        return u8string{obj};
    }

    static MSTL_CONSTEXPR20 u16string to_u16string(const basic_string_view<value_type>& obj) {
        if (obj.empty()) return {};
        u16string result;
        const size_t len = obj.size();
        result.reserve(len);

        size_t i = 0;
        while (i < len) {
            const auto data = reinterpret_cast<const byte_t*>(obj.data());
            uint32_t cp;
            if (_INNER decode_utf8_char(data, i, len, cp)) {
                _INNER codepoint_to_utf16(result, cp);
            } else {
                result.push_back(0xFFFD);
            }
        }
        return result;
    }

    static MSTL_CONSTEXPR20 u32string to_u32string(const basic_string_view<value_type>& obj) {
        if (obj.empty()) return {};
        u32string result;
        const size_t len = obj.size();
        result.reserve(len);

        size_t i = 0;
        while (i < len) {
            const auto data = reinterpret_cast<const byte_t*>(obj.data());
            uint32_t cp;
            if (_INNER decode_utf8_char(data, i, len, cp)) {
                result.push_back(static_cast<char32_t>(cp));
            } else {
                result.push_back(0xFFFD);
            }
        }
        return result;
    }
};

template <>
struct package_base<char8_t> {
    using type = u8character;
};
template <>
struct unpackage_base<u8character> {
    using type = char8_t;
};

#endif


struct u16character : icharacter<u16character, char16_t> {
    using value_type = char16_t;
    using self = u16character;
    using base = icharacter<u16character, char16_t>;

private:
    value_type value_ = _MSTL initialize<value_type>();

public:
    constexpr u16character () = default;
    constexpr u16character (const self&) noexcept = default;
    constexpr u16character (const value_type& val) noexcept : value_(val) {}
    constexpr u16character & operator=(const self&) noexcept = default;
    constexpr u16character & operator=(const value_type& other) noexcept { value_ = other; return *this; }

    constexpr u16character (self&& other) noexcept : value_(other. value_) {
        other. value_ = _MSTL initialize<value_type>();
    }
    constexpr u16character (value_type&& other) noexcept : value_(other) {}

    constexpr self& operator=(self&& other) noexcept {
        if (this != &other) {
            value_ = other. value_;
            other. value_ = 0;
        }
        return *this;
    }
    constexpr self& operator=(value_type&& other) noexcept {
        value_ = other; return *this;
    }

    MSTL_CONSTEXPR20 ~u16character () = default;

    MSTL_NODISCARD constexpr explicit operator bool() const noexcept {
        return value_ != _MSTL initialize<value_type>();
    }
    MSTL_NODISCARD constexpr operator value_type() const noexcept { return value_; }
    MSTL_NODISCARD constexpr value_type value() const noexcept { return value_; }
    static constexpr size_t bytes() noexcept { return sizeof(value_type); }
    static constexpr size_t bits() noexcept { return sizeof(value_type) * 8; }

    MSTL_NODISCARD constexpr size_t to_hash() const noexcept {
        return _MSTL hash<value_type>()(value_);
    }

    constexpr void swap(self& other) noexcept {
        _MSTL swap(value_, other.value_);
    }

    constexpr bool operator ==(const self& other) const noexcept { return value_ == other.value_; }
    constexpr bool operator !=(const self& other) const noexcept { return value_ != other.value_; }
    constexpr bool operator <(const self& other) const noexcept { return value_ < other.value_; }
    constexpr bool operator <=(const self& other) const noexcept { return value_ <= other.value_; }
    constexpr bool operator >(const self& other) const noexcept { return value_ > other.value_; }
    constexpr bool operator >=(const self& other) const noexcept { return value_ >= other.value_; }

    constexpr bool is_space() const noexcept { return _MSTL is_space(value_); }
    constexpr bool is_alpha() const noexcept { return _MSTL is_alpha(value_); }
    constexpr bool is_digit() const noexcept { return _MSTL is_digit(value_); }
    constexpr bool is_xdigit() const noexcept { return _MSTL is_xdigit(value_); }
    constexpr bool is_alpha_or_digit() const noexcept { return _MSTL is_alpha_or_digit(value_); }
    constexpr bool is_digit_or_alpha() const noexcept { return _MSTL is_digit_or_alpha(value_); }

    constexpr void to_lowercase() noexcept { value_ = _MSTL to_lowercase(value_); }
    constexpr void to_uppercase() noexcept { value_ = _MSTL to_uppercase(value_); }

    constexpr bool is_high_surrogate() const noexcept { return _MSTL is_high_surrogate(value_); }
    constexpr bool is_low_surrogate() const noexcept { return _MSTL is_low_surrogate(value_); }


    static MSTL_CONSTEXPR20 string to_string(const basic_string_view<value_type>& obj) {
        if (obj.empty()) return {};
        string result;

        size_t start_pos = 0;
        if (!obj.empty() && obj[0] == 0xFEFF) {
            start_pos = 1;
        }

        for (size_t i = start_pos; i < obj.size(); ) {
            uint32_t cp;
            size_t chars_consumed;

            if (_INNER get_utf16_codepoint(obj.data(), i, obj.size(), cp, chars_consumed)) {
                if (cp <= 0x10FFFF && !_MSTL is_high_surrogate(cp) && !_MSTL is_low_surrogate(cp)) {
                    _INNER append_utf8_char(result, cp);
                } else {
                    _INNER append_utf8_char(result, 0xFFFD);
                }
                i += chars_consumed;
            } else {
                _INNER append_utf8_char(result, 0xFFFD);
                i++;
            }
        }
        return result;
    }

    static MSTL_CONSTEXPR20 wstring to_wstring(const basic_string_view<value_type>& obj) {
        if (obj.empty()) return {};
        wstring result;
        result.reserve(obj.size());

        for (size_t i = 0; i < obj.size(); ) {
            if (i == 0 && obj[i] == 0xFEFF) {
                i++;
                continue;
            }

            uint32_t cp;
            size_t chars_consumed;
            if (_INNER get_utf16_codepoint(obj.data(), i, obj.size(), cp, chars_consumed)) {
                _INNER codepoint_to_wchar(result, cp);
                i += chars_consumed;
            } else {
                result.push_back(0xFFFD);
                i++;
            }
        }
        return result;
    }

#ifdef MSTL_VERSION_20__
    static MSTL_CONSTEXPR20 u8string to_u8string(const basic_string_view<value_type>& obj) {
        if (obj.empty()) return {};
        u8string result;
        result.reserve(obj.size() * 3);

        for (size_t i = 0; i < obj.size(); ) {
            if (i == 0 && obj[i] == 0xFEFF) {
                i++;
                continue;
            }

            uint32_t cp;
            size_t chars_consumed;
            if (_INNER get_utf16_codepoint(obj.data(), i, obj.size(), cp, chars_consumed)) {
                _INNER append_utf8_char(result, cp);
                i += chars_consumed;
            } else {
                _INNER append_utf8_char(result, 0xFFFD);
                i++;
            }
        }
        return result;
    }
#endif

    static MSTL_CONSTEXPR20 u16string to_u16string(const basic_string_view<value_type>& obj) {
        return u16string{obj};
    }

    static MSTL_CONSTEXPR20 u32string to_u32string(const basic_string_view<value_type>& obj) {
        if (obj.empty()) return {};
        u32string result;
        result.reserve(obj.size());

        for (size_t i = 0; i < obj.size(); ) {
            if (i == 0 && obj[i] == 0xFEFF) {
                i++;
                continue;
            }

            uint32_t cp;
            size_t chars_consumed;
            if (_INNER get_utf16_codepoint(obj.data(), i, obj.size(), cp, chars_consumed)) {
                result.push_back(static_cast<char32_t>(cp));
                i += chars_consumed;
            } else {
                result.push_back(0xFFFD);
                i++;
            }
        }
        return result;
    }
};

template <>
struct package_base<char16_t> {
    using type = u16character;
};
template <>
struct unpackage_base<u16character> {
    using type = char16_t;
};


struct u32character : icharacter<u32character, char32_t> {
    using value_type = char32_t;
    using self = u32character;
    using base = icharacter<u32character, char32_t>;

private:
    value_type value_ = _MSTL initialize<value_type>();

public:
    constexpr u32character () = default;
    constexpr u32character (const self&) noexcept = default;
    constexpr u32character (const value_type& val) noexcept : value_(val) {}
    constexpr u32character & operator=(const self&) noexcept = default;
    constexpr u32character & operator=(const value_type& other) noexcept { value_ = other; return *this; }

    constexpr u32character (self&& other) noexcept : value_(other. value_) {
        other. value_ = _MSTL initialize<value_type>();
    }
    constexpr u32character (value_type&& other) noexcept : value_(other) {}

    constexpr self& operator=(self&& other) noexcept {
        if (this != &other) {
            value_ = other. value_;
            other. value_ = 0;
        }
        return *this;
    }
    constexpr self& operator=(value_type&& other) noexcept {
        value_ = other; return *this;
    }

    MSTL_CONSTEXPR20 ~u32character () = default;

    MSTL_NODISCARD constexpr explicit operator bool() const noexcept {
        return value_ != _MSTL initialize<value_type>();
    }
    MSTL_NODISCARD constexpr operator value_type() const noexcept { return value_; }
    MSTL_NODISCARD constexpr value_type value() const noexcept { return value_; }
    static constexpr size_t bytes() noexcept { return sizeof(value_type); }
    static constexpr size_t bits() noexcept { return sizeof(value_type) * 8; }

    MSTL_NODISCARD constexpr size_t to_hash() const noexcept {
        return _MSTL hash<value_type>()(value_);
    }

    constexpr void swap(self& other) noexcept {
        _MSTL swap(value_, other.value_);
    }

    constexpr bool operator ==(const self& other) const noexcept { return value_ == other.value_; }
    constexpr bool operator !=(const self& other) const noexcept { return value_ != other.value_; }
    constexpr bool operator <(const self& other) const noexcept { return value_ < other.value_; }
    constexpr bool operator <=(const self& other) const noexcept { return value_ <= other.value_; }
    constexpr bool operator >(const self& other) const noexcept { return value_ > other.value_; }
    constexpr bool operator >=(const self& other) const noexcept { return value_ >= other.value_; }

    constexpr bool is_space() const noexcept { return _MSTL is_space(value_); }
    constexpr bool is_alpha() const noexcept { return _MSTL is_alpha(value_); }
    constexpr bool is_digit() const noexcept { return _MSTL is_digit(value_); }
    constexpr bool is_xdigit() const noexcept { return _MSTL is_xdigit(value_); }
    constexpr bool is_alpha_or_digit() const noexcept { return _MSTL is_alpha_or_digit(value_); }
    constexpr bool is_digit_or_alpha() const noexcept { return _MSTL is_digit_or_alpha(value_); }

    constexpr void to_lowercase() noexcept { value_ = _MSTL to_lowercase(value_); }
    constexpr void to_uppercase() noexcept { value_ = _MSTL to_uppercase(value_); }


    static MSTL_CONSTEXPR20 string to_string(const basic_string_view<value_type>& obj) {
        if (obj.empty()) return {};
        string result;
        for (size_t i = 0; i < obj.size(); ++i) {
            _INNER append_utf8_char(result, obj[i]);
        }
        return result;
    }

    static MSTL_CONSTEXPR20 wstring to_wstring(const basic_string_view<value_type>& obj) {
        if (obj.empty()) return {};
        wstring result;
        result.reserve(obj.size());
        for (size_t i = 0; i < obj.size(); ++i) {
            _INNER codepoint_to_wchar(result, obj[i]);
        }
        return result;
    }

#ifdef MSTL_VERSION_20__
    static MSTL_CONSTEXPR20 u8string to_u8string(const basic_string_view<value_type>& obj) {
        if (obj.empty()) return {};
        u8string result;
        result.reserve(obj.size() * 4);
        for (size_t i = 0; i < obj.size(); ++i) {
            _INNER append_utf8_char(result, obj[i]);
        }
        return result;
    }
#endif

    static MSTL_CONSTEXPR20 u16string to_u16string(const basic_string_view<value_type>& obj) {
        if (obj.empty()) return {};
        u16string result;
        result.reserve(obj.size() * 2);
        for (size_t i = 0; i < obj.size(); ++i) {
            _INNER codepoint_to_utf16(result, obj[i]);
        }
        return result;
    }

    static MSTL_CONSTEXPR20 u32string to_u32string(const basic_string_view<value_type>& obj) {
        return u32string{obj};
    }
};

template <>
struct package_base<char32_t> {
    using type = u32character;
};
template <>
struct unpackage_base<u32character> {
    using type = char32_t;
};


MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string(nullptr_t) {
    return {"nullptr"};
}
template <typename T, enable_if_t<is_pointer_v<T> && !is_cstring_v<T>, int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string(const T& x) {
    return _MSTL address_string(x);
}

template <typename T, enable_if_t<is_union_v<T>, int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string(const T& x) {
    return _MSTL address_string(&x);
}


template <typename T, enable_if_t<is_function_v<T>, int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string(T&& x);

template <typename T, enable_if_t<is_member_object_pointer_v<T>, int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string(T&& x);

template <typename T, enable_if_t<is_member_function_pointer_v<T>, int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string(T&& x);


MSTL_BEGIN_INNER__
template <typename Collector>
MSTL_NODISCARD MSTL_CONSTEXPR20 string collector_to_string(const Collector& c) {
    if (_MSTL empty(c)) return {"[]"};
    string result;
    result += "[ ";
    for (auto iter = _MSTL cbegin(c); iter != _MSTL cend(c); ++iter) {
        if (iter != _MSTL cbegin(c)) result += ", ";
        string tmp = to_string(*iter);
        result += tmp;
    }
    result += " ]";
    return result;
}
MSTL_END_INNER__

template <typename T, enable_if_t<is_unbounded_array_v<T>, int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string(const T&) {
    return {"[]"};
}
template <typename T, enable_if_t<is_bounded_array_v<T> && !is_cstring_v<T>, int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string(const T& x) {
    return _INNER collector_to_string(x);
}


template <typename T, enable_if_t<is_base_of_v<_MSTL Error, T>, int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string(const T& obj) {
    return string(obj.type_) + "(" + obj.info_ + ")";
}


template <typename IfEmpty, typename T>
MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string(const compressed_pair<IfEmpty, T, true>& obj) {
    return to_string(obj.value);
}
template <typename IfEmpty, typename T>
MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string(const compressed_pair<IfEmpty, T, false>& obj) {
    return "{ " + to_string(obj.value) + ", " + to_string(obj.no_compressed) + " }";
}


template <typename T1, typename T2>
MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string(const pair<T1, T2>& obj) {
    return "{ " + to_string(obj.first) + ", " + to_string(obj.second) + " }";
}


MSTL_BEGIN_INNER__
template <typename Tuple, size_t I, enable_if_t<I == tuple_size_v<Tuple> - 1, int> = 0>
MSTL_CONSTEXPR20 void __to_string_tuple_elements(const Tuple& t, string& result) {
    result += to_string(_MSTL get<I>(t));
}
template <typename Tuple, size_t I, enable_if_t<I < tuple_size_v<Tuple> - 1, int> = 0>
MSTL_CONSTEXPR20 void __to_string_tuple_elements(const Tuple& t, string& result) {
    result += to_string(_MSTL get<I>(t)) + ", ";
    _INNER __to_string_tuple_elements<Tuple, I + 1>(t, result);
}
template <typename... UArgs, enable_if_t<sizeof...(UArgs) == 0, int> = 0>
MSTL_CONSTEXPR20 string __to_string_tuple_dispatch(const tuple<UArgs...>&) {
    return {"()"};
}
template <typename... UArgs, enable_if_t<sizeof...(UArgs) != 0, int> = 0>
MSTL_CONSTEXPR20 string __to_string_tuple_dispatch(const tuple<UArgs...>& t) {
    string result;
    result += "( ";
    _INNER __to_string_tuple_elements<decltype(t), 0>(t, result);
    result += " )";
    return result;
}
MSTL_END_INNER__

template <typename... Args>
MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string(const tuple<Args...>& t) {
    return _INNER __to_string_tuple_dispatch(t);
}


template <typename T>
MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string(const shared_ptr<T>& sp) {
    return address_string(sp.get());
}
template <typename T, typename Deleter>
MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string(const unique_ptr<T, Deleter>& sp) {
    return address_string(sp.get());
}


MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string(const bstring& x) {
    return string(x.begin(), x.end());
}
MSTL_NODISCARD MSTL_CONSTEXPR20 bstring to_bstring(const string& x) {
    return bstring(x.begin(), x.end());
}
MSTL_NODISCARD MSTL_CONSTEXPR20 bstring to_bstring(const string_view x) {
    return bstring(x.begin(), x.end());
}


#ifndef MSTL_VERSION_17__

MSTL_BEGIN_INNER__
template <typename T>
string to_string_concat(T&& t) {
    return to_string(_MSTL forward<T>(t));
}
template <typename First, typename... Rest>
string to_string_concat(First&& first, Rest&&... rest) {
    return to_string(_MSTL forward<First>(first)) + to_string_concat(_MSTL forward<Rest>(rest)...);
}
MSTL_END_INNER__

template <typename... Args, enable_if_t<(sizeof...(Args) > 1), int> = 0>
MSTL_NODISCARD string to_string(Args&&... args) {
    return _INNER to_string_concat(_MSTL forward<Args>(args)...);
}

#else
template <typename... Args, enable_if_t<(sizeof...(Args) > 1), int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string(Args&&... args) {
    return (to_string(_MSTL forward<Args>(args)) + ...);
}
#endif


MSTL_BEGIN_INNER__

#ifndef MSTL_DATA_BUS_WIDTH_64__
template <typename CharT, typename UT, enable_if_t<(sizeof(UT) > 4), int> = 0>
constexpr void __uint_to_buff_aux(CharT* riter, UT& ux) noexcept {
    while (ux > static_cast<UT>(0xFFFFFFFFU)) {
        auto chunk = static_cast<uint32_t>(ux % static_cast<UT>(1000000000));
        ux /= static_cast<UT>(1000000000);
        for (int idx = 0; idx != 9; ++idx) {
            *--riter = static_cast<CharT>('0' + chunk % 10);
            chunk /= 10;
        }
    }
}
template <typename CharT, typename UT, enable_if_t<sizeof(UT) <= 4, int> = 0>
constexpr void __uint_to_buff_aux(CharT*, UT&) noexcept {}
#endif // MSTL_DATA_BUS_WIDTH_64__

template <typename CharT, typename UT, enable_if_t<is_unsigned<UT>::value, int> = 0>
MSTL_NODISCARD constexpr CharT* __uint_to_buff(CharT* riter, UT ux) noexcept {
#ifdef MSTL_DATA_BUS_WIDTH_64__
    UT holder = ux;
#else
    _INNER __uint_to_buff_aux(riter, ux);
    auto holder = static_cast<uint32_t>(ux);
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
    } else {
        rnext = _INNER __uint_to_buff(rnext, unsigned_x);
    }
    const size_t count = buffer_end - rnext;
    _MSTL memory_zero(buffer, count);
    return basic_string<CharT>(rnext, count);
}

template <typename CharT, typename T, enable_if_t<conjunction<is_integral<T>, is_unsigned<T>>::value, int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 basic_string<CharT> __uint_to_string(T x) {
    CharT buffer[21];
    CharT* const buffer_end = buffer + 21;
    CharT* const rnext = _INNER __uint_to_buff(buffer_end, x);
    const size_t count = buffer_end - rnext;
    _MSTL memory_zero(buffer, count);
    return basic_string<CharT>(rnext, count);
}

MSTL_CONSTEXPR20 string __uint_to_string_base(uint64_t value, const int base, const bool uppercase) {
    if (value == 0) {
        return "0";
    }
    string result;
    constexpr auto digits_lower = "0123456789abcdef";
    constexpr auto digits_upper = "0123456789ABCDEF";
    const auto digits = uppercase ? digits_upper : digits_lower;
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
    T x, int precision = 6, const bool force_scientific = false, const bool force_fixed = false) {
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

MSTL_END_INNER__

MSTL_END_NAMESPACE__
#endif // MSTL_STRING_HPP__
