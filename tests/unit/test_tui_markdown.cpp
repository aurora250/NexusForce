#include <NeForce/core/async/io_context.hpp>
#include <NeForce/core/async/strand.hpp>
#include <NeForce/tui/component/component.hpp>
#include <NeForce/tui/dom/markdown.hpp>
#include <NeForce/tui/dom/text_layout.hpp>
#include <NeForce/tui/headless.hpp>
#include <gtest/gtest.h>
using namespace neforce;
using namespace neforce::tui;

namespace {
    struct markdown_root : component<> {
        element content = element::empty();

        element render() override { return content; }
    };

    string render_plaintext(const string& source, const int width, const int height) {
        markdown_root root;
        markdown_option opt;
        opt.width = width;
        root.content = markdown(source.view(), opt);
        headless app(&root, width, height);
        app.render();
        return app.plaintext();
    }

    bool contains(const string& haystack, const string_view needle) {
        return haystack.view().find(needle) != string_view::npos;
    }
} // namespace


TEST(TuiMarkdownTableTest, DrawsBordersAndAlignsHeader) {
    const string source = "| step | owner |\n"
                          "| :--- | ---: |\n"
                          "| parse | al |\n";
    const string text = render_plaintext(source, 40, 8);
    EXPECT_TRUE(contains(text, "\xe2\x94\x8c"));
    EXPECT_TRUE(contains(text, "\xe2\x94\xac"));
    EXPECT_TRUE(contains(text, "\xe2\x94\x82"));
    EXPECT_TRUE(contains(text, "step"));
    EXPECT_TRUE(contains(text, "parse"));
}

TEST(TuiMarkdownTableTest, WrapsCellsWithinBudget) {
    const string source = "| a | b |\n"
                          "| --- | --- |\n"
                          "| a very long cell that must wrap | short |\n";
    const string text = render_plaintext(source, 30, 12);
    int widest = 0;
    size_t start = 0;
    const auto& raw = text.view();
    for (size_t i = 0; i <= raw.size(); ++i) {
        if (i == raw.size() || raw[i] == '\n') {
            widest = max(widest, text_display_width(raw.substr(start, i - start)));
            start = i + 1;
        }
    }
    EXPECT_LE(widest, 30) << "\n" << text.data();
}

TEST(TuiMarkdownQuoteTest, WrappedLinesKeepTheRail) {
    const string source = "> a quoted line that is long enough to wrap onto another row\n";
    const string text = render_plaintext(source, 24, 10);
    const size_t first = text.view().find(string_view{"\xe2\x96\x8e"});
    ASSERT_NE(first, string_view::npos);
    const size_t second = text.view().find(string_view{"\xe2\x96\x8e"}, first + 1);
    EXPECT_NE(second, string_view::npos);
}

