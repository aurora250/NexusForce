#include <NeForce/core/async/io_context.hpp>
#include <NeForce/core/async/strand.hpp>
#include <NeForce/tui/application.hpp>
#include <NeForce/tui/component/approve_prompt.hpp>
#include <NeForce/tui/component/motion.hpp>
#include <NeForce/tui/component/select_list.hpp>
#include <NeForce/tui/component/composer.hpp>
#include <NeForce/tui/component/log_view.hpp>
#include <NeForce/tui/component/output_block.hpp>
#include <NeForce/tui/component/status_indicator.hpp>
#include <NeForce/tui/component/tool_card.hpp>
#include <NeForce/tui/keymap.hpp>
#include <NeForce/tui/recorder.hpp>
#include <NeForce/core/async/process_watcher.hpp>
#include <NeForce/tui/component/tool_runner.hpp>
#include <NeForce/tui/component/streaming_text.hpp>
#include <NeForce/tui/component/text_area.hpp>
#include <NeForce/tui/component/virtual_list.hpp>
#include <NeForce/tui/dom/element.hpp>
#include <NeForce/tui/dom/layout.hpp>
#include <NeForce/tui/dom/markdown.hpp>
#include <NeForce/tui/dom/text_layout.hpp>
#include <NeForce/tui/headless.hpp>
#include <NeForce/tui/renderer.hpp>
#include <NeForce/tui/screen.hpp>
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
    };

    struct host_root : component<> {
        element render() override {
            if (child_count() == 0) {
                return element::empty();
            }
            elements parts;
            for (size_t i = 0; i < child_count(); ++i) {
                parts.push_back(child_at(i)->render());
            }
            return element::vbox(move(parts));
        }

        template <typename T>
        state<T>& make_state(T initial) {
            return this->create_state<T>(_NEFORCE move(initial));
        }

        void bind(strand& s, io_context& c) {
            strand_ = &s;
            ctx_ = &c;
        }
    };

    key_event make_key(const key_event::type type, const key_modifier mods = key_modifier::none) {
        key_event e;
        e.key = type;
        e.mods = mods;
        return e;
    }

    key_event make_char(const char32_t ch, const key_modifier mods = key_modifier::none) {
        key_event e;
        e.key = key_event::type::printable;
        e.cp = codepoint{ch};
        e.mods = mods;
        return e;
    }

    string shell_short_echo_command() {
#ifdef NEFORCE_PLATFORM_WINDOWS
        return "echo life";
#else
        return "printf 'life\\n'";
#endif
    }

    string shell_long_running_command() {
#ifdef NEFORCE_PLATFORM_WINDOWS
        return "ping -n 6 127.0.0.1 >nul";
#else
        return "sleep 5";
#endif
    }

    string shell_two_echoes_exit_command() {
#ifdef NEFORCE_PLATFORM_WINDOWS
        return "echo alpha&echo beta&exit 7";
#else
        return "printf 'alpha'; printf 'beta'; exit 7";
#endif
    }

    string shell_tool_output_command() {
#ifdef NEFORCE_PLATFORM_WINDOWS
        return "echo tool-output";
#else
        return "printf 'tool-output\\n'; exit 0";
#endif
    }

    string compact_output(const string_view text) {
        string out;
        for (size_t i = 0; i < text.size(); ++i) {
            const char c = text[i];
            if (c != '\r' && c != '\n') {
                out.push_back(c);
            }
        }
        return out;
    }
} // namespace


TEST(TuiTextLayoutTest, WordWrapBreaksAtSpacesAndDropsThem) {
    const auto lay = layout_text("aaa bbb ccc"_sv, 7, style::wrap_mode::word);

    ASSERT_EQ(lay.lines.size(), 2U);
    EXPECT_EQ(lay.lines[0].begin, 0U);
    EXPECT_EQ(lay.lines[0].end, 7U);
    EXPECT_EQ(lay.lines[1].begin, 8U);
    EXPECT_EQ(lay.lines[1].end, 11U);
}

TEST(TuiTextLayoutTest, CharacterWrapBreaksAnywhere) {
    const auto lay = layout_text("abcdefgh"_sv, 3, style::wrap_mode::character);

    ASSERT_EQ(lay.lines.size(), 3U);
    EXPECT_EQ(lay.lines[0].end, 3U);
    EXPECT_EQ(lay.lines[1].begin, 3U);
    EXPECT_EQ(lay.lines[2].end, 8U);
}

TEST(TuiTextLayoutTest, NoneModeOnlyBreaksAtNewlines) {
    const auto lay = layout_text("abcdef\ngh"_sv, 2, style::wrap_mode::none);

    ASSERT_EQ(lay.lines.size(), 2U);
    EXPECT_EQ(lay.lines[0].end, 6U);
    EXPECT_EQ(lay.lines[1].begin, 7U);
}

TEST(TuiTextLayoutTest, WideCharactersCountTwoColumns) {
    const auto lay = layout_text("中文测试"_sv, 4, style::wrap_mode::character);

    ASSERT_EQ(lay.lines.size(), 2U);
    EXPECT_EQ(lay.lines[0].end, 6U);
    EXPECT_EQ(lay.lines[1].end, 12U);
}

TEST(TuiTextLayoutTest, CaretPositionMapsOffsetToRowAndColumn) {
    const auto lay = layout_text("aaa bbb ccc"_sv, 7, style::wrap_mode::word);

    int row = 0;
    int col = 0;
    caret_position(lay, "aaa bbb ccc"_sv, 0, row, col);
    EXPECT_EQ(row, 0);
    EXPECT_EQ(col, 0);

    caret_position(lay, "aaa bbb ccc"_sv, 3, row, col);
    EXPECT_EQ(row, 0);
    EXPECT_EQ(col, 3);

    caret_position(lay, "aaa bbb ccc"_sv, 8, row, col);
    EXPECT_EQ(row, 1);
    EXPECT_EQ(col, 0);

    caret_position(lay, "aaa bbb ccc"_sv, 11, row, col);
    EXPECT_EQ(row, 1);
    EXPECT_EQ(col, 3);
}

TEST(TuiPanelTest, PanelOccupiesItsOwnRectAndInsetsChild) {
    tui::style st;
    st.border = style::border::single;

    const element tree = element::panel(element::text("hi"), st);
    const auto layout = compute_layout(tree, 10, 5);

    ASSERT_EQ(layout.size(), 2U);
    EXPECT_EQ(layout[0].w, 10);
    EXPECT_EQ(layout[0].h, 5);
    EXPECT_EQ(layout[1].x, 1);
    EXPECT_EQ(layout[1].y, 1);
    EXPECT_EQ(layout[1].w, 2);
    EXPECT_EQ(layout[1].h, 1);

    screen scr(10, 5);
    renderer r(scr, dark_theme, 10, 5);
    r.render(tree, layout, false);
    EXPECT_EQ(scr.fast_cell_at(0, 0).character, "\xe2\x94\x8c");
    EXPECT_EQ(scr.fast_cell_at(1, 1).character, "h");
}


TEST(TuiVirtualListComponentTest, AppendingFollowsBottom) {
    io_context ctx;
    strand s{ctx};
    host_root holder;
    holder.bind(s, ctx);

    virtual_list_option opt;
    opt.item_count = 3;
    opt.item_height = 1;
    opt.render_item = [](const size_t index) { return element::text(to_string(static_cast<int64_t>(index))); };
    opt.style.height = style::size_hint{style::size_hint::fixed, 3};

    auto view = make_unique<virtual_list_view>(opt);
    virtual_list_view* list = view.get();
    holder.add_child(move(view));

    headless app(&holder, 10, 3);
    app.render();
    EXPECT_EQ(list->viewport_rows(), 3);
    EXPECT_EQ(list->scroll_offset(), 0);

    list->set_item_count(6);
    app.tick();
    EXPECT_EQ(list->scroll_offset(), 3);
    EXPECT_TRUE(list->at_bottom());
    EXPECT_EQ(app.frame().fast_cell_at(0, 0).character, "3");
    EXPECT_EQ(app.frame().fast_cell_at(0, 1).character, "4");
    EXPECT_EQ(app.frame().fast_cell_at(0, 2).character, "5");
}

TEST(TuiVirtualListComponentTest, ManualScrollStopsFollowing) {
    io_context ctx;
    strand s{ctx};
    host_root holder;
    holder.bind(s, ctx);

    virtual_list_option opt;
    opt.item_count = 5;
    opt.render_item = [](const size_t index) { return element::text(to_string(static_cast<int64_t>(index))); };
    opt.style.height = style::size_hint{style::size_hint::fixed, 2};

    auto view = make_unique<virtual_list_view>(opt);
    virtual_list_view* list = view.get();
    holder.add_child(move(view));

    headless app(&holder, 10, 2);
    app.render();
    EXPECT_EQ(list->scroll_offset(), 3);

    list->scroll_to(0);
    app.tick();
    EXPECT_FALSE(list->auto_follow());

    list->set_item_count(8);
    app.tick();
    EXPECT_EQ(list->scroll_offset(), 0);

    list->scroll_to_bottom();
    app.tick();
    EXPECT_TRUE(list->auto_follow());
    EXPECT_EQ(list->scroll_offset(), 6);
}

TEST(TuiVirtualListComponentTest, KeyboardScrollsByPage) {
    io_context ctx;
    strand s{ctx};
    host_root holder;
    holder.bind(s, ctx);

    virtual_list_option opt;
    opt.item_count = 20;
    opt.render_item = [](const size_t index) { return element::text(to_string(static_cast<int64_t>(index))); };
    opt.style.height = style::size_hint{style::size_hint::fixed, 4};

    auto view = make_unique<virtual_list_view>(opt);
    virtual_list_view* list = view.get();
    holder.add_child(move(view));

    headless app(&holder, 10, 4);
    app.render();
    list->set_has_focus(true);

    EXPECT_TRUE(list->on_key(make_key(key_event::type::home)));
    EXPECT_EQ(list->scroll_offset(), 0);
    EXPECT_TRUE(list->on_key(make_key(key_event::type::page_down)));
    EXPECT_EQ(list->scroll_offset(), 4);
    EXPECT_TRUE(list->on_key(make_key(key_event::type::down)));
    EXPECT_EQ(list->scroll_offset(), 5);
    EXPECT_TRUE(list->on_key(make_key(key_event::type::end)));
    EXPECT_EQ(list->scroll_offset(), 16);
}


TEST(TuiTextAreaTest, TypingInsertsAndEnterAddsNewline) {
    io_context ctx;
    strand s{ctx};
    host_root holder;
    holder.bind(s, ctx);

    auto& text = holder.make_state<string>(string());
    text_area_option opt;
    opt.text = &text;

    auto view = make_unique<text_area_view>(opt);
    text_area_view* area = view.get();
    holder.add_child(move(view));
    area->set_has_focus(true);

    EXPECT_TRUE(area->on_key(make_char(U'a')));
    EXPECT_TRUE(area->on_key(make_char(U'b')));
    EXPECT_TRUE(area->on_key(make_key(key_event::type::enter)));
    EXPECT_TRUE(area->on_key(make_char(U'c')));
    EXPECT_EQ(text.value(), "ab\nc");
    EXPECT_EQ(area->cursor(), 4U);
}

