#ifndef NEFORCE_CORE_NUMERIC_INT128_HPP__
#define NEFORCE_CORE_NUMERIC_INT128_HPP__

/**
 * @file int128.hpp
 * @brief 128位整数类型实现
 *
 * 此文件提供了128位有符号和无符号整数类型的完整实现。
 * 支持所有基本算术运算、位运算、比较运算、字符串转换等功能。
 *
 * 主要功能：
 * - 128位无符号整数与有符号整数
 * - 所有算术运算符重载
 * - 位运算支持
 * - 字符串解析和格式化
 * - 与内置整数类型的隐式转换
 * - 数值特性
 * - 字面量支持
 */

#include "NeForce/core/string/to_numerics.hpp"
#include "NeForce/core/string/to_string.hpp"
#include "NeForce/core/interface/iobject.hpp"
#include "NeForce/core/interface/inumeric.hpp"
NEFORCE_BEGIN_NAMESPACE__

struct NEFORCE_API int128_t;

/**
 * @defgroup Int128 128位整数
 * @brief 128位有符号和无符号整数类型的完整实现
 * @{
 */

/**
 * @struct uint128_t
 * @brief 128位无符号整数类型
 */
struct NEFORCE_API uint128_t : icommon<uint128_t>, iarithmetic<uint128_t>, ibinary<uint128_t>, iobject<uint128_t> {
public:
    uint64_t lo{0}; ///< 低64位
    uint64_t hi{0}; ///< 高64位

    constexpr uint128_t() noexcept = default;
    NEFORCE_CONSTEXPR20 ~uint128_t() = default;

    /**
     * @brief 从32位有符号整数构造
     * @param low 低32位值
     */
    constexpr uint128_t(const int32_t low) noexcept :
    lo(static_cast<uint64_t>(static_cast<int64_t>(low))),
    hi(low < 0 ? ~static_cast<uint64_t>(0) : 0) {}

    /**
     * @brief 从32位无符号整数构造
     * @param low 低32位值
     */
    constexpr uint128_t(const uint32_t low) noexcept :
    lo(low) {}

    /**
     * @brief 从无符号 long 构造
     * @param low 无符号 long 值
     */
    constexpr uint128_t(const unsigned long low) noexcept :
    lo(low) {}

    /**
     * @brief 从无符号 long long 构造
     * @param low 无符号 long long 值
     */
    constexpr uint128_t(const unsigned long long low) noexcept :
    lo(low) {}

    /**
     * @brief 从高低64位构造
     * @param high 高64位
     * @param low 低64位
     */
    constexpr uint128_t(const uint64_t high, const uint64_t low) noexcept :
    lo(low),
    hi(high) {}

    constexpr uint128_t(const uint128_t&) noexcept = default;
    constexpr uint128_t& operator=(const uint128_t&) noexcept = default;
    constexpr uint128_t(uint128_t&&) noexcept = default;
    constexpr uint128_t& operator=(uint128_t&&) noexcept = default;

    /**
     * @brief 从字符串构造
     * @param str 数字字符串
     * @param base 进制基数（默认10）
     * @throws typecast_exception 字符串格式无效时抛出
     */
    explicit NEFORCE_CONSTEXPR20 uint128_t(const string& str, int base = 10) :
    uint128_t(str.view(), base) {}

    /**
     * @brief 从字符串视图构造
     * @param str 数字字符串视图
     * @param base 进制基数（默认10）
     * @throws typecast_exception 字符串格式无效时抛出
     */
    explicit constexpr uint128_t(string_view str, int base = 10);

    /**
     * @brief 转换为有符号128位整数
     * @return 有符号128位整数
     */
    NEFORCE_NODISCARD constexpr int128_t to_int128() const noexcept;

