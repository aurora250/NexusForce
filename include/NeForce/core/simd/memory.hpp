#ifndef NEFORCE_CORE_SIMD_MEMORY_HPP__
#define NEFORCE_CORE_SIMD_MEMORY_HPP__

/**
 * @file memory.hpp
 * @brief 跨平台 SIMD 内存操作
 *
 * 提供对齐/非对齐加载与存储、流式访问及软件预取操作。
 */

#include "NeForce/core/simd/bytes.hpp"
#include "NeForce/core/simd/compare.hpp"
#if defined(NEFORCE_COMPILER_MSVC) && defined(NEFORCE_ARCH_X86)
#    include <intrin.h>
#endif
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_SIMD__

/**
 * @addtogroup SIMD
 * @{
 */

NEFORCE_BEGIN_INNER__

template <size_t Size>
vec128_t match_lanes(vec128_t a, vec128_t b) noexcept;

template <>
NEFORCE_ALWAYS_INLINE_INLINE vec128_t match_lanes<1>(vec128_t a, vec128_t b) noexcept {
    return match_bytes(a, b);
}

template <>
NEFORCE_ALWAYS_INLINE_INLINE vec128_t match_lanes<2>(vec128_t a, vec128_t b) noexcept {
    return cmpeq_i16(a, b);
}

template <>
NEFORCE_ALWAYS_INLINE_INLINE vec128_t match_lanes<4>(vec128_t a, vec128_t b) noexcept {
    return cmpeq_i32(a, b);
}

#if defined(NEFORCE_SIMD_AVX2)
/**
 * @brief 将不超过 4 字节的值广播到 256-bit 向量的全部通道
 * @tparam CharT 值类型，宽度不得超过 4 字节
 * @param value 待广播的值
 * @return 广播后的 256-bit 向量
 */
template <typename CharT>
NEFORCE_ALWAYS_INLINE_INLINE vec256_t fill_lanes256(const CharT value) noexcept {
    static_assert(sizeof(CharT) <= 4, "fill_lanes256 requires value type size <= 4");
    NEFORCE_IF_CONSTEXPR(sizeof(CharT) == 1) {
        return ::_mm256_set1_epi8(static_cast<int8_t>(static_cast<uint8_t>(value)));
    }
    NEFORCE_IF_CONSTEXPR(sizeof(CharT) == 2) {
        return ::_mm256_set1_epi16(static_cast<int16_t>(static_cast<uint16_t>(value)));
    }
    return ::_mm256_set1_epi32(static_cast<int32_t>(static_cast<uint32_t>(value)));
}

/**
 * @brief 提取 256-bit 向量中每个字节的最高位，组成 32-bit 掩码
 * @param v 输入向量
 * @return 32-bit 掩码，bit i 对应字节 i 的最高位
 */
NEFORCE_ALWAYS_INLINE_INLINE int to_bitmask256(const vec256_t v) noexcept { return ::_mm256_movemask_epi8(v); }

/**
 * @brief 按通道宽度匹配 256-bit 向量
 * @tparam Size 通道字节宽度，仅支持 1、2、4
 * @param a 左侧向量
 * @param b 右侧向量
 * @return 通道相等的位置置全 1 的向量
 */
template <size_t Size>
vec256_t match_lanes256(vec256_t a, vec256_t b) noexcept;

template <>
NEFORCE_ALWAYS_INLINE_INLINE vec256_t match_lanes256<1>(const vec256_t a, const vec256_t b) noexcept {
    return ::_mm256_cmpeq_epi8(a, b);
}

template <>
NEFORCE_ALWAYS_INLINE_INLINE vec256_t match_lanes256<2>(const vec256_t a, const vec256_t b) noexcept {
    return ::_mm256_cmpeq_epi16(a, b);
}

template <>
NEFORCE_ALWAYS_INLINE_INLINE vec256_t match_lanes256<4>(const vec256_t a, const vec256_t b) noexcept {
    return ::_mm256_cmpeq_epi32(a, b);
}

/**
 * @brief 从 256-bit 掩码中取出最低置位的位置
 * @param mask 通道掩码
 * @param base 该掩码对应的字节起始偏移
 * @return 首个匹配位置的字节偏移，无匹配时返回值不可用
 */
NEFORCE_ALWAYS_INLINE_INLINE size_t first_match_offset(const int mask, const size_t base) noexcept {
    return base + static_cast<size_t>(countr_zero(static_cast<unsigned>(mask)));
}
#endif // NEFORCE_SIMD_AVX2

NEFORCE_END_INNER__

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


/**
 * @brief 大块内存拷贝
 * @param dest 目标内存指针
 * @param src 源内存指针
 * @param count 字节数
 * @return 目标指针
 * @note 仅在长度达到阈值时调用；使用 ERMS 批量传送指令，调用方需保证范围合法。
 */
NEFORCE_NOINLINE NEFORCE_COLD inline void* memory_copy_large(void* dest, const void* src, const size_t count) noexcept {
    auto* d = static_cast<byte_t*>(dest);
    const auto* s = static_cast<const byte_t*>(src);
#if defined(NEFORCE_SIMD_AVX2)
    const size_t head = (32U - (reinterpret_cast<uintptr_t>(d) & 31U)) & 31U;
    size_t remaining = count;
    if (head != 0 && head <= remaining) {
        storeu_si256(d, loadu_si256(s));
        d += head;
        s += head;
        remaining -= head;
    }
#else
    size_t remaining = count;
#endif
#if defined(NEFORCE_COMPILER_MSVC) && defined(NEFORCE_ARCH_X86)
    ::__movsb(d, s, remaining);
#elif defined(NEFORCE_COMPILER_GNUC) && defined(NEFORCE_ARCH_X86) && !defined(NEFORCE_HAS_ADDRESS_SANITIZER) && \
        !defined(NEFORCE_HAS_MEMORY_SANITIZER) && !defined(NEFORCE_HAS_THREAD_SANITIZER)
    __asm__ __volatile__("rep movsb" : "+D"(d), "+S"(s), "+c"(remaining) : : "memory");
#else
    for (size_t i = 0; i < remaining; ++i) {
        d[i] = s[i];
    }
#endif
    return dest;
}