TEST(TuiTextAreaTest, BackspaceAndDeleteEditAroundCursor) {
    io_context ctx;
    strand s{ctx};
    host_root holder;
    holder.bind(s, ctx);

    auto& text = holder.make_state<string>(string("abc"));
    text_area_option opt;
    opt.text = &text;

    auto view = make_unique<text_area_view>(opt);
    text_area_view* area = view.get();
    holder.add_child(move(view));
    area->set_has_focus(true);

    area->set_cursor(3);
    EXPECT_TRUE(area->on_key(make_key(key_event::type::backspace)));
    EXPECT_EQ(text.value(), "ab");

    area->set_cursor(0);
    EXPECT_TRUE(area->on_key(make_key(key_event::type::delete_)));
    EXPECT_EQ(text.value(), "b");
}

TEST(TuiTextAreaTest, ShiftSelectionIsReplacedByTyping) {
    io_context ctx;
    strand s{ctx};
    host_root holder;
    holder.bind(s, ctx);

    auto& text = holder.make_state<string>(string("hello"));
    text_area_option opt;
    opt.text = &text;

    auto view = make_unique<text_area_view>(opt);
    text_area_view* area = view.get();
    holder.add_child(move(view));
    area->set_has_focus(true);

    area->set_cursor(5);
    EXPECT_TRUE(area->on_key(make_key(key_event::type::left, key_modifier::shift)));
    EXPECT_TRUE(area->on_key(make_key(key_event::type::left, key_modifier::shift)));
    EXPECT_TRUE(area->has_selection());
    EXPECT_EQ(area->selection_begin(), 3U);
    EXPECT_EQ(area->selection_end(), 5U);

    EXPECT_TRUE(area->on_key(make_char(U'p')));
    EXPECT_EQ(text.value(), "help");
}

TEST(TuiTextAreaTest, SelectAllThenTypeReplacesDocument) {
    io_context ctx;
    strand s{ctx};
    host_root holder;
    holder.bind(s, ctx);

    auto& text = holder.make_state<string>(string("one\ntwo"));
    text_area_option opt;
    opt.text = &text;

    auto view = make_unique<text_area_view>(opt);
    text_area_view* area = view.get();
    holder.add_child(move(view));
    area->set_has_focus(true);

    area->select_all();
    EXPECT_EQ(area->selection_begin(), 0U);
    EXPECT_EQ(area->selection_end(), 7U);

    EXPECT_TRUE(area->on_key(make_char(U'x')));
    EXPECT_EQ(text.value(), "x");
}

TEST(TuiTextAreaTest, UndoAndRedoRestoreSnapshots) {
    io_context ctx;
    strand s{ctx};
    host_root holder;
    holder.bind(s, ctx);

    auto& text = holder.make_state<string>(string());
    text_area_option opt;
    opt.text = &text;

    auto view = make_unique<text_area_view>(opt);
    text_area_view* area = view.get();
    holder.add_child(move(view));
    area->set_has_focus(true);

    area->on_key(make_char(U'a'));
    area->on_key(make_char(U'b'));
    EXPECT_EQ(text.value(), "ab");

    EXPECT_TRUE(area->undo());
    EXPECT_EQ(text.value(), "a");

    EXPECT_TRUE(area->redo());
    EXPECT_EQ(text.value(), "ab");
}

TEST(TuiTextAreaTest, HistoryNavigationWithCtrlPAndCtrlN) {
    io_context ctx;
    strand s{ctx};
    host_root holder;
    holder.bind(s, ctx);

    auto& text = holder.make_state<string>(string());
    text_area_option opt;
    opt.text = &text;

    auto view = make_unique<text_area_view>(opt);
    text_area_view* area = view.get();
    holder.add_child(move(view));
    area->set_has_focus(true);

    area->set_text(string("first"));
    area->submit();
    EXPECT_EQ(area->history_size(), 1U);

    area->set_text(string("second"));
    area->submit();
    EXPECT_EQ(area->history_size(), 2U);

    EXPECT_TRUE(area->on_key(make_char(U'p', key_modifier::ctrl)));
    EXPECT_EQ(text.value(), "second");
    EXPECT_TRUE(area->on_key(make_char(U'p', key_modifier::ctrl)));
    EXPECT_EQ(text.value(), "first");
}

TEST(TuiTextAreaTest, SubmitOnEnterInvokesCallback) {
    io_context ctx;
    strand s{ctx};
    host_root holder;
    holder.bind(s, ctx);

    auto& text = holder.make_state<string>(string("hi"));
    int submitted = 0;
    text_area_option opt;
    opt.text = &text;
    opt.submit_on_enter = true;
    opt.on_submit = [&submitted] { ++submitted; };

    auto view = make_unique<text_area_view>(opt);
    text_area_view* area = view.get();
    holder.add_child(move(view));
    area->set_has_focus(true);

    EXPECT_TRUE(area->on_key(make_key(key_event::type::enter)));
    EXPECT_EQ(submitted, 1);
    EXPECT_EQ(text.value(), "hi");
}

TEST(TuiTextAreaTest, ClipboardAnswerFromTerminalIsPasted) {
    host_root holder;
    auto& text = holder.make_state<string>(string("ab"));

    text_area_option opt;
    opt.text = &text;
    auto view = make_unique<text_area_view>(opt);
    text_area_view* area = view.get();
    holder.add_child(move(view));

    headless app(&holder, 20, 4);
    app.render();
    app.feed("\t");
    ASSERT_TRUE(area->has_focus());
    area->set_cursor(2);

    app.feed("\033]52;c;cGFzdGVk\a");
    EXPECT_EQ(text.value(), "abpasted");
}

TEST(TuiTextAreaTest, CtrlVRequestsTheClipboardInsteadOfInsertingV) {
    host_root holder;
    auto& text = holder.make_state<string>(string());

    text_area_option opt;
    opt.text = &text;
    auto view = make_unique<text_area_view>(opt);
    text_area_view* area = view.get();
    holder.add_child(move(view));

    headless app(&holder, 20, 4);
    app.render();
    area->set_has_focus(true);

    key_event ctrl_v;
    ctrl_v.key = key_event::type::printable;
    ctrl_v.cp = codepoint{static_cast<char32_t>('v')};
    ctrl_v.mods = key_modifier::ctrl;
    EXPECT_TRUE(area->on_key(ctrl_v));
    EXPECT_TRUE(text.value().empty());
}

TEST(TuiTextAreaTest, PasteInsertsWholeText) {
    io_context ctx;
    strand s{ctx};
    host_root holder;
    holder.bind(s, ctx);

    auto& text = holder.make_state<string>(string("ab"));
    text_area_option opt;
    opt.text = &text;

    auto view = make_unique<text_area_view>(opt);
    text_area_view* area = view.get();
    holder.add_child(move(view));
    area->set_has_focus(true);
    area->set_cursor(2);

    paste_event pe;
    pe.text = "X\nY";
    EXPECT_TRUE(area->on_paste(pe));
    EXPECT_EQ(text.value(), "abX\nY");
}

TEST(TuiTextAreaTest, CaretMovesByVisualLineOnWrappedText) {
    io_context ctx;
    strand s{ctx};
    host_root holder;
    holder.bind(s, ctx);

    auto& text = holder.make_state<string>(string("aaa bbb ccc"));
    text_area_option opt;
    opt.text = &text;
    opt.style.width = style::size_hint{style::size_hint::fixed, 7};
    opt.style.height = style::size_hint{style::size_hint::fixed, 2};

    auto view = make_unique<text_area_view>(opt);
    text_area_view* area = view.get();
    holder.add_child(move(view));

    headless app(&holder, 7, 2);
    app.render();
    area->set_has_focus(true);

    area->set_cursor(0);
    app.tick();
    EXPECT_TRUE(area->on_key(make_key(key_event::type::down)));
    EXPECT_EQ(area->cursor(), 8U);
}

TEST(TuiTextAreaTest, RenderShowsTextAndScrollsToCaret) {
    io_context ctx;
    strand s{ctx};
    host_root holder;
    holder.bind(s, ctx);

    auto& text = holder.make_state<string>(string("l1\nl2\nl3\nl4\nl5"));
    text_area_option opt;
    opt.text = &text;
    opt.style.height = style::size_hint{style::size_hint::fixed, 2};

    auto view = make_unique<text_area_view>(opt);
    text_area_view* area = view.get();
    holder.add_child(move(view));

    headless app(&holder, 6, 2);
    app.render();
    area->set_has_focus(true);
    area->set_cursor(text.value().size());
    app.tick();

    EXPECT_EQ(app.plaintext(), "l4\nl5");
}


TEST(TuiSelectListTest, FilterNarrowsMatches) {
    select_list_option opt;
    opt.items = {"alpha", "beta", "gamma"};
    opt.filterable = true;

    auto view = make_unique<select_list_view>(opt);
    select_list_view* list = view.get();

    EXPECT_EQ(list->matches().size(), 3U);

    EXPECT_TRUE(list->on_key(make_char(U'a')));
    EXPECT_EQ(list->filter(), "a");
    EXPECT_EQ(list->matches().size(), 3U);

    EXPECT_TRUE(list->on_key(make_char(U'm')));
    EXPECT_EQ(list->matches().size(), 1U);
    EXPECT_EQ(list->matches()[0], "gamma");
    EXPECT_EQ(list->selected_index(), 2);
}

TEST(TuiSelectListTest, MoveAndConfirmReportOriginalIndex) {
    select_list_option opt;
    opt.items = {"alpha", "beta", "gamma"};
    int picked = -1;
    opt.on_select = [&picked](const int index) { picked = index; };

    auto view = make_unique<select_list_view>(opt);
    select_list_view* list = view.get();

    EXPECT_TRUE(list->on_key(make_key(key_event::type::down)));
    EXPECT_EQ(list->selected_position(), 1);
    EXPECT_TRUE(list->on_key(make_key(key_event::type::enter)));
    EXPECT_EQ(picked, 1);

    EXPECT_TRUE(list->on_key(make_key(key_event::type::up)));
    EXPECT_EQ(list->selected_position(), 0);
    EXPECT_TRUE(list->on_key(make_key(key_event::type::up)));
    EXPECT_EQ(list->selected_position(), 2);
}

TEST(TuiSelectListTest, EscapeClearsFilterBeforeCancelling) {
    select_list_option opt;
    opt.items = {"alpha", "beta"};
    bool cancelled = false;
    opt.on_cancel = [&cancelled] { cancelled = true; };

    auto view = make_unique<select_list_view>(opt);
    select_list_view* list = view.get();

    list->on_key(make_char(U'b'));
    EXPECT_EQ(list->filter(), "b");

    EXPECT_TRUE(list->on_key(make_key(key_event::type::escape)));
    EXPECT_EQ(list->filter(), "");
    EXPECT_FALSE(cancelled);

    EXPECT_TRUE(list->on_key(make_key(key_event::type::escape)));
    EXPECT_TRUE(cancelled);
}

