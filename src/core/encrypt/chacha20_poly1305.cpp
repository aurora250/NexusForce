#include <NeForce/core/encrypt/chacha20_poly1305.hpp>
#include <NeForce/core/memory/endian.hpp>
#include <NeForce/core/memory/memory.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    constexpr byte_t CHACHA_CONST[16] = {0x65, 0x78, 0x70, 0x61, 0x6e, 0x64, 0x20, 0x33,
                                         0x32, 0x2d, 0x62, 0x79, 0x74, 0x65, 0x20, 0x6b};

    void chacha20_quarter_round(uint32_t* state, const int a, const int b, const int c, const int d) {
        const auto rotl = [](const uint32_t v, const int n) { return (v << n) | (v >> (32 - n)); };
        state[a] += state[b];
        state[d] ^= state[a];
        state[d] = rotl(state[d], 16);
        state[c] += state[d];
        state[b] ^= state[c];
        state[b] = rotl(state[b], 12);
        state[a] += state[b];
        state[d] ^= state[a];
        state[d] = rotl(state[d], 8);
        state[c] += state[d];
        state[b] ^= state[c];
        state[b] = rotl(state[b], 7);
    }

    void chacha20_block(const byte_t* key, const uint32_t counter, const byte_t* nonce, byte_t* output) {
        uint32_t state[16];

        state[0] = endian::read_le32(CHACHA_CONST + 0);
        state[1] = endian::read_le32(CHACHA_CONST + 4);
        state[2] = endian::read_le32(CHACHA_CONST + 8);
        state[3] = endian::read_le32(CHACHA_CONST + 12);
        for (int i = 0; i < 8; ++i) {
            state[4 + i] = endian::read_le32(key + static_cast<ptrdiff_t>(i * 4));
        }
        state[12] = counter;
        for (int i = 0; i < 3; ++i) {
            state[13 + i] = endian::read_le32(nonce + static_cast<ptrdiff_t>(i * 4));
        }

        uint32_t working[16];
        memory_copy(working, state, sizeof(state));

        for (int i = 0; i < 10; ++i) {
            chacha20_quarter_round(working, 0, 4, 8, 12);
            chacha20_quarter_round(working, 1, 5, 9, 13);
            chacha20_quarter_round(working, 2, 6, 10, 14);
            chacha20_quarter_round(working, 3, 7, 11, 15);
            chacha20_quarter_round(working, 0, 5, 10, 15);
            chacha20_quarter_round(working, 1, 6, 11, 12);
            chacha20_quarter_round(working, 2, 7, 8, 13);
            chacha20_quarter_round(working, 3, 4, 9, 14);
        }

        for (int i = 0; i < 16; ++i) {
            working[i] += state[i];
            endian::write_le32(output + static_cast<ptrdiff_t>(i * 4), working[i]);
        }
    }

    byte_vector chacha20_encrypt(const byte_t* key, const uint32_t counter, const byte_t* nonce, const byte_t* data,
                                 const size_t len) {
        byte_vector out;
        out.reserve(len);

        byte_t keystream[64];
        uint32_t blk_ctr = counter;

        for (size_t offset = 0; offset < len; offset += 64) {
            chacha20_block(key, blk_ctr, nonce, keystream);
            const size_t chunk = (len - offset > 64) ? 64 : len - offset;
            for (size_t j = 0; j < chunk; ++j) {
                out.push_back(data[offset + j] ^ keystream[j]);
            }
            ++blk_ctr;
        }

        return out;
    }

    void poly1305_clamp(byte_t r[16]) {
        r[3] &= 15;
        r[7] &= 15;
        r[11] &= 15;
        r[15] &= 15;
        r[4] &= 252;
        r[8] &= 252;
        r[12] &= 252;
    }

    void poly1305_mac(const byte_t* key, const byte_t* msg, const size_t msg_len, byte_t* tag) {
        byte_t r_bytes[16];
        memory_copy(r_bytes, key, 16);
        poly1305_clamp(r_bytes);

        const uint64_t r0 = endian::read_le32(r_bytes + 0) & 0x03ffffffU;
        const uint64_t r1 = (endian::read_le32(r_bytes + 3) >> 2) & 0x03ffffffU;
        const uint64_t r2 = (endian::read_le32(r_bytes + 6) >> 4) & 0x03ffffffU;
        const uint64_t r3 = (endian::read_le32(r_bytes + 9) >> 6) & 0x03ffffffU;
        const uint64_t r4 = (endian::read_le32(r_bytes + 12) >> 8) & 0x03ffffffU;

        const uint64_t s1 = r1 * 5;
        const uint64_t s2 = r2 * 5;
        const uint64_t s3 = r3 * 5;
        const uint64_t s4 = r4 * 5;

        const uint64_t s_lo = endian::read_le64(key + 16);
        const uint64_t s_hi = endian::read_le64(key + 24);

        uint64_t h0 = 0, h1 = 0, h2 = 0, h3 = 0, h4 = 0;

        size_t offset = 0;
        while (offset < msg_len) {
            const size_t block_len = (msg_len - offset >= 16) ? 16 : msg_len - offset;
            byte_t block[17] = {0};
            memory_copy(block, msg + offset, block_len);
            block[block_len] = 0x01;

            const uint64_t b0 = endian::read_le32(block + 0) & 0x03ffffffU;
            const uint64_t b1 = (endian::read_le32(block + 3) >> 2) & 0x03ffffffU;
            const uint64_t b2 = (endian::read_le32(block + 6) >> 4) & 0x03ffffffU;
            const uint64_t b3 = (endian::read_le32(block + 9) >> 6) & 0x03ffffffU;
            const uint64_t b4 =
                    (endian::read_le32(block + 12) >> 8) & 0x03ffffffU | (static_cast<uint64_t>(block[16]) << 24);

            h0 += b0;
            h1 += b1;
            h2 += b2;
            h3 += b3;
            h4 += b4;

            const uint64_t d0 = h0 * r0 + h1 * s4 + h2 * s3 + h3 * s2 + h4 * s1;
            const uint64_t d1 = h0 * r1 + h1 * r0 + h2 * s4 + h3 * s3 + h4 * s2;
            const uint64_t d2 = h0 * r2 + h1 * r1 + h2 * r0 + h3 * s4 + h4 * s3;
            const uint64_t d3 = h0 * r3 + h1 * r2 + h2 * r1 + h3 * r0 + h4 * s4;
            const uint64_t d4 = h0 * r4 + h1 * r3 + h2 * r2 + h3 * r1 + h4 * r0;

            uint64_t c = d0 >> 26;
            h0 = d0 & 0x03ffffffU;
            c += d1;
            h1 = c & 0x03ffffffU;
            c = (c >> 26) + d2;
            h2 = c & 0x03ffffffU;
            c = (c >> 26) + d3;
            h3 = c & 0x03ffffffU;
            c = (c >> 26) + d4;
            h4 = c & 0x03ffffffU;
            c >>= 26;

            h0 += c * 5;
            c = h0 >> 26;
            h0 &= 0x03ffffffU;
            h1 += c;

            offset += block_len;
        }

        for (int pass = 0; pass < 2; ++pass) {
            uint64_t c = h1 >> 26;
            h1 &= 0x03ffffffU;
            h2 += c;
            c = h2 >> 26;
            h2 &= 0x03ffffffU;
            h3 += c;
            c = h3 >> 26;
            h3 &= 0x03ffffffU;
            h4 += c;
            c = h4 >> 26;
            h4 &= 0x03ffffffU;
            h0 += c * 5;
            c = h0 >> 26;
            h0 &= 0x03ffffffU;
            h1 += c;
        }

        uint64_t g0 = h0 | (h1 << 26) | ((h2 & 0xFFF) << 52);
        uint64_t g1 = (h2 >> 12) | (h3 << 14) | ((h4 & 0xFFFFFF) << 40);

        g0 += s_lo;
        const uint64_t carry1 = (g0 < s_lo) ? 1 : 0;
        g1 += s_hi + carry1;

        endian::write_le64(tag + 0, g0);
        endian::write_le64(tag + 8, g1);
    }

    bool secure_compare(const byte_t* a, const byte_t* b, const size_t len) {
        volatile byte_t diff = 0;
        for (size_t i = 0; i < len; ++i) {
            diff |= a[i] ^ b[i];
        }
        return diff != 0;
    }

    size_t pad16_len(const size_t len) { return (16 - (len % 16)) % 16; }
} // namespace


