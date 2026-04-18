#ifndef NEFORCE_CORE_UTILITY_BYTE_SIZE_HPP__
#define NEFORCE_CORE_UTILITY_BYTE_SIZE_HPP__

/**
 * @file byte_size.hpp
 * @brief 字节大小表示和转换工具
 *
 * 此文件提供了字节大小的类型安全表示，支持不同单位之间的转换、
 * 字符串解析和格式化输出。支持二进制和十进制两种标准。
 */

#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/interface/iobject.hpp"
#include "NeForce/core/string/format.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup ByteSize 字节大小
 * @brief 进制安全的字节大小表示实现
 * @{
 */

/**
 * @class byte_size
 * @brief 字节大小类
 *
 * 表示一个字节大小值，支持多种单位之间的转换和运算。
 * 提供类型安全的字节大小操作，避免原始整数带来的单位混淆问题。
 *
 * 主要功能：
 * - 字节大小的单位表示
 * - 从字符串解析字节大小
 * - 格式化为可读字符串
 * - 单位之间的算术运算
 * - 支持二进制（IEC标准）和十进制（SI标准）
 * - 哈希支持
 *
 * 使用示例：
 * @code
 * // 从字节数构造
 * byte_size size1(1024);
 * println(size1);  // "1 KB"
 *
 * // 从值和单位构造
 * byte_size size2(decimal_t(1.5), byte_size::unit::MB);
 * println(size2.bytes());  // 1572864
 *
 * // 从字符串解析
 * auto size3 = byte_size::parse("2.5 GB");
 * auto size4 = byte_size::parse("1 MiB", true);  // 二进制模式
 *
 * // 算术运算
 * byte_size total = size1 + size2;
 * byte_size half = total / 2;
 *
 * // 单位转换
 * double in_mb = size2.get_as(byte_size::unit::MB);
 *
 * // 格式化输出
 * println(size2.to_string(byte_size::unit::KB, 0));  // "1536 KB"
 * @endcode
 */
class NEFORCE_API byte_size : public iobject<byte_size>, public icommon<byte_size> {
public:
    /**
     * @enum unit
     * @brief 字节大小单位枚举
     */
    enum class unit {
        AUTO, ///< 自动选择合适单位
        B,    ///< 字节
        KB,   ///< 千字节
        MB,   ///< 兆字节
        GB,   ///< 吉字节
        TB,   ///< 太字节
        PB,   ///< 拍字节
        EB    ///< 艾字节
    };

private:
    uint64_t bytes_{0}; ///< 存储的字节数

public:
    /**
     * @brief 默认构造函数
     *
     * 创建大小为0的字节对象。
     */
    constexpr byte_size() = default;

    /**
     * @brief 从字节数构造
     * @param bytes 字节数
     */
    constexpr explicit byte_size(uint64_t bytes) :
    bytes_(bytes) {}

    /**
     * @brief 从值和单位构造
     * @param value 数值
     * @param u 单位
     * @param binary 是否使用二进制，默认true
     * @throws value_exception 值为负数或超出范围时抛出
     */
    byte_size(decimal_t value, unit u, bool binary = true);

    /**
     * @brief 从字符串解析字节大小
     * @param str 字符串
     * @return 字节大小对象
     * @throws value_exception 解析失败时抛出
     *
     * 支持的单位：B、KB/K、MB/M、GB/G、TB/T、PB/P、EB/E
     * 示例："1024", "1.5 MB", "2G", "500KB"
     */
    NEFORCE_NODISCARD static byte_size parse(string_view str) { return parse(str, true); }

    /**
     * @brief 从字符串解析字节大小（指定进制）
     * @param str 字符串
     * @param binary 是否使用二进制
     * @return 字节大小对象
     * @throws value_exception 解析失败时抛出
     */
    NEFORCE_NODISCARD static byte_size parse(string_view str, bool binary);

    /**
     * @brief 获取字节数
     * @return 原始字节数
     */
    NEFORCE_NODISCARD constexpr uint64_t bytes() const { return bytes_; }

    /**
     * @brief 转换为指定单位的值
     * @param u 目标单位
     * @param binary 是否使用二进制
     * @return 转换后的浮点值
     * @throws value_exception unit为AUTO时抛出
     */
    NEFORCE_NODISCARD decimal_t as(unit u, bool binary = true) const;

    /**
     * @brief 转换为可读字符串
     * @return 格式化字符串
     */
    NEFORCE_NODISCARD string to_string() const { return to_string(unit::AUTO, 2, true); }

    /**
     * @brief 转换为指定单位的字符串
     * @param u 目标单位
     * @param precision 小数精度
     * @param binary 是否使用二进制
     * @return 格式化字符串
     */
    NEFORCE_NODISCARD string to_string(unit u, int precision = 2, bool binary = true) const;

    /**
     * @brief 检查是否为零
     * @return 如果字节数为0返回true
     */
    NEFORCE_NODISCARD constexpr bool is_zero() const noexcept { return bytes_ == 0; }

    byte_size& operator+=(const byte_size& rhs) {
        bytes_ += rhs.bytes_;
        return *this;
    }

    byte_size& operator-=(const byte_size& rhs) {
        if (bytes_ < rhs.bytes_) {
            NEFORCE_THROW_EXCEPTION(value_exception("Memory size subtraction underflow"));
        }
        bytes_ -= rhs.bytes_;
        return *this;
    }

    byte_size& operator*=(uint64_t factor) {
        bytes_ *= factor;
        return *this;
    }

    byte_size& operator/=(uint64_t divisor) {
        if (divisor == 0) {
            NEFORCE_THROW_EXCEPTION(value_exception("Division by zero"));
        }
        bytes_ /= divisor;
        return *this;
    }

    NEFORCE_NODISCARD constexpr byte_size operator+(const byte_size& rhs) const noexcept {
        return byte_size(bytes_ + rhs.bytes_);
    }

    NEFORCE_NODISCARD byte_size operator-(const byte_size& rhs) const {
        if (bytes_ < rhs.bytes_) {
            NEFORCE_THROW_EXCEPTION(value_exception("Memory size subtraction underflow"));
        }
        return byte_size(bytes_ - rhs.bytes_);
    }

    NEFORCE_NODISCARD byte_size operator*(uint64_t factor) const {
        if (factor > 0 && bytes_ > numeric_traits<uint64_t>::max() / factor) {
            NEFORCE_THROW_EXCEPTION(value_exception("Memory size multiplication overflow"));
        }
        return byte_size(bytes_ * factor);
    }

    NEFORCE_NODISCARD byte_size operator/(uint64_t divisor) const {
        if (divisor == 0) {
            NEFORCE_THROW_EXCEPTION(value_exception("Division by zero"));
        }
        return byte_size(bytes_ / divisor);
    }

    NEFORCE_NODISCARD friend byte_size operator*(uint64_t factor, const byte_size& size) { return size * factor; }

    NEFORCE_NODISCARD bool operator==(const byte_size& rhs) const { return bytes_ == rhs.bytes_; }
    NEFORCE_NODISCARD bool operator<(const byte_size& rhs) const { return bytes_ < rhs.bytes_; }

    /**
     * @brief 计算哈希值
     * @return 哈希值
     */
    NEFORCE_NODISCARD size_t to_hash() const noexcept { return hash<uint64_t>()(bytes_); }
};

template <>
struct unpackage<byte_size> {
    using type = uint64_t;
};

/** @} */ // ByteSize

NEFORCE_BEGIN_LITERALS__

/**
 * @defgroup UserLiterals 字面量
 * @brief 用户定义字面量支持
 * @{
 */

/**
 * @brief 创建byte_size的字面量操作符
 * @param bytes 字节数
 * @return byte_size对象
 */
NEFORCE_NODISCARD inline byte_size operator""_B(const uint64_t bytes) noexcept { return byte_size{bytes}; }

/**
 * @brief 创建byte_size的字面量操作符
 * @param bytes 字节数
 * @return byte_size对象
 */
NEFORCE_NODISCARD inline byte_size operator""_B(const decimal_t bytes) noexcept {
    return byte_size{bytes, byte_size::unit::B};
}

/**
 * @brief 创建byte_size的字面量操作符
 * @param bytes 字节数
 * @return byte_size对象
 */
NEFORCE_NODISCARD inline byte_size operator""_KB(const uint64_t bytes) noexcept {
    return byte_size{static_cast<decimal_t>(bytes), byte_size::unit::KB};
}

/**
 * @brief 创建byte_size的字面量操作符
 * @param bytes 字节数
 * @return byte_size对象
 */
NEFORCE_NODISCARD inline byte_size operator""_KB(const decimal_t bytes) noexcept {
    return byte_size{bytes, byte_size::unit::KB};
}

/**
 * @brief 创建byte_size的字面量操作符
 * @param bytes 字节数
 * @return byte_size对象
 */
NEFORCE_NODISCARD inline byte_size operator""_MB(const uint64_t bytes) noexcept {
    return byte_size{static_cast<decimal_t>(bytes), byte_size::unit::MB};
}

/**
 * @brief 创建byte_size的字面量操作符
 * @param bytes 字节数
 * @return byte_size对象
 */
NEFORCE_NODISCARD inline byte_size operator""_MB(const decimal_t bytes) noexcept {
    return byte_size{bytes, byte_size::unit::MB};
}

/**
 * @brief 创建byte_size的字面量操作符
 * @param bytes 字节数
 * @return byte_size对象
 */
NEFORCE_NODISCARD inline byte_size operator""_GB(const uint64_t bytes) noexcept {
    return byte_size{static_cast<decimal_t>(bytes), byte_size::unit::GB};
}

/**
 * @brief 创建byte_size的字面量操作符
 * @param bytes 字节数
 * @return byte_size对象
 */
NEFORCE_NODISCARD inline byte_size operator""_GB(const decimal_t bytes) noexcept {
    return byte_size{bytes, byte_size::unit::GB};
}

/**
 * @brief 创建byte_size的字面量操作符
 * @param bytes 字节数
 * @return byte_size对象
 */
NEFORCE_NODISCARD inline byte_size operator""_TB(const uint64_t bytes) noexcept {
    return byte_size{static_cast<decimal_t>(bytes), byte_size::unit::TB};
}

/**
 * @brief 创建byte_size的字面量操作符
 * @param bytes 字节数
 * @return byte_size对象
 */
NEFORCE_NODISCARD inline byte_size operator""_TB(const decimal_t bytes) noexcept {
    return byte_size{bytes, byte_size::unit::TB};
}

/**
 * @brief 创建byte_size的字面量操作符
 * @param bytes 字节数
 * @return byte_size对象
 */
NEFORCE_NODISCARD inline byte_size operator""_PB(const uint64_t bytes) noexcept {
    return byte_size{static_cast<decimal_t>(bytes), byte_size::unit::PB};
}

/**
 * @brief 创建byte_size的字面量操作符
 * @param bytes 字节数
 * @return byte_size对象
 */
NEFORCE_NODISCARD inline byte_size operator""_PB(const decimal_t bytes) noexcept {
    return byte_size{bytes, byte_size::unit::PB};
}

/**
 * @brief 创建byte_size的字面量操作符
 * @param bytes 字节数
 * @return byte_size对象
 */
NEFORCE_NODISCARD inline byte_size operator""_EB(const uint64_t bytes) noexcept {
    return byte_size{static_cast<decimal_t>(bytes), byte_size::unit::EB};
}

/**
 * @brief 创建byte_size的字面量操作符
 * @param bytes 字节数
 * @return byte_size对象
 */
NEFORCE_NODISCARD inline byte_size operator""_EB(const decimal_t bytes) noexcept {
    return byte_size{bytes, byte_size::unit::EB};
}

/** @} */ // UserLiterals

NEFORCE_END_LITERALS__

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_UTILITY_BYTE_SIZE_HPP__
