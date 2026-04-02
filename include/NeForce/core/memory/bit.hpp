#ifndef NEFORCE_CORE_MEMORY_BIT_HPP__
#define NEFORCE_CORE_MEMORY_BIT_HPP__

/**
 * @file bit.hpp
 * @brief 位操作函数
 *
 * 此文件提供了各种位操作函数的实现，包括位计数、前导零计数、位旋转、位反转等。
 * 支持32位和64位平台，使用查表法和位运算技巧优化性能。
 */

#include "NeForce/core/typeinfo/types.hpp"
NEFORCE_BEGIN_NAMESPACE__

NEFORCE_BEGIN_CONSTANTS__

/**
 * @defgroup BitManipulation 位操作
 * @brief 位操作类与函数的实现
 * @{
 */

/**
 * @var POPCOUNT_TABLE
 * @brief popcount查找表
 *
 * 预计算的popcount表，用于快速计算字节中1的个数。
 * 包含0-255所有可能字节值的popcount值。
 */
NEFORCE_INLINE17 constexpr byte_t POPCOUNT_TABLE[256] = {
    0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
    1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
    1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
    1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
    3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8
};

/** @} */ // BitManipulation

NEFORCE_END_CONSTANTS__

/**
 * @defgroup BitManipulation 位操作
 * @brief 位操作类与函数的实现
 * @{
 */

#ifdef NEFORCE_ARCH_BITS_64

/**
 * @brief 计算64位整数中1的个数
 * @param x 64位无符号整数
 * @return x中1的个数
 *
 * 使用查表法分别计算8个字节的popcount，然后求和。
 */
constexpr int popcount64(const uint64_t x) noexcept {
    return
        constants::POPCOUNT_TABLE[static_cast<byte_t>(x & 0xFFULL)] +
        constants::POPCOUNT_TABLE[static_cast<byte_t>((x >> 8) & 0xFFULL)] +
        constants::POPCOUNT_TABLE[static_cast<byte_t>((x >> 16) & 0xFFULL)] +
        constants::POPCOUNT_TABLE[static_cast<byte_t>((x >> 24) & 0xFFULL)] +
        constants::POPCOUNT_TABLE[static_cast<byte_t>((x >> 32) & 0xFFULL)] +
        constants::POPCOUNT_TABLE[static_cast<byte_t>((x >> 40) & 0xFFULL)] +
        constants::POPCOUNT_TABLE[static_cast<byte_t>((x >> 48) & 0xFFULL)] +
        constants::POPCOUNT_TABLE[static_cast<byte_t>((x >> 56) & 0xFFULL)];
}

/**
 * @brief 计算64位整数前导零的个数
 * @param x 64位无符号整数
 * @return x中前导零的个数，如果x为0则返回64
 *
 * 使用二分查找法优化前导零计数。
 */
NEFORCE_CONSTEXPR14 int clz64(uint64_t x) noexcept {
    if (x == 0) return 64;
    int n = 0;
    if (x <= 0x00000000FFFFFFFFULL) { n += 32; x <<= 32; }
    if (x <= 0x0000FFFFFFFFFFFFULL) { n += 16; x <<= 16; }
    if (x <= 0x00FFFFFFFFFFFFFFULL) { n += 8; x <<= 8; }
    if (x <= 0x0FFFFFFFFFFFFFFFULL) { n += 4; x <<= 4; }
    if (x <= 0x3FFFFFFFFFFFFFFFULL) { n += 2; x <<= 2; }
    if (x <= 0x7FFFFFFFFFFFFFFFULL) { n += 1; }
    return n;
}

#endif

/**
 * @brief 计算32位整数中1的个数
 * @param x 32位无符号整数
 * @return x中1的个数
 *
 * 使用查表法分别计算4个字节的popcount，然后求和。
 */
constexpr int popcount32(const uint32_t x) noexcept {
    return
        constants::POPCOUNT_TABLE[static_cast<byte_t>(x & 0xFFU)] +
        constants::POPCOUNT_TABLE[static_cast<byte_t>((x >> 8) & 0xFFU)] +
        constants::POPCOUNT_TABLE[static_cast<byte_t>((x >> 16) & 0xFFU)] +
        constants::POPCOUNT_TABLE[static_cast<byte_t>((x >> 24) & 0xFFU)];
}

