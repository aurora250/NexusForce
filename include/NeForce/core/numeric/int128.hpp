#ifndef NEFORCE_CORE_NUMERIC_INT128_HPP__
#define NEFORCE_CORE_NUMERIC_INT128_HPP__

/**
 * @file int128.hpp
 * @brief 128位整数类型实现
 *
 * 此文件提供了128位有符号和无符号整数类型的完整实现。
 */

#include "NeForce/core/config/msvc_intrinsic.hpp"
#include "NeForce/core/exception/exception.hpp"
#include "NeForce/core/numeric/numeric_traits.hpp"
#include "NeForce/core/interface/inumeric.hpp"
#include "NeForce/core/interface/icommon.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Int128 128位整数
 * @brief 128位有符号和无符号整数类型的完整实现
 * @{
 */

struct int128_t;

/**
 * @struct uint128_t
 * @brief 128位无符号整数类型
 */
struct uint128_t : icommon<uint128_t>, iarithmetic<uint128_t>, ibinary<uint128_t> {
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

    NEFORCE_NODISCARD constexpr explicit operator bool() const noexcept { return lo != 0U || hi != 0U; }
    NEFORCE_NODISCARD constexpr explicit operator char() const noexcept { return static_cast<char>(lo); }
    NEFORCE_NODISCARD constexpr explicit operator int8_t() const noexcept { return static_cast<int8_t>(lo); }
    NEFORCE_NODISCARD constexpr explicit operator uint8_t() const noexcept { return static_cast<uint8_t>(lo); }
    NEFORCE_NODISCARD constexpr explicit operator uint16_t() const noexcept { return static_cast<uint16_t>(lo); }
    NEFORCE_NODISCARD constexpr explicit operator uint32_t() const noexcept { return static_cast<uint32_t>(lo); }
    NEFORCE_NODISCARD constexpr explicit operator uint64_t() const noexcept { return lo; }
    NEFORCE_NODISCARD constexpr explicit operator int128_t() const noexcept;

    /**
     * @brief 转换为有符号128位整数
     * @return 有符号128位整数
     */
    NEFORCE_NODISCARD constexpr int128_t to_int128() const noexcept;

    /**
     * @brief 相等比较
     * @param rhs 右侧操作数
     * @return 是否相等
     */
    NEFORCE_NODISCARD constexpr bool equal_to(const uint128_t& rhs) const noexcept {
        return hi == rhs.hi && lo == rhs.lo;
    }

    /**
     * @brief 小于比较
     * @param rhs 右侧操作数
     * @return 是否小于
     */
    NEFORCE_NODISCARD constexpr bool less_than(const uint128_t& rhs) const noexcept {
        return hi < rhs.hi || (hi == rhs.hi && lo < rhs.lo);
    }

    /**
     * @brief 取负
     * @return 二补数结果
     */
    NEFORCE_NODISCARD constexpr uint128_t negation() const noexcept {
        const uint64_t new_lo = ~lo + 1ULL;
        const uint64_t new_hi = ~hi + (lo == 0ULL ? 1ULL : 0ULL);
        return {new_hi, new_lo};
    }

    /**
     * @brief 加法赋值
     * @param other 右侧操作数
     * @return 自身引用
     */
    constexpr uint128_t& operator+=(const uint128_t& other) noexcept {
#if defined(NEFORCE_SUPPORT_INTRINSIC_INT128) && !defined(NEFORCE_PLATFORM_WINDOWS)
        const auto old_lo = lo;
        lo += other.lo;
        hi += other.hi + static_cast<uint64_t>(lo < old_lo);
#else
        const byte_t carry = _NEFORCE _addcarry_u64(0, lo, other.lo, &lo);
        _NEFORCE _addcarry_u64(carry, hi, other.hi, &hi);
#endif
        return *this;
    }

    /**
     * @brief 减法赋值
     * @param other 右侧操作数
     * @return 自身引用
     */
    constexpr uint128_t& operator-=(const uint128_t& other) noexcept {
#if defined(NEFORCE_SUPPORT_INTRINSIC_INT128) && !defined(NEFORCE_PLATFORM_WINDOWS)
        const auto old_lo = lo;
        lo -= other.lo;
        hi -= other.hi + static_cast<uint64_t>(old_lo < other.lo);
#else
        const byte_t borrow = _NEFORCE _subborrow_u64(0, lo, other.lo, &lo);
        _NEFORCE _subborrow_u64(borrow, hi, other.hi, &hi);
#endif
        return *this;
    }

    /**
     * @brief 乘法赋值
     * @param other 右侧操作数
     * @return 自身引用
     */
    constexpr uint128_t& operator*=(const uint128_t& other) noexcept;

    /**
     * @brief 除法赋值
     * @param other 右侧操作数
     * @return 自身引用
     * @throws math_exception 除数为0时抛出
     */
    constexpr uint128_t& operator/=(const uint128_t& other);

    /**
     * @brief 取模赋值
     * @param other 右侧操作数
     * @return 自身引用
     * @throws math_exception 除数为0时抛出
     */
    constexpr uint128_t& operator%=(const uint128_t& other);

    /**
     * @brief 前置自增
     * @return 自身引用
     */
    constexpr uint128_t& operator++() noexcept { return *this += uint128_t{1ULL}; }

    /**
     * @brief 前置自减
     * @return 自身引用
     */
    constexpr uint128_t& operator--() noexcept { return *this -= uint128_t{1ULL}; }

    /**
     * @brief 按位取反
     * @return 按位取反结果
     */
    constexpr uint128_t operator~() const noexcept { return {~hi, ~lo}; }

    /**
     * @brief 按位与赋值
     * @param other 右侧操作数
     * @return 自身引用
     */
    constexpr uint128_t& operator&=(const uint128_t& other) noexcept {
        hi &= other.hi;
        lo &= other.lo;
        return *this;
    }

    /**
     * @brief 按位或赋值
     * @param other 右侧操作数
     * @return 自身引用
     */
    constexpr uint128_t& operator|=(const uint128_t& other) noexcept {
        hi |= other.hi;
        lo |= other.lo;
        return *this;
    }

    /**
     * @brief 按位异或赋值
     * @param other 右侧操作数
     * @return 自身引用
     */
    constexpr uint128_t& operator^=(const uint128_t& other) noexcept {
        hi ^= other.hi;
        lo ^= other.lo;
        return *this;
    }

    /**
     * @brief 左移赋值
     * @param shift 移位量
     * @return 自身引用
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
     * @return 自身引用
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
    NEFORCE_NODISCARD static constexpr uint128_t mul64(const uint64_t a, const uint64_t b) noexcept {
        uint128_t res;
#if defined(NEFORCE_SUPPORT_INTRINSIC_INT128) && !defined(NEFORCE_PLATFORM_WINDOWS)
        const unsigned __int128 prod = static_cast<unsigned __int128>(a) * b;
        res.lo = static_cast<uint64_t>(prod);
        res.hi = static_cast<uint64_t>(prod >> 64);
#else
        res.lo = _NEFORCE _umul128(a, b, &res.hi);
#endif
        return res;
    }

    /**
     * @brief 64位除法
     * @param divisor 除数
     * @param remainder 输出余数指针（可为空）
     * @return 商
     */
    NEFORCE_NODISCARD constexpr uint64_t div64(const uint64_t divisor, uint64_t* remainder = nullptr) const {
        if (hi == 0) {
            if (remainder != nullptr) {
                *remainder = lo % divisor;
            }
            return lo / divisor;
        }
#if defined(NEFORCE_SUPPORT_INTRINSIC_INT128) && !defined(NEFORCE_PLATFORM_WINDOWS)
        const unsigned __int128 dividend = (static_cast<unsigned __int128>(hi) << 64) | lo;
        const auto quot = static_cast<uint64_t>(dividend / divisor);
        if (remainder != nullptr) {
            *remainder = static_cast<uint64_t>(dividend % divisor);
        }
        return quot;
#else
        return _NEFORCE _udiv128(hi, lo, divisor, remainder);
#endif
    }

    /**
     * @brief 计算哈希值
     * @return 哈希值
     */
    NEFORCE_NODISCARD constexpr size_t to_hash() const noexcept {
        constexpr uint64_t GOLDEN = 0x9E3779B97F4A7C15ULL;
        size_t seed = hash<uint64_t>()(lo);
        seed ^= hash<uint64_t>()(hi) + GOLDEN + (seed << 6) + (seed >> 2);
        return seed;
    }

    /**
     * @brief 128位乘法辅助函数
     * @param a 第一个乘数
     * @param b 第二个乘数
     * @return 128位乘积
     */
    NEFORCE_NODISCARD static constexpr uint128_t mul128(const uint128_t& a, const uint128_t& b) noexcept {
        uint128_t result;
#if defined(NEFORCE_SUPPORT_INTRINSIC_INT128) && !defined(NEFORCE_PLATFORM_WINDOWS)
        const unsigned __int128 prod = (static_cast<unsigned __int128>(a.hi) << 64 | a.lo) *
                                       (static_cast<unsigned __int128>(b.hi) << 64 | b.lo);
        result.lo = static_cast<uint64_t>(prod);
        result.hi = static_cast<uint64_t>(prod >> 64);
#else
        result.lo = _NEFORCE _umul128(a.lo, b.lo, &result.hi);
        uint64_t t1_hi = 0, t2_hi = 0;
        const uint64_t t1_lo = _NEFORCE _umul128(a.lo, b.hi, &t1_hi);
        const uint64_t t2_lo = _NEFORCE _umul128(a.hi, b.lo, &t2_hi);
        uint64_t carry_hi = result.hi;
        const byte_t c1 = _NEFORCE _addcarry_u64(0, carry_hi, t1_lo, &carry_hi);
        uint64_t final_hi = 0;
        _NEFORCE _addcarry_u64(c1, carry_hi, t2_lo, &final_hi);
        result.hi = final_hi;
#endif
        return result;
    }

    /**
     * @brief 128位除法辅助函数
     * @param dividend 被除数
     * @param divisor 除数
     * @param quotient 输出商
     * @param remainder 输出余数
     * @throws math_exception 除数为0时抛出
     */
    constexpr static void divmod128(const uint128_t& dividend, const uint128_t& divisor, uint128_t& quotient,
                                    uint128_t& remainder);

    /**
     * @brief 获取最小值
     * @return 最小值（0）
     */
    static constexpr uint128_t min() noexcept { return {static_cast<uint64_t>(0), static_cast<uint64_t>(0)}; }

    /**
     * @brief 获取最大值
     * @return 最大值
     */
    static constexpr uint128_t max() noexcept { return {~static_cast<uint64_t>(0), ~static_cast<uint64_t>(0)}; }
};

/**
 * @struct int128_t
 * @brief 128位有符号整数类型
 */
struct int128_t : icommon<int128_t>, iarithmetic<int128_t>, ibinary<int128_t> {
    uint64_t lo{0}; ///< 低64位
    uint64_t hi{0}; ///< 高64位

    constexpr int128_t() noexcept = default;
    NEFORCE_CONSTEXPR20 ~int128_t() = default;

    /**
     * @brief 从 long 构造
     * @param value 整数值
     */
    constexpr int128_t(const long value) noexcept :
    int128_t(static_cast<make_integer_t<sizeof(long)>>(value)) {}

    /**
     * @brief 从32位有符号整数构造
     * @param value 整数值
     */
    constexpr int128_t(const int32_t value) noexcept :
    int128_t(static_cast<long long>(value)) {}

    /**
     * @brief 从 long long 构造
     * @param value 整数值
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
     * @brief 从无符号128位整数构造/赋值
     * @param other 无符号128位整数
     */
    constexpr int128_t(const uint128_t& other) noexcept :
    lo(other.lo),
    hi(other.hi) {}

    /**
     * @brief 从无符号128位整数赋值
     * @param other 无符号128位整数
     * @return 自身引用
     */
    constexpr int128_t& operator=(const uint128_t& other) noexcept {
        lo = other.lo;
        hi = other.hi;
        return *this;
    }

    NEFORCE_NODISCARD constexpr explicit operator bool() const noexcept { return (lo != 0U) || (hi != 0U); }
    NEFORCE_NODISCARD constexpr explicit operator char() const noexcept { return static_cast<char>(lo); }
    NEFORCE_NODISCARD constexpr explicit operator int8_t() const noexcept { return static_cast<int8_t>(lo); }
    NEFORCE_NODISCARD constexpr explicit operator int16_t() const noexcept { return static_cast<int16_t>(lo); }
    NEFORCE_NODISCARD constexpr explicit operator int32_t() const noexcept { return static_cast<int32_t>(lo); }
    NEFORCE_NODISCARD constexpr explicit operator int64_t() const noexcept { return static_cast<int64_t>(lo); }
    NEFORCE_NODISCARD constexpr explicit operator uint8_t() const noexcept { return static_cast<uint8_t>(lo); }
    NEFORCE_NODISCARD constexpr explicit operator uint16_t() const noexcept { return static_cast<uint16_t>(lo); }
    NEFORCE_NODISCARD constexpr explicit operator uint32_t() const noexcept { return static_cast<uint32_t>(lo); }
    NEFORCE_NODISCARD constexpr explicit operator uint64_t() const noexcept { return lo; }
    NEFORCE_NODISCARD constexpr explicit operator uint128_t() const noexcept { return {hi, lo}; }

    /**
     * @brief 检查是否为负数
     * @return 是否为负数
     */
    NEFORCE_NODISCARD constexpr bool is_negative() const noexcept { return static_cast<int64_t>(hi) < 0; }

    /**
     * @brief 转换为无符号128位整数
     * @return 无符号128位整数
     */
    NEFORCE_NODISCARD constexpr uint128_t to_uint128() const noexcept { return {hi, lo}; }

    /**
     * @brief 相等比较
     * @param rhs 右侧操作数
     * @return 是否相等
     */
    NEFORCE_NODISCARD constexpr bool equal_to(const int128_t& rhs) const noexcept {
        return hi == rhs.hi && lo == rhs.lo;
    }

    /**
     * @brief 小于比较（考虑符号）
     * @param rhs 右侧操作数
     * @return 是否小于
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
     * @return 取负结果
     */
    NEFORCE_NODISCARD constexpr int128_t negation() const noexcept {
        const uint64_t new_lo = ~lo + 1ULL;
        const uint64_t new_hi = ~hi + (lo == 0ULL ? 1ULL : 0ULL);
        return {new_hi, new_lo};
    }

    /**
     * @brief 加法赋值
     * @param other 右侧操作数
     * @return 自身引用
     */
    constexpr int128_t& operator+=(const int128_t& other) noexcept {
        const uint128_t a = to_uint128();
        const uint128_t b = other.to_uint128();
        const uint128_t c = a + b;
        lo = c.lo;
        hi = c.hi;
        return *this;
    }

    /**
     * @brief 减法赋值
     * @param other 右侧操作数
     * @return 自身引用
     */
    constexpr int128_t& operator-=(const int128_t& other) noexcept {
        const uint128_t a = to_uint128();
        const uint128_t b = other.to_uint128();
        const uint128_t c = a - b;
        lo = c.lo;
        hi = c.hi;
        return *this;
    }

    /**
     * @brief 乘法赋值
     * @param other 右侧操作数
     * @return 自身引用
     */
    constexpr int128_t& operator*=(const int128_t& other) noexcept {
        const uint128_t a = to_uint128();
        const uint128_t b = other.to_uint128();
        const uint128_t c = a * b;
        lo = c.lo;
        hi = c.hi;
        return *this;
    }

    /**
     * @brief 除法赋值
     * @param other 右侧操作数
     * @return 自身引用
     * @throws math_exception 除数为0时抛出
     */
    constexpr int128_t& operator/=(const int128_t& other);

    /**
     * @brief 取模赋值
     * @param other 右侧操作数
     * @return 自身引用
     * @throws math_exception 除数为0时抛出
     */
    constexpr int128_t& operator%=(const int128_t& other);

    /**
     * @brief 前置自增
     * @return 自身引用
     */
    constexpr int128_t& operator++() noexcept { return *this += int128_t{1}; }

    /**
     * @brief 前置自减
     * @return 自身引用
     */
    constexpr int128_t& operator--() noexcept { return *this -= int128_t{1}; }

    /**
     * @brief 按位取反
     * @return 取反结果
     */
    NEFORCE_NODISCARD constexpr int128_t operator~() const noexcept { return {~hi, ~lo}; }

    /**
     * @brief 按位与赋值
     * @param other 右侧操作数
     * @return 自身引用
     */
    constexpr int128_t& operator&=(const int128_t& other) noexcept {
        hi &= other.hi;
        lo &= other.lo;
        return *this;
    }

    /**
     * @brief 按位或赋值
     * @param other 右侧操作数
     * @return 自身引用
     */
    constexpr int128_t& operator|=(const int128_t& other) noexcept {
        hi |= other.hi;
        lo |= other.lo;
        return *this;
    }

    /**
     * @brief 按位异或赋值
     * @param other 右侧操作数
     * @return 自身引用
     */
    constexpr int128_t& operator^=(const int128_t& other) noexcept {
        hi ^= other.hi;
        lo ^= other.lo;
        return *this;
    }

    /**
     * @brief 左移赋值
     * @param shift 移位量
     * @return 自身引用
     */
    constexpr int128_t& operator<<=(const uint32_t shift) noexcept {
        *this = to_uint128() << shift;
        return *this;
    }

    /**
     * @brief 右移赋值（算术右移）
     * @param shift 移位量
     * @return 自身引用
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
     * @return 哈希值
     */
    NEFORCE_NODISCARD constexpr size_t to_hash() const noexcept {
        constexpr uint64_t GOLDEN = 0x9E3779B97F4A7C15ULL;
        size_t seed = hash<uint64_t>()(lo);
        seed ^= hash<uint64_t>()(hi) + GOLDEN + (seed << 6) + (seed >> 2);
        return seed;
    }

    /**
     * @brief 获取最小值
     * @return 最小值
     */
    static constexpr int128_t min() noexcept {
        return {static_cast<uint64_t>(0x8000000000000000ULL), static_cast<uint64_t>(0ULL)};
    }

    /**
     * @brief 获取最大值
     * @return 最大值
     */
    static constexpr int128_t max() noexcept { return {0x7FFFFFFFFFFFFFFFULL, ~static_cast<uint64_t>(0)}; }
};

constexpr uint128_t& uint128_t::operator*=(const uint128_t& other) noexcept {
    *this = mul128(*this, other);
    return *this;
}

constexpr uint128_t& uint128_t::operator/=(const uint128_t& other) {
    const uint128_t copy{*this};
    uint128_t r;
    divmod128(copy, other, *this, r);
    return *this;
}

constexpr uint128_t& uint128_t::operator%=(const uint128_t& other) {
    const uint128_t copy{*this};
    uint128_t q;
    divmod128(copy, other, q, *this);
    return *this;
}

constexpr void uint128_t::divmod128(const uint128_t& dividend, const uint128_t& divisor, uint128_t& quotient,
                                    uint128_t& remainder) {
    if (!divisor) {
        NEFORCE_THROW_EXCEPTION(math_exception("Division by zero"));
    }
#if defined(NEFORCE_SUPPORT_INTRINSIC_INT128) && !defined(NEFORCE_PLATFORM_WINDOWS)
    const unsigned __int128 d = (static_cast<unsigned __int128>(dividend.hi) << 64) | dividend.lo;
    const unsigned __int128 r = (static_cast<unsigned __int128>(divisor.hi) << 64) | divisor.lo;
    const unsigned __int128 q = d / r;
    const unsigned __int128 m = d % r;
    quotient = uint128_t{static_cast<uint64_t>(q >> 64), static_cast<uint64_t>(q)};
    remainder = uint128_t{static_cast<uint64_t>(m >> 64), static_cast<uint64_t>(m)};
#else
    if (divisor == uint128_t{1ULL}) {
        quotient = dividend;
        remainder = uint128_t{0ULL};
        return;
    }
    if (dividend < divisor) {
        quotient = uint128_t{0ULL};
        remainder = dividend;
        return;
    }

    const int bits = dividend.hi != 0 ? (128 - clz64(dividend.hi)) : (dividend.lo != 0 ? (64 - clz64(dividend.lo)) : 0);

    quotient = uint128_t{0ULL};
    remainder = uint128_t{0ULL};

    for (int i = bits - 1; i >= 0; --i) {
        remainder <<= 1;
        const uint64_t bit = (i >= 64) ? ((dividend.hi >> (i - 64)) & 1ULL) : ((dividend.lo >> i) & 1ULL);
        remainder.lo |= bit;
        if (remainder >= divisor) {
            remainder -= divisor;
            if (i >= 64) {
                quotient.hi |= (static_cast<uint64_t>(1) << (i - 64));
            } else {
                quotient.lo |= (static_cast<uint64_t>(1) << i);
            }
        }
    }
#endif
}

constexpr int128_t& int128_t::operator/=(const int128_t& other) {
    if (!other) {
        NEFORCE_THROW_EXCEPTION(math_exception("Division by zero"));
    }
    const bool neg_a = is_negative();
    const bool neg_b = other.is_negative();
    const int128_t abs_a = neg_a ? negation() : *this;
    const int128_t abs_b = neg_b ? other.negation() : other;
    const uint128_t q = abs_a.to_uint128() / abs_b.to_uint128();
    const int128_t result(q.hi, q.lo);
    *this = ((neg_a ^ neg_b) != 0) ? result.negation() : result;
    return *this;
}

constexpr int128_t& int128_t::operator%=(const int128_t& other) {
    if (!other) {
        NEFORCE_THROW_EXCEPTION(math_exception("Division by zero"));
    }
    const bool neg_a = is_negative();
    const int128_t abs_a = neg_a ? negation() : *this;
    const int128_t abs_b = other.is_negative() ? other.negation() : other;
    const uint128_t r = abs_a.to_uint128() % abs_b.to_uint128();
    const int128_t result(r.hi, r.lo);
    *this = neg_a ? result.negation() : result;
    return *this;
}

constexpr uint128_t::operator int128_t() const noexcept { return {hi, lo}; }
constexpr int128_t uint128_t::to_int128() const noexcept { return {hi, lo}; }


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

template <>
struct is_integral<int128_t> : true_type {};

template <>
struct is_signed<int128_t> : true_type {};

/** @} */ // Int128

/**
 * @addtogroup NumericTraits 数值特征
 * @{
 */

/**
 * @brief uint128_t类型的数值特征特化
 */
template <>
class numeric_traits<uint128_t> : public inner::numeric_int_base {
public:
    static constexpr uint128_t min() noexcept { return uint128_t::min(); }
    static constexpr uint128_t lowest() noexcept { return uint128_t::min(); }
    static constexpr uint128_t max() noexcept { return uint128_t::max(); }
    static constexpr uint128_t epsilon() noexcept { return uint128_t{0ULL}; }
    static constexpr uint128_t round_error() noexcept { return uint128_t{0ULL}; }
    static constexpr uint128_t infinity() noexcept { return uint128_t{0ULL}; }
    static constexpr uint128_t quiet_nan() noexcept { return uint128_t{0ULL}; }
    static constexpr uint128_t signaling_nan() noexcept { return uint128_t{0ULL}; }
    static constexpr uint128_t denorm_min() noexcept { return uint128_t{0ULL}; }

    static constexpr bool is_signed = false;
    static constexpr bool is_modulo = true;

    static constexpr int digits = 128;
    static constexpr int digits10 = 38;
    static constexpr int max_digits10 = 0;
    static constexpr int min_exponent = 0;
    static constexpr int min_exponent10 = 0;
    static constexpr int max_exponent = 0;
    static constexpr int max_exponent10 = 0;
};

/**
 * @brief int128_t类型的数值特征特化
 */
template <>
class numeric_traits<int128_t> : public inner::numeric_int_base {
public:
    static constexpr int128_t min() noexcept { return int128_t::min(); }
    static constexpr int128_t lowest() noexcept { return int128_t::min(); }
    static constexpr int128_t max() noexcept { return int128_t::max(); }
    static constexpr int128_t epsilon() noexcept { return int128_t{0}; }
    static constexpr int128_t round_error() noexcept { return int128_t{0}; }
    static constexpr int128_t infinity() noexcept { return int128_t{0}; }
    static constexpr int128_t quiet_nan() noexcept { return int128_t{0}; }
    static constexpr int128_t signaling_nan() noexcept { return int128_t{0}; }
    static constexpr int128_t denorm_min() noexcept { return int128_t{0}; }

    static constexpr bool is_signed = true;
    static constexpr int digits = 127;
    static constexpr int digits10 = 38;
    static constexpr int max_digits10 = 0;
    static constexpr int min_exponent = 0;
    static constexpr int min_exponent10 = 0;
    static constexpr int max_exponent = 0;
    static constexpr int max_exponent10 = 0;
};

/** @} */ // NumericTraits

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
NEFORCE_NODISCARD constexpr uint128_t operator""_u128(const unsigned long long val) noexcept {
    return {static_cast<uint64_t>(val)};
}

/**
 * @brief 128位有符号整数字面量
 * @param val 整数值
 * @return int128_t对象
 */
NEFORCE_NODISCARD constexpr int128_t operator""_i128(const unsigned long long val) noexcept {
    return {static_cast<uint64_t>(val)};
}

/** @} */ // UserLiterals

NEFORCE_END_LITERALS__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_NUMERIC_INT128_HPP__
