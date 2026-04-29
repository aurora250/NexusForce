#include <gtest/gtest.h>
#ifdef NEFORCE_SUPPORT_LZ4
#    include <NeForce/compress/lz4_compress.hpp>
#    include <NeForce/core/algorithm/type_erase.hpp>
using namespace neforce;

namespace {
    constexpr uint8_t sample_data[] = "Hello, LZ4! This is a test string for compression and decompression.";
    constexpr size_t sample_size = sizeof(sample_data) - 1;

    byte_vector make_sample_vector() { return {begin(sample_data), end(sample_data) - 1}; }

    string make_sample_string() { return {reinterpret_cast<const char*>(sample_data), sample_size}; }
} // namespace


TEST(Lz4Compressor, CompressDecompressIterators) {
    auto original = make_sample_vector();
    auto compressed = lz4_compressor::compress(original.begin(), original.end(), 0);
    ASSERT_FALSE(compressed.empty());
    auto decompressed = lz4_compressor::decompress(compressed.begin(), compressed.end());
    EXPECT_EQ(decompressed, original);
}

TEST(Lz4Compressor, CompressDecompressStringView) {
    auto original = make_sample_string();
    auto compressed = lz4_compressor::compress(original.view());
    auto decompressed = lz4_compressor::decompress(cbyte_view(compressed.data(), compressed.size()));
    EXPECT_EQ(string_view(reinterpret_cast<const char*>(decompressed.data()), decompressed.size()), original);
}

TEST(Lz4Compressor, CompressDecompressByteVector) {
    auto original = make_sample_vector();
    auto compressed = lz4_compressor::compress(original);
    auto decompressed = lz4_compressor::decompress(cbyte_view(compressed.data(), compressed.size()));
    EXPECT_EQ(decompressed, original);
}

TEST(Lz4Compressor, CompressionLevels) {
    auto original = make_sample_vector();
    for (int level: {0, 1, 6, 12, 13, 20, 99}) {
        auto compressed = lz4_compressor::compress(original.begin(), original.end(), level);
        ASSERT_FALSE(compressed.empty()) << "level=" << level;
        auto decompressed = lz4_compressor::decompress(compressed.begin(), compressed.end());
        EXPECT_EQ(decompressed, original) << "level=" << level;
    }
}

TEST(Lz4Compressor, DecompressWithEstimatedSize) {
    auto original = make_sample_vector();
    auto compressed = lz4_compressor::compress(original.begin(), original.end());
    auto decompressed = lz4_compressor::decompress(compressed.begin(), compressed.end(), original.size());
    EXPECT_EQ(decompressed, original);
}

TEST(Lz4Compressor, DISABLED_DecompressWithUnderestimatedSize) {
    auto original = make_sample_vector();
    auto compressed = lz4_compressor::compress(original);
    size_t small_est = 4;
    auto decompressed = lz4_compressor::decompress(compressed.begin(), compressed.end(), small_est);
    EXPECT_EQ(decompressed, original);
}

TEST(Lz4Compressor, DecompressZeroEstimateUsesDefault) {
    auto original = make_sample_vector();
    auto compressed = lz4_compressor::compress(original.begin(), original.end());
    auto decompressed = lz4_compressor::decompress(compressed.begin(), compressed.end(), 0);
    EXPECT_EQ(decompressed, original);
}

TEST(Lz4Compressor, DecompressCorruptedDataThrows) {
    byte_vector corrupted = {0x00, 0x01, 0x02, 0x03};
    EXPECT_THROW((void) lz4_compressor::decompress(corrupted.begin(), corrupted.end()), lz4_exception);
}

class StreamCompressorTest : public ::testing::Test {
protected:
    byte_vector original_;
    void SetUp() override { original_ = make_sample_vector(); }
};

TEST_F(StreamCompressorTest, CompressDecompressRoundTrip) {
    lz4_compressor::stream_compressor sc(0);
    auto part1 = sc.compress(cbyte_view(original_.data(), 10));
    auto part2 = sc.compress(cbyte_view(original_.data() + 10, original_.size() - 10));
    auto finish = sc.finish();
    EXPECT_TRUE(finish.empty());

    lz4_compressor::stream_decompressor sd;
    auto dec1 = sd.decompress(part1.view());
    auto dec2 = sd.decompress(part2.view());
    auto dec_fin = sd.finish();
    EXPECT_TRUE(dec_fin.empty());

    byte_vector result;
    result.insert(result.end(), dec1.begin(), dec1.end());
    result.insert(result.end(), dec2.begin(), dec2.end());
    EXPECT_EQ(result, original_);
}

TEST_F(StreamCompressorTest, SingleBlockAndFinish) {
    lz4_compressor::stream_compressor sc;
    auto compressed = sc.compress(cbyte_view(original_.data(), original_.size()), true);
    lz4_compressor::stream_decompressor sd;
    auto decompressed = sd.decompress(compressed.view(), true);
    EXPECT_EQ(decompressed, original_);
}