/**
 * @brief 计算32位整数前导零的个数
 * @param x 32位无符号整数
 * @return x中前导零的个数，如果x为0则返回32
 *
 * 使用二分查找法优化前导零计数。
 */
NEFORCE_CONSTEXPR14 int clz32(uint32_t x) noexcept {
    if (x == 0) return 32;
    int n = 0;
    if (x <= 0x0000FFFFU) { n += 16; x <<= 16; }
    if (x <= 0x00FFFFFFU) { n += 8; x <<= 8; }
    if (x <= 0x0FFFFFFFU) { n += 4; x <<= 4; }
    if (x <= 0x3FFFFFFFU) { n += 2; x <<= 2; }
    if (x <= 0x7FFFFFFFU) { n += 1; }
    return n;
}

/**
 * @brief 计算整数中1的个数
 * @param x 无符号整数
 * @return x中1的个数
 *
 * 根据平台位数调用相应的popcount函数。
 */
constexpr int popcount(const uintptr_t x) noexcept {
#ifdef NEFORCE_ARCH_BITS_64
    return popcount64(x);
#else
    return popcount32(x);
#endif
}

/**
 * @brief 计算整数前导零的个数
 * @param x 无符号整数
 * @return x中前导零的个数
 *
 * 根据平台位数调用相应的clz函数。
 */
constexpr int countl_zero(const uintptr_t x) noexcept {
#ifdef NEFORCE_ARCH_BITS_64
    return clz64(x);
#else
    return clz32(x);
#endif
}

/**
 * @brief 计算整数前导1的个数
 * @param x 无符号整数
 * @return x中前导1的个数
 *
 * 通过对x按位取反后计算前导零个数。
 */
constexpr int countl_one(const uintptr_t x) noexcept {
    return countl_zero(~x);
}

/**
 * @brief 计算整数尾随零的个数
 * @param x 无符号整数
 * @return x中尾随零的个数
 *
 * 使用位运算技巧得到最低有效位的掩码。
 */
constexpr int countr_zero(const uintptr_t x) noexcept {
    return popcount((x & (~x + 1)) - 1);
}

/**
 * @brief 计算整数尾随1的个数
 * @param x 无符号整数
 * @return x中尾随1的个数
 *
 * 通过对x按位取反后计算尾随零个数。
 */
constexpr int countr_one(const uintptr_t x) noexcept {
    return countr_zero(~x);
}


/**
 * @brief 获取最低设置位的位置，从0开始
 * @param x 有符号整数
 * @return 最低设置位的位置，如果没有设置位则返回-1
 */
constexpr int lowest_set_bit_pos(const intptr_t x) noexcept {
    return x == 0 ? -1 : countr_zero(x);
}

/**
 * @brief 获取最高设置位的位置
 * @param x 有符号整数
 * @return 最高设置位的位置，如果没有设置位则返回-1
 */
NEFORCE_CONSTEXPR14 int highest_set_bit_pos(const intptr_t x) noexcept {
    if (x == 0) return -1;
#ifdef NEFORCE_ARCH_BITS_64
    return 63 - clz64(x);
#else
    return 31 - clz32(x);
#endif
}


/**
 * @brief 计算32位整数的奇偶性
 * @param x 32位无符号整数
 * @return 如果x中1的个数为奇数返回true，否则返回false
 *
 * 使用分治法计算奇偶性。
 */
NEFORCE_CONSTEXPR14 bool parity32(uint32_t x) noexcept {
    x ^= x >> 16;
    x ^= x >> 8;
    x ^= x >> 4;
    x ^= x >> 2;
    x ^= x >> 1;
    return (x & 1) != 0;
}

#ifdef NEFORCE_ARCH_BITS_64

/**
 * @brief 计算64位整数的奇偶性
 * @param x 64位无符号整数
 * @return 如果x中1的个数为奇数返回true，否则返回false
 */
