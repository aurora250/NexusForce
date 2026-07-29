#include <NeForce/tui/component/component.hpp>
#include <NeForce/tui/component/containers.hpp>
#include <NeForce/tui/component/menu.hpp>
#include <NeForce/tui/component/radiobox.hpp>
#include <NeForce/tui/component/toggle.hpp>
#include <NeForce/tui/component/modal.hpp>
#include <NeForce/tui/component/renderer.hpp>
#include <NeForce/tui/component/dropdown.hpp>
#include <NeForce/tui/component/animation.hpp>
#include <NeForce/tui/dom/element.hpp>
#include <NeForce/tui/dom/layout.hpp>
#include <NeForce/tui/dom/linear_gradient.hpp>
#include <NeForce/tui/dom/ref.hpp>
#include <NeForce/tui/dom/style.hpp>
#include <NeForce/tui/dom/gauge.hpp>
#include <NeForce/tui/dom/spinner.hpp>
#include <NeForce/tui/dom/paragraph.hpp>
#include <NeForce/tui/event_dispatcher.hpp>
#include <NeForce/tui/focus_manager.hpp>
#include <NeForce/tui/renderer.hpp>
#include <NeForce/tui/screen.hpp>
#include <NeForce/tui/events.hpp>
#include <gtest/gtest.h>
using namespace neforce;
using namespace neforce::tui;
using namespace neforce::tui::components;

namespace {
    struct TestComponent : component<> {
        int renderCount = 0;

        element render() override {
            ++renderCount;
            return element::text("hello");
        }

        template <typename T>
        state<T>& createPublicState(T initial) {
            return this->create_state<T>(_NEFORCE move(initial));
        }

        void setStrand(strand& s) { strand_ = &s; }
        void setCtx(io_context& c) { ctx_ = &c; }
    };
} // namespace


TEST(TuiStyleTest, MergeFgOverrides) {
    tui::style base;
    base.fg = color::white();
    base.bg = color::black();

    tui::style over;
    over.fg = color::red();

    auto merged = tui::style::merge(base, over);
    EXPECT_EQ(merged.fg.value(), color::red());
    EXPECT_EQ(merged.bg.value(), color::black());
}

TEST(TuiStyleTest, MergeBoolFlagsPreserve) {
    tui::style base;
    base.bold = true;

    tui::style over;
    over.underline = true;

    auto merged = tui::style::merge(base, over);
    EXPECT_TRUE(merged.bold.value_or(false));
    EXPECT_TRUE(merged.underline.value_or(false));
}

TEST(TuiStyleTest, MergePaddingOverrides) {
    tui::style base;
    using padding = struct style::padding;
    base.padding = padding{1, 1, 1, 1};

    tui::style over;
    over.padding = padding{2, 3, 4, 5};

    auto merged = tui::style::merge(base, over);
    EXPECT_EQ(merged.padding->top, 2);
    EXPECT_EQ(merged.padding->right, 3);
    EXPECT_EQ(merged.padding->bottom, 4);
    EXPECT_EQ(merged.padding->left, 5);
}

TEST(TuiStyleTest, ThemeButtonPrimary) {
    auto s = dark_theme.button_style(style::variant::primary);
    EXPECT_EQ(s.bg.value(), color::cyan());
    EXPECT_TRUE(s.bold.value_or(false));
}

TEST(TuiStyleTest, ThemeButtonDanger) {
    auto s = dark_theme.button_style(style::variant::danger);
    EXPECT_EQ(s.fg.value(), color::red());
    EXPECT_TRUE(s.bold.value_or(false));
}

TEST(TuiStyleTest, ThemeInputFocused) {
    auto s = dark_theme.input_style(true);
    EXPECT_EQ(*s.border, style::border::single);
    EXPECT_EQ(s.borderColor.value(), color::cyan());
}

TEST(TuiStyleTest, ThemeInputUnfocused) {
    auto s = dark_theme.input_style(false);
    EXPECT_EQ(*s.border, style::border::single);
    EXPECT_EQ(s.borderColor.value(), dark_theme.border);
}

TEST(TuiElementTest, EmptyElement) {
    auto el = tui::element::empty();
    EXPECT_EQ(el.kind(), element::kind::empty);
}

TEST(TuiElementTest, TextElement) {
    auto el = tui::element::text("hello");
    EXPECT_EQ(el.kind(), element::kind::text);
    EXPECT_EQ(el.text(), "hello");
}

TEST(TuiElementTest, TextElementWithStyle) {
    tui::style s;
    s.fg = color::red();
    s.bold = true;

    auto el = tui::element::text("styled", s);
    EXPECT_EQ(el.style().fg.value(), color::red());
    EXPECT_TRUE(el.style().bold.value_or(false));
}

TEST(TuiElementTest, ButtonElement) {
    int calls = 0;
    auto el = tui::element::button("Click", [&] { ++calls; });
    EXPECT_EQ(el.kind(), element::kind::button);
    EXPECT_EQ(el.text(), "Click");

    el.on_click()();
    EXPECT_EQ(calls, 1);
}

TEST(TuiElementTest, ButtonWithVariant) {
    auto el = tui::element::button("OK", [] {}, {}, style::variant::danger);
    EXPECT_EQ(el.kind(), element::kind::button);
}

TEST(TuiElementTest, SpacerElement) {
    auto el = tui::element::spacer(3);
    EXPECT_EQ(el.kind(), element::kind::spacer);
    EXPECT_EQ(el.flex(), 3);
}

TEST(TuiElementTest, SeparatorElement) {
    auto el = tui::element::separator();
    EXPECT_EQ(el.kind(), element::kind::separator);
}

TEST(TuiElementTest, VBoxStoresChildren) {
    auto child1 = tui::element::text("a");
    auto child2 = tui::element::text("b");
    vector<tui::element> children;
    children.push_back(child1);
    children.push_back(child2);

    auto el = tui::element::vbox(_NEFORCE move(children));
    EXPECT_EQ(el.kind(), element::kind::vbox);
    EXPECT_EQ(el.children().size(), 2U);
    EXPECT_EQ(el.layout().dir, style::direction::column);
}

TEST(TuiElementTest, HBoxDirectionIsRow) {
    auto el = tui::element::hbox({});
    EXPECT_EQ(el.kind(), element::kind::hbox);
    EXPECT_EQ(el.layout().dir, style::direction::row);
}

