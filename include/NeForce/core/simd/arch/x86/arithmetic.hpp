#ifndef NEFORCE_CORE_SIMD_ARITHMETIC_X86_HPP__
#define NEFORCE_CORE_SIMD_ARITHMETIC_X86_HPP__
#include "NeForce/core/numeric/numeric_traits.hpp"
#include "NeForce/core/simd/types.hpp"
#ifdef NEFORCE_SIMD_BRANCH_X86
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
#    if defined(NEFORCE_SIMD_SSE2)
    return ::_mm_add_epi8(a, b);
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
#    if defined(NEFORCE_SIMD_SSE2)
    return ::_mm_add_epi16(a, b);
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
#    if defined(NEFORCE_SIMD_SSE2)
    return ::_mm_add_epi32(a, b);
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
#    if defined(NEFORCE_SIMD_SSE2)
    return ::_mm_add_epi64(a, b);
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
#    if defined(NEFORCE_SIMD_SSE2)
    return ::_mm_sub_epi8(a, b);
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
#    if defined(NEFORCE_SIMD_SSE2)
    return ::_mm_sub_epi16(a, b);
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
#    if defined(NEFORCE_SIMD_SSE2)
    return ::_mm_sub_epi32(a, b);
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
#    if defined(NEFORCE_SIMD_SSE2)
    return ::_mm_sub_epi64(a, b);
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
#    if defined(NEFORCE_SIMD_SSE2)
    return ::_mm_mullo_epi16(a, b);
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
#    if defined(NEFORCE_SIMD_AVX2)
    return ::_mm_mullo_epi32(a, b);
#    elif defined(NEFORCE_SIMD_SSE2)
    const ::__m128i prod_even = ::_mm_mul_epu32(a, b);
    const ::__m128i prod_odd = ::_mm_mul_epu32(::_mm_srli_epi64(a, 32), ::_mm_srli_epi64(b, 32));
    const ::__m128i shuf_even = _mm_shuffle_epi32(prod_even, _MM_SHUFFLE(2, 0, 2, 0));
    const ::__m128i shuf_odd = _mm_shuffle_epi32(prod_odd, _MM_SHUFFLE(2, 0, 2, 0));
    return ::_mm_unpacklo_epi32(shuf_even, shuf_odd);
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
#    if defined(NEFORCE_SIMD_SSE2)
    return ::_mm_mulhi_epi16(a, b);
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
#    if defined(NEFORCE_SIMD_AVX2)
    const ::__m128i zero = ::_mm_setzero_si128();
    const ::__m128i sign_a = ::_mm_cmpgt_epi8(zero, a);
    const ::__m128i sign_b = ::_mm_cmpgt_epi8(zero, b);
    const ::__m128i a_lo = ::_mm_unpacklo_epi8(a, sign_a);
    const ::__m128i a_hi = ::_mm_unpackhi_epi8(a, sign_a);
    const ::__m128i b_lo = ::_mm_unpacklo_epi8(b, sign_b);
    const ::__m128i b_hi = ::_mm_unpackhi_epi8(b, sign_b);
    const ::__m128i p_lo = ::_mm_mullo_epi16(a_lo, b_lo);
    const ::__m128i p_hi = ::_mm_mullo_epi16(a_hi, b_hi);
    return ::_mm_hadd_epi16(p_lo, p_hi);
#    elif defined(NEFORCE_SIMD_SSE2)
    const auto* sa = reinterpret_cast<const int8_t*>(&a);
    const auto* sb = reinterpret_cast<const int8_t*>(&b);
    alignas(16) int16_t result[8];
    for (int i = 0; i < 8; ++i) {
        result[i] = static_cast<int16_t>(static_cast<int16_t>(sa[static_cast<ptrdiff_t>(2 * i)]) *
                                                 static_cast<int16_t>(sb[static_cast<ptrdiff_t>(2 * i)]) +
                                         static_cast<int16_t>(sa[static_cast<ptrdiff_t>(2 * i + 1)]) *
                                                 static_cast<int16_t>(sb[static_cast<ptrdiff_t>(2 * i + 1)]));
    }
    return ::_mm_load_si128(reinterpret_cast<const ::__m128i*>(result));
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
#    if defined(NEFORCE_SIMD_SSE2)
    return ::_mm_adds_epi8(a, b);
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
#    if defined(NEFORCE_SIMD_SSE2)
    return ::_mm_adds_epi16(a, b);
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
#    if defined(NEFORCE_SIMD_SSE2)
    return ::_mm_adds_epu8(a, b);
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
#    if defined(NEFORCE_SIMD_AVX2)
    return ::_mm_adds_epu16(a, b);
