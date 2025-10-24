#ifndef MSTL_HASH_HPP__
#define MSTL_HASH_HPP__
#include "type_traits.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename T>
struct hash<T*> {
    MSTL_NODISCARD MSTL_CONSTEXPR20 size_t operator()(const T* ptr) const noexcept {
        return static_cast<size_t>(reinterpret_cast<uintptr_t>(ptr));
    }
};

#ifdef MSTL_DATA_BUS_WIDTH_64__
MSTL_INLINE17 constexpr size_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
MSTL_INLINE17 constexpr size_t FNV_PRIME = 1099511628211ULL;
#else
MSTL_INLINE17 constexpr size_t FNV_OFFSET_BASIS = 2166136261U;
MSTL_INLINE17 constexpr size_t FNV_PRIME = 16777619U;
#endif

// the FNV (Fowler-Noll-Vo) is a non-cryptographic hash algorithm
// with a good avalanche effect and a low collision rate.
// FNV_hash function is FNV-1a version.
constexpr size_t FNV_hash(const byte_t* first, const size_t count) noexcept {
    size_t result = FNV_OFFSET_BASIS;
    for (size_t i = 0; i < count; i++) {
        result ^= static_cast<size_t>(first[i]);
        result *= FNV_PRIME;
    }
    return result;
}


MSTL_BEGIN_INNER__

template <typename T, enable_if_t<is_integral_v<T>, int> = 0>
constexpr size_t FNV_hash_integer(const T& value) noexcept {
    size_t result = FNV_OFFSET_BASIS;
    for (size_t i = 0; i < sizeof(T); ++i) {
        const byte_t byte_val = static_cast<byte_t>((value >> (i * 8)) & 0xFF);
        result ^= static_cast<size_t>(byte_val);
        result *= FNV_PRIME;
    }
    return result;
}

template <typename CharT>
constexpr size_t FNV_hash_string(const CharT* str, const size_t len) noexcept {
    size_t result = FNV_OFFSET_BASIS;
    for (size_t i = 0; i < len; ++i) {
        result ^= static_cast<size_t>(static_cast<byte_t>(str[i]));
        result *= FNV_PRIME;
    }
    return result;
}

MSTL_END_INNER__


template <>
struct hash<bool> {
    MSTL_NODISCARD constexpr size_t operator()(const bool x) const noexcept {
        return x ? 0x9e3779b9 : 0x7f4a7c15;
    }
};

#define __MSTL_BUILD_INTEGER_HASH_STRUCT(OPT) \
template <> struct hash<OPT> { \
    MSTL_NODISCARD constexpr size_t operator ()(const OPT& x) const noexcept { \
        return x == 0.0f ? 0 : _INNER FNV_hash_integer(x); \
    } \
};

MSTL_MACRO_RANGE_CHARS(__MSTL_BUILD_INTEGER_HASH_STRUCT)
MSTL_MACRO_RANGE_INT(__MSTL_BUILD_INTEGER_HASH_STRUCT)
#undef FLOAT_HASH_STRUCT__

#define __MSTL_BUILD_FLOAT_HASH_STRUCT(OPT) \
template <> \
struct hash<OPT> { \
    MSTL_NODISCARD constexpr size_t operator()(const OPT x) const noexcept { \
        if (x == 0.0f) return 0; \
        union { OPT f; uint64_t i; } converter{}; \
        converter.f = x; \
        return _INNER FNV_hash_integer(converter.i); \
    } \
};

MSTL_MACRO_RANGE_FLOAT(__MSTL_BUILD_FLOAT_HASH_STRUCT)
#undef __MSTL_BUILD_FLOAT_HASH_STRUCT


// DJB2 is a non-cryptographic hash algorithm
// with simple implement, fast speed and evenly distributed.
// but in some special cases, there will still occur hash conflicts.
constexpr size_t DJB2_hash(const char* str, const size_t len) noexcept {
    size_t hash = 5381;
    for (size_t i = 0; i < len; ++i) {
        hash = (hash << 5) + hash + static_cast<byte_t>(str[i]);
    }
    return hash;
}

// use switch penetrate
#pragma warning(push)
#pragma warning(disable: 26819)

#ifdef MSTL_DATA_BUS_WIDTH_32__