TEST(TuiElementTest, ZStackStoresChildren) {
    auto c1 = tui::element::text("layer1");
    auto c2 = tui::element::text("layer2");
    vector<tui::element> children;
    children.push_back(c1);
    children.push_back(c2);

    auto el = tui::element::zstack(_NEFORCE move(children));
    EXPECT_EQ(el.kind(), element::kind::zstack);
    EXPECT_EQ(el.children().size(), 2U);
}

TEST(TuiElementTest, WithKey) {
    auto el = tui::element::text("item").with_key(42);
    EXPECT_EQ(el.key(), 42U);
    EXPECT_EQ(el.kind(), element::kind::text);
}

TEST(TuiElementTest, WhenTrue) {
    auto el =
            tui::element::when(true, [] { return tui::element::text("yes"); }, [] { return tui::element::text("no"); });
    EXPECT_EQ(el.kind(), element::kind::when);
    ASSERT_EQ(el.children().size(), 1U);
    EXPECT_EQ(el.children()[0].kind(), element::kind::text);
    EXPECT_EQ(el.children()[0].text(), "yes");
}

TEST(TuiElementTest, WhenFalse) {
    auto el = tui::element::when(
            false, [] { return tui::element::text("yes"); }, [] { return tui::element::text("no"); });
    EXPECT_EQ(el.kind(), element::kind::when);
    ASSERT_EQ(el.children().size(), 1U);
    EXPECT_EQ(el.children()[0].kind(), element::kind::text);
    EXPECT_EQ(el.children()[0].text(), "no");
}

TEST(TuiElementTest, EachEmptyList) {
    vector<int> items;
    auto el = tui::element::each<int>(items, [](const int&, size_t) { return tui::element::text("x"); });
    EXPECT_EQ(el.kind(), element::kind::each);
    EXPECT_EQ(el.children().size(), 0U);
}

TEST(TuiElementTest, EachNonEmptyList) {
    vector<string> items = {"1", "2", "3"};
    auto el = tui::element::each<string>(items, [](const string& v, size_t) { return tui::element::text(v); });
    EXPECT_EQ(el.kind(), element::kind::each);
    EXPECT_EQ(el.children().size(), 3U);
    EXPECT_EQ(el.children()[0].text(), "1");
    EXPECT_EQ(el.children()[1].text(), "2");
    EXPECT_EQ(el.children()[2].text(), "3");
}

TEST(TuiElementTest, ScrollViewStoresChild) {
    auto inner = tui::element::text("content");
    auto el = tui::element::scroll_view(inner);
    EXPECT_EQ(el.kind(), element::kind::scroll_view);
    EXPECT_EQ(el.children().size(), 1U);
    EXPECT_EQ(el.children()[0].text(), "content");
}

TEST(TuiElementTest, CopyElement) {
    auto el1 = tui::element::text("original");
    auto el2 = el1;
    EXPECT_EQ(el1.text(), el2.text());
    EXPECT_EQ(el2.kind(), element::kind::text);
}

TEST(TuiElementTest, MoveElement) {
    auto el1 = tui::element::text("move");
    auto el2 = _NEFORCE move(el1);
    EXPECT_EQ(el2.kind(), element::kind::text);
    EXPECT_EQ(el2.text(), "move");
}

TEST(TuiLayoutTest, EmptyElement) {
    auto el = tui::element::empty();
    auto result = compute_layout(el, 80, 24);
    EXPECT_EQ(result.size(), 0U);
}

TEST(TuiLayoutTest, SingleText) {
    auto el = tui::element::text("hello");
    auto result = compute_layout(el, 80, 24);
    ASSERT_EQ(result.size(), 1U);
    EXPECT_EQ(result[0].w, 5);
    EXPECT_EQ(result[0].h, 1);
}

TEST(TuiLayoutTest, VBoxMultipleChildren) {
    auto c1 = tui::element::text("line1");
    auto c2 = tui::element::text("line2");
    vector<tui::element> children;
    children.push_back(c1);
    children.push_back(c2);

    auto el = tui::element::vbox(_NEFORCE move(children));
    auto result = compute_layout(el, 80, 24);
    ASSERT_EQ(result.size(), 2U);
    EXPECT_EQ(result[0].h, 1);
    EXPECT_EQ(result[1].h, 1);
    EXPECT_EQ(result[0].y + result[0].h, result[1].y);
}

TEST(TuiLayoutTest, VBoxWithGap) {
    auto c1 = tui::element::text("a");
    auto c2 = tui::element::text("b");
    vector<tui::element> children;
    children.push_back(c1);
    children.push_back(c2);

    tui::box_props props;
    props.gap = 2;

    auto el = tui::element::vbox(_NEFORCE move(children), props);
    auto result = compute_layout(el, 80, 24);
    ASSERT_EQ(result.size(), 2U);
    EXPECT_EQ(result[0].y + result[0].h + 2, result[1].y);
}

TEST(TuiLayoutTest, HBoxMultipleChildren) {
    auto c1 = tui::element::text("ab");
    auto c2 = tui::element::text("cd");
    vector<tui::element> children;
    children.push_back(c1);
    children.push_back(c2);

    auto el = tui::element::hbox(_NEFORCE move(children));
    auto result = compute_layout(el, 80, 24);
    ASSERT_EQ(result.size(), 2U);
    EXPECT_EQ(result[0].h, 1);
    EXPECT_EQ(result[1].h, 1);
    EXPECT_EQ(result[0].x + result[0].w, result[1].x);
}

TEST(TuiLayoutTest, HBoxWithGap) {
    auto c1 = tui::element::text("a");
    auto c2 = tui::element::text("b");
    vector<tui::element> children;
    children.push_back(c1);
    children.push_back(c2);

    tui::box_props props;
    props.gap = 3;

    auto el = tui::element::hbox(_NEFORCE move(children), props);
    auto result = compute_layout(el, 80, 24);
    ASSERT_EQ(result.size(), 2U);
    EXPECT_EQ(result[0].x + result[0].w + 3, result[1].x);
}

TEST(TuiLayoutTest, NestedLayout) {
    auto inner1 = tui::element::text("1");
    auto inner2 = tui::element::text("2");
    vector<tui::element> innerChildren;
    innerChildren.push_back(inner1);
    innerChildren.push_back(inner2);

    auto hbox = tui::element::hbox(_NEFORCE move(innerChildren));
    auto outerText = tui::element::text("top");

    vector<tui::element> outerChildren;
    outerChildren.push_back(outerText);
    outerChildren.push_back(hbox);

    auto vbox = tui::element::vbox(_NEFORCE move(outerChildren));
    auto result = compute_layout(vbox, 80, 24);
    ASSERT_EQ(result.size(), 3U);
}