    explicit constexpr operator bool() const noexcept { return lo || hi; }
    explicit constexpr operator char() const noexcept { return static_cast<char>(lo); }
    explicit constexpr operator int8_t() const noexcept { return static_cast<int8_t>(lo); }
    explicit constexpr operator uint8_t() const noexcept { return static_cast<uint32_t>(lo); }
    explicit constexpr operator uint16_t() const noexcept { return static_cast<uint32_t>(lo); }
    explicit constexpr operator uint32_t() const noexcept { return static_cast<uint32_t>(lo); }
    explicit constexpr operator uint64_t() const noexcept { return lo; }
    explicit constexpr operator int128_t() const noexcept;

    /**
     * @brief 相等比较
     */
    NEFORCE_NODISCARD constexpr bool equal_to(const uint128_t& rhs) const noexcept {
        return hi == rhs.hi && lo == rhs.lo;
    }

    /**
     * @brief 小于比较
     */
    NEFORCE_NODISCARD constexpr bool less_than(const uint128_t& rhs) const noexcept {
        return hi < rhs.hi || (hi == rhs.hi && lo < rhs.lo);
    }

    /**
     * @brief 取负
     * @return 二补数结果
     */
    constexpr uint128_t operator-() const noexcept {
        const uint64_t new_lo = ~lo + 1ULL;
        const uint64_t new_hi = ~hi + (lo == 0ULL ? 1ULL : 0ULL);
        return {new_hi, new_lo};
    }

    /**
     * @brief 加法赋值
     */
    uint128_t& operator+=(const uint128_t& other) noexcept;

    /**
     * @brief 减法赋值
     */
    uint128_t& operator-=(const uint128_t& other) noexcept;

    /**
     * @brief 乘法赋值
     */
    uint128_t& operator*=(const uint128_t& other) noexcept;

    /**
     * @brief 除法赋值
     * @throws math_exception 除数为0时抛出
     */
    uint128_t& operator/=(const uint128_t& other);

    /**
     * @brief 取模赋值
     * @throws math_exception 除数为0时抛出
     */
    uint128_t& operator%=(const uint128_t& other);

    /**
     * @brief 前置自增
     */
    uint128_t& operator++() noexcept {
        *this += 1;
        return *this;
    }

    /**
     * @brief 前置自减
     */
    uint128_t& operator--() noexcept {
        *this -= 1;
        return *this;
    }

    /**
     * @brief 按位取反
     */
    constexpr uint128_t operator~() const noexcept { return uint128_t(~hi, ~lo); }

    /**
     * @brief 按位与赋值
     */
    constexpr uint128_t& operator&=(const uint128_t& other) noexcept {
        hi &= other.hi;
        lo &= other.lo;
        return *this;
    }

    /**
     * @brief 按位或赋值
     */
    constexpr uint128_t& operator|=(const uint128_t& other) noexcept {
        hi |= other.hi;
        lo |= other.lo;
        return *this;
    }

    /**
     * @brief 按位异或赋值
     */
    constexpr uint128_t& operator^=(const uint128_t& other) noexcept {
        hi ^= other.hi;
        lo ^= other.lo;
        return *this;
    }

    /**
     * @brief 左移赋值
     * @param shift 移位量
     */
    constexpr uint128_t& operator<<=(const uint32_t shift) noexcept {
        if (shift == 0) {
            return *this;
        }
        if (shift >= 128) {
            hi = 0;
            lo = 0;
            return *this;
        }
        if (shift >= 64) {
            hi = lo << (shift - 64);
            lo = 0;
            return *this;
        }
        hi = (hi << shift) | (lo >> (64 - shift));
        lo = lo << shift;
        return *this;
    }

    /**
     * @brief 右移赋值
     * @param shift 移位量
     */
    constexpr uint128_t& operator>>=(const uint32_t shift) noexcept {
        if (shift == 0) {
            return *this;
        }
        if (shift >= 128) {
            hi = 0;
            lo = 0;
            return *this;
        }
        if (shift >= 64) {
            lo = hi >> (shift - 64);
            hi = 0;
            return *this;
        }
        lo = (lo >> shift) | (hi << (64 - shift));
        hi = hi >> shift;
        return *this;
    }

