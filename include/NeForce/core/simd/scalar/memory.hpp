#ifndef NEFORCE_CORE_SIMD_MEMORY_SCALAR_HPP__
#define NEFORCE_CORE_SIMD_MEMORY_SCALAR_HPP__
#include "NeForce/core/simd/bytes.hpp"
#include "NeForce/core/simd/compare.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_SIMD__
NEFORCE_BEGIN_SCALAR__

/**
 * @addtogroup SIMD
 * @{
 */

/// @cond
NEFORCE_BEGIN_INNER__
template <size_t Size>
vec128_t match_lanes(vec128_t a, vec128_t b) noexcept;
template <>
NEFORCE_ALWAYS_INLINE_INLINE vec128_t match_lanes<1>(vec128_t a, vec128_t b) noexcept {
    return scalar::match_bytes(a, b);
}
template <>
NEFORCE_ALWAYS_INLINE_INLINE vec128_t match_lanes<2>(vec128_t a, vec128_t b) noexcept {
    return scalar::cmpeq_i16(a, b);
}
template <>
NEFORCE_ALWAYS_INLINE_INLINE vec128_t match_lanes<4>(vec128_t a, vec128_t b) noexcept {
    return scalar::cmpeq_i32(a, b);
}
NEFORCE_END_INNER__
/// @endcond


/**
 * @brief 对齐加载 128-bit 向量
 * @param ptr 16 字节对齐的内存地址
 * @return 加载的向量
 * @warning ptr 必须 16 字节对齐，否则为未定义行为
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t load_aligned(const void* ptr) noexcept {
    vec128_t result;
    const auto* src = static_cast<const byte_t*>(ptr);
    for (int i = 0; i < 16; ++i) {
        lane_bytes(result)[i] = src[i];
    }
    return result;
}

/**
 * @brief 非对齐加载 128-bit 向量
 * @param ptr 源内存地址
 * @return 加载的向量
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t loadu_si128(const void* ptr) noexcept {
    vec128_t result;
    const auto* src = static_cast<const byte_t*>(ptr);
    for (int i = 0; i < 16; ++i) {
        lane_bytes(result)[i] = src[i];
    }
    return result;
}

/**
 * @brief 非对齐加载 256-bit 向量
 * @param ptr 源内存地址
 * @return 加载的向量
 */
NEFORCE_ALWAYS_INLINE_INLINE vec256_t loadu_si256(const void* ptr) noexcept {
    vec256_t result;
    const auto* src = static_cast<const byte_t*>(ptr);
    for (int i = 0; i < 32; ++i) {
        lane_bytes(result)[i] = src[i];
    }
    return result;
}

/**
 * @brief 非对齐加载 512-bit 向量
 * @param ptr 源内存地址
 * @return 加载的向量
 */
NEFORCE_ALWAYS_INLINE_INLINE vec512_t loadu_si512(const void* ptr) noexcept {
    vec512_t result;
    const auto* src = static_cast<const byte_t*>(ptr);
    for (int i = 0; i < 64; ++i) {
        lane_bytes(result)[i] = src[i];
    }
    return result;
}

/**
 * @brief 非对齐加载单精度浮点向量（128-bit）
 * @param ptr 源内存地址
 * @return 加载的向量
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128f_t loadu_ps(const void* ptr) noexcept {
    vec128f_t result;
    const auto* src = static_cast<const float*>(ptr);
    for (int i = 0; i < 4; ++i) {
        lane_view<float>(result)[i] = src[i];
    }
    return result;
}

/**
 * @brief 非对齐加载双精度浮点向量（128-bit）
 * @param ptr 源内存地址
 * @return 加载的向量
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128d_t loadu_pd(const void* ptr) noexcept {
    vec128d_t result;
    const auto* src = static_cast<const double*>(ptr);
    for (int i = 0; i < 2; ++i) {
        lane_view<double>(result)[i] = src[i];
    }
    return result;
}

/**
 * @brief 对齐存储 128-bit 向量
 * @param ptr 16 字节对齐的目标内存地址
 * @param v 待存储的向量
 * @warning ptr 必须 16 字节对齐，否则为未定义行为
 */
