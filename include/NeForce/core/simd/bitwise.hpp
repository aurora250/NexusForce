#ifndef NEFORCE_CORE_SIMD_BITWISE_HPP__
#define NEFORCE_CORE_SIMD_BITWISE_HPP__

/**
 * @file bitwise.hpp
 * @brief 跨平台 SIMD 位运算
 *
 * 提供按位与/或/异或/非、移位及 popcount 操作。
 */

#include "NeForce/core/simd/arch/x86/bitwise.hpp"
#include "NeForce/core/simd/arch/arm/bitwise.hpp"
#include "NeForce/core/simd/arch/riscv/bitwise.hpp"
#include "NeForce/core/simd/arch/loongarch/bitwise.hpp"
#ifdef NEFORCE_SIMD_BRANCH_NOT_SUPPORTED
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_SIMD__

using namespace scalar;

NEFORCE_END_SIMD__
NEFORCE_END_NAMESPACE__
#endif
#endif // NEFORCE_CORE_SIMD_BITWISE_HPP__
