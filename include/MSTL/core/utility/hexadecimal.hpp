#ifndef MSTL_CORE_UTILITY_HEXADECIMAL_HPP__
#define MSTL_CORE_UTILITY_HEXADECIMAL_HPP__

/**
 * @file hexadecimal.hpp
 * @brief 十六进制数值包装类
 *
 * 此文件提供了十六进制数值的包装类，
 * 支持十六进制字符串的解析、位操作、格式化输出等功能。
 */

#include "MSTL/core/interface/iobject.hpp"
#include "MSTL/core/string/format.hpp"
#include "MSTL/core/string/to_numerics.hpp"
MSTL_BEGIN_NAMESPACE__

/**
 * @defgroup Packages 数值包装
 * @brief 数值类型的包装类集合
 * @{
 */

/**
 * @struct hexadecimal
 * @brief 十六进制数值包装类
 *
 * 提供十六进制数值的包装，支持从字符串解析、位操作、格式化等功能。
 * 内部使用int64_t存储，支持64位以内的十六进制值。
 */
struct MSTL_API hexadecimal : iobject<hexadecimal>, ipackage<hexadecimal, int64_t> {
public:
    using value_type = int64_t;  ///< 值类型
    using base = ipackage;       ///< 基类类型

private:
    /**
     * @brief 从字符串视图解析十六进制值
     * @param view 要解析的字符串视图
     * @return 解析后的十六进制值
     * @throws value_exception 解析失败时抛出
     *
     * 支持的格式：可选的符号（+/-），可选的0x/0X前缀，
     * 十六进制数字（0-9, a-f, A-F），忽略前导和尾随空格。
     */
    static MSTL_CONSTEXPR20 value_type parse_view(const string_view view) {
        if (view.empty()) return 0;

        bool negative = false;
        size_t start = 0;

        while (start < view.size() && _MSTL is_space(view[start])) ++start;
        if (start == view.size()) return 0;

        if (view[start] == '-') {
            negative = true;
            ++start;
        } else if (view[start] == '+') {
            ++start;
        }

        if (start + 1 < view.size() && view[start] == '0' &&
            (view[start+1] == 'x' || view[start+1] == 'X')) {
            start += 2;
        }

        string hex_digits;
        while (start < view.size()) {
            const char c = view[start++];
            if (_MSTL is_xdigit(c)) {
                hex_digits += c;
            } else if (!_MSTL is_space(c)) {
                throw_exception(value_exception("Invalid hexadecimal character"));
            }
        }

        if (hex_digits.empty()) return 0;

        size_t pos = 0;
        const uint64_t raw = _MSTL to_uint64(hex_digits.view(), &pos, 16);
        if (pos != hex_digits.size()) {
            throw_exception(value_exception("Invalid hexadecimal format"));
        }

        if (negative) {
            if (raw > static_cast<uint64_t>(numeric_traits<int64_t>::max()) + 1) {
                throw_exception(value_exception("Hexadecimal value out of range"));
            }
            return -static_cast<int64_t>(raw);
        }
        if (raw > static_cast<uint64_t>(numeric_traits<int64_t>::max())) {
            throw_exception(value_exception("Hexadecimal value out of range"));
        }
        return static_cast<value_type>(raw);
    }

public:
    /**
     * @brief 从16位有符号整数构造
     * @param value 整数值
     */
    explicit constexpr hexadecimal(const int16_t value) noexcept
    : base(value) {}

    /**
     * @brief 从32位有符号整数构造
     * @param value 整数值
     */
    explicit constexpr hexadecimal(const int32_t value) noexcept
    : base(value) {}

    /**
     * @brief 从16位无符号整数构造
     * @param value 整数值
     */
    explicit constexpr hexadecimal(const uint16_t value) noexcept
    : base(value) {}

    /**
     * @brief 从32位无符号整数构造
     * @param value 整数值
     */
    explicit constexpr hexadecimal(const uint32_t value) noexcept
    : base(value) {}

    /**
     * @brief 从64位无符号整数构造
     * @param value 整数值
     */
    explicit constexpr hexadecimal(const uint64_t value) noexcept
    : base(value) {}

