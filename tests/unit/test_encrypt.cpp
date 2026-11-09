#include <NeForce/core/container/vector.hpp>
#include <NeForce/core/encrypt/aes256.hpp>
#include <NeForce/core/encrypt/chacha20_poly1305.hpp>
#include <NeForce/core/string/string.hpp>
#include <NeForce/core/utility/hexadecimal.hpp>
#include <gtest/gtest.h>
using namespace neforce;

class AES256Test : public ::testing::Test {
protected:
    byte_vector key_32;
    byte_vector iv_16;
    byte_vector iv_12;

    void SetUp() override {
        key_32.resize(32);
        for (size_t i = 0; i < 32; ++i) {
            key_32[i] = static_cast<byte_t>(i);
        }

        iv_16.resize(16);
        for (size_t i = 0; i < 16; ++i) {
            iv_16[i] = static_cast<byte_t>(i + 0x10);
        }

        iv_12.resize(12);
        for (size_t i = 0; i < 12; ++i) {
            iv_12[i] = static_cast<byte_t>(i + 0x20);
        }
    }
};

TEST_F(AES256Test, EncryptEcbSingleBlockKnownVector) {
    const byte_t key_bytes[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
                                0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                                0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};
    const byte_t plaintext[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                                0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
    const byte_t expected[] = {0x8e, 0xa2, 0xb7, 0xca, 0x51, 0x67, 0x45, 0xbf,
                               0xea, 0xfc, 0x49, 0x90, 0x4b, 0x49, 0x60, 0x89};

    auto encrypted = AES256::encrypt_ecb(cbyte_view{plaintext, 16}, cbyte_view{key_bytes, 32});
    EXPECT_EQ(encrypted.size(), 16);
    for (size_t i = 0; i < 16; ++i) {
        EXPECT_EQ(encrypted[i], expected[i]) << "Byte mismatch at index " << i;
    }
}

TEST_F(AES256Test, DecryptEcbSingleBlockKnownVector) {
    const byte_t key_bytes[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
                                0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                                0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};
    const byte_t ciphertext[] = {0x8e, 0xa2, 0xb7, 0xca, 0x51, 0x67, 0x45, 0xbf,
                                 0xea, 0xfc, 0x49, 0x90, 0x4b, 0x49, 0x60, 0x89};
    const byte_t expected[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                               0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};

    auto decrypted = AES256::decrypt_ecb(cbyte_view{ciphertext, 16}, cbyte_view{key_bytes, 32});
    EXPECT_EQ(decrypted.size(), 16);
    for (size_t i = 0; i < 16; ++i) {
        EXPECT_EQ(decrypted[i], expected[i]) << "Byte mismatch at index " << i;
    }
}

TEST_F(AES256Test, EncryptEcbAllZeroKeyTestVector) {
    const byte_t key_bytes[32] = {0};
    const byte_t plaintext[16] = {0};
    const byte_t expected[] = {0xDC, 0x95, 0xC0, 0x78, 0xA2, 0x40, 0x89, 0x89,
                               0xAD, 0x48, 0xA2, 0x14, 0x92, 0x84, 0x20, 0x87};

    auto encrypted = AES256::encrypt_ecb(cbyte_view{plaintext, 16}, cbyte_view{key_bytes, 32});
    for (size_t i = 0; i < 16; ++i) {
        EXPECT_EQ(encrypted[i], expected[i]);
    }
}

TEST_F(AES256Test, DecryptEcbAllZeroKeyTestVector) {
    const byte_t key_bytes[32] = {0};
    const byte_t ciphertext[] = {0xDC, 0x95, 0xC0, 0x78, 0xA2, 0x40, 0x89, 0x89,
                                 0xAD, 0x48, 0xA2, 0x14, 0x92, 0x84, 0x20, 0x87};
    const byte_t expected[16] = {0};

    auto decrypted = AES256::decrypt_ecb(cbyte_view{ciphertext, 16}, cbyte_view{key_bytes, 32});
    for (size_t i = 0; i < 16; ++i) {
        EXPECT_EQ(decrypted[i], expected[i]);
    }
}

TEST_F(AES256Test, EncryptEcbTwoBlocksKnownVector) {
    const byte_t key_bytes[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
                                0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                                0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};
    const byte_t plaintext[32] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa,
                                  0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55,
                                  0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
    const byte_t expected[] = {0x8e, 0xa2, 0xb7, 0xca, 0x51, 0x67, 0x45, 0xbf, 0xea, 0xfc, 0x49,
                               0x90, 0x4b, 0x49, 0x60, 0x89, 0x8e, 0xa2, 0xb7, 0xca, 0x51, 0x67,
                               0x45, 0xbf, 0xea, 0xfc, 0x49, 0x90, 0x4b, 0x49, 0x60, 0x89};

    auto encrypted = AES256::encrypt_ecb(cbyte_view{plaintext, 32}, cbyte_view{key_bytes, 32});
    for (size_t i = 0; i < 32; ++i) {
        EXPECT_EQ(encrypted[i], expected[i]);
    }
}

TEST_F(AES256Test, EncryptDecryptEcbPkcs7Roundtrip) {
    const byte_t key_bytes[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
                                0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                                0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};

    const char* plaintext = "Hello, AES-256!";
    const size_t plain_len = 15;

    cbyte_view data{reinterpret_cast<const byte_t*>(plaintext), plain_len};
    cbyte_view key_view{key_bytes, 32};

    auto encrypted = AES256::encrypt_ecb_pkcs7(data, key_view);
    EXPECT_EQ(encrypted.size(), 16);

    auto decrypted = AES256::decrypt_ecb_pkcs7(encrypted.view(), key_view);
    EXPECT_EQ(decrypted.size(), plain_len);

    for (size_t i = 0; i < plain_len; ++i) {
        EXPECT_EQ(decrypted[i], static_cast<byte_t>(plaintext[i])) << "Mismatch at byte " << i;
    }
}

TEST_F(AES256Test, EncryptDecryptEcbPkcs7VariousSizes) {
    const byte_t key_bytes[32] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
                                  0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                                  0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};
    cbyte_view key_view{key_bytes, 32};

    const vector<size_t> test_sizes = {1, 15, 16, 17, 31, 32, 33};

    for (size_t size: test_sizes) {
        byte_vector test_data(size);
        for (size_t i = 0; i < size; ++i) {
            test_data[i] = static_cast<byte_t>(i & 0xFF);
        }

        auto encrypted = AES256::encrypt_ecb_pkcs7(test_data.view(), key_view);
        EXPECT_EQ(encrypted.size() % 16, 0);
        EXPECT_GT(encrypted.size(), size);
        EXPECT_LE(encrypted.size(), size + 16);

        auto decrypted = AES256::decrypt_ecb_pkcs7(encrypted.view(), key_view);
        EXPECT_EQ(decrypted.size(), size);
        EXPECT_TRUE(equal(decrypted.begin(), decrypted.end(), test_data.begin()));
    }
}

TEST_F(AES256Test, DecryptEcbPkcs7Roundtrip) {
    const byte_t key_bytes[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
                                0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                                0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};
    const char* plaintext = "Hello, AES-256!";
    const size_t plain_len = 15;

    cbyte_view data{reinterpret_cast<const byte_t*>(plaintext), plain_len};
    cbyte_view key_view{key_bytes, 32};

    auto encrypted = AES256::encrypt_ecb_pkcs7(data, key_view);
    auto decrypted = AES256::decrypt_ecb_pkcs7(encrypted.view(), key_view);

    EXPECT_EQ(decrypted.size(), plain_len);
    for (size_t i = 0; i < plain_len; ++i) {
        EXPECT_EQ(decrypted[i], static_cast<byte_t>(plaintext[i]));
    }
}

TEST_F(AES256Test, EncryptEcbHexRoundtrip) {
    string_view key_hex = "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f";
    string_view plaintext = "Hello, AES-256!";

    auto encrypted = AES256::encrypt_ecb_hex(plaintext, key_hex);
    auto decrypted = AES256::decrypt_ecb_hex(encrypted.view(), key_hex);

    EXPECT_EQ(decrypted, plaintext);
}

