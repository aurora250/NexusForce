#ifndef MSTL_CORE_STRING_FORMAT_HPP__
#define MSTL_CORE_STRING_FORMAT_HPP__
#include "to_string.hpp"
MSTL_BEGIN_NAMESPACE__

enum class FORMAT_ALIGN {
    LEFT        = '<',
    RIGHT       = '>',
    INTERNAL    = '=',
    CENTER      = '^'
};

enum class FORMAT_TYPE {
    BINARY      = 'b',
    OCTAL       = 'o',
    DECIMAL     = 'd',
    HEX_LOW     = 'x',
    HEX_UP      = 'X',
    FLOAT_FIX   = 'f',
    FLOAT_EXP   = 'e',
    FLOAT_GEN   = 'g'
};

struct MSTL_API format_options {
    char fill = ' ';
    int width = 0;
    int precision = -1;
    FORMAT_ALIGN alignment = FORMAT_ALIGN::RIGHT;
    char sign_mode = 0;  // 0: default, '+': always show, '-': only nega, ' ': posi space
    bool show_base = false;
    FORMAT_TYPE type = FORMAT_TYPE::DECIMAL;
    bool zero_pad = false;
};


MSTL_BEGIN_INNER__

constexpr FORMAT_ALIGN to_number_alignment(const char c) {
    switch (c) {
        case '<': return FORMAT_ALIGN::LEFT;
        case '>': return FORMAT_ALIGN::RIGHT;
        case '^': return FORMAT_ALIGN::CENTER;
        case '=': return FORMAT_ALIGN::INTERNAL;
        default:  return FORMAT_ALIGN::RIGHT;
    }
}

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