NEFORCE_CONSTEXPR14 bool parity64(uint64_t x) noexcept {
    x ^= x >> 32;
    x ^= x >> 16;
    x ^= x >> 8;
    x ^= x >> 4;
    x ^= x >> 2;
    x ^= x >> 1;
    return (x & 1) != 0;
}

#endif

/**
 * @brief 计算整数的奇偶性
 * @param x 无符号整数
 * @return 如果x中1的个数为奇数返回true，否则返回false
 *
 * 根据平台位数调用相应的奇偶性计算函数。
 */
constexpr bool parity(const uintptr_t x) noexcept {
#ifdef NEFORCE_ARCH_BITS_64
    return parity64(x);
#else
    return parity32(x);
#endif
}


/**
 * @brief 计算表示整数所需的最小位宽
 * @param x 无符号整数
 * @return 表示x所需的最小位数
 */
constexpr int bit_width(const uintptr_t x) noexcept {
#ifdef NEFORCE_ARCH_BITS_64
    return x == 0 ? 0 : 64 - countl_zero(x);
#else
    return x == 0 ? 0 : 32 - countl_zero(x);
#endif
}

/**
 * @brief 获取不大于x的最大2的幂
 * @param x 无符号整数
 * @return 不大于x的最大2的幂，如果x为0则返回0
 */
constexpr uintptr_t bit_floor(const uintptr_t x) noexcept {
    return x == 0 ? 0 : uintptr_t{1} << (bit_width(x) - 1);
}

/**
 * @brief 获取不小于x的最小2的幂
 * @param x 无符号整数
 * @return 不小于x的最小2的幂，如果x为0则返回1
 */
NEFORCE_CONSTEXPR14 uint64_t bit_ceil(const uintptr_t x) noexcept {
    if (x <= 1) return 1;
    const uint64_t floor = bit_floor(x);
    return floor == x ? x : floor << 1;
}

/**
 * @brief 检查整数是否为2的幂
 * @param x 无符号整数
 * @return 如果x是2的幂则返回true，否则返回false
 */
constexpr bool has_single_bit(const uintptr_t x) noexcept {
    return x != 0 && (x & (x - 1)) == 0;
}

/**
 * @brief 32位整数循环左移
 * @param x 32位无符号整数
 * @param s 旋转位数
 * @return 循环左移后的结果
 */
NEFORCE_CONSTEXPR14 uint32_t rotate_l32(const uint32_t x, const int s) noexcept {
    constexpr int bits = 32;
    const int shift = ((s % bits) + bits) % bits;
    if (shift == 0) return x;
    return (x << shift) | (x >> (bits - shift));
}

/**
 * @brief 32位整数循环右移
 * @param x 32位无符号整数
 * @param s 旋转位数
 * @return 循环右移后的结果
 */
NEFORCE_CONSTEXPR14 uint32_t rotate_r32(const uint32_t x, const int s) noexcept {
    return rotate_l32(x, -s);
}

#ifdef NEFORCE_ARCH_BITS_64

/**
 * @brief 64位整数循环左移
 * @param x 64位无符号整数
 * @param s 旋转位数
 * @return 循环左移后的结果
 */
NEFORCE_CONSTEXPR14 uint64_t rotate_l64(const uint64_t x, const int s) noexcept {
    constexpr int bits = 64;
    const int shift = ((s % bits) + bits) % bits;
    if (shift == 0) return x;
    return (x << shift) | (x >> (bits - shift));
}

/**
 * @brief 64位整数循环右移
 * @param x 64位无符号整数
 * @param s 旋转位数
 * @return 循环右移后的结果
 */
NEFORCE_CONSTEXPR14 uint64_t rotate_r64(const uint64_t x, const int s) noexcept {
    return rotate_l64(x, -s);
}

#endif

/**
 * @brief 整数循环左移
 * @param x 无符号整数
 * @param s 旋转位数
 * @return 循环左移后的结果
 *
 * 根据平台位数调用相应的循环左移函数。
 */
NEFORCE_CONSTEXPR14 int rotate_l(const uintptr_t x, const int s) noexcept {
#ifdef NEFORCE_ARCH_BITS_64
    return rotate_l64(x, s);
#else
    return rotate_l32(x, s);
#endif
}

