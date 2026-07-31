#ifndef NEFORCE_CORE_SIMD_MEMORY_HPP__
#define NEFORCE_CORE_SIMD_MEMORY_HPP__

/**
 * @file memory.hpp
 * @brief 跨平台 SIMD 内存操作
 *
 * 提供对齐/非对齐加载与存储、流式访问及软件预取操作。
 */

#include "NeForce/core/simd/types.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_SIMD__

/**
 * @addtogroup SIMD
 * @{
 */

/**
 * @brief 对齐加载 128-bit 向量
 * @param ptr 16 字节对齐的内存地址
 * @return 加载的向量
 * @warning ptr 必须 16 字节对齐，否则为未定义行为
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t load_aligned(const void* ptr) noexcept {
#ifdef NEFORCE_SIMD_SSE2
    return ::_mm_load_si128(static_cast<const vec128_t*>(ptr));
#elif defined(NEFORCE_SIMD_NEON)
    return vld1q_u8(static_cast<const uint8_t*>(ptr));
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
 * @brief 非对齐加载 128-bit 向量
 * @param ptr 源内存地址
 * @return 加载的向量
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t loadu_si128(const void* ptr) noexcept {
#ifdef NEFORCE_SIMD_SSE2
    return ::_mm_loadu_si128(static_cast<const vec128_t*>(ptr));
#elif defined(NEFORCE_SIMD_NEON)
    return vld1q_u8(static_cast<const uint8_t*>(ptr));
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
 * @brief 非对齐加载 256-bit 向量
 * @param ptr 源内存地址
 * @return 加载的向量
 */
NEFORCE_ALWAYS_INLINE_INLINE vec256_t loadu_si256(const void* ptr) noexcept {
#if defined(NEFORCE_SIMD_AVX)
    return ::_mm256_loadu_si256(static_cast<const vec256_t*>(ptr));
#else
    vec256_t result;
    const auto* src = static_cast<const byte_t*>(ptr);
    for (int i = 0; i < 32; ++i) {
        result.data[i] = src[i];
    }
    return result;
#endif
}

/**
 * @brief 非对齐加载 512-bit 向量
 * @param ptr 源内存地址
 * @return 加载的向量
 */
NEFORCE_ALWAYS_INLINE_INLINE vec512_t loadu_si512(const void* ptr) noexcept {
#ifdef NEFORCE_SIMD_AVX512F
    return ::_mm512_loadu_si512(ptr);
#else
    vec512_t result;
    const auto* src = static_cast<const byte_t*>(ptr);
    for (int i = 0; i < 64; ++i) {
        result.data[i] = src[i];
    }
    return result;
#endif
}

/**
 * @brief 非对齐加载单精度浮点向量（128-bit）
 * @param ptr 源内存地址
 * @return 加载的向量
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128f_t loadu_ps(const void* ptr) noexcept {
#ifdef NEFORCE_SIMD_SSE2
    return ::_mm_loadu_ps(static_cast<const float*>(ptr));
#elif defined(NEFORCE_SIMD_NEON)
    return vld1q_f32(static_cast<const float*>(ptr));
#else
    vec128f_t result;
    const auto* src = static_cast<const float*>(ptr);
    for (int i = 0; i < 4; ++i) {
        result.data[i] = src[i];
    }
    return result;
#endif
}

/**
 * @brief 非对齐加载双精度浮点向量（128-bit）
 * @param ptr 源内存地址
 * @return 加载的向量
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128d_t loadu_pd(const void* ptr) noexcept {
#ifdef NEFORCE_SIMD_SSE2
    return ::_mm_loadu_pd(static_cast<const double*>(ptr));
#elif defined(NEFORCE_SIMD_NEON)
    return vld1q_f64(static_cast<const double*>(ptr));
#else
    vec128d_t result;
    const auto* src = static_cast<const double*>(ptr);
    for (int i = 0; i < 2; ++i) {
        result.data[i] = src[i];
    }
    return result;
#endif
}

/**
 * @brief 对齐存储 128-bit 向量
 * @param ptr 16 字节对齐的目标内存地址
 * @param v 待存储的向量
 * @warning ptr 必须 16 字节对齐，否则为未定义行为
 */
