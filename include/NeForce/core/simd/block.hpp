#ifndef NEFORCE_CORE_SIMD_BLOCK_HPP__
#define NEFORCE_CORE_SIMD_BLOCK_HPP__

/**
 * @file block.hpp
 * @brief 跨平台 SIMD 批量内核
 *
 * 逐元素内核一次只处理一个定长向量。
 */

#include "NeForce/core/simd/arch/x86/block.hpp"
#include "NeForce/core/simd/arch/arm/block.hpp"
#include "NeForce/core/simd/arch/riscv/block.hpp"
#include "NeForce/core/simd/arch/loongarch/block.hpp"
#ifdef NEFORCE_SIMD_BRANCH_NOT_SUPPORTED
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_SIMD__

using namespace scalar;

NEFORCE_END_SIMD__
NEFORCE_END_NAMESPACE__
#endif
#endif // NEFORCE_CORE_SIMD_BLOCK_HPP__