MSTL_INLINE17 constexpr uint32_t BLOCK_MULTIPLIER32_1 = 0xcc9e2d51;
MSTL_INLINE17 constexpr uint32_t BLOCK_MULTIPLIER32_2 = 0x1b873593;
MSTL_INLINE17 constexpr uint32_t HASH_UPDATE_CONSTANT32 = 0xe6546b64;
MSTL_INLINE17 constexpr uint32_t FINAL_MIX_MULTIPLIER32_1 = 0x85ebca6b;
MSTL_INLINE17 constexpr uint32_t FINAL_MIX_MULTIPLIER32_2 = 0xc2b2ae35;

constexpr uint32_t hash_rotate_x32(const uint32_t x, const int r) noexcept {
    return (x << r) | (x >> (32 - r));
}

inline uint32_t MurmurHash_x32(const char* key, const size_t len, const uint32_t seed) noexcept {
    const auto* data = reinterpret_cast<const byte_t*>(key);
    const size_t nblocks = len / 4;
    uint32_t h1 = seed;

    const auto* blocks = reinterpret_cast<const uint32_t*>(data);
    for (size_t i = 0; i < nblocks; ++i) {
        uint32_t k1 = blocks[i];

        k1 *= BLOCK_MULTIPLIER32_1;
        k1 = hash_rotate_x32(k1, 15);
        k1 *= BLOCK_MULTIPLIER32_2;

        h1 ^= k1;
        h1 = hash_rotate_x32(h1, 13);
        h1 = h1 * 5 + HASH_UPDATE_CONSTANT32;
    }

    const byte_t* tail = data + nblocks * 4;
    uint32_t k1 = 0;

    switch (len & 3) {
    case 3:
        k1 ^= static_cast<uint32_t>(tail[2]) << 16;
    case 2:
        k1 ^= static_cast<uint32_t>(tail[1]) << 8;
    case 1:
        k1 ^= static_cast<uint32_t>(tail[0]);
        k1 *= BLOCK_MULTIPLIER32_1;
        k1 = hash_rotate_x32(k1, 15);
        k1 *= BLOCK_MULTIPLIER32_2;
        h1 ^= k1;
    default: break;
    }

    h1 ^= static_cast<uint32_t>(len);
    h1 ^= h1 >> 16;
    h1 *= FINAL_MIX_MULTIPLIER32_1;
    h1 ^= h1 >> 13;
    h1 *= FINAL_MIX_MULTIPLIER32_2;
    h1 ^= h1 >> 16;
    return h1;
}

#endif

#ifdef MSTL_DATA_BUS_WIDTH_64__

constexpr uint64_t hash_rotate_x64(const uint64_t x, const int r) noexcept {
    return (x << r) | (x >> (64 - r));
}

MSTL_INLINE17 constexpr uint64_t MULTIPLIER64_1 = 0x87c37b91114253d5ULL;
MSTL_INLINE17 constexpr uint64_t MULTIPLIER64_2 = 0x4cf5ad432745937fULL;
MSTL_INLINE17 constexpr uint64_t FINAL_MIX_MULTIPLIER64_1 = 0xff51afd7ed558ccdULL;
MSTL_INLINE17 constexpr uint64_t FINAL_MIX_MULTIPLIER64_2 = 0xc4ceb9fe1a85ec53ULL;
MSTL_INLINE17 constexpr uint64_t HASH_UPDATE_CONSTANT64_1 = 0x52dce729;
MSTL_INLINE17 constexpr uint64_t HASH_UPDATE_CONSTANT64_2 = 0x38495ab5;

constexpr uint64_t hash_mix_x64(uint64_t k) noexcept {
    k ^= k >> 33;
    k *= FINAL_MIX_MULTIPLIER64_1;
    k ^= k >> 33;
    k *= FINAL_MIX_MULTIPLIER64_2;
    k ^= k >> 33;
    return k;
}

struct murmur_hash {
    size_t hash1 = 0;
    size_t hash2 = 0;

    constexpr murmur_hash() noexcept = default;
    constexpr murmur_hash(const size_t h1, const size_t h2) noexcept
    : hash1(h1), hash2(h2) {}

    MSTL_CONSTEXPR20 ~murmur_hash() noexcept = default;
};


