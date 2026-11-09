#include <NeForce/core/hash/blake3.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    constexpr uint32_t BLAKE3_IV[8] = {0x6A09E667U, 0xBB67AE85U, 0x3C6EF372U, 0xA54FF53AU,
                                       0x510E527FU, 0x9B05688CU, 0x1F83D9ABU, 0x5BE0CD19U};

    constexpr byte_t BLAKE3_MSG_PERMUTATION[16] = {2, 6, 3, 10, 7, 0, 4, 13, 1, 11, 12, 5, 9, 14, 15, 8};

    constexpr uint32_t BLAKE3_CHUNK_START = 1U << 0;
    constexpr uint32_t BLAKE3_CHUNK_END = 1U << 1;
    constexpr uint32_t BLAKE3_PARENT = 1U << 2;
    constexpr uint32_t BLAKE3_ROOT = 1U << 3;
    constexpr uint32_t BLAKE3_KEYED_HASH = 1U << 4;
    constexpr uint32_t BLAKE3_DERIVE_KEY_CONTEXT = 1U << 5;
    constexpr uint32_t BLAKE3_DERIVE_KEY_MATERIAL = 1U << 6;

    constexpr size_t BLAKE3_BLOCK_SIZE = 64;
    constexpr size_t BLAKE3_CHUNK_SIZE = 1024;

    void blake3_g(uint32_t* state, const size_t a, const size_t b, const size_t c, const size_t d, const uint32_t mx,
                  const uint32_t my) noexcept {
        state[a] = state[a] + state[b] + mx;
        state[d] = rotate_r32(state[d] ^ state[a], 16);
        state[c] = state[c] + state[d];
        state[b] = rotate_r32(state[b] ^ state[c], 12);
        state[a] = state[a] + state[b] + my;
        state[d] = rotate_r32(state[d] ^ state[a], 8);
        state[c] = state[c] + state[d];
        state[b] = rotate_r32(state[b] ^ state[c], 7);
    }

    void blake3_round(uint32_t* state, const uint32_t* m) noexcept {
        blake3_g(state, 0, 4, 8, 12, m[0], m[1]);
        blake3_g(state, 1, 5, 9, 13, m[2], m[3]);
        blake3_g(state, 2, 6, 10, 14, m[4], m[5]);
        blake3_g(state, 3, 7, 11, 15, m[6], m[7]);
        blake3_g(state, 0, 5, 10, 15, m[8], m[9]);
        blake3_g(state, 1, 6, 11, 12, m[10], m[11]);
        blake3_g(state, 2, 7, 8, 13, m[12], m[13]);
        blake3_g(state, 3, 4, 9, 14, m[14], m[15]);
    }

    void blake3_permute(uint32_t* m) noexcept {
        uint32_t permuted[16];
        for (size_t i = 0; i < 16; ++i) {
            permuted[i] = m[BLAKE3_MSG_PERMUTATION[i]];
        }
        for (size_t i = 0; i < 16; ++i) {
            m[i] = permuted[i];
        }
    }

    void blake3_compress(const uint32_t* cv, const uint32_t* block, const uint64_t counter, const uint32_t block_len,
                         const uint32_t flags, uint32_t* out) noexcept {
        uint32_t state[16];
        for (size_t i = 0; i < 8; ++i) {
            state[i] = cv[i];
            state[i + 8] = BLAKE3_IV[i];
        }
        state[12] = static_cast<uint32_t>(counter);
        state[13] = static_cast<uint32_t>(counter >> 32);
        state[14] = block_len;
        state[15] = flags;

        uint32_t m[16];
        for (size_t i = 0; i < 16; ++i) {
            m[i] = block[i];
        }

        for (size_t round = 0; round < 7; ++round) {
            blake3_round(state, m);
            if (round < 6) {
                blake3_permute(m);
            }
        }

        for (size_t i = 0; i < 8; ++i) {
            out[i] = state[i] ^ state[i + 8];
            out[i + 8] = state[i + 8] ^ cv[i];
        }
    }

    struct blake3_output {
        uint32_t input_cv[8];
        uint32_t block_words[16];
        uint64_t counter;
        uint32_t block_len;
        uint32_t flags;
    };

    struct blake3_cv {
        uint32_t words[8];
    };

    void blake3_output_cv(const blake3_output& output, uint32_t* cv) noexcept {
        uint32_t words[16];
        blake3_compress(output.input_cv, output.block_words, output.counter, output.block_len, output.flags, words);
        for (size_t i = 0; i < 8; ++i) {
            cv[i] = words[i];
        }
    }

    void blake3_root_bytes(const blake3_output& output, byte_t* result, const size_t length) noexcept {
        uint64_t output_block_counter = 0;
        size_t offset = 0;
        while (offset < length) {
            uint32_t words[16];
            blake3_compress(output.input_cv, output.block_words, output_block_counter, output.block_len,
                            output.flags | BLAKE3_ROOT, words);
            for (size_t i = 0; i < 16 && offset < length; ++i) {
                for (size_t j = 0; j < 4 && offset < length; ++j) {
                    result[offset++] = static_cast<byte_t>((words[i] >> (j * 8)) & 0xFF);
                }
            }
            ++output_block_counter;
        }
    }

    void blake3_load_block(const byte_t* data, const size_t length, uint32_t* block) noexcept {
        for (size_t i = 0; i < 16; ++i) {
            block[i] = 0;
        }
        for (size_t i = 0; i < length; ++i) {
            block[i / 4] |= static_cast<uint32_t>(data[i]) << ((i % 4) * 8);
        }
    }

    blake3_output blake3_chunk_output(const cbyte_view chunk, const uint64_t chunk_counter, const uint32_t* key,
                                      const uint32_t base_flags) noexcept {
        uint32_t cv[8];
        for (size_t i = 0; i < 8; ++i) {
            cv[i] = key[i];
        }

        const size_t block_count = chunk.empty() ? 1 : (chunk.size() + BLAKE3_BLOCK_SIZE - 1) / BLAKE3_BLOCK_SIZE;

        for (size_t i = 0; i + 1 < block_count; ++i) {
            uint32_t block[16];
            blake3_load_block(chunk.data() + i * BLAKE3_BLOCK_SIZE, BLAKE3_BLOCK_SIZE, block);
            uint32_t words[16];
            const uint32_t flags = base_flags | (i == 0 ? BLAKE3_CHUNK_START : 0U);
            blake3_compress(cv, block, chunk_counter, static_cast<uint32_t>(BLAKE3_BLOCK_SIZE), flags, words);
            for (size_t j = 0; j < 8; ++j) {
                cv[j] = words[j];
            }
        }

        const size_t last_offset = (block_count - 1) * BLAKE3_BLOCK_SIZE;
        const size_t last_length = chunk.size() - last_offset;
        blake3_output output{};
        for (size_t i = 0; i < 8; ++i) {
            output.input_cv[i] = cv[i];
        }
        blake3_load_block(chunk.data() + last_offset, last_length, output.block_words);
        output.counter = chunk_counter;
        output.block_len = static_cast<uint32_t>(last_length);
        output.flags = base_flags | (block_count == 1 ? BLAKE3_CHUNK_START : 0U) | BLAKE3_CHUNK_END;
        return output;
    }

    blake3_output blake3_parent_output(const uint32_t* left, const uint32_t* right, const uint32_t* key,
                                       const uint32_t base_flags) noexcept {
        blake3_output output{};
        for (size_t i = 0; i < 8; ++i) {
            output.input_cv[i] = key[i];
            output.block_words[i] = left[i];
            output.block_words[i + 8] = right[i];
        }
        output.counter = 0;
        output.block_len = static_cast<uint32_t>(BLAKE3_BLOCK_SIZE);
        output.flags = base_flags | BLAKE3_PARENT;
        return output;
    }

    blake3_cv blake3_chunk_cv(const cbyte_view chunk, const uint64_t chunk_index, const uint32_t* key,
                              const uint32_t base_flags) noexcept {
        blake3_cv cv{};
        const blake3_output output = blake3_chunk_output(chunk, chunk_index, key, base_flags);
        blake3_output_cv(output, cv.words);
        return cv;
    }

    blake3_cv blake3_parent_cv(const blake3_cv& left, const blake3_cv& right, const uint32_t* key,
                               const uint32_t base_flags) noexcept {
        blake3_cv cv{};
        const blake3_output output = blake3_parent_output(left.words, right.words, key, base_flags);
        blake3_output_cv(output, cv.words);
        return cv;
    }

