#include <NeForce/core/simd/bytes.hpp>
#include <NeForce/core/simd/memory.hpp>
#include <NeForce/core/simd/bitwise.hpp>
#include <NeForce/core/simd/arithmetic.hpp>
#include <NeForce/core/simd/compare.hpp>
#include <gtest/gtest.h>
using namespace neforce;

class SimdUtilTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(SimdUtilTest, Vec128DefaultSizeIs16) {
    simd::vec128_t v{};
    EXPECT_EQ(sizeof(v), 16);
}

TEST_F(SimdUtilTest, Vec128fSizeIs16) { EXPECT_EQ(sizeof(simd::vec128f_t), 16); }

TEST_F(SimdUtilTest, Vec128dSizeIs16) { EXPECT_EQ(sizeof(simd::vec128d_t), 16); }

TEST_F(SimdUtilTest, Vec256SizeIs32) { EXPECT_EQ(sizeof(simd::vec256_t), 32); }

TEST_F(SimdUtilTest, Vec256fSizeIs32) { EXPECT_EQ(sizeof(simd::vec256f_t), 32); }

TEST_F(SimdUtilTest, Vec256dSizeIs32) { EXPECT_EQ(sizeof(simd::vec256d_t), 32); }

TEST_F(SimdUtilTest, Vec512SizeIs64) { EXPECT_EQ(sizeof(simd::vec512_t), 64); }

TEST_F(SimdUtilTest, Vec512fSizeIs64) { EXPECT_EQ(sizeof(simd::vec512f_t), 64); }

TEST_F(SimdUtilTest, Vec512dSizeIs64) { EXPECT_EQ(sizeof(simd::vec512d_t), 64); }

TEST_F(SimdUtilTest, FillByteAllEqual) {
    simd::vec128_t v = simd::fill_i8(0x42);
    const byte_t* bytes = reinterpret_cast<const byte_t*>(&v);
    for (int i = 0; i < 16; ++i) {
        EXPECT_EQ(bytes[i], 0x42);
    }
}

TEST_F(SimdUtilTest, FillByteZero) {
    simd::vec128_t v = simd::fill_i8(0);
    const byte_t* bytes = reinterpret_cast<const byte_t*>(&v);
    for (int i = 0; i < 16; ++i) {
        EXPECT_EQ(bytes[i], 0);
    }
}

TEST_F(SimdUtilTest, FillByteMax) {
    simd::vec128_t v = simd::fill_i8(0xFF);
    const byte_t* bytes = reinterpret_cast<const byte_t*>(&v);
    for (int i = 0; i < 16; ++i) {
        EXPECT_EQ(bytes[i], 0xFF);
    }
}

TEST_F(SimdUtilTest, FillByteHighBitSet) {
    simd::vec128_t v = simd::fill_i8(0x80);
    const byte_t* bytes = reinterpret_cast<const byte_t*>(&v);
    for (int i = 0; i < 16; ++i) {
        EXPECT_EQ(bytes[i], 0x80);
    }
}

