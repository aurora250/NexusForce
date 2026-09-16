#include <NeForce/core/string/codepoint.hpp>
#include <NeForce/core/string/utf.hpp>
#include <NeForce/core/string/utf_iterator.hpp>
#include <gtest/gtest.h>
using namespace neforce;

namespace {
    template <typename S>
    bool same_content(const S& lhs, const S& rhs) {
        if (lhs.size() != rhs.size()) {
            return false;
        }
        for (size_t i = 0; i < lhs.size(); ++i) {
            if (lhs.data()[i] != rhs.data()[i]) {
                return false;
            }
        }
        return true;
    }

    const byte_t* as_bytes(const std::string& s) { return reinterpret_cast<const byte_t*>(s.data()); }

    string reference_utf8_to_string(const std::string& input) {
        string result;
        size_t i = 0;
        const size_t len = input.size();
        while (i < len) {
            codepoint::decode_utf8(as_bytes(input), i, len).append_to(result);
        }
        return result;
    }

    u16string reference_utf8_to_u16(const std::string& input) {
        u16string result;
        size_t i = 0;
        const size_t len = input.size();
        while (i < len) {
            codepoint::decode_utf8(as_bytes(input), i, len).append_to(result);
        }
        return result;
    }

    u32string reference_utf8_to_u32(const std::string& input) {
        u32string result;
        size_t i = 0;
        const size_t len = input.size();
        while (i < len) {
            codepoint::decode_utf8(as_bytes(input), i, len).append_to(result);
        }
        return result;
    }

    wstring reference_utf8_to_wstring(const std::string& input) {
        wstring result;
        size_t i = 0;
        const size_t len = input.size();
        while (i < len) {
            codepoint::decode_utf8(as_bytes(input), i, len).append_to(result);
        }
        return result;
    }

    string reference_utf16_to_string(const char16_t* data, const size_t len, const bool need_swap) {
        string result;
        size_t i = 0;
        while (i < len) {
            codepoint::decode_utf16(data, i, len, need_swap).append_to(result);
        }
        return result;
    }

    u32string reference_utf16_to_u32(const char16_t* data, const size_t len, const bool need_swap) {
        u32string result;
        size_t i = 0;
        while (i < len) {
            codepoint::decode_utf16(data, i, len, need_swap).append_to(result);
        }
        return result;
    }

    wstring reference_utf16_to_wstring(const char16_t* data, const size_t len, const bool need_swap) {
        wstring result;
        size_t i = 0;
        while (i < len) {
            codepoint::decode_utf16(data, i, len, need_swap).append_to(result);
        }
        return result;
    }

    string reference_utf32_to_string(const char32_t* data, const size_t len) {
        string result;
        for (size_t i = 0; i < len; ++i) {
            codepoint::from_utf32(data[i]).append_to(result);
        }
        return result;
    }

    u16string reference_utf32_to_u16(const char32_t* data, const size_t len) {
        u16string result;
        for (size_t i = 0; i < len; ++i) {
            codepoint::from_utf32(data[i]).append_to(result);
        }
        return result;
    }

    string reference_wchar_to_string(const wchar_t* data, const size_t len) {
        string result;
#ifdef NEFORCE_PLATFORM_WINDOWS
        size_t i = 0;
        while (i < len) {
            codepoint::decode_utf16(data, i, len, false).append_to(result);
        }
#else
        for (size_t i = 0; i < len; ++i) {
            codepoint(static_cast<uint32_t>(data[i])).append_to(result);
        }
#endif
        return result;
    }

    u32string reference_wchar_to_u32(const wchar_t* data, const size_t len) {
        u32string result;
#ifdef NEFORCE_PLATFORM_WINDOWS
        size_t i = 0;
        while (i < len) {
            codepoint::decode_utf16(data, i, len, false).append_to(result);
        }
#else
        for (size_t i = 0; i < len; ++i) {
            codepoint(static_cast<uint32_t>(data[i])).append_to(result);
        }
#endif
        return result;
    }

