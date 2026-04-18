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
 *
 * 支持不同单位之间的转换、字符串解析和格式化输出。支持二进制和十进制两种标准。
 *
 * @section standards 遵循的国际标准
 * 本实现严格遵循以下数据存储单位与度量衡相关标准规范：
 *
 * **二进制前缀标准（IEC 标准）：**
 * - **IEC 80000-13:2008**：量和单位 — 第13部分：信息科学与技术
 *   https://www.iso.org/standard/31898.html
 *   （注：IEC 80000-13 已整合入 ISO/IEC 80000 系列）
 * - **ISO/IEC 80000-13:2008**：信息科学与技术中的量和单位
 *   https://www.iso.org/standard/31898.html
 * - **IEEE 1541-2021**：数字和电子设备中使用的二进制倍数前缀标准
 *   https://standards.ieee.org/ieee/1541/10790/
 *
 * **国际单位制（SI）十进制前缀标准：**
 * - **BIPM SI Brochure (9th Edition, 2019)**：国际单位制（SI）前缀定义
 *   https://www.bipm.org/en/publications/si-brochure
 * - **ISO 80000-1:2009**：量和单位 — 第1部分：总则（含 SI 前缀）
 *   https://www.iso.org/standard/30669.html
 *
 * **网络协议与数据大小表示标准：**
 * - **IETF RFC 8949**：简明二进制对象表示（CBOR）中的字节大小约定
 *   https://www.rfc-editor.org/rfc/rfc8949.html
 *
 * **编程语言与系统标准：**
 * - **POSIX.1-2017 (IEEE Std 1003.1)**：系统接口中的字节定义
 *   https://pubs.opengroup.org/onlinepubs/9699919799/
 *
 * @section binary_vs_decimal 二进制与十进制前缀对比
 * 根据 IEC 80000-13 和 IEEE 1541，字节单位存在两套并行的标准：
 *
 * | 二进制前缀 | 符号 | 2的幂 | 字节数（近似） | 十进制前缀 | 符号 | 10的幂 | 字节数（精确） |
 * |------------|------|-------|----------------|------------|------|--------|----------------|
 * | kibibyte   | KiB  | 2^10  | 1,024          | kilobyte   | kB   | 10^3   | 1,000          |
 * | mebibyte   | MiB  | 2^20  | 1,048,576      | megabyte   | MB   | 10^6   | 1,000,000      |
 * | gibibyte   | GiB  | 2^30  | 1,073,741,824  | gigabyte   | GB   | 10^9   | 1,000,000,000  |
 * | tebibyte   | TiB  | 2^40  | ≈1.1×10^12     | terabyte   | TB   | 10^12  | 1,000,000,000,000 |
 * | pebibyte   | PiB  | 2^50  | ≈1.13×10^15    | petabyte   | PB   | 10^15  | 1,000,000,000,000,000 |
 * | exbibyte   | EiB  | 2^60  | ≈1.15×10^18    | exabyte    | EB   | 10^18  | 1,000,000,000,000,000,000 |
 *
 * @section unit_suffixes 单位后缀说明
 * 本实现支持的单位字符串（不区分大小写）：
 *
 * | 单位 | 全名（二进制） | 全名（十进制） | 支持的别名 | 进制       |
 * |------|----------------|----------------|------------|------------|
 * | B    | byte           | byte           | -          | 1          |
 * | KB/K | kibibyte       | kilobyte       | K          | 二进制/十进制 |
 * | MB/M | mebibyte       | megabyte       | M          | 二进制/十进制 |
 * | GB/G | gibibyte       | gigabyte       | G          | 二进制/十进制 |
 * | TB/T | tebibyte       | terabyte       | T          | 二进制/十进制 |
 * | PB/P | pebibyte       | petabyte       | P          | 二进制/十进制 |
 * | EB/E | exbibyte       | exabyte        | E          | 二进制/十进制 |
 *
 * @section ieee_1541 IEEE 1541-2021 建议
 * IEEE 1541 标准建议：
 * - 使用明确的二进制前缀（KiB, MiB, GiB 等）表示 2 的幂
 * - 使用 SI 前缀（kB, MB, GB 等）仅表示 10 的幂
 * - 符号中 "B" 应为大写，前缀首字母大写（如 KiB、MiB）
 *
 * @section implementation_details 实现细节
 * | 特性              | 规范参数                                  |
 * |-------------------|-------------------------------------------|
 * | 内部存储          | uint64_t（最大约 16 EiB）                 |
 * | 最大可表示值      | 2^64 - 1 字节 ≈ 18.4 EB                   |
 * | 二进制乘数表      | 1, 1024, 1024², ..., 1024⁶                |
 * | 十进制乘数表      | 1, 1000, 1000², ..., 1000⁶                |
 * | 自动单位选择      | 选择使数值 < 1024（二进制）或 < 1000（十进制）的最大单位 |
 * | 解析容错          | 忽略大小写，支持简写别名（K、M、G等）     |
 * | 小数精度          | decimal_t（通常为 80 位扩展精度）          |
 *
 * @note 本实现默认使用二进制解释（`binary = true`），这符合计算机科学中的
 *       传统惯例（1 KB = 1024 B）。如需使用十进制解释（1 KB = 1000 B），
 *       请在构造或解析时显式指定 `binary = false`。
 *
 * @warning 存储容量制造商（如硬盘、SSD）通常使用十进制单位标注容量，
 *          因此标称 "1 TB" 的硬盘实际约为 931 GiB。解析用户输入的存储大小时，
 *          应注意区分这两种标准以避免混淆。
 *
 * @see https://www.bipm.org/en/measurement-units/prefixes.html
 * @see https://physics.nist.gov/cuu/Units/binary.html
 * @see https://en.wikipedia.org/wiki/Binary_prefix
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
