#include <NeForce/core/async/io_context.hpp>
#include <NeForce/core/async/strand.hpp>
#include <NeForce/tui/component/text_input.hpp>
#include <NeForce/tui/dom/element.hpp>
#include <NeForce/tui/dom/layout.hpp>
#include <NeForce/tui/dom/gauge.hpp>
#include <NeForce/tui/dom/paragraph.hpp>
#include <NeForce/tui/event_dispatcher.hpp>
#include <NeForce/tui/focus_manager.hpp>
#include <NeForce/tui/input_parser.hpp>
#include <NeForce/tui/renderer.hpp>
#include <NeForce/tui/screen.hpp>
#include <gtest/gtest.h>
using namespace neforce;
using namespace neforce::tui;
using namespace neforce::tui::components;

namespace {
    struct parser_runner {
        input_parser parser;
        vector<parsed_input> events;

        void feed_bytes(const char* bytes, const size_t len) {
            for (size_t i = 0; i < len; ++i) {
                parsed_input in;
                if (parser.feed(static_cast<byte_t>(bytes[i]), in)) {
                    events.push_back(in);
                }
            }
            parsed_input pending;
            if (parser.flush(pending)) {
                events.push_back(pending);
            }
        }

        void feed_string(const string_view text) { feed_bytes(text.data(), text.size()); }

        NEFORCE_NODISCARD size_t size() const noexcept { return events.size(); }

        NEFORCE_NODISCARD const parsed_input& at(const size_t i) const { return events[i]; }

        NEFORCE_NODISCARD bool only_key(const key_event::type t) const {
            return events.size() == 1 && events[0].type == parsed_input::kind::key && events[0].key.key == t;
        }
    };

    struct focusable_leaf : component<> {
        element render() override { return element::text("leaf"); }

        NEFORCE_NODISCARD bool focusable() const override { return true; }
    };

    struct tab_leaf : component<> {
        bool consumed = false;

        element render() override { return element::text("tab"); }

        bool on_key(const key_event& e) override {
            if (e.key == key_event::type::tab) {
                consumed = true;
                return true;
            }
            return false;
        }

        NEFORCE_NODISCARD bool focusable() const override { return true; }
    };

    struct paste_leaf : component<> {
        string received;
        bool consume = true;

        element render() override { return element::text("paste"); }

        bool on_paste(const paste_event& e) override {
            received = e.text;
            return consume;
        }
    };
} // namespace


TEST(TuiInputParserTest, PrintableAsciiBecomesPrintableKey) {
    parser_runner runner;
    runner.feed_string("ab");

    ASSERT_EQ(runner.size(), 2U);
    EXPECT_EQ(runner.at(0).type, parsed_input::kind::key);
    EXPECT_EQ(runner.at(0).key.key, key_event::type::printable);
    EXPECT_EQ(runner.at(0).key.cp.value(), static_cast<uint32_t>('a'));
    EXPECT_EQ(runner.at(1).key.cp.value(), static_cast<uint32_t>('b'));
}

TEST(TuiInputParserTest, Utf8CharactersAreDecoded) {
    parser_runner runner;
    runner.feed_string("中a");

    ASSERT_EQ(runner.size(), 2U);
    EXPECT_EQ(runner.at(0).key.cp.value(), static_cast<uint32_t>(0x4E2D));
    EXPECT_EQ(runner.at(1).key.cp.value(), static_cast<uint32_t>('a'));
}

TEST(TuiInputParserTest, ControlCharactersMapToKeys) {
    parser_runner enter_runner;
    enter_runner.feed_string("\r");
    EXPECT_TRUE(enter_runner.only_key(key_event::type::enter));

    parser_runner tab_runner;
    tab_runner.feed_string("\t");
    EXPECT_TRUE(tab_runner.only_key(key_event::type::tab));

    parser_runner backspace_runner;
    backspace_runner.feed_string("\x7f");
    EXPECT_TRUE(backspace_runner.only_key(key_event::type::backspace));
}

TEST(TuiInputParserTest, CtrlLettersCarryCtrlModifier) {
    parser_runner runner;
    runner.feed_string("\x03");

    ASSERT_EQ(runner.size(), 1U);
    EXPECT_EQ(runner.at(0).key.key, key_event::type::printable);
    EXPECT_EQ(runner.at(0).key.cp.value(), static_cast<uint32_t>('c'));
    EXPECT_EQ(runner.at(0).key.mods & key_modifier::ctrl, key_modifier::ctrl);
}

