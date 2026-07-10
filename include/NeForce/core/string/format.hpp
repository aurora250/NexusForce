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
#include "NeForce/core/system/locale.hpp"
#include "NeForce/core/utility/tuple.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Format 字符串格式化
 * @brief 字符串格式化功能
 * @{
 */

/**
 * @enum format_align
 * @brief 对齐方式枚举
 */
enum class format_align {
    DEFAULT, ///< 默认（数字右对齐，其他左对齐）
    LEFT,    ///< 左对齐 '<'
    RIGHT,   ///< 右对齐 '>'
    CENTER,  ///< 居中 '^'
    NUMERIC  ///< 符号感知填充 '='
};

/**
 * @enum format_type
 * @brief 数值类型格式枚举
 */
enum class format_type : uint8_t {
    DEFAULT,    ///< 默认（由类型决定）
    DECIMAL,    ///< 十进制 'd' / 'f' / 'g'
    BINARY,     ///< 二进制 'b'
    OCTAL,      ///< 八进制 'o'
    HEX,        ///< 十六进制 'x' / 'X'
    SCIENTIFIC, ///< 科学计数法 'e' / 'E'
    FIXED,      ///< 固定小数 'f'
    GENERAL,    ///< 通用浮点 'g' / 'G'
    CHAR,       ///< 字符 'c'
};

/**
 * @struct format_options
 * @brief 格式选项
 *
 * 对应格式规范：[[fill]align][sign][#][0][width][.precision][type]
 */
struct format_options {
    char fill = ' ';                            ///< 填充字符
    format_align align = format_align::DEFAULT; ///< 对齐方式
    format_type type = format_type::DEFAULT;    ///< 类型
    int width = 0;                              ///< 最小宽度（0表示不限制）
    int precision = -1;                         ///< 精度（-1表示默认）
    bool uppercase = false;                     ///< 是否大写
    bool alternate = false;                     ///< 是否备用格式（# 前缀）
    bool zero_pad = false;                      ///< 是否零填充
    bool show_sign = false;                     ///< 是否强制显示符号 '+'
    bool space_sign = false;                    ///< 是否空格占位符号 ' '
    const locale* loc = nullptr;                ///< locale 指针，非空时进行 locale 感知格式化
};

/// @cond
NEFORCE_BEGIN_INNER__

NEFORCE_CONSTEXPR20 string uint_to_string_base(uint64_t value, const int base, const bool uppercase) {
    if (value == 0) {
        return "0";
    }

    string result;
    result.reserve(20);

    constexpr auto digits_lower = "0123456789abcdef";
    constexpr auto digits_upper = "0123456789ABCDEF";
    const auto* digits = uppercase ? digits_upper : digits_lower;

    while (value > 0) {
        const uint64_t remainder = value % base;
        value /= base;
        result.push_back(digits[remainder]);
    }

    result.reverse();
    return result;
}

constexpr format_align to_number_alignment(const char c) {
    switch (c) {
        case '<':
            return format_align::LEFT;
        case '>':
            return format_align::RIGHT;
        case '^':
            return format_align::CENTER;
        case '=':
            return format_align::NUMERIC;
        default:
            return format_align::DEFAULT;
    }
}

constexpr format_type to_number_type(const char c) {
    switch (c) {
        case 'd':
            return format_type::DECIMAL;
        case 'b':
        case 'B':
            return format_type::BINARY;
        case 'o':
            return format_type::OCTAL;
        case 'x':
        case 'X':
            return format_type::HEX;
        case 'e':
        case 'E':
            return format_type::SCIENTIFIC;
        case 'f':
        case 'F':
            return format_type::FIXED;
        case 'g':
        case 'G':
            return format_type::GENERAL;
        case 'c':
            return format_type::CHAR;
        default:
            return format_type::DEFAULT;
    }
}

constexpr format_options parse_number_format(const string_view& fmt_str) {
    format_options options;
    size_t pos = 0;

    if (fmt_str.empty()) {
        return options;
    }

    bool found_align = false;
    if (pos + 1 < fmt_str.size()) {
        const char first_char = fmt_str[pos];
        const char second_char = fmt_str[pos + 1];

        if (second_char == '<' || second_char == '>' || second_char == '^' || second_char == '=') {
            if (first_char != '+' && first_char != '-' && first_char != ' ') {
                options.fill = first_char;
                options.align = to_number_alignment(second_char);
                pos += 2;
                found_align = true;
            }
        }
    }

    if (!found_align && pos < fmt_str.size()) {
        const char c = fmt_str[pos];
        if (c == '<' || c == '>' || c == '^' || c == '=') {
            options.align = to_number_alignment(c);
            ++pos;
        }
    }

    if (pos < fmt_str.size()) {
        const char c = fmt_str[pos];
        if (c == '+') {
            options.show_sign = true;
            ++pos;
        } else if (c == ' ') {
            options.space_sign = true;
            ++pos;
        } else if (c == '-') {
            ++pos;
            if (pos < fmt_str.size()) {
                const char next = fmt_str[pos];
                if (next == '<' || next == '>' || next == '^' || next == '=') {
                    options.align = to_number_alignment(next);
                    ++pos;
                }
            }
        }
    }

    if (pos < fmt_str.size() && fmt_str[pos] == '#') {
        options.alternate = true;
        ++pos;
    }

    if (pos < fmt_str.size() && fmt_str[pos] == '0' && options.fill == ' ' && options.align == format_align::DEFAULT) {
        options.zero_pad = true;
        options.fill = '0';
        ++pos;
    }

    if (pos < fmt_str.size() && is_digit(fmt_str[pos])) {
        int width = 0;
        while (pos < fmt_str.size() && is_digit(fmt_str[pos])) {
            width = width * 10 + (fmt_str[pos] - '0');
            ++pos;
        }
        options.width = width;
    }

    if (pos < fmt_str.size() && fmt_str[pos] == '.') {
        ++pos;
        int precision = 0;
        while (pos < fmt_str.size() && is_digit(fmt_str[pos])) {
            precision = precision * 10 + (fmt_str[pos] - '0');
            ++pos;
        }
        options.precision = precision;
    }

    if (pos < fmt_str.size()) {
        const char c = fmt_str[pos];
        options.type = to_number_type(c);
        if (c == 'X' || c == 'E' || c == 'G' || c == 'B') {
            options.uppercase = true;
        }
        ++pos;
    }

    return options;
}


NEFORCE_CONSTEXPR20 string apply_format_options(string raw, const format_options& options,
                                                const bool is_numeric = false) {
    char existing_sign = '\0';
    if (!raw.empty() && (raw[0] == '-' || raw[0] == '+' || raw[0] == ' ')) {
        char sign = raw[0];
        raw = raw.tail(1);
        existing_sign = sign;
    }

    string prefix;
    if (options.alternate && is_numeric) {
        switch (options.type) {
            case format_type::HEX: {
                prefix = options.uppercase ? "0X" : "0x";
                break;
            }
            case format_type::BINARY: {
                prefix = options.uppercase ? "0B" : "0b";
                break;
            }
            case format_type::OCTAL: {
                if (raw.empty() || raw[0] != '0') {
                    prefix = "0";
                }
                break;
            }
            default: {
                break;
            }
        }
    }

    string sign_str;
    if (existing_sign == '-') {
        sign_str = "-";
    } else if (options.show_sign) {
        sign_str = "+";
    } else if (options.space_sign) {
        sign_str = " ";
    }

    const size_t content_len = sign_str.size() + prefix.size() + raw.size();
    const size_t target_width = (options.width > 0) ? static_cast<size_t>(options.width) : 0;

    const size_t pad_total = (content_len < target_width) ? target_width - content_len : 0;

    format_align align = options.align;
    if (align == format_align::DEFAULT) {
        align = is_numeric ? format_align::RIGHT : format_align::LEFT;
    }

    if (options.zero_pad && is_numeric && align == format_align::RIGHT) {
        align = format_align::NUMERIC;
    }

    if (align == format_align::NUMERIC && is_numeric) {
        const char fill_char = options.fill;
        string result;
        result.reserve(target_width > 0 ? target_width : content_len);
        result += sign_str;
        result += prefix;
        for (size_t i = 0; i < pad_total; ++i) {
            result += fill_char;
        }
        result += raw;
        return result;
    }

    const char fill_char = options.fill;
    string left_pad;
    string right_pad;

    switch (align) {
        case format_align::LEFT: {
            right_pad = string(pad_total, fill_char);
            break;
        }
        case format_align::CENTER: {
            const size_t left_count = pad_total / 2;
            const size_t right_count = pad_total - left_count;
            left_pad = string(left_count, fill_char);
            right_pad = string(right_count, fill_char);
            break;
        }
        case format_align::RIGHT:
        default: {
            left_pad = string(pad_total, fill_char);
            break;
        }
    }

    string result;
    result.reserve(target_width > 0 ? target_width : content_len);
    result += left_pad;
    result += sign_str;
    result += prefix;
    result += raw;
    result += right_pad;
    return result;
}

template <typename T, bool Signed>
struct integer_formatter_impl {
    NEFORCE_CONSTEXPR20 string operator()(const T value, const format_options& options) const {
        using UT = conditional_t<Signed, make_unsigned_t<T>, T>;

        const bool is_negative = Signed && (value < 0);
        const UT abs_value = is_negative ? static_cast<UT>(0 - static_cast<UT>(value)) : static_cast<UT>(value);
        const auto compatible = static_cast<uint64_t>(abs_value);

        string raw;

        switch (options.type) {
            case format_type::BINARY: {
                raw = inner::uint_to_string_base(compatible, 2, options.uppercase);
                break;
            }
            case format_type::OCTAL: {
                raw = inner::uint_to_string_base(compatible, 8, options.uppercase);
                break;
            }
            case format_type::HEX: {
                raw = inner::uint_to_string_base(compatible, 16, options.uppercase);
                break;
            }
            case format_type::CHAR: {
                return inner::apply_format_options(string(1, static_cast<char>(value)), options, false);
            }
            case format_type::DECIMAL:
            case format_type::DEFAULT:
            default: {
                raw = inner::__int_to_string_dispatch(value);
                return inner::apply_format_options(_NEFORCE move(raw), options, true);
            }
        }

        if (is_negative) {
            raw = "-" + raw;
        }
        return inner::apply_format_options(_NEFORCE move(raw), options, true);
    }
};

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
    NEFORCE_CONSTEXPR20 string operator()(const T& value, const format_options& options) const {
        if (options.loc != nullptr && (options.type == format_type::DEFAULT || options.type == format_type::FIXED ||
                                       options.type == format_type::GENERAL)) {
            const int prec = (options.precision >= 0) ? options.precision : 2;
            return options.loc->format_number(static_cast<double>(value), prec);
        }
        const int prec = (options.precision >= 0) ? options.precision : 6;
        string raw;

        switch (options.type) {
            case format_type::SCIENTIFIC: {
                raw = _NEFORCE to_string_scientific(value, prec);
                break;
            }
            case format_type::FIXED: {
                raw = _NEFORCE to_string_fixed(value, prec);
                break;
            }
            case format_type::GENERAL:
            case format_type::DECIMAL:
            case format_type::DEFAULT:
            default: {
                raw = _NEFORCE to_string_general(value, prec);
                break;
            }
        }

        if (options.uppercase) {
            for (auto& c: raw) {
                if (c == 'e') {
                    c = 'E';
                    break;
                }
            }
        }

        return inner::apply_format_options(_NEFORCE move(raw), options, true);
    }
};

