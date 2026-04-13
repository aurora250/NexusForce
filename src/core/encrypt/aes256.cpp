#include <NeForce/core/encrypt/aes256.hpp>
#include <NeForce/core/memory/endian.hpp>
#include <NeForce/core/utility/hexadecimal.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    constexpr byte_t AES256_sbox[256] = {
            0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76, 0xca, 0x82,
            0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0, 0xb7, 0xfd, 0x93, 0x26,
            0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15, 0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96,
            0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75, 0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0,
            0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84, 0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb,
            0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf, 0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f,
            0x50, 0x3c, 0x9f, 0xa8, 0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff,
            0xf3, 0xd2, 0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
            0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb, 0xe0, 0x32,
            0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79, 0xe7, 0xc8, 0x37, 0x6d,
            0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08, 0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6,
            0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a, 0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e,
            0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e, 0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e,
            0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf, 0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f,
            0xb0, 0x54, 0xbb, 0x16};

    constexpr byte_t AES256_inv_sbox[256] = {
            0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb, 0x7c, 0xe3,
            0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb, 0x54, 0x7b, 0x94, 0x32,
            0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e, 0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9,
            0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25, 0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16,
            0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92, 0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15,
            0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84, 0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05,
            0xb8, 0xb3, 0x45, 0x06, 0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13,
            0x8a, 0x6b, 0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
            0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e, 0x47, 0xf1,
            0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b, 0xfc, 0x56, 0x3e, 0x4b,
            0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4, 0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07,
            0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f, 0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d,
            0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef, 0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb,
            0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61, 0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63,
            0x55, 0x21, 0x0c, 0x7d};

    constexpr byte_t AES256_rcon[15] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80,
                                        0x1b, 0x36, 0x6c, 0xd8, 0xab, 0x4d, 0x9a};

    constexpr void AES256_sub_bytes(byte_t state[16]) {
        for (int i = 0; i < 16; ++i) {
            state[i] = AES256_sbox[state[i]];
        }
    }

    constexpr void AES256_inv_sub_bytes(byte_t state[16]) {
        for (int i = 0; i < 16; ++i) {
            state[i] = AES256_inv_sbox[state[i]];
        }
    }

    constexpr void AES256_add_round_key(byte_t state[16], const byte_t* round_key) {
        for (int i = 0; i < 16; ++i) {
            state[i] ^= round_key[i];
        }
    }

    constexpr byte_t AES256_gf_mult(byte_t a, byte_t b) {
        byte_t result = 0;
        for (int i = 0; i < 8; ++i) {
            if ((b & 1) == 1) {
                result ^= a;
            }
            const byte_t hi_bit_set = (a & 0x80);
            a <<= 1;
            if (hi_bit_set == 0x80) {
                a ^= 0x1b;
            }
            b >>= 1;
        }
        return result;
    }

    constexpr void AES256_key_expansion(const byte_t* key, byte_t* expanded_key) {
        memory_copy(expanded_key, key, 32);
        for (int i = 8; i < 60; ++i) {
            byte_t temp[4];
            memory_copy(temp, expanded_key + static_cast<ptrdiff_t>((i - 1) * 4), 4);

            if (i % 8 == 0) {
                const byte_t t = temp[0];
                temp[0] = temp[1];
                temp[1] = temp[2];
                temp[2] = temp[3];
                temp[3] = t;
                for (byte_t& j: temp) {
                    j = AES256_sbox[j];
                }
                temp[0] ^= AES256_rcon[(i / 8) - 1];
            } else if (i % 8 == 4) {
                for (byte_t& j: temp) {
                    j = AES256_sbox[j];
                }
            }

            for (int j = 0; j < 4; ++j) {
                expanded_key[(i * 4) + j] = expanded_key[((i - 8) * 4) + j] ^ temp[j];
            }
        }
    }

    constexpr void AES256_shift_rows(byte_t state[16]) {
        byte_t temp = state[1];
        state[1] = state[5];
        state[5] = state[9];
        state[9] = state[13];
        state[13] = temp;

        temp = state[2];
        state[2] = state[10];
        state[10] = temp;
        temp = state[6];
        state[6] = state[14];
        state[14] = temp;

        temp = state[3];
        state[3] = state[15];
        state[15] = state[11];
        state[11] = state[7];
        state[7] = temp;
    }

    constexpr void AES256_inv_shift_rows(byte_t state[16]) {
        byte_t temp = state[13];
        state[13] = state[9];
        state[9] = state[5];
        state[5] = state[1];
        state[1] = temp;

        temp = state[2];
        state[2] = state[10];
        state[10] = temp;
        temp = state[6];
        state[6] = state[14];
        state[14] = temp;

        temp = state[7];
        state[7] = state[11];
        state[11] = state[15];
        state[15] = state[3];
        state[3] = temp;
    }

    constexpr void AES256_mix_columns(byte_t state[16]) {
        for (ptrdiff_t c = 0; c < 4; ++c) {
            const byte_t s0 = state[c * 4];
            const byte_t s1 = state[c * 4 + 1];
            const byte_t s2 = state[c * 4 + 2];
            const byte_t s3 = state[c * 4 + 3];

            state[c * 4] = AES256_gf_mult(0x02, s0) ^ AES256_gf_mult(0x03, s1) ^ s2 ^ s3;
            state[c * 4 + 1] = s0 ^ AES256_gf_mult(0x02, s1) ^ AES256_gf_mult(0x03, s2) ^ s3;
            state[c * 4 + 2] = s0 ^ s1 ^ AES256_gf_mult(0x02, s2) ^ AES256_gf_mult(0x03, s3);
            state[c * 4 + 3] = AES256_gf_mult(0x03, s0) ^ s1 ^ s2 ^ AES256_gf_mult(0x02, s3);
        }
    }

    constexpr void AES256_inv_mix_columns(byte_t state[16]) {
        for (ptrdiff_t c = 0; c < 4; ++c) {
            const byte_t s0 = state[c * 4];
            const byte_t s1 = state[c * 4 + 1];
            const byte_t s2 = state[c * 4 + 2];
            const byte_t s3 = state[c * 4 + 3];

            state[c * 4] = AES256_gf_mult(0x0e, s0) ^ AES256_gf_mult(0x0b, s1) ^ AES256_gf_mult(0x0d, s2) ^
                           AES256_gf_mult(0x09, s3);
            state[c * 4 + 1] = AES256_gf_mult(0x09, s0) ^ AES256_gf_mult(0x0e, s1) ^ AES256_gf_mult(0x0b, s2) ^
                               AES256_gf_mult(0x0d, s3);
            state[c * 4 + 2] = AES256_gf_mult(0x0d, s0) ^ AES256_gf_mult(0x09, s1) ^ AES256_gf_mult(0x0e, s2) ^
                               AES256_gf_mult(0x0b, s3);
            state[c * 4 + 3] = AES256_gf_mult(0x0b, s0) ^ AES256_gf_mult(0x0d, s1) ^ AES256_gf_mult(0x09, s2) ^
                               AES256_gf_mult(0x0e, s3);
        }
    }

    constexpr void encrypt_block(byte_t block[16], const byte_t* expanded_key) {
        AES256_add_round_key(block, expanded_key);

        for (int round = 1; round < 14; ++round) {
            AES256_sub_bytes(block);
            AES256_shift_rows(block);
            AES256_mix_columns(block);
            AES256_add_round_key(block, expanded_key + static_cast<ptrdiff_t>(round * 16));
        }

        AES256_sub_bytes(block);
        AES256_shift_rows(block);
        AES256_add_round_key(block, expanded_key + static_cast<ptrdiff_t>(14 * 16));
    }

    constexpr void decrypt_block(byte_t block[16], const byte_t* expanded_key) {
        AES256_add_round_key(block, expanded_key + static_cast<ptrdiff_t>(14 * 16));

        for (int round = 13; round >= 1; --round) {
            AES256_inv_shift_rows(block);
            AES256_inv_sub_bytes(block);
            AES256_add_round_key(block, expanded_key + static_cast<ptrdiff_t>(round * 16));
            AES256_inv_mix_columns(block);
        }

        AES256_inv_shift_rows(block);
        AES256_inv_sub_bytes(block);
        AES256_add_round_key(block, expanded_key);
    }

    constexpr void xor_block(byte_t* dst, const byte_t* src) {
        for (int i = 0; i < 16; ++i) {
            dst[i] ^= src[i];
        }
    }

    int secure_compare(const byte_t* a, const byte_t* b, size_t len) {
        volatile byte_t diff = 0;
        for (size_t i = 0; i < len; ++i) {
            diff |= a[i] ^ b[i];
        }
        return diff != 0;
    }

    struct ghash_context {
        uint64_t H_hi;
        uint64_t H_lo;
        uint64_t table_hi[256];
        uint64_t table_lo[256];
        uint64_t red_hi[256];
        uint64_t red_lo[256];
    };

    constexpr uint64_t R_hi = 0xE100000000000000ULL;
    constexpr uint64_t R_lo = 0;

    constexpr void ghash_init(ghash_context& ctx, const byte_t H[16]) {
        ctx.H_hi = endian::read_be64(H);
        ctx.H_lo = endian::read_be64(H + 8);

        ctx.table_hi[0] = 0;
        ctx.table_lo[0] = 0;
        ctx.table_hi[1] = ctx.H_hi;
        ctx.table_lo[1] = ctx.H_lo;

        for (int i = 2; i < 256; ++i) {
            if (i % 2 == 0) {
                uint64_t hi = ctx.table_hi[i / 2];
                uint64_t lo = ctx.table_lo[i / 2];
                bool carry = (hi >> 63) & 1;
                hi = (hi << 1) | (lo >> 63);
                lo = lo << 1;
                if (carry) {
                    hi ^= R_hi;
                    lo ^= R_lo;
                }
                ctx.table_hi[i] = hi;
                ctx.table_lo[i] = lo;
            } else {
                ctx.table_hi[i] = ctx.table_hi[i - 1] ^ ctx.H_hi;
                ctx.table_lo[i] = ctx.table_lo[i - 1] ^ ctx.H_lo;
            }
        }

        for (int i = 0; i < 256; ++i) {
            uint64_t rh = 0, rl = 0;
            uint64_t vh = ctx.H_hi, vl = ctx.H_lo;
            for (int bit = 0; bit < 8; ++bit) {
                if ((i >> bit) & 1) {
                    rh ^= vh;
                    rl ^= vl;
                }
                bool c = (vh >> 63) & 1;
                vh = (vh << 1) | (vl >> 63);
                vl = vl << 1;
                if (c) {
                    vh ^= R_hi;
                    vl ^= R_lo;
                }
            }
            ctx.red_hi[i] = rh;
            ctx.red_lo[i] = rl;
        }
    }

    constexpr void ghash_update(const ghash_context& ctx, uint64_t& state_hi, uint64_t& state_lo, const byte_t* data,
                                size_t len) {
        while (len >= 16) {
            state_hi ^= endian::read_be64(data);
            state_lo ^= endian::read_be64(data + 8);

            byte_t bytes[16];
            endian::write_be64(bytes, state_hi);
            endian::write_be64(bytes + 8, state_lo);

            uint64_t new_hi = 0, new_lo = 0;
            for (int i = 0; i < 16; ++i) {
                const uint8_t idx = bytes[i];

                const uint64_t carry = new_hi >> 56;
                new_hi = (new_hi << 8) | (new_lo >> 56);
                new_lo = new_lo << 8;

                new_hi ^= ctx.red_hi[carry];
                new_lo ^= ctx.red_lo[carry];

                new_hi ^= ctx.table_hi[idx];
                new_lo ^= ctx.table_lo[idx];
            }
            state_hi = new_hi;
            state_lo = new_lo;

            data += 16;
            len -= 16;
        }

        if (len > 0) {
            byte_t padded[16] = {0};
            memory_copy(padded, data, len);
            ghash_update(ctx, state_hi, state_lo, padded, 16);
        }
    }

    constexpr void ghash_final(const ghash_context& ctx, uint64_t state_hi, uint64_t state_lo, size_t a_len,
                               size_t c_len, byte_t out_tag[16]) {
        byte_t len_block[16] = {0};
        endian::write_be64(len_block, a_len * 8);
        endian::write_be64(len_block + 8, c_len * 8);

        ghash_update(ctx, state_hi, state_lo, len_block, 16);

        endian::write_be64(out_tag, state_hi);
        endian::write_be64(out_tag + 8, state_lo);
    }

    constexpr void inc32(byte_t* counter) {
        const uint32_t n = endian::read_be32(counter + 12);
        endian::write_be32(counter + 12, n + 1);
    }
} // namespace


