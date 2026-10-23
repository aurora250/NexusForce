#include <NeForce/core/string/width.hpp>
#include <NeForce/tui/component/component.hpp>
#include <NeForce/tui/dom/diff.hpp>
#include <NeForce/tui/dom/element.hpp>
#include <NeForce/tui/headless.hpp>
#include <gtest/gtest.h>
using namespace neforce;
using namespace neforce::tui;

namespace {
    struct diff_root : component<> {
        element content = element::empty();

        element render() override { return content; }
    };

    string render_plaintext(const element& content, const int width, const int height) {
        diff_root root;
        root.content = content;
        headless app(&root, width, height);
        app.render();
        return app.plaintext();
    }

    bool contains(const string& haystack, const string_view needle) {
        return haystack.view().find(needle) != string_view::npos;
    }

    size_t count_occurrences(const string& haystack, const string_view needle) {
        size_t count = 0;
        size_t pos = 0;
        const auto raw = haystack.view();
        while ((pos = raw.find(needle, pos)) != string_view::npos) {
            ++count;
            pos += needle.size();
        }
        return count;
    }

    vector<string_view> text_lines(const string& text) {
        vector<string_view> lines;
        const auto raw = text.view();
        size_t start = 0;
        for (size_t i = 0; i <= raw.size(); ++i) {
            if (i == raw.size() || raw[i] == '\n') {
                lines.push_back(raw.substr(start, i - start));
                start = i + 1;
            }
        }
        return lines;
    }

    string inline_text(const element& el) {
        if (el.kind() == element::kind::text) {
            return el.text();
        }
        if (el.kind() == element::kind::rich_text) {
            string result;
            for (const auto& span: el.spans()) {
                result += span.content;
            }
            return result;
        }
        string result;
        for (const auto& child: el.children()) {
            result += inline_text(child);
        }
        return result;
    }

    void collect_rows(const element& el, vector<string>& rows) {
        switch (el.kind()) {
            case element::kind::text:
            case element::kind::rich_text:
            case element::kind::hbox:
                rows.push_back(inline_text(el));
                return;
            case element::kind::vbox:
                for (const auto& child: el.children()) {
                    collect_rows(child, rows);
                }
                return;
            case element::kind::band:
            case element::kind::panel:
            case element::kind::scroll_view:
                if (!el.children().empty()) {
                    collect_rows(el.children().front(), rows);
                }
                return;
            default:
                break;
        }
        for (const auto& child: el.children()) {
            collect_rows(child, rows);
        }
    }

    vector<string> rows_of(const element& el) {
        vector<string> rows;
        collect_rows(el, rows);
        return rows;
    }

    void collect_rich_texts(const element& el, vector<const element*>& out) {
        if (el.kind() == element::kind::rich_text) {
            out.push_back(&el);
        }
        for (const auto& child: el.children()) {
            collect_rich_texts(child, out);
        }
    }

    vector<const element*> rich_texts_of(const element& el) {
        vector<const element*> result;
        collect_rich_texts(el, result);
        return result;
    }

    string spans_text(const element& el) {
        string result;
        for (const auto& span: el.spans()) {
            result += span.content;
        }
        return result;
    }

    const text_span* find_span(const element& el, const string_view content) {
        for (const auto& span: el.spans()) {
            if (span.content.view() == content) {
                return &span;
            }
        }
        return nullptr;
    }

    bool valid_utf8(const string_view text) {
        size_t i = 0;
        while (i < text.size()) {
            const auto lead = static_cast<unsigned char>(text[i]);
            size_t extra = 0;
            if (lead < 0x80) {
                ++i;
                continue;
            }
            if ((lead & 0xE0) == 0xC0) {
                extra = 1;
            } else if ((lead & 0xF0) == 0xE0) {
                extra = 2;
            } else if ((lead & 0xF8) == 0xF0) {
                extra = 3;
            } else {
                return false;
            }
            if (i + extra >= text.size()) {
                return false;
            }
            for (size_t k = 1; k <= extra; ++k) {
                if ((static_cast<unsigned char>(text[i + k]) & 0xC0) != 0x80) {
                    return false;
                }
            }
            i += extra + 1;
        }
        return true;
    }