    u16string reference_wchar_to_u16(const wchar_t* data, const size_t len) {
        u16string result;
#ifdef NEFORCE_PLATFORM_WINDOWS
        for (size_t i = 0; i < len; ++i) {
            result.push_back(static_cast<char16_t>(static_cast<uint16_t>(data[i])));
        }
#else
        for (size_t i = 0; i < len; ++i) {
            codepoint(static_cast<uint32_t>(data[i])).append_to(result);
        }
#endif
        return result;
    }

    std::vector<std::string> utf8_corpus() {
        std::vector<std::string> corpus;
        corpus.emplace_back("");
        corpus.emplace_back("a");
        corpus.emplace_back("hello world");
        corpus.emplace_back(15, 'a');
        corpus.emplace_back(16, 'a');
        corpus.emplace_back(17, 'a');
        corpus.emplace_back(31, 'a');
        corpus.emplace_back(32, 'a');
        corpus.emplace_back(33, 'a');
        corpus.emplace_back(63, 'a');
        corpus.emplace_back(64, 'a');
        corpus.emplace_back(65, 'a');
        corpus.emplace_back("\xC3\xA9");
        for (int i = 0; i < 40; ++i) {
            corpus.back() += "\xC3\xA9";
        }
        corpus.emplace_back("\xE4\xB8\xAD\xE6\x96\x87");
        corpus.emplace_back("");
        for (int i = 0; i < 40; ++i) {
            corpus.back() += "\xE4\xB8\xAD\xE6\x96\x87";
        }
        corpus.emplace_back("\xF0\x9F\x98\x80");
        corpus.emplace_back("");
        for (int i = 0; i < 40; ++i) {
            corpus.back() += "\xF0\x9F\x98\x80";
        }
        corpus.emplace_back("a\xC3\xA9\xE4\xB8\xAD\xF0\x9F\x98\x80"
                            "b");
        corpus.emplace_back("");
        for (int i = 0; i < 33; ++i) {
            corpus.back() += "a\xC3\xA9\xE4\xB8\xAD\xF0\x9F\x98\x80";
        }

        corpus.emplace_back("\xC0\xAF");
        corpus.emplace_back("\xE0\x80\xAF");
        corpus.emplace_back("\xF0\x80\x80\xAF");
        corpus.emplace_back("\xED\xA0\x80");
        corpus.emplace_back("\xED\xBF\xBF");
        corpus.emplace_back("\xF4\x90\x80\x80");
        corpus.emplace_back("\xF5\x80\x80\x80");
        corpus.emplace_back("\x80");
        corpus.emplace_back("\xC3");
        corpus.emplace_back("\xE4\xB8");
        corpus.emplace_back("\xF0\x9F\x98");
        corpus.emplace_back("\xFF\xFE\xFD");
        corpus.emplace_back("\xC2");
        corpus.emplace_back("ok\xE4\xB8ok\xF0\x9Fok");
        corpus.emplace_back("\xE0\xA0\x80");
        corpus.emplace_back("\xE0\x9F\xBF");
        corpus.emplace_back("\xED\x9F\xBF");
        corpus.emplace_back("\xF0\x90\x80\x80");
        corpus.emplace_back("\xF4\x8F\xBF\xBF");

        corpus.emplace_back("");
        for (int i = 0; i < 300; ++i) {
            corpus.back() += "\xE4\xB8\xAD";
        }
        corpus.emplace_back("");
        for (int i = 0; i < 300; ++i) {
            corpus.back() += "\xF0\x9F\x98\x80";
        }
        corpus.emplace_back("");
        for (int i = 0; i < 200; ++i) {
            corpus.back() += "abc\xE4\xB8\xAD\xF0\x9F\x98\x80\xC3\xA9";
        }
        return corpus;
    }

