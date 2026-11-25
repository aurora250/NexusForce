#ifndef NEFORCE_CORE_SIMD_BYTES_RISCV_HPP__
#define NEFORCE_CORE_SIMD_BYTES_RISCV_HPP__
#include "NeForce/core/simd/scalar/bytes.hpp"
#ifdef NEFORCE_SIMD_BRANCH_RISCV
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_SIMD__

/**
 * @brief 将单 8-bit 值广播到 128-bit SIMD 向量的全部 16 个位置
 * @param c 源字节值
 * @return 所有字节均为 c 的向量
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t fill_i8(uint8_t c) noexcept {
    vec128_t result;
    __riscv_vse8_v_u8m1(scalar::lane_bytes(result), __riscv_vmv_v_x_u8m1(c, 16), 16);
    return result;
}

/**
 * @brief 将单 16-bit 值广播到 128-bit SIMD 向量的全部 8 个位置
 * @param v 源值
 * @return 所有 16-bit 通道均为 v 的向量
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t fill_i16(const uint16_t v) noexcept {
    vec128_t result;
    __riscv_vse16_v_u16m1(reinterpret_cast<uint16_t*>(scalar::lane_bytes(result)), __riscv_vmv_v_x_u16m1(v, 8), 8);
    return result;
}

/**
 * @brief 将单 32-bit 值广播到 128-bit SIMD 向量的全部 4 个位置
 * @param v 源值
 * @return 所有 32-bit 通道均为 v 的向量
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t fill_i32(const uint32_t v) noexcept {
    vec128_t result;
    __riscv_vse32_v_u32m1(reinterpret_cast<uint32_t*>(scalar::lane_bytes(result)), __riscv_vmv_v_x_u32m1(v, 4), 4);
    return result;
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
    vec128_t result;
    __riscv_vse8_v_u8m1(scalar::lane_bytes(result), __riscv_vle8_v_u8m1(static_cast<const uint8_t*>(ptr), 16), 16);
    return result;
}

/**
 * @brief 逐字节比较两个 128-bit SIMD 向量的相等性
 * @param a 左操作数
 * @param b 右操作数
 * @return 相等位置为 0xFF，不等位置为 0x00 的向量
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t match_bytes(vec128_t a, vec128_t b) noexcept {
    vec128_t result;
    __riscv_vse8_v_u8m1(
            scalar::lane_bytes(result),
            __riscv_vmerge_vvm_u8m1(__riscv_vmv_v_x_u8m1(0, 16), __riscv_vmv_v_x_u8m1(0xFF, 16),
                                    __riscv_vmseq_vv_u8m1_b8(__riscv_vle8_v_u8m1(scalar::lane_bytes(a), 16),
                                                             __riscv_vle8_v_u8m1(scalar::lane_bytes(b), 16), 16),
                                    16),
            16);
    return result;
}

/**
 * @brief 检测向量中是否包含指定字节
 * @param v 输入向量
 * @param c 待检测的字节值
 * @return 存在返回 true，否则返回 false
 */
NEFORCE_ALWAYS_INLINE_INLINE bool contains_byte(vec128_t v, byte_t c) noexcept {
    const vuint8m1_t x = __riscv_vle8_v_u8m1(scalar::lane_bytes(v), 16);
    return __riscv_vfirst_m_b8(__riscv_vmseq_vx_u8m1_b8(x, static_cast<uint8_t>(c), 16), 16) >= 0;
}

/**
 * @brief 查找向量中指定字节首次出现的位置
 * @param v 输入向量
 * @param c 待查找的字节值
 * @return 首次出现的索引（0-15），未找到返回 -1
 */
NEFORCE_ALWAYS_INLINE_INLINE int find_first_byte(vec128_t v, byte_t c) noexcept {
    const vuint8m1_t x = __riscv_vle8_v_u8m1(scalar::lane_bytes(v), 16);
    return static_cast<int>(__riscv_vfirst_m_b8(__riscv_vmseq_vx_u8m1_b8(x, static_cast<uint8_t>(c), 16), 16));
}

/**
 * @brief 检测向量是否全为零
 * @param v 输入向量
 * @return 全零返回 true，否则返回 false
 */
NEFORCE_ALWAYS_INLINE_INLINE bool is_all_zero(vec128_t v) noexcept {
    const vuint8m1_t x = __riscv_vle8_v_u8m1(scalar::lane_bytes(v), 16);
    return __riscv_vfirst_m_b8(__riscv_vmsne_vx_u8m1_b8(x, 0, 16), 16) < 0;
}

/**
 * @brief 检测向量中是否存在零字节
 * @param v 输入向量
 * @return 存在零字节返回 true，否则返回 false
 */
NEFORCE_ALWAYS_INLINE_INLINE bool has_any_zero(vec128_t v) noexcept {
    const vuint8m1_t x = __riscv_vle8_v_u8m1(scalar::lane_bytes(v), 16);
    return __riscv_vfirst_m_b8(__riscv_vmseq_vx_u8m1_b8(x, 0, 16), 16) >= 0;
}


using scalar::blend_bytes;
using scalar::count_byte;
using scalar::find_last_byte;
using scalar::load_bytes_n;
using scalar::reverse_bytes;
using scalar::shuffle_bytes;
using scalar::store_bytes_n;
using scalar::to_bitmask;

NEFORCE_END_SIMD__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_SIMD_BRANCH_RISCV
#endif // NEFORCE_CORE_SIMD_BYTES_RISCV_HPP__
