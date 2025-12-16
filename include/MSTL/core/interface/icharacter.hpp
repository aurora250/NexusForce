#ifndef MSTL_CORE_INTERFACE_ICHARACTER_HPP__
#define MSTL_CORE_INTERFACE_ICHARACTER_HPP__
#include "../string/string.hpp"
#include "ipackage.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename T, typename CharT>
struct icharacter : ipackage<T, CharT> {
    static_assert(is_character_v<CharT>, "icharacter can only be used with characters");

    constexpr icharacter() noexcept = default;
    constexpr icharacter(CharT value) noexcept : ipackage<T, CharT>(value) {}

    MSTL_NODISCARD constexpr explicit operator bool() const noexcept {
        return ipackage<T, CharT>::value_ != _MSTL initialize<CharT>();
    }

    static MSTL_CONSTEXPR20 string to_string(const basic_string_view<CharT>& obj) {
        return T::to_string(obj);
    }
    static MSTL_CONSTEXPR20 wstring to_wstring(const basic_string_view<CharT>& obj) {
        return T::to_wstring(obj);
    }
#ifdef MSTL_STANDARD_20__
    static MSTL_CONSTEXPR20 u8string to_u8string(const basic_string_view<CharT>& obj) {
        return T::to_u8string(obj);
    }
#endif
    static MSTL_CONSTEXPR20 u16string to_u16string(const basic_string_view<CharT>& obj) {
        return T::to_u16string(obj);
    }
    static MSTL_CONSTEXPR20 u32string to_u32string(const basic_string_view<CharT>& obj) {
        return T::to_u32string(obj);
    }
};

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

#ifdef MSTL_STANDARD_20__
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

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_INTERFACE_ICHARACTER_HPP__
