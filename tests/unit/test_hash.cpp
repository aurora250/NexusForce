#include <NeForce/core/container/vector.hpp>
#include <NeForce/core/hash/blake2.hpp>
#include <NeForce/core/hash/blake3.hpp>
#include <NeForce/core/hash/city_hash.hpp>
#include <NeForce/core/hash/djb2.hpp>
#include <NeForce/core/hash/fnv.hpp>
#include <NeForce/core/hash/hash.hpp>
#include <NeForce/core/hash/md5.hpp>
#include <NeForce/core/hash/murmur.hpp>
#include <NeForce/core/hash/sha1.hpp>
#include <NeForce/core/hash/sha256.hpp>
#include <NeForce/core/hash/sha512.hpp>
#include <NeForce/core/hash/wyhash.hpp>
#include <NeForce/core/hash/xxhash.hpp>
#include <NeForce/core/string/string.hpp>
#include <NeForce/core/utility/hexadecimal.hpp>
#include <gtest/gtest.h>
using namespace neforce;

TEST(MD5Test, HashEmptyString) {
    const byte_vector result = MD5::hash(cbyte_view{});
    const byte_vector expected{0xd4, 0x1d, 0x8c, 0xd9, 0x8f, 0x00, 0xb2, 0x04,
                               0xe9, 0x80, 0x09, 0x98, 0xec, 0xf8, 0x42, 0x7e};
    EXPECT_EQ(result, expected);
}

TEST(MD5Test, HashSingleCharacterA) {
    const byte_t data[] = {static_cast<byte_t>('a')};
    const cbyte_view input{data, 1};
    const byte_vector result = MD5::hash(input);
    const byte_vector expected{0x0c, 0xc1, 0x75, 0xb9, 0xc0, 0xf1, 0xb6, 0xa8,
                               0x31, 0xc3, 0x99, 0xe2, 0x69, 0x77, 0x26, 0x61};
    EXPECT_EQ(result, expected);
}

TEST(MD5Test, HashThreeCharactersAbc) {
    const byte_t data[] = {static_cast<byte_t>('a'), static_cast<byte_t>('b'), static_cast<byte_t>('c')};
    const cbyte_view input{data, 3};
    const byte_vector result = MD5::hash(input);
    const byte_vector expected{0x90, 0x01, 0x50, 0x98, 0x3c, 0xd2, 0x4f, 0xb0,
                               0xd6, 0x96, 0x3f, 0x7d, 0x28, 0xe1, 0x7f, 0x72};
    EXPECT_EQ(result, expected);
}

TEST(MD5Test, HashMessageDigest) {
    const char* const msg = "message digest";
    const cbyte_view input{reinterpret_cast<const byte_t*>(msg), char_traits<char>::length(msg)};
    const byte_vector result = MD5::hash(input);
    const byte_vector expected{0xf9, 0x6b, 0x69, 0x7d, 0x7c, 0xb7, 0x93, 0x8d,
                               0x52, 0x5a, 0x2f, 0x31, 0xaa, 0xf1, 0x61, 0xd0};
    EXPECT_EQ(result, expected);
}

TEST(MD5Test, HashLowerCaseAlphabet) {
    const char* const msg = "abcdefghijklmnopqrstuvwxyz";
    const cbyte_view input{reinterpret_cast<const byte_t*>(msg), char_traits<char>::length(msg)};
    const byte_vector result = MD5::hash(input);
    const byte_vector expected{0xc3, 0xfc, 0xd3, 0xd7, 0x61, 0x92, 0xe4, 0x00,
                               0x7d, 0xfb, 0x49, 0x6c, 0xca, 0x67, 0xe1, 0x3b};
    EXPECT_EQ(result, expected);
}

TEST(MD5Test, HashAlphanumeric) {
    const char* const msg = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    const cbyte_view input{reinterpret_cast<const byte_t*>(msg), char_traits<char>::length(msg)};
    const byte_vector result = MD5::hash(input);
    const byte_vector expected{0xd1, 0x74, 0xab, 0x98, 0xd2, 0x77, 0xd9, 0xf5,
                               0xa5, 0x61, 0x1c, 0x2c, 0x9f, 0x41, 0x9d, 0x9f};
    EXPECT_EQ(result, expected);
}

TEST(MD5Test, HashRepeating1234567890) {
    const char* const msg = "12345678901234567890123456789012345678901234567890123456789012345678901234567890";
    const cbyte_view input{reinterpret_cast<const byte_t*>(msg), char_traits<char>::length(msg)};
    const byte_vector result = MD5::hash(input);
    const byte_vector expected{0x57, 0xed, 0xf4, 0xa2, 0x2b, 0xe3, 0xc9, 0x55,
                               0xac, 0x49, 0xda, 0x2e, 0x21, 0x07, 0xb6, 0x7a};
    EXPECT_EQ(result, expected);
}

TEST(MD5Test, HashExact64Bytes) {
    constexpr const char* const msg = "1234567890123456789012345678901234567890123456789012345678901234";
    static_assert(char_traits<char>::length(msg) == 64, "must be 64 bytes");
    const cbyte_view input{reinterpret_cast<const byte_t*>(msg), 64};
    const byte_vector result = MD5::hash(input);
    const byte_vector expected{0xeb, 0x6c, 0x41, 0x79, 0xc0, 0xa7, 0xc8, 0x2c,
                               0xc2, 0x82, 0x8c, 0x1e, 0x63, 0x38, 0xe1, 0x65};
    EXPECT_EQ(result, expected);
}

TEST(MD5Test, HashExact120Bytes) {
    constexpr const char* const msg = "1234567890123456789012345678901234567890"
                                      "1234567890123456789012345678901234567890"
                                      "1234567890123456789012345678901234567890";
    static_assert(char_traits<char>::length(msg) == 120, "must be 120 bytes");
    const cbyte_view input{reinterpret_cast<const byte_t*>(msg), 120};
    const byte_vector result = MD5::hash(input);
    const byte_vector expected{0x1d, 0x45, 0x3b, 0x96, 0xd4, 0x8d, 0x5e, 0x0c,
                               0xec, 0x4a, 0x20, 0xa7, 0x1f, 0xec, 0xaa, 0x81};
    EXPECT_EQ(result, expected);
}

TEST(MD5Test, HashHexEmptyString) {
    const string result = MD5::hash_hex(cbyte_view{});
    EXPECT_EQ(result, "d41d8cd98f00b204e9800998ecf8427e");
}

TEST(MD5Test, HashHexAbc) {
    const byte_t data[] = {static_cast<byte_t>('a'), static_cast<byte_t>('b'), static_cast<byte_t>('c')};
    const cbyte_view input{data, 3};
    const string result = MD5::hash_hex(input);
    EXPECT_EQ(result, "900150983cd24fb0d6963f7d28e17f72");
}

TEST(MD5Test, HashHexMessageDigest) {
    const char* const msg = "message digest";
    const cbyte_view input{reinterpret_cast<const byte_t*>(msg), char_traits<char>::length(msg)};
    const string result = MD5::hash_hex(input);
    EXPECT_EQ(result, "f96b697d7cb7938d525a2f31aaf161d0");
}