TEST(TuiInputParserTest, CsiArrowsAndNavigationKeys) {
    parser_runner up;
    up.feed_string("\033[A");
    EXPECT_TRUE(up.only_key(key_event::type::up));

    parser_runner left;
    left.feed_string("\033[D");
    EXPECT_TRUE(left.only_key(key_event::type::left));

    parser_runner home;
    home.feed_string("\033[H");
    EXPECT_TRUE(home.only_key(key_event::type::home));

    parser_runner end;
    end.feed_string("\033[4~");
    EXPECT_TRUE(end.only_key(key_event::type::end));

    parser_runner del;
    del.feed_string("\033[3~");
    EXPECT_TRUE(del.only_key(key_event::type::delete_));

    parser_runner page_up;
    page_up.feed_string("\033[5~");
    EXPECT_TRUE(page_up.only_key(key_event::type::page_up));
}

TEST(TuiInputParserTest, Ss3SequencesAreDecoded) {
    parser_runner arrows;
    arrows.feed_string("\033OB");
    EXPECT_TRUE(arrows.only_key(key_event::type::down));

    parser_runner function;
    function.feed_string("\033OP");
    EXPECT_TRUE(function.only_key(key_event::type::F1));

    parser_runner f5;
    f5.feed_string("\033[15~");
    EXPECT_TRUE(f5.only_key(key_event::type::F5));
}

TEST(TuiInputParserTest, CsiModifiersAreDecoded) {
    parser_runner ctrl_right;
    ctrl_right.feed_string("\033[1;5C");
    ASSERT_EQ(ctrl_right.size(), 1U);
    EXPECT_EQ(ctrl_right.at(0).key.key, key_event::type::right);
    EXPECT_EQ(ctrl_right.at(0).key.mods & key_modifier::ctrl, key_modifier::ctrl);

    parser_runner shift_up;
    shift_up.feed_string("\033[1;2A");
    ASSERT_EQ(shift_up.size(), 1U);
    EXPECT_EQ(shift_up.at(0).key.key, key_event::type::up);
    EXPECT_EQ(shift_up.at(0).key.mods & key_modifier::shift, key_modifier::shift);

    parser_runner ctrl_delete;
    ctrl_delete.feed_string("\033[3;5~");
    ASSERT_EQ(ctrl_delete.size(), 1U);
    EXPECT_EQ(ctrl_delete.at(0).key.key, key_event::type::delete_);
    EXPECT_EQ(ctrl_delete.at(0).key.mods & key_modifier::ctrl, key_modifier::ctrl);
}

TEST(TuiInputParserTest, ShiftTabIsTabReverse) {
    parser_runner runner;
    runner.feed_string("\033[Z");
    EXPECT_TRUE(runner.only_key(key_event::type::tab_reverse));
}

TEST(TuiInputParserTest, AltPrefixedCharacterIsPrintableWithAlt) {
    parser_runner runner;
    runner.feed_string("\033x");

    ASSERT_EQ(runner.size(), 1U);
    EXPECT_EQ(runner.at(0).key.key, key_event::type::printable);
    EXPECT_EQ(runner.at(0).key.cp.value(), static_cast<uint32_t>('x'));
    EXPECT_EQ(runner.at(0).key.mods & key_modifier::alt, key_modifier::alt);
}

TEST(TuiInputParserTest, AltPrefixedUtf8CharacterIsDecoded) {
    parser_runner runner;
    runner.feed_string("\033中");

    ASSERT_EQ(runner.size(), 1U);
    EXPECT_EQ(runner.at(0).key.cp.value(), static_cast<uint32_t>(0x4E2D));
    EXPECT_EQ(runner.at(0).key.mods & key_modifier::alt, key_modifier::alt);
}

TEST(TuiInputParserTest, LoneEscapeIsFlushedAsEscapeKey) {
    parser_runner runner;
    runner.feed_string("\033");
    EXPECT_TRUE(runner.only_key(key_event::type::escape));
}

TEST(TuiInputParserTest, DoubleEscapeProducesTwoEscapeKeys) {
    parser_runner runner;
    runner.feed_string("\033\033");
    ASSERT_EQ(runner.size(), 2U);
    EXPECT_EQ(runner.at(0).key.key, key_event::type::escape);
    EXPECT_EQ(runner.at(1).key.key, key_event::type::escape);
}