/**
 * @brief 整数循环右移
 * @param x 无符号整数
 * @param s 旋转位数
 * @return 循环右移后的结果
 *
 * 根据平台位数调用相应的循环右移函数。
 */
NEFORCE_CONSTEXPR14 int rotate_r(const uintptr_t x, const int s) noexcept {
#ifdef NEFORCE_ARCH_BITS_64
    return rotate_r64(x, s);
#else
    return rotate_r32(x, s);
#endif
}


/**
 * @brief 从整数中提取指定位段
 * @param x 源整数
 * @param pos 起始位置，从0开始
 * @param len 要提取的位数
 * @return 提取的位段
 */
constexpr uintptr_t bit_extract(const uintptr_t x, const int pos, const int len) noexcept {
    return (x >> pos) & ((uintptr_t{1} << len) - 1);
}

/**
 * @brief 向整数中插入指定位段
 * @param x 目标整数
 * @param bits 要插入的位段
 * @param pos 插入位置，从0开始
 * @param len 要插入的位数
 * @return 插入后的整数
 */
NEFORCE_CONSTEXPR14 uintptr_t bit_insert(const uintptr_t x, const uintptr_t bits, const int pos, const int len) noexcept {
    const uintptr_t mask = ((uintptr_t{1} << len) - 1) << pos;
    return (x & ~mask) | ((bits << pos) & mask);
}


/**
 * @brief 反转32位整数的位顺序
 * @param x 32位无符号整数
 * @return 位反转后的整数
 */
NEFORCE_CONSTEXPR14 uint32_t reverse_bits32(uint32_t x) noexcept {
    x = ((x >> 1) & 0x55555555U) | ((x & 0x55555555U) << 1);
    x = ((x >> 2) & 0x33333333U) | ((x & 0x33333333U) << 2);
    x = ((x >> 4) & 0x0F0F0F0FU) | ((x & 0x0F0F0F0FU) << 4);
    x = ((x >> 8) & 0x00FF00FFU) | ((x & 0x00FF00FFU) << 8);
    x = ((x >> 16) & 0x0000FFFFU) | ((x & 0x0000FFFFU) << 16);
    return x;
}

#ifdef NEFORCE_ARCH_BITS_64

/**
 * @brief 反转64位整数的位顺序
 * @param x 64位无符号整数
 * @return 位反转后的整数
 */
NEFORCE_CONSTEXPR14 uint64_t reverse_bits64(uint64_t x) noexcept {
    x = ((x >> 1) & 0x5555555555555555ULL) | ((x & 0x5555555555555555ULL) << 1);
    x = ((x >> 2) & 0x3333333333333333ULL) | ((x & 0x3333333333333333ULL) << 2);
    x = ((x >> 4) & 0x0F0F0F0F0F0F0F0FULL) | ((x & 0x0F0F0F0F0F0F0F0FULL) << 4);
    x = ((x >> 8) & 0x00FF00FF00FF00FFULL) | ((x & 0x00FF00FF00FF00FFULL) << 8);
    x = ((x >> 16) & 0x0000FFFF0000FFFFULL) | ((x & 0x0000FFFF0000FFFFULL) << 16);
    x = (x >> 32) | (x << 32);
    return x;
}

#endif

/**
 * @brief 反转整数的位顺序
 * @param x 无符号整数
 * @return 位反转后的整数
 *
 * 根据平台位数调用相应的位反转函数。
 */
constexpr uintptr_t reverse_bits(const uintptr_t x) noexcept {
#ifdef NEFORCE_ARCH_BITS_64
    return reverse_bits64(x);
#else
    return reverse_bits32(x);
#endif
}


/**
 * @brief 生成从from到to的位掩码
 * @param from 起始位置（包含）
 * @param to 结束位置（包含）
 * @return 从from位到to位为1，其他位为0的掩码
 */
constexpr uintptr_t mask_from_to(const int from, const int to) noexcept {
    return ((uintptr_t{1} << (to - from + 1)) - 1) << from;
}

/** @} */ // BitManipulation

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_MEMORY_BIT_HPP__