/**
 * @brief 有符号整数类型的格式化器特化
 * @tparam T 有符号整数类型
 */
template <typename T>
struct formatter<T, enable_if_t<is_standard_integral_v<T> && is_signed_v<T>>> {
    /**
     * @brief 格式化有符号整数
     * @param value 要格式化的值
     * @param options 格式化选项
     * @return 格式化后的字符串
     */
    NEFORCE_CONSTEXPR20 string operator()(const T value, const format_options& options) const {
        if (options.loc != nullptr && options.type == format_type::DEFAULT) {
            return options.loc->format_number(static_cast<int64_t>(value));
        }
        return inner::integer_formatter_impl<T, true>{}(value, options);
    }
};

/**
 * @brief 无符号整数类型的格式化器特化
 * @tparam T 无符号整数类型
 */
template <typename T>
struct formatter<T, enable_if_t<is_standard_integral_v<T> && is_unsigned_v<T>>> {
    /**
     * @brief 格式化无符号整数
     * @param value 要格式化的值
     * @param options 格式化选项
     * @return 格式化后的字符串
     */
    NEFORCE_CONSTEXPR20 string operator()(const T value, const format_options& options) const {
        if (options.loc != nullptr && options.type == format_type::DEFAULT) {
            return options.loc->format_number(static_cast<int64_t>(value));
        }
        // NOLINTNEXTLINE(readability-implicit-bool-conversion)
        return inner::integer_formatter_impl<T, false>{}(value, options);
    }
};