TEST(TuiSelectListTest, RendersSelectedRow) {
    io_context ctx;
    strand s{ctx};
    host_root holder;
    holder.bind(s, ctx);

    select_list_option opt;
    opt.items = {"one", "two"};
    opt.visible_rows = 2;
    opt.filterable = false;

    auto view = make_unique<select_list_view>(opt);
    holder.add_child(move(view));

    headless app(&holder, 8, 4);
    app.render();

    ASSERT_EQ(app.frame().fast_cell_at(0, 0).character, "o");
    EXPECT_TRUE(app.frame().fast_cell_at(0, 0).inverted);
    EXPECT_FALSE(app.frame().fast_cell_at(0, 1).inverted);
}

TEST(TuiSelectListTest, SelectionStaysVisibleWhenMovingBeyondViewport) {
    io_context ctx;
    strand s{ctx};
    host_root holder;
    holder.bind(s, ctx);

    select_list_option opt;
    opt.items = {"i0", "i1", "i2", "i3", "i4", "i5"};
    opt.visible_rows = 2;
    opt.filterable = false;

    auto view = make_unique<select_list_view>(opt);
    select_list_view* list = view.get();
    holder.add_child(move(view));

    headless app(&holder, 8, 2);
    app.render();
    EXPECT_EQ(app.frame().fast_cell_at(0, 0).character, "i");

    list->on_key(make_key(key_event::type::end));
    app.tick();
    EXPECT_EQ(list->selected_position(), 5);
    EXPECT_EQ(app.frame().fast_cell_at(0, 0).character, "i");
    EXPECT_EQ(app.frame().fast_cell_at(1, 0).character, "4");
    EXPECT_EQ(app.frame().fast_cell_at(0, 1).character, "i");
    EXPECT_EQ(app.frame().fast_cell_at(1, 1).character, "5");
    EXPECT_TRUE(app.frame().fast_cell_at(0, 1).inverted);
}

TEST(TuiSelectListTest, EmptyMatchShowsPlaceholder) {
    io_context ctx;
    strand s{ctx};
    host_root holder;
    holder.bind(s, ctx);

    select_list_option opt;
    opt.items = {"alpha"};
    opt.filterable = true;
    opt.empty_text = "none";

    auto view = make_unique<select_list_view>(opt);
    select_list_view* list = view.get();
    holder.add_child(move(view));

    headless app(&holder, 12, 3);
    app.render();
    list->on_key(make_char(U'z'));
    app.tick();

    EXPECT_TRUE(app.plaintext().view().find("none"_sv) != string_view::npos);
}


TEST(TuiApprovePromptTest, DefaultHighlightIsApproveAndEnterConfirms) {
    approve_prompt_option opt;
    opt.title = "Run command?";
    opt.message = "rm -rf build";

    auto view = make_unique<approve_prompt_view>(opt);
    approve_prompt_view* prompt = view.get();

    EXPECT_EQ(prompt->highlighted(), 0);
    EXPECT_TRUE(prompt->on_key(make_key(key_event::type::right)));
    EXPECT_EQ(prompt->highlighted(), 1);
    EXPECT_TRUE(prompt->on_key(make_key(key_event::type::enter)));
    EXPECT_EQ(prompt->result(), approve_result::rejected);
}

TEST(TuiApprovePromptTest, DigitKeysDecideDirectly) {
    approve_prompt_option opt;
    opt.always_label = "Always";

    auto view = make_unique<approve_prompt_view>(opt);
    approve_prompt_view* prompt = view.get();

    EXPECT_TRUE(prompt->on_key(make_char(U'3')));
    EXPECT_EQ(prompt->result(), approve_result::approved_always);

    EXPECT_TRUE(prompt->on_key(make_char(U'1')));
    EXPECT_EQ(prompt->result(), approve_result::approved);
}

TEST(TuiApprovePromptTest, EscapeRejectsAndResultStateUpdates) {
    io_context ctx;
    strand s{ctx};
    host_root holder;
    holder.bind(s, ctx);
    auto& result = holder.make_state<approve_result>(approve_result::pending);

    approve_prompt_option opt;
    opt.result = &result;

    auto view = make_unique<approve_prompt_view>(opt);
    approve_prompt_view* prompt = view.get();

    EXPECT_TRUE(prompt->on_key(make_key(key_event::type::escape)));
    EXPECT_EQ(result.value(), approve_result::rejected);
}

TEST(TuiApprovePromptTest, RendersPanelWithChoices) {
    io_context ctx;
    strand s{ctx};
    host_root holder;
    holder.bind(s, ctx);

    approve_prompt_option opt;
    opt.title = "Deploy?";
    opt.message = "production";
    opt.width = 24;

    holder.add_child(make_unique<approve_prompt_view>(opt));

    headless app(&holder, 24, 8);
    app.render();

    const string plain = app.plaintext();
    EXPECT_TRUE(plain.view().find("Deploy?"_sv) != string_view::npos);
    EXPECT_TRUE(plain.view().find("Allow"_sv) != string_view::npos);
    EXPECT_TRUE(plain.view().find("Deny"_sv) != string_view::npos);
}


TEST(TuiStatusIndicatorTest, InactiveRendersNothing) {
    status_indicator_option opt;
    opt.label = "thinking";
    opt.active = false;

    auto view = make_unique<status_indicator_view>(opt);
    EXPECT_EQ(view->render().kind(), element::kind::empty);
}

TEST(TuiStatusIndicatorTest, AnimationAdvancesFrameAndElapsed) {
    status_indicator_option opt;
    opt.label = "thinking";
    opt.frame_interval_ms = 100;

    auto view = make_unique<status_indicator_view>(opt);
    status_indicator_view* status = view.get();

    const string first = status->render().spans()[0].content;
    for (int i = 0; i < 5; ++i) {
        status->on_animation(100);
    }
    const string later = status->render().spans()[0].content;

    EXPECT_NE(first, later);
    EXPECT_EQ(status->elapsed_ms(), 500);

    status->restart();
    EXPECT_EQ(status->elapsed_ms(), 0);
}

TEST(TuiStatusIndicatorTest, RendersLabelAndElapsedTime) {
    status_indicator_option opt;
    opt.label = "thinking";
    opt.show_elapsed = true;

    auto view = make_unique<status_indicator_view>(opt);
    status_indicator_view* status = view.get();
    status->on_animation(1500);

    const element el = status->render();
    ASSERT_EQ(el.kind(), element::kind::rich_text);
    EXPECT_GE(el.spans().size(), 2U);
    EXPECT_EQ(el.spans()[1].content, " thinking");
    EXPECT_EQ(el.spans()[2].content, " 1.5s");
}


TEST(TuiStreamingTextTest, AppendIsCoalescedUntilCommit) {
    io_context ctx;
    strand s{ctx};
    host_root holder;
    holder.bind(s, ctx);

    auto view = make_unique<streaming_text_view>();
    streaming_text_view* stream = view.get();
    holder.add_child(move(view));

    stream->append("hello"_sv);
    EXPECT_TRUE(stream->pending());
    EXPECT_EQ(stream->text(), "");

    stream->append(" world"_sv);
    stream->commit();
    EXPECT_FALSE(stream->pending());
    EXPECT_EQ(stream->text(), "hello world");
}

TEST(TuiStreamingTextTest, AnimationCommitsPendingChunks) {
    io_context ctx;
    strand s{ctx};
    host_root holder;
    holder.bind(s, ctx);

    auto view = make_unique<streaming_text_view>();
    streaming_text_view* stream = view.get();
    holder.add_child(move(view));

    stream->append("tok"_sv);
    stream->on_animation(16);
    EXPECT_EQ(stream->text(), "tok");

    stream->clear();
    EXPECT_EQ(stream->text(), "");
}

TEST(TuiStreamingTextTest, RenderShowsCommittedText) {
    io_context ctx;
    strand s{ctx};
    host_root holder;
    holder.bind(s, ctx);

    auto view = make_unique<streaming_text_view>();
    streaming_text_view* stream = view.get();
    holder.add_child(move(view));

    headless app(&holder, 12, 2);
    stream->append("abc"_sv);
    stream->commit();
    app.tick();

    EXPECT_EQ(app.plaintext(), "abc");
}

TEST(TuiReconcilerTest, InvalidateForcesFullRepaint) {
    fixed_root holder;
    holder.content = element::text("hi");

    headless app(&holder, 6, 2);
    app.render();
    app.render();
    EXPECT_TRUE(app.ansi().view().find("\033[2J"_sv) == string_view::npos);

    app.pipeline().invalidate();
    app.render();
    EXPECT_TRUE(app.ansi().view().find("\033[2J"_sv) != string_view::npos);
}

TEST(TuiReconcilerTest, ColorDepthPropagatesToScreenOutput) {
    fixed_root holder;
    tui::style st;
    st.fg = _NEFORCE color{1, 2, 3};
    holder.content = element::text("x", st);

    headless app(&holder, 4, 1);
    app.pipeline().set_color_depth(color_depth::truecolor);
    app.render();
    EXPECT_EQ(app.pipeline().color_depth_mode(), color_depth::truecolor);
    EXPECT_TRUE(app.ansi().view().find("\033[38;2;1;2;3m"_sv) != string_view::npos);

    app.pipeline().set_color_depth(color_depth::ansi16);
    app.render();
    EXPECT_TRUE(app.ansi().view().find("\033[38;2;1;2;3m"_sv) == string_view::npos);
}

TEST(TuiApplicationTest, SuspendAndResumeAreSafeWithoutRunning) {
    application app;
    EXPECT_FALSE(app.suspended());
    EXPECT_NO_THROW(app.suspend());
    EXPECT_FALSE(app.suspended());
    EXPECT_NO_THROW(app.resume());
    EXPECT_FALSE(app.suspended());
}

TEST(TuiApplicationTest, RunWithoutComponentIsRejected) {
    application app;
    EXPECT_EQ(app.run(), 1);
}

TEST(TuiMarkdownCacheTest, ReusesElementTreeForSameTextAndWidth) {
    markdown_option opt;
    markdown_cache cache(opt, 4);

    const string source = "# title\n\n- a\n- b";
    const element first = cache.render(source.view(), 20);
    const element& second = cache.render(source.view(), 20);

    EXPECT_EQ(cache.hit_count(), 1U);
    EXPECT_EQ(cache.miss_count(), 1U);
    EXPECT_EQ(cache.size(), 1U);
    EXPECT_EQ(first.kind(), second.kind());
    EXPECT_EQ(first.children().size(), second.children().size());
}

TEST(TuiMarkdownCacheTest, WidthChangeInvalidatesEntry) {
    markdown_cache cache(markdown_option{}, 4);
    const string source = "paragraph with wrapping words here";

    cache.render(source.view(), 10);
    cache.render(source.view(), 20);

    EXPECT_EQ(cache.miss_count(), 2U);
    EXPECT_EQ(cache.hit_count(), 0U);
    EXPECT_EQ(cache.size(), 2U);

    // Same width again hits.
    cache.render(source.view(), 10);
    EXPECT_EQ(cache.hit_count(), 1U);
}

TEST(TuiMarkdownCacheTest, EvictsLeastRecentlyUsedEntry) {
    markdown_cache cache(markdown_option{}, 2);

    cache.render(string("one").view(), 10);
    cache.render(string("two").view(), 10);
    cache.render(string("one").view(), 10);   // one becomes most recent
    cache.render(string("three").view(), 10); // evicts two
    EXPECT_EQ(cache.size(), 2U);

    cache.render(string("one").view(), 10); // still cached
    EXPECT_EQ(cache.hit_count(), 2U);

    cache.render(string("two").view(), 10); // was evicted
    EXPECT_EQ(cache.miss_count(), 4U);
}