byte_vector AES256::encrypt_ecb(const cbyte_view data, const cbyte_view key) {
    if (key.size() != 32) {
        NEFORCE_THROW_EXCEPTION(value_exception("AES-256 requires 32-byte key"));
    }
    if (data.size() % 16 != 0) {
        NEFORCE_THROW_EXCEPTION(value_exception("Data size must be multiple of 16 bytes"));
    }

    byte_t expanded_key[240];
    AES256_key_expansion(key.data(), expanded_key);
    byte_vector result;
    result.reserve(data.size());

    for (size_t i = 0; i < data.size(); i += 16) {
        byte_t block[16];
        memory_copy(block, data.data() + i, 16);
        encrypt_block(block, expanded_key);
        for (const byte_t j: block) {
            result.push_back(j);
        }
    }
    return result;
}

byte_vector AES256::decrypt_ecb(const cbyte_view data, const cbyte_view key) {
    if (key.size() != 32) {
        NEFORCE_THROW_EXCEPTION(value_exception("AES-256 requires 32-byte key"));
    }
    if (data.size() % 16 != 0) {
        NEFORCE_THROW_EXCEPTION(value_exception("Data size must be multiple of 16 bytes"));
    }

    byte_t expanded_key[240];
    AES256_key_expansion(key.data(), expanded_key);
    byte_vector result;
    result.reserve(data.size());

    for (size_t i = 0; i < data.size(); i += 16) {
        byte_t block[16];
        memory_copy(block, data.data() + i, 16);
        decrypt_block(block, expanded_key);
        for (const byte_t j: block) {
            result.push_back(j);
        }
    }
    return result;
}

