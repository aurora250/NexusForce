#include <NeForce/core/async/io_context.hpp>
#include <NeForce/core/async/strand.hpp>
#include <NeForce/tui/component/text_input.hpp>
#include <NeForce/tui/component/approve_prompt.hpp>
#include <NeForce/tui/component/composer.hpp>
#include <NeForce/tui/component/log_view.hpp>
#include <NeForce/tui/component/select_list.hpp>
#include <NeForce/tui/component/tool_card.hpp>
#include <NeForce/tui/component/status_indicator.hpp>
#include <NeForce/tui/component/text_area.hpp>
#include <NeForce/tui/dom/code_block.hpp>
#include <NeForce/tui/dom/diff.hpp>
#include <NeForce/tui/dom/element.hpp>
#include <NeForce/tui/dom/markdown.hpp>
#include <NeForce/tui/dom/layout.hpp>
#include <NeForce/tui/dom/style.hpp>
#include <NeForce/tui/headless.hpp>
#include <NeForce/tui/renderer.hpp>
#include <NeForce/tui/screen.hpp>
#include "tui_snapshot.hpp"
#include <gtest/gtest.h>
using namespace neforce;
using namespace neforce::tui;
using namespace neforce::tui::components;

namespace {
    struct fixed_root : component<> {
        element content = element::empty();

        element render() override { return content; }

        template <typename T>
        state<T>& make_state(T initial) {
            return this->create_state<T>(_NEFORCE move(initial));
        }

        void bind(strand& s, io_context& c) {
            strand_ = &s;
            ctx_ = &c;
        }
    };

    text_span span_of(const string& content, tui::style st = {}) { return text_span{content, _NEFORCE move(st)}; }

    struct component_host : component<> {
        element render() override {
            if (child_count() == 0) {
                return element::empty();
            }
            return child_at(0)->render();
        }

        template <typename T>
        state<T>& make_state(T initial) {
            return this->create_state<T>(_NEFORCE move(initial));
        }
    };

    tui::style color_style(const _NEFORCE color& c) {
        tui::style st;
        st.fg = c;
        return st;
    }
} // namespace


TEST(TuiRichTextTest, SpansCarryTheirOwnStyle) {
    tui::style bold_style;
    bold_style.bold = true;

    const element el = element::rich_text({span_of("a"), span_of("b", bold_style), span_of("c")});
    ASSERT_EQ(el.kind(), element::kind::rich_text);
    ASSERT_EQ(el.spans().size(), 3U);
    EXPECT_EQ(el.spans()[1].content, "b");
    EXPECT_TRUE(el.spans()[1].style.bold.value());
    EXPECT_FALSE(el.spans()[0].style.bold.has_value());
}

TEST(TuiRichTextTest, LayoutMeasuresConcatenatedWidthAndWraps) {
    const element el = element::rich_text({span_of("abc"), span_of(" def")});

    const auto wide = compute_layout(el, 40, 5);
    ASSERT_EQ(wide.size(), 1U);
    EXPECT_EQ(wide[0].w, 7);
    EXPECT_EQ(wide[0].h, 1);

    const auto narrow = compute_layout(el, 3, 5);
    ASSERT_EQ(narrow.size(), 1U);
    EXPECT_EQ(narrow[0].h, 2);
}

TEST(TuiRichTextTest, RendersMixedStylesIntoDistinctCells) {
    const element tree = element::rich_text({span_of("ERR", color_style(_NEFORCE color::red())), span_of(" ok")},
                                            style{}, style::wrap_mode::word);

    screen scr(10, 1);
    renderer r(scr, dark_theme, 10, 1);
    r.render(tree, compute_layout(tree, 10, 1), false);

    EXPECT_EQ(scr.to_plaintext(), "ERR ok");
    EXPECT_EQ(scr.fast_cell_at(0, 0).foreground, _NEFORCE color::red());
    EXPECT_NE(scr.fast_cell_at(4, 0).foreground, _NEFORCE color::red());
}

TEST(TuiRichTextTest, BaseStyleIsMergedUnderSpanStyle) {
    tui::style base;
    base.italic = true;
    tui::style span;
    span.bold = true;

    const element tree = element::rich_text({span_of("x", span)}, base);
    screen scr(2, 1);
    renderer r(scr, dark_theme, 2, 1);
    r.render(tree, compute_layout(tree, 2, 1), false);

    EXPECT_TRUE(scr.fast_cell_at(0, 0).bold);
    EXPECT_TRUE(scr.fast_cell_at(0, 0).italic);
}