TEST(TuiMarkdownCacheTest, ClearDropsEntries) {
    markdown_cache cache(markdown_option{}, 4);
    cache.render(string("# t").view(), 10);
    EXPECT_EQ(cache.size(), 1U);

    cache.clear();
    EXPECT_EQ(cache.size(), 0U);
    cache.render(string("# t").view(), 10);
    EXPECT_EQ(cache.miss_count(), 2U);
}

TEST(TuiMarkdownCacheTest, CachedTreeRendersIdenticalOutput) {
    markdown_cache cache(markdown_option{}, 4);
    const string source = "## head\n\n| a | b |\n| - | - |\n| 1 | 2 |\n";

    const element cached = cache.render(source.view(), 24); // 深拷贝以独立校验布局
    const element direct = markdown(source.view(), [&] {
        markdown_option opt;
        opt.width = 24;
        return opt;
    }());

    const auto layout_cached = compute_layout(cached, 24, 8);
    const auto layout_direct = compute_layout(direct, 24, 8);
    ASSERT_EQ(layout_cached.size(), layout_direct.size());
    for (size_t i = 0; i < layout_cached.size(); ++i) {
        EXPECT_EQ(layout_cached[i].w, layout_direct[i].w) << i;
        EXPECT_EQ(layout_cached[i].h, layout_direct[i].h) << i;
    }
}


TEST(TuiBandTest, FillsBackgroundAndInsetsChild) {
    tui::style st;
    st.bg = _NEFORCE color{10, 20, 30};
    using padding = struct style::padding;
    st.padding = padding{0, 2, 0, 2};

    const element tree = element::band(element::text("hi"), st);
    const auto layout = compute_layout(tree, 10, 3);

    ASSERT_EQ(layout.size(), 2U);
    EXPECT_EQ(layout[0].w, 10);
    EXPECT_EQ(layout[1].x, 2);

    screen scr(10, 3);
    renderer r(scr, dark_theme, 10, 3);
    r.render(tree, layout, false);

    EXPECT_EQ(scr.fast_cell_at(0, 0).background, (_NEFORCE color{10, 20, 30}));
    EXPECT_EQ(scr.fast_cell_at(9, 2).background, (_NEFORCE color{10, 20, 30}));
    EXPECT_EQ(scr.fast_cell_at(2, 0).character, "h");
    EXPECT_EQ(scr.fast_cell_at(2, 0).background, (_NEFORCE color{10, 20, 30}));
}

TEST(TuiBandTest, TextInsideBandInheritsBandBackground) {
    tui::style band_style;
    band_style.bg = _NEFORCE color{40, 40, 40};

    const element tree = element::band(element::rich_text({text_span{"ab", style{}}}), band_style);
    const auto layout = compute_layout(tree, 6, 2);

    screen scr(6, 2);
    renderer r(scr, dark_theme, 6, 2);
    r.render(tree, layout, false);

    EXPECT_EQ(scr.fast_cell_at(0, 0).character, "a");
    EXPECT_EQ(scr.fast_cell_at(0, 0).background, (_NEFORCE color{40, 40, 40}));
}

TEST(TuiLinkTest, SpanLinkBecomesOscHyperlink) {
    text_span span;
    span.content = "docs";
    span.link = "https://example.com";

    const element tree = element::rich_text({span}, style{}, style::wrap_mode::none);
    const auto layout = compute_layout(tree, 8, 1);

    screen scr(8, 1);
    screen prev(8, 1);
    renderer r(scr, dark_theme, 8, 1);
    r.render(tree, layout, false);

    const uint8_t id = scr.fast_cell_at(0, 0).hyperlink;
    ASSERT_NE(id, 0);
    EXPECT_EQ(scr.hyperlink(id), "https://example.com"_sv);

    const string ansi = scr.to_string(prev);
    EXPECT_TRUE(ansi.view().find("\033]8;;https://example.com"_sv) != string_view::npos);
}


TEST(TuiVariableVirtualListTest, MeasuredHeightsDriveOffsets) {
    io_context ctx;
    strand s{ctx};
    host_root holder;
    holder.bind(s, ctx);

    virtual_list_option opt;
    opt.item_count = 3;
    opt.variable_height = true;
    opt.item_height = 1;
    opt.render_item = [](const size_t index) {
        elements rows;
        rows.push_back(element::text("head " + to_string(static_cast<int64_t>(index))));
        for (size_t i = 0; i < index; ++i) {
            rows.push_back(element::text("extra"));
        }
        return element::vbox(move(rows));
    };
    opt.style.height = style::size_hint{style::size_hint::fixed, 6};

    auto view = make_unique<virtual_list_view>(opt);
    virtual_list_view* list = view.get();
    holder.add_child(move(view));

    headless app(&holder, 10, 6);
    app.render();
    app.tick();
    app.tick();

    EXPECT_EQ(list->measured_height(0), 1);
    EXPECT_EQ(list->measured_height(1), 2);
    EXPECT_EQ(list->measured_height(2), 3);
    EXPECT_EQ(list->content_rows(), 6);
    EXPECT_EQ(app.frame().fast_cell_at(0, 0).character, "h");
    EXPECT_EQ(app.frame().fast_cell_at(0, 1).character, "h");
    EXPECT_EQ(app.frame().fast_cell_at(0, 3).character, "h");
}

TEST(TuiVariableVirtualListTest, WidthChangeInvalidatesMeasurements) {
    io_context ctx;
    strand s{ctx};
    host_root holder;
    holder.bind(s, ctx);

    virtual_list_option opt;
    opt.item_count = 2;
    opt.variable_height = true;
    opt.item_height = 1;
    opt.render_item = [](const size_t) {
        return element::text("a b c d e f g h i j", style{}, style::wrap_mode::word);
    };
    opt.style.height = style::size_hint{style::size_hint::fixed, 5};

    auto view = make_unique<virtual_list_view>(opt);
    virtual_list_view* list = view.get();
    holder.add_child(move(view));

    headless app(&holder, 20, 5);
    app.render();
    app.tick();
    const int wide = list->measured_height(0);
    EXPECT_EQ(wide, 1);

    app.resize(8, 5);
    app.tick();
    app.tick();
    EXPECT_GE(list->measured_height(0), 2);
}


TEST(TuiLogViewTest, AppendsMessagesAndReportsCount) {
    log_view log;
    log.setup();

    const uint64_t first = log.append(log_role::user, "hello");
    const uint64_t second = log.append(log_role::assistant, "hi there");

    EXPECT_EQ(log.message_count(), 2U);
    EXPECT_NE(first, second);
}

TEST(TuiLogViewTest, StreamingChunksAreCoalescedUntilCommit) {
    log_view log;
    log.setup();

    const uint64_t id = log.append(log_role::assistant, "a");
    log.append_chunk(id, "b");
    log.append_chunk(id, "c");

    // Nothing is visible to the renderer before commit, but the message list is intact.
    EXPECT_EQ(log.message_count(), 1U);

    log.commit();
    log.finish(id);

    const element tree = log.render();
    EXPECT_EQ(tree.kind(), element::kind::vbox);
}

TEST(TuiLogViewTest, AutoFollowKeepsNewMessagesVisible) {
    io_context ctx;
    strand s{ctx};
    host_root holder;
    holder.bind(s, ctx);

    log_view_option opt;
    opt.gap = 0;
    auto owned = make_unique<log_view>(opt);
    log_view* log = owned.get();
    holder.add_child(move(owned));

    headless app(&holder, 20, 3);
    app.render();

    for (int i = 0; i < 6; ++i) {
        const uint64_t id = log->append(log_role::user, "line" + to_string(static_cast<int64_t>(i)));
        log->finish(id);
    }
    // Variable heights need a measurement pass before the scroll metrics settle.
    app.tick();
    app.tick();
    app.tick();

    // Only items that became visible are measured, which is the point of the
    // virtual list; the tail therefore converges first.
    EXPECT_GE(log->list().measured_count(), 1U);
    EXPECT_TRUE(log->at_bottom());
    EXPECT_EQ(log->pending_messages(), 0U);
    EXPECT_TRUE(app.plaintext().view().find("line5"_sv) != string_view::npos);
}

TEST(TuiLogViewTest, ScrolledUpCountsPendingMessages) {
    io_context ctx;
    strand s{ctx};
    host_root holder;
    holder.bind(s, ctx);

    log_view_option opt;
    opt.gap = 0;
    auto owned = make_unique<log_view>(opt);
    log_view* log = owned.get();
    holder.add_child(move(owned));

    headless app(&holder, 20, 3);
    app.render();

    for (int i = 0; i < 5; ++i) {
        const uint64_t id = log->append(log_role::user, "line" + to_string(static_cast<int64_t>(i)));
        log->finish(id);
    }
    app.tick();
    app.tick();
    ASSERT_TRUE(log->at_bottom());

    log->list().scroll_to(0);
    app.tick();
    EXPECT_FALSE(log->at_bottom());

    const uint64_t id = log->append(log_role::assistant, "new");
    log->finish(id);
    app.tick();
    EXPECT_EQ(log->pending_messages(), 1U);

    log->scroll_to_bottom();
    app.tick();
    EXPECT_EQ(log->pending_messages(), 0U);
}

TEST(TuiLogViewTest, FinishedMessagesUseRenderCache) {
    io_context ctx;
    strand s{ctx};
    host_root holder;
    holder.bind(s, ctx);

    log_view_option opt;
    opt.gap = 0;
    auto owned = make_unique<log_view>(opt);
    log_view* log = owned.get();
    holder.add_child(move(owned));

    headless app(&holder, 24, 6);
    app.render();

    const uint64_t id = log->append(log_role::assistant, "**bold** text");
    log->finish(id);
    app.tick();
    app.tick();

    const size_t misses_before = log->cache().miss_count();
    app.render();
    app.render();

    EXPECT_GT(log->cache().miss_count(), 0U);
    EXPECT_EQ(log->cache().miss_count(), misses_before);
    EXPECT_GT(log->cache().hit_count(), 0U);
}

TEST(TuiLogViewTest, ClearDropsEverything) {
    log_view log;
    log.setup();
    log.append(log_role::user, "a");
    log.clear();

    EXPECT_EQ(log.message_count(), 0U);
    EXPECT_EQ(log.pending_messages(), 0U);
}


TEST(TuiComposerTest, RoundedBorderRendersSingleBoxAroundMultilineInput) {
    host_root holder;
    composer_option opt;
    opt.style.border = style::border::rounded;
    auto owned = make_unique<composer>(opt);
    composer* box = owned.get();
    holder.add_child(move(owned));
    box->setup();
    box->editor().set_text(string{"first\nsecond"});
    headless app(&holder, 40, 8);
    app.render();
    const string text = app.plaintext();
    EXPECT_TRUE(text.contains("\xe2\x95\xad"));
    EXPECT_TRUE(text.contains("\xe2\x95\xae"));
    EXPECT_TRUE(text.contains("\xe2\x95\xb0"));
    EXPECT_TRUE(text.contains("first"));
    EXPECT_TRUE(text.contains("second"));
    size_t corners = 0;
    size_t offset = 0;
    while ((offset = text.find("\xe2\x95\xad", offset)) != string::npos) {
        ++corners;
        offset += 3;
    }
    EXPECT_EQ(corners, 1u);
}

