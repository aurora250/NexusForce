#include <NeForce/core/encrypt/sha256.hpp>
#include <NeForce/core/utility/hexadecimal.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    constexpr uint32_t SHA256_K[64] = {
            0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
            0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
            0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
            0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
            0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
            0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
            0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
            0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

    constexpr uint32_t SHA256_rotr(const uint32_t x, const uint32_t n) { return (x >> n) | (x << (32 - n)); }
    constexpr uint32_t SHA256_ch(const uint32_t x, const uint32_t y, const uint32_t z) { return (x & y) ^ (~x & z); }
    constexpr uint32_t SHA256_maj(const uint32_t x, const uint32_t y, const uint32_t z) {
        return (x & y) ^ (x & z) ^ (y & z);
    }
    constexpr uint32_t SHA256_sig0(const uint32_t x) {
        return SHA256_rotr(x, 2) ^ SHA256_rotr(x, 13) ^ SHA256_rotr(x, 22);
    }
    constexpr uint32_t SHA256_sig1(const uint32_t x) {
        return SHA256_rotr(x, 6) ^ SHA256_rotr(x, 11) ^ SHA256_rotr(x, 25);
    }
    constexpr uint32_t SHA256_gamma0(const uint32_t x) { return SHA256_rotr(x, 7) ^ SHA256_rotr(x, 18) ^ (x >> 3); }
    constexpr uint32_t SHA256_gamma1(const uint32_t x) { return SHA256_rotr(x, 17) ^ SHA256_rotr(x, 19) ^ (x >> 10); }
} // namespace


byte_vector SHA256::hash(const cbyte_view data) {
    byte_vector byte_data{data.data(), data.size()};
    const uint64_t original_len = byte_data.size();
    byte_data.push_back(0x80);

    while ((byte_data.size() % 64) != 56) {
        byte_data.push_back(0);
    }
    const uint64_t bit_len = original_len * 8;
    for (int i = 7; i >= 0; --i) {
        byte_data.push_back((bit_len >> (i * 8)) & 0xFF);
    }

    uint32_t h[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};

    for (size_t chunk_start = 0; chunk_start < byte_data.size(); chunk_start += 64) {
        uint32_t w[64];
        for (size_t i = 0; i < 16; ++i) {
            w[i] = (byte_data[chunk_start + i * 4] << 24) | (byte_data[chunk_start + i * 4 + 1] << 16) |
                   (byte_data[chunk_start + i * 4 + 2] << 8) | (byte_data[chunk_start + i * 4 + 3]);
        }

        for (int i = 16; i < 64; ++i) {
            w[i] = SHA256_gamma1(w[i - 2]) + w[i - 7] + SHA256_gamma0(w[i - 15]) + w[i - 16];
        }
        uint32_t a = h[0], b = h[1], c = h[2], d = h[3];
        uint32_t e = h[4], f = h[5], g = h[6], h_val = h[7];

        for (int i = 0; i < 64; ++i) {
            const uint32_t t1 = h_val + SHA256_sig1(e) + SHA256_ch(e, f, g) + SHA256_K[i] + w[i];
            const uint32_t t2 = SHA256_sig0(a) + SHA256_maj(a, b, c);

            h_val = g;
            g = f;
            f = e;
            e = d + t1;
            d = c;
            c = b;
            b = a;
            a = t1 + t2;
        }
        h[0] += a;
        h[1] += b;
        h[2] += c;
        h[3] += d;
        h[4] += e;
        h[5] += f;
        h[6] += g;
        h[7] += h_val;
    }

    byte_vector result(32);
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 4; ++j) {
            result[i * 4 + j] = (h[i] >> (24 - j * 8)) & 0xFF;
        }
    }
    return result;
}

string SHA256::hash_hex(const cbyte_view data) {
    byte_vector hash_result = hash(data);
    string hex_result;
    for (const byte_t byte: hash_result) {
        hex_result += format("{:02x}", byte);
    }
    return hex_result;
}

NEFORCE_END_NAMESPACE__
