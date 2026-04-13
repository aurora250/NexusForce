#include <NeForce/core/encrypt/sha1.hpp>
#include <NeForce/core/utility/hexadecimal.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    constexpr uint32_t SHA1_rotleft(const uint32_t x, const uint32_t c) { return (x << c) | (x >> (32 - c)); }
} // namespace


byte_vector SHA1::hash(cbyte_view data) {
    const uint64_t original_len = data.size();

    if (original_len > 0x1FFFFFFFFFFFFFFFULL) {
        NEFORCE_THROW_EXCEPTION(value_exception("SHA1 input too large (max 2^61 bytes)"));
    }

    const size_t padded_len = ((original_len + 1 + 8 + 63) / 64) * 64;
    byte_vector byte_data(padded_len, 0);
    memory_copy(byte_data.data(), data.data(), original_len);
    byte_data[original_len] = 0x80;

    const uint64_t bit_len = original_len * 8;
    const size_t len_offset = padded_len - 8;
    for (int i = 0; i < 8; ++i) {
        byte_data[len_offset + i] = static_cast<byte_t>((bit_len >> (56 - i * 8)) & 0xFF);
    }

    uint32_t h0 = 0x67452301;
    uint32_t h1 = 0xEFCDAB89;
    uint32_t h2 = 0x98BADCFE;
    uint32_t h3 = 0x10325476;
    uint32_t h4 = 0xC3D2E1F0;

    for (size_t chunk_start = 0; chunk_start < padded_len; chunk_start += 64) {
        uint32_t w[80];

        for (size_t i = 0; i < 16; ++i) {
            const size_t idx = chunk_start + i * 4;
            w[i] = (static_cast<uint32_t>(byte_data[idx]) << 24) | (static_cast<uint32_t>(byte_data[idx + 1]) << 16) |
                   (static_cast<uint32_t>(byte_data[idx + 2]) << 8) | static_cast<uint32_t>(byte_data[idx + 3]);
        }

        for (int i = 16; i < 80; ++i) {
            w[i] = SHA1_rotleft(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
        }

        uint32_t a = h0, b = h1, c = h2, d = h3, e = h4;

        for (int i = 0; i < 80; ++i) {
            uint32_t f = 0, k = 0;
            if (i < 20) {
                f = (b & c) | ((~b) & d);
                k = 0x5A827999;
            } else if (i < 40) {
                f = b ^ c ^ d;
                k = 0x6ED9EBA1;
            } else if (i < 60) {
                f = (b & c) | (b & d) | (c & d);
                k = 0x8F1BBCDC;
            } else {
                f = b ^ c ^ d;
                k = 0xCA62C1D6;
            }

            const uint32_t temp = SHA1_rotleft(a, 5) + f + e + k + w[i];
            e = d;
            d = c;
            c = SHA1_rotleft(b, 30);
            b = a;
            a = temp;
        }

        h0 += a;
        h1 += b;
        h2 += c;
        h3 += d;
        h4 += e;
    }

    byte_vector result(20);
    for (size_t i = 0; i < 4; ++i) {
        result[i] = static_cast<byte_t>((h0 >> (24 - i * 8)) & 0xFF);
        result[i + 4] = static_cast<byte_t>((h1 >> (24 - i * 8)) & 0xFF);
        result[i + 8] = static_cast<byte_t>((h2 >> (24 - i * 8)) & 0xFF);
        result[i + 12] = static_cast<byte_t>((h3 >> (24 - i * 8)) & 0xFF);
        result[i + 16] = static_cast<byte_t>((h4 >> (24 - i * 8)) & 0xFF);
    }
    return result;
}

string SHA1::hash_hex(cbyte_view data) {
    byte_vector hash_result = hash(data);
    string hex_result;
    for (const byte_t byte: hash_result) {
        hex_result += format("{:02x}", byte);
    }
    return hex_result;
}

NEFORCE_END_NAMESPACE__
