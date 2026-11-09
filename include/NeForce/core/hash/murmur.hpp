#ifndef NEFORCE_CORE_HASH_MURMUR_HPP__
#define NEFORCE_CORE_HASH_MURMUR_HPP__

/**
 * @file murmur.hpp
 * @brief MurmurHash3 非加密哈希算法
 */

#include "NeForce/core/typeinfo/types.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup HashFast
 * @{
 */

#ifdef NEFORCE_ARCH_BITS_64

/**
 * @struct murmur_hash
 * @brief MurmurHash_x64的128位哈希结果容器
 *
 * MurmurHash_x64算法产生128位哈希值，拆分为两个64位整数存储。
 */
struct murmur_hash {
    size_t low = 0;  ///< 哈希值的低64位
    size_t high = 0; ///< 哈希值的高64位

    murmur_hash() noexcept = default;
    ~murmur_hash() noexcept = default;

    /**
     * @brief 构造函数
     * @param l 哈希值的低64位
     * @param h 哈希值的高64位
     */
    murmur_hash(const size_t l, const size_t h) noexcept :
    low(l),
    high(h) {}
};

/**
 * @brief MurmurHash3_x64_128算法
 * @param key 要哈希的数据
 * @param len 数据长度
 * @param seed 哈希种子
 * @return 128位哈希结果
 *
 * MurmurHash是一种非加密哈希算法，具有：
 * 1. 速度快
 * 2. 碰撞率低
 * 3. 可自定义种子
 *
 * MurmurHash3 的 64 位版本，产生 128 位哈希值。
 */
NEFORCE_API murmur_hash murmur_hash64(const void* key, size_t len, uint32_t seed) noexcept;

#endif

/**
 * @brief MurmurHash3_x86_32算法
 * @param key 要哈希的数据
 * @param len 数据长度
 * @param seed 哈希种子
 * @return 32位哈希结果
 *
 * MurmurHash3 的 32 位版本，产生 32 位哈希值。
 */
NEFORCE_API uint32_t murmur_hash32(const void* key, size_t len, uint32_t seed) noexcept;

/** @} */ // HashFast
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_HASH_MURMUR_HPP__