TEST(TuiInputParserTest, UnknownCsiDoesNotWedgeTheParser) {
    parser_runner runner;
    runner.feed_string("\033[99~a");

    ASSERT_EQ(runner.size(), 1U);
    EXPECT_EQ(runner.at(0).key.cp.value(), static_cast<uint32_t>('a'));
}

TEST(TuiInputParserTest, TruncatedSequenceIsDroppedAndInputRecovers) {
    parser_runner runner;
    runner.feed_string("\033[1;");
    runner.feed_string("a");

    ASSERT_EQ(runner.size(), 1U);
    EXPECT_EQ(runner.at(0).key.cp.value(), static_cast<uint32_t>('a'));
}

TEST(TuiInputParserTest, SgrMousePressAndReleaseAreDecoded) {
    parser_runner press;
    press.feed_string("\033[<0;12;5M");
    ASSERT_EQ(press.size(), 1U);
    EXPECT_EQ(press.at(0).type, parsed_input::kind::mouse);
    EXPECT_EQ(press.at(0).mouse.action, mouse_action::press);
    EXPECT_EQ(press.at(0).mouse.button, mouse_button::left);
    EXPECT_EQ(press.at(0).mouse.x, 11);
    EXPECT_EQ(press.at(0).mouse.y, 4);

    parser_runner release;
    release.feed_string("\033[<0;12;5m");
    ASSERT_EQ(release.size(), 1U);
    EXPECT_EQ(release.at(0).mouse.action, mouse_action::release);
    EXPECT_EQ(release.at(0).mouse.button, mouse_button::left);
}

TEST(TuiInputParserTest, SgrMouseWheelAndModifiersAreDecoded) {
    parser_runner wheel;
    wheel.feed_string("\033[<64;3;3M");
    ASSERT_EQ(wheel.size(), 1U);
    EXPECT_EQ(wheel.at(0).mouse.action, mouse_action::wheel);
    EXPECT_EQ(wheel.at(0).mouse.button, mouse_button::wheelup);

    parser_runner ctrl_press;
    ctrl_press.feed_string("\033[<16;3;3M");
    ASSERT_EQ(ctrl_press.size(), 1U);
    EXPECT_EQ(ctrl_press.at(0).mouse.action, mouse_action::press);
    EXPECT_EQ(ctrl_press.at(0).mouse.mods & key_modifier::ctrl, key_modifier::ctrl);
}

TEST(TuiInputParserTest, MalformedMouseSequenceKeepsParserUsable) {
    parser_runner runner;
    runner.feed_string("\033[<0;1M");

    for (const auto& event: runner.events) {
        EXPECT_NE(event.type, parsed_input::kind::mouse);
    }

    runner.feed_string("z");
    ASSERT_GE(runner.size(), 1U);
    EXPECT_EQ(runner.at(runner.size() - 1).type, parsed_input::kind::key);
    EXPECT_EQ(runner.at(runner.size() - 1).key.cp.value(), static_cast<uint32_t>('z'));
}

TEST(TuiInputParserTest, BracketedPasteProducesSinglePasteEvent) {
    parser_runner runner;
    runner.feed_string("\033[200~hello\nworld\033[201~");

    ASSERT_EQ(runner.size(), 1U);
    EXPECT_EQ(runner.at(0).type, parsed_input::kind::paste);
    EXPECT_EQ(runner.at(0).paste_text, "hello\nworld");
}

TEST(TuiInputParserTest, PasteNormalizesCarriageReturns) {
    parser_runner runner;
    runner.feed_string("\033[200~a\r\nb\rc\033[201~");

    ASSERT_EQ(runner.size(), 1U);
    EXPECT_EQ(runner.at(0).paste_text, "a\nb\nc");
}

TEST(TuiInputParserTest, PasteIsNotSplitIntoKeystrokes) {
    parser_runner runner;
    runner.feed_string("\033[200~x");
    EXPECT_TRUE(runner.parser.in_paste());
    EXPECT_EQ(runner.size(), 0U);

    runner.feed_string("\033[201~");
    EXPECT_FALSE(runner.parser.in_paste());
    ASSERT_EQ(runner.size(), 1U);
    EXPECT_EQ(runner.at(0).paste_text, "x");
}


