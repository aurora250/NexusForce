#include <NeForce/tui/image.hpp>
#include <gtest/gtest.h>
#ifdef NEFORCE_SUPPORT_ZLIB
#    include <zlib.h>
#endif

using namespace neforce;
using namespace neforce::tui;

namespace {
    size_t count_occurrences(const string& text, const string_view needle) {
        size_t count = 0;
        size_t position = 0;
        while ((position = text.find(needle, position)) != string::npos) {
            ++count;
            position += needle.size();
        }
        return count;
    }

    string_view view_of(const vector<byte_t>& bytes) {
        return string_view{reinterpret_cast<const char*>(bytes.data()), bytes.size()};
    }

    int base64_value(const char character) noexcept {
        if (character >= 'A' && character <= 'Z') {
            return character - 'A';
        }
        if (character >= 'a' && character <= 'z') {
            return character - 'a' + 26;
        }
        if (character >= '0' && character <= '9') {
            return character - '0' + 52;
        }
        if (character == '+') {
            return 62;
        }
        if (character == '/') {
            return 63;
        }
        return -1;
    }

    vector<byte_t> decode_base64(const string_view text) {
        vector<byte_t> result;
        result.reserve(text.size() / 4 * 3);
        uint32_t buffer = 0;
        int bits = 0;
        for (size_t index = 0; index < text.size(); ++index) {
            const char character = text[index];
            if (character == '=') {
                break;
            }
            const int value = base64_value(character);
            if (value < 0) {
                continue;
            }
            buffer = (buffer << 6) | static_cast<uint32_t>(value);
            bits += 6;
            if (bits >= 8) {
                bits -= 8;
                result.push_back(static_cast<byte_t>((buffer >> bits) & 0xFF));
                buffer = bits == 0 ? 0 : buffer & ((1U << bits) - 1);
            }
        }
        return result;
    }

    image_source make_flat_image(const int width, const int height, const byte_t red, const byte_t green,
                                 const byte_t blue, const byte_t alpha) {
        image_source image;
        image.width = width;
        image.height = height;
        image.rgba.resize(static_cast<size_t>(width) * static_cast<size_t>(height) * 4);
        for (size_t index = 0; index < image.rgba.size(); index += 4) {
            image.rgba[index] = red;
            image.rgba[index + 1] = green;
            image.rgba[index + 2] = blue;
            image.rgba[index + 3] = alpha;
        }
        return image;
    }

#ifdef NEFORCE_SUPPORT_ZLIB
    void append_be32(vector<byte_t>& out, const uint32_t value) {
        out.push_back(static_cast<byte_t>((value >> 24) & 0xFF));
        out.push_back(static_cast<byte_t>((value >> 16) & 0xFF));
        out.push_back(static_cast<byte_t>((value >> 8) & 0xFF));
        out.push_back(static_cast<byte_t>(value & 0xFF));
    }

    void append_chunk(vector<byte_t>& png, const char* type, const byte_t* data, const size_t size) {
        append_be32(png, static_cast<uint32_t>(size));
        for (int index = 0; index < 4; ++index) {
            png.push_back(static_cast<byte_t>(type[index]));
        }
        if (size != 0) {
            png.insert(png.end(), data, data + size);
        }
        uLong crc = crc32(0L, Z_NULL, 0);
        crc = crc32(crc, reinterpret_cast<const Bytef*>(type), 4);
        if (size != 0) {
            crc = crc32(crc, data, static_cast<uInt>(size));
        }
        append_be32(png, static_cast<uint32_t>(crc));
    }

    vector<byte_t> compress_bytes(const vector<byte_t>& input) {
        uLongf bound = compressBound(static_cast<uLong>(input.size()));
        vector<byte_t> output(bound);
        const int status =
                compress2(output.data(), &bound, input.data(), static_cast<uLong>(input.size()), Z_BEST_COMPRESSION);
        EXPECT_EQ(status, Z_OK);
        output.resize(static_cast<size_t>(bound));
        return output;
    }