byte_vector AES256::encrypt_ecb_pkcs7(const cbyte_view data, const cbyte_view key) {
    byte_vector byte_data{data.data(), data.size()};
    const byte_t padding = 16 - (byte_data.size() % 16);
    for (int i = 0; i < padding; ++i) {
        byte_data.push_back(padding);
    }
    return encrypt_ecb(byte_data.view(), key);
}

byte_vector AES256::decrypt_ecb_pkcs7(const cbyte_view data, const cbyte_view key) {
    byte_vector decrypted = decrypt_ecb(data, key);
    if (decrypted.empty()) {
        return decrypted;
    }

    const byte_t padding_len = decrypted.back();
    const size_t original_size = decrypted.size();

    if (padding_len == 0 || padding_len > 16 || padding_len > original_size) {
        NEFORCE_THROW_EXCEPTION(value_exception("Invalid PKCS7 padding (invalid length)"));
    }

    const size_t new_size = original_size - padding_len;
    for (size_t i = new_size; i < original_size; ++i) {
        if (decrypted[i] != padding_len) {
            NEFORCE_THROW_EXCEPTION(value_exception("Invalid PKCS7 padding (mismatched value)"));
        }
    }

    decrypted.resize(new_size);
    return decrypted;
}

string AES256::encrypt_ecb_hex(const string_view data, const string_view key_hex) {
    if (key_hex.size() != 64) {
        NEFORCE_THROW_EXCEPTION(value_exception("AES-256 key must be 64 hex characters (32 bytes)"));
    }

    byte_vector key_bytes;
    key_bytes.reserve(32);
    for (size_t i = 0; i + 2 <= key_hex.size(); i += 2) {
        const auto hex_val = hexadecimal::parse(key_hex.substr(i, 2));
        key_bytes.push_back(static_cast<byte_t>(hex_val.value()));
    }

    const byte_vector data_bytes(reinterpret_cast<const byte_t*>(data.data()), data.size());
    byte_vector encrypted = encrypt_ecb_pkcs7(data_bytes.view(), key_bytes.view());

    string result;
    for (const byte_t byte: encrypted) {
        hexadecimal hex_byte(byte);
        result += format("{:02x}", hex_byte);
    }
    return result;
}

