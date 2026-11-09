#include <NeForce/core/container/vector.hpp>
#include <NeForce/core/exception/exception.hpp>
#include <NeForce/core/kdf/bcrypt.hpp>
#include <NeForce/core/kdf/hkdf.hpp>
#include <NeForce/core/utility/hexadecimal.hpp>
#include <gtest/gtest.h>
using namespace neforce;

namespace {
    byte_vector from_hex(const string_view hex) {
        byte_vector result;
        result.reserve(hex.size() / 2);
        for (size_t i = 0; i + 1 < hex.size(); i += 2) {
            const byte_t high = hexadecimal::xdigit_value(hex[i]);
            const byte_t low = hexadecimal::xdigit_value(hex[i + 1]);
            result.push_back(static_cast<byte_t>((high << 4) | low));
        }
        return result;
    }

    byte_vector repeat_byte(const size_t count, const byte_t value) {
        byte_vector result;
        result.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            result.push_back(value);
        }
        return result;
    }

    byte_vector make_sequence(const size_t count, const size_t offset = 0) {
        byte_vector result;
        result.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            result.push_back(static_cast<byte_t>(offset + i));
        }
        return result;
    }
} // namespace


TEST(HKDFTest, Rfc5869TestCase1) {
    const byte_vector ikm = repeat_byte(22, 0x0b);
    const byte_vector salt = make_sequence(13, 0x00);
    const byte_vector info = make_sequence(10, 0xf0);

    const byte_vector prk = HKDF::extract(salt.view(), ikm.view());
    EXPECT_EQ(prk, from_hex("077709362c2e32df0ddc3f0dc47bba6390b6c73bb50f9c3122ec844ad7c2b3e5"));

    const byte_vector okm = HKDF::expand(prk.view(), info.view(), 42);
    EXPECT_EQ(okm, from_hex("3cb25f25faacd57a90434f64d0362f2a2d2d0a90cf1a5a4c5db02d56ecc4c5bf"
                            "34007208d5b887185865"));

    EXPECT_EQ(HKDF::derive(ikm.view(), salt.view(), info.view(), 42), okm);
    EXPECT_EQ(hkdf_sha256(ikm.view(), salt.view(), info.view(), 42), okm);
}

TEST(HKDFTest, Rfc5869TestCase2) {
    const byte_vector ikm = make_sequence(80, 0x00);
    const byte_vector salt = make_sequence(80, 0x60);
    const byte_vector info = make_sequence(80, 0xb0);

    const byte_vector prk = HKDF::extract(salt.view(), ikm.view());
    EXPECT_EQ(prk, from_hex("06a6b88c5853361a06104c9ceb35b45cef760014904671014a193f40c15fc244"));

    const byte_vector okm = HKDF::derive(ikm.view(), salt.view(), info.view(), 82);
    EXPECT_EQ(okm, from_hex("b11e398dc80327a1c8e7f78c596a49344f012eda2d4efad8a050cc4c19afa97c"
                            "59045a99cac7827271cb41c65e590e09da3275600c2f09b8367793a9aca3db71"
                            "cc30c58179ec3e87c14c01d5c1f3434f1d87"));
}

TEST(HKDFTest, Rfc5869TestCase3EmptySaltAndInfo) {
    const byte_vector ikm = repeat_byte(22, 0x0b);

    const byte_vector prk = HKDF::extract(cbyte_view{}, ikm.view());
    EXPECT_EQ(prk, from_hex("19ef24a32c717b167f33a91d6f648bdf96596776afdb6377ac434c1c293ccb04"));

    const byte_vector okm = HKDF::derive(ikm.view(), cbyte_view{}, cbyte_view{}, 42);
    EXPECT_EQ(okm, from_hex("8da4e775a563c18f715f802a063c5a31b8a11f5c5ee1879ec3454e5f3c738d2d"
                            "9d201395faa4b61a96c8"));
}

