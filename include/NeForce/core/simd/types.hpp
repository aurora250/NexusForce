#ifndef NEFORCE_CORE_SIMD_TYPES_HPP__
#define NEFORCE_CORE_SIMD_TYPES_HPP__

/**
 * @file types.hpp
 * @brief SIMD 向量类型定义
 *
 * 此文件定义跨平台 SIMD 向量类型。
 * 未启用任何 SIMD 指令集时，所有类型退化为标量数组结构体。
 */

#include "NeForce/core/typeinfo/types.hpp"
#if defined(NEFORCE_SIMD_AVX2) || defined(NEFORCE_SIMD_AVX512F)
#    include <immintrin.h>
#elif defined(NEFORCE_SIMD_SSE2)
#    include <emmintrin.h>
#elif defined(NEFORCE_SIMD_NEON)
#    include <arm_neon.h>
#endif
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_SIMD__

/**
 * @defgroup SIMD SIMD
 * @brief SIMD 操作封装
 * @{
 */

#ifdef NEFORCE_SIMD_SSE2
/// @brief 128-bit 整型向量（16×i8 / 8×i16 / 4×i32 / 2×i64）
using vec128_t = ::__m128i;
/// @brief 128-bit 单精度浮点向量（4×f32）
using vec128f_t = ::__m128;
/// @brief 128-bit 双精度浮点向量（2×f64）
using vec128d_t = ::__m128d;
#elif defined(NEFORCE_SIMD_NEON)
/// @brief 128-bit 整型向量（16×i8 / 8×i16 / 4×i32 / 2×i64）
using vec128_t = ::uint8x16_t;
/// @brief 128-bit 单精度浮点向量（4×f32）
using vec128f_t = ::float32x4_t;
#    ifdef __aarch64__
/// @brief 128-bit 双精度浮点向量（2×f64，仅 AArch64）
using vec128d_t = ::float64x2_t;
#    else
/// @brief 128-bit 双精度浮点向量（2×f64，ARMv7 标量回退）
struct vec128d_t {
    double data[2];
};
#    endif
#else
/// @brief 128-bit 整型向量（标量回退）
struct vec128_t {
    byte_t data[16];
};
/// @brief 128-bit 单精度浮点向量（标量回退）
struct vec128f_t {
    float data[4];
};
/// @brief 128-bit 双精度浮点向量（标量回退）
struct vec128d_t {
    double data[2];
};
#endif

#if defined(NEFORCE_SIMD_AVX2) || defined(NEFORCE_SIMD_AVX)
/// @brief 256-bit 整型向量（32×i8 / 16×i16 / 8×i32 / 4×i64，需 AVX2）
#    ifdef NEFORCE_SIMD_AVX2
using vec256_t = ::__m256i;
#    else
/// @brief 256-bit 整型向量（标量回退，AVX 不含整型操作）
struct vec256_t {
    byte_t data[32];
};
#    endif
/// @brief 256-bit 单精度浮点向量（8×f32）
using vec256f_t = ::__m256;
/// @brief 256-bit 双精度浮点向量（4×f64）
using vec256d_t = ::__m256d;
#else
/// @brief 256-bit 整型向量（标量回退）
struct vec256_t {
    byte_t data[32];
};
/// @brief 256-bit 单精度浮点向量（标量回退）
struct vec256f_t {
    float data[8];
};
/// @brief 256-bit 双精度浮点向量（标量回退）
struct vec256d_t {
    double data[4];
};
#endif

#ifdef NEFORCE_SIMD_AVX512F
/// @brief 512-bit 整型向量（64×i8 / 32×i16 / 16×i32 / 8×i64）
using vec512_t = ::__m512i;
/// @brief 512-bit 单精度浮点向量（16×f32）
using vec512f_t = ::__m512;
/// @brief 512-bit 双精度浮点向量（8×f64）
using vec512d_t = ::__m512d;
#else
/// @brief 512-bit 整型向量（标量回退）
struct vec512_t {
    byte_t data[64];
};
/// @brief 512-bit 单精度浮点向量（标量回退）
struct vec512f_t {
    float data[16];
};
/// @brief 512-bit 双精度浮点向量（标量回退）
struct vec512d_t {
    double data[8];
};
#endif

/** @} */ // SIMD

NEFORCE_END_SIMD__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_SIMD_TYPES_HPP__
