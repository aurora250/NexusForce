#include <NeForce/core/file/md/markdown_builder.hpp>
#include <NeForce/core/file/md/markdown_parser.hpp>
#include <NeForce/core/string/to_string.hpp>
#include <NeForce/core/utility/packages.hpp>
#include <gtest/gtest.h>
using namespace neforce;

namespace {
    string dump(const md_inlines& inlines);

    string dump_node(const md_inline& node) {
        switch (node.kind) {
            case md_inline_kind::text: {
                return "t(\"" + escape((node.text)) + "\")";
            }
            case md_inline_kind::strong: {
                return "strong(" + dump(node.children) + ")";
            }
            case md_inline_kind::emphasis: {
                return "em(" + dump(node.children) + ")";
            }
            case md_inline_kind::strikethrough: {
                return "del(" + dump(node.children) + ")";
            }
            case md_inline_kind::code: {
                return "code(\"" + escape((node.text)) + "\")";
            }
            case md_inline_kind::link: {
                string result = "link(" + dump(node.children) + "->" + (node.destination);
                if (!node.title.empty()) {
                    result += " \"" + (node.title) + "\"";
                }
                return result + ")";
            }
            case md_inline_kind::image: {
                return "img(\"" + escape((node.text)) + "\"->" + (node.destination) + ")";
            }
            case md_inline_kind::autolink: {
                return "auto(" + (node.text) + ")";
            }
            case md_inline_kind::html: {
                return "html(" + (node.text) + ")";
            }
            case md_inline_kind::line_break: {
                return "br()";
            }
        }
        return "?";
    }

    string dump(const md_inlines& inlines) {
        string result;
        for (size_t i = 0; i < inlines.size(); ++i) {
            if (i > 0) {
                result += " ";
            }
            result += dump_node(inlines[i]);
        }
        return result;
    }

    string align_char(const md_align align) {
        switch (align) {
            case md_align::left: {
                return "l";
            }
            case md_align::center: {
                return "c";
            }
            case md_align::right: {
                return "r";
            }
            default: {
                return "-";
            }
        }
    }

    void dump_block(const md_block& block, string& out, int depth);

    string dump_cells(const vector<md_inlines>& cells) {
        string result;
        for (size_t i = 0; i < cells.size(); ++i) {
            if (i > 0) {
                result += " | ";
            }
            result += dump(cells[i]);
        }
        return result;
    }

    void dump_block(const md_block& block, string& out, const int depth) {
        const string pad(static_cast<size_t>(depth) * 2, ' ');
        switch (block.kind) {
            case md_block_kind::paragraph: {
                out += pad + "para [" + dump(block.inlines) + "]\n";
                break;
            }
            case md_block_kind::heading: {
                out += pad + "heading" + to_string(static_cast<int64_t>(block.level)) +
                       (block.setext ? " setext" : "") + " [" + dump(block.inlines) + "]\n";
                break;
            }
            case md_block_kind::code_block: {
                out += pad + "code(fenced=" + to_string(static_cast<int64_t>(block.fenced)) +
                       ",lang=" + (block.language) + ",info=" + (block.info) + ") [" + escape((block.literal)) + "]\n";
                break;
            }
            case md_block_kind::html_block: {
                out += pad + "html" + to_string(static_cast<int64_t>(block.html_type)) + " [" +
                       escape((block.literal)) + "]\n";
                break;
            }
            case md_block_kind::thematic_break: {
                out += pad + "hr\n";
                break;
            }
            case md_block_kind::block_quote: {
                out += pad + "quote\n";
                for (const auto& child: block.children) {
                    dump_block(child, out, depth + 1);
                }
                break;
            }
            case md_block_kind::list: {
                out += pad + "list(ordered=" + to_string(static_cast<int64_t>(block.ordered)) +
                       ",start=" + to_string(static_cast<int64_t>(block.start)) +
                       ",tight=" + to_string(static_cast<int64_t>(block.tight)) + ")\n";
                for (const auto& item: block.children) {
                    dump_block(item, out, depth + 1);
                }
                break;
            }
            case md_block_kind::list_item: {
                out += pad + "item(n=" + to_string(static_cast<int64_t>(block.number)) +
                       ",task=" + to_string(static_cast<int64_t>(block.task)) +
                       ",checked=" + to_string(static_cast<int64_t>(block.checked)) + ")\n";
                for (const auto& child: block.children) {
                    dump_block(child, out, depth + 1);
                }
                break;
            }
            case md_block_kind::table: {
                out += pad + "table align=";
                for (const auto& align: block.alignments) {
                    out += align_char(align);
                }
                out += " header=[" + dump_cells(block.header) + "]\n";
                for (const auto& row: block.rows) {
                    out += pad + "  row=[" + dump_cells(row) + "]\n";
                }
                break;
            }
        }
    }

