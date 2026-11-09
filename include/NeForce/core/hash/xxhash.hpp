#ifndef NEFORCE_CORE_HASH_XXHASH_HPP__
#define NEFORCE_CORE_HASH_XXHASH_HPP__

/**
 * @file xxhash.hpp
 * @brief xxHash 非加密哈希算法
 */

#include "NeForce/core/memory/bit.hpp"
#include "NeForce/core/memory/endian.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup HashFast
 * @{
 */

#ifdef NEFORCE_COMPILER_MSVC
// use switch penetrate
#    pragma warning(push)
#    pragma warning(disable : 26819)
#endif

/**
 * @brief XXH32 哈希算法
 * @param input 输入数据指针
 * @param len 数据长度
 * @param seed 种子值，默认为 0
 * @return 32 位哈希值
 *
 * xxHash 是一种极速非加密哈希算法，XXH32 是其 32 位版本。
 * 适用于哈希表、数据校验等非安全场景。
 */
NEFORCE_CONSTEXPR14 uint32_t XXH32(const void* input, size_t len, uint32_t seed = 0) noexcept {
    constexpr uint32_t PRIME32_1 = 0x9E3779B1U;
    constexpr uint32_t PRIME32_2 = 0x85EBCA77U;
    constexpr uint32_t PRIME32_3 = 0xC2B2AE3DU;
    constexpr uint32_t PRIME32_4 = 0x27D4EB2FU;
    constexpr uint32_t PRIME32_5 = 0x165667B1U;

    const auto* p = static_cast<const byte_t*>(input);
    uint32_t hash = 0;

    if (len >= 16) {
        uint32_t acc1 = seed + PRIME32_1 + PRIME32_2;
        uint32_t acc2 = seed + PRIME32_2;
        uint32_t acc3 = seed + 0;
        uint32_t acc4 = seed - PRIME32_1;

        const byte_t* limit = p + (len & ~static_cast<size_t>(15));
        for (; p < limit; p += 16) {
            acc1 = rotate_l32(acc1 + endian::read_le32(p + 0) * PRIME32_2, 13) * PRIME32_1;
            acc2 = rotate_l32(acc2 + endian::read_le32(p + 4) * PRIME32_2, 13) * PRIME32_1;
            acc3 = rotate_l32(acc3 + endian::read_le32(p + 8) * PRIME32_2, 13) * PRIME32_1;
            acc4 = rotate_l32(acc4 + endian::read_le32(p + 12) * PRIME32_2, 13) * PRIME32_1;
        }

        hash = rotate_l32(acc1, 1) + rotate_l32(acc2, 7) + rotate_l32(acc3, 12) + rotate_l32(acc4, 18);
    } else {
        hash = seed + PRIME32_5;
    }

    hash += static_cast<uint32_t>(len);

    size_t remaining = len - static_cast<size_t>(p - static_cast<const byte_t*>(input));
    for (; remaining >= 4; remaining -= 4, p += 4) {
        hash += endian::read_le32(p) * PRIME32_3;
        hash = rotate_l32(hash, 17) * PRIME32_4;
    }
    switch (remaining) {
        case 3:
            hash += static_cast<uint32_t>(p[2]) * PRIME32_5;
            hash = rotate_l32(hash, 11) * PRIME32_1;
        case 2:
            hash += static_cast<uint32_t>(p[1]) * PRIME32_5;
            hash = rotate_l32(hash, 11) * PRIME32_1;
        case 1:
            hash += static_cast<uint32_t>(p[0]) * PRIME32_5;
            hash = rotate_l32(hash, 11) * PRIME32_1;
        default:
            break;
    }

    hash ^= hash >> 15;
    hash *= PRIME32_2;
    hash ^= hash >> 13;
    hash *= PRIME32_3;
    hash ^= hash >> 16;

    return hash;
}

#ifdef NEFORCE_ARCH_BITS_64

/**
 * @brief XXH64 哈希算法
 * @param input 输入数据指针
 * @param len 数据长度
 * @param seed 种子值，默认为 0
 * @return 64 位哈希值
 *
 * xxHash 的 64 位版本，适合中等到大型数据的哈希。
 * 在 64 位平台上速度极快。
 */
NEFORCE_CONSTEXPR14 uint64_t XXH64(const void* input, size_t len, uint64_t seed = 0) noexcept {
    constexpr uint64_t PRIME64_1 = 0x9E3779B185EBCA87ULL;
    constexpr uint64_t PRIME64_2 = 0xC2B2AE3D27D4EB4FULL;
    constexpr uint64_t PRIME64_3 = 0x165667B19E3779F9ULL;
    constexpr uint64_t PRIME64_4 = 0x85EBCA77C2B2AE63ULL;
    constexpr uint64_t PRIME64_5 = 0x27D4EB2F165667C5ULL;

    const auto* p = static_cast<const byte_t*>(input);
    uint64_t hash = 0;

    if (len >= 32) {
        uint64_t acc1 = seed + PRIME64_1 + PRIME64_2;
        uint64_t acc2 = seed + PRIME64_2;
        uint64_t acc3 = seed + 0;
        uint64_t acc4 = seed - PRIME64_1;

        const byte_t* limit = p + (len & ~static_cast<size_t>(31));
        for (; p < limit; p += 32) {
            acc1 = rotate_l64(acc1 + endian::read_le64(p + 0) * PRIME64_2, 31) * PRIME64_1;
            acc2 = rotate_l64(acc2 + endian::read_le64(p + 8) * PRIME64_2, 31) * PRIME64_1;
            acc3 = rotate_l64(acc3 + endian::read_le64(p + 16) * PRIME64_2, 31) * PRIME64_1;
            acc4 = rotate_l64(acc4 + endian::read_le64(p + 24) * PRIME64_2, 31) * PRIME64_1;
        }

        hash = rotate_l64(acc1, 1) + rotate_l64(acc2, 7) + rotate_l64(acc3, 12) + rotate_l64(acc4, 18);

        hash ^= rotate_l64(acc1 * PRIME64_2, 31) * PRIME64_1;
        hash = hash * PRIME64_1 + PRIME64_4;
        hash ^= rotate_l64(acc2 * PRIME64_2, 31) * PRIME64_1;
        hash = hash * PRIME64_1 + PRIME64_4;
        hash ^= rotate_l64(acc3 * PRIME64_2, 31) * PRIME64_1;
        hash = hash * PRIME64_1 + PRIME64_4;
        hash ^= rotate_l64(acc4 * PRIME64_2, 31) * PRIME64_1;
        hash = hash * PRIME64_1 + PRIME64_4;
    } else {
        hash = seed + PRIME64_5;
    }

    hash += static_cast<uint64_t>(len);

    size_t remaining = len - static_cast<size_t>(p - static_cast<const byte_t*>(input));
    for (; remaining >= 8; remaining -= 8, p += 8) {
        uint64_t k1 = endian::read_le64(p);
        k1 *= PRIME64_2;
        k1 = rotate_l64(k1, 31);
        k1 *= PRIME64_1;
        hash ^= k1;
        hash = rotate_l64(hash, 27) * PRIME64_1 + PRIME64_4;
    }
    for (; remaining >= 4; remaining -= 4, p += 4) {
        hash ^= static_cast<uint64_t>(endian::read_le32(p)) * PRIME64_1;
        hash = rotate_l64(hash, 23) * PRIME64_2 + PRIME64_3;
    }
    switch (remaining) {
        case 3:
            hash ^= static_cast<uint64_t>(p[2]) * PRIME64_5;
            hash = rotate_l64(hash, 11) * PRIME64_1;
        case 2:
            hash ^= static_cast<uint64_t>(p[1]) * PRIME64_5;
            hash = rotate_l64(hash, 11) * PRIME64_1;
        case 1:
            hash ^= static_cast<uint64_t>(p[0]) * PRIME64_5;
            hash = rotate_l64(hash, 11) * PRIME64_1;
        default:
            break;
    }

    hash ^= hash >> 33;
    hash *= PRIME64_2;
    hash ^= hash >> 29;
    hash *= PRIME64_3;
    hash ^= hash >> 32;

    return hash;
}

#endif // NEFORCE_ARCH_BITS_64

#ifdef NEFORCE_COMPILER_MSVC
#    pragma warning(pop)
#endif

/**
 * @brief XXH3_64bits 哈希算法
 * @param data 输入数据指针
 * @param len 数据长度
 * @return 64 位哈希值
 *
 * XXH3 是 xxHash 系列的最新版本，在 64 位平台上速度极快，适合大数据量哈希。
 */
NEFORCE_API uint64_t XXH3_64(const void* data, size_t len) noexcept;

/** @} */ // HashFast

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_HASH_XXHASH_HPP__
