#include <NeForce/core/hash/blake2.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    constexpr uint64_t BLAKE2B_IV[8] = {0x6a09e667f3bcc908ULL, 0xbb67ae8584caa73bULL, 0x3c6ef372fe94f82bULL,
                                        0xa54ff53a5f1d36f1ULL, 0x510e527fade682d1ULL, 0x9b05688c2b3e6c1fULL,
                                        0x1f83d9abfb41bd6bULL, 0x5be0cd19137e2179ULL};

    constexpr uint32_t BLAKE2S_IV[8] = {0x6a09e667U, 0xbb67ae85U, 0x3c6ef372U, 0xa54ff53aU,
                                        0x510e527fU, 0x9b05688cU, 0x1f83d9abU, 0x5be0cd19U};

    constexpr byte_t BLAKE2_SIGMA[10][16] = {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
                                             {14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3},
                                             {11, 8, 12, 0, 5, 2, 15, 13, 10, 14, 3, 6, 7, 1, 9, 4},
                                             {7, 9, 3, 1, 13, 12, 11, 14, 2, 6, 5, 10, 4, 0, 15, 8},
                                             {9, 0, 5, 7, 2, 4, 10, 15, 14, 1, 11, 12, 6, 8, 3, 13},
                                             {2, 12, 6, 10, 0, 11, 8, 3, 4, 13, 7, 5, 15, 14, 1, 9},
                                             {12, 5, 1, 15, 14, 13, 4, 10, 0, 7, 6, 3, 9, 2, 8, 11},
                                             {13, 11, 7, 14, 12, 1, 3, 9, 5, 0, 15, 4, 8, 6, 2, 10},
                                             {6, 15, 14, 9, 11, 3, 0, 8, 12, 2, 13, 7, 1, 4, 10, 5},
                                             {10, 2, 8, 4, 7, 6, 1, 5, 15, 11, 9, 14, 3, 12, 13, 0}};

    struct blake2b_state {
        uint64_t h[8];
        uint64_t t[2];
        byte_t buffer[128];
        size_t buffer_len;
        size_t digest_size;
    };

    struct blake2s_state {
        uint32_t h[8];
        uint32_t t[2];
        byte_t buffer[64];
        size_t buffer_len;
        size_t digest_size;
    };

    void blake2b_compress(blake2b_state* state, const byte_t* block, const bool last) {
        uint64_t m[16];
        for (size_t i = 0; i < 16; ++i) {
            m[i] = endian::read_le64(block + i * 8);
        }

        uint64_t v[16];
        for (size_t i = 0; i < 8; ++i) {
            v[i] = state->h[i];
            v[i + 8] = BLAKE2B_IV[i];
        }
        v[12] ^= state->t[0];
        v[13] ^= state->t[1];
        if (last) {
            v[14] = ~v[14];
        }

        const auto g = [&v](const size_t a, const size_t b, const size_t c, const size_t d, const uint64_t x,
                            const uint64_t y) {
            v[a] = v[a] + v[b] + x;
            v[d] = rotate_r64(v[d] ^ v[a], 32);
            v[c] = v[c] + v[d];
            v[b] = rotate_r64(v[b] ^ v[c], 24);
            v[a] = v[a] + v[b] + y;
            v[d] = rotate_r64(v[d] ^ v[a], 16);
            v[c] = v[c] + v[d];
            v[b] = rotate_r64(v[b] ^ v[c], 63);
        };

        for (size_t round = 0; round < 12; ++round) {
            const byte_t* s = BLAKE2_SIGMA[round % 10];
            g(0, 4, 8, 12, m[s[0]], m[s[1]]);
            g(1, 5, 9, 13, m[s[2]], m[s[3]]);
            g(2, 6, 10, 14, m[s[4]], m[s[5]]);
            g(3, 7, 11, 15, m[s[6]], m[s[7]]);
            g(0, 5, 10, 15, m[s[8]], m[s[9]]);
            g(1, 6, 11, 12, m[s[10]], m[s[11]]);
            g(2, 7, 8, 13, m[s[12]], m[s[13]]);
            g(3, 4, 9, 14, m[s[14]], m[s[15]]);
        }

        for (size_t i = 0; i < 8; ++i) {
            state->h[i] ^= v[i] ^ v[i + 8];
        }
    }

    void blake2s_compress(blake2s_state* state, const byte_t* block, const bool last) {
        uint32_t m[16];
        for (size_t i = 0; i < 16; ++i) {
            m[i] = endian::read_le32(block + i * 4);
        }

        uint32_t v[16];
        for (size_t i = 0; i < 8; ++i) {
            v[i] = state->h[i];
            v[i + 8] = BLAKE2S_IV[i];
        }
        v[12] ^= state->t[0];
        v[13] ^= state->t[1];
        if (last) {
            v[14] = ~v[14];
        }

        const auto g = [&v](const size_t a, const size_t b, const size_t c, const size_t d, const uint32_t x,
                            const uint32_t y) {
            v[a] = v[a] + v[b] + x;
            v[d] = rotate_r32(v[d] ^ v[a], 16);
            v[c] = v[c] + v[d];
            v[b] = rotate_r32(v[b] ^ v[c], 12);
            v[a] = v[a] + v[b] + y;
            v[d] = rotate_r32(v[d] ^ v[a], 8);
            v[c] = v[c] + v[d];
            v[b] = rotate_r32(v[b] ^ v[c], 7);
        };

        for (size_t round = 0; round < 10; ++round) {
            const byte_t* s = BLAKE2_SIGMA[round];
            g(0, 4, 8, 12, m[s[0]], m[s[1]]);
            g(1, 5, 9, 13, m[s[2]], m[s[3]]);
            g(2, 6, 10, 14, m[s[4]], m[s[5]]);
            g(3, 7, 11, 15, m[s[6]], m[s[7]]);
            g(0, 5, 10, 15, m[s[8]], m[s[9]]);
            g(1, 6, 11, 12, m[s[10]], m[s[11]]);
            g(2, 7, 8, 13, m[s[12]], m[s[13]]);
            g(3, 4, 9, 14, m[s[14]], m[s[15]]);
        }

        for (size_t i = 0; i < 8; ++i) {
            state->h[i] ^= v[i] ^ v[i + 8];
        }
    }

    void blake2b_init(blake2b_state* state, const size_t digest_size, const size_t key_size) {
        for (size_t i = 0; i < 8; ++i) {
            state->h[i] = BLAKE2B_IV[i];
        }
        state->h[0] ^= 0x01010000ULL ^ (static_cast<uint64_t>(key_size) << 8) ^ digest_size;
        state->t[0] = 0;
        state->t[1] = 0;
        state->buffer_len = 0;
        state->digest_size = digest_size;
    }

    void blake2s_init(blake2s_state* state, const size_t digest_size, const size_t key_size) {
        for (size_t i = 0; i < 8; ++i) {
            state->h[i] = BLAKE2S_IV[i];
        }
        state->h[0] ^= 0x01010000U ^ (static_cast<uint32_t>(key_size) << 8) ^ digest_size;
        state->t[0] = 0;
        state->t[1] = 0;
        state->buffer_len = 0;
        state->digest_size = digest_size;
    }

    void blake2b_increment(blake2b_state* state, const uint64_t count) {
        const uint64_t previous = state->t[0];
        state->t[0] += count;
        if (state->t[0] < previous) {
            ++state->t[1];
        }
    }

    void blake2s_increment(blake2s_state* state, const uint32_t count) {
        const uint32_t previous = state->t[0];
        state->t[0] += count;
        if (state->t[0] < previous) {
            ++state->t[1];
        }
    }

    void blake2b_update(blake2b_state* state, const cbyte_view data) {
        if (data.empty()) {
            return;
        }

        size_t offset = 0;
        if (state->buffer_len + data.size() > 128) {
            const size_t fill = 128 - state->buffer_len;
            for (size_t i = 0; i < fill; ++i) {
                state->buffer[state->buffer_len + i] = data[i];
            }
            blake2b_increment(state, 128);
            blake2b_compress(state, state->buffer, false);
            state->buffer_len = 0;
            offset = fill;

            while (data.size() - offset > 128) {
                blake2b_increment(state, 128);
                blake2b_compress(state, data.data() + offset, false);
                offset += 128;
            }
        }

        while (offset < data.size()) {
            state->buffer[state->buffer_len++] = data[offset++];
        }
    }

    void blake2s_update(blake2s_state* state, const cbyte_view data) {
        if (data.empty()) {
            return;
        }

        size_t offset = 0;
        if (state->buffer_len + data.size() > 64) {
            const size_t fill = 64 - state->buffer_len;
            for (size_t i = 0; i < fill; ++i) {
                state->buffer[state->buffer_len + i] = data[i];
            }
            blake2s_increment(state, 64);
            blake2s_compress(state, state->buffer, false);
            state->buffer_len = 0;
            offset = fill;

            while (data.size() - offset > 64) {
                blake2s_increment(state, 64);
                blake2s_compress(state, data.data() + offset, false);
                offset += 64;
            }
        }

        while (offset < data.size()) {
            state->buffer[state->buffer_len++] = data[offset++];
        }
    }

    byte_vector blake2b_final(blake2b_state* state) {
        blake2b_increment(state, state->buffer_len);
        for (size_t i = state->buffer_len; i < 128; ++i) {
            state->buffer[i] = 0;
        }
        blake2b_compress(state, state->buffer, true);

        byte_vector result;
        result.reserve(state->digest_size);
        for (size_t i = 0; i < 8 && result.size() < state->digest_size; ++i) {
            for (size_t j = 0; j < 8 && result.size() < state->digest_size; ++j) {
                result.push_back(static_cast<byte_t>((state->h[i] >> (j * 8)) & 0xFF));
            }
        }
        return result;
    }

    byte_vector blake2s_final(blake2s_state* state) {
        blake2s_increment(state, static_cast<uint32_t>(state->buffer_len));
        for (size_t i = state->buffer_len; i < 64; ++i) {
            state->buffer[i] = 0;
        }
        blake2s_compress(state, state->buffer, true);

        byte_vector result;
        result.reserve(state->digest_size);
        for (size_t i = 0; i < 8 && result.size() < state->digest_size; ++i) {
            for (size_t j = 0; j < 4 && result.size() < state->digest_size; ++j) {
                result.push_back(static_cast<byte_t>((state->h[i] >> (j * 8)) & 0xFF));
            }
        }
        return result;
    }

    string to_hex(const byte_vector& bytes) {
        constexpr char hex_chars[] = "0123456789abcdef";
        string result;
        result.reserve(bytes.size() * 2);
        for (const byte_t byte: bytes) {
            result.push_back(hex_chars[byte >> 4]);
            result.push_back(hex_chars[byte & 0x0F]);
        }
        return result;
    }

    byte_vector blake2b_hash(const cbyte_view data, const cbyte_view key, const size_t digest_size) {
        if (digest_size == 0 || digest_size > 64) {
            NEFORCE_THROW_EXCEPTION(value_exception("Invalid BLAKE2b digest size"));
        }
        if (key.size() > 64) {
            NEFORCE_THROW_EXCEPTION(value_exception("Invalid BLAKE2b key size"));
        }

        blake2b_state state{};
        blake2b_init(&state, digest_size, key.size());

        if (!key.empty()) {
            byte_t key_block[128] = {};
            for (size_t i = 0; i < key.size(); ++i) {
                key_block[i] = key[i];
            }
            blake2b_update(&state, cbyte_view{key_block, 128});
        }

        blake2b_update(&state, data);
        return blake2b_final(&state);
    }

    byte_vector blake2s_hash(const cbyte_view data, const cbyte_view key, const size_t digest_size) {
        if (digest_size == 0 || digest_size > 32) {
            NEFORCE_THROW_EXCEPTION(value_exception("Invalid BLAKE2s digest size"));
        }
        if (key.size() > 32) {
            NEFORCE_THROW_EXCEPTION(value_exception("Invalid BLAKE2s key size"));
        }

        blake2s_state state{};
        blake2s_init(&state, digest_size, key.size());

        if (!key.empty()) {
            byte_t key_block[64] = {};
            for (size_t i = 0; i < key.size(); ++i) {
                key_block[i] = key[i];
            }
            blake2s_update(&state, cbyte_view{key_block, 64});
        }

        blake2s_update(&state, data);
        return blake2s_final(&state);
    }
} // namespace


