#ifndef MSTL_CORE_STRING_CHARACTER_HPP__
#define MSTL_CORE_STRING_CHARACTER_HPP__
#include "../interface/icharacter.hpp"
MSTL_BEGIN_NAMESPACE__

MSTL_BEGIN_INNER__

template <typename T>
MSTL_ALWAYS_INLINE constexpr void __append_utf8_char_aux(T&) {}
template <>
MSTL_CONSTEXPR20 void __append_utf8_char_aux<string>(string& result) {
    result.append("\xEF\xBF\xBD", 3);
}
#ifdef MSTL_STANDARD_20__
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
    constexpr bool operator <(const self& other) const noexcept { return value_ < other.value_; }

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

#ifdef MSTL_STANDARD_20__
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
    constexpr bool operator <(const self& other) const noexcept { return value_ < other.value_; }

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

#ifdef MSTL_STANDARD_20__
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


#ifdef MSTL_STANDARD_20__

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
    constexpr bool operator <(const self& other) const noexcept { return value_ < other.value_; }

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
    constexpr bool operator <(const self& other) const noexcept { return value_ < other.value_; }

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

#ifdef MSTL_STANDARD_20__
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
    constexpr bool operator <(const self& other) const noexcept { return value_ < other.value_; }

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

#ifdef MSTL_STANDARD_20__
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

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_STRING_CHARACTER_HPP__