MSTL_CONSTEXPR20 format_options parse_number_format(const string_view& fmt_str) {
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

    if (!found_align && pos < fmt_str.size()) {
        const char c = fmt_str[pos];
        if (c == '<' || c == '>' || c == '^' || c == '=') {
            options.alignment = to_number_alignment(c);
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

    if (pos < fmt_str.size() && _MSTL is_digit(fmt_str[pos])) {
        int width = 0;
        while (pos < fmt_str.size() && _MSTL is_digit(fmt_str[pos])) {
            width = width * 10 + (fmt_str[pos] - '0');
            ++pos;
        }
        options.width = width;
    }

    if (pos < fmt_str.size() && fmt_str[pos] == '.') {
        ++pos;
        int precision = 0;
        while (pos < fmt_str.size() && _MSTL is_digit(fmt_str[pos])) {
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

MSTL_END_INNER__


template <typename Number, typename = void>
struct formatter;


template <typename T>
struct formatter<T, enable_if_t<is_floating_point_v<T>>> {
    MSTL_CONSTEXPR20 string operator ()(const T& value, const format_options& options) const {
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
                number_str = _MSTL to_string_fixed(val, precision);
                break;
            }
            case FORMAT_TYPE::FLOAT_EXP: {
                number_str = _MSTL to_string_scientific(val, precision);
                break;
            }
            case FORMAT_TYPE::FLOAT_GEN:
            default: {
                number_str = _MSTL to_string_general(val, precision);
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
            case FORMAT_ALIGN::LEFT:
                return result + fill_str;
            case FORMAT_ALIGN::RIGHT:
                return fill_str + result;
            case FORMAT_ALIGN::INTERNAL:
                return prefix + fill_str + number_str;
            case FORMAT_ALIGN::CENTER: {
                const size_t left_fill = fill_count / 2;
                const size_t right_fill = fill_count - left_fill;
                return string(left_fill, options.fill) + result + string(right_fill, options.fill);
            }
            default:
                return fill_str + result;
        }
    }
};

template <typename T>
struct formatter<T, enable_if_t<is_integral_v<T> && is_signed_v<T>>> {
    MSTL_CONSTEXPR20 string operator ()(const T& value, const format_options& options) const {
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
            prefix += "-";
        } else if (options.sign_mode == '+') {
            prefix += "+";
        } else if (options.sign_mode == ' ') {
            prefix += " ";
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
            case FORMAT_ALIGN::LEFT:
                return result + fill_str;
            case FORMAT_ALIGN::RIGHT:
                return fill_str + result;
            case FORMAT_ALIGN::INTERNAL:
                return prefix + fill_str + number_str;
            case FORMAT_ALIGN::CENTER: {
                const size_t left_fill = fill_count / 2;
                const size_t right_fill = fill_count - left_fill;
                return string(left_fill, options.fill) + result + string(right_fill, options.fill);
            }
            default:
                return fill_str + result;
        }
    }
};

template <typename T>
struct formatter<T, enable_if_t<is_integral_v<T> && is_unsigned_v<T>>> {
    MSTL_CONSTEXPR20 string operator ()(const T& value, const format_options& options) const {
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
            digits = _INNER __uint_to_string<char>(static_cast<const unpackage_t<T>&>(value));
        } else {
            digits = _INNER __uint_to_string_base(static_cast<const unpackage_t<T>&>(value), base, uppercase);
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
            case FORMAT_ALIGN::LEFT:
                return number_str + string(fill_count, options.fill);
            case FORMAT_ALIGN::RIGHT:
                return string(fill_count, options.fill) + number_str;
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
            default:
                return string(fill_count, options.fill) + number_str;
        }
    }
};

template <>
struct formatter<string> {
    MSTL_CONSTEXPR20 string operator ()(const string& value, const format_options& options) const {
        if (options.width <= 0 || value.size() >= static_cast<size_t>(options.width)) {
            return value;
        }

        const size_t fill_count = options.width - value.size();
        const string fill_str(fill_count, options.fill);

        switch (options.alignment) {
            case FORMAT_ALIGN::LEFT:
                return value + fill_str;
            case FORMAT_ALIGN::RIGHT:
                return fill_str + value;
            case FORMAT_ALIGN::CENTER: {
                const size_t left_fill = fill_count / 2;
                const size_t right_fill = fill_count - left_fill;
                return string(left_fill, options.fill) + value + string(right_fill, options.fill);
            }
            case FORMAT_ALIGN::INTERNAL:
                default:
                    return fill_str + value;
        }
    }
};

template <>
struct formatter<const char*> {
    MSTL_CONSTEXPR20 string operator ()(const char* value, const format_options& options) const {
        return formatter<string>()(string(value), options);
    }
};

template <>
struct formatter<string_view> {
    MSTL_CONSTEXPR20 string operator ()(const string_view value, const format_options& options) const {
        return formatter<string>()(string(value), options);
    }
};

template <>
struct formatter<char*> {
    MSTL_CONSTEXPR20 string operator ()(char* value, const format_options& options) const {
        return formatter<string>()(string(value), options);
    }
};

template <>
struct formatter<char> {
    MSTL_CONSTEXPR20 string operator ()(const char value, const format_options& options) const {
        return formatter<string>()(string(1, value), options);
    }
};


MSTL_BEGIN_INNER__
MSTL_CONSTEXPR20 string format_impl(const string_view fmt, size_t& pos) {
    string result;
    while (pos < fmt.size()) {
        if (fmt[pos] == '{') {
            if (pos + 1 < fmt.size() && fmt[pos + 1] == '{') {
                result += '{';
                pos += 2;
            } else {
                throw_exception(value_exception("Not enough arguments for format string"));
            }
        } else if (fmt[pos] == '}') {
            if (pos + 1 < fmt.size() && fmt[pos + 1] == '}') {
                result += '}';
                pos += 2;
            } else {
                throw_exception(value_exception("Unmatched '}' in format string"));
            }
        } else {
            result += fmt[pos];
            ++pos;
        }
    }
    return result;
}

template <typename First, typename... Rest>
MSTL_CONSTEXPR20 string format_impl(const string_view fmt, size_t& pos, First&& first, Rest&&... rest) {
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
                throw_exception(value_exception("Unmatched '{' in format string"));
            }

            const string_view spec_str = fmt.substr(pos, end_pos - pos);
            pos = end_pos + 1;
            format_options opts;
            if (spec_str.empty()) {
                opts = _INNER parse_number_format("");
            } else if (spec_str[0] == ':') {
                opts = _INNER parse_number_format(spec_str.substr(1));
            } else {
                opts = _INNER parse_number_format(spec_str);
            }
            result += formatter<decay_t<First>>()(_MSTL forward<First>(first), opts);
            result += _INNER format_impl(fmt, pos, _MSTL forward<Rest>(rest)...);
            return result;
        } else if (fmt[pos] == '}') {
            if (pos + 1 < fmt.size() && fmt[pos + 1] == '}') {
                result += '}';
                pos += 2;
            } else {
                throw_exception(value_exception("Unmatched '}' in format string"));
            }
        } else {
            result += fmt[pos];
            ++pos;
        }
    }
    return result;
}
MSTL_END_INNER__


template <typename... Args, enable_if_t<(sizeof...(Args) > 0), int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 string format(const string_view fmt, Args&&... args) {
    size_t pos = 0;
    return _INNER format_impl(fmt, pos, _MSTL forward<Args>(args)...);
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_STRING_FORMAT_HPP__