string AES256::decrypt_ecb_hex(const string_view encrypted_hex, const string_view key_hex) {
    if (key_hex.size() != 64) {
        NEFORCE_THROW_EXCEPTION(value_exception("AES-256 key must be 64 hex characters (32 bytes)"));
    }

    byte_vector key_bytes;
    for (size_t i = 0; i + 2 <= key_hex.size(); i += 2) {
        const auto hex_val = hexadecimal::parse(key_hex.substr(i, 2));
        key_bytes.push_back(static_cast<byte_t>(hex_val.value()));
    }

    byte_vector encrypted_bytes;
    for (size_t i = 0; i + 2 <= encrypted_hex.size(); i += 2) {
        const auto hex_val = hexadecimal::parse(encrypted_hex.substr(i, 2));
        encrypted_bytes.push_back(static_cast<byte_t>(hex_val.value()));
    }

    byte_vector decrypted = decrypt_ecb_pkcs7(encrypted_bytes.view(), key_bytes.view());
    return {reinterpret_cast<const char*>(decrypted.data()), decrypted.size()};
}

byte_vector AES256::encrypt_cbc(cbyte_view data, cbyte_view key, cbyte_view iv) {
    if (key.size() != 32) {
        NEFORCE_THROW_EXCEPTION(value_exception("AES-256 requires 32-byte key"));
    }
    if (iv.size() != 16) {
        NEFORCE_THROW_EXCEPTION(value_exception("CBC mode requires 16-byte IV"));
    }
    if (data.size() % 16 != 0) {
        NEFORCE_THROW_EXCEPTION(value_exception("Data size must be multiple of 16 bytes for CBC no padding"));
    }

    byte_t expanded_key[240];
    AES256_key_expansion(key.data(), expanded_key);

    byte_vector result;
    result.reserve(data.size());

    byte_t prev[16];
    memory_copy(prev, iv.data(), 16);

    for (size_t i = 0; i < data.size(); i += 16) {
        byte_t block[16];
        memory_copy(block, data.data() + i, 16);
        xor_block(block, prev);
        encrypt_block(block, expanded_key);
        memory_copy(prev, block, 16);
        result.insert(result.end(), block, block + 16);
    }
    return result;
}