    vector<byte_t> make_png_from_idat(const uint32_t width, const uint32_t height, const byte_t color_type,
                                      const vector<vector<byte_t>>& parts, const byte_t bit_depth = 8,
                                      const byte_t interlace = 0) {
        vector<byte_t> png = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
        vector<byte_t> header;
        append_be32(header, width);
        append_be32(header, height);
        header.push_back(bit_depth);
        header.push_back(color_type);
        header.push_back(0);
        header.push_back(0);
        header.push_back(interlace);
        append_chunk(png, "IHDR", header.data(), header.size());
        for (const auto& part: parts) {
            append_chunk(png, "IDAT", part.data(), part.size());
        }
        append_chunk(png, "IEND", nullptr, 0);
        return png;
    }

    vector<byte_t> make_png(const uint32_t width, const uint32_t height, const byte_t color_type,
                            const vector<byte_t>& filtered, const bool split_idat = false, const byte_t bit_depth = 8,
                            const byte_t interlace = 0) {
        const vector<byte_t> compressed = compress_bytes(filtered);
        vector<vector<byte_t>> parts;
        if (split_idat && compressed.size() > 1) {
            const size_t middle = compressed.size() / 2;
            parts.push_back(vector<byte_t>(compressed.begin(), compressed.begin() + middle));
            parts.push_back(vector<byte_t>(compressed.begin() + middle, compressed.end()));
        } else {
            parts.push_back(compressed);
        }
        return make_png_from_idat(width, height, color_type, parts, bit_depth, interlace);
    }

    optional<image_source> decode_png_bytes(const vector<byte_t>& bytes) { return decode_png(view_of(bytes)); }
#endif
} // namespace

TEST(TuiImageSource, ValidatesBufferLength) {
    image_source image;
    EXPECT_FALSE(image.valid());
    image.width = 1;
    image.height = 1;
    image.rgba = {0, 0, 0, 255};
    EXPECT_TRUE(image.valid());
    image.rgba.push_back(0);
    EXPECT_FALSE(image.valid());
    image.rgba = {0, 0, 0, 255};
    image.height = 2;
    EXPECT_FALSE(image.valid());
    image.width = -1;
    EXPECT_FALSE(image.valid());
}

#ifdef NEFORCE_SUPPORT_ZLIB
TEST(TuiImagePng, DecodesRgba) {
    const vector<byte_t> filtered = {0, 255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 0, 255, 255, 255, 255, 0, 128};
    const auto decoded = decode_png_bytes(make_png(2, 2, 6, filtered));
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(decoded->width, 2);
    EXPECT_EQ(decoded->height, 2);
    EXPECT_EQ(decoded->rgba.size(), 16u);
    EXPECT_TRUE(decoded->valid());
    EXPECT_EQ(static_cast<int>(decoded->rgba[0]), 255);
    EXPECT_EQ(static_cast<int>(decoded->rgba[1]), 0);
    EXPECT_EQ(static_cast<int>(decoded->rgba[2]), 0);
    EXPECT_EQ(static_cast<int>(decoded->rgba[3]), 255);
    EXPECT_EQ(static_cast<int>(decoded->rgba[4]), 0);
    EXPECT_EQ(static_cast<int>(decoded->rgba[5]), 255);
    EXPECT_EQ(static_cast<int>(decoded->rgba[15]), 128);
}

TEST(TuiImagePng, ExpandsRgbToRgba) {
    const vector<byte_t> filtered = {0, 255, 0, 0, 0, 255, 0};
    const auto decoded = decode_png_bytes(make_png(2, 1, 2, filtered));
    ASSERT_TRUE(decoded.has_value());
    ASSERT_EQ(decoded->rgba.size(), 8u);
    EXPECT_EQ(static_cast<int>(decoded->rgba[0]), 255);
    EXPECT_EQ(static_cast<int>(decoded->rgba[1]), 0);
    EXPECT_EQ(static_cast<int>(decoded->rgba[2]), 0);
    EXPECT_EQ(static_cast<int>(decoded->rgba[3]), 255);
    EXPECT_EQ(static_cast<int>(decoded->rgba[4]), 0);
    EXPECT_EQ(static_cast<int>(decoded->rgba[5]), 255);
    EXPECT_EQ(static_cast<int>(decoded->rgba[6]), 0);
    EXPECT_EQ(static_cast<int>(decoded->rgba[7]), 255);
}