TEST_F(StreamCompressorTest, DISABLED_MultipleBlocks) {
    lz4_compressor::stream_compressor sc;
    byte_vector acc_compressed;
    for (size_t i = 0; i < original_.size(); i += 16) {
        size_t len = min(static_cast<size_t>(16), original_.size() - i);
        auto block = sc.compress(cbyte_view(original_.data() + i, len), (i + len) >= original_.size());
        acc_compressed.insert(acc_compressed.end(), block.begin(), block.end());
    }
    auto final = sc.finish();
    acc_compressed.insert(acc_compressed.end(), final.begin(), final.end());

    lz4_compressor::stream_decompressor sd;
    byte_vector acc_decompressed;
    size_t offset = 0;
    while (offset < acc_compressed.size()) {
        size_t chunk = min(static_cast<size_t>(32), acc_compressed.size() - offset);
        bool finish = (offset + chunk) >= acc_compressed.size();
        auto dec = sd.decompress(acc_compressed.view(offset, chunk), finish);
        acc_decompressed.insert(acc_decompressed.end(), dec.begin(), dec.end());
        offset += chunk;
    }
    auto tail = sd.finish();
    acc_decompressed.insert(acc_decompressed.end(), tail.begin(), tail.end());
    EXPECT_EQ(acc_decompressed, original_);
}

TEST_F(StreamCompressorTest, InputBlockExceedsLimitThrows) {
    lz4_compressor::stream_compressor sc;
    byte_vector large(lz4_compressor::block_size + 1, 0x41);
    EXPECT_THROW(sc.compress(cbyte_view(large.data(), large.size())), lz4_exception);
}

TEST_F(StreamCompressorTest, CompressAfterFinishThrows) {
    lz4_compressor::stream_compressor sc;
    sc.compress(cbyte_view(original_.data(), 1), true);
    EXPECT_THROW(sc.compress(cbyte_view(original_.data(), 1)), lz4_exception);
}

TEST_F(StreamCompressorTest, UninitializedCompressorThrows) {
    lz4_compressor::stream_compressor sc;
    lz4_compressor::stream_compressor moved(move(sc));
    EXPECT_THROW(sc.compress(cbyte_view(original_.data(), 1)), lz4_exception);
}

TEST_F(StreamCompressorTest, MoveConstructorTransfersState) {
    lz4_compressor::stream_compressor sc1(5);
    auto block = sc1.compress(cbyte_view(original_.data(), 5));
    EXPECT_FALSE(block.empty());
    auto input_before = sc1.bytes_input();

    lz4_compressor::stream_compressor sc2(move(sc1));
    EXPECT_EQ(sc2.bytes_input(), input_before);
    auto block2 = sc2.compress(cbyte_view(original_.data(), 5));
    EXPECT_FALSE(block2.empty());
}

TEST_F(StreamCompressorTest, MoveAssignment) {
    lz4_compressor::stream_compressor sc1(3);
    sc1.compress(cbyte_view(original_.data(), 5));
    lz4_compressor::stream_compressor sc2(0);
    sc2 = move(sc1);
    auto block = sc2.compress(cbyte_view(original_.data() + 5, 5));
    EXPECT_FALSE(block.empty());
}

TEST_F(StreamCompressorTest, SelfMoveAssignment) {
    lz4_compressor::stream_compressor sc(0);
    sc.compress(cbyte_view(original_.data(), 5));
    sc = move(sc);
    auto block = sc.compress(cbyte_view(original_.data() + 5, 5));
    EXPECT_FALSE(block.empty());
}

TEST_F(StreamCompressorTest, ResetAfterUse) {
    lz4_compressor::stream_compressor sc(4);
    sc.compress(cbyte_view(original_.data(), 10));
    sc.reset(1);
    auto block = sc.compress(cbyte_view(original_.data(), 20), true);
    lz4_compressor::stream_decompressor sd;
    auto dec = sd.decompress(block.view(), true);
    EXPECT_EQ(dec, byte_vector(original_.begin(), original_.begin() + 20));
}

TEST_F(StreamCompressorTest, Statistics) {
    lz4_compressor::stream_compressor sc;
    auto block = sc.compress(cbyte_view(original_.data(), original_.size()), true);
    EXPECT_EQ(sc.bytes_input(), original_.size());
    EXPECT_EQ(sc.bytes_output(), block.size());
    if (original_.size() > 0) {
        EXPECT_NEAR(sc.compression_ratio(), static_cast<double>(block.size()) / original_.size(), 1e-9);
    } else {
        EXPECT_DOUBLE_EQ(sc.compression_ratio(), 0.0);
    }
}

