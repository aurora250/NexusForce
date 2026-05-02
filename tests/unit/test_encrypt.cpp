#include <NeForce/core/encrypt/aes256.hpp>
#include <NeForce/core/encrypt/base64.hpp>
#include <NeForce/core/encrypt/md5.hpp>
#include <NeForce/core/encrypt/sha256.hpp>
#include <NeForce/core/container/vector.hpp>
#include <NeForce/core/string/string.hpp>
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
    const byte_vector expected{0x27, 0xe7, 0xf9, 0xb0, 0x58, 0x99, 0x5b, 0x6d,
                               0xa9, 0x02, 0x47, 0x19, 0x9a, 0xd5, 0xc3, 0xcb};
    EXPECT_EQ(result, expected);
}

TEST(MD5Test, HashExact120Bytes) {
    constexpr const char* const msg = "1234567890123456789012345678901234567890"
                                      "1234567890123456789012345678901234567890"
                                      "1234567890123456789012345678901234567890";
    static_assert(char_traits<char>::length(msg) == 120, "must be 120 bytes");
    const cbyte_view input{reinterpret_cast<const byte_t*>(msg), 120};
    const byte_vector result = MD5::hash(input);
    const byte_vector expected{0x7a, 0xdd, 0xa1, 0xb0, 0x36, 0x04, 0x74, 0xee,
                               0xa3, 0x93, 0x22, 0x50, 0xce, 0xe3, 0x27, 0x35};
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
    const byte_vector expected{0xc2, 0x05, 0xa7, 0xb3, 0xf0, 0x5b, 0x32, 0x6b,
                               0x18, 0x74, 0x3f, 0x2d, 0x87, 0x3d, 0x73, 0x20};
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
