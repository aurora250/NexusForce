#ifndef NEFORCE_CORE_INTERFACE_ICHARACTER_HPP__
#define NEFORCE_CORE_INTERFACE_ICHARACTER_HPP__

/**
 * @file icharacter.hpp
 * @brief 字符类型接口
 *
 * 此文件提供了字符类型的通用接口，
 * 支持不同字符类型到各种字符串类型的转换功能。
 */

#include "NeForce/core/interface/ipackage.hpp"
#include "NeForce/core/string/string.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup CRTPInterfaces CRTP接口
 * @brief 提供基本功能的CRTP基类
 * @{
 */

/**
 * @struct icharacter
 * @brief 字符类型接口
 * @tparam T 派生类型
 * @tparam CharT 字符类型
 *
 * 为字符包装类型提供统一的接口，支持字符到各种字符串类型的转换。
 */
template <typename T, typename CharT>
struct icharacter : ipackage<T, CharT> {
    static_assert(is_character_v<CharT>, "CharT must be character.");

    constexpr icharacter() noexcept = default;
    NEFORCE_CONSTEXPR20 ~icharacter() = default;

    constexpr icharacter(const icharacter&) noexcept = default;
    constexpr icharacter(icharacter&&) noexcept = default;

    constexpr icharacter& operator =(const icharacter&) noexcept = default;
    constexpr icharacter& operator =(icharacter&&) noexcept = default;

    /**
     * @brief 构造函数
     * @param value 字符值
     */
    constexpr icharacter(CharT value) noexcept
    : ipackage<T, CharT>(value) {}

    /**
     * @brief 转换为bool操作符
     * @return 字符是否非空
     */
    NEFORCE_NODISCARD constexpr explicit operator bool() const noexcept {
        return ipackage<T, CharT>::value_ != static_cast<CharT>(0);
    }

    /**
     * @brief 转换为普通字符串
     * @param obj 字符视图
     * @return 普通字符串
     */
    static NEFORCE_CONSTEXPR20 string to_string(const basic_string_view<CharT>& obj) {
        return T::to_string(obj);
    }

    /**
     * @brief 转换为宽字符串
     * @param obj 字符视图
     * @return 宽字符串
     */
    static NEFORCE_CONSTEXPR20 wstring to_wstring(const basic_string_view<CharT>& obj) {
        return T::to_wstring(obj);
    }

#ifdef NEFORCE_STANDARD_20
    /**
     * @brief 转换为UTF-8字符串
     * @param obj 字符视图
     * @return UTF-8字符串
     */
    static NEFORCE_CONSTEXPR20 u8string to_u8string(const basic_string_view<CharT>& obj) {
        return T::to_u8string(obj);
    }
#endif

    /**
     * @brief 转换为UTF-16字符串
     * @param obj 字符视图
     * @return UTF-16字符串
     */
    static NEFORCE_CONSTEXPR20 u16string to_u16string(const basic_string_view<CharT>& obj) {
        return T::to_u16string(obj);
    }

    /**
     * @brief 转换为UTF-32字符串
     * @param obj 字符视图
     * @return UTF-32字符串
     */
    static NEFORCE_CONSTEXPR20 u32string to_u32string(const basic_string_view<CharT>& obj) {
        return T::to_u32string(obj);
    }
};

/** @} */ // CRTPInterfaces

/**
 * @defgroup ToString 转换字符串
 * @brief 各类型到字符串的转换函数
 * @{
 */

/**
 * @brief 将字符转换为普通字符串
 * @tparam CharT 字符类型
 * @param c 单个字符
 * @return 转换后的普通字符串
 */
template <typename CharT, enable_if_t<is_character_v<CharT>, int> = 0>
NEFORCE_CONSTEXPR20 string to_string(const CharT& c) {
    return icharacter<package_t<CharT>, CharT>::to_string(basic_string<CharT>(1, c).view());
}

/**
 * @brief 将C风格字符串转换为普通字符串
 * @tparam CharT 字符类型
 * @param x C风格字符串
 * @return 转换后的普通字符串
 */
template <typename CharT, enable_if_t<is_character_v<CharT>, int> = 0>
NEFORCE_CONSTEXPR20 string to_string(const CharT* x) {
    return icharacter<package_t<CharT>, CharT>::to_string(basic_string_view<CharT>(x));
}

/**
 * @brief 将字符视图转换为普通字符串
 * @tparam CharT 字符类型
 * @param x 字符视图
 * @return 转换后的普通字符串
 */
template <typename CharT, enable_if_t<is_character_v<CharT>, int> = 0>
NEFORCE_CONSTEXPR20 string to_string(const basic_string_view<CharT> x) {
    return icharacter<package_t<CharT>, CharT>::to_string(x);
}

/**
 * @brief 将字符串对象转换为普通字符串
 * @tparam CharT 字符类型
 * @param x 字符串对象
 * @return 转换后的普通字符串
 */
template <typename CharT, enable_if_t<is_character_v<CharT>, int> = 0>
NEFORCE_CONSTEXPR20 string to_string(const basic_string<CharT>& x) {
    return icharacter<package_t<CharT>, CharT>::to_string(x.view());
}

/**
 * @brief 将右值字符串对象转换为普通字符串
 * @tparam CharT 字符类型
 * @param x 右值字符串对象
 * @return 转换后的普通字符串
 */
template <typename CharT, enable_if_t<is_character_v<CharT>, int> = 0>
NEFORCE_CONSTEXPR20 string to_string(basic_string<CharT>&& x) {
    return icharacter<package_t<CharT>, CharT>::to_string(x.view());
}

/**
 * @brief char类型右值字符串特化 - 直接移动
 * @param x 右值普通字符串
 * @return 移动后的普通字符串
 */
template <>
NEFORCE_CONSTEXPR20 string to_string<char>(string&& x) {
    return _NEFORCE move(x);
}

/**
 * @brief 将字符转换为宽字符串
 * @tparam CharT 字符类型
 * @param x 单个字符
 * @return 转换后的宽字符串
 */
template <typename CharT, enable_if_t<is_character_v<CharT>, int> = 0>
NEFORCE_CONSTEXPR20 wstring to_wstring(const CharT& x) {
    return icharacter<package_t<CharT>, CharT>::to_wstring(basic_string<CharT>(1, x).view());
}

/**
 * @brief 将C风格字符串转换为宽字符串
 * @tparam CharT 字符类型
 * @param x C风格字符串
 * @return 转换后的宽字符串
 */
template <typename CharT, enable_if_t<is_character_v<CharT>, int> = 0>
NEFORCE_CONSTEXPR20 wstring to_wstring(const CharT* x) {
    return icharacter<package_t<CharT>, CharT>::to_wstring(basic_string_view<CharT>(x));
}

/**
 * @brief 将字符视图转换为宽字符串
 * @tparam CharT 字符类型
 * @param x 字符视图
 * @return 转换后的宽字符串
 */
template <typename CharT, enable_if_t<is_character_v<CharT>, int> = 0>
NEFORCE_CONSTEXPR20 wstring to_wstring(const basic_string_view<CharT> x) {
    return icharacter<package_t<CharT>, CharT>::to_wstring(x);
}

/**
 * @brief 将字符串对象转换为宽字符串
 * @tparam CharT 字符类型
 * @param x 字符串对象
 * @return 转换后的宽字符串
 */
template <typename CharT, enable_if_t<is_character_v<CharT>, int> = 0>
NEFORCE_CONSTEXPR20 wstring to_wstring(const basic_string<CharT>& x) {
    return icharacter<package_t<CharT>, CharT>::to_wstring(x.view());
}

/**
 * @brief 将右值字符串对象转换为宽字符串
 * @tparam CharT 字符类型
 * @param x 右值字符串对象
 * @return 转换后的宽字符串
 */
template <typename CharT, enable_if_t<is_character_v<CharT>, int> = 0>
NEFORCE_CONSTEXPR20 wstring to_wstring(basic_string<CharT>&& x) {
    return icharacter<package_t<CharT>, CharT>::to_wstring(x.view());
}

/**
 * @brief wchar_t类型右值宽字符串特化 - 直接移动
 * @param x 右值宽字符串
 * @return 移动后的宽字符串
 */
template <>
NEFORCE_CONSTEXPR20 wstring to_wstring<wchar_t>(wstring&& x) {
    return _NEFORCE move(x);
}

#ifdef NEFORCE_STANDARD_20

/**
 * @brief 将字符转换为UTF-8字符串
 * @tparam CharT 字符类型
 * @param x 单个字符
 * @return 转换后的UTF-8字符串
 */
template <typename CharT, enable_if_t<is_character_v<CharT>, int> = 0>
NEFORCE_CONSTEXPR20 u8string to_u8string(const CharT& x) {
    return icharacter<package_t<CharT>, CharT>::to_u8string(basic_string<CharT>(1, x).view());
}

/**
 * @brief 将C风格字符串转换为UTF-8字符串
 * @tparam CharT 字符类型
 * @param x C风格字符串
 * @return 转换后的UTF-8字符串
 */
template <typename CharT, enable_if_t<is_character_v<CharT>, int> = 0>
NEFORCE_CONSTEXPR20 u8string to_u8string(const CharT* x) {
    return icharacter<package_t<CharT>, CharT>::to_u8string(basic_string_view<CharT>(x));
}

/**
 * @brief 将字符视图转换为UTF-8字符串
 * @tparam CharT 字符类型
 * @param x 字符视图
 * @return 转换后的UTF-8字符串
 */
template <typename CharT, enable_if_t<is_character_v<CharT>, int> = 0>
NEFORCE_CONSTEXPR20 u8string to_u8string(const basic_string_view<CharT> x) {
    return icharacter<package_t<CharT>, CharT>::to_u8string(x);
}

/**
 * @brief 将字符串对象转换为UTF-8字符串
 * @tparam CharT 字符类型
 * @param x 字符串对象
 * @return 转换后的UTF-8字符串
 */
template <typename CharT, enable_if_t<is_character_v<CharT>, int> = 0>
NEFORCE_CONSTEXPR20 u8string to_u8string(const basic_string<CharT>& x) {
    return icharacter<package_t<CharT>, CharT>::to_u8string(x.view());
}

/**
 * @brief 将右值字符串对象转换为UTF-8字符串
 * @tparam CharT 字符类型
 * @param x 右值字符串对象
 * @return 转换后的UTF-8字符串
 */
template <typename CharT, enable_if_t<is_character_v<CharT>, int> = 0>
NEFORCE_CONSTEXPR20 u8string to_u8string(basic_string<CharT>&& x) {
    return icharacter<package_t<CharT>, CharT>::to_u8string(x.view());
}

/**
 * @brief char8_t类型右值UTF-8字符串特化 - 直接移动
 * @param x 右值UTF-8字符串
 * @return 移动后的UTF-8字符串
 */
template <>
NEFORCE_CONSTEXPR20 u8string to_u8string<char8_t>(u8string&& x) {
    return _NEFORCE move(x);
}

#endif

/**
 * @brief 将字符转换为UTF-16字符串
 * @tparam CharT 字符类型
 * @param x 单个字符
 * @return 转换后的UTF-16字符串
 */
template <typename CharT, enable_if_t<is_character_v<CharT>, int> = 0>
NEFORCE_CONSTEXPR20 u16string to_u16string(const CharT& x) {
    return icharacter<package_t<CharT>, CharT>::to_u16string(basic_string<CharT>(1, x).view());
}

/**
 * @brief 将C风格字符串转换为UTF-16字符串
 * @tparam CharT 字符类型
 * @param x C风格字符串
 * @return 转换后的UTF-16字符串
 */
template <typename CharT, enable_if_t<is_character_v<CharT>, int> = 0>
NEFORCE_CONSTEXPR20 u16string to_u16string(const CharT* x) {
    return icharacter<package_t<CharT>, CharT>::to_u16string(basic_string_view<CharT>(x));
}

/**
 * @brief 将字符视图转换为UTF-16字符串
 * @tparam CharT 字符类型
 * @param x 字符视图
 * @return 转换后的UTF-16字符串
 */
template <typename CharT, enable_if_t<is_character_v<CharT>, int> = 0>
NEFORCE_CONSTEXPR20 u16string to_u16string(const basic_string_view<CharT> x) {
    return icharacter<package_t<CharT>, CharT>::to_u16string(x);
}

/**
 * @brief 将字符串对象转换为UTF-16字符串
 * @tparam CharT 字符类型
 * @param x 字符串对象
 * @return 转换后的UTF-16字符串
 */
template <typename CharT, enable_if_t<is_character_v<CharT>, int> = 0>
NEFORCE_CONSTEXPR20 u16string to_u16string(const basic_string<CharT>& x) {
    return icharacter<package_t<CharT>, CharT>::to_u16string(x.view());
}

/**
 * @brief 将右值字符串对象转换为UTF-16字符串
 * @tparam CharT 字符类型
 * @param x 右值字符串对象
 * @return 转换后的UTF-16字符串
 */
template <typename CharT, enable_if_t<is_character_v<CharT>, int> = 0>
NEFORCE_CONSTEXPR20 u16string to_u16string(basic_string<CharT>&& x) {
    return icharacter<package_t<CharT>, CharT>::to_u16string(x.view());
}

/**
 * @brief char16_t类型右值UTF-16字符串特化 - 直接移动
 * @param x 右值UTF-16字符串
 * @return 移动后的UTF-16字符串
 */
template <>
NEFORCE_CONSTEXPR20 u16string to_u16string<char16_t>(u16string&& x) {
    return _NEFORCE move(x);
}

/**
 * @brief 将字符转换为UTF-32字符串
 * @tparam CharT 字符类型
 * @param x 单个字符
 * @return 转换后的UTF-32字符串
 */
template <typename CharT, enable_if_t<is_character_v<CharT>, int> = 0>
NEFORCE_CONSTEXPR20 u32string to_u32string(const CharT& x) {
    return icharacter<package_t<CharT>, CharT>::to_u32string(basic_string<CharT>(1, x).view());
}

/**
 * @brief 将C风格字符串转换为UTF-32字符串
 * @tparam CharT 字符类型
 * @param x C风格字符串
 * @return 转换后的UTF-32字符串
 */
template <typename CharT, enable_if_t<is_character_v<CharT>, int> = 0>
NEFORCE_CONSTEXPR20 u32string to_u32string(const CharT* x) {
    return icharacter<package_t<CharT>, CharT>::to_u32string(basic_string_view<CharT>(x));
}

/**
 * @brief 将字符视图转换为UTF-32字符串
 * @tparam CharT 字符类型
 * @param x 字符视图
 * @return 转换后的UTF-32字符串
 */
template <typename CharT, enable_if_t<is_character_v<CharT>, int> = 0>
NEFORCE_CONSTEXPR20 u32string to_u32string(const basic_string_view<CharT> x) {
    return icharacter<package_t<CharT>, CharT>::to_u32string(x);
}

/**
 * @brief 将字符串对象转换为UTF-32字符串
 * @tparam CharT 字符类型
 * @param x 字符串对象
 * @return 转换后的UTF-32字符串
 */
template <typename CharT, enable_if_t<is_character_v<CharT>, int> = 0>
NEFORCE_CONSTEXPR20 u32string to_u32string(const basic_string<CharT>& x) {
    return icharacter<package_t<CharT>, CharT>::to_u32string(x.view());
}

/**
 * @brief 将右值字符串对象转换为UTF-32字符串
 * @tparam CharT 字符类型
 * @param x 右值字符串对象
 * @return 转换后的UTF-32字符串
 */
template <typename CharT, enable_if_t<is_character_v<CharT>, int> = 0>
NEFORCE_CONSTEXPR20 u32string to_u32string(basic_string<CharT>&& x) {
    return icharacter<package_t<CharT>, CharT>::to_u32string(x.view());
}

/**
 * @brief char32_t类型右值UTF-32字符串特化 - 直接移动
 * @param x 右值UTF-32字符串
 * @return 移动后的UTF-32字符串
 */
template <>
NEFORCE_CONSTEXPR20 u32string to_u32string<char32_t>(u32string&& x) {
    return _NEFORCE move(x);
}

/** @} */ // ToString

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_INTERFACE_ICHARACTER_HPP__
