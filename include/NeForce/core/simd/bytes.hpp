#ifndef NEFORCE_CORE_SIMD_BYTES_HPP__
#define NEFORCE_CORE_SIMD_BYTES_HPP__

/**
 * @file bytes.hpp
 * @brief 跨平台 SIMD 字节级操作
 *
 * 提供 128-bit 向量上基于逐字节的广播、加载、比较、搜索、计数、重排与部分存储操作。
 * 实现自动派发至 SSE2 / AVX2 / NEON 或标量回退。
 */

#include "NeForce/core/simd/types.hpp"
#include "NeForce/core/memory/bit.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_SIMD__

/**
 * @addtogroup SIMD
 * @{
 */

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
 * @brief 非对齐加载 16 字节到 128-bit SIMD 向量
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
    return ::vceqq_u8(a, b);
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

/**
 * @brief 检测向量中是否包含指定字节
 * @param v 输入向量
 * @param c 待检测的字节值
 * @return 存在返回 true，否则返回 false
 */
NEFORCE_ALWAYS_INLINE_INLINE bool contains_byte(vec128_t v, byte_t c) noexcept {
#ifdef NEFORCE_SIMD_SSE2
    return ::_mm_movemask_epi8(::_mm_cmpeq_epi8(v, ::_mm_set1_epi8(static_cast<char>(c)))) != 0;
#elif defined(NEFORCE_SIMD_NEON)
    const uint8x16_t match = ::vceqq_u8(v, ::vdupq_n_u8(c));
    const auto* bytes = reinterpret_cast<const byte_t*>(&match);
    for (int i = 0; i < 16; ++i) {
        if (bytes[i] != 0) {
            return true;
        }
    }
    return false;
#else
    for (int i = 0; i < 16; ++i) {
        if (v.data[i] == c) {
            return true;
        }
    }
    return false;
#endif
}

/**
 * @brief 查找向量中指定字节首次出现的位置
 * @param v 输入向量
 * @param c 待查找的字节值
 * @return 首次出现的索引（0-15），未找到返回 -1
 */
NEFORCE_ALWAYS_INLINE_INLINE int find_first_byte(vec128_t v, byte_t c) noexcept {
#ifdef NEFORCE_SIMD_SSE2
    const int mask = ::_mm_movemask_epi8(::_mm_cmpeq_epi8(v, ::_mm_set1_epi8(static_cast<char>(c))));
    if (mask == 0) {
        return -1;
    }
    return countr_zero(mask);
#elif defined(NEFORCE_SIMD_NEON)
    const uint8x16_t match = ::vceqq_u8(v, ::vdupq_n_u8(c));
    const auto* bytes = reinterpret_cast<const byte_t*>(&match);
    for (int i = 0; i < 16; ++i) {
        if (bytes[i] != 0) {
            return i;
        }
    }
    return -1;
#else
    for (int i = 0; i < 16; ++i) {
        if (v.data[i] == c) {
            return i;
        }
    }
    return -1;
#endif
}

/**
 * @brief 查找向量中指定字节末次出现的位置
 * @param v 输入向量
 * @param c 待查找的字节值
 * @return 末次出现的索引（0-15），未找到返回 -1
 */
NEFORCE_ALWAYS_INLINE_INLINE int find_last_byte(vec128_t v, byte_t c) noexcept {
#ifdef NEFORCE_SIMD_SSE2
    const int mask = ::_mm_movemask_epi8(::_mm_cmpeq_epi8(v, ::_mm_set1_epi8(static_cast<char>(c))));
    if (mask == 0) {
        return -1;
    }
#    ifdef NEFORCE_COMPILER_MSVC
    unsigned long idx = 0;
    ::_BitScanReverse(&idx, static_cast<unsigned long>(mask));
    return static_cast<int>(idx);
#    else
    return 31 - __builtin_clz(mask);
#    endif
#elif defined(NEFORCE_SIMD_NEON)
    const uint8x16_t match = ::vceqq_u8(v, ::vdupq_n_u8(c));
    const auto* bytes = reinterpret_cast<const byte_t*>(&match);
    for (int i = 15; i >= 0; --i) {
        if (bytes[i] != 0) {
            return i;
        }
    }
    return -1;
#else
    for (int i = 15; i >= 0; --i) {
        if (v.data[i] == c) {
            return i;
        }
    }
    return -1;
#endif
}

/**
 * @brief 统计向量中指定字节出现的次数
 * @param v 输入向量
 * @param c 待统计的字节值
 * @return 出现次数（0-16）
 */