TEST(TuiLayoutTest, SpacerElement) {
    auto c1 = tui::element::text("a");
    auto sp = tui::element::spacer(1);
    auto c2 = tui::element::text("b");
    vector<tui::element> children;
    children.push_back(c1);
    children.push_back(sp);
    children.push_back(c2);

    auto el = tui::element::hbox(_NEFORCE move(children));
    auto result = compute_layout(el, 80, 24);
    ASSERT_EQ(result.size(), 3U);
    EXPECT_GT(result[1].w, 0);
}

TEST(TuiLayoutTest, PaddingOffsetsChildren) {
    auto inner = tui::element::text("hello");
    vector<tui::element> children;
    children.push_back(inner);

    tui::box_props props;
    using padding = struct style::padding;
    props.padding = padding{2, 3, 1, 4};

    auto el = tui::element::vbox(_NEFORCE move(children), props);
    auto result = compute_layout(el, 80, 24);
    ASSERT_EQ(result.size(), 1U);
    EXPECT_EQ(result[0].x, 4);
    EXPECT_EQ(result[0].y, 2);
}

TEST(TuiLayoutTest, FixedWidth) {
    tui::style s;
    s.width = style::size_hint{style::size_hint::fixed, 10};

    auto el = tui::element::text("hello", s);
    auto result = compute_layout(el, 80, 24);
    ASSERT_EQ(result.size(), 1U);
    EXPECT_EQ(result[0].w, 10);
}

TEST(TuiEventTest, KeyEventPrintable) {
    tui::key_event e;
    e.key = tui::key_event::type::printable;
    e.cp = U'A';
    EXPECT_TRUE(e.is_printable());
    EXPECT_EQ(e.cp, U'A');
}

TEST(TuiEventTest, KeyEventSpecial) {
    tui::key_event e;
    e.key = tui::key_event::type::escape;
    EXPECT_FALSE(e.is_printable());
}

TEST(TuiEventTest, KeyEventWithModifier) {
    tui::key_event e;
    e.key = tui::key_event::type::printable;
    e.cp = U'c';
    e.mods = tui::key_modifier::ctrl;
    EXPECT_TRUE(e.is_printable());
    EXPECT_EQ(e.mods, tui::key_modifier::ctrl);
}

TEST(TuiEventTest, MouseEventDefaults) {
    tui::mouse_event e;
    EXPECT_EQ(e.button, tui::mouse_button::none);
    EXPECT_EQ(e.action, tui::mouse_action::move);
    EXPECT_EQ(e.x, 0);
    EXPECT_EQ(e.y, 0);
}

TEST(TuiComponentTest, EmptyPropsDefault) {
    TestComponent comp;
    EXPECT_EQ(comp.parent(), nullptr);
}

TEST(TuiComponentTest, OnKeyReturnsFalseByDefault) {
    TestComponent comp;
    tui::key_event e;
    EXPECT_FALSE(comp.on_key(e));
}

TEST(TuiComponentTest, OnMouseReturnsFalseByDefault) {
    TestComponent comp;
    tui::mouse_event e;
    EXPECT_FALSE(comp.on_mouse(e));
}

TEST(TuiComponentTest, RenderCreatesElement) {
    TestComponent comp;
    auto el = comp.render();
    EXPECT_EQ(el.kind(), element::kind::text);
    EXPECT_EQ(el.text(), "hello");
}

TEST(TuiStateTest, InitialValue) {
    io_context ctx;
    strand s{ctx};
    TestComponent comp;
    auto& state = comp.createPublicState<int>(7);
    EXPECT_EQ(state.value(), 7);
    EXPECT_EQ(*state, 7);
}

TEST(TuiStateTest, OperatorEqualsChangesValue) {
    io_context ctx;
    strand s{ctx};
    TestComponent comp;
    auto& state = comp.createPublicState<int>(1);
    comp.setStrand(s);
    comp.setCtx(ctx);

    state = 42;
    EXPECT_EQ(state.value(), 42);
}

TEST(TuiStateTest, ModifyChangesValue) {
    io_context ctx;
    strand s{ctx};
    TestComponent comp;
    auto& state = comp.createPublicState<_NEFORCE string>("hello");
    comp.setStrand(s);
    comp.setCtx(ctx);

    state.modify([](_NEFORCE string& v) { v += " world"; });
    EXPECT_EQ(state.value(), "hello world");
}

TEST(TuiStateTest, SetQuietAndNotify) {
    io_context ctx;
    strand s{ctx};
    TestComponent comp;
    auto& state = comp.createPublicState<int>(0);
    comp.setStrand(s);
    comp.setCtx(ctx);

    state.set_quiet(100);
    EXPECT_EQ(state.value(), 100);
}

TEST(TuiStateTest, OnChangeSignal) {
    io_context ctx;
    strand s{ctx};
    TestComponent comp;
    auto& state = comp.createPublicState<int>(0);
    comp.setStrand(s);
    comp.setCtx(ctx);

    int received = -1;
    state.on_change([&](const int& v) { received = v; });

    state = 99;
    EXPECT_EQ(received, 99);
}

TEST(TuiStateTest, BoolState) {
    io_context ctx;
    strand s{ctx};
    TestComponent comp;
    auto& checked = comp.createPublicState<bool>(false);
    comp.setStrand(s);
    comp.setCtx(ctx);

    EXPECT_FALSE(checked.value());
    checked = true;
    EXPECT_TRUE(checked.value());
}

TEST(TuiDecoratorTest, PipeOperatorAppliesDecorator) {
    decorator d = bold();
    auto el = element::text("hi") | d;
    EXPECT_TRUE(el.style().bold.value_or(false));
}

TEST(TuiDecoratorTest, PipeAssignModifiesInPlace) {
    auto el = element::text("hi");
    el |= bold();
    EXPECT_TRUE(el.style().bold.value_or(false));
}

TEST(TuiDecoratorTest, DecoratorComposition) {
    auto both = bold() | tui::color(color::red());
    auto el = element::text("styled") | both;
    EXPECT_TRUE(el.style().bold.value_or(false));
    EXPECT_EQ(el.style().fg.value(), color::red());
}

TEST(TuiDecoratorTest, StyleDecoratorBold) {
    auto el = element::text("bold") | bold();
    EXPECT_TRUE(el.style().bold.value_or(false));
}

TEST(TuiDecoratorTest, StyleDecoratorDim) {
    auto el = element::text("dim") | dim();
    EXPECT_TRUE(el.style().dim.value_or(false));
}

TEST(TuiDecoratorTest, StyleDecoratorItalic) {
    auto el = element::text("italic") | italic();
    EXPECT_TRUE(el.style().italic.value_or(false));
}