/**
 * @brief 单个字符的格式化器特化
 */
template <>
struct formatter<char> {
    NEFORCE_CONSTEXPR20 string operator()(const char value, const format_options& options) const {
        switch (options.type) {
            case format_type::BINARY:
            case format_type::OCTAL:
            case format_type::HEX:
            case format_type::DECIMAL: {
                return inner::integer_formatter_impl<int, true>{}(static_cast<int>(value), options);
            }
            default: {
                break;
            }
        }
        return inner::apply_format_options(string(1, value), options, false);
    }
};

template <typename T>
struct formatter<T, enable_if_t<is_unpackaged_v<T> && is_base_of_v<ipackage<T, unpackage_t<T>>, T>>> {
    NEFORCE_CONSTEXPR20 string operator()(const T value, const format_options& options) const {
        return formatter<unpackage_t<T>>()(value.value(), options);
    }
};

/**
 * @brief 布尔类型的格式化器特化
 */
template <>
struct formatter<bool> {
    NEFORCE_CONSTEXPR20 string operator()(const bool value, const format_options& options) const {
        switch (options.type) {
            case format_type::BINARY:
            case format_type::OCTAL:
            case format_type::HEX:
            case format_type::DECIMAL: {
                return inner::integer_formatter_impl<int, false>{}(static_cast<int>(value), options);
            }
            default: {
                break;
            }
        }
        return inner::apply_format_options(value ? "true" : "false", options, false);
    }
};