    /**
     * @brief 64位乘法
     * @param a 第一个乘数
     * @param b 第二个乘数
     * @return 128位乘积
     */
    static uint128_t mul64(uint64_t a, uint64_t b) noexcept;

    /**
     * @brief 64位除法
     * @param divisor 除数
     * @param remainder 输出余数
     * @return 商
     */
    uint64_t div64(uint64_t divisor, uint64_t* remainder = nullptr) const;

    /**
     * @brief 计算哈希值
     */
    NEFORCE_NODISCARD constexpr size_t to_hash() const noexcept {
        constexpr uint64_t GOLDEN = 0x9E3779B97F4A7C15ULL;
        size_t seed = hash<uint64_t>()(lo);
        seed ^= hash<uint64_t>()(hi) + GOLDEN + (seed << 6) + (seed >> 2);
        return seed;
    }

    /**
     * @brief 从字符串解析
     * @param view 字符串视图
     * @return 解析结果
     * @throws typecast_exception 字符串格式无效时抛出
     */
    static constexpr uint128_t parse(const string_view view) { return uint128_t{view}; }

    /**
     * @brief 转换为字符串
     * @return 十进制字符串表示
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 string to_string() const;

    /**
     * @brief 获取最小值
     */
    static constexpr uint128_t min() noexcept { return {static_cast<uint64_t>(0), static_cast<uint64_t>(0)}; }

    /**
     * @brief 获取最大值
     */
    static constexpr uint128_t max() noexcept { return {~static_cast<uint64_t>(0), ~static_cast<uint64_t>(0)}; }
};

/**
 * @struct int128_t
 * @brief 128位有符号整数类型
 */
struct NEFORCE_API int128_t : icommon<int128_t>, iarithmetic<int128_t>, ibinary<int128_t>, iobject<int128_t> {
public:
    uint64_t lo{0}; ///< 低64位
    uint64_t hi{0}; ///< 高64位

    constexpr int128_t() noexcept = default;
    NEFORCE_CONSTEXPR20 ~int128_t() = default;

    /**
     * @brief 从 long 构造
     */
    constexpr int128_t(const long value) noexcept :
    int128_t(static_cast<make_integer_t<sizeof(long)>>(value)) {}

    /**
     * @brief 从32位有符号整数构造
     */
    constexpr int128_t(const int32_t value) noexcept :
    int128_t(static_cast<long long>(value)) {}

    /**
     * @brief 从 long long 构造
     */
    constexpr int128_t(const long long value) noexcept :
    lo(static_cast<uint64_t>(value)),
    hi(value < 0 ? ~static_cast<uint64_t>(0) : 0) {}

    /**
     * @brief 从低64位和符号构造
     * @param low 低64位
     * @param negative 是否为负数
     */
    constexpr int128_t(const uint64_t low, const bool negative = false) noexcept :
    lo(low),
    hi(negative ? ~static_cast<uint64_t>(0) : 0) {}

    /**
     * @brief 从高低64位构造
     * @param high 高64位
     * @param low 低64位
     */
    constexpr int128_t(const uint64_t high, const uint64_t low) noexcept :
    lo(low),
    hi(high) {}

    constexpr int128_t(const int128_t&) noexcept = default;
    constexpr int128_t& operator=(const int128_t&) noexcept = default;
    constexpr int128_t(int128_t&&) noexcept = default;
    constexpr int128_t& operator=(int128_t&&) noexcept = default;

    /**
     * @brief 从无符号128位整数赋值
     */
    constexpr int128_t& operator=(const uint128_t& other) noexcept {
        lo = other.lo;
        hi = other.hi;
        return *this;
    }

    /**
     * @brief 从字符串构造
     * @param str 数字字符串
     * @param base 进制基数（默认10）
     * @throws typecast_exception 字符串格式无效时抛出
     */
    explicit NEFORCE_CONSTEXPR20 int128_t(const string& str, int base = 10) :
    int128_t(str.view(), base) {}

