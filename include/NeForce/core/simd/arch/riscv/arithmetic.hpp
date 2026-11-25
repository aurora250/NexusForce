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

using scalar::abs_i16;
using scalar::abs_i32;
using scalar::abs_i8;
using scalar::add_i16;
using scalar::add_i32;
using scalar::add_i64;
using scalar::add_i8;
using scalar::avg_u16;
using scalar::avg_u8;
using scalar::madds_i8x16;
using scalar::max_i16;
using scalar::max_i32;
using scalar::max_i8;
using scalar::max_u16;
using scalar::max_u32;
using scalar::max_u8;
using scalar::min_i16;
using scalar::min_i32;
using scalar::min_i8;
using scalar::min_u16;
using scalar::min_u32;
using scalar::min_u8;
using scalar::mulhi_i16;
using scalar::mullo_i16;
using scalar::mullo_i32;
using scalar::saturated_add_i16;
using scalar::saturated_add_i8;
using scalar::saturated_add_u16;
using scalar::saturated_add_u8;
using scalar::saturated_sub_i16;
using scalar::saturated_sub_i8;
using scalar::saturated_sub_u16;
using scalar::saturated_sub_u8;
using scalar::sub_i16;
using scalar::sub_i32;
using scalar::sub_i64;
using scalar::sub_i8;

NEFORCE_END_SIMD__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_SIMD_BRANCH_RISCV
#endif // NEFORCE_CORE_SIMD_ARITHMETIC_RISCV_HPP__
