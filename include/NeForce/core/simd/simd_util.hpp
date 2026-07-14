#ifndef NEFORCE_CORE_SIMD_SIMD_UTIL_HPP__
#define NEFORCE_CORE_SIMD_SIMD_UTIL_HPP__

/**
 * @file simd_util.hpp
 * @brief 通用 SIMD 工具函数
 *
 * 此文件提供跨平台 SIMD 操作的统一封装，包括 SSE2、AVX2、ARM NEON 的支持。
 * 非 SIMD 环境自动回退为标量 C++ 实现。
 */

#include "NeForce/core/typeinfo/types.hpp"
#ifdef NEFORCE_SIMD_SSE2
#    include <emmintrin.h>
#elif defined(NEFORCE_SIMD_NEON)
#    include <arm_neon.h>
#endif
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_SIMD__

/**
 * @defgroup SimdUtil SIMD
 * @brief 跨平台 SIMD 操作的抽象封装
 * @{
 */

#ifdef NEFORCE_SIMD_SSE2
/// @brief 128-bit SIMD 向量类型
using vec128_t = ::__m128i;
#elif defined(NEFORCE_SIMD_NEON)
/// @brief 128-bit SIMD 向量类型
using vec128_t = ::uint8x16_t;
#else
/// @brief 128-bit SIMD 向量类型
struct vec128_t {
    byte_t data[16];
};
#endif

/**
 * @brief 将单字节广播到 128-bit SIMD 向量的全部 16 个位置
 * @param c 源字节值
 * @return 所有字节均为 c 的向量
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t fill_byte(byte_t c) noexcept {
#ifdef NEFORCE_SIMD_SSE2
    return ::_mm_set1_epi8(static_cast<char>(c));
#elif defined(NEFORCE_SIMD_NEON)
    return ::vdupq_n_u8(c);
#else
    vec128_t result;
    for (int i = 0; i < 16; ++i) {
        result.data[i] = c;
    }
    return result;
#endif
}

/**
 * @brief 非对齐加载 16 字节到 SIMD 向量
 * @param ptr 源内存地址（无需 16 字节对齐）
 * @return 加载的向量
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t load_unaligned(const void* ptr) noexcept {
#ifdef NEFORCE_SIMD_SSE2
    return ::_mm_loadu_si128(static_cast<const vec128_t*>(ptr));
#elif defined(NEFORCE_SIMD_NEON)
    return ::vld1q_u8(static_cast<const uint8_t*>(ptr));
#else
    vec128_t result;
    const auto* src = static_cast<const byte_t*>(ptr);
    for (int i = 0; i < 16; ++i) {
        result.data[i] = src[i];
    }
    return result;
#endif
}

/**
 * @brief 逐字节比较两个 128-bit SIMD 向量的相等性
 * @param a 左操作数
 * @param b 右操作数
 * @return 相等位置为 0xFF，不等位置为 0x00 的向量
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t match_bytes(vec128_t a, vec128_t b) noexcept {
#ifdef NEFORCE_SIMD_SSE2
    return ::_mm_cmpeq_epi8(a, b);
#elif defined(NEFORCE_SIMD_NEON)
    return ::vreinterpretq_u8_u8(vceqq_u8(a, b));
#else
    vec128_t result;
    for (int i = 0; i < 16; ++i) {
        result.data[i] = (a.data[i] == b.data[i]) ? 0xFF : 0x00;
    }
    return result;
#endif
}

/**
 * @brief 提取向量中每个字节的最高位，组成 16-bit 掩码
 * @param v 输入向量
 * @return 16-bit 掩码，bit i 对应字节 i 的最高位
 */
NEFORCE_ALWAYS_INLINE_INLINE int to_bitmask(vec128_t v) noexcept {
#ifdef NEFORCE_SIMD_SSE2
    return ::_mm_movemask_epi8(v);
#elif defined(NEFORCE_SIMD_NEON)
    static const int8_t __attribute__((aligned(16))) shift_data[16] = {-128, -128, -128, -128, -128, -128, -128, -128,
                                                                       -128, -128, -128, -128, -128, -128, -128, -128};
    int mask = 0;
    const auto* bytes = reinterpret_cast<const byte_t*>(&v);
    for (int i = 0; i < 16; ++i) {
        if (bytes[i] & 0x80) {
            mask |= (1 << i);
        }
    }
    return mask;
#else
    int mask = 0;
    for (int i = 0; i < 16; ++i) {
        if (v.data[i] & 0x80) {
            mask |= (1 << i);
        }
    }
    return mask;
#endif
}

/** @} */ // SimdUtil

NEFORCE_END_SIMD__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_SIMD_SIMD_UTIL_HPP__