    /**
     * @brief 从字符串视图构造
     * @param str 数字字符串视图
     * @param base 进制基数（默认10）
     * @throws typecast_exception 字符串格式无效时抛出
     */
    explicit constexpr int128_t(string_view str, int base = 10);

    /**
     * @brief 转换为无符号128位整数
     */
    NEFORCE_NODISCARD constexpr uint128_t to_uint128() const noexcept { return {hi, lo}; }

    explicit constexpr operator bool() const noexcept { return (lo != 0U) || (hi != 0U); }
    explicit constexpr operator char() const noexcept { return static_cast<char>(lo); }
    explicit constexpr operator int8_t() const noexcept { return static_cast<int8_t>(lo); }
    explicit constexpr operator int16_t() const noexcept { return static_cast<int16_t>(lo); }
    explicit constexpr operator int32_t() const noexcept { return static_cast<int32_t>(lo); }
    explicit constexpr operator int64_t() const noexcept { return static_cast<int64_t>(lo); }
    explicit constexpr operator uint8_t() const noexcept { return static_cast<uint8_t>(lo); }
    explicit constexpr operator uint16_t() const noexcept { return static_cast<uint16_t>(lo); }
    explicit constexpr operator uint32_t() const noexcept { return static_cast<uint32_t>(lo); }
    explicit constexpr operator uint64_t() const noexcept { return lo; }
    explicit constexpr operator uint128_t() const noexcept { return to_uint128(); }

    /**
     * @brief 检查是否为负数
     */
    NEFORCE_NODISCARD constexpr bool is_negative() const noexcept { return static_cast<int64_t>(hi) < 0; }

    /**
     * @brief 相等比较
     */
    NEFORCE_NODISCARD constexpr bool equal_to(const int128_t& rhs) const noexcept {
        return hi == rhs.hi && lo == rhs.lo;
    }

    /**
     * @brief 小于比较（考虑符号）
     */
    NEFORCE_NODISCARD constexpr bool less_than(const int128_t& rhs) const noexcept {
        const bool a_neg = is_negative();
        const bool b_neg = rhs.is_negative();
        if (a_neg != b_neg) {
            return a_neg;
        }
        return hi < rhs.hi || (hi == rhs.hi && lo < rhs.lo);
    }

    /**
     * @brief 负号
     */
    constexpr int128_t operator-() const noexcept {
        const uint64_t new_lo = ~lo + 1ULL;
        const uint64_t new_hi = ~hi + (lo == 0ULL ? 1ULL : 0ULL);
        return {new_hi, new_lo};
    }

    /**
     * @brief 加法赋值
     */
    int128_t& operator+=(const int128_t& other) noexcept;

    /**
     * @brief 减法赋值
     */
    int128_t& operator-=(const int128_t& other) noexcept;

    /**
     * @brief 乘法赋值
     */
    int128_t& operator*=(const int128_t& other) noexcept;

    /**
     * @brief 除法赋值
     * @throws math_exception 除数为0时抛出
     */
    int128_t& operator/=(const int128_t& other);

    /**
     * @brief 取模赋值
     * @throws math_exception 除数为0时抛出
     */
    int128_t& operator%=(const int128_t& other);

    /**
     * @brief 前置自增
     */
    int128_t& operator++() noexcept {
        *this += 1;
        return *this;
    }

    /**
     * @brief 前置自减
     */
    int128_t& operator--() noexcept {
        *this -= 1;
        return *this;
    }

    /**
     * @brief 按位取反
     */
    constexpr int128_t operator~() const noexcept { return int128_t(~hi, ~lo); }

    /**
     * @brief 按位与赋值
     */
    constexpr int128_t& operator&=(const int128_t& other) noexcept {
        hi &= other.hi;
        lo &= other.lo;
        return *this;
    }

