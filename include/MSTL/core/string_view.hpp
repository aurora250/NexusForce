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
MSTL_BEGIN_LITERALS__

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

MSTL_END_LITERALS__
#endif // MSTL_VERSION_17__

MSTL_END_NAMESPACE__
#endif // MSTL_STRING_VIEW_HPP__
