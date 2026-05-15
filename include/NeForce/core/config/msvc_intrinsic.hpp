#ifndef NEFORCE_CORE_CONFIG_MSVC_INTRINSIC_HPP__
#define NEFORCE_CORE_CONFIG_MSVC_INTRINSIC_HPP__

/**
 * @file msvc_intrinsic.hpp
 * @brief MSVC内部函数替代实现
 *
 * 此文件提供了MSVC编译器内部函数的替代实现，使得这些函数在其他环境也能使用。
 *
 * 主要功能：
 * - 带进位的64位加法
 * - 带借位的64位减法
 * - 64位无符号乘法
 * - 128位无符号除法
 */

#include "NeForce/core/memory/bit.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup MSVCCompilerIntrinsics MSVC内部函数替代实现
 * @brief MSVC编译器内部函数的替代实现
 * @{
 */

/**
 * @brief 带进位的64位无符号加法
 * @param carry_in 进位输入（0或1）
 * @param a 第一个加数
 * @param b 第二个加数
 * @param out 输出结果（低64位）
 * @return 进位输出（0或1）
 *
 * 计算 a + b + carry_in，结果存入out，返回进位标志。
 */
NEFORCE_CONSTEXPR14 uint8_t _addcarry_u64(const uint8_t carry_in, const uint64_t a, const uint64_t b,
                                          uint64_t* out) noexcept {
    const auto a_lo = static_cast<uint32_t>(a);
    const auto a_hi = static_cast<uint32_t>(a >> 32);
    const auto b_lo = static_cast<uint32_t>(b);
    const auto b_hi = static_cast<uint32_t>(b >> 32);

    const uint64_t sum_lo = static_cast<uint64_t>(a_lo) + static_cast<uint64_t>(b_lo) + carry_in;
    const uint64_t sum_hi = static_cast<uint64_t>(a_hi) + static_cast<uint64_t>(b_hi) + (sum_lo >> 32);

    *out = (sum_hi << 32) | (sum_lo & 0xFFFFFFFFULL);
    return static_cast<uint8_t>(sum_hi >> 32);
}

/**
 * @brief 带借位的64位无符号减法
 * @param borrow_in 借位输入（0或1）
 * @param a 被减数
 * @param b 减数
 * @param out 输出结果（低64位）
 * @return 借位输出（0或1）
 *
 * 计算 a - b - borrow_in，结果存入out，返回借位标志。
 */
NEFORCE_CONSTEXPR14 uint8_t _subborrow_u64(const uint8_t borrow_in, const uint64_t a, const uint64_t b,
                                           uint64_t* out) noexcept {
    const auto a_lo = static_cast<uint32_t>(a);
    const auto a_hi = static_cast<uint32_t>(a >> 32);
    const auto b_lo = static_cast<uint32_t>(b);
    const auto b_hi = static_cast<uint32_t>(b >> 32);

    const uint64_t diff_lo = static_cast<uint64_t>(a_lo) - static_cast<uint64_t>(b_lo) - borrow_in;
    const uint64_t borrow_lo = (diff_lo >> 63);

    const uint64_t diff_hi = static_cast<uint64_t>(a_hi) - static_cast<uint64_t>(b_hi) - borrow_lo;
    const uint64_t borrow_hi = (diff_hi >> 63);

    *out = ((diff_hi & 0xFFFFFFFFULL) << 32) | (diff_lo & 0xFFFFFFFFULL);
    return static_cast<uint8_t>(borrow_hi);
}

/**
 * @brief 64位无符号乘法
 * @param a 第一个乘数
 * @param b 第二个乘数
 * @param hi_out 输出结果的高64位
 * @return 结果的低64位
 *
 * 计算 a * b 的128位结果，低64位作为返回值，高64位存入hi_out。
 */
NEFORCE_CONSTEXPR14 uint64_t _umul128(const uint64_t a, const uint64_t b, uint64_t* hi_out) noexcept {
    const auto a_lo = static_cast<uint32_t>(a);
    const auto a_hi = static_cast<uint32_t>(a >> 32);
    const auto b_lo = static_cast<uint32_t>(b);
    const auto b_hi = static_cast<uint32_t>(b >> 32);

    const uint64_t p_ll = static_cast<uint64_t>(a_lo) * b_lo; // [63:0]
    const uint64_t p_lh = static_cast<uint64_t>(a_lo) * b_hi; // [95:32]
    const uint64_t p_hl = static_cast<uint64_t>(a_hi) * b_lo; // [95:32]
    const uint64_t p_hh = static_cast<uint64_t>(a_hi) * b_hi; // [127:64]

    const uint64_t mid = (p_ll >> 32) + (p_lh & 0xFFFFFFFFULL) + (p_hl & 0xFFFFFFFFULL);
    const uint64_t lo = (p_ll & 0xFFFFFFFFULL) | (mid << 32);
    const uint64_t hi = p_hh + (p_lh >> 32) + (p_hl >> 32) + (mid >> 32);

    *hi_out = hi;
    return lo;
}

/**
 * @brief 128位无符号除法（基于Knuth-D）
 * @param dividend_hi 被除数高64位
 * @param dividend_lo 被除数低64位
 * @param divisor 除数（64位，必须非0）
 * @param remainder 输出余数（可为空）
 * @return 商（64位）
 */
NEFORCE_CONSTEXPR14 uint64_t _udiv128(const uint64_t dividend_hi, const uint64_t dividend_lo, const uint64_t divisor,
                                      uint64_t* remainder) noexcept {
    if (dividend_hi == 0) {
        if (remainder != nullptr) {
            *remainder = dividend_lo % divisor;
        }
        return dividend_lo / divisor;
    }

    const int s = _NEFORCE clz64(divisor);
    const uint64_t d = divisor << s;

    uint64_t u2 = 0, u1 = 0, u0 = 0;
    if (s == 0) {
        u1 = dividend_hi;
        u0 = dividend_lo;
    } else {
        u2 = dividend_hi >> (64 - s);
        u1 = (dividend_hi << s) | (dividend_lo >> (64 - s));
        u0 = dividend_lo << s;
    }

    const auto d_hi = static_cast<uint32_t>(d >> 32);
    const auto d_lo = static_cast<uint32_t>(d);

    uint64_t u_hi = (static_cast<uint64_t>(u2) << 32) | (u1 >> 32);
    uint64_t q1 = u_hi / d_hi;
    uint64_t r1 = u_hi % d_hi;

    while (q1 >= 0x100000000ULL || q1 * d_lo > ((r1 << 32) | (u1 & 0xFFFFFFFF))) {
        --q1;
        r1 += d_hi;
        if (r1 >= 0x100000000ULL) {
            break;
        }
    }

    uint64_t prod_hi = 0;
    uint64_t prod_lo = _NEFORCE _umul128(q1, d, &prod_hi);
    uint64_t rem_hi = u2 - prod_hi;
    uint64_t rem_lo = u1 - prod_lo;
    if (u1 < prod_lo) {
        --rem_hi;
    }

    if ((rem_hi & (1ULL << 63)) != 0U) {
        --q1;
        rem_lo += d;
        if (rem_lo < d) {
            ++rem_hi;
        }
    }

    u_hi = rem_lo >> 32;
    uint64_t q0 = u_hi / d_hi;
    uint64_t r0 = u_hi % d_hi;

    while (q0 >= 0x100000000ULL || q0 * d_lo > ((r0 << 32) | (rem_lo & 0xFFFFFFFF))) {
        --q0;
        r0 += d_hi;
        if (r0 >= 0x100000000ULL) {
            break;
        }
    }

    prod_lo = _NEFORCE _umul128(q0, d, &prod_hi);
    uint64_t rem_mid_hi = 0 - prod_hi;
    uint64_t rem_mid_lo = rem_lo - prod_lo;
    if (rem_lo < prod_lo) {
        --rem_mid_hi;
    }

    if ((rem_mid_hi & (1ULL << 63)) != 0U) {
        --q0;
        rem_mid_lo += d;
    }

    const uint64_t quotient = (q1 << 32) | q0;

    if (remainder != nullptr) {
        if (s > 0) {
            *remainder = (rem_mid_lo << (64 - s)) | (u0 >> s);
        } else {
            *remainder = u0;
        }
    }

    return quotient;
}

/** @} */ // MSVCCompilerIntrinsics

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_CONFIG_MSVC_INTRINSIC_HPP__
