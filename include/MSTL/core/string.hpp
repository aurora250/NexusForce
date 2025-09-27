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

template <>
struct hash<_MSTL string> {
    MSTL_NODISCARD size_t operator ()(const _MSTL string& s) const noexcept {
        return string_hash(s.c_str(), _MSTL string_length(s.c_str()), 0);
    }
};

template <typename CharT, typename Traits, typename Alloc>
struct hash<basic_string<CharT, Traits, Alloc>> {
    MSTL_NODISCARD size_t operator ()(
        const basic_string<CharT, Traits, Alloc>& str) const noexcept {
        return FNV_hash(reinterpret_cast<const byte_t*>(str.c_str()), sizeof(CharT) * str.size());
    }
};


#ifdef MSTL_VERSION_17__
inline namespace literals {
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
}
#endif // MSTL_VERSION_17__


MSTL_API string wstring_to_utf8(const wchar_t* str);

#ifdef MSTL_VERSION_20__
inline string u8string_to_utf8(const char8_t* str) {
    return {reinterpret_cast<const char*>(str)};
}
#endif

#ifdef MSTL_PLATFORM_WINDOWS__
inline string u16string_to_utf8(const char16_t* str) {
    return wstring_to_utf8(reinterpret_cast<const wchar_t*>(str));
}
#elif defined(MSTL_PLATFORM_LINUX__)
constexpr bool is_high_surrogate(const char16_t c) {
    return (c >= 0xD800 && c <= 0xDBFF);
}
constexpr bool is_low_surrogate(const char16_t c) {
    return (c >= 0xDC00 && c <= 0xDFFF);
}

constexpr uint32_t combine_surrogates(const char16_t high, const char16_t low) {
    return 0x10000 + ((static_cast<uint32_t>(high) - 0xD800) << 10) + (static_cast<uint32_t>(low) - 0xDC00);
}

MSTL_API string u16string_to_utf8(const char16_t* str);
#endif

MSTL_API string u32string_to_utf8(const char32_t* str);


MSTL_API string escape_string(const string& str);


MSTL_BEGIN_INNER__

#ifndef MSTL_DATA_BUS_WIDTH_64__
template <typename CharT, typename UT, enable_if_t<(sizeof(UT) > 4), int> = 0>
void __uint_to_buff_aux(CharT* riter, UT& ux) {
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
void __uint_to_buff_aux(CharT*, UT&) {}
#endif // MSTL_DATA_BUS_WIDTH_64__

template <typename CharT, typename UT>
MSTL_NODISCARD CharT* __uint_to_buff(CharT* riter, UT ux) {
    static_assert(is_unsigned<UT>::value, "UT must be unsigned types.");
#ifdef MSTL_DATA_BUS_WIDTH_64__
    auto holder = ux;
#else
    _INNER __uint_to_buff_aux(riter, ux);
    auto holder = static_cast<unsigned long>(ux);
#endif
    do {
        *--riter = static_cast<CharT>('0' + holder % 10);
        holder /= 10;
    } while (holder != 0);
    return riter;
}

template <typename CharT, typename T, enable_if_t<is_integral<T>::value, int> = 0>
MSTL_NODISCARD basic_string<CharT> __int_to_string(const T x) {
    CharT buffer[21];
    CharT* const buffer_end = buffer + 21;
    CharT* rnext = buffer_end;
    const auto unsigned_x = static_cast<make_unsigned_t<T>>(x);
    if (x < 0) {
        rnext = (__uint_to_buff)(rnext, 0 - unsigned_x);
        *--rnext = '-';
    }
    else 
        rnext = (__uint_to_buff)(rnext, unsigned_x);
    return basic_string<CharT>(rnext, buffer_end);
}

template <typename CharT, typename T,
    enable_if_t<conjunction_v<is_integral<T>, is_unsigned<T>>, int> = 0>
MSTL_NODISCARD basic_string<CharT> __uint_to_string(const T x) {
    CharT buffer[21];
    CharT* const buffer_end = buffer + 21;
    CharT* const rnext = (__uint_to_buff)(buffer_end, x);
    return basic_string<CharT>(rnext, buffer_end);
}

template <typename CharT, typename T, enable_if_t<is_floating_point<T>::value, int> = 0>
MSTL_NODISCARD basic_string<CharT> __float_to_string(const T x) {
#ifdef MSTL_PLATFORM_LINUX__
    const int len = ::snprintf(nullptr, 0, "%f", x);
    string str(len, '\0');
    ::snprintf(&str[0], len + 1, "%f", x);
    return str;
#else
    const auto len = static_cast<size_t>(::_scprintf("%f", x));
    string str(len, '\0');
    ::sprintf_s(&str[0], len + 1, "%f", x);
    return str;
#endif
}

MSTL_END_INNER__


template <typename T, enable_if_t<is_boolean_v<T>, int> = 0>
MSTL_NODISCARD string to_string(const T x) {
    if (x) return {"true"};
    return {"false"};
}
template <typename T, enable_if_t<conjunction_v<is_standard_integral<T>, is_signed<T>>, int> = 0>
MSTL_NODISCARD string to_string(const T x) {
    return _INNER __int_to_string<char>(x);
}
template <typename T, enable_if_t<conjunction_v<is_standard_integral<T>, is_unsigned<T>>, int> = 0>
MSTL_NODISCARD string to_string(const T x) {
    return _INNER __uint_to_string<char>(x);
}
template <typename T, enable_if_t<is_floating_point<T>::value, int> = 0>
MSTL_NODISCARD string to_string(const T x) {
    return _INNER __float_to_string<char>(x);
}


MSTL_BEGIN_INNER__

MSTL_NODISCARD inline string __to_string_dispatch(const char* x) {
    return {x};
}
MSTL_NODISCARD inline string __to_string_dispatch(const wchar_t* x) {
    return wstring_to_utf8(x);
}
#ifdef MSTL_VERSION_20__
MSTL_NODISCARD inline string __to_string_dispatch(const char8_t* x) {
    return u8string_to_utf8(x);
}
#endif
MSTL_NODISCARD inline string __to_string_dispatch(const char16_t* x) {
    return u16string_to_utf8(x);
}
MSTL_NODISCARD inline string __to_string_dispatch(const char32_t* x) {
    return u32string_to_utf8(x);
}

MSTL_END_INNER__

template <typename CharT, enable_if_t<is_character_v<CharT>, int> = 0>
MSTL_NODISCARD string to_string(const CharT x) {
    CharT str[2] = { x, static_cast<CharT>(0) };
    return _INNER __to_string_dispatch(str);
}
template <typename T, enable_if_t<is_ctype_string_v<T>, int> = 0>
MSTL_NODISCARD string to_string(const T& x) {
    return _INNER __to_string_dispatch(x);
}
template <typename CharT>
MSTL_NODISCARD string to_string(const basic_string_view<CharT> x) {
    return _INNER __to_string_dispatch(x.data());
}
template <typename CharT>
MSTL_NODISCARD string to_string(const basic_string<CharT>& x) {
    return _INNER __to_string_dispatch(x);
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