byte_vector AES256::decrypt_cbc(cbyte_view data, cbyte_view key, cbyte_view iv) {
    if (key.size() != 32) {
        NEFORCE_THROW_EXCEPTION(value_exception("AES-256 requires 32-byte key"));
    }
    if (iv.size() != 16) {
        NEFORCE_THROW_EXCEPTION(value_exception("CBC mode requires 16-byte IV"));
    }
    if (data.size() % 16 != 0) {
        NEFORCE_THROW_EXCEPTION(value_exception("Data size must be multiple of 16 bytes"));
    }

    byte_t expanded_key[240];
    AES256_key_expansion(key.data(), expanded_key);

    byte_vector result;
    result.reserve(data.size());

    byte_t prev[16];
    memory_copy(prev, iv.data(), 16);

    for (size_t i = 0; i < data.size(); i += 16) {
        byte_t block[16];
        memory_copy(block, data.data() + i, 16);
        byte_t cipher[16];
        memory_copy(cipher, block, 16);

        decrypt_block(block, expanded_key);
        xor_block(block, prev);
        result.insert(result.end(), block, block + 16);

        memory_copy(prev, cipher, 16);
    }
    return result;
}

byte_vector AES256::encrypt_cbc_pkcs7(cbyte_view data, cbyte_view key, cbyte_view iv) {
    byte_vector padded{data.data(), data.size()};
    const byte_t padding = 16 - (padded.size() % 16);
    for (int i = 0; i < padding; ++i) {
        padded.push_back(padding);
    }
    return encrypt_cbc(padded.view(), key, iv);
}

byte_vector AES256::decrypt_cbc_pkcs7(cbyte_view data, cbyte_view key, cbyte_view iv) {
    byte_vector decrypted = decrypt_cbc(data, key, iv);
    if (decrypted.empty()) {
        return decrypted;
    }

    const byte_t padding_len = decrypted.back();
    const size_t original_size = decrypted.size();

    if (padding_len == 0 || padding_len > 16 || padding_len > original_size) {
        NEFORCE_THROW_EXCEPTION(value_exception("Invalid PKCS7 padding"));
    }
    const size_t new_size = original_size - padding_len;
    for (size_t i = new_size; i < original_size; ++i) {
        if (decrypted[i] != padding_len) {
            NEFORCE_THROW_EXCEPTION(value_exception("Invalid PKCS7 padding"));
        }
    }
    decrypted.resize(new_size);
    return decrypted;
}