    string dump(const markdown_document& document) {
        string out;
        for (const auto& block: document.blocks) {
            dump_block(block, out, 0);
        }
        return out;
    }

    markdown_document parse_doc(const string_view text, const markdown_parser::option opt = markdown_parser::option{}) {
        return markdown_parser(string(text), opt).parse();
    }

    md_inlines parse_inl(const string_view text, const markdown_parser::option opt = markdown_parser::option{},
                         const markdown_document* document = nullptr) {
        return markdown_parser(string(text), opt).parse_inline(document);
    }

    string dump_parse(const string& text) { return dump(parse_doc(text.view())); }
} // namespace


TEST(MdParseTest, EmptyAndBlankInputYieldEmptyDocument) {
    EXPECT_TRUE(md_empty(parse_doc("")));
    EXPECT_TRUE(md_empty(parse_doc("\n\n   \n")));
}

TEST(MdParseTest, AtxHeadingsWithOptionalClosingSequence) {
    EXPECT_EQ(dump_parse("# one"), "heading1 [t(\"one\")]\n");
    EXPECT_EQ(dump_parse("### three"), "heading3 [t(\"three\")]\n");
    EXPECT_EQ(dump_parse("####### seven"), "para [t(\"####### seven\")]\n");
    EXPECT_EQ(dump_parse("#nospace"), "para [t(\"#nospace\")]\n");
    EXPECT_EQ(dump_parse("## closed ##"), "heading2 [t(\"closed\")]\n");
}

TEST(MdParseTest, SetextHeadings) {
    EXPECT_EQ(dump_parse("Title\n====="), "heading1 setext [t(\"Title\")]\n");
    EXPECT_EQ(dump_parse("Title\n-----"), "heading2 setext [t(\"Title\")]\n");
    EXPECT_EQ(dump_parse("---\n\n- item"),
              "hr\nlist(ordered=0,start=1,tight=1)\n  item(n=0,task=0,checked=0)\n    para [t(\"item\")]\n");
}

TEST(MdParseTest, SetextHeadingsCanBeDisabled) {
    markdown_parser::option opt;
    opt.setext_headings = false;
    const auto document = parse_doc("Title\n=====", opt);

    // With Setext disabled the underline is ordinary paragraph text.
    ASSERT_EQ(document.blocks.size(), 1U);
    EXPECT_EQ(document.blocks[0].kind, md_block_kind::paragraph);
    EXPECT_EQ((md_plain_text(document.blocks[0])), "Title =====\n");
}

TEST(MdParseTest, ParagraphSoftAndHardBreaks) {
    EXPECT_EQ(dump_parse("first line\nsecond line"), "para [t(\"first line second line\")]\n");
    EXPECT_EQ(dump_parse("a  \nb"), "para [t(\"a\") br() t(\"b\")]\n");
    EXPECT_EQ(dump_parse("a\\\nb"), "para [t(\"a\") br() t(\"b\")]\n");
}

TEST(MdParseTest, FencedCodeKeepsInfoAndLanguage) {
    const auto document = parse_doc("```cpp extra\nint a = 1;\n```");

    ASSERT_EQ(document.blocks.size(), 1U);
    const auto& code = document.blocks[0];
    EXPECT_EQ(code.kind, md_block_kind::code_block);
    EXPECT_TRUE(code.fenced);
    EXPECT_EQ((code.info), "cpp extra");
    EXPECT_EQ((code.language), "cpp");
    EXPECT_EQ((code.literal), "int a = 1;\n");
}

TEST(MdParseTest, FenceRequiresMatchingLengthAndChar) {
    const auto document = parse_doc("````\n```\nstill code\n````");

    ASSERT_EQ(document.blocks.size(), 1U);
    EXPECT_EQ((document.blocks[0].literal), "```\nstill code\n");
}