    /**
     * @brief 从字符串视图构造
     * @param view 十六进制字符串视图
     * @throws value_exception 解析失败时抛出
     */
    MSTL_CONSTEXPR20 explicit hexadecimal(const string_view view)
    : base(parse_view(view)) {}

    /**
     * @brief 从C风格字符串构造
     * @param str 十六进制字符串
     * @throws value_exception 解析失败时抛出
     */
    MSTL_CONSTEXPR20 explicit hexadecimal(const char* str)
    : hexadecimal(string_view(str)) {}

    /**
     * @brief 从字符串对象构造
     * @param str 十六进制字符串
     * @throws value_exception 解析失败时抛出
     */
    MSTL_CONSTEXPR20 explicit hexadecimal(const string& str)
    : hexadecimal(str.view()) {}

    MSTL_BUILD_PACKAGE_CONSTRUCTOR(hexadecimal)

    /**
     * @brief 转换为bool操作符
     * @return 值是否非零
     */
    MSTL_NODISCARD explicit constexpr operator bool() const noexcept {
        return value_ != _MSTL initialize<value_type>();
    }

    /**
     * @brief 获取指定位的值
     * @param position 位位置（0-63）
     * @return 该位的值（0或1）
     * @throws value_exception 位置超出范围时抛出
     */
    MSTL_NODISCARD constexpr bool get_bit(const size_t position) const {
        if (position >= 64) {
            throw_exception(value_exception("Bit position out of range"));
        }
        return (value_ >> position) & 1;
    }

    /**
     * @brief 设置指定位的值
     * @param position 位位置（0-63）
     * @param bit_value_ 要设置的值（true为1，false为0）
     * @return 自身引用
     * @throws value_exception 位置超出范围时抛出
     */
    constexpr hexadecimal& set_bit(const size_t position, const bool bit_value_ = true) {
        if (position >= 64) {
            throw_exception(value_exception("Bit position out of range"));
        }
        if (bit_value_) {
            value_ |= (1ULL << position);
        } else {
            value_ &= ~(1ULL << position);
        }
        return *this;
    }

    /**
     * @brief 翻转指定位
     * @param position 位位置（0-63）
     * @return 自身引用
     * @throws value_exception 位置超出范围时抛出
     */
    constexpr hexadecimal& flip_bit(const size_t position) {
        if (position >= 64) {
            throw_exception(value_exception("Bit position out of range"));
        }
        value_ ^= (1ULL << position);
        return *this;
    }

    /**
     * @brief 转换为字符串
     * @return 十六进制格式的字符串（带0x前缀）
     */
    MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string() const;

    /**
     * @brief 从字符串解析十六进制值
     * @param str 要解析的字符串
     * @return 解析得到的十六进制对象
     * @throws value_exception 解析失败时抛出
     */
    MSTL_NODISCARD static MSTL_CONSTEXPR20 hexadecimal parse(const string_view str) {
        return hexadecimal(str);
    }
};

template <>
struct unpackage<hexadecimal> {
    using type = int64_t;
};


template <>
struct formatter<hexadecimal> {
    MSTL_CONSTEXPR20 string operator ()(const hexadecimal& value, const format_options& options) const {
        return formatter<int64_t>()(value.value(), options);
    }
};

MSTL_CONSTEXPR20 string hexadecimal::to_string() const {
    return _MSTL format("{#x}", *this);
}

/** @} */ // Packages

MSTL_BEGIN_LITERALS__

/**
 * @defgroup UserLiterals 字面量
 * @brief 用户定义字面量支持
 * @{
 */

/**
 * @brief 字符串字面量转十六进制
 * @param str 字符串
 * @param len 长度
 * @return 解析得到的十六进制对象
 */
MSTL_CONSTEXPR20 hexadecimal operator ""_hex(const char* str, const size_t len) {
    return hexadecimal{string_view(str, len)};
}

/**
 * @brief 整数字面量转十六进制
 * @param value 整数值
 * @return 十六进制对象
 */
constexpr hexadecimal operator ""_hex(const unsigned long long value) {
    return hexadecimal{static_cast<int64_t>(value)};
}

/** @} */ // UserLiterals

MSTL_END_LITERALS__

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_UTILITY_HEXADECIMAL_HPP__