TEST(TuiDispatcherTest, ComponentCanClaimTab) {
    focus_manager fm;
    event_dispatcher dispatcher(fm);

    tab_leaf root;
    fm.rebuild_chain(&root);
    fm.set_focus(&root);

    const key_event tab = [] {
        key_event e;
        e.key = key_event::type::tab;
        return e;
    }();

    EXPECT_TRUE(dispatcher.dispatch_key(tab, &root));
    EXPECT_TRUE(root.consumed);
}

TEST(TuiDispatcherTest, UnconsumedTabMovesFocus) {
    focus_manager fm;
    event_dispatcher dispatcher(fm);

    struct holder : component<> {
        element render() override { return element::empty(); }
        NEFORCE_NODISCARD bool focusable() const override { return false; }
    };

    holder root;
    auto* a = new focusable_leaf();
    auto* b = new focusable_leaf();
    root.add_child(unique_ptr<component_base>(a));
    root.add_child(unique_ptr<component_base>(b));

    fm.rebuild_chain(&root);
    fm.set_focus(a);

    key_event tab;
    tab.key = key_event::type::tab;
    EXPECT_TRUE(dispatcher.dispatch_key(tab, &root));
    EXPECT_EQ(fm.focused(), b);
}

TEST(TuiDispatcherTest, PasteIsRoutedToFocusedComponent) {
    focus_manager fm;
    event_dispatcher dispatcher(fm);

    struct holder : component<> {
        element render() override { return element::empty(); }
    };

    holder root;
    auto* leaf = new paste_leaf();
    root.add_child(unique_ptr<component_base>(leaf));

    fm.rebuild_chain(&root);
    fm.set_focus(leaf);

    paste_event pe;
    pe.text = "hi";
    EXPECT_TRUE(dispatcher.dispatch_paste(pe, &root));
    EXPECT_EQ(leaf->received, "hi");
}

TEST(TuiFocusManagerTest, FocusChainRebuildsAfterStructuralChange) {
    focus_manager fm;

    struct holder : component<> {
        element render() override { return element::empty(); }
        NEFORCE_NODISCARD bool focusable() const override { return false; }
    };

    holder root;
    root.add_child(unique_ptr<component_base>(new focusable_leaf()));

    fm.rebuild_chain(&root);
    EXPECT_EQ(fm.chain().size(), 1U);

    root.add_child(unique_ptr<component_base>(new focusable_leaf()));
    EXPECT_TRUE(root.structure_dirty());

    fm.mark_chain_dirty();
    fm.rebuild_chain(&root);
    EXPECT_EQ(fm.chain().size(), 2U);
}

TEST(TuiFocusManagerTest, ValidateFocusClearsRemovedComponent) {
    focus_manager fm;

    struct holder : component<> {
        element render() override { return element::empty(); }
        NEFORCE_NODISCARD bool focusable() const override { return false; }
    };

    holder root;
    auto* leaf = new focusable_leaf();
    root.add_child(unique_ptr<component_base>(leaf));
    fm.rebuild_chain(&root);
    fm.set_focus(leaf);
    ASSERT_EQ(fm.focused(), leaf);

    auto owned = root.remove_child(leaf);
    fm.mark_chain_dirty();
    fm.rebuild_chain(&root);
    fm.validate_focus();

    EXPECT_EQ(fm.focused(), nullptr);
    EXPECT_EQ(fm.chain().size(), 0U);
}


TEST(TuiTextInputPasteTest, PasteInsertsWholeTextAtCursor) {
    io_context ctx;
    strand s{ctx};

    struct owner : component<> {
        element render() override { return element::empty(); }
        void bind(strand& st, io_context& c) {
            strand_ = &st;
            ctx_ = &c;
        }
        state<string>& make_state(string initial) { return this->create_state<string>(_NEFORCE move(initial)); }
    };

    owner root;
    root.bind(s, ctx);
    auto& text = root.make_state(string("abc"));

    text_input_option opt;
    opt.text = &text;
    auto child = text_input(opt);
    component_base* leaf = child.get();
    root.add_child(move(child));
    leaf->set_has_focus(true);
    leaf->take_focus();

    paste_event pe;
    pe.text = "XY";
    EXPECT_TRUE(leaf->on_paste(pe));
    EXPECT_EQ(text.value(), "abcXY");
}