TEST(MdParseTest, UnterminatedFenceConsumesRestOfDocument) {
    const auto document = parse_doc("```\nbody\nmore");

    ASSERT_EQ(document.blocks.size(), 1U);
    EXPECT_EQ((document.blocks[0].literal), "body\nmore\n");
}

TEST(MdParseTest, IndentedCodeBlockIsRecognized) {
    const auto document = parse_doc("    line one\n    line two");

    ASSERT_EQ(document.blocks.size(), 1U);
    EXPECT_EQ(document.blocks[0].kind, md_block_kind::code_block);
    EXPECT_FALSE(document.blocks[0].fenced);
    EXPECT_EQ((document.blocks[0].literal), "line one\nline two\n");
}

TEST(MdParseTest, HtmlBlockTypes) {
    const auto comment = parse_doc("<!-- c -->\ntext");
    ASSERT_GE(comment.blocks.size(), 1U);
    EXPECT_EQ(comment.blocks[0].kind, md_block_kind::html_block);
    EXPECT_EQ(comment.blocks[0].html_type, 2);

    const auto pre = parse_doc("<pre>\ncode\n</pre>\n\nafter");
    ASSERT_EQ(pre.blocks.size(), 2U);
    EXPECT_EQ(pre.blocks[0].html_type, 1);
    EXPECT_EQ(pre.blocks[1].kind, md_block_kind::paragraph);

    const auto block_tag = parse_doc("<div>\nraw\n</div>");
    ASSERT_EQ(block_tag.blocks.size(), 1U);
    EXPECT_EQ(block_tag.blocks[0].html_type, 6);

    const auto custom = parse_doc("<custom-tag attr=\"1\">\n\npara");
    ASSERT_EQ(custom.blocks.size(), 2U);
    EXPECT_EQ(custom.blocks[0].html_type, 7);
}

TEST(MdParseTest, HtmlBlockProcessingInstructionAndDeclaration) {
    const auto pi = parse_doc("<?php echo 1; ?>\n\ntext");
    ASSERT_EQ(pi.blocks.size(), 2U);
    EXPECT_EQ(pi.blocks[0].html_type, 3);

    const auto declaration = parse_doc("<!DOCTYPE html>\n\ntext");
    ASSERT_EQ(declaration.blocks.size(), 2U);
    EXPECT_EQ(declaration.blocks[0].html_type, 4);

    const auto cdata = parse_doc("<![CDATA[ raw ]]>\n\ntext");
    ASSERT_EQ(cdata.blocks.size(), 2U);
    EXPECT_EQ(cdata.blocks[0].html_type, 5);
}

TEST(MdParseTest, HtmlBlockCanBeDisabled) {
    markdown_parser::option opt;
    opt.html_blocks = false;
    const auto document = parse_doc("<div>\nx\n</div>", opt);

    ASSERT_EQ(document.blocks.size(), 1U);
    EXPECT_EQ(document.blocks[0].kind, md_block_kind::paragraph);
}

TEST(MdParseTest, BlockQuoteNestsAndHoldsBlocks) {
    const auto document = parse_doc("> quoted\n> > nested");

    ASSERT_EQ(document.blocks.size(), 1U);
    EXPECT_EQ(document.blocks[0].kind, md_block_kind::block_quote);
    ASSERT_EQ(document.blocks[0].children.size(), 2U);
    EXPECT_EQ(document.blocks[0].children[0].kind, md_block_kind::paragraph);
    EXPECT_EQ(document.blocks[0].children[1].kind, md_block_kind::block_quote);
}

TEST(MdParseTest, BlockQuoteTakesLazyContinuation) {
    const auto document = parse_doc("> first\nsecond");

    ASSERT_EQ(document.blocks.size(), 1U);
    ASSERT_EQ(document.blocks[0].children.size(), 1U);
    EXPECT_EQ((md_plain_text(document.blocks[0].children[0])), "first second\n");
}

TEST(MdParseTest, BlockQuoteCanContainList) {
    const auto document = parse_doc("> - a\n> - b");

    ASSERT_EQ(document.blocks.size(), 1U);
    ASSERT_EQ(document.blocks[0].children.size(), 1U);
    EXPECT_EQ(document.blocks[0].children[0].kind, md_block_kind::list);
    EXPECT_EQ(document.blocks[0].children[0].children.size(), 2U);
}