byte_vector BLAKE2b::hash(const cbyte_view data, const size_t digest_size) {
    return blake2b_hash(data, cbyte_view{}, digest_size);
}

string BLAKE2b::hash_hex(const cbyte_view data, const size_t digest_size) { return to_hex(hash(data, digest_size)); }

byte_vector BLAKE2b::keyed_hash(const cbyte_view data, const cbyte_view key, const size_t digest_size) {
    return blake2b_hash(data, key, digest_size);
}

string BLAKE2b::keyed_hash_hex(const cbyte_view data, const cbyte_view key, const size_t digest_size) {
    return to_hex(keyed_hash(data, key, digest_size));
}

byte_vector BLAKE2s::hash(const cbyte_view data, const size_t digest_size) {
    return blake2s_hash(data, cbyte_view{}, digest_size);
}

string BLAKE2s::hash_hex(const cbyte_view data, const size_t digest_size) { return to_hex(hash(data, digest_size)); }

byte_vector BLAKE2s::keyed_hash(const cbyte_view data, const cbyte_view key, const size_t digest_size) {
    return blake2s_hash(data, key, digest_size);
}

string BLAKE2s::keyed_hash_hex(const cbyte_view data, const cbyte_view key, const size_t digest_size) {
    return to_hex(keyed_hash(data, key, digest_size));
}

NEFORCE_END_NAMESPACE__