TEST(TuiDecoratorTest, StyleDecoratorUnderlined) {
    auto el = element::text("ul") | underlined();
    EXPECT_TRUE(el.style().underline.value_or(false));
}

TEST(TuiDecoratorTest, StyleDecoratorUnderlinedDouble) {
    auto el = element::text("ul2") | underlined_double();
    EXPECT_TRUE(el.style().underlined_double.value_or(false));
}

TEST(TuiDecoratorTest, StyleDecoratorBlink) {
    auto el = element::text("blink") | blink();
    EXPECT_TRUE(el.style().blink.value_or(false));
}

TEST(TuiDecoratorTest, StyleDecoratorStrikethrough) {
    auto el = element::text("strike") | strikethrough();
    EXPECT_TRUE(el.style().strikethrough.value_or(false));
}

TEST(TuiDecoratorTest, StyleDecoratorInverted) {
    auto el = element::text("inv") | inverted();
    EXPECT_TRUE(el.style().reverse.value_or(false));
}

TEST(TuiDecoratorTest, StyleDecoratorColor) {
    auto el = element::text("red") | tui::color(color::red());
    EXPECT_EQ(el.style().fg.value(), color::red());
}

TEST(TuiDecoratorTest, StyleDecoratorBgcolor) {
    auto el = element::text("bg") | bgcolor(color::blue());
    EXPECT_EQ(el.style().bg.value(), color::blue());
}

TEST(TuiDecoratorTest, StyleDecoratorBorder) {
    auto el = element::text("border") | border(style::border::double_);
    EXPECT_EQ(el.style().border.value(), style::border::double_);
}

TEST(TuiDecoratorTest, ChainMultipleDecorators) {
    auto el = element::text("multi") | bold() | tui::color(color::green()) | underlined();
    EXPECT_TRUE(el.style().bold.value_or(false));
    EXPECT_EQ(el.style().fg.value(), color::green());
    EXPECT_TRUE(el.style().underline.value_or(false));
}

TEST(TuiWithStyleTest, MergesForeground) {
    auto el = element::text("test");
    style s;
    s.fg = color::red();
    el.with_style(s);
    EXPECT_EQ(el.style().fg.value(), color::red());
}

TEST(TuiWithStyleTest, DoesNotOverwriteExistingUnrelatedFields) {
    auto el = element::text("test");
    style base;
    base.bold = true;
    el.with_style(base);

    style over;
    over.italic = true;
    el.with_style(over);

    EXPECT_TRUE(el.style().bold.value_or(false));
    EXPECT_TRUE(el.style().italic.value_or(false));
}

TEST(TuiRefTest, OwnsValueByDefault) {
    tui::ref<int> r(42);
    EXPECT_EQ(*r, 42);
}

TEST(TuiRefTest, BorrowsPointer) {
    int x = 99;
    tui::ref<int> r(&x);
    EXPECT_EQ(*r, 99);
    *r = 100;
    EXPECT_EQ(x, 100);
}

TEST(TuiRefTest, AssignThroughRef) {
    int x = 5;
    tui::ref<int> r(&x);
    r = 10;
    EXPECT_EQ(x, 10);
}

TEST(TuiRefTest, HoldsPointerFlag) {
    int x = 0;
    tui::ref<int> owned(1);
    tui::ref<int> borrowed(&x);
    EXPECT_FALSE(owned.holds_pointer());
    EXPECT_TRUE(borrowed.holds_pointer());
}

TEST(TuiRefTest, StringRefFromCStr) {
    string_ref sr("hello");
    EXPECT_EQ(*sr, "hello");
}

TEST(TuiRefTest, ConstRefReadOnly) {
    int x = 7;
    const_ref<int> cr(&x);
    EXPECT_EQ(*cr, 7);
    EXPECT_TRUE(cr.holds_pointer());
}

TEST(TuiScreenTest, SurfaceDimensions) {
    surface s(80, 24);
    EXPECT_EQ(s.dimx(), 80);
    EXPECT_EQ(s.dimy(), 24);
}

TEST(TuiScreenTest, SurfaceCellAccess) {
    surface s(10, 5);
    s.cell_at(3, 2).character = "X";
    EXPECT_EQ(s.cell_at(3, 2).character, "X");
}

TEST(TuiScreenTest, SurfaceClearResetsCells) {
    surface s(5, 3);
    s.cell_at(1, 1).character = "A";
    s.cell_at(1, 1).bold = true;
    s.clear();
    EXPECT_EQ(s.cell_at(1, 1).character, " ");
    EXPECT_FALSE(s.cell_at(1, 1).bold);
}

TEST(TuiScreenTest, CellVisuallyEqual) {
    cell a, b;
    a.character = "X";
    b.character = "X";
    EXPECT_TRUE(a.visually_equal(b));

    b.bold = true;
    EXPECT_FALSE(a.visually_equal(b));
}

TEST(TuiScreenTest, ScreenCursorDefault) {
    screen scr(10, 10);
    EXPECT_EQ(scr.cursor().x, 0);
    EXPECT_EQ(scr.cursor().y, 0);
}

TEST(TuiScreenTest, ScreenSetCursor) {
    screen scr(10, 10);
    cursor c;
    c.x = 5;
    c.y = 3;
    scr.set_cursor(c);
    EXPECT_EQ(scr.cursor().x, 5);
    EXPECT_EQ(scr.cursor().y, 3);
}

TEST(TuiScreenTest, ScreenResize) {
    screen scr(10, 10);
    scr.resize(120, 40);
    EXPECT_EQ(scr.dimx(), 120);
    EXPECT_EQ(scr.dimy(), 40);
}

TEST(TuiScreenTest, ScreenHyperlink) {
    screen scr(10, 10);
    auto id = scr.register_hyperlink("https://example.com");
    EXPECT_GT(id, 0u);
    EXPECT_EQ(scr.hyperlink(id), "https://example.com");
}

TEST(TuiScreenTest, ScreenClearResetsCursor) {
    screen scr(10, 10);
    scr.set_cursor(cursor{3, 5});
    scr.clear();
    EXPECT_EQ(scr.cursor().x, 0);
    EXPECT_EQ(scr.cursor().y, 0);
}

namespace {
    struct FocusTestComp : component_base {
        element render() override { return element::text("f"); }
        bool focusable() const override { return true; }
        void collectPublicFocusable(vector<component_base*>& out) { collect_focusable(out); }
    };
} // namespace

TEST(TuiFocusTest, RootActiveChildNullByDefault) {
    auto root = make_unique<FocusTestComp>();
    EXPECT_EQ(root->active_child(), nullptr);
}