    std::string boundary_input(const size_t offset, const std::string& unit) {
        std::string input(offset, 'a');
        input += unit;
        input += "tail\xE4\xB8\xAD";
        return input;
    }
} // namespace

TEST(Utf8BulkDecodeTest, MatchesScalarReferenceOnCorpus) {
    for (const auto& input: utf8_corpus()) {
        const auto expected_string = reference_utf8_to_string(input);
        const auto expected_u16 = reference_utf8_to_u16(input);
        const auto expected_u32 = reference_utf8_to_u32(input);
        const auto expected_w = reference_utf8_to_wstring(input);

        string actual_string;
        u16string actual_u16;
        u32string actual_u32;
        wstring actual_w;
        codepoint::decode_utf8(as_bytes(input), input.size(), actual_string);
        codepoint::decode_utf8(as_bytes(input), input.size(), actual_u16);
        codepoint::decode_utf8(as_bytes(input), input.size(), actual_u32);
        codepoint::decode_utf8(as_bytes(input), input.size(), actual_w);

        EXPECT_TRUE(same_content(expected_string, actual_string)) << "len=" << input.size();
        EXPECT_TRUE(same_content(expected_u16, actual_u16)) << "len=" << input.size();
        EXPECT_TRUE(same_content(expected_u32, actual_u32)) << "len=" << input.size();
        EXPECT_TRUE(same_content(expected_w, actual_w)) << "len=" << input.size();
    }
}

TEST(Utf8BulkDecodeTest, TwoByteBoundaryOffsets) {
    for (size_t offset = 0; offset <= 48; ++offset) {
        const auto input = boundary_input(offset, "\xC3\xA9");
        const auto expected = reference_utf8_to_u32(input);
        u32string actual;
        codepoint::decode_utf8(as_bytes(input), input.size(), actual);
        EXPECT_TRUE(same_content(expected, actual)) << "offset=" << offset;
    }
}

TEST(Utf8BulkDecodeTest, ThreeByteBoundaryOffsets) {
    for (size_t offset = 0; offset <= 48; ++offset) {
        const auto input = boundary_input(offset, "\xE4\xB8\xAD");
        const auto expected = reference_utf8_to_string(input);
        string actual;
        codepoint::decode_utf8(as_bytes(input), input.size(), actual);
        EXPECT_TRUE(same_content(expected, actual)) << "offset=" << offset;
    }
}

TEST(Utf8BulkDecodeTest, FourByteBoundaryOffsets) {
    for (size_t offset = 0; offset <= 48; ++offset) {
        const auto input = boundary_input(offset, "\xF0\x9F\x98\x80");
        const auto expected = reference_utf8_to_u16(input);
        u16string actual;
        codepoint::decode_utf8(as_bytes(input), input.size(), actual);
        EXPECT_TRUE(same_content(expected, actual)) << "offset=" << offset;
    }
}

TEST(Utf8BulkDecodeTest, LengthSweepAcrossSimdBlocks) {
    const char* const units[] = {"a", "\xC3\xA9", "\xE4\xB8\xAD", "\xF0\x9F\x98\x80"};
    for (const char* unit: units) {
        for (size_t count = 0; count <= 40; ++count) {
            std::string input;
            for (size_t i = 0; i < count; ++i) {
                input += unit;
            }
            const auto expected = reference_utf8_to_u32(input);
            u32string actual;
            codepoint::decode_utf8(as_bytes(input), input.size(), actual);
            EXPECT_TRUE(same_content(expected, actual)) << "unit=" << unit << " count=" << count;
        }
    }
}

TEST(Utf8BulkDecodeTest, MultipleChunksLongInput) {
    std::string input;
    for (int i = 0; i < 400; ++i) {
        input += "a\xC3\xA9\xE4\xB8\xAD\xF0\x9F\x98\x80";
    }
    const auto expected = reference_utf8_to_u32(input);
    u32string actual;
    codepoint::decode_utf8(as_bytes(input), input.size(), actual);
    EXPECT_TRUE(same_content(expected, actual));
    EXPECT_EQ(actual.size(), 1600U);
}

