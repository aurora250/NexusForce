#include <MSTL/core/memory/hexadecimal.hpp>
#include <MSTL/core/encrypt/encrypt.hpp>
MSTL_BEGIN_NAMESPACE__

bstring XOR::encrypt(const bstring_view data, const bstring_view key) {
    if (key.empty()) throw_exception(value_exception("Key cannot be empty"));

    bstring result;
    result.reserve(data.size());

    for (size_t i = 0; i < data.size(); ++i) {
        result.push_back(data[i] ^ key[i % key.size()]);
    }
    return result;
}


string base64::encode(const bstring_view data) {
    string result;
    size_t i = 0;

    while (i + 2 < data.size()) {
        const uint32_t val = (data[i] << 16) | (data[i + 1] << 8) | data[i + 2];
        result.push_back(chars[(val >> 18) & 0x3F]);
        result.push_back(chars[(val >> 12) & 0x3F]);
        result.push_back(chars[(val >> 6) & 0x3F]);
        result.push_back(chars[val & 0x3F]);
        i += 3;
    }

    if (i < data.size()) {
        uint32_t val = data[i] << 16;
        if (i + 1 < data.size()) val |= data[i + 1] << 8;

        result.push_back(chars[(val >> 18) & 0x3F]);
        result.push_back(chars[(val >> 12) & 0x3F]);
        result.push_back(i + 1 < data.size() ? chars[(val >> 6) & 0x3F] : '=');
        result.push_back('=');
    }
    return result;
}

bstring base64::decode(const string_view data) {
    bstring result;
    size_t i = 0;

    while (i + 3 < data.size()) {
        const int a = char_to_index(data[i]);
        const int b = char_to_index(data[i + 1]);
        const int c = data[i + 2] == '=' ? 0 : char_to_index(data[i + 2]);
        const int d = data[i + 3] == '=' ? 0 : char_to_index(data[i + 3]);

        if (a < 0 || b < 0) throw_exception(value_exception("Invalid Base64 character"));

        const uint32_t val = (a << 18) | (b << 12) | (c << 6) | d;
        result.push_back((val >> 16) & 0xFF);
        if (data[i + 2] != '=') result.push_back((val >> 8) & 0xFF);
        if (data[i + 3] != '=') result.push_back(val & 0xFF);

        i += 4;
    }
    return result;
}


bstring MD5::hash(const bstring_view data) {
    bstring byte_data{data};
    const uint64_t original_len = byte_data.size();
    byte_data.push_back(0x80);
    while ((byte_data.size() % 64) != 56) {
        byte_data.push_back(0);
    }

    const uint64_t bit_len = original_len * 8;
    for (int i = 0; i < 8; ++i) {
        byte_data.push_back((bit_len >> (i * 8)) & 0xFF);
    }

    uint32_t h0 = 0x67452301;
    uint32_t h1 = 0xefcdab89;
    uint32_t h2 = 0x98badcfe;
    uint32_t h3 = 0x10325476;

    for (size_t chunk_start = 0; chunk_start < byte_data.size(); chunk_start += 64) {
        uint32_t w[16];
        for (int i = 0; i < 16; ++i) {
            w[i] = (byte_data[chunk_start + i * 4]) |
                   (byte_data[chunk_start + i * 4 + 1] << 8) |
                   (byte_data[chunk_start + i * 4 + 2] << 16) |
                   (byte_data[chunk_start + i * 4 + 3] << 24);
        }

        uint32_t a = h0, b = h1, c = h2, d = h3;
        for (int i = 0; i < 64; ++i) {
            uint32_t f, g;
            if (i < 16) {
                f = F(b, c, d);
                g = i;
            } else if (i < 32) {
                f = G(b, c, d);
                g = (5 * i + 1) % 16;
            } else if (i < 48) {
                f = H(b, c, d);
                g = (3 * i + 5) % 16;
            } else {
                f = I(b, c, d);
                g = (7 * i) % 16;
            }

            f = f + a + K[i] + w[g];
            a = d;
            d = c;
            c = b;
            b = b + rotleft(f, S[i]);
        }
        h0 += a;
        h1 += b;
        h2 += c;
        h3 += d;
    }

    bstring result(16);
    for (int i = 0; i < 4; ++i) {
        result[i] = (h0 >> (i * 8)) & 0xFF;
        result[i + 4] = (h1 >> (i * 8)) & 0xFF;
        result[i + 8] = (h2 >> (i * 8)) & 0xFF;
        result[i + 12] = (h3 >> (i * 8)) & 0xFF;
    }
    return result;
}

