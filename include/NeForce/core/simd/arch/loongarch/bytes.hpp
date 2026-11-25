#ifndef NEFORCE_CORE_SIMD_BYTES_LOONGARCH_HPP__
#define NEFORCE_CORE_SIMD_BYTES_LOONGARCH_HPP__
#include "NeForce/core/simd/scalar/bytes.hpp"
#ifdef NEFORCE_SIMD_BRANCH_LOONGARCH
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_SIMD__

/**
 * @brief 将单 8-bit 值广播到 128-bit SIMD 向量的全部 16 个位置
 * @param c 源字节值
 * @return 所有字节均为 c 的向量
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t fill_i8(uint8_t c) noexcept { return ::__lsx_vreplgr2vr_b(static_cast<int>(c)); }

/**
 * @brief 将单 16-bit 值广播到 128-bit SIMD 向量的全部 8 个位置
 * @param v 源值
 * @return 所有 16-bit 通道均为 v 的向量
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t fill_i16(const uint16_t v) noexcept {
    return ::__lsx_vreplgr2vr_h(static_cast<int>(v));
}

/**
 * @brief 将单 32-bit 值广播到 128-bit SIMD 向量的全部 4 个位置
 * @param v 源值
 * @return 所有 32-bit 通道均为 v 的向量
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t fill_i32(const uint32_t v) noexcept {
    return ::__lsx_vreplgr2vr_w(static_cast<int>(v));
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
NEFORCE_ALWAYS_INLINE_INLINE vec128_t load_unaligned(const void* ptr) noexcept { return __lsx_vld(ptr, 0); }

/**
 * @brief 逐字节比较两个 128-bit SIMD 向量的相等性
 * @param a 左操作数
 * @param b 右操作数
 * @return 相等位置为 0xFF，不等位置为 0x00 的向量
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t match_bytes(vec128_t a, vec128_t b) noexcept { return ::__lsx_vseq_b(a, b); }

/**
 * @brief 提取向量中每个字节的最高位，组成 16-bit 掩码
 * @param v 输入向量
 * @return 16-bit 掩码，bit i 对应字节 i 的最高位
 */
NEFORCE_ALWAYS_INLINE_INLINE int to_bitmask(vec128_t v) noexcept {
    return static_cast<int>(__lsx_vpickve2gr_w(::__lsx_vmskltz_b(v), 0)) & 0xFFFF;
}

/**
 * @brief 按掩码逐字节混合两个向量
 * @param a 掩码位为 0 时选取的向量
 * @param b 掩码位为 1 时选取的向量
 * @param mask 字节掩码，每字节最高位为 1 则选取 b，否则选取 a
 * @return 混合后的向量
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t blend_bytes(vec128_t a, vec128_t b, vec128_t mask) noexcept {
    const auto select = __lsx_vslti_b(mask, 0);
    return ::__lsx_vbitsel_v(a, b, select);
}


using scalar::contains_byte;
using scalar::count_byte;
using scalar::find_first_byte;
using scalar::find_last_byte;
using scalar::has_any_zero;
using scalar::is_all_zero;
using scalar::load_bytes_n;
using scalar::reverse_bytes;
using scalar::shuffle_bytes;
using scalar::store_bytes_n;

NEFORCE_END_SIMD__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_SIMD_BRANCH_LOONGARCH
#endif // NEFORCE_CORE_SIMD_BYTES_LOONGARCH_HPP__
