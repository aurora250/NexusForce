#ifndef NEFORCE_CORE_HASH_FNV_HPP__
#define NEFORCE_CORE_HASH_FNV_HPP__

/**
 * @file fnv.hpp
 * @brief FNV-1a 非加密哈希算法
 */

#include "NeForce/core/typeinfo/type_traits.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup HashFast
 * @{
 */

NEFORCE_BEGIN_CONSTANTS__

/**
 * @var FNV_OFFSET_BASIS64
 * @brief FNV64哈希算法的偏移基础值
 */
NEFORCE_INLINE17 constexpr size_t FNV_OFFSET_BASIS64 = 14695981039346656037ULL;

/**
 * @var FNV_PRIME64
 * @brief FNV64哈希算法的质数乘数
 */
NEFORCE_INLINE17 constexpr size_t FNV_PRIME64 = 1099511628211ULL;

/**
 * @var FNV_OFFSET_BASIS32
 * @brief FNV32哈希算法的偏移基础值
 */
NEFORCE_INLINE17 constexpr size_t FNV_OFFSET_BASIS32 = 2166136261U;

/**
 * @var FNV_PRIME32
 * @brief FNV32哈希算法的质数乘数
 */
NEFORCE_INLINE17 constexpr size_t FNV_PRIME32 = 16777619U;

NEFORCE_END_CONSTANTS__

/**
 * @brief FNV-1a 64位 哈希算法
 * @param first 数据的起始字节指针
 * @param count 数据的字节数
 * @return 计算出的哈希值
 *
 * FNV（Fowler-Noll-Vo）是一种非加密哈希算法，具有：
 * 1. 良好的雪崩效应（avalanche effect）
 * 2. 较低的碰撞率
 * 3. 实现简单高效
 */
NEFORCE_CONSTEXPR14 size_t FNV_hash64(const byte_t* first, const size_t count) noexcept {
    size_t result = constants::FNV_OFFSET_BASIS64;
    for (size_t i = 0; i < count; i++) {
        result ^= static_cast<size_t>(first[i]);
        result *= constants::FNV_PRIME64;
    }
    return result;
}

/**
 * @brief FNV-1a 32位 哈希算法
 * @param first 数据的起始字节指针
 * @param count 数据的字节数
 * @return 计算出的哈希值
 */
NEFORCE_CONSTEXPR14 size_t FNV_hash32(const byte_t* first, const size_t count) noexcept {
    size_t result = constants::FNV_OFFSET_BASIS32;
    for (size_t i = 0; i < count; i++) {
        result ^= static_cast<size_t>(first[i]);
        result *= constants::FNV_PRIME32;
    }
    return result;
}

/**
 * @brief 整数类型的FNV哈希
 * @tparam T 整数类型
 * @param value 要哈希的整数值
 * @return 整数的哈希值
 */
template <typename T>
NEFORCE_CONSTEXPR14 size_t FNV_hash_integer(const T value) noexcept {
    static_assert(is_integral<T>::value, "T must be integral");

    size_t result = constants::FNV_OFFSET_BASIS64;
    for (size_t i = 0; i < sizeof(T); ++i) {
        const auto byte_val = static_cast<byte_t>((value >> (i * 8)) & 0xFF);
        result ^= static_cast<size_t>(byte_val);
        result *= constants::FNV_PRIME64;
    }
    return result;
}

/**
 * @brief 字符串类型的 FNV 64位 哈希
 * @tparam CharT 字符类型
 * @param str 字符串指针
 * @param len 字符串长度
 * @return 字符串的哈希值
 */
template <typename CharT>
NEFORCE_CONSTEXPR14 size_t FNV_hash_string64(const CharT* str, const size_t len) noexcept {
    static_assert(is_character<CharT>::value, "CharT must be character types");

    size_t result = constants::FNV_OFFSET_BASIS64;
    for (size_t i = 0; i < len; ++i) {
        result ^= static_cast<size_t>(static_cast<byte_t>(str[i]));
        result *= constants::FNV_PRIME64;
    }
    return result;
}

/**
 * @brief 字符串类型的 FNV 32位 哈希
 * @tparam CharT 字符类型
 * @param str 字符串指针
 * @param len 字符串长度
 * @return 字符串的哈希值
 */
template <typename CharT>
NEFORCE_CONSTEXPR14 size_t FNV_hash_string32(const CharT* str, const size_t len) noexcept {
    static_assert(is_character<CharT>::value, "CharT must be character types");

    size_t result = constants::FNV_OFFSET_BASIS32;
    for (size_t i = 0; i < len; ++i) {
        result ^= static_cast<size_t>(static_cast<byte_t>(str[i]));
        result *= constants::FNV_PRIME32;
    }
    return result;
}

/** @} */ // HashFast

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_HASH_FNV_HPP__
