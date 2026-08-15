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

#if defined(NEFORCE_SIMD_AVX512F)
    while (count >= 64) {
        storeu_si512(d, loadu_si512(s));
        d += 64;
        s += 64;
        count -= 64;
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
    while (count >= 64) {
        storeu_si512(d, loadu_si512(s));
        d += 64;
        s += 64;
        count -= 64;
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
#if defined(NEFORCE_SIMD_AVX512F)
        while (count >= 64) {
            storeu_si512(d, loadu_si512(s));
            d += 64;
            s += 64;
            count -= 64;
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
#if defined(NEFORCE_COMPILER_MSVC) && defined(NEFORCE_ARCH_X86)
    if (count >= 4096) {
        ::__stosb(d, value, count);
        return dest;
    }
#endif
#if defined(NEFORCE_SIMD_AVX2)
    const vec256_t pattern256 = ::_mm256_set1_epi8(static_cast<char>(value));
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
    while (count >= 32) {
        const vec256_t v = loadu_si256(p);
        const int mask = ::_mm256_movemask_epi8(::_mm256_cmpeq_epi8(v, target256));
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

NEFORCE_END_INNER__


/**
 * @brief 计算字符串长度
 * @tparam CharT 字符类型
 * @param str 字符串指针
 * @return 字符串长度，str 为 nullptr 时返回 0
 */
template <typename CharT>
NEFORCE_PURE_FUNCTION NEFORCE_ALWAYS_INLINE_INLINE size_t string_length(const CharT* str) noexcept {
    static_assert(sizeof(CharT) <= 4, "string_length SIMD requires CharT size <= 4");
    if (str == nullptr) {
        return 0;
    }

    const auto* p = reinterpret_cast<const byte_t*>(str);
    const vec128_t zero = simd::fill_i(CharT(0));
    const int stride = static_cast<int>(sizeof(CharT));
    size_t char_offset = 0;

    while (true) {
        const vec128_t v = loadu_si128(p);
        const vec128_t eq = inner::match_lanes<sizeof(CharT)>(v, zero);
        const int mask = to_bitmask(eq);
        if (mask != 0) {
            return char_offset + static_cast<size_t>(countr_zero(static_cast<unsigned>(mask)) / stride);
        }
        p += 16;
        char_offset += 16 / stride;
    }
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

    const auto* p = reinterpret_cast<const byte_t*>(str);
    const vec128_t target = simd::fill_i(chr);
    const vec128_t zero = simd::fill_i(CharT(0));
    const int stride = static_cast<int>(sizeof(CharT));
    size_t char_offset = 0;

    while (true) {
        const vec128_t v = loadu_si128(p);
        int mask_target = simd::to_bitmask(inner::match_lanes<sizeof(CharT)>(v, target));
        const int mask_zero = simd::to_bitmask(inner::match_lanes<sizeof(CharT)>(v, zero));

        if (mask_zero != 0) {
            const int first_null_bit = countr_zero(static_cast<unsigned>(mask_zero));
            if ((mask_target & (1 << first_null_bit)) != 0) {
                return str + char_offset + (first_null_bit / stride);
            }
            mask_target &= (1 << first_null_bit) - 1;
        }

        if (mask_target != 0) {
            return str + char_offset + (countr_zero(static_cast<unsigned>(mask_target)) / stride);
        }

        if (mask_zero != 0) {
            return nullptr;
        }

        p += 16;
        char_offset += 16 / stride;
    }
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

    const auto* p = reinterpret_cast<const byte_t*>(str);
    const vec128_t target = fill_i(chr);
    const vec128_t zero = fill_i(CharT(0));
    const int stride = static_cast<int>(sizeof(CharT));
    const size_t byte_limit = count * stride;
    size_t offset = 0;

    while (offset < byte_limit) {
        const vec128_t v = loadu_si128(p + offset);
        const size_t remaining = byte_limit - offset;

        int mask_target = simd::to_bitmask(inner::match_lanes<sizeof(CharT)>(v, target));
        const int mask_zero = simd::to_bitmask(inner::match_lanes<sizeof(CharT)>(v, zero));

        if (remaining < 16) {
            const int keep_mask = (1 << remaining) - 1;
            mask_target &= keep_mask;
        }

        if (mask_zero != 0) {
            const int first_null_bit = countr_zero(static_cast<unsigned>(mask_zero));
            if (first_null_bit < remaining) {
                if ((mask_target & (1 << first_null_bit)) != 0) {
                    return str + (offset / stride) + (first_null_bit / stride);
                }
                mask_target &= (1 << first_null_bit) - 1;
            }
        }

        if (mask_target != 0) {
            return str + (offset / stride) + (countr_zero(static_cast<unsigned>(mask_target)) / stride);
        }

        if (mask_zero != 0) {
            const int first_null_bit = countr_zero(static_cast<unsigned>(mask_zero));
            if (first_null_bit < remaining) {
                break;
            }
        }

        offset += 16;
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

    const auto* p1 = reinterpret_cast<const byte_t*>(s1);
    const auto* p2 = reinterpret_cast<const byte_t*>(s2);
    const vec128_t zero = fill_i(CharT(0));
    const int stride = static_cast<int>(sizeof(CharT));

    while (true) {
        const vec128_t v1 = loadu_si128(p1);
        const vec128_t v2 = loadu_si128(p2);
        const vec128_t eq = inner::match_lanes<sizeof(CharT)>(v1, v2);
        const vec128_t z1 = inner::match_lanes<sizeof(CharT)>(v1, zero);
        const vec128_t z2 = inner::match_lanes<sizeof(CharT)>(v2, zero);

        const int eq_mask = to_bitmask(eq);
        const int null_mask = to_bitmask(z1) | to_bitmask(z2);

        const int diff_mask = (~eq_mask) | null_mask;

        if ((diff_mask & 0xFFFF) != 0) {
            const int first_byte = countr_zero(static_cast<unsigned>(diff_mask));
            const int lane = first_byte / stride;

            const auto* cs1 = reinterpret_cast<const CharT*>(p1);
            const auto* cs2 = reinterpret_cast<const CharT*>(p2);
            const CharT c1 = cs1[lane];
            const CharT c2 = cs2[lane];

            if (c1 != c2) {
                return c1 < c2 ? -1 : 1;
            }
            return 0;
        }

        p1 += 16;
        p2 += 16;
    }
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

    const auto* p1 = reinterpret_cast<const byte_t*>(s1);
    const auto* p2 = reinterpret_cast<const byte_t*>(s2);
    const vec128_t zero = simd::fill_i(CharT(0));
    const int stride = static_cast<int>(sizeof(CharT));
    const size_t byte_limit = count * stride;
    size_t offset = 0;

    while (offset < byte_limit) {
        const vec128_t v1 = loadu_si128(p1 + offset);
        const vec128_t v2 = loadu_si128(p2 + offset);
        const vec128_t eq = inner::match_lanes<sizeof(CharT)>(v1, v2);
        const vec128_t z1 = inner::match_lanes<sizeof(CharT)>(v1, zero);
        const vec128_t z2 = inner::match_lanes<sizeof(CharT)>(v2, zero);

        int eq_mask = to_bitmask(eq);
        int null_mask = to_bitmask(z1) | to_bitmask(z2);

        const size_t remaining = byte_limit - offset;
        if (remaining < 16) {
            const int keep_mask = (1 << remaining) - 1;
            eq_mask |= ~keep_mask;
            null_mask &= keep_mask;
        }

        if (null_mask != 0 || (eq_mask & 0xFFFF) != 0xFFFF) {
            const auto* cs1 = reinterpret_cast<const CharT*>(p1 + offset);
            const auto* cs2 = reinterpret_cast<const CharT*>(p2 + offset);
            const size_t lanes = remaining < 16 ? remaining / stride : static_cast<size_t>(16 / stride);
            for (size_t i = 0; i < lanes; ++i) {
                if (cs1[i] != cs2[i]) {
                    return cs1[i] < cs2[i] ? -1 : 1;
                }
                if (cs1[i] == CharT(0)) {
                    return 0;
                }
            }
            if (remaining < 16) {
                return 0;
            }
        }

        offset += 16;
    }
    return 0;
}

/** @} */ // SIMD

NEFORCE_END_SIMD__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_SIMD_MEMORY_HPP__