TEST(HKDFTest, EmptySaltEqualsZeroSalt) {
    const byte_vector ikm = repeat_byte(32, 0x01);
    const byte_vector zero_salt = repeat_byte(32, 0x00);
    EXPECT_EQ(HKDF::extract(cbyte_view{}, ikm.view()), HKDF::extract(zero_salt.view(), ikm.view()));
}

TEST(HKDFTest, Sha512KnownVector) {
    const byte_vector ikm = repeat_byte(22, 0x0b);
    const byte_vector salt = make_sequence(13, 0x00);
    const byte_vector info = make_sequence(10, 0xf0);

    const byte_vector prk = HKDF_SHA512::extract(salt.view(), ikm.view());
    EXPECT_EQ(prk, from_hex("665799823737ded04a88e47e54a5890bb2c3d247c7a4254a8e61350723590a26"
                            "c36238127d8661b88cf80ef802d57e2f7cebcf1e00e083848be19929c61b4237"));

    const byte_vector okm = HKDF_SHA512::derive(ikm.view(), salt.view(), info.view(), 82);
    EXPECT_EQ(okm, from_hex("832390086cda71fb47625bb5ceb168e4c8e26a1a16ed34d9fc7fe92c14815793"
                            "38da362cb8d9f925d7cbcce0dff7098769cf15959867d571c1715450cb530137"
                            "be3fb62f3cf32b84feba8f1eb1b563e20d97"));
    EXPECT_EQ(hkdf_sha512(ikm.view(), salt.view(), info.view(), 82), okm);
}

TEST(HKDFTest, ZeroLengthReturnsEmpty) {
    const byte_vector ikm = repeat_byte(16, 0x02);
    const byte_vector prk = HKDF::extract(cbyte_view{}, ikm.view());
    EXPECT_TRUE(HKDF::expand(prk.view(), cbyte_view{}, 0).empty());
    EXPECT_TRUE(HKDF::derive(ikm.view(), cbyte_view{}, cbyte_view{}, 0).empty());
}

TEST(HKDFTest, OutputLongerThanDigest) {
    const byte_vector ikm = repeat_byte(16, 0x03);
    const byte_vector okm = HKDF::derive(ikm.view(), cbyte_view{}, cbyte_view{}, 100);
    ASSERT_EQ(okm.size(), static_cast<size_t>(100));
    EXPECT_EQ(HKDF::derive(ikm.view(), cbyte_view{}, cbyte_view{}, 100), okm);
}

TEST(HKDFTest, Sha256LengthLimitThrows) {
    const byte_vector ikm = repeat_byte(16, 0x04);
    const byte_vector prk = HKDF::extract(cbyte_view{}, ikm.view());
    EXPECT_EQ(HKDF::expand(prk.view(), cbyte_view{}, 255 * 32).size(), static_cast<size_t>(255 * 32));
    EXPECT_THROW(HKDF::expand(prk.view(), cbyte_view{}, 255 * 32 + 1), value_exception);
}

TEST(HKDFTest, Sha512LengthLimitThrows) {
    const byte_vector ikm = repeat_byte(16, 0x05);
    const byte_vector prk = HKDF_SHA512::extract(cbyte_view{}, ikm.view());
    EXPECT_THROW(HKDF_SHA512::expand(prk.view(), cbyte_view{}, 255 * 64 + 1), value_exception);
}

TEST(BcryptTest, KnownVectorPassword) {
    const byte_t salt[] = {0x3f, 0xfb, 0x2a, 0xfb, 0x03, 0x50, 0x91, 0xe9,
                           0xa2, 0xcf, 0x86, 0xce, 0x4d, 0xba, 0x8e, 0xd2};
    EXPECT_EQ(bcrypt::hash(string_view{"password"}, cbyte_view{salt, 16}, 10),
              "$2b$10$N9qo8uLOickgx2ZMRZoMye8fOsiTWZqYtkxvXkKm8BMzjT7t/vIdq");
}