TEST(MdParseTest, UnorderedListWithNestedList) {
    const auto document = parse_doc("- a\n  - b\n- c");

    ASSERT_EQ(document.blocks.size(), 1U);
    const auto& list = document.blocks[0];
    ASSERT_EQ(list.children.size(), 2U);
    ASSERT_EQ(list.children[0].children.size(), 2U);
    EXPECT_EQ(list.children[0].children[1].kind, md_block_kind::list);
    EXPECT_EQ(list.children[0].children[1].children.size(), 1U);
}

TEST(MdParseTest, OrderedListKeepsStartAndDelimiter) {
    const auto document = parse_doc("3) three\n4) four");

    ASSERT_EQ(document.blocks.size(), 1U);
    EXPECT_TRUE(document.blocks[0].ordered);
    EXPECT_EQ(document.blocks[0].start, 3);
    EXPECT_EQ(document.blocks[0].delimiter, ')');
    ASSERT_EQ(document.blocks[0].children.size(), 2U);
    EXPECT_EQ(document.blocks[0].children[0].number, 3);
    EXPECT_EQ(document.blocks[0].children[1].number, 4);
}

TEST(MdParseTest, ListTypeSwitchStartsNewList) {
    const auto document = parse_doc("- a\n\n1. b");

    ASSERT_EQ(document.blocks.size(), 2U);
    EXPECT_FALSE(document.blocks[0].ordered);
    EXPECT_TRUE(document.blocks[1].ordered);
}

TEST(MdParseTest, ListItemCanContainMultipleBlocks) {
    const auto document = parse_doc("- first\n\n  second paragraph\n\n      indented code");

    ASSERT_EQ(document.blocks.size(), 1U);
    const auto& item = document.blocks[0].children[0];
    ASSERT_EQ(item.children.size(), 3U);
    EXPECT_EQ(item.children[0].kind, md_block_kind::paragraph);
    EXPECT_EQ(item.children[1].kind, md_block_kind::paragraph);
    EXPECT_EQ(item.children[2].kind, md_block_kind::code_block);
    EXPECT_FALSE(document.blocks[0].tight);
}

TEST(MdParseTest, ListItemCanContainQuoteAndCode) {
    const auto document = parse_doc("- item\n\n  > quoted\n\n  ```\n  code\n  ```");

    ASSERT_EQ(document.blocks.size(), 1U);
    const auto& item = document.blocks[0].children[0];
    ASSERT_GE(item.children.size(), 3U);
    EXPECT_EQ(item.children[0].kind, md_block_kind::paragraph);
    EXPECT_EQ(item.children[1].kind, md_block_kind::block_quote);
    EXPECT_EQ(item.children[2].kind, md_block_kind::code_block);
}

TEST(MdParseTest, TightVersusLooseList) {
    const auto tight = parse_doc("- a\n- b");
    ASSERT_EQ(tight.blocks.size(), 1U);
    EXPECT_TRUE(tight.blocks[0].tight);

    const auto loose = parse_doc("- a\n\n- b");
    ASSERT_EQ(loose.blocks.size(), 1U);
    EXPECT_FALSE(loose.blocks[0].tight);
}

TEST(MdParseTest, TaskListItemsAreFlagged) {
    const auto document = parse_doc("- [ ] todo\n- [x] done");

    ASSERT_EQ(document.blocks.size(), 1U);
    ASSERT_EQ(document.blocks[0].children.size(), 2U);
    EXPECT_TRUE(document.blocks[0].children[0].task);
    EXPECT_FALSE(document.blocks[0].children[0].checked);
    EXPECT_TRUE(document.blocks[0].children[1].checked);
    EXPECT_EQ((md_plain_text(document.blocks[0].children[1])), "done\n");
}

TEST(MdParseTest, TildeFenceIsSupported) {
    const auto document = parse_doc("~~~py\nx = 1\n~~~");

    ASSERT_EQ(document.blocks.size(), 1U);
    EXPECT_TRUE(document.blocks[0].fenced);
    EXPECT_EQ(document.blocks[0].fence_char, '~');
    EXPECT_EQ((document.blocks[0].language), "py");
}

