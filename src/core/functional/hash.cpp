#include <NeForce/core/functional/hash.hpp>
#include <NeForce/core/memory/bit.hpp>
#include <NeForce/core/numeric/int128.hpp>
#include <NeForce/core/simd/types.hpp>
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

uint32_t murmur_hash32(const void* key, const size_t len, const uint32_t seed) noexcept {
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

murmur_hash murmur_hash64(const void* key, const size_t len, const uint32_t seed) noexcept {
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


namespace {
    constexpr uint64_t CITY_K0 = 0xc3a5c85c97cb3127ULL;
    constexpr uint64_t CITY_K1 = 0xb492b66fbe98f273ULL;
    constexpr uint64_t CITY_K2 = 0x9ae16a3b2f90404fULL;

    uint64_t city_shift_mix(const uint64_t val) noexcept { return val ^ (val >> 47); }

    uint64_t city_hash_len16(const uint64_t u, const uint64_t v) noexcept {
        constexpr uint64_t mul = 0x9ddfea08eb382d69ULL;
        uint64_t a = (u ^ v) * mul;
        a ^= (a >> 47);
        uint64_t b = (v ^ a) * mul;
        b ^= (b >> 47);
        b *= mul;
        return b;
    }

    uint64_t city_hash_len16(const uint64_t u, const uint64_t v, const uint64_t mul) noexcept {
        uint64_t a = (u ^ v) * mul;
        a ^= (a >> 47);
        uint64_t b = (v ^ a) * mul;
        b ^= (b >> 47);
        b *= mul;
        return b;
    }

    uint64_t city_hash_len0to16(const byte_t* s, const size_t len) noexcept {
        if (len >= 8) {
            const uint64_t mul = CITY_K2 + len * 2;
            const uint64_t a = endian::read_le64(s) + CITY_K2;
            const uint64_t b = endian::read_le64(s + len - 8);
            const uint64_t c = rotate_l64(b, 37) * mul + a;
            const uint64_t d = (rotate_l64(a, 25) + b) * mul;
            return city_hash_len16(c, d, mul);
        }
        if (len >= 4) {
            const uint64_t mul = CITY_K2 + len * 2;
            const uint64_t a = endian::read_le32(s);
            const uint64_t b = endian::read_le32(s + len - 4);
            const uint64_t c = a | ((b + static_cast<uint64_t>(s[0]) * len) << 32);
            return city_hash_len16(c, len, mul);
        }
        if (len > 0) {
            const uint8_t a = s[0];
            const uint8_t b = s[len >> 1];
            const uint8_t c = s[len - 1];
            const uint32_t y = static_cast<uint32_t>(a) + (static_cast<uint32_t>(b) << 8);
            const uint32_t z = static_cast<uint32_t>(len) + (static_cast<uint32_t>(c) << 2);
            return city_shift_mix(y * CITY_K2 ^ z * CITY_K0) * CITY_K2;
        }
        return CITY_K2;
    }

    uint64_t city_hash_len17to32(const byte_t* s, const size_t len) noexcept {
        const uint64_t mul = CITY_K2 + len * 2;
        const uint64_t a = endian::read_le64(s) * CITY_K1;
        const uint64_t b = endian::read_le64(s + 8);
        const uint64_t c = endian::read_le64(s + len - 8) * mul;
        const uint64_t d = endian::read_le64(s + len - 16) * CITY_K2;
        return city_hash_len16(rotate_l64(a + b, 43) + rotate_l64(c, 30) + d, a + rotate_l64(b + CITY_K2, 18) + c, mul);
    }

    uint64_t city_hash_len33to64(const byte_t* s, const size_t len) noexcept {
        const uint64_t mul = CITY_K2 + len * 2;
        const uint64_t a = endian::read_le64(s) * CITY_K2;
        const uint64_t b = endian::read_le64(s + 8);
        const uint64_t c = endian::read_le64(s + len - 8) * mul;
        const uint64_t d = endian::read_le64(s + len - 16) * CITY_K2;
        const uint64_t y = rotate_l64(a + b, 43) + rotate_l64(c, 30) + d;
        const uint64_t z = city_hash_len16(y, a + rotate_l64(b + CITY_K2, 18) + c, mul);
        const uint64_t e = endian::read_le64(s + 16) * mul;
        const uint64_t f = endian::read_le64(s + 24);
        const uint64_t g = (y + endian::read_le64(s + len - 32)) * mul;
        const uint64_t h = (z + endian::read_le64(s + len - 24)) * mul;
        return city_hash_len16(rotate_l64(e + f, 43) + rotate_l64(g, 30) + h, e + rotate_l64(f + a, 18) + g, mul);
    }
} // namespace


uint64_t wyhash(const void* key, const size_t len, const uint64_t seed) noexcept {
    const auto* p = static_cast<const byte_t*>(key);
    const uint64_t secret0 = seed ^ 0xa3b195354a39b70dULL;
    const uint64_t secret1 = seed ^ 0x1b03738712fad5c9ULL;
    const uint64_t secret2 = seed ^ 0x60bec20cf2e92253ULL;
    const uint64_t secret3 = seed ^ 0x56e6c6f1eb84fdcbULL;
    constexpr uint64_t secret4 = 0x3a7dbd58c8c44253ULL;

    uint64_t a = secret0;
    uint64_t b = secret1;

    if (len >= 32) {
        const byte_t* limit = p + (len & ~static_cast<size_t>(31));
        for (; p < limit; p += 32) {
            const uint64_t v0 = endian::read_le64(p);
            const uint64_t v1 = endian::read_le64(p + 8);
            const uint64_t v2 = endian::read_le64(p + 16);
            const uint64_t v3 = endian::read_le64(p + 24);

            const uint128_t ma = uint128_t::mul64(v0 ^ secret1, v2 ^ secret2);
            const uint128_t mb = uint128_t::mul64(v1 ^ a, v3 ^ b);
            a = ma.lo ^ ma.hi;
            b = mb.lo ^ mb.hi;
        }
    }

    if ((len & 16) != 0U) {
        a ^= endian::read_le64(p);
        b ^= endian::read_le64(p + 8);
        p += 16;
    }

    if ((len & 8) != 0U) {
        a ^= endian::read_le64(p);
        p += 8;
    }
    if ((len & 4) != 0U) {
        b ^= static_cast<uint64_t>(endian::read_le32(p)) << 32;
        p += 4;
    }
    if ((len & 2) != 0U) {
        b ^= static_cast<uint64_t>(endian::read_le16(p)) << 16;
        p += 2;
    }
    if ((len & 1) != 0U) {
        a ^= static_cast<uint64_t>(*p);
    }

    const uint128_t m = uint128_t::mul64(a ^ secret1, b ^ secret2);
    a = m.lo ^ m.hi;

    return a ^ secret4;
}

size_t city_hash64(const void* key, const size_t len) noexcept {
    const auto* s = static_cast<const byte_t*>(key);

    if (len <= 16) {
        return static_cast<size_t>(city_hash_len0to16(s, len));
    }
    if (len <= 32) {
        return static_cast<size_t>(city_hash_len17to32(s, len));
    }
    if (len <= 64) {
        return static_cast<size_t>(city_hash_len33to64(s, len));
    }

    uint64_t x = endian::read_le64(s + len - 40);
    uint64_t y = endian::read_le64(s + len - 16) + endian::read_le64(s + len - 56);
    uint64_t z = city_hash_len16(endian::read_le64(s + len - 48) + len, endian::read_le64(s + len - 24));
    uint64_t v_lo = 0, v_hi = 0;

    uint64_t u_lo = 0, u_hi = 0;
    {
        const uint64_t a = endian::read_le64(s);
        const uint64_t b = endian::read_le64(s + 8);
        const uint64_t c = endian::read_le64(s + len - 40);
        const uint64_t d = endian::read_le64(s + len - 32);
        u_lo = city_hash_len16(a, b);
        u_hi = city_hash_len16(c, d);
    }
    v_lo = endian::read_le64(s + 16);
    v_hi = endian::read_le64(s + len - 64);

    size_t offset = 24;
    const size_t tail_offset = len - 56;

    {
        uint64_t a = v_lo;
        uint64_t b = v_hi;
        constexpr uint64_t mul = CITY_K0 + 16;
        a += u_lo;
        b += u_hi;
        a += CITY_K0;
        b += CITY_K0;
        a = rotate_l64(a + u_lo, 16);
        a *= mul;
        b = rotate_l64(b + u_hi, 17);
        b *= mul;
        u_lo = a ^ b;
        u_hi = rotate_l64(b, 16);
    }

    {
        v_lo = endian::read_le64(s + offset);
        offset += 8;
        v_hi = endian::read_le64(s + tail_offset + (offset - 24));
    }

    while (offset < tail_offset) {
        uint64_t a = v_lo, b = v_hi;
        constexpr uint64_t mul = CITY_K0 + 16;
        a += u_lo;
        b += u_hi;
        a += CITY_K0;
        b += CITY_K0;
        a = rotate_l64(a + u_lo, 16);
        a *= mul;
        b = rotate_l64(b + u_hi, 17);
        b *= mul;
        u_lo = a ^ b;
        u_hi = rotate_l64(b, 16);

        v_lo = endian::read_le64(s + offset);
        offset += 8;
        v_hi = endian::read_le64(s + tail_offset + (offset - 24));
    }

    x += rotate_l64(u_lo + v_lo, 51) * CITY_K0;
    y += u_hi + v_hi;
    z += u_lo + v_lo;

    x = city_shift_mix(x) * CITY_K0;
    y = city_shift_mix(y);
    z = city_shift_mix(z);

    return static_cast<size_t>(city_hash_len16(x + y + z, len * 8 + z, CITY_K1));
}


namespace {
    constexpr uint64_t XXH3_PRIME64_1 = 0x9E3779B185EBCA87ULL;
    constexpr uint64_t XXH3_PRIME64_2 = 0xC2B2AE3D27D4EB4FULL;
    constexpr uint64_t XXH3_PRIME64_3 = 0x165667B19E3779F9ULL;
    constexpr uint64_t XXH3_PRIME64_4 = 0x85EBCA77C2B2AE63ULL;
    constexpr uint64_t XXH3_PRIME64_5 = 0x27D4EB2F165667C5ULL;
    constexpr size_t XXH3_SECRET_DEFAULT_SIZE = 192;
    constexpr size_t XXH3_STRIPE_LEN = 64;
    constexpr size_t XXH3_ACC_NB = 8;

    constexpr uint8_t XXH3_kSecret[192] = {
            0xb8, 0xfe, 0x6c, 0x39, 0x23, 0xa4, 0x4b, 0xbe, 0x7c, 0x01, 0x81, 0x2c, 0xf7, 0x21, 0xad, 0x1c, 0xde, 0xd4,
            0x6d, 0xe9, 0x83, 0x90, 0x97, 0xdb, 0x72, 0x40, 0xa4, 0xa4, 0xb7, 0xb3, 0x67, 0x1f, 0xcb, 0x4a, 0x8c, 0x4f,
            0xc9, 0x71, 0x0e, 0x74, 0x41, 0xfe, 0xb7, 0xec, 0xca, 0x23, 0x44, 0xdb, 0x01, 0x19, 0x92, 0x2c, 0x7f, 0x3a,
            0x64, 0x62, 0x28, 0x99, 0xb3, 0x32, 0x4c, 0x1f, 0x20, 0x02, 0x56, 0x27, 0x48, 0x37, 0x6e, 0x33, 0xee, 0x58,
            0x11, 0x4d, 0xf7, 0x79, 0x88, 0x84, 0xde, 0x56, 0xe8, 0xb6, 0x6f, 0x85, 0xbe, 0xd9, 0x17, 0x50, 0x53, 0x5f,
            0x60, 0xd7, 0xa6, 0x06, 0x04, 0x75, 0xcf, 0x82, 0x6b, 0x50, 0x00, 0x17, 0x6f, 0xce, 0x99, 0xb6, 0x3f, 0xe2,
            0xad, 0x13, 0x11, 0x09, 0x56, 0x21, 0x28, 0xc7, 0xaa, 0x97, 0xde, 0x50, 0x00, 0xcf, 0x2b, 0xd5, 0xb1, 0x02,
            0x14, 0x67, 0x2c, 0x93, 0x93, 0x2c, 0x91, 0x00, 0x01, 0x0b, 0x7c, 0xc4, 0xed, 0xfb, 0x3b, 0x81, 0x25, 0x28,
            0x74, 0x3a, 0x1b, 0x02, 0xd7, 0xa1, 0x10, 0x04, 0x33, 0x89, 0x56, 0x3e, 0x19, 0xbc, 0x3b, 0xcf, 0x60, 0x03,
            0x75, 0xf4, 0x09, 0x88, 0x3e, 0xe3, 0xd1, 0x00, 0x30, 0x46, 0x0a, 0x36, 0x21, 0x49, 0x33, 0x28, 0x50, 0xa8,
            0x93, 0xb0, 0x15, 0x53, 0xba, 0x5b, 0x65, 0xa9, 0xd8, 0xc2, 0x87, 0x3d,
    };

    uint64_t XXH3_avalanche(uint64_t h) noexcept {
        h ^= h >> 37;
        h *= 0x165667919E3779F9ULL;
        h ^= h >> 32;
        return h;
    }

    uint64_t XXH3_rrmxmx(uint64_t h, const uint64_t len) noexcept {
        h ^= rotate_l64(h, 49) ^ rotate_l64(h, 24);
        h *= 0x9FB21C651E98DF25ULL;
        h ^= (h >> 35) + len;
        h *= 0x9FB21C651E98DF25ULL;
        h ^= (h >> 28);
        return h;
    }

    uint64_t XXH3_mix16B(const byte_t* data, const byte_t* secret, uint64_t seed) noexcept {
        const uint64_t lo = endian::read_le64(data + 0);
        const uint64_t hi = endian::read_le64(data + 8);
        const uint64_t sl = endian::read_le64(secret + 0) ^ seed;
        const uint64_t sh = endian::read_le64(secret + 8) ^ seed;
        return XXH3_rrmxmx(rotate_l64(lo ^ sl, 35) + rotate_l64(hi ^ sh, 3), 16);
    }

    uint64_t XXH3_len_0to16(const byte_t* data, const size_t len, const byte_t* secret, const uint64_t seed) noexcept {
        if (len > 8) {
            return XXH3_mix16B(data, secret, seed);
        }
        if (len >= 4) {
            const uint64_t lo = endian::read_le32(data);
            const uint64_t hi = endian::read_le32(data + len - 4);
            const uint64_t sl = (endian::read_le32(secret) ^ static_cast<uint32_t>(seed)) & 0xFFFFFFFFULL;
            const uint64_t sh = (endian::read_le32(secret + 4) ^ static_cast<uint32_t>(seed >> 32)) & 0xFFFFFFFFULL;
            return XXH3_rrmxmx(rotate_l64(lo ^ sl, 35) + rotate_l64(hi ^ sh, 3), static_cast<uint64_t>(len));
        }
        if (len > 0) {
            const uint8_t a = data[0];
            const uint8_t b = data[len >> 1];
            const uint8_t c = data[len - 1];
            const uint32_t combined = (static_cast<uint32_t>(a) << 16) | (static_cast<uint32_t>(b) << 24) |
                                      static_cast<uint32_t>(c) | (static_cast<uint32_t>(len) << 8);
            const uint64_t key = static_cast<uint64_t>(endian::read_le32(secret)) ^
                                 static_cast<uint64_t>(endian::read_le32(secret + 4));
            return XXH3_rrmxmx(rotate_l64(combined ^ key, 17), static_cast<uint64_t>(len));
        }
        {
            const uint64_t bitflipl = endian::read_le64(secret + 64) ^ endian::read_le64(secret + 72);
            const uint64_t bitfliph = endian::read_le64(secret + 80) ^ endian::read_le64(secret + 88);
            return XXH3_avalanche(seed ^ bitflipl ^ bitfliph);
        }
    }

    uint64_t XXH3_len_17to128(const byte_t* data, const size_t len, const byte_t* secret, const size_t secret_size,
                              const uint64_t seed) noexcept {
        uint64_t acc = len * XXH3_PRIME64_1;
        const size_t nb_rounds = len / 16;
        for (size_t i = 0; i < nb_rounds; ++i) {
            acc += XXH3_mix16B(data + 16 * i, secret + 16 * i, seed);
            acc = rotate_l64(acc, 47);
            acc *= XXH3_PRIME64_1;
        }
        const size_t tail_offset = len - 16;
        acc += XXH3_mix16B(data + tail_offset, secret + secret_size - 17 + (tail_offset % 16), seed);
        return XXH3_avalanche(acc);
    }

    void XXH3_accumulate_512(uint64_t* acc, const byte_t* data, const byte_t* secret) noexcept {
#if defined(NEFORCE_SIMD_AVX2)
        for (size_t i = 0; i < 8; i += 4) {
            ::__m256i data_vec = ::_mm256_loadu_si256(reinterpret_cast<const ::__m256i*>(data + i * 8));
            ::__m256i key_vec = ::_mm256_xor_si256(
                    data_vec, ::_mm256_loadu_si256(reinterpret_cast<const ::__m256i*>(secret + i * 8)));

            ::__m256i swapped = ::_mm256_permute4x64_epi64(data_vec, _MM_SHUFFLE(2, 3, 0, 1));
            ::__m256i acc_vec = ::_mm256_loadu_si256(reinterpret_cast<const ::__m256i*>(acc + i));
            acc_vec = ::_mm256_add_epi64(acc_vec, swapped);

            ::__m256i product = ::_mm256_mul_epu32(key_vec, ::_mm256_srli_epi64(key_vec, 32));
            acc_vec = ::_mm256_add_epi64(acc_vec, product);

            ::_mm256_storeu_si256(reinterpret_cast<::__m256i*>(acc + i), acc_vec);
        }
#elif defined(NEFORCE_SIMD_SSE2)
        for (size_t i = 0; i < 8; i += 2) {
            ::__m128i data_vec = ::_mm_loadu_si128(reinterpret_cast<const ::__m128i*>(data + i * 8));
            ::__m128i key_vec =
                    ::_mm_xor_si128(data_vec, ::_mm_loadu_si128(reinterpret_cast<const ::__m128i*>(secret + i * 8)));

            ::__m128i swapped = _mm_shuffle_epi32(data_vec, _MM_SHUFFLE(1, 0, 3, 2));
            ::__m128i acc_vec = ::_mm_loadu_si128(reinterpret_cast<const ::__m128i*>(acc + i));
            acc_vec = ::_mm_add_epi64(acc_vec, swapped);

            ::__m128i product = ::_mm_mul_epu32(key_vec, ::_mm_srli_epi64(key_vec, 32));
            acc_vec = ::_mm_add_epi64(acc_vec, product);

            ::_mm_storeu_si128(reinterpret_cast<::__m128i*>(acc + i), acc_vec);
        }
#elif defined(NEFORCE_SIMD_NEON)
        for (size_t i = 0; i < 8; i += 2) {
            ::uint64x2_t data_vec = vld1q_u64(reinterpret_cast<const uint64_t*>(data + i * 8));
            ::uint64x2_t key_vec =
                    ::veorq_u64(data_vec, ::vld1q_u64(reinterpret_cast<const uint64_t*>(secret + i * 8)));

            ::uint64x2_t swapped = ::vextq_u64(data_vec, data_vec, 1);
            ::uint64x2_t acc_vec = ::vld1q_u64(reinterpret_cast<const uint64_t*>(acc + i));
            acc_vec = ::vaddq_u64(acc_vec, swapped);

            ::uint32x4_t key_u32 = ::vreinterpretq_u32_u64(key_vec);
            ::uint64x2_t product = ::vmull_u32(vget_low_u32(key_u32), ::vget_high_u32(key_u32));
            acc_vec = ::vaddq_u64(acc_vec, product);

            ::vst1q_u64(reinterpret_cast<uint64_t*>(acc + i), acc_vec);
        }
#else
        for (size_t i = 0; i < 8; ++i) {
            const size_t offset = i * 8;
            const uint64_t data_val = endian::read_le64(data + offset);
            const uint64_t data_key = data_val ^ endian::read_le64(secret + offset);
            acc[i ^ 1] += data_val;
            acc[i] += static_cast<uint32_t>(data_key) * static_cast<uint64_t>(data_key >> 32);
        }
#endif
    }

    void XXH3_scrambleAcc_scalar(uint64_t* acc, const byte_t* secret) noexcept {
        for (size_t i = 0; i < XXH3_ACC_NB; ++i) {
            const uint64_t key = endian::read_le64(secret + i * 8);
            acc[i] ^= acc[i] >> 47;
            acc[i] ^= key;
            acc[i] *= XXH3_PRIME64_1;
        }
    }

    uint64_t XXH3_hashLong_64b(const byte_t* data, const size_t len, const byte_t* secret,
                               const size_t secret_size) noexcept {
        uint64_t acc[XXH3_ACC_NB] = {
                XXH3_PRIME64_3, XXH3_PRIME64_1, XXH3_PRIME64_2, XXH3_PRIME64_4,
                XXH3_PRIME64_1, XXH3_PRIME64_2, XXH3_PRIME64_3, XXH3_PRIME64_4,
        };

        const size_t nb_blocks = (len - 1) / XXH3_STRIPE_LEN;
        for (size_t b = 0; b < nb_blocks; ++b) {
            for (size_t j = 0; j < XXH3_STRIPE_LEN; j += XXH3_ACC_NB * 8) {
                XXH3_accumulate_512(acc, data + b * XXH3_STRIPE_LEN + j, secret + j % secret_size);
            }
            XXH3_scrambleAcc_scalar(acc, secret + secret_size - XXH3_STRIPE_LEN);
        }

        const size_t last_stripe_offset = (nb_blocks - 1) * XXH3_STRIPE_LEN;
        for (size_t j = 0; j < secret_size - XXH3_STRIPE_LEN; j += XXH3_ACC_NB * 8) {
            const size_t data_offset = last_stripe_offset + j;
            if (data_offset + XXH3_ACC_NB * 8 > len) {
                break;
            }
            XXH3_accumulate_512(acc, data + data_offset, secret + j + XXH3_STRIPE_LEN);
        }

        {
            const size_t tail_offset = len - 1;
            const byte_t* tail = data + tail_offset;
            const size_t secret_offset = tail_offset % secret_size;
            for (size_t i = 0; i < XXH3_ACC_NB; ++i) {
                const uint64_t data_val = endian::read_le64(tail + i * 8);
                const uint64_t key = endian::read_le64(secret + secret_offset + i * 8);
                acc[i] += data_val ^ key;
                acc[i] *= XXH3_PRIME64_1;
            }
        }

        return XXH3_avalanche(rotate_l64(acc[0], 11) + rotate_l64(acc[1], 19) + rotate_l64(acc[2], 27) +
                              rotate_l64(acc[3], 35) + rotate_l64(acc[4], 43) + rotate_l64(acc[5], 51) +
                              rotate_l64(acc[6], 59) + rotate_l64(acc[7], 3));
    }
} // namespace


uint64_t XXH3_64(const void* data, const size_t len) noexcept {
    const auto* input = static_cast<const byte_t*>(data);
    const byte_t* secret = XXH3_kSecret;
    constexpr size_t secret_size = XXH3_SECRET_DEFAULT_SIZE;
    constexpr uint64_t seed = 0;

    if (len <= 16) {
        return XXH3_len_0to16(input, len, secret, seed);
    }
    if (len <= 128) {
        return XXH3_len_17to128(input, len, secret, secret_size, seed);
    }
    return XXH3_hashLong_64b(input, len, secret, secret_size);
}

#ifdef NEFORCE_COMPILER_MSVC
#    pragma warning(pop)
#endif

NEFORCE_END_NAMESPACE__