TEST(Utf8BulkDecodeTest, AppendsWithoutClearing) {
    string result = "prefix-";
    codepoint::decode_utf8(as_bytes(std::string("\xE4\xB8\xAD")), 3, result);
    EXPECT_EQ(result, "prefix-\xE4\xB8\xAD");
}

TEST(Utf8BulkDecodeTest, EmptyInputKeepsTargetIntact) {
    string result = "keep";
    codepoint::decode_utf8(as_bytes(std::string()), 0, result);
    EXPECT_EQ(result, "keep");
}

TEST(Utf8BulkDecodeTest, AsciiFastPathEqualsInput) {
    std::string input(1024, 'x');
    for (size_t i = 0; i < input.size(); ++i) {
        input[i] = static_cast<char>('!' + (i % 90));
    }
    u32string actual;
    codepoint::decode_utf8(as_bytes(input), input.size(), actual);
    ASSERT_EQ(actual.size(), input.size());
    for (size_t i = 0; i < input.size(); ++i) {
        EXPECT_EQ(actual[i], static_cast<char32_t>(static_cast<unsigned char>(input[i])));
    }
}

TEST(Utf8BulkDecodeTest, Utf8IteratorMatchesBulkDecode) {
    for (const auto& input: utf8_corpus()) {
        u32string bulk;
        codepoint::decode_utf8(as_bytes(input), input.size(), bulk);

        u32string iterated;
        for (utf8_iterator it(as_bytes(input), input.size()); it != utf8_iterator(); ++it) {
            iterated.push_back((*it).to_char32());
        }
        EXPECT_TRUE(same_content(bulk, iterated)) << "len=" << input.size();
    }
}

TEST(Utf16BulkDecodeTest, MatchesScalarReference) {
    const char16_t data[] = {u'a', 0x00E9, 0x4E2D, 0xD83D, 0xDE00, u'z'};
    constexpr size_t count = sizeof(data) / sizeof(data[0]);
    for (const bool need_swap: {false, true}) {
        const auto expected_string = reference_utf16_to_string(data, count, need_swap);
        const auto expected_u32 = reference_utf16_to_u32(data, count, need_swap);
        const auto expected_w = reference_utf16_to_wstring(data, count, need_swap);

        string actual_string;
        u32string actual_u32;
        u16string actual_u16;
        wstring actual_w;
        codepoint::decode_utf16(data, count, need_swap, actual_string);
        codepoint::decode_utf16(data, count, need_swap, actual_u32);
        codepoint::decode_utf16(data, count, need_swap, actual_u16);
        codepoint::decode_utf16(data, count, need_swap, actual_w);

        EXPECT_TRUE(same_content(expected_string, actual_string)) << "swap=" << need_swap;
        EXPECT_TRUE(same_content(expected_u32, actual_u32)) << "swap=" << need_swap;
        EXPECT_TRUE(same_content(expected_w, actual_w)) << "swap=" << need_swap;
        EXPECT_EQ(actual_u16.size(), count);
        EXPECT_EQ(actual_w.size(), sizeof(wchar_t) == 2 ? count : expected_u32.size());
    }
}

TEST(Utf16BulkDecodeTest, BmpAndAsciiBlocks) {
    char16_t data[64];
    for (size_t i = 0; i < 64; ++i) {
        data[i] = static_cast<char16_t>(0x4E00 + i);
    }
    const auto expected = reference_utf16_to_u32(data, 64, false);
    u32string actual;
    codepoint::decode_utf16(data, 64, false, actual);
    EXPECT_TRUE(same_content(expected, actual));
}

