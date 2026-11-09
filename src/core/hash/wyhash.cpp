#include <NeForce/core/hash/wyhash.hpp>
#include <NeForce/core/memory/bit.hpp>
#include <NeForce/core/memory/endian.hpp>
#include <NeForce/core/numeric/int128.hpp>
NEFORCE_BEGIN_NAMESPACE__

uint64_t wyhash(const void* key, const size_t len, uint64_t seed) noexcept {
    const auto* p = static_cast<const byte_t*>(key);
    constexpr uint64_t WYHASH_P0 = 0xa0761d6478bd642fULL;
    constexpr uint64_t WYHASH_P1 = 0xe7037ed1a0b428dbULL;
    constexpr uint64_t WYHASH_P2 = 0x8ebc6af09c88c6a3ULL;
    constexpr uint64_t WYHASH_P3 = 0x589965cc75374cc3ULL;
    constexpr uint64_t WYHASH_P4 = 0x1d8e4e27c8d1ed8bULL;

    auto a = static_cast<uint64_t>(len);
    uint64_t b = seed ^ WYHASH_P0;

    const auto wymix = [](const uint64_t lhs, const uint64_t rhs) noexcept {
        const uint128_t r = uint128_t::mul64(lhs, rhs);
        return r.lo ^ r.hi;
    };

    if (len <= 16) {
        if (len >= 4) {
            const size_t half_offset = (len >> 3) << 2;
            a = (static_cast<uint64_t>(endian::read_le32(p)) << 32) | endian::read_le32(p + half_offset);
            b = (static_cast<uint64_t>(endian::read_le32(p + len - 4)) << 32) |
                endian::read_le32(p + len - 4 - half_offset);
        } else if (len > 0) {
            const uint8_t c1 = p[0];
            const uint8_t c2 = p[len >> 1];
            const uint8_t c3 = p[len - 1];
            a = static_cast<uint64_t>(c1) | (static_cast<uint64_t>(c2) << 8) | (static_cast<uint64_t>(c3) << 16) |
                (static_cast<uint64_t>(len) << 24);
        }
    } else {
        uint64_t see1 = seed;
        uint64_t see2 = seed;
        size_t i = 0;

        if (len > 48) {
            do {
                see1 = wymix(endian::read_le64(p + i) ^ WYHASH_P1, endian::read_le64(p + i + 8) ^ see1);
                see2 = wymix(endian::read_le64(p + i + 16) ^ WYHASH_P3, endian::read_le64(p + i + 24) ^ see2);
                i += 32;
            } while (i + 32 <= len);
        }
        if (len > 16) {
            while (i + 16 <= len) {
                see1 = wymix(endian::read_le64(p + i) ^ WYHASH_P1, endian::read_le64(p + i + 8) ^ see1);
                i += 16;
            }
        }
        a = endian::read_le64(p + len - 16);
        b = endian::read_le64(p + len - 8);
    }

    uint64_t c = static_cast<uint64_t>(len) ^ WYHASH_P2;
    a ^= WYHASH_P1;
    c ^= WYHASH_P3;
    b ^= seed;
    seed = wymix(seed ^ WYHASH_P1, WYHASH_P2);

    const uint64_t d = wymix(a ^ WYHASH_P1, seed);
    a = wymix(d ^ b, c);
    b = wymix(a ^ static_cast<uint64_t>(len), WYHASH_P4);
    return wymix(b ^ a, a);
}

NEFORCE_END_NAMESPACE__