string MD5::hash_hex(const bstring_view data) {
    bstring hash_result = hash(data);
    string hex_result;
    for (const byte_t byte : hash_result) {
        hexadecimal hex_byte(byte);
        hex_result += format("{02x}", hex_byte);
    }
    return hex_result;
}

constexpr uint32_t MD5::S[64];
constexpr uint32_t MD5::K[64];


bstring SHA1::hash(const bstring_view data) {
    bstring byte_data{data};
    const uint64_t original_len = byte_data.size();
    byte_data.push_back(0x80);

    while ((byte_data.size() % 64) != 56) {
        byte_data.push_back(0);
    }
    const uint64_t bit_len = original_len * 8;
    for (int i = 7; i >= 0; --i) {
        byte_data.push_back((bit_len >> (i * 8)) & 0xFF);
    }

    uint32_t h0 = 0x67452301;
    uint32_t h1 = 0xEFCDAB89;
    uint32_t h2 = 0x98BADCFE;
    uint32_t h3 = 0x10325476;
    uint32_t h4 = 0xC3D2E1F0;

    for (size_t chunk_start = 0; chunk_start < byte_data.size(); chunk_start += 64) {
        uint32_t w[80];

        for (int i = 0; i < 16; ++i) {
            w[i] = (byte_data[chunk_start + i * 4] << 24) |
                   (byte_data[chunk_start + i * 4 + 1] << 16) |
                   (byte_data[chunk_start + i * 4 + 2] << 8) |
                   (byte_data[chunk_start + i * 4 + 3]);
        }

        for (int i = 16; i < 80; ++i) {
            w[i] = rotleft(w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16], 1);
        }

        uint32_t a = h0, b = h1, c = h2, d = h3, e = h4;

        for (int i = 0; i < 80; ++i) {
            uint32_t f, k;

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

            const uint32_t temp = rotleft(a, 5) + f + e + k + w[i];
            e = d;
            d = c;
            c = rotleft(b, 30);
            b = a;
            a = temp;
        }

        h0 += a;
        h1 += b;
        h2 += c;
        h3 += d;
        h4 += e;
    }

    bstring result(20);
    for (int i = 0; i < 4; ++i) {
        result[i] = (h0 >> (24 - i * 8)) & 0xFF;
        result[i + 4] = (h1 >> (24 - i * 8)) & 0xFF;
        result[i + 8] = (h2 >> (24 - i * 8)) & 0xFF;
        result[i + 12] = (h3 >> (24 - i * 8)) & 0xFF;
        result[i + 16] = (h4 >> (24 - i * 8)) & 0xFF;
    }
    return result;
}

string SHA1::hash_hex(const bstring_view data) {
    bstring hash_result = hash(data);
    string hex_result;
    for (const byte_t byte : hash_result) {
        hexadecimal hex_byte(byte);
        hex_result += format("{02x}", hex_byte);
    }
    return hex_result;
}


bstring SHA256::hash(const bstring_view data) {
    bstring byte_data{data};
    const uint64_t original_len = byte_data.size();
    byte_data.push_back(0x80);

    while ((byte_data.size() % 64) != 56) {
        byte_data.push_back(0);
    }
    const uint64_t bit_len = original_len * 8;
    for (int i = 7; i >= 0; --i) {
        byte_data.push_back((bit_len >> (i * 8)) & 0xFF);
    }

    uint32_t h[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };

    for (size_t chunk_start = 0; chunk_start < byte_data.size(); chunk_start += 64) {
        uint32_t w[64];
        for (int i = 0; i < 16; ++i) {
            w[i] = (byte_data[chunk_start + i * 4] << 24) |
                   (byte_data[chunk_start + i * 4 + 1] << 16) |
                   (byte_data[chunk_start + i * 4 + 2] << 8) |
                   (byte_data[chunk_start + i * 4 + 3]);
        }

        for (int i = 16; i < 64; ++i) {
            w[i] = gamma1(w[i-2]) + w[i-7] + gamma0(w[i-15]) + w[i-16];
        }
        uint32_t a = h[0], b = h[1], c = h[2], d = h[3];
        uint32_t e = h[4], f = h[5], g = h[6], h_val = h[7];

        for (int i = 0; i < 64; ++i) {
            const uint32_t t1 = h_val + sig1(e) + ch(e, f, g) + K[i] + w[i];
            const uint32_t t2 = sig0(a) + maj(a, b, c);

            h_val = g;
            g = f;
            f = e;
            e = d + t1;
            d = c;
            c = b;
            b = a;
            a = t1 + t2;
        }
        h[0] += a; h[1] += b; h[2] += c; h[3] += d;
        h[4] += e; h[5] += f; h[6] += g; h[7] += h_val;
    }

    bstring result(32);
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 4; ++j) {
            result[i * 4 + j] = (h[i] >> (24 - j * 8)) & 0xFF;
        }
    }
    return result;
}

