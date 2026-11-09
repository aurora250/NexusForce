#include <NeForce/core/container/vector.hpp>
#include <NeForce/core/exception/exception.hpp>
#include <NeForce/core/encoding/base32.hpp>
#include <NeForce/core/encoding/base64.hpp>
#include <NeForce/core/encoding/percent.hpp>
#include <NeForce/core/string/string.hpp>
#include <gtest/gtest.h>
using namespace neforce;

TEST(Base64Test, EncodeEmptyString) {
    const string result = base64::encode(cbyte_view{});
    EXPECT_EQ(result, "");
}

TEST(Base64Test, EncodeSingleByte) {
    const byte_t data[] = {static_cast<byte_t>('M')};
    const string result = base64::encode(cbyte_view{data, 1});
    EXPECT_EQ(result, "TQ==");
}

TEST(Base64Test, EncodeTwoBytes) {
    const byte_t data[] = {static_cast<byte_t>('M'), static_cast<byte_t>('a')};
    const string result = base64::encode(cbyte_view{data, 2});
    EXPECT_EQ(result, "TWE=");
}

TEST(Base64Test, EncodeThreeBytes) {
    const byte_t data[] = {static_cast<byte_t>('M'), static_cast<byte_t>('a'), static_cast<byte_t>('n')};
    const string result = base64::encode(cbyte_view{data, 3});
    EXPECT_EQ(result, "TWFu");
}

TEST(Base64Test, EncodeFullAlphabet) {
    const byte_t data[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C,
                           0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
                           0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26,
                           0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33,
                           0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F};
    const string result = base64::encode(cbyte_view{data, 64});
    EXPECT_EQ(result, "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+Pw==");
}

TEST(Base64Test, EncodeBinaryData) {
    const byte_t data[] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
    const string result = base64::encode(cbyte_view{data, 8});
    EXPECT_EQ(result, "iVBORw0KGgo=");
}

TEST(Base64Test, EncodeAllCharsAscii) {
    string ascii;
    for (int i = 0; i < 128; i++) {
        ascii.push_back(static_cast<char>(i));
    }
    const cbyte_view input{reinterpret_cast<const byte_t*>(ascii.data()), ascii.size()};
    const string result = base64::encode(input);
    const string expected = "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+"
                            "P0BBQkNERUZHSElKS0xNTk9QUVJTVFVWV1hZWltcXV5fYGFiY2RlZmdoaWprbG1ub3BxcnN0dXZ3eHl6e3x9fn8=";
    EXPECT_EQ(result, expected);
}

TEST(Base64Test, EncodeLongSequence) {
    string long_str(1000, 'A');
    cbyte_view input{reinterpret_cast<const byte_t*>(long_str.data()), long_str.size()};

    string result = base64::encode(input);
    ASSERT_EQ(result.size(), 1336);

    const string block = "QUFB";
    for (size_t i = 0; i < 333; ++i) {
        EXPECT_EQ(result.substr(i * 4, 4), block);
    }

    EXPECT_EQ(result.substr(1332), "QQ==");
}

TEST(Base64Test, DecodeEmptyString) {
    const byte_vector result = base64::decode("");
    EXPECT_TRUE(result.empty());
}

TEST(Base64Test, DecodeSingleByteWithPadding) {
    const byte_vector result = base64::decode("TQ==");
    const byte_vector expected{static_cast<byte_t>('M')};
    EXPECT_EQ(result, expected);
}

TEST(Base64Test, DecodeTwoBytesWithPadding) {
    const byte_vector result = base64::decode("TWE=");
    const byte_vector expected{static_cast<byte_t>('M'), static_cast<byte_t>('a')};
    EXPECT_EQ(result, expected);
}

TEST(Base64Test, DecodeThreeBytesNoPadding) {
    const byte_vector result = base64::decode("TWFu");
    const byte_vector expected{static_cast<byte_t>('M'), static_cast<byte_t>('a'), static_cast<byte_t>('n')};
    EXPECT_EQ(result, expected);
}

TEST(Base64Test, DecodeWithWhitespace) {
    const byte_vector result = base64::decode("T Q =\n=\r \t");
    const byte_vector expected{static_cast<byte_t>('M')};
    EXPECT_EQ(result, expected);
}

TEST(Base64Test, DecodeMultipleWhitespaceTypes) {
    const byte_vector result = base64::decode("T W E =\n \r\n\t \v\f");
    const byte_vector expected{static_cast<byte_t>('M'), static_cast<byte_t>('a')};
    EXPECT_EQ(result, expected);
}