byte_vector AES256::encrypt_gcm(cbyte_view data, cbyte_view key, cbyte_view iv, cbyte_view aad, byte_t* tag,
                                size_t tag_len) {
    if (key.size() != 32) {
        NEFORCE_THROW_EXCEPTION(value_exception("AES-256 requires 32-byte key"));
    }
    if (iv.empty()) {
        NEFORCE_THROW_EXCEPTION(value_exception("GCM IV cannot be empty"));
    }
    if (tag == nullptr) {
        NEFORCE_THROW_EXCEPTION(value_exception("GCM tag output pointer cannot be null"));
    }
    if (tag_len < 12 || tag_len > 16) {
        NEFORCE_THROW_EXCEPTION(value_exception("GCM tag length must be between 12 and 16 bytes"));
    }

    byte_t expanded_key[240];
    AES256_key_expansion(key.data(), expanded_key);

    byte_t H[16] = {0};
    encrypt_block(H, expanded_key);
    ghash_context gh_ctx;
    ghash_init(gh_ctx, H);

    byte_t J0[16] = {0};
    if (iv.size() == 12) {
        memory_copy(J0, iv.data(), 12);
        J0[15] = 1;
    } else {
        uint64_t state_hi = 0, state_lo = 0;
        ghash_update(gh_ctx, state_hi, state_lo, iv.data(), iv.size());
        ghash_final(gh_ctx, state_hi, state_lo, 0, iv.size(), J0);
    }

    byte_vector ciphertext;
    ciphertext.reserve(data.size());

    byte_t counter[16];
    memory_copy(counter, J0, 16);
    inc32(counter);

    for (size_t i = 0; i < data.size(); i += 16) {
        byte_t keystream[16];
        memory_copy(keystream, counter, 16);
        encrypt_block(keystream, expanded_key);

        const size_t block_len = min<size_t>(16, data.size() - i);
        for (size_t j = 0; j < block_len; ++j) {
            ciphertext.push_back(data[i + j] ^ keystream[j]);
        }
        inc32(counter);
    }

    uint64_t state_hi = 0, state_lo = 0;
    ghash_update(gh_ctx, state_hi, state_lo, aad.data(), aad.size());
    ghash_update(gh_ctx, state_hi, state_lo, ciphertext.data(), ciphertext.size());

    byte_t full_tag[16];
    ghash_final(gh_ctx, state_hi, state_lo, aad.size(), ciphertext.size(), full_tag);

    byte_t enc_J0[16];
    memory_copy(enc_J0, J0, 16);
    encrypt_block(enc_J0, expanded_key);
    for (int i = 0; i < 16; ++i) {
        full_tag[i] ^= enc_J0[i];
    }

    memory_copy(tag, full_tag, tag_len);
    return ciphertext;
}

byte_vector AES256::decrypt_gcm(cbyte_view data, cbyte_view key, cbyte_view iv, cbyte_view aad, cbyte_view tag,
                                size_t tag_len) {
    if (key.size() != 32) {
        NEFORCE_THROW_EXCEPTION(value_exception("AES-256 requires 32-byte key"));
    }
    if (iv.empty()) {
        NEFORCE_THROW_EXCEPTION(value_exception("GCM IV cannot be empty"));
    }
    if (tag_len < 12 || tag_len > 16) {
        NEFORCE_THROW_EXCEPTION(value_exception("GCM tag length must be between 12 and 16 bytes"));
    }
    if (tag.size() < tag_len) {
        NEFORCE_THROW_EXCEPTION(value_exception("Tag size mismatch"));
    }

    byte_t expanded_key[240];
    AES256_key_expansion(key.data(), expanded_key);

    byte_t H[16] = {0};
    encrypt_block(H, expanded_key);
    ghash_context gh_ctx;
    ghash_init(gh_ctx, H);

    byte_t J0[16] = {0};
    if (iv.size() == 12) {
        memory_copy(J0, iv.data(), 12);
        J0[15] = 1;
    } else {
        uint64_t state_hi = 0, state_lo = 0;
        ghash_update(gh_ctx, state_hi, state_lo, iv.data(), iv.size());
        ghash_final(gh_ctx, state_hi, state_lo, 0, iv.size(), J0);
    }

    uint64_t state_hi = 0, state_lo = 0;
    ghash_update(gh_ctx, state_hi, state_lo, aad.data(), aad.size());
    ghash_update(gh_ctx, state_hi, state_lo, data.data(), data.size());

    byte_t expected_tag[16];
    ghash_final(gh_ctx, state_hi, state_lo, aad.size(), data.size(), expected_tag);

    byte_t enc_J0[16];
    memory_copy(enc_J0, J0, 16);
    encrypt_block(enc_J0, expanded_key);
    for (int i = 0; i < 16; ++i) {
        expected_tag[i] ^= enc_J0[i];
    }

    if (secure_compare(expected_tag, tag.data(), tag_len)) {
        NEFORCE_THROW_EXCEPTION(value_exception("GCM authentication failed"));
    }

    byte_vector plaintext;
    plaintext.reserve(data.size());

    byte_t counter[16];
    memory_copy(counter, J0, 16);
    inc32(counter);

    for (size_t i = 0; i < data.size(); i += 16) {
        byte_t keystream[16];
        memory_copy(keystream, counter, 16);
        encrypt_block(keystream, expanded_key);

        const size_t block_len = min<size_t>(16, data.size() - i);
        for (size_t j = 0; j < block_len; ++j) {
            plaintext.push_back(data[i + j] ^ keystream[j]);
        }
        inc32(counter);
    }

    return plaintext;
}

NEFORCE_END_NAMESPACE__