TEST(TuiFocusTest, AddChildOwnership) {
    auto parent = make_unique<FocusTestComp>();
    auto child = make_unique<FocusTestComp>();
    auto* raw_child = child.get();
    parent->add_child(move(child));
    EXPECT_EQ(parent->child_count(), 1u);
    EXPECT_EQ(parent->child_at(0), raw_child);
    EXPECT_EQ(raw_child->parent(), parent.get());
}

TEST(TuiFocusTest, FocusedTrueForRoot) {
    auto root = make_unique<FocusTestComp>();
    EXPECT_TRUE(root->focused());
}

TEST(TuiFocusTest, ActiveAfterSetActiveChild) {
    auto parent = make_unique<FocusTestComp>();
    auto child = make_unique<FocusTestComp>();
    auto* raw = child.get();
    parent->add_child(move(child));
    parent->set_active_child(raw);
    EXPECT_EQ(parent->active_child(), raw);
    EXPECT_TRUE(raw->active());
}

TEST(TuiFocusTest, TakeFocusWalksUp) {
    auto grandparent = make_unique<FocusTestComp>();
    auto parent = make_unique<FocusTestComp>();
    auto child = make_unique<FocusTestComp>();
    auto* raw_parent = parent.get();
    auto* raw_child = child.get();

    parent->add_child(move(child));
    grandparent->add_child(move(parent));

    raw_child->take_focus();
    EXPECT_EQ(raw_parent->active_child(), raw_child);
    EXPECT_EQ(grandparent->active_child(), raw_parent);
}

TEST(TuiFocusTest, FocusedChildAfterTakeFocus) {
    auto parent = make_unique<FocusTestComp>();
    auto child = make_unique<FocusTestComp>();
    auto* raw_child = child.get();
    parent->add_child(move(child));
    raw_child->take_focus();
    EXPECT_TRUE(raw_child->focused());
}

TEST(TuiFocusTest, RemoveChildReturnsOwnership) {
    auto parent = make_unique<FocusTestComp>();
    auto child = make_unique<FocusTestComp>();
    auto* raw_child = child.get();
    parent->add_child(move(child));

    auto returned = parent->remove_child(raw_child);
    EXPECT_EQ(returned.get(), raw_child);
    EXPECT_EQ(parent->child_count(), 0u);
    EXPECT_EQ(raw_child->parent(), nullptr);
}

TEST(TuiFocusTest, DetachReturnsSelf) {
    auto parent = make_unique<FocusTestComp>();
    auto child = make_unique<FocusTestComp>();
    auto* raw_child = child.get();
    parent->add_child(move(child));

    auto detached = raw_child->detach();
    EXPECT_EQ(detached.get(), raw_child);
    EXPECT_EQ(parent->child_count(), 0u);
}

TEST(TuiFocusTest, DetachAllChildren) {
    auto parent = make_unique<FocusTestComp>();
    parent->add_child(make_unique<FocusTestComp>());
    parent->add_child(make_unique<FocusTestComp>());
    parent->detach_all_children();
    EXPECT_EQ(parent->child_count(), 0u);
}

TEST(TuiFocusTest, NonFocusableComponent) {
    struct NonFocusable : component_base {
        element render() override { return element::text("nf"); }
    };
    auto comp = make_unique<NonFocusable>();
    EXPECT_FALSE(comp->focusable());
}

TEST(TuiFocusTest, CollectFocusableSkipsNonFocusable) {
    auto parent = make_unique<FocusTestComp>();
    struct NonFocusable : component_base {
        element render() override { return element::text("nf"); }
    };
    auto nf = make_unique<NonFocusable>();
    auto* raw_nf = nf.get();
    parent->add_child(move(nf));

    vector<component_base*> chain;
    parent->collectPublicFocusable(chain);
    EXPECT_EQ(chain.size(), 1u);
    EXPECT_EQ(chain[0], parent.get());
    (void) raw_nf;
}

TEST(TuiFocusTest, IndexInParent) {
    auto parent = make_unique<FocusTestComp>();
    auto c0 = make_unique<FocusTestComp>();
    auto c1 = make_unique<FocusTestComp>();
    auto* raw0 = c0.get();
    auto* raw1 = c1.get();
    parent->add_child(move(c0));
    parent->add_child(move(c1));
    EXPECT_EQ(raw0->index(), 0);
    EXPECT_EQ(raw1->index(), 1);
}

TEST(TuiContainerTest, VerticalCreatesChildren) {
    auto c1 = make_unique<FocusTestComp>();
    auto c2 = make_unique<FocusTestComp>();
    vector<unique_ptr<component_base>> children;
    children.push_back(move(c1));
    children.push_back(move(c2));
    auto v = container::vertical(move(children));
    EXPECT_EQ(v->child_count(), 2u);
}

TEST(TuiContainerTest, HorizontalCreatesChildren) {
    auto c1 = make_unique<FocusTestComp>();
    vector<unique_ptr<component_base>> children;
    children.push_back(move(c1));
    auto h = container::horizontal(move(children));
    EXPECT_EQ(h->child_count(), 1u);
    EXPECT_TRUE(h->focusable());
}

TEST(TuiContainerTest, TabWithExternalSelector) {
    int selected = 1;
    auto c0 = make_unique<FocusTestComp>();
    auto c1 = make_unique<FocusTestComp>();
    auto c2 = make_unique<FocusTestComp>();
    vector<unique_ptr<component_base>> children;
    children.push_back(move(c0));
    children.push_back(move(c1));
    children.push_back(move(c2));
    auto tab = container::tab(move(children), &selected);
    EXPECT_EQ(tab->child_count(), 3u);
}

TEST(TuiRendererTest, StandaloneRenderer) {
    using components::renderer;
    auto r = renderer([] { return element::text("custom"); });
    auto el = r->render();
    EXPECT_EQ(el.text(), "custom");
}

TEST(TuiRendererTest, RendererWithChild) {
    using components::renderer;
    auto child = make_unique<FocusTestComp>();
    auto r = renderer(move(child), [] { return element::text("wrapped"); });
    auto el = r->render();
    EXPECT_EQ(el.text(), "wrapped");
}

TEST(TuiMenuTest, RendersAllEntries) {
    menu_option opt;
    opt.entries = {"Alice", "Bob", "Charlie"};
    int sel = 0;
    opt.selected = &sel;
    auto menu = components::menu(opt);
    auto el = menu->render();
    EXPECT_EQ(el.kind(), element::kind::vbox);
    EXPECT_EQ(el.children().size(), 3u);
}

