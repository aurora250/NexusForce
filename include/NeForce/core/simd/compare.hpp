#ifndef NEFORCE_CORE_SIMD_COMPARE_HPP__
#define NEFORCE_CORE_SIMD_COMPARE_HPP__

/**
 * @file compare.hpp
 * @brief 跨平台 SIMD 比较运算
 *
 * 提供整型及浮点逐元素比较操作。
 */

#include "NeForce/core/simd/types.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_SIMD__

/**
 * @addtogroup SIMD
 * @{
 */

/**
 * @brief 16 路 8-bit 相等比较
 * @param a 左操作数（16×i8）
 * @param b 右操作数（16×i8）
 * @return a == b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t cmpeq_i8(vec128_t a, vec128_t b) noexcept {
#ifdef NEFORCE_SIMD_SSE2
    return ::_mm_cmpeq_epi8(a, b);
#elif defined(NEFORCE_SIMD_NEON)
    return ::vceqq_s8(::vreinterpretq_s8_u8(a), ::vreinterpretq_s8_u8(b));
#else
    vec128_t result;
    for (int i = 0; i < 16; ++i) {
        result.data[i] = (a.data[i] == b.data[i]) ? 0xFF : 0x00;
    }
    return result;
#endif
}

/**
 * @brief 8 路 16-bit 相等比较
 * @param a 左操作数（8×i16）
 * @param b 右操作数（8×i16）
 * @return a == b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t cmpeq_i16(vec128_t a, vec128_t b) noexcept {
#ifdef NEFORCE_SIMD_SSE2
    return ::_mm_cmpeq_epi16(a, b);
#elif defined(NEFORCE_SIMD_NEON)
    return ::vreinterpretq_u8_u16(::vceqq_s16(::vreinterpretq_s16_u8(a), ::vreinterpretq_s16_u8(b)));
#else
    const auto* sa = reinterpret_cast<const int16_t*>(a.data);
    const auto* sb = reinterpret_cast<const int16_t*>(b.data);
    vec128_t result;
    auto* rd = reinterpret_cast<uint16_t*>(result.data);
    for (int i = 0; i < 8; ++i) {
        rd[i] = (sa[i] == sb[i]) ? 0xFFFF : 0;
    }
    return result;
#endif
}

/**
 * @brief 4 路 32-bit 相等比较
 * @param a 左操作数（4×i32）
 * @param b 右操作数（4×i32）
 * @return a == b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t cmpeq_i32(vec128_t a, vec128_t b) noexcept {
#ifdef NEFORCE_SIMD_SSE2
    return ::_mm_cmpeq_epi32(a, b);
#elif defined(NEFORCE_SIMD_NEON)
    return ::vreinterpretq_u8_u32(::vceqq_s32(::vreinterpretq_s32_u8(a), ::vreinterpretq_s32_u8(b)));
#else
    const auto* sa = reinterpret_cast<const int32_t*>(a.data);
    const auto* sb = reinterpret_cast<const int32_t*>(b.data);
    vec128_t result;
    auto* rd = reinterpret_cast<uint32_t*>(result.data);
    for (int i = 0; i < 4; ++i) {
        rd[i] = (sa[i] == sb[i]) ? 0xFFFFFFFF : 0;
    }
    return result;
#endif
}

/**
 * @brief 2 路 64-bit 相等比较
 * @param a 左操作数（2×i64）
 * @param b 右操作数（2×i64）
 * @return a == b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t cmpeq_i64(vec128_t a, vec128_t b) noexcept {
#if defined(__SSE4_1__) || defined(NEFORCE_SIMD_AVX2)
    return ::_mm_cmpeq_epi64(a, b);
#elif defined(NEFORCE_SIMD_SSE2)
    const ::__m128i eq_lo = ::_mm_cmpeq_epi32(a, b);
    const ::__m128i eq_hi = ::_mm_cmpeq_epi32(_mm_shuffle_epi32(a, _MM_SHUFFLE(2, 3, 0, 1)),
                                              _mm_shuffle_epi32(b, _MM_SHUFFLE(2, 3, 0, 1)));
    return ::_mm_and_si128(eq_lo, eq_hi);
#elif defined(NEFORCE_SIMD_NEON)
    return ::vreinterpretq_u8_u64(::vceqq_s64(::vreinterpretq_s64_u8(a), ::vreinterpretq_s64_u8(b)));
#else
    const auto* sa = reinterpret_cast<const int64_t*>(a.data);
    const auto* sb = reinterpret_cast<const int64_t*>(b.data);
    vec128_t result;
    auto* rd = reinterpret_cast<uint64_t*>(result.data);
    for (int i = 0; i < 2; ++i) {
        rd[i] = (sa[i] == sb[i]) ? 0xFFFFFFFFFFFFFFFFULL : 0;
    }
    return result;
#endif
}

/**
 * @brief 16 路 8-bit 有符号大于比较
 * @param a 左操作数（16×i8）
 * @param b 右操作数（16×i8）
 * @return a > b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t cmpgt_i8(vec128_t a, vec128_t b) noexcept {
#ifdef NEFORCE_SIMD_SSE2
    return ::_mm_cmpgt_epi8(a, b);
#elif defined(NEFORCE_SIMD_NEON)
    return ::vcgtq_s8(::vreinterpretq_s8_u8(a), ::vreinterpretq_s8_u8(b));
#else
    vec128_t result;
    for (int i = 0; i < 16; ++i) {
        result.data[i] = (static_cast<int8_t>(a.data[i]) > static_cast<int8_t>(b.data[i])) ? 0xFF : 0x00;
    }
    return result;
#endif
}

/**
 * @brief 8 路 16-bit 有符号大于比较
 * @param a 左操作数（8×i16）
 * @param b 右操作数（8×i16）
 * @return a > b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t cmpgt_i16(vec128_t a, vec128_t b) noexcept {
#ifdef NEFORCE_SIMD_SSE2
    return ::_mm_cmpgt_epi16(a, b);
#elif defined(NEFORCE_SIMD_NEON)
    return ::vreinterpretq_u8_u16(::vcgtq_s16(::vreinterpretq_s16_u8(a), ::vreinterpretq_s16_u8(b)));
#else
    const auto* sa = reinterpret_cast<const int16_t*>(a.data);
    const auto* sb = reinterpret_cast<const int16_t*>(b.data);
    vec128_t result;
    auto* rd = reinterpret_cast<uint16_t*>(result.data);
    for (int i = 0; i < 8; ++i) {
        rd[i] = (sa[i] > sb[i]) ? 0xFFFF : 0;
    }
    return result;
#endif
}

/**
 * @brief 4 路 32-bit 有符号大于比较
 * @param a 左操作数（4×i32）
 * @param b 右操作数（4×i32）
 * @return a > b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t cmpgt_i32(vec128_t a, vec128_t b) noexcept {
#ifdef NEFORCE_SIMD_SSE2
    return ::_mm_cmpgt_epi32(a, b);
#elif defined(NEFORCE_SIMD_NEON)
    return ::vreinterpretq_u8_u32(::vcgtq_s32(::vreinterpretq_s32_u8(a), ::vreinterpretq_s32_u8(b)));
#else
    const auto* sa = reinterpret_cast<const int32_t*>(a.data);
    const auto* sb = reinterpret_cast<const int32_t*>(b.data);
    vec128_t result;
    auto* rd = reinterpret_cast<uint32_t*>(result.data);
    for (int i = 0; i < 4; ++i) {
        rd[i] = (sa[i] > sb[i]) ? 0xFFFFFFFF : 0;
    }
    return result;
#endif
}

/**
 * @brief 2 路 64-bit 有符号大于比较
 * @param a 左操作数（2×i64）
 * @param b 右操作数（2×i64）
 * @return a > b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t cmpgt_i64(vec128_t a, vec128_t b) noexcept {
#if defined(__SSE4_2__) || defined(NEFORCE_SIMD_AVX2)
    return ::_mm_cmpgt_epi64(a, b);
#elif defined(NEFORCE_SIMD_SSE2)
    const auto* sa = reinterpret_cast<const int64_t*>(&a);
    const auto* sb = reinterpret_cast<const int64_t*>(&b);
    alignas(16) int64_t rd[2];
    rd[0] = sa[0] > sb[0] ? -1LL : 0LL;
    rd[1] = sa[1] > sb[1] ? -1LL : 0LL;
    return ::_mm_load_si128(reinterpret_cast<const ::__m128i*>(rd));
#elif defined(NEFORCE_SIMD_NEON)
    return ::vreinterpretq_u8_u64(::vcgtq_s64(::vreinterpretq_s64_u8(a), ::vreinterpretq_s64_u8(b)));
#else
    const auto* sa = reinterpret_cast<const int64_t*>(a.data);
    const auto* sb = reinterpret_cast<const int64_t*>(b.data);
    vec128_t result;
    auto* rd = reinterpret_cast<uint64_t*>(result.data);
    for (int i = 0; i < 2; ++i) {
        rd[i] = (sa[i] > sb[i]) ? 0xFFFFFFFFFFFFFFFFULL : 0;
    }
    return result;
#endif
}

/**
 * @brief 4 路单精度浮点相等比较
 * @param a 左操作数（4×f32）
 * @param b 右操作数（4×f32）
 * @return a == b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128f_t cmpeq_f32(vec128f_t a, vec128f_t b) noexcept {
#ifdef NEFORCE_SIMD_SSE2
    return ::_mm_cmpeq_ps(a, b);
#elif defined(NEFORCE_SIMD_NEON)
    return ::vreinterpretq_f32_u32(::vceqq_f32(a, b));
#else
    vec128f_t result;
    for (int i = 0; i < 4; ++i) {
        reinterpret_cast<uint32_t&>(result.data[i]) = (a.data[i] == b.data[i]) ? 0xFFFFFFFF : 0;
    }
    return result;
#endif
}

/**
 * @brief 4 路单精度浮点大于比较
 * @param a 左操作数（4×f32）
 * @param b 右操作数（4×f32）
 * @return a > b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128f_t cmpgt_f32(vec128f_t a, vec128f_t b) noexcept {
#ifdef NEFORCE_SIMD_SSE2
    return ::_mm_cmpgt_ps(a, b);
#elif defined(NEFORCE_SIMD_NEON)
    return ::vreinterpretq_f32_u32(::vcgtq_f32(a, b));
#else
    vec128f_t result;
    for (int i = 0; i < 4; ++i) {
        reinterpret_cast<uint32_t&>(result.data[i]) = (a.data[i] > b.data[i]) ? 0xFFFFFFFF : 0;
    }
    return result;
#endif
}

/**
 * @brief 4 路单精度浮点大于等于比较
 * @param a 左操作数（4×f32）
 * @param b 右操作数（4×f32）
 * @return a >= b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128f_t cmpge_f32(vec128f_t a, vec128f_t b) noexcept {
#ifdef NEFORCE_SIMD_SSE2
    return ::_mm_cmpge_ps(a, b);
#elif defined(NEFORCE_SIMD_NEON)
    return ::vreinterpretq_f32_u32(::vcgeq_f32(a, b));
#else
    vec128f_t result;
    for (int i = 0; i < 4; ++i) {
        reinterpret_cast<uint32_t&>(result.data[i]) = (a.data[i] >= b.data[i]) ? 0xFFFFFFFF : 0;
    }
    return result;
#endif
}

/**
 * @brief 4 路单精度浮点小于比较
 * @param a 左操作数（4×f32）
 * @param b 右操作数（4×f32）
 * @return a < b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128f_t cmplt_f32(vec128f_t a, vec128f_t b) noexcept {
#ifdef NEFORCE_SIMD_SSE2
    return ::_mm_cmplt_ps(a, b);
#elif defined(NEFORCE_SIMD_NEON)
    return ::vreinterpretq_f32_u32(::vcltq_f32(a, b));
#else
    vec128f_t result;
    for (int i = 0; i < 4; ++i) {
        reinterpret_cast<uint32_t&>(result.data[i]) = (a.data[i] < b.data[i]) ? 0xFFFFFFFF : 0;
    }
    return result;
#endif
}

/**
 * @brief 4 路单精度浮点小于等于比较
 * @param a 左操作数（4×f32）
 * @param b 右操作数（4×f32）
 * @return a <= b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128f_t cmple_f32(vec128f_t a, vec128f_t b) noexcept {
#ifdef NEFORCE_SIMD_SSE2
    return ::_mm_cmple_ps(a, b);
#elif defined(NEFORCE_SIMD_NEON)
    return ::vreinterpretq_f32_u32(::vcleq_f32(a, b));
#else
    vec128f_t result;
    for (int i = 0; i < 4; ++i) {
        reinterpret_cast<uint32_t&>(result.data[i]) = (a.data[i] <= b.data[i]) ? 0xFFFFFFFF : 0;
    }
    return result;
#endif
}

/**
 * @brief 2 路双精度浮点相等比较
 * @param a 左操作数（2×f64）
 * @param b 右操作数（2×f64）
 * @return a == b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128d_t cmpeq_f64(vec128d_t a, vec128d_t b) noexcept {
#ifdef NEFORCE_SIMD_SSE2
    return ::_mm_cmpeq_pd(a, b);
#elif defined(NEFORCE_SIMD_NEON)
    return ::vreinterpretq_f64_u64(::vceqq_f64(a, b));
#else
    vec128d_t result;
    for (int i = 0; i < 2; ++i) {
        reinterpret_cast<uint64_t&>(result.data[i]) = (a.data[i] == b.data[i]) ? 0xFFFFFFFFFFFFFFFFULL : 0;
    }
    return result;
#endif
}

/**
 * @brief 2 路双精度浮点大于比较
 * @param a 左操作数（2×f64）
 * @param b 右操作数（2×f64）
 * @return a > b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128d_t cmpgt_f64(vec128d_t a, vec128d_t b) noexcept {
#ifdef NEFORCE_SIMD_SSE2
    return ::_mm_cmpgt_pd(a, b);
#elif defined(NEFORCE_SIMD_NEON)
    return ::vreinterpretq_f64_u64(::vcgtq_f64(a, b));
#else
    vec128d_t result;
    for (int i = 0; i < 2; ++i) {
        reinterpret_cast<uint64_t&>(result.data[i]) = (a.data[i] > b.data[i]) ? 0xFFFFFFFFFFFFFFFFULL : 0;
    }
    return result;
#endif
}

/**
 * @brief 2 路双精度浮点大于等于比较
 * @param a 左操作数（2×f64）
 * @param b 右操作数（2×f64）
 * @return a >= b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128d_t cmpge_f64(vec128d_t a, vec128d_t b) noexcept {
#ifdef NEFORCE_SIMD_SSE2
    return ::_mm_cmpge_pd(a, b);
#elif defined(NEFORCE_SIMD_NEON)
    return ::vreinterpretq_f64_u64(::vcgeq_f64(a, b));
#else
    vec128d_t result;
    for (int i = 0; i < 2; ++i) {
        reinterpret_cast<uint64_t&>(result.data[i]) = (a.data[i] >= b.data[i]) ? 0xFFFFFFFFFFFFFFFFULL : 0;
    }
    return result;
#endif
}

/**
 * @brief 2 路双精度浮点小于比较
 * @param a 左操作数（2×f64）
 * @param b 右操作数（2×f64）
 * @return a < b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128d_t cmplt_f64(vec128d_t a, vec128d_t b) noexcept {
#ifdef NEFORCE_SIMD_SSE2
    return ::_mm_cmplt_pd(a, b);
#elif defined(NEFORCE_SIMD_NEON)
    return ::vreinterpretq_f64_u64(::vcltq_f64(a, b));
#else
    vec128d_t result;
    for (int i = 0; i < 2; ++i) {
        reinterpret_cast<uint64_t&>(result.data[i]) = (a.data[i] < b.data[i]) ? 0xFFFFFFFFFFFFFFFFULL : 0;
    }
    return result;
#endif
}

/**
 * @brief 2 路双精度浮点小于等于比较
 * @param a 左操作数（2×f64）
 * @param b 右操作数（2×f64）
 * @return a <= b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128d_t cmple_f64(vec128d_t a, vec128d_t b) noexcept {
#ifdef NEFORCE_SIMD_SSE2
    return ::_mm_cmple_pd(a, b);
#elif defined(NEFORCE_SIMD_NEON)
    return ::vreinterpretq_f64_u64(::vcleq_f64(a, b));
#else
    vec128d_t result;
    for (int i = 0; i < 2; ++i) {
        reinterpret_cast<uint64_t&>(result.data[i]) = (a.data[i] <= b.data[i]) ? 0xFFFFFFFFFFFFFFFFULL : 0;
    }
    return result;
#endif
}

/** @} */ // SIMD

NEFORCE_END_SIMD__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_SIMD_COMPARE_HPP__