NEFORCE_ALWAYS_INLINE_INLINE void store_aligned(void* ptr, vec128_t v) noexcept {
    auto* dst = static_cast<byte_t*>(ptr);
    for (int i = 0; i < 16; ++i) {
        dst[i] = lane_bytes(v)[i];
    }
}

/**
 * @brief 非对齐存储 128-bit 向量
 * @param ptr 目标内存地址
 * @param v 待存储的向量
 */
NEFORCE_ALWAYS_INLINE_INLINE void storeu_si128(void* ptr, vec128_t v) noexcept {
    auto* dst = static_cast<byte_t*>(ptr);
    for (int i = 0; i < 16; ++i) {
        dst[i] = lane_bytes(v)[i];
    }
}

/**
 * @brief 非对齐加载 64 位
 * @param ptr 源内存地址
 * @return 加载的低 64 位向量，高 64 位为零
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t loadl_epi64(const void* ptr) noexcept {
    vec128_t result;
    auto* dst = reinterpret_cast<byte_t*>(&result);
    const auto* src = static_cast<const byte_t*>(ptr);
    for (int i = 0; i < 8; ++i) {
        dst[i] = src[i];
    }
    for (int i = 8; i < 16; ++i) {
        dst[i] = static_cast<byte_t>(0);
    }
    return result;
}

/**
 * @brief 非对齐存储 64 位
 * @param ptr 目标内存地址
 * @param v 待存储的向量，仅低 64 位被写入
 */
NEFORCE_ALWAYS_INLINE_INLINE void storel_epi64(void* ptr, vec128_t v) noexcept {
    auto* dst = static_cast<byte_t*>(ptr);
    const auto* src = reinterpret_cast<const byte_t*>(&v);
    for (int i = 0; i < 8; ++i) {
        dst[i] = src[i];
    }
}

/**
 * @brief 非对齐加载 32 位
 * @param ptr 源内存地址
 * @return 加载的低 32 位向量，其余位为零
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t loadu_si32(const void* ptr) noexcept {
    vec128_t result;
    auto* dst = reinterpret_cast<byte_t*>(&result);
    const auto* src = static_cast<const byte_t*>(ptr);
    for (int i = 0; i < 4; ++i) {
        dst[i] = src[i];
    }
    for (int i = 4; i < 16; ++i) {
        dst[i] = static_cast<byte_t>(0);
    }
    return result;
}

/**
 * @brief 非对齐存储 32 位
 * @param ptr 目标内存地址
 * @param v 待存储的向量，仅低 32 位被写入
 */
NEFORCE_ALWAYS_INLINE_INLINE void storeu_si32(void* ptr, vec128_t v) noexcept {
    auto* dst = static_cast<byte_t*>(ptr);
    const auto* src = reinterpret_cast<const byte_t*>(&v);
    for (int i = 0; i < 4; ++i) {
        dst[i] = src[i];
    }
}

/**
 * @brief 非对齐存储 256-bit 向量
 * @param ptr 目标内存地址
 * @param v 待存储的向量
 */
NEFORCE_ALWAYS_INLINE_INLINE void storeu_si256(void* ptr, vec256_t v) noexcept {
    auto* dst = static_cast<byte_t*>(ptr);
    for (int i = 0; i < 32; ++i) {
        dst[i] = lane_bytes(v)[i];
    }
}

/**
 * @brief 非对齐存储 512-bit 向量
 * @param ptr 目标内存地址
 * @param v 待存储的向量
 */
NEFORCE_ALWAYS_INLINE_INLINE void storeu_si512(void* ptr, vec512_t v) noexcept {
    auto* dst = static_cast<byte_t*>(ptr);
    for (int i = 0; i < 64; ++i) {
        dst[i] = lane_bytes(v)[i];
    }
}

/**
 * @brief 流式存储 128-bit 向量，绕过缓存
 * @param ptr 目标内存地址
 * @param v 待存储的向量
 * @note 适用于一次性写入后不会立即读取的大块数据，ptr 建议 16 字节对齐以获得最佳性能
 *
 */
NEFORCE_ALWAYS_INLINE_INLINE void store_stream(void* ptr, vec128_t v) noexcept {
    auto* dst = static_cast<byte_t*>(ptr);
    for (int i = 0; i < 16; ++i) {
        dst[i] = lane_bytes(v)[i];
    }
}