/**
 * @brief 大块内存填充
 * @param dest 目标内存指针
 * @param value 填充字节值
 * @param count 字节数
 * @return 目标指针
 * @note 仅在长度达到阈值时调用；使用 ERMS 批量传送指令，调用方需保证范围合法。
 */
NEFORCE_NOINLINE NEFORCE_COLD inline void* memory_set_large(void* dest, const byte_t value,
                                                            const size_t count) noexcept {
    auto* d = static_cast<byte_t*>(dest);
#if defined(NEFORCE_COMPILER_MSVC) && defined(NEFORCE_ARCH_X86)
    ::__stosb(d, value, count);
#elif defined(NEFORCE_COMPILER_GNUC) && defined(NEFORCE_ARCH_X86) && !defined(NEFORCE_HAS_ADDRESS_SANITIZER) && \
        !defined(NEFORCE_HAS_MEMORY_SANITIZER) && !defined(NEFORCE_HAS_THREAD_SANITIZER)
    size_t remaining = count;
    __asm__ __volatile__("rep stosb" : "+D"(d), "+c"(remaining) : "a"(value) : "memory");
#else
    for (size_t i = 0; i < count; ++i) {
        d[i] = value;
    }
#endif
    return dest;
}

/**
 * @brief 内存拷贝
 * @param dest 目标内存指针
 * @param src 源内存指针
 * @param count 字节数
 * @return 目标指针，参数无效时返回 nullptr
 */
NEFORCE_ALWAYS_INLINE_INLINE void* memory_copy(void* dest, const void* src, size_t count) noexcept {
    if (dest == nullptr || src == nullptr) {
        return nullptr;
    }
    if (count == 0) {
        return dest;
    }

    auto* d = static_cast<byte_t*>(dest);
    const auto* s = static_cast<const byte_t*>(src);
    const size_t total = count;

#if (defined(NEFORCE_COMPILER_MSVC) && defined(NEFORCE_ARCH_X86)) ||                                               \
        (defined(NEFORCE_COMPILER_GNUC) && defined(NEFORCE_ARCH_X86) && !defined(NEFORCE_HAS_ADDRESS_SANITIZER) && \
         !defined(NEFORCE_HAS_MEMORY_SANITIZER) && !defined(NEFORCE_HAS_THREAD_SANITIZER))
    if (count >= 4096) {
        return memory_copy_large(dest, src, count);
    }
#endif

#if defined(NEFORCE_SIMD_AVX512F)
    while (count >= 256) {
        storeu_si512(d, loadu_si512(s));
        storeu_si512(d + 64, loadu_si512(s + 64));
        storeu_si512(d + 128, loadu_si512(s + 128));
        storeu_si512(d + 192, loadu_si512(s + 192));
        d += 256;
        s += 256;
        count -= 256;
    }
    while (count >= 64) {
        storeu_si512(d, loadu_si512(s));
        d += 64;
        s += 64;
        count -= 64;
    }
#endif
#if defined(NEFORCE_SIMD_AVX2)
    while (count >= 128) {
        storeu_si256(d, loadu_si256(s));
        storeu_si256(d + 32, loadu_si256(s + 32));
        storeu_si256(d + 64, loadu_si256(s + 64));
        storeu_si256(d + 96, loadu_si256(s + 96));
        d += 128;
        s += 128;
        count -= 128;
    }
#endif
#if defined(NEFORCE_SIMD_AVX)
    while (count >= 32) {
        storeu_si256(d, loadu_si256(s));
        d += 32;
        s += 32;
        count -= 32;
    }
#endif
#if defined(NEFORCE_SIMD_AVX2)
    if (count != 0 && total >= 32) {
        storeu_si256(static_cast<byte_t*>(dest) + (total - 32),
                     loadu_si256(static_cast<const byte_t*>(src) + (total - 32)));
        return dest;
    }
#endif
    while (count >= 16) {
        storeu_si128(d, loadu_si128(s));
        d += 16;
        s += 16;
        count -= 16;
    }
    if (count >= 8) {
        d[0] = s[0];
        d[1] = s[1];
        d[2] = s[2];
        d[3] = s[3];
        d[4] = s[4];
        d[5] = s[5];
        d[6] = s[6];
        d[7] = s[7];
        d += 8;
        s += 8;
        count -= 8;
    }
    if (count >= 4) {
        d[0] = s[0];
        d[1] = s[1];
        d[2] = s[2];
        d[3] = s[3];
        d += 4;
        s += 4;
        count -= 4;
    }
    if (count >= 2) {
        d[0] = s[0];
        d[1] = s[1];
        d += 2;
        s += 2;
        count -= 2;
    }
    if (count != 0) {
        *d = *s;
    }
    return dest;
}

/**
 * @brief 内存拷贝并返回结束位置
 * @param dest 目标内存指针
 * @param src 源内存指针
 * @param count 字节数
 * @return 目标内存复制结束后的下一个位置指针
 */