#    elif defined(NEFORCE_SIMD_SSE2)
    const ::__m128i sum = ::_mm_add_epi16(a, b);
    const ::__m128i sign = ::_mm_set1_epi16(static_cast<short>(0x8000));
    const ::__m128i overflow = ::_mm_cmpgt_epi16(::_mm_xor_si128(a, sign), ::_mm_xor_si128(sum, sign));
    return ::_mm_or_si128(overflow, sum);
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
#    if defined(NEFORCE_SIMD_SSE2)
    return ::_mm_subs_epi8(a, b);
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
#    if defined(NEFORCE_SIMD_SSE2)
    return ::_mm_subs_epi16(a, b);
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
#    if defined(NEFORCE_SIMD_SSE2)
    return ::_mm_subs_epu8(a, b);
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
#    if defined(NEFORCE_SIMD_AVX2)
    return ::_mm_subs_epu16(a, b);
#    elif defined(NEFORCE_SIMD_SSE2)
    const ::__m128i sub = ::_mm_sub_epi16(a, b);
    const ::__m128i sign = ::_mm_set1_epi16(static_cast<short>(0x8000));
    const ::__m128i underflow = ::_mm_cmpgt_epi16(::_mm_xor_si128(b, sign), ::_mm_xor_si128(a, sign));
    return ::_mm_andnot_si128(underflow, sub);
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
#    if defined(NEFORCE_SIMD_AVX2)
    return ::_mm_abs_epi8(v);
#    elif defined(NEFORCE_SIMD_SSE2)
    const ::__m128i mask = ::_mm_cmpgt_epi8(::_mm_setzero_si128(), v);
    return ::_mm_or_si128(::_mm_and_si128(mask, ::_mm_sub_epi8(::_mm_setzero_si128(), v)), ::_mm_andnot_si128(mask, v));
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
#    if defined(NEFORCE_SIMD_AVX2)
    return ::_mm_abs_epi16(v);
#    elif defined(NEFORCE_SIMD_SSE2)
    const ::__m128i mask = ::_mm_cmpgt_epi16(::_mm_setzero_si128(), v);
    const ::__m128i neg = ::_mm_sub_epi16(::_mm_setzero_si128(), v);
    return ::_mm_or_si128(::_mm_and_si128(mask, neg), ::_mm_andnot_si128(mask, v));
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
#    if defined(NEFORCE_SIMD_AVX2)
    return ::_mm_abs_epi32(v);
#    elif defined(NEFORCE_SIMD_SSE2)
    const ::__m128i mask = ::_mm_srai_epi32(v, 31);
    const ::__m128i flipped = ::_mm_xor_si128(v, mask);
    return ::_mm_sub_epi32(flipped, mask);
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
#    if defined(NEFORCE_SIMD_AVX2)
    return ::_mm_min_epi8(a, b);
#    elif defined(NEFORCE_SIMD_SSE2)
    const ::__m128i sign = ::_mm_set1_epi8(static_cast<char>(0x80));
    const ::__m128i flipped_a = ::_mm_xor_si128(a, sign);
    const ::__m128i flipped_b = ::_mm_xor_si128(b, sign);
    return ::_mm_xor_si128(::_mm_min_epu8(flipped_a, flipped_b), sign);
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
#    if defined(NEFORCE_SIMD_SSE2)
    return ::_mm_min_epi16(a, b);
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
#    if defined(NEFORCE_SIMD_AVX2)
    return ::_mm_min_epi32(a, b);
