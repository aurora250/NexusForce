#include <NeForce/core/simd/simd_util.hpp>
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

TEST_F(SimdUtilTest, FillByteAllEqual) {
    simd::vec128_t v = simd::fill_byte(0x42);
    const byte_t* bytes = reinterpret_cast<const byte_t*>(&v);
    for (int i = 0; i < 16; ++i) {
        EXPECT_EQ(bytes[i], 0x42);
    }
}

TEST_F(SimdUtilTest, FillByteZero) {
    simd::vec128_t v = simd::fill_byte(0);
    const byte_t* bytes = reinterpret_cast<const byte_t*>(&v);
    for (int i = 0; i < 16; ++i) {
        EXPECT_EQ(bytes[i], 0);
    }
}

TEST_F(SimdUtilTest, FillByteMax) {
    simd::vec128_t v = simd::fill_byte(0xFF);
    const byte_t* bytes = reinterpret_cast<const byte_t*>(&v);
    for (int i = 0; i < 16; ++i) {
        EXPECT_EQ(bytes[i], 0xFF);
    }
}

TEST_F(SimdUtilTest, FillByteHighBitSet) {
    simd::vec128_t v = simd::fill_byte(0x80);
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
    simd::vec128_t a = simd::fill_byte(0x55);
    simd::vec128_t b = simd::fill_byte(0x55);
    simd::vec128_t result = simd::match_bytes(a, b);
    const byte_t* bytes = reinterpret_cast<const byte_t*>(&result);
    for (int i = 0; i < 16; ++i) {
        EXPECT_EQ(bytes[i], 0xFF);
    }
}

TEST_F(SimdUtilTest, MatchBytesDifferentVectors) {
    simd::vec128_t a = simd::fill_byte(0x55);
    simd::vec128_t b = simd::fill_byte(0xAA);
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
    simd::vec128_t v = simd::fill_byte(0x00);
    int mask = simd::to_bitmask(v);
    EXPECT_EQ(mask, 0);
}

TEST_F(SimdUtilTest, ToBitmaskAllHighBits) {
    simd::vec128_t v = simd::fill_byte(0x80);
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
    simd::vec128_t a = simd::fill_byte(0x7F);
    simd::vec128_t b = simd::fill_byte(0x7F);
    simd::vec128_t matched = simd::match_bytes(a, b);
    int mask = simd::to_bitmask(matched);
    EXPECT_EQ(mask, 0xFFFF);
}

TEST_F(SimdUtilTest, FillThenMatchDifferentThenToBitmask) {
    simd::vec128_t a = simd::fill_byte(0x42);
    simd::vec128_t b = simd::fill_byte(0x43);
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

#ifdef NEFORCE_SIMD_SSE2

TEST_F(SimdUtilTest, Sse2Vec128MatchesM128i) { EXPECT_EQ(sizeof(simd::vec128_t), sizeof(::__m128i)); }

TEST_F(SimdUtilTest, Sse2FillByteMatchesIntrinsic) {
    simd::vec128_t v = simd::fill_byte(0x5A);
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

#endif
