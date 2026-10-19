#include <NeForce/core/async/io_context.hpp>
#include <NeForce/core/async/strand.hpp>
#include <NeForce/tui/application.hpp>
#include <NeForce/tui/component/approve_prompt.hpp>
#include <NeForce/tui/component/select_list.hpp>
#include <NeForce/tui/component/status_indicator.hpp>
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
            return child_at(0)->render();
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
