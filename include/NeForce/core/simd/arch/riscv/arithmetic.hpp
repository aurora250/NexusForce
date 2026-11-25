#ifndef NEFORCE_CORE_SIMD_ARITHMETIC_RISCV_HPP__
#define NEFORCE_CORE_SIMD_ARITHMETIC_RISCV_HPP__

/**
 * @file arithmetic.hpp
 * @brief RISC-V 整型算术后端
 *
 * RVV 的可伸缩向量类型（vec_rvv*_t）无法承载定长的 vec128_t，
 * 定长向量的逐元素算术只能经由内存中转，无法映射为更优的单条向量指令，
 * 因此本后端整体复用 scalar 后端，而不是重复实现同一套逐通道算法。
 */

#include "NeForce/core/simd/scalar/arithmetic.hpp"
#ifdef NEFORCE_SIMD_BRANCH_RISCV
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_SIMD__

/**
 * @brief 8 路 16-bit 有符号整数乘法（取高半部分）
 * @param a 左操作数（8×i16）
 * @param b 右操作数（8×i16）
 * @return (a * b) >> 16（8×i16，高 16-bit 算术结果）
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t mulhi_i16(vec128_t a, vec128_t b) noexcept {
    vec128_t result;
    const vint16m1_t va = __riscv_vle16_v_i16m1(reinterpret_cast<const int16_t*>(scalar::lane_bytes(a)), 8);
    const vint16m1_t vb = __riscv_vle16_v_i16m1(reinterpret_cast<const int16_t*>(scalar::lane_bytes(b)), 8);
    __riscv_vse16_v_i16m1(reinterpret_cast<int16_t*>(scalar::lane_bytes(result)), __riscv_vmulh_vv_i16m1(va, vb, 8), 8);
    return result;
}

/**
 * @brief 有符号 8-bit 相邻对乘加：将相邻 i8 相乘后累加为 i16
 * @param a 左操作数（16×i8）
 * @param b 右操作数（16×i8）
 * @return [a[0]*b[0]+a[1]*b[1], a[2]*b[2]+a[3]*b[3], ...]（8×i16）
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t madds_i8x16(vec128_t a, vec128_t b) noexcept {
    const auto* pa = reinterpret_cast<const int8_t*>(scalar::lane_bytes(a));
    const auto* pb = reinterpret_cast<const int8_t*>(scalar::lane_bytes(b));
    const vint8m1_t even_a = __riscv_vlse8_v_i8m1(pa, 2, 8);
    const vint8m1_t odd_a = __riscv_vlse8_v_i8m1(pa + 1, 2, 8);
    const vint8m1_t even_b = __riscv_vlse8_v_i8m1(pb, 2, 8);
    const vint8m1_t odd_b = __riscv_vlse8_v_i8m1(pb + 1, 2, 8);
    const vint16m2_t prod_even = __riscv_vwmul_vv_i16m2(even_a, even_b, 8);
    const vint16m2_t prod_odd = __riscv_vwmul_vv_i16m2(odd_a, odd_b, 8);
    vec128_t result;
    __riscv_vse16_v_i16m2(reinterpret_cast<int16_t*>(scalar::lane_bytes(result)),
                          __riscv_vadd_vv_i16m2(prod_even, prod_odd, 8), 8);
    return result;
}

/**
 * @brief 16 路 8-bit 有符号饱和加法
 * @param a 左操作数（16×i8）
 * @param b 右操作数（16×i8）
 * @return 饱和到 [-128, 127] 的加法结果
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t saturated_add_i8(vec128_t a, vec128_t b) noexcept {
    vec128_t result;
    const vint8m1_t va = __riscv_vle8_v_i8m1(reinterpret_cast<const int8_t*>(scalar::lane_bytes(a)), 16);
    const vint8m1_t vb = __riscv_vle8_v_i8m1(reinterpret_cast<const int8_t*>(scalar::lane_bytes(b)), 16);
    __riscv_vse8_v_i8m1(reinterpret_cast<int8_t*>(scalar::lane_bytes(result)), __riscv_vsadd_vv_i8m1(va, vb, 16), 16);
    return result;
}

/**
 * @brief 8 路 16-bit 有符号饱和加法
 * @param a 左操作数（8×i16）
 * @param b 右操作数（8×i16）
 * @return 饱和到 [-32768, 32767] 的加法结果
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t saturated_add_i16(vec128_t a, vec128_t b) noexcept {
    vec128_t result;
    const vint16m1_t va = __riscv_vle16_v_i16m1(reinterpret_cast<const int16_t*>(scalar::lane_bytes(a)), 8);
    const vint16m1_t vb = __riscv_vle16_v_i16m1(reinterpret_cast<const int16_t*>(scalar::lane_bytes(b)), 8);
    __riscv_vse16_v_i16m1(reinterpret_cast<int16_t*>(scalar::lane_bytes(result)), __riscv_vsadd_vv_i16m1(va, vb, 8), 8);
    return result;
}

/**
 * @brief 16 路 8-bit 无符号饱和加法
 * @param a 左操作数（16×u8）
 * @param b 右操作数（16×u8）
 * @return 饱和到 [0, 255] 的加法结果
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t saturated_add_u8(vec128_t a, vec128_t b) noexcept {
    vec128_t result;
    const vuint8m1_t va = __riscv_vle8_v_u8m1(reinterpret_cast<const uint8_t*>(scalar::lane_bytes(a)), 16);
    const vuint8m1_t vb = __riscv_vle8_v_u8m1(reinterpret_cast<const uint8_t*>(scalar::lane_bytes(b)), 16);
    __riscv_vse8_v_u8m1(reinterpret_cast<uint8_t*>(scalar::lane_bytes(result)), __riscv_vsaddu_vv_u8m1(va, vb, 16), 16);
    return result;
}

/**
 * @brief 8 路 16-bit 无符号饱和加法
 * @param a 左操作数（8×u16）
 * @param b 右操作数（8×u16）
 * @return 饱和到 [0, 65535] 的加法结果
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t saturated_add_u16(vec128_t a, vec128_t b) noexcept {
    vec128_t result;
    const vuint16m1_t va = __riscv_vle16_v_u16m1(reinterpret_cast<const uint16_t*>(scalar::lane_bytes(a)), 8);
    const vuint16m1_t vb = __riscv_vle16_v_u16m1(reinterpret_cast<const uint16_t*>(scalar::lane_bytes(b)), 8);
    __riscv_vse16_v_u16m1(reinterpret_cast<uint16_t*>(scalar::lane_bytes(result)), __riscv_vsaddu_vv_u16m1(va, vb, 8),
                          8);
    return result;
}

/**
 * @brief 16 路 8-bit 有符号饱和减法
 * @param a 左操作数（16×i8）
 * @param b 右操作数（16×i8）
 * @return 饱和到 [-128, 127] 的减法结果
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t saturated_sub_i8(vec128_t a, vec128_t b) noexcept {
    vec128_t result;
    const vint8m1_t va = __riscv_vle8_v_i8m1(reinterpret_cast<const int8_t*>(scalar::lane_bytes(a)), 16);
    const vint8m1_t vb = __riscv_vle8_v_i8m1(reinterpret_cast<const int8_t*>(scalar::lane_bytes(b)), 16);
    __riscv_vse8_v_i8m1(reinterpret_cast<int8_t*>(scalar::lane_bytes(result)), __riscv_vssub_vv_i8m1(va, vb, 16), 16);
    return result;
}

/**
 * @brief 8 路 16-bit 有符号饱和减法
 * @param a 左操作数（8×i16）
 * @param b 右操作数（8×i16）
 * @return 饱和到 [-32768, 32767] 的减法结果
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t saturated_sub_i16(vec128_t a, vec128_t b) noexcept {
    vec128_t result;
    const vint16m1_t va = __riscv_vle16_v_i16m1(reinterpret_cast<const int16_t*>(scalar::lane_bytes(a)), 8);
    const vint16m1_t vb = __riscv_vle16_v_i16m1(reinterpret_cast<const int16_t*>(scalar::lane_bytes(b)), 8);
    __riscv_vse16_v_i16m1(reinterpret_cast<int16_t*>(scalar::lane_bytes(result)), __riscv_vssub_vv_i16m1(va, vb, 8), 8);
    return result;
}

/**
 * @brief 16 路 8-bit 无符号饱和减法
 * @param a 左操作数（16×u8）
 * @param b 右操作数（16×u8）
 * @return 饱和到 [0, 255] 的减法结果
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t saturated_sub_u8(vec128_t a, vec128_t b) noexcept {
    vec128_t result;
    const vuint8m1_t va = __riscv_vle8_v_u8m1(reinterpret_cast<const uint8_t*>(scalar::lane_bytes(a)), 16);
    const vuint8m1_t vb = __riscv_vle8_v_u8m1(reinterpret_cast<const uint8_t*>(scalar::lane_bytes(b)), 16);
    __riscv_vse8_v_u8m1(reinterpret_cast<uint8_t*>(scalar::lane_bytes(result)), __riscv_vssubu_vv_u8m1(va, vb, 16), 16);
    return result;
}

/**
 * @brief 8 路 16-bit 无符号饱和减法
 * @param a 左操作数（8×u16）
 * @param b 右操作数（8×u16）
 * @return 饱和到 [0, 65535] 的减法结果
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t saturated_sub_u16(vec128_t a, vec128_t b) noexcept {
    vec128_t result;
    const vuint16m1_t va = __riscv_vle16_v_u16m1(reinterpret_cast<const uint16_t*>(scalar::lane_bytes(a)), 8);
    const vuint16m1_t vb = __riscv_vle16_v_u16m1(reinterpret_cast<const uint16_t*>(scalar::lane_bytes(b)), 8);
    __riscv_vse16_v_u16m1(reinterpret_cast<uint16_t*>(scalar::lane_bytes(result)), __riscv_vssubu_vv_u16m1(va, vb, 8),
                          8);
    return result;
}

/**
 * @brief 16 路 8-bit 有符号整数绝对值
 * @param v 输入向量（16×i8）
 * @return scalar::abs(v)（16×i8）
 * @note 输入最小值时不饱和（按 `max(v, 0 - v)` 实现，RVV 无整型取绝对值指令）
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t abs_i8(vec128_t v) noexcept {
    const vint8m1_t va = __riscv_vle8_v_i8m1(reinterpret_cast<const int8_t*>(scalar::lane_bytes(v)), 16);
    vec128_t result;
    __riscv_vse8_v_i8m1(reinterpret_cast<int8_t*>(scalar::lane_bytes(result)),
                        __riscv_vmax_vv_i8m1(va, __riscv_vsub_vv_i8m1(__riscv_vmv_v_x_i8m1(0, 16), va, 16), 16), 16);
    return result;
}

/**
 * @brief 8 路 16-bit 有符号整数绝对值
 * @param v 输入向量（8×i16）
 * @return scalar::abs(v)（8×i16）
 * @note 输入 INT16_MIN 时不饱和
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t abs_i16(vec128_t v) noexcept {
    const vint16m1_t va = __riscv_vle16_v_i16m1(reinterpret_cast<const int16_t*>(scalar::lane_bytes(v)), 8);
    vec128_t result;
    __riscv_vse16_v_i16m1(reinterpret_cast<int16_t*>(scalar::lane_bytes(result)),
                          __riscv_vmax_vv_i16m1(va, __riscv_vsub_vv_i16m1(__riscv_vmv_v_x_i16m1(0, 8), va, 8), 8), 8);
    return result;
}

/**
 * @brief 4 路 32-bit 有符号整数绝对值
 * @param v 输入向量（4×i32）
 * @return scalar::abs(v)（4×i32）
 * @note 输入 INT32_MIN 时不饱和
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t abs_i32(vec128_t v) noexcept {
    const vint32m1_t va = __riscv_vle32_v_i32m1(reinterpret_cast<const int32_t*>(scalar::lane_bytes(v)), 4);
    vec128_t result;
    __riscv_vse32_v_i32m1(reinterpret_cast<int32_t*>(scalar::lane_bytes(result)),
                          __riscv_vmax_vv_i32m1(va, __riscv_vsub_vv_i32m1(__riscv_vmv_v_x_i32m1(0, 4), va, 4), 4), 4);
    return result;
}

/**
 * @brief 16 路 8-bit 有符号整数最小值
 * @param a 左操作数（16×i8）
 * @param b 右操作数（16×i8）
 * @return scalar::min(a, b) 逐元素
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t min_i8(vec128_t a, vec128_t b) noexcept {
    vec128_t result;
    const vint8m1_t va = __riscv_vle8_v_i8m1(reinterpret_cast<const int8_t*>(scalar::lane_bytes(a)), 16);
    const vint8m1_t vb = __riscv_vle8_v_i8m1(reinterpret_cast<const int8_t*>(scalar::lane_bytes(b)), 16);
    __riscv_vse8_v_i8m1(reinterpret_cast<int8_t*>(scalar::lane_bytes(result)), __riscv_vmin_vv_i8m1(va, vb, 16), 16);
    return result;
}

/**
 * @brief 8 路 16-bit 有符号整数最小值
 * @param a 左操作数（8×i16）
 * @param b 右操作数（8×i16）
 * @return scalar::min(a, b) 逐元素
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t min_i16(vec128_t a, vec128_t b) noexcept {
    vec128_t result;
    const vint16m1_t va = __riscv_vle16_v_i16m1(reinterpret_cast<const int16_t*>(scalar::lane_bytes(a)), 8);
    const vint16m1_t vb = __riscv_vle16_v_i16m1(reinterpret_cast<const int16_t*>(scalar::lane_bytes(b)), 8);
    __riscv_vse16_v_i16m1(reinterpret_cast<int16_t*>(scalar::lane_bytes(result)), __riscv_vmin_vv_i16m1(va, vb, 8), 8);
    return result;
}

/**
 * @brief 4 路 32-bit 有符号整数最小值
 * @param a 左操作数（4×i32）
 * @param b 右操作数（4×i32）
 * @return scalar::min(a, b) 逐元素
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t min_i32(vec128_t a, vec128_t b) noexcept {
    vec128_t result;
    const vint32m1_t va = __riscv_vle32_v_i32m1(reinterpret_cast<const int32_t*>(scalar::lane_bytes(a)), 4);
    const vint32m1_t vb = __riscv_vle32_v_i32m1(reinterpret_cast<const int32_t*>(scalar::lane_bytes(b)), 4);
    __riscv_vse32_v_i32m1(reinterpret_cast<int32_t*>(scalar::lane_bytes(result)), __riscv_vmin_vv_i32m1(va, vb, 4), 4);
    return result;
}

/**
 * @brief 16 路 8-bit 无符号整数最小值
 * @param a 左操作数（16×u8）
 * @param b 右操作数（16×u8）
 * @return scalar::min(a, b) 逐元素
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t min_u8(vec128_t a, vec128_t b) noexcept {
    vec128_t result;
    const vuint8m1_t va = __riscv_vle8_v_u8m1(reinterpret_cast<const uint8_t*>(scalar::lane_bytes(a)), 16);
    const vuint8m1_t vb = __riscv_vle8_v_u8m1(reinterpret_cast<const uint8_t*>(scalar::lane_bytes(b)), 16);
    __riscv_vse8_v_u8m1(reinterpret_cast<uint8_t*>(scalar::lane_bytes(result)), __riscv_vminu_vv_u8m1(va, vb, 16), 16);
    return result;
}

/**
 * @brief 8 路 16-bit 无符号整数最小值
 * @param a 左操作数（8×u16）
 * @param b 右操作数（8×u16）
 * @return scalar::min(a, b) 逐元素
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t min_u16(vec128_t a, vec128_t b) noexcept {
    vec128_t result;
    const vuint16m1_t va = __riscv_vle16_v_u16m1(reinterpret_cast<const uint16_t*>(scalar::lane_bytes(a)), 8);
    const vuint16m1_t vb = __riscv_vle16_v_u16m1(reinterpret_cast<const uint16_t*>(scalar::lane_bytes(b)), 8);
    __riscv_vse16_v_u16m1(reinterpret_cast<uint16_t*>(scalar::lane_bytes(result)), __riscv_vminu_vv_u16m1(va, vb, 8),
                          8);
    return result;
}

/**
 * @brief 4 路 32-bit 无符号整数最小值
 * @param a 左操作数（4×u32）
 * @param b 右操作数（4×u32）
 * @return scalar::min(a, b) 逐元素
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t min_u32(vec128_t a, vec128_t b) noexcept {
    vec128_t result;
    const vuint32m1_t va = __riscv_vle32_v_u32m1(reinterpret_cast<const uint32_t*>(scalar::lane_bytes(a)), 4);
    const vuint32m1_t vb = __riscv_vle32_v_u32m1(reinterpret_cast<const uint32_t*>(scalar::lane_bytes(b)), 4);
    __riscv_vse32_v_u32m1(reinterpret_cast<uint32_t*>(scalar::lane_bytes(result)), __riscv_vminu_vv_u32m1(va, vb, 4),
                          4);
    return result;
}

/**
 * @brief 16 路 8-bit 有符号整数最大值
 * @param a 左操作数（16×i8）
 * @param b 右操作数（16×i8）
 * @return scalar::max(a, b) 逐元素
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t max_i8(vec128_t a, vec128_t b) noexcept {
    vec128_t result;
    const vint8m1_t va = __riscv_vle8_v_i8m1(reinterpret_cast<const int8_t*>(scalar::lane_bytes(a)), 16);
    const vint8m1_t vb = __riscv_vle8_v_i8m1(reinterpret_cast<const int8_t*>(scalar::lane_bytes(b)), 16);
    __riscv_vse8_v_i8m1(reinterpret_cast<int8_t*>(scalar::lane_bytes(result)), __riscv_vmax_vv_i8m1(va, vb, 16), 16);
    return result;
}

/**
 * @brief 8 路 16-bit 有符号整数最大值
 * @param a 左操作数（8×i16）
 * @param b 右操作数（8×i16）
 * @return scalar::max(a, b) 逐元素
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t max_i16(vec128_t a, vec128_t b) noexcept {
    vec128_t result;
    const vint16m1_t va = __riscv_vle16_v_i16m1(reinterpret_cast<const int16_t*>(scalar::lane_bytes(a)), 8);
    const vint16m1_t vb = __riscv_vle16_v_i16m1(reinterpret_cast<const int16_t*>(scalar::lane_bytes(b)), 8);
    __riscv_vse16_v_i16m1(reinterpret_cast<int16_t*>(scalar::lane_bytes(result)), __riscv_vmax_vv_i16m1(va, vb, 8), 8);
    return result;
}

/**
 * @brief 4 路 32-bit 有符号整数最大值
 * @param a 左操作数（4×i32）
 * @param b 右操作数（4×i32）
 * @return scalar::max(a, b) 逐元素
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t max_i32(vec128_t a, vec128_t b) noexcept {
    vec128_t result;
    const vint32m1_t va = __riscv_vle32_v_i32m1(reinterpret_cast<const int32_t*>(scalar::lane_bytes(a)), 4);
    const vint32m1_t vb = __riscv_vle32_v_i32m1(reinterpret_cast<const int32_t*>(scalar::lane_bytes(b)), 4);
    __riscv_vse32_v_i32m1(reinterpret_cast<int32_t*>(scalar::lane_bytes(result)), __riscv_vmax_vv_i32m1(va, vb, 4), 4);
    return result;
}

/**
 * @brief 16 路 8-bit 无符号整数最大值
 * @param a 左操作数（16×u8）
 * @param b 右操作数（16×u8）
 * @return scalar::max(a, b) 逐元素
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t max_u8(vec128_t a, vec128_t b) noexcept {
    vec128_t result;
    const vuint8m1_t va = __riscv_vle8_v_u8m1(reinterpret_cast<const uint8_t*>(scalar::lane_bytes(a)), 16);
    const vuint8m1_t vb = __riscv_vle8_v_u8m1(reinterpret_cast<const uint8_t*>(scalar::lane_bytes(b)), 16);
    __riscv_vse8_v_u8m1(reinterpret_cast<uint8_t*>(scalar::lane_bytes(result)), __riscv_vmaxu_vv_u8m1(va, vb, 16), 16);
    return result;
}

/**
 * @brief 8 路 16-bit 无符号整数最大值
 * @param a 左操作数（8×u16）
 * @param b 右操作数（8×u16）
 * @return scalar::max(a, b) 逐元素
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t max_u16(vec128_t a, vec128_t b) noexcept {
    vec128_t result;
    const vuint16m1_t va = __riscv_vle16_v_u16m1(reinterpret_cast<const uint16_t*>(scalar::lane_bytes(a)), 8);
    const vuint16m1_t vb = __riscv_vle16_v_u16m1(reinterpret_cast<const uint16_t*>(scalar::lane_bytes(b)), 8);
    __riscv_vse16_v_u16m1(reinterpret_cast<uint16_t*>(scalar::lane_bytes(result)), __riscv_vmaxu_vv_u16m1(va, vb, 8),
                          8);
    return result;
}

/**
 * @brief 4 路 32-bit 无符号整数最大值
 * @param a 左操作数（4×u32）
 * @param b 右操作数（4×u32）
 * @return scalar::max(a, b) 逐元素
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t max_u32(vec128_t a, vec128_t b) noexcept {
    vec128_t result;
    const vuint32m1_t va = __riscv_vle32_v_u32m1(reinterpret_cast<const uint32_t*>(scalar::lane_bytes(a)), 4);
    const vuint32m1_t vb = __riscv_vle32_v_u32m1(reinterpret_cast<const uint32_t*>(scalar::lane_bytes(b)), 4);
    __riscv_vse32_v_u32m1(reinterpret_cast<uint32_t*>(scalar::lane_bytes(result)), __riscv_vmaxu_vv_u32m1(va, vb, 4),
                          4);
    return result;
}

/**
 * @brief 16 路 8-bit 无符号整数平均值（向偶数舍入）
 * @param a 左操作数（16×u8）
 * @param b 右操作数（16×u8）
 * @return (a + b + 1) / 2 逐元素
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t avg_u8(vec128_t a, vec128_t b) noexcept {
    const vuint8m1_t va = __riscv_vle8_v_u8m1(reinterpret_cast<const uint8_t*>(scalar::lane_bytes(a)), 16);
    const vuint8m1_t vb = __riscv_vle8_v_u8m1(reinterpret_cast<const uint8_t*>(scalar::lane_bytes(b)), 16);
    const vuint8m1_t hi = __riscv_vmaxu_vv_u8m1(va, vb, 16);
    const vuint8m1_t lo = __riscv_vminu_vv_u8m1(va, vb, 16);
    vec128_t result;
    __riscv_vse8_v_u8m1(reinterpret_cast<uint8_t*>(scalar::lane_bytes(result)),
                        __riscv_vsub_vv_u8m1(hi, __riscv_vsrl_vx_u8m1(__riscv_vsub_vv_u8m1(hi, lo, 16), 1, 16), 16),
                        16);
    return result;
}

/**
 * @brief 8 路 16-bit 无符号整数平均值（向偶数舍入）
 * @param a 左操作数（8×u16）
 * @param b 右操作数（8×u16）
 * @return (a + b + 1) / 2 逐元素
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t avg_u16(vec128_t a, vec128_t b) noexcept {
    const vuint16m1_t va = __riscv_vle16_v_u16m1(reinterpret_cast<const uint16_t*>(scalar::lane_bytes(a)), 8);
    const vuint16m1_t vb = __riscv_vle16_v_u16m1(reinterpret_cast<const uint16_t*>(scalar::lane_bytes(b)), 8);
    const vuint16m1_t hi = __riscv_vmaxu_vv_u16m1(va, vb, 8);
    const vuint16m1_t lo = __riscv_vminu_vv_u16m1(va, vb, 8);
    vec128_t result;
    __riscv_vse16_v_u16m1(reinterpret_cast<uint16_t*>(scalar::lane_bytes(result)),
                          __riscv_vsub_vv_u16m1(hi, __riscv_vsrl_vx_u16m1(__riscv_vsub_vv_u16m1(hi, lo, 8), 1, 8), 8),
                          8);
    return result;
}


using scalar::add_i16;
using scalar::add_i32;
using scalar::add_i64;
using scalar::add_i8;
using scalar::mullo_i16;
using scalar::mullo_i32;
using scalar::sub_i16;
using scalar::sub_i32;
using scalar::sub_i64;
using scalar::sub_i8;

NEFORCE_END_SIMD__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_SIMD_BRANCH_RISCV
#endif // NEFORCE_CORE_SIMD_ARITHMETIC_RISCV_HPP__
