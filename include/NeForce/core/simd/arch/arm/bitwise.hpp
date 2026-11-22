#ifndef NEFORCE_CORE_SIMD_BITWISE_ARM_HPP__
#define NEFORCE_CORE_SIMD_BITWISE_ARM_HPP__
#include "NeForce/core/simd/types.hpp"
#ifdef NEFORCE_SIMD_BRANCH_ARM
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_SIMD__

/**
 * @addtogroup SIMD
 * @{
 */

/**
 * @brief 按位与
 * @param a 左操作数
 * @param b 右操作数
 * @return a & b
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t bit_and(vec128_t a, vec128_t b) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    return ::vandq_u8(a, b);
#    else
    vec128_t result;
    for (int i = 0; i < 16; ++i) {
        result.data[i] = a.data[i] & b.data[i];
    }
    return result;
#    endif
}

/**
 * @brief 按位或
 * @param a 左操作数
 * @param b 右操作数
 * @return a | b
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t bit_or(vec128_t a, vec128_t b) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    return ::vorrq_u8(a, b);
#    else
    vec128_t result;
    for (int i = 0; i < 16; ++i) {
        result.data[i] = a.data[i] | b.data[i];
    }
    return result;
#    endif
}

/**
 * @brief 按位异或
 * @param a 左操作数
 * @param b 右操作数
 * @return a ^ b
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t bit_xor(vec128_t a, vec128_t b) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    return ::veorq_u8(a, b);
#    else
    vec128_t result;
    for (int i = 0; i < 16; ++i) {
        result.data[i] = a.data[i] ^ b.data[i];
    }
    return result;
#    endif
}

/**
 * @brief 按位取反
 * @param v 输入向量
 * @return ~v
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t bit_not(vec128_t v) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    return ::vmvnq_u8(v);
#    else
    vec128_t result;
    for (int i = 0; i < 16; ++i) {
        result.data[i] = static_cast<byte_t>(~v.data[i]);
    }
    return result;
#    endif
}

/**
 * @brief 按位与非
 * @param a 左操作数
 * @param b 右操作数
 * @return a & ~b
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t bit_andnot(vec128_t a, vec128_t b) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    return ::vbicq_u8(a, b);
#    else
    vec128_t result;
    for (int i = 0; i < 16; ++i) {
        result.data[i] = a.data[i] & static_cast<byte_t>(~b.data[i]);
    }
    return result;
#    endif
}

/**
 * @brief 按位与（256-bit）
 * @param a 左操作数
 * @param b 右操作数
 * @return a & b
 */
NEFORCE_ALWAYS_INLINE_INLINE vec256_t bit_and(vec256_t a, vec256_t b) noexcept {
    vec256_t result;
    for (int i = 0; i < 32; ++i) {
        result.data[i] = a.data[i] & b.data[i];
    }
    return result;
}

/**
 * @brief 按位或（256-bit）
 * @param a 左操作数
 * @param b 右操作数
 * @return a | b
 */
NEFORCE_ALWAYS_INLINE_INLINE vec256_t bit_or(vec256_t a, vec256_t b) noexcept {
    vec256_t result;
    for (int i = 0; i < 32; ++i) {
        result.data[i] = a.data[i] | b.data[i];
    }
    return result;
}

/**
 * @brief 按位异或（256-bit）
 * @param a 左操作数
 * @param b 右操作数
 * @return a ^ b
 */
NEFORCE_ALWAYS_INLINE_INLINE vec256_t bit_xor(vec256_t a, vec256_t b) noexcept {
    vec256_t result;
    for (int i = 0; i < 32; ++i) {
        result.data[i] = a.data[i] ^ b.data[i];
    }
    return result;
}

/**
 * @brief 按位与（512-bit）
 * @param a 左操作数
 * @param b 右操作数
 * @return a & b
 */
NEFORCE_ALWAYS_INLINE_INLINE vec512_t bit_and(vec512_t a, vec512_t b) noexcept {
    vec512_t result;
    for (int i = 0; i < 64; ++i) {
        result.data[i] = a.data[i] & b.data[i];
    }
    return result;
}

/**
 * @brief 按位或（512-bit）
 * @param a 左操作数
 * @param b 右操作数
 * @return a | b
 */
