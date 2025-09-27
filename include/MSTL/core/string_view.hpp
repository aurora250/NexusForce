#ifndef MSTL_STRING_VIEW_HPP__
#define MSTL_STRING_VIEW_HPP__
#include "basic_string_view.hpp"
MSTL_BEGIN_NAMESPACE__

using string_view = basic_string_view<char>;
using bstring_view = basic_string_view<byte_t>;
using wstring_view = basic_string_view<wchar_t>;
#ifdef MSTL_VERSION_20__
using u8string_view = basic_string_view<char8_t>;
#endif // MSTL_VERSION_20__
using u16string_view = basic_string_view<char16_t>;
using u32string_view = basic_string_view<char32_t>;


#ifdef MSTL_VERSION_17__

inline namespace string_operator {
    MSTL_NODISCARD constexpr string_view operator ""_sv(const char* str, size_t len) noexcept {
        return {str, len};
    }
    MSTL_NODISCARD constexpr wstring_view operator ""_sv(const wchar_t* str, size_t len) noexcept {
        return {str, len};
    }
#ifdef MSTL_VERSION_20__
    MSTL_NODISCARD constexpr u8string_view operator ""_sv(const char8_t* str, size_t len) noexcept {
        return {str, len};
    }
#endif // MSTL_VERSION_20__
    MSTL_NODISCARD constexpr u16string_view operator ""_sv(const char16_t* str, size_t len) noexcept {
        return {str, len};
    }
    MSTL_NODISCARD constexpr u32string_view operator ""_sv(const char32_t* str, size_t len) noexcept {
        return {str, len};
    }
}

#endif // MSTL_VERSION_17__


inline size_t string_hash(const char* s, size_t len, uint32_t seed) noexcept {
#if defined(MSTL_DATA_BUS_WIDTH_64__)
    const pair<size_t, size_t> p = MurmurHash_x64(s, len, seed);
    return p.first ^ p.second;
#elif defined(MSTL_DATA_BUS_WIDTH_32__)
    return (size_t)MurmurHash_x32(s, len, seed);
#else 
    return DJB2_hash(s, len);
#endif
}

template <>
struct hash<char*> {
    MSTL_NODISCARD size_t operator ()(const char* str) const noexcept {
        return string_hash(str, _MSTL string_length(str), 0);
    }
}; 
template <>
struct hash<const char*> {
    MSTL_NODISCARD size_t operator ()(const char* str) const noexcept {
        return string_hash(str, _MSTL string_length(str), 0);
    }
};

#define CHAR_PTR_HASH_STRUCTS__(OPT) \
    template <> \
    struct hash<OPT*> { \
        MSTL_NODISCARD size_t operator ()(const OPT* str) const noexcept { \
            return FNV_hash(reinterpret_cast<const byte_t*>(str), sizeof(OPT) * char_traits<OPT>::length(str)); \
        } \
    }; \
    template <> \
    struct hash<const OPT*> { \
        MSTL_NODISCARD size_t operator ()(const OPT* str) const noexcept { \
            return FNV_hash(reinterpret_cast<const byte_t*>(str), sizeof(OPT) * char_traits<OPT>::length(str)); \
        } \
    };

CHAR_PTR_HASH_STRUCTS__(wchar_t)
MSTL_MACRO_RANGES_UNICODE_CHARS(CHAR_PTR_HASH_STRUCTS__)
#undef CHAR_PTR_HASH_STRUCTS__

template <>
struct hash<_MSTL string_view> {
    MSTL_NODISCARD size_t operator ()(const _MSTL string_view str) const noexcept {
        return string_hash(str.data(), str.size(), 0);
    }
};
template <typename CharT, typename Traits>
struct hash<basic_string_view<CharT, Traits>> {
    MSTL_NODISCARD constexpr size_t operator ()(
        const basic_string_view<CharT, Traits> str) const noexcept {
        return FNV_hash(reinterpret_cast<const byte_t*>(str.data()), sizeof(CharT) * str.size());
    }
};

MSTL_END_NAMESPACE__
#endif // MSTL_STRING_VIEW_HPP__