TEST(MdParseTest, EscapedPipeStaysInsideTableCell) {
    const auto document = parse_doc("| a \\| b | c |\n| --- | --- |\n| 1 | 2 |");

    ASSERT_EQ(document.blocks.size(), 1U);
    ASSERT_EQ(document.blocks[0].header.size(), 2U);
    EXPECT_EQ((md_plain_text(document.blocks[0].header[0])), "a | b");
}

TEST(MdParseTest, PipeTableWithAlignment) {
    const auto document = parse_doc("| a | b | c |\n| :-- | :-: | --: |\n| 1 | 2 | 3 |");

    ASSERT_EQ(document.blocks.size(), 1U);
    const auto& table = document.blocks[0];
    EXPECT_EQ(table.kind, md_block_kind::table);
    ASSERT_EQ(table.alignments.size(), 3U);
    EXPECT_EQ(table.alignments[0], md_align::left);
    EXPECT_EQ(table.alignments[1], md_align::center);
    EXPECT_EQ(table.alignments[2], md_align::right);
    EXPECT_EQ((md_plain_text(table.header[1])), "b");
    EXPECT_EQ((md_plain_text(table.rows[0][2])), "3");
}

TEST(MdParseTest, TableRequiresMatchingDelimiterCount) {
    const auto document = parse_doc("| a | b |\n| --- |\n| 1 | 2 |");

    ASSERT_FALSE(document.blocks.empty());
    EXPECT_EQ(document.blocks[0].kind, md_block_kind::paragraph);
}

TEST(MdParseTest, LinkReferenceDefinitionsAreCollected) {
    const auto document = parse_doc("[ref]: https://example.com \"Title\"\n[other]: <https://x.io>\n\nsee [ref]");

    ASSERT_EQ(document.references.size(), 2U);
    EXPECT_EQ((document.references[0].label), "ref");
    EXPECT_EQ((document.references[0].destination), "https://example.com");
    EXPECT_EQ((document.references[0].title), "Title");
    EXPECT_EQ((document.references[1].destination), "https://x.io");
}

TEST(MdParseTest, ReferenceLinksResolveForward) {
    const auto document = parse_doc("see [text][ref] and [ref] and [ref][]\n\n[ref]: /x");

    ASSERT_FALSE(document.blocks.empty());
    EXPECT_EQ(dump(document.blocks[0].inlines),
              "t(\"see \") link(t(\"text\")->/x) t(\" and \") link(t(\"ref\")->/x) t(\" and \") link(t(\"ref\")->/x)");
}

TEST(MdParseTest, ReferenceDefinitionAcceptsMultiLineForm) {
    const auto document = parse_doc("[a]:\n  /a\n  \"Title a\"\n\n[text][a]\n");

    ASSERT_EQ(document.references.size(), 1U);
    EXPECT_EQ((document.references[0].destination), "/a");
    EXPECT_EQ((document.references[0].title), "Title a");
    EXPECT_EQ(dump(document.blocks[0].inlines), "link(t(\"text\")->/a \"Title a\")");
}

TEST(MdParseTest, MissingReferenceStaysLiteral) {
    const auto document = parse_doc("[nope] and [nope][ref]");

    ASSERT_EQ(document.blocks.size(), 1U);
    EXPECT_EQ(dump(document.blocks[0].inlines), "t(\"[nope] and [nope][ref]\")");
}

TEST(MdParseTest, ReferenceLookupIgnoresCaseAndSpacing) {
    const auto document = parse_doc("[Foo  Bar]: /x\n\n[y]: /y");

    ASSERT_EQ(document.references.size(), 2U);
    EXPECT_NE(md_find_reference(document, string_view("foo bar")), nullptr);
}

TEST(MdParseTest, InlineEmphasisForms) {
    EXPECT_EQ(dump(markdown_parser(string(string_view("a **b** c"))).parse_inline()),
              "t(\"a \") strong(t(\"b\")) t(\" c\")");
    EXPECT_EQ(dump(markdown_parser(string(string_view("a *b* c"))).parse_inline()), "t(\"a \") em(t(\"b\")) t(\" c\")");
    EXPECT_EQ(dump(markdown_parser(string(string_view("a __b__ c"))).parse_inline()),
              "t(\"a \") strong(t(\"b\")) t(\" c\")");
    EXPECT_EQ(dump(markdown_parser(string(string_view("a _b_ c"))).parse_inline()), "t(\"a \") em(t(\"b\")) t(\" c\")");
    EXPECT_EQ(dump(markdown_parser(string(string_view("***b***"))).parse_inline()), "em(strong(t(\"b\")))");
}

