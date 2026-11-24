#ifndef NEFORCE_CORE_SIMD_COMPARE_X86_HPP__
#define NEFORCE_CORE_SIMD_COMPARE_X86_HPP__
#include "NeForce/core/simd/scalar/compare.hpp"
#ifdef NEFORCE_SIMD_BRANCH_X86
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
#    if defined(NEFORCE_SIMD_SSE2)
    return ::_mm_cmpeq_epi8(a, b);
#    else
    return scalar::cmpeq_i8(a, b);
#    endif
}

/**
 * @brief 8 路 16-bit 相等比较
 * @param a 左操作数（8×i16）
 * @param b 右操作数（8×i16）
 * @return a == b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t cmpeq_i16(vec128_t a, vec128_t b) noexcept {
#    if defined(NEFORCE_SIMD_SSE2)
    return ::_mm_cmpeq_epi16(a, b);
#    else
    return scalar::cmpeq_i16(a, b);
#    endif
}

/**
 * @brief 4 路 32-bit 相等比较
 * @param a 左操作数（4×i32）
 * @param b 右操作数（4×i32）
 * @return a == b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t cmpeq_i32(vec128_t a, vec128_t b) noexcept {
#    if defined(NEFORCE_SIMD_SSE2)
    return ::_mm_cmpeq_epi32(a, b);
#    else
    return scalar::cmpeq_i32(a, b);
#    endif
}

/**
 * @brief 2 路 64-bit 相等比较
 * @param a 左操作数（2×i64）
 * @param b 右操作数（2×i64）
 * @return a == b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t cmpeq_i64(vec128_t a, vec128_t b) noexcept {
#    if defined(NEFORCE_SIMD_AVX2)
    return ::_mm_cmpeq_epi64(a, b);
#    elif defined(NEFORCE_SIMD_SSE2)
    const ::__m128i eq_lo = ::_mm_cmpeq_epi32(a, b);
    const ::__m128i eq_hi = ::_mm_cmpeq_epi32(_mm_shuffle_epi32(a, _MM_SHUFFLE(2, 3, 0, 1)),
                                              _mm_shuffle_epi32(b, _MM_SHUFFLE(2, 3, 0, 1)));
    return ::_mm_and_si128(eq_lo, eq_hi);
#    else
    return scalar::cmpeq_i64(a, b);
#    endif
}

/**
 * @brief 16 路 8-bit 有符号大于比较
 * @param a 左操作数（16×i8）
 * @param b 右操作数（16×i8）
 * @return a > b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t cmpgt_i8(vec128_t a, vec128_t b) noexcept {
#    if defined(NEFORCE_SIMD_SSE2)
    return ::_mm_cmpgt_epi8(a, b);
#    else
    return scalar::cmpgt_i8(a, b);
#    endif
}

/**
 * @brief 8 路 16-bit 有符号大于比较
 * @param a 左操作数（8×i16）
 * @param b 右操作数（8×i16）
 * @return a > b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t cmpgt_i16(vec128_t a, vec128_t b) noexcept {
#    if defined(NEFORCE_SIMD_SSE2)
    return ::_mm_cmpgt_epi16(a, b);
#    else
    return scalar::cmpgt_i16(a, b);
#    endif
}

/**
 * @brief 4 路 32-bit 有符号大于比较
 * @param a 左操作数（4×i32）
 * @param b 右操作数（4×i32）
 * @return a > b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t cmpgt_i32(vec128_t a, vec128_t b) noexcept {
#    if defined(NEFORCE_SIMD_SSE2)
    return ::_mm_cmpgt_epi32(a, b);
#    else
    return scalar::cmpgt_i32(a, b);
#    endif
}

/**
 * @brief 2 路 64-bit 有符号大于比较
 * @param a 左操作数（2×i64）
 * @param b 右操作数（2×i64）
 * @return a > b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t cmpgt_i64(vec128_t a, vec128_t b) noexcept {
#    if defined(NEFORCE_SIMD_AVX2)
    return ::_mm_cmpgt_epi64(a, b);
#    elif defined(NEFORCE_SIMD_SSE2)
    const auto* sa = reinterpret_cast<const int64_t*>(&a);
    const auto* sb = reinterpret_cast<const int64_t*>(&b);
    alignas(16) int64_t rd[2];
    rd[0] = sa[0] > sb[0] ? -1LL : 0LL;
    rd[1] = sa[1] > sb[1] ? -1LL : 0LL;
    return ::_mm_load_si128(reinterpret_cast<const ::__m128i*>(rd));
#    else
    return scalar::cmpgt_i64(a, b);
#    endif
}

/**
 * @brief 4 路单精度浮点相等比较
 * @param a 左操作数（4×f32）
 * @param b 右操作数（4×f32）
 * @return a == b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128f_t cmpeq_f32(vec128f_t a, vec128f_t b) noexcept {
#    if defined(NEFORCE_SIMD_SSE2)
    return ::_mm_cmpeq_ps(a, b);
#    else
    return scalar::cmpeq_f32(a, b);
#    endif
}

/**
 * @brief 4 路单精度浮点大于比较
 * @param a 左操作数（4×f32）
 * @param b 右操作数（4×f32）
 * @return a > b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128f_t cmpgt_f32(vec128f_t a, vec128f_t b) noexcept {
#    if defined(NEFORCE_SIMD_SSE2)
    return ::_mm_cmpgt_ps(a, b);
#    else
    return scalar::cmpgt_f32(a, b);
#    endif
}

/**
 * @brief 4 路单精度浮点大于等于比较
 * @param a 左操作数（4×f32）
 * @param b 右操作数（4×f32）
 * @return a >= b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128f_t cmpge_f32(vec128f_t a, vec128f_t b) noexcept {
#    if defined(NEFORCE_SIMD_SSE2)
    return ::_mm_cmpge_ps(a, b);
#    else
    return scalar::cmpge_f32(a, b);
#    endif
}

/**
 * @brief 4 路单精度浮点小于比较
 * @param a 左操作数（4×f32）
 * @param b 右操作数（4×f32）
 * @return a < b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128f_t cmplt_f32(vec128f_t a, vec128f_t b) noexcept {
#    if defined(NEFORCE_SIMD_SSE2)
    return ::_mm_cmplt_ps(a, b);
#    else
    return scalar::cmplt_f32(a, b);
#    endif
}

/**
 * @brief 4 路单精度浮点小于等于比较
 * @param a 左操作数（4×f32）
 * @param b 右操作数（4×f32）
 * @return a <= b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128f_t cmple_f32(vec128f_t a, vec128f_t b) noexcept {
#    if defined(NEFORCE_SIMD_SSE2)
    return ::_mm_cmple_ps(a, b);
#    else
    return scalar::cmple_f32(a, b);
#    endif
}

/**
 * @brief 2 路双精度浮点相等比较
 * @param a 左操作数（2×f64）
 * @param b 右操作数（2×f64）
 * @return a == b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128d_t cmpeq_f64(vec128d_t a, vec128d_t b) noexcept {
#    if defined(NEFORCE_SIMD_SSE2)
    return ::_mm_cmpeq_pd(a, b);
#    else
    return scalar::cmpeq_f64(a, b);
#    endif
}

/**
 * @brief 2 路双精度浮点大于比较
 * @param a 左操作数（2×f64）
 * @param b 右操作数（2×f64）
 * @return a > b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128d_t cmpgt_f64(vec128d_t a, vec128d_t b) noexcept {
#    if defined(NEFORCE_SIMD_SSE2)
    return ::_mm_cmpgt_pd(a, b);
#    else
    return scalar::cmpgt_f64(a, b);
#    endif
}

/**
 * @brief 2 路双精度浮点大于等于比较
 * @param a 左操作数（2×f64）
 * @param b 右操作数（2×f64）
 * @return a >= b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128d_t cmpge_f64(vec128d_t a, vec128d_t b) noexcept {
#    if defined(NEFORCE_SIMD_SSE2)
    return ::_mm_cmpge_pd(a, b);
#    else
    return scalar::cmpge_f64(a, b);
#    endif
}

/**
 * @brief 2 路双精度浮点小于比较
 * @param a 左操作数（2×f64）
 * @param b 右操作数（2×f64）
 * @return a < b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128d_t cmplt_f64(vec128d_t a, vec128d_t b) noexcept {
#    if defined(NEFORCE_SIMD_SSE2)
    return ::_mm_cmplt_pd(a, b);
#    else
    return scalar::cmplt_f64(a, b);
#    endif
}

/**
 * @brief 2 路双精度浮点小于等于比较
 * @param a 左操作数（2×f64）
 * @param b 右操作数（2×f64）
 * @return a <= b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128d_t cmple_f64(vec128d_t a, vec128d_t b) noexcept {
#    if defined(NEFORCE_SIMD_SSE2)
    return ::_mm_cmple_pd(a, b);
#    else
    return scalar::cmple_f64(a, b);
#    endif
}

/** @} */ // SIMD

NEFORCE_END_SIMD__
NEFORCE_END_NAMESPACE__
#endif
#endif // NEFORCE_CORE_SIMD_COMPARE_X86_HPP__
