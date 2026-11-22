#ifndef NEFORCE_CORE_SIMD_ARITHMETIC_ARM_HPP__
#define NEFORCE_CORE_SIMD_ARITHMETIC_ARM_HPP__
#include "NeForce/core/numeric/numeric_traits.hpp"
#include "NeForce/core/simd/types.hpp"
#ifdef NEFORCE_SIMD_BRANCH_ARM
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_SIMD__

/**
 * @addtogroup SIMD
 * @{
 */

/**
 * @brief 16 路 8-bit 有符号整数加法
 * @param a 左操作数（16×i8）
 * @param b 右操作数（16×i8）
 * @return a + b（16×i8，溢出回绕）
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t add_i8(vec128_t a, vec128_t b) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    return ::vreinterpretq_u8_s8(::vaddq_s8(::vreinterpretq_s8_u8(a), ::vreinterpretq_s8_u8(b)));
#    else
    vec128_t result;
    for (int i = 0; i < 16; ++i) {
        result.data[i] = static_cast<byte_t>(static_cast<int8_t>(a.data[i]) + static_cast<int8_t>(b.data[i]));
    }
    return result;
#    endif
}

/**
 * @brief 8 路 16-bit 有符号整数加法
 * @param a 左操作数（8×i16）
 * @param b 右操作数（8×i16）
 * @return a + b（8×i16，溢出回绕）
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t add_i16(vec128_t a, vec128_t b) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    return ::vreinterpretq_u8_s16(::vaddq_s16(::vreinterpretq_s16_u8(a), ::vreinterpretq_s16_u8(b)));
#    else
    const auto* sa = reinterpret_cast<const int16_t*>(a.data);
    const auto* sb = reinterpret_cast<const int16_t*>(b.data);
    vec128_t result;
    auto* rd = reinterpret_cast<int16_t*>(result.data);
    for (int i = 0; i < 8; ++i) {
        rd[i] = static_cast<int16_t>(sa[i] + sb[i]);
    }
    return result;
#    endif
}

/**
 * @brief 4 路 32-bit 有符号整数加法
 * @param a 左操作数（4×i32）
 * @param b 右操作数（4×i32）
 * @return a + b（4×i32，溢出回绕）
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t add_i32(vec128_t a, vec128_t b) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    return ::vreinterpretq_u8_s32(::vaddq_s32(::vreinterpretq_s32_u8(a), ::vreinterpretq_s32_u8(b)));
#    else
    const auto* sa = reinterpret_cast<const int32_t*>(a.data);
    const auto* sb = reinterpret_cast<const int32_t*>(b.data);
    vec128_t result;
    auto* rd = reinterpret_cast<int32_t*>(result.data);
    for (int i = 0; i < 4; ++i) {
        rd[i] = sa[i] + sb[i];
    }
    return result;
#    endif
}

/**
 * @brief 2 路 64-bit 有符号整数加法
 * @param a 左操作数（2×i64）
 * @param b 右操作数（2×i64）
 * @return a + b（2×i64，溢出回绕）
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t add_i64(vec128_t a, vec128_t b) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    return ::vreinterpretq_u8_s64(::vaddq_s64(::vreinterpretq_s64_u8(a), ::vreinterpretq_s64_u8(b)));
#    else
    const auto* sa = reinterpret_cast<const int64_t*>(a.data);
    const auto* sb = reinterpret_cast<const int64_t*>(b.data);
    vec128_t result;
    auto* rd = reinterpret_cast<int64_t*>(result.data);
    for (int i = 0; i < 2; ++i) {
        rd[i] = sa[i] + sb[i];
    }
    return result;
#    endif
}

/**
 * @brief 16 路 8-bit 有符号整数减法
 * @param a 左操作数（16×i8）
 * @param b 右操作数（16×i8）
 * @return a - b（16×i8，溢出回绕）
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t sub_i8(vec128_t a, vec128_t b) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    return ::vreinterpretq_u8_s8(::vsubq_s8(::vreinterpretq_s8_u8(a), ::vreinterpretq_s8_u8(b)));
#    else
    vec128_t result;
    for (int i = 0; i < 16; ++i) {
        result.data[i] = static_cast<byte_t>(static_cast<int8_t>(a.data[i]) - static_cast<int8_t>(b.data[i]));
    }
    return result;
#    endif
}

/**
 * @brief 8 路 16-bit 有符号整数减法
 * @param a 左操作数（8×i16）
 * @param b 右操作数（8×i16）
 * @return a - b（8×i16，溢出回绕）
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t sub_i16(vec128_t a, vec128_t b) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    return ::vreinterpretq_u8_s16(::vsubq_s16(::vreinterpretq_s16_u8(a), ::vreinterpretq_s16_u8(b)));
#    else
    const auto* sa = reinterpret_cast<const int16_t*>(a.data);
    const auto* sb = reinterpret_cast<const int16_t*>(b.data);
    vec128_t result;
    auto* rd = reinterpret_cast<int16_t*>(result.data);
    for (int i = 0; i < 8; ++i) {
        rd[i] = static_cast<int16_t>(sa[i] - sb[i]);
    }
    return result;
#    endif
}

/**
 * @brief 4 路 32-bit 有符号整数减法
 * @param a 左操作数（4×i32）
 * @param b 右操作数（4×i32）
 * @return a - b（4×i32，溢出回绕）
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t sub_i32(vec128_t a, vec128_t b) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    return ::vreinterpretq_u8_s32(::vsubq_s32(::vreinterpretq_s32_u8(a), ::vreinterpretq_s32_u8(b)));
#    else
    const auto* sa = reinterpret_cast<const int32_t*>(a.data);
    const auto* sb = reinterpret_cast<const int32_t*>(b.data);
    vec128_t result;
    auto* rd = reinterpret_cast<int32_t*>(result.data);
    for (int i = 0; i < 4; ++i) {
        rd[i] = sa[i] - sb[i];
    }
    return result;
#    endif
}

/**
 * @brief 2 路 64-bit 有符号整数减法
 * @param a 左操作数（2×i64）
 * @param b 右操作数（2×i64）
 * @return a - b（2×i64，溢出回绕）
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t sub_i64(vec128_t a, vec128_t b) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    return ::vreinterpretq_u8_s64(::vsubq_s64(::vreinterpretq_s64_u8(a), ::vreinterpretq_s64_u8(b)));
#    else
    const auto* sa = reinterpret_cast<const int64_t*>(a.data);
    const auto* sb = reinterpret_cast<const int64_t*>(b.data);
    vec128_t result;
    auto* rd = reinterpret_cast<int64_t*>(result.data);
    for (int i = 0; i < 2; ++i) {
        rd[i] = sa[i] - sb[i];
    }
    return result;
#    endif
}

/**
 * @brief 8 路 16-bit 整数乘法（取低半部分）
 * @param a 左操作数（8×i16）
 * @param b 右操作数（8×i16）
 * @return (a * b) & 0xFFFF（8×i16，低 16-bit）
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t mullo_i16(vec128_t a, vec128_t b) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    return ::vreinterpretq_u8_s16(::vmulq_s16(::vreinterpretq_s16_u8(a), ::vreinterpretq_s16_u8(b)));
#    else
    const auto* sa = reinterpret_cast<const int16_t*>(a.data);
    const auto* sb = reinterpret_cast<const int16_t*>(b.data);
    vec128_t result;
    auto* rd = reinterpret_cast<int16_t*>(result.data);
    for (int i = 0; i < 8; ++i) {
        rd[i] = static_cast<int16_t>(sa[i] * sb[i]);
    }
    return result;
#    endif
}

/**
 * @brief 4 路 32-bit 整数乘法（取低半部分）
 * @param a 左操作数（4×i32）
 * @param b 右操作数（4×i32）
 * @return (a * b) & 0xFFFFFFFF（4×i32，低 32-bit）
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t mullo_i32(vec128_t a, vec128_t b) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    return ::vreinterpretq_u8_s32(::vmulq_s32(::vreinterpretq_s32_u8(a), ::vreinterpretq_s32_u8(b)));
#    else
    const auto* sa = reinterpret_cast<const int32_t*>(a.data);
    const auto* sb = reinterpret_cast<const int32_t*>(b.data);
    vec128_t result;
    auto* rd = reinterpret_cast<int32_t*>(result.data);
    for (int i = 0; i < 4; ++i) {
        rd[i] = static_cast<int32_t>(sa[i] * sb[i]);
    }
    return result;
#    endif
}

/**
 * @brief 8 路 16-bit 有符号整数乘法（取高半部分）
 * @param a 左操作数（8×i16）
 * @param b 右操作数（8×i16）
 * @return (a * b) >> 16（8×i16，高 16-bit 算术结果）
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t mulhi_i16(vec128_t a, vec128_t b) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    const ::int16x8_t sa = ::vreinterpretq_s16_u8(a);
    const ::int16x8_t sb = ::vreinterpretq_s16_u8(b);
    const ::int16x4_t a_lo = ::vget_low_s16(sa);
    const ::int16x4_t b_lo = ::vget_low_s16(sb);
    const ::int16x4_t a_hi = ::vget_high_s16(sa);
    const ::int16x4_t b_hi = ::vget_high_s16(sb);
    const ::int32x4_t prod_lo = ::vmull_s16(a_lo, b_lo);
    const ::int32x4_t prod_hi = ::vmull_s16(a_hi, b_hi);
    const ::int16x4_t hi_lo = vshrn_n_s32(prod_lo, 16);
    const ::int16x4_t hi_hi = vshrn_n_s32(prod_hi, 16);
    return ::vreinterpretq_u8_s16(::vcombine_s16(hi_lo, hi_hi));
#    else
    const auto* sa = reinterpret_cast<const int16_t*>(a.data);
    const auto* sb = reinterpret_cast<const int16_t*>(b.data);
    vec128_t result;
    auto* rd = reinterpret_cast<int16_t*>(result.data);
    for (int i = 0; i < 8; ++i) {
        rd[i] = static_cast<int16_t>((static_cast<int32_t>(sa[i]) * static_cast<int32_t>(sb[i])) >> 16);
    }
    return result;
#    endif
}

/**
 * @brief 有符号 8-bit 相邻对乘加：将相邻 i8 相乘后累加为 i16
 * @param a 左操作数（16×i8）
 * @param b 右操作数（16×i8）
 * @return [a[0]*b[0]+a[1]*b[1], a[2]*b[2]+a[3]*b[3], ...]（8×i16）
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t madds_i8x16(vec128_t a, vec128_t b) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    const ::int8x16_t sa = ::vreinterpretq_s8_u8(a);
    const ::int8x16_t sb = ::vreinterpretq_s8_u8(b);
    const ::int16x8_t prod = ::vmull_s8(::vget_low_s8(sa), ::vget_low_s8(sb));
    const ::int16x8_t prod_hi = ::vmull_s8(::vget_high_s8(sa), ::vget_high_s8(sb));
    return ::vreinterpretq_u8_s16(::vpaddq_s16(prod, prod_hi));
#    else
    const auto* sa = reinterpret_cast<const int8_t*>(a.data);
    const auto* sb = reinterpret_cast<const int8_t*>(b.data);
    vec128_t result;
    auto* rd = reinterpret_cast<int16_t*>(result.data);
    for (int i = 0; i < 8; ++i) {
        rd[i] = static_cast<int16_t>(static_cast<int16_t>(sa[static_cast<ptrdiff_t>(2 * i)]) *
                                             static_cast<int16_t>(sb[static_cast<ptrdiff_t>(2 * i)]) +
                                     static_cast<int16_t>(sa[static_cast<ptrdiff_t>(2 * i + 1)]) *
                                             static_cast<int16_t>(sb[static_cast<ptrdiff_t>(2 * i + 1)]));
    }
    return result;
#    endif
}

/**
 * @brief 16 路 8-bit 有符号饱和加法
 * @param a 左操作数（16×i8）
 * @param b 右操作数（16×i8）
 * @return 饱和到 [-128, 127] 的加法结果
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t saturated_add_i8(vec128_t a, vec128_t b) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    return ::vreinterpretq_u8_s8(::vqaddq_s8(::vreinterpretq_s8_u8(a), ::vreinterpretq_s8_u8(b)));
#    else
    vec128_t result;
    for (int i = 0; i < 16; ++i) {
        const int val = static_cast<int8_t>(a.data[i]) + static_cast<int8_t>(b.data[i]);
        if (val > 127) {
            result.data[i] = 127;
        } else if (val < -128) {
            result.data[i] = 128;
        } else {
            result.data[i] = static_cast<byte_t>(static_cast<int8_t>(val));
        }
    }
    return result;
#    endif
}

/**
 * @brief 8 路 16-bit 有符号饱和加法
 * @param a 左操作数（8×i16）
 * @param b 右操作数（8×i16）
 * @return 饱和到 [-32768, 32767] 的加法结果
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t saturated_add_i16(vec128_t a, vec128_t b) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    return ::vreinterpretq_u8_s16(::vqaddq_s16(::vreinterpretq_s16_u8(a), ::vreinterpretq_s16_u8(b)));
#    else
    const auto* sa = reinterpret_cast<const int16_t*>(a.data);
    const auto* sb = reinterpret_cast<const int16_t*>(b.data);
    vec128_t result;
    auto* rd = reinterpret_cast<int16_t*>(result.data);
    for (int i = 0; i < 8; ++i) {
        const int val = sa[i] + sb[i];
        if (val > 32767) {
            rd[i] = 32767;
        } else if (val < -32768) {
            rd[i] = -32768;
        } else {
            rd[i] = static_cast<int16_t>(val);
        }
    }
    return result;
#    endif
}

/**
 * @brief 16 路 8-bit 无符号饱和加法
 * @param a 左操作数（16×u8）
 * @param b 右操作数（16×u8）
 * @return 饱和到 [0, 255] 的加法结果
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t saturated_add_u8(vec128_t a, vec128_t b) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    return ::vqaddq_u8(a, b);
#    else
    vec128_t result;
    for (int i = 0; i < 16; ++i) {
        const int val = static_cast<int>(a.data[i]) + static_cast<int>(b.data[i]);
        result.data[i] = static_cast<byte_t>(val > 255 ? 255 : val);
    }
    return result;
#    endif
}

/**
 * @brief 8 路 16-bit 无符号饱和加法
 * @param a 左操作数（8×u16）
 * @param b 右操作数（8×u16）
 * @return 饱和到 [0, 65535] 的加法结果
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t saturated_add_u16(vec128_t a, vec128_t b) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    return ::vreinterpretq_u8_u16(::vqaddq_u16(::vreinterpretq_u16_u8(a), ::vreinterpretq_u16_u8(b)));
#    else
    vec128_t result;
    for (int i = 0; i < 8; ++i) {
        const int val = static_cast<int>(reinterpret_cast<const uint16_t*>(a.data)[i]) +
                        static_cast<int>(reinterpret_cast<const uint16_t*>(b.data)[i]);
        reinterpret_cast<uint16_t*>(result.data)[i] = static_cast<uint16_t>(val > 65535 ? 65535 : val);
    }
    return result;
#    endif
}

/**
 * @brief 16 路 8-bit 有符号饱和减法
 * @param a 左操作数（16×i8）
 * @param b 右操作数（16×i8）
 * @return 饱和到 [-128, 127] 的减法结果
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t saturated_sub_i8(vec128_t a, vec128_t b) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    return ::vreinterpretq_u8_s8(::vqsubq_s8(::vreinterpretq_s8_u8(a), ::vreinterpretq_s8_u8(b)));
#    else
    vec128_t result;
    for (int i = 0; i < 16; ++i) {
        const int val = static_cast<int8_t>(a.data[i]) - static_cast<int8_t>(b.data[i]);
        if (val > 127) {
            result.data[i] = 127;
        } else if (val < -128) {
            result.data[i] = 128;
        } else {
            result.data[i] = static_cast<byte_t>(static_cast<int8_t>(val));
        }
    }
    return result;
#    endif
}

/**
 * @brief 8 路 16-bit 有符号饱和减法
 * @param a 左操作数（8×i16）
 * @param b 右操作数（8×i16）
 * @return 饱和到 [-32768, 32767] 的减法结果
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t saturated_sub_i16(vec128_t a, vec128_t b) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    return ::vreinterpretq_u8_s16(::vqsubq_s16(::vreinterpretq_s16_u8(a), ::vreinterpretq_s16_u8(b)));
#    else
    const auto* sa = reinterpret_cast<const int16_t*>(a.data);
    const auto* sb = reinterpret_cast<const int16_t*>(b.data);
    vec128_t result;
    auto* rd = reinterpret_cast<int16_t*>(result.data);
    for (int i = 0; i < 8; ++i) {
        const int val = sa[i] - sb[i];
        if (val > 32767) {
            rd[i] = 32767;
        } else if (val < -32768) {
            rd[i] = -32768;
        } else {
            rd[i] = static_cast<int16_t>(val);
        }
    }
    return result;
#    endif
}

/**
 * @brief 16 路 8-bit 无符号饱和减法
 * @param a 左操作数（16×u8）
 * @param b 右操作数（16×u8）
 * @return 饱和到 [0, 255] 的减法结果
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t saturated_sub_u8(vec128_t a, vec128_t b) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    return ::vqsubq_u8(a, b);
#    else
    vec128_t result;
    for (int i = 0; i < 16; ++i) {
        const int val = static_cast<int>(a.data[i]) - static_cast<int>(b.data[i]);
        result.data[i] = static_cast<byte_t>(val < 0 ? 0 : val);
    }
    return result;
#    endif
}

/**
 * @brief 8 路 16-bit 无符号饱和减法
 * @param a 左操作数（8×u16）
 * @param b 右操作数（8×u16）
 * @return 饱和到 [0, 65535] 的减法结果
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t saturated_sub_u16(vec128_t a, vec128_t b) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    return ::vreinterpretq_u8_u16(::vqsubq_u16(::vreinterpretq_u16_u8(a), ::vreinterpretq_u16_u8(b)));
#    else
    vec128_t result;
    for (int i = 0; i < 8; ++i) {
        const int val = static_cast<int>(reinterpret_cast<const uint16_t*>(a.data)[i]) -
                        static_cast<int>(reinterpret_cast<const uint16_t*>(b.data)[i]);
        reinterpret_cast<uint16_t*>(result.data)[i] = static_cast<uint16_t>(val < 0 ? 0 : val);
    }
    return result;
#    endif
}

/**
 * @brief 16 路 8-bit 有符号整数绝对值
 * @param v 输入向量（16×i8）
 * @return abs(v)（16×i8）
 * @note 输入 INT8_MIN 时不饱和
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t abs_i8(vec128_t v) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    return ::vreinterpretq_u8_s8(::vabsq_s8(::vreinterpretq_s8_u8(v)));
#    else
    vec128_t result;
    for (int i = 0; i < 16; ++i) {
        const auto val = static_cast<int8_t>(v.data[i]);
        // Use unsigned negation to avoid UB, matches x86 PABSB wrap-around behavior
        result.data[i] =
                val < 0 ? static_cast<byte_t>(static_cast<int8_t>(-static_cast<unsigned>(static_cast<uint8_t>(val))))
                        : static_cast<byte_t>(val);
    }
    return result;
#    endif
}

/**
 * @brief 8 路 16-bit 有符号整数绝对值
 * @param v 输入向量（8×i16）
 * @return abs(v)（8×i16）
 * @note 输入 INT16_MIN 时不饱和
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t abs_i16(vec128_t v) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    return ::vreinterpretq_u8_s16(::vabsq_s16(::vreinterpretq_s16_u8(v)));
#    else
    const auto* sv = reinterpret_cast<const int16_t*>(v.data);
    vec128_t result;
    auto* rd = reinterpret_cast<int16_t*>(result.data);
    for (int i = 0; i < 8; ++i) {
        const int16_t val = sv[i];
        rd[i] = val < 0 ? static_cast<int16_t>(-static_cast<int>(static_cast<uint16_t>(val))) : val;
    }
    return result;
#    endif
}

/**
 * @brief 4 路 32-bit 有符号整数绝对值
 * @param v 输入向量（4×i32）
 * @return abs(v)（4×i32）
 * @note 输入 INT32_MIN 时不饱和
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t abs_i32(vec128_t v) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    return ::vreinterpretq_u8_s32(::vabsq_s32(::vreinterpretq_s32_u8(v)));
#    else
    const auto* sv = reinterpret_cast<const int32_t*>(v.data);
    vec128_t result;
    auto* rd = reinterpret_cast<int32_t*>(result.data);
    for (int i = 0; i < 4; ++i) {
        const int32_t val = sv[i];
        rd[i] = val < 0 ? static_cast<int32_t>(-static_cast<int64_t>(static_cast<uint32_t>(val))) : val;
    }
    return result;
#    endif
}

/**
 * @brief 16 路 8-bit 有符号整数最小值
 * @param a 左操作数（16×i8）
 * @param b 右操作数（16×i8）
 * @return min(a, b) 逐元素
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t min_i8(vec128_t a, vec128_t b) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    return ::vreinterpretq_u8_s8(::vminq_s8(::vreinterpretq_s8_u8(a), ::vreinterpretq_s8_u8(b)));
#    else
    vec128_t result;
    for (int i = 0; i < 16; ++i) {
        const auto va = static_cast<int8_t>(a.data[i]);
        const auto vb = static_cast<int8_t>(b.data[i]);
        result.data[i] = static_cast<byte_t>(va < vb ? va : vb);
    }
    return result;
#    endif
}

/**
 * @brief 8 路 16-bit 有符号整数最小值
 * @param a 左操作数（8×i16）
 * @param b 右操作数（8×i16）
 * @return min(a, b) 逐元素
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t min_i16(vec128_t a, vec128_t b) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    return ::vreinterpretq_u8_s16(::vminq_s16(::vreinterpretq_s16_u8(a), ::vreinterpretq_s16_u8(b)));
#    else
    const auto* sa = reinterpret_cast<const int16_t*>(a.data);
    const auto* sb = reinterpret_cast<const int16_t*>(b.data);
    vec128_t result;
    auto* rd = reinterpret_cast<int16_t*>(result.data);
    for (int i = 0; i < 8; ++i) {
        rd[i] = sa[i] < sb[i] ? sa[i] : sb[i];
    }
    return result;
#    endif
}

/**
 * @brief 4 路 32-bit 有符号整数最小值
 * @param a 左操作数（4×i32）
 * @param b 右操作数（4×i32）
 * @return min(a, b) 逐元素
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t min_i32(vec128_t a, vec128_t b) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    return ::vreinterpretq_u8_s32(::vminq_s32(::vreinterpretq_s32_u8(a), ::vreinterpretq_s32_u8(b)));
#    else
    const auto* sa = reinterpret_cast<const int32_t*>(a.data);
    const auto* sb = reinterpret_cast<const int32_t*>(b.data);
    vec128_t result;
    auto* rd = reinterpret_cast<int32_t*>(result.data);
    for (int i = 0; i < 4; ++i) {
        rd[i] = sa[i] < sb[i] ? sa[i] : sb[i];
    }
    return result;
#    endif
}

/**
 * @brief 16 路 8-bit 无符号整数最小值
 * @param a 左操作数（16×u8）
 * @param b 右操作数（16×u8）
 * @return min(a, b) 逐元素
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t min_u8(vec128_t a, vec128_t b) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    return ::vminq_u8(a, b);
#    else
    vec128_t result;
    for (int i = 0; i < 16; ++i) {
        result.data[i] = a.data[i] < b.data[i] ? a.data[i] : b.data[i];
    }
    return result;
#    endif
}

/**
 * @brief 8 路 16-bit 无符号整数最小值
 * @param a 左操作数（8×u16）
 * @param b 右操作数（8×u16）
 * @return min(a, b) 逐元素
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t min_u16(vec128_t a, vec128_t b) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    return ::vreinterpretq_u8_u16(::vminq_u16(::vreinterpretq_u16_u8(a), ::vreinterpretq_u16_u8(b)));
#    else
    const auto* ua = reinterpret_cast<const uint16_t*>(a.data);
    const auto* ub = reinterpret_cast<const uint16_t*>(b.data);
    vec128_t result;
    auto* rd = reinterpret_cast<uint16_t*>(result.data);
    for (int i = 0; i < 8; ++i) {
        rd[i] = ua[i] < ub[i] ? ua[i] : ub[i];
    }
    return result;
#    endif
}

/**
 * @brief 4 路 32-bit 无符号整数最小值
 * @param a 左操作数（4×u32）
 * @param b 右操作数（4×u32）
 * @return min(a, b) 逐元素
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t min_u32(vec128_t a, vec128_t b) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    return ::vreinterpretq_u8_u32(::vminq_u32(::vreinterpretq_u32_u8(a), ::vreinterpretq_u32_u8(b)));
#    else
    const auto* ua = reinterpret_cast<const uint32_t*>(a.data);
    const auto* ub = reinterpret_cast<const uint32_t*>(b.data);
    vec128_t result;
    auto* rd = reinterpret_cast<uint32_t*>(result.data);
    for (int i = 0; i < 4; ++i) {
        rd[i] = ua[i] < ub[i] ? ua[i] : ub[i];
    }
    return result;
#    endif
}

/**
 * @brief 16 路 8-bit 有符号整数最大值
 * @param a 左操作数（16×i8）
 * @param b 右操作数（16×i8）
 * @return max(a, b) 逐元素
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t max_i8(vec128_t a, vec128_t b) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    return ::vreinterpretq_u8_s8(::vmaxq_s8(::vreinterpretq_s8_u8(a), ::vreinterpretq_s8_u8(b)));
#    else
    vec128_t result;
    for (int i = 0; i < 16; ++i) {
        const auto va = static_cast<int8_t>(a.data[i]);
        const auto vb = static_cast<int8_t>(b.data[i]);
        result.data[i] = static_cast<byte_t>(va > vb ? va : vb);
    }
    return result;
#    endif
}

/**
 * @brief 8 路 16-bit 有符号整数最大值
 * @param a 左操作数（8×i16）
 * @param b 右操作数（8×i16）
 * @return max(a, b) 逐元素
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t max_i16(vec128_t a, vec128_t b) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    return ::vreinterpretq_u8_s16(::vmaxq_s16(::vreinterpretq_s16_u8(a), ::vreinterpretq_s16_u8(b)));
#    else
    const auto* sa = reinterpret_cast<const int16_t*>(a.data);
    const auto* sb = reinterpret_cast<const int16_t*>(b.data);
    vec128_t result;
    auto* rd = reinterpret_cast<int16_t*>(result.data);
    for (int i = 0; i < 8; ++i) {
        rd[i] = sa[i] > sb[i] ? sa[i] : sb[i];
    }
    return result;
#    endif
}

/**
 * @brief 4 路 32-bit 有符号整数最大值
 * @param a 左操作数（4×i32）
 * @param b 右操作数（4×i32）
 * @return max(a, b) 逐元素
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t max_i32(vec128_t a, vec128_t b) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    return ::vreinterpretq_u8_s32(::vmaxq_s32(::vreinterpretq_s32_u8(a), ::vreinterpretq_s32_u8(b)));
#    else
    const auto* sa = reinterpret_cast<const int32_t*>(a.data);
    const auto* sb = reinterpret_cast<const int32_t*>(b.data);
    vec128_t result;
    auto* rd = reinterpret_cast<int32_t*>(result.data);
    for (int i = 0; i < 4; ++i) {
        rd[i] = sa[i] > sb[i] ? sa[i] : sb[i];
    }
    return result;
#    endif
}

/**
 * @brief 16 路 8-bit 无符号整数最大值
 * @param a 左操作数（16×u8）
 * @param b 右操作数（16×u8）
 * @return max(a, b) 逐元素
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t max_u8(vec128_t a, vec128_t b) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    return ::vmaxq_u8(a, b);
#    else
    vec128_t result;
    for (int i = 0; i < 16; ++i) {
        result.data[i] = a.data[i] > b.data[i] ? a.data[i] : b.data[i];
    }
    return result;
#    endif
}

/**
 * @brief 8 路 16-bit 无符号整数最大值
 * @param a 左操作数（8×u16）
 * @param b 右操作数（8×u16）
 * @return max(a, b) 逐元素
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t max_u16(vec128_t a, vec128_t b) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    return ::vreinterpretq_u8_u16(::vmaxq_u16(::vreinterpretq_u16_u8(a), ::vreinterpretq_u16_u8(b)));
#    else
    const auto* ua = reinterpret_cast<const uint16_t*>(a.data);
    const auto* ub = reinterpret_cast<const uint16_t*>(b.data);
    vec128_t result;
    auto* rd = reinterpret_cast<uint16_t*>(result.data);
    for (int i = 0; i < 8; ++i) {
        rd[i] = ua[i] > ub[i] ? ua[i] : ub[i];
    }
    return result;
#    endif
}

/**
 * @brief 4 路 32-bit 无符号整数最大值
 * @param a 左操作数（4×u32）
 * @param b 右操作数（4×u32）
 * @return max(a, b) 逐元素
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t max_u32(vec128_t a, vec128_t b) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    return ::vreinterpretq_u8_u32(::vmaxq_u32(::vreinterpretq_u32_u8(a), ::vreinterpretq_u32_u8(b)));
#    else
    const auto* ua = reinterpret_cast<const uint32_t*>(a.data);
    const auto* ub = reinterpret_cast<const uint32_t*>(b.data);
    vec128_t result;
    auto* rd = reinterpret_cast<uint32_t*>(result.data);
    for (int i = 0; i < 4; ++i) {
        rd[i] = ua[i] > ub[i] ? ua[i] : ub[i];
    }
    return result;
#    endif
}

/**
 * @brief 16 路 8-bit 无符号整数平均值（向偶数舍入）
 * @param a 左操作数（16×u8）
 * @param b 右操作数（16×u8）
 * @return (a + b + 1) / 2 逐元素
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t avg_u8(vec128_t a, vec128_t b) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    return ::vrhaddq_u8(a, b);
#    else
    vec128_t result;
    for (int i = 0; i < 16; ++i) {
        result.data[i] = static_cast<byte_t>((static_cast<int>(a.data[i]) + static_cast<int>(b.data[i]) + 1) >> 1);
    }
    return result;
#    endif
}

/**
 * @brief 8 路 16-bit 无符号整数平均值（向偶数舍入）
 * @param a 左操作数（8×u16）
 * @param b 右操作数（8×u16）
 * @return (a + b + 1) / 2 逐元素
 */
NEFORCE_ALWAYS_INLINE_INLINE vec128_t avg_u16(vec128_t a, vec128_t b) noexcept {
#    if defined(NEFORCE_SIMD_NEON)
    return ::vreinterpretq_u8_u16(::vrhaddq_u16(::vreinterpretq_u16_u8(a), ::vreinterpretq_u16_u8(b)));
#    else
    const auto* ua = reinterpret_cast<const uint16_t*>(a.data);
    const auto* ub = reinterpret_cast<const uint16_t*>(b.data);
    vec128_t result;
    auto* rd = reinterpret_cast<uint16_t*>(result.data);
    for (int i = 0; i < 8; ++i) {
        rd[i] = static_cast<uint16_t>((static_cast<int>(ua[i]) + static_cast<int>(ub[i]) + 1) >> 1);
    }
    return result;
#    endif
}

/** @} */ // SIMD

NEFORCE_END_SIMD__
NEFORCE_END_NAMESPACE__
#endif
#endif // NEFORCE_CORE_SIMD_ARITHMETIC_ARM_HPP__