TEST_F(AES256Test, DecryptEcbHexRoundtrip) {
    string_view key_hex = "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f";
    string_view plaintext = "Test message 123";

    auto encrypted = AES256::encrypt_ecb_hex(plaintext, key_hex);
    auto decrypted = AES256::decrypt_ecb_hex(encrypted.view(), key_hex);

    EXPECT_EQ(decrypted, plaintext);
}

TEST_F(AES256Test, EncryptEcbHexEmptyRoundtrip) {
    string_view key_hex = "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f";

    auto encrypted = AES256::encrypt_ecb_hex("", key_hex);
    auto decrypted = AES256::decrypt_ecb_hex(encrypted.view(), key_hex);

    EXPECT_EQ(decrypted, "");
}

TEST_F(AES256Test, DecryptEcbHexEmptyRoundtrip) {
    string_view key_hex = "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f";

    auto encrypted = AES256::encrypt_ecb_hex("", key_hex);
    auto decrypted = AES256::decrypt_ecb_hex(encrypted.view(), key_hex);

    EXPECT_EQ(decrypted, "");
}

TEST_F(AES256Test, EncryptCbcRoundtrip) {
    const byte_t key_bytes[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
                                0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                                0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};
    const byte_t iv_bytes[] = {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                               0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};

    byte_t plaintext[32];
    for (size_t i = 0; i < 32; ++i) {
        plaintext[i] = static_cast<byte_t>(i);
    }

    cbyte_view data{plaintext, 32};
    auto encrypted = AES256::encrypt_cbc(data, cbyte_view{key_bytes, 32}, cbyte_view{iv_bytes, 16});
    auto decrypted = AES256::decrypt_cbc(encrypted.view(), cbyte_view{key_bytes, 32}, cbyte_view{iv_bytes, 16});

    EXPECT_EQ(decrypted.size(), 32);
    for (size_t i = 0; i < 32; ++i) {
        EXPECT_EQ(decrypted[i], plaintext[i]);
    }
}

TEST_F(AES256Test, DecryptCbcRoundtrip) {
    const byte_t key_bytes[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
                                0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                                0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};
    const byte_t iv_bytes[] = {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                               0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};
    byte_t plaintext[32];
    for (size_t i = 0; i < 32; ++i) {
        plaintext[i] = static_cast<byte_t>(i);
    }

    auto encrypted =
            AES256::encrypt_cbc(cbyte_view{plaintext, 32}, cbyte_view{key_bytes, 32}, cbyte_view{iv_bytes, 16});
    auto decrypted = AES256::decrypt_cbc(encrypted.view(), cbyte_view{key_bytes, 32}, cbyte_view{iv_bytes, 16});

    EXPECT_EQ(decrypted.size(), 32);
    for (size_t i = 0; i < 32; ++i) {
        EXPECT_EQ(decrypted[i], plaintext[i]);
    }
}

TEST_F(AES256Test, EncryptCbcPkcs7Roundtrip) {
    const byte_t key_bytes[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
                                0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                                0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};
    const byte_t iv_bytes[] = {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                               0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};
    const char* plaintext = "Hello, AES-256!";
    const size_t plain_len = 15;

    cbyte_view data{reinterpret_cast<const byte_t*>(plaintext), plain_len};
    auto encrypted = AES256::encrypt_cbc_pkcs7(data, cbyte_view{key_bytes, 32}, cbyte_view{iv_bytes, 16});
    auto decrypted = AES256::decrypt_cbc_pkcs7(encrypted.view(), cbyte_view{key_bytes, 32}, cbyte_view{iv_bytes, 16});

    EXPECT_EQ(decrypted.size(), plain_len);
    for (size_t i = 0; i < plain_len; ++i) {
        EXPECT_EQ(decrypted[i], static_cast<byte_t>(plaintext[i]));
    }
}

TEST_F(AES256Test, DecryptCbcPkcs7Roundtrip) {
    const byte_t key_bytes[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
                                0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                                0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};
    const byte_t iv_bytes[] = {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                               0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};
    const char* plaintext = "Another test!";
    const size_t plain_len = 13;

    auto encrypted = AES256::encrypt_cbc_pkcs7(cbyte_view{reinterpret_cast<const byte_t*>(plaintext), plain_len},
                                               cbyte_view{key_bytes, 32}, cbyte_view{iv_bytes, 16});
    auto decrypted = AES256::decrypt_cbc_pkcs7(encrypted.view(), cbyte_view{key_bytes, 32}, cbyte_view{iv_bytes, 16});

    EXPECT_EQ(decrypted.size(), plain_len);
    for (size_t i = 0; i < plain_len; ++i) {
        EXPECT_EQ(decrypted[i], static_cast<byte_t>(plaintext[i]));
    }
}

TEST_F(AES256Test, EncryptGcmKnownVectorTest1) {
    const byte_t key_bytes[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
                                0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                                0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};
    const byte_t iv_bytes[] = {0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b};
    const byte_t plaintext[] = {0x48, 0x65, 0x6c, 0x6c, 0x6f};
    const byte_t aad[] = {0x41, 0x55, 0x54, 0x48};
    const byte_t expected_ciphertext[] = {0x9a, 0x5f, 0xca, 0x1c, 0x03};
    const byte_t expected_tag[] = {0x7a, 0x80, 0x98, 0x3c, 0x35, 0x41, 0x5d, 0xeb,
                                   0xd6, 0xc8, 0xe3, 0x20, 0x13, 0xd3, 0xac, 0x34};

    byte_t tag[16];
    auto encrypted = AES256::encrypt_gcm(cbyte_view{plaintext, 5}, cbyte_view{key_bytes, 32}, cbyte_view{iv_bytes, 12},
                                         cbyte_view{aad, 4}, tag, 16);
    EXPECT_EQ(encrypted.size(), 5);
    for (size_t i = 0; i < 5; ++i) {
        EXPECT_EQ(encrypted[i], expected_ciphertext[i]);
    }
    for (size_t i = 0; i < 16; ++i) {
        EXPECT_EQ(tag[i], expected_tag[i]);
    }
}

TEST_F(AES256Test, EncryptGcmKnownVector64Bytes) {
    const byte_t key_bytes[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
                                0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                                0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};
    const byte_t iv_bytes[] = {0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b};
    const byte_t aad[] = {0x41, 0x55, 0x54, 0x48};
    const byte_t expected_ciphertext[] = {0xd2, 0x3b, 0xa4, 0x73, 0x68, 0x9d, 0x1c, 0x09, 0x12, 0x75, 0x48, 0xc5, 0xcd,
                                          0x15, 0xfa, 0xf6, 0xc0, 0x58, 0xfe, 0x8f, 0x93, 0x95, 0x75, 0xf9, 0x74, 0xef,
                                          0x76, 0x09, 0x55, 0x97, 0x40, 0x16, 0x55, 0xaf, 0xc7, 0x27, 0xaa, 0x00, 0x13,
                                          0xfd, 0x3c, 0xd4, 0x44, 0xb9, 0x74, 0x24, 0x31, 0xe7, 0xd2, 0x1b, 0xde, 0xbd,
                                          0xae, 0x09, 0x1b, 0x56, 0xf6, 0x5b, 0x45, 0x26, 0x70, 0x8f, 0x34, 0xe2};
    const byte_t expected_tag[] = {0x9d, 0x3d, 0x31, 0xc2, 0x81, 0xfc, 0x80, 0x25,
                                   0x51, 0x29, 0xde, 0x2c, 0x78, 0xc4, 0xba, 0x67};

    byte_vector plaintext(64);
    for (size_t i = 0; i < plaintext.size(); ++i) {
        plaintext[i] = static_cast<byte_t>(i);
    }

    byte_t tag[16];
    auto encrypted = AES256::encrypt_gcm(plaintext.view(), cbyte_view{key_bytes, 32}, cbyte_view{iv_bytes, 12},
                                         cbyte_view{aad, 4}, tag, 16);
    ASSERT_EQ(encrypted.size(), 64);
    for (size_t i = 0; i < 64; ++i) {
        EXPECT_EQ(encrypted[i], expected_ciphertext[i]);
    }
    for (size_t i = 0; i < 16; ++i) {
        EXPECT_EQ(tag[i], expected_tag[i]);
    }
    auto decrypted = AES256::decrypt_gcm(encrypted.view(), cbyte_view{key_bytes, 32}, cbyte_view{iv_bytes, 12},
                                         cbyte_view{aad, 4}, cbyte_view{tag, 16}, 16);
    for (size_t i = 0; i < 64; ++i) {
        EXPECT_EQ(decrypted[i], plaintext[i]);
    }
}