NEFORCE_ALWAYS_INLINE_INLINE void* memory_copy_offset(void* dest, const void* src, size_t count) noexcept {
    if (dest == nullptr || src == nullptr) {
        return nullptr;
    }
    if (count == 0) {
        return dest;
    }

    auto* d = static_cast<byte_t*>(dest);
    const auto* s = static_cast<const byte_t*>(src);

#if defined(NEFORCE_SIMD_AVX512F)
    while (count >= 256) {
        storeu_si512(d, loadu_si512(s));
        storeu_si512(d + 64, loadu_si512(s + 64));
        storeu_si512(d + 128, loadu_si512(s + 128));
        storeu_si512(d + 192, loadu_si512(s + 192));
        d += 256;
        s += 256;
        count -= 256;
    }
    while (count >= 64) {
        storeu_si512(d, loadu_si512(s));
        d += 64;
        s += 64;
        count -= 64;
    }
#endif
#if defined(NEFORCE_SIMD_AVX2)
    while (count >= 128) {
        storeu_si256(d, loadu_si256(s));
        storeu_si256(d + 32, loadu_si256(s + 32));
        storeu_si256(d + 64, loadu_si256(s + 64));
        storeu_si256(d + 96, loadu_si256(s + 96));
        d += 128;
        s += 128;
        count -= 128;
    }
#endif
#if defined(NEFORCE_SIMD_AVX)
    while (count >= 32) {
        storeu_si256(d, loadu_si256(s));
        d += 32;
        s += 32;
        count -= 32;
    }
#endif
    while (count >= 16) {
        storeu_si128(d, loadu_si128(s));
        d += 16;
        s += 16;
        count -= 16;
    }
    for (size_t i = 0; i < count; ++i) {
        d[i] = s[i];
    }
    return d + count;
}

/**
 * @brief 内存比较
 * @param lhs 左侧内存指针
 * @param rhs 右侧内存指针
 * @param count 字节数
 * @return 比较结果：0 相等，正值 lhs>rhs，负值 lhs<rhs
 */
NEFORCE_PURE_FUNCTION NEFORCE_ALWAYS_INLINE_INLINE int memory_compare(const void* lhs, const void* rhs,
                                                                      size_t count) noexcept {
    if (lhs == nullptr && rhs == nullptr) {
        return 0;
    }
    if (lhs == nullptr) {
        return -1;
    }
    if (rhs == nullptr) {
        return 1;
    }
    if (count == 0) {
        return 0;
    }

    const auto* l = static_cast<const byte_t*>(lhs);
    const auto* r = static_cast<const byte_t*>(rhs);

#if defined(NEFORCE_SIMD_AVX2)
    while (count >= 128) {
        const vec256_t a0 = ::_mm256_cmpeq_epi8(loadu_si256(l), loadu_si256(r));
        const vec256_t a1 = ::_mm256_cmpeq_epi8(loadu_si256(l + 32), loadu_si256(r + 32));
        const vec256_t a2 = ::_mm256_cmpeq_epi8(loadu_si256(l + 64), loadu_si256(r + 64));
        const vec256_t a3 = ::_mm256_cmpeq_epi8(loadu_si256(l + 96), loadu_si256(r + 96));
        const int joined =
                inner::to_bitmask256(::_mm256_or_si256(::_mm256_or_si256(a0, a1), ::_mm256_or_si256(a2, a3)));
        if (joined != -1) {
            size_t delta = 0;
            if (inner::to_bitmask256(a0) != -1) {
                delta = inner::first_match_offset(~inner::to_bitmask256(a0), 0);
            } else if (inner::to_bitmask256(a1) != -1) {
                delta = inner::first_match_offset(~inner::to_bitmask256(a1), 32);
            } else if (inner::to_bitmask256(a2) != -1) {
                delta = inner::first_match_offset(~inner::to_bitmask256(a2), 64);
            } else {
                delta = inner::first_match_offset(~inner::to_bitmask256(a3), 96);
            }
            return static_cast<int>(l[delta]) - static_cast<int>(r[delta]);
        }
        l += 128;
        r += 128;
        count -= 128;
    }
    while (count >= 32) {
        const int mask = inner::to_bitmask256(::_mm256_cmpeq_epi8(loadu_si256(l), loadu_si256(r)));
        if (mask != -1) {
            const size_t delta = inner::first_match_offset(~mask, 0);
            return static_cast<int>(l[delta]) - static_cast<int>(r[delta]);
        }
        l += 32;
        r += 32;
        count -= 32;
    }
#endif
    while (count >= 16) {
        const vec128_t vl = loadu_si128(l);
        const vec128_t vr = loadu_si128(r);
        const vec128_t neq = match_bytes(vl, vr);
        const int mask = to_bitmask(neq);
        if (mask != 0xFFFF) {
            const int pos = countr_zero(static_cast<unsigned>(~mask));
            return static_cast<int>(l[pos]) - static_cast<int>(r[pos]);
        }
        l += 16;
        r += 16;
        count -= 16;
    }
    for (size_t i = 0; i < count; ++i) {
        if (l[i] != r[i]) {
            return static_cast<int>(l[i]) - static_cast<int>(r[i]);
        }
    }
    return 0;
}

/**
 * @brief 内存移动
 * @param dest 目标内存指针
 * @param src 源内存指针
 * @param count 字节数
 * @return 目标指针
 */
NEFORCE_ALWAYS_INLINE_INLINE void* memory_move(void* dest, const void* src, size_t count) noexcept {
    if (dest == nullptr || src == nullptr) {
        return nullptr;
    }

    auto* d = static_cast<byte_t*>(dest);
    const auto* s = static_cast<const byte_t*>(src);

    if (d < s) {
#if (defined(NEFORCE_COMPILER_MSVC) && defined(NEFORCE_ARCH_X86)) ||                                               \
        (defined(NEFORCE_COMPILER_GNUC) && defined(NEFORCE_ARCH_X86) && !defined(NEFORCE_HAS_ADDRESS_SANITIZER) && \
         !defined(NEFORCE_HAS_MEMORY_SANITIZER) && !defined(NEFORCE_HAS_THREAD_SANITIZER))
        if (count >= 4096) {
            return memory_copy_large(dest, src, count);
        }
#endif
#if defined(NEFORCE_SIMD_AVX512F)
        while (count >= 256) {
            storeu_si512(d, loadu_si512(s));
            storeu_si512(d + 64, loadu_si512(s + 64));
            storeu_si512(d + 128, loadu_si512(s + 128));
            storeu_si512(d + 192, loadu_si512(s + 192));
            d += 256;
            s += 256;
            count -= 256;
        }
        while (count >= 64) {
            storeu_si512(d, loadu_si512(s));
            d += 64;
            s += 64;
            count -= 64;
        }
#endif
#if defined(NEFORCE_SIMD_AVX2)
        while (count >= 128) {
            storeu_si256(d, loadu_si256(s));
            storeu_si256(d + 32, loadu_si256(s + 32));
            storeu_si256(d + 64, loadu_si256(s + 64));
            storeu_si256(d + 96, loadu_si256(s + 96));
            d += 128;
            s += 128;
            count -= 128;
        }
#endif
#if defined(NEFORCE_SIMD_AVX)
        while (count >= 32) {
            storeu_si256(d, loadu_si256(s));
            d += 32;
            s += 32;
            count -= 32;
        }
#endif
        while (count >= 16) {
            storeu_si128(d, loadu_si128(s));
            d += 16;
            s += 16;
            count -= 16;
        }
        for (; count > 0; --count) {
            *d++ = *s++;
        }
    } else if (d > s) {
        d += count;
        s += count;
#if defined(NEFORCE_SIMD_AVX)
        while (count >= 32) {
            d -= 32;
            s -= 32;
            storeu_si256(d, loadu_si256(s));
            count -= 32;
        }
#endif
        while (count >= 16) {
            d -= 16;
            s -= 16;
            storeu_si128(d, loadu_si128(s));
            count -= 16;
        }
        while (count > 0) {
            --count;
            --d;
            --s;
            *d = *s;
        }
    }
    return dest;
}

