#ifndef NEFORCE_CORE_SIMD_BYTES_ARM_HPP__
#define NEFORCE_CORE_SIMD_BYTES_ARM_HPP__
#include "NeForce/core/simd/scalar/bytes.hpp"
#ifdef NEFORCE_SIMD_BRANCH_ARM
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_SIMD__

/**
 * @addtogroup SIMD
 * @{
 */

/**
 * @brief 将单 8-bit 值广播到 128-bit SIMD 向量的全部 16 个位置
 * @param c 源字节值
 * @return 所有字节均为 c 的向量
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t fill_i8(uint8_t c) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    return ::vdupq_n_u8(c);
#    else
    return scalar::fill_i8(c);
#    endif
}

/**
 * @brief 将单 16-bit 值广播到 128-bit SIMD 向量的全部 8 个位置
 * @param v 源值
 * @return 所有 16-bit 通道均为 v 的向量
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t fill_i16(const uint16_t v) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    return ::vreinterpretq_u8_u16(::vdupq_n_u16(v));
#    else
    return scalar::fill_i16(v);
#    endif
}

/**
 * @brief 将单 32-bit 值广播到 128-bit SIMD 向量的全部 4 个位置
 * @param v 源值
 * @return 所有 32-bit 通道均为 v 的向量
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t fill_i32(const uint32_t v) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    return ::vreinterpretq_u8_u32(::vdupq_n_u32(v));
#    else
    return scalar::fill_i32(v);
#    endif
}

/**
 * @brief 按类型大小自动选择广播宽度
 * @tparam T 值类型
 * @param v 源值
 * @return 所有通道均为 v 的向量
 */
template <typename T>
NEFORCE_ALWAYS_INLINE_INLINE vec128_t fill_i(const T v) noexcept {
    static_assert(sizeof(T) <= 4, "fill_i not supports value types upper than 4 bytes");
    NEFORCE_IF_CONSTEXPR(sizeof(T) == 1) { return fill_i8(static_cast<byte_t>(v)); }
    NEFORCE_IF_CONSTEXPR(sizeof(T) == 2) { return fill_i16(static_cast<uint16_t>(v)); }
    return fill_i32(static_cast<uint32_t>(v));
}


/**
 * @brief 非对齐加载 16 字节到 128-bit SIMD 向量
 * @param ptr 源内存地址（无需 16 字节对齐）
 * @return 加载的向量
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t load_unaligned(const void* ptr) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    return vld1q_u8(static_cast<const uint8_t*>(ptr));
#    else
    return scalar::load_unaligned(ptr);
#    endif
}

/**
 * @brief 逐字节比较两个 128-bit SIMD 向量的相等性
 * @param a 左操作数
 * @param b 右操作数
 * @return 相等位置为 0xFF，不等位置为 0x00 的向量
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t match_bytes(vec128_t a, vec128_t b) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    return ::vceqq_u8(a, b);
#    else
    return scalar::match_bytes(a, b);
#    endif
}

/**
 * @brief 提取向量中每个字节的最高位，组成 16-bit 掩码
 * @param v 输入向量
 * @return 16-bit 掩码，bit i 对应字节 i 的最高位
 */
NEFORCE_ALWAYS_INLINE_INLINE int to_bitmask(vec128_t v) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    int mask = 0;
    const auto* bytes = reinterpret_cast<const byte_t*>(&v);
    for (int i = 0; i < 16; ++i) {
        if ((bytes[i] & 0x80) != 0) {
            mask |= (1 << i);
        }
    }
    return mask;
#    else
    return scalar::to_bitmask(v);
#    endif
}

/**
 * @brief 检测向量中是否包含指定字节
 * @param v 输入向量
 * @param c 待检测的字节值
 * @return 存在返回 true，否则返回 false
 */
NEFORCE_ALWAYS_INLINE_INLINE bool contains_byte(vec128_t v, byte_t c) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    const ::uint8x16_t match = ::vceqq_u8(v, ::vdupq_n_u8(c));
    const auto* bytes = reinterpret_cast<const byte_t*>(&match);
    for (int i = 0; i < 16; ++i) {
        if (bytes[i] != 0) {
            return true;
        }
    }
    return false;
#    else
    return scalar::contains_byte(v, c);
#    endif
}

/**
 * @brief 查找向量中指定字节首次出现的位置
 * @param v 输入向量
 * @param c 待查找的字节值
 * @return 首次出现的索引（0-15），未找到返回 -1
 */
NEFORCE_ALWAYS_INLINE_INLINE int find_first_byte(vec128_t v, byte_t c) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    const ::uint8x16_t match = ::vceqq_u8(v, ::vdupq_n_u8(c));
    const auto* bytes = reinterpret_cast<const byte_t*>(&match);
    for (int i = 0; i < 16; ++i) {
        if (bytes[i] != 0) {
            return i;
        }
    }
    return -1;
#    else
    return scalar::find_first_byte(v, c);
#    endif
}

/**
 * @brief 查找向量中指定字节末次出现的位置
 * @param v 输入向量
 * @param c 待查找的字节值
 * @return 末次出现的索引（0-15），未找到返回 -1
 */
NEFORCE_ALWAYS_INLINE_INLINE int find_last_byte(vec128_t v, byte_t c) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    const ::uint8x16_t match = ::vceqq_u8(v, ::vdupq_n_u8(c));
    const auto* bytes = reinterpret_cast<const byte_t*>(&match);
    for (int i = 15; i >= 0; --i) {
        if (bytes[i] != 0) {
            return i;
        }
    }
    return -1;