NEFORCE_ALWAYS_INLINE_INLINE int count_byte(vec128_t v, byte_t c) noexcept {
#ifdef NEFORCE_SIMD_SSE2
    const ::__m128i match = ::_mm_cmpeq_epi8(v, ::_mm_set1_epi8(static_cast<char>(c)));
    const ::__m128i ones = ::_mm_and_si128(match, ::_mm_set1_epi8(1));
    const ::__m128i sums = ::_mm_sad_epu8(ones, ::_mm_setzero_si128());
    return _mm_extract_epi16(sums, 0) + _mm_extract_epi16(sums, 4);
#elif defined(NEFORCE_SIMD_NEON)
    const uint8x16_t match = ::vceqq_u8(v, ::vdupq_n_u8(c));
    const uint8x16_t ones = ::vandq_u8(match, ::vdupq_n_u8(1));
#    ifdef NEFORCE_ARCH_AARCH64
    return static_cast<int>(::vaddvq_u8(ones));
#    else
    int count = 0;
    const auto* bytes = reinterpret_cast<const byte_t*>(&ones);
    for (int i = 0; i < 16; ++i) {
        count += bytes[i];
    }
    return count;
#    endif
#else
    int count = 0;
    for (int i = 0; i < 16; ++i) {
        if (v.data[i] == c) {
            ++count;
        }
    }
    return count;
#endif
}

/**
 * @brief 检测向量是否全为零
 * @param v 输入向量
 * @return 全零返回 true，否则返回 false
 */
NEFORCE_ALWAYS_INLINE_INLINE bool is_all_zero(vec128_t v) noexcept {
#ifdef NEFORCE_SIMD_SSE2
    return ::_mm_movemask_epi8(::_mm_cmpeq_epi8(v, ::_mm_setzero_si128())) == 0xFFFF;
#elif defined(NEFORCE_SIMD_NEON)
    uint64x2_t v64 = ::vreinterpretq_u64_u8(v);
    return (::vgetq_lane_u64(v64, 0) | ::vgetq_lane_u64(v64, 1)) == 0;
#else
    for (int i = 0; i < 16; ++i) {
        if (v.data[i] != 0) {
            return false;
        }
    }
    return true;
#endif
}

/**
 * @brief 检测向量中是否存在零字节
 * @param v 输入向量
 * @return 存在零字节返回 true，否则返回 false
 */
NEFORCE_ALWAYS_INLINE_INLINE bool has_any_zero(vec128_t v) noexcept {
#ifdef NEFORCE_SIMD_SSE2
    return ::_mm_movemask_epi8(::_mm_cmpeq_epi8(v, ::_mm_setzero_si128())) != 0;
#elif defined(NEFORCE_SIMD_NEON)
    const uint8x16_t match = ::vceqq_u8(v, ::vdupq_n_u8(0));
    uint64x2_t v64 = ::vreinterpretq_u64_u8(match);
    return (::vgetq_lane_u64(v64, 0) | ::vgetq_lane_u64(v64, 1)) != 0;
#else
    for (int i = 0; i < 16; ++i) {
        if (v.data[i] == 0) {
            return true;
        }
    }
    return false;
#endif
}