NEFORCE_ALWAYS_INLINE_INLINE void store_aligned(void* ptr, vec128_t v) noexcept {
#ifdef NEFORCE_SIMD_SSE2
    ::_mm_store_si128(static_cast<vec128_t*>(ptr), v);
#elif defined(NEFORCE_SIMD_NEON)
    vst1q_u8(static_cast<uint8_t*>(ptr), v);
#else
    auto* dst = static_cast<byte_t*>(ptr);
    for (int i = 0; i < 16; ++i) {
        dst[i] = v.data[i];
    }
#endif
}

/**
 * @brief 非对齐存储 128-bit 向量
 * @param ptr 目标内存地址
 * @param v 待存储的向量
 */
NEFORCE_ALWAYS_INLINE_INLINE void storeu_si128(void* ptr, vec128_t v) noexcept {
#ifdef NEFORCE_SIMD_SSE2
    ::_mm_storeu_si128(static_cast<vec128_t*>(ptr), v);
#elif defined(NEFORCE_SIMD_NEON)
    vst1q_u8(static_cast<uint8_t*>(ptr), v);
#else
    auto* dst = static_cast<byte_t*>(ptr);
    for (int i = 0; i < 16; ++i) {
        dst[i] = v.data[i];
    }
#endif
}

/**
 * @brief 非对齐存储 256-bit 向量
 * @param ptr 目标内存地址
 * @param v 待存储的向量
 */
NEFORCE_ALWAYS_INLINE_INLINE void storeu_si256(void* ptr, vec256_t v) noexcept {
#if defined(NEFORCE_SIMD_AVX)
    ::_mm256_storeu_si256(static_cast<vec256_t*>(ptr), v);
#else
    auto* dst = static_cast<byte_t*>(ptr);
    for (int i = 0; i < 32; ++i) {
        dst[i] = v.data[i];
    }
#endif
}

/**
 * @brief 非对齐存储 512-bit 向量
 * @param ptr 目标内存地址
 * @param v 待存储的向量
 */
NEFORCE_ALWAYS_INLINE_INLINE void storeu_si512(void* ptr, vec512_t v) noexcept {
#ifdef NEFORCE_SIMD_AVX512F
    ::_mm512_storeu_si512(ptr, v);
#else
    auto* dst = static_cast<byte_t*>(ptr);
    for (int i = 0; i < 64; ++i) {
        dst[i] = v.data[i];
    }
#endif
}

/**
 * @brief 流式存储 128-bit 向量，绕过缓存
 * @param ptr 目标内存地址
 * @param v 待存储的向量
 * @note 适用于一次性写入后不会立即读取的大块数据，ptr 建议 16 字节对齐以获得最佳性能
 *
 */
NEFORCE_ALWAYS_INLINE_INLINE void store_stream(void* ptr, vec128_t v) noexcept {
#ifdef NEFORCE_SIMD_SSE2
    ::_mm_stream_si128(static_cast<vec128_t*>(ptr), v);
#elif defined(NEFORCE_SIMD_NEON)
    vst1q_u8(static_cast<uint8_t*>(ptr), v);
#else
    auto* dst = static_cast<byte_t*>(ptr);
    for (int i = 0; i < 16; ++i) {
        dst[i] = v.data[i];
    }
#endif
}

/**
 * @brief 流式存储 256-bit 向量，绕过缓存
 * @param ptr 目标内存地址
 * @param v 待存储的向量
 * @note ptr 建议 32 字节对齐以获得最佳性能
 */
NEFORCE_ALWAYS_INLINE_INLINE void store_stream256(void* ptr, vec256_t v) noexcept {
#if defined(NEFORCE_SIMD_AVX)
    ::_mm256_stream_si256(static_cast<vec256_t*>(ptr), v);
#else
    auto* dst = static_cast<byte_t*>(ptr);
    for (int i = 0; i < 32; ++i) {
        dst[i] = v.data[i];
    }
#endif
}