    const string_view separator{"\xe2\x94\x82"};
    const string_view ellipsis{"\xe2\x80\xa6"};

    vector<diff_line> simple_lines() {
        return parse_unified_diff("@@ -1,3 +1,3 @@\n"
                                  " alpha\n"
                                  "-old line\n"
                                  "+new line\n"
                                  " omega\n");
    }
} // namespace


TEST(TuiDiffSideBySideTest, ShowsBothSidesWithVerticalSeparator) {
    diff_view_option opt;
    opt.width = 120;
    opt.side_by_side = true;
    opt.background = false;

    const element el = diff_view(simple_lines(), opt);
    const string plain = render_plaintext(el, 120, 8);

    EXPECT_TRUE(contains(plain, "old line"));
    EXPECT_TRUE(contains(plain, "new line"));
    EXPECT_TRUE(contains(plain, separator));

    bool same_row = false;
    for (const auto line: text_lines(plain)) {
        if (line.find("old line") != string_view::npos && line.find("new line") != string_view::npos) {
            same_row = true;
        }
    }
    EXPECT_TRUE(same_row);
}

TEST(TuiDiffSideBySideTest, EveryRowFillsRequestedWidth) {
    diff_view_option opt;
    opt.width = 120;
    opt.side_by_side = true;
    opt.background = false;

    const vector<string> rows = rows_of(diff_view(simple_lines(), opt));

    ASSERT_EQ(rows.size(), 4U);
    for (const auto& row: rows) {
        EXPECT_EQ(text_display_width(row.view()), 120) << row.data();
    }
    EXPECT_EQ(count_occurrences(rows[1], separator), 1U);
    EXPECT_EQ(count_occurrences(rows[0], separator), 0U);
}

TEST(TuiDiffSideBySideTest, PadsUnequalRemovedAndAddedRuns) {
    const auto lines = parse_unified_diff("@@ -1,2 +1,1 @@\n"
                                          "-old one\n"
                                          "-old two\n"
                                          "+new one\n");
    diff_view_option opt;
    opt.width = 120;
    opt.side_by_side = true;
    opt.background = false;

    const vector<string> rows = rows_of(diff_view(lines, opt));

    ASSERT_EQ(rows.size(), 3U);
    EXPECT_TRUE(contains(rows[1], "old one"));
    EXPECT_TRUE(contains(rows[1], "new one"));

    const auto position = rows[2].view().find(separator);
    ASSERT_NE(position, string_view::npos);
    const string_view tail = rows[2].view().substr(position + separator.size());
    bool blank = !tail.empty();
    for (size_t i = 0; i < tail.size(); ++i) {
        if (tail[i] != ' ') {
            blank = false;
        }
    }
    EXPECT_TRUE(blank);
    EXPECT_EQ(text_display_width(rows[2].view()), 120);
}

TEST(TuiDiffSideBySideTest, FallsBackToUnifiedWhenNarrow) {
    diff_view_option narrow;
    narrow.width = 60;
    narrow.side_by_side = true;
    narrow.background = false;

    diff_view_option unified;
    unified.width = 60;
    unified.background = false;

    const vector<string> split_rows = rows_of(diff_view(simple_lines(), narrow));
    const vector<string> unified_rows = rows_of(diff_view(simple_lines(), unified));

    ASSERT_EQ(split_rows.size(), unified_rows.size());
    for (size_t i = 0; i < split_rows.size(); ++i) {
        EXPECT_EQ(split_rows[i], unified_rows[i]);
        EXPECT_EQ(split_rows[i].view().find(separator), string_view::npos);
    }

    diff_view_option threshold;
    threshold.width = 110;
    threshold.side_by_side = true;
    threshold.background = false;
    const vector<string> threshold_rows = rows_of(diff_view(simple_lines(), threshold));
    ASSERT_GT(threshold_rows.size(), 1U);
    EXPECT_NE(threshold_rows[1].view().find(separator), string_view::npos);
}

TEST(TuiDiffSideBySideTest, RepeatsContextOnBothSides) {
    const auto lines = parse_unified_diff("@@ -1,1 +1,1 @@\n"
                                          " same\n");
    diff_view_option opt;
    opt.width = 120;
    opt.side_by_side = true;
    opt.background = false;

    const vector<string> rows = rows_of(diff_view(lines, opt));

    ASSERT_EQ(rows.size(), 2U);
    EXPECT_EQ(count_occurrences(rows[1], "same"), 2U);
    EXPECT_EQ(count_occurrences(rows[1], separator), 1U);
}

