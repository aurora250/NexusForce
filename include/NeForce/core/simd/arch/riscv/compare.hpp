#ifndef NEFORCE_CORE_SIMD_COMPARE_RISCV_HPP__
#define NEFORCE_CORE_SIMD_COMPARE_RISCV_HPP__

/**
 * @file compare.hpp
 * @brief RISC-V 比较后端
 *
 * 整型比较在 RVV 上可映射为单条向量比较指令，故保留原生实现；
 * 定长浮点向量 vec128f_t/vec128d_t 在 RVV 上只能逐通道处理，
 * 其逐通道算法与 scalar 后端完全一致，因此直接复用 scalar 后端。
 */

#include "NeForce/core/simd/scalar/compare.hpp"
#ifdef NEFORCE_SIMD_BRANCH_RISCV
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_SIMD__

/**
 * @brief 16 路 8-bit 相等比较
 * @param a 左操作数（16×i8）
 * @param b 右操作数（16×i8）
 * @return a == b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t cmpeq_i8(vec128_t a, vec128_t b) noexcept {
    vec128_t result;
    const vuint8m1_t va = __riscv_vle8_v_u8m1(reinterpret_cast<const uint8_t*>(scalar::lane_bytes(a)), 16);
    const vuint8m1_t vb = __riscv_vle8_v_u8m1(reinterpret_cast<const uint8_t*>(scalar::lane_bytes(b)), 16);
    const vbool8_t m = __riscv_vmseq_vv_u8m1_b8(va, vb, 16);
    __riscv_vse8_v_u8m1(reinterpret_cast<uint8_t*>(scalar::lane_bytes(result)),
                        __riscv_vmerge_vvm_u8m1(__riscv_vmv_v_x_u8m1(0, 16), __riscv_vmv_v_x_u8m1(0xFF, 16), m, 16),
                        16);
    return result;
}

/**
 * @brief 8 路 16-bit 相等比较
 * @param a 左操作数（8×i16）
 * @param b 右操作数（8×i16）
 * @return a == b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t cmpeq_i16(vec128_t a, vec128_t b) noexcept {
    vec128_t result;
    const vuint16m1_t va = __riscv_vle16_v_u16m1(reinterpret_cast<const uint16_t*>(scalar::lane_bytes(a)), 8);
    const vuint16m1_t vb = __riscv_vle16_v_u16m1(reinterpret_cast<const uint16_t*>(scalar::lane_bytes(b)), 8);
    const vbool16_t m = __riscv_vmseq_vv_u16m1_b16(va, vb, 8);
    __riscv_vse16_v_u16m1(reinterpret_cast<uint16_t*>(scalar::lane_bytes(result)),
                          __riscv_vmerge_vvm_u16m1(__riscv_vmv_v_x_u16m1(0, 8), __riscv_vmv_v_x_u16m1(0xFFFF, 8), m, 8),
                          8);
    return result;
}

/**
 * @brief 4 路 32-bit 相等比较
 * @param a 左操作数（4×i32）
 * @param b 右操作数（4×i32）
 * @return a == b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t cmpeq_i32(vec128_t a, vec128_t b) noexcept {
    vec128_t result;
    const vuint32m1_t va = __riscv_vle32_v_u32m1(reinterpret_cast<const uint32_t*>(scalar::lane_bytes(a)), 4);
    const vuint32m1_t vb = __riscv_vle32_v_u32m1(reinterpret_cast<const uint32_t*>(scalar::lane_bytes(b)), 4);
    const vbool32_t m = __riscv_vmseq_vv_u32m1_b32(va, vb, 4);
    __riscv_vse32_v_u32m1(
            reinterpret_cast<uint32_t*>(scalar::lane_bytes(result)),
            __riscv_vmerge_vvm_u32m1(__riscv_vmv_v_x_u32m1(0, 4), __riscv_vmv_v_x_u32m1(0xFFFFFFFF, 4), m, 4), 4);
    return result;
}

/**
 * @brief 2 路 64-bit 相等比较
 * @param a 左操作数（2×i64）
 * @param b 右操作数（2×i64）
 * @return a == b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t cmpeq_i64(vec128_t a, vec128_t b) noexcept {
    vec128_t result;
    const vuint64m1_t va = __riscv_vle64_v_u64m1(reinterpret_cast<const uint64_t*>(scalar::lane_bytes(a)), 2);
    const vuint64m1_t vb = __riscv_vle64_v_u64m1(reinterpret_cast<const uint64_t*>(scalar::lane_bytes(b)), 2);
    const vbool64_t m = __riscv_vmseq_vv_u64m1_b64(va, vb, 2);
    __riscv_vse64_v_u64m1(reinterpret_cast<uint64_t*>(scalar::lane_bytes(result)),
                          __riscv_vmerge_vvm_u64m1(__riscv_vmv_v_x_u64m1(0, 2),
                                                   __riscv_vmv_v_x_u64m1(0xFFFFFFFFFFFFFFFFULL, 2), m, 2),
                          2);
    return result;
}

/**
 * @brief 16 路 8-bit 有符号大于比较
 * @param a 左操作数（16×i8）
 * @param b 右操作数（16×i8）
 * @return a > b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t cmpgt_i8(vec128_t a, vec128_t b) noexcept {
    vec128_t result;
    const vint8m1_t va = __riscv_vle8_v_i8m1(reinterpret_cast<const int8_t*>(scalar::lane_bytes(a)), 16);
    const vint8m1_t vb = __riscv_vle8_v_i8m1(reinterpret_cast<const int8_t*>(scalar::lane_bytes(b)), 16);
    const vbool8_t m = __riscv_vmsgt_vv_i8m1_b8(va, vb, 16);
    __riscv_vse8_v_u8m1(reinterpret_cast<uint8_t*>(scalar::lane_bytes(result)),
                        __riscv_vmerge_vvm_u8m1(__riscv_vmv_v_x_u8m1(0, 16), __riscv_vmv_v_x_u8m1(0xFF, 16), m, 16),
                        16);
    return result;
}

/**
 * @brief 8 路 16-bit 有符号大于比较
 * @param a 左操作数（8×i16）
 * @param b 右操作数（8×i16）
 * @return a > b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t cmpgt_i16(vec128_t a, vec128_t b) noexcept {
    vec128_t result;
    const vint16m1_t va = __riscv_vle16_v_i16m1(reinterpret_cast<const int16_t*>(scalar::lane_bytes(a)), 8);
    const vint16m1_t vb = __riscv_vle16_v_i16m1(reinterpret_cast<const int16_t*>(scalar::lane_bytes(b)), 8);
    const vbool16_t m = __riscv_vmsgt_vv_i16m1_b16(va, vb, 8);
    __riscv_vse16_v_u16m1(reinterpret_cast<uint16_t*>(scalar::lane_bytes(result)),
                          __riscv_vmerge_vvm_u16m1(__riscv_vmv_v_x_u16m1(0, 8), __riscv_vmv_v_x_u16m1(0xFFFF, 8), m, 8),
                          8);
    return result;
}

/**
 * @brief 4 路 32-bit 有符号大于比较
 * @param a 左操作数（4×i32）
 * @param b 右操作数（4×i32）
 * @return a > b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t cmpgt_i32(vec128_t a, vec128_t b) noexcept {
    vec128_t result;
    const vint32m1_t va = __riscv_vle32_v_i32m1(reinterpret_cast<const int32_t*>(scalar::lane_bytes(a)), 4);
    const vint32m1_t vb = __riscv_vle32_v_i32m1(reinterpret_cast<const int32_t*>(scalar::lane_bytes(b)), 4);
    const vbool32_t m = __riscv_vmsgt_vv_i32m1_b32(va, vb, 4);
    __riscv_vse32_v_u32m1(
            reinterpret_cast<uint32_t*>(scalar::lane_bytes(result)),
            __riscv_vmerge_vvm_u32m1(__riscv_vmv_v_x_u32m1(0, 4), __riscv_vmv_v_x_u32m1(0xFFFFFFFF, 4), m, 4), 4);
    return result;
}

/**
 * @brief 2 路 64-bit 有符号大于比较
 * @param a 左操作数（2×i64）
 * @param b 右操作数（2×i64）
 * @return a > b 位置为全 1，否则全 0
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t cmpgt_i64(vec128_t a, vec128_t b) noexcept {
    vec128_t result;
    const vint64m1_t va = __riscv_vle64_v_i64m1(reinterpret_cast<const int64_t*>(scalar::lane_bytes(a)), 2);
    const vint64m1_t vb = __riscv_vle64_v_i64m1(reinterpret_cast<const int64_t*>(scalar::lane_bytes(b)), 2);
    const vbool64_t m = __riscv_vmsgt_vv_i64m1_b64(va, vb, 2);
    __riscv_vse64_v_u64m1(reinterpret_cast<uint64_t*>(scalar::lane_bytes(result)),
                          __riscv_vmerge_vvm_u64m1(__riscv_vmv_v_x_u64m1(0, 2),
                                                   __riscv_vmv_v_x_u64m1(0xFFFFFFFFFFFFFFFFULL, 2), m, 2),
                          2);
    return result;
}


using scalar::cmpeq_f32;
using scalar::cmpeq_f64;
using scalar::cmpge_f32;
using scalar::cmpge_f64;
using scalar::cmpgt_f32;
using scalar::cmpgt_f64;
using scalar::cmple_f32;
using scalar::cmple_f64;
using scalar::cmplt_f32;
using scalar::cmplt_f64;

NEFORCE_END_SIMD__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_SIMD_BRANCH_RISCV
#endif // NEFORCE_CORE_SIMD_COMPARE_RISCV_HPP__