TEST(Utf16BulkDecodeTest, SurrogateBoundaries) {
    const char16_t lone_high[] = {0xD800, u'a'};
    const char16_t lone_low[] = {0xDC00, u'a'};
    const char16_t high_at_end[] = {u'a', 0xDBFF};
    const char16_t valid_pair[] = {0xD800, 0xDC00};
    const char16_t highest_pair[] = {0xDBFF, 0xDFFF};
    const char16_t bad_pair[] = {0xD800, 0xD800};

    const char16_t* const cases[] = {lone_high, lone_low, high_at_end, valid_pair, highest_pair, bad_pair};
    const size_t sizes[] = {2, 2, 2, 2, 2, 2};
    for (size_t c = 0; c < 6; ++c) {
        const auto expected = reference_utf16_to_u32(cases[c], sizes[c], false);
        u32string actual;
        codepoint::decode_utf16(cases[c], sizes[c], false, actual);
        EXPECT_TRUE(same_content(expected, actual)) << "case=" << c;
    }
}

TEST(Utf32BulkDecodeTest, MatchesScalarReference) {
    const char32_t data[] = {U'a', 0x00E9, 0x4E2D, 0x1F600, 0x10FFFF, 0xD800, 0x110000, 0xFFFFFFFFU, U'z'};
    constexpr size_t count = sizeof(data) / sizeof(data[0]);
    const auto expected_string = reference_utf32_to_string(data, count);
    const auto expected_u16 = reference_utf32_to_u16(data, count);

    string actual_string;
    u16string actual_u16;
    u32string actual_u32;
    codepoint::encode_utf32(data, count, actual_string);
    codepoint::encode_utf32(data, count, actual_u16);
    codepoint::encode_utf32(data, count, actual_u32);

    EXPECT_TRUE(same_content(expected_string, actual_string));
    EXPECT_TRUE(same_content(expected_u16, actual_u16));
    EXPECT_EQ(actual_u32.size(), count);
    EXPECT_EQ(actual_u32[5], 0xFFFDU);
    EXPECT_EQ(actual_u32[6], 0xFFFDU);
    EXPECT_EQ(actual_u32[7], 0xFFFDU);
}

TEST(Utf32BulkDecodeTest, LongValidRun) {
    std::vector<char32_t> data(600);
    for (size_t i = 0; i < data.size(); ++i) {
        data[i] = static_cast<char32_t>(0x4E00 + (i % 1000));
    }
    const auto expected = reference_utf32_to_string(data.data(), data.size());
    string actual;
    codepoint::encode_utf32(data.data(), data.size(), actual);
    EXPECT_TRUE(same_content(expected, actual));
}

TEST(WcharBulkDecodeTest, MatchesScalarReference) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    const wchar_t data[] = {L'a', L'z', 0xD83D, 0xDE00};
#else
    const wchar_t data[] = {L'a', static_cast<wchar_t>(0x00E9), static_cast<wchar_t>(0x4E2D),
                            static_cast<wchar_t>(0x1F600)};
#endif
    constexpr size_t count = sizeof(data) / sizeof(data[0]);
    const auto expected_string = reference_wchar_to_string(data, count);
    const auto expected_u16 = reference_wchar_to_u16(data, count);

    string actual_string;
    u16string actual_u16;
    u32string actual_u32;
    codepoint::decode_wchar(data, count, false, actual_string);
    codepoint::decode_wchar(data, count, false, actual_u16);
    codepoint::decode_wchar(data, count, false, actual_u32);

    const auto expected_u32 = reference_wchar_to_u32(data, count);

    EXPECT_TRUE(same_content(expected_string, actual_string));
    EXPECT_TRUE(same_content(expected_u16, actual_u16));
    EXPECT_TRUE(same_content(expected_u32, actual_u32));
}

TEST(UtfWrapperTest, CharacterConversionsMatchScalarReference) {
    for (const auto& input: utf8_corpus()) {
        const string_view view{input.data(), input.size()};
        // character::to_string keeps the raw bytes; the other targets transcode.
        EXPECT_TRUE(same_content(string{string_view{input.data(), input.size()}}, character::to_string(view)));
        EXPECT_TRUE(same_content(reference_utf8_to_wstring(input), character::to_wstring(view)));
        EXPECT_TRUE(same_content(reference_utf8_to_u16(input), character::to_u16string(view)));
        EXPECT_TRUE(same_content(reference_utf8_to_u32(input), character::to_u32string(view)));
    }
}

