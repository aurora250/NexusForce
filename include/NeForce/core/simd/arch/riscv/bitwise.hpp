#ifndef NEFORCE_CORE_SIMD_BITWISE_RISCV_HPP__
#define NEFORCE_CORE_SIMD_BITWISE_RISCV_HPP__

/**
 * @file bitwise.hpp
 * @brief RISC-V 位运算后端
 *
 * RVV 的可伸缩向量类型（vec_rvv*_t）无法承载定长的 vec128_t/vec256_t/vec512_t，
 * 定长向量的位运算只能经由内存中转，无法映射为更优的单条向量指令，
 * 因此本后端整体复用 scalar 后端，而不是重复实现同一套逐字节算法。
 */

#include "NeForce/core/simd/scalar/bitwise.hpp"
#ifdef NEFORCE_SIMD_BRANCH_RISCV
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_SIMD__

using scalar::bit_and;
using scalar::bit_andnot;
using scalar::bit_not;
using scalar::bit_or;
using scalar::bit_xor;
using scalar::popcount;
using scalar::shift_left_bytes;
using scalar::shift_right_bytes;

NEFORCE_END_SIMD__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_SIMD_BRANCH_RISCV
#endif // NEFORCE_CORE_SIMD_BITWISE_RISCV_HPP__