/**
 * @brief 内存填充
 * @param dest 目标内存指针
 * @param value 填充字节值
 * @param count 字节数
 * @return 目标指针
 */
NEFORCE_ALWAYS_INLINE_INLINE void* memory_set(void* dest, const byte_t value, size_t count) noexcept {
    if (dest == nullptr) {
        return nullptr;
    }
    if (count == 0) {
        return dest;
    }

    auto* d = static_cast<byte_t*>(dest);
#if (defined(NEFORCE_COMPILER_MSVC) && defined(NEFORCE_ARCH_X86)) ||                                               \
        (defined(NEFORCE_COMPILER_GNUC) && defined(NEFORCE_ARCH_X86) && !defined(NEFORCE_HAS_ADDRESS_SANITIZER) && \
         !defined(NEFORCE_HAS_MEMORY_SANITIZER) && !defined(NEFORCE_HAS_THREAD_SANITIZER))
    if (count >= 4096) {
        return memory_set_large(dest, value, count);
    }
#endif
#if defined(NEFORCE_SIMD_AVX2)
    const vec256_t pattern256 = ::_mm256_set1_epi8(static_cast<char>(value));
    while (count >= 128) {
        storeu_si256(d, pattern256);
        storeu_si256(d + 32, pattern256);
        storeu_si256(d + 64, pattern256);
        storeu_si256(d + 96, pattern256);
        d += 128;
        count -= 128;
    }
    while (count >= 32) {
        storeu_si256(d, pattern256);
        d += 32;
        count -= 32;
    }
#endif
    const vec128_t pattern = fill_i8(value);

    while (count >= 16) {
        storeu_si128(d, pattern);
        d += 16;
        count -= 16;
    }
    if (count >= 8) {
        d[0] = value;
        d[1] = value;
        d[2] = value;
        d[3] = value;
        d[4] = value;
        d[5] = value;
        d[6] = value;
        d[7] = value;
        d += 8;
        count -= 8;
    }
    if (count >= 4) {
        d[0] = value;
        d[1] = value;
        d[2] = value;
        d[3] = value;
        d += 4;
        count -= 4;
    }
    if (count >= 2) {
        d[0] = value;
        d[1] = value;
        d += 2;
        count -= 2;
    }
    if (count != 0) {
        *d = value;
    }
    return dest;
}

/**
 * @brief 内存清零
 * @param dest 目标内存指针
 * @param count 字节数
 */
NEFORCE_ALWAYS_INLINE_INLINE void memory_zero(void* dest, const size_t count) noexcept {
    if (dest == nullptr || count == 0) {
        return;
    }

    auto* d = static_cast<byte_t*>(dest);
    const vec128_t zero = fill_i8(0);
    size_t remaining = count;

    while (remaining >= 16) {
        storeu_si128(d, zero);
        d += 16;
        remaining -= 16;
    }
    for (size_t i = 0; i < remaining; ++i) {
        d[i] = 0;
    }
}

#if defined(NEFORCE_SIMD_AVX2)
/**
 * @brief 大范围字节查找
 * @param p 搜索起始地址，非空
 * @param value 目标字节值
 * @param count 搜索字节数，调用方保证不小于阈值
 * @return 指向首次匹配的指针，未找到返回 nullptr
 *
 * 先把起始地址推进到 32 字节边界，使主体循环全部使用对齐载入；
 * 末尾以回卷块处理不足一个块的剩余部分，全程不越出搜索范围。
 */