TEST(TuiDiffSideBySideTest, OmitsGutterAndMarkersWhenDisabled) {
    diff_view_option opt;
    opt.width = 120;
    opt.side_by_side = true;
    opt.background = false;
    opt.show_gutter = false;
    opt.show_markers = false;

    const vector<string> rows = rows_of(diff_view(simple_lines(), opt));

    ASSERT_EQ(rows.size(), 4U);
    for (const auto& row: rows) {
        EXPECT_EQ(text_display_width(row.view()), 120) << row.data();
    }
    EXPECT_EQ(rows[2].view().find("old line"), 0U);
    EXPECT_EQ(count_occurrences(rows[2], separator), 1U);
}

TEST(TuiDiffSideBySideTest, PadsGroupedFileHeaderToWidth) {
    const string patch = "diff --git a/demo.txt b/demo.txt\n"
                         "--- a/demo.txt\n"
                         "+++ b/demo.txt\n"
                         "@@ -1,1 +1,1 @@\n"
                         "-old\n"
                         "+new\n";
    diff_view_option opt;
    opt.width = 120;
    opt.side_by_side = true;
    opt.background = false;

    const vector<string> rows = rows_of(diff_view(patch.view(), opt));

    ASSERT_EQ(rows.size(), 3U);
    EXPECT_TRUE(contains(rows[0], "demo.txt"));
    EXPECT_EQ(text_display_width(rows[0].view()), 120);
    EXPECT_EQ(count_occurrences(rows[1], separator), 0U);
    EXPECT_EQ(count_occurrences(rows[2], separator), 1U);
}

TEST(TuiDiffWordLevelTest, HighlightsDifferingMiddleInUnifiedView) {
    const vector<diff_line> lines{diff_line{diff_kind::removed, "foo bar baz", 1, 0},
                                  diff_line{diff_kind::added, "foo qux baz", 0, 1}};
    diff_view_option opt;
    opt.word_level = true;
    opt.background = false;

    const element el = diff_view(lines, opt);
    const auto rich = rich_texts_of(el);

    ASSERT_EQ(rich.size(), 2U);
    ASSERT_EQ(rich[0]->spans().size(), 3U);
    EXPECT_EQ(rich[0]->spans()[0].content, "foo ");
    EXPECT_EQ(rich[0]->spans()[1].content, "bar");
    EXPECT_EQ(rich[0]->spans()[2].content, " baz");
    EXPECT_TRUE(rich[0]->spans()[1].style.bold.has_value());
    EXPECT_TRUE(rich[0]->spans()[1].style.bold.value());
    EXPECT_FALSE(rich[0]->spans()[0].style.bold.has_value());

    ASSERT_EQ(rich[1]->spans().size(), 3U);
    EXPECT_EQ(rich[1]->spans()[0].content, "foo ");
    EXPECT_EQ(rich[1]->spans()[1].content, "qux");
    EXPECT_TRUE(rich[1]->spans()[1].style.bold.value());
    EXPECT_EQ(spans_text(*rich[0]), "foo bar baz");
    EXPECT_EQ(spans_text(*rich[1]), "foo qux baz");
}

TEST(TuiDiffWordLevelTest, HighlightsDifferingMiddleInSplitView) {
    const vector<diff_line> lines{diff_line{diff_kind::removed, "foo bar baz", 1, 0},
                                  diff_line{diff_kind::added, "foo qux baz", 0, 1}};
    diff_view_option opt;
    opt.width = 120;
    opt.side_by_side = true;
    opt.word_level = true;
    opt.background = false;

    const element el = diff_view(lines, opt);
    const auto rich = rich_texts_of(el);

    ASSERT_EQ(rich.size(), 2U);
    EXPECT_TRUE(spans_text(*rich[0]).view().starts_with("foo bar baz"));
    EXPECT_TRUE(spans_text(*rich[1]).view().starts_with("foo qux baz"));

    const text_span* removed_word = find_span(*rich[0], "bar");
    ASSERT_NE(removed_word, nullptr);
    EXPECT_TRUE(removed_word->style.bold.has_value());
    EXPECT_TRUE(removed_word->style.bold.value());

    const text_span* added_word = find_span(*rich[1], "qux");
    ASSERT_NE(added_word, nullptr);
    EXPECT_TRUE(added_word->style.bold.value());

    const vector<string> rows = rows_of(el);
    ASSERT_EQ(rows.size(), 1U);
    EXPECT_EQ(text_display_width(rows[0].view()), 120);
}