byte_vector chacha20_poly1305::encrypt(cbyte_view data, cbyte_view key, cbyte_view nonce, cbyte_view aad, byte_t* tag) {
    if (key.size() != 32) {
        NEFORCE_THROW_EXCEPTION(value_exception("ChaCha20-Poly1305 requires 32-byte key"));
    }
    if (nonce.size() != 12) {
        NEFORCE_THROW_EXCEPTION(value_exception("ChaCha20-Poly1305 requires 12-byte nonce"));
    }
    if (tag == nullptr) {
        NEFORCE_THROW_EXCEPTION(value_exception("ChaCha20-Poly1305 tag output pointer cannot be null"));
    }

    byte_t otk[64];
    chacha20_block(key.data(), 0, nonce.data(), otk);

    byte_vector ciphertext = chacha20_encrypt(key.data(), 1, nonce.data(), data.data(), data.size());
    const size_t pad_aad = pad16_len(aad.size());
    const size_t pad_ct = pad16_len(ciphertext.size());
    const size_t poly_msg_len = aad.size() + pad_aad + ciphertext.size() + pad_ct + 8 + 8;

    byte_vector poly_msg;
    poly_msg.reserve(poly_msg_len);
    poly_msg.insert(poly_msg.end(), aad.data(), aad.data() + aad.size());
    poly_msg.resize(poly_msg.size() + pad_aad, 0);
    poly_msg.insert(poly_msg.end(), ciphertext.begin(), ciphertext.end());
    poly_msg.resize(poly_msg.size() + pad_ct, 0);

    byte_t len_buf[8];
    endian::write_le64(len_buf, static_cast<uint64_t>(aad.size()));
    poly_msg.insert(poly_msg.end(), len_buf, len_buf + 8);
    endian::write_le64(len_buf, static_cast<uint64_t>(ciphertext.size()));
    poly_msg.insert(poly_msg.end(), len_buf, len_buf + 8);

    poly1305_mac(otk, poly_msg.data(), poly_msg.size(), tag);

    memory_zero(otk, sizeof(otk));
    return ciphertext;
}