#ifdef NEFORCE_SIMD_AVX2
    using blake3_vec = ::__m256i;

    blake3_vec blake3_rotr16(const blake3_vec value) noexcept {
        return _mm256_or_si256(_mm256_srli_epi32(value, 16), _mm256_slli_epi32(value, 16));
    }

    blake3_vec blake3_rotr12(const blake3_vec value) noexcept {
        return _mm256_or_si256(_mm256_srli_epi32(value, 12), _mm256_slli_epi32(value, 20));
    }

    blake3_vec blake3_rotr8(const blake3_vec value) noexcept {
        return _mm256_or_si256(_mm256_srli_epi32(value, 8), _mm256_slli_epi32(value, 24));
    }

    blake3_vec blake3_rotr7(const blake3_vec value) noexcept {
        return _mm256_or_si256(_mm256_srli_epi32(value, 7), _mm256_slli_epi32(value, 25));
    }

    void blake3_g_x8(blake3_vec* state, const size_t a, const size_t b, const size_t c, const size_t d,
                     const blake3_vec mx, const blake3_vec my) noexcept {
        state[a] = _mm256_add_epi32(_mm256_add_epi32(state[a], state[b]), mx);
        state[d] = blake3_rotr16(_mm256_xor_si256(state[d], state[a]));
        state[c] = _mm256_add_epi32(state[c], state[d]);
        state[b] = blake3_rotr12(_mm256_xor_si256(state[b], state[c]));
        state[a] = _mm256_add_epi32(_mm256_add_epi32(state[a], state[b]), my);
        state[d] = blake3_rotr8(_mm256_xor_si256(state[d], state[a]));
        state[c] = _mm256_add_epi32(state[c], state[d]);
        state[b] = blake3_rotr7(_mm256_xor_si256(state[b], state[c]));
    }

    void blake3_round_x8(blake3_vec* state, const blake3_vec* m) noexcept {
        blake3_g_x8(state, 0, 4, 8, 12, m[0], m[1]);
        blake3_g_x8(state, 1, 5, 9, 13, m[2], m[3]);
        blake3_g_x8(state, 2, 6, 10, 14, m[4], m[5]);
        blake3_g_x8(state, 3, 7, 11, 15, m[6], m[7]);
        blake3_g_x8(state, 0, 5, 10, 15, m[8], m[9]);
        blake3_g_x8(state, 1, 6, 11, 12, m[10], m[11]);
        blake3_g_x8(state, 2, 7, 8, 13, m[12], m[13]);
        blake3_g_x8(state, 3, 4, 9, 14, m[14], m[15]);
    }

    void blake3_compress_x8(const blake3_vec* cv, const blake3_vec* m, const blake3_vec counter_lo,
                            const blake3_vec counter_hi, const blake3_vec block_len, const blake3_vec flags,
                            blake3_vec* out) noexcept {
        blake3_vec state[16];
        for (size_t i = 0; i < 8; ++i) {
            state[i] = cv[i];
            state[i + 8] = _mm256_set1_epi32(static_cast<int>(BLAKE3_IV[i]));
        }
        state[12] = counter_lo;
        state[13] = counter_hi;
        state[14] = block_len;
        state[15] = flags;

        blake3_vec message[16];
        for (size_t i = 0; i < 16; ++i) {
            message[i] = m[i];
        }

        for (size_t round = 0; round < 7; ++round) {
            blake3_round_x8(state, message);
            if (round < 6) {
                blake3_vec permuted[16];
                for (size_t i = 0; i < 16; ++i) {
                    permuted[i] = message[BLAKE3_MSG_PERMUTATION[i]];
                }
                for (size_t i = 0; i < 16; ++i) {
                    message[i] = permuted[i];
                }
            }
        }

        for (size_t i = 0; i < 8; ++i) {
            out[i] = _mm256_xor_si256(state[i], state[i + 8]);
            out[i + 8] = _mm256_xor_si256(state[i + 8], cv[i]);
        }
    }

    blake3_vec blake3_load_message_x8(const byte_t* const* chunks, const size_t block, const size_t word) {
        alignas(32) uint32_t lanes[8];
        for (size_t lane = 0; lane < 8; ++lane) {
            lanes[lane] = endian::read_le32(chunks[lane] + block * BLAKE3_BLOCK_SIZE + word * 4);
        }
        return _mm256_load_si256(reinterpret_cast<const blake3_vec*>(lanes));
    }

    void blake3_chunk_cv_x8(const byte_t* const* chunks, const uint64_t* counters, const uint32_t* key,
                            const uint32_t base_flags, blake3_cv* output_cvs) noexcept {
        blake3_vec cv[8];
        for (size_t i = 0; i < 8; ++i) {
            cv[i] = _mm256_set1_epi32(static_cast<int>(key[i]));
        }

        alignas(32) uint32_t counter_low[8];
        alignas(32) uint32_t counter_high[8];
        for (size_t lane = 0; lane < 8; ++lane) {
            counter_low[lane] = static_cast<uint32_t>(counters[lane]);
            counter_high[lane] = static_cast<uint32_t>(counters[lane] >> 32);
        }
        const blake3_vec counter_lo = _mm256_load_si256(reinterpret_cast<const blake3_vec*>(counter_low));
        const blake3_vec counter_hi = _mm256_load_si256(reinterpret_cast<const blake3_vec*>(counter_high));
        const blake3_vec block_len = _mm256_set1_epi32(static_cast<int>(BLAKE3_BLOCK_SIZE));

        for (size_t block = 0; block < 16; ++block) {
            blake3_vec message[16];
            for (size_t word = 0; word < 16; ++word) {
                message[word] = blake3_load_message_x8(chunks, block, word);
            }

            const uint32_t flags =
                    base_flags | (block == 0 ? BLAKE3_CHUNK_START : 0U) | (block == 15 ? BLAKE3_CHUNK_END : 0U);
            blake3_vec out[16];
            blake3_compress_x8(cv, message, counter_lo, counter_hi, block_len,
                               _mm256_set1_epi32(static_cast<int>(flags)), out);
            for (size_t word = 0; word < 8; ++word) {
                cv[word] = out[word];
            }
        }

        for (size_t word = 0; word < 8; ++word) {
            alignas(32) uint32_t lanes[8];
            _mm256_store_si256(reinterpret_cast<blake3_vec*>(lanes), cv[word]);
            for (size_t lane = 0; lane < 8; ++lane) {
                output_cvs[lane].words[word] = lanes[lane];
            }
        }
    }