TEST(TuiImagePng, DecodesAllFilterTypes) {
    const vector<byte_t> filtered = {0,  10, 20, 30, 255, 1,  40,  50, 60, 255, 2,  30, 30,
                                     30, 0,  3,  65, 70,  75, 128, 4,  30, 30,  30, 0};
    const auto decoded = decode_png_bytes(make_png(1, 5, 6, filtered));
    ASSERT_TRUE(decoded.has_value());
    ASSERT_EQ(decoded->rgba.size(), 20u);
    for (int row = 0; row < 5; ++row) {
        EXPECT_EQ(static_cast<int>(decoded->rgba[static_cast<size_t>(row) * 4]), 10 + row * 30);
        EXPECT_EQ(static_cast<int>(decoded->rgba[static_cast<size_t>(row) * 4 + 1]), 20 + row * 30);
        EXPECT_EQ(static_cast<int>(decoded->rgba[static_cast<size_t>(row) * 4 + 2]), 30 + row * 30);
        EXPECT_EQ(static_cast<int>(decoded->rgba[static_cast<size_t>(row) * 4 + 3]), 255);
    }
}

TEST(TuiImagePng, DecodesSubAndPaethFilters) {
    const vector<byte_t> filtered = {1, 255, 0, 0, 255, 1, 255, 0, 0, 4, 1, 0, 255, 0, 255, 0, 1, 129};
    const auto decoded = decode_png_bytes(make_png(2, 2, 6, filtered));
    ASSERT_TRUE(decoded.has_value());
    ASSERT_EQ(decoded->rgba.size(), 16u);
    EXPECT_EQ(static_cast<int>(decoded->rgba[0]), 255);
    EXPECT_EQ(static_cast<int>(decoded->rgba[4]), 0);
    EXPECT_EQ(static_cast<int>(decoded->rgba[5]), 255);
    EXPECT_EQ(static_cast<int>(decoded->rgba[8]), 0);
    EXPECT_EQ(static_cast<int>(decoded->rgba[10]), 255);
    EXPECT_EQ(static_cast<int>(decoded->rgba[12]), 255);
    EXPECT_EQ(static_cast<int>(decoded->rgba[13]), 255);
    EXPECT_EQ(static_cast<int>(decoded->rgba[14]), 0);
    EXPECT_EQ(static_cast<int>(decoded->rgba[15]), 128);
}

TEST(TuiImagePng, ConcatenatesIdatChunks) {
    const vector<byte_t> filtered = {0, 1, 2, 3, 255};
    const auto decoded = decode_png_bytes(make_png(1, 1, 6, filtered, true));
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(static_cast<int>(decoded->rgba[0]), 1);
    EXPECT_EQ(static_cast<int>(decoded->rgba[1]), 2);
    EXPECT_EQ(static_cast<int>(decoded->rgba[2]), 3);
    EXPECT_EQ(static_cast<int>(decoded->rgba[3]), 255);
}

TEST(TuiImagePng, RejectsBadSignature) {
    auto png = make_png(1, 1, 6, {0, 1, 2, 3, 255});
    png[0] = 0;
    EXPECT_FALSE(decode_png_bytes(png).has_value());
}

TEST(TuiImagePng, RejectsTruncatedData) {
    auto png = make_png(1, 1, 6, {0, 1, 2, 3, 255});
    png.resize(12);
    EXPECT_FALSE(decode_png_bytes(png).has_value());
    EXPECT_FALSE(decode_png(string_view{}).has_value());
}

TEST(TuiImagePng, RejectsGrayscaleAndPalette) {
    EXPECT_FALSE(decode_png_bytes(make_png(1, 1, 0, {0, 0})).has_value());
    EXPECT_FALSE(decode_png_bytes(make_png(1, 1, 3, {0, 0})).has_value());
}

TEST(TuiImagePng, RejectsNonEightBitDepth) {
    EXPECT_FALSE(decode_png_bytes(make_png(1, 1, 6, {0, 0, 0, 0, 0}, false, 16)).has_value());
}