TEST(MdParseTest, UnderscoreDoesNotEmphasiseInsideWord) {
    EXPECT_EQ(dump(markdown_parser(string(string_view("snake_case_name"))).parse_inline()), "t(\"snake_case_name\")");
}

TEST(MdParseTest, StrikethroughAndItsOption) {
    EXPECT_EQ(dump(markdown_parser(string(string_view("a ~~b~~ c"))).parse_inline()),
              "t(\"a \") del(t(\"b\")) t(\" c\")");

    markdown_parser::option opt;
    opt.strikethrough = false;
    EXPECT_EQ(dump(markdown_parser(string(string_view("~~a~~")), opt).parse_inline()), "t(\"~~a~~\")");
}

TEST(MdParseTest, CodeSpansSupportBacktickRuns) {
    EXPECT_EQ(dump(markdown_parser(string(string_view("a `b` c"))).parse_inline()), "t(\"a \") code(\"b\") t(\" c\")");
    EXPECT_EQ(dump(markdown_parser(string(string_view("``a ` b``"))).parse_inline()), "code(\"a ` b\")");
    // One leading and one trailing space are stripped when both are present.
    EXPECT_EQ(dump(markdown_parser(string(string_view("a ` b `"))).parse_inline()), "t(\"a \") code(\"b\")");
}

TEST(MdParseTest, InlineLinksAndImages) {
    EXPECT_EQ(dump(parse_inl(string_view("[label](https://x.io \"title\")"))),
              "link(t(\"label\")->https://x.io \"title\")");
    EXPECT_EQ(dump(parse_inl(string_view("![alt](a.png)"))), "img(\"alt\"->a.png)");
    EXPECT_EQ(dump(parse_inl(string_view("[a [b] c](u)"))), "link(t(\"a [b] c\")->u)");
}

TEST(MdParseTest, AutolinksAndInlineHtml) {
    EXPECT_EQ(dump(markdown_parser(string(string_view("<https://x.io>"))).parse_inline()), "auto(https://x.io)");
    EXPECT_EQ(dump(markdown_parser(string(string_view("<user@example.com>"))).parse_inline()),
              "auto(user@example.com)");
    EXPECT_EQ(dump(markdown_parser(string(string_view("a <b>c</b> d"))).parse_inline()),
              "t(\"a \") html(<b>) t(\"c\") html(</b>) t(\" d\")");
    EXPECT_EQ(dump(markdown_parser(string(string_view("a <!-- c --> b"))).parse_inline()),
              "t(\"a \") html(<!-- c -->) t(\" b\")");
}

TEST(MdParseTest, EntitiesAreDecoded) {
    EXPECT_EQ(dump(markdown_parser(string(string_view("a &amp; b"))).parse_inline()), "t(\"a & b\")");
    EXPECT_EQ(dump(markdown_parser(string(string_view("&#65;&#x42;"))).parse_inline()), "t(\"AB\")");
    EXPECT_EQ(dump(markdown_parser(string(string_view("&unknown;"))).parse_inline()), "t(\"&unknown;\")");

    markdown_parser::option opt;
    opt.entities = false;
    EXPECT_EQ(dump(markdown_parser(string(string_view("&amp;")), opt).parse_inline()), "t(\"&amp;\")");
}

TEST(MdParseTest, BackslashEscapes) {
    EXPECT_EQ(dump(markdown_parser(string(string_view("\\*not em\\*"))).parse_inline()), "t(\"*not em*\")");
    EXPECT_EQ(dump(markdown_parser(string(string_view("\\`not code\\`"))).parse_inline()), "t(\"`not code`\")");
}

TEST(MdParseTest, UnclosedMarkersStayLiteral) {
    EXPECT_EQ(dump(markdown_parser(string(string_view("a ** b"))).parse_inline()), "t(\"a ** b\")");
    EXPECT_EQ(dump(markdown_parser(string(string_view("[x]("))).parse_inline()), "t(\"[x](\")");
}


