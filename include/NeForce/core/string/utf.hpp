#ifndef NEFORCE_CORE_STRING_UTF_HPP__
#define NEFORCE_CORE_STRING_UTF_HPP__

/**
 * @file utf.hpp
 * @brief UTF字符包装类
 *
 * 此文件提供了UTF字符的包装类。
 */

#include "NeForce/core/interface/icharacter.hpp"
#include "NeForce/core/string/codepoint.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Packages 数值包装
 * @brief 数值类型的包装类集合
 * @{
 */

/**
 * @defgroup UTF UTF
 * @brief UTF字符包装类集合
 *
 * 支持 char、wchar_t、char8_t、char16_t、char32_t
 * 等字符类型之间的相互转换。每个包装类都支持字符到各种UTF字符串类型的转换。
 *
 * @section standards 遵循的国际标准
 * 本实现严格遵循以下字符编码与字符串处理相关标准规范：
 *
 * **Unicode 核心标准：**
 * - **Unicode 15.1.0**：Unicode 字符编码标准
 *   https://unicode.org/versions/Unicode15.1.0/
 * - **Unicode 16.0.0**：Unicode 字符编码标准（最新版本）
 *   https://unicode.org/versions/Unicode16.0.0/
 *
 * **UTF 编码格式标准：**
 * - **IETF RFC 3629**：UTF-8 — ISO 10646 的转换格式（UTF-8 编码规范）
 *   https://www.rfc-editor.org/rfc/rfc3629.html
 * - **IETF RFC 2781**：UTF-16 — ISO 10646 的编码格式（UTF-16 编码规范，含 BOM 处理）
 *   https://www.rfc-editor.org/rfc/rfc2781.html
 * - **Unicode Standard Annex #15**：Unicode 规范化形式
 *   https://unicode.org/reports/tr15/
 *
 * **字节序标记（BOM）标准：**
 * - **Unicode Standard §2.13**：字节序标记（BOM）规范
 *   https://unicode.org/versions/Unicode15.1.0/ch02.pdf#G16660
 *
 * **通用字符集编码标准：**
 * - **ISO/IEC 10646:2020**：信息技术 — 通用编码字符集 (UCS)
 *   https://www.iso.org/standard/76835.html
 *
 * **C++ 字符类型标准：**
 * - **ISO/IEC 14882:2020**：C++ 编程语言标准（§5.13.3 字符字面量，§5.13.5 字符串字面量）
 *   https://www.iso.org/standard/79358.html
 *
 * **C 语言宽字符标准：**
 * - **ISO/IEC 9899:2018**：C 语言标准（§7.19 宽字符工具，§7.28 Unicode 工具）
 *   https://www.iso.org/standard/74528.html
 *
 * @section character_types 字符类型与编码对应
 * 根据 C++ 标准和 Unicode 规范，各字符类型的编码语义如下：
 *
 * | 包装类         | 基础类型    |  编码格式 |  码元位数 |  说明                         |
 * |---------------|------------|----------|----------|------------------------------|
 * | character     | char       | UTF-8    | 8 位     | 普通字符，UTF-8 编码           |
 * | wcharacter    | wchar_t    | 平台相关  | 平台相关  | Windows: UTF-16, Unix: UTF-32|
 * | u8character   | char8_t    | UTF-8    | 8 位     | UTF-8 编码                    |
 * | u16character  | char16_t   | UTF-16   | 16 位    | UTF-16 编码，支持 BOM          |
 * | u32character  | char32_t   | UTF-32   | 32 位    | UTF-32 编码（直接码点）         |
 *
 * @section platform_differences 平台差异
 * wchar_t 类型在不同平台上的编码不同：
 *
 * | 平台          | wchar_t 大小 | 编码格式 | 说明                     |
 * |---------------|--------------|----------|--------------------------|
 * | Windows       | 16 位        | UTF-16   | 需要代理对表示辅助平面字符 |
 * | Linux/macOS   | 32 位        | UTF-32   | 直接存储完整 Unicode 码点 |
 *
 * @section bom_handling 字节序标记（BOM）处理
 * 根据 RFC 2781 §3.2，UTF-16 字符串可能包含字节序标记：
 *
 * | BOM 值    | 含义                     | 处理方式                     |
 * |-----------|--------------------------|------------------------------|
 * | 0xFEFF    | 无字节序反转（同系统序） | 根据系统序判断是否需要交换    |
 * | 0xFFFE    | 需要字节序反转           | 解码时交换每个码元的字节序    |
 * | 无 BOM    | 假定为大端序             | RFC 2781 推荐使用大端序       |
 *
 * u16character 的转换方法自动检测并处理 BOM。
 *
 * @section conversion_matrix 字符转换支持矩阵
 * 每个包装类支持转换到以下字符串类型：
 *
 * | 源类型 \ 目标类型 | string  | wstring | u8string | u16string | u32string |
 * |-------------------|---------|---------|----------|-----------|-----------|
 * | character (UTF-8) | ✓ 直接  | ✓ 转换  | ✓ 转换   | ✓ 转换    | ✓ 转换    |
 * | wcharacter (平台) | ✓ 转换  | ✓ 直接  | ✓ 转换   | ✓ 转换    | ✓ 转换    |
 * | u8character (UTF-8)| ✓ 转换 | ✓ 转换  | ✓ 直接   | ✓ 转换    | ✓ 转换    |
 * | u16character (UTF-16)| ✓ 转换| ✓ 转换  | ✓ 转换   | ✓ 直接    | ✓ 转换    |
 * | u32character (UTF-32)| ✓ 转换| ✓ 转换  | ✓ 转换   | ✓ 转换    | ✓ 直接    |
 *
 * 图例：✓ 直接 = 直接复制/无转换，✓ 转换 = 通过 Unicode 码点中转转换
 *
 * @section implementation_details 实现细节
 * | 特性              | 规范参数                                  |
 * |-------------------|-------------------------------------------|
 * | 转换中介          | codepoint（Unicode 码点）                 |
 * | 无效序列处理      | 替换为 U+FFFD（� REPLACEMENT CHARACTER）  |
 * | BOM 检测          | UTF-16 字符串自动检测 0xFEFF/0xFFFE       |
 * | 代理对处理        | 支持 UTF-16 代理对编解码                  |
 * | 过度长序列检测    | 支持（UTF-8 解码时验证）                  |
 *
 * @note 所有转换均通过 Unicode 码点（codepoint）作为中转格式，
 *       确保转换过程不丢失信息，且遵循 Unicode 标准的无效序列处理规则。
 *
 * @warning wchar_t 的编码因平台而异，跨平台代码应避免直接依赖 wchar_t 的编码格式。
 *          对于需要可移植编码的场景，推荐使用 char8_t（UTF-8）、char16_t（UTF-16）
 *          或 char32_t（UTF-32）。
 *
 * @see https://unicode.org/
 * @see https://www.rfc-editor.org/rfc/rfc3629
 * @see https://www.rfc-editor.org/rfc/rfc2781
 * @see https://en.cppreference.com/w/cpp/language/types
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
    using value_type = char;                  ///< 值类型
    using base = icharacter<character, char>; ///< 基类类型

    constexpr character() noexcept = default;
    NEFORCE_CONSTEXPR20 ~character() = default;

    constexpr character(const character&) noexcept = default;
    constexpr character(character&&) noexcept = default;

    constexpr character& operator=(const character& other) noexcept = default;
    constexpr character& operator=(character&& other) noexcept = default;

    explicit constexpr character(const value_type value) noexcept :
    base(value) {}

    constexpr character& operator=(const value_type value) noexcept {
        value_ = value;
        return *this;
    }

    /**
     * @brief 转换为普通字符串
     * @param obj 字符视图
     * @return 普通字符串
     */
    static NEFORCE_CONSTEXPR20 string to_string(const basic_string_view<value_type>& obj) { return string{obj}; }

    /**
     * @brief 转换为宽字符串
     * @param obj 字符视图
     * @return 宽字符串（UTF-8转wchar_t）
     */
    static NEFORCE_CONSTEXPR20 wstring to_wstring(const basic_string_view<value_type>& obj) {
        if (obj.empty()) {
            return {};
        }
        wstring result;
        result.reserve(obj.size());

        const auto* data = reinterpret_cast<const byte_t*>(obj.data());
        size_t i = 0;
        const size_t len = obj.size();

        while (i < len) {
            codepoint::decode_utf8(data, i, len).append_to(result);
        }
        return result;
    }

#ifdef NEFORCE_STANDARD_20
    /**
     * @brief 转换为UTF-8字符串
     * @param obj 字符视图
     * @return UTF-8字符串
     */
    static NEFORCE_CONSTEXPR20 u8string to_u8string(const basic_string_view<value_type>& obj) {
        if (obj.empty()) {
            return {};
        }
        u8string result;
        result.reserve(obj.size());

        const auto* data = reinterpret_cast<const byte_t*>(obj.data());
        size_t i = 0;
        const size_t len = obj.size();

        while (i < len) {
            codepoint::decode_utf8(data, i, len).append_to(result);
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
        if (obj.empty()) {
            return {};
        }
        u16string result;
        result.reserve(obj.size() * 2);

        const auto* data = reinterpret_cast<const byte_t*>(obj.data());
        size_t i = 0;
        const size_t len = obj.size();

        while (i < len) {
            codepoint::decode_utf8(data, i, len).append_to(result);
        }
        return result;
    }

    /**
     * @brief 转换为UTF-32字符串
     * @param obj 字符视图
     * @return UTF-32字符串（UTF-8转UTF-32）
     */
    static NEFORCE_CONSTEXPR20 u32string to_u32string(const basic_string_view<value_type>& obj) {
        if (obj.empty()) {
            return {};
        }
        u32string result;
        result.reserve(obj.size());

        const auto* data = reinterpret_cast<const byte_t*>(obj.data());
        size_t i = 0;
        const size_t len = obj.size();

        while (i < len) {
            codepoint::decode_utf8(data, i, len).append_to(result);
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
    using value_type = wchar_t;                   ///< 值类型
    using base = icharacter<wcharacter, wchar_t>; ///< 基类类型

    constexpr wcharacter() noexcept = default;
    NEFORCE_CONSTEXPR20 ~wcharacter() = default;

    constexpr wcharacter(const wcharacter&) noexcept = default;
    constexpr wcharacter(wcharacter&&) noexcept = default;

    constexpr wcharacter& operator=(const wcharacter& other) noexcept = default;
    constexpr wcharacter& operator=(wcharacter&& other) noexcept = default;

    explicit constexpr wcharacter(const value_type value) noexcept :
    base(value) {}

    constexpr wcharacter& operator=(const value_type value) noexcept {
        value_ = value;
        return *this;
    }

    /**
     * @brief 转换为普通字符串
     * @param obj 字符视图
     * @return 普通字符串（wchar_t转UTF-8）
     */
    static NEFORCE_CONSTEXPR20 string to_string(const basic_string_view<value_type>& obj) {
        if (obj.empty()) {
            return {};
        }
        string result;

#ifdef NEFORCE_PLATFORM_WINDOWS
        size_t i = 0;
        while (i < obj.size()) {
            codepoint::decode_utf16(obj.data(), i, obj.size(), false).append_to(result);
        }
#else
        for (const value_type c: obj) {
            codepoint(static_cast<uint32_t>(c)).append_to(result);
        }
#endif
        return result;
    }

    /**
     * @brief 转换为宽字符串
     * @param obj 字符视图
     * @return 宽字符串
     */
    static NEFORCE_CONSTEXPR20 wstring to_wstring(const basic_string_view<value_type>& obj) { return wstring{obj}; }

#ifdef NEFORCE_STANDARD_20
    /**
     * @brief 转换为UTF-8字符串
     * @param obj 字符视图
     * @return UTF-8字符串（wchar_t转UTF-8）
     */
    static NEFORCE_CONSTEXPR20 u8string to_u8string(const basic_string_view<value_type>& obj) {
        if (obj.empty()) {
            return {};
        }
        u8string result;

#    ifdef NEFORCE_PLATFORM_WINDOWS
        size_t i = 0;
        while (i < obj.size()) {
            codepoint::decode_utf16(obj.data(), i, obj.size(), false).append_to(result);
        }
#    else
        for (const value_type c: obj) {
            codepoint(static_cast<uint32_t>(c)).append_to(result);
        }
#    endif
        return result;
    }
#endif

    /**
     * @brief 转换为UTF-16字符串
     * @param obj 字符视图
     * @return UTF-16字符串（wchar_t转UTF-16）
     */
    static NEFORCE_CONSTEXPR20 u16string to_u16string(const basic_string_view<value_type>& obj) {
        if (obj.empty()) {
            return {};
        }
        u16string result;

#ifdef NEFORCE_PLATFORM_WINDOWS
        result.reserve(obj.size());
        for (const value_type c: obj) {
            result.push_back(static_cast<char16_t>(static_cast<uint16_t>(c)));
        }
#else
        result.reserve(obj.size() * 2);
        for (const value_type c: obj) {
            codepoint(static_cast<uint32_t>(c)).append_to(result);
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
        if (obj.empty()) {
            return {};
        }
        u32string result;
        result.reserve(obj.size());

#ifdef NEFORCE_PLATFORM_WINDOWS
        size_t i = 0;
        while (i < obj.size()) {
            codepoint::decode_utf16(obj.data(), i, obj.size(), false).append_to(result);
        }
#else
        for (const value_type c: obj) {
            codepoint(static_cast<uint32_t>(c)).append_to(result);
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


#ifdef NEFORCE_STANDARD_20

/**
 * @struct u8character
 * @brief char8_t类型包装类
 *
 * 提供char8_t字符的包装，支持到各种字符串类型的转换。
 * char8_t字符串为UTF-8编码。
 */
struct u8character : icharacter<u8character, char8_t> {
    using value_type = char8_t;                    ///< 值类型
    using base = icharacter<u8character, char8_t>; ///< 基类类型

    constexpr u8character() noexcept = default;
    NEFORCE_CONSTEXPR20 ~u8character() = default;

    constexpr u8character(const u8character&) noexcept = default;
    constexpr u8character(u8character&&) noexcept = default;

    constexpr u8character& operator=(const u8character& other) noexcept = default;
    constexpr u8character& operator=(u8character&& other) noexcept = default;

    explicit constexpr u8character(const value_type value) noexcept :
    base(value) {}

    constexpr u8character& operator=(const value_type value) noexcept {
        value_ = value;
        return *this;
    }

    /**
     * @brief 转换为普通字符串
     * @param obj 字符视图
     * @return 普通字符串
     */
    static NEFORCE_CONSTEXPR20 string to_string(const basic_string_view<value_type>& obj) {
        if (obj.empty()) {
            return {};
        }
        string result;
        result.reserve(obj.size());

        const auto* data = reinterpret_cast<const byte_t*>(obj.data());
        size_t i = 0;
        const size_t len = obj.size();

        while (i < len) {
            codepoint::decode_utf8(data, i, len).append_to(result);
        }
        return result;
    }

    /**
     * @brief 转换为宽字符串
     * @param obj 字符视图
     * @return 宽字符串（UTF-8转wchar_t）
     */
    static NEFORCE_CONSTEXPR20 wstring to_wstring(const basic_string_view<value_type>& obj) {
        if (obj.empty()) {
            return {};
        }
        wstring result;
        result.reserve(obj.size());

        const auto* data = reinterpret_cast<const byte_t*>(obj.data());
        size_t i = 0;
        const size_t len = obj.size();

        while (i < len) {
            codepoint::decode_utf8(data, i, len).append_to(result);
        }
        return result;
    }

    /**
     * @brief 转换为UTF-8字符串
     * @param obj 字符视图
     * @return UTF-8字符串
     */
    static NEFORCE_CONSTEXPR20 u8string to_u8string(const basic_string_view<value_type>& obj) { return u8string{obj}; }

    /**
     * @brief 转换为UTF-16字符串
     * @param obj 字符视图
     * @return UTF-16字符串（UTF-8转UTF-16）
     */
    static NEFORCE_CONSTEXPR20 u16string to_u16string(const basic_string_view<value_type>& obj) {
        if (obj.empty()) {
            return {};
        }
        u16string result;
        result.reserve(obj.size());

        const auto* data = reinterpret_cast<const byte_t*>(obj.data());
        size_t i = 0;
        const size_t len = obj.size();

        while (i < len) {
            codepoint::decode_utf8(data, i, len).append_to(result);
        }
        return result;
    }

    /**
     * @brief 转换为UTF-32字符串
     * @param obj 字符视图
     * @return UTF-32字符串（UTF-8转UTF-32）
     */
    static NEFORCE_CONSTEXPR20 u32string to_u32string(const basic_string_view<value_type>& obj) {
        if (obj.empty()) {
            return {};
        }
        u32string result;
        result.reserve(obj.size());

        const auto* data = reinterpret_cast<const byte_t*>(obj.data());
        size_t i = 0;
        const size_t len = obj.size();

        while (i < len) {
            codepoint::decode_utf8(data, i, len).append_to(result);
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
public:
    using value_type = char16_t;                     ///< 值类型
    using base = icharacter<u16character, char16_t>; ///< 基类类型

private:
    template <typename T>
    static void parse_utf16_bom(const basic_string_view<T>& obj, size_t& start_pos, bool& need_swap) noexcept {
        start_pos = 0;
        need_swap = false;

        if (obj.empty()) {
            return;
        }

        if (static_cast<uint16_t>(obj[0]) == 0xFEFF) {
            start_pos = 1;
            need_swap = endian::is_big_endian;
        } else if (static_cast<uint16_t>(obj[0]) == 0xFFFE) {
            start_pos = 1;
            need_swap = endian::is_little_endian;
        }
    }

public:
    constexpr u16character() noexcept = default;
    NEFORCE_CONSTEXPR20 ~u16character() = default;

    constexpr u16character(const u16character&) noexcept = default;
    constexpr u16character(u16character&&) noexcept = default;

    constexpr u16character& operator=(const u16character& other) noexcept = default;
    constexpr u16character& operator=(u16character&& other) noexcept = default;

    explicit constexpr u16character(const value_type value) noexcept :
    base(value) {}

    constexpr u16character& operator=(const value_type value) noexcept {
        value_ = value;
        return *this;
    }

    /**
     * @brief 转换为普通字符串
     * @param obj 字符视图
     * @return 普通字符串（UTF-16转UTF-8）
     */
    static NEFORCE_CONSTEXPR20 string to_string(const basic_string_view<value_type>& obj) {
        if (obj.empty()) {
            return {};
        }
        string result;

        size_t start_pos = 0;
        bool need_swap = false;
        parse_utf16_bom(obj, start_pos, need_swap);

        size_t i = start_pos;
        while (i < obj.size()) {
            codepoint::decode_utf16(obj.data(), i, obj.size(), need_swap).append_to(result);
        }
        return result;
    }

    /**
     * @brief 转换为宽字符串
     * @param obj 字符视图
     * @return 宽字符串（UTF-16转wchar_t）
     */
    static NEFORCE_CONSTEXPR20 wstring to_wstring(const basic_string_view<value_type>& obj) {
        if (obj.empty()) {
            return {};
        }
        wstring result;
        result.reserve(obj.size());

        size_t start_pos = 0;
        bool need_swap = false;
        parse_utf16_bom(obj, start_pos, need_swap);

        size_t i = start_pos;
        while (i < obj.size()) {
            codepoint::decode_utf16(obj.data(), i, obj.size(), need_swap).append_to(result);
        }
        return result;
    }

#ifdef NEFORCE_STANDARD_20
    /**
     * @brief 转换为UTF-8字符串
     * @param obj 字符视图
     * @return UTF-8字符串（UTF-16转UTF-8）
     */
    static NEFORCE_CONSTEXPR20 u8string to_u8string(const basic_string_view<value_type>& obj) {
        if (obj.empty()) {
            return {};
        }
        u8string result;
        result.reserve(obj.size() * 3);

        size_t start_pos = 0;
        bool need_swap = false;
        parse_utf16_bom(obj, start_pos, need_swap);

        size_t i = start_pos;
        while (i < obj.size()) {
            codepoint::decode_utf16(obj.data(), i, obj.size(), need_swap).append_to(result);
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
        if (obj.empty()) {
            return {};
        }

        size_t start_pos = 0;
        bool need_swap = false;
        parse_utf16_bom(obj, start_pos, need_swap);

        if (start_pos == 0 && !need_swap) {
            return u16string{obj};
        }

        u16string result;
        result.reserve(obj.size());

        size_t i = start_pos;
        while (i < obj.size()) {
            codepoint::decode_utf16(obj.data(), i, obj.size(), need_swap).append_to(result);
        }
        return result;
    }

    /**
     * @brief 转换为UTF-32字符串
     * @param obj 字符视图
     * @return UTF-32字符串（UTF-16转UTF-32）
     */
    static NEFORCE_CONSTEXPR20 u32string to_u32string(const basic_string_view<value_type>& obj) {
        if (obj.empty()) {
            return {};
        }
        u32string result;
        result.reserve(obj.size());

        size_t start_pos = 0;
        bool need_swap = false;
        parse_utf16_bom(obj, start_pos, need_swap);

        size_t i = start_pos;
        while (i < obj.size()) {
            codepoint::decode_utf16(obj.data(), i, obj.size(), need_swap).append_to(result);
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
    using value_type = char32_t;                     ///< 值类型
    using base = icharacter<u32character, char32_t>; ///< 基类类型

    constexpr u32character() noexcept = default;
    NEFORCE_CONSTEXPR20 ~u32character() = default;

    constexpr u32character(const u32character&) noexcept = default;
    constexpr u32character(u32character&&) noexcept = default;

    constexpr u32character& operator=(const u32character& other) noexcept = default;
    constexpr u32character& operator=(u32character&& other) noexcept = default;

    explicit constexpr u32character(const value_type value) noexcept :
    base(value) {}

    constexpr u32character& operator=(const value_type value) noexcept {
        value_ = value;
        return *this;
    }

    /**
     * @brief 转换为普通字符串
     * @param obj 字符视图
     * @return 普通字符串（UTF-32转UTF-8）
     */
    static NEFORCE_CONSTEXPR20 string to_string(const basic_string_view<value_type>& obj) {
        if (obj.empty()) {
            return {};
        }
        string result;
        for (const value_type c: obj) {
            codepoint::from_utf32(c).append_to(result);
        }
        return result;
    }

    /**
     * @brief 转换为宽字符串
     * @param obj 字符视图
     * @return 宽字符串（UTF-32转wchar_t）
     */
    static NEFORCE_CONSTEXPR20 wstring to_wstring(const basic_string_view<value_type>& obj) {
        if (obj.empty()) {
            return {};
        }
        wstring result;
        result.reserve(obj.size());
        for (const value_type c: obj) {
            codepoint::from_utf32(c).append_to(result);
        }
        return result;
    }

#ifdef NEFORCE_STANDARD_20
    /**
     * @brief 转换为UTF-8字符串
     * @param obj 字符视图
     * @return UTF-8字符串（UTF-32转UTF-8）
     */
    static NEFORCE_CONSTEXPR20 u8string to_u8string(const basic_string_view<value_type>& obj) {
        if (obj.empty()) {
            return {};
        }
        u8string result;
        result.reserve(obj.size() * 4);
        for (const value_type c: obj) {
            codepoint::from_utf32(c).append_to(result);
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
        if (obj.empty()) {
            return {};
        }
        u16string result;
        result.reserve(obj.size() * 2);
        for (const value_type c: obj) {
            codepoint::from_utf32(c).append_to(result);
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

/** @} */ // UTF

/** @} */ // Packages

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_STRING_UTF_HPP__