TEST(TuiVirtualListTest, OnlyVisibleItemsAreBuilt) {
    size_t built = 0;
    const element tree = element::virtual_list(
            100, 1,
            [&built](const size_t index) {
                ++built;
                return element::text("item " + to_string(static_cast<int64_t>(index)));
            },
            10);

    const auto layout = compute_layout(tree, 12, 5);
    ASSERT_EQ(layout.size(), 1U);
    EXPECT_EQ(layout[0].w, 12);
    EXPECT_EQ(layout[0].h, 5);

    screen scr(12, 5);
    renderer r(scr, dark_theme, 12, 5);
    r.render(tree, layout, false);

    EXPECT_LE(built, 7U);
    EXPECT_EQ(scr.to_plaintext(), "item 10\nitem 11\nitem 12\nitem 13\nitem 14");
}

TEST(TuiVirtualListTest, LargeTranscriptSnapshotsWindow) {
    fixed_root holder;
    holder.content = element::virtual_list(
            1000, 1, [](const size_t index) { return element::text("line " + to_string(static_cast<int64_t>(index))); },
            997);

    headless app(&holder, 12, 3);
    app.render();
    NEFORCE_EXPECT_SNAPSHOT("virtual_list_tail", app.plaintext().view().data());
}

TEST(TuiColorDepthTest, Ansi16UsesBasicSgrCodes) {
    screen scr(2, 1);
    scr.set_color_depth(color_depth::ansi16);
    scr.fast_cell_at(0, 0).foreground = _NEFORCE color::red();
    scr.fast_cell_at(0, 0).character = "x";

    screen prev(2, 1);
    prev.set_color_depth(color_depth::ansi16);
    const string out = scr.to_string(prev);

    EXPECT_TRUE(out.view().find("\033[31m"_sv) != string_view::npos);
}

TEST(TuiColorDepthTest, Ansi256UsesPaletteSgr) {
    screen scr(2, 1);
    scr.fast_cell_at(0, 0).foreground = _NEFORCE color::red();
    scr.fast_cell_at(0, 0).character = "x";

    screen prev(2, 1);
    const string out = scr.to_string(prev);

    EXPECT_TRUE(out.view().find("\033[38;5;"_sv) != string_view::npos);
}

TEST(TuiColorDepthTest, TruecolorEmitsTwentyFourBitSgr) {
    screen scr(2, 1);
    scr.set_color_depth(color_depth::truecolor);
    scr.fast_cell_at(0, 0).foreground = _NEFORCE color{1, 2, 3};
    scr.fast_cell_at(0, 0).character = "x";

    screen prev(2, 1);
    prev.set_color_depth(color_depth::truecolor);
    const string out = scr.to_string(prev);

    EXPECT_TRUE(out.view().find("\033[38;2;1;2;3m"_sv) != string_view::npos);
}

TEST(TuiHeadlessTest, RendersComponentTreeToText) {
    fixed_root holder;
    holder.content = element::vbox({element::text("hello"), element::text("world")});

    headless app(&holder, 10, 3);
    app.render();

    EXPECT_EQ(app.plaintext(), "hello\nworld");
    EXPECT_EQ(app.width(), 10);
    EXPECT_EQ(app.height(), 3);
}

TEST(TuiHeadlessTest, FirstFrameClearsAndUsesSynchronizedOutput) {
    fixed_root holder;
    holder.content = element::text("hi");

    headless app(&holder, 4, 2);
    EXPECT_TRUE(app.ansi().view().find("\033[2J"_sv) != string_view::npos);
    EXPECT_TRUE(app.ansi().view().find("\033[?2026h"_sv) != string_view::npos);
    EXPECT_TRUE(app.ansi().view().find("\033[?2026l"_sv) != string_view::npos);
}

TEST(TuiHeadlessTest, SynchronizedOutputCanBeDisabled) {
    fixed_root holder;
    holder.content = element::text("hi");

    headless app(&holder, 4, 2);
    app.pipeline().set_synchronized_output(false);
    app.render();

    EXPECT_TRUE(app.ansi().view().find("\033[?2026h"_sv) == string_view::npos);
}