NEFORCE_PURE_FUNCTION NEFORCE_NOINLINE inline const void* memory_find_large(const byte_t* p, const byte_t value,
                                                                            const size_t count) noexcept {
    const vec256_t target256 = ::_mm256_set1_epi8(static_cast<char>(value));
    size_t remaining = count;

    const auto misalign = static_cast<size_t>(reinterpret_cast<uintptr_t>(p) & 31U);
    if (misalign != 0) {
        const size_t head = 32 - misalign;
        const int head_mask = inner::to_bitmask256(::_mm256_cmpeq_epi8(loadu_si256(p), target256)) &
                              static_cast<int>((static_cast<uint32_t>(1) << head) - 1);
        if (head_mask != 0) {
            return p + countr_zero(static_cast<unsigned>(head_mask));
        }
        p += head;
        remaining -= head;
    }

    while (remaining >= 128) {
        const vec256_t m0 = ::_mm256_cmpeq_epi8(::_mm256_load_si256(reinterpret_cast<const vec256_t*>(p)), target256);
        const vec256_t m1 =
                ::_mm256_cmpeq_epi8(::_mm256_load_si256(reinterpret_cast<const vec256_t*>(p + 32)), target256);
        const vec256_t m2 =
                ::_mm256_cmpeq_epi8(::_mm256_load_si256(reinterpret_cast<const vec256_t*>(p + 64)), target256);
        const vec256_t m3 =
                ::_mm256_cmpeq_epi8(::_mm256_load_si256(reinterpret_cast<const vec256_t*>(p + 96)), target256);
        const int joined =
                inner::to_bitmask256(::_mm256_or_si256(::_mm256_or_si256(m0, m1), ::_mm256_or_si256(m2, m3)));
        if (joined != 0) {
            const int b0 = inner::to_bitmask256(m0);
            if (b0 != 0) {
                return p + inner::first_match_offset(b0, 0);
            }
            const int b1 = inner::to_bitmask256(m1);
            if (b1 != 0) {
                return p + inner::first_match_offset(b1, 32);
            }
            const int b2 = inner::to_bitmask256(m2);
            if (b2 != 0) {
                return p + inner::first_match_offset(b2, 64);
            }
            return p + inner::first_match_offset(inner::to_bitmask256(m3), 96);
        }
        p += 128;
        remaining -= 128;
    }
    while (remaining >= 32) {
        const int mask = inner::to_bitmask256(
                ::_mm256_cmpeq_epi8(::_mm256_load_si256(reinterpret_cast<const vec256_t*>(p)), target256));
        if (mask != 0) {
            return p + countr_zero(static_cast<unsigned>(mask));
        }
        p += 32;
        remaining -= 32;
    }
    if (remaining != 0) {
        const auto* tail = p + remaining - 32;
        const int tail_mask = inner::to_bitmask256(::_mm256_cmpeq_epi8(loadu_si256(tail), target256)) &
                              static_cast<int>(~((static_cast<uint32_t>(1) << (32 - remaining)) - 1));
        if (tail_mask != 0) {
            return tail + countr_zero(static_cast<unsigned>(tail_mask));
        }
    }
    return nullptr;
}
#endif

/**
 * @brief 内存字节查找
 * @param ptr 搜索起始指针
 * @param value 目标字节值
 * @param count 搜索字节数
 * @return 指向首次匹配的指针，未找到返回 nullptr
 */
NEFORCE_PURE_FUNCTION NEFORCE_ALWAYS_INLINE_INLINE const void* memory_find(const void* ptr, const byte_t value,
                                                                           size_t count) noexcept {
    if (ptr == nullptr || count == 0) {
        return nullptr;
    }

    const auto* p = static_cast<const byte_t*>(ptr);

    if (count < 16) {
        for (size_t i = 0; i < count; ++i) {
            if (p[i] == value) {
                return p + i;
            }
        }
        return nullptr;
    }

#if defined(NEFORCE_SIMD_AVX2)
    const vec256_t target256 = ::_mm256_set1_epi8(static_cast<char>(value));
    if (count >= 256) {
        return memory_find_large(p, value, count);
    }

    while (count >= 128) {
        const vec256_t m0 = ::_mm256_cmpeq_epi8(loadu_si256(p), target256);
        const vec256_t m1 = ::_mm256_cmpeq_epi8(loadu_si256(p + 32), target256);
        const vec256_t m2 = ::_mm256_cmpeq_epi8(loadu_si256(p + 64), target256);
        const vec256_t m3 = ::_mm256_cmpeq_epi8(loadu_si256(p + 96), target256);
        const int joined =
                inner::to_bitmask256(::_mm256_or_si256(::_mm256_or_si256(m0, m1), ::_mm256_or_si256(m2, m3)));
        if (joined != 0) {
            const int b0 = inner::to_bitmask256(m0);
            if (b0 != 0) {
                return p + inner::first_match_offset(b0, 0);
            }
            const int b1 = inner::to_bitmask256(m1);
            if (b1 != 0) {
                return p + inner::first_match_offset(b1, 32);
            }
            const int b2 = inner::to_bitmask256(m2);
            if (b2 != 0) {
                return p + inner::first_match_offset(b2, 64);
            }
            return p + inner::first_match_offset(inner::to_bitmask256(m3), 96);
        }
        p += 128;
        count -= 128;
    }
    while (count >= 32) {
        const int mask = inner::to_bitmask256(::_mm256_cmpeq_epi8(loadu_si256(p), target256));
        if (mask != 0) {
            return p + countr_zero(static_cast<unsigned>(mask));
        }
        p += 32;
        count -= 32;
    }
#endif
    while (count >= 16) {
        const vec128_t v = loadu_si128(p);
        const int offset = find_first_byte(v, value);
        if (offset >= 0) {
            return p + offset;
        }
        p += 16;
        count -= 16;
    }
    for (size_t i = 0; i < count; ++i) {
        if (p[i] == value) {
            return p + i;
        }
    }
    return nullptr;
}
/**
 * @brief 内存模式查找
 * @param data 搜索区域指针
 * @param data_len 搜索区域长度
 * @param pattern 模式指针
 * @param pattern_len 模式长度
 * @return 指向首次匹配的指针，未找到返回 nullptr
 */