TEST(TuiDiffWordLevelTest, KeepsUtf8CodepointsTogether) {
    const vector<diff_line> lines{diff_line{diff_kind::removed, "héllo 世界x", 1, 0},
                                  diff_line{diff_kind::added, "héllo 世界y", 0, 1}};
    diff_view_option opt;
    opt.word_level = true;
    opt.background = false;

    const element el = diff_view(lines, opt);
    const auto rich = rich_texts_of(el);

    ASSERT_EQ(rich.size(), 2U);
    ASSERT_GE(rich[0]->spans().size(), 2U);
    EXPECT_EQ(rich[0]->spans()[0].content, "héllo 世界");
    EXPECT_EQ(rich[0]->spans()[1].content, "x");
    EXPECT_EQ(rich[1]->spans()[1].content, "y");
    EXPECT_EQ(spans_text(*rich[0]), "héllo 世界x");
    EXPECT_EQ(spans_text(*rich[1]), "héllo 世界y");
    for (const auto* rich_text: rich) {
        for (const auto& span: rich_text->spans()) {
            EXPECT_TRUE(valid_utf8(span.content.view()));
        }
    }
}

TEST(TuiDiffWordLevelTest, TruncatesSplitCellsWithoutBreakingUtf8) {
    const vector<diff_line> lines{diff_line{diff_kind::removed, "世界世界世界世界世界世界", 1, 0},
                                  diff_line{diff_kind::added, "世界世界世界世界世界世界x", 0, 1}};
    diff_view_option opt;
    opt.width = 24;
    opt.split_min_width = 16;
    opt.side_by_side = true;
    opt.word_level = true;
    opt.show_gutter = false;
    opt.show_markers = false;
    opt.background = false;

    const element el = diff_view(lines, opt);
    const vector<string> rows = rows_of(el);

    ASSERT_EQ(rows.size(), 1U);
    EXPECT_EQ(text_display_width(rows[0].view()), 24);
    EXPECT_NE(rows[0].view().find(separator), string_view::npos);
    EXPECT_GE(count_occurrences(rows[0], ellipsis), 2U);
    EXPECT_EQ(rows[0].view().find("\xef\xbf\xbd"), string_view::npos);

    const auto rich = rich_texts_of(el);
    ASSERT_EQ(rich.size(), 2U);
    for (const auto* rich_text: rich) {
        EXPECT_FALSE(spans_text(*rich_text).empty());
        for (const auto& span: rich_text->spans()) {
            EXPECT_TRUE(valid_utf8(span.content.view()));
        }
    }
}

TEST(TuiDiffCompatibilityTest, UnifiedRenderingIsUnchanged) {
    const auto lines = parse_unified_diff("@@ -1,2 +1,2 @@\n"
                                          " same\n"
                                          "-old\n"
                                          "+new\n");
    diff_view_option opt;
    opt.background = false;

    const vector<string> rows = rows_of(diff_view(lines, opt));

    ASSERT_EQ(rows.size(), 4U);
    EXPECT_EQ(rows[0], "     @@ -1,2 +1,2 @@");
    EXPECT_EQ(rows[1], "1 1  same");
    EXPECT_EQ(rows[2], "2   -old");
    EXPECT_EQ(rows[3], "  2 +new");
}

TEST(TuiDiffCompatibilityTest, SideBySideIsOffByDefault) {
    diff_view_option opt;
    opt.width = 120;
    opt.background = false;

    const vector<string> rows = rows_of(diff_view(simple_lines(), opt));

    ASSERT_FALSE(rows.empty());
    for (const auto& row: rows) {
        EXPECT_EQ(row.view().find(separator), string_view::npos);
    }
}
