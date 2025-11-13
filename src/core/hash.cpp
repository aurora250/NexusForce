#include <MSTL/core/hash.hpp>
MSTL_BEGIN_NAMESPACE__

// use switch penetrate
#pragma warning(push)
#pragma warning(disable: 26819)

#ifdef MSTL_DATA_BUS_WIDTH_32__
uint32_t MurmurHash_x32(const char* key, const size_t len, const uint32_t seed) noexcept {
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
murmur_hash MurmurHash_x64(const char* key, const size_t len, const uint32_t seed) noexcept {
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

MSTL_END_NAMESPACE__