TEST(UtfWrapperTest, U16CharacterConversionsMatchScalarReference) {
    const char16_t data[] = {0xFEFF, u'a', 0x00E9, 0x4E2D, 0xD83D, 0xDE00, u'z'};
    constexpr size_t count = sizeof(data) / sizeof(data[0]);
    const basic_string_view<char16_t> view{data, count};

    EXPECT_TRUE(same_content(reference_utf16_to_string(data + 1, count - 1, false), u16character::to_string(view)));
    EXPECT_TRUE(same_content(reference_utf16_to_u32(data + 1, count - 1, false), u16character::to_u32string(view)));
    EXPECT_EQ(u16character::to_u16string(view).size(), count - 1);
    EXPECT_FALSE(u16character::to_wstring(view).empty());
}

TEST(UtfWrapperTest, U32CharacterConversionsMatchScalarReference) {
    const char32_t data[] = {U'a', 0x00E9, 0x4E2D, 0x1F600, U'z'};
    constexpr size_t count = sizeof(data) / sizeof(data[0]);
    const basic_string_view<char32_t> view{data, count};

    EXPECT_TRUE(same_content(reference_utf32_to_string(data, count), u32character::to_string(view)));
    EXPECT_TRUE(same_content(reference_utf32_to_u16(data, count), u32character::to_u16string(view)));
    EXPECT_EQ(u32character::to_u32string(view).size(), count);
    EXPECT_FALSE(u32character::to_wstring(view).empty());
}

TEST(UtfWrapperTest, WCharacterConversionsMatchScalarReference) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    const wchar_t data[] = {L'a', L'z', 0xD83D, 0xDE00};
#else
    const wchar_t data[] = {L'a', static_cast<wchar_t>(0x00E9), static_cast<wchar_t>(0x4E2D)};
#endif
    constexpr size_t count = sizeof(data) / sizeof(data[0]);
    const basic_string_view<wchar_t> view{data, count};

    EXPECT_TRUE(same_content(reference_wchar_to_string(data, count), wcharacter::to_string(view)));
    EXPECT_TRUE(same_content(reference_wchar_to_u16(data, count), wcharacter::to_u16string(view)));
    EXPECT_EQ(wcharacter::to_wstring(view).size(), count);
    EXPECT_TRUE(same_content(reference_wchar_to_u32(data, count), wcharacter::to_u32string(view)));
}

TEST(UtfRoundTripTest, Utf8ToUtf16AndBack) {
    const std::vector<std::string> cases = {"",
                                            "ascii",
                                            "\xC3\xA9",
                                            "\xE4\xB8\xAD\xE6\x96\x87",
                                            "\xF0\x9F\x98\x80",
                                            "a\xC3\xA9\xE4\xB8\xAD\xF0\x9F\x98\x80z"};
    for (const auto& input: cases) {
        u16string utf16;
        codepoint::decode_utf8(as_bytes(input), input.size(), utf16);
        string back;
        codepoint::decode_utf16(utf16.data(), utf16.size(), false, back);
        EXPECT_TRUE(same_content(string{string_view{input.data(), input.size()}}, back))
                << "input len=" << input.size();
    }
}

TEST(UtfRoundTripTest, Utf8ToUtf32AndBack) {
    const std::vector<std::string> cases = {
            "", "ascii", "\xC3\xA9", "\xE4\xB8\xAD", "\xF0\x9F\x98\x80", "\xE4\xB8\xAD\xF0\x9F\x98\x80"};
    for (const auto& input: cases) {
        u32string utf32;
        codepoint::decode_utf8(as_bytes(input), input.size(), utf32);
        string back;
        codepoint::encode_utf32(utf32.data(), utf32.size(), back);
        EXPECT_TRUE(same_content(string{string_view{input.data(), input.size()}}, back))
                << "input len=" << input.size();
    }
}

