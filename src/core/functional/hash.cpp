#include <NeForce/core/functional/hash.hpp>
#include <NeForce/core/memory/bit.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    constexpr uint32_t BLOCK_MULTIPLIER32_1 = 0xcc9e2d51;
    constexpr uint32_t BLOCK_MULTIPLIER32_2 = 0x1b873593;
    constexpr uint32_t HASH_UPDATE_CONSTANT32 = 0xe6546b64;
    constexpr uint32_t FINAL_MIX_MULTIPLIER32_1 = 0x85ebca6b;
    constexpr uint32_t FINAL_MIX_MULTIPLIER32_2 = 0xc2b2ae35;

#ifdef NEFORCE_ARCH_BITS_64

    constexpr uint64_t MULTIPLIER64_1 = 0x87c37b91114253d5ULL;
    constexpr uint64_t MULTIPLIER64_2 = 0x4cf5ad432745937fULL;
    constexpr uint64_t FINAL_MIX_MULTIPLIER64_1 = 0xff51afd7ed558ccdULL;
    constexpr uint64_t FINAL_MIX_MULTIPLIER64_2 = 0xc4ceb9fe1a85ec53ULL;
    constexpr uint64_t HASH_UPDATE_CONSTANT64_1 = 0x52dce729;
    constexpr uint64_t HASH_UPDATE_CONSTANT64_2 = 0x38495ab5;

    constexpr uint64_t hash_mix_x64(uint64_t k) noexcept {
        k ^= k >> 33;
        k *= FINAL_MIX_MULTIPLIER64_1;
        k ^= k >> 33;
        k *= FINAL_MIX_MULTIPLIER64_2;
        k ^= k >> 33;
        return k;
    }

#endif
} // namespace


#ifdef NEFORCE_COMPILER_MSVC
// use switch penetrate
#    pragma warning(push)
#    pragma warning(disable : 26819)
#endif

uint32_t MurmurHash_x32(const void* key, const size_t len, const uint32_t seed) noexcept {
    const auto* data = static_cast<const byte_t*>(key);
    const size_t nblocks = len / 4;
    uint32_t h1 = seed;

    const auto* blocks = reinterpret_cast<const uint32_t*>(data);
    for (size_t i = 0; i < nblocks; ++i) {
        uint32_t k1 = blocks[i];

        k1 *= BLOCK_MULTIPLIER32_1;
        k1 = rotate_l32(k1, 15);
        k1 *= BLOCK_MULTIPLIER32_2;

        h1 ^= k1;
        h1 = rotate_l32(h1, 13);
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
            k1 = rotate_l32(k1, 15);
            k1 *= BLOCK_MULTIPLIER32_2;
            h1 ^= k1;
        default:
            break;
    }

    h1 ^= static_cast<uint32_t>(len);
    h1 ^= h1 >> 16;
    h1 *= FINAL_MIX_MULTIPLIER32_1;
    h1 ^= h1 >> 13;
    h1 *= FINAL_MIX_MULTIPLIER32_2;
    h1 ^= h1 >> 16;
    return h1;
}

#ifdef NEFORCE_ARCH_BITS_64

murmur_hash MurmurHash_x64(const void* key, const size_t len, const uint32_t seed) noexcept {
    const auto* data = static_cast<const byte_t*>(key);
    const size_t nblocks = len / 16;
    uint64_t h1 = seed;
    uint64_t h2 = seed;

    const auto* blocks = reinterpret_cast<const uint64_t*>(data);
    for (size_t i = 0; i < nblocks; i++) {
        uint64_t k1 = blocks[i * 2];
        uint64_t k2 = blocks[i * 2 + 1];

        k1 *= MULTIPLIER64_1;
        k1 = rotate_l64(k1, 31);
        k1 *= MULTIPLIER64_2;
        h1 ^= k1;

        h1 = rotate_l64(h1, 27);
        h1 += h2;
        h1 = h1 * 5 + HASH_UPDATE_CONSTANT64_1;

        k2 *= MULTIPLIER64_2;
        k2 = rotate_l64(k2, 33);
        k2 *= MULTIPLIER64_1;
        h2 ^= k2;

        h2 = rotate_l64(h2, 31);
        h2 += h1;
        h2 = h2 * 5 + HASH_UPDATE_CONSTANT64_2;
    }

    const byte_t* tail = data + nblocks * 16;
    uint64_t k1 = 0;
    uint64_t k2 = 0;

    switch (len & 15) {
        case 15:
            k2 ^= static_cast<uint64_t>(tail[14]) << 48;
        case 14:
            k2 ^= static_cast<uint64_t>(tail[13]) << 40;
        case 13:
            k2 ^= static_cast<uint64_t>(tail[12]) << 32;
        case 12:
            k2 ^= static_cast<uint64_t>(tail[11]) << 24;
        case 11:
            k2 ^= static_cast<uint64_t>(tail[10]) << 16;
        case 10:
            k2 ^= static_cast<uint64_t>(tail[9]) << 8;
        case 9:
            k2 ^= static_cast<uint64_t>(tail[8]) << 0;
            k2 *= MULTIPLIER64_2;
            k2 = rotate_l64(k2, 33);
            k2 *= MULTIPLIER64_1;
            h2 ^= k2;
        case 8:
            k1 ^= static_cast<uint64_t>(tail[7]) << 56;
        case 7:
            k1 ^= static_cast<uint64_t>(tail[6]) << 48;
        case 6:
            k1 ^= static_cast<uint64_t>(tail[5]) << 40;
        case 5:
            k1 ^= static_cast<uint64_t>(tail[4]) << 32;
        case 4:
            k1 ^= static_cast<uint64_t>(tail[3]) << 24;
        case 3:
            k1 ^= static_cast<uint64_t>(tail[2]) << 16;
        case 2:
            k1 ^= static_cast<uint64_t>(tail[1]) << 8;
        case 1:
            k1 ^= static_cast<uint64_t>(tail[0]) << 0;
            k1 *= MULTIPLIER64_1;
            k1 = rotate_l64(k1, 31);
            k1 *= MULTIPLIER64_2;
            h1 ^= k1;
        default:
            break;
    }

    h1 ^= len;
    h2 ^= len;
    h1 += h2;
    h2 += h1;
    h1 = hash_mix_x64(h1);
    h2 = hash_mix_x64(h2);
    h1 += h2;
    h2 += h1;
    return {h1, h2};
}

#endif

#ifdef NEFORCE_COMPILER_MSVC
#    pragma warning(pop)
#endif

NEFORCE_END_NAMESPACE__