NEFORCE_ALWAYS_INLINE_INLINE const void* memory_find_pattern(const void* data, const size_t data_len,
                                                             const void* pattern, const size_t pattern_len) noexcept {
    if (data == nullptr || pattern == nullptr || data_len == 0 || pattern_len == 0 || pattern_len > data_len) {
        return nullptr;
    }

    const auto* data_ptr = static_cast<const byte_t*>(data);
    const auto* pattern_ptr = static_cast<const byte_t*>(pattern);

    if (pattern_len == 1) {
        return memory_find(data, pattern_ptr[0], data_len);
    }

    const byte_t first_byte = pattern_ptr[0];
    const size_t last_possible = data_len - pattern_len + 1;
    const vec128_t target = fill_i8(first_byte);

    size_t pos = 0;
    const byte_t* search_start = data_ptr;

    while (pos + 16 <= last_possible) {
        const vec128_t v = loadu_si128(search_start + pos);
        int bits = to_bitmask(match_bytes(v, target));
        while (bits != 0) {
            const int off = countr_zero(bits);
            const size_t candidate = pos + static_cast<size_t>(off);
            if (candidate >= last_possible) {
                return nullptr;
            }
            bool matched = true;
            for (size_t j = 1; j < pattern_len; ++j) {
                if (data_ptr[candidate + j] != pattern_ptr[j]) {
                    matched = false;
                    break;
                }
            }
            if (matched) {
                return data_ptr + candidate;
            }
            bits &= (bits - 1);
        }
        pos += 16;
    }

    for (; pos < last_possible; ++pos) {
        if (data_ptr[pos] == first_byte) {
            bool matched = true;
            for (size_t j = 1; j < pattern_len; ++j) {
                if (data_ptr[pos + j] != pattern_ptr[j]) {
                    matched = false;
                    break;
                }
            }
            if (matched) {
                return data_ptr + pos;
            }
        }
    }
    return nullptr;
}


/// @cond
NEFORCE_BEGIN_INNER__

/**
 * @struct block_scan_anchor
 * @brief 块扫描的向下对齐锚点
 */
struct block_scan_anchor {
    const byte_t* base;   ///< 向下对齐到扫描块字节数的起始地址
    uint32_t prefix_mask; ///< 首块中位于目标起始之前的车道掩码
    size_t prefix_chars;  ///< 目标起始之前被跳过的字符数
};

/**
 * @brief 计算块扫描锚点
 * @tparam BlockBytes 扫描块字节数，必须为 2 的幂且不超过页大小
 * @tparam CharT 字符类型
 * @param ptr 目标起始地址
 * @return 锚点结构
 *
 * 锚点地址向下对齐到 BlockBytes，因此对齐加载永远不会跨越页边界。
 */
template <size_t BlockBytes = 16, typename CharT>
NEFORCE_ALWAYS_INLINE_INLINE block_scan_anchor anchor_of(const CharT* const ptr) noexcept {
    static_assert(BlockBytes % sizeof(CharT) == 0, "scan block must hold an integral number of characters");
    const auto misalign = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(ptr) & (BlockBytes - 1));

    block_scan_anchor anchor;
    anchor.base = reinterpret_cast<const byte_t*>(reinterpret_cast<uintptr_t>(ptr) - misalign);
    anchor.prefix_mask = ~((static_cast<uint32_t>(1) << misalign) - 1);
    anchor.prefix_chars = misalign / sizeof(CharT);
    return anchor;
}

/**
 * @brief 字符串长度的块扫描部分
 * @tparam CharT 字符类型
 * @param str 起始地址，调用方需保证前若干字符均非终止字符
 * @return 自 str 起至首个终止字符的字符数
 */
template <typename CharT>
NEFORCE_PURE_FUNCTION NEFORCE_NOINLINE size_t string_length_tail(const CharT* str) noexcept {
    const size_t stride = sizeof(CharT);
#if defined(NEFORCE_SIMD_AVX2)
    const block_scan_anchor anchor = anchor_of<32>(str);
    const vec256_t zero256 = fill_lanes256(CharT(0));
    int mask = to_bitmask256(match_lanes256<sizeof(CharT)>(
                       ::_mm256_load_si256(reinterpret_cast<const vec256_t*>(anchor.base)), zero256)) &
               static_cast<int>(anchor.prefix_mask);
    size_t char_offset = 0;

    while (mask == 0) {
        char_offset += 32 / stride;
        mask = to_bitmask256(match_lanes256<sizeof(CharT)>(
                ::_mm256_load_si256(reinterpret_cast<const vec256_t*>(anchor.base + (char_offset * stride))), zero256));
    }
    return char_offset + (static_cast<size_t>(countr_zero(static_cast<unsigned>(mask))) / stride) - anchor.prefix_chars;
#else
    const block_scan_anchor anchor = anchor_of(str);
    const vec128_t zero = fill_i(CharT(0));
    size_t char_offset = 0;

    while (true) {
        const vec128_t v = load_aligned(anchor.base + (char_offset * stride));
        int mask = to_bitmask(match_lanes<sizeof(CharT)>(v, zero));
        if (char_offset == 0) {
            mask &= static_cast<int>(anchor.prefix_mask);
        }
        if (mask != 0) {
            return char_offset + (static_cast<size_t>(countr_zero(static_cast<unsigned>(mask))) / stride) -
                   anchor.prefix_chars;
        }
        char_offset += 16 / stride;
    }
#endif
}

NEFORCE_END_INNER__
/// @endcond


/**
 * @brief 计算字符串长度
 * @tparam CharT 字符类型
 * @param str 字符串指针
 * @return 字符串长度，str 为 nullptr 时返回 0
 * @note 前 8 个字符逐字符扫描：该部分可被编译器整体展开与常量折叠，
 *       因而由字符串字面量调用时可退化为编译期常数；只有更长的字符串才进入 SIMD 块扫描。
 */
template <typename CharT>
NEFORCE_PURE_FUNCTION NEFORCE_ALWAYS_INLINE_INLINE size_t string_length(const CharT* str) noexcept {
    static_assert(sizeof(CharT) <= 4, "string_length SIMD requires CharT size <= 4");
    if (str == nullptr) {
        return 0;
    }

#ifdef NEFORCE_SANITIZED_SCAN
    size_t scalar_length = 0;
    while (str[scalar_length] != CharT(0)) {
        ++scalar_length;
    }
    return scalar_length;
#else
    constexpr size_t prefix_chars = 8;

    for (size_t i = 0; i < prefix_chars; ++i) {
        if (str[i] == CharT(0)) {
            return i;
        }
    }
    return prefix_chars + inner::string_length_tail(str + prefix_chars);
#endif
}

/**
 * @brief 字符串字符查找
 * @tparam CharT 字符类型
 * @param str 字符串指针
 * @param chr 目标字符
 * @return 指向首次匹配的指针，未找到返回 nullptr
 */
