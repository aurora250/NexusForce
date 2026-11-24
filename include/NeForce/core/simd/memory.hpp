#ifndef NEFORCE_CORE_SIMD_MEMORY_HPP__
#define NEFORCE_CORE_SIMD_MEMORY_HPP__

/**
 * @file memory.hpp
 * @brief 跨平台 SIMD 内存操作
 *
 * 提供对齐/非对齐加载与存储、流式访问及软件预取操作。
 */

#include "NeForce/core/simd/arch/x86/memory.hpp"
#include "NeForce/core/simd/arch/arm/memory.hpp"
#include "NeForce/core/simd/arch/riscv/memory.hpp"
#include "NeForce/core/simd/arch/loongarch/memory.hpp"
#ifdef NEFORCE_SIMD_BRANCH_NOT_SUPPORTED
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_SIMD__

using namespace scalar;

NEFORCE_END_SIMD__
NEFORCE_END_NAMESPACE__
#endif
#endif // NEFORCE_CORE_SIMD_MEMORY_HPP__