TEST(MdBuildTest, RoundTripsEveryBlockStructure) {
    const string source = "# Title\n"
                          "\n"
                          "Setext\n"
                          "------\n"
                          "\n"
                          "Some **bold**, *italic*, `code`, ~~gone~~, a [link](https://x.io \"t\") and\n"
                          "<https://auto.io> plus <!-- html -->.\n"
                          "\n"
                          "> quoted line\n"
                          ">\n"
                          "> - nested item\n"
                          ">   - deeper\n"
                          "\n"
                          "1. one\n"
                          "2. two\n"
                          "\n"
                          "   second paragraph of two\n"
                          "\n"
                          "- [x] done\n"
                          "- [ ] open\n"
                          "\n"
                          "```cpp\n"
                          "int main() { return 0; }\n"
                          "```\n"
                          "\n"
                          "    indented code\n"
                          "\n"
                          "| h1 | h2 |\n"
                          "| :-- | --: |\n"
                          "| a | b |\n"
                          "\n"
                          "<div>\n"
                          "raw html\n"
                          "</div>\n"
                          "\n"
                          "---\n"
                          "\n"
                          "see [ref]\n"
                          "\n"
                          "[ref]: https://example.com \"Ref Title\"\n";

    const auto document = parse_doc(source.view());
    const string build = (markdown_builder::dump(document));
    const auto reparsed = parse_doc(build.view());

    EXPECT_EQ(dump(document), dump(reparsed)) << "built:\n" << build.data();
}

TEST(MdBuildTest, EscapesTextSoItReparsesIdentically) {
    md_inlines inlines;
    md_inline node;
    node.kind = md_inline_kind::text;
    node.text = "# not a heading *not em* [not link] | pipe & amp";
    inlines.push_back(node);

    const string build = markdown_builder::dump_inline(inlines);
    const auto reparsed = parse_inl(build.view());

    ASSERT_EQ(reparsed.size(), 1U);
    EXPECT_EQ(reparsed[0].kind, md_inline_kind::text);
    EXPECT_EQ((reparsed[0].text), (node.text));
}

TEST(MdBuildTest, BlockStartMarkersAreEscapedInsideParagraphs) {
    md_inlines inlines;
    md_inline node;
    node.kind = md_inline_kind::text;
    node.text = "- looks like a list";
    inlines.push_back(node);

    md_block paragraph;
    paragraph.kind = md_block_kind::paragraph;
    paragraph.inlines = inlines;

    markdown_document document;
    document.blocks.push_back(paragraph);

    const string build = (markdown_builder::dump(document));
    const auto reparsed = parse_doc(build.view());

    ASSERT_EQ(reparsed.blocks.size(), 1U);
    EXPECT_EQ(reparsed.blocks[0].kind, md_block_kind::paragraph);
    EXPECT_EQ((md_plain_text(reparsed.blocks[0])), "- looks like a list\n");
}

TEST(MdBuildTest, LineStartMarkersAreEscapedExactlyOnce) {
    for (const char* sample: {"# heading-like", "*em-like", "_em-like", "|table-like", "- list-like", "+ list-like",
                              "> quote-like", "= underline-like", "1. ordered-like", "1) ordered-like", "plain text"}) {
        md_inlines inlines;
        md_inline node;
        node.kind = md_inline_kind::text;
        node.text = sample;
        inlines.push_back(node);

        md_block paragraph;
        paragraph.kind = md_block_kind::paragraph;
        paragraph.inlines = inlines;
        markdown_document document;
        document.blocks.push_back(paragraph);

        const string text = document.to_string();
        const auto reparsed = parse_doc(text.view());

        ASSERT_EQ(reparsed.blocks.size(), 1U) << sample << " -> " << text.view().data();
        EXPECT_EQ(reparsed.blocks[0].kind, md_block_kind::paragraph) << text.view().data();
        EXPECT_EQ(md_plain_text(reparsed.blocks[0]), string(sample) + "\n") << text.view().data();
    }
}

TEST(MdBuildTest, CodeFenceGrowsToCoverBackticks) {
    const auto document = parse_doc("```\ncode with ``` inside\n```");
    const string build = (markdown_builder::dump(document));

    EXPECT_NE(build.find("````"), string::npos);
    EXPECT_EQ(dump(parse_doc(build.view())), dump(document));
}

