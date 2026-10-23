#include <NeForce/core/string/codepoint.hpp>
#include <NeForce/core/string/width.hpp>
#include <NeForce/tui/dom/text_layout.hpp>
#include <gtest/gtest.h>
using namespace neforce;
using namespace neforce::tui;


TEST(TuiWidthTest, AsciiCountsOneCell) {
    EXPECT_EQ(text_display_width("hello"), 5);
    EXPECT_EQ(text_display_width(""), 0);
    EXPECT_EQ(text_display_width("a b"), 3);
}

TEST(TuiWidthTest, CjkCountsTwoCells) {
    EXPECT_EQ(text_display_width("\xe4\xbd\xa0\xe5\xa5\xbd"), 4);
    EXPECT_EQ(text_display_width("\xe3\x81\x82"), 2);
    EXPECT_EQ(text_display_width("\xed\x95\x9c"), 2);
}

TEST(TuiWidthTest, EmojiPresentationCountsTwoCells) {
    EXPECT_EQ(text_display_width("\xf0\x9f\x98\x80"), 2);
    EXPECT_EQ(text_display_width("\xf0\x9f\x90\x8b"), 2);
    EXPECT_EQ(text_display_width("a\xf0\x9f\x98\x80"
                                 "b"),
              4);
}

TEST(TuiWidthTest, TextDefaultSymbolsCountOneCell) {
    EXPECT_EQ(text_display_width("\xe2\x9a\xa0"), 1);
    EXPECT_EQ(text_display_width("\xe2\x9c\xb3"), 1);
    EXPECT_EQ(text_display_width("\xe2\x9d\xa4"), 1);
    EXPECT_EQ(text_display_width("\xe2\x9c\x82"), 1);
}

TEST(TuiWidthTest, CombiningMarksAndSelectorsAreZeroWidth) {
    EXPECT_EQ(text_display_width("e\xcc\x81"), 1);
    EXPECT_EQ(text_display_width("\xe2\x80\x8b"), 0);
}

TEST(TuiWidthTest, TextDefaultEmojiWithVs16StaysNarrow) {
    // Width is computed per code point; a variation-selector sequence that
    // upgrades a text-default emoji needs grapheme context and is not widened.
    EXPECT_EQ(text_display_width("\xe2\x9d\xa4\xef\xb8\x8f"), 1);
}

TEST(TuiWidthTest, MixedLine) { EXPECT_EQ(text_display_width("\xe4\xbd\xa0 ok \xf0\x9f\x98\x80"), 2 + 4 + 2); }

TEST(TuiWidthTest, LayoutHonoursWideGlyphs) {
    const text_layout lay = layout_text("\xe4\xbd\xa0\xe4\xbd\xa0\xe4\xbd\xa0", 4, style::wrap_mode::character);
    ASSERT_EQ(lay.lines.size(), 2u);
    EXPECT_EQ(lay.lines[0].end, 6u);
}

TEST(TuiWidthTest, LayoutKeepsEmojiOnOneRow) {
    const text_layout lay = layout_text("\xf0\x9f\x98\x80\xf0\x9f\x98\x80", 4, style::wrap_mode::character);
    ASSERT_EQ(lay.lines.size(), 1u);
}

TEST(TextWidthCoreTest, WidthIsUsableWithoutTheTuiLayer) {
    EXPECT_EQ(::neforce::text_display_width(string_view{"abc"}), 3);
    EXPECT_EQ(::neforce::text_display_width(string_view{"\xe4\xb8\xad"}), 2);
    EXPECT_EQ(::neforce::text_display_width(string_view{"e\xcc\x81"}), 1);
    EXPECT_EQ(::neforce::text_display_width(string_view{}), 0);
}