/**
 * @brief 字符串类型的格式化器特化
 */
template <>
struct formatter<string> {
    NEFORCE_CONSTEXPR20 string operator()(const string& value, const format_options& options) const {
        string raw = value;
        if (options.precision >= 0 && raw.size() > static_cast<size_t>(options.precision)) {
            raw = raw.head(static_cast<size_t>(options.precision));
        }
        return inner::apply_format_options(_NEFORCE move(raw), options, false);
    }
};

/**
 * @brief 字符串视图的格式化器特化
 */
template <>
struct formatter<string_view> {
    NEFORCE_CONSTEXPR20 string operator()(const string_view value, const format_options& options) const {
        return formatter<string>()(string(value), options);
    }
};

/**
 * @brief C风格字符串的格式化器特化
 */
template <>
struct formatter<const char*> {
    NEFORCE_CONSTEXPR20 string operator()(const char* value, const format_options& options) const {
        if (value == nullptr) {
            return inner::apply_format_options("nullptr", options, false);
        }
        return formatter<string>{}(string(value), options);
    }
};

/**
 * @brief nullptr_t 格式化器
 */
template <>
struct formatter<nullptr_t> {
    NEFORCE_CONSTEXPR20 string operator()(nullptr_t, const format_options& options) const {
        return inner::apply_format_options("nullptr", options, false);
    }
};

/**
 * @brief 指针格式化器
 */
template <typename T>
struct formatter<T*, enable_if_t<!is_cstring_v<T*>>> {
    NEFORCE_CONSTEXPR20 string operator()(const T* ptr, const format_options& options) const {
        return inner::apply_format_options(_NEFORCE address_string(ptr), options, false);
    }
};

/**
 * @brief 非const C风格字符串的格式化器特化
 */
template <>
struct formatter<char*> {
    NEFORCE_CONSTEXPR20 string operator()(char* value, const format_options& options) const {
        return formatter<string>()(string(value), options);
    }
};

/// @cond
NEFORCE_BEGIN_INNER__

#ifdef NEFORCE_STANDARD_20

/**
 * @brief 编译期格式字符串验证
 * @tparam N 格式字符串长度
 * @param fmt 格式字符串
 * @return 括号平衡时返回true
 */
template <size_t N>
consteval bool validate_format_string(const char (&fmt)[N]) noexcept {
    size_t brace_count = 0;
    for (size_t i = 0; i < N - 1; ++i) {
        if (fmt[i] == '{') {
            if (i + 1 < N - 1 && fmt[i + 1] == '{') {
                ++i;
                continue;
            }
            ++brace_count;
        } else if (fmt[i] == '}') {
            if (i + 1 < N - 1 && fmt[i + 1] == '}') {
                ++i;
                continue;
            }
            if (brace_count == 0) {
                return false;
            }
            --brace_count;
        }
    }
    return brace_count == 0;
}

#endif

/**
 * @brief 从tuple中按运行时索引获取参数并应用formatter
 *
 * 通过编译期递归展开实现运行时索引的O(N)分发。
 * @tparam I 当前检查的索引
 * @tparam Tuple 参数tuple类型
 * @param idx 运行时目标索引
 * @param args 参数tuple
 * @param opts 格式化选项
 * @return 格式化后的字符串
 * @throws value_exception 索引越界时抛出
 */
