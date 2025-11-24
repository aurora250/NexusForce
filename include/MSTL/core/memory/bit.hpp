#ifndef MSTL_CORE_MEMORY_BIT_HPP__
#define MSTL_CORE_MEMORY_BIT_HPP__
#include "../typeinfo/types.hpp"
MSTL_BEGIN_NAMESPACE__

MSTL_BEGIN_CONSTANTS__
MSTL_INLINE17 constexpr byte_t POPCOUNT_TABLE[256] = {
    0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
    1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
    1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
    1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
    3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8
};
MSTL_END_CONSTANTS__

constexpr int popcountll(const uint64_t x) noexcept {
    return
        _CONSTANTS POPCOUNT_TABLE[static_cast<uint8_t>(x & 0xFFULL)] +
        _CONSTANTS POPCOUNT_TABLE[static_cast<uint8_t>((x >> 8) & 0xFFULL)] +
        _CONSTANTS POPCOUNT_TABLE[static_cast<uint8_t>((x >> 16) & 0xFFULL)] +
        _CONSTANTS POPCOUNT_TABLE[static_cast<uint8_t>((x >> 24) & 0xFFULL)] +
        _CONSTANTS POPCOUNT_TABLE[static_cast<uint8_t>((x >> 32) & 0xFFULL)] +
        _CONSTANTS POPCOUNT_TABLE[static_cast<uint8_t>((x >> 40) & 0xFFULL)] +
        _CONSTANTS POPCOUNT_TABLE[static_cast<uint8_t>((x >> 48) & 0xFFULL)] +
        _CONSTANTS POPCOUNT_TABLE[static_cast<uint8_t>((x >> 56) & 0xFFULL)];
}

constexpr int clzll(uint64_t x) noexcept {
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

constexpr int popcount(const uint64_t x) noexcept {
    return popcountll(x);
}

constexpr int countl_zero(const uint64_t x) noexcept {
    return clzll(x);
}

constexpr int countl_one(const uint64_t x) noexcept {
    return countl_zero(~x);
}

constexpr int countr_zero(const uint64_t x) noexcept {
    return popcountll((x & (~x + 1)) - 1);
}

constexpr int countr_one(const uint64_t x) noexcept {
    return countr_zero(~x);
}

constexpr int bit_width(const uint64_t x) noexcept {
    return x == 0 ? 0 : 64 - countl_zero(x);
}

constexpr uint64_t bit_floor(const uint64_t x) noexcept {
    return x == 0 ? 0 : uint64_t{1} << (bit_width(x) - 1);
}

constexpr uint64_t bit_ceil(const uint64_t x) noexcept {
    if (x <= 1) return 1;
    const uint64_t floor = bit_floor(x);
    return floor == x ? x : floor << 1;
}

constexpr bool has_single_bit(const uint64_t x) noexcept {
    return x != 0 && (x & (x - 1)) == 0;
}

constexpr uint64_t rotate_l(const uint64_t x, const int s) noexcept {
    int shift = s % 64;
    if (shift < 0) shift += 64;
    if (shift == 0) return x;
    return (x << shift) | (x >> (64 - shift));
}

constexpr uint64_t rotate_r(const uint64_t x, const int s) noexcept {
    int shift = s % 64;
    if (shift < 0) shift += 64;
    if (shift == 0) return x;
    return (x >> shift) | (x << (64 - shift));
}

constexpr int lowest_set_bit_pos(const uint64_t x) noexcept {
    return x == 0 ? -1 : countr_zero(x);
}

constexpr int highest_set_bit_pos(const uint64_t x) noexcept {
    return x == 0 ? -1 : 63 - countl_zero(x);
}

constexpr uint64_t bit_extract(const uint64_t x, const int pos, const int len) noexcept {
    return (x >> pos) & ((uint64_t{1} << len) - 1);
}

constexpr uint64_t bit_insert(const uint64_t x, const uint64_t bits, const int pos, const int len) noexcept {
    const uint64_t mask = ((uint64_t{1} << len) - 1) << pos;
    return (x & ~mask) | ((bits << pos) & mask);
}

constexpr uint64_t reverse_bits(uint64_t x) noexcept {
    x = ((x >> 1) & 0x5555555555555555ULL) | ((x & 0x5555555555555555ULL) << 1);
    x = ((x >> 2) & 0x3333333333333333ULL) | ((x & 0x3333333333333333ULL) << 2);
    x = ((x >> 4) & 0x0F0F0F0F0F0F0F0FULL) | ((x & 0x0F0F0F0F0F0F0F0FULL) << 4);
    x = ((x >> 8) & 0x00FF00FF00FF00FFULL) | ((x & 0x00FF00FF00FF00FFULL) << 8);
    x = ((x >> 16) & 0x0000FFFF0000FFFFULL) | ((x & 0x0000FFFF0000FFFFULL) << 16);
    x = (x >> 32) | (x << 32);
    return x;
}

constexpr bool parity(uint64_t x) noexcept {
    x ^= x >> 32;
    x ^= x >> 16;
    x ^= x >> 8;
    x ^= x >> 4;
    x ^= x >> 2;
    x ^= x >> 1;
    return (x & 1) != 0;
}

constexpr uint64_t mask_from_to(const int from, const int to) noexcept {
    return ((uint64_t{1} << (to - from + 1)) - 1) << from;
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_MEMORY_BIT_HPP__