/**
 * @brief 反转向量中 16 字节的顺序
 * @param v 输入向量
 * @return 字节逆序排列的向量
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t reverse_bytes(vec128_t v) noexcept {
#ifdef NEFORCE_SIMD_SSSE3
    const ::__m128i indices = ::_mm_set_epi8(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
    return ::_mm_shuffle_epi8(v, indices);
#elif defined(NEFORCE_SIMD_SSE2)
    const auto* src = reinterpret_cast<const byte_t*>(&v);
    alignas(16) byte_t buf[16];
    for (int i = 0; i < 16; ++i) {
        buf[15 - i] = src[i];
    }
    return ::_mm_loadu_si128(reinterpret_cast<const ::__m128i*>(buf));
#elif defined(NEFORCE_SIMD_NEON)
    return ::vrev64q_u8(::vrev32q_u8(::vrev16q_u8(v)));
#else
    vec128_t result;
    for (int i = 0; i < 16; ++i) {
        result.data[15 - i] = v.data[i];
    }
    return result;
#endif
}

/**
 * @brief 按索引表逐字节重排向量
 * @param v 输入向量
 * @param indices 索引向量，每字节低 4 位为源字节索引（0-15）
 * @return 重排后的向量
 * @note 当某索引字节高 4 位非零（SSE）或 >= 16（NEON）时，对应结果字节置零
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t shuffle_bytes(vec128_t v, vec128_t indices) noexcept {
#ifdef NEFORCE_SIMD_SSSE3
    return ::_mm_shuffle_epi8(v, indices);
#elif defined(NEFORCE_SIMD_SSE2)
    alignas(16) byte_t src[16];
    alignas(16) byte_t idx[16];
    ::_mm_store_si128(reinterpret_cast<::__m128i*>(src), v);
    ::_mm_store_si128(reinterpret_cast<::__m128i*>(idx), indices);
    alignas(16) byte_t result[16] = {};
    for (int i = 0; i < 16; ++i) {
        const int index = idx[i] & 0x0F;
        result[i] = src[index];
    }
    return ::_mm_load_si128(reinterpret_cast<const ::__m128i*>(result));
#elif defined(NEFORCE_SIMD_NEON)
    return ::vqtbl1q_u8(v, ::vandq_u8(indices, ::vdupq_n_u8(0x0F)));
#else
    vec128_t result;
    for (int i = 0; i < 16; ++i) {
        const int index = indices.data[i] & 0x0F;
        result.data[i] = v.data[index];
    }
    return result;
#endif
}

/**
 * @brief 按掩码逐字节混合两个向量
 * @param a 掩码位为 0 时选取的向量
 * @param b 掩码位为 1 时选取的向量
 * @param mask 字节掩码，每字节最高位为 1 则选取 b，否则选取 a
 * @return 混合后的向量
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t blend_bytes(vec128_t a, vec128_t b, vec128_t mask) noexcept {
#ifdef NEFORCE_SIMD_SSE4_1
    return ::_mm_blendv_epi8(a, b, mask);
#elif defined(NEFORCE_SIMD_SSE2)
    const ::__m128i sel = ::_mm_cmpgt_epi8(::_mm_setzero_si128(), mask);
    return ::_mm_or_si128(::_mm_and_si128(sel, b), ::_mm_andnot_si128(sel, a));
#elif defined(NEFORCE_SIMD_NEON)
    const int8x16_t sel = ::vshrq_n_s8(::vreinterpretq_s8_u8(mask), 7);
    return ::vbslq_u8(::vreinterpretq_u8_s8(sel), b, a);
#else
    vec128_t result;
    for (int i = 0; i < 16; ++i) {
        result.data[i] = (mask.data[i] & 0x80) ? b.data[i] : a.data[i];
    }
    return result;
#endif
}

/**
 * @brief 存储向量的前 n 字节到内存，不写入越界数据
 * @param ptr 目标内存地址
 * @param v 源向量
 * @param n 要写入的字节数（0-16）
 * @note 用于需要部分写入的尾部处理场景
 */
NEFORCE_ALWAYS_INLINE_INLINE void store_bytes_n(void* ptr, vec128_t v, int n) noexcept {
#ifdef NEFORCE_SIMD_SSE2
    alignas(16) byte_t buf[16];
    ::_mm_store_si128(reinterpret_cast<::__m128i*>(buf), v);
    for (int i = 0; i < n; ++i) {
        static_cast<byte_t*>(ptr)[i] = buf[i];
    }
#elif defined(NEFORCE_SIMD_NEON)
    alignas(16) byte_t buf[16];
    ::vst1q_u8(buf, v);
    for (int i = 0; i < n; ++i) {
        static_cast<byte_t*>(ptr)[i] = buf[i];
    }
#else
    for (int i = 0; i < n; ++i) {
        static_cast<byte_t*>(ptr)[i] = v.data[i];
    }
#endif
}

/**
 * @brief 从内存加载 n 字节到向量，不足部分填零
 * @param ptr 源内存地址
 * @param n 要读取的字节数（0-16）
 * @return 前 n 字节为源数据、剩余字节为零的向量
 * @note 用于需要部分加载的头部/尾部处理场景
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t load_bytes_n(const void* ptr, int n) noexcept {
#ifdef NEFORCE_SIMD_SSE2
    alignas(16) byte_t buf[16] = {};
    const auto* src = static_cast<const byte_t*>(ptr);
    for (int i = 0; i < n; ++i) {
        buf[i] = src[i];
    }
    return ::_mm_load_si128(reinterpret_cast<const ::__m128i*>(buf));
#elif defined(NEFORCE_SIMD_NEON)
    alignas(16) byte_t buf[16] = {};
    const auto* src = static_cast<const byte_t*>(ptr);
    for (int i = 0; i < n; ++i) {
        buf[i] = src[i];
    }
    return ::vld1q_u8(buf);
#else
    vec128_t result = {};
    const auto* src = static_cast<const byte_t*>(ptr);
    for (int i = 0; i < n; ++i) {
        result.data[i] = src[i];
    }
    return result;
#endif
}

/** @} */ // SIMD

NEFORCE_END_SIMD__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_SIMD_BYTES_HPP__