    /**
     * @brief 按位或赋值
     */
    constexpr int128_t& operator|=(const int128_t& other) noexcept {
        hi |= other.hi;
        lo |= other.lo;
        return *this;
    }

    /**
     * @brief 按位异或赋值
     */
    constexpr int128_t& operator^=(const int128_t& other) noexcept {
        hi ^= other.hi;
        lo ^= other.lo;
        return *this;
    }

    /**
     * @brief 左移赋值
     */
    constexpr int128_t& operator<<=(const uint32_t shift) noexcept {
        *this = to_uint128() << shift;
        return *this;
    }

    /**
     * @brief 右移赋值（算术右移）
     */
    constexpr int128_t& operator>>=(const uint32_t shift) noexcept {
        if (shift == 0) {
            return *this;
        }
        const bool neg = is_negative();
        if (shift >= 128) {
            *this = neg ? int128_t(~static_cast<uint64_t>(0), ~static_cast<uint64_t>(0)) : int128_t(0);
            return *this;
        }
        if (shift >= 64) {
            lo = static_cast<uint64_t>(static_cast<int64_t>(hi) >> (shift - 64));
            hi = neg ? ~0ULL : 0ULL;
        } else {
            lo = (lo >> shift) | (hi << (64 - shift));
            hi = static_cast<uint64_t>(static_cast<int64_t>(hi) >> shift);
        }
        return *this;
    }

    /**
     * @brief 计算哈希值
     */
    NEFORCE_NODISCARD constexpr size_t to_hash() const noexcept {
        constexpr uint64_t GOLDEN = 0x9E3779B97F4A7C15ULL;
        size_t seed = hash<uint64_t>()(lo);
        seed ^= hash<uint64_t>()(hi) + GOLDEN + (seed << 6) + (seed >> 2);
        return seed;
    }

    /**
     * @brief 从字符串解析
     * @param view 字符串视图
     * @return 解析结果
     * @throws typecast_exception 字符串格式无效时抛出
     */
    static constexpr int128_t parse(const string_view view) { return int128_t{view}; }

    /**
     * @brief 转换为字符串
     * @return 十进制字符串表示
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 string to_string() const;

    /**
     * @brief 获取最小值
     */
    static constexpr int128_t min() noexcept {
        return {static_cast<uint64_t>(0x8000000000000000ULL), static_cast<uint64_t>(0ULL)};
    }

    /**
     * @brief 获取最大值
     */
    static constexpr int128_t max() noexcept { return {0x7FFFFFFFFFFFFFFFULL, ~static_cast<uint64_t>(0)}; }
};

/** @} */ // Int128

template <>
struct make_signed<uint128_t> {
    using type = int128_t;
};

template <>
struct make_unsigned<int128_t> {
    using type = uint128_t;
};

#define __NEFORCE_DEFINE_MAKE_SIGN(CV)  \
    template <>                         \
    struct make_signed<uint128_t CV> {  \
        using type = int128_t;          \
    };                                  \
    template <>                         \
    struct make_unsigned<int128_t CV> { \
        using type = uint128_t;         \
    };
NEFORCE_MACRO_RANGES_CV_REF(__NEFORCE_DEFINE_MAKE_SIGN)
#undef __NEFORCE_DEFINE_MAKE_SIGN


template <>
struct is_integral<uint128_t> : true_type {};

template <>
struct is_unsigned<uint128_t> : true_type {};

NEFORCE_CONSTEXPR20 string uint128_t::to_string() const { return inner::__int_to_string_dispatch<uint128_t>(*this); }

constexpr uint128_t operator-(const uint128_t& lhs, const uint128_t& rhs) noexcept {
    const uint64_t new_lo = lhs.lo - rhs.lo;
    const uint64_t new_hi = lhs.hi - rhs.hi - (lhs.lo < rhs.lo);
    return uint128_t(new_hi, new_lo);
}


template <>
struct is_integral<int128_t> : true_type {};

