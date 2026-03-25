#ifndef NEFORCE_CORE_STRING_FORMAT_HPP__
#define NEFORCE_CORE_STRING_FORMAT_HPP__

/**
 * @file format.hpp
 * @brief 字符串格式化功能
 *
 * 此文件提供字符串格式化功能。
 * 支持数值的格式化选项，包括对齐、填充、进制、精度等。
 */

#include "NeForce/core/string/to_string.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup StringFormat 字符串格式化
 * @brief 字符串格式化功能
 * @{
 */

/**
 * @enum FORMAT_ALIGN
 * @brief 对齐方式枚举
 */
enum class FORMAT_ALIGN {
    LEFT        = '<',  ///< 左对齐
    RIGHT       = '>',  ///< 右对齐
    INTERNAL    = '=',  ///< 内部对齐
    CENTER      = '^'   ///< 居中对齐
};

/**
 * @enum FORMAT_TYPE
 * @brief 数值类型格式枚举
 */
enum class FORMAT_TYPE {
    BINARY      = 'b',  ///< 二进制
    OCTAL       = 'o',  ///< 八进制
    DECIMAL     = 'd',  ///< 十进制
    HEX_LOW     = 'x',  ///< 十六进制小写
    HEX_UP      = 'X',  ///< 十六进制大写
    FLOAT_FIX   = 'f',  ///< 固定小数位浮点数
    FLOAT_EXP   = 'e',  ///< 科学计数法
    FLOAT_GEN   = 'g'   ///< 通用格式
};

/**
 * @struct format_options
 * @brief 格式化选项结构体
 *
 * 包含所有格式化相关的选项：
 * - fill: 填充字符
 * - width: 总宽度
 * - precision: 精度（浮点数有效位）
 * - alignment: 对齐方式
 * - sign_mode: 符号显示模式
 * - show_base: 是否显示进制前缀
 * - type: 数值类型格式
 * - zero_pad: 是否用0填充
 */
struct format_options {
    char fill = ' ';                     ///< 填充字符
    int width = 0;                        ///< 总宽度
    int precision = -1;                    ///< 精度（-1表示默认）
    FORMAT_ALIGN alignment = FORMAT_ALIGN::RIGHT;  ///< 对齐方式
    char sign_mode = 0;  ///< 符号模式：0默认，+总是显示，-只显示负数，空格正数显示空格
    bool show_base = false;                ///< 是否显示进制前缀
    FORMAT_TYPE type = FORMAT_TYPE::DECIMAL;  ///< 数值类型格式
    bool zero_pad = false;                 ///< 是否用0填充
};

/// @cond
NEFORCE_BEGIN_INNER__

/**
 * @brief 将字符转换为对齐方式枚举
 * @param c 字符
 * @return 对应的对齐方式
 */
constexpr FORMAT_ALIGN to_number_alignment(const char c) {
    switch (c) {
        case '<': return FORMAT_ALIGN::LEFT;
        case '>': return FORMAT_ALIGN::RIGHT;
        case '^': return FORMAT_ALIGN::CENTER;
        case '=': return FORMAT_ALIGN::INTERNAL;
        default:  return FORMAT_ALIGN::RIGHT;
    }
}

/**
 * @brief 将字符转换为数值类型格式枚举
 * @param c 字符
 * @return 对应的数值类型格式
 */
constexpr FORMAT_TYPE to_number_type(const char c) {
    switch (c) {
        case 'b': return FORMAT_TYPE::BINARY;
        case 'o': return FORMAT_TYPE::OCTAL;
        case 'd': return FORMAT_TYPE::DECIMAL;
        case 'x': return FORMAT_TYPE::HEX_LOW;
        case 'X': return FORMAT_TYPE::HEX_UP;
        case 'f': return FORMAT_TYPE::FLOAT_FIX;
        case 'e': return FORMAT_TYPE::FLOAT_EXP;
        case 'g': return FORMAT_TYPE::FLOAT_GEN;
        default:  return FORMAT_TYPE::DECIMAL;
    }
}

/**
 * @brief 解析数字格式字符串
 * @param fmt_str 格式字符串
 * @return 解析后的格式化选项
 *
 * 支持的格式语法：[填充字符]对齐方式[符号][#][0][宽度][.精度][类型]
 * 例如：":#010x" 表示用0填充，宽度10，显示进制前缀，十六进制
 */
NEFORCE_CONSTEXPR20 format_options parse_number_format(const string_view& fmt_str) {
    format_options options;
    size_t pos = 0;

    if (fmt_str.empty()) return options;

    bool found_align = false;
    if (pos + 1 < fmt_str.size()) {
        const char second_char = fmt_str[pos + 1];
        const char first_char = fmt_str[pos];

        if ((second_char == '<' || second_char == '>' ||
             second_char == '^' || second_char == '=') &&
            (first_char != '+' && first_char != '-' && first_char != ' ')) {
            options.fill = first_char;
            options.alignment = to_number_alignment(second_char);
            pos += 2;
            found_align = true;
        }
    }

    if (!found_align && pos < fmt_str.size()) {
        const char c = fmt_str[pos];
        if (c == '<' || c == '>' || c == '^' || c == '=') {
            options.alignment = to_number_alignment(c);
            ++pos;
            found_align = true;
        }
    }

    if (pos < fmt_str.size()) {
        const char c = fmt_str[pos];
        if (c == '+' || c == '-' || c == ' ') {
            options.sign_mode = c;
            ++pos;
        }
    }

    if (pos < fmt_str.size() && fmt_str[pos] == '#') {
        options.show_base = true;
        ++pos;
    }

    if (pos < fmt_str.size() && fmt_str[pos] == '0' &&
        options.fill == ' ' && options.alignment == FORMAT_ALIGN::RIGHT) {
        options.zero_pad = true;
        options.fill = '0';
        options.alignment = FORMAT_ALIGN::INTERNAL;
        ++pos;
    }

    if (pos < fmt_str.size() && _NEFORCE is_digit(fmt_str[pos])) {
        int width = 0;
        while (pos < fmt_str.size() && _NEFORCE is_digit(fmt_str[pos])) {
            width = width * 10 + (fmt_str[pos] - '0');
            ++pos;
        }
        options.width = width;
    }

    if (pos < fmt_str.size() && fmt_str[pos] == '.') {
        ++pos;
        int precision = 0;
        while (pos < fmt_str.size() && _NEFORCE is_digit(fmt_str[pos])) {
            precision = precision * 10 + (fmt_str[pos] - '0');
            ++pos;
        }
        options.precision = precision;
    }

    if (pos < fmt_str.size()) {
        options.type = to_number_type(fmt_str[pos]);
        ++pos;
    }

    return options;
}

NEFORCE_END_INNER__
/// @endcond

/**
 * @struct formatter
 * @brief 格式化器主模板
 * @tparam Number 要格式化的类型
 * @tparam Dummy 用于SFINAE
 *
 * 通过特化实现不同类型值的格式化。
 */
template <typename Number, typename Dummy = void>
struct formatter;

/**
 * @brief 浮点数类型的格式化器特化
 * @tparam T 浮点数类型
 */
template <typename T>
struct formatter<T, enable_if_t<is_floating_point_v<T>>> {
    /**
     * @brief 格式化浮点数
     * @param value 要格式化的值
     * @param options 格式化选项
     * @return 格式化后的字符串
     */
    NEFORCE_CONSTEXPR20 string operator ()(const T& value, const format_options& options) const {
        double val = static_cast<double>(value);
        const bool is_negative = val < 0;
        if (is_negative) val = -val;

        string prefix;
        if (is_negative) {
            prefix += "-";
        } else if (options.sign_mode == '+') {
            prefix += "+";
        } else if (options.sign_mode == ' ') {
            prefix += " ";
        }

        string number_str;
        const int precision = options.precision >= 0 ? options.precision : 6;

        switch (options.type) {
            case FORMAT_TYPE::FLOAT_FIX: {
                number_str = _NEFORCE to_string_fixed(val, precision);
                break;
            }
            case FORMAT_TYPE::FLOAT_EXP: {
                number_str = _NEFORCE to_string_scientific(val, precision);
                break;
            }
            case FORMAT_TYPE::FLOAT_GEN:
            default: {
                number_str = _NEFORCE to_string_general(val, precision);
                break;
            }
        }

        if (!number_str.empty() && number_str[0] == '-') {
            number_str = number_str.substr(1);
        }

        string result = prefix + number_str;
        if (options.width <= 0 || result.size() >= static_cast<size_t>(options.width)) {
            return result;
        }

        const size_t fill_count = options.width - result.size();
        const string fill_str(fill_count, options.fill);

        switch (options.alignment) {
            case FORMAT_ALIGN::LEFT: {
                return result + fill_str;
            }
            case FORMAT_ALIGN::RIGHT: {
                return fill_str + result;
            }
            case FORMAT_ALIGN::INTERNAL: {
                return prefix + fill_str + number_str;
            }
            case FORMAT_ALIGN::CENTER: {
                const size_t left_fill = fill_count / 2;
                const size_t right_fill = fill_count - left_fill;
                return string(left_fill, options.fill) + result + string(right_fill, options.fill);
            }
            default: {
                return fill_str + result;
            }
        }
    }
};

/**
 * @brief 有符号整数类型的格式化器特化
 * @tparam T 有符号整数类型
 */
template <typename T>
struct formatter<T, enable_if_t<is_integral_v<T> && is_signed_v<T>>> {
    /**
     * @brief 格式化有符号整数
     * @param value 要格式化的值
     * @param options 格式化选项
     * @return 格式化后的字符串
     */
    NEFORCE_CONSTEXPR20 string operator ()(const T& value, const format_options& options) const {
        const int64_t val = static_cast<int64_t>(value);
        const bool is_negative = val < 0;
        uint64_t abs_value = is_negative ? static_cast<uint64_t>(-val) : static_cast<uint64_t>(val);

        const char* digits;
        int base;
        switch (options.type) {
            case FORMAT_TYPE::BINARY: {
                digits = "01";
                base = 2;
                break;
            } case FORMAT_TYPE::OCTAL: {
                digits = "01234567";
                base = 8;
                break;
            } case FORMAT_TYPE::HEX_LOW: {
                digits = "0123456789abcdef";
                base = 16;
                break;
            } case FORMAT_TYPE::HEX_UP: {
                digits = "0123456789ABCDEF";
                base = 16;
                break;
            } case FORMAT_TYPE::DECIMAL: default: {
                digits = "0123456789";
                base = 10;
                break;
            }
        }

        string number_str;
        if (abs_value == 0) {
            number_str = "0";
        } else {
            while (abs_value > 0) {
                number_str = digits[abs_value % base] + number_str;
                abs_value /= base;
            }
        }

        string prefix;
        if (is_negative) {
            prefix = "-";
        } else if (options.sign_mode == '+') {
            prefix = "+";
        } else if (options.sign_mode == ' ') {
            prefix = " ";
        }

        if (options.show_base && base != 10) {
            switch (options.type) {
                case FORMAT_TYPE::BINARY:  prefix += "0b"; break;
                case FORMAT_TYPE::OCTAL:   prefix += "0o"; break;
                case FORMAT_TYPE::HEX_LOW: prefix += "0x"; break;
                case FORMAT_TYPE::HEX_UP:  prefix += "0X"; break;
                default: break;
            }
        }

        string result = prefix + number_str;
        if (options.width <= 0 || result.size() >= static_cast<size_t>(options.width)) {
            return result;
        }

        const size_t fill_count = options.width - result.size();
        const string fill_str(fill_count, options.fill);

        switch (options.alignment) {
            case FORMAT_ALIGN::LEFT: {
                return result + fill_str;
            }
            case FORMAT_ALIGN::RIGHT: {
                return fill_str + result;
            }
            case FORMAT_ALIGN::INTERNAL: {
                return prefix + fill_str + number_str;
            }
            case FORMAT_ALIGN::CENTER: {
                const size_t left_fill = fill_count / 2;
                const size_t right_fill = fill_count - left_fill;
                return string(left_fill, options.fill) + result + string(right_fill, options.fill);
            }
            default: {
                return fill_str + result;
            }
        }
    }
};

/**
 * @brief 无符号整数类型的格式化器特化
 * @tparam T 无符号整数类型
 */
template <typename T>
struct formatter<T, enable_if_t<is_integral_v<T> && is_unsigned_v<T>>> {
    /**
     * @brief 格式化无符号整数
     * @param value 要格式化的值
     * @param options 格式化选项
     * @return 格式化后的字符串
     */
    NEFORCE_CONSTEXPR20 string operator ()(const T& value, const format_options& options) const {
        string digits;
        int base;
        bool uppercase = false;

        switch (options.type) {
            case FORMAT_TYPE::HEX_LOW: {
                base = 16;
                uppercase = false;
                break;
            } case FORMAT_TYPE::HEX_UP: {
                base = 16;
                uppercase = true;
                break;
            } case FORMAT_TYPE::OCTAL: {
                base = 8;
                break;
            } case FORMAT_TYPE::BINARY: {
                base = 2;
                break;
            } case FORMAT_TYPE::DECIMAL: default: {
                base = 10;
                break;
            }
        }

        if (base == 10) {
            digits = inner::__uint_to_string<char>(static_cast<const unpackage_t<T>&>(value));
        } else {
            digits = inner::__uint_to_string_base(static_cast<const unpackage_t<T>&>(value), base, uppercase);
        }

        string base_prefix = "";
        if (options.show_base) {
            switch (base) {
                case 16: {
                    base_prefix = uppercase ? "0X" : "0x";
                    break;
                } case 8: {
                    base_prefix = "0";
                    break;
                } case 2: {
                    base_prefix = uppercase ? "0B" : "0b";
                    break;
                } default: break;
            }
        }

        string number_str = base_prefix + digits;

        if (options.width <= 0 || number_str.size() >= static_cast<size_t>(options.width)) {
            return number_str;
        }

        const size_t fill_count = options.width - number_str.size();

        switch (options.alignment) {
            case FORMAT_ALIGN::LEFT: {
                return number_str + string(fill_count, options.fill);
            }
            case FORMAT_ALIGN::RIGHT: {
                return string(fill_count, options.fill) + number_str;
            }
            case FORMAT_ALIGN::INTERNAL: {
                if (!base_prefix.empty()) {
                    return base_prefix + string(fill_count, options.fill) + digits;
                }
                return string(fill_count, options.fill) + number_str;
            }
            case FORMAT_ALIGN::CENTER: {
                const size_t left_fill = fill_count / 2;
                const size_t right_fill = fill_count - left_fill;
                return string(left_fill, options.fill) + number_str + string(right_fill, options.fill);
            }
            default: {
                return string(fill_count, options.fill) + number_str;
            }
        }
    }
};

/**
 * @brief 字符串类型的格式化器特化
 */
template <>
struct formatter<string> {
    /**
     * @brief 格式化字符串
     * @param value 要格式化的字符串
     * @param options 格式化选项
     * @return 格式化后的字符串
     */
    NEFORCE_CONSTEXPR20 string operator ()(const string& value, const format_options& options) const {
        if (options.width <= 0 || value.size() >= static_cast<size_t>(options.width)) {
            return value;
        }

        const size_t fill_count = options.width - value.size();
        const string fill_str(fill_count, options.fill);

        switch (options.alignment) {
            case FORMAT_ALIGN::LEFT: {
                return value + fill_str;
            }
            case FORMAT_ALIGN::RIGHT: {
                return fill_str + value;
            }
            case FORMAT_ALIGN::CENTER: {
                const size_t left_fill = fill_count / 2;
                const size_t right_fill = fill_count - left_fill;
                return string(left_fill, options.fill) + value + string(right_fill, options.fill);
            }
            case FORMAT_ALIGN::INTERNAL: default: {
                return fill_str + value;
            }
        }
    }
};

/**
 * @brief C风格字符串的格式化器特化
 */
template <>
struct formatter<const char*> {
    NEFORCE_CONSTEXPR20 string operator ()(const char* value, const format_options& options) const {
        return formatter<string>()(string(value), options);
    }
};

/**
 * @brief 字符串视图的格式化器特化
 */
template <>
struct formatter<string_view> {
    NEFORCE_CONSTEXPR20 string operator ()(const string_view value, const format_options& options) const {
        return formatter<string>()(string(value), options);
    }
};

/**
 * @brief 非const C风格字符串的格式化器特化
 */
template <>
struct formatter<char*> {
    NEFORCE_CONSTEXPR20 string operator ()(char* value, const format_options& options) const {
        return formatter<string>()(string(value), options);
    }
};

/**
 * @brief 单个字符的格式化器特化
 */
template <>
struct formatter<char> {
    NEFORCE_CONSTEXPR20 string operator ()(const char value, const format_options& options) const {
        return formatter<string>()(string(1, value), options);
    }
};

/**
 * @brief 布尔类型的格式化器特化
 */
template <>
struct formatter<bool> {
    NEFORCE_CONSTEXPR20 string operator ()(const bool value, const format_options& options) const {
        return formatter<string>()(value ? "true" : "false", options);
    }
};

/// @cond
NEFORCE_BEGIN_INNER__

/**
 * @brief 格式化实现（无参数版本）
 * @param fmt 格式字符串
 * @param pos 当前解析位置
 * @return 格式化后的字符串
 * @throws value_exception 如果格式错误
 */
NEFORCE_CONSTEXPR20 string format_impl(const string_view fmt, size_t& pos) {
    string result;
    while (pos < fmt.size()) {
        if (fmt[pos] == '{') {
            if (pos + 1 < fmt.size() && fmt[pos + 1] == '{') {
                result += '{';
                pos += 2;
            } else {
                NEFORCE_THROW_EXCEPTION(value_exception("Not enough arguments for format string"));
            }
        } else if (fmt[pos] == '}') {
            if (pos + 1 < fmt.size() && fmt[pos + 1] == '}') {
                result += '}';
                pos += 2;
            } else {
                NEFORCE_THROW_EXCEPTION(value_exception("Unmatched '}' in format string"));
            }
        } else {
            result += fmt[pos];
            ++pos;
        }
    }
    return result;
}

/**
 * @brief 格式化实现（带参数版本）
 * @tparam First 第一个参数类型
 * @tparam Rest 剩余参数类型
 * @param fmt 格式字符串
 * @param pos 当前解析位置
 * @param first 第一个参数
 * @param rest 剩余参数
 * @return 格式化后的字符串
 * @throws value_exception 如果格式错误
 */
template <typename First, typename... Rest>
NEFORCE_CONSTEXPR20 string format_impl(const string_view fmt, size_t& pos, First&& first, Rest&&... rest) {
    string result;
    while (pos < fmt.size()) {
        if (fmt[pos] == '{') {
            if (pos + 1 < fmt.size() && fmt[pos + 1] == '{') {
                result += '{';
                pos += 2;
                continue;
            }
            ++pos;
            size_t end_pos = pos;
            int depth = 1;
            while (end_pos < fmt.size() && depth > 0) {
                if (fmt[end_pos] == '{') {
                    ++depth;
                } else if (fmt[end_pos] == '}') {
                    --depth;
                }
                if (depth > 0) ++end_pos;
            }
            if (depth != 0) {
                NEFORCE_THROW_EXCEPTION(value_exception("Unmatched '{' in format string"));
            }

            const string_view spec_str = fmt.substr(pos, end_pos - pos);
            pos = end_pos + 1;
            format_options opts;
            if (spec_str.empty()) {
                opts = inner::parse_number_format("");
            } else if (spec_str[0] == ':') {
                opts = inner::parse_number_format(spec_str.substr(1));
            } else {
                opts = inner::parse_number_format(spec_str);
            }
            result += formatter<decay_t<First>>()(_NEFORCE forward<First>(first), opts);
            result += inner::format_impl(fmt, pos, _NEFORCE forward<Rest>(rest)...);
            return result;
        } else if (fmt[pos] == '}') {
            if (pos + 1 < fmt.size() && fmt[pos + 1] == '}') {
                result += '}';
                pos += 2;
            } else {
                NEFORCE_THROW_EXCEPTION(value_exception("Unmatched '}' in format string"));
            }
        } else {
            result += fmt[pos];
            ++pos;
        }
    }
    return result;
}

NEFORCE_END_INNER__
/// @endcond

/**
 * @brief 格式化字符串
 * @tparam Args 参数类型
 * @param fmt 格式字符串
 * @param args 要格式化的参数
 * @return 格式化后的字符串
 * @throws value_exception 如果格式错误
 */
template <typename... Args, enable_if_t<(sizeof...(Args) > 0), int> = 0>
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 string format(const string_view fmt, Args&&... args) {
    size_t pos = 0;
    return inner::format_impl(fmt, pos, _NEFORCE forward<Args>(args)...);
}

/** @} */ // StringFormat

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_STRING_FORMAT_HPP__