/**
 * @brief 流式存储 512-bit 向量，绕过缓存
 * @param ptr 目标内存地址
 * @param v 待存储的向量
 * @note ptr 建议 64 字节对齐以获得最佳性能
 */
NEFORCE_ALWAYS_INLINE_INLINE void store_stream512(void* ptr, vec512_t v) noexcept {
#ifdef NEFORCE_SIMD_AVX512F
    ::_mm512_stream_si512(ptr, v);
#else
    auto* dst = static_cast<byte_t*>(ptr);
    for (int i = 0; i < 64; ++i) {
        dst[i] = v.data[i];
    }
#endif
}

/**
 * @brief 流式加载 128-bit 向量
 * @param ptr 源内存地址
 * @return 加载的向量
 * @note 适用于一次性读取后不会再次访问的数据流，ptr 建议 16 字节对齐以获得最佳性能
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t load_stream(const void* ptr) noexcept {
#if defined(NEFORCE_SIMD_SSE4_1)
    return ::_mm_stream_load_si128(const_cast<vec128_t*>(static_cast<const vec128_t*>(ptr)));
#else
    return loadu_si128(ptr);
#endif
}

/**
 * @brief 预取数据到所有缓存层级以供读取
 * @param ptr 预取的内存地址
 */
NEFORCE_ALWAYS_INLINE_INLINE void prefetch_read(const void* ptr) noexcept {
#ifdef NEFORCE_SIMD_SSE2
    _mm_prefetch(static_cast<const char*>(ptr), _MM_HINT_T0);
#elif defined(NEFORCE_SIMD_NEON)
    __builtin_prefetch(ptr, 0, 3);
#endif
}

/**
 * @brief 预取数据到所有缓存层级以供写入
 * @param ptr 预取的内存地址
 */
NEFORCE_ALWAYS_INLINE_INLINE void prefetch_write(const void* ptr) noexcept {
#ifdef NEFORCE_SIMD_SSE2
    _mm_prefetch(static_cast<const char*>(ptr), _MM_HINT_T0);
#elif defined(NEFORCE_SIMD_NEON)
    __builtin_prefetch(ptr, 1, 3);
#endif
}

/**
 * @brief 预取数据到 L1 缓存以供读取
 * @param ptr 预取的内存地址
 */
NEFORCE_ALWAYS_INLINE_INLINE void prefetch_l1(const void* ptr) noexcept {
#ifdef NEFORCE_SIMD_SSE2
    _mm_prefetch(static_cast<const char*>(ptr), _MM_HINT_T0);
#elif defined(NEFORCE_SIMD_NEON)
    __builtin_prefetch(ptr, 0, 3);
#else
    (void) ptr;
#endif
}

/**
 * @brief 预取数据到 L2 缓存
 * @param ptr 预取的内存地址
 */
NEFORCE_ALWAYS_INLINE_INLINE void prefetch_l2(const void* ptr) noexcept {
#ifdef NEFORCE_SIMD_SSE2
    _mm_prefetch(static_cast<const char*>(ptr), _MM_HINT_T1);
#elif defined(NEFORCE_SIMD_NEON)
    __builtin_prefetch(ptr, 0, 2);
#else
    (void) ptr;
#endif
}

/**
 * @brief 预取数据到缓存槽
 * @param ptr 预取的内存地址
 * @note 适用于仅访问一次的大块数据流式读取
 */
NEFORCE_ALWAYS_INLINE_INLINE void prefetch_nta(const void* ptr) noexcept {
#ifdef NEFORCE_SIMD_SSE2
    _mm_prefetch(static_cast<const char*>(ptr), _MM_HINT_NTA);
#elif defined(NEFORCE_SIMD_NEON)
    __builtin_prefetch(ptr, 0, 0);
#else
    (void) ptr;
#endif
}

/** @} */ // SIMD

NEFORCE_END_SIMD__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_SIMD_MEMORY_HPP__
