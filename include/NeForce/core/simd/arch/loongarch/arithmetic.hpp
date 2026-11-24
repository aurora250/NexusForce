#ifndef NEFORCE_CORE_SIMD_ARITHMETIC_LOONGARCH_HPP__
#define NEFORCE_CORE_SIMD_ARITHMETIC_LOONGARCH_HPP__
#include "NeForce/core/simd/scalar/arithmetic.hpp"
#ifdef NEFORCE_SIMD_BRANCH_LOONGARCH
#    include <lsxintrin.h>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_SIMD__

/**
 * @brief 16 路 8-bit 有符号整数加法
 * @param a 左操作数（16×i8）
 * @param b 右操作数（16×i8）
 * @return a + b（16×i8，溢出回绕）
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t add_i8(vec128_t a, vec128_t b) noexcept { return __lsx_vadd_b(a, b); }

/**
 * @brief 8 路 16-bit 有符号整数加法
 * @param a 左操作数（8×i16）
 * @param b 右操作数（8×i16）
 * @return a + b（8×i16，溢出回绕）
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t add_i16(vec128_t a, vec128_t b) noexcept { return __lsx_vadd_h(a, b); }

/**
 * @brief 4 路 32-bit 有符号整数加法
 * @param a 左操作数（4×i32）
 * @param b 右操作数（4×i32）
 * @return a + b（4×i32，溢出回绕）
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t add_i32(vec128_t a, vec128_t b) noexcept { return __lsx_vadd_w(a, b); }

/**
 * @brief 2 路 64-bit 有符号整数加法
 * @param a 左操作数（2×i64）
 * @param b 右操作数（2×i64）
 * @return a + b（2×i64，溢出回绕）
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t add_i64(vec128_t a, vec128_t b) noexcept { return __lsx_vadd_d(a, b); }

/**
 * @brief 16 路 8-bit 有符号整数减法
 * @param a 左操作数（16×i8）
 * @param b 右操作数（16×i8）
 * @return a - b（16×i8，溢出回绕）
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t sub_i8(vec128_t a, vec128_t b) noexcept { return __lsx_vsub_b(a, b); }

/**
 * @brief 8 路 16-bit 有符号整数减法
 * @param a 左操作数（8×i16）
 * @param b 右操作数（8×i16）
 * @return a - b（8×i16，溢出回绕）
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t sub_i16(vec128_t a, vec128_t b) noexcept { return __lsx_vsub_h(a, b); }

/**
 * @brief 4 路 32-bit 有符号整数减法
 * @param a 左操作数（4×i32）
 * @param b 右操作数（4×i32）
 * @return a - b（4×i32，溢出回绕）
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t sub_i32(vec128_t a, vec128_t b) noexcept { return __lsx_vsub_w(a, b); }

/**
 * @brief 2 路 64-bit 有符号整数减法
 * @param a 左操作数（2×i64）
 * @param b 右操作数（2×i64）
 * @return a - b（2×i64，溢出回绕）
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t sub_i64(vec128_t a, vec128_t b) noexcept { return __lsx_vsub_d(a, b); }

/**
 * @brief 8 路 16-bit 整数乘法（取低半部分）
 * @param a 左操作数（8×i16）
 * @param b 右操作数（8×i16）
 * @return (a * b) & 0xFFFF（8×i16，低 16-bit）
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t mullo_i16(vec128_t a, vec128_t b) noexcept { return __lsx_vmul_h(a, b); }

/**
 * @brief 4 路 32-bit 整数乘法（取低半部分）
 * @param a 左操作数（4×i32）
 * @param b 右操作数（4×i32）
 * @return (a * b) & 0xFFFFFFFF（4×i32，低 32-bit）
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t mullo_i32(vec128_t a, vec128_t b) noexcept { return __lsx_vmul_w(a, b); }

/**
 * @brief 8 路 16-bit 有符号整数乘法（取高半部分）
 * @param a 左操作数（8×i16）
 * @param b 右操作数（8×i16）
 * @return (a * b) >> 16（8×i16，高 16-bit 算术结果）
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t mulhi_i16(vec128_t a, vec128_t b) noexcept {
    const auto* sa = reinterpret_cast<const int16_t*>(scalar::lane_bytes(a));
    const auto* sb = reinterpret_cast<const int16_t*>(scalar::lane_bytes(b));
    vec128_t result;
    auto* rd = reinterpret_cast<int16_t*>(scalar::lane_bytes(result));
    for (int i = 0; i < 8; ++i) {
        rd[i] = static_cast<int16_t>((static_cast<int32_t>(sa[i]) * static_cast<int32_t>(sb[i])) >> 16);
    }
    return result;
}

/**
 * @brief 有符号 8-bit 相邻对乘加：将相邻 i8 相乘后累加为 i16
 * @param a 左操作数（16×i8）
 * @param b 右操作数（16×i8）
 * @return [a[0]*b[0]+a[1]*b[1], a[2]*b[2]+a[3]*b[3], ...]（8×i16）
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t madds_i8x16(vec128_t a, vec128_t b) noexcept {
    const auto* sa = reinterpret_cast<const int8_t*>(scalar::lane_bytes(a));
    const auto* sb = reinterpret_cast<const int8_t*>(scalar::lane_bytes(b));
    vec128_t result;
    auto* rd = reinterpret_cast<int16_t*>(scalar::lane_bytes(result));
    for (int i = 0; i < 8; ++i) {
        rd[i] = static_cast<int16_t>(static_cast<int16_t>(sa[static_cast<ptrdiff_t>(2 * i)]) *
                                             static_cast<int16_t>(sb[static_cast<ptrdiff_t>(2 * i)]) +
                                     static_cast<int16_t>(sa[static_cast<ptrdiff_t>(2 * i + 1)]) *
                                             static_cast<int16_t>(sb[static_cast<ptrdiff_t>(2 * i + 1)]));
    }
    return result;
}

/**
 * @brief 16 路 8-bit 有符号饱和加法
 * @param a 左操作数（16×i8）
 * @param b 右操作数（16×i8）
 * @return 饱和到 [-128, 127] 的加法结果
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t saturated_add_i8(vec128_t a, vec128_t b) noexcept { return __lsx_vsadd_b(a, b); }

/**
 * @brief 8 路 16-bit 有符号饱和加法
 * @param a 左操作数（8×i16）
 * @param b 右操作数（8×i16）
 * @return 饱和到 [-32768, 32767] 的加法结果
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t saturated_add_i16(vec128_t a, vec128_t b) noexcept { return __lsx_vsadd_h(a, b); }

/**
 * @brief 16 路 8-bit 无符号饱和加法
 * @param a 左操作数（16×u8）
 * @param b 右操作数（16×u8）
 * @return 饱和到 [0, 255] 的加法结果
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t saturated_add_u8(vec128_t a, vec128_t b) noexcept { return __lsx_vsadd_bu(a, b); }

/**
 * @brief 8 路 16-bit 无符号饱和加法
 * @param a 左操作数（8×u16）
 * @param b 右操作数（8×u16）
 * @return 饱和到 [0, 65535] 的加法结果
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t saturated_add_u16(vec128_t a, vec128_t b) noexcept {
    return __lsx_vsadd_hu(a, b);
}

/**
 * @brief 16 路 8-bit 有符号饱和减法
 * @param a 左操作数（16×i8）
 * @param b 右操作数（16×i8）
 * @return 饱和到 [-128, 127] 的减法结果
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t saturated_sub_i8(vec128_t a, vec128_t b) noexcept { return __lsx_vssub_b(a, b); }

/**
 * @brief 8 路 16-bit 有符号饱和减法
 * @param a 左操作数（8×i16）
 * @param b 右操作数（8×i16）
 * @return 饱和到 [-32768, 32767] 的减法结果
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t saturated_sub_i16(vec128_t a, vec128_t b) noexcept { return __lsx_vssub_h(a, b); }

/**
 * @brief 16 路 8-bit 无符号饱和减法
 * @param a 左操作数（16×u8）
 * @param b 右操作数（16×u8）
 * @return 饱和到 [0, 255] 的减法结果
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t saturated_sub_u8(vec128_t a, vec128_t b) noexcept { return __lsx_vssub_bu(a, b); }

/**
 * @brief 8 路 16-bit 无符号饱和减法
 * @param a 左操作数（8×u16）
 * @param b 右操作数（8×u16）
 * @return 饱和到 [0, 65535] 的减法结果
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t saturated_sub_u16(vec128_t a, vec128_t b) noexcept {
    return __lsx_vssub_hu(a, b);
}

/**
 * @brief 16 路 8-bit 有符号整数绝对值
 * @param v 输入向量（16×i8）
 * @return scalar::abs(v)（16×i8）
 * @note 输入 INT8_MIN 时不饱和
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t abs_i8(vec128_t v) noexcept { return __lsx_vabsd_b(v, __lsx_vreplgr2vr_b(0)); }

/**
 * @brief 8 路 16-bit 有符号整数绝对值
 * @param v 输入向量（8×i16）
 * @return scalar::abs(v)（8×i16）
 * @note 输入 INT16_MIN 时不饱和
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t abs_i16(vec128_t v) noexcept { return __lsx_vabsd_h(v, __lsx_vreplgr2vr_h(0)); }

/**
 * @brief 4 路 32-bit 有符号整数绝对值
 * @param v 输入向量（4×i32）
 * @return scalar::abs(v)（4×i32）
 * @note 输入 INT32_MIN 时不饱和
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t abs_i32(vec128_t v) noexcept { return __lsx_vabsd_w(v, __lsx_vreplgr2vr_w(0)); }

/**
 * @brief 16 路 8-bit 有符号整数最小值
 * @param a 左操作数（16×i8）
 * @param b 右操作数（16×i8）
 * @return scalar::min(a, b) 逐元素
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t min_i8(vec128_t a, vec128_t b) noexcept { return __lsx_vmin_b(a, b); }

/**
 * @brief 8 路 16-bit 有符号整数最小值
 * @param a 左操作数（8×i16）
 * @param b 右操作数（8×i16）
 * @return scalar::min(a, b) 逐元素
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t min_i16(vec128_t a, vec128_t b) noexcept { return __lsx_vmin_h(a, b); }

/**
 * @brief 4 路 32-bit 有符号整数最小值
 * @param a 左操作数（4×i32）
 * @param b 右操作数（4×i32）
 * @return scalar::min(a, b) 逐元素
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t min_i32(vec128_t a, vec128_t b) noexcept { return __lsx_vmin_w(a, b); }

/**
 * @brief 16 路 8-bit 无符号整数最小值
 * @param a 左操作数（16×u8）
 * @param b 右操作数（16×u8）
 * @return scalar::min(a, b) 逐元素
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t min_u8(vec128_t a, vec128_t b) noexcept { return __lsx_vmin_bu(a, b); }

/**
 * @brief 8 路 16-bit 无符号整数最小值
 * @param a 左操作数（8×u16）
 * @param b 右操作数（8×u16）
 * @return scalar::min(a, b) 逐元素
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t min_u16(vec128_t a, vec128_t b) noexcept { return __lsx_vmin_hu(a, b); }

/**
 * @brief 4 路 32-bit 无符号整数最小值
 * @param a 左操作数（4×u32）
 * @param b 右操作数（4×u32）
 * @return scalar::min(a, b) 逐元素
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t min_u32(vec128_t a, vec128_t b) noexcept { return __lsx_vmin_wu(a, b); }

/**
 * @brief 16 路 8-bit 有符号整数最大值
 * @param a 左操作数（16×i8）
 * @param b 右操作数（16×i8）
 * @return scalar::max(a, b) 逐元素
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t max_i8(vec128_t a, vec128_t b) noexcept { return __lsx_vmax_b(a, b); }

/**
 * @brief 8 路 16-bit 有符号整数最大值
 * @param a 左操作数（8×i16）
 * @param b 右操作数（8×i16）
 * @return scalar::max(a, b) 逐元素
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t max_i16(vec128_t a, vec128_t b) noexcept { return __lsx_vmax_h(a, b); }

/**
 * @brief 4 路 32-bit 有符号整数最大值
 * @param a 左操作数（4×i32）
 * @param b 右操作数（4×i32）
 * @return scalar::max(a, b) 逐元素
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t max_i32(vec128_t a, vec128_t b) noexcept { return __lsx_vmax_w(a, b); }

/**
 * @brief 16 路 8-bit 无符号整数最大值
 * @param a 左操作数（16×u8）
 * @param b 右操作数（16×u8）
 * @return scalar::max(a, b) 逐元素
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t max_u8(vec128_t a, vec128_t b) noexcept { return __lsx_vmax_bu(a, b); }

/**
 * @brief 8 路 16-bit 无符号整数最大值
 * @param a 左操作数（8×u16）
 * @param b 右操作数（8×u16）
 * @return scalar::max(a, b) 逐元素
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t max_u16(vec128_t a, vec128_t b) noexcept { return __lsx_vmax_hu(a, b); }

/**
 * @brief 4 路 32-bit 无符号整数最大值
 * @param a 左操作数（4×u32）
 * @param b 右操作数（4×u32）
 * @return scalar::max(a, b) 逐元素
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t max_u32(vec128_t a, vec128_t b) noexcept { return __lsx_vmax_wu(a, b); }

/**
 * @brief 16 路 8-bit 无符号整数平均值（向偶数舍入）
 * @param a 左操作数（16×u8）
 * @param b 右操作数（16×u8）
 * @return (a + b + 1) / 2 逐元素
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t avg_u8(vec128_t a, vec128_t b) noexcept { return __lsx_vavgr_bu(a, b); }

/**
 * @brief 8 路 16-bit 无符号整数平均值（向偶数舍入）
 * @param a 左操作数（8×u16）
 * @param b 右操作数（8×u16）
 * @return (a + b + 1) / 2 逐元素
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t avg_u16(vec128_t a, vec128_t b) noexcept { return __lsx_vavgr_hu(a, b); }

NEFORCE_END_SIMD__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_SIMD_BRANCH_LOONGARCH
#endif // NEFORCE_CORE_SIMD_ARITHMETIC_LOONGARCH_HPP__