TEST(TuiMarkdownLinkTest, LinkSpansCarryTheDestination) {
    const element tree = markdown("see [docs](https://example.com) now", markdown_option{});
    ASSERT_GT(tree.children().size(), 0u);
    const element& paragraph = tree.children()[0];
    const vector<text_span>& spans = paragraph.spans();
    bool found = false;
    for (const auto& span: spans) {
        if (span.content == string{"docs"}) {
            EXPECT_EQ(span.link, string{"https://example.com"});
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST(TuiMarkdownImageTest, ImageRendersMarkerAndAltText) {
    const string text = render_plaintext("![a chart](chart.png)", 40, 3);
    EXPECT_TRUE(contains(text, "[img]"));
    EXPECT_TRUE(contains(text, "a chart"));
}

TEST(TuiMarkdownStreamTest, MatchesOneShotRendering) {
    const string source = "# title\n"
                          "\n"
                          "first paragraph\n"
                          "\n"
                          "second paragraph with more text\n"
                          "\n"
                          "```cpp\n"
                          "int x = 1;\n"
                          "```\n";
    markdown_option opt;
    opt.width = 40;
    markdown_stream stream(opt);
    for (size_t i = 0; i < source.size(); i += 7) {
        stream.append(source.view().substr(i, min<size_t>(7, source.size() - i)));
        stream.render(40);
    }

    markdown_root root;
    root.content = stream.render(40);
    headless streamed(&root, 40, 14);
    streamed.render();

    const string one_shot = render_plaintext(source, 40, 14);
    EXPECT_EQ(streamed.plaintext(), one_shot);
}

TEST(TuiMarkdownStreamTest, SealsOnlyAtSafeBoundaries) {
    markdown_option opt;
    opt.width = 40;
    markdown_stream stream(opt);
    string chunk;
    for (int i = 0; i < 400; ++i) {
        chunk += "line of text\n";
        if (i % 2 == 0) {
            chunk += "\n";
        }
    }
    stream.append(chunk.view());
    stream.render(40);
    EXPECT_GT(stream.sealed_blocks(), 0u);
    EXPECT_LE(stream.sealed_bytes(), stream.text().size());
}

TEST(TuiMarkdownStreamTest, UnclosedFenceIsNotSealed) {
    markdown_option opt;
    opt.width = 40;
    markdown_stream stream(opt);
    string body;
    for (int i = 0; i < 200; ++i) {
        body += "paragraph ";
        body += to_string(static_cast<int64_t>(i));
        body += "\n\n";
    }
    body += "```cpp\nint a = 1;\n\nint b = 2;\n";
    stream.append(body.view());
    stream.render(40);
    const size_t sealed = stream.sealed_bytes();
    EXPECT_LE(sealed, body.view().find(string_view{"```cpp"}));
}

TEST(TuiMarkdownStreamTest, ResetClearsState) {
    markdown_option opt;
    markdown_stream stream(opt);
    stream.append("hello\n\n");
    stream.render(20);
    stream.reset();
    EXPECT_EQ(stream.text().size(), 0u);
    EXPECT_EQ(stream.sealed_bytes(), 0u);
    EXPECT_EQ(stream.sealed_blocks(), 0u);
}


TEST(TuiMarkdownMathTest, InlineFormulaRendersUnicode) {
    const string text = render_plaintext("value $x^2$ here", 40, 3);
    EXPECT_TRUE(text.view().find(string_view{"x\xc2\xb2"}) != string_view::npos);
    EXPECT_TRUE(text.view().find(string_view{"$"}) == string_view::npos);
}

TEST(TuiMarkdownMathTest, UnsupportedFormulaKeepsSource) {
    const string text = render_plaintext("bad $\\unknowncmd{q}$ here", 40, 3);
    EXPECT_TRUE(text.view().find(string_view{"\\unknowncmd"}) != string_view::npos);
}

TEST(TuiMarkdownMathTest, BlockFormulaStacksFraction) {
    const string text = render_plaintext("$$\n\\frac{a}{b}\n$$\n", 40, 6);
    EXPECT_TRUE(text.view().find(string_view{"\xe2\x94\x80"}) != string_view::npos);
    EXPECT_TRUE(text.view().find(string_view{"a"}) != string_view::npos);
    EXPECT_TRUE(text.view().find(string_view{"b"}) != string_view::npos);
}

TEST(TuiMarkdownMathTest, BlockFormulaWithLimitsUsesThreeRows) {
    const string text = render_plaintext("$$\n\\sum_{i=0}^{n} x_i\n$$\n", 40, 8);
    EXPECT_TRUE(text.view().find(string_view{"\xe2\x88\x91"}) != string_view::npos);
}

TEST(TuiMarkdownMathTest, MathCanBeDisabled) {
    markdown_option opt;
    opt.parse_math = false;
    const element tree = markdown("value $x^2$ here", opt);
    const vector<text_span>& spans = tree.children()[0].spans();
    string flat;
    for (const auto& span: spans) {
        flat += span.content;
    }
    EXPECT_TRUE(flat.view().find(string_view{"$x^2$"}) != string_view::npos);
}

TEST(TuiMarkdownMathTest, DollarsInsideCodeFenceStayLiteral) {
    const string source = "```sh\necho $$\nprice=$5\n```\n";
    const string text = render_plaintext(source, 40, 8);
    EXPECT_TRUE(text.view().find(string_view{"echo $$"}) != string_view::npos);
    EXPECT_TRUE(text.view().find(string_view{"price=$5"}) != string_view::npos);
}


TEST(TuiMarkdownMermaidTest, RendersFlowchartFence) {
    const string source = "```mermaid\n"
                          "graph TD\n"
                          "A[Start] --> B[End]\n"
                          "```\n";
    const string text = render_plaintext(source, 60, 12);
    EXPECT_TRUE(contains(text, "Start"));
    EXPECT_TRUE(contains(text, "End"));
    EXPECT_TRUE(contains(text, "\xe2\x94\x80"));
}

TEST(TuiMarkdownMermaidTest, UnsupportedDiagramFallsBackToCode) {
    const string source = "```mermaid\n"
                          "notadiagram\n"
                          "  a --> b\n"
                          "```\n";
    const string text = render_plaintext(source, 60, 12);
    EXPECT_TRUE(contains(text, "notadiagram"));
}