TEST_F(StreamCompressorTest, StringViewOverload) {
    string s = make_sample_string();
    lz4_compressor::stream_compressor sc;
    auto block = sc.compress(s.view(), true);
    ASSERT_FALSE(block.empty());
    lz4_compressor::stream_decompressor sd;
    auto dec = sd.decompress(block.view(), true);
    string result(reinterpret_cast<const char*>(dec.data()), dec.size());
    EXPECT_EQ(result, s);
}

TEST_F(StreamCompressorTest, HighCompressionModeDispatchesCorrectly) {
    lz4_compressor::stream_compressor hc_sc(9);
    auto hc_block = hc_sc.compress(cbyte_view(original_.data(), original_.size()), true);
    lz4_compressor::stream_compressor fast_sc(0);
    auto fast_block = fast_sc.compress(cbyte_view(original_.data(), original_.size()), true);
    byte_vector hc_dec, fast_dec;
    {
        lz4_compressor::stream_decompressor sd;
        hc_dec = sd.decompress(hc_block.view(), true);
    }
    {
        lz4_compressor::stream_decompressor sd;
        fast_dec = sd.decompress(fast_block.view(), true);
    }
    EXPECT_EQ(hc_dec, original_);
    EXPECT_EQ(fast_dec, original_);
    EXPECT_LE(hc_block.size(), fast_block.size());
}

class StreamDecompressorTest : public ::testing::Test {
protected:
    byte_vector original_;
    byte_vector compressed_;
    void SetUp() override {
        original_ = make_sample_vector();
        lz4_compressor::stream_compressor sc;
        compressed_ = sc.compress(cbyte_view(original_.data(), original_.size()), true);
    }
};

TEST_F(StreamDecompressorTest, SingleDecompressCall) {
    lz4_compressor::stream_decompressor sd;
    auto result = sd.decompress(compressed_.view(), true);
    EXPECT_EQ(result, original_);
}

TEST_F(StreamDecompressorTest, DecompressAfterFinishThrows) {
    lz4_compressor::stream_decompressor sd;
    sd.decompress(compressed_.view(), true);
    EXPECT_THROW(sd.decompress(compressed_.view(0, 1)), lz4_exception);
}

TEST_F(StreamDecompressorTest, UninitializedDecompressorThrows) {
    lz4_compressor::stream_decompressor sd;
    lz4_compressor::stream_decompressor moved(move(sd));
    EXPECT_THROW(sd.decompress(compressed_.view(0, 5)), lz4_exception);
}

TEST_F(StreamDecompressorTest, DISABLED_MoveConstructor) {
    lz4_compressor::stream_decompressor sd1;
    auto part = sd1.decompress(compressed_.view(0, 5));
    auto input1 = sd1.bytes_input();

    lz4_compressor::stream_decompressor sd2(move(sd1));
    EXPECT_EQ(sd2.bytes_input(), input1);
    auto rest = sd2.decompress(compressed_.view(5, compressed_.size() - 5), true);
    EXPECT_EQ(rest.size() + part.size(), original_.size());
}

TEST_F(StreamDecompressorTest, DISABLED_MoveAssignment) {
    lz4_compressor::stream_decompressor sd1;
    sd1.decompress(compressed_.view(0, 5));
    lz4_compressor::stream_decompressor sd2;
    sd2 = move(sd1);
    auto rest = sd2.decompress(compressed_.view(5, compressed_.size() - 5), true);
    EXPECT_FALSE(rest.empty());
}

TEST_F(StreamDecompressorTest, DISABLED_SelfMoveAssignment) {
    lz4_compressor::stream_decompressor sd;
    sd.decompress(compressed_.view(0, 5));
    sd = move(sd);
    auto rest = sd.decompress(compressed_.view(5, compressed_.size() - 5), true);
    EXPECT_FALSE(rest.empty());
}

TEST_F(StreamDecompressorTest, DISABLED_Reset) {
    lz4_compressor::stream_decompressor sd;
    sd.decompress(compressed_.view(0, compressed_.size() / 2));
    sd.reset();
    auto result = sd.decompress(compressed_.view(), true);
    EXPECT_EQ(result, original_);
}

TEST_F(StreamDecompressorTest, CorruptedDataThrows) {
    byte_vector fake = {0x00, 0x01};
    lz4_compressor::stream_decompressor sd;
    EXPECT_THROW(sd.decompress(fake.view()), lz4_exception);
}

TEST_F(StreamDecompressorTest, Statistics) {
    lz4_compressor::stream_decompressor sd;
    auto result = sd.decompress(compressed_.view(), true);
    EXPECT_EQ(sd.bytes_input(), compressed_.size());
    EXPECT_EQ(sd.bytes_output(), result.size());
    EXPECT_NEAR(sd.expansion_ratio(), static_cast<double>(result.size()) / compressed_.size(), 1e-9);
}

#endif