TEST_F(SimdUtilTest, LoadUnalignedFromArray) {
    byte_t data[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    simd::vec128_t v = simd::load_unaligned(data);
    const byte_t* bytes = reinterpret_cast<const byte_t*>(&v);
    for (int i = 0; i < 16; ++i) {
        EXPECT_EQ(bytes[i], i);
    }
}

TEST_F(SimdUtilTest, LoadUnalignedOffsetPointer) {
    byte_t buffer[17] = {0xFF, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    simd::vec128_t v = simd::load_unaligned(buffer + 1);
    const byte_t* bytes = reinterpret_cast<const byte_t*>(&v);
    for (int i = 0; i < 16; ++i) {
        EXPECT_EQ(bytes[i], i);
    }
}

TEST_F(SimdUtilTest, MatchBytesIdenticalVectors) {
    simd::vec128_t a = simd::fill_i8(0x55);
    simd::vec128_t b = simd::fill_i8(0x55);
    simd::vec128_t result = simd::match_bytes(a, b);
    const byte_t* bytes = reinterpret_cast<const byte_t*>(&result);
    for (int i = 0; i < 16; ++i) {
        EXPECT_EQ(bytes[i], 0xFF);
    }
}

TEST_F(SimdUtilTest, MatchBytesDifferentVectors) {
    simd::vec128_t a = simd::fill_i8(0x55);
    simd::vec128_t b = simd::fill_i8(0xAA);
    simd::vec128_t result = simd::match_bytes(a, b);
    const byte_t* bytes = reinterpret_cast<const byte_t*>(&result);
    for (int i = 0; i < 16; ++i) {
        EXPECT_EQ(bytes[i], 0x00);
    }
}

TEST_F(SimdUtilTest, MatchBytesPartialMatch) {
    byte_t data_a[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    byte_t data_b[16] = {1, 2, 3, 99, 5, 6, 99, 8, 9, 10, 11, 12, 13, 14, 99, 16};
    simd::vec128_t a = simd::load_unaligned(data_a);
    simd::vec128_t b = simd::load_unaligned(data_b);
    simd::vec128_t result = simd::match_bytes(a, b);
    const byte_t* bytes = reinterpret_cast<const byte_t*>(&result);
    EXPECT_EQ(bytes[0], 0xFF);
    EXPECT_EQ(bytes[1], 0xFF);
    EXPECT_EQ(bytes[2], 0xFF);
    EXPECT_EQ(bytes[3], 0x00);
    EXPECT_EQ(bytes[4], 0xFF);
    EXPECT_EQ(bytes[5], 0xFF);
    EXPECT_EQ(bytes[6], 0x00);
    EXPECT_EQ(bytes[7], 0xFF);
    EXPECT_EQ(bytes[14], 0x00);
    EXPECT_EQ(bytes[15], 0xFF);
}

TEST_F(SimdUtilTest, ToBitmaskAllZeros) {
    simd::vec128_t v = simd::fill_i8(0x00);
    int mask = simd::to_bitmask(v);
    EXPECT_EQ(mask, 0);
}

TEST_F(SimdUtilTest, ToBitmaskAllHighBits) {
    simd::vec128_t v = simd::fill_i8(0x80);
    int mask = simd::to_bitmask(v);
    EXPECT_EQ(mask, 0xFFFF);
}

TEST_F(SimdUtilTest, ToBitmaskAlternating) {
    byte_t data[16] = {0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00};
    simd::vec128_t v = simd::load_unaligned(data);
    int mask = simd::to_bitmask(v);
    EXPECT_EQ(mask, 0x5555);
}

TEST_F(SimdUtilTest, ToBitmaskBitsSetAbove7) {
    byte_t data[16] = {0x81, 0x82, 0x80, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    simd::vec128_t v = simd::load_unaligned(data);
    int mask = simd::to_bitmask(v);
    EXPECT_EQ(mask, 0x000F);
}

TEST_F(SimdUtilTest, FillThenMatchThenToBitmask) {
    simd::vec128_t a = simd::fill_i8(0x7F);
    simd::vec128_t b = simd::fill_i8(0x7F);
    simd::vec128_t matched = simd::match_bytes(a, b);
    int mask = simd::to_bitmask(matched);
    EXPECT_EQ(mask, 0xFFFF);
}

TEST_F(SimdUtilTest, FillThenMatchDifferentThenToBitmask) {
    simd::vec128_t a = simd::fill_i8(0x42);
    simd::vec128_t b = simd::fill_i8(0x43);
    simd::vec128_t matched = simd::match_bytes(a, b);
    int mask = simd::to_bitmask(matched);
    EXPECT_EQ(mask, 0);
}

TEST_F(SimdUtilTest, LoadAndMatchAtVariousOffsets) {
    byte_t source[32];
    for (int i = 0; i < 32; ++i) {
        source[i] = static_cast<byte_t>(i);
    }
    for (int offset = 0; offset < 16; ++offset) {
        simd::vec128_t a = simd::load_unaligned(source + offset);
        const byte_t* a_bytes = reinterpret_cast<const byte_t*>(&a);
        for (int i = 0; i < 16; ++i) {
            EXPECT_EQ(a_bytes[i], offset + i);
        }
    }
}

TEST_F(SimdUtilTest, ContainsByteFound) {
    simd::vec128_t v = simd::fill_i8(0x00);
    const byte_t b = byte_t{0x00};
    v = simd::load_unaligned(&b);
    byte_t data[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x7F, 0, 0};
    v = simd::load_unaligned(data);
    EXPECT_TRUE(simd::contains_byte(v, 0x7F));
}

TEST_F(SimdUtilTest, ContainsByteNotFound) {
    simd::vec128_t v = simd::fill_i8(0x42);
    EXPECT_FALSE(simd::contains_byte(v, 0x00));
}

TEST_F(SimdUtilTest, ContainsByteFirstPosition) {
    byte_t data[16] = {0xAB, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    simd::vec128_t v = simd::load_unaligned(data);
    EXPECT_TRUE(simd::contains_byte(v, 0xAB));
}

TEST_F(SimdUtilTest, ContainsByteLastPosition) {
    byte_t data[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xCD};
    simd::vec128_t v = simd::load_unaligned(data);
    EXPECT_TRUE(simd::contains_byte(v, 0xCD));
}

TEST_F(SimdUtilTest, FindFirstByteFound) {
    byte_t data[16] = {0, 0, 0, 0x5A, 0, 0x5A, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    simd::vec128_t v = simd::load_unaligned(data);
    EXPECT_EQ(simd::find_first_byte(v, 0x5A), 3);
}

TEST_F(SimdUtilTest, FindFirstByteAtZero) {
    byte_t data[16] = {0x5A, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    simd::vec128_t v = simd::load_unaligned(data);
    EXPECT_EQ(simd::find_first_byte(v, 0x5A), 0);
}

TEST_F(SimdUtilTest, FindFirstByteNotFound) {
    simd::vec128_t v = simd::fill_i8(0x42);
    EXPECT_EQ(simd::find_first_byte(v, 0x99), -1);
}

TEST_F(SimdUtilTest, FindLastByteFound) {
    byte_t data[16] = {0x5A, 0, 0, 0, 0x5A, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    simd::vec128_t v = simd::load_unaligned(data);
    EXPECT_EQ(simd::find_last_byte(v, 0x5A), 4);
}

TEST_F(SimdUtilTest, FindLastByteAtEnd) {
    byte_t data[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x5A};
    simd::vec128_t v = simd::load_unaligned(data);
    EXPECT_EQ(simd::find_last_byte(v, 0x5A), 15);
}

TEST_F(SimdUtilTest, FindLastByteNotFound) {
    simd::vec128_t v = simd::fill_i8(0x42);
    EXPECT_EQ(simd::find_last_byte(v, 0x99), -1);
}

TEST_F(SimdUtilTest, CountByteNone) {
    simd::vec128_t v = simd::fill_i8(0x42);
    EXPECT_EQ(simd::count_byte(v, 0x00), 0);
}

TEST_F(SimdUtilTest, CountByteAll) {
    simd::vec128_t v = simd::fill_i8(0x7F);
    EXPECT_EQ(simd::count_byte(v, 0x7F), 16);
}

TEST_F(SimdUtilTest, CountBytePartial) {
    byte_t data[16] = {1, 1, 2, 1, 3, 1, 4, 1, 5, 1, 6, 1, 7, 1, 8, 1};
    simd::vec128_t v = simd::load_unaligned(data);
    EXPECT_EQ(simd::count_byte(v, 1), 9);
}

TEST_F(SimdUtilTest, IsAllZeroTrue) {
    simd::vec128_t v = simd::fill_i8(0x00);
    EXPECT_TRUE(simd::is_all_zero(v));
}

TEST_F(SimdUtilTest, IsAllZeroFalse) {
    simd::vec128_t v = simd::fill_i8(0x01);
    EXPECT_FALSE(simd::is_all_zero(v));
}

TEST_F(SimdUtilTest, IsAllZeroSingleBit) {
    byte_t data[16] = {};
    data[9] = 0x01;
    simd::vec128_t v = simd::load_unaligned(data);
    EXPECT_FALSE(simd::is_all_zero(v));
}

TEST_F(SimdUtilTest, HasAnyZeroTrue) {
    byte_t data[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0};
    simd::vec128_t v = simd::load_unaligned(data);
    EXPECT_TRUE(simd::has_any_zero(v));
}

TEST_F(SimdUtilTest, HasAnyZeroFalse) {
    simd::vec128_t v = simd::fill_i8(0xFF);
    EXPECT_FALSE(simd::has_any_zero(v));
}

TEST_F(SimdUtilTest, HasAnyZeroAtStart) {
    byte_t data[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    simd::vec128_t v = simd::load_unaligned(data);
    EXPECT_TRUE(simd::has_any_zero(v));
}

TEST_F(SimdUtilTest, ReverseBytesIdentity) {
    byte_t data[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    simd::vec128_t v = simd::load_unaligned(data);
    simd::vec128_t r = simd::reverse_bytes(simd::reverse_bytes(v));
    simd::vec128_t matched = simd::match_bytes(v, r);
    EXPECT_EQ(simd::to_bitmask(matched), 0xFFFF);
}

TEST_F(SimdUtilTest, ReverseBytesCorrect) {
    byte_t data[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    simd::vec128_t v = simd::load_unaligned(data);
    simd::vec128_t r = simd::reverse_bytes(v);
    const byte_t* rb = reinterpret_cast<const byte_t*>(&r);
    for (int i = 0; i < 16; ++i) {
        EXPECT_EQ(rb[i], 15 - i);
    }
}

TEST_F(SimdUtilTest, ShuffleBytesIdentity) {
    byte_t data[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    simd::vec128_t v = simd::load_unaligned(data);
    simd::vec128_t indices = simd::load_unaligned(data);
    simd::vec128_t r = simd::shuffle_bytes(v, indices);
    simd::vec128_t matched = simd::match_bytes(v, r);
    EXPECT_EQ(simd::to_bitmask(matched), 0xFFFF);
}

TEST_F(SimdUtilTest, ShuffleBytesReverse) {
    byte_t src[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    byte_t idx[16] = {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    simd::vec128_t v = simd::load_unaligned(src);
    simd::vec128_t indices = simd::load_unaligned(idx);
    simd::vec128_t r = simd::shuffle_bytes(v, indices);
    simd::vec128_t expected = simd::reverse_bytes(v);
    simd::vec128_t matched = simd::match_bytes(r, expected);
    EXPECT_EQ(simd::to_bitmask(matched), 0xFFFF);
}

TEST_F(SimdUtilTest, ShuffleBytesBroadcastFirst) {
    byte_t src[16] = {0xAA, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    simd::vec128_t v = simd::load_unaligned(src);
    simd::vec128_t indices = simd::fill_i8(0);
    simd::vec128_t r = simd::shuffle_bytes(v, indices);
    EXPECT_TRUE(simd::contains_byte(r, 0xAA));
    EXPECT_EQ(simd::count_byte(r, 0xAA), 16);
}

TEST_F(SimdUtilTest, BlendBytesSelectA) {
    simd::vec128_t a = simd::fill_i8(0xAA);
    simd::vec128_t b = simd::fill_i8(0x55);
    simd::vec128_t mask = simd::fill_i8(0x00);
    simd::vec128_t r = simd::blend_bytes(a, b, mask);
    simd::vec128_t matched = simd::match_bytes(r, a);
    EXPECT_EQ(simd::to_bitmask(matched), 0xFFFF);
}

TEST_F(SimdUtilTest, BlendBytesSelectB) {
    simd::vec128_t a = simd::fill_i8(0xAA);
    simd::vec128_t b = simd::fill_i8(0x55);
    simd::vec128_t mask = simd::fill_i8(0x80);
    simd::vec128_t r = simd::blend_bytes(a, b, mask);
    simd::vec128_t matched = simd::match_bytes(r, b);
    EXPECT_EQ(simd::to_bitmask(matched), 0xFFFF);
}

TEST_F(SimdUtilTest, BlendBytesAlternating) {
    byte_t data_a[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    byte_t data_b[16] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
    byte_t mask_data[16] = {0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00,
                            0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00};
    simd::vec128_t a = simd::load_unaligned(data_a);
    simd::vec128_t b = simd::load_unaligned(data_b);
    simd::vec128_t mask = simd::load_unaligned(mask_data);
    simd::vec128_t r = simd::blend_bytes(a, b, mask);
    const byte_t* rb = reinterpret_cast<const byte_t*>(&r);
    for (int i = 0; i < 16; ++i) {
        EXPECT_EQ(rb[i], (i % 2 == 0) ? 1 : 0);
    }
}

TEST_F(SimdUtilTest, StoreBytesNRoundTrip) {
    byte_t src[16] = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120, 130, 140, 150, 160};
    simd::vec128_t v = simd::load_unaligned(src);
    byte_t dst[16] = {};
    simd::store_bytes_n(dst, v, 7);
    for (int i = 0; i < 7; ++i) {
        EXPECT_EQ(dst[i], src[i]);
    }
    for (int i = 7; i < 16; ++i) {
        EXPECT_EQ(dst[i], 0);
    }
}

TEST_F(SimdUtilTest, StoreBytesNZeroBytes) {
    simd::vec128_t v = simd::fill_i8(0xFF);
    byte_t dst[16] = {};
    simd::store_bytes_n(dst, v, 0);
    for (int i = 0; i < 16; ++i) {
        EXPECT_EQ(dst[i], 0);
    }
}

TEST_F(SimdUtilTest, LoadBytesNRoundTrip) {
    byte_t src[16] = {5, 10, 15, 20, 25, 30, 35, 40, 0, 0, 0, 0, 0, 0, 0, 0};
    simd::vec128_t v = simd::load_bytes_n(src, 8);
    const byte_t* vb = reinterpret_cast<const byte_t*>(&v);
    for (int i = 0; i < 8; ++i) {
        EXPECT_EQ(vb[i], src[i]);
    }
    for (int i = 8; i < 16; ++i) {
        EXPECT_EQ(vb[i], 0);
    }
}

TEST_F(SimdUtilTest, LoadBytesNFull16) {
    byte_t src[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    simd::vec128_t v = simd::load_bytes_n(src, 16);
    simd::vec128_t ref = simd::load_unaligned(src);
    simd::vec128_t matched = simd::match_bytes(v, ref);
    EXPECT_EQ(simd::to_bitmask(matched), 0xFFFF);
}

TEST_F(SimdUtilTest, LoadAlignedRoundTrip) {
    alignas(16) byte_t data[16] = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120, 130, 140, 150, 160};
    simd::vec128_t v = simd::load_aligned(data);
    alignas(16) byte_t dst[16] = {};
    simd::store_aligned(dst, v);
    for (int i = 0; i < 16; ++i) {
        EXPECT_EQ(dst[i], data[i]);
    }
}

TEST_F(SimdUtilTest, LoaduSi128RoundTrip) {
    byte_t data[16] = {1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31};
    simd::vec128_t v = simd::loadu_si128(data);
    byte_t dst[16] = {};
    simd::storeu_si128(dst, v);
    for (int i = 0; i < 16; ++i) {
        EXPECT_EQ(dst[i], data[i]);
    }
}

TEST_F(SimdUtilTest, LoaduSi256RoundTrip) {
    byte_t data[32];
    for (int i = 0; i < 32; ++i) {
        data[i] = static_cast<byte_t>(i * 3);
    }
    simd::vec256_t v = simd::loadu_si256(data);
    byte_t dst[32] = {};
    simd::storeu_si256(dst, v);
    for (int i = 0; i < 32; ++i) {
        EXPECT_EQ(dst[i], data[i]);
    }
}

TEST_F(SimdUtilTest, LoaduSi512RoundTrip) {
    byte_t data[64];
    for (int i = 0; i < 64; ++i) {
        data[i] = static_cast<byte_t>(i * 7 + 1);
    }
    simd::vec512_t v = simd::loadu_si512(data);
    byte_t dst[64] = {};
    simd::storeu_si512(dst, v);
    for (int i = 0; i < 64; ++i) {
        EXPECT_EQ(dst[i], data[i]);
    }
}

TEST_F(SimdUtilTest, LoaduPsRoundTrip) {
    float data[4] = {1.0f, 2.5f, -3.0f, 0.0f};
    simd::vec128f_t v = simd::loadu_ps(data);
    const float* fv = reinterpret_cast<const float*>(&v);
    for (int i = 0; i < 4; ++i) {
        EXPECT_FLOAT_EQ(fv[i], data[i]);
    }
}

TEST_F(SimdUtilTest, LoaduPdRoundTrip) {
    double data[2] = {3.14159, -2.71828};
    simd::vec128d_t v = simd::loadu_pd(data);
    const double* dv = reinterpret_cast<const double*>(&v);
    for (int i = 0; i < 2; ++i) {
        EXPECT_DOUBLE_EQ(dv[i], data[i]);
    }
}

TEST_F(SimdUtilTest, StoreStreamRoundTrip) {
    alignas(16) byte_t data[16] = {0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30};
    simd::vec128_t v = simd::load_aligned(data);
    alignas(16) byte_t dst[16] = {};
    simd::store_stream(dst, v);
    for (int i = 0; i < 16; ++i) {
        EXPECT_EQ(dst[i], data[i]);
    }
}

TEST_F(SimdUtilTest, StoreStream256RoundTrip) {
    alignas(32) byte_t data[32];
    for (int i = 0; i < 32; ++i) {
        data[i] = static_cast<byte_t>(i + 10);
    }
    simd::vec256_t v = simd::loadu_si256(data);
    alignas(32) byte_t dst[32] = {};
    simd::store_stream256(dst, v);
    for (int i = 0; i < 32; ++i) {
        EXPECT_EQ(dst[i], data[i]);
    }
}

TEST_F(SimdUtilTest, StoreStream512RoundTrip) {
    alignas(64) byte_t data[64];
    for (int i = 0; i < 64; ++i) {
        data[i] = static_cast<byte_t>(i * 2);
    }
    simd::vec512_t v = simd::loadu_si512(data);
    alignas(64) byte_t dst[64] = {};
    simd::store_stream512(dst, v);
    for (int i = 0; i < 64; ++i) {
        EXPECT_EQ(dst[i], data[i]);
    }
}

TEST_F(SimdUtilTest, LoadStreamReadsCorrectly) {
    alignas(16) byte_t data[16] = {5, 15, 25, 35, 45, 55, 65, 75, 85, 95, 105, 115, 125, 135, 145, 155};
    simd::vec128_t v = simd::load_stream(data);
    const byte_t* vb = reinterpret_cast<const byte_t*>(&v);
    for (int i = 0; i < 16; ++i) {
        EXPECT_EQ(vb[i], data[i]);
    }
}

TEST_F(SimdUtilTest, PrefetchDoesNotCrash) {
    byte_t buf[64] = {};
    simd::prefetch_read(buf);
    simd::prefetch_write(buf);
    simd::prefetch_l1(buf);
    simd::prefetch_l2(buf);
    simd::prefetch_nta(buf);
    SUCCEED();
}

TEST_F(SimdUtilTest, BitAnd) {
    simd::vec128_t a = simd::fill_i8(0xFF);
    simd::vec128_t b = simd::fill_i8(0x0F);
    simd::vec128_t r = simd::bit_and(a, b);
    simd::vec128_t expected = simd::fill_i8(0x0F);
    simd::vec128_t matched = simd::match_bytes(r, expected);
    EXPECT_EQ(simd::to_bitmask(matched), 0xFFFF);
}

TEST_F(SimdUtilTest, BitOr) {
    simd::vec128_t a = simd::fill_i8(0xF0);
    simd::vec128_t b = simd::fill_i8(0x0F);
    simd::vec128_t r = simd::bit_or(a, b);
    simd::vec128_t expected = simd::fill_i8(0xFF);
    simd::vec128_t matched = simd::match_bytes(r, expected);
    EXPECT_EQ(simd::to_bitmask(matched), 0xFFFF);
}

TEST_F(SimdUtilTest, BitXor) {
    simd::vec128_t a = simd::fill_i8(0xFF);
    simd::vec128_t b = simd::fill_i8(0xAA);
    simd::vec128_t r = simd::bit_xor(a, b);
    simd::vec128_t expected = simd::fill_i8(0x55);
    simd::vec128_t matched = simd::match_bytes(r, expected);
    EXPECT_EQ(simd::to_bitmask(matched), 0xFFFF);
}

TEST_F(SimdUtilTest, BitNot) {
    simd::vec128_t v = simd::fill_i8(0x00);
    simd::vec128_t r = simd::bit_not(v);
    simd::vec128_t expected = simd::fill_i8(0xFF);
    simd::vec128_t matched = simd::match_bytes(r, expected);
    EXPECT_EQ(simd::to_bitmask(matched), 0xFFFF);
}

TEST_F(SimdUtilTest, BitNotOfNot) {
    simd::vec128_t v = simd::fill_i8(0x3C);
    simd::vec128_t r = simd::bit_not(simd::bit_not(v));
    simd::vec128_t matched = simd::match_bytes(r, v);
    EXPECT_EQ(simd::to_bitmask(matched), 0xFFFF);
}

TEST_F(SimdUtilTest, BitAndNot) {
    simd::vec128_t a = simd::fill_i8(0xFF);
    simd::vec128_t b = simd::fill_i8(0x0F);
    simd::vec128_t r = simd::bit_andnot(a, b);
    simd::vec128_t expected = simd::fill_i8(0xF0);
    simd::vec128_t matched = simd::match_bytes(r, expected);
    EXPECT_EQ(simd::to_bitmask(matched), 0xFFFF);
}

TEST_F(SimdUtilTest, BitAnd256) {
    byte_t da[32], db[32];
    for (int i = 0; i < 32; ++i) {
        da[i] = 0xFF;
        db[i] = static_cast<byte_t>(i);
    }
    simd::vec256_t a = simd::loadu_si256(da);
    simd::vec256_t b = simd::loadu_si256(db);
    simd::vec256_t r = simd::bit_and(a, b);
    byte_t dst[32];
    simd::storeu_si256(dst, r);
    for (int i = 0; i < 32; ++i) {
        EXPECT_EQ(dst[i], db[i]);
    }
}

TEST_F(SimdUtilTest, BitOr256) {
    byte_t da[32], db[32];
    for (int i = 0; i < 32; ++i) {
        da[i] = 0xF0;
        db[i] = 0x0F;
    }
    simd::vec256_t a = simd::loadu_si256(da);
    simd::vec256_t b = simd::loadu_si256(db);
    simd::vec256_t r = simd::bit_or(a, b);
    byte_t dst[32];
    simd::storeu_si256(dst, r);
    for (int i = 0; i < 32; ++i) {
        EXPECT_EQ(dst[i], 0xFF);
    }
}

TEST_F(SimdUtilTest, BitXor256) {
    byte_t da[32], db[32];
    for (int i = 0; i < 32; ++i) {
        da[i] = 0xFF;
        db[i] = 0xAA;
    }
    simd::vec256_t a = simd::loadu_si256(da);
    simd::vec256_t b = simd::loadu_si256(db);
    simd::vec256_t r = simd::bit_xor(a, b);
    byte_t dst[32];
    simd::storeu_si256(dst, r);
    for (int i = 0; i < 32; ++i) {
        EXPECT_EQ(dst[i], 0x55);
    }
}

TEST_F(SimdUtilTest, BitAnd512) {
    byte_t da[64], db[64];
    for (int i = 0; i < 64; ++i) {
        da[i] = 0xFF;
        db[i] = static_cast<byte_t>(i & 0x0F);
    }
    simd::vec512_t a = simd::loadu_si512(da);
    simd::vec512_t b = simd::loadu_si512(db);
    simd::vec512_t r = simd::bit_and(a, b);
    byte_t dst[64];
    simd::storeu_si512(dst, r);
    for (int i = 0; i < 64; ++i) {
        EXPECT_EQ(dst[i], db[i]);
    }
}

TEST_F(SimdUtilTest, ShiftLeftBytesZero) {
    byte_t data[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    simd::vec128_t v = simd::load_unaligned(data);
    simd::vec128_t r = simd::shift_left_bytes<0>(v);
    simd::vec128_t matched = simd::match_bytes(v, r);
    EXPECT_EQ(simd::to_bitmask(matched), 0xFFFF);
}

TEST_F(SimdUtilTest, ShiftLeftBytes4) {
    byte_t data[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    simd::vec128_t v = simd::load_unaligned(data);
    simd::vec128_t r = simd::shift_left_bytes<4>(v);
    const byte_t* rb = reinterpret_cast<const byte_t*>(&r);
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(rb[i], 0);
    }
    for (int i = 4; i < 16; ++i) {
        EXPECT_EQ(rb[i], data[i - 4]);
    }
}

TEST_F(SimdUtilTest, ShiftRightBytes4) {
    byte_t data[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    simd::vec128_t v = simd::load_unaligned(data);
    simd::vec128_t r = simd::shift_right_bytes<4>(v);
    const byte_t* rb = reinterpret_cast<const byte_t*>(&r);
    for (int i = 0; i < 12; ++i) {
        EXPECT_EQ(rb[i], data[i + 4]);
    }
    for (int i = 12; i < 16; ++i) {
        EXPECT_EQ(rb[i], 0);
    }
}

TEST_F(SimdUtilTest, ShiftLeftRightByteRoundTrip) {
    byte_t data[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    simd::vec128_t v = simd::load_unaligned(data);
    simd::vec128_t shifted = simd::shift_left_bytes<3>(v);
    simd::vec128_t r = simd::shift_right_bytes<3>(shifted);
    const byte_t* rb = reinterpret_cast<const byte_t*>(&r);
    for (int i = 0; i < 13; ++i) {
        EXPECT_EQ(rb[i], data[i]);
    }
    for (int i = 13; i < 16; ++i) {
        EXPECT_EQ(rb[i], 0);
    }
}

TEST_F(SimdUtilTest, PopcountAllZeros) {
    simd::vec128_t v = simd::fill_i8(0x00);
    EXPECT_EQ(simd::popcount(v), 0);
}

TEST_F(SimdUtilTest, PopcountAllOnes) {
    simd::vec128_t v = simd::fill_i8(0xFF);
    EXPECT_EQ(simd::popcount(v), 128);
}

TEST_F(SimdUtilTest, PopcountAlternating) {
    simd::vec128_t v = simd::fill_i8(0xAA);
    EXPECT_EQ(simd::popcount(v), 64);
}

TEST_F(SimdUtilTest, PopcountSingleBitEach) {
    byte_t data[16] = {1, 2, 4, 8, 16, 32, 64, 128, 1, 2, 4, 8, 16, 32, 64, 128};
    simd::vec128_t v = simd::load_unaligned(data);
    EXPECT_EQ(simd::popcount(v), 16);
}

TEST_F(SimdUtilTest, AddI8) {
    simd::vec128_t a = simd::fill_i8(1);
    simd::vec128_t b = simd::fill_i8(2);
    simd::vec128_t r = simd::add_i8(a, b);
    simd::vec128_t expected = simd::fill_i8(3);
    simd::vec128_t matched = simd::match_bytes(r, expected);
    EXPECT_EQ(simd::to_bitmask(matched), 0xFFFF);
}

TEST_F(SimdUtilTest, AddI8OverflowWraps) {
    simd::vec128_t a = simd::fill_i8(static_cast<byte_t>(127));
    simd::vec128_t b = simd::fill_i8(1);
    simd::vec128_t r = simd::add_i8(a, b);
    const auto* sb = reinterpret_cast<const int8_t*>(&r);
    for (int i = 0; i < 16; ++i) {
        EXPECT_EQ(sb[i], -128);
    }
}

TEST_F(SimdUtilTest, AddI16) {
    byte_t da[16], db[16];
    auto* sa = reinterpret_cast<int16_t*>(da);
    auto* sb = reinterpret_cast<int16_t*>(db);
    for (int i = 0; i < 8; ++i) {
        sa[i] = 100;
        sb[i] = 200;
    }
    simd::vec128_t a = simd::load_unaligned(da);
    simd::vec128_t b = simd::load_unaligned(db);
    simd::vec128_t r = simd::add_i16(a, b);
    const auto* sr = reinterpret_cast<const int16_t*>(&r);
    for (int i = 0; i < 8; ++i) {
        EXPECT_EQ(sr[i], 300);
    }
}

TEST_F(SimdUtilTest, AddI32) {
    int32_t data_a[4] = {1000, 2000, -3000, INT32_MAX};
    int32_t data_b[4] = {500, -100, 1000, 1};
    simd::vec128_t a = simd::load_unaligned(data_a);
    simd::vec128_t b = simd::load_unaligned(data_b);
    simd::vec128_t r = simd::add_i32(a, b);
    const auto* sr = reinterpret_cast<const int32_t*>(&r);
    EXPECT_EQ(sr[0], 1500);
    EXPECT_EQ(sr[1], 1900);
    EXPECT_EQ(sr[2], -2000);
    EXPECT_EQ(sr[3], INT32_MIN);
}

TEST_F(SimdUtilTest, AddI64) {
    int64_t data_a[2] = {1LL << 60, -(1LL << 60)};
    int64_t data_b[2] = {1LL << 60, 1};
    simd::vec128_t a = simd::load_unaligned(data_a);
    simd::vec128_t b = simd::load_unaligned(data_b);
    simd::vec128_t r = simd::add_i64(a, b);
    const auto* sr = reinterpret_cast<const int64_t*>(&r);
    EXPECT_EQ(sr[0], 2LL << 60);
    EXPECT_EQ(sr[1], -(1LL << 60) + 1);
}

TEST_F(SimdUtilTest, SubI8) {
    simd::vec128_t a = simd::fill_i8(10);
    simd::vec128_t b = simd::fill_i8(3);
    simd::vec128_t r = simd::sub_i8(a, b);
    simd::vec128_t expected = simd::fill_i8(7);
    simd::vec128_t matched = simd::match_bytes(r, expected);
    EXPECT_EQ(simd::to_bitmask(matched), 0xFFFF);
}

TEST_F(SimdUtilTest, SubI16) {
    byte_t da[16], db[16];
    auto* sa = reinterpret_cast<int16_t*>(da);
    auto* sb = reinterpret_cast<int16_t*>(db);
    for (int i = 0; i < 8; ++i) {
        sa[i] = 500;
        sb[i] = 200;
    }
    simd::vec128_t a = simd::load_unaligned(da);
    simd::vec128_t b = simd::load_unaligned(db);
    simd::vec128_t r = simd::sub_i16(a, b);
    const auto* sr = reinterpret_cast<const int16_t*>(&r);
    for (int i = 0; i < 8; ++i) {
        EXPECT_EQ(sr[i], 300);
    }
}

TEST_F(SimdUtilTest, SubI32) {
    int32_t data_a[4] = {5000, 0, -5000, INT32_MAX};
    int32_t data_b[4] = {1000, 1, -2000, -1};
    simd::vec128_t a = simd::load_unaligned(data_a);
    simd::vec128_t b = simd::load_unaligned(data_b);
    simd::vec128_t r = simd::sub_i32(a, b);
    const auto* sr = reinterpret_cast<const int32_t*>(&r);
    EXPECT_EQ(sr[0], 4000);
    EXPECT_EQ(sr[1], -1);
    EXPECT_EQ(sr[2], -3000);
    EXPECT_EQ(sr[3], INT32_MIN);
}

TEST_F(SimdUtilTest, SubI64) {
    int64_t data_a[2] = {10000000000LL, -10000000000LL};
    int64_t data_b[2] = {1, -1};
    simd::vec128_t a = simd::load_unaligned(data_a);
    simd::vec128_t b = simd::load_unaligned(data_b);
    simd::vec128_t r = simd::sub_i64(a, b);
    const auto* sr = reinterpret_cast<const int64_t*>(&r);
    EXPECT_EQ(sr[0], 9999999999LL);
    EXPECT_EQ(sr[1], -9999999999LL);
}

TEST_F(SimdUtilTest, MulloI16) {
    int16_t data_a[8] = {3, -5, 100, 0, 32767, -32768, 7, 11};
    int16_t data_b[8] = {4, 2, 3, -1, 1, 1, 0, -2};
    simd::vec128_t a = simd::load_unaligned(data_a);
    simd::vec128_t b = simd::load_unaligned(data_b);
    simd::vec128_t r = simd::mullo_i16(a, b);
    const auto* sr = reinterpret_cast<const int16_t*>(&r);
    EXPECT_EQ(sr[0], 12);
    EXPECT_EQ(sr[1], -10);
    EXPECT_EQ(sr[2], 300);
    EXPECT_EQ(sr[3], 0);
    EXPECT_EQ(sr[4], 32767);
    EXPECT_EQ(sr[5], -32768);
    EXPECT_EQ(sr[6], 0);
    EXPECT_EQ(sr[7], -22);
}

TEST_F(SimdUtilTest, MulloI32) {
    int32_t data_a[4] = {1000, -500, 0, 0x10000};
    int32_t data_b[4] = {2000, 10, -1, 0x10000};
    simd::vec128_t a = simd::load_unaligned(data_a);
    simd::vec128_t b = simd::load_unaligned(data_b);
    simd::vec128_t r = simd::mullo_i32(a, b);
    const auto* sr = reinterpret_cast<const int32_t*>(&r);
    EXPECT_EQ(sr[0], 2000000);
    EXPECT_EQ(sr[1], -5000);
    EXPECT_EQ(sr[2], 0);
    EXPECT_EQ(sr[3], 0);
}

TEST_F(SimdUtilTest, MulhiI16) {
    int16_t data_a[8] = {1000, 2000, -1000, -2000, 32767, -32768, 1, 0};
    int16_t data_b[8] = {1000, 1000, 1000, 1000, 2, -1, 0, 0};
    simd::vec128_t a = simd::load_unaligned(data_a);
    simd::vec128_t b = simd::load_unaligned(data_b);
    simd::vec128_t r = simd::mulhi_i16(a, b);
    const auto* sr = reinterpret_cast<const int16_t*>(&r);
    EXPECT_EQ(sr[0], static_cast<int16_t>((1000 * 1000) >> 16));
    EXPECT_EQ(sr[1], static_cast<int16_t>((2000 * 1000) >> 16));
    EXPECT_EQ(sr[2], static_cast<int16_t>((-1000 * 1000) >> 16));
    EXPECT_EQ(sr[7], 0);
}

TEST_F(SimdUtilTest, MaddsI8x16) {
    int8_t data_a[16] = {1, 2, 3, 4, 5, 6, 7, 8, -1, -1, -1, -1, 0, 0, 0, 0};
    int8_t data_b[16] = {10, 10, 10, 10, 1, 1, 1, 1, 2, 3, 4, 5, 0, 0, 0, 0};
    simd::vec128_t a = simd::load_unaligned(data_a);
    simd::vec128_t b = simd::load_unaligned(data_b);
    simd::vec128_t r = simd::madds_i8x16(a, b);
    const auto* sr = reinterpret_cast<const int16_t*>(&r);
    EXPECT_EQ(sr[0], 1 * 10 + 2 * 10);
    EXPECT_EQ(sr[1], 3 * 10 + 4 * 10);
    EXPECT_EQ(sr[2], 5 * 1 + 6 * 1);
    EXPECT_EQ(sr[3], 7 * 1 + 8 * 1);
    EXPECT_EQ(sr[4], (-1) * 2 + (-1) * 3);
    EXPECT_EQ(sr[5], (-1) * 4 + (-1) * 5);
    EXPECT_EQ(sr[6], 0);
    EXPECT_EQ(sr[7], 0);
}

TEST_F(SimdUtilTest, SaturatedAddI8) {
    simd::vec128_t a = simd::fill_i8(static_cast<byte_t>(100));
    simd::vec128_t b = simd::fill_i8(static_cast<byte_t>(50));
    simd::vec128_t r = simd::saturated_add_i8(a, b);
    const auto* sb = reinterpret_cast<const int8_t*>(&r);
    for (int i = 0; i < 16; ++i) {
        EXPECT_EQ(sb[i], 127);
    }
}

TEST_F(SimdUtilTest, SaturatedAddI8NoOverflow) {
    simd::vec128_t a = simd::fill_i8(static_cast<byte_t>(10));
    simd::vec128_t b = simd::fill_i8(static_cast<byte_t>(20));
    simd::vec128_t r = simd::saturated_add_i8(a, b);
    const auto* sb = reinterpret_cast<const int8_t*>(&r);
    for (int i = 0; i < 16; ++i) {
        EXPECT_EQ(sb[i], 30);
    }
}

TEST_F(SimdUtilTest, SaturatedAddI8Negative) {
    simd::vec128_t a = simd::fill_i8(static_cast<byte_t>(-100));
    simd::vec128_t b = simd::fill_i8(static_cast<byte_t>(-50));
    simd::vec128_t r = simd::saturated_add_i8(a, b);
    const auto* sb = reinterpret_cast<const int8_t*>(&r);
    for (int i = 0; i < 16; ++i) {
        EXPECT_EQ(sb[i], -128);
    }
}

TEST_F(SimdUtilTest, SaturatedAddI16) {
    int16_t data_a[8] = {20000, 30000, -20000, -30000, 0, 1, -1, 100};
    int16_t data_b[8] = {20000, 10000, -20000, -10000, 0, 0, 0, 32700};
    simd::vec128_t a = simd::load_unaligned(data_a);
    simd::vec128_t b = simd::load_unaligned(data_b);
    simd::vec128_t r = simd::saturated_add_i16(a, b);
    const auto* sr = reinterpret_cast<const int16_t*>(&r);
    EXPECT_EQ(sr[0], 32767);
    EXPECT_EQ(sr[1], 32767);
    EXPECT_EQ(sr[2], -32768);
    EXPECT_EQ(sr[3], -32768);
    EXPECT_EQ(sr[4], 0);
    EXPECT_EQ(sr[7], 32767);
}

TEST_F(SimdUtilTest, SaturatedAddU8) {
    simd::vec128_t a = simd::fill_i8(200);
    simd::vec128_t b = simd::fill_i8(100);
    simd::vec128_t r = simd::saturated_add_u8(a, b);
    const byte_t* rb = reinterpret_cast<const byte_t*>(&r);
    for (int i = 0; i < 16; ++i) {
        EXPECT_EQ(rb[i], 255);
    }
}

TEST_F(SimdUtilTest, SaturatedAddU8NoOverflow) {
    simd::vec128_t a = simd::fill_i8(30);
    simd::vec128_t b = simd::fill_i8(40);
    simd::vec128_t r = simd::saturated_add_u8(a, b);
    const byte_t* rb = reinterpret_cast<const byte_t*>(&r);
    for (int i = 0; i < 16; ++i) {
        EXPECT_EQ(rb[i], 70);
    }
}

TEST_F(SimdUtilTest, SaturatedAddU16) {
    uint16_t data_a[8] = {60000, 50000, 0, 1, 65535, 100, 200, 300};
    uint16_t data_b[8] = {10000, 20000, 0, 0, 1, 65435, 65335, 65235};
    simd::vec128_t a = simd::load_unaligned(data_a);
    simd::vec128_t b = simd::load_unaligned(data_b);
    simd::vec128_t r = simd::saturated_add_u16(a, b);
    const auto* sr = reinterpret_cast<const uint16_t*>(&r);
    EXPECT_EQ(sr[0], 65535);
    EXPECT_EQ(sr[1], 65535);
    EXPECT_EQ(sr[2], 0);
    EXPECT_EQ(sr[3], 1);
    EXPECT_EQ(sr[4], 65535);
}

TEST_F(SimdUtilTest, SaturatedSubI8) {
    simd::vec128_t a = simd::fill_i8(static_cast<byte_t>(-100));
    simd::vec128_t b = simd::fill_i8(static_cast<byte_t>(50));
    simd::vec128_t r = simd::saturated_sub_i8(a, b);
    const auto* sb = reinterpret_cast<const int8_t*>(&r);
    for (int i = 0; i < 16; ++i) {
        EXPECT_EQ(sb[i], -128);
    }
}

TEST_F(SimdUtilTest, SaturatedSubI16) {
    int16_t data_a[8] = {-30000, -30000, 30000, 0};
    int16_t data_b[8] = {10000, -1, -10000, 0};
    simd::vec128_t a = simd::load_unaligned(data_a);
    simd::vec128_t b = simd::load_unaligned(data_b);
    simd::vec128_t r = simd::saturated_sub_i16(a, b);
    const auto* sr = reinterpret_cast<const int16_t*>(&r);
    EXPECT_EQ(sr[0], -32768);
    EXPECT_EQ(sr[1], -29999);
    EXPECT_EQ(sr[2], 32767);
    EXPECT_EQ(sr[3], 0);
}

TEST_F(SimdUtilTest, SaturatedSubU8) {
    simd::vec128_t a = simd::fill_i8(50);
    simd::vec128_t b = simd::fill_i8(100);
    simd::vec128_t r = simd::saturated_sub_u8(a, b);
    const byte_t* rb = reinterpret_cast<const byte_t*>(&r);
    for (int i = 0; i < 16; ++i) {
        EXPECT_EQ(rb[i], 0);
    }
}

TEST_F(SimdUtilTest, SaturatedSubU8Normal) {
    simd::vec128_t a = simd::fill_i8(100);
    simd::vec128_t b = simd::fill_i8(30);
    simd::vec128_t r = simd::saturated_sub_u8(a, b);
    const byte_t* rb = reinterpret_cast<const byte_t*>(&r);
    for (int i = 0; i < 16; ++i) {
        EXPECT_EQ(rb[i], 70);
    }
}

TEST_F(SimdUtilTest, SaturatedSubU16) {
    uint16_t data_a[8] = {100, 200, 50000, 0};
    uint16_t data_b[8] = {200, 100, 60000, 1};
    simd::vec128_t a = simd::load_unaligned(data_a);
    simd::vec128_t b = simd::load_unaligned(data_b);
    simd::vec128_t r = simd::saturated_sub_u16(a, b);
    const auto* sr = reinterpret_cast<const uint16_t*>(&r);
    EXPECT_EQ(sr[0], 0);
    EXPECT_EQ(sr[1], 100);
    EXPECT_EQ(sr[2], 0);
    EXPECT_EQ(sr[3], 0);
}

TEST_F(SimdUtilTest, AbsI8) {
    int8_t data[16] = {-5, -1, 0, 1, 5, -128, 127, -127, 2, -2, 3, -3, 4, -4, 6, -6};
    simd::vec128_t v = simd::load_unaligned(data);
    simd::vec128_t r = simd::abs_i8(v);
    const auto* sb = reinterpret_cast<const int8_t*>(&r);
    EXPECT_EQ(sb[0], 5);
    EXPECT_EQ(sb[1], 1);
    EXPECT_EQ(sb[2], 0);
    EXPECT_EQ(sb[3], 1);
    EXPECT_EQ(sb[4], 5);
    EXPECT_EQ(sb[5], -128);
    EXPECT_EQ(sb[6], 127);
    EXPECT_EQ(sb[7], 127);
}

TEST_F(SimdUtilTest, AbsI16) {
    int16_t data[8] = {-100, -1, 0, 1, 100, -32768, 32767, -32767};
    simd::vec128_t v = simd::load_unaligned(data);
    simd::vec128_t r = simd::abs_i16(v);
    const auto* sr = reinterpret_cast<const int16_t*>(&r);
    EXPECT_EQ(sr[0], 100);
    EXPECT_EQ(sr[1], 1);
    EXPECT_EQ(sr[2], 0);
    EXPECT_EQ(sr[3], 1);
    EXPECT_EQ(sr[4], 100);
    EXPECT_EQ(sr[5], -32768);
    EXPECT_EQ(sr[6], 32767);
}

TEST_F(SimdUtilTest, AbsI32) {
    int32_t data[4] = {-1000, -1, 0, 1};
    simd::vec128_t v = simd::load_unaligned(data);
    simd::vec128_t r = simd::abs_i32(v);
    const auto* sr = reinterpret_cast<const int32_t*>(&r);
    EXPECT_EQ(sr[0], 1000);
    EXPECT_EQ(sr[1], 1);
    EXPECT_EQ(sr[2], 0);
    EXPECT_EQ(sr[3], 1);
}

TEST_F(SimdUtilTest, AbsI32IntMin) {
    int32_t data[4] = {INT32_MIN, INT32_MAX, 0, -1};
    simd::vec128_t v = simd::load_unaligned(data);
    simd::vec128_t r = simd::abs_i32(v);
    const auto* sr = reinterpret_cast<const int32_t*>(&r);
    EXPECT_EQ(sr[0], INT32_MIN);
    EXPECT_EQ(sr[1], INT32_MAX);
}

TEST_F(SimdUtilTest, MinI8) {
    int8_t data_a[16] = {5, -5, 0, 10, -10, 127, -128, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    int8_t data_b[16] = {3, 3, 0, 5, 5, 0, 0, -1, 1, 4, 3, 6, 5, 8, 7, 10};
    simd::vec128_t a = simd::load_unaligned(data_a);
    simd::vec128_t b = simd::load_unaligned(data_b);
    simd::vec128_t r = simd::min_i8(a, b);
    const auto* sr = reinterpret_cast<const int8_t*>(&r);
    EXPECT_EQ(sr[0], 3);
    EXPECT_EQ(sr[1], -5);
    EXPECT_EQ(sr[2], 0);
    EXPECT_EQ(sr[3], 5);
    EXPECT_EQ(sr[4], -10);
    EXPECT_EQ(sr[5], 0);
    EXPECT_EQ(sr[6], -128);
    EXPECT_EQ(sr[7], -1);
}

TEST_F(SimdUtilTest, MinI16) {
    int16_t data_a[8] = {100, -100, 0, 32767, -32768, 500, -500, 1};
    int16_t data_b[8] = {50, 50, 0, 0, 0, 1000, 0, -1};
    simd::vec128_t a = simd::load_unaligned(data_a);
    simd::vec128_t b = simd::load_unaligned(data_b);
    simd::vec128_t r = simd::min_i16(a, b);
    const auto* sr = reinterpret_cast<const int16_t*>(&r);
    EXPECT_EQ(sr[0], 50);
    EXPECT_EQ(sr[1], -100);
    EXPECT_EQ(sr[2], 0);
    EXPECT_EQ(sr[3], 0);
    EXPECT_EQ(sr[4], -32768);
    EXPECT_EQ(sr[5], 500);
    EXPECT_EQ(sr[6], -500);
    EXPECT_EQ(sr[7], -1);
}

TEST_F(SimdUtilTest, MinI32) {
    int32_t data_a[4] = {1000, -1000, 0, INT32_MAX};
    int32_t data_b[4] = {500, 500, 0, INT32_MIN};
    simd::vec128_t a = simd::load_unaligned(data_a);
    simd::vec128_t b = simd::load_unaligned(data_b);
    simd::vec128_t r = simd::min_i32(a, b);
    const auto* sr = reinterpret_cast<const int32_t*>(&r);
    EXPECT_EQ(sr[0], 500);
    EXPECT_EQ(sr[1], -1000);
    EXPECT_EQ(sr[2], 0);
    EXPECT_EQ(sr[3], INT32_MIN);
}

TEST_F(SimdUtilTest, MinU8) {
    simd::vec128_t a = simd::fill_i8(100);
    simd::vec128_t b = simd::fill_i8(200);
    simd::vec128_t r = simd::min_u8(a, b);
    simd::vec128_t expected = simd::fill_i8(100);
    simd::vec128_t matched = simd::match_bytes(r, expected);
    EXPECT_EQ(simd::to_bitmask(matched), 0xFFFF);
}

TEST_F(SimdUtilTest, MinU16) {
    uint16_t data_a[8] = {100, 200, 300, 400, 1000, 2000, 3000, 4000};
    uint16_t data_b[8] = {50, 250, 250, 500, 2000, 1000, 4000, 3000};
    simd::vec128_t a = simd::load_unaligned(data_a);
    simd::vec128_t b = simd::load_unaligned(data_b);
    simd::vec128_t r = simd::min_u16(a, b);
    const auto* sr = reinterpret_cast<const uint16_t*>(&r);
    EXPECT_EQ(sr[0], 50);
    EXPECT_EQ(sr[1], 200);
    EXPECT_EQ(sr[2], 250);
    EXPECT_EQ(sr[3], 400);
}

TEST_F(SimdUtilTest, MinU32) {
    uint32_t data_a[4] = {100, 0xFFFFFFFF, 500, 0};
    uint32_t data_b[4] = {200, 0, 1000, 1};
    simd::vec128_t a = simd::load_unaligned(data_a);
    simd::vec128_t b = simd::load_unaligned(data_b);
    simd::vec128_t r = simd::min_u32(a, b);
    const auto* sr = reinterpret_cast<const uint32_t*>(&r);
    EXPECT_EQ(sr[0], 100u);
    EXPECT_EQ(sr[1], 0u);
    EXPECT_EQ(sr[2], 500u);
    EXPECT_EQ(sr[3], 0u);
}

TEST_F(SimdUtilTest, MaxI8) {
    int8_t data_a[16] = {5, -5, 0, 10, -10, 127, -128, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    int8_t data_b[16] = {3, 3, 0, 5, 5, 0, 0, -1, 1, 4, 3, 6, 5, 8, 7, 10};
    simd::vec128_t a = simd::load_unaligned(data_a);
    simd::vec128_t b = simd::load_unaligned(data_b);
    simd::vec128_t r = simd::max_i8(a, b);
    const auto* sr = reinterpret_cast<const int8_t*>(&r);
    EXPECT_EQ(sr[0], 5);
    EXPECT_EQ(sr[1], 3);
    EXPECT_EQ(sr[2], 0);
    EXPECT_EQ(sr[3], 10);
    EXPECT_EQ(sr[4], 5);
    EXPECT_EQ(sr[5], 127);
    EXPECT_EQ(sr[6], 0);
    EXPECT_EQ(sr[7], 1);
}

TEST_F(SimdUtilTest, MaxI16) {
    int16_t data_a[8] = {100, -100, 0, 32767, -32768, 500, -500, 1};
    int16_t data_b[8] = {50, 50, 0, 0, 0, 1000, 0, -1};
    simd::vec128_t a = simd::load_unaligned(data_a);
    simd::vec128_t b = simd::load_unaligned(data_b);
    simd::vec128_t r = simd::max_i16(a, b);
    const auto* sr = reinterpret_cast<const int16_t*>(&r);
    EXPECT_EQ(sr[0], 100);
    EXPECT_EQ(sr[1], 50);
    EXPECT_EQ(sr[2], 0);
    EXPECT_EQ(sr[3], 32767);
    EXPECT_EQ(sr[4], 0);
    EXPECT_EQ(sr[5], 1000);
    EXPECT_EQ(sr[6], 0);
    EXPECT_EQ(sr[7], 1);
}

TEST_F(SimdUtilTest, MaxI32) {
    int32_t data_a[4] = {1000, -1000, 0, INT32_MIN};
    int32_t data_b[4] = {500, 500, 0, INT32_MAX};
    simd::vec128_t a = simd::load_unaligned(data_a);
    simd::vec128_t b = simd::load_unaligned(data_b);
    simd::vec128_t r = simd::max_i32(a, b);
    const auto* sr = reinterpret_cast<const int32_t*>(&r);
    EXPECT_EQ(sr[0], 1000);
    EXPECT_EQ(sr[1], 500);
    EXPECT_EQ(sr[2], 0);
    EXPECT_EQ(sr[3], INT32_MAX);
}

TEST_F(SimdUtilTest, MaxU8) {
    simd::vec128_t a = simd::fill_i8(100);
    simd::vec128_t b = simd::fill_i8(200);
    simd::vec128_t r = simd::max_u8(a, b);
    simd::vec128_t expected = simd::fill_i8(200);
    simd::vec128_t matched = simd::match_bytes(r, expected);
    EXPECT_EQ(simd::to_bitmask(matched), 0xFFFF);
}

TEST_F(SimdUtilTest, MaxU16) {
    uint16_t data_a[8] = {100, 200, 300, 400, 1000, 2000, 3000, 4000};
    uint16_t data_b[8] = {50, 250, 250, 500, 2000, 1000, 4000, 3000};
    simd::vec128_t a = simd::load_unaligned(data_a);
    simd::vec128_t b = simd::load_unaligned(data_b);
    simd::vec128_t r = simd::max_u16(a, b);
    const auto* sr = reinterpret_cast<const uint16_t*>(&r);
    EXPECT_EQ(sr[0], 100);
    EXPECT_EQ(sr[1], 250);
    EXPECT_EQ(sr[2], 300);
    EXPECT_EQ(sr[3], 500);
}

TEST_F(SimdUtilTest, MaxU32) {
    uint32_t data_a[4] = {100, 0xFFFFFFFF, 500, 0};
    uint32_t data_b[4] = {200, 0, 1000, 1};
    simd::vec128_t a = simd::load_unaligned(data_a);
    simd::vec128_t b = simd::load_unaligned(data_b);
    simd::vec128_t r = simd::max_u32(a, b);
    const auto* sr = reinterpret_cast<const uint32_t*>(&r);
    EXPECT_EQ(sr[0], 200u);
    EXPECT_EQ(sr[1], 0xFFFFFFFFu);
    EXPECT_EQ(sr[2], 1000u);
    EXPECT_EQ(sr[3], 1u);
}

TEST_F(SimdUtilTest, AvgU8) {
    simd::vec128_t a = simd::fill_i8(100);
    simd::vec128_t b = simd::fill_i8(200);
    simd::vec128_t r = simd::avg_u8(a, b);
    const byte_t* rb = reinterpret_cast<const byte_t*>(&r);
    for (int i = 0; i < 16; ++i) {
        EXPECT_EQ(rb[i], 150);
    }
}

TEST_F(SimdUtilTest, AvgU8OddSum) {
    simd::vec128_t a = simd::fill_i8(10);
    simd::vec128_t b = simd::fill_i8(11);
    simd::vec128_t r = simd::avg_u8(a, b);
    const byte_t* rb = reinterpret_cast<const byte_t*>(&r);
    for (int i = 0; i < 16; ++i) {
        EXPECT_EQ(rb[i], 11);
    }
}

TEST_F(SimdUtilTest, AvgU16) {
    uint16_t data_a[8] = {100, 200, 300, 400, 0, 65535, 1000, 2000};
    uint16_t data_b[8] = {200, 100, 400, 300, 0, 0, 2000, 1000};
    simd::vec128_t a = simd::load_unaligned(data_a);
    simd::vec128_t b = simd::load_unaligned(data_b);
    simd::vec128_t r = simd::avg_u16(a, b);
    const auto* sr = reinterpret_cast<const uint16_t*>(&r);
    EXPECT_EQ(sr[0], 150);
    EXPECT_EQ(sr[1], 150);
    EXPECT_EQ(sr[2], 350);
    EXPECT_EQ(sr[3], 350);
    EXPECT_EQ(sr[4], 0);
    EXPECT_EQ(sr[5], 32768);
}

TEST_F(SimdUtilTest, CmpeqI8) {
    simd::vec128_t a = simd::fill_i8(0x55);
    simd::vec128_t b = simd::fill_i8(0x55);
    simd::vec128_t r = simd::cmpeq_i8(a, b);
    EXPECT_EQ(simd::to_bitmask(r), 0xFFFF);
}

TEST_F(SimdUtilTest, CmpeqI8False) {
    simd::vec128_t a = simd::fill_i8(0x55);
    simd::vec128_t b = simd::fill_i8(0xAA);
    simd::vec128_t r = simd::cmpeq_i8(a, b);
    EXPECT_EQ(simd::to_bitmask(r), 0);
}

TEST_F(SimdUtilTest, CmpeqI16) {
    int16_t data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    simd::vec128_t a = simd::load_unaligned(data);
    simd::vec128_t b = simd::load_unaligned(data);
    simd::vec128_t r = simd::cmpeq_i16(a, b);
    EXPECT_EQ(simd::to_bitmask(r), 0xFFFF);
}

TEST_F(SimdUtilTest, CmpeqI16Mixed) {
    int16_t data_a[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    int16_t data_b[8] = {1, 99, 3, 99, 5, 99, 7, 99};
    simd::vec128_t a = simd::load_unaligned(data_a);
    simd::vec128_t b = simd::load_unaligned(data_b);
    simd::vec128_t r = simd::cmpeq_i16(a, b);
    EXPECT_EQ(simd::to_bitmask(r), 0x3333);
}

TEST_F(SimdUtilTest, CmpeqI32) {
    int32_t data[4] = {10, -20, 30, 0};
    simd::vec128_t a = simd::load_unaligned(data);
    simd::vec128_t b = simd::load_unaligned(data);
    simd::vec128_t r = simd::cmpeq_i32(a, b);
    EXPECT_EQ(simd::to_bitmask(r), 0xFFFF);
}

TEST_F(SimdUtilTest, CmpeqI64) {
    int64_t data[2] = {1LL << 40, -(1LL << 40)};
    simd::vec128_t a = simd::load_unaligned(data);
    simd::vec128_t b = simd::load_unaligned(data);
    simd::vec128_t r = simd::cmpeq_i64(a, b);
    EXPECT_EQ(simd::to_bitmask(r), 0xFFFF);
}

TEST_F(SimdUtilTest, CmpeqI64NotEqual) {
    int64_t data_a[2] = {100, 200};
    int64_t data_b[2] = {100, 201};
    simd::vec128_t a = simd::load_unaligned(data_a);
    simd::vec128_t b = simd::load_unaligned(data_b);
    simd::vec128_t r = simd::cmpeq_i64(a, b);
    EXPECT_EQ(simd::to_bitmask(r), 0x00FF);
}

TEST_F(SimdUtilTest, CmpgtI8) {
    int8_t data_a[16] = {10, 5, 0, -5, -10, 127, -128, 0, 1, 2, 3, 4, 5, 6, 7, 8};
    int8_t data_b[16] = {5, 5, 0, 5, 5, 0, 0, 1, 0, 1, 4, 3, 6, 5, 8, 7};
    simd::vec128_t a = simd::load_unaligned(data_a);
    simd::vec128_t b = simd::load_unaligned(data_b);
    simd::vec128_t r = simd::cmpgt_i8(a, b);
    const int mask = simd::to_bitmask(r);
    EXPECT_NE(mask & (1 << 0), 0);
    EXPECT_EQ(mask & (1 << 1), 0);
    EXPECT_EQ(mask & (1 << 2), 0);
    EXPECT_EQ(mask & (1 << 3), 0);
    EXPECT_EQ(mask & (1 << 4), 0);
    EXPECT_NE(mask & (1 << 5), 0);
    EXPECT_NE(mask & (1 << 8), 0);
    EXPECT_NE(mask & (1 << 9), 0);
    EXPECT_NE(mask & (1 << 11), 0);
    EXPECT_NE(mask & (1 << 13), 0);
    EXPECT_NE(mask & (1 << 15), 0);
}

TEST_F(SimdUtilTest, CmpgtI16) {
    int16_t data_a[8] = {100, 0, -100, 32767, -32768, 500, 0, 10};
    int16_t data_b[8] = {50, 0, 50, 0, 0, 1000, -1, 10};
    simd::vec128_t a = simd::load_unaligned(data_a);
    simd::vec128_t b = simd::load_unaligned(data_b);
    simd::vec128_t r = simd::cmpgt_i16(a, b);
    const int mask = simd::to_bitmask(r);
    EXPECT_NE(mask & ((1 << 0) | (1 << 1)), 0);
    EXPECT_EQ(mask & ((1 << 2) | (1 << 3)), 0);
    EXPECT_EQ(mask & ((1 << 4) | (1 << 5)), 0);
    EXPECT_NE(mask & ((1 << 6) | (1 << 7)), 0);
    EXPECT_EQ(mask & ((1 << 8) | (1 << 9)), 0);
    EXPECT_EQ(mask & ((1 << 10) | (1 << 11)), 0);
    EXPECT_NE(mask & ((1 << 12) | (1 << 13)), 0);
    EXPECT_EQ(mask & ((1 << 14) | (1 << 15)), 0);
}

TEST_F(SimdUtilTest, CmpgtI32) {
    int32_t data_a[4] = {1000, 0, -1000, INT32_MAX};
    int32_t data_b[4] = {500, 0, 500, INT32_MIN};
    simd::vec128_t a = simd::load_unaligned(data_a);
    simd::vec128_t b = simd::load_unaligned(data_b);
    simd::vec128_t r = simd::cmpgt_i32(a, b);
    const int mask = simd::to_bitmask(r);
    EXPECT_NE(mask & 0x000F, 0);
    EXPECT_EQ(mask & 0x00F0, 0);
    EXPECT_EQ(mask & 0x0F00, 0);
    EXPECT_NE(mask & 0xF000, 0);
}

TEST_F(SimdUtilTest, CmpgtI64) {
    int64_t data_a[2] = {1000, -5000};
    int64_t data_b[2] = {500, 0};
    simd::vec128_t a = simd::load_unaligned(data_a);
    simd::vec128_t b = simd::load_unaligned(data_b);
    simd::vec128_t r = simd::cmpgt_i64(a, b);
    const int mask = simd::to_bitmask(r);
    EXPECT_EQ(mask, 0x00FF);
}

TEST_F(SimdUtilTest, CmpeqF32AllEqual) {
    float data[4] = {1.5f, -2.0f, 0.0f, 3.14f};
    simd::vec128f_t a = simd::loadu_ps(data);
    simd::vec128f_t b = simd::loadu_ps(data);
    simd::vec128f_t r = simd::cmpeq_f32(a, b);
    const auto* ri = reinterpret_cast<const uint32_t*>(&r);
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(ri[i], 0xFFFFFFFF);
    }
}

TEST_F(SimdUtilTest, CmpeqF32NotEqual) {
    float data_a[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    float data_b[4] = {1.0f, 99.0f, 3.0f, 99.0f};
    simd::vec128f_t a = simd::loadu_ps(data_a);
    simd::vec128f_t b = simd::loadu_ps(data_b);
    simd::vec128f_t r = simd::cmpeq_f32(a, b);
    const auto* ri = reinterpret_cast<const uint32_t*>(&r);
    EXPECT_EQ(ri[0], 0xFFFFFFFF);
    EXPECT_EQ(ri[1], 0u);
    EXPECT_EQ(ri[2], 0xFFFFFFFF);
    EXPECT_EQ(ri[3], 0u);
}

TEST_F(SimdUtilTest, CmpgtF32) {
    float data_a[4] = {5.0f, 2.0f, -1.0f, 0.0f};
    float data_b[4] = {2.0f, 5.0f, -2.0f, 0.0f};
    simd::vec128f_t a = simd::loadu_ps(data_a);
    simd::vec128f_t b = simd::loadu_ps(data_b);
    simd::vec128f_t r = simd::cmpgt_f32(a, b);
    const auto* ri = reinterpret_cast<const uint32_t*>(&r);
    EXPECT_EQ(ri[0], 0xFFFFFFFF);
    EXPECT_EQ(ri[1], 0u);
    EXPECT_EQ(ri[2], 0xFFFFFFFF);
    EXPECT_EQ(ri[3], 0u);
}

TEST_F(SimdUtilTest, CmpgeF32) {
    float data_a[4] = {5.0f, 5.0f, -1.0f, 0.0f};
    float data_b[4] = {2.0f, 5.0f, -2.0f, 0.0f};
    simd::vec128f_t a = simd::loadu_ps(data_a);
    simd::vec128f_t b = simd::loadu_ps(data_b);
    simd::vec128f_t r = simd::cmpge_f32(a, b);
    const auto* ri = reinterpret_cast<const uint32_t*>(&r);
    EXPECT_EQ(ri[0], 0xFFFFFFFF);
    EXPECT_EQ(ri[1], 0xFFFFFFFF);
    EXPECT_EQ(ri[2], 0xFFFFFFFF);
    EXPECT_EQ(ri[3], 0xFFFFFFFF);
}

TEST_F(SimdUtilTest, CmpltF32) {
    float data_a[4] = {2.0f, 5.0f, -2.0f, 0.0f};
    float data_b[4] = {5.0f, 2.0f, -1.0f, 0.0f};
    simd::vec128f_t a = simd::loadu_ps(data_a);
    simd::vec128f_t b = simd::loadu_ps(data_b);
    simd::vec128f_t r = simd::cmplt_f32(a, b);
    const auto* ri = reinterpret_cast<const uint32_t*>(&r);
    EXPECT_EQ(ri[0], 0xFFFFFFFF);
    EXPECT_EQ(ri[1], 0u);
    EXPECT_EQ(ri[2], 0xFFFFFFFF);
    EXPECT_EQ(ri[3], 0u);
}

TEST_F(SimdUtilTest, CmpleF32) {
    float data_a[4] = {2.0f, 5.0f, -2.0f, 0.0f};
    float data_b[4] = {5.0f, 5.0f, -1.0f, 0.0f};
    simd::vec128f_t a = simd::loadu_ps(data_a);
    simd::vec128f_t b = simd::loadu_ps(data_b);
    simd::vec128f_t r = simd::cmple_f32(a, b);
    const auto* ri = reinterpret_cast<const uint32_t*>(&r);
    EXPECT_EQ(ri[0], 0xFFFFFFFF);
    EXPECT_EQ(ri[1], 0xFFFFFFFF);
    EXPECT_EQ(ri[2], 0xFFFFFFFF);
    EXPECT_EQ(ri[3], 0xFFFFFFFF);
}

TEST_F(SimdUtilTest, CmpeqF64) {
    double data[2] = {3.14159, -2.71828};
    simd::vec128d_t a = simd::loadu_pd(data);
    simd::vec128d_t b = simd::loadu_pd(data);
    simd::vec128d_t r = simd::cmpeq_f64(a, b);
    const auto* ri = reinterpret_cast<const uint64_t*>(&r);
    EXPECT_EQ(ri[0], 0xFFFFFFFFFFFFFFFFULL);
    EXPECT_EQ(ri[1], 0xFFFFFFFFFFFFFFFFULL);
}

TEST_F(SimdUtilTest, CmpeqF64NotEqual) {
    double data_a[2] = {1.0, 2.0};
    double data_b[2] = {1.0, 99.0};
    simd::vec128d_t a = simd::loadu_pd(data_a);
    simd::vec128d_t b = simd::loadu_pd(data_b);
    simd::vec128d_t r = simd::cmpeq_f64(a, b);
    const auto* ri = reinterpret_cast<const uint64_t*>(&r);
    EXPECT_EQ(ri[0], 0xFFFFFFFFFFFFFFFFULL);
    EXPECT_EQ(ri[1], 0u);
}

TEST_F(SimdUtilTest, CmpgtF64) {
    double data_a[2] = {5.0, 2.0};
    double data_b[2] = {2.0, 5.0};
    simd::vec128d_t a = simd::loadu_pd(data_a);
    simd::vec128d_t b = simd::loadu_pd(data_b);
    simd::vec128d_t r = simd::cmpgt_f64(a, b);
    const auto* ri = reinterpret_cast<const uint64_t*>(&r);
    EXPECT_EQ(ri[0], 0xFFFFFFFFFFFFFFFFULL);
    EXPECT_EQ(ri[1], 0u);
}

TEST_F(SimdUtilTest, CmpgeF64) {
    double data_a[2] = {5.0, 5.0};
    double data_b[2] = {2.0, 5.0};
    simd::vec128d_t a = simd::loadu_pd(data_a);
    simd::vec128d_t b = simd::loadu_pd(data_b);
    simd::vec128d_t r = simd::cmpge_f64(a, b);
    const auto* ri = reinterpret_cast<const uint64_t*>(&r);
    EXPECT_EQ(ri[0], 0xFFFFFFFFFFFFFFFFULL);
    EXPECT_EQ(ri[1], 0xFFFFFFFFFFFFFFFFULL);
}

TEST_F(SimdUtilTest, CmpltF64) {
    double data_a[2] = {2.0, 5.0};
    double data_b[2] = {5.0, 2.0};
    simd::vec128d_t a = simd::loadu_pd(data_a);
    simd::vec128d_t b = simd::loadu_pd(data_b);
    simd::vec128d_t r = simd::cmplt_f64(a, b);
    const auto* ri = reinterpret_cast<const uint64_t*>(&r);
    EXPECT_EQ(ri[0], 0xFFFFFFFFFFFFFFFFULL);
    EXPECT_EQ(ri[1], 0u);
}

TEST_F(SimdUtilTest, CmpleF64) {
    double data_a[2] = {2.0, 5.0};
    double data_b[2] = {5.0, 5.0};
    simd::vec128d_t a = simd::loadu_pd(data_a);
    simd::vec128d_t b = simd::loadu_pd(data_b);
    simd::vec128d_t r = simd::cmple_f64(a, b);
    const auto* ri = reinterpret_cast<const uint64_t*>(&r);
    EXPECT_EQ(ri[0], 0xFFFFFFFFFFFFFFFFULL);
    EXPECT_EQ(ri[1], 0xFFFFFFFFFFFFFFFFULL);
}

#ifdef NEFORCE_SIMD_SSE2

TEST_F(SimdUtilTest, Sse2Vec128MatchesM128i) { EXPECT_EQ(sizeof(simd::vec128_t), sizeof(::__m128i)); }

TEST_F(SimdUtilTest, Sse2FillByteMatchesIntrinsic) {
    simd::vec128_t v = simd::fill_i8(0x5A);
    ::__m128i ref = ::_mm_set1_epi8(0x5A);
    simd::vec128_t matched = simd::match_bytes(v, simd::load_unaligned(&ref));
    int mask = simd::to_bitmask(matched);
    EXPECT_EQ(mask, 0xFFFF);
}

TEST_F(SimdUtilTest, Sse2ToBitmaskMatchesIntrinsic) {
    ::__m128i v = ::_mm_set1_epi8(static_cast<char>(0x81));
    simd::vec128_t sv = simd::load_unaligned(&v);
    int simd_mask = simd::to_bitmask(sv);
    int ref_mask = ::_mm_movemask_epi8(v);
    EXPECT_EQ(simd_mask, ref_mask);
}

TEST_F(SimdUtilTest, Sse2AddI8MatchesIntrinsic) {
    ::__m128i a = ::_mm_set1_epi8(10);
    ::__m128i b = ::_mm_set1_epi8(20);
    ::__m128i ref = ::_mm_add_epi8(a, b);
    simd::vec128_t sa = simd::load_unaligned(&a);
    simd::vec128_t sb = simd::load_unaligned(&b);
    simd::vec128_t sr = simd::add_i8(sa, sb);
    simd::vec128_t matched = simd::match_bytes(sr, simd::load_unaligned(&ref));
    EXPECT_EQ(simd::to_bitmask(matched), 0xFFFF);
}

TEST_F(SimdUtilTest, Sse2MinU8MatchesIntrinsic) {
    ::__m128i a = ::_mm_set1_epi8(50);
    ::__m128i b = ::_mm_set1_epi8(100);
    ::__m128i ref = ::_mm_min_epu8(a, b);
    simd::vec128_t sa = simd::load_unaligned(&a);
    simd::vec128_t sb = simd::load_unaligned(&b);
    simd::vec128_t sr = simd::min_u8(sa, sb);
    simd::vec128_t matched = simd::match_bytes(sr, simd::load_unaligned(&ref));
    EXPECT_EQ(simd::to_bitmask(matched), 0xFFFF);
}

TEST_F(SimdUtilTest, Sse2AvgU8MatchesIntrinsic) {
    ::__m128i a = ::_mm_set1_epi8(10);
    ::__m128i b = ::_mm_set1_epi8(20);
    ::__m128i ref = ::_mm_avg_epu8(a, b);
    simd::vec128_t sa = simd::load_unaligned(&a);
    simd::vec128_t sb = simd::load_unaligned(&b);
    simd::vec128_t sr = simd::avg_u8(sa, sb);
    simd::vec128_t matched = simd::match_bytes(sr, simd::load_unaligned(&ref));
    EXPECT_EQ(simd::to_bitmask(matched), 0xFFFF);
}

TEST_F(SimdUtilTest, Sse2CmpeqI32MatchesIntrinsic) {
    ::__m128i a = ::_mm_setr_epi32(1, 2, 3, 4);
    ::__m128i b = ::_mm_setr_epi32(1, 99, 3, 99);
    ::__m128i ref = ::_mm_cmpeq_epi32(a, b);
    simd::vec128_t sa = simd::load_unaligned(&a);
    simd::vec128_t sb = simd::load_unaligned(&b);
    simd::vec128_t sr = simd::cmpeq_i32(sa, sb);
    simd::vec128_t matched = simd::match_bytes(sr, simd::load_unaligned(&ref));
    EXPECT_EQ(simd::to_bitmask(matched), 0xFFFF);
}

TEST_F(SimdUtilTest, Sse2BitAndNotMatchesIntrinsic) {
    ::__m128i a = ::_mm_set1_epi8(0xFF);
    ::__m128i b = ::_mm_set1_epi8(0x0F);
    ::__m128i ref = ::_mm_andnot_si128(b, a);
    simd::vec128_t sa = simd::load_unaligned(&a);
    simd::vec128_t sb = simd::load_unaligned(&b);
    simd::vec128_t sr = simd::bit_andnot(sa, sb);
    simd::vec128_t matched = simd::match_bytes(sr, simd::load_unaligned(&ref));
    EXPECT_EQ(simd::to_bitmask(matched), 0xFFFF);
}

#endif

#ifdef NEFORCE_SIMD_NEON

TEST_F(SimdUtilTest, NeonFillByteMatchesIntrinsic) {
    simd::vec128_t v = simd::fill_i8(0x5A);
    uint8x16_t ref = vdupq_n_u8(0x5A);
    simd::vec128_t matched = simd::match_bytes(v, ref);
    EXPECT_EQ(simd::to_bitmask(matched), 0xFFFF);
}

TEST_F(SimdUtilTest, NeonAddI8MatchesIntrinsic) {
    int8x16_t a = vdupq_n_s8(10);
    int8x16_t b = vdupq_n_s8(20);
    int8x16_t ref = vaddq_s8(a, b);
    simd::vec128_t sa = vreinterpretq_u8_s8(a);
    simd::vec128_t sb = vreinterpretq_u8_s8(b);
    simd::vec128_t sr = simd::add_i8(sa, sb);
    simd::vec128_t matched = simd::match_bytes(sr, vreinterpretq_u8_s8(ref));
    EXPECT_EQ(simd::to_bitmask(matched), 0xFFFF);
}

#endif
