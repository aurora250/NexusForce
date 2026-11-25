#ifndef NEFORCE_CORE_SIMD_BITWISE_LOONGARCH_HPP__
#define NEFORCE_CORE_SIMD_BITWISE_LOONGARCH_HPP__
#include "NeForce/core/simd/scalar/bitwise.hpp"
#ifdef NEFORCE_SIMD_BRANCH_LOONGARCH
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_SIMD__

/**
 * @brief 按位与
 * @param a 左操作数
 * @param b 右操作数
 * @return a & b
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t bit_and(vec128_t a, vec128_t b) noexcept { return __lsx_vand_v(a, b); }

/**
 * @brief 按位或
 * @param a 左操作数
 * @param b 右操作数
 * @return a | b
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t bit_or(vec128_t a, vec128_t b) noexcept { return __lsx_vor_v(a, b); }

/**
 * @brief 按位异或
 * @param a 左操作数
 * @param b 右操作数
 * @return a ^ b
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t bit_xor(vec128_t a, vec128_t b) noexcept { return __lsx_vxor_v(a, b); }

/**
 * @brief 按位取反
 * @param v 输入向量
 * @return ~v
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t bit_not(vec128_t v) noexcept { return __lsx_vnor_v(v, v); }

/**
 * @brief 按位与非
 * @param a 左操作数
 * @param b 右操作数
 * @return a & ~b
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t bit_andnot(vec128_t a, vec128_t b) noexcept {
    return __lsx_vand_v(a, __lsx_vnor_v(b, b));
}

/**
 * @brief 按位与（256-bit）
 * @param a 左操作数
 * @param b 右操作数
 * @return a & b
 */
NEFORCE_ALWAYS_INLINE_INLINE vec256_t bit_and(vec256_t a, vec256_t b) noexcept {
#    if defined(NEFORCE_SIMD_LASX)
    return ::__lasx_xvand_v(a, b);
#    else
    return scalar::bit_and(a, b);
#    endif
}

/**
 * @brief 按位或（256-bit）
 * @param a 左操作数
 * @param b 右操作数
 * @return a | b
 */
NEFORCE_ALWAYS_INLINE_INLINE vec256_t bit_or(vec256_t a, vec256_t b) noexcept {
#    if defined(NEFORCE_SIMD_LASX)
    return ::__lasx_xvor_v(a, b);
#    else
    return scalar::bit_or(a, b);
#    endif
}

/**
 * @brief 按位异或（256-bit）
 * @param a 左操作数
 * @param b 右操作数
 * @return a ^ b
 */
NEFORCE_ALWAYS_INLINE_INLINE vec256_t bit_xor(vec256_t a, vec256_t b) noexcept {
#    if defined(NEFORCE_SIMD_LASX)
    return ::__lasx_xvxor_v(a, b);
#    else
    return scalar::bit_xor(a, b);
#    endif
}

/**
 * @brief 按位与（512-bit）
 * @param a 左操作数
 * @param b 右操作数
 * @return a & b
 */
NEFORCE_ALWAYS_INLINE_INLINE vec512_t bit_and(vec512_t a, vec512_t b) noexcept { return scalar::bit_and(a, b); }

/**
 * @brief 按位或（512-bit）
 * @param a 左操作数
 * @param b 右操作数
 * @return a | b
 */
NEFORCE_ALWAYS_INLINE_INLINE vec512_t bit_or(vec512_t a, vec512_t b) noexcept { return scalar::bit_or(a, b); }

/**
 * @brief 按位异或（512-bit）
 * @param a 左操作数
 * @param b 右操作数
 * @return a ^ b
 */
NEFORCE_ALWAYS_INLINE_INLINE vec512_t bit_xor(vec512_t a, vec512_t b) noexcept { return scalar::bit_xor(a, b); }


using scalar::popcount;
using scalar::shift_left_bytes;
using scalar::shift_right_bytes;

NEFORCE_END_SIMD__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_SIMD_BRANCH_LOONGARCH
#endif // NEFORCE_CORE_SIMD_BITWISE_LOONGARCH_HPP__