#    else
    return scalar::find_last_byte(v, c);
#    endif
}

/**
 * @brief 统计向量中指定字节出现的次数
 * @param v 输入向量
 * @param c 待统计的字节值
 * @return 出现次数（0-16）
 */
NEFORCE_ALWAYS_INLINE_INLINE int count_byte(vec128_t v, byte_t c) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    const ::uint8x16_t match = ::vceqq_u8(v, ::vdupq_n_u8(c));
    const ::uint8x16_t ones = ::vandq_u8(match, ::vdupq_n_u8(1));
#        ifdef NEFORCE_ARCH_AARCH64
    return static_cast<int>(::vaddvq_u8(ones));
#        else
    int count = 0;
    const auto* bytes = reinterpret_cast<const byte_t*>(&ones);
    for (int i = 0; i < 16; ++i) {
        count += bytes[i];
    }
    return count;
#        endif
#    else
    return scalar::count_byte(v, c);
#    endif
}

/**
 * @brief 检测向量是否全为零
 * @param v 输入向量
 * @return 全零返回 true，否则返回 false
 */
NEFORCE_ALWAYS_INLINE_INLINE bool is_all_zero(vec128_t v) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    const ::uint64x2_t v64 = ::vreinterpretq_u64_u8(v);
    return (vgetq_lane_u64(v64, 0) | vgetq_lane_u64(v64, 1)) == 0;
#    else
    return scalar::is_all_zero(v);
#    endif
}

/**
 * @brief 检测向量中是否存在零字节
 * @param v 输入向量
 * @return 存在零字节返回 true，否则返回 false
 */
NEFORCE_ALWAYS_INLINE_INLINE bool has_any_zero(vec128_t v) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    const ::uint8x16_t match = ::vceqq_u8(v, ::vdupq_n_u8(0));
    const ::uint64x2_t v64 = ::vreinterpretq_u64_u8(match);
    return (vgetq_lane_u64(v64, 0) | vgetq_lane_u64(v64, 1)) != 0;
#    else
    return scalar::has_any_zero(v);
#    endif
}

/**
 * @brief 反转向量中 16 字节的顺序
 * @param v 输入向量
 * @return 字节逆序排列的向量
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t reverse_bytes(vec128_t v) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    const auto lo = ::vrev64_u8(::vget_low_u8(v));
    const auto hi = ::vrev64_u8(::vget_high_u8(v));
    return ::vcombine_u8(hi, lo);
#    else
    return scalar::reverse_bytes(v);
#    endif
}

/**
 * @brief 按索引表逐字节重排向量
 * @param v 输入向量
 * @param indices 索引向量，每字节低 4 位为源字节索引（0-15）
 * @return 重排后的向量
 * @note 当某索引字节高 4 位非零（SSE）或 >= 16（NEON）时，对应结果字节置零
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t shuffle_bytes(vec128_t v, vec128_t indices) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    return ::vqtbl1q_u8(v, ::vandq_u8(indices, ::vdupq_n_u8(0x0F)));
#    else
    return scalar::shuffle_bytes(v, indices);
#    endif
}

/**
 * @brief 按掩码逐字节混合两个向量
 * @param a 掩码位为 0 时选取的向量
 * @param b 掩码位为 1 时选取的向量
 * @param mask 字节掩码，每字节最高位为 1 则选取 b，否则选取 a
 * @return 混合后的向量
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t blend_bytes(vec128_t a, vec128_t b, vec128_t mask) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    const ::int8x16_t sel = vshrq_n_s8(::vreinterpretq_s8_u8(mask), 7);
    return ::vbslq_u8(::vreinterpretq_u8_s8(sel), b, a);
#    else
    return scalar::blend_bytes(a, b, mask);
#    endif
}

/**
 * @brief 存储向量的前 n 字节到内存，不写入越界数据
 * @param ptr 目标内存地址
 * @param v 源向量
 * @param n 要写入的字节数（0-16）
 * @note 用于需要部分写入的尾部处理场景
 */
NEFORCE_ALWAYS_INLINE_INLINE void store_bytes_n(void* ptr, vec128_t v, int n) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    alignas(16) byte_t buf[16];
    vst1q_u8(buf, v);
    for (int i = 0; i < n; ++i) {
        static_cast<byte_t*>(ptr)[i] = buf[i];
    }
#    else
    return scalar::store_bytes_n(ptr, v, n);
#    endif
}

/**
 * @brief 从内存加载 n 字节到向量，不足部分填零
 * @param ptr 源内存地址
 * @param n 要读取的字节数（0-16）
 * @return 前 n 字节为源数据、剩余字节为零的向量
 * @note 用于需要部分加载的头部/尾部处理场景
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t load_bytes_n(const void* ptr, int n) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    alignas(16) byte_t buf[16] = {};
    const auto* src = static_cast<const byte_t*>(ptr);
    for (int i = 0; i < n; ++i) {
        buf[i] = src[i];
    }
    return vld1q_u8(buf);
#    else
    return scalar::load_bytes_n(ptr, n);
#    endif
}

/** @} */ // SIMD

NEFORCE_END_SIMD__
NEFORCE_END_NAMESPACE__
#endif
#endif // NEFORCE_CORE_SIMD_BYTES_ARM_HPP__