TEST(TuiComposerTest, EnterSubmitsAndClearsInput) {
    io_context ctx;
    strand s{ctx};
    host_root holder;
    holder.bind(s, ctx);

    string submitted;
    composer_option opt;
    opt.on_submit = [&submitted](const string& text) { submitted = text; };

    auto owned = make_unique<composer>(opt);
    composer* box = owned.get();
    holder.add_child(move(owned));
    box->setup();
    box->editor().set_text(string("hello"));
    box->editor().set_has_focus(true);

    EXPECT_TRUE(box->on_key(make_key(key_event::type::enter)));
    EXPECT_EQ(submitted, "hello");
    EXPECT_EQ(box->text(), "");
}

TEST(TuiComposerTest, SubmitOnEnterCanBeDisabledAtRuntime) {
    composer_option opt;
    string submitted;
    opt.on_submit = [&submitted](const string& text) { submitted = text; };
    auto owned = make_unique<composer>(opt);
    composer* box = owned.get();
    box->setup();
    box->editor().set_text(string("draft"));
    box->editor().set_has_focus(true);
    box->set_submit_on_enter(false);
    EXPECT_TRUE(box->on_key(make_key(key_event::type::enter)));
    EXPECT_TRUE(submitted.empty());
    key_event submit = make_key(key_event::type::enter);
    submit.mods = key_modifier::ctrl;
    EXPECT_TRUE(box->on_key(submit));
    EXPECT_EQ(submitted, string{"draft\n"});
}

TEST(TuiComposerTest, HistoryLimitCanBeChangedAtRuntime) {
    composer_option opt;
    auto owned = make_unique<composer>(opt);
    composer* box = owned.get();
    box->setup();
    box->editor().set_text(string("one"));
    box->submit();
    box->editor().set_text(string("two"));
    box->submit();
    box->editor().set_text(string("three"));
    box->submit();
    EXPECT_EQ(box->history_size(), 3U);
    box->set_history_limit(2);
    EXPECT_EQ(box->history_size(), 2U);
    box->set_history_limit(0);
    EXPECT_EQ(box->history_size(), 0U);
}

TEST(TuiComposerTest, ShiftEnterInsertsNewline) {
    io_context ctx;
    strand s{ctx};
    host_root holder;
    holder.bind(s, ctx);

    int submits = 0;
    composer_option opt;
    opt.on_submit = [&submits](const string&) { ++submits; };

    auto owned = make_unique<composer>(opt);
    composer* box = owned.get();
    holder.add_child(move(owned));
    box->setup();
    box->editor().set_has_focus(true);

    EXPECT_TRUE(box->on_key(make_char(U'a')));
    EXPECT_TRUE(box->on_key(make_key(key_event::type::enter, key_modifier::shift)));
    EXPECT_TRUE(box->on_key(make_char(U'b')));
    EXPECT_EQ(submits, 0);
    EXPECT_EQ(box->text(), "a\nb");
}

TEST(TuiComposerTest, SlashTriggerOpensAndAcceptsCompletion) {
    io_context ctx;
    strand s{ctx};
    host_root holder;
    holder.bind(s, ctx);

    composer_option opt;
    opt.complete = [](const completion_request& request, vector<completion_item>& items) {
        for (const auto* name: {"/help", "/clear", "/compact"}) {
            const string text(name);
            if (request.query.empty() || text.view().find(request.query.view()) != string_view::npos) {
                completion_item item;
                item.label = text;
                items.push_back(move(item));
            }
        }
    };

    auto owned = make_unique<composer>(opt);
    composer* box = owned.get();
    holder.add_child(move(owned));
    box->setup();
    box->editor().set_has_focus(true);

    EXPECT_TRUE(box->on_key(make_char(U'/')));
    EXPECT_TRUE(box->completion_open());

    EXPECT_TRUE(box->on_key(make_char(U'c')));
    EXPECT_TRUE(box->on_key(make_char(U'l')));
    EXPECT_TRUE(box->completion_open());

    EXPECT_TRUE(box->on_key(make_key(key_event::type::tab)));
    EXPECT_FALSE(box->completion_open());
    EXPECT_EQ(box->text(), "/clear");
}

TEST(TuiComposerTest, EscapeClosesPopupBeforeBubbling) {
    io_context ctx;
    strand s{ctx};
    host_root holder;
    holder.bind(s, ctx);

    composer_option opt;
    opt.complete = [](const completion_request&, vector<completion_item>& items) {
        completion_item item;
        item.label = "/help";
        items.push_back(move(item));
    };

    auto owned = make_unique<composer>(opt);
    composer* box = owned.get();
    holder.add_child(move(owned));
    box->setup();
    box->editor().set_has_focus(true);

    box->on_key(make_char(U'/'));
    ASSERT_TRUE(box->completion_open());

    EXPECT_TRUE(box->on_key(make_key(key_event::type::escape)));
    EXPECT_FALSE(box->completion_open());

    EXPECT_FALSE(box->on_key(make_key(key_event::type::escape)));
}

TEST(TuiComposerTest, RunningPhaseEscInterrupts) {
    io_context ctx;
    strand s{ctx};
    host_root holder;
    holder.bind(s, ctx);

    int interrupts = 0;
    composer_option opt;
    opt.on_interrupt = [&interrupts] { ++interrupts; };

    auto owned = make_unique<composer>(opt);
    composer* box = owned.get();
    holder.add_child(move(owned));
    box->setup();

    box->set_phase(composer_phase::running);
    EXPECT_TRUE(box->on_key(make_key(key_event::type::escape)));
    EXPECT_EQ(interrupts, 1);
}

TEST(TuiComposerTest, NonTriggerCharacterKeepsPopupClosed) {
    io_context ctx;
    strand s{ctx};
    host_root holder;
    holder.bind(s, ctx);

    composer_option opt;
    opt.complete = [](const completion_request&, vector<completion_item>& items) {
        completion_item item;
        item.label = "x";
        items.push_back(move(item));
    };

    auto owned = make_unique<composer>(opt);
    composer* box = owned.get();
    holder.add_child(move(owned));
    box->setup();
    box->editor().set_has_focus(true);

    box->on_key(make_char(U'a'));
    EXPECT_FALSE(box->completion_open());
}


TEST(TuiKeymapTest, GlobalLayerWinsOverFocusLayer) {
    keymap map;
    int global_calls = 0;
    int focus_calls = 0;

    map.bind(shortcut_layer::global,
             key_binding{key_event::type::printable, key_modifier::ctrl, key_modifier::none, U'c',
                         [&global_calls](const key_event&) {
                             ++global_calls;
                             return true;
                         },
                         "ctrl+c quit"});
    map.bind(shortcut_layer::focus,
             key_binding{key_event::type::printable, key_modifier::ctrl, key_modifier::none, U'c',
                         [&focus_calls](const key_event&) {
                             ++focus_calls;
                             return true;
                         },
                         "focus ctrl+c"});

    EXPECT_TRUE(map.dispatch(make_char(U'c', key_modifier::ctrl)));
    EXPECT_EQ(global_calls, 1);
    EXPECT_EQ(focus_calls, 0);
}

TEST(TuiKeymapTest, UnmatchedKeysFallThrough) {
    keymap map;
    bool called = false;
    map.bind(key_binding{key_event::type::enter, key_modifier::none, key_modifier::none, 0,
                         [&called](const key_event&) {
                             called = true;
                             return true;
                         },
                         "enter"});

    EXPECT_FALSE(map.dispatch(make_key(key_event::type::escape)));
    EXPECT_FALSE(called);
    EXPECT_TRUE(map.dispatch(make_key(key_event::type::enter)));
    EXPECT_TRUE(called);
}

TEST(TuiKeymapTest, ModifiersAreRequiredAndForbidden) {
    keymap map;
    int calls = 0;
    map.bind(key_binding{key_event::type::printable, key_modifier::shift, key_modifier::ctrl, U't',
                         [&calls](const key_event&) {
                             ++calls;
                             return true;
                         },
                         "shift+t"});

    EXPECT_FALSE(map.dispatch(make_char(U't')));
    EXPECT_FALSE(map.dispatch(make_char(U't', key_modifier::ctrl)));
    EXPECT_TRUE(map.dispatch(make_char(U't', key_modifier::shift)));
    EXPECT_EQ(calls, 1);
}

TEST(TuiKeymapTest, HandlerCanDeclineToKeepLooking) {
    keymap map;
    int declining = 0;
    int accepting = 0;
    map.bind(key_binding{key_event::type::enter, key_modifier::none, key_modifier::none, 0,
                         [&accepting](const key_event&) {
                             ++accepting;
                             return true;
                         },
                         ""});
    // Later registrations are tried first, so this one runs before the accepting one.
    map.bind(key_binding{key_event::type::enter, key_modifier::none, key_modifier::none, 0,
                         [&declining](const key_event&) {
                             ++declining;
                             return false;
                         },
                         ""});

    EXPECT_TRUE(map.dispatch(make_key(key_event::type::enter)));
    EXPECT_EQ(declining, 1);
    EXPECT_EQ(accepting, 1);
}

TEST(TuiKeymapTest, HintsAndCountsTrackBindings) {
    keymap map;
    map.bind(shortcut_layer::global,
             key_binding{key_event::type::enter, key_modifier::none, key_modifier::none, 0, nullptr, "enter send"});
    map.bind(shortcut_layer::overlay,
             key_binding{key_event::type::escape, key_modifier::none, key_modifier::none, 0, nullptr, "esc close"});

    EXPECT_EQ(map.count(), 2U);
    EXPECT_EQ(map.count(shortcut_layer::global), 1U);
    ASSERT_EQ(map.hints(shortcut_layer::global).size(), 1U);
    EXPECT_EQ(map.hints(shortcut_layer::global)[0], "enter send");

    map.clear(shortcut_layer::global);
    EXPECT_EQ(map.count(), 1U);
}


TEST(TuiOutputBlockTest, TogglesBetweenCollapsedAndExpanded) {
    output_block_option opt;
    opt.header = element::text("head");
    opt.body = element::text("body1\nbody2");
    opt.collapsed = true;
    opt.hidden_lines = 2;

    output_block block(opt);
    block.setup();
    EXPECT_TRUE(block.collapsed());

    const element collapsed = block.render();
    EXPECT_EQ(collapsed.kind(), element::kind::band);

    EXPECT_TRUE(block.on_key(make_key(key_event::type::enter)));
    EXPECT_FALSE(block.collapsed());

    const element expanded = block.render();
    EXPECT_EQ(expanded.kind(), element::kind::band);

    EXPECT_TRUE(block.on_key(make_key(key_event::type::left)));
    EXPECT_TRUE(block.collapsed());
}