#    elif defined(NEFORCE_SIMD_SSE2)
    const ::__m128i mask = ::_mm_cmpgt_epi32(b, a);
    return ::_mm_or_si128(::_mm_and_si128(mask, a), ::_mm_andnot_si128(mask, b));
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
#    if defined(NEFORCE_SIMD_SSE2)
    return ::_mm_min_epu8(a, b);
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
#    if defined(NEFORCE_SIMD_AVX2)
    return ::_mm_min_epu16(a, b);
#    elif defined(NEFORCE_SIMD_SSE2)
    const ::__m128i offset = ::_mm_set1_epi16(static_cast<short>(0x8000));
    const ::__m128i mask = ::_mm_cmpgt_epi16(::_mm_sub_epi16(b, offset), ::_mm_sub_epi16(a, offset));
    return ::_mm_or_si128(::_mm_and_si128(mask, a), ::_mm_andnot_si128(mask, b));
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
#    if defined(NEFORCE_SIMD_AVX2)
    return ::_mm_min_epu32(a, b);
#    elif defined(NEFORCE_SIMD_SSE2)
    const ::__m128i sign = ::_mm_set1_epi32(static_cast<int>(0x80000000));
    const ::__m128i mask = ::_mm_cmpgt_epi32(::_mm_xor_si128(b, sign), ::_mm_xor_si128(a, sign));
    return ::_mm_or_si128(::_mm_and_si128(mask, a), ::_mm_andnot_si128(mask, b));
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
#    if defined(NEFORCE_SIMD_AVX2)
    return ::_mm_max_epi8(a, b);
#    elif defined(NEFORCE_SIMD_SSE2)
    const ::__m128i sign = ::_mm_set1_epi8(static_cast<char>(0x80));
    return ::_mm_xor_si128(::_mm_max_epu8(::_mm_xor_si128(a, sign), ::_mm_xor_si128(b, sign)), sign);
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
#    if defined(NEFORCE_SIMD_SSE2)
    return ::_mm_max_epi16(a, b);
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
#    if defined(NEFORCE_SIMD_AVX2)
    return ::_mm_max_epi32(a, b);
#    elif defined(NEFORCE_SIMD_SSE2)
    const ::__m128i mask = ::_mm_cmpgt_epi32(a, b);
    return ::_mm_or_si128(::_mm_and_si128(mask, a), ::_mm_andnot_si128(mask, b));
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
#    if defined(NEFORCE_SIMD_SSE2)
    return ::_mm_max_epu8(a, b);
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
#    if defined(NEFORCE_SIMD_AVX2)
    return ::_mm_max_epu16(a, b);
#    elif defined(NEFORCE_SIMD_SSE2)
    const ::__m128i mask = ::_mm_cmpgt_epi16(a, b);
    const ::__m128i overflow = ::_mm_cmpgt_epi16(::_mm_xor_si128(a, ::_mm_set1_epi16(static_cast<short>(0x8000))),
                                                 ::_mm_xor_si128(b, ::_mm_set1_epi16(static_cast<short>(0x8000))));
    return ::_mm_or_si128(::_mm_and_si128(overflow, a), ::_mm_andnot_si128(overflow, b));
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
#    if defined(NEFORCE_SIMD_AVX2)
    return ::_mm_max_epu32(a, b);
#    elif defined(NEFORCE_SIMD_SSE2)
    const ::__m128i sign = ::_mm_set1_epi32(static_cast<int>(0x80000000));
    const ::__m128i mask = ::_mm_cmpgt_epi32(::_mm_xor_si128(a, sign), ::_mm_xor_si128(b, sign));
    return ::_mm_or_si128(::_mm_and_si128(mask, a), ::_mm_andnot_si128(mask, b));
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
#    if defined(NEFORCE_SIMD_SSE2)
    return ::_mm_avg_epu8(a, b);
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
#    if defined(NEFORCE_SIMD_SSE2)
    return ::_mm_avg_epu16(a, b);
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
#endif // NEFORCE_CORE_SIMD_ARITHMETIC_X86_HPP__