template <size_t I, typename Tuple>
NEFORCE_CONSTEXPR20 enable_if_t<I == tuple_size_v<Tuple>, string>
format_get_and_apply(const size_t idx, const Tuple& args, const format_options& opts) {
    NEFORCE_THROW_EXCEPTION(value_exception("Format argument index out of range"));
}

template <size_t I, typename Tuple>
NEFORCE_CONSTEXPR20 enable_if_t<(I < tuple_size_v<Tuple>), string>
format_get_and_apply(const size_t idx, const Tuple& args, const format_options& opts) {
    if (idx == I) {
        return formatter<decay_t<tuple_element_t<I, Tuple>>>()(_NEFORCE get<I>(args), opts);
    }
    return format_get_and_apply<I + 1, Tuple>(idx, args, opts);
}

/**
 * @brief 格式化实现（无参数tuple版本，仅文本复制）
 */
NEFORCE_CONSTEXPR20 void format_impl(const string_view fmt, size_t& pos, string& out) {
    while (pos < fmt.size()) {
        if (fmt[pos] == '{') {
            if (pos + 1 < fmt.size() && fmt[pos + 1] == '{') {
                out += '{';
                pos += 2;
            } else {
                NEFORCE_THROW_EXCEPTION(value_exception("Not enough arguments"));
            }
        } else if (fmt[pos] == '}') {
            if (pos + 1 < fmt.size() && fmt[pos + 1] == '}') {
                out += '}';
                pos += 2;
            } else {
                NEFORCE_THROW_EXCEPTION(value_exception("Unmatched '}'"));
            }
        } else {
            out += fmt[pos++];
        }
    }
}

/**
 * @brief 格式化实现（带参数tuple版本，支持{0}{1}位置参数）
 * @tparam Tuple 参数tuple类型
 * @param fmt 格式字符串
 * @param pos 当前解析位置
 * @param out 输出到的字符串位置
 * @param args 参数tuple的const引用
 * @param next_seq 下一个顺序参数索引
 */
template <typename Tuple>
NEFORCE_CONSTEXPR20 void format_impl(const string_view fmt, size_t& pos, string& out, const Tuple& args,
                                     size_t& next_seq) {
    while (pos < fmt.size()) {
        if (fmt[pos] == '{') {
            if (pos + 1 < fmt.size() && fmt[pos + 1] == '{') {
                out += '{';
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
                if (depth > 0) {
                    ++end_pos;
                }
            }
            if (depth != 0) {
                NEFORCE_THROW_EXCEPTION(value_exception("Unmatched '{' in format string"));
            }

            const string_view spec_str = fmt.substr(pos, end_pos - pos);
            pos = end_pos + 1;

            format_options opts;
            size_t arg_idx = next_seq;

            if (spec_str.empty()) {
                // {} — sequential consumption
                opts = inner::parse_number_format("");
                ++next_seq;
            } else if (is_digit(spec_str[0])) {
                // {N} or {N:options} — positional
                size_t num_end = 0;
                arg_idx = 0;
                while (num_end < spec_str.size() && is_digit(spec_str[num_end])) {
                    arg_idx = arg_idx * 10 + static_cast<size_t>(spec_str[num_end] - '0');
                    ++num_end;
                }
                const string_view rest = spec_str.tail(num_end);
                if (rest.empty()) {
                    opts = inner::parse_number_format("");
                } else if (rest[0] == ':') {
                    opts = inner::parse_number_format(rest.tail(1));
                } else {
                    NEFORCE_THROW_EXCEPTION(value_exception("Invalid format specifier"));
                }
            } else if (spec_str[0] == ':') {
                // {:options} — sequential consumption with format options
                opts = inner::parse_number_format(spec_str.tail(1));
                ++next_seq;
            } else {
                NEFORCE_THROW_EXCEPTION(value_exception("Invalid format specifier"));
            }

            if (arg_idx >= tuple_size_v<Tuple>) {
                NEFORCE_THROW_EXCEPTION(value_exception("Format argument index out of range"));
            }

            out += inner::format_get_and_apply<0, Tuple>(arg_idx, args, opts);
        } else if (fmt[pos] == '}') {
            if (pos + 1 < fmt.size() && fmt[pos + 1] == '}') {
                out += '}';
                pos += 2;
            } else {
                NEFORCE_THROW_EXCEPTION(value_exception("Unmatched '}' in format string"));
            }
        } else {
            out += fmt[pos++];
        }
    }
}