TEST(TuiOutputBlockTest, HiddenLineCountFormatsHint) {
    output_block_option opt;
    opt.header = element::text("head");
    opt.body = element::text("body");
    opt.collapsed = true;
    opt.hidden_lines = 42;
    opt.collapsed_format = "%d lines";

    output_block block(opt);
    block.setup();
    const element tree = block.render();

    ASSERT_FALSE(tree.children().empty());
    const element& inner = tree.children()[0];
    ASSERT_EQ(inner.kind(), element::kind::vbox);
    ASSERT_GE(inner.children().size(), 2U);
    const element& hint_row = inner.children()[1];
    EXPECT_EQ(hint_row.kind(), element::kind::rich_text);
    EXPECT_TRUE(hint_row.spans()[0].content.view().find("42"_sv) != string_view::npos);
}

TEST(TuiOutputBlockTest, ClickInsideToggles) {
    output_block_option opt;
    opt.header = element::text("head");
    opt.body = element::text("body");
    opt.collapsed = true;

    output_block block(opt);
    block.setup();
    ignore = block.render();

    mouse_event press;
    press.action = mouse_action::press;
    press.x = 1;
    press.y = 0;
    EXPECT_TRUE(block.on_mouse(press));
    EXPECT_FALSE(block.collapsed());
}


TEST(TuiToolCardTest, StatusChangesIconAndStyle) {
    tool_card_option opt;
    opt.title = "bash";
    opt.summary = "ls -la";
    opt.status = tool_status::running;

    tool_card card(opt);
    card.setup();
    EXPECT_TRUE(card.running());

    const element running = card.render();
    EXPECT_EQ(running.kind(), element::kind::band);

    card.set_status(tool_status::success, 1200);
    EXPECT_FALSE(card.running());
    const element success = card.render();
    EXPECT_EQ(success.kind(), element::kind::band);
}

TEST(TuiToolCardTest, AnimationAccumulatesElapsedTimeWhileRunning) {
    tool_card_option opt;
    opt.title = "bash";

    tool_card card(opt);
    card.setup();
    card.on_animation(500);
    card.on_animation(700);
    EXPECT_TRUE(card.running());

    card.set_status(tool_status::failed);
    card.on_animation(1000);
    EXPECT_FALSE(card.running());
}

TEST(TuiToolCardTest, OutputCollapsesAndExpands) {
    tool_card_option opt;
    opt.title = "bash";
    opt.collapsed = true;

    tool_card card(opt);
    card.setup();
    card.set_output(string("line1\nline2\nline3"));
    ignore = card.render();

    EXPECT_TRUE(card.output().collapsed());
    EXPECT_TRUE(card.on_key(make_key(key_event::type::enter)));
    EXPECT_FALSE(card.output().collapsed());
}

TEST(TuiToolCardTest, RepeatedRenderDoesNotScheduleAnotherFrame) {
    host_root holder;
    auto owned = make_unique<tool_card>(tool_card_option{});
    auto* card = owned.get();
    holder.add_child(move(owned));

    headless app(&holder, 30, 6);
    app.render();

    card->set_output(string("out"));
    app.tick();
    const auto first = app.plaintext();
    app.tick();
    app.tick();
    EXPECT_EQ(app.plaintext(), first);
}

TEST(TuiVirtualListComponentTest, AutoParentMeasuresEstimatedHeight) {
    const auto builder = [](const size_t index) {
        return element::text(to_string(static_cast<int64_t>(index)), style{}, style::wrap_mode::none);
    };

    elements rows;
    rows.push_back(element::text("header", style{}, style::wrap_mode::none));
    rows.push_back(element::virtual_list(5, 2, builder));
    const element tree = element::vbox(move(rows));

    const auto layout = compute_layout(tree, 20, 40);
    ASSERT_EQ(layout.size(), 2U);
    EXPECT_EQ(layout[0].h, 1);
    EXPECT_EQ(layout[1].y, 1);
    EXPECT_EQ(layout[1].h, 10);

    const element tight = element::vbox(elements{element::virtual_list(5, 2, builder)});
    const auto tight_layout = compute_layout(tight, 20, 3);
    ASSERT_EQ(tight_layout.size(), 1U);
    EXPECT_EQ(tight_layout[0].h, 3);
}

TEST(TuiComponentLifetimeTest, TreeReleasedBeforeEventLoopDies) {
    host_root holder;
    auto app = make_unique<headless>(&holder, 40, 8);

    tool_runner_option opt;
    auto owned = make_unique<tool_runner>(app->context(), app->executor(), opt);
    tool_runner* runner = owned.get();
    holder.add_child(move(owned));
    app->render();

    ASSERT_TRUE(runner->start(shell_short_echo_command()));
    for (int i = 0; i < 400 && runner->running(); ++i) {
        app->tick();
        this_thread::sleep_for(milliseconds(2));
    }
    EXPECT_FALSE(runner->running());
    EXPECT_EQ(runner->exit_code(), 0);

    ASSERT_TRUE(runner->start(shell_long_running_command()));
    app->tick();
    EXPECT_TRUE(runner->running());

    app.reset();
    EXPECT_FALSE(runner->running());

    holder.detach_all_children();
}


TEST(TuiProcessWatcherTest, StreamsOutputAndReportsExit) {
    io_context ctx;
    strand s{ctx};

    process_watcher watcher(ctx, s);
    string output;
    int code = -99;
    watcher.set_on_output([&output](const string_view chunk) { output.append(string(chunk)); });
    watcher.set_on_exit([&ctx, &code](const int value) {
        code = value;
        ctx.stop();
    });

    ASSERT_TRUE(watcher.start_shell(shell_two_echoes_exit_command()));
    EXPECT_TRUE(watcher.running());

    io_context::work guard{ctx};
    ctx.run();

    EXPECT_EQ(compact_output(output.view()), "alphabeta");
    EXPECT_EQ(code, 7);
    EXPECT_FALSE(watcher.running());
    EXPECT_EQ(watcher.exit_code(), 7);
}

TEST(TuiProcessWatcherTest, PtyModeFallsBackWhenUnavailable) {
    io_context ctx;
    strand s{ctx};

    process_watcher watcher(ctx, s);
    string output;
    watcher.set_on_output([&output](const string_view chunk) { output.append(string(chunk)); });
    watcher.set_on_exit([&ctx](int) { ctx.stop(); });

    process_watcher::option opt;
    opt.use_pty = true;
#ifdef NEFORCE_PLATFORM_WINDOWS
    ASSERT_TRUE(watcher.start("cmd.exe", {"/c", "echo fallback"}, opt));
#else
    ASSERT_TRUE(watcher.start("/bin/sh", {"-c", "printf fallback"}, opt));
#endif
    io_context::work guard{ctx};
    ctx.run();

    EXPECT_EQ(watcher.uses_pty(), process_watcher::supports_pty());
    if (!watcher.uses_pty()) {
        EXPECT_NE(output.view().find("fallback"_sv), string_view::npos);
    }
}

TEST(TuiProcessWatcherTest, FailingCommandReportsNonZeroExit) {
    io_context ctx;
    strand s{ctx};

    process_watcher watcher(ctx, s);
    int code = 0;
    watcher.set_on_exit([&ctx, &code](const int value) {
        code = value;
        ctx.stop();
    });
    ASSERT_TRUE(watcher.start_shell("exit 3"));
    io_context::work guard{ctx};
    ctx.run();

    EXPECT_EQ(code, 3);
}

TEST(TuiProcessWatcherTest, RepeatedStartsDoNotAccumulateTimerCallbacks) {
    for (int i = 0; i < 4; ++i) {
        io_context ctx;
        strand s{ctx};
        process_watcher watcher(ctx, s);
        int code = -99;
        watcher.set_on_exit([&ctx, &code](const int value) {
            code = value;
            ctx.stop();
        });

        ASSERT_TRUE(watcher.start_shell("exit 0"));
        io_context::work guard{ctx};
        ctx.run();

        EXPECT_FALSE(watcher.running());
        EXPECT_EQ(code, 0);
        EXPECT_EQ(watcher.exit_code(), 0);
    }
}

TEST(TuiToolRunnerTest, StreamsToCardAndMarksSuccess) {
    host_root holder;
    headless app(&holder, 40, 10);

    tool_runner_option opt;
    opt.card.title = "bash";
    opt.card.collapsed = false;
    auto owned = make_unique<tool_runner>(app.context(), app.executor(), opt);
    tool_runner* runner = owned.get();
    holder.add_child(move(owned));
    app.render();

    ASSERT_TRUE(runner->start(shell_tool_output_command()));
    for (int i = 0; i < 300 && runner->running(); ++i) {
        app.tick();
        this_thread::sleep_for(milliseconds(5));
    }
    app.tick();

    EXPECT_FALSE(runner->running());
    EXPECT_EQ(runner->exit_code(), 0);
    EXPECT_NE(runner->output().view().find("tool-output"_sv), string_view::npos);
    EXPECT_EQ(runner->card().status(), tool_status::success);
}

TEST(TuiToolRunnerTest, FailureSetsFailedStatus) {
    host_root holder;
    headless app(&holder, 40, 8);

    tool_runner_option opt;
    opt.card.title = "bash";
    auto owned = make_unique<tool_runner>(app.context(), app.executor(), opt);
    tool_runner* runner = owned.get();
    holder.add_child(move(owned));
    app.render();

    ASSERT_TRUE(runner->start("exit 9"));
    for (int i = 0; i < 300 && runner->running(); ++i) {
        app.tick();
        this_thread::sleep_for(milliseconds(5));
    }
    app.tick();

    EXPECT_EQ(runner->exit_code(), 9);
    EXPECT_EQ(runner->card().status(), tool_status::failed);
}

TEST(TuiToolRunnerTest, InterruptMarksInterrupted) {
    host_root holder;
    headless app(&holder, 40, 8);

    tool_runner_option opt;
    opt.card.title = "bash";
    auto owned = make_unique<tool_runner>(app.context(), app.executor(), opt);
    tool_runner* runner = owned.get();
    holder.add_child(move(owned));
    app.render();

    ASSERT_TRUE(runner->start(shell_long_running_command()));
    app.tick();

    runner->interrupt();
    for (int i = 0; i < 600 && runner->running(); ++i) {
        app.tick();
        this_thread::sleep_for(milliseconds(5));
    }
    app.tick();

    EXPECT_FALSE(runner->running());
    EXPECT_EQ(runner->card().status(), tool_status::interrupted);
}

TEST(TuiToolRunnerTest, OutputRingDropsOldestLines) {
    host_root holder;
    headless app(&holder, 40, 8);

    tool_runner_option opt;
    opt.card.title = "bash";
    opt.max_output_lines = 3;
    auto owned = make_unique<tool_runner>(app.context(), app.executor(), opt);
    tool_runner* runner = owned.get();
    holder.add_child(move(owned));
    app.render();

    const string payload("l1\nl2\nl3\nl4\nl5");
    runner->append_output(payload.view());
    runner->on_animation(16);

    EXPECT_EQ(runner->output(), "l3\nl4\nl5");
}