template <>
struct is_signed<int128_t> : true_type {};

NEFORCE_CONSTEXPR20 string int128_t::to_string() const { return inner::__int_to_string_dispatch<int128_t>(*this); }

constexpr int128_t operator-(const int128_t& lhs, const int128_t& rhs) noexcept {
    const uint128_t a = lhs.to_uint128();
    const uint128_t b = rhs.to_uint128();
    const uint128_t c = a - b;
    return int128_t(c.hi, c.lo);
}

constexpr int128_t uint128_t::to_int128() const noexcept { return int128_t(hi, lo); }
constexpr uint128_t::operator int128_t() const noexcept { return int128_t(hi, lo); }


NEFORCE_BEGIN_LITERALS__

/**
 * @defgroup UserLiterals 字面量
 * @brief 用户定义字面量支持
 * @{
 */

/**
 * @brief 128位无符号整数字面量
 * @param val 整数值
 * @return uint128_t对象
 */
constexpr uint128_t operator""_u128(const unsigned long long val) noexcept {
    return uint128_t(static_cast<uint64_t>(val));
}

/**
 * @brief 128位有符号整数字面量
 * @param val 整数值
 * @return int128_t对象
 */
constexpr int128_t operator""_i128(const unsigned long long val) noexcept {
    return int128_t(static_cast<uint64_t>(val));
}

/**
 * @brief 128位无符号整数字符串字面量
 * @param str 字符串
 * @param len 字符串长度
 * @return uint128_t对象
 * @throws typecast_exception 字符串格式无效时抛出
 */
constexpr uint128_t operator""_u128(const char* str, const size_t len) { return uint128_t(string_view{str, len}); }

/**
 * @brief 128位有符号整数字符串字面量
 * @param str 字符串
 * @param len 字符串长度
 * @return int128_t对象
 * @throws typecast_exception 字符串格式无效时抛出
 */
constexpr int128_t operator""_i128(const char* str, const size_t len) { return int128_t(string_view{str, len}); }

/** @} */ // UserLiterals

NEFORCE_END_LITERALS__

/**
 * @addtogroup NumericTraits 数值特征
 * @{
 */

/**
 * @brief uint128_t类型的数值特征特化
 */
template <>
class numeric_traits<uint128_t> {
public:
    static constexpr bool is_specialized = true;
    static constexpr bool is_signed = false;
    static constexpr bool is_integer = true;
    static constexpr bool is_exact = true;
    static constexpr bool has_infinity = false;
    static constexpr bool has_quiet_NaN = false;
    static constexpr bool has_signaling_NaN = false;
    static constexpr auto has_denorm = float_denorm_type::ABSENT;
    static constexpr bool has_denorm_loss = false;
    static constexpr auto round_style = float_round_type::TOWARD_ZERO;
    static constexpr bool is_iec559 = false;
    static constexpr bool is_bounded = true;
    static constexpr bool is_modulo = true;
    static constexpr int digits = 128;
    static constexpr int digits10 = 38;
    static constexpr int max_digits10 = 0;
    static constexpr int radix = 2;
    static constexpr int min_exponent = 0;
    static constexpr int min_exponent10 = 0;
    static constexpr int max_exponent = 0;
    static constexpr int max_exponent10 = 0;
    static constexpr bool traps = true;
    static constexpr bool tinyness_before = false;
    static constexpr uint128_t min() noexcept { return uint128_t::min(); }
    static constexpr uint128_t lowest() noexcept { return uint128_t::min(); }
    static constexpr uint128_t max() noexcept { return uint128_t::max(); }
    static constexpr uint128_t epsilon() noexcept { return 0; }
    static constexpr uint128_t round_error() noexcept { return 0; }
    static constexpr uint128_t infinity() noexcept { return 0; }
    static constexpr uint128_t quiet_NaN() noexcept { return 0; }
    static constexpr uint128_t signaling_NaN() noexcept { return 0; }
    static constexpr uint128_t denorm_min() noexcept { return 0; }
};