// MurmurHash is a non-cryptographic hash algorithm
// with fast speed, low collision rate and customizable seed.
// MurmurHash_x64 is MurmurHash3 version.
inline murmur_hash MurmurHash_x64(const char* key, const size_t len, const uint32_t seed) noexcept {
    const auto* data = reinterpret_cast<const byte_t*>(key);
    const size_t nblocks = len / 16;
    uint64_t h1 = seed;
    uint64_t h2 = seed;

    const auto* blocks = reinterpret_cast<const uint64_t*>(data);
    for (size_t i = 0; i < nblocks; i++) {
        uint64_t k1 = blocks[i * 2];
        uint64_t k2 = blocks[i * 2 + 1];

        k1 *= MULTIPLIER64_1;
        k1 = hash_rotate_x64(k1, 31);
        k1 *= MULTIPLIER64_2;
        h1 ^= k1;

        h1 = hash_rotate_x64(h1, 27);
        h1 += h2;
        h1 = h1 * 5 + HASH_UPDATE_CONSTANT64_1;

        k2 *= MULTIPLIER64_2;
        k2 = hash_rotate_x64(k2, 33);
        k2 *= MULTIPLIER64_1;
        h2 ^= k2;

        h2 = hash_rotate_x64(h2, 31);
        h2 += h1;
        h2 = h2 * 5 + HASH_UPDATE_CONSTANT64_2;
    }

    const byte_t* tail = data + nblocks * 16;
    uint64_t k1 = 0;
    uint64_t k2 = 0;

    switch (len & 15) {
    case 15: k2 ^= static_cast<uint64_t>(tail[14]) << 48;
    case 14: k2 ^= static_cast<uint64_t>(tail[13]) << 40;
    case 13: k2 ^= static_cast<uint64_t>(tail[12]) << 32;
    case 12: k2 ^= static_cast<uint64_t>(tail[11]) << 24;
    case 11: k2 ^= static_cast<uint64_t>(tail[10]) << 16;
    case 10: k2 ^= static_cast<uint64_t>(tail[9]) << 8;
    case 9:  k2 ^= static_cast<uint64_t>(tail[8]) << 0;
        k2 *= MULTIPLIER64_2;
        k2 = hash_rotate_x64(k2, 33);
        k2 *= MULTIPLIER64_1;
        h2 ^= k2;
    case 8:  k1 ^= static_cast<uint64_t>(tail[7]) << 56;
    case 7:  k1 ^= static_cast<uint64_t>(tail[6]) << 48;
    case 6:  k1 ^= static_cast<uint64_t>(tail[5]) << 40;
    case 5:  k1 ^= static_cast<uint64_t>(tail[4]) << 32;
    case 4:  k1 ^= static_cast<uint64_t>(tail[3]) << 24;
    case 3:  k1 ^= static_cast<uint64_t>(tail[2]) << 16;
    case 2:  k1 ^= static_cast<uint64_t>(tail[1]) << 8;
    case 1:  k1 ^= static_cast<uint64_t>(tail[0]) << 0;
        k1 *= MULTIPLIER64_1;
        k1 = hash_rotate_x64(k1, 31);
        k1 *= MULTIPLIER64_2;
        h1 ^= k1;
    default: break;
    }

    h1 ^= len;
    h2 ^= len;
    h1 += h2;
    h2 += h1;
    h1 = hash_mix_x64(h1);
    h2 = hash_mix_x64(h2);
    h1 += h2;
    h2 += h1;
    return murmur_hash(h1, h2);
}

#endif
#pragma warning(pop)


template <typename, typename = void>
struct is_nothrow_hashable : false_type {};
template <typename Key>
struct is_nothrow_hashable<Key, void_t<decltype(_MSTL hash<Key>{}(_MSTL declval<const Key&>()))>>
    : bool_constant<noexcept(_MSTL hash<Key>{}(_MSTL declval<const Key&>()))> {};
template <typename Key>
MSTL_INLINE17 constexpr bool is_nothrow_hashable_v = is_nothrow_hashable<Key>::value;


template <typename, typename, typename = void>
struct is_hash : false_type {};

template <typename Func, typename Arg>
struct is_hash<Func, Arg, enable_if_t<
    is_convertible_v<decltype(_MSTL declval<Func>()(_MSTL declval<Arg>())), size_t>
>> : true_type {};

template <typename Func, typename Arg>
constexpr bool is_hash_v = is_hash<Func, Arg>::value;

MSTL_END_NAMESPACE__
#endif // MSTL_HASH_HPP__