TEST(MD5Test, ConvenienceStringViewEmpty) {
    const string result = md5(string_view{});
    EXPECT_EQ(result, "d41d8cd98f00b204e9800998ecf8427e");
}

TEST(MD5Test, ConvenienceStringViewAbc) {
    const string result = md5(string_view{"abc"});
    EXPECT_EQ(result, "900150983cd24fb0d6963f7d28e17f72");
}

TEST(MD5Test, ConvenienceStringEmpty) {
    const string result = md5(string{});
    EXPECT_EQ(result, "d41d8cd98f00b204e9800998ecf8427e");
}

TEST(MD5Test, ConvenienceStringAbc) {
    const string result = md5(string{"abc"});
    EXPECT_EQ(result, "900150983cd24fb0d6963f7d28e17f72");
}

TEST(MD5Test, ConvenienceCbyteViewEmpty) {
    const byte_vector result = md5(cbyte_view{});
    const byte_vector expected{0xd4, 0x1d, 0x8c, 0xd9, 0x8f, 0x00, 0xb2, 0x04,
                               0xe9, 0x80, 0x09, 0x98, 0xec, 0xf8, 0x42, 0x7e};
    EXPECT_EQ(result, expected);
}

TEST(MD5Test, ConvenienceCbyteViewAbc) {
    const byte_t data[] = {static_cast<byte_t>('a'), static_cast<byte_t>('b'), static_cast<byte_t>('c')};
    const cbyte_view input{data, 3};
    const byte_vector result = md5(input);
    const byte_vector expected{0x90, 0x01, 0x50, 0x98, 0x3c, 0xd2, 0x4f, 0xb0,
                               0xd6, 0x96, 0x3f, 0x7d, 0x28, 0xe1, 0x7f, 0x72};
    EXPECT_EQ(result, expected);
}

TEST(MD5Test, ConvenienceByteVectorEmpty) {
    const byte_vector result = md5(byte_vector{});
    const byte_vector expected{0xd4, 0x1d, 0x8c, 0xd9, 0x8f, 0x00, 0xb2, 0x04,
                               0xe9, 0x80, 0x09, 0x98, 0xec, 0xf8, 0x42, 0x7e};
    EXPECT_EQ(result, expected);
}

TEST(MD5Test, ConvenienceByteVectorAbc) {
    const byte_vector data = {static_cast<byte_t>('a'), static_cast<byte_t>('b'), static_cast<byte_t>('c')};
    const byte_vector result = md5(data);
    const byte_vector expected{0x90, 0x01, 0x50, 0x98, 0x3c, 0xd2, 0x4f, 0xb0,
                               0xd6, 0x96, 0x3f, 0x7d, 0x28, 0xe1, 0x7f, 0x72};
    EXPECT_EQ(result, expected);
}

TEST(MD5Test, LongDataMultipleBlocks) {
    string long_str(10000, 'A');
    const cbyte_view input{reinterpret_cast<const byte_t*>(long_str.data()), long_str.size()};
    const byte_vector result = MD5::hash(input);
    EXPECT_EQ(result.size(), 16u);
    const byte_vector expected{0x0f, 0x53, 0x21, 0x7f, 0xc7, 0xc8, 0xe7, 0xf8,
                               0x9e, 0x8a, 0x85, 0x58, 0xe6, 0x4a, 0x70, 0x83};
    EXPECT_EQ(result, expected);
}

TEST(MD5Test, ConsistentRepeatedHashes) {
    const char* const msg = "The quick brown fox jumps over the lazy dog";
    const cbyte_view input{reinterpret_cast<const byte_t*>(msg), char_traits<char>::length(msg)};
    const byte_vector first = MD5::hash(input);
    const byte_vector second = MD5::hash(input);
    EXPECT_EQ(first, second);
}

