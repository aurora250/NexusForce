#ifndef NEFORCE_CORE_SIMD_BYTES_HPP__
#define NEFORCE_CORE_SIMD_BYTES_HPP__

/**
 * @file bytes.hpp
 * @brief 跨平台 SIMD 字节级操作
 *
 * 提供基于逐字节的广播、加载、比较、搜索、计数、重排与部分存储操作。
 */

#include "NeForce/core/simd/arch/x86/bytes.hpp"
#include "NeForce/core/simd/arch/arm/bytes.hpp"
#include "NeForce/core/simd/arch/riscv/bytes.hpp"
#include "NeForce/core/simd/arch/loongarch/bytes.hpp"
#ifdef NEFORCE_SIMD_BRANCH_NOT_SUPPORTED
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_SIMD__

using namespace scalar;

NEFORCE_END_SIMD__
NEFORCE_END_NAMESPACE__
#endif
#endif // NEFORCE_CORE_SIMD_BYTES_HPP__