TEST(MdBuildTest, ReferencesAreEmitted) {
    const auto document = parse_doc("[a]: /a \"T\"\n\nsee [a]");
    const string build = (markdown_builder::dump(document));

    EXPECT_NE(build.find("[a]: /a \"T\""), string::npos) << build.data();
    EXPECT_EQ(dump(parse_doc(build.view())), dump(document));
}

TEST(MdBuildTest, LooseListKeepsBlankLines) {
    const auto document = parse_doc("- a\n\n- b");
    const string build = (markdown_builder::dump(document));

    EXPECT_NE(build.find("\n\n"), string::npos) << build.data();
    EXPECT_EQ(dump(parse_doc(build.view())), dump(document));
}


TEST(MdValueTest, PlainTextFlattensStructure) {
    const auto document = parse_doc("# T\n\n- a **b**\n");

    ASSERT_EQ(document.blocks.size(), 2U);
    EXPECT_EQ((md_plain_text(document.blocks[0].inlines)), "T");
    EXPECT_EQ((md_plain_text(document.blocks[1].children[0])), "a b\n");
}

TEST(MdValueTest, PlainTextKeepsTableCells) {
    const auto document = parse_doc("| a | b |\n| - | - |\n| 1 | 2 |\n");

    ASSERT_EQ(document.blocks.size(), 1U);
    EXPECT_EQ((md_plain_text(document.blocks[0])), "a\tb\n1\t2\n");
}

TEST(MdValueTest, PlainTextSkipsHtmlBlockAsText) {
    const auto document = parse_doc("<div>\nx\n</div>\n\ntext");

    ASSERT_EQ(document.blocks.size(), 2U);
    EXPECT_EQ((md_plain_text(document.blocks[1])), "text\n");
}

TEST(MdApiTest, ParserClassExposesOptions) {
    markdown_parser parser(string("- item"));
    EXPECT_TRUE(parser.options().task_lists);

    markdown_parser::option opt;
    opt.tables = false;
    parser.set_options(opt);
    EXPECT_FALSE(parser.options().tables);
    EXPECT_EQ(parser.options().task_lists, true);

    const auto document = parser.parse();
    ASSERT_EQ(document.blocks.size(), 1U);
    EXPECT_EQ(document.blocks[0].kind, md_block_kind::list);
}

TEST(MdApiTest, DocumentSerialisesItself) {
    const auto document = parse_doc("# title\n\n- a");
    const string text = document.to_string();

    EXPECT_NE(text.view().find("# title"_sv), string_view::npos);
    EXPECT_EQ(dump(parse_doc(text.view())), dump(document));
}

TEST(MdApiTest, BuilderAccumulatesBlocksAndReferences) {
    markdown_builder builder;

    EXPECT_TRUE(builder.empty());
    EXPECT_EQ(builder.block_count(), 0U);

    markdown_document references;
    md_reference entry;
    entry.label = "ref";
    entry.destination = "https://example.com";
    entry.title = "Example";
    references.references.push_back(entry);

    builder.reference("ref", "https://example.com", "Example");
    builder.append_inline(markdown_parser(string("[text][ref]")).parse_inline(&references));
    builder.append(parse_doc("- a\n- b").blocks[0]);

    EXPECT_FALSE(builder.empty());
    EXPECT_EQ(builder.block_count(), 2U);
    EXPECT_EQ(builder.document().references.size(), 1U);

    const string text = builder.build();
    EXPECT_NE(text.view().find("[ref]: https://example.com \"Example\""_sv), string_view::npos);

    const auto reparsed = parse_doc(text.view());
    EXPECT_EQ(reparsed.blocks.size(), 2U);
    EXPECT_EQ(reparsed.references.size(), 1U);
    EXPECT_EQ(dump(reparsed.blocks[0].inlines), "link(t(\"text\")->https://example.com \"Example\")");

    builder.clear();
    EXPECT_TRUE(builder.empty());
}

TEST(MdApiTest, BuilderHonoursBulletAndListIndent) {
    const auto document = parse_doc("- a\n  - b");

    markdown_builder::option opt;
    opt.bullet = "* ";
    opt.list_indent = 4;
    const string text = markdown_builder::dump(document, opt);

    EXPECT_NE(text.view().find("* a"_sv), string_view::npos) << text.view().data();
    EXPECT_NE(text.view().find("    * b"_sv), string_view::npos) << text.view().data();
    EXPECT_EQ(dump(parse_doc(text.view())), dump(document));
}