TEST_F(AES256Test, EncryptGcmKnownVector144Bytes) {
    const byte_t key_bytes[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
                                0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                                0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};
    const byte_t iv_bytes[] = {0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b};
    const byte_t aad[] = {0x41, 0x55, 0x54, 0x48};
    const byte_t expected_ciphertext[] = {
            0xd2, 0x3b, 0xa4, 0x73, 0x68, 0x9d, 0x1c, 0x09, 0x12, 0x75, 0x48, 0xc5, 0xcd, 0x15, 0xfa, 0xf6, 0xc0, 0x58,
            0xfe, 0x8f, 0x93, 0x95, 0x75, 0xf9, 0x74, 0xef, 0x76, 0x09, 0x55, 0x97, 0x40, 0x16, 0x55, 0xaf, 0xc7, 0x27,
            0xaa, 0x00, 0x13, 0xfd, 0x3c, 0xd4, 0x44, 0xb9, 0x74, 0x24, 0x31, 0xe7, 0xd2, 0x1b, 0xde, 0xbd, 0xae, 0x09,
            0x1b, 0x56, 0xf6, 0x5b, 0x45, 0x26, 0x70, 0x8f, 0x34, 0xe2, 0x98, 0x67, 0x8f, 0x48, 0xd2, 0xd2, 0xa3, 0xd1,
            0xc3, 0x23, 0xa3, 0x6c, 0xf7, 0x10, 0x77, 0x2e, 0x85, 0xc6, 0x61, 0x57, 0xe3, 0xae, 0xd0, 0x49, 0x97, 0x16,
            0x98, 0x71, 0x46, 0x41, 0xc0, 0x83, 0x62, 0x35, 0x4c, 0x35, 0xac, 0x21, 0xf8, 0x94, 0xa1, 0xf5, 0xce, 0x6c,
            0xcf, 0x9b, 0xa3, 0x7c, 0x00, 0xe0, 0xfd, 0x31, 0xe5, 0xab, 0xd9, 0x7a, 0x28, 0x8b, 0x7d, 0x74, 0x89, 0xfa,
            0xfc, 0x57, 0xbc, 0x65, 0xff, 0x1d, 0xbf, 0x6b, 0x85, 0x36, 0xed, 0x06, 0x28, 0xe1, 0x82, 0xb2, 0x92, 0x29};
    const byte_t expected_tag[] = {0x37, 0x5e, 0x6e, 0x14, 0x5e, 0xad, 0xea, 0x41,
                                   0xd5, 0x0d, 0xcb, 0x3c, 0x1d, 0x08, 0x80, 0x13};

    byte_vector plaintext(144);
    for (size_t i = 0; i < plaintext.size(); ++i) {
        plaintext[i] = static_cast<byte_t>(i);
    }

    byte_t tag[16];
    auto encrypted = AES256::encrypt_gcm(plaintext.view(), cbyte_view{key_bytes, 32}, cbyte_view{iv_bytes, 12},
                                         cbyte_view{aad, 4}, tag, 16);
    ASSERT_EQ(encrypted.size(), 144);
    for (size_t i = 0; i < 144; ++i) {
        EXPECT_EQ(encrypted[i], expected_ciphertext[i]);
    }
    for (size_t i = 0; i < 16; ++i) {
        EXPECT_EQ(tag[i], expected_tag[i]);
    }
    auto decrypted = AES256::decrypt_gcm(encrypted.view(), cbyte_view{key_bytes, 32}, cbyte_view{iv_bytes, 12},
                                         cbyte_view{aad, 4}, cbyte_view{tag, 16}, 16);
    for (size_t i = 0; i < 144; ++i) {
        EXPECT_EQ(decrypted[i], plaintext[i]);
    }
}

TEST_F(AES256Test, DecryptGcmKnownVectorTest1) {
    const byte_t key_bytes[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
                                0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                                0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};
    const byte_t iv_bytes[] = {0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b};
    const byte_t ciphertext[] = {0x9a, 0x5f, 0xca, 0x1c, 0x03};
    const byte_t aad[] = {0x41, 0x55, 0x54, 0x48};
    const byte_t tag[] = {0x7a, 0x80, 0x98, 0x3c, 0x35, 0x41, 0x5d, 0xeb,
                          0xd6, 0xc8, 0xe3, 0x20, 0x13, 0xd3, 0xac, 0x34};
    const byte_t expected_plaintext[] = {0x48, 0x65, 0x6c, 0x6c, 0x6f};

    auto decrypted = AES256::decrypt_gcm(cbyte_view{ciphertext, 5}, cbyte_view{key_bytes, 32}, cbyte_view{iv_bytes, 12},
                                         cbyte_view{aad, 4}, cbyte_view{tag, 16}, 16);
    EXPECT_EQ(decrypted.size(), 5);
    for (size_t i = 0; i < 5; ++i) {
        EXPECT_EQ(decrypted[i], expected_plaintext[i]);
    }
}

TEST_F(AES256Test, DecryptGcmWrongTagTest) {
    const byte_t key_bytes[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
                                0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                                0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};
    const byte_t iv_bytes[] = {0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b};
    const byte_t ciphertext[] = {0x9a, 0x5f, 0xca, 0x1c, 0x03};
    const byte_t aad[] = {0x41, 0x55, 0x54, 0x48};

    const byte_t wrong_tag[] = {0x7a, 0x80, 0x98, 0x3c, 0x35, 0x41, 0x5d, 0xeb,
                                0xd6, 0xc8, 0xe3, 0x20, 0x13, 0xd3, 0xac, 0x00};

    EXPECT_THROW(
            {
                auto decrypted = AES256::decrypt_gcm(cbyte_view{ciphertext, 5}, cbyte_view{key_bytes, 32},
                                                     cbyte_view{iv_bytes, 12}, cbyte_view{aad, 4},
                                                     cbyte_view{wrong_tag, 16}, 16);
            },
            value_exception);
}

TEST_F(AES256Test, DecryptGcmWrongCiphertextTest) {
    const byte_t key_bytes[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
                                0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                                0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};
    const byte_t iv_bytes[] = {0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b};

    const byte_t wrong_ciphertext[] = {0x00, 0x5f, 0xca, 0x1c, 0x03};
    const byte_t aad[] = {0x41, 0x55, 0x54, 0x48};
    const byte_t tag[] = {0x7a, 0x80, 0x98, 0x3c, 0x35, 0x41, 0x5d, 0xeb,
                          0xd6, 0xc8, 0xe3, 0x20, 0x13, 0xd3, 0xac, 0x34};

    EXPECT_THROW(
            {
                auto decrypted =
                        AES256::decrypt_gcm(cbyte_view{wrong_ciphertext, 5}, cbyte_view{key_bytes, 32},
                                            cbyte_view{iv_bytes, 12}, cbyte_view{aad, 4}, cbyte_view{tag, 16}, 16);
            },
            value_exception);
}

