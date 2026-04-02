#ifndef NEFORCE_CORE_STRING_STRING_VIEW_HPP__
#define NEFORCE_CORE_STRING_STRING_VIEW_HPP__
#include "basic_string_view.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup StringView 字符串视图
 * @brief 非拥有只读字符串视图
 * @{
 */

/// 字符字符串视图
using string_view = basic_string_view<char>;

/// 宽字符字符串视图
using wstring_view = basic_string_view<wchar_t>;

#if defined(NEFORCE_STANDARD_20) || defined(NEXUSFORCE_ENABLE_DOXYGEN)
/// UTF-8字符串视图
using u8string_view = basic_string_view<char8_t>;
#endif // NEFORCE_STANDARD_20

/// UTF-16字符串视图
using u16string_view = basic_string_view<char16_t>;

/// UTF-32字符串视图
using u32string_view = basic_string_view<char32_t>;

/** @} */ // StringView


NEFORCE_BEGIN_LITERALS__

/**
 * @defgroup UserLiterals 字面量
 * @brief 用户定义字面量支持
 * @{
 */

/**
 * @brief 创建char字符串视图的字面量操作符
 * @param str 字符串字面量
 * @param len 字符串长度
 * @return string_view对象
 */
NEFORCE_NODISCARD constexpr string_view operator""_sv(const char* str, size_t len) noexcept { return {str, len}; }

/**
 * @brief 创建wchar_t字符串视图的字面量操作符
 * @param str 宽字符串字面量
 * @param len 字符串长度
 * @return wstring_view对象
 */
NEFORCE_NODISCARD constexpr wstring_view operator""_sv(const wchar_t* str, size_t len) noexcept { return {str, len}; }

#if defined(NEFORCE_STANDARD_20) || defined(NEXUSFORCE_ENABLE_DOXYGEN)
/**
 * @brief 创建char8_t字符串视图的字面量操作符
 * @param str UTF-8字符串字面量
 * @param len 字符串长度
 * @return u8string_view对象
 */
NEFORCE_NODISCARD constexpr u8string_view operator""_sv(const char8_t* str, size_t len) noexcept { return {str, len}; }
#endif // NEFORCE_STANDARD_20

/**
 * @brief 创建char16_t字符串视图的字面量操作符
 * @param str UTF-16字符串字面量
 * @param len 字符串长度
 * @return u16string_view对象
 */
NEFORCE_NODISCARD constexpr u16string_view operator""_sv(const char16_t* str, size_t len) noexcept {
    return {str, len};
}

/**
 * @brief 创建char32_t字符串视图的字面量操作符
 * @param str UTF-32字符串字面量
 * @param len 字符串长度
 * @return u32string_view对象
 */
NEFORCE_NODISCARD constexpr u32string_view operator""_sv(const char32_t* str, size_t len) noexcept {
    return {str, len};
}

/** @} */ // UserLiterals

NEFORCE_END_LITERALS__


/**
 * @defgroup StringView 字符串视图
 * @brief 非拥有只读字符串视图
 * @{
 */

/**
 * @brief 从字符串视图中按分隔符提取一行
 * @tparam CharT 字符类型
 * @param data 源字符串视图
 * @param pos 当前读取位置（输入输出参数）
 * @param str 输出参数，存储提取的行
 * @param delim 分隔符，默认为换行符'\n'
 * @return 是否成功提取到行（false表示已到达末尾）
 */
template <typename CharT>
constexpr bool getline(const basic_string_view<CharT> data, size_t& pos, basic_string_view<CharT>& str,
                       CharT delim = static_cast<CharT>('\n')) {

    if (pos >= data.size()) {
        str = basic_string_view<CharT>();
        return false;
    }

    size_t start = pos;
    size_t end = pos;
    while (end < data.size() && data[end] != delim) {
        ++end;
    }
    str = data.substr(start, end - start);
    pos = (end < data.size()) ? end + 1 : end;

    return true;
}

/**
 * @brief 从字符串视图中按谓词判断的分隔符提取一行
 * @tparam CharT 字符类型
 * @tparam Pred 谓词类型
 * @param data 源字符串视图
 * @param pos 当前读取位置（输入输出参数）
 * @param str 输出参数，存储提取的行
 * @param split 谓词函数，返回true表示该字符是分隔符
 * @return 是否成功提取到行（false表示已到达末尾）
 *
 * 使用自定义谓词判断分隔符，可以处理复杂的行分割逻辑。
 */
template <typename CharT, typename Pred>
constexpr bool getline(
        const basic_string_view<CharT> data, size_t& pos, basic_string_view<CharT>& str,
        Pred split = [](const CharT ch) { return ch == static_cast<CharT>('\n'); }) {

    if (pos >= data.size()) {
        str = basic_string_view<CharT>();
        return false;
    }

    size_t start = pos;
    size_t end = pos;
    while (end < data.size() && !split(data[end])) {
        ++end;
    }
    str = data.substr(start, end - start);
    pos = (end < data.size()) ? end + 1 : end;

    return true;
}

/** @} */ // StringView

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_STRING_STRING_VIEW_HPP__