TEST(BcryptTest, KnownVectorAbc) {
    const byte_t salt[] = {0x71, 0xd7, 0x9f, 0x82, 0x18, 0xa3, 0x92, 0x59,
                           0xa7, 0xa2, 0x9a, 0xab, 0xb2, 0xdb, 0xaf, 0xc3};
    EXPECT_EQ(bcrypt::hash(string_view{"abc"}, cbyte_view{salt, 16}, 4),
              "$2b$04$abcdefghijklmnopqrstuuCi15uRb1eH7NAlJ/TgeJertyknQpYn2");
}

TEST(BcryptTest, VerifyKnownVectors) {
    EXPECT_TRUE(bcrypt::verify(string_view{"password"},
                               string_view{"$2b$10$N9qo8uLOickgx2ZMRZoMye8fOsiTWZqYtkxvXkKm8BMzjT7t/vIdq"}));
    EXPECT_FALSE(bcrypt::verify(string_view{"Password"},
                                string_view{"$2b$10$N9qo8uLOickgx2ZMRZoMye8fOsiTWZqYtkxvXkKm8BMzjT7t/vIdq"}));
    EXPECT_TRUE(bcrypt::verify(string_view{"password"},
                               string_view{"$2a$10$N9qo8uLOickgx2ZMRZoMye8fOsiTWZqYtkxvXkKm8BMzjT7t/vIdq"}));
    EXPECT_TRUE(bcrypt::verify(string_view{"password"},
                               string_view{"$2y$10$N9qo8uLOickgx2ZMRZoMye8fOsiTWZqYtkxvXkKm8BMzjT7t/vIdq"}));
}

TEST(BcryptTest, RandomSaltRoundTrip) {
    const string hashed = bcrypt::hash(string_view{"correct horse battery staple"}, 4);
    EXPECT_EQ(hashed.size(), static_cast<size_t>(60));
    EXPECT_TRUE(bcrypt::verify(string_view{"correct horse battery staple"}, hashed.view()));
    EXPECT_FALSE(bcrypt::verify(string_view{"wrong password"}, hashed.view()));

    const string second = bcrypt::hash(string_view{"correct horse battery staple"}, 4);
    EXPECT_NE(hashed, second);
}

TEST(BcryptTest, InvalidParameters) {
    EXPECT_THROW(bcrypt::hash(string_view{"x"}, 3), value_exception);
    EXPECT_THROW(bcrypt::hash(string_view{"x"}, 32), value_exception);
    const byte_t short_salt[15] = {};
    EXPECT_THROW(bcrypt::hash(string_view{"x"}, cbyte_view{short_salt, 15}, 4), value_exception);
}

TEST(BcryptTest, InvalidHashIsRejected) {
    EXPECT_FALSE(bcrypt::verify(string_view{"x"}, string_view{"not-a-hash"}));
    EXPECT_FALSE(bcrypt::is_valid_hash(string_view{"not-a-hash"}));
    EXPECT_FALSE(bcrypt::is_valid_hash(string_view{"$2b$03$abcdefghijklmnopqrstuuCi15uRb1eH7NAlJ/TgeJertyknQpYn2"}));
    EXPECT_TRUE(bcrypt::is_valid_hash(string_view{"$2b$10$N9qo8uLOickgx2ZMRZoMye8fOsiTWZqYtkxvXkKm8BMzjT7t/vIdq"}));
}

TEST(BcryptTest, LongPasswordTruncatedAt72Bytes) {
    const byte_t salt[16] = {};
    string password72;
    for (size_t i = 0; i < 72; ++i) {
        password72.push_back('a');
    }
    string password73 = password72;
    password73.push_back('b');
    EXPECT_EQ(bcrypt::hash(password72.view(), cbyte_view{salt, 16}, 4),
              bcrypt::hash(password73.view(), cbyte_view{salt, 16}, 4));
}