TEST_F(AES256Test, EncryptGcmKnownVectorNoAad) {
    const byte_t key_bytes[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
                                0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                                0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};
    const byte_t iv_bytes[] = {0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b};
    const byte_t plaintext[] = {0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x20, 0x57, 0x6f, 0x72, 0x6c, 0x64};
    const byte_t expected_ciphertext[] = {0x9a, 0x5f, 0xca, 0x1c, 0x03, 0xb8, 0x4d, 0x61, 0x68, 0x10, 0x26};
    const byte_t expected_tag[] = {0xe7, 0xf3, 0x55, 0xe4, 0xfe, 0x72, 0x30, 0x50,
                                   0xc6, 0x67, 0x54, 0xe3, 0x4e, 0xc9, 0x2f, 0x88};

    byte_t tag[16];
    auto encrypted = AES256::encrypt_gcm(cbyte_view{plaintext, 11}, cbyte_view{key_bytes, 32}, cbyte_view{iv_bytes, 12},
                                         {}, tag, 16);
    EXPECT_EQ(encrypted.size(), 11);
    for (size_t i = 0; i < 11; ++i) {
        EXPECT_EQ(encrypted[i], expected_ciphertext[i]);
    }
    for (size_t i = 0; i < 16; ++i) {
        EXPECT_EQ(tag[i], expected_tag[i]);
    }
}

TEST_F(AES256Test, EncryptGcmTagLength12) {
    const byte_t key_bytes[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
                                0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                                0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};
    const byte_t iv_bytes[] = {0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b};
    const byte_t plaintext[] = {0x48, 0x65, 0x6c, 0x6c, 0x6f};
    const byte_t expected_ciphertext[] = {0x9a, 0x5f, 0xca, 0x1c, 0x03};
    const byte_t expected_tag[] = {0x0a, 0xca, 0xd6, 0xf5, 0x5c, 0xdc, 0x52, 0xbf, 0x27, 0xbe, 0xab, 0x7e};

    byte_t tag[12];
    auto encrypted = AES256::encrypt_gcm(cbyte_view{plaintext, 5}, cbyte_view{key_bytes, 32}, cbyte_view{iv_bytes, 12},
                                         {}, tag, 12);
    EXPECT_EQ(encrypted.size(), 5);
    for (size_t i = 0; i < 5; ++i) {
        EXPECT_EQ(encrypted[i], expected_ciphertext[i]);
    }
    for (size_t i = 0; i < 12; ++i) {
        EXPECT_EQ(tag[i], expected_tag[i]);
    }
}

TEST_F(AES256Test, DecryptGcmTagLength12) {
    const byte_t key_bytes[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
                                0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                                0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};
    const byte_t iv_bytes[] = {0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b};
    const byte_t ciphertext[] = {0x9a, 0x5f, 0xca, 0x1c, 0x03};
    const byte_t tag[] = {0x0a, 0xca, 0xd6, 0xf5, 0x5c, 0xdc, 0x52, 0xbf, 0x27, 0xbe, 0xab, 0x7e};
    const byte_t expected_plaintext[] = {0x48, 0x65, 0x6c, 0x6c, 0x6f};

    auto decrypted = AES256::decrypt_gcm(cbyte_view{ciphertext, 5}, cbyte_view{key_bytes, 32}, cbyte_view{iv_bytes, 12},
                                         {}, cbyte_view{tag, 12}, 12);
    EXPECT_EQ(decrypted.size(), 5);
    for (size_t i = 0; i < 5; ++i) {
        EXPECT_EQ(decrypted[i], expected_plaintext[i]);
    }
}

TEST_F(AES256Test, EncryptGcmNon12ByteIv) {
    const byte_t key_bytes[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
                                0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                                0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};
    const byte_t iv_bytes[] = {0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
                               0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f};
    const byte_t plaintext[] = {0x48, 0x65, 0x6c, 0x6c, 0x6f};
    const byte_t expected_ciphertext[] = {0x83, 0xa9, 0xc8, 0x8a, 0x73};
    const byte_t expected_tag[] = {0xc9, 0xec, 0xaf, 0x31, 0x88, 0x1a, 0x3d, 0xbd,
                                   0x7f, 0x86, 0xe1, 0xb0, 0xb3, 0x53, 0xe0, 0x88};

    byte_t tag[16];
    auto encrypted = AES256::encrypt_gcm(cbyte_view{plaintext, 5}, cbyte_view{key_bytes, 32}, cbyte_view{iv_bytes, 16},
                                         {}, tag, 16);
    EXPECT_EQ(encrypted.size(), 5);
    for (size_t i = 0; i < 5; ++i) {
        EXPECT_EQ(encrypted[i], expected_ciphertext[i]);
    }
    for (size_t i = 0; i < 16; ++i) {
        EXPECT_EQ(tag[i], expected_tag[i]);
    }
}

TEST_F(AES256Test, DecryptGcmNon12ByteIv) {
    const byte_t key_bytes[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
                                0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                                0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};
    const byte_t iv_bytes[] = {0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
                               0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f};
    const byte_t ciphertext[] = {0x83, 0xa9, 0xc8, 0x8a, 0x73};
    const byte_t tag[] = {0xc9, 0xec, 0xaf, 0x31, 0x88, 0x1a, 0x3d, 0xbd,
                          0x7f, 0x86, 0xe1, 0xb0, 0xb3, 0x53, 0xe0, 0x88};
    const byte_t expected_plaintext[] = {0x48, 0x65, 0x6c, 0x6c, 0x6f};

    auto decrypted = AES256::decrypt_gcm(cbyte_view{ciphertext, 5}, cbyte_view{key_bytes, 32}, cbyte_view{iv_bytes, 16},
                                         {}, cbyte_view{tag, 16}, 16);
    EXPECT_EQ(decrypted.size(), 5);
    for (size_t i = 0; i < 5; ++i) {
        EXPECT_EQ(decrypted[i], expected_plaintext[i]);
    }
}

TEST_F(AES256Test, EncryptGcmAllZeroKeyAndIv) {
    const byte_t key_bytes[32] = {0};
    const byte_t iv_bytes[12] = {0};
    const byte_t plaintext[16] = {0};
    const byte_t expected_ciphertext[] = {0xce, 0xa7, 0x40, 0x3d, 0x4d, 0x60, 0x6b, 0x6e,
                                          0x07, 0x4e, 0xc5, 0xd3, 0xba, 0xf3, 0x9d, 0x18};
    const byte_t expected_tag[] = {0xd0, 0xd1, 0xc8, 0xa7, 0x99, 0x99, 0x6b, 0xf0,
                                   0x26, 0x5b, 0x98, 0xb5, 0xd4, 0x8a, 0xb9, 0x19};

    byte_t tag[16];
    auto encrypted = AES256::encrypt_gcm(cbyte_view{plaintext, 16}, cbyte_view{key_bytes, 32}, cbyte_view{iv_bytes, 12},
                                         {}, tag, 16);
    EXPECT_EQ(encrypted.size(), 16);
    for (size_t i = 0; i < 16; ++i) {
        EXPECT_EQ(encrypted[i], expected_ciphertext[i]);
    }
    for (size_t i = 0; i < 16; ++i) {
        EXPECT_EQ(tag[i], expected_tag[i]);
    }
}

TEST_F(AES256Test, EncryptEcbKnownVectorNist) {
    const byte_t key_bytes[] = {0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe, 0x2b, 0x73, 0xae,
                                0xf0, 0x85, 0x7d, 0x77, 0x81, 0x1f, 0x35, 0x2c, 0x07, 0x3b, 0x61,
                                0x08, 0xd7, 0x2d, 0x98, 0x10, 0xa3, 0x09, 0x14, 0xdf, 0xf4};
    const byte_t plaintext[] = {0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
                                0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a};
    const byte_t expected[] = {0xf3, 0xee, 0xd1, 0xbd, 0xb5, 0xd2, 0xa0, 0x3c,
                               0x06, 0x4b, 0x5a, 0x7e, 0x3d, 0xb1, 0x81, 0xf8};

    auto encrypted = AES256::encrypt_ecb(cbyte_view{plaintext, 16}, cbyte_view{key_bytes, 32});
    for (size_t i = 0; i < 16; ++i) {
        EXPECT_EQ(encrypted[i], expected[i]);
    }
}