/**
 * @brief 流式存储 256-bit 向量，绕过缓存
 * @param ptr 目标内存地址
 * @param v 待存储的向量
 * @note ptr 建议 32 字节对齐以获得最佳性能
 */
NEFORCE_ALWAYS_INLINE_INLINE void store_stream256(void* ptr, vec256_t v) noexcept {
    auto* dst = static_cast<byte_t*>(ptr);
    for (int i = 0; i < 32; ++i) {
        dst[i] = lane_bytes(v)[i];
    }
}

/**
 * @brief 流式存储 512-bit 向量，绕过缓存
 * @param ptr 目标内存地址
 * @param v 待存储的向量
 * @note ptr 建议 64 字节对齐以获得最佳性能
 */
NEFORCE_ALWAYS_INLINE_INLINE void store_stream512(void* ptr, vec512_t v) noexcept {
    auto* dst = static_cast<byte_t*>(ptr);
    for (int i = 0; i < 64; ++i) {
        dst[i] = lane_bytes(v)[i];
    }
}

/**
 * @brief 流式加载 128-bit 向量
 * @param ptr 源内存地址
 * @return 加载的向量
 * @note 适用于一次性读取后不会再次访问的数据流，ptr 建议 16 字节对齐以获得最佳性能
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t load_stream(const void* ptr) noexcept { return loadu_si128(ptr); }

/**
 * @brief 预取数据到所有缓存层级以供读取
 * @param ptr 预取的内存地址
 */
NEFORCE_ALWAYS_INLINE_INLINE void prefetch_read(const void* ptr) noexcept {}

/**
 * @brief 预取数据到所有缓存层级以供写入
 * @param ptr 预取的内存地址
 */
NEFORCE_ALWAYS_INLINE_INLINE void prefetch_write(const void* ptr) noexcept {}

/**
 * @brief 预取数据到 L1 缓存以供读取
 * @param ptr 预取的内存地址
 */
NEFORCE_ALWAYS_INLINE_INLINE void prefetch_l1(const void* ptr) noexcept { (void) ptr; }

/**
 * @brief 预取数据到 L2 缓存
 * @param ptr 预取的内存地址
 */
NEFORCE_ALWAYS_INLINE_INLINE void prefetch_l2(const void* ptr) noexcept { (void) ptr; }

/**
 * @brief 预取数据到缓存槽
 * @param ptr 预取的内存地址
 * @note 适用于仅访问一次的大块数据流式读取
 */