TEST(Base64Test, DecodeFullAlphabet) {
    const string base64_str =
            "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+Pw==";
    const byte_vector result = base64::decode(base64_str.view());
    byte_vector expected;
    for (int i = 0; i < 64; i++) {
        expected.push_back(static_cast<byte_t>(i));
    }
    EXPECT_EQ(result, expected);
}

TEST(Base64Test, DecodeBinaryData) {
    const byte_vector result = base64::decode("iVBORw0KGgo=");
    const byte_t expected[] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
    const byte_vector expected_vec(expected, expected + 8);
    EXPECT_EQ(result, expected_vec);
}

TEST(Base64Test, DecodeLongSequence) {
    string long_str(1000, 'Q');
    const string base64_str =
            base64::encode(cbyte_view{reinterpret_cast<const byte_t*>(long_str.data()), long_str.size()});
    const byte_vector result = base64::decode(base64_str.view());
    const byte_vector expected(reinterpret_cast<const byte_t*>(long_str.data()),
                               reinterpret_cast<const byte_t*>(long_str.data()) + long_str.size());
    EXPECT_EQ(result, expected);
}

TEST(Base64Test, DecodeInvalidLengthThrows) {
    EXPECT_THROW(base64::decode("TQ="), value_exception);
    EXPECT_THROW(base64::decode("TQ"), value_exception);
    EXPECT_THROW(base64::decode("TQE"), value_exception);
}

TEST(Base64Test, DecodeInvalidCharacterThrows) {
    EXPECT_THROW(base64::decode("TQ#="), value_exception);
    EXPECT_THROW(base64::decode("T$@="), value_exception);
    EXPECT_THROW(base64::decode("!@#$"), value_exception);
}

TEST(Base64Test, DecodeInvalidPaddingThrows) {
    EXPECT_THROW(base64::decode("TQ=Q"), value_exception);
    EXPECT_THROW(base64::decode("T==="), value_exception);
    EXPECT_THROW(base64::decode("T=QQ"), value_exception);
}

TEST(Base64Test, EncodeUrlEmptyString) {
    const string result = base64::encode_url(cbyte_view{}, false);
    EXPECT_EQ(result, "");
}

TEST(Base64Test, EncodeUrlSingleByteNoPadding) {
    const byte_t data[] = {static_cast<byte_t>('M')};
    const string result = base64::encode_url(cbyte_view{data, 1}, false);
    EXPECT_EQ(result, "TQ");
}

TEST(Base64Test, EncodeUrlSingleByteWithPadding) {
    const byte_t data[] = {static_cast<byte_t>('M')};
    const string result = base64::encode_url(cbyte_view{data, 1}, true);
    EXPECT_EQ(result, "TQ==");
}

TEST(Base64Test, EncodeUrlTwoBytesNoPadding) {
    const byte_t data[] = {static_cast<byte_t>('M'), static_cast<byte_t>('a')};
    const string result = base64::encode_url(cbyte_view{data, 2}, false);
    EXPECT_EQ(result, "TWE");
}

TEST(Base64Test, EncodeUrlTwoBytesWithPadding) {
    const byte_t data[] = {static_cast<byte_t>('M'), static_cast<byte_t>('a')};
    const string result = base64::encode_url(cbyte_view{data, 2}, true);
    EXPECT_EQ(result, "TWE=");
}

TEST(Base64Test, EncodeUrlThreeBytesNoPadding) {
    const byte_t data[] = {static_cast<byte_t>('M'), static_cast<byte_t>('a'), static_cast<byte_t>('n')};
    const string result = base64::encode_url(cbyte_view{data, 3}, false);
    EXPECT_EQ(result, "TWFu");
}

TEST(Base64Test, EncodeUrlThreeBytesWithPadding) {
    const byte_t data[] = {static_cast<byte_t>('M'), static_cast<byte_t>('a'), static_cast<byte_t>('n')};
    const string result = base64::encode_url(cbyte_view{data, 3}, true);
    EXPECT_EQ(result, "TWFu");
}

TEST(Base64Test, EncodeUrlWithPlusSlash) {
    const byte_t data[] = {0x3E, 0x3F, 0x3E};
    const string result = base64::encode_url(cbyte_view{data, 3}, false);
    EXPECT_EQ(result, "Pj8-");
}

