#include <NeForce/core/encrypt/md5.hpp>
#include <NeForce/core/utility/hexadecimal.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    constexpr uint32_t md5_S[64] = {7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
                                    5, 9,  14, 20, 5, 9,  14, 20, 5, 9,  14, 20, 5, 9,  14, 20,
                                    4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
                                    6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21};

    constexpr uint32_t md5_K[64] = {
            0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
            0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be, 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
            0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
            0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
            0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
            0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
            0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
            0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1, 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391};

    constexpr uint32_t md5_rotleft(const uint32_t x, const uint32_t c) { return (x << c) | (x >> (32 - c)); }
    constexpr uint32_t md5_F(const uint32_t x, const uint32_t y, const uint32_t z) { return (x & y) | (~x & z); }
    constexpr uint32_t md5_G(const uint32_t x, const uint32_t y, const uint32_t z) { return (x & z) | (y & ~z); }
    constexpr uint32_t md5_H(const uint32_t x, const uint32_t y, const uint32_t z) { return x ^ y ^ z; }
    constexpr uint32_t md5_I(const uint32_t x, const uint32_t y, const uint32_t z) { return y ^ (x | ~z); }
} // namespace


byte_vector MD5::hash(const cbyte_view data) {
    byte_vector byte_data{data.begin(), data.end()};
    const uint64_t original_len = byte_data.size();

    byte_data.push_back(0x80);

    const size_t current_mod = byte_data.size() % 64;
    const size_t padding_zeros = (current_mod <= 56) ? (56 - current_mod) : (120 - current_mod);

    byte_data.resize(byte_data.size() + padding_zeros, 0);

    const uint64_t bit_len = original_len * 8;
    for (int i = 0; i < 8; ++i) {
        byte_data.push_back(static_cast<byte_t>((bit_len >> (i * 8)) & 0xFF));
    }

    uint32_t h0 = 0x67452301;
    uint32_t h1 = 0xefcdab89;
    uint32_t h2 = 0x98badcfe;
    uint32_t h3 = 0x10325476;

    for (size_t chunk_start = 0; chunk_start < byte_data.size(); chunk_start += 64) {
        uint32_t w[16];

        for (int i = 0; i < 16; ++i) {
            const size_t offset = chunk_start + static_cast<size_t>(i * 4);
            w[i] = static_cast<uint32_t>(byte_data[offset + 0]) | (static_cast<uint32_t>(byte_data[offset + 1]) << 8) |
                   (static_cast<uint32_t>(byte_data[offset + 2]) << 16) |
                   (static_cast<uint32_t>(byte_data[offset + 3]) << 24);
        }

        uint32_t a = h0, b = h1, c = h2, d = h3;

        for (int i = 0; i < 64; ++i) {
            uint32_t f = 0, g = 0;
            if (i < 16) {
                f = md5_F(b, c, d);
                g = static_cast<uint32_t>(i);
            } else if (i < 32) {
                f = md5_G(b, c, d);
                g = (5 * static_cast<uint32_t>(i) + 1) % 16;
            } else if (i < 48) {
                f = md5_H(b, c, d);
                g = (3 * static_cast<uint32_t>(i) + 5) % 16;
            } else {
                f = md5_I(b, c, d);
                g = (7 * static_cast<uint32_t>(i)) % 16;
            }

            f = f + a + md5_K[i] + w[g];
            a = d;
            d = c;
            c = b;
            b = b + md5_rotleft(f, md5_S[i]);
        }

        h0 += a;
        h1 += b;
        h2 += c;
        h3 += d;
    }

    byte_vector result(16);
    for (int i = 0; i < 4; ++i) {
        result[i] = static_cast<byte_t>((h0 >> (i * 8)) & 0xFF);
        result[i + 4] = static_cast<byte_t>((h1 >> (i * 8)) & 0xFF);
        result[i + 8] = static_cast<byte_t>((h2 >> (i * 8)) & 0xFF);
        result[i + 12] = static_cast<byte_t>((h3 >> (i * 8)) & 0xFF);
    }
    return result;
}

string MD5::hash_hex(const cbyte_view data) {
    byte_vector hash_result = hash(data);
    string hex_result;
    for (const byte_t byte: hash_result) {
        hex_result += format("{:02x}", byte);
    }
    return hex_result;
}

NEFORCE_END_NAMESPACE__