template <typename CharT>
NEFORCE_PURE_FUNCTION NEFORCE_ALWAYS_INLINE_INLINE const CharT* string_find(const CharT* str,
                                                                            const CharT chr) noexcept {
    static_assert(sizeof(CharT) <= 4, "string_find SIMD requires CharT size <= 4");
    if (str == nullptr) {
        return nullptr;
    }

#ifdef NEFORCE_SANITIZED_SCAN
    for (const CharT* cursor = str; *cursor != CharT(0); ++cursor) {
        if (*cursor == chr) {
            return cursor;
        }
    }
    return nullptr;
#else
    const size_t stride = sizeof(CharT);
#    if defined(NEFORCE_SIMD_AVX2)
    const inner::block_scan_anchor anchor = inner::anchor_of<32>(str);
    const vec256_t target256 = inner::fill_lanes256(chr);
    const vec256_t zero256 = inner::fill_lanes256(CharT(0));
    size_t char_offset = 0;

    while (true) {
        const vec256_t v = ::_mm256_load_si256(reinterpret_cast<const vec256_t*>(anchor.base + (char_offset * stride)));
        int mask_target = inner::to_bitmask256(inner::match_lanes256<sizeof(CharT)>(v, target256));
        int mask_zero = inner::to_bitmask256(inner::match_lanes256<sizeof(CharT)>(v, zero256));
        if (char_offset == 0) {
            mask_target &= static_cast<int>(anchor.prefix_mask);
            mask_zero &= static_cast<int>(anchor.prefix_mask);
        }

        if (mask_zero != 0) {
            const int first_null_bit = countr_zero(static_cast<unsigned>(mask_zero));
            if ((mask_target & (1 << first_null_bit)) != 0) {
                return str + char_offset + (static_cast<size_t>(first_null_bit) / stride) - anchor.prefix_chars;
            }
            mask_target &= (1 << first_null_bit) - 1;
        }

        if (mask_target != 0) {
            return str + char_offset + (static_cast<size_t>(countr_zero(static_cast<unsigned>(mask_target))) / stride) -
                   anchor.prefix_chars;
        }

        if (mask_zero != 0) {
            return nullptr;
        }

        char_offset += 32 / stride;
    }
#    else
    const inner::block_scan_anchor anchor = inner::anchor_of(str);
    const vec128_t target = simd::fill_i(chr);
    const vec128_t zero = simd::fill_i(CharT(0));
    size_t char_offset = 0;

    while (true) {
        const vec128_t v = load_aligned(anchor.base + (char_offset * stride));
        int mask_target = simd::to_bitmask(inner::match_lanes<sizeof(CharT)>(v, target));
        int mask_zero = simd::to_bitmask(inner::match_lanes<sizeof(CharT)>(v, zero));
        if (char_offset == 0) {
            mask_target &= static_cast<int>(anchor.prefix_mask);
            mask_zero &= static_cast<int>(anchor.prefix_mask);
        }

        if (mask_zero != 0) {
            const int first_null_bit = countr_zero(static_cast<unsigned>(mask_zero));
            if ((mask_target & (1 << first_null_bit)) != 0) {
                return str + char_offset + (static_cast<size_t>(first_null_bit) / stride) - anchor.prefix_chars;
            }
            mask_target &= (1 << first_null_bit) - 1;
        }

        if (mask_target != 0) {
            return str + char_offset + (static_cast<size_t>(countr_zero(static_cast<unsigned>(mask_target))) / stride) -
                   anchor.prefix_chars;
        }

        if (mask_zero != 0) {
            return nullptr;
        }

        char_offset += 16 / stride;
    }
#    endif
#endif
}


/**
 * @brief 前 n 个字符内查找
 * @tparam CharT 字符类型
 * @param str 字符串指针
 * @param chr 目标字符
 * @param count 最大搜索字符数
 * @return 指向首次匹配的指针，未找到返回 nullptr
 */
template <typename CharT>
NEFORCE_PURE_FUNCTION NEFORCE_ALWAYS_INLINE_INLINE const CharT* string_find(const CharT* str, const CharT chr,
                                                                            const size_t count) noexcept {
    static_assert(sizeof(CharT) <= 4, "string_find SIMD requires CharT size <= 4");
    if (str == nullptr || count == 0) {
        return nullptr;
    }

    const auto* const p = reinterpret_cast<const byte_t*>(str);
    const size_t stride = sizeof(CharT);
    const size_t byte_limit = count * stride;
    size_t offset = 0;

#if defined(NEFORCE_SIMD_AVX2)
    const vec256_t target256 = inner::fill_lanes256(chr);
    const vec256_t zero256 = inner::fill_lanes256(CharT(0));

    while (byte_limit - offset >= 32) {
        const vec256_t v = loadu_si256(p + offset);

        int mask_target = inner::to_bitmask256(inner::match_lanes256<sizeof(CharT)>(v, target256));
        const int mask_zero = inner::to_bitmask256(inner::match_lanes256<sizeof(CharT)>(v, zero256));

        if (mask_zero != 0) {
            const int first_null_bit = countr_zero(static_cast<unsigned>(mask_zero));
            if ((mask_target & (1 << first_null_bit)) != 0) {
                return str + ((offset + static_cast<size_t>(first_null_bit)) / stride);
            }
            mask_target &= (1 << first_null_bit) - 1;
        }

        if (mask_target != 0) {
            return str + ((offset + static_cast<size_t>(countr_zero(static_cast<unsigned>(mask_target)))) / stride);
        }

        if (mask_zero != 0) {
            return nullptr;
        }

        offset += 32;
    }
#endif
    const vec128_t target = fill_i(chr);
    const vec128_t zero = fill_i(CharT(0));

    while (byte_limit - offset >= 16) {
        const vec128_t v = loadu_si128(p + offset);

        int mask_target = simd::to_bitmask(inner::match_lanes<sizeof(CharT)>(v, target));
        const int mask_zero = simd::to_bitmask(inner::match_lanes<sizeof(CharT)>(v, zero));

        if (mask_zero != 0) {
            const int first_null_bit = countr_zero(static_cast<unsigned>(mask_zero));
            if ((mask_target & (1 << first_null_bit)) != 0) {
                return str + ((offset + static_cast<size_t>(first_null_bit)) / stride);
            }
            mask_target &= (1 << first_null_bit) - 1;
        }

        if (mask_target != 0) {
            return str + ((offset + static_cast<size_t>(countr_zero(static_cast<unsigned>(mask_target)))) / stride);
        }

        if (mask_zero != 0) {
            return nullptr;
        }

        offset += 16;
    }

    for (size_t i = offset / stride; i < count; ++i) {
        if (str[i] == chr) {
            return str + i;
        }
        if (str[i] == CharT(0)) {
            return nullptr;
        }
    }
    return nullptr;
}