TEST(Base64Test, EncodeUrlLongSequenceNoPadding) {
    string long_str(1000, 'A');
    cbyte_view input{reinterpret_cast<const byte_t*>(long_str.data()), long_str.size()};
    string result = base64::encode_url(input, false);

    EXPECT_EQ(result.find('+'), string::npos);
    EXPECT_EQ(result.find('/'), string::npos);

    for (char c: result) {
        EXPECT_NE(c, '=');
    }
}

TEST(Base64Test, DecodeUrlEmptyString) {
    const byte_vector result = base64::decode_url("");
    EXPECT_TRUE(result.empty());
}

TEST(Base64Test, DecodeUrlSingleByteNoPadding) {
    const byte_vector result = base64::decode_url("TQ");
    const byte_vector expected{static_cast<byte_t>('M')};
    EXPECT_EQ(result, expected);
}

TEST(Base64Test, DecodeUrlSingleByteWithPadding) {
    const byte_vector result = base64::decode_url("TQ==");
    const byte_vector expected{static_cast<byte_t>('M')};
    EXPECT_EQ(result, expected);
}

TEST(Base64Test, DecodeUrlTwoBytesNoPadding) {
    const byte_vector result = base64::decode_url("TWE");
    const byte_vector expected{static_cast<byte_t>('M'), static_cast<byte_t>('a')};
    EXPECT_EQ(result, expected);
}

TEST(Base64Test, DecodeUrlTwoBytesWithPadding) {
    const byte_vector result = base64::decode_url("TWE=");
    const byte_vector expected{static_cast<byte_t>('M'), static_cast<byte_t>('a')};
    EXPECT_EQ(result, expected);
}

TEST(Base64Test, DecodeUrlThreeBytesNoPadding) {
    const byte_vector result = base64::decode_url("TWFu");
    const byte_vector expected{static_cast<byte_t>('M'), static_cast<byte_t>('a'), static_cast<byte_t>('n')};
    EXPECT_EQ(result, expected);
}

TEST(Base64Test, DecodeUrlWithMinusUnderscore) {
    const byte_vector result = base64::decode_url("Pj8-");
    const byte_t expected[] = {0x3E, 0x3F, 0x3E};
    const byte_vector expected_vec(expected, expected + 3);
    EXPECT_EQ(result, expected_vec);
}

TEST(Base64Test, DecodeUrlWithWhitespace) {
    const byte_vector result = base64::decode_url("T Q =\n=\r \t");
    const byte_vector expected{static_cast<byte_t>('M')};
    EXPECT_EQ(result, expected);
}

TEST(Base64Test, DecodeUrlInvalidLengthAutoPadding) {
    const byte_vector result = base64::decode_url("TQ");
    const byte_vector expected{static_cast<byte_t>('M')};
    EXPECT_EQ(result, expected);
}

TEST(Base64Test, DecodeUrlInvalidCharacterThrows) {
    EXPECT_THROW(base64::decode_url("TQ#"), value_exception);
    EXPECT_THROW(base64::decode_url("T$@"), value_exception);
}

TEST(Base64Test, EncodeUrlAndDecodeRoundtrip) {
    const string original = "Hello World! 123 @#$%^&*()";
    const cbyte_view input{reinterpret_cast<const byte_t*>(original.data()), original.size()};
    const string encoded = base64::encode_url(input, false);
    const byte_vector decoded = base64::decode_url(encoded.view());
    const string decoded_str{reinterpret_cast<const char*>(decoded.data()), decoded.size()};
    EXPECT_EQ(original, decoded_str);
}

TEST(Base64Test, EncodeStandardAndDecodeRoundtrip) {
    const string original = "Hello World! 123 @#$%^&*()";
    const cbyte_view input{reinterpret_cast<const byte_t*>(original.data()), original.size()};
    const string encoded = base64::encode(input);
    const byte_vector decoded = base64::decode(encoded.view());
    const string decoded_str{reinterpret_cast<const char*>(decoded.data()), decoded.size()};
    EXPECT_EQ(original, decoded_str);
}

TEST(Base64Test, EncodeUrlAndStandardCompatibility) {
    const byte_t data[] = {0x3E, 0x3F, 0x3F};
    const cbyte_view input{data, 3};
    const string url_encoded = base64::encode_url(input, false);
    const string standard_encoded = base64::encode(input);
    EXPECT_NE(url_encoded, standard_encoded);
    const string standard_encoded_url_friendly = "Pj8/";
    EXPECT_EQ(standard_encoded, standard_encoded_url_friendly);
}

