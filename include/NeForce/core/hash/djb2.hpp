#ifndef NEFORCE_CORE_HASH_DJB2_HPP__
#define NEFORCE_CORE_HASH_DJB2_HPP__

/**
 * @file djb2.hpp
 * @brief DJB2 非加密哈希算法
 */

#include "NeForce/core/typeinfo/types.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup HashFast
 * @{
 */

/**
 * @brief DJB2哈希算法
 * @param str 字符串指针
 * @param len 字符串长度
 * @return 计算出的哈希值
 *
 * DJB2是一种非加密哈希算法，具有以下特点：
 * 1. 实现简单
 * 2. 速度快
 * 3. 分布均匀
 *
 * 但在某些特殊情况下仍可能出现哈希冲突。
 */
NEFORCE_CONSTEXPR14 size_t DJB2_hash(const char* str, const size_t len) noexcept {
    size_t hash = 5381;
    for (size_t i = 0; i < len; ++i) {
        hash = (hash << 5) + hash + static_cast<byte_t>(str[i]);
    }
    return hash;
}

/** @} */ // HashFast

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_HASH_DJB2_HPP__