byte_vector chacha20_poly1305::decrypt(cbyte_view data, cbyte_view key, cbyte_view nonce, cbyte_view aad,
                                       cbyte_view tag) {
    if (key.size() != 32) {
        NEFORCE_THROW_EXCEPTION(value_exception("ChaCha20-Poly1305 requires 32-byte key"));
    }
    if (nonce.size() != 12) {
        NEFORCE_THROW_EXCEPTION(value_exception("ChaCha20-Poly1305 requires 12-byte nonce"));
    }
    if (tag.size() != 16) {
        NEFORCE_THROW_EXCEPTION(value_exception("ChaCha20-Poly1305 requires 16-byte authentication tag"));
    }

    byte_t otk[64];
    chacha20_block(key.data(), 0, nonce.data(), otk);

    const size_t pad_aad = pad16_len(aad.size());
    const size_t pad_ct = pad16_len(data.size());
    const size_t poly_msg_len = aad.size() + pad_aad + data.size() + pad_ct + 8 + 8;

    byte_vector poly_msg;
    poly_msg.reserve(poly_msg_len);
    poly_msg.insert(poly_msg.end(), aad.data(), aad.data() + aad.size());
    poly_msg.resize(poly_msg.size() + pad_aad, 0);
    poly_msg.insert(poly_msg.end(), data.data(), data.data() + data.size());
    poly_msg.resize(poly_msg.size() + pad_ct, 0);

    byte_t len_buf[8];
    endian::write_le64(len_buf, static_cast<uint64_t>(aad.size()));
    poly_msg.insert(poly_msg.end(), len_buf, len_buf + 8);
    endian::write_le64(len_buf, static_cast<uint64_t>(data.size()));
    poly_msg.insert(poly_msg.end(), len_buf, len_buf + 8);

    byte_t computed_tag[16];
    poly1305_mac(otk, poly_msg.data(), poly_msg.size(), computed_tag);
    memory_zero(otk, sizeof(otk));

    if (secure_compare(computed_tag, tag.data(), 16)) {
        memory_zero(computed_tag, sizeof(computed_tag));
        NEFORCE_THROW_EXCEPTION(value_exception("ChaCha20-Poly1305 authentication failed"));
    }

    memory_zero(computed_tag, sizeof(computed_tag));

    return chacha20_encrypt(key.data(), 1, nonce.data(), data.data(), data.size());
}

NEFORCE_END_NAMESPACE__