TEST(UtfRoundTripTest, InvalidSequencesNormalizeToReplacement) {
    string result;
    const std::string input = "\xED\xA0\x80";
    codepoint::decode_utf8(as_bytes(input), input.size(), result);
    EXPECT_EQ(result, "\xEF\xBF\xBD");
}

TEST(DisplayWidthTest, KnownCodePoints) {
    EXPECT_EQ(codepoint(static_cast<uint32_t>('A')).display_width(), 1);
    EXPECT_EQ(codepoint(0x0060U).display_width(), 1);
    EXPECT_EQ(codepoint(0x007FU).display_width(), 0);
    EXPECT_EQ(codepoint(0x001FU).display_width(), 0);
    EXPECT_EQ(codepoint(0x0020U).display_width(), 1);
    EXPECT_EQ(codepoint(0x00A0U).display_width(), 1);
    EXPECT_EQ(codepoint(0x0301U).display_width(), 0);
    EXPECT_EQ(codepoint(0x200CU).display_width(), 0);
    EXPECT_EQ(codepoint(0x200DU).display_width(), 0);
    EXPECT_EQ(codepoint(0x200EU).display_width(), 0);
    EXPECT_EQ(codepoint(0x200FU).display_width(), 0);
    EXPECT_EQ(codepoint(0x2010U).display_width(), 1);
    EXPECT_EQ(codepoint(0x4E2DU).display_width(), 2);
    EXPECT_EQ(codepoint(0x1F600U).display_width(), 2);
    EXPECT_EQ(codepoint(0x1F004U).display_width(), 2);
    EXPECT_EQ(codepoint(0xE000U).display_width(), 1);
    EXPECT_EQ(codepoint(0x10FFFFU).display_width(), 1);
    EXPECT_EQ(codepoint(0xFE0FU).display_width(), 0);
}

TEST(DisplayWidthTest, WideRangeEdges) {
    EXPECT_EQ(codepoint(0x10FFU).display_width(), 1);
    EXPECT_EQ(codepoint(0x1100U).display_width(), 2);
    EXPECT_EQ(codepoint(0x115FU).display_width(), 2);
    EXPECT_EQ(codepoint(0x1160U).display_width(), 1);

    EXPECT_EQ(codepoint(0x2E7FU).display_width(), 1);
    EXPECT_EQ(codepoint(0x2E80U).display_width(), 2);
    EXPECT_EQ(codepoint(0x33BFU).display_width(), 2);
    EXPECT_EQ(codepoint(0x33C0U).display_width(), 1);

    EXPECT_EQ(codepoint(0x4DFFU).display_width(), 1);
    EXPECT_EQ(codepoint(0x4E00U).display_width(), 2);
    EXPECT_EQ(codepoint(0xA4CFU).display_width(), 2);
    EXPECT_EQ(codepoint(0xA4D0U).display_width(), 1);

    EXPECT_EQ(codepoint(0xABFFU).display_width(), 1);
    EXPECT_EQ(codepoint(0xAC00U).display_width(), 2);
    EXPECT_EQ(codepoint(0xD7AFU).display_width(), 2);
    EXPECT_EQ(codepoint(0xD7B0U).display_width(), 1);

    EXPECT_EQ(codepoint(0xFF00U).display_width(), 1);
    EXPECT_EQ(codepoint(0xFF01U).display_width(), 2);
    EXPECT_EQ(codepoint(0xFF60U).display_width(), 2);
    EXPECT_EQ(codepoint(0xFF61U).display_width(), 1);

    EXPECT_EQ(codepoint(0x1F003U).display_width(), 1);
    EXPECT_EQ(codepoint(0x1F9FFU).display_width(), 2);
    EXPECT_EQ(codepoint(0x1FA00U).display_width(), 1);

    EXPECT_EQ(codepoint(0x1FFFEU).display_width(), 1);
    EXPECT_EQ(codepoint(0x1FFFFU).display_width(), 1);
    EXPECT_EQ(codepoint(0x20000U).display_width(), 2);
    EXPECT_EQ(codepoint(0x2FFFDU).display_width(), 2);
    EXPECT_EQ(codepoint(0x2FFFEU).display_width(), 1);
    EXPECT_EQ(codepoint(0x2FFFFU).display_width(), 1);
    EXPECT_EQ(codepoint(0x30000U).display_width(), 2);
    EXPECT_EQ(codepoint(0x3FFFDU).display_width(), 2);
    EXPECT_EQ(codepoint(0x3FFFEU).display_width(), 1);
}