NEFORCE_ALWAYS_INLINE_INLINE vec512_t bit_or(vec512_t a, vec512_t b) noexcept {
    vec512_t result;
    for (int i = 0; i < 64; ++i) {
        result.data[i] = a.data[i] | b.data[i];
    }
    return result;
}

/**
 * @brief 按位异或（512-bit）
 * @param a 左操作数
 * @param b 右操作数
 * @return a ^ b
 */
NEFORCE_ALWAYS_INLINE_INLINE vec512_t bit_xor(vec512_t a, vec512_t b) noexcept {
    vec512_t result;
    for (int i = 0; i < 64; ++i) {
        result.data[i] = a.data[i] ^ b.data[i];
    }
    return result;
}

/**
 * @brief 将 128-bit 向量按字节左移
 * @tparam Bytes 左移字节数
 * @param v 输入向量
 * @return 左移后右端补零的向量
 */
template <int Bytes>
NEFORCE_ALWAYS_INLINE_INLINE vec128_t shift_left_bytes(vec128_t v) noexcept {
    static_assert(Bytes >= 0 && Bytes <= 16, "shift_left_bytes: Bytes must be in [0, 16]");
#    if defined(NEFORCE_SIMD_NEON)
    return vextq_u8(::vdupq_n_u8(0), v, static_cast<int>(16 - Bytes));
#    else
    vec128_t result;
    for (int i = 0; i < Bytes; ++i) {
        result.data[i] = 0;
    }
    for (int i = Bytes; i < 16; ++i) {
        result.data[i] = v.data[i - Bytes];
    }
    return result;
#    endif
}

template <>
NEFORCE_ALWAYS_INLINE_INLINE vec128_t shift_left_bytes<0>(vec128_t v) noexcept {
    return v;
}

/**
 * @brief 将 128-bit 向量按字节右移
 * @tparam Bytes 右移字节数
 * @param v 输入向量
 * @return 右移后左端补零的向量
 */
template <int Bytes>
NEFORCE_ALWAYS_INLINE_INLINE vec128_t shift_right_bytes(vec128_t v) noexcept {
    static_assert(Bytes >= 0 && Bytes <= 16, "shift_right_bytes: Bytes must be in [0, 16]");
#    if defined(NEFORCE_SIMD_NEON)
    return vextq_u8(v, ::vdupq_n_u8(0), static_cast<int>(Bytes));
#    else
    vec128_t result;
    for (int i = 0; i < 16 - Bytes; ++i) {
        result.data[i] = v.data[i + Bytes];
    }
    for (int i = 16 - Bytes; i < 16; ++i) {
        result.data[i] = 0;
    }
    return result;
#    endif
}

template <>
NEFORCE_ALWAYS_INLINE_INLINE vec128_t shift_right_bytes<0>(vec128_t v) noexcept {
    return v;
}

/**
 * @brief 统计 128-bit 向量中置位比特总数
 * @param v 输入向量
 * @return 置位比特数（0-128）
 */
NEFORCE_ALWAYS_INLINE_INLINE int popcount(vec128_t v) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    const ::uint8x16_t low_nibble = ::vandq_u8(v, ::vdupq_n_u8(0x0F));
    const ::uint8x16_t high_nibble = vshrq_n_u8(v, 4);
    const ::uint8x16_t lookup = {0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4};
    ::uint8x16_t pop = ::vaddq_u8(::vqtbl1q_u8(lookup, low_nibble), ::vqtbl1q_u8(lookup, high_nibble));
#        ifdef NEFORCE_ARCH_ARM
    return static_cast<int>(::vaddvq_u8(pop));
#        else
    int total = 0;
    const auto* bytes = reinterpret_cast<const byte_t*>(&pop);
    for (int i = 0; i < 16; ++i) {
        total += bytes[i];
    }
    return total;
#        endif
#    else
    int total = 0;
    for (int i = 0; i < 16; ++i) {
        byte_t b = v.data[i];
        total += (b & 1) + ((b >> 1) & 1) + ((b >> 2) & 1) + ((b >> 3) & 1) + ((b >> 4) & 1) + ((b >> 5) & 1) +
                 ((b >> 6) & 1) + ((b >> 7) & 1);
    }
    return total;
#    endif
}

/** @} */ // SIMD

NEFORCE_END_SIMD__
NEFORCE_END_NAMESPACE__
#endif
#endif // NEFORCE_CORE_SIMD_BITWISE_ARM_HPP__