NEFORCE_ALWAYS_INLINE_INLINE void prefetch_nta(const void* ptr) noexcept { (void) ptr; }


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
    size_t remaining = count;
    for (size_t i = 0; i < remaining; ++i) {
        d[i] = s[i];
    }
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
    for (size_t i = 0; i < count; ++i) {
        d[i] = value;
    }
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

    while (count >= 16) {
        scalar::storeu_si128(d, scalar::loadu_si128(s));
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

    while (count >= 16) {
        scalar::storeu_si128(d, scalar::loadu_si128(s));
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
        const vec128_t vl = scalar::loadu_si128(l);
        const vec128_t vr = scalar::loadu_si128(r);
        const vec128_t neq = scalar::match_bytes(vl, vr);
        const int mask = scalar::to_bitmask(neq);
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
 * @brief 大范围内存移动
 * @param dest 目标内存指针
 * @param src 源内存指针
 * @param count 字节数，调用方保证不小于 1 KiB
 * @return 目标指针
 */
NEFORCE_NOINLINE inline void* memory_move_large(void* dest, const void* src, size_t count) noexcept {
    auto* d = static_cast<byte_t*>(dest);
    const auto* s = static_cast<const byte_t*>(src);

    if (d < s) {
        while (count >= 16) {
            scalar::storeu_si128(d, scalar::loadu_si128(s));
            d += 16;
            s += 16;
            count -= 16;
        }
        for (; count > 0; --count) {
            *d++ = *s++;
        }
        return dest;
    }

    d += count;
    s += count;
    while (count >= 16) {
        d -= 16;
        s -= 16;
        scalar::storeu_si128(d, scalar::loadu_si128(s));
        count -= 16;
    }
    while (count > 0) {
        --count;
        --d;
        --s;
        *d = *s;
    }
    return dest;
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
    if (count >= 4096) {
        return scalar::memory_move_large(dest, src, count);
    }

    auto* d = static_cast<byte_t*>(dest);
    const auto* s = static_cast<const byte_t*>(src);

    if (d < s) {
        while (count >= 16) {
            scalar::storeu_si128(d, scalar::loadu_si128(s));
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
            scalar::storeu_si128(d, scalar::loadu_si128(s));
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
    const vec128_t pattern = scalar::fill_i8(value);

    while (count >= 16) {
        scalar::storeu_si128(d, pattern);
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
    const vec128_t zero = scalar::fill_i8(0);
    size_t remaining = count;

    while (remaining >= 16) {
        scalar::storeu_si128(d, zero);
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

    while (count >= 16) {
        const vec128_t v = scalar::loadu_si128(p);
        const int offset = scalar::find_first_byte(v, value);
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
        return scalar::memory_find(data, pattern_ptr[0], data_len);
    }

    const byte_t first_byte = pattern_ptr[0];
    const size_t last_possible = data_len - pattern_len + 1;
    const vec128_t target = scalar::fill_i8(first_byte);

    size_t pos = 0;
    const byte_t* search_start = data_ptr;

    while (pos + 16 <= last_possible) {
        const vec128_t v = scalar::loadu_si128(search_start + pos);
        int bits = scalar::to_bitmask(scalar::match_bytes(v, target));
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
    const block_scan_anchor anchor = inner::anchor_of(str);
    const vec128_t zero = scalar::fill_i(CharT(0));
    size_t char_offset = 0;

    while (true) {
        const vec128_t v = scalar::load_aligned(anchor.base + (char_offset * stride));
        int mask = scalar::to_bitmask(match_lanes<sizeof(CharT)>(v, zero));
        if (char_offset == 0) {
            mask &= static_cast<int>(anchor.prefix_mask);
        }
        if (mask != 0) {
            return char_offset + (static_cast<size_t>(countr_zero(static_cast<unsigned>(mask))) / stride) -
                   anchor.prefix_chars;
        }
        char_offset += 16 / stride;
    }
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
    const inner::block_scan_anchor anchor = inner::anchor_of(str);
    const vec128_t target = scalar::fill_i(chr);
    const vec128_t zero = scalar::fill_i(CharT(0));
    size_t char_offset = 0;

    while (true) {
        const vec128_t v = scalar::load_aligned(anchor.base + (char_offset * stride));
        int mask_target = scalar::to_bitmask(inner::match_lanes<sizeof(CharT)>(v, target));
        int mask_zero = scalar::to_bitmask(inner::match_lanes<sizeof(CharT)>(v, zero));
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

    const vec128_t target = scalar::fill_i(chr);
    const vec128_t zero = scalar::fill_i(CharT(0));

    while (byte_limit - offset >= 16) {
        const vec128_t v = scalar::loadu_si128(p + offset);

        int mask_target = scalar::to_bitmask(inner::match_lanes<sizeof(CharT)>(v, target));
        const int mask_zero = scalar::to_bitmask(inner::match_lanes<sizeof(CharT)>(v, zero));

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

    const size_t length1 = scalar::string_length(s1);
    const size_t length2 = scalar::string_length(s2);
    const size_t common = (length1 < length2) ? length1 : length2;

    if (common != 0 && scalar::memory_compare(s1, s2, common * sizeof(CharT)) != 0) {
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

    const vec128_t zero = scalar::fill_i(CharT(0));

    while (byte_limit - offset >= 16) {
        const vec128_t v1 = scalar::loadu_si128(p1 + offset);
        const vec128_t v2 = scalar::loadu_si128(p2 + offset);

        const int eq_mask = scalar::to_bitmask(inner::match_lanes<sizeof(CharT)>(v1, v2));
        const int null_mask = scalar::to_bitmask(inner::match_lanes<sizeof(CharT)>(v1, zero)) |
                              scalar::to_bitmask(inner::match_lanes<sizeof(CharT)>(v2, zero));

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

NEFORCE_END_SCALAR__
NEFORCE_END_SIMD__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_SIMD_MEMORY_SCALAR_HPP__