TEST(Base64Test, ConvenientBase64EncodeByteView) {
    const byte_t data[] = {static_cast<byte_t>('M'), static_cast<byte_t>('a'), static_cast<byte_t>('n')};
    const string result = base64_encode(cbyte_view{data, 3});
    EXPECT_EQ(result, "TWFu");
}

TEST(Base64Test, ConvenientBase64EncodeByteVector) {
    const byte_vector data = {static_cast<byte_t>('M'), static_cast<byte_t>('a'), static_cast<byte_t>('n')};
    const string result = base64_encode(data);
    EXPECT_EQ(result, "TWFu");
}

TEST(Base64Test, ConvenientBase64EncodeString) {
    const string result = base64_encode(string{"Man"});
    EXPECT_EQ(result, "TWFu");
}

TEST(Base64Test, ConvenientBase64EncodeEmpty) {
    const string result = base64_encode(string{});
    EXPECT_EQ(result, "");
}

TEST(Base64Test, ConvenientBase64DecodeStringView) {
    const string result = base64_decode(string_view{"TWFu"});
    EXPECT_EQ(result, "Man");
}

TEST(Base64Test, ConvenientBase64DecodeByteVector) {
    const byte_vector input = {static_cast<byte_t>('T'), static_cast<byte_t>('W'), static_cast<byte_t>('F'),
                               static_cast<byte_t>('u')};
    const byte_vector result = base64_decode(input);
    const byte_vector expected{static_cast<byte_t>('M'), static_cast<byte_t>('a'), static_cast<byte_t>('n')};
    EXPECT_EQ(result, expected);
}

TEST(Base64Test, ConvenientBase64DecodeString) {
    const string result = base64_decode(string{"TWFu"});
    EXPECT_EQ(result, "Man");
}

TEST(Base64Test, ConvenientBase64DecodeEmpty) {
    const string result = base64_decode(string{});
    EXPECT_TRUE(result.empty());
}

TEST(Base64Test, EncodeAllStandardAndUrlVariants) {
    const string original = "Hello World!";
    const cbyte_view input{reinterpret_cast<const byte_t*>(original.data()), original.size()};
    const string standard = base64::encode(input);
    const string url_no_pad = base64::encode_url(input, false);
    const string url_with_pad = base64::encode_url(input, true);
    EXPECT_EQ(standard, "SGVsbG8gV29ybGQh");
    EXPECT_EQ(url_no_pad, "SGVsbG8gV29ybGQh");
    EXPECT_EQ(url_with_pad, "SGVsbG8gV29ybGQh");
}

TEST(Base64Test, EncodeUrlSpecialCharacters) {
    const byte_t data[] = {0xFF, 0x00, 0x80, 0x7F, 0x01, 0xFE};
    const cbyte_view input{data, 6};
    const string standard = base64::encode(input);
    const string url = base64::encode_url(input, false);
    EXPECT_EQ(standard, "/wCAfwH+");
    EXPECT_EQ(url, "_wCAfwH-");
}

TEST(Base64Test, DecodeUrlSpecialCharacters) {
    const byte_vector expected = base64::decode("/wCAfwH+");
    const byte_vector result = base64::decode_url("_wCAfwH-");
    EXPECT_EQ(result, expected);
}

TEST(Base64Test, ConsistentEncodingAcrossCalls) {
    const byte_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    const cbyte_view input{data, 5};
    const string first = base64::encode(input);
    const string second = base64::encode(input);
    EXPECT_EQ(first, second);
    const string first_url = base64::encode_url(input, false);
    const string second_url = base64::encode_url(input, false);
    EXPECT_EQ(first_url, second_url);
}

TEST(Base64Test, DecodeWithVariousWhitespaceCombinations) {
    const string encoded = "T W F u";
    const byte_vector result = base64::decode(encoded.view());
    const byte_vector expected{static_cast<byte_t>('M'), static_cast<byte_t>('a'), static_cast<byte_t>('n')};
    EXPECT_EQ(result, expected);
}

TEST(Base64Test, DecodeUrlWithVariousWhitespaceCombinations) {
    const string encoded = "T W F u";
    const byte_vector result = base64::decode_url(encoded.view());
    const byte_vector expected{static_cast<byte_t>('M'), static_cast<byte_t>('a'), static_cast<byte_t>('n')};
    EXPECT_EQ(result, expected);
}