TEST(TuiHeadlessTest, ResizeRepaintsWholeScreen) {
    fixed_root holder;
    holder.content = element::text("hi");

    headless app(&holder, 4, 2);
    app.render();

    app.resize(8, 3);
    app.render();
    EXPECT_TRUE(app.ansi().view().find("\033[2J"_sv) != string_view::npos);
}

TEST(TuiHeadlessTest, InjectedKeysDriveTheComponentTree) {
    io_context ctx;
    strand s{ctx};

    fixed_root holder;
    holder.bind(s, ctx);
    auto& text = holder.make_state<string>(string());

    text_input_option opt;
    opt.text = &text;
    auto child = text_input(opt);
    holder.add_child(move(child));

    headless app(&holder, 12, 3);
    app.render();

    app.feed("\t");
    app.tick();
    app.feed("ab");
    app.tick();

    EXPECT_EQ(text.value(), "ab");
}

TEST(TuiHeadlessTest, PasteIsDeliveredAsOneEvent) {
    io_context ctx;
    strand s{ctx};

    fixed_root holder;
    holder.bind(s, ctx);
    auto& text = holder.make_state<string>(string());

    text_input_option opt;
    opt.text = &text;
    auto child = text_input(opt);
    holder.add_child(move(child));

    headless app(&holder, 12, 3);
    app.render();
    app.feed("\t");
    app.tick();
    app.feed("\033[200~line1\nline2\033[201~");
    app.tick();

    EXPECT_EQ(text.value(), "line1\nline2");
}

TEST(TuiMarkdownSnapshotTest, RendersShowcaseDocument) {
    const string source = "# Release notes\n"
                          "\n"
                          "This release adds **markdown** support with `inline code` and *emphasis*.\n"
                          "\n"
                          "- list item one\n"
                          "- list item with **bold**\n"
                          "\n"
                          "1. first step\n"
                          "2. second step\n"
                          "\n"
                          "> quoted guidance\n"
                          "\n"
                          "```cpp\n"
                          "int answer = 42; // comment\n"
                          "```\n";

    fixed_root holder;
    markdown_option opt;
    opt.width = 40;
    holder.content = markdown(source.view(), opt);

    headless app(&holder, 40, 20);
    app.render();
    NEFORCE_EXPECT_SNAPSHOT("markdown_showcase", app.plaintext());
}

TEST(TuiMarkdownSnapshotTest, RendersTableNestedAndTaskList) {
    const string source = "## Plan\n"
                          "\n"
                          "| step | owner | state |\n"
                          "| :--- | :---: | ---: |\n"
                          "| parse | al | done |\n"
                          "| render | me | wip |\n"
                          "\n"
                          "- top level\n"
                          "  - nested one\n"
                          "  - nested two\n"
                          "- [x] done item\n"
                          "- [ ] open item\n"
                          "\n"
                          "See [the docs](https://x.io) for `details`.\n";

    fixed_root holder;
    markdown_option opt;
    opt.width = 44;
    holder.content = markdown(source.view(), opt);

    headless app(&holder, 44, 18);
    app.render();
    NEFORCE_EXPECT_SNAPSHOT("markdown_table", app.plaintext());
}

TEST(TuiMarkdownTest, InlineEmphasisProducesStyledSpans) {
    tui::style code_style;
    code_style.reverse = true;
    const auto spans = parse_inline_markdown("a **b** c `d` e"_sv, style{}, code_style);

    ASSERT_EQ(spans.size(), 5U);
    EXPECT_EQ(spans[0].content, "a ");
    EXPECT_EQ(spans[1].content, "b");
    EXPECT_EQ(spans[1].style.bold.value_or(false), true);
    EXPECT_EQ(spans[2].content, " c ");
    EXPECT_EQ(spans[3].content, "d");
    EXPECT_EQ(spans[3].style.reverse.value_or(false), true);
    EXPECT_EQ(spans[4].content, " e");
}

TEST(TuiMarkdownTest, UnclosedMarkersStayAsText) {
    const auto spans = parse_inline_markdown("a ** b"_sv, style{}, style{});

    ASSERT_EQ(spans.size(), 1U);
    EXPECT_EQ(spans[0].content, "a ** b");
}