TEST(TuiMenuTest, NavigateDown) {
    menu_option opt;
    opt.entries = {"A", "B"};
    int sel = 0;
    opt.selected = &sel;

    io_context ctx;
    strand s{ctx};
    auto menu = components::menu(opt);

    key_event down;
    down.key = key_event::type::down;
    menu->on_key(down);
    EXPECT_EQ(sel, 1);
}

TEST(TuiMenuTest, NavigateUpWraps) {
    menu_option opt;
    opt.entries = {"A", "B"};
    int sel = 0;
    opt.selected = &sel;

    auto menu = components::menu(opt);

    key_event up;
    up.key = key_event::type::up;
    menu->on_key(up);
    EXPECT_EQ(sel, 1);
}

TEST(TuiRadioboxTest, RendersEntries) {
    radiobox_option opt;
    opt.entries = {"One", "Two", "Three"};
    auto rb = components::radiobox(opt);
    auto el = rb->render();
    EXPECT_EQ(el.children().size(), 3u);
}

TEST(TuiToggleTest, CycleForward) {
    toggle_option opt;
    opt.entries = {"On", "Off"};
    int sel = 0;
    opt.selected = &sel;
    auto tog = components::toggle(opt);

    key_event enter;
    enter.key = key_event::type::enter;
    tog->on_key(enter);
    EXPECT_EQ(sel, 1);
}

TEST(TuiDropdownTest, InitiallyCollapsed) {
    dropdown_option opt;
    opt.entries = {"A", "B", "C"};
    bool open = false;
    opt.open = &open;
    auto dd = components::dropdown(opt);
    EXPECT_TRUE(dd->focusable());
}

TEST(TuiModalTest, ShowsMainWhenClosed) {
    bool show = false;
    auto main = make_unique<FocusTestComp>();
    auto overlay = make_unique<FocusTestComp>();
    auto* main_raw = main.get();
    auto* overlay_raw = overlay.get();
    auto modal = components::modal(move(main), move(overlay), &show);

    EXPECT_EQ(modal->child_count(), 2u);
    EXPECT_EQ(modal->child_at(0), main_raw);
    EXPECT_EQ(modal->child_at(1), overlay_raw);
}

TEST(TuiGaugeTest, HorizontalGauge) {
    auto g = gauge(0.5f, 20);
    EXPECT_EQ(g.kind(), element::kind::text);
    EXPECT_FALSE(g.text().empty());
}

TEST(TuiGaugeTest, GaugeDirectionUp) {
    auto g = gauge_direction(0.3f, gauge_direction::up, 8);
    EXPECT_EQ(g.kind(), element::kind::vbox);
    EXPECT_EQ(g.children().size(), 8u);
}

TEST(TuiSpinnerTest, CharsetFrames) {
    auto s = spinner(0, 1);
    EXPECT_EQ(s.kind(), element::kind::text);
}

TEST(TuiParagraphTest, BasicWrapping) {
    auto p = paragraph("hello world", 5);
    EXPECT_EQ(p.kind(), element::kind::vbox);
    EXPECT_GE(p.children().size(), 1u);
}

TEST(TuiParagraphTest, NoWrapWhenShorter) {
    auto p = paragraph("hi", 20);
    EXPECT_EQ(p.children().size(), 1u);
}

