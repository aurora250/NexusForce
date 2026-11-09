#include <NeForce/core/hash/city_hash.hpp>
#include <NeForce/core/memory/bit.hpp>
#include <NeForce/core/memory/endian.hpp>
#include <NeForce/core/numeric/int128.hpp>
#include <NeForce/core/utility/pair.hpp>
NEFORCE_BEGIN_NAMESPACE__

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
        if (len > 8) {
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
            return city_hash_len16(static_cast<uint64_t>(len) + (a << 3), endian::read_le32(s + len - 4), mul);
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

    pair<uint64_t, uint64_t> city_weak_hash32_with_seeds(const uint64_t w, const uint64_t x, const uint64_t y,
                                                         const uint64_t z, uint64_t a, uint64_t b) noexcept {
        a += w;
        b = rotate_l64(b + a + z, 21);
        const uint64_t c = a;
        a += x;
        a += y;
        b += rotate_l64(a, 44);
        return {a + z, b + c};
    }

    pair<uint64_t, uint64_t> city_weak_hash32_with_seeds(const byte_t* s, const uint64_t a, const uint64_t b) noexcept {
        return city_weak_hash32_with_seeds(endian::read_le64(s), endian::read_le64(s + 8), endian::read_le64(s + 16),
                                           endian::read_le64(s + 24), a, b);
    }
} // namespace


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
    auto v = city_weak_hash32_with_seeds(s + len - 64, static_cast<uint64_t>(len), z);
    auto w = city_weak_hash32_with_seeds(s + len - 32, y + CITY_K1, x);
    x = x * CITY_K1 + endian::read_le64(s);

    size_t remaining = (len - 1) & ~static_cast<size_t>(63);
    do {
        x = rotate_l64(x + y + v.first + endian::read_le64(s + 8), 37) * CITY_K1;
        y = rotate_l64(y + v.second + endian::read_le64(s + 48), 42) * CITY_K1;
        x ^= w.second;
        y += v.first + endian::read_le64(s + 40);
        z = rotate_l64(z + w.first, 33) * CITY_K1;
        v = city_weak_hash32_with_seeds(s, v.second * CITY_K1, x + w.first);
        w = city_weak_hash32_with_seeds(s + 32, z + w.second, y + endian::read_le64(s + 16));
        swap(z, x);
        s += 64;
        remaining -= 64;
    } while (remaining != 0);

    return static_cast<size_t>(city_hash_len16(city_hash_len16(v.first, w.first) + city_shift_mix(y) * CITY_K1 + z,
                                               city_hash_len16(v.second, w.second) + x));
}

NEFORCE_END_NAMESPACE__