TEST(TuiCodeBlockSnapshotTest, RendersGutterAndHighlighting) {
    code_block_option opt;
    opt.code = "int main() {\n"
               "    // entry point\n"
               "    return 0;\n"
               "}\n";
    opt.language = "cpp";

    fixed_root holder;
    holder.content = code_block(opt);

    headless app(&holder, 30, 6);
    app.render();
    NEFORCE_EXPECT_SNAPSHOT("code_block_cpp", app.plaintext());
}

TEST(TuiCodeBlockTest, GutterNumbersStartFromConfiguredLine) {
    code_block_option opt;
    opt.code = "a\nb";
    opt.start_line = 10;
    opt.background = false;
    opt.left_bar = false;

    const element el = code_block(opt);
    ASSERT_EQ(el.kind(), element::kind::vbox);
    ASSERT_EQ(el.children().size(), 2U);
    ASSERT_EQ(el.children()[0].kind(), element::kind::rich_text);
    ASSERT_FALSE(el.children()[0].spans().empty());
    EXPECT_EQ(el.children()[0].spans()[0].content, "10 ");
    ASSERT_FALSE(el.children()[1].spans().empty());
    EXPECT_EQ(el.children()[1].spans()[0].content, "11 ");
}

TEST(TuiCodeBlockTest, BackgroundAndLeftBarWrapTheRows) {
    code_block_option opt;
    opt.code = "a\nb";

    const element el = code_block(opt);
    ASSERT_EQ(el.kind(), element::kind::band);
    ASSERT_EQ(el.children().size(), 1U);

    const element& row = el.children()[0];
    ASSERT_EQ(row.kind(), element::kind::hbox);
    ASSERT_EQ(row.children().size(), 3U);
    EXPECT_EQ(row.children()[0].text(), "\xe2\x94\x82");
    EXPECT_EQ(row.children()[2].kind(), element::kind::vbox);
}

TEST(TuiCodeBlockTest, CollapseKeepsHeadAndTailWithSummary) {
    code_block_option opt;
    opt.code = "1\n2\n3\n4\n5\n6\n7\n8";
    opt.collapse_after = 4;
    opt.background = false;
    opt.left_bar = false;

    const element el = code_block(opt);
    ASSERT_EQ(el.kind(), element::kind::vbox);

    bool found_hint = false;
    for (const auto& row: el.children()) {
        if (row.kind() == element::kind::hbox && row.children().size() >= 2) {
            const string& text = row.children()[1].text();
            if (text.view().find("lines hidden"_sv) != string_view::npos) {
                found_hint = true;
            }
        } else if (row.text().view().find("lines hidden"_sv) != string_view::npos) {
            found_hint = true;
        }
    }
    EXPECT_TRUE(found_hint);
}

TEST(TuiDiffSnapshotTest, RendersUnifiedDiff) {
    const string patch = "diff --git a/sample.txt b/sample.txt\n"
                         "--- a/sample.txt\n"
                         "+++ b/sample.txt\n"
                         "@@ -1,3 +1,3 @@\n"
                         " context line\n"
                         "-old value\n"
                         "+new value\n"
                         " trailing context\n";

    fixed_root holder;
    diff_view_option opt;
    opt.width = 34;
    holder.content = diff_view(patch.view(), opt);

    headless app(&holder, 34, 8);
    app.render();
    NEFORCE_EXPECT_SNAPSHOT("diff_unified", app.plaintext());
}

TEST(TuiDiffTest, ParsesLineNumbersAndKinds) {
    const auto lines = parse_unified_diff("@@ -5,2 +5,3 @@\n ctx\n-gone\n+added\n+extra\n"_sv);

    ASSERT_EQ(lines.size(), 5U);
    EXPECT_EQ(lines[0].kind, diff_kind::hunk);
    EXPECT_EQ(lines[1].kind, diff_kind::context);
    EXPECT_EQ(lines[1].old_number, 5);
    EXPECT_EQ(lines[2].kind, diff_kind::removed);
    EXPECT_EQ(lines[2].old_number, 6);
    EXPECT_EQ(lines[3].kind, diff_kind::added);
    EXPECT_EQ(lines[3].new_number, 6);
    EXPECT_EQ(lines[4].new_number, 7);
}