TEST(DisplayWidthTest, ZeroRangeEdges) {
    EXPECT_EQ(codepoint(0x0080U).display_width(), 0);
    EXPECT_EQ(codepoint(0x009FU).display_width(), 0);
    EXPECT_EQ(codepoint(0x00A1U).display_width(), 1);
    EXPECT_EQ(codepoint(0x02FFU).display_width(), 1);
    EXPECT_EQ(codepoint(0x0370U).display_width(), 1);
    EXPECT_EQ(codepoint(0x1AAFU).display_width(), 1);
    EXPECT_EQ(codepoint(0x1AB0U).display_width(), 0);
    EXPECT_EQ(codepoint(0x1AFFU).display_width(), 0);
    EXPECT_EQ(codepoint(0x1B00U).display_width(), 1);
    EXPECT_EQ(codepoint(0x1DBFU).display_width(), 1);
    EXPECT_EQ(codepoint(0x1DC0U).display_width(), 0);
    EXPECT_EQ(codepoint(0x1DFFU).display_width(), 0);
    EXPECT_EQ(codepoint(0x1E00U).display_width(), 1);
    EXPECT_EQ(codepoint(0x20CFU).display_width(), 1);
    EXPECT_EQ(codepoint(0x20D0U).display_width(), 0);
    EXPECT_EQ(codepoint(0x20FFU).display_width(), 0);
    EXPECT_EQ(codepoint(0x2100U).display_width(), 1);
    EXPECT_EQ(codepoint(0xFDFFU).display_width(), 1);
    EXPECT_EQ(codepoint(0xFE10U).display_width(), 2);
    EXPECT_EQ(codepoint(0xFE30U).display_width(), 2);
    EXPECT_EQ(codepoint(0xFE70U).display_width(), 1);
}

TEST(CodepointBulkApiTest, SingleCodePointApiUnchanged) {
    const std::string input = "a\xC3\xA9\xE4\xB8\xAD\xF0\x9F\x98\x80";
    size_t i = 0;
    const size_t len = input.size();
    u32string collected;
    while (i < len) {
        collected.push_back(codepoint::decode_utf8(as_bytes(input), i, len).to_char32());
    }
    ASSERT_EQ(collected.size(), 4U);
    EXPECT_EQ(collected[0], U'a');
    EXPECT_EQ(collected[1], 0x00E9U);
    EXPECT_EQ(collected[2], 0x4E2DU);
    EXPECT_EQ(collected[3], 0x1F600U);
}

TEST(CodepointBulkApiTest, SingleCodePointEncodeUnchanged) {
    string result;
    codepoint(0x4E2DU).append_to(result);
    codepoint(0x1F600U).append_to(result);
    EXPECT_EQ(result, "\xE4\xB8\xAD\xF0\x9F\x98\x80");

    u16string utf16;
    codepoint(0x1F600U).append_to(utf16);
    ASSERT_EQ(utf16.size(), 2U);
    EXPECT_EQ(utf16[0], 0xD83DU);
    EXPECT_EQ(utf16[1], 0xDE00U);
}