TEST_F(AES256Test, EncryptCbcKnownVectorNist) {
    const byte_t key_bytes[] = {0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe, 0x2b, 0x73, 0xae,
                                0xf0, 0x85, 0x7d, 0x77, 0x81, 0x1f, 0x35, 0x2c, 0x07, 0x3b, 0x61,
                                0x08, 0xd7, 0x2d, 0x98, 0x10, 0xa3, 0x09, 0x14, 0xdf, 0xf4};
    const byte_t iv_bytes[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                               0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
    const byte_t plaintext[] = {0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
                                0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a};
    const byte_t expected[] = {0xf5, 0x8c, 0x4c, 0x04, 0xd6, 0xe5, 0xf1, 0xba,
                               0x77, 0x9e, 0xab, 0xfb, 0x5f, 0x7b, 0xfb, 0xd6};

    auto encrypted =
            AES256::encrypt_cbc(cbyte_view{plaintext, 16}, cbyte_view{key_bytes, 32}, cbyte_view{iv_bytes, 16});
    for (size_t i = 0; i < 16; ++i) {
        EXPECT_EQ(encrypted[i], expected[i]);
    }
}

TEST_F(AES256Test, EncryptCbcTwoBlocksKnownVectorNist) {
    const byte_t key_bytes[] = {0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe, 0x2b, 0x73, 0xae,
                                0xf0, 0x85, 0x7d, 0x77, 0x81, 0x1f, 0x35, 0x2c, 0x07, 0x3b, 0x61,
                                0x08, 0xd7, 0x2d, 0x98, 0x10, 0xa3, 0x09, 0x14, 0xdf, 0xf4};
    const byte_t iv_bytes[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                               0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
    const byte_t plaintext[] = {0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96, 0xe9, 0x3d, 0x7e,
                                0x11, 0x73, 0x93, 0x17, 0x2a, 0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03,
                                0xac, 0x9c, 0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51};
    const byte_t expected[] = {0xf5, 0x8c, 0x4c, 0x04, 0xd6, 0xe5, 0xf1, 0xba, 0x77, 0x9e, 0xab,
                               0xfb, 0x5f, 0x7b, 0xfb, 0xd6, 0x9c, 0xfc, 0x4e, 0x96, 0x7e, 0xdb,
                               0x80, 0x8d, 0x67, 0x9f, 0x77, 0x7b, 0xc6, 0x70, 0x2c, 0x7d};

    auto encrypted =
            AES256::encrypt_cbc(cbyte_view{plaintext, 32}, cbyte_view{key_bytes, 32}, cbyte_view{iv_bytes, 16});
    for (size_t i = 0; i < 32; ++i) {
        EXPECT_EQ(encrypted[i], expected[i]);
    }
}

TEST_F(AES256Test, EncryptEcbPkcs7LongStringRoundTrip) {
    string_view key_hex = "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f";
    string_view original = "The quick brown fox jumps over the lazy dog. The quick brown fox jumps over the lazy dog. "
                           "The quick brown fox jumps over the lazy dog. The quick brown fox jumps over the lazy dog. "
                           "The quick brown fox jumps over the lazy dog. The quick brown fox jumps over the lazy dog.";

    auto encrypted = AES256::encrypt_ecb_hex(original, key_hex);
    auto decrypted = AES256::decrypt_ecb_hex(encrypted.view(), key_hex);
    EXPECT_EQ(decrypted, original);
}

TEST_F(AES256Test, EncryptCbcPkcs7LongStringRoundTrip) {
    string_view key_hex = "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f";
    string_view iv_hex = "101112131415161718191a1b1c1d1e1f";
    string_view original = "The quick brown fox jumps over the lazy dog. The quick brown fox jumps over the lazy dog. "
                           "The quick brown fox jumps over the lazy dog. The quick brown fox jumps over the lazy dog. "
                           "The quick brown fox jumps over the lazy dog. The quick brown fox jumps over the lazy dog.";

    byte_vector key_bytes;
    for (size_t i = 0; i + 2 <= key_hex.size(); i += 2) {
        auto hex_val = hexadecimal::parse(key_hex.substr(i, 2));
        key_bytes.push_back(static_cast<byte_t>(hex_val.value()));
    }
    byte_vector iv_bytes;
    for (size_t i = 0; i + 2 <= iv_hex.size(); i += 2) {
        auto hex_val = hexadecimal::parse(iv_hex.substr(i, 2));
        iv_bytes.push_back(static_cast<byte_t>(hex_val.value()));
    }

    cbyte_view data{reinterpret_cast<const byte_t*>(original.data()), original.size()};
    auto encrypted = AES256::encrypt_cbc_pkcs7(data, key_bytes.view(), iv_bytes.view());
    auto decrypted = AES256::decrypt_cbc_pkcs7(encrypted.view(), key_bytes.view(), iv_bytes.view());
    string decrypted_str{reinterpret_cast<const char*>(decrypted.data()), decrypted.size()};
    EXPECT_EQ(decrypted_str, original);
}

TEST_F(AES256Test, EncryptEcbPkcs7AllPaddingSizes) {
    string_view key_hex = "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f";

    for (size_t len = 0; len <= 32; ++len) {
        string original(len, static_cast<char>('A' + len % 26));
        auto encrypted = AES256::encrypt_ecb_hex(original.view(), key_hex);
        auto decrypted = AES256::decrypt_ecb_hex(encrypted.view(), key_hex);
        EXPECT_EQ(decrypted, original) << "Failed for length " << len;
    }
}

TEST_F(AES256Test, EncryptCbcPkcs7AllPaddingSizes) {
    string_view key_hex = "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f";
    string_view iv_hex = "101112131415161718191a1b1c1d1e1f";

    byte_vector key_bytes;
    for (size_t i = 0; i + 2 <= key_hex.size(); i += 2) {
        auto hex_val = hexadecimal::parse(key_hex.substr(i, 2));
        key_bytes.push_back(static_cast<byte_t>(hex_val.value()));
    }
    byte_vector iv_bytes;
    for (size_t i = 0; i + 2 <= iv_hex.size(); i += 2) {
        auto hex_val = hexadecimal::parse(iv_hex.substr(i, 2));
        iv_bytes.push_back(static_cast<byte_t>(hex_val.value()));
    }

    for (size_t len = 0; len <= 32; ++len) {
        string original(len, static_cast<char>('A' + len % 26));
        cbyte_view data{reinterpret_cast<const byte_t*>(original.data()), original.size()};
        auto encrypted = AES256::encrypt_cbc_pkcs7(data, key_bytes.view(), iv_bytes.view());
        auto decrypted = AES256::decrypt_cbc_pkcs7(encrypted.view(), key_bytes.view(), iv_bytes.view());
        string decrypted_str{reinterpret_cast<const char*>(decrypted.data()), decrypted.size()};
        EXPECT_EQ(decrypted_str, original) << "Failed for length " << len;
    }
}

TEST_F(AES256Test, EncryptEcbHexInvalidKeyLength) {
    string_view short_key = "0001020304050607";
    EXPECT_THROW(AES256::encrypt_ecb_hex("test", short_key), value_exception);
    EXPECT_THROW(AES256::encrypt_ecb_hex("test", ""), value_exception);
}

TEST_F(AES256Test, EncryptEcbHexNonHexCharacters) {
    string_view invalid_key = "gg0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f";
    EXPECT_THROW(AES256::encrypt_ecb_hex("test", invalid_key), value_exception);
}

TEST_F(AES256Test, EncryptGcmKnownVectorWithLongAad) {
    const byte_t key_bytes[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
                                0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                                0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};
    const byte_t iv_bytes[] = {0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b};
    const byte_t plaintext[] = {0x48, 0x65, 0x6c, 0x6c, 0x6f};
    byte_t aad[255];
    for (size_t i = 0; i < 255; ++i) {
        aad[i] = static_cast<byte_t>(i);
    }
    const byte_t expected_ciphertext[] = {0x9a, 0x5f, 0xca, 0x1c, 0x03};
    const byte_t expected_tag[] = {0xf2, 0x76, 0x09, 0x78, 0xe7, 0x7d, 0x58, 0x7c,
                                   0x92, 0xfa, 0x7e, 0xaf, 0xeb, 0x5c, 0x1d, 0x49};

    byte_t tag[16];
    auto encrypted = AES256::encrypt_gcm(cbyte_view{plaintext, 5}, cbyte_view{key_bytes, 32}, cbyte_view{iv_bytes, 12},
                                         cbyte_view{aad, 255}, tag, 16);
    for (size_t i = 0; i < 5; ++i) {
        EXPECT_EQ(encrypted[i], expected_ciphertext[i]);
    }
    for (size_t i = 0; i < 16; ++i) {
        EXPECT_EQ(tag[i], expected_tag[i]);
    }
}

TEST_F(AES256Test, EncryptGcmExactBlockBoundary) {
    const byte_t key_bytes[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
                                0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                                0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};
    const byte_t iv_bytes[] = {0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b};
    byte_t plaintext[16];
    for (size_t i = 0; i < 16; ++i) {
        plaintext[i] = static_cast<byte_t>(i);
    }
    const byte_t expected_ciphertext[] = {0xd2, 0x3b, 0xa4, 0x73, 0x68, 0x9d, 0x1c, 0x09,
                                          0x12, 0x75, 0x48, 0xc5, 0xcd, 0x15, 0xfa, 0xf6};
    const byte_t expected_tag[] = {0x22, 0xbe, 0x22, 0x44, 0x8f, 0x3a, 0xd7, 0x84,
                                   0x31, 0x1b, 0x0f, 0x54, 0xd0, 0xd9, 0x5e, 0xf6};

    byte_t tag[16];
    auto encrypted = AES256::encrypt_gcm(cbyte_view{plaintext, 16}, cbyte_view{key_bytes, 32}, cbyte_view{iv_bytes, 12},
                                         {}, tag, 16);
    for (size_t i = 0; i < 16; ++i) {
        EXPECT_EQ(encrypted[i], expected_ciphertext[i]);
    }
    for (size_t i = 0; i < 16; ++i) {
        EXPECT_EQ(tag[i], expected_tag[i]);
    }
}

TEST_F(AES256Test, DecryptGcmExactBlockBoundary) {
    const byte_t key_bytes[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
                                0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                                0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};
    const byte_t iv_bytes[] = {0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b};
    const byte_t ciphertext[] = {0xd2, 0x3b, 0xa4, 0x73, 0x68, 0x9d, 0x1c, 0x09,
                                 0x12, 0x75, 0x48, 0xc5, 0xcd, 0x15, 0xfa, 0xf6};
    const byte_t tag[] = {0x22, 0xbe, 0x22, 0x44, 0x8f, 0x3a, 0xd7, 0x84,
                          0x31, 0x1b, 0x0f, 0x54, 0xd0, 0xd9, 0x5e, 0xf6};
    byte_t expected_plaintext[16];
    for (size_t i = 0; i < 16; ++i) {
        expected_plaintext[i] = static_cast<byte_t>(i);
    }

    auto decrypted = AES256::decrypt_gcm(cbyte_view{ciphertext, 16}, cbyte_view{key_bytes, 32},
                                         cbyte_view{iv_bytes, 12}, {}, cbyte_view{tag, 16}, 16);
    for (size_t i = 0; i < 16; ++i) {
        EXPECT_EQ(decrypted[i], expected_plaintext[i]);
    }
}

TEST_F(AES256Test, EncryptDecryptGcmMultiBlockRoundTrip) {
    const byte_t key_bytes[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
                                0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                                0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};
    const byte_t iv_bytes[] = {0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b};
    byte_vector aad(23);
    byte_vector plaintext(4096 + 7);
    for (size_t i = 0; i < aad.size(); ++i) {
        aad[i] = static_cast<byte_t>(i * 3);
    }
    for (size_t i = 0; i < plaintext.size(); ++i) {
        plaintext[i] = static_cast<byte_t>(i * 7 + 11);
    }

    byte_t tag[16];
    auto encrypted = AES256::encrypt_gcm(plaintext.view(), cbyte_view{key_bytes, 32}, cbyte_view{iv_bytes, 12},
                                         aad.view(), tag, 16);
    EXPECT_EQ(encrypted.size(), plaintext.size());
    auto decrypted = AES256::decrypt_gcm(encrypted.view(), cbyte_view{key_bytes, 32}, cbyte_view{iv_bytes, 12},
                                         aad.view(), cbyte_view{tag, 16}, 16);
    EXPECT_EQ(decrypted.size(), plaintext.size());
    for (size_t i = 0; i < plaintext.size(); ++i) {
        EXPECT_EQ(decrypted[i], plaintext[i]);
    }
}

TEST_F(AES256Test, ConvenienceAes256EncryptDecryptRoundTrip) {
    string_view key_hex = "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f";
    string_view original = "Test convenience functions";

    auto encrypted = aes256_encrypt(original, key_hex);
    auto decrypted = aes256_decrypt(encrypted.view(), key_hex);
    EXPECT_EQ(decrypted, original);
}

TEST_F(AES256Test, MultipleEncryptDecryptOperationsConsistent) {
    string_view key_hex = "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f";
    string_view original = "Consistency test string";

    auto encrypted1 = aes256_encrypt(original, key_hex);
    auto encrypted2 = aes256_encrypt(original, key_hex);
    auto decrypted1 = aes256_decrypt(encrypted1.view(), key_hex);
    auto decrypted2 = aes256_decrypt(encrypted2.view(), key_hex);

    EXPECT_EQ(encrypted1, encrypted2);
    EXPECT_EQ(decrypted1, original);
    EXPECT_EQ(decrypted2, original);
}

class ChaCha20Poly1305Test : public ::testing::Test {
protected:
    byte_vector key_32;
    byte_vector nonce_12;

    void SetUp() override {
        key_32.resize(32);
        for (size_t i = 0; i < 32; ++i) {
            key_32[i] = static_cast<byte_t>(i);
        }

        nonce_12.resize(12);
        for (size_t i = 0; i < 12; ++i) {
            nonce_12[i] = static_cast<byte_t>(i + 0x20);
        }
    }
};

TEST_F(ChaCha20Poly1305Test, EncryptRfc8439TestVector) {
    const byte_t key_bytes[] = {0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a,
                                0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 0x90, 0x91, 0x92, 0x93, 0x94, 0x95,
                                0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f};
    const byte_t nonce_bytes[] = {0x07, 0x00, 0x00, 0x00, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47};
    const char* plaintext_str =
            "Ladies and Gentlemen of the class of '99: If I could offer you only one tip for the future, sunscreen "
            "would be it.";
    const byte_t aad[] = {0x50, 0x51, 0x52, 0x53, 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7};
    const byte_t expected_ct[] = {
            0xd3, 0x1a, 0x8d, 0x34, 0x64, 0x8e, 0x60, 0xdb, 0x7b, 0x86, 0xaf, 0xbc, 0x53, 0xef, 0x7e, 0xc2, 0xa4,
            0xad, 0xed, 0x51, 0x29, 0x6e, 0x08, 0xfe, 0xa9, 0xe2, 0xb5, 0xa7, 0x36, 0xee, 0x62, 0xd6, 0x3d, 0xbe,
            0xa4, 0x5e, 0x8c, 0xa9, 0x67, 0x12, 0x82, 0xfa, 0xfb, 0x69, 0xda, 0x92, 0x72, 0x8b, 0x1a, 0x71, 0xde,
            0x0a, 0x9e, 0x06, 0x0b, 0x29, 0x05, 0xd6, 0xa5, 0xb6, 0x7e, 0xcd, 0x3b, 0x36, 0x92, 0xdd, 0xbd, 0x7f,
            0x2d, 0x77, 0x8b, 0x8c, 0x98, 0x03, 0xae, 0xe3, 0x28, 0x09, 0x1b, 0x58, 0xfa, 0xb3, 0x24, 0xe4, 0xfa,
            0xd6, 0x75, 0x94, 0x55, 0x85, 0x80, 0x8b, 0x48, 0x31, 0xd7, 0xbc, 0x3f, 0xf4, 0xde, 0xf0, 0x8e, 0x4b,
            0x7a, 0x9d, 0xe5, 0x76, 0xd2, 0x65, 0x86, 0xce, 0xc6, 0x4b, 0x61, 0x16};
    const byte_t expected_tag[] = {0x1a, 0xe1, 0x0b, 0x59, 0x4f, 0x09, 0xe2, 0x6a,
                                   0x7e, 0x90, 0x2e, 0xcb, 0xd0, 0x60, 0x06, 0x91};
    const size_t pt_len = char_traits<char>::length(plaintext_str);

    byte_t tag[16];
    auto encrypted = chacha20_poly1305::encrypt(cbyte_view{reinterpret_cast<const byte_t*>(plaintext_str), pt_len},
                                                cbyte_view{key_bytes, 32}, cbyte_view{nonce_bytes, 12},
                                                cbyte_view{aad, 12}, tag);

    ASSERT_EQ(encrypted.size(), pt_len);
    for (size_t i = 0; i < pt_len; ++i) {
        EXPECT_EQ(static_cast<unsigned>(encrypted[i]), static_cast<unsigned>(expected_ct[i])) << "at index " << i;
    }
    for (size_t i = 0; i < 16; ++i) {
        EXPECT_EQ(static_cast<unsigned>(tag[i]), static_cast<unsigned>(expected_tag[i])) << "at tag index " << i;
    }
}

TEST_F(ChaCha20Poly1305Test, DecryptRfc8439TestVector) {
    const byte_t key_bytes[] = {0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a,
                                0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 0x90, 0x91, 0x92, 0x93, 0x94, 0x95,
                                0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f};
    const byte_t nonce_bytes[] = {0x07, 0x00, 0x00, 0x00, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47};
    const char* expected_plaintext =
            "Ladies and Gentlemen of the class of '99: If I could offer you only one tip for the future, sunscreen "
            "would be it.";
    const byte_t aad[] = {0x50, 0x51, 0x52, 0x53, 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7};
    const byte_t ciphertext[] = {
            0xd3, 0x1a, 0x8d, 0x34, 0x64, 0x8e, 0x60, 0xdb, 0x7b, 0x86, 0xaf, 0xbc, 0x53, 0xef, 0x7e, 0xc2, 0xa4,
            0xad, 0xed, 0x51, 0x29, 0x6e, 0x08, 0xfe, 0xa9, 0xe2, 0xb5, 0xa7, 0x36, 0xee, 0x62, 0xd6, 0x3d, 0xbe,
            0xa4, 0x5e, 0x8c, 0xa9, 0x67, 0x12, 0x82, 0xfa, 0xfb, 0x69, 0xda, 0x92, 0x72, 0x8b, 0x1a, 0x71, 0xde,
            0x0a, 0x9e, 0x06, 0x0b, 0x29, 0x05, 0xd6, 0xa5, 0xb6, 0x7e, 0xcd, 0x3b, 0x36, 0x92, 0xdd, 0xbd, 0x7f,
            0x2d, 0x77, 0x8b, 0x8c, 0x98, 0x03, 0xae, 0xe3, 0x28, 0x09, 0x1b, 0x58, 0xfa, 0xb3, 0x24, 0xe4, 0xfa,
            0xd6, 0x75, 0x94, 0x55, 0x85, 0x80, 0x8b, 0x48, 0x31, 0xd7, 0xbc, 0x3f, 0xf4, 0xde, 0xf0, 0x8e, 0x4b,
            0x7a, 0x9d, 0xe5, 0x76, 0xd2, 0x65, 0x86, 0xce, 0xc6, 0x4b, 0x61, 0x16};
    const byte_t tag[] = {0x1a, 0xe1, 0x0b, 0x59, 0x4f, 0x09, 0xe2, 0x6a,
                          0x7e, 0x90, 0x2e, 0xcb, 0xd0, 0x60, 0x06, 0x91};
    const size_t ct_len = sizeof(ciphertext);
    const size_t pt_len = char_traits<char>::length(expected_plaintext);

    auto decrypted = chacha20_poly1305::decrypt(cbyte_view{ciphertext, ct_len}, cbyte_view{key_bytes, 32},
                                                cbyte_view{nonce_bytes, 12}, cbyte_view{aad, 12}, cbyte_view{tag, 16});

    ASSERT_EQ(decrypted.size(), pt_len);
    for (size_t i = 0; i < pt_len; ++i) {
        EXPECT_EQ(static_cast<unsigned>(decrypted[i]),
                  static_cast<unsigned>(static_cast<byte_t>(expected_plaintext[i])))
                << "at index " << i;
    }
}

TEST_F(ChaCha20Poly1305Test, EncryptDecryptRoundtrip) {
    const char* plaintext_str = "Hello, ChaCha20-Poly1305!";
    const size_t pt_len = char_traits<char>::length(plaintext_str);
    cbyte_view data{reinterpret_cast<const byte_t*>(plaintext_str), pt_len};

    byte_t tag[16];
    auto ciphertext = chacha20_poly1305::encrypt(data, key_32.view(), nonce_12.view(), {}, tag);

    auto decrypted =
            chacha20_poly1305::decrypt(ciphertext.view(), key_32.view(), nonce_12.view(), {}, cbyte_view{tag, 16});

    ASSERT_EQ(decrypted.size(), pt_len);
    for (size_t i = 0; i < pt_len; ++i) {
        EXPECT_EQ(decrypted[i], static_cast<byte_t>(plaintext_str[i]));
    }
}

TEST_F(ChaCha20Poly1305Test, EncryptDecryptRoundtripWithAad) {
    const char* plaintext_str = "Data protected with associated data";
    const size_t pt_len = char_traits<char>::length(plaintext_str);
    const byte_t aad[] = {0xfe, 0xed, 0xfa, 0xce, 0xde, 0xad, 0xbe, 0xef};
    cbyte_view data{reinterpret_cast<const byte_t*>(plaintext_str), pt_len};

    byte_t tag[16];
    auto ciphertext = chacha20_poly1305::encrypt(data, key_32.view(), nonce_12.view(), cbyte_view{aad, 8}, tag);

    auto decrypted = chacha20_poly1305::decrypt(ciphertext.view(), key_32.view(), nonce_12.view(), cbyte_view{aad, 8},
                                                cbyte_view{tag, 16});

    ASSERT_EQ(decrypted.size(), pt_len);
    for (size_t i = 0; i < pt_len; ++i) {
        EXPECT_EQ(decrypted[i], static_cast<byte_t>(plaintext_str[i]));
    }
}

TEST_F(ChaCha20Poly1305Test, EncryptEmptyPlaintext) {
    byte_t tag[16];
    auto ciphertext = chacha20_poly1305::encrypt({}, key_32.view(), nonce_12.view(), {}, tag);

    EXPECT_TRUE(ciphertext.empty());

    auto decrypted =
            chacha20_poly1305::decrypt(ciphertext.view(), key_32.view(), nonce_12.view(), {}, cbyte_view{tag, 16});
    EXPECT_TRUE(decrypted.empty());
}

TEST_F(ChaCha20Poly1305Test, EncryptEmptyAad) {
    const byte_t plaintext[] = {0x01, 0x02, 0x03, 0x04, 0x05};

    byte_t tag[16];
    auto ciphertext = chacha20_poly1305::encrypt(cbyte_view{plaintext, 5}, key_32.view(), nonce_12.view(), {}, tag);

    ASSERT_EQ(ciphertext.size(), 5);

    auto decrypted =
            chacha20_poly1305::decrypt(ciphertext.view(), key_32.view(), nonce_12.view(), {}, cbyte_view{tag, 16});

    ASSERT_EQ(decrypted.size(), 5);
    for (size_t i = 0; i < 5; ++i) {
        EXPECT_EQ(decrypted[i], plaintext[i]);
    }
}

TEST_F(ChaCha20Poly1305Test, EncryptEmptyBoth) {
    byte_t tag[16];
    auto ciphertext = chacha20_poly1305::encrypt({}, key_32.view(), nonce_12.view(), {}, tag);

    EXPECT_TRUE(ciphertext.empty());

    auto decrypted =
            chacha20_poly1305::decrypt(ciphertext.view(), key_32.view(), nonce_12.view(), {}, cbyte_view{tag, 16});
    EXPECT_TRUE(decrypted.empty());
}

TEST_F(ChaCha20Poly1305Test, DecryptTamperedCiphertextThrows) {
    const byte_t plaintext[] = {0x01, 0x02, 0x03, 0x04, 0x05};

    byte_t tag[16];
    auto ciphertext = chacha20_poly1305::encrypt(cbyte_view{plaintext, 5}, key_32.view(), nonce_12.view(), {}, tag);

    byte_vector tampered(ciphertext.begin(), ciphertext.end());
    tampered[0] ^= 0x01;

    EXPECT_THROW(
            { chacha20_poly1305::decrypt(tampered.view(), key_32.view(), nonce_12.view(), {}, cbyte_view{tag, 16}); },
            value_exception);
}

TEST_F(ChaCha20Poly1305Test, DecryptTamperedTagThrows) {
    const byte_t plaintext[] = {0x01, 0x02, 0x03, 0x04, 0x05};

    byte_t tag[16];
    auto ciphertext = chacha20_poly1305::encrypt(cbyte_view{plaintext, 5}, key_32.view(), nonce_12.view(), {}, tag);

    byte_t wrong_tag[16];
    for (size_t i = 0; i < 16; ++i) {
        wrong_tag[i] = tag[i];
    }
    wrong_tag[15] ^= 0x01;

    EXPECT_THROW(
            {
                chacha20_poly1305::decrypt(ciphertext.view(), key_32.view(), nonce_12.view(), {},
                                           cbyte_view{wrong_tag, 16});
            },
            value_exception);
}

TEST_F(ChaCha20Poly1305Test, DecryptTamperedAadThrows) {
    const byte_t plaintext[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    const byte_t aad[] = {0xaa, 0xbb, 0xcc, 0xdd};

    byte_t tag[16];
    auto ciphertext = chacha20_poly1305::encrypt(cbyte_view{plaintext, 5}, key_32.view(), nonce_12.view(),
                                                 cbyte_view{aad, 4}, tag);

    const byte_t wrong_aad[] = {0xaa, 0xbb, 0xcc, 0xde};

    EXPECT_THROW(
            {
                chacha20_poly1305::decrypt(ciphertext.view(), key_32.view(), nonce_12.view(), cbyte_view{wrong_aad, 4},
                                           cbyte_view{tag, 16});
            },
            value_exception);
}

TEST_F(ChaCha20Poly1305Test, EncryptLargeData) {
    byte_vector plaintext(1024);
    for (size_t i = 0; i < 1024; ++i) {
        plaintext[i] = static_cast<byte_t>(i & 0xFF);
    }

    byte_t tag[16];
    auto ciphertext = chacha20_poly1305::encrypt(plaintext.view(), key_32.view(), nonce_12.view(), {}, tag);

    ASSERT_EQ(ciphertext.size(), 1024);

    auto decrypted =
            chacha20_poly1305::decrypt(ciphertext.view(), key_32.view(), nonce_12.view(), {}, cbyte_view{tag, 16});

    ASSERT_EQ(decrypted.size(), 1024);
    for (size_t i = 0; i < 1024; ++i) {
        EXPECT_EQ(decrypted[i], plaintext[i]) << "at index " << i;
    }
}

TEST_F(ChaCha20Poly1305Test, EncryptDeterministic) {
    const byte_t plaintext[] = {0x01, 0x02, 0x03, 0x04, 0x05};

    byte_t tag1[16], tag2[16];
    auto ct1 = chacha20_poly1305::encrypt(cbyte_view{plaintext, 5}, key_32.view(), nonce_12.view(), {}, tag1);
    auto ct2 = chacha20_poly1305::encrypt(cbyte_view{plaintext, 5}, key_32.view(), nonce_12.view(), {}, tag2);

    ASSERT_EQ(ct1.size(), ct2.size());
    for (size_t i = 0; i < ct1.size(); ++i) {
        EXPECT_EQ(ct1[i], ct2[i]);
    }
    for (size_t i = 0; i < 16; ++i) {
        EXPECT_EQ(tag1[i], tag2[i]);
    }
}

TEST_F(ChaCha20Poly1305Test, EncryptInvalidKeySizeThrows) {
    const byte_t plaintext[] = {0x01, 0x02, 0x03};
    const byte_t bad_key[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
                              0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                              0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e};
    byte_t tag[16];

    EXPECT_THROW(
            {
                chacha20_poly1305::encrypt(cbyte_view{plaintext, 3}, cbyte_view{bad_key, 31}, nonce_12.view(), {}, tag);
            },
            value_exception);
}

TEST_F(ChaCha20Poly1305Test, EncryptInvalidNonceSizeThrows) {
    const byte_t plaintext[] = {0x01, 0x02, 0x03};
    const byte_t bad_nonce[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
    byte_t tag[16];

    EXPECT_THROW(
            { chacha20_poly1305::encrypt(cbyte_view{plaintext, 3}, key_32.view(), cbyte_view{bad_nonce, 8}, {}, tag); },
            value_exception);
}

TEST_F(ChaCha20Poly1305Test, DecryptInvalidTagSizeThrows) {
    const byte_t plaintext[] = {0x01, 0x02, 0x03};

    byte_t tag[16];
    auto ciphertext = chacha20_poly1305::encrypt(cbyte_view{plaintext, 3}, key_32.view(), nonce_12.view(), {}, tag);

    EXPECT_THROW(
            { chacha20_poly1305::decrypt(ciphertext.view(), key_32.view(), nonce_12.view(), {}, cbyte_view{tag, 12}); },
            value_exception);
}

TEST_F(ChaCha20Poly1305Test, DecryptShortCiphertextValidTag) {
    const byte_t plaintext[] = {0x42};

    byte_t tag[16];
    auto ciphertext = chacha20_poly1305::encrypt(cbyte_view{plaintext, 1}, key_32.view(), nonce_12.view(), {}, tag);

    ASSERT_EQ(ciphertext.size(), 1);

    auto decrypted =
            chacha20_poly1305::decrypt(ciphertext.view(), key_32.view(), nonce_12.view(), {}, cbyte_view{tag, 16});

    ASSERT_EQ(decrypted.size(), 1);
    EXPECT_EQ(decrypted[0], plaintext[0]);
}

TEST_F(ChaCha20Poly1305Test, EncryptNullTagThrows) {
    const byte_t plaintext[] = {0x01, 0x02, 0x03};

    EXPECT_THROW(
            { chacha20_poly1305::encrypt(cbyte_view{plaintext, 3}, key_32.view(), nonce_12.view(), {}, nullptr); },
            value_exception);
}

TEST_F(ChaCha20Poly1305Test, DecryptWrongAadLength) {
    const byte_t plaintext[] = {0x01, 0x02, 0x03, 0x04};
    const byte_t aad[] = {0xaa, 0xbb, 0xcc, 0xdd};

    byte_t tag[16];
    auto ciphertext = chacha20_poly1305::encrypt(cbyte_view{plaintext, 4}, key_32.view(), nonce_12.view(),
                                                 cbyte_view{aad, 4}, tag);

    const byte_t different_aad[] = {0xaa, 0xbb};

    EXPECT_THROW(
            {
                chacha20_poly1305::decrypt(ciphertext.view(), key_32.view(), nonce_12.view(),
                                           cbyte_view{different_aad, 2}, cbyte_view{tag, 16});
            },
            value_exception);
}