TEST(Base64Test, EncodeUrlBinaryDataRoundtrip) {
    byte_vector binary;
    for (int i = 0; i < 256; i++) {
        binary.push_back(static_cast<byte_t>(i));
    }
    const cbyte_view input{binary.data(), binary.size()};
    const string encoded = base64::encode_url(input, false);
    const byte_vector decoded = base64::decode_url(encoded.view());
    EXPECT_EQ(binary, decoded);
}

TEST(Base32Test, EncodeRfc4648Vectors) {
    EXPECT_EQ(base32::encode(cbyte_view{}), string(""));
    EXPECT_EQ(base32::encode(cbyte_view{reinterpret_cast<const byte_t*>("f"), 1}), string("MY======"));
    EXPECT_EQ(base32::encode(cbyte_view{reinterpret_cast<const byte_t*>("fo"), 2}), string("MZXQ===="));
    EXPECT_EQ(base32::encode(cbyte_view{reinterpret_cast<const byte_t*>("foo"), 3}), string("MZXW6==="));
    EXPECT_EQ(base32::encode(cbyte_view{reinterpret_cast<const byte_t*>("foob"), 4}), string("MZXW6YQ="));
    EXPECT_EQ(base32::encode(cbyte_view{reinterpret_cast<const byte_t*>("fooba"), 5}), string("MZXW6YTB"));
    EXPECT_EQ(base32::encode(cbyte_view{reinterpret_cast<const byte_t*>("foobar"), 6}), string("MZXW6YTBOI======"));
}

TEST(Base32Test, DecodeRfc4648Vectors) {
    EXPECT_EQ(base32_decode(string_view{"MY======"}), string("f"));
    EXPECT_EQ(base32_decode(string_view{"MZXQ===="}), string("fo"));
    EXPECT_EQ(base32_decode(string_view{"MZXW6==="}), string("foo"));
    EXPECT_EQ(base32_decode(string_view{"MZXW6YQ="}), string("foob"));
    EXPECT_EQ(base32_decode(string_view{"MZXW6YTB"}), string("fooba"));
    EXPECT_EQ(base32_decode(string_view{"MZXW6YTBOI======"}), string("foobar"));
}

TEST(Base32Test, RoundTripAllLengths) {
    for (size_t len = 0; len < 40; ++len) {
        byte_vector input;
        for (size_t i = 0; i < len; ++i) {
            input.push_back(static_cast<byte_t>(i * 37 + 11));
        }
        const string encoded = base32_encode(input);
        EXPECT_EQ(base32::decode(encoded.view()), input);
    }
}

TEST(Base32Test, DecodeIsCaseInsensitive) {
    EXPECT_EQ(base32_decode(string_view{"mzxw6ytboi======"}), string("foobar"));
    EXPECT_EQ(base32_decode(string_view{"MzXw6YtBoI======"}), string("foobar"));
}

TEST(Base32Test, DecodeIgnoresWhitespace) {
    EXPECT_EQ(base32_decode(string_view{"MZXW 6YTB OI==\n===="}), string("foobar"));
}

TEST(Base32Test, DecodeAcceptsMissingPadding) {
    EXPECT_EQ(base32_decode(string_view{"MY"}), string("f"));
    EXPECT_EQ(base32_decode(string_view{"MZXQ"}), string("fo"));
    EXPECT_EQ(base32_decode(string_view{"MZXW6"}), string("foo"));
    EXPECT_EQ(base32_decode(string_view{"MZXW6YQ"}), string("foob"));
    EXPECT_EQ(base32_decode(string_view{"MZXW6YTBOI"}), string("foobar"));
}

TEST(Base32Test, DecodeInvalidCharacterThrows) {
    EXPECT_THROW(base32_decode(string_view{"MZXW6YT!"}), value_exception);
    EXPECT_THROW(base32_decode(string_view{"MZXW6YT0"}), value_exception);
}

TEST(Base32Test, DecodeInvalidLengthThrows) {
    EXPECT_THROW(base32_decode(string_view{"M"}), value_exception);
    EXPECT_THROW(base32_decode(string_view{"MZX"}), value_exception);
    EXPECT_THROW(base32_decode(string_view{"MZXW6Y"}), value_exception);
}

TEST(Base32Test, DecodeInvalidPaddingThrows) {
    EXPECT_THROW(base32_decode(string_view{"MY=====A"}), value_exception);
    EXPECT_THROW(base32_decode(string_view{"MY====="}), value_exception);
    EXPECT_THROW(base32_decode(string_view{"MY======="}), value_exception);
}

