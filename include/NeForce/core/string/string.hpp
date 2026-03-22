#ifndef NEFORCE_CORE_STRING_STRING_HPP__
#define NEFORCE_CORE_STRING_STRING_HPP__

/**
 * @file string.hpp
 * @brief 字符串类型别名和实用函数
 *
 * 此文件提供了basic_string的具体类型别名，
 * 以及用于创建字符串的字面量操作符、转义函数、读取行函数和地址格式化函数。
 */

#include "NeForce/core/string/basic_string.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup String 字符串
 * @brief 动态字符序列容器
 * @{
 */

/// 字符字符串
using string    = basic_string<char>;

/// 宽字符字符串
using wstring   = basic_string<wchar_t>;

#if defined(NEFORCE_STANDARD_20) || defined(NEXUSFORCE_ENABLE_DOXYGEN)
/// UTF-8字符串
using u8string  = basic_string<char8_t>;
#endif

/// UTF-16字符串
using u16string = basic_string<char16_t>;

/// UTF-32字符串
using u32string = basic_string<char32_t>;

/** @} */ // String

NEFORCE_BEGIN_LITERALS__

/**
 * @defgroup UserLiterals 字面量
 * @brief 用户定义字面量支持
 * @{
 */

/**
 * @brief 创建char字符串的字面量操作符
 * @param str 字符串字面量
 * @param len 字符串长度
 * @return string对象
 */
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 string operator ""_s(const char* str, size_t len) noexcept {
    return {str, len};
}

/**
 * @brief 创建wchar_t字符串的字面量操作符
 * @param str 宽字符串字面量
 * @param len 字符串长度
 * @return wstring对象
 */
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 wstring operator ""_s(const wchar_t* str, size_t len) noexcept {
    return {str, len};
}

#if defined(NEFORCE_STANDARD_20) || defined(NEXUSFORCE_ENABLE_DOXYGEN)
/**
 * @brief 创建char8_t字符串的字面量操作符
 * @param str UTF-8字符串字面量
 * @param len 字符串长度
 * @return u8string对象
 */
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 u8string operator ""_s(const char8_t* str, size_t len) noexcept {
    return {str, len};
}
#endif // NEFORCE_STANDARD_20

/**
 * @brief 创建char16_t字符串的字面量操作符
 * @param str UTF-16字符串字面量
 * @param len 字符串长度
 * @return u16string对象
 */
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 u16string operator ""_s(const char16_t* str, size_t len) noexcept {
    return {str, len};
}

/**
 * @brief 创建char32_t字符串的字面量操作符
 * @param str UTF-32字符串字面量
 * @param len 字符串长度
 * @return u32string对象
 */
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 u32string operator ""_s(const char32_t* str, size_t len) noexcept {
    return {str, len};
}

/** @} */ // UserLiterals

NEFORCE_END_LITERALS__

/**
 * @defgroup String 字符串
 * @brief 动态字符序列容器
 * @{
 */

/**
 * @brief 转义字符串视图中的特殊字符
 * @param str 要转义的字符串视图
 * @return 转义后的字符串
 *
 * 将字符串中的特殊字符转换为转义序列。
 * 对于不可打印的控制字符（<0x20），转换为Unicode转义。
 */
NEFORCE_CONSTEXPR20 string escape(const string_view str) {
    string result;
    result.reserve(str.length() + str.length() / 4);

    for (const char c : str) {
        switch (c) {
            case '\"': {
                result += "\\\"";
                break;
            }
            case '\'': {
                result += "\\\'";
                break;
            }
            case '\\': {
                result += "\\\\";
                break;
            }
            case '\b': {
                result += "\\b";
                break;
            }
            case '\f': {
                result += "\\f";
                break;
            }
            case '\n': {
                result += "\\n";
                break;
            }
            case '\r': {
                result += "\\r";
                break;
            }
            case '\t': {
                result += "\\t";
                break;
            }
            case '\v': {
                result += "\\v";
                break;
            }
            default: {
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
    }
    return result;
}

/**
 * @brief 转义字符串中的特殊字符
 * @param str 要转义的字符串
 * @return 转义后的字符串
 */
NEFORCE_CONSTEXPR20 string escape(const string& str) {
    return escape(str.view());
}

/**
 * @brief 转义C风格字符串中的特殊字符
 * @param str C风格字符串
 * @return 转义后的字符串
 */
NEFORCE_CONSTEXPR20 string escape(const char* str) {
    return escape(string_view{str});
}

/**
 * @brief 从字符串视图中按分隔符读取一行（字符版本）
 * @tparam CharT 字符类型
 * @param data 源字符串视图
 * @param pos 当前读取位置
 * @param str 输出参数，存储读取的行
 * @param delim 分隔符，默认为换行符
 * @return 是否成功读取到数据
 *
 * 从data的pos位置开始读取字符，直到遇到分隔符或到达末尾。
 * 将读取的字符（不包括分隔符）存入str，并更新pos位置。
 * 返回true表示至少读取了一个字符（包括分隔符的情况）。
 */
template <typename CharT>
NEFORCE_CONSTEXPR20 bool getline(const basic_string_view<CharT> data, size_t& pos,
                              basic_string<CharT>& str, CharT delim = static_cast<CharT>('\n')) {
    str.clear();
    bool has_read = false;
    while (pos < data.size()) {
        has_read = true;
        const CharT c = data[pos++];
        if (c == delim) break;
        str.push_back(c);
    }
    return has_read;
}

/**
 * @brief 从字符串中按分隔符读取一行（字符串版本）
 * @tparam CharT 字符类型
 * @param data 源字符串
 * @param pos 当前读取位置
 * @param str 输出参数，存储读取的行
 * @param delim 分隔符，默认为换行符
 * @return 是否成功读取到数据
 */
template <typename CharT>
NEFORCE_CONSTEXPR20 bool getline(const basic_string<CharT>& data, size_t& pos,
                              basic_string<CharT>& str, CharT delim = static_cast<CharT>('\n')) {
    str.clear();
    bool has_read = false;
    while (pos < data.size()) {
        has_read = true;
        const CharT c = data[pos++];
        if (c == delim) break;
        str.push_back(c);
    }
    return has_read;
}

/**
 * @brief 从字符串视图中按谓词判断的分隔符读取一行
 * @tparam CharT 字符类型
 * @tparam Pred 谓词类型
 * @param data 源字符串视图
 * @param pos 当前读取位置
 * @param str 输出参数，存储读取的行
 * @param split 谓词函数，返回true表示该字符是分隔符
 * @return 是否成功读取到数据
 *
 * 使用自定义谓词判断分隔符，可以处理复杂的行分割逻辑。
 */
template <typename CharT, typename Pred>
NEFORCE_CONSTEXPR20 bool getline(const basic_string_view<CharT> data, size_t& pos,
                              basic_string<CharT>& str, Pred split = [](const CharT c) {
                                  return c == static_cast<CharT>('\n');
                              }) {
    str.clear();
    bool has_read = false;
    while (pos < data.size()) {
        has_read = true;
        const CharT c = data[pos++];
        if (split(c)) break;
        str.push_back(c);
    }
    return has_read;
}

/**
 * @brief 从字符串中按谓词判断的分隔符读取一行
 * @tparam CharT 字符类型
 * @tparam Pred 谓词类型
 * @param data 源字符串
 * @param pos 当前读取位置
 * @param str 输出参数，存储读取的行
 * @param split 谓词函数，返回true表示该字符是分隔符
 * @return 是否成功读取到数据
 */
template <typename CharT, typename Pred>
NEFORCE_CONSTEXPR20 bool getline(const basic_string<CharT>& data, size_t& pos,
                              basic_string<CharT>& str, Pred split = [](const CharT c) {
                                  return c == static_cast<CharT>('\n');
                              }) {
    str.clear();
    bool has_read = false;
    while (pos < data.size()) {
        has_read = true;
        const CharT c = data[pos++];
        if (split(c)) break;
        str.push_back(c);
    }
    return has_read;
}

/**
 * @brief 将指针转换为十六进制地址字符串
 * @param p 要转换的指针
 * @return 格式化的地址字符串
 *
 * 将指针转换为"0x"开头的十六进制字符串表示。
 * 地址长度根据系统位数自动调整。
 * 如果指针为空，返回"nullptr"。
 */
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 string address_string(const void* p) {
    if (p == nullptr) return {"nullptr"};

#ifdef NEFORCE_ARCH_BITS_64
    constexpr uintptr_t address_mask = 0xF000000000000000ULL;
    constexpr int address_shift = 60;
#else
    constexpr uintptr_t address_mask = 0xF0000000UL;
    constexpr int address_shift = 28;
#endif
    constexpr char hex_digits[] = "0123456789abcdef";
    constexpr size_t hex_digit_count = sizeof(void*) * 2;

    const uintptr_t addr_val = reinterpret_cast<uintptr_t>(p);
    uintptr_t mask = address_mask;
    int shift = address_shift;

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

/** @} */ // String

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_STRING_STRING_HPP__