#endif

    blake3_cv blake3_subtree_cv(const vector<blake3_cv>& cvs, const size_t start, const size_t count,
                                const uint32_t* key, const uint32_t base_flags) noexcept {
        if (count == 1) {
            return cvs[start];
        }

        size_t split = 1;
        while (split * 2 < count) {
            split *= 2;
        }

        const blake3_cv left = blake3_subtree_cv(cvs, start, split, key, base_flags);
        const blake3_cv right = blake3_subtree_cv(cvs, start + split, count - split, key, base_flags);
        return blake3_parent_cv(left, right, key, base_flags);
    }

    blake3_output blake3_root_node(const cbyte_view data, const uint32_t* key, const uint32_t base_flags) {
        const size_t chunk_count = data.empty() ? 1 : (data.size() + BLAKE3_CHUNK_SIZE - 1) / BLAKE3_CHUNK_SIZE;
        if (chunk_count == 1) {
            return blake3_chunk_output(data, 0, key, base_flags);
        }

        const size_t full_chunks = data.size() / BLAKE3_CHUNK_SIZE;
        vector<blake3_cv> cvs;
        cvs.reserve(chunk_count);

        size_t index = 0;
#ifdef NEFORCE_SIMD_AVX2
        for (; index + 8 <= full_chunks; index += 8) {
            const byte_t* chunks[8];
            uint64_t counters[8];
            for (size_t lane = 0; lane < 8; ++lane) {
                chunks[lane] = data.data() + (index + lane) * BLAKE3_CHUNK_SIZE;
                counters[lane] = static_cast<uint64_t>(index + lane);
            }
            blake3_cv group[8]{};
            blake3_chunk_cv_x8(chunks, counters, key, base_flags, group);
            for (size_t lane = 0; lane < 8; ++lane) {
                cvs.push_back(group[lane]);
            }
        }
#endif

        for (; index < chunk_count; ++index) {
            const size_t offset = index * BLAKE3_CHUNK_SIZE;
            const size_t remaining = data.size() - offset;
            const size_t length = remaining < BLAKE3_CHUNK_SIZE ? remaining : BLAKE3_CHUNK_SIZE;
            cvs.push_back(blake3_chunk_cv(cbyte_view{data.data() + offset, length}, static_cast<uint64_t>(index), key,
                                          base_flags));
        }

        size_t split = 1;
        while (split * 2 < chunk_count) {
            split *= 2;
        }
        const blake3_cv left = blake3_subtree_cv(cvs, 0, split, key, base_flags);
        const blake3_cv right = blake3_subtree_cv(cvs, split, chunk_count - split, key, base_flags);
        return blake3_parent_output(left.words, right.words, key, base_flags);
    }

    void blake3_load_key(const cbyte_view key, uint32_t* words) noexcept {
        for (size_t i = 0; i < 8; ++i) {
            words[i] = endian::read_le32(key.data() + i * 4);
        }
    }

    byte_vector blake3_hash_internal(const cbyte_view data, const uint32_t* key, const uint32_t base_flags,
                                     const size_t length) {
        const blake3_output root = blake3_root_node(data, key, base_flags);
        byte_vector result(length);
        if (length != 0) {
            blake3_root_bytes(root, result.data(), length);
        }
        return result;
    }

    string blake3_to_hex(const byte_vector& bytes) {
        constexpr char hex_chars[] = "0123456789abcdef";
        string result;
        result.reserve(bytes.size() * 2);
        for (const byte_t byte: bytes) {
            result.push_back(hex_chars[byte >> 4]);
            result.push_back(hex_chars[byte & 0x0F]);
        }
        return result;
    }
} // namespace