string SHA256::hash_hex(const bstring_view data) {
    bstring hash_result = hash(data);
    string hex_result;
    for (const byte_t byte : hash_result) {
        hexadecimal hex_byte(byte);
        hex_result += format("{02x}", hex_byte);
    }
    return hex_result;
}

constexpr uint32_t SHA256::K[64];


byte_t AES256::gf_mult(byte_t a, byte_t b) {
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

void AES256::key_expansion(const byte_t* key, byte_t* expanded_key) {
    memory_copy(expanded_key, key, 32);
    for (int i = 8; i < 60; ++i) {
        byte_t temp[4];
        memory_copy(temp, expanded_key + (i - 1) * 4, 4);

        if (i % 8 == 0) {
            const byte_t t = temp[0];
            temp[0] = temp[1];
            temp[1] = temp[2];
            temp[2] = temp[3];
            temp[3] = t;
            for (int j = 0; j < 4; ++j) {
                temp[j] = sbox[temp[j]];
            }
            temp[0] ^= rcon[i / 8 - 1];
        } else if (i % 8 == 4) {
            for (int j = 0; j < 4; ++j) {
                temp[j] = sbox[temp[j]];
            }
        }

        for (int j = 0; j < 4; ++j) {
            expanded_key[i * 4 + j] = expanded_key[(i - 8) * 4 + j] ^ temp[j];
        }
    }
}

void AES256::shift_rows(byte_t state[16]) {
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

void AES256::inv_shift_rows(byte_t state[16]) {
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

void AES256::mix_columns(byte_t state[16]) {
    for (int c = 0; c < 4; ++c) {
        const byte_t s0 = state[c * 4];
        const byte_t s1 = state[c * 4 + 1];
        const byte_t s2 = state[c * 4 + 2];
        const byte_t s3 = state[c * 4 + 3];

        state[c * 4] = gf_mult(0x02, s0) ^ gf_mult(0x03, s1) ^ s2 ^ s3;
        state[c * 4 + 1] = s0 ^ gf_mult(0x02, s1) ^ gf_mult(0x03, s2) ^ s3;
        state[c * 4 + 2] = s0 ^ s1 ^ gf_mult(0x02, s2) ^ gf_mult(0x03, s3);
        state[c * 4 + 3] = gf_mult(0x03, s0) ^ s1 ^ s2 ^ gf_mult(0x02, s3);
    }
}

void AES256::inv_mix_columns(byte_t state[16]) {
    for (int c = 0; c < 4; ++c) {
        const byte_t s0 = state[c * 4];
        const byte_t s1 = state[c * 4 + 1];
        const byte_t s2 = state[c * 4 + 2];
        const byte_t s3 = state[c * 4 + 3];

        state[c * 4] = gf_mult(0x0e, s0) ^ gf_mult(0x0b, s1) ^ gf_mult(0x0d, s2) ^ gf_mult(0x09, s3);
        state[c * 4 + 1] = gf_mult(0x09, s0) ^ gf_mult(0x0e, s1) ^ gf_mult(0x0b, s2) ^ gf_mult(0x0d, s3);
        state[c * 4 + 2] = gf_mult(0x0d, s0) ^ gf_mult(0x09, s1) ^ gf_mult(0x0e, s2) ^ gf_mult(0x0b, s3);
        state[c * 4 + 3] = gf_mult(0x0b, s0) ^ gf_mult(0x0d, s1) ^ gf_mult(0x09, s2) ^ gf_mult(0x0e, s3);
    }
}

void AES256::encrypt_block(byte_t block[16], const byte_t* expanded_key) {
    add_round_key(block, expanded_key);

    for (int round = 1; round < 14; ++round) {
        sub_bytes(block);
        shift_rows(block);
        mix_columns(block);
        add_round_key(block, expanded_key + round * 16);
    }

    sub_bytes(block);
    shift_rows(block);
    add_round_key(block, expanded_key + 14 * 16);
}

void AES256::decrypt_block(byte_t block[16], const byte_t* expanded_key) {
    add_round_key(block, expanded_key + 14 * 16);

    for (int round = 13; round >= 1; --round) {
        inv_shift_rows(block);
        inv_sub_bytes(block);
        add_round_key(block, expanded_key + round * 16);
        inv_mix_columns(block);
    }

    inv_shift_rows(block);
    inv_sub_bytes(block);
    add_round_key(block, expanded_key);
}

bstring AES256::encrypt(const bstring_view data, const bstring_view key) {
    if (key.size() != 32) {
        throw_exception(value_exception("AES-256 requires 32-byte key"));
    }
    if (data.size() % 16 != 0) {
        throw_exception(value_exception("Data size must be multiple of 16 bytes"));
    }

    byte_t expanded_key[240];
    key_expansion(key.data(), expanded_key);
    bstring result;
    result.reserve(data.size());

    for (size_t i = 0; i < data.size(); i += 16) {
        byte_t block[16];
        memory_copy(block, data.data() + i, 16);
        encrypt_block(block, expanded_key);
        for (int j = 0; j < 16; ++j) {
            result.push_back(block[j]);
        }
    }
    return result;
}

bstring AES256::decrypt(const bstring_view data, const bstring_view key) {
    if (key.size() != 32) {
        throw_exception(value_exception("AES-256 requires 32-byte key"));
    }
    if (data.size() % 16 != 0) {
        throw_exception(value_exception("Data size must be multiple of 16 bytes"));
    }

    byte_t expanded_key[240];
    key_expansion(key.data(), expanded_key);
    bstring result;
    result.reserve(data.size());

    for (size_t i = 0; i < data.size(); i += 16) {
        byte_t block[16];
        memory_copy(block, data.data() + i, 16);
        decrypt_block(block, expanded_key);
        for (int j = 0; j < 16; ++j) {
            result.push_back(block[j]);
        }
    }
    return result;
}

bstring AES256::encrypt_pkcs7(const bstring_view data, const bstring_view key) {
    bstring byte_data{data};
    const byte_t padding = 16 - (byte_data.size() % 16);
    for (int i = 0; i < padding; ++i) {
        byte_data.push_back(padding);
    }
    return encrypt(byte_data.view(), key);
}

bstring AES256::decrypt_pkcs7(const bstring_view data, const bstring_view key) {
    bstring decrypted = decrypt(data, key);
    if (decrypted.empty()) return decrypted;

    const byte_t padding_len = decrypted.back();
    const size_t original_size = decrypted.size();

    if (padding_len == 0 || padding_len > 16) {
        throw_exception(value_exception("Invalid PKCS7 padding (invalid length)"));
    }
    if (padding_len > original_size) {
        throw_exception(value_exception("Invalid PKCS7 padding (length exceeds data size)"));
    }
    const size_t new_size = original_size - padding_len;
    if (new_size >= original_size) {
        throw_exception(value_exception("Invalid PKCS7 padding (overflow detected)"));
    }
    for (size_t i = new_size; i < original_size; ++i) {
        if (decrypted[i] != padding_len) {
            throw_exception(value_exception("Invalid PKCS7 padding (mismatched value)"));
        }
    }

    decrypted.resize(new_size);
    return decrypted;
}

string AES256::encrypt_hex(const string_view data, const string_view key_hex) {
    bstring key_bytes;
    key_bytes.reserve(32);
    for (size_t i = 0; i < key_hex.size(); i += 2) {
        if (i + 1 < key_hex.size()) {
            const auto hex_val = hexadecimal::parse(key_hex.substr(i, 2));
            key_bytes.push_back(static_cast<byte_t>(hex_val.to_int64()));
        }
    }

    const bstring data_bytes(data.begin(), data.end());
    bstring encrypted = encrypt_pkcs7(data_bytes.view(), key_bytes.view());

    string result;
    for (const byte_t byte : encrypted) {
        hexadecimal hex_byte(byte);
        result += format("{02x}", hex_byte);
    }
    return result;
}

string AES256::decrypt_hex(const string_view encrypted_hex, const string_view key_hex) {
    bstring key_bytes;
    for (size_t i = 0; i < key_hex.size(); i += 2) {
        if (i + 1 < key_hex.size()) {
            const auto hex_val = hexadecimal::parse(key_hex.substr(i, 2));
            key_bytes.push_back(static_cast<byte_t>(hex_val.to_int64()));
        }
    }

    bstring encrypted_bytes;
    for (size_t i = 0; i < encrypted_hex.size(); i += 2) {
        if (i + 1 < encrypted_hex.size()) {
            const auto hex_val = hexadecimal::parse(encrypted_hex.substr(i, 2));
            encrypted_bytes.push_back(static_cast<byte_t>(hex_val.to_int64()));
        }
    }
    bstring decrypted = decrypt_pkcs7(encrypted_bytes.view(), key_bytes.view());
    return string(decrypted.begin(), decrypted.end());
}

constexpr byte_t AES256::sbox[256];
constexpr byte_t AES256::inv_sbox[256];
constexpr byte_t AES256::rcon[15];

MSTL_END_NAMESPACE__
