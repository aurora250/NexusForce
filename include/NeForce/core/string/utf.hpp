#ifndef NEFORCE_CORE_STRING_CHARACTER_HPP__
#define NEFORCE_CORE_STRING_CHARACTER_HPP__

/**
 * @file utf.hpp
 * @brief UTF字符包装类
 *
 * 此文件提供了UTF字符的包装类。
 * 每个包装类都支持字符到各种UTF字符串类型的转换。
 */

#include "NeForce/core/interface/icharacter.hpp"
NEFORCE_BEGIN_NAMESPACE__

/// @cond
NEFORCE_BEGIN_INNER__

/**
 * @brief 辅助函数：在UTF-8字符串中追加无效字符的替换符
 * @tparam T 字符串类型
 * @param result 目标字符串
 */
template <typename T>
NEFORCE_ALWAYS_INLINE constexpr void __append_utf8_char_aux(T& result) {}

/**
 * @brief string类型的特化：追加UTF-8替换符
 * @param result 目标普通字符串
 */
template <>
NEFORCE_CONSTEXPR20 void __append_utf8_char_aux<string>(string& result) {
    result.append("\xEF\xBF\xBD", 3);
}

#ifdef NEFORCE_STANDARD_20
/**
 * @brief u8string类型的特化：追加UTF-8替换符
 * @param result 目标UTF-8字符串
 */
template <>
NEFORCE_CONSTEXPR20 void __append_utf8_char_aux<u8string>(u8string& result) {
    result.append(u8"\xEF\xBF\xBD", 3);
}
#endif

/**
 * @brief 将Unicode码点追加到UTF-8字符串
 * @tparam T 字符串字符类型
 * @param result 目标字符串
 * @param cp Unicode码点
 *
 * 将给定的Unicode码点编码为UTF-8序列并追加到字符串中。
 * 如果码点无效，则追加替换符U+FFFD。
 */