byte_vector BLAKE3::hash(const cbyte_view data) { return blake3_hash_internal(data, BLAKE3_IV, 0, 32); }

string BLAKE3::hash_hex(const cbyte_view data) { return blake3_to_hex(hash(data)); }

byte_vector BLAKE3::hash_xof(const cbyte_view data, const size_t length) {
    return blake3_hash_internal(data, BLAKE3_IV, 0, length);
}

byte_vector BLAKE3::keyed_hash(const cbyte_view key, const cbyte_view data) {
    if (key.size() != 32) {
        NEFORCE_THROW_EXCEPTION(value_exception("BLAKE3 key must be exactly 32 bytes"));
    }
    uint32_t key_words[8];
    blake3_load_key(key, key_words);
    return blake3_hash_internal(data, key_words, BLAKE3_KEYED_HASH, 32);
}

byte_vector BLAKE3::derive_key(const cbyte_view context, const cbyte_view key_material, const size_t length) {
    const byte_vector context_key = blake3_hash_internal(context, BLAKE3_IV, BLAKE3_DERIVE_KEY_CONTEXT, 32);
    uint32_t context_words[8];
    blake3_load_key(context_key.view(), context_words);
    return blake3_hash_internal(key_material, context_words, BLAKE3_DERIVE_KEY_MATERIAL, length);
}

NEFORCE_END_NAMESPACE__
