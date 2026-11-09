#ifndef NEFORCE_CORE_HASH_CITY_HASH_HPP__
#define NEFORCE_CORE_HASH_CITY_HASH_HPP__

/**
 * @file city_hash.hpp
 * @brief CityHash64 非加密哈希算法
 */

#include "NeForce/core/typeinfo/types.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup HashFast
 * @{
 */

/**
 * @brief CityHash64 哈希算法
 * @param key 输入数据指针
 * @param len 数据长度
 * @return 64 位哈希值
 *
 * Google CityHash 是一种非加密哈希算法，专为字符串哈希优化。
 */
NEFORCE_API size_t city_hash64(const void* key, size_t len) noexcept;

/** @} */ // HashFast

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_HASH_CITY_HASH_HPP__
