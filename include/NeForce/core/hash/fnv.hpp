#ifndef NEFORCE_CORE_HASH_FNV_HPP__
#define NEFORCE_CORE_HASH_FNV_HPP__

/**
 * @file fnv.hpp
 * @brief FNV-1a 非加密哈希算法
 */

#include "NeForce/core/numeric/constants.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup HashFast
 * @{
 */

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
NEFORCE_CONSTEXPR14 size_t fnv_hash64(const byte_t* first, const size_t count) noexcept {
    size_t result = constants::fnv_offset_basis64;
    for (size_t i = 0; i < count; i++) {
        result ^= static_cast<size_t>(first[i]);
        result *= constants::fnv_prime64;
    }
    return result;
}

/**
 * @brief FNV-1a 32位 哈希算法
 * @param first 数据的起始字节指针
 * @param count 数据的字节数
 * @return 计算出的哈希值
 */
NEFORCE_CONSTEXPR14 size_t fnv_hash32(const byte_t* first, const size_t count) noexcept {
    size_t result = constants::fnv_offset_basis32;
    for (size_t i = 0; i < count; i++) {
        result ^= static_cast<size_t>(first[i]);
        result *= constants::fnv_prime32;
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
NEFORCE_CONSTEXPR14 size_t fnv_hash_integer(const T value) noexcept {
    static_assert(is_integral<T>::value, "T must be integral");

    size_t result = constants::fnv_offset_basis64;
    for (size_t i = 0; i < sizeof(T); ++i) {
        const auto byte_val = static_cast<byte_t>((value >> (i * 8)) & 0xFF);
        result ^= static_cast<size_t>(byte_val);
        result *= constants::fnv_prime64;
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
NEFORCE_CONSTEXPR14 size_t fnv_hash_string64(const CharT* str, const size_t len) noexcept {
    static_assert(is_character<CharT>::value, "CharT must be character types");

    size_t result = constants::fnv_offset_basis64;
    for (size_t i = 0; i < len; ++i) {
        result ^= static_cast<size_t>(static_cast<byte_t>(str[i]));
        result *= constants::fnv_prime64;
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
NEFORCE_CONSTEXPR14 size_t fnv_hash_string32(const CharT* str, const size_t len) noexcept {
    static_assert(is_character<CharT>::value, "CharT must be character types");

    size_t result = constants::fnv_offset_basis32;
    for (size_t i = 0; i < len; ++i) {
        result ^= static_cast<size_t>(static_cast<byte_t>(str[i]));
        result *= constants::fnv_prime32;
    }
    return result;
}

/** @} */ // HashFast

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_HASH_FNV_HPP__