TEST(TuiImagePng, RejectsInterlacedData) {
    EXPECT_FALSE(decode_png_bytes(make_png(1, 1, 6, {0, 0, 0, 0, 0}, false, 8, 1)).has_value());
}

TEST(TuiImagePng, RejectsMissingIend) {
    auto png = make_png(1, 1, 6, {0, 1, 2, 3, 255});
    ASSERT_GT(png.size(), 12u);
    png.resize(png.size() - 12);
    EXPECT_FALSE(decode_png_bytes(png).has_value());
}

TEST(TuiImagePng, RejectsInvalidFilter) {
    EXPECT_FALSE(decode_png_bytes(make_png(1, 1, 6, {5, 1, 2, 3, 4})).has_value());
}

TEST(TuiImagePng, RejectsCorruptCompressedData) {
    EXPECT_FALSE(decode_png_bytes(make_png_from_idat(1, 1, 6, {{0, 1, 2, 3}})).has_value());
}
#endif

TEST(TuiImageKitty, EncodesSingleChunk) {
    const image_source image = make_flat_image(1, 1, 1, 2, 3, 255);
    const auto output = encode_kitty(image, {1, 1, 0, 0}, 42, true);
    ASSERT_FALSE(output.empty());
    EXPECT_EQ(output.find("\x1b_Gf=32,s=1,v=1,a=T,i=42,p=1,m=0,q=2;"), 0u);
    EXPECT_EQ(count_occurrences(output, "\x1b_G"), 1u);
    EXPECT_EQ(count_occurrences(output, "q=2"), 1u);
    EXPECT_EQ(output, "\x1b_Gf=32,s=1,v=1,a=T,i=42,p=1,m=0,q=2;AQID/w==\x1b\\");
}

TEST(TuiImageKitty, ChunksPayloadAtMost4096Bytes) {
    const image_source image = make_flat_image(64, 64, 12, 34, 56, 255);
    const auto output = encode_kitty(image, {8, 4, 0, 0}, 7, true);
    ASSERT_FALSE(output.empty());
    EXPECT_EQ(output.find("\x1b_Gf=32,s=64,v=64,a=T,i=7,p=1,m=1,q=2;"), 0u);

    string payload;
    size_t position = 0;
    size_t chunks = 0;
    while (true) {
        const size_t start = output.find("\x1b_G", position);
        if (start == string::npos) {
            break;
        }
        const size_t separator = output.find(';', start);
        ASSERT_NE(separator, string::npos);
        const size_t end = output.find("\x1b\\", separator + 1);
        ASSERT_NE(end, string::npos);
        const size_t length = end - separator - 1;
        EXPECT_LE(length, 4096u);
        payload.append(output.data() + separator + 1, length);
        ++chunks;
        position = end + 2;
    }
    EXPECT_GT(chunks, 1u);
    EXPECT_EQ(count_occurrences(output, "q=2"), 1u);
    EXPECT_EQ(count_occurrences(output, "m=0"), 1u);
    EXPECT_EQ(count_occurrences(output, "m=1"), chunks - 1);
    EXPECT_EQ(decode_base64(string_view{payload.data(), payload.size()}), image.rgba);
}

TEST(TuiImageKitty, PlacementReusesExistingImage) {
    const image_source image = make_flat_image(2, 2, 255, 255, 255, 255);
    const auto output = encode_kitty(image, {6, 3, 0, 0}, 9, false);
    EXPECT_EQ(output, "\x1b_Ga=p,i=9,p=1,c=6,r=3\x1b\\");
    EXPECT_EQ(output.find("f=32"), string::npos);
    EXPECT_EQ(output.find(';'), string::npos);
}

TEST(TuiImageKitty, RejectsInvalidImage) {
    const image_source image;
    EXPECT_TRUE(encode_kitty(image, {1, 1, 0, 0}, 1, true).empty());
}