TEST(TuiTextInputCursorTest, CaretFollowsWrappedLine) {
    io_context ctx;
    strand s{ctx};

    struct owner : component<> {
        element render() override { return element::empty(); }
        void bind(strand& st, io_context& c) {
            strand_ = &st;
            ctx_ = &c;
        }
        state<string>& make_state(string initial) { return this->create_state<string>(_NEFORCE move(initial)); }
    };

    owner root;
    root.bind(s, ctx);
    auto& text = root.make_state(string("aaa bbb ccc ddd"));

    text_input_option opt;
    opt.text = &text;
    opt.wrap = style::wrap_mode::word;
    opt.style.width = style::size_hint{style::size_hint::fixed, 12};
    opt.style.height = style::size_hint{style::size_hint::fixed, 5};

    auto child = text_input(opt);
    component_base* leaf = child.get();
    root.add_child(move(child));
    leaf->set_has_focus(true);
    leaf->take_focus();

    const element tree = leaf->render();
    const auto layout = compute_layout(tree, 12, 5);
    ASSERT_EQ(layout.size(), 1U);

    screen scr(12, 5);
    renderer r(scr, dark_theme, 12, 5);
    r.render(tree, layout, true);

    EXPECT_EQ(scr.fast_cell_at(8, 2).character, "\xe2\x96\x8c");
}

TEST(TuiScreenCursorTest, HiddenCursorEmitsHideSequenceOnly) {
    screen scr(10, 3);
    screen prev(10, 3);

    const string out = scr.to_string(prev);
    EXPECT_TRUE(out.find("\033[?25l"_sv) != string::npos);
    EXPECT_TRUE(out.find("\033[?25h"_sv) == string::npos);
}

TEST(TuiScreenCursorTest, VisibleCursorEmitsPositionShapeAndShow) {
    screen scr(10, 3);
    screen prev(10, 3);

    cursor c;
    c.x = 2;
    c.y = 1;
    c.shape = cursor::shape::bar_blinking;
    scr.set_cursor(c);

    const string out = scr.to_string(prev);
    EXPECT_TRUE(out.find("\033[2;3H"_sv) != string::npos);
    EXPECT_TRUE(out.find("\033[5 q"_sv) != string::npos);
    EXPECT_TRUE(out.find("\033[?25h"_sv) != string::npos);
}

TEST(TuiScreenCursorTest, CursorPositionIsClampedToSurface) {
    screen scr(4, 2);
    screen prev(4, 2);

    cursor c;
    c.x = 99;
    c.y = 99;
    c.shape = cursor::shape::block;
    scr.set_cursor(c);

    const string out = scr.to_string(prev);
    EXPECT_TRUE(out.find("\033[2;4H"_sv) != string::npos);
}

TEST(TuiScreenHyperlinkTest, RegisterHyperlinkDeduplicates) {
    screen scr(4, 2);

    const uint8_t first = scr.register_hyperlink(string("https://example.com"));
    const uint8_t second = scr.register_hyperlink(string("https://example.com"));
    const uint8_t third = scr.register_hyperlink(string("https://other.example"));

    EXPECT_EQ(first, second);
    EXPECT_NE(first, third);
    EXPECT_EQ(scr.hyperlink(first), "https://example.com"_sv);
    EXPECT_EQ(scr.hyperlink(third), "https://other.example"_sv);
}


TEST(TuiRendererHitTestTest, CanvasDoesNotDesynchronizeHitTesting) {
    style canvas_style;
    canvas_style.height = style::size_hint{style::size_hint::fixed, 2};
    int clicked = 0;

    element tree = element::vbox({
            element::canvas([](int, int, int, int) {}, canvas_style),
            element::button("ok", [&clicked] { ++clicked; }),
    });

    const auto layout = compute_layout(tree, 20, 10);
    ASSERT_EQ(layout.size(), 2U);

    int idx = 0;
    const element* hit = find_element_at(layout, tree, layout[1].x, layout[1].y, idx);
    ASSERT_NE(hit, nullptr);
    EXPECT_EQ(hit->kind(), element::kind::button);

    idx = 0;
    const element* canvas_hit = find_element_at(layout, tree, layout[0].x, layout[0].y, idx);
    ASSERT_NE(canvas_hit, nullptr);
    EXPECT_EQ(canvas_hit->kind(), element::kind::canvas);
}

