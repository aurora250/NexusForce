#ifndef NEFORCE_CORE_SIMD_ARITHMETIC_HPP__
#define NEFORCE_CORE_SIMD_ARITHMETIC_HPP__

/**
 * @file arithmetic.hpp
 * @brief 跨平台 SIMD 整型算术运算
 *
 * 提供加法、减法、乘法、饱和运算、绝对值、最大/最小值及平均等整型算术操作。
 */

#include "NeForce/core/simd/arch/x86/arithmetic.hpp"
#include "NeForce/core/simd/arch/arm/arithmetic.hpp"
#include "NeForce/core/simd/arch/riscv/arithmetic.hpp"
#include "NeForce/core/simd/arch/loongarch/arithmetic.hpp"
#ifdef NEFORCE_SIMD_BRANCH_NOT_SUPPORTED
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_SIMD__

using namespace scalar;

NEFORCE_END_SIMD__
NEFORCE_END_NAMESPACE__
#endif
#endif // NEFORCE_CORE_SIMD_ARITHMETIC_HPP__