/**
 * @brief int128_t类型的数值特征特化
 */
template <>
class numeric_traits<int128_t> {
public:
    static constexpr bool is_specialized = true;
    static constexpr bool is_signed = true;
    static constexpr bool is_integer = true;
    static constexpr bool is_exact = true;
    static constexpr bool has_infinity = false;
    static constexpr bool has_quiet_NaN = false;
    static constexpr bool has_signaling_NaN = false;
    static constexpr auto has_denorm = float_denorm_type::ABSENT;
    static constexpr bool has_denorm_loss = false;
    static constexpr auto round_style = float_round_type::TOWARD_ZERO;
    static constexpr bool is_iec559 = false;
    static constexpr bool is_bounded = true;
    static constexpr bool is_modulo = false;
    static constexpr int digits = 127;
    static constexpr int digits10 = 38;
    static constexpr int max_digits10 = 0;
    static constexpr int radix = 2;
    static constexpr int min_exponent = 0;
    static constexpr int min_exponent10 = 0;
    static constexpr int max_exponent = 0;
    static constexpr int max_exponent10 = 0;
    static constexpr bool traps = true;
    static constexpr bool tinyness_before = false;
    static constexpr int128_t min() noexcept { return int128_t::min(); }
    static constexpr int128_t lowest() noexcept { return int128_t::min(); }
    static constexpr int128_t max() noexcept { return int128_t::max(); }
    static constexpr int128_t epsilon() noexcept { return 0; }
    static constexpr int128_t round_error() noexcept { return 0; }
    static constexpr int128_t infinity() noexcept { return 0; }
    static constexpr int128_t quiet_NaN() noexcept { return 0; }
    static constexpr int128_t signaling_NaN() noexcept { return 0; }
    static constexpr int128_t denorm_min() noexcept { return 0; }
};

/** @} */ // NumericTraits

/**
 * @addtogroup StringConverts 字符串转换
 * @{
 */

/**
 * @brief 将字符串转换为128位无符号整数
 * @param sv 要转换的字符串视图
 * @param idx 可选参数，存储转换结束位置索引
 * @param base 进制基数（0表示自动检测）
 * @return 转换后的128位无符号整数
 * @throws typecast_exception 转换失败时
 */
NEFORCE_NODISCARD constexpr uint128_t to_uint128(const string_view sv, size_t* idx = nullptr, const int base = 10) {
    char* endptr = nullptr;
    const uint128_t num = inner::str_to_uints<uint128_t>(sv, &endptr, base);
    if (sv.data() == endptr) {
        NEFORCE_THROW_EXCEPTION(typecast_exception("Convert from string failed."));
    }
    if (idx != nullptr) {
        *idx = static_cast<size_t>(endptr - sv.data());
    }
    return num;
}

/**
 * @brief 将字符串转换为128位有符号整数
 * @param sv 要转换的字符串视图
 * @param idx 可选参数，存储转换结束位置索引
 * @param base 进制基数（0表示自动检测）
 * @return 转换后的128位有符号整数
 * @throws typecast_exception 转换失败时
 */
NEFORCE_NODISCARD constexpr int128_t to_int128(const string_view sv, size_t* idx = nullptr, const int base = 10) {
    char* endptr = nullptr;
    const int128_t num = inner::str_to_ints<int128_t>(sv, &endptr, base);
    if (sv.data() == endptr) {
        NEFORCE_THROW_EXCEPTION(typecast_exception("Convert from string failed."));
    }
    if (idx != nullptr) {
        *idx = static_cast<size_t>(endptr - sv.data());
    }
    return num;
}

/** @} */ // StringConverts

constexpr uint128_t::uint128_t(const string_view str, const int base) { *this = to_uint128(str, nullptr, base); }

constexpr int128_t::int128_t(const string_view str, const int base) { *this = to_int128(str, nullptr, base); }

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_NUMERIC_INT128_HPP__