/**
 * @brief 字符串比较
 * @tparam CharT 字符类型
 * @param s1 第一个字符串指针
 * @param s2 第二个字符串指针
 * @return 0 相等，正数 s1 > s2，负数 s1 < s2
 */
template <typename CharT>
NEFORCE_PURE_FUNCTION NEFORCE_ALWAYS_INLINE_INLINE int string_compare(const CharT* s1, const CharT* s2) noexcept {
    static_assert(sizeof(CharT) <= 4, "string_compare SIMD requires CharT size <= 4");
    if (s1 == nullptr && s2 == nullptr) {
        return 0;
    }
    if (s1 == nullptr) {
        return -1;
    }
    if (s2 == nullptr) {
        return 1;
    }

    const size_t length1 = string_length(s1);
    const size_t length2 = string_length(s2);
    const size_t common = (length1 < length2) ? length1 : length2;

    if (common != 0 && memory_compare(s1, s2, common * sizeof(CharT)) != 0) {
        for (size_t i = 0; i < common; ++i) {
            if (s1[i] != s2[i]) {
                return s1[i] < s2[i] ? -1 : 1;
            }
        }
    }

    if (length1 == length2) {
        return 0;
    }
    return (length1 < length2) ? -1 : 1;
}

/**
 * @brief 前 n 个字符内比较
 * @tparam CharT 字符类型
 * @param s1 第一个字符串指针
 * @param s2 第二个字符串指针
 * @param count 最大比较字符数
 * @return 0 相等，正数 s1 > s2，负数 s1 < s2
 */
template <typename CharT>
NEFORCE_PURE_FUNCTION NEFORCE_ALWAYS_INLINE_INLINE int string_compare(const CharT* s1, const CharT* s2,
                                                                      const size_t count) noexcept {
    static_assert(sizeof(CharT) <= 4, "string_compare SIMD requires CharT size <= 4");
    if (s1 == nullptr && s2 == nullptr) {
        return 0;
    }
    if (s1 == nullptr) {
        return -1;
    }
    if (s2 == nullptr) {
        return 1;
    }
    if (count == 0) {
        return 0;
    }

    const auto* const p1 = reinterpret_cast<const byte_t*>(s1);
    const auto* const p2 = reinterpret_cast<const byte_t*>(s2);
    const size_t stride = sizeof(CharT);
    const size_t byte_limit = count * stride;
    size_t offset = 0;

#if defined(NEFORCE_SIMD_AVX2)
    const vec256_t zero256 = inner::fill_lanes256(CharT(0));

    while (byte_limit - offset >= 32) {
        const vec256_t v1 = loadu_si256(p1 + offset);
        const vec256_t v2 = loadu_si256(p2 + offset);

        const int eq_mask = inner::to_bitmask256(inner::match_lanes256<sizeof(CharT)>(v1, v2));
        const int null_mask = inner::to_bitmask256(inner::match_lanes256<sizeof(CharT)>(v1, zero256)) |
                              inner::to_bitmask256(inner::match_lanes256<sizeof(CharT)>(v2, zero256));

        if (null_mask != 0 || eq_mask != -1) {
            const auto* const cs1 = reinterpret_cast<const CharT*>(p1 + offset);
            const auto* const cs2 = reinterpret_cast<const CharT*>(p2 + offset);
            for (size_t i = 0; i < 32 / stride; ++i) {
                if (cs1[i] != cs2[i]) {
                    return cs1[i] < cs2[i] ? -1 : 1;
                }
                if (cs1[i] == CharT(0)) {
                    return 0;
                }
            }
        }

        offset += 32;
    }
#endif
    const vec128_t zero = simd::fill_i(CharT(0));

    while (byte_limit - offset >= 16) {
        const vec128_t v1 = loadu_si128(p1 + offset);
        const vec128_t v2 = loadu_si128(p2 + offset);

        const int eq_mask = to_bitmask(inner::match_lanes<sizeof(CharT)>(v1, v2));
        const int null_mask = to_bitmask(inner::match_lanes<sizeof(CharT)>(v1, zero)) |
                              to_bitmask(inner::match_lanes<sizeof(CharT)>(v2, zero));

        if (null_mask != 0 || (eq_mask & 0xFFFF) != 0xFFFF) {
            const auto* const cs1 = reinterpret_cast<const CharT*>(p1 + offset);
            const auto* const cs2 = reinterpret_cast<const CharT*>(p2 + offset);
            for (size_t i = 0; i < 16 / stride; ++i) {
                if (cs1[i] != cs2[i]) {
                    return cs1[i] < cs2[i] ? -1 : 1;
                }
                if (cs1[i] == CharT(0)) {
                    return 0;
                }
            }
        }

        offset += 16;
    }

    for (size_t i = offset / stride; i < count; ++i) {
        const CharT c1 = s1[i];
        const CharT c2 = s2[i];
        if (c1 != c2) {
            return c1 < c2 ? -1 : 1;
        }
        if (c1 == CharT(0)) {
            return 0;
        }
    }
    return 0;
}

/** @} */ // SIMD

NEFORCE_END_SIMD__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_SIMD_MEMORY_HPP__