TEST(TuiAnimationTest, LinearEasing) {
    auto fn = easing::linear();
    EXPECT_FLOAT_EQ(fn(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(fn(1.0f), 1.0f);
    EXPECT_FLOAT_EQ(fn(0.5f), 0.5f);
}

TEST(TuiAnimationTest, QuadraticOutEasing) {
    auto fn = easing::quadratic_out();
    EXPECT_FLOAT_EQ(fn(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(fn(1.0f), 1.0f);
}

TEST(TuiAnimationTest, SineInOutEasing) {
    auto fn = easing::sine_in_out();
    EXPECT_NEAR(fn(0.5f), 0.5f, 0.01f);
}

TEST(TuiAnimationTest, AnimatorReachesTarget) {
    float value = 0.0f;
    animator anim(&value, 100.0f, 100_ms);
    EXPECT_FALSE(anim.is_done());

    anim.on_animation(100_ms);
    EXPECT_TRUE(anim.is_done());
    EXPECT_FLOAT_EQ(value, 100.0f);
}

TEST(TuiAnimationTest, AnimatorRespectsDelay) {
    float value = 0.0f;
    animator anim(&value, 100.0f, 100_ms, easing::linear(), 50_ms);

    anim.on_animation(30_ms);
    EXPECT_FLOAT_EQ(value, 0.0f);
    EXPECT_FALSE(anim.is_done());
}

TEST(TuiAnimationTest, AnimatorReset) {
    float value = 0.0f;
    animator anim(&value, 50.0f, 100_ms);
    anim.on_animation(100_ms);
    anim.reset(100.0f, 200_ms);
    EXPECT_EQ(anim.to(), 100.0f);
    EXPECT_FALSE(anim.is_done());
}

TEST(TuiScreenPlaintextTest, EmptyScreen) {
    screen s(10, 5);
    s.clear();
    string plain = s.to_plaintext();
    EXPECT_TRUE(plain.find_first_not_of('\n') == string::npos);
}

TEST(TuiScreenPlaintextTest, SingleChar) {
    screen s(10, 3);
    s.clear();
    s.fast_cell_at(0, 0).character = "X";
    string plain = s.to_plaintext();
    EXPECT_EQ(plain, "X");
}

TEST(TuiScreenPlaintextTest, MultiLine) {
    screen s(10, 4);
    s.clear();
    s.fast_cell_at(2, 0).character = "A";
    s.fast_cell_at(5, 2).character = "B";
    string plain = s.to_plaintext();
    EXPECT_NE(plain.find("A"), string::npos);
    EXPECT_NE(plain.find("B"), string::npos);
}

TEST(TuiScreenPlaintextTest, TrailingSpacesTrimmed) {
    screen s(20, 2);
    s.clear();
    s.fast_cell_at(0, 0).character = "H";
    s.fast_cell_at(1, 0).character = "i";
    string plain = s.to_plaintext();
    EXPECT_EQ(plain, "Hi");
}

TEST(TuiScreenPlaintextTest, AutomergedCellsSkipped) {
    screen s(10, 2);
    s.clear();
    s.fast_cell_at(0, 0).character = "\xe4\xb8\xad";
    s.fast_cell_at(1, 0).automerge = true;
    string plain = s.to_plaintext();
    EXPECT_EQ(plain, "\xe4\xb8\xad");
}

TEST(TuiRendererTest, RenderSingleText) {
    using tui::renderer;
    screen scr(80, 24);
    scr.clear();
    renderer r(scr, dark_theme, 80, 24);
    auto tree = element::text("hello");
    auto layout = compute_layout(tree, 80, 24);
    r.render(tree, layout, false);
    EXPECT_EQ(scr.fast_cell_at(0, 0).character, "h");
    EXPECT_EQ(scr.fast_cell_at(1, 0).character, "e");
}

TEST(TuiRendererTest, RenderTextBlockNoWrap) {
    using tui::renderer;
    screen scr(40, 10);
    scr.clear();
    renderer r(scr, dark_theme, 40, 10);
    int end_x = 0, end_y = 0;
    r.render_text_block(0, 0, 10, 2, "hello world", style{}, style::wrap_mode::none, &end_x, &end_y);
    EXPECT_GT(end_x, 0);
    EXPECT_EQ(end_y, 0);
}

TEST(TuiRendererTest, RenderTextBlockCharWrap) {
    using tui::renderer;
    screen scr(40, 10);
    scr.clear();
    renderer r(scr, dark_theme, 40, 10);
    int end_y = 0;
    r.render_text_block(0, 0, 3, 10, "abcdef", style{}, style::wrap_mode::character, nullptr, &end_y);
    EXPECT_GT(end_y, 0);
}

TEST(TuiRendererTest, RenderTextBlockWordWrap) {
    using tui::renderer;
    screen scr(40, 10);
    scr.clear();
    renderer r(scr, dark_theme, 40, 10);
    int end_y = 0;
    r.render_text_block(0, 0, 5, 10, "hello world foo", style{}, style::wrap_mode::word, nullptr, &end_y);
    EXPECT_GT(end_y, 0);
}

TEST(TuiRendererTest, ApplyBorderSingle) {
    using tui::renderer;
    screen scr(20, 10);
    scr.clear();
    renderer r(scr, dark_theme, 20, 10);
    r.apply_border(0, 0, 5, 3, style::border::single, color::white());
    EXPECT_NE(scr.fast_cell_at(0, 0).character, " ");
    EXPECT_NE(scr.fast_cell_at(4, 2).character, " ");
}

TEST(TuiRendererTest, ApplyBorderDouble) {
    using tui::renderer;
    screen scr(20, 10);
    scr.clear();
    renderer r(scr, dark_theme, 20, 10);
    r.apply_border(0, 0, 5, 3, style::border::double_, color::white());
    EXPECT_NE(scr.fast_cell_at(0, 0).character, " ");
}

TEST(TuiRendererTest, ApplyBorderRounded) {
    using tui::renderer;
    screen scr(20, 10);
    scr.clear();
    renderer r(scr, dark_theme, 20, 10);
    r.apply_border(0, 0, 5, 3, style::border::rounded, color::white());
    EXPECT_NE(scr.fast_cell_at(0, 0).character, " ");
}

TEST(TuiRendererTest, ApplyBorderNoneNoop) {
    using tui::renderer;
    screen scr(20, 10);
    scr.clear();
    renderer r(scr, dark_theme, 20, 10);
    r.apply_border(0, 0, 5, 3, style::border::none, color::white());
    EXPECT_EQ(scr.fast_cell_at(0, 0).character, " ");
}

TEST(TuiRendererTest, ApplyStyleToCellFg) {
    using tui::renderer;
    screen scr(1, 1);
    cell c;
    renderer r(scr, dark_theme, 1, 1);
    style s;
    s.fg = color::red();
    r.apply_style_to_cell(c, s);
    EXPECT_EQ(c.foreground, color::red());
}

TEST(TuiRendererTest, ApplyStyleToCellBold) {
    using tui::renderer;
    screen scr(1, 1);
    cell c;
    renderer r(scr, dark_theme, 1, 1);
    style s;
    s.bold = true;
    r.apply_style_to_cell(c, s);
    EXPECT_TRUE(c.bold);
}

TEST(TuiRendererTest, RenderButton) {
    using tui::renderer;
    screen scr(30, 10);
    scr.clear();
    renderer r(scr, dark_theme, 30, 10);
    r.render_button(0, 0, 10, 3, "OK", style{}, dark_theme, style::variant::primary);
    EXPECT_NE(scr.fast_cell_at(0, 0).character, " ");
}

TEST(TuiRendererTest, RenderCheckbox) {
    using tui::renderer;
    screen scr(20, 5);
    scr.clear();
    renderer r(scr, dark_theme, 20, 5);
    r.render_checkbox(0, 0, 10, 1, "Option", true, style{});
    string plain = scr.to_plaintext();
    EXPECT_NE(plain.find("[x]"), string::npos);
}

TEST(TuiRendererTest, RenderSeparator) {
    using tui::renderer;
    screen scr(20, 5);
    scr.clear();
    renderer r(scr, dark_theme, 20, 5);
    r.render_separator(0, 0, 10);
    EXPECT_EQ(scr.fast_cell_at(0, 0).character, "-");
    EXPECT_EQ(scr.fast_cell_at(5, 0).character, "-");
}

TEST(TuiRendererTest, RenderVBox) {
    using tui::renderer;
    screen scr(40, 10);
    scr.clear();
    renderer r(scr, dark_theme, 40, 10);
    auto tree = element::vbox({element::text("A"), element::text("B")});
    auto layout = compute_layout(tree, 40, 10);
    r.render(tree, layout, false);
    string plain = scr.to_plaintext();
    EXPECT_NE(plain.find("A"), string::npos);
    EXPECT_NE(plain.find("B"), string::npos);
}

TEST(TuiRendererTest, ScrollbarHitsCollected) {
    using tui::renderer;
    screen scr(40, 20);
    scr.clear();
    renderer r(scr, dark_theme, 40, 20);
    auto inner = element::vbox({element::text(string(100, 'x'))});
    auto sv = element::scroll_view(move(inner));
    auto layout = compute_layout(sv, 40, 20);
    r.render(sv, layout, false);
    EXPECT_FALSE(r.scrollbar_hits().empty());
}

TEST(TuiRendererTest, EmptyTreeNoop) {
    using tui::renderer;
    screen scr(40, 10);
    scr.clear();
    renderer r(scr, dark_theme, 40, 10);
    auto tree = element::empty();
    auto layout = compute_layout(tree, 40, 10);
    r.render(tree, layout, false);
    string plain = scr.to_plaintext();
    EXPECT_TRUE(plain.find_first_not_of('\n') == string::npos);
}

TEST(TuiFocusManagerTest, DefaultState) {
    focus_manager fm;
    EXPECT_EQ(fm.focused(), nullptr);
    EXPECT_TRUE(fm.chain().empty());
    EXPECT_TRUE(fm.is_chain_dirty());
}

TEST(TuiFocusManagerTest, SetFocus) {
    focus_manager fm;
    TestComponent comp;
    fm.set_focus(&comp);
    EXPECT_EQ(fm.focused(), &comp);
}

TEST(TuiFocusManagerTest, SetFocusSameNoop) {
    focus_manager fm;
    TestComponent comp;
    fm.set_focus(&comp);
    fm.set_focus(&comp);
    EXPECT_EQ(fm.focused(), &comp);
}

TEST(TuiFocusManagerTest, SetFocusNull) {
    focus_manager fm;
    TestComponent comp;
    fm.set_focus(&comp);
    fm.set_focus(nullptr);
    EXPECT_EQ(fm.focused(), nullptr);
}

TEST(TuiFocusManagerTest, MarkChainDirty) {
    focus_manager fm;
    EXPECT_TRUE(fm.is_chain_dirty());
    fm.mark_chain_dirty();
    EXPECT_TRUE(fm.is_chain_dirty());
}

TEST(TuiEventDispatcherTest, DispatchKeyTab) {
    focus_manager fm;
    event_dispatcher ed(fm);
    TestComponent root;

    bool dirty_marked = false;
    ed.set_dirty_callback([&] { dirty_marked = true; });

    key_event tab;
    tab.key = key_event::type::tab;
    bool handled = ed.dispatch_key(tab, &root);
    EXPECT_TRUE(handled);
    EXPECT_TRUE(dirty_marked);
}

TEST(TuiEventDispatcherTest, DispatchKeyShiftTab) {
    focus_manager fm;
    event_dispatcher ed(fm);
    TestComponent root;

    key_event stab;
    stab.key = key_event::type::tab_reverse;
    bool handled = ed.dispatch_key(stab, &root);
    EXPECT_TRUE(handled);
}

TEST(TuiEventDispatcherTest, DispatchKeyNullRoot) {
    focus_manager fm;
    event_dispatcher ed(fm);
    key_event ev;
    ev.key = key_event::type::tab;
    EXPECT_FALSE(ed.dispatch_key(ev, nullptr));
}

TEST(TuiEventDispatcherTest, DispatchMouseRelease) {
    focus_manager fm;
    event_dispatcher ed(fm);
    mouse_event me;
    me.action = mouse_action::release;
    me.x = 0;
    me.y = 0;
    vector<layout_rect> layout;
    element tree = element::empty();
    vector<scrollbar_hit> hits;
    EXPECT_FALSE(ed.dispatch_mouse(me, nullptr, layout, tree, hits));
}

TEST(TuiEventDispatcherTest, SetDirtyCallback) {
    focus_manager fm;
    event_dispatcher ed(fm);
    bool called = false;
    ed.set_dirty_callback([&] { called = true; });

    TestComponent root;
    key_event ev;
    ev.key = key_event::type::tab;
    ed.dispatch_key(ev, &root);
    EXPECT_TRUE(called);
}

TEST(TuiLayoutCacheTest, InitialDirty) {
    auto tree = element::text("hello");
    EXPECT_TRUE(tree.is_layout_dirty());
}

TEST(TuiLayoutCacheTest, CacheAfterCompute) {
    auto tree = element::text("hello");
    auto result = compute_layout(tree, 80, 24);
    EXPECT_FALSE(tree.is_layout_dirty());
    EXPECT_EQ(tree.cached_constraint_w(), 80);
    EXPECT_EQ(tree.cached_constraint_h(), 24);
}

TEST(TuiLayoutCacheTest, CacheReused) {
    auto tree = element::text("hello");
    auto r1 = compute_layout(tree, 80, 24);
    auto r2 = compute_layout(tree, 80, 24);
    EXPECT_EQ(r1.size(), r2.size());
    EXPECT_FALSE(tree.is_layout_dirty());
}

TEST(TuiLayoutCacheTest, ConstraintChangeInvalidates) {
    auto tree = element::text("hello");
    auto r1 = compute_layout(tree, 80, 24);
    EXPECT_FALSE(tree.is_layout_dirty());
    EXPECT_EQ(tree.cached_constraint_w(), 80);
    EXPECT_EQ(tree.cached_constraint_h(), 24);
    auto r2 = compute_layout(tree, 40, 10);
    EXPECT_FALSE(tree.is_layout_dirty());
    EXPECT_EQ(tree.cached_constraint_w(), 40);
    EXPECT_EQ(tree.cached_constraint_h(), 10);
}

TEST(TuiLayoutCacheTest, WithStyleMarksDirty) {
    auto tree = element::text("hello");
    compute_layout(tree, 80, 24);
    EXPECT_FALSE(tree.is_layout_dirty());

    style s;
    s.bold = true;
    tree = tree | bold();
    EXPECT_TRUE(tree.is_layout_dirty());
}

TEST(TuiLayoutCacheTest, EmptyElementCache) {
    auto tree = element::empty();
    auto result = compute_layout(tree, 80, 24);
    EXPECT_TRUE(result.empty());
    EXPECT_FALSE(tree.is_layout_dirty());
}

TEST(TuiLayoutCacheTest, NestedLayoutCache) {
    auto tree = element::vbox({element::text("A"), element::text("B")});
    auto result = compute_layout(tree, 80, 24);
    EXPECT_EQ(result.size(), 2u);
    EXPECT_FALSE(tree.is_layout_dirty());
}

TEST(TuiRendererHitTest, FindElementAtText) {
    auto tree = element::text("hello");
    auto layout = compute_layout(tree, 80, 24);
    int idx = 0;
    auto* hit = find_element_at(layout, tree, 0, 0, idx);
    EXPECT_NE(hit, nullptr);
}

TEST(TuiRendererHitTest, FindElementAtMiss) {
    auto tree = element::text("hello");
    auto layout = compute_layout(tree, 80, 24);
    int idx = 0;
    auto* hit = find_element_at(layout, tree, 100, 100, idx);
    EXPECT_EQ(hit, nullptr);
}

TEST(TuiRendererHitTest, HitTestAtReturnsOwner) {
    auto tree = element::text("hello");
    TestComponent comp;
    tree.set_owner(&comp);
    auto layout = compute_layout(tree, 80, 24);
    int idx = 0;
    auto* hit = hit_test_at(layout, tree, 0, 0, idx, nullptr);
    EXPECT_EQ(hit, &comp);
}