TEST(TuiTextAreaMouseTest, ClickPositionsCaretAndDragSelects) {
    io_context ctx;
    strand s{ctx};
    host_root holder;
    holder.bind(s, ctx);

    auto& text = holder.make_state<string>(string("hello world"));
    text_area_option opt;
    opt.text = &text;
    opt.style.width = style::size_hint{style::size_hint::fixed, 20};
    opt.style.height = style::size_hint{style::size_hint::fixed, 3};

    auto view = make_unique<text_area_view>(opt);
    text_area_view* area = view.get();
    holder.add_child(move(view));
    area->set_has_focus(true);

    headless app(&holder, 20, 3);
    app.render();
    app.tick();

    mouse_event press;
    press.action = mouse_action::press;
    press.button = mouse_button::left;
    press.x = 5;
    press.y = 0;
    EXPECT_TRUE(area->on_mouse(press));
    EXPECT_EQ(area->cursor(), 5U);

    mouse_event drag = press;
    drag.action = mouse_action::move;
    drag.x = 11;
    EXPECT_TRUE(area->on_mouse(drag));
    EXPECT_EQ(area->selection_begin(), 5U);
    EXPECT_EQ(area->selection_end(), 11U);
}

TEST(TuiTextAreaMouseTest, CopySelectionWithoutSelectionFails) {
    io_context ctx;
    strand s{ctx};
    host_root holder;
    holder.bind(s, ctx);

    auto& text = holder.make_state<string>(string("abc"));
    text_area_option opt;
    opt.text = &text;

    auto view = make_unique<text_area_view>(opt);
    text_area_view* area = view.get();
    holder.add_child(move(view));

    EXPECT_FALSE(area->copy_selection());

    area->select_all();
    EXPECT_TRUE(area->copy_selection());
}


TEST(TuiLogViewMouseTest, ClickSelectsMessageUnderCursor) {
    io_context ctx;
    strand s{ctx};
    host_root holder;
    holder.bind(s, ctx);

    log_view_option opt;
    opt.gap = 0;
    auto owned = make_unique<log_view>(opt);
    log_view* log = owned.get();
    holder.add_child(move(owned));

    headless app(&holder, 24, 8);
    app.render();

    uint64_t clicked = 0;
    const uint64_t first = log->append(log_role::user, "first");
    log->finish(first);
    const uint64_t second = log->append(log_role::assistant, "second");
    log->finish(second);
    app.tick();
    app.tick();
    app.tick();

    mouse_event press;
    press.action = mouse_action::press;
    press.button = mouse_button::left;
    press.x = 1;
    press.y = 0;
    EXPECT_TRUE(log->on_mouse(press));
    clicked = log->selected_message();
    EXPECT_TRUE(clicked == first || clicked == second);
    EXPECT_TRUE(log->copy_selected());
}

TEST(TuiLogViewMouseTest, CopyMessageWritesClipboardPath) {
    log_view log;
    log.setup();

    const uint64_t id = log.append(log_role::assistant, "copy me");
    log.finish(id);
    EXPECT_TRUE(log.copy_message(id));
    EXPECT_FALSE(log.copy_message(id + 99));
}


TEST(TuiRecorderTest, RoundTripsSessionThroughJson) {
    io_context ctx;
    strand s{ctx};
    host_root holder;
    holder.bind(s, ctx);

    auto& text = holder.make_state<string>(string());
    text_area_option opt;
    opt.text = &text;
    auto view = make_unique<text_area_view>(opt);
    holder.add_child(move(view));

    recorder rec("session");
    rec.begin("session", 20, 4);

    headless app(&holder, 20, 4);
    app.set_recorder(&rec);
    app.render();

    // The Tab that establishes focus is itself recorded, so replay reproduces it.
    app.feed("\t");
    app.tick();
    app.feed("abc");
    app.tick();
    app.render();
    app.feed("\x7f");
    app.tick();
    app.render();

    EXPECT_GT(rec.size(), 0U);
    EXPECT_GT(rec.frame_count(), 2U);

    const string json = rec.serialize();
    EXPECT_NE(json.view().find("\"records\""_sv), string_view::npos);

    host_root holder2;
    holder2.bind(s, ctx);
    auto& text2 = holder2.make_state<string>(string());
    text_area_option opt2;
    opt2.text = &text2;
    holder2.add_child(make_unique<text_area_view>(opt2));

    headless app2(&holder2, 20, 4);
    app2.render();

    const auto report = recorder::replay(json.view(), app2);
    const auto raw = (report.mismatch_details.empty() ? string() : report.mismatch_details.front());
    EXPECT_TRUE(report.ok()) << raw.data();
    EXPECT_GT(report.frames, 2U);
    EXPECT_EQ(text2.value(), "ab");
}

TEST(TuiRecorderTest, DetectsFrameMismatch) {
    io_context ctx;
    strand s{ctx};
    host_root holder;
    holder.bind(s, ctx);
    auto& text = holder.make_state<string>(string("x"));
    text_area_option opt;
    opt.text = &text;
    holder.add_child(make_unique<text_area_view>(opt));

    headless app(&holder, 20, 4);
    app.render();

    recorder rec("bad");
    rec.begin("bad");
    rec.record_frame("this frame never existed");
    const auto report = recorder::replay(rec.serialize().view(), app);
    EXPECT_FALSE(report.ok());
    EXPECT_EQ(report.mismatches, 1U);
    EXPECT_EQ(report.mismatch_events.size(), 1U);
}

TEST(TuiRecorderTest, RejectsMalformedJson) {
    io_context ctx;
    strand s{ctx};
    fixed_root holder;
    holder.content = element::text("x");

    headless app(&holder, 20, 4);
    app.render();

    const auto report = recorder::replay("{not json"_sv, app);
    EXPECT_FALSE(report.ok());
    EXPECT_EQ(report.mismatch_details.size(), 1U);
}

namespace {
    void put_glyph(screen& frame, const int x, const int y, const char* glyph) {
        frame.fast_cell_at(x, y).character = string(glyph);
    }

    void put_line(screen& frame, const int x, const int y, const string_view text) {
        for (size_t i = 0; i < text.size(); ++i) {
            frame.fast_cell_at(x + static_cast<int>(i), y).character = string(text.data() + i, 1);
        }
    }
} // namespace

TEST(TuiSelectionTest, FullRowsAreJoinedWithNewlinesAndTrailingBlanksTrimmed) {
    screen frame(12, 3);
    put_line(frame, 0, 0, "hello");
    put_line(frame, 0, 1, "world");

    screen_selection selection;
    selection.start(0, 0);
    selection.extend_to(11, 1);

    EXPECT_EQ(string("hello\nworld"), selection_text(frame, selection));
}

TEST(TuiSelectionTest, PartialRowsUseTheDragColumns) {
    screen frame(12, 2);
    put_line(frame, 0, 0, "hello");
    put_line(frame, 0, 1, "world");

    screen_selection selection;
    selection.start(2, 0);
    selection.extend_to(2, 1);

    EXPECT_EQ(string("llo\nwor"), selection_text(frame, selection));
}

TEST(TuiSelectionTest, ReverseDragProducesTheSameText) {
    screen frame(12, 3);
    put_line(frame, 0, 0, "alpha");
    put_line(frame, 0, 1, "beta");
    put_line(frame, 0, 2, "gamma");

    screen_selection forward;
    forward.start(0, 0);
    forward.extend_to(4, 2);

    screen_selection backward;
    backward.start(4, 2);
    backward.extend_to(0, 0);

    EXPECT_EQ(selection_text(frame, forward), selection_text(frame, backward));
    EXPECT_EQ(string("alpha\nbeta\ngamma"), selection_text(frame, backward));
}

TEST(TuiSelectionTest, WideGlyphsDoNotInsertPadding) {
    screen frame(8, 1);
    put_glyph(frame, 0, 0, "\xe4\xb8\xad");
    put_glyph(frame, 2, 0, "x");

    screen_selection selection;
    selection.start(0, 0);
    selection.extend_to(2, 0);

    EXPECT_EQ(string("\xe4\xb8\xadx"), selection_text(frame, selection));
}

TEST(TuiSelectionTest, ContainsCoversTheMiddleRows) {
    screen_selection selection;
    selection.start(2, 1);
    selection.extend_to(4, 3);

    EXPECT_TRUE(selection.contains(2, 1));
    EXPECT_TRUE(selection.contains(7, 2));
    EXPECT_TRUE(selection.contains(4, 3));
    EXPECT_FALSE(selection.contains(1, 1));
    EXPECT_FALSE(selection.contains(5, 3));
    EXPECT_FALSE(selection.contains(3, 4));
}

TEST(TuiSelectionTest, EmptySelectionYieldsNoText) {
    screen frame(8, 2);
    put_line(frame, 0, 0, "abc");

    const screen_selection selection;
    EXPECT_TRUE(selection.empty());
    EXPECT_TRUE(selection_text(frame, selection).empty());
}

TEST(TuiSelectionTest, PaintMarksOnlyTheSelectedCells) {
    screen frame(6, 2);
    screen_selection selection;
    selection.start(1, 0);
    selection.extend_to(3, 0);

    paint_selection(frame, selection);

    EXPECT_TRUE(frame.fast_cell_at(1, 0).inverted);
    EXPECT_TRUE(frame.fast_cell_at(3, 0).inverted);
    EXPECT_FALSE(frame.fast_cell_at(0, 0).inverted);
    EXPECT_FALSE(frame.fast_cell_at(4, 0).inverted);
    EXPECT_FALSE(frame.fast_cell_at(1, 1).inverted);
}

TEST(TuiSelectionTest, DragAcrossLinesCopiesThroughTheReconciler) {
    io_context ctx;
    strand s{ctx};
    fixed_root root;
    root.content = element::vbox({element::text("hello"), element::text("world")});

    headless app(&root, 12, 3);
    app.render();

    reconciler& pipe = app.pipeline();
    string copied;
    pipe.set_clipboard_writer([&copied](const string_view text) { copied = string(text); });

    mouse_event press;
    press.x = 0;
    press.y = 0;
    press.button = mouse_button::left;
    press.action = mouse_action::press;
    pipe.dispatch_mouse(press);

    mouse_event drag = press;
    drag.action = mouse_action::move;
    drag.x = 4;
    drag.y = 1;
    pipe.dispatch_mouse(drag);

    mouse_event release = drag;
    release.action = mouse_action::release;
    pipe.dispatch_mouse(release);

    EXPECT_TRUE(pipe.selection().active);
    EXPECT_TRUE(pipe.copy_selection());
    EXPECT_EQ(string("hello\nworld"), copied);
    EXPECT_FALSE(pipe.selection().active);
}

TEST(TuiSelectionTest, ClickWithoutDragDoesNotStartSelection) {
    io_context ctx;
    strand s{ctx};
    fixed_root root;
    root.content = element::text("hello");

    headless app(&root, 12, 2);
    app.render();

    reconciler& pipe = app.pipeline();

    mouse_event press;
    press.x = 1;
    press.y = 0;
    press.button = mouse_button::left;
    press.action = mouse_action::press;
    pipe.dispatch_mouse(press);

    mouse_event release = press;
    release.action = mouse_action::release;
    pipe.dispatch_mouse(release);

    EXPECT_FALSE(pipe.selection().active);
    EXPECT_FALSE(pipe.copy_selection());
}

TEST(TuiSelectionTest, ScrollbarDragIsNotTreatedAsSelection) {
    io_context ctx;
    strand s{ctx};
    fixed_root root;
    root.content = element::text("hello");

    headless app(&root, 12, 2);
    app.render();

    reconciler& pipe = app.pipeline();

    mouse_event press;
    press.x = 2;
    press.y = 0;
    press.button = mouse_button::left;
    press.action = mouse_action::press;
    pipe.dispatch_mouse(press);

    mouse_event drag = press;
    drag.action = mouse_action::move;
    drag.x = 5;
    drag.y = 1;
    pipe.dispatch_mouse(drag);

    EXPECT_TRUE(pipe.selection().active);
    pipe.clear_selection();
    EXPECT_FALSE(pipe.selection().active);
}