template <typename T>
NEFORCE_CONSTEXPR20 void append_utf8_char(basic_string<T>& result, uint32_t cp) {
    if (cp > 0x10FFFF || _NEFORCE is_high_surrogate(cp) || _NEFORCE is_low_surrogate(cp)) {
        inner::__append_utf8_char_aux(result);
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

/**
 * @brief 解码UTF-8字符
 * @param data UTF-8字节数据
 * @param i 当前解析位置
 * @param len 数据长度
 * @param cp 输出的Unicode码点
 * @return 解码是否成功
 *
 * 从UTF-8字节序列中解码一个字符，返回对应的Unicode码点。
 * 如果解码失败，cp设置为0xFFFD并返回false。
 */
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

/**
 * @brief 从UTF-16序列获取Unicode码点
 * @tparam T 字符类型
 * @param obj UTF-16字符数组
 * @param index 当前位置
 * @param len 数组长度
 * @param cp 输出的Unicode码点
 * @param consumed 消耗的字符数
 * @return 是否成功获取
 *
 * 处理UTF-16的代理对，返回对应的Unicode码点。
 */
template <typename T>
constexpr bool get_utf16_codepoint(const T* obj, size_t index, const size_t len, uint32_t& cp, size_t& consumed) {
    const auto c1 = static_cast<uint32_t>(obj[index]);
    consumed = 1;

    if (_NEFORCE is_high_surrogate(c1)) {
        if (index + 1 < len) {
            const auto c2 = static_cast<uint32_t>(obj[index + 1]);
            if (_NEFORCE is_low_surrogate(c2)) {
                cp = _NEFORCE combine_surrogates(c1, c2);
                consumed = 2;
                return true;
            }
        }
        cp = 0xFFFD;
        return true;
    } else if (_NEFORCE is_low_surrogate(c1)) {
        cp = 0xFFFD;
        return true;
    }

    cp = c1;
    return true;
}

/**
 * @brief 处理UTF-16代理对并更新位置
 * @tparam T 字符类型
 * @param obj UTF-16字符数组
 * @param index 当前位置
 * @param len 数组长度
 * @param cp 输出的Unicode码点
 * @return 是否成功处理
 */
template <typename T>
constexpr bool handle_utf16_surrogate_pair(
    const T* obj, size_t& index, const size_t len, uint32_t& cp) noexcept {
    const auto c1 = static_cast<uint32_t>(obj[index]);
    if (_NEFORCE is_high_surrogate(c1)) {
        if (index + 1 < len) {
            const auto c2 = static_cast<uint32_t>(obj[index + 1]);
            if (_NEFORCE is_low_surrogate(c2)) {
                cp = _NEFORCE combine_surrogates(c1, c2);
                index += 2;
                return true;
            }
        }
        cp = 0xFFFD;
        index += 1;
        return true;
    } else if (_NEFORCE is_low_surrogate(c1)) {
        cp = 0xFFFD;
        index += 1;
        return true;
    }
    cp = c1;
    index += 1;
    return true;
}

/**
 * @brief 检查Unicode码点是否有效
 * @param cp Unicode码点
 * @return 是否有效
 *
 * 有效范围：0x0-0x10FFFF，且不能是代理项。
 */
constexpr bool is_valid_unicode_codepoint(const uint32_t cp) noexcept {
    return cp <= 0x10FFFF && !_NEFORCE is_high_surrogate(cp) && !_NEFORCE is_low_surrogate(cp);
}

/**
 * @brief 将Unicode码点转换为UTF-16序列
 * @tparam T 字符串字符类型
 * @param result 目标UTF-16字符串
 * @param cp Unicode码点
 *
 * 将Unicode码点编码为UTF-16序列并追加到字符串中。
 */
template <typename T>
NEFORCE_CONSTEXPR20 void codepoint_to_utf16(basic_string<T>& result, uint32_t cp) {
    if (!inner::is_valid_unicode_codepoint(cp)) {
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

/**
 * @brief 将Unicode码点转换为宽字符序列
 * @tparam T 字符串字符类型
 * @param result 目标宽字符串
 * @param cp Unicode码点
 *
 * 根据平台特性进行转换：
 * - Windows: 使用UTF-16
 * - Linux: 使用UTF-32
 */
template <typename T>
NEFORCE_CONSTEXPR20 void codepoint_to_wchar(basic_string<T>& result, uint32_t cp) {
    if (!inner::is_valid_unicode_codepoint(cp)) {
        result.push_back(0xFFFD);
        return;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    if (cp <= 0xFFFF) {
        result.push_back(static_cast<wchar_t>(cp));
    } else {
        const uint32_t adjusted = cp - 0x10000;
        const wchar_t high_surrogate = static_cast<wchar_t>((adjusted >> 10) + 0xD800);
        const wchar_t low_surrogate = static_cast<wchar_t>((adjusted & 0x3FF) + 0xDC00);
        result.push_back(high_surrogate);
        result.push_back(low_surrogate);
    }
#elif defined(NEFORCE_PLATFORM_LINUX)
    result.push_back(static_cast<wchar_t>(cp));
#endif
}

/**
 * @brief 追加ASCII字符序列
 * @tparam T 目标字符串字符类型
 * @tparam U 源字符类型
 * @param result 目标字符串
 * @param str 源字符串
 * @param len 长度
 *
 * 直接将ASCII字符复制到目标字符串。
 */
template <typename T, typename U>
NEFORCE_CONSTEXPR20 void append_ascii_chars(basic_string<T>& result, const U* str, size_t len) {
    result.reserve(result.size() + len);
    for (size_t i = 0; i < len; ++i) {
        result.push_back(static_cast<T>(static_cast<byte_t>(str[i])));
    }
}

NEFORCE_END_INNER__
/// @endcond

#define NEFORCE_BUILD_PACKAGE_CONSTRUCTOR(T) \
constexpr T() noexcept = default; \
constexpr T(const T&) noexcept = default; \
constexpr T(T&&) noexcept = default; \
constexpr T(const value_type value) noexcept : base(value) {} \
NEFORCE_CONSTEXPR20 ~T() = default; \
constexpr T& operator =(const T& other) noexcept { \
    value_ = other.value_; \
    return *this; \
} \
constexpr T& operator =(T&& other) noexcept { \
    value_ = other.value_; \
    other.value_ = initialize<package_type>(); \
    return *this; \
} \
constexpr T& operator =(const value_type value) noexcept { \
    value_ = value; \
    return *this; \
}


/**
 * @defgroup Packages 数值包装
 * @brief 数值类型的包装类集合
 * @{
 */

/**
 * @struct character
 * @brief char类型包装类
 *
 * 提供char字符的包装，支持到各种字符串类型的转换。
 * char字符串被视为UTF-8编码。
 */
struct character : icharacter<character, char> {
    using value_type = char;                    ///< 值类型
    using base = icharacter<character, char>;   ///< 基类类型

    NEFORCE_BUILD_PACKAGE_CONSTRUCTOR(character)

    /**
     * @brief 转换为普通字符串
     * @param obj 字符视图
     * @return 普通字符串
     */
    static NEFORCE_CONSTEXPR20 string to_string(const basic_string_view<value_type>& obj) {
        return string{obj};
    }

    /**
     * @brief 转换为宽字符串
     * @param obj 字符视图
     * @return 宽字符串（UTF-8转wchar_t）
     */
    static NEFORCE_CONSTEXPR20 wstring to_wstring(const basic_string_view<value_type>& obj) {
        if (obj.empty()) return {};
        wstring result;

        const auto* data = reinterpret_cast<const byte_t*>(obj.data());
        size_t i = 0;
        const size_t len = obj.size();
        result.reserve(len);

        while (i < len) {
            uint32_t cp;
            if (inner::decode_utf8_char(data, i, len, cp)) {
                inner::codepoint_to_wchar(result, cp);
            } else {
                inner::codepoint_to_wchar(result, 0xFFFD);
            }
        }
        return result;
    }

#if defined(NEFORCE_STANDARD_20) || defined(NEXUSFORCE_ENABLE_DOXYGEN)
    /**
     * @brief 转换为UTF-8字符串
     * @param obj 字符视图
     * @return UTF-8字符串
     */
    static NEFORCE_CONSTEXPR20 u8string to_u8string(const basic_string_view<value_type>& obj) {
        if (obj.empty()) return {};
        u8string result;
        result.reserve(obj.size());
        for (const char c : obj) {
            result.push_back(static_cast<char8_t>(static_cast<byte_t>(c)));
        }
        return result;
    }
#endif

    /**
     * @brief 转换为UTF-16字符串
     * @param obj 字符视图
     * @return UTF-16字符串（UTF-8转UTF-16）
     */
    static NEFORCE_CONSTEXPR20 u16string to_u16string(const basic_string_view<value_type>& obj) {
        if (obj.empty()) return {};
        u16string result;

        const auto* data = reinterpret_cast<const byte_t*>(obj.data());
        size_t i = 0;
        const size_t len = obj.size();
        result.reserve(len * 2);

        while (i < len) {
            uint32_t cp;
            if (inner::decode_utf8_char(data, i, len, cp)) {
                inner::codepoint_to_utf16(result, cp);
            } else {
                result.push_back(0xFFFD);
            }
        }
        return result;
    }

    /**
     * @brief 转换为UTF-32字符串
     * @param obj 字符视图
     * @return UTF-32字符串（UTF-8转UTF-32）
     */
    static NEFORCE_CONSTEXPR20 u32string to_u32string(const basic_string_view<value_type>& obj) {
        if (obj.empty()) return {};
        u32string result;

        const auto* data = reinterpret_cast<const byte_t*>(obj.data());
        size_t i = 0;
        const size_t len = obj.size();
        result.reserve(len);

        while (i < len) {
            uint32_t cp;
            if (inner::decode_utf8_char(data, i, len, cp)) {
                result.push_back(static_cast<char32_t>(cp));
            } else {
                result.push_back(0xFFFD);
            }
        }
        return result;
    }
};

template <>
struct package<char> {
    using type = character;
};
template <>
struct unpackage<character> {
    using type = char;
};

/**
 * @struct wcharacter
 * @brief wchar_t类型包装类
 *
 * 提供wchar_t字符的包装，支持到各种字符串类型的转换。
 * 根据平台特性处理编码转换。
 */
struct wcharacter : icharacter<wcharacter, wchar_t> {
    using value_type = wchar_t;                       ///< 值类型
    using base = icharacter<wcharacter, wchar_t>;     ///< 基类类型

    NEFORCE_BUILD_PACKAGE_CONSTRUCTOR(wcharacter)

    /**
     * @brief 转换为普通字符串
     * @param obj 字符视图
     * @return 普通字符串（wchar_t转UTF-8）
     */
    static NEFORCE_CONSTEXPR20 string to_string(const basic_string_view<value_type>& obj) {
        if (obj.empty()) return {};
        string result;

#ifdef NEFORCE_PLATFORM_WINDOWS
        for (size_t i = 0; i < obj.size(); ) {
            uint32_t cp;
            size_t chars_consumed;
            if (inner::get_utf16_codepoint(obj.data(), i, obj.size(), cp, chars_consumed)) {
                inner::append_utf8_char(result, cp);
                i += chars_consumed;
            } else {
                inner::append_utf8_char(result, 0xFFFD);
                i++;
            }
        }
#elif defined(NEFORCE_PLATFORM_LINUX)
        for (const value_type i : obj) {
            inner::append_utf8_char(result, i);
        }
#endif
        return result;
    }

    /**
     * @brief 转换为宽字符串
     * @param obj 字符视图
     * @return 宽字符串
     */
    static NEFORCE_CONSTEXPR20 wstring to_wstring(const basic_string_view<value_type>& obj) {
        return wstring{obj};
    }

#if defined(NEFORCE_STANDARD_20) || defined(NEXUSFORCE_ENABLE_DOXYGEN)
    /**
     * @brief 转换为UTF-8字符串
     * @param obj 字符视图
     * @return UTF-8字符串（wchar_t转UTF-8）
     */
    static NEFORCE_CONSTEXPR20 u8string to_u8string(const basic_string_view<value_type>& obj) {
        if (obj.empty()) return {};
        u8string result;

#ifdef NEFORCE_PLATFORM_WINDOWS
        for (size_t i = 0; i < obj.size(); ) {
            uint32_t cp;
            size_t chars_consumed;
            if (inner::get_utf16_codepoint(obj.data(), i, obj.size(), cp, chars_consumed)) {
                inner::append_utf8_char(result, cp);
                i += chars_consumed;
            } else {
                inner::append_utf8_char(result, 0xFFFD);
                i++;
            }
        }
#elif defined(NEFORCE_PLATFORM_LINUX)
        for (const value_type i : obj) {
            inner::append_utf8_char(result, i);
        }
#endif
        return result;
    }
#endif

    /**
     * @brief 转换为UTF-16字符串
     * @param obj 字符视图
     * @return UTF-16字符串（wchar_t转UTF-16）
     */
    static NEFORCE_CONSTEXPR20 u16string to_u16string(const basic_string_view<value_type>& obj) {
        if (obj.empty()) return {};
        u16string result;

#ifdef NEFORCE_PLATFORM_WINDOWS
        result.reserve(obj.size());
        for (size_t i = 0; i < obj.size(); ++i) {
            result.push_back(static_cast<char16_t>(static_cast<uint16_t>(obj[i])));
        }
#elif defined(NEFORCE_PLATFORM_LINUX)
        result.reserve(obj.size() * 2);
        for (const value_type i : obj) {
            inner::codepoint_to_utf16(result, i);
        }
#endif
        return result;
    }

    /**
     * @brief 转换为UTF-32字符串
     * @param obj 字符视图
     * @return UTF-32字符串（wchar_t转UTF-32）
     */
    static NEFORCE_CONSTEXPR20 u32string to_u32string(const basic_string_view<value_type>& obj) {
        if (obj.empty()) return {};
        u32string result;
        result.reserve(obj.size());

#ifdef NEFORCE_PLATFORM_WINDOWS
        for (size_t i = 0; i < obj.size(); ) {
            uint32_t cp;
            size_t chars_consumed;
            if (inner::get_utf16_codepoint(obj.data(), i, obj.size(), cp, chars_consumed)) {
                result.push_back(static_cast<char32_t>(cp));
                i += chars_consumed;
            } else {
                result.push_back(0xFFFD);
                i++;
            }
        }
#elif defined(NEFORCE_PLATFORM_LINUX)
        for (const value_type i : obj) {
            result.push_back(static_cast<char32_t>(i));
        }
#endif
        return result;
    }
};

template <>
struct package<wchar_t> {
    using type = wcharacter;
};
template <>
struct unpackage<wcharacter> {
    using type = wchar_t;
};


#if defined(NEFORCE_STANDARD_20) || defined(NEXUSFORCE_ENABLE_DOXYGEN)

/**
 * @struct u8character
 * @brief char8_t类型包装类
 *
 * 提供char8_t字符的包装，支持到各种字符串类型的转换。
 * char8_t字符串为UTF-8编码。
 */
struct u8character : icharacter<u8character, char8_t> {
    using value_type = char8_t;                       ///< 值类型
    using base = icharacter<u8character, char8_t>;    ///< 基类类型

    NEFORCE_BUILD_PACKAGE_CONSTRUCTOR(u8character)

    /**
     * @brief 转换为普通字符串
     * @param obj 字符视图
     * @return 普通字符串
     */
    static NEFORCE_CONSTEXPR20 string to_string(const basic_string_view<value_type>& obj) {
        if (obj.empty()) return {};
        string result;
        inner::append_ascii_chars(result, obj.data(), obj.size());
        return result;
    }

    /**
     * @brief 转换为宽字符串
     * @param obj 字符视图
     * @return 宽字符串（UTF-8转wchar_t）
     */
    static NEFORCE_CONSTEXPR20 wstring to_wstring(const basic_string_view<value_type>& obj) {
        if (obj.empty()) return {};
        wstring result;
        const size_t len = obj.size();
        result.reserve(len);

        size_t i = 0;
        while (i < len) {
            const auto data = reinterpret_cast<const byte_t*>(obj.data());
            uint32_t cp;
            if (inner::decode_utf8_char(data, i, len, cp)) {
                inner::codepoint_to_wchar(result, cp);
            } else {
                result.push_back(0xFFFD);
            }
        }
        return result;
    }

    /**
     * @brief 转换为UTF-8字符串
     * @param obj 字符视图
     * @return UTF-8字符串
     */
    static NEFORCE_CONSTEXPR20 u8string to_u8string(const basic_string_view<value_type>& obj) {
        return u8string{obj};
    }

    /**
     * @brief 转换为UTF-16字符串
     * @param obj 字符视图
     * @return UTF-16字符串（UTF-8转UTF-16）
     */
    static NEFORCE_CONSTEXPR20 u16string to_u16string(const basic_string_view<value_type>& obj) {
        if (obj.empty()) return {};
        u16string result;
        const size_t len = obj.size();
        result.reserve(len);

        size_t i = 0;
        while (i < len) {
            const auto data = reinterpret_cast<const byte_t*>(obj.data());
            uint32_t cp;
            if (inner::decode_utf8_char(data, i, len, cp)) {
                inner::codepoint_to_utf16(result, cp);
            } else {
                result.push_back(0xFFFD);
            }
        }
        return result;
    }

    /**
     * @brief 转换为UTF-32字符串
     * @param obj 字符视图
     * @return UTF-32字符串（UTF-8转UTF-32）
     */
    static NEFORCE_CONSTEXPR20 u32string to_u32string(const basic_string_view<value_type>& obj) {
        if (obj.empty()) return {};
        u32string result;
        const size_t len = obj.size();
        result.reserve(len);

        size_t i = 0;
        while (i < len) {
            const auto data = reinterpret_cast<const byte_t*>(obj.data());
            uint32_t cp;
            if (inner::decode_utf8_char(data, i, len, cp)) {
                result.push_back(static_cast<char32_t>(cp));
            } else {
                result.push_back(0xFFFD);
            }
        }
        return result;
    }
};

template <>
struct package<char8_t> {
    using type = u8character;
};
template <>
struct unpackage<u8character> {
    using type = char8_t;
};

#endif

/**
 * @struct u16character
 * @brief char16_t类型包装类
 *
 * 提供char16_t字符的包装，支持到各种字符串类型的转换。
 * char16_t字符串为UTF-16编码，自动处理字节序标记（BOM）。
 */
struct u16character : icharacter<u16character, char16_t> {
    using value_type = char16_t;                      ///< 值类型
    using base = icharacter<u16character, char16_t>;  ///< 基类类型

    NEFORCE_BUILD_PACKAGE_CONSTRUCTOR(u16character)

    /**
     * @brief 转换为普通字符串
     * @param obj 字符视图
     * @return 普通字符串（UTF-16转UTF-8）
     */
    static NEFORCE_CONSTEXPR20 string to_string(const basic_string_view<value_type>& obj) {
        if (obj.empty()) return {};
        string result;

        size_t start_pos = 0;
        if (!obj.empty() && obj[0] == 0xFEFF) {
            start_pos = 1;
        }

        for (size_t i = start_pos; i < obj.size(); ) {
            uint32_t cp;
            size_t chars_consumed;

            if (inner::get_utf16_codepoint(obj.data(), i, obj.size(), cp, chars_consumed)) {
                if (cp <= 0x10FFFF && !_NEFORCE is_high_surrogate(cp) && !_NEFORCE is_low_surrogate(cp)) {
                    inner::append_utf8_char(result, cp);
                } else {
                    inner::append_utf8_char(result, 0xFFFD);
                }
                i += chars_consumed;
            } else {
                inner::append_utf8_char(result, 0xFFFD);
                i++;
            }
        }
        return result;
    }

    /**
     * @brief 转换为宽字符串
     * @param obj 字符视图
     * @return 宽字符串（UTF-16转wchar_t）
     */
    static NEFORCE_CONSTEXPR20 wstring to_wstring(const basic_string_view<value_type>& obj) {
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
            if (inner::get_utf16_codepoint(obj.data(), i, obj.size(), cp, chars_consumed)) {
                inner::codepoint_to_wchar(result, cp);
                i += chars_consumed;
            } else {
                result.push_back(0xFFFD);
                i++;
            }
        }
        return result;
    }

#if defined(NEFORCE_STANDARD_20) || defined(NEXUSFORCE_ENABLE_DOXYGEN)
    /**
     * @brief 转换为UTF-8字符串
     * @param obj 字符视图
     * @return UTF-8字符串（UTF-16转UTF-8）
     */
    static NEFORCE_CONSTEXPR20 u8string to_u8string(const basic_string_view<value_type>& obj) {
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
            if (inner::get_utf16_codepoint(obj.data(), i, obj.size(), cp, chars_consumed)) {
                inner::append_utf8_char(result, cp);
                i += chars_consumed;
            } else {
                inner::append_utf8_char(result, 0xFFFD);
                i++;
            }
        }
        return result;
    }
#endif

    /**
     * @brief 转换为UTF-16字符串
     * @param obj 字符视图
     * @return UTF-16字符串
     */
    static NEFORCE_CONSTEXPR20 u16string to_u16string(const basic_string_view<value_type>& obj) {
        return u16string{obj};
    }

    /**
     * @brief 转换为UTF-32字符串
     * @param obj 字符视图
     * @return UTF-32字符串（UTF-16转UTF-32）
     */
    static NEFORCE_CONSTEXPR20 u32string to_u32string(const basic_string_view<value_type>& obj) {
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
            if (inner::get_utf16_codepoint(obj.data(), i, obj.size(), cp, chars_consumed)) {
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
struct package<char16_t> {
    using type = u16character;
};
template <>
struct unpackage<u16character> {
    using type = char16_t;
};

/**
 * @struct u32character
 * @brief char32_t类型包装类
 *
 * 提供char32_t字符的包装，支持到各种字符串类型的转换。
 * char32_t字符串为UTF-32编码。
 */
struct u32character : icharacter<u32character, char32_t> {
    using value_type = char32_t;                      ///< 值类型
    using base = icharacter<u32character, char32_t>;  ///< 基类类型

    NEFORCE_BUILD_PACKAGE_CONSTRUCTOR(u32character)

    /**
     * @brief 转换为普通字符串
     * @param obj 字符视图
     * @return 普通字符串（UTF-32转UTF-8）
     */
    static NEFORCE_CONSTEXPR20 string to_string(const basic_string_view<value_type>& obj) {
        if (obj.empty()) return {};
        string result;
        for (const value_type i : obj) {
            inner::append_utf8_char(result, i);
        }
        return result;
    }

    /**
     * @brief 转换为宽字符串
     * @param obj 字符视图
     * @return 宽字符串（UTF-32转wchar_t）
     */
    static NEFORCE_CONSTEXPR20 wstring to_wstring(const basic_string_view<value_type>& obj) {
        if (obj.empty()) return {};
        wstring result;
        result.reserve(obj.size());
        for (const value_type i : obj) {
            inner::codepoint_to_wchar(result, i);
        }
        return result;
    }

#if defined(NEFORCE_STANDARD_20) || defined(NEXUSFORCE_ENABLE_DOXYGEN)
    /**
     * @brief 转换为UTF-8字符串
     * @param obj 字符视图
     * @return UTF-8字符串（UTF-32转UTF-8）
     */
    static NEFORCE_CONSTEXPR20 u8string to_u8string(const basic_string_view<value_type>& obj) {
        if (obj.empty()) return {};
        u8string result;
        result.reserve(obj.size() * 4);
        for (const value_type i : obj) {
            inner::append_utf8_char(result, i);
        }
        return result;
    }
#endif

    /**
     * @brief 转换为UTF-16字符串
     * @param obj 字符视图
     * @return UTF-16字符串（UTF-32转UTF-16）
     */
    static NEFORCE_CONSTEXPR20 u16string to_u16string(const basic_string_view<value_type>& obj) {
        if (obj.empty()) return {};
        u16string result;
        result.reserve(obj.size() * 2);
        for (const value_type i : obj) {
            inner::codepoint_to_utf16(result, i);
        }
        return result;
    }

    /**
     * @brief 转换为UTF-32字符串
     * @param obj 字符视图
     * @return UTF-32字符串
     */
    static NEFORCE_CONSTEXPR20 u32string to_u32string(const basic_string_view<value_type>& obj) {
        return u32string{obj};
    }
};

template <>
struct package<char32_t> {
    using type = u32character;
};
template <>
struct unpackage<u32character> {
    using type = char32_t;
};

/** @} */ // Packages

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_STRING_CHARACTER_HPP__