TEST(TuiEditorSnapshotTest, RendersMultilineBufferWithSelection) {
    io_context ctx;
    strand s{ctx};
    component_host holder;
    auto& text = holder.make_state<string>(string("first line\nsecond line\nthird line"));

    text_area_option opt;
    opt.text = &text;
    opt.style.width = style::size_hint{style::size_hint::fixed, 20};
    opt.style.height = style::size_hint{style::size_hint::fixed, 3};

    auto view = make_unique<text_area_view>(opt);
    text_area_view* area = view.get();
    holder.add_child(move(view));

    headless app(&holder, 20, 3);
    app.render();
    area->set_has_focus(true);
    area->set_cursor(22);
    area->on_key(key_event{key_event::type::left, codepoint{}, key_modifier::shift});
    app.tick();

    NEFORCE_EXPECT_SNAPSHOT("editor_multiline", app.plaintext());
}

TEST(TuiSelectListSnapshotTest, RendersFilterAndSelection) {
    component_host holder;

    select_list_option opt;
    opt.items = {"/help", "/clear", "/compact", "/model", "/review"};
    opt.visible_rows = 4;
    opt.on_change = nullptr;

    auto view = make_unique<select_list_view>(opt);
    select_list_view* list = view.get();
    holder.add_child(move(view));

    headless app(&holder, 18, 5);
    app.render();
    list->on_key(key_event{key_event::type::printable, codepoint{static_cast<char32_t>('c')}, key_modifier::none});
    app.tick();

    NEFORCE_EXPECT_SNAPSHOT("select_list_filtered", app.plaintext());
}

TEST(TuiApprovePromptSnapshotTest, RendersDialogChoices) {
    component_host holder;

    approve_prompt_option opt;
    opt.title = "Run tool: bash";
    opt.message = "This command writes to disk.";
    opt.always_label = "Always";
    opt.width = 36;

    holder.add_child(make_unique<approve_prompt_view>(opt));

    headless app(&holder, 36, 8);
    app.render();
    NEFORCE_EXPECT_SNAPSHOT("approve_prompt", app.plaintext());
}

TEST(TuiStatusIndicatorSnapshotTest, RendersSpinnerLabelAndTimer) {
    component_host host;

    status_indicator_option opt;
    opt.label = "Thinking";
    opt.charset = 3;

    auto owned = make_unique<status_indicator_view>(opt);
    status_indicator_view* indicator = owned.get();
    host.add_child(move(owned));
    indicator->on_animation(1000);

    headless app(&host, 24, 1);
    app.render();
    NEFORCE_EXPECT_SNAPSHOT("status_indicator", app.plaintext());
}


TEST(TuiChatSnapshotTest, RendersTranscriptWithToolCardAndPendingHint) {
    io_context ctx;
    strand s{ctx};

    component_host host;
    log_view_option opt;
    opt.gap = 1;
    auto owned = make_unique<log_view>(opt);
    log_view* log = owned.get();
    host.add_child(move(owned));

    headless app(&host, 52, 16);
    app.render();

    const uint64_t first = log->append(log_role::user, "Add a **retry** helper to `net.hpp`");
    log->finish(first);

    const uint64_t second = log->append(log_role::assistant, "I will add `retry_async` with exponential backoff.\n\n"
                                                             "- jitter\n- cap at 30s\n\n```cpp\n"
                                                             "auto retry_async(task fn, int attempts = 3);\n```");
    log->finish(second);

    const uint64_t third = log->append(log_role::tool, "bash  \xe2\x80\xa2 exited 0\nCompiling...\n"
                                                       "2 warnings\nBuild finished in 3.4s");
    log->finish(third);
    app.tick();
    app.tick();
    app.tick();

    NEFORCE_EXPECT_SNAPSHOT("chat_transcript", app.plaintext());
}

TEST(TuiComposerSnapshotTest, ShowsCompletionPopupAndHintLine) {
    io_context ctx;
    strand s{ctx};

    component_host host;
    composer_option opt;
    opt.complete = [](const completion_request& request, vector<completion_item>& items) {
        struct entry {
            const char* name;
            const char* detail;
        };
        static const entry entries[] = {{"/help", "show commands"},
                                        {"/clear", "clear transcript"},
                                        {"/compact", "compact context"},
                                        {"/model", "switch model"}};
        for (const auto& item: entries) {
            const string label(item.name);
            if (request.query.empty() || label.view().find(request.query.view()) != string_view::npos) {
                completion_item entry_item;
                entry_item.label = label;
                entry_item.detail = string(item.detail);
                items.push_back(move(entry_item));
            }
        }
    };

    auto owned = make_unique<composer>(opt);
    host.add_child(move(owned));

    headless app(&host, 40, 9);
    app.render();
    app.feed("\t");
    app.tick();
    app.feed("/c");
    app.tick();

    NEFORCE_EXPECT_SNAPSHOT("composer_completions", app.plaintext());
}