TEST(TuiSelectionTest, CopyShortcutTakesPrecedenceAndClearsTheSelection) {
    io_context ctx;
    strand s{ctx};
    fixed_root root;
    root.content = element::text("hello");

    headless app(&root, 12, 2);
    app.render();

    reconciler& pipe = app.pipeline();
    string copied;
    pipe.set_clipboard_writer([&copied](const string_view text) { copied = string(text); });

    mouse_event press;
    press.x = 0;
    press.y = 0;
    press.button = mouse_button::left;
    press.action = mouse_action::press;
    pipe.dispatch_mouse(press);

    mouse_event drag = press;
    drag.action = mouse_action::move;
    drag.x = 4;
    drag.y = 0;
    pipe.dispatch_mouse(drag);

    ASSERT_TRUE(pipe.selection().active);

    EXPECT_TRUE(pipe.dispatch_key(make_char(U'c', key_modifier::ctrl)));
    EXPECT_EQ(string("hello"), copied);
    EXPECT_FALSE(pipe.selection().active);
}

TEST(TuiSelectionTest, OtherKeysDropTheSelection) {
    io_context ctx;
    strand s{ctx};
    fixed_root root;
    root.content = element::text("hello");

    headless app(&root, 12, 2);
    app.render();

    reconciler& pipe = app.pipeline();

    mouse_event press;
    press.x = 0;
    press.y = 0;
    press.button = mouse_button::left;
    press.action = mouse_action::press;
    pipe.dispatch_mouse(press);

    mouse_event drag = press;
    drag.action = mouse_action::move;
    drag.x = 3;
    drag.y = 0;
    pipe.dispatch_mouse(drag);

    ASSERT_TRUE(pipe.selection().active);

    pipe.dispatch_key(make_char(U'x'));
    EXPECT_FALSE(pipe.selection().active);
}

TEST(TuiComposerTest, SubmitRecordsHistoryAndArrowKeysBrowseIt) {
    io_context ctx;
    strand s{ctx};
    host_root holder;
    holder.bind(s, ctx);

    vector<string> submitted;
    composer_option opt;
    opt.on_submit = [&submitted](const string& text) { submitted.push_back(text); };

    auto view = make_unique<composer>(opt);
    composer* comp = view.get();
    holder.add_child(move(view));
    comp->take_focus();

    comp->on_key(make_char(U'l'));
    comp->on_key(make_char(U's'));
    comp->on_key(make_key(key_event::type::enter));

    comp->on_key(make_char(U'p'));
    comp->on_key(make_char(U'w'));
    comp->on_key(make_char(U'd'));
    comp->on_key(make_key(key_event::type::enter));

    ASSERT_EQ(2u, submitted.size());
    EXPECT_EQ(string("ls"), submitted[0]);
    EXPECT_EQ(string("pwd"), submitted[1]);
    EXPECT_EQ(2u, comp->history_size());
    EXPECT_EQ(string(""), string(comp->text()));

    EXPECT_TRUE(comp->on_key(make_key(key_event::type::up)));
    EXPECT_EQ(string("pwd"), string(comp->text()));
    EXPECT_TRUE(comp->on_key(make_key(key_event::type::up)));
    EXPECT_EQ(string("ls"), string(comp->text()));
    EXPECT_TRUE(comp->on_key(make_key(key_event::type::down)));
    EXPECT_EQ(string("pwd"), string(comp->text()));
    EXPECT_TRUE(comp->on_key(make_key(key_event::type::down)));
    EXPECT_TRUE(comp->text().empty());
}

TEST(TuiComposerTest, DownWithoutHistoryDoesNotEraseTheDraft) {
    io_context ctx;
    strand s{ctx};
    host_root holder;
    holder.bind(s, ctx);

    composer_option opt;
    auto view = make_unique<composer>(opt);
    composer* comp = view.get();
    holder.add_child(move(view));
    comp->take_focus();

    comp->on_key(make_char(U'a'));
    comp->on_key(make_char(U'b'));

    comp->on_key(make_key(key_event::type::down));
    EXPECT_EQ(string("ab"), string(comp->text()));
}

TEST(TuiComposerTest, MultiLineDraftKeepsArrowKeysForTheCaret) {
    io_context ctx;
    strand s{ctx};
    host_root holder;
    holder.bind(s, ctx);

    vector<string> submitted;
    composer_option opt;
    opt.on_submit = [&submitted](const string& text) { submitted.push_back(text); };

    auto view = make_unique<composer>(opt);
    composer* comp = view.get();
    holder.add_child(move(view));
    comp->take_focus();

    comp->on_key(make_char(U'x'));
    comp->on_key(make_key(key_event::type::enter, key_modifier::shift));
    comp->on_key(make_char(U'y'));
    ASSERT_EQ(string("x\ny"), string(comp->text()));
    comp->on_key(make_key(key_event::type::enter));
    ASSERT_EQ(1u, submitted.size());
    EXPECT_EQ(string("x\ny"), submitted[0]);
    EXPECT_EQ(1u, comp->history_size());

    comp->on_key(make_char(U'p'));
    comp->on_key(make_key(key_event::type::enter, key_modifier::shift));
    comp->on_key(make_char(U'q'));

    comp->on_key(make_key(key_event::type::up));
    EXPECT_EQ(string("p\nq"), string(comp->text()));
}

TEST(TuiComposerTest, HistoryLimitZeroDisablesRecording) {
    io_context ctx;
    strand s{ctx};
    host_root holder;
    holder.bind(s, ctx);

    composer_option opt;
    opt.history_limit = 0;
    auto view = make_unique<composer>(opt);
    composer* comp = view.get();
    holder.add_child(move(view));
    comp->take_focus();

    comp->on_key(make_char(U'a'));
    comp->on_key(make_key(key_event::type::enter));

    EXPECT_EQ(0u, comp->history_size());
}


TEST(TuiSelectionTest, CopyOnSelectWritesOnReleaseAndKeepsHighlight) {
    io_context ctx;
    strand s{ctx};
    fixed_root root;
    root.content = element::vbox({element::text("hello"), element::text("world")});

    headless app(&root, 12, 3);
    app.render();

    reconciler& pipe = app.pipeline();
    string copied;
    pipe.set_clipboard_writer([&copied](const string_view text) { copied = string(text); });
    pipe.set_copy_on_select(true);

    mouse_event press;
    press.x = 0;
    press.y = 0;
    press.button = mouse_button::left;
    press.action = mouse_action::press;
    pipe.dispatch_mouse(press);

    mouse_event drag = press;
    drag.action = mouse_action::move;
    drag.x = 4;
    drag.y = 1;
    pipe.dispatch_mouse(drag);

    mouse_event release = drag;
    release.action = mouse_action::release;
    pipe.dispatch_mouse(release);

    EXPECT_EQ(string("hello\nworld"), copied);
    EXPECT_TRUE(pipe.selection().active);
}

TEST(TuiSelectionTest, ScrollSelectionFollowsContent) {
    screen_selection selection;
    selection.start(0, 2);
    selection.extend_to(4, 3);
    selection.translate(-2);
    EXPECT_TRUE(selection.active);
    EXPECT_EQ(selection.anchor_y, 0);
    EXPECT_EQ(selection.caret_y, 1);

    selection.translate(-3);
    EXPECT_FALSE(selection.active);
}

TEST(TuiSelectionTest, ScrollSelectionThroughReconcilerKeepsTextAligned) {
    io_context ctx;
    strand s{ctx};
    fixed_root root;
    root.content = element::vbox({element::text("row0"), element::text("row1"), element::text("row2")});
    headless app(&root, 12, 4);
    app.render();
    reconciler& pipe = app.pipeline();

    mouse_event press;
    press.x = 0;
    press.y = 1;
    press.button = mouse_button::left;
    press.action = mouse_action::press;
    pipe.dispatch_mouse(press);
    mouse_event drag = press;
    drag.action = mouse_action::move;
    drag.x = 3;
    drag.y = 1;
    pipe.dispatch_mouse(drag);

    EXPECT_EQ(string("row1"), pipe.selection_text());
    pipe.scroll_selection(-1);
    EXPECT_EQ(string("row0"), pipe.selection_text());
}

TEST(TuiFocusDispatchTest, FocusChangeReachesComponentsAndPausesClock) {
    io_context ctx;
    strand s{ctx};
    fixed_root root;
    root.content = element::text("hello");
    headless app(&root, 10, 2);
    app.render();

    frame_clock::instance().reset();
    frame_clock::instance().set_paused(false);

    focus_event lost;
    lost.focused = false;
    app.pipeline().dispatch_focus(lost);
    EXPECT_TRUE(frame_clock::instance().paused());
    EXPECT_FALSE(app.pipeline().focused());

    focus_event gained;
    gained.focused = true;
    app.pipeline().dispatch_focus(gained);
    EXPECT_FALSE(frame_clock::instance().paused());
    EXPECT_TRUE(app.pipeline().focused());
}


namespace {
    struct preview_root : component<> {
        int previews = 0;
        element render() override { return element::text("root"); }
        bool preview_key(const key_event& e) override {
            ++previews;
            return e.key == key_event::type::printable && e.cp.value() == U'q';
        }
    };
} // namespace

TEST(TuiPreviewKeyTest, RootSeesKeysBeforeTheFocusedChild) {
    io_context ctx;
    strand s{ctx};
    preview_root root;
    headless app(&root, 20, 3);
    app.render();

    key_event q;
    q.key = key_event::type::printable;
    q.cp = codepoint{U'q'};
    EXPECT_TRUE(app.pipeline().dispatch_key(q));
    EXPECT_EQ(root.previews, 1);

    key_event other;
    other.key = key_event::type::printable;
    other.cp = codepoint{U'x'};
    app.pipeline().dispatch_key(other);
    EXPECT_EQ(root.previews, 2);
}

TEST(TuiTextAreaTest, WordUndoGroupsConsecutiveTyping) {
    io_context ctx;
    strand s{ctx};
    host_root holder;
    holder.bind(s, ctx);

    auto& text = holder.make_state<string>(string());
    text_area_option opt;
    opt.text = &text;
    opt.word_undo = true;

    auto view = make_unique<text_area_view>(opt);
    text_area_view* area = view.get();
    holder.add_child(move(view));
    area->set_has_focus(true);

    area->on_key(make_char(U'h'));
    area->on_key(make_char(U'e'));
    area->on_key(make_char(U'l'));
    area->on_key(make_char(U'l'));
    area->on_key(make_char(U'o'));
    EXPECT_EQ(text.value(), "hello");

    EXPECT_TRUE(area->undo());
    EXPECT_EQ(text.value(), "");
    EXPECT_TRUE(area->redo());
    EXPECT_EQ(text.value(), "hello");

    area->on_key(make_char(U' '));
    area->on_key(make_char(U'w'));
    EXPECT_EQ(text.value(), "hello w");
    EXPECT_TRUE(area->undo());
    EXPECT_EQ(text.value(), "hello");
}