TEST(TuiRendererHitTestTest, CanvasDoesNotDesynchronizeHitTestAt) {
    style canvas_style;
    canvas_style.height = style::size_hint{style::size_hint::fixed, 2};

    element tree = element::vbox({
            element::canvas([](int, int, int, int) {}, canvas_style),
            element::button("ok", [] {}),
    });

    const auto layout = compute_layout(tree, 20, 10);
    ASSERT_EQ(layout.size(), 2U);

    screen scr(20, 10);
    renderer r(scr, dark_theme, 20, 10);
    r.render(tree, layout, false);

    focusable_leaf fallback;
    int idx = 0;
    EXPECT_EQ(hit_test_at(layout, tree, layout[1].x, layout[1].y, idx, &fallback), &fallback);
}


TEST(TuiRenderTest, WordWrapKeepsSingleSpacesBetweenWords) {
    const element tree = element::text(string("aaa bbb ccc"), style{}, style::wrap_mode::word);
    const auto layout = compute_layout(tree, 7, 4);

    screen scr(7, 4);
    renderer r(scr, dark_theme, 7, 4);
    r.render(tree, layout, false);

    EXPECT_EQ(scr.to_plaintext(), "aaa bbb\nccc");
}

TEST(TuiRenderTest, ExplicitNewlinesStartNewLine) {
    const element tree = element::text(string("ab\ncd"), style{}, style::wrap_mode::word);
    const auto layout = compute_layout(tree, 10, 4);
    ASSERT_EQ(layout.size(), 1U);
    EXPECT_EQ(layout[0].h, 2);

    screen scr(10, 4);
    renderer r(scr, dark_theme, 10, 4);
    r.render(tree, layout, false);

    EXPECT_EQ(scr.to_plaintext(), "ab\ncd");
}

TEST(TuiRenderTest, ControlCharactersNeverReachTheGrid) {
    const element tree = element::text(string("a\tb"), style{});
    const auto layout = compute_layout(tree, 10, 2);

    screen scr(10, 2);
    renderer r(scr, dark_theme, 10, 2);
    r.render(tree, layout, false);

    EXPECT_EQ(scr.to_plaintext(), "a b");
}

TEST(TuiGaugeTest, PartialProgressFillsProportionalCells) {
    const element half = gauge(0.5F, 10);
    EXPECT_EQ(half.text(), "█████░░░░░");
}

TEST(TuiGaugeTest, ZeroAndFullProgressFillNothingAndEverything) {
    const element empty_bar = gauge(0.0F, 8);
    EXPECT_EQ(empty_bar.text(), "░░░░░░░░");

    const element full_bar = gauge(1.0F, 8);
    EXPECT_EQ(full_bar.text(), "████████");
}

TEST(TuiGaugeTest, ProgressIsClampedAndRounded) {
    const element over = gauge(2.0F, 4);
    EXPECT_EQ(over.text(), "████");

    const element under = gauge(-1.0F, 4);
    EXPECT_EQ(under.text(), "░░░░");
}


TEST(TuiParagraphTest, WrapsByDisplayWidthForWideCharacters) {
    const element el = paragraph(string("中文测试内容"), 4);

    ASSERT_EQ(el.kind(), element::kind::vbox);
    ASSERT_EQ(el.children().size(), 3U);
    EXPECT_EQ(el.children()[0].text(), "中文");
    EXPECT_EQ(el.children()[1].text(), "测试");
    EXPECT_EQ(el.children()[2].text(), "内容");
}

TEST(TuiParagraphTest, BreaksOverlongWordAcrossLines) {
    const element el = paragraph(string("abcdefghij"), 4);

    ASSERT_EQ(el.kind(), element::kind::vbox);
    ASSERT_EQ(el.children().size(), 3U);
    EXPECT_EQ(el.children()[0].text(), "abcd");
    EXPECT_EQ(el.children()[1].text(), "efgh");
    EXPECT_EQ(el.children()[2].text(), "ij  ");
}

TEST(TuiParagraphTest, KeepsExplicitNewlines) {
    const element el = paragraph(string("ab\ncd"), 0);

    ASSERT_EQ(el.children().size(), 2U);
    EXPECT_EQ(el.children()[0].text(), "ab");
    EXPECT_EQ(el.children()[1].text(), "cd");
}