TEST(Base32Test, HexEncodeRfc4648Vectors) {
    EXPECT_EQ(base32::encode_hex(cbyte_view{}), string(""));
    EXPECT_EQ(base32::encode_hex(cbyte_view{reinterpret_cast<const byte_t*>("f"), 1}), string("CO======"));
    EXPECT_EQ(base32::encode_hex(cbyte_view{reinterpret_cast<const byte_t*>("fo"), 2}), string("CPNG===="));
    EXPECT_EQ(base32::encode_hex(cbyte_view{reinterpret_cast<const byte_t*>("foo"), 3}), string("CPNMU==="));
    EXPECT_EQ(base32::encode_hex(cbyte_view{reinterpret_cast<const byte_t*>("foob"), 4}), string("CPNMUOG="));
    EXPECT_EQ(base32::encode_hex(cbyte_view{reinterpret_cast<const byte_t*>("fooba"), 5}), string("CPNMUOJ1"));
    EXPECT_EQ(base32::encode_hex(cbyte_view{reinterpret_cast<const byte_t*>("foobar"), 6}), string("CPNMUOJ1E8======"));
}

TEST(Base32Test, HexDecodeRoundTrip) {
    const byte_t data[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    const cbyte_view view{data, sizeof(data)};
    const byte_vector expected{data, sizeof(data)};
    EXPECT_EQ(base32::decode_hex(base32::encode_hex(view).view()), expected);
}

TEST(Base32Test, HexDecodeRejectsStandardAlphabet) {
    EXPECT_THROW(base32_hex_decode(string_view{"MZXW6YTBOI======"}), value_exception);
}

TEST(PercentTest, EncodeUnreservedUnchanged) {
    const char text[] = "AZaz09-_.~";
    EXPECT_EQ(percent::encode(cbyte_view{reinterpret_cast<const byte_t*>(text), sizeof(text) - 1}),
              string("AZaz09-_.~"));
}

TEST(PercentTest, EncodeReserved) {
    const char text[] = "a b";
    EXPECT_EQ(percent::encode(cbyte_view{reinterpret_cast<const byte_t*>(text), 3}), string("a%20b"));

    const char path[] = "/a/b";
    EXPECT_EQ(percent::encode(cbyte_view{reinterpret_cast<const byte_t*>(path), 4}), string("%2Fa%2Fb"));
    EXPECT_EQ(percent::encode(cbyte_view{reinterpret_cast<const byte_t*>(path), 4}, false), string("/a/b"));
}

TEST(PercentTest, EncodeUsesUppercaseHex) {
    const byte_t data[] = {0x00, 0x1F, 0x7F, 0x80, 0xFF};
    EXPECT_EQ(percent::encode(cbyte_view{data, sizeof(data)}), string("%00%1F%7F%80%FF"));
}

TEST(PercentTest, Decode) {
    const byte_vector expected{'a', ' ', 'b'};
    EXPECT_EQ(percent::decode(string_view{"a%20b"}), expected);

    const byte_vector slashes{'/', '/'};
    EXPECT_EQ(percent::decode(string_view{"%2f%2F"}), slashes);
}

TEST(PercentTest, DecodeInvalid) {
    EXPECT_THROW(percent::decode(string_view{"%"}), value_exception);
    EXPECT_THROW(percent::decode(string_view{"%2"}), value_exception);
    EXPECT_THROW(percent::decode(string_view{"%GG"}), value_exception);
    EXPECT_THROW(percent::decode(string_view{"%2G"}), value_exception);
    EXPECT_FALSE(static_cast<bool>(percent::try_decode(string_view{"%GG"})));
    EXPECT_TRUE(static_cast<bool>(percent::try_decode(string_view{"%41"})));
}

TEST(PercentTest, DecodeTolerant) {
    EXPECT_EQ(percent::decode_tolerant(string_view{"a+b%20c%ZZ%d"}), string("a b c%ZZ%d"));
    EXPECT_EQ(percent::decode_tolerant(string_view{"%2F"}), string("/"));
}

TEST(PercentTest, EncodeForm) {
    const char text[] = "a b+c";
    EXPECT_EQ(percent::encode_form(cbyte_view{reinterpret_cast<const byte_t*>(text), 5}), string("a+b%2Bc"));
}

TEST(PercentTest, RoundTripAllBytes) {
    byte_vector input;
    for (size_t i = 0; i < 256; ++i) {
        input.push_back(static_cast<byte_t>(i));
    }
    const string encoded = percent::encode(input.view());
    EXPECT_EQ(percent::decode(encoded.view()), input);
}
