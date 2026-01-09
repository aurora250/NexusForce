#ifndef MSTL_CORE_FUNCTIONAL_HASH_HPP__
#define MSTL_CORE_FUNCTIONAL_HASH_HPP__
#include "../typeinfo/type_traits.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename Key, typename = void>
struct hash;

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
MSTL_CONSTEXPR14 size_t FNV_hash(const byte_t* first, const size_t count) noexcept {
    size_t result = FNV_OFFSET_BASIS;
    for (size_t i = 0; i < count; i++) {
        result ^= static_cast<size_t>(first[i]);
        result *= FNV_PRIME;
    }
    return result;
}


MSTL_BEGIN_INNER__

template <typename T, enable_if_t<is_integral<T>::value, int> = 0>
MSTL_CONSTEXPR14 size_t FNV_hash_integer(const T& value) noexcept {
    size_t result = FNV_OFFSET_BASIS;
    for (size_t i = 0; i < sizeof(T); ++i) {
        const byte_t byte_val = static_cast<byte_t>((value >> (i * 8)) & 0xFF);
        result ^= static_cast<size_t>(byte_val);
        result *= FNV_PRIME;
    }
    return result;
}

template <typename CharT>
MSTL_CONSTEXPR14 size_t FNV_hash_string(const CharT* str, const size_t len) noexcept {
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

MSTL_BEGIN_INNER__
template <typename T>
union __float_converter { T f; uint64_t i; };
MSTL_END_INNER__

#define __MSTL_BUILD_FLOAT_HASH_STRUCT(OPT) \
template <> \
struct hash<OPT> { \
    MSTL_NODISCARD MSTL_CONSTEXPR14 size_t operator()(const OPT x) const noexcept { \
        if (x == 0.0f) return 0; \
        _INNER __float_converter<OPT> converter{}; \
        converter.f = x; \
        return _INNER FNV_hash_integer(converter.i); \
    } \
};

MSTL_MACRO_RANGE_FLOAT(__MSTL_BUILD_FLOAT_HASH_STRUCT)
#undef __MSTL_BUILD_FLOAT_HASH_STRUCT


// DJB2 is a non-cryptographic hash algorithm
// with simple implement, fast speed and evenly distributed.
// but in some special cases, there will still occur hash conflicts.
MSTL_CONSTEXPR14 size_t DJB2_hash(const char* str, const size_t len) noexcept {
    size_t hash = 5381;
    for (size_t i = 0; i < len; ++i) {
        hash = (hash << 5) + hash + static_cast<byte_t>(str[i]);
    }
    return hash;
}


#ifdef MSTL_DATA_BUS_WIDTH_32__

MSTL_INLINE17 constexpr uint32_t BLOCK_MULTIPLIER32_1 = 0xcc9e2d51;
MSTL_INLINE17 constexpr uint32_t BLOCK_MULTIPLIER32_2 = 0x1b873593;
MSTL_INLINE17 constexpr uint32_t HASH_UPDATE_CONSTANT32 = 0xe6546b64;
MSTL_INLINE17 constexpr uint32_t FINAL_MIX_MULTIPLIER32_1 = 0x85ebca6b;
MSTL_INLINE17 constexpr uint32_t FINAL_MIX_MULTIPLIER32_2 = 0xc2b2ae35;

constexpr uint32_t hash_rotate_x32(const uint32_t x, const int r) noexcept {
    return (x << r) | (x >> (32 - r));
}

uint32_t MurmurHash_x32(const char* key, size_t len, uint32_t seed) noexcept;

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

MSTL_CONSTEXPR14 uint64_t hash_mix_x64(uint64_t k) noexcept {
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
murmur_hash MurmurHash_x64(const char* key, size_t len, uint32_t seed) noexcept;

#endif
#pragma warning(pop)


template <typename, typename = void>
struct is_nothrow_hashable : false_type {};
template <typename Key>
struct is_nothrow_hashable<Key, void_t<decltype(_MSTL hash<Key>{}(_MSTL declval<const Key&>()))>>
    : bool_constant<noexcept(_MSTL hash<Key>{}(_MSTL declval<const Key&>()))> {};

#ifdef MSTL_STANDARD_14__
template <typename Key>
MSTL_INLINE17 constexpr bool is_nothrow_hashable_v = is_nothrow_hashable<Key>::value;
#endif


template <typename, typename, typename = void>
struct is_hash : false_type {};

template <typename Func, typename Arg>
struct is_hash<Func, Arg, enable_if_t<
    is_convertible<decltype(_MSTL declval<Func>()(_MSTL declval<Arg>())), size_t>::value
>> : true_type {};

#ifdef MSTL_STANDARD_14__
template <typename Func, typename Arg>
constexpr bool is_hash_v = is_hash<Func, Arg>::value;
#endif

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_FUNCTIONAL_HASH_HPP__