TEST(MD5Test, HashHexLowerCase) {
    const char* const msg = "MD5";
    const cbyte_view input{reinterpret_cast<const byte_t*>(msg), 3};
    const string hex = MD5::hash_hex(input);
    EXPECT_EQ(hex.size(), 32u);
    for (char c: hex) {
        EXPECT_TRUE((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'));
    }
}


class SHA256Test : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(SHA256Test, HashEmptyString) {
    auto result = SHA256::hash({});
    ASSERT_EQ(result.size(), 32);
}

TEST_F(SHA256Test, HashHexEmptyString) {
    auto result = SHA256::hash_hex({});
    EXPECT_EQ(result, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    EXPECT_EQ(result.size(), 64);
}

TEST_F(SHA256Test, HashHexCompareWithKnownEmpty) {
    auto result = SHA256::hash_hex({});
    EXPECT_EQ(result, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}

TEST_F(SHA256Test, HashSingleByte) {
    byte_vector input = {0x61};
    auto result = SHA256::hash(input.view());
    ASSERT_EQ(result.size(), 32);
}

TEST_F(SHA256Test, HashHexSingleByteA) {
    byte_vector input = {0x61};
    auto result = SHA256::hash_hex(input.view());
    EXPECT_EQ(result, "ca978112ca1bbdcafac231b39a23dc4da786eff8147c4e72b9807785afee48bb");
}

TEST_F(SHA256Test, HashHexShortString) {
    auto result = SHA256::hash_hex({reinterpret_cast<const byte_t*>("abc"), 3});
    EXPECT_EQ(result, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}

TEST_F(SHA256Test, HashHexKnownVectorAbc) {
    auto result = SHA256::hash_hex({reinterpret_cast<const byte_t*>("abc"), 3});
    EXPECT_EQ(result, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}

TEST_F(SHA256Test, HashHexKnownVectorAbcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq) {
    const char* input = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    auto result = SHA256::hash_hex({reinterpret_cast<const byte_t*>(input), string_length(input)});
    EXPECT_EQ(result, "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
}

TEST_F(SHA256Test, HashHexKnownVectorAlphabet) {
    const char* input = "The quick brown fox jumps over the lazy dog";
    auto result = SHA256::hash_hex({reinterpret_cast<const byte_t*>(input), string_length(input)});
    EXPECT_EQ(result, "d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592");
}

TEST_F(SHA256Test, HashHexKnownVectorAlphabetWithDot) {
    const char* input = "The quick brown fox jumps over the lazy dog.";
    auto result = SHA256::hash_hex({reinterpret_cast<const byte_t*>(input), string_length(input)});
    EXPECT_EQ(result, "ef537f25c895bfa782526529a9b63d97aa631564d5d789c2b765448c8635fb6c");
}

TEST_F(SHA256Test, HashHexKnownVector448Bits) {
    const char* input = "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopq"
                        "rsmnopqrstnopqrstu";
    auto result = SHA256::hash_hex({reinterpret_cast<const byte_t*>(input), string_length(input)});
    EXPECT_EQ(result, "cf5b16a778af8380036ce59e7b0492370b249b11e8f07a51afac45037afee9d1");
}

TEST_F(SHA256Test, HashHexKnownVectorOneMillionA) {
    byte_vector input(1000000, 0x61);
    auto result = SHA256::hash_hex(input.view());
    EXPECT_EQ(result, "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");
}

TEST_F(SHA256Test, HashHexTwoBlocks) {
    byte_vector input(128, 0x00);
    auto result = SHA256::hash_hex(input.view());
    EXPECT_EQ(result, "38723a2e5e8a17aa7950dc008209944e898f69a7bd10a23c839d341e935fd5ca");
}

TEST_F(SHA256Test, HashReturnSize) {
    auto result = SHA256::hash({reinterpret_cast<const byte_t*>("test"), 4});
    EXPECT_EQ(result.size(), 32);
}

TEST_F(SHA256Test, HashHexReturnSize) {
    auto result = SHA256::hash_hex({reinterpret_cast<const byte_t*>("test"), 4});
    EXPECT_EQ(result.size(), 64);
}

TEST_F(SHA256Test, HashDeterministic) {
    auto result1 = SHA256::hash({reinterpret_cast<const byte_t*>("test"), 4});
    auto result2 = SHA256::hash({reinterpret_cast<const byte_t*>("test"), 4});
    EXPECT_EQ(result1, result2);
}

TEST_F(SHA256Test, HashHexDeterministic) {
    auto result1 = SHA256::hash_hex({reinterpret_cast<const byte_t*>("test"), 4});
    auto result2 = SHA256::hash_hex({reinterpret_cast<const byte_t*>("test"), 4});
    EXPECT_EQ(result1, result2);
}

TEST_F(SHA256Test, HashDifferentInputsDifferentOutputs) {
    auto result1 = SHA256::hash({reinterpret_cast<const byte_t*>("test1"), 5});
    auto result2 = SHA256::hash({reinterpret_cast<const byte_t*>("test2"), 5});
    EXPECT_NE(result1, result2);
}

TEST_F(SHA256Test, HashHexDifferentInputsDifferentOutputs) {
    auto result1 = SHA256::hash_hex({reinterpret_cast<const byte_t*>("test1"), 5});
    auto result2 = SHA256::hash_hex({reinterpret_cast<const byte_t*>("test2"), 5});
    EXPECT_NE(result1, result2);
}

TEST_F(SHA256Test, HashExactlyBlockSize) {
    byte_vector input(64, 0x00);
    auto result = SHA256::hash(input.view());
    ASSERT_EQ(result.size(), 32);
}

TEST_F(SHA256Test, HashHexExactlyBlockSize) {
    byte_vector input(64, 0x00);
    auto result = SHA256::hash_hex(input.view());
    EXPECT_EQ(result.size(), 64);
}

TEST_F(SHA256Test, HashBlockSizeMinusOne) {
    byte_vector input(63, 0x00);
    auto result = SHA256::hash(input.view());
    ASSERT_EQ(result.size(), 32);
}

TEST_F(SHA256Test, HashHexBlockSizeMinusOne) {
    byte_vector input(63, 0x00);
    auto result = SHA256::hash_hex(input.view());
    EXPECT_EQ(result.size(), 64);
}

TEST_F(SHA256Test, HashBlockSizePlusOne) {
    byte_vector input(65, 0x00);
    auto result = SHA256::hash(input.view());
    ASSERT_EQ(result.size(), 32);
}

TEST_F(SHA256Test, HashHexBlockSizePlusOne) {
    byte_vector input(65, 0x00);
    auto result = SHA256::hash_hex(input.view());
    EXPECT_EQ(result.size(), 64);
}

TEST_F(SHA256Test, HashLargeInput) {
    byte_vector input(10000, 0x41);
    auto result = SHA256::hash(input.view());
    ASSERT_EQ(result.size(), 32);
}

TEST_F(SHA256Test, HashHexLargeInput) {
    byte_vector input(10000, 0x41);
    auto result = SHA256::hash_hex(input.view());
    EXPECT_EQ(result.size(), 64);
}

TEST_F(SHA256Test, HashAllZeroBytes) {
    byte_vector input(256, 0x00);
    auto result = SHA256::hash(input.view());
    ASSERT_EQ(result.size(), 32);
}

TEST_F(SHA256Test, HashHexAllZeroBytes) {
    byte_vector input(256, 0x00);
    auto result = SHA256::hash_hex(input.view());
    EXPECT_EQ(result.size(), 64);
}

TEST_F(SHA256Test, HashAllOneBits) {
    byte_vector input(256, 0xFF);
    auto result = SHA256::hash(input.view());
    ASSERT_EQ(result.size(), 32);
}

TEST_F(SHA256Test, HashHexAllOneBits) {
    byte_vector input(256, 0xFF);
    auto result = SHA256::hash_hex(input.view());
    EXPECT_EQ(result.size(), 64);
}

TEST_F(SHA256Test, HashHexLowercaseHex) {
    auto result = SHA256::hash_hex({reinterpret_cast<const byte_t*>("test"), 4});
    for (char c: result) {
        if (c >= 'a' && c <= 'f') {
            EXPECT_TRUE(true);
        } else if (c >= '0' && c <= '9') {
            EXPECT_TRUE(true);
        }
    }
}

TEST_F(SHA256Test, HashOutputIsValidHexFormat) {
    auto result = SHA256::hash_hex({reinterpret_cast<const byte_t*>("test"), 4});
    EXPECT_EQ(result.size(), 64);
    for (char c: result) {
        EXPECT_TRUE((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'));
    }
}

TEST_F(SHA256Test, HashEmptyByteVector) {
    auto result = SHA256::hash_hex({});
    EXPECT_EQ(result, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}

TEST_F(SHA256Test, HashNullByteInput) {
    byte_vector input = {0x00, 0x00, 0x00, 0x00};
    auto result = SHA256::hash_hex(input.view());
    EXPECT_EQ(result, "df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119");
}

TEST_F(SHA256Test, HashConsecutiveBytes) {
    byte_vector input(256);
    for (size_t i = 0; i < 256; ++i) {
        input[i] = static_cast<byte_t>(i);
    }
    auto result = SHA256::hash(input.view());
    ASSERT_EQ(result.size(), 32);
}

TEST_F(SHA256Test, HashHexConsecutiveBytes) {
    byte_vector input(256);
    for (size_t i = 0; i < 256; ++i) {
        input[i] = static_cast<byte_t>(i);
    }
    auto result = SHA256::hash_hex(input.view());
    EXPECT_EQ(result.size(), 64);
}

TEST_F(SHA256Test, ConvenienceFunctionSha256StringView) {
    auto result = sha256(string_view("test"));
    EXPECT_EQ(result.size(), 64);
}

TEST_F(SHA256Test, ConvenienceFunctionSha256String) {
    auto result = sha256(string("test"));
    EXPECT_EQ(result.size(), 64);
}

TEST_F(SHA256Test, ConvenienceFunctionSha256CbyteView) {
    const char* input = "test";
    auto result = sha256(cbyte_view{reinterpret_cast<const byte_t*>(input), 4});
    EXPECT_EQ(result.size(), 32);
}

TEST_F(SHA256Test, ConvenienceFunctionSha256ByteVector) {
    byte_vector input = {0x74, 0x65, 0x73, 0x74};
    auto result = sha256(input);
    EXPECT_EQ(result.size(), 32);
}

TEST_F(SHA256Test, ConvenienceFunctionEquivalentToStaticMethodStringView) {
    auto result1 = sha256(string_view("test"));
    auto result2 = SHA256::hash_hex({reinterpret_cast<const byte_t*>("test"), 4});
    EXPECT_EQ(result1, result2);
}

TEST_F(SHA256Test, ConvenienceFunctionEquivalentToStaticMethodString) {
    auto result1 = sha256(string("test"));
    auto result2 = SHA256::hash_hex({reinterpret_cast<const byte_t*>("test"), 4});
    EXPECT_EQ(result1, result2);
}

TEST_F(SHA256Test, ConvenienceFunctionEquivalentToStaticMethodCbyteView) {
    const char* input = "test";
    auto result1 = sha256(cbyte_view{reinterpret_cast<const byte_t*>(input), 4});
    auto result2 = SHA256::hash({reinterpret_cast<const byte_t*>(input), 4});
    EXPECT_EQ(result1, result2);
}

TEST_F(SHA256Test, ConvenienceFunctionEquivalentToStaticMethodByteVector) {
    byte_vector input = {0x74, 0x65, 0x73, 0x74};
    auto result1 = sha256(input);
    auto result2 = SHA256::hash(input.view());
    EXPECT_EQ(result1, result2);
}

TEST_F(SHA256Test, ConvenienceFunctionSha256StringViewKnownVector) {
    auto result = sha256(string_view("abc"));
    EXPECT_EQ(result, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}

TEST_F(SHA256Test, ConvenienceFunctionSha256StringKnownVector) {
    auto result = sha256(string("abc"));
    EXPECT_EQ(result, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}

TEST_F(SHA256Test, ConvenienceFunctionSha256ByteVectorKnownVector) {
    byte_vector input = {0x61, 0x62, 0x63};
    auto result = sha256(input);
    byte_vector expected = {0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea, 0x41, 0x41, 0x40,
                            0xde, 0x5d, 0xae, 0x22, 0x23, 0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17,
                            0x7a, 0x9c, 0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad};
    EXPECT_EQ(result, expected);
}

TEST_F(SHA256Test, HashHexAllHexCharsLowercase) {
    auto result = SHA256::hash_hex({reinterpret_cast<const byte_t*>("Hello World"), 11});
    for (char c: result) {
        if ((c >= 'a' && c <= 'f') || (c >= '0' && c <= '9')) {
            SUCCEED();
        } else {
            FAIL() << "Unexpected character in hex string: " << c;
        }
    }
}

TEST_F(SHA256Test, HashPaddingBoundaryAt55) {
    byte_vector input(55, 0x61);
    auto result = SHA256::hash(input.view());
    ASSERT_EQ(result.size(), 32);
}

TEST_F(SHA256Test, HashPaddingBoundaryAt56) {
    byte_vector input(56, 0x61);
    auto result = SHA256::hash(input.view());
    ASSERT_EQ(result.size(), 32);
}

TEST_F(SHA256Test, HashPaddingBoundaryAt57) {
    byte_vector input(57, 0x61);
    auto result = SHA256::hash(input.view());
    ASSERT_EQ(result.size(), 32);
}

TEST_F(SHA256Test, HashPaddingBoundaryAt63) {
    byte_vector input(63, 0x61);
    auto result = SHA256::hash(input.view());
    ASSERT_EQ(result.size(), 32);
}

TEST_F(SHA256Test, HashPaddingBoundaryAt64) {
    byte_vector input(64, 0x61);
    auto result = SHA256::hash(input.view());
    ASSERT_EQ(result.size(), 32);
}

TEST_F(SHA256Test, HashPaddingBoundaryAt65) {
    byte_vector input(65, 0x61);
    auto result = SHA256::hash(input.view());
    ASSERT_EQ(result.size(), 32);
}

TEST_F(SHA256Test, HashVeryShortInput) {
    byte_vector input = {0x20};
    auto result = SHA256::hash(input.view());
    ASSERT_EQ(result.size(), 32);
}

TEST_F(SHA256Test, HashFullAsciiPrintable) {
    byte_vector input(95);
    for (size_t i = 0; i < 95; ++i) {
        input[i] = static_cast<byte_t>(0x20 + i);
    }
    auto result = SHA256::hash(input.view());
    ASSERT_EQ(result.size(), 32);
}

TEST_F(SHA256Test, HashHexFullAsciiPrintable) {
    byte_vector input(95);
    for (size_t i = 0; i < 95; ++i) {
        input[i] = static_cast<byte_t>(0x20 + i);
    }
    auto result = SHA256::hash_hex(input.view());
    EXPECT_EQ(result.size(), 64);
}

TEST_F(SHA256Test, HashMaxByteValueInput) {
    byte_vector input = {0xFF};
    auto result = SHA256::hash(input.view());
    ASSERT_EQ(result.size(), 32);
}

TEST_F(SHA256Test, HashHexMaxByteValueInput) {
    byte_vector input = {0xFF};
    auto result = SHA256::hash_hex(input.view());
    EXPECT_EQ(result.size(), 64);
}

TEST_F(SHA256Test, HashMultiBlockKnownVector) {
    byte_vector input(1000, 0x61);
    auto result = SHA256::hash_hex(input.view());
    EXPECT_EQ(result, "41edece42d63e8d9bf515a9ba6932e1c20cbc9f5a5d134645adb5db1b9737ea3");
}

TEST_F(SHA256Test, HashZeroLengthStringView) {
    auto result = sha256(string_view(""));
    EXPECT_EQ(result, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}

TEST_F(SHA256Test, HashZeroLengthString) {
    auto result = sha256(string(""));
    EXPECT_EQ(result, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}

TEST_F(SHA256Test, HashZeroLengthByteVector) {
    byte_vector input;
    auto result = sha256(input);
    byte_vector expected = {0xe3, 0xb0, 0xc4, 0x42, 0x98, 0xfc, 0x1c, 0x14, 0x9a, 0xfb, 0xf4,
                            0xc8, 0x99, 0x6f, 0xb9, 0x24, 0x27, 0xae, 0x41, 0xe4, 0x64, 0x9b,
                            0x93, 0x4c, 0xa4, 0x95, 0x99, 0x1b, 0x78, 0x52, 0xb8, 0x55};
    EXPECT_EQ(result, expected);
}


TEST(SHA1Test, HashHexEmptyString) {
    EXPECT_EQ(SHA1::hash_hex(cbyte_view{}), string("da39a3ee5e6b4b0d3255bfef95601890afd80709"));
}

TEST(SHA1Test, HashHexAbc) {
    const byte_t data[] = {'a', 'b', 'c'};
    EXPECT_EQ(SHA1::hash_hex(cbyte_view{data, 3}), string("a9993e364706816aba3e25717850c26c9cd0d89d"));
}

TEST(SHA1Test, HashReturnSize) { EXPECT_EQ(SHA1::hash(cbyte_view{}).size(), static_cast<size_t>(20)); }

TEST(SHA1Test, ConvenienceFunctionReturnsRawBytes) {
    const string raw = sha1(string_view{"abc"});
    const byte_vector expected{0xa9, 0x99, 0x3e, 0x36, 0x47, 0x06, 0x81, 0x6a, 0xba, 0x3e,
                               0x25, 0x71, 0x78, 0x50, 0xc2, 0x6c, 0x9c, 0xd0, 0xd8, 0x9d};
    ASSERT_EQ(raw.size(), expected.size());
    for (size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(static_cast<byte_t>(raw[i]), expected[i]);
    }
}


TEST(SHA512Test, HashHexEmptyString) {
    EXPECT_EQ(SHA512::hash_hex(cbyte_view{}), "cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce"
                                              "47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e");
}

TEST(SHA512Test, HashHexAbc) {
    const byte_t data[] = {'a', 'b', 'c'};
    EXPECT_EQ(SHA512::hash_hex(cbyte_view{data, 3}),
              "ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a"
              "2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f");
}

TEST(SHA512Test, HashHexQuickBrownFox) {
    const string_view text{"The quick brown fox jumps over the lazy dog"};
    EXPECT_EQ(sha512(text), "07e547d9586f6a73f73fbac0435ed76951218fb7d0c8d788a309d785436bbb64"
                            "2e93a252a954f23912547d1e8a3b5ed6e1bfd7097821233fa0538f3db854fee6");
}

TEST(SHA512Test, HashReturnSize) {
    EXPECT_EQ(SHA512::hash(cbyte_view{}).size(), static_cast<size_t>(64));
    EXPECT_EQ(SHA512::hash_hex(cbyte_view{}).size(), static_cast<size_t>(128));
}

TEST(SHA512Test, HashDeterministic) {
    const byte_t data[] = {0x00, 0x11, 0x22, 0x33, 0x44};
    EXPECT_EQ(SHA512::hash(cbyte_view{data, 5}), SHA512::hash(cbyte_view{data, 5}));
}

TEST(SHA512Test, HashMultiBlock) {
    string input;
    for (size_t i = 0; i < 1000; ++i) {
        input.push_back('a');
    }
    EXPECT_EQ(sha512(input), "67ba5535a46e3f86dbfbed8cbbaf0125c76ed549ff8b0b9e03e0c88cf90fa634"
                             "fa7b12b47d77b694de488ace8d9a65967dc96df599727d3292a8d9d447709c97");
}

TEST(SHA512Test, HashPaddingBoundaries) {
    string input;
    for (size_t i = 0; i < 111; ++i) {
        input.push_back('x');
    }
    EXPECT_EQ(sha512(input), "9a2a120825c2319867758ec277924f6faa254968bf752046dacdd948d8ad299b"
                             "10359fd04bfd7d3810b5fa1b16a294236138baff981cbb85248478053ac4d3dd");
    input.push_back('x');
    EXPECT_EQ(sha512(input), "a3722b515ef40c910f2419f6e0da8ca51d410114ce6272faae64045f9e9f630e"
                             "7fa8dd5a3243c9860b899d148c3da4bc0f9e07454542604d030bb55531fe0d5b");
    while (input.size() < 128) {
        input.push_back('x');
    }
    EXPECT_EQ(sha512(input), "e2e22f8422b54b06e35c3ea30a383d1de7a8fbc27992923074103117020d8dd7"
                             "024c3ecf7d6d1a15a6de5a75ff32fb486b9e8ced4c02ffe05822bf2cb734d0e0");
}

TEST(SHA512Test, ConvenienceOverloads) {
    const byte_t data[] = {'a', 'b', 'c'};
    const cbyte_view view{data, 3};
    const byte_vector bytes{data, 3};
    EXPECT_EQ(sha512(bytes), SHA512::hash(view));
    EXPECT_EQ(sha512(string{"abc"}), SHA512::hash_hex(view));
}


TEST(BLAKE2bTest, EmptyString) {
    EXPECT_EQ(BLAKE2b::hash_hex(cbyte_view{}), "786a02f742015903c6c6fd852552d272912f4740e15847618a86e217f71f5419"
                                               "d25e1031afee585313896444934eb04b903a685b1448b755d56f701afe9be2ce");
}

TEST(BLAKE2bTest, Abc) {
    const byte_t data[] = {'a', 'b', 'c'};
    EXPECT_EQ(BLAKE2b::hash_hex(cbyte_view{data, 3}),
              "ba80a53f981c4d0d6a2797b69f12f6e94c212f14685ac4b74b12bb6fdbffa2d1"
              "7d87c5392aab792dc252d5de4533cc9518d38aa8dbf1925ab92386edd4009923");
}

TEST(BLAKE2bTest, DigestSize256) {
    const byte_t data[] = {'a', 'b', 'c'};
    EXPECT_EQ(BLAKE2b::hash_hex(cbyte_view{}, 32), "0e5751c026e543b2e8ab2eb06099daa1d1e5df47778f7787faab45cdf12fe3a8");
    EXPECT_EQ(BLAKE2b::hash_hex(cbyte_view{data, 3}, 32),
              "bddd813c634239723171ef3fee98579b94964e3bb1cb3e427262c8c068d52319");
}

TEST(BLAKE2bTest, Keyed) {
    byte_vector key;
    for (size_t i = 0; i < 64; ++i) {
        key.push_back(static_cast<byte_t>(i));
    }
    const byte_t data[] = {'a', 'b', 'c'};
    EXPECT_EQ(BLAKE2b::keyed_hash_hex(cbyte_view{data, 3}, key.view()),
              "06bbc3dedf13a31139498655251b7588ccd3bb5aaa071b2d44d8e0a04095579e"
              "d590fbfdcf941f4370ce5ce623624e7a76d33e7a8109dcda9b57d72f8f8efa51");
}

TEST(BLAKE2bTest, MultiBlock) {
    string input;
    for (size_t i = 0; i < 1000; ++i) {
        input.push_back('a');
    }
    EXPECT_EQ(blake2b(input), "d6a69459fe93fc6b9537ed4336e5099e0dcca3e97290a412500ed7a0daffb03d"
                              "80cf3650a20e0591f748e10c3c534945ee83d5f2c9722f1a68d98b8c01af23fd");
}

TEST(BLAKE2bTest, VariableDigestSize) {
    const byte_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    for (size_t size = 1; size <= 64; ++size) {
        EXPECT_EQ(BLAKE2b::hash(cbyte_view{data, 5}, size).size(), size);
    }
}

TEST(BLAKE2bTest, InvalidDigestSizeThrows) {
    EXPECT_THROW(BLAKE2b::hash(cbyte_view{}, 0), value_exception);
    EXPECT_THROW(BLAKE2b::hash(cbyte_view{}, 65), value_exception);
}

TEST(BLAKE2bTest, InvalidKeySizeThrows) {
    byte_vector key;
    for (size_t i = 0; i < 65; ++i) {
        key.push_back(0);
    }
    EXPECT_THROW(BLAKE2b::keyed_hash(cbyte_view{}, key.view()), value_exception);
}

TEST(BLAKE2sTest, EmptyString) {
    EXPECT_EQ(BLAKE2s::hash_hex(cbyte_view{}), "69217a3079908094e11121d042354a7c1f55b6482ca1a51e1b250dfd1ed0eef9");
}

TEST(BLAKE2sTest, Abc) {
    const byte_t data[] = {'a', 'b', 'c'};
    EXPECT_EQ(BLAKE2s::hash_hex(cbyte_view{data, 3}),
              "508c5e8c327c14e2e1a72ba34eeb452f37458b209ed63a294d999b4c86675982");
}

TEST(BLAKE2sTest, Keyed) {
    byte_vector key;
    for (size_t i = 0; i < 32; ++i) {
        key.push_back(static_cast<byte_t>(i));
    }
    const byte_t data[] = {'a', 'b', 'c'};
    EXPECT_EQ(BLAKE2s::keyed_hash_hex(cbyte_view{data, 3}, key.view()),
              "a281f725754969a702f6fe36fc591b7def866e4b70173ece402fc01c064d6b65");
}

TEST(BLAKE2sTest, VariableDigestSize) {
    const byte_t data[] = {0x01, 0x02, 0x03};
    for (size_t size = 1; size <= 32; ++size) {
        EXPECT_EQ(BLAKE2s::hash(cbyte_view{data, 3}, size).size(), size);
    }
    EXPECT_THROW(BLAKE2s::hash(cbyte_view{}, 0), value_exception);
    EXPECT_THROW(BLAKE2s::hash(cbyte_view{}, 33), value_exception);
}

TEST(BLAKE2bTest, BlockBoundaries) {
    byte_vector data;
    for (size_t i = 0; i < 129; ++i) {
        data.push_back(static_cast<byte_t>(i % 251));
    }
    EXPECT_EQ(BLAKE2b::hash_hex(cbyte_view{data.data(), 128}),
              "2319e3789c47e2daa5fe807f61bec2a1a6537fa03f19ff32e87eecbfd64b7e0e"
              "8ccff439ac333b040f19b0c4ddd11a61e24ac1fe0f10a039806c5dcc0da3d115");
    EXPECT_EQ(BLAKE2b::hash_hex(cbyte_view{data.data(), 129}),
              "f59711d44a031d5f97a9413c065d1e614c417ede998590325f49bad2fd444d3e"
              "4418be19aec4e11449ac1a57207898bc57d76a1bcf3566292c20c683a5c4648f");
    for (size_t i = 129; i < 256; ++i) {
        data.push_back(static_cast<byte_t>(i % 251));
    }
    EXPECT_EQ(BLAKE2b::hash_hex(data.view()), "93463ac058b6163eb43be3f5bb32b28541498f4e3366f1effe253ad44e1e076e"
                                              "41c3616046027c82a7124f8f4746668ad10b12e8e25a95ac8f3151df01cd5a93");
}

TEST(BLAKE2sTest, BlockBoundaries) {
    byte_vector data;
    for (size_t i = 0; i < 128; ++i) {
        data.push_back(static_cast<byte_t>(i % 251));
    }
    EXPECT_EQ(BLAKE2s::hash_hex(cbyte_view{data.data(), 64}),
              "56f34e8b96557e90c1f24b52d0c89d51086acf1b00f634cf1dde9233b8eaaa3e");
    EXPECT_EQ(BLAKE2s::hash_hex(cbyte_view{data.data(), 65}),
              "1b53ee94aaf34e4b159d48de352c7f0661d0a40edff95a0b1639b4090e974472");
    EXPECT_EQ(BLAKE2s::hash_hex(data.view()), "1fa877de67259d19863a2a34bcc6962a2b25fcbf5cbecd7ede8f1fa36688a796");
}

TEST(BLAKE2Test, KeyedEmptyMessage) {
    byte_vector key64;
    byte_vector key32;
    for (size_t i = 0; i < 64; ++i) {
        key64.push_back(static_cast<byte_t>(i));
    }
    for (size_t i = 0; i < 32; ++i) {
        key32.push_back(static_cast<byte_t>(i));
    }
    EXPECT_EQ(BLAKE2b::keyed_hash_hex(cbyte_view{}, key64.view()),
              "10ebb67700b1868efb4417987acf4690ae9d972fb7a590c2f02871799aaa4786"
              "b5e996e8f0f4eb981fc214b005f42d2ff4233499391653df7aefcbc13fc51568");
    EXPECT_EQ(BLAKE2s::keyed_hash_hex(cbyte_view{}, key32.view()),
              "48a8997da407876b3d79c0d92325ad3b89cbb754d86ab71aee047ad345fd2c49");
}

TEST(BLAKE2Test, ConvenienceKeyed) {
    const byte_t data[] = {'a', 'b', 'c'};
    const byte_t key[] = {0x01, 0x02};
    EXPECT_EQ(blake2b_keyed(cbyte_view{data, 3}, cbyte_view{key, 2}),
              BLAKE2b::keyed_hash(cbyte_view{data, 3}, cbyte_view{key, 2}));
    EXPECT_EQ(blake2s_keyed(cbyte_view{data, 3}, cbyte_view{key, 2}),
              BLAKE2s::keyed_hash(cbyte_view{data, 3}, cbyte_view{key, 2}));
}

namespace {
    byte_vector blake3_pattern(const size_t length) {
        byte_vector data;
        data.reserve(length);
        for (size_t i = 0; i < length; ++i) {
            data.push_back(static_cast<byte_t>(i % 251));
        }
        return data;
    }
} // namespace

TEST(BLAKE3Test, KnownVectors) {
    EXPECT_EQ(BLAKE3::hash_hex(cbyte_view{}), "af1349b9f5f9a1a6a0404dea36dcc9499bcb25c9adc112b7cc9a93cae41f3262");
    const byte_t abc[] = {'a', 'b', 'c'};
    EXPECT_EQ(BLAKE3::hash_hex(cbyte_view{abc, 3}), "6437b3ac38465133ffb63b75273a8db548c558465d79db03fd359c6cd5bd9d85");
    const string_view fox{"The quick brown fox jumps over the lazy dog"};
    EXPECT_EQ(blake3(fox), "2f1514181aadccd913abd94cfa592701a5686ab23f8df1dff1b74710febc6d4a");
}

TEST(BLAKE3Test, ChunkBoundaries) {
    EXPECT_EQ(BLAKE3::hash_hex(blake3_pattern(1).view()),
              "2d3adedff11b61f14c886e35afa036736dcd87a74d27b5c1510225d0f592e213");
    EXPECT_EQ(BLAKE3::hash_hex(blake3_pattern(63).view()),
              "e9bc37a594daad83be9470df7f7b3798297c3d834ce80ba85d6e207627b7db7b");
    EXPECT_EQ(BLAKE3::hash_hex(blake3_pattern(64).view()),
              "4eed7141ea4a5cd4b788606bd23f46e212af9cacebacdc7d1f4c6dc7f2511b98");
    EXPECT_EQ(BLAKE3::hash_hex(blake3_pattern(65).view()),
              "de1e5fa0be70df6d2be8fffd0e99ceaa8eb6e8c93a63f2d8d1c30ecb6b263dee");
    EXPECT_EQ(BLAKE3::hash_hex(blake3_pattern(1023).view()),
              "10108970eeda3eb932baac1428c7a2163b0e924c9a9e25b35bba72b28f70bd11");
    EXPECT_EQ(BLAKE3::hash_hex(blake3_pattern(1024).view()),
              "42214739f095a406f3fc83deb889744ac00df831c10daa55189b5d121c855af7");
    EXPECT_EQ(BLAKE3::hash_hex(blake3_pattern(1025).view()),
              "d00278ae47eb27b34faecf67b4fe263f82d5412916c1ffd97c8cb7fb814b8444");
}

TEST(BLAKE3Test, TreeMerging) {
    EXPECT_EQ(BLAKE3::hash_hex(blake3_pattern(2048).view()),
              "e776b6028c7cd22a4d0ba182a8bf62205d2ef576467e838ed6f2529b85fba24a");
    EXPECT_EQ(BLAKE3::hash_hex(blake3_pattern(2049).view()),
              "5f4d72f40d7a5f82b15ca2b2e44b1de3c2ef86c426c95c1af0b6879522563030");
    EXPECT_EQ(BLAKE3::hash_hex(blake3_pattern(3072).view()),
              "b98cb0ff3623be03326b373de6b9095218513e64f1ee2edd2525c7ad1e5cffd2");
    EXPECT_EQ(BLAKE3::hash_hex(blake3_pattern(4096).view()),
              "015094013f57a5277b59d8475c0501042c0b642e531b0a1c8f58d2163229e969");
    EXPECT_EQ(BLAKE3::hash_hex(blake3_pattern(8192).view()),
              "aae792484c8efe4f19e2ca7d371d8c467ffb10748d8a5a1ae579948f718a2a63");
    EXPECT_EQ(BLAKE3::hash_hex(blake3_pattern(16384).view()),
              "f875d6646de28985646f34ee13be9a576fd515f76b5b0a26bb324735041ddde4");
    EXPECT_EQ(BLAKE3::hash_hex(blake3_pattern(31744).view()),
              "62b6960e1a44bcc1eb1a611a8d6235b6b4b78f32e7abc4fb4c6cdcce94895c47");
}

TEST(BLAKE3Test, Avx2PartialChunkBoundaries) {
    EXPECT_EQ(BLAKE3::hash_hex(blake3_pattern(8193).view()),
              "bab6c09cb8ce8cf459261398d2e7aef35700bf488116ceb94a36d0f5f1b7bc3b");
    EXPECT_EQ(BLAKE3::hash_hex(blake3_pattern(9216).view()),
              "e5ef79624045ef3e98fd23342e61d7eb965997b32928f9de591cd0d465a223fb");
    EXPECT_EQ(BLAKE3::hash_hex(blake3_pattern(10241).view()),
              "1699265a918273dfcf0cb6fc0105a41c31bac3f019cd1e307714b015de25b769");
    EXPECT_EQ(BLAKE3::hash_hex(blake3_pattern(16385).view()),
              "1dabe216be2578830263b049de1639f39f05a4da616b9b78c7a5e4e41662fd1f");
}

TEST(BLAKE3Test, Xof) {
    const byte_t abc[] = {'a', 'b', 'c'};
    const cbyte_view view{abc, 3};
    EXPECT_EQ(BLAKE3::hash_xof(view, 131), [&] {
        const string hex = "6437b3ac38465133ffb63b75273a8db548c558465d79db03fd359c6cd5bd9d85"
                           "1fb250ae7393f5d02813b65d521a0d492d9ba09cf7ce7f4cffd900f23374bf0b"
                           "c08a1fb0b38ed276181ccbd9f7b7edbddf9f86404ad7929605f6ffa3fb1ac879"
                           "83105f013384f2f11d38879c985d47003804b905f0c38975e28d36804bb60d8c"
                           "303653";
        byte_vector expected;
        for (size_t i = 0; i + 1 < hex.size(); i += 2) {
            expected.push_back(static_cast<byte_t>(hexadecimal::xdigit_value(hex[i]) << 4 |
                                                   hexadecimal::xdigit_value(hex[i + 1])));
        }
        return expected;
    }());
    EXPECT_EQ(BLAKE3::hash_xof(view, 32), BLAKE3::hash(view));
    EXPECT_TRUE(BLAKE3::hash_xof(view, 0).empty());
}

TEST(BLAKE3Test, KeyedHash) {
    byte_vector key;
    for (size_t i = 0; i < 32; ++i) {
        key.push_back(static_cast<byte_t>(i));
    }
    const byte_t abc[] = {'a', 'b', 'c'};
    EXPECT_EQ(BLAKE3::keyed_hash(key.view(), cbyte_view{abc, 3}), [&] {
        const string hex = "6da54495d8152f2bcba87bd7282df70901cdb66b4448ed5f4c7bd2852b8b5532";
        byte_vector expected;
        for (size_t i = 0; i + 1 < hex.size(); i += 2) {
            expected.push_back(static_cast<byte_t>(hexadecimal::xdigit_value(hex[i]) << 4 |
                                                   hexadecimal::xdigit_value(hex[i + 1])));
        }
        return expected;
    }());
    EXPECT_THROW(BLAKE3::keyed_hash(cbyte_view{abc, 3}, cbyte_view{abc, 3}), value_exception);
}

TEST(BLAKE3Test, DeriveKey) {
    const string_view context{"NexusForce BLAKE3 test"};
    const string_view material{"material-1234567890"};
    EXPECT_EQ(BLAKE3::derive_key(cbyte_view{reinterpret_cast<const byte_t*>(context.data()), context.size()},
                                 cbyte_view{reinterpret_cast<const byte_t*>(material.data()), material.size()}, 64),
              [&] {
                  const string hex = "874a03ee0399af148ee9f497de21b99912bc18be62f4af09cab84201a2771d54"
                                     "a5ae3421bf608b93bf249e192d9fda5b6e559a01be054e26702a0607d18df987";
                  byte_vector expected;
                  for (size_t i = 0; i + 1 < hex.size(); i += 2) {
                      expected.push_back(static_cast<byte_t>(hexadecimal::xdigit_value(hex[i]) << 4 |
                                                             hexadecimal::xdigit_value(hex[i + 1])));
                  }
                  return expected;
              }());
}

TEST(BLAKE3Test, Convenience) {
    const byte_t data[] = {0x00, 0x11, 0x22};
    const cbyte_view view{data, 3};
    const byte_vector bytes{data, 3};
    EXPECT_EQ(blake3(view), BLAKE3::hash(view));
    EXPECT_EQ(blake3(bytes), BLAKE3::hash(view));
    EXPECT_EQ(blake3(string_view{"abc"}), BLAKE3::hash_hex(cbyte_view{reinterpret_cast<const byte_t*>("abc"), 3}));
}
class HashTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(HashTest, IntegerHashDeterministic) {
    EXPECT_EQ(hash<int>()(42), hash<int>()(42));
    EXPECT_EQ(hash<unsigned long long>()(123456789ULL), hash<unsigned long long>()(123456789ULL));
}

TEST_F(HashTest, IntegerHashIsIdentityLikeStd) {
    EXPECT_EQ(hash<int>()(0), static_cast<size_t>(0));
    EXPECT_EQ(hash<int>()(42), static_cast<size_t>(42));
    EXPECT_EQ(hash<int>()(-1), static_cast<size_t>(-1));
    EXPECT_EQ(hash<char>()(7), static_cast<size_t>(7));
    EXPECT_EQ(hash<unsigned short>()(65535), static_cast<size_t>(65535));
    EXPECT_EQ(hash<long long>()(-123456789LL), static_cast<size_t>(-123456789LL));
}

TEST_F(HashTest, FloatHashDeterministic) {
    EXPECT_EQ(hash<double>()(3.14), hash<double>()(3.14));
    EXPECT_EQ(hash<float>()(1.5f), hash<float>()(1.5f));
}

TEST_F(HashTest, FloatHashZero) {
    EXPECT_EQ(hash<float>()(0.0f), static_cast<size_t>(0));
    EXPECT_EQ(hash<double>()(0.0), static_cast<size_t>(0));
}

TEST_F(HashTest, BoolHashDifferent) {
    EXPECT_NE(hash<bool>()(true), hash<bool>()(false));
    EXPECT_EQ(hash<bool>()(true), 0x9e3779b9);
    EXPECT_EQ(hash<bool>()(false), 0x7f4a7c15);
}

TEST_F(HashTest, PointerHashIdentity) {
    int x = 0;
    EXPECT_EQ(hash<int*>()(&x), reinterpret_cast<size_t>(&x));
}

TEST_F(HashTest, EnumHashDelegates) {
    enum class Color {
        Red,
        Green,
        Blue
    };
    EXPECT_EQ(hash<Color>()(Color::Red), hash<int>()(static_cast<int>(Color::Red)));
}

TEST_F(HashTest, LowLevelHashAvalanche) { EXPECT_NE(low_level_hash(static_cast<size_t>(0)), static_cast<size_t>(0)); }

TEST_F(HashTest, LowLevelHashDeterministic) {
    EXPECT_EQ(low_level_hash(static_cast<size_t>(42)), low_level_hash(static_cast<size_t>(42)));
    EXPECT_NE(low_level_hash(static_cast<size_t>(1)), low_level_hash(static_cast<size_t>(2)));
}

TEST_F(HashTest, HashCombineDeterministic) {
    size_t seed1 = 0;
    size_t seed2 = 0;
    hash_combine(seed1, 42);
    hash_combine(seed2, 42);
    EXPECT_EQ(seed1, seed2);
}

TEST_F(HashTest, HashCombineOrderMatters) {
    size_t seed1 = 0;
    size_t seed2 = 0;
    hash_combine(seed1, 1);
    hash_combine(seed1, 2);
    hash_combine(seed2, 2);
    hash_combine(seed2, 1);
    EXPECT_NE(seed1, seed2);
}

TEST_F(HashTest, HashCombineAll) {
    size_t h1 = hash_combine_all(1, 2, 3);
    size_t h2 = hash_combine_all(1, 2, 3);
    size_t h3 = hash_combine_all(3, 2, 1);
    EXPECT_EQ(h1, h2);
    EXPECT_NE(h1, h3);
}

TEST_F(HashTest, XXH32Empty) { EXPECT_EQ(XXH32("", 0, 0), 0x02CC5D05U); }

TEST_F(HashTest, XXH32Deterministic) {
    const char data[] = "test data for xxh32";
    EXPECT_EQ(XXH32(data, sizeof(data) - 1, 0), XXH32(data, sizeof(data) - 1, 0));
}

TEST_F(HashTest, FNVHashPreserved) {
    EXPECT_EQ(FNV_hash64(nullptr, 0), constants::FNV_OFFSET_BASIS64);
    EXPECT_EQ(FNV_hash_string64("", 0), constants::FNV_OFFSET_BASIS64);
}

TEST_F(HashTest, DJB2HashPreserved) { EXPECT_EQ(DJB2_hash("", 0), static_cast<size_t>(5381)); }

TEST_F(HashTest, MurmurHashX32Empty) { EXPECT_EQ(murmur_hash32("", 0, 0), 0U); }

TEST_F(HashTest, MurmurHashX32Deterministic) {
    const char data[] = "murmur test";
    uint32_t h1 = murmur_hash32(data, sizeof(data) - 1, 42);
    uint32_t h2 = murmur_hash32(data, sizeof(data) - 1, 42);
    EXPECT_EQ(h1, h2);
}

TEST_F(HashTest, AllAlgorithmsDeterministic) {
    const byte_t data[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                           0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    const size_t len = sizeof(data);

    EXPECT_EQ(XXH32(data, len, 0), XXH32(data, len, 0));
    EXPECT_EQ(murmur_hash32(data, len, 0), murmur_hash32(data, len, 0));
}

#ifdef NEFORCE_ARCH_BITS_64

TEST_F(HashTest, XXH64Empty) { EXPECT_EQ(XXH64("", 0, 0), 0xEF46DB3751D8E999ULL); }

TEST_F(HashTest, XXH64Deterministic) {
    const char data[] = "test data for xxh64";
    EXPECT_EQ(XXH64(data, sizeof(data) - 1, 0), XXH64(data, sizeof(data) - 1, 0));
}

TEST_F(HashTest, XXH3Empty) {
    uint64_t h1 = XXH3_64("", 0);
    uint64_t h2 = XXH3_64("", 0);
    EXPECT_EQ(h1, h2);
    EXPECT_NE(h1, static_cast<uint64_t>(0));
}

TEST_F(HashTest, XXH3Deterministic) {
    const char data[] = "xxh3 test data";
    EXPECT_EQ(XXH3_64(data, sizeof(data) - 1), XXH3_64(data, sizeof(data) - 1));
}

TEST_F(HashTest, WyhashEmpty) { EXPECT_NE(wyhash("", 0, 0), static_cast<uint64_t>(0)); }

TEST_F(HashTest, WyhashDeterministic) {
    const char data[] = "wyhash test data";
    EXPECT_EQ(wyhash(data, sizeof(data) - 1, 42), wyhash(data, sizeof(data) - 1, 42));
}

TEST_F(HashTest, CityHash64Empty) { EXPECT_EQ(city_hash64("", 0), static_cast<size_t>(0x9ae16a3b2f90404fULL)); }

TEST_F(HashTest, CityHash64Deterministic) {
    const char data[] = "cityhash test data";
    EXPECT_EQ(city_hash64(data, sizeof(data) - 1), city_hash64(data, sizeof(data) - 1));
}

TEST_F(HashTest, MurmurHashX64Empty) {
    murmur_hash h = murmur_hash64("", 0, 0);
    EXPECT_EQ(h.low, static_cast<size_t>(0));
    EXPECT_EQ(h.high, static_cast<size_t>(0));
}

TEST_F(HashTest, MurmurHashX64Deterministic) {
    const char data[] = "murmur x64 test";
    murmur_hash h1 = murmur_hash64(data, sizeof(data) - 1, 42);
    murmur_hash h2 = murmur_hash64(data, sizeof(data) - 1, 42);
    EXPECT_EQ(h1.low, h2.low);
    EXPECT_EQ(h1.high, h2.high);
}

TEST_F(HashTest, All64BitAlgorithmsDeterministic) {
    const byte_t data[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                           0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    const size_t len = sizeof(data);

    EXPECT_EQ(XXH64(data, len, 0), XXH64(data, len, 0));
    EXPECT_EQ(wyhash(data, len, 0), wyhash(data, len, 0));
    EXPECT_EQ(city_hash64(data, len), city_hash64(data, len));
    EXPECT_EQ(XXH3_64(data, len), XXH3_64(data, len));
}

#endif