/**
 * @brief 格式化实现
 */
template <typename First, typename... Rest>
NEFORCE_CONSTEXPR20 void format_impl(const string_view fmt, size_t& pos, string& out, First&& first, Rest&&... rest) {
    const auto args = _NEFORCE forward_as_tuple(_NEFORCE forward<First>(first), _NEFORCE forward<Rest>(rest)...);
    size_t next_seq = 0;
    inner::format_impl(fmt, pos, out, args, next_seq);
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
 *
 * 支持以下格式：
 * - `{}`：顺序参数
 * - `{0}` `{1}`：位置参数
 * - `{:d}` `{:x}`：带格式选项的顺序参数
 * - `{0:d}` `{1:x}`：带格式选项的位置参数
 * - `{{` `}}`：转义花括号
 */
template <typename... Args, enable_if_t<(sizeof...(Args) > 0), int> = 0>
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 string format(const string_view fmt, Args&&... args) {
    string result;
    result.reserve(fmt.size() + sizeof...(Args) * 8);
    const auto args_tuple = _NEFORCE forward_as_tuple(_NEFORCE forward<Args>(args)...);
    size_t next_seq = 0;
    size_t pos = 0;
    inner::format_impl(fmt, pos, result, args_tuple, next_seq);
    return result;
}

/**
 * @brief 格式化字符串
 * @tparam N 格式字符串长度
 * @tparam Args 参数类型
 * @param fmt 格式字符串字面量
 * @param args 要格式化的参数
 * @return 格式化后的字符串
 * @throws value_exception 如果格式错误
 *
 * 在编译期验证格式字符串中花括号的平衡性。
 * 用法：format("Hello, {}!", name)
 */
template <size_t N, typename... Args, enable_if_t<(sizeof...(Args) > 0), int> = 0>
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 string format(const char (&fmt)[N], Args&&... args) {
    return _NEFORCE format(string_view(fmt, N - 1), _NEFORCE forward<Args>(args)...);
}

/**
 * @brief 命名参数格式化
 * @param fmt 格式字符串，使用 {name} 标记命名占位符
 * @param params 命名参数列表
 * @return 格式化后的字符串
 *
 * 将格式字符串中的 {name} 替换为 params 中的对应值。
 * 不支持格式选项，仅做纯文本替换。
 * 用法：format_named("{greeting}, {name}!", {{"greeting", "Hello"}, {"name", "World"}})
 */
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 string
format_named(const string_view fmt, const std::initializer_list<pair<const char*, string_view>> params) {
    string result;
    result.reserve(fmt.size());
    size_t i = 0;

    while (i < fmt.size()) {
        if (fmt[i] == '{' && i + 1 < fmt.size()) {
            if (fmt[i + 1] == '{') {
                result += '{';
                i += 2;
                continue;
            }

            size_t j = i + 1;
            while (j < fmt.size() && fmt[j] != '}') {
                ++j;
            }
            if (j >= fmt.size()) {
                result += fmt[i++];
                continue;
            }

            const string_view name = fmt.view(i + 1, j - (i + 1));
            bool found = false;
            for (const auto& param: params) {
                const string_view key(param.first);
                if (key == name) {
                    result += param.second;
                    found = true;
                    break;
                }
            }
            if (!found) {
                result += fmt.view(i, j - i + 1);
            }
            i = j + 1;
        } else if (fmt[i] == '}' && i + 1 < fmt.size() && fmt[i + 1] == '}') {
            result += '}';
            i += 2;
        } else {
            result += fmt[i++];
        }
    }
    return result;
}

/**
 * @brief locale 感知的格式化
 * @tparam Args 参数类型
 * @param loc locale 对象
 * @param fmt 格式字符串
 * @param args 要格式化的参数
 * @return 格式化后的字符串
 *
 * 格式选项中的 loc 指针将被设置为 &loc，所有 formatter 特化
 * 均可通过 opts.loc 访问 locale 以进行本地化格式化。
 */
template <typename... Args, enable_if_t<(sizeof...(Args) > 0), int> = 0>
NEFORCE_NODISCARD string format(const locale& loc, const string_view fmt, Args&&... args) {
    string result;
    result.reserve(fmt.size() * 2);
    format_options opts;
    opts.loc = &loc;
    format_impl(result, fmt, opts, _NEFORCE forward<Args>(args)...);
    return result;
}

/** @} */ // Format

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_STRING_FORMAT_HPP__