TEST(TuiToolCardSnapshotTest, RendersRunningAndFinishedCards) {
    io_context ctx;
    strand s{ctx};

    struct stack : component<> {
        vector<component_base*> cards;

        void adopt(unique_ptr<component_base> child) {
            cards.push_back(child.get());
            add_child(move(child));
        }

        element render() override {
            elements parts;
            for (auto* card: cards) {
                parts.push_back(card->render());
            }
            return element::vbox(move(parts));
        }
    };

    stack host;
    tool_card_option running_opt;
    running_opt.title = "bash";
    running_opt.summary = "pytest -q";
    running_opt.status = tool_status::running;
    host.adopt(make_unique<tool_card>(running_opt));

    tool_card_option done_opt;
    done_opt.title = "edit";
    done_opt.summary = "src/net.hpp";
    done_opt.status = tool_status::success;
    done_opt.elapsed_ms = 2400;
    done_opt.collapsed = false;
    auto done = make_unique<tool_card>(done_opt);
    tool_card* done_card = done.get();
    host.adopt(move(done));

    auto failed = make_unique<tool_card>(tool_card_option{string("bash"), string("make test"), tool_status::failed,
                                                          int64_t{1400}, true, size_t{12}, style{}, style{}, style{}});
    tool_card* failed_card = failed.get();
    host.adopt(move(failed));

    headless app(&host, 48, 14);
    done_card->set_output(string("diff --git a/src/net.hpp\n+auto retry_async(...);"));
    failed_card->set_output(string("line1\nline2\nline3"));
    app.render();
    app.tick();

    NEFORCE_EXPECT_SNAPSHOT("tool_cards", app.plaintext());
}

TEST(TuiDiffGroupedSnapshotTest, GroupsByFileAndCollapses) {
    const string patch = "diff --git a/src/a.cpp b/src/a.cpp\n"
                         "--- a/src/a.cpp\n"
                         "+++ b/src/a.cpp\n"
                         "@@ -1,3 +1,4 @@\n"
                         " int main() {\n"
                         "-    return 1;\n"
                         "+    log();\n"
                         "+    return 0;\n"
                         " }\n"
                         "diff --git a/src/b.cpp b/src/b.cpp\n"
                         "--- a/src/b.cpp\n"
                         "+++ b/src/b.cpp\n"
                         "@@ -10,2 +10,2 @@\n"
                         "-old\n"
                         "+new\n";

    fixed_root holder;
    diff_view_option opt;
    opt.width = 40;
    holder.content = diff_view(patch.view(), opt);

    headless app(&holder, 40, 16);
    app.render();
    NEFORCE_EXPECT_SNAPSHOT("diff_grouped", app.plaintext());
}

TEST(TuiScreenDiffTest, RelativeCursorMoveAccountsForCursorColumn) {
    screen current(12, 1);
    screen previous(12, 1);
    previous.fast_cell_at(0, 0).character = "a";
    current.fast_cell_at(0, 0).character = "b";
    current.fast_cell_at(5, 0).character = "c";

    const string out = current.to_string(previous);
    // Cursor is at column 1 after "b"; reaching cell 5 needs four columns, not five.
    EXPECT_NE(out.find("\033[4C"), string::npos);
    EXPECT_EQ(out.find("\033[5C"), string::npos);
}

TEST(TuiScreenDiffTest, RelativeCursorMoveAccountsForWideGlyphRun) {
    screen current(12, 1);
    screen previous(12, 1);
    current.fast_cell_at(0, 0).character = "\xe4\xbd\xa0";
    current.fast_cell_at(1, 0).automerge = true;
    current.fast_cell_at(5, 0).character = "c";

    const string out = current.to_string(previous);
    // The wide glyph occupies columns 0-1, so reaching cell 5 needs three columns, not four.
    EXPECT_NE(out.find("\033[3C"), string::npos);
    EXPECT_EQ(out.find("\033[4C"), string::npos);
}
