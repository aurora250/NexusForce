#ifndef NEFORCE_CORE_HASH_WYHASH_HPP__
#define NEFORCE_CORE_HASH_WYHASH_HPP__

/**
 * @file wyhash.hpp
 * @brief wyhash 非加密哈希算法
 */

#include "NeForce/core/typeinfo/types.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup HashFast
 * @{
 */

#ifdef NEFORCE_ARCH_BITS_64

/**
 * @brief wyhash 哈希算法
 * @param key 输入数据指针
 * @param len 数据长度
 * @param seed 种子值
 * @return 64 位哈希值
 *
 * wyhash（Wang Yi's hash）是一种现代非加密哈希算法，
 * 在 SMHasher 测试套件中表现优异，适合哈希表等场景。
 */
NEFORCE_API uint64_t wyhash(const void* key, size_t len, uint64_t seed) noexcept;

#endif

/** @} */ // HashFast
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_HASH_WYHASH_HPP__
