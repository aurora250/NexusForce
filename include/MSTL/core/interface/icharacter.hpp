#ifndef MSTL_CORE_INTERFACE_ICHARACTER_HPP__
#define MSTL_CORE_INTERFACE_ICHARACTER_HPP__
#include "../string/string.hpp"
MSTL_BEGIN_NAMESPACE__

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
#ifdef MSTL_STANDARD_20__
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