TEST(TuiImageIterm2, FramesInlineImage) {
    const image_source image = make_flat_image(2, 2, 10, 20, 30, 40);
    const auto output = encode_iterm2(image, {10, 5, 0, 0});
    ASSERT_FALSE(output.empty());
    EXPECT_EQ(output.find("\x1b]1337;File=inline=1;width=10;height=5;preserveAspectRatio=1:"), 0u);
    EXPECT_EQ(output.back(), '\a');
    const size_t payload_start = output.find(':') + 1;
    const string_view encoded(output.data() + payload_start, output.size() - 1 - payload_start);
    EXPECT_EQ(decode_base64(encoded), image.rgba);
}

TEST(TuiImageIterm2, ClampsNegativeCellBox) {
    const image_source image = make_flat_image(1, 1, 1, 2, 3, 255);
    const auto output = encode_iterm2(image, {-1, -2, 0, 0});
    EXPECT_NE(output.find("width=0;height=0;"), string::npos);
    EXPECT_EQ(output.back(), '\a');
}

TEST(TuiImageIterm2, RejectsInvalidImage) { EXPECT_TRUE(encode_iterm2(image_source{}, {1, 1, 0, 0}).empty()); }

TEST(TuiImageSixel, EncodesHeaderRasterAndRle) {
    const image_source image = make_flat_image(4, 3, 255, 0, 0, 255);
    const auto output = encode_sixel(image, {4, 3, 0, 0});
    ASSERT_FALSE(output.empty());
    EXPECT_EQ(output.find("\x1bPq\"1;1;4;3"), 0u);
    EXPECT_NE(output.find("#180"), string::npos);
    EXPECT_NE(output.find("!4F"), string::npos);
    EXPECT_NE(output.find('$'), string::npos);
    EXPECT_EQ(count_occurrences(output, "-"), 0u);
    ASSERT_GE(output.size(), 2u);
    EXPECT_EQ(output[output.size() - 1], '\\');
    EXPECT_EQ(output[output.size() - 2], '\x1b');
}

TEST(TuiImageSixel, SeparatesBandsWithDash) {
    const image_source image = make_flat_image(4, 8, 0, 255, 0, 255);
    const auto output = encode_sixel(image, {4, 8, 0, 0});
    EXPECT_EQ(count_occurrences(output, "-"), 1u);
    EXPECT_EQ(count_occurrences(output, "$"), 2u);
}

TEST(TuiImageSixel, LeavesTransparentPixelsBlank) {
    const image_source image = make_flat_image(2, 2, 255, 255, 255, 0);
    EXPECT_EQ(encode_sixel(image, {2, 2, 0, 0}), "\x1bPq\"1;1;2;2\x1b\\");
}

TEST(TuiImageSixel, RejectsInvalidImage) { EXPECT_TRUE(encode_sixel(image_source{}, {1, 1, 0, 0}).empty()); }

TEST(TuiImageDetect, DetectsKitty) {
    EXPECT_EQ(detect_image_protocol("xterm-kitty", "", "", ""), image_protocol::kitty);
    EXPECT_EQ(detect_image_protocol("xterm-256color", "", "1", ""), image_protocol::kitty);
}

TEST(TuiImageDetect, DetectsIterm2) {
    EXPECT_EQ(detect_image_protocol("xterm-256color", "iTerm.app", "", ""), image_protocol::iterm2);
    EXPECT_EQ(detect_image_protocol("xterm-256color", "WezTerm", "", ""), image_protocol::iterm2);
}

TEST(TuiImageDetect, DetectsSixel) {
    EXPECT_EQ(detect_image_protocol("xterm-256color", "", "", "1"), image_protocol::sixel);
    EXPECT_EQ(detect_image_protocol("xterm-sixel", "", "", ""), image_protocol::sixel);
}

TEST(TuiImageDetect, ReturnsNoneWithoutSupport) {
    EXPECT_EQ(detect_image_protocol("", "", "", ""), image_protocol::none);
    EXPECT_EQ(detect_image_protocol("xterm-256color", "Apple_Terminal", "", ""), image_protocol::none);
}

TEST(TuiImageDetect, HonorsProtocolPriority) {
    EXPECT_EQ(detect_image_protocol("xterm-kitty", "iTerm.app", "1", "1"), image_protocol::kitty);
    EXPECT_EQ(detect_image_protocol("xterm-256color", "WezTerm", "", "1"), image_protocol::iterm2);
}
