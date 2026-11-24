#ifndef NEFORCE_CORE_SIMD_COMPARE_HPP__
#define NEFORCE_CORE_SIMD_COMPARE_HPP__

/**
 * @file compare.hpp
 * @brief 跨平台 SIMD 比较运算
 *
 * 提供整型及浮点逐元素比较操作。
 */

#include "NeForce/core/simd/arch/x86/compare.hpp"
#include "NeForce/core/simd/arch/arm/compare.hpp"
#include "NeForce/core/simd/arch/riscv/compare.hpp"
#include "NeForce/core/simd/arch/loongarch/compare.hpp"
#ifdef NEFORCE_SIMD_BRANCH_NOT_SUPPORTED
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_SIMD__

using namespace scalar;

NEFORCE_END_SIMD__
NEFORCE_END_NAMESPACE__
#endif
#endif // NEFORCE_CORE_SIMD_COMPARE_HPP__
