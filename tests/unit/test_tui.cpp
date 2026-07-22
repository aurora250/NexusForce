#include <NeForce/tui/element.hpp>
#include <NeForce/tui/events.hpp>
#include <NeForce/tui/layout.hpp>
#include <NeForce/tui/style.hpp>
#include <NeForce/tui/component.hpp>
#include <gtest/gtest.h>
using namespace neforce;
using namespace neforce::tui;

namespace {
    struct TestComponent : Component<EmptyProps> {
        int renderCount = 0;

        Element render() override {
            ++renderCount;
            return Element::text("hello");
        }

        template <typename T>
        State<T>& createPublicState(T initial) {
            return this->createState<T>(_NEFORCE move(initial));
        }

        void setStrand(strand& s) { strand_ = &s; }
        void setCtx(io_context& c) { ctx_ = &c; }
    };
} // namespace


TEST(TuiStyleTest, MergeFgOverrides) {
    tui::Style base;
    base.fg = color::white();
    base.bg = color::black();

    tui::Style over;
    over.fg = color::red();

    auto merged = tui::Style::merge(base, over);
    EXPECT_EQ(merged.fg.value(), color::red());
    EXPECT_EQ(merged.bg.value(), color::black());
}

TEST(TuiStyleTest, MergeBoolFlagsPreserve) {
    tui::Style base;
    base.bold = true;

    tui::Style over;
    over.underline = true;

    auto merged = tui::Style::merge(base, over);
    EXPECT_TRUE(merged.bold.value_or(false));
    EXPECT_TRUE(merged.underline.value_or(false));
}

TEST(TuiStyleTest, MergePaddingOverrides) {
    tui::Style base;
    base.padding = tui::Padding{1, 1, 1, 1};

    tui::Style over;
    over.padding = tui::Padding{2, 3, 4, 5};

    auto merged = tui::Style::merge(base, over);
    EXPECT_EQ(merged.padding->top, 2);
    EXPECT_EQ(merged.padding->right, 3);
    EXPECT_EQ(merged.padding->bottom, 4);
    EXPECT_EQ(merged.padding->left, 5);
}

TEST(TuiStyleTest, ThemeButtonPrimary) {
    auto s = dark_theme.buttonStyle(tui::Variant::Primary);
    EXPECT_EQ(s.bg.value(), color::cyan());
    EXPECT_TRUE(s.bold.value_or(false));
}

TEST(TuiStyleTest, ThemeButtonDanger) {
    auto s = dark_theme.buttonStyle(tui::Variant::Danger);
    EXPECT_EQ(s.fg.value(), color::red());
    EXPECT_TRUE(s.bold.value_or(false));
}

TEST(TuiStyleTest, ThemeInputFocused) {
    auto s = dark_theme.inputStyle(true);
    EXPECT_EQ(*s.border, tui::Border::Single);
    EXPECT_EQ(s.borderColor.value(), color::cyan());
}

TEST(TuiStyleTest, ThemeInputUnfocused) {
    auto s = dark_theme.inputStyle(false);
    EXPECT_EQ(*s.border, tui::Border::Single);
    EXPECT_EQ(s.borderColor.value(), dark_theme.border);
}

TEST(TuiElementTest, EmptyElement) {
    auto el = tui::Element::empty();
    EXPECT_EQ(el.kind(), tui::ElementKind::Empty);
}

TEST(TuiElementTest, TextElement) {
    auto el = tui::Element::text("hello");
    EXPECT_EQ(el.kind(), tui::ElementKind::Text);
    EXPECT_EQ(el.text(), "hello");
}

TEST(TuiElementTest, TextElementWithStyle) {
    tui::Style s;
    s.fg = color::red();
    s.bold = true;

    auto el = tui::Element::text("styled", s);
    EXPECT_EQ(el.style().fg.value(), color::red());
    EXPECT_TRUE(el.style().bold.value_or(false));
}

TEST(TuiElementTest, ButtonElement) {
    int calls = 0;
    auto el = tui::Element::button("Click", [&] { ++calls; });
    EXPECT_EQ(el.kind(), tui::ElementKind::Button);
    EXPECT_EQ(el.text(), "Click");

    el.onClick()();
    EXPECT_EQ(calls, 1);
}

TEST(TuiElementTest, ButtonWithVariant) {
    auto el = tui::Element::button("OK", [] {}, {}, tui::Variant::Danger);
    EXPECT_EQ(el.kind(), tui::ElementKind::Button);
}

TEST(TuiElementTest, SpacerElement) {
    auto el = tui::Element::spacer(3);
    EXPECT_EQ(el.kind(), tui::ElementKind::Spacer);
    EXPECT_EQ(el.flex(), 3);
}

TEST(TuiElementTest, SeparatorElement) {
    auto el = tui::Element::separator();
    EXPECT_EQ(el.kind(), tui::ElementKind::Separator);
}

TEST(TuiElementTest, VBoxStoresChildren) {
    auto child1 = tui::Element::text("a");
    auto child2 = tui::Element::text("b");
    vector<tui::Element> children;
    children.push_back(child1);
    children.push_back(child2);

    auto el = tui::Element::vbox(_NEFORCE move(children));
    EXPECT_EQ(el.kind(), tui::ElementKind::VBox);
    EXPECT_EQ(el.children().size(), 2U);
    EXPECT_EQ(el.layout().dir, tui::Direction::Column);
}

TEST(TuiElementTest, HBoxDirectionIsRow) {
    auto el = tui::Element::hbox({});
    EXPECT_EQ(el.kind(), tui::ElementKind::HBox);
    EXPECT_EQ(el.layout().dir, tui::Direction::Row);
}

TEST(TuiElementTest, ZStackStoresChildren) {
    auto c1 = tui::Element::text("layer1");
    auto c2 = tui::Element::text("layer2");
    vector<tui::Element> children;
    children.push_back(c1);
    children.push_back(c2);

    auto el = tui::Element::zstack(_NEFORCE move(children));
    EXPECT_EQ(el.kind(), tui::ElementKind::ZStack);
    EXPECT_EQ(el.children().size(), 2U);
}

TEST(TuiElementTest, WithKey) {
    auto el = tui::Element::text("item").withKey(42);
    EXPECT_EQ(el.key(), 42U);
    EXPECT_EQ(el.kind(), tui::ElementKind::Text);
}

TEST(TuiElementTest, WhenTrue) {
    auto el =
            tui::Element::when(true, [] { return tui::Element::text("yes"); }, [] { return tui::Element::text("no"); });
    EXPECT_EQ(el.kind(), tui::ElementKind::When);
    ASSERT_EQ(el.children().size(), 1U);
    EXPECT_EQ(el.children()[0].kind(), tui::ElementKind::Text);
    EXPECT_EQ(el.children()[0].text(), "yes");
}

TEST(TuiElementTest, WhenFalse) {
    auto el = tui::Element::when(
            false, [] { return tui::Element::text("yes"); }, [] { return tui::Element::text("no"); });
    EXPECT_EQ(el.kind(), tui::ElementKind::When);
    ASSERT_EQ(el.children().size(), 1U);
    EXPECT_EQ(el.children()[0].kind(), tui::ElementKind::Text);
    EXPECT_EQ(el.children()[0].text(), "no");
}

TEST(TuiElementTest, EachEmptyList) {
    vector<int> items;
    auto el = tui::Element::each<int>(items, [](const int&, size_t) { return tui::Element::text("x"); });
    EXPECT_EQ(el.kind(), tui::ElementKind::Each);
    EXPECT_EQ(el.children().size(), 0U);
}

TEST(TuiElementTest, EachNonEmptyList) {
    vector<string> items = {"1", "2", "3"};
    auto el = tui::Element::each<string>(items, [](const string& v, size_t) { return tui::Element::text(v); });
    EXPECT_EQ(el.kind(), tui::ElementKind::Each);
    EXPECT_EQ(el.children().size(), 3U);
    EXPECT_EQ(el.children()[0].text(), "1");
    EXPECT_EQ(el.children()[1].text(), "2");
    EXPECT_EQ(el.children()[2].text(), "3");
}

TEST(TuiElementTest, ScrollViewStoresChild) {
    auto inner = tui::Element::text("content");
    auto el = tui::Element::scrollView(inner);
    EXPECT_EQ(el.kind(), tui::ElementKind::ScrollView);
    EXPECT_EQ(el.children().size(), 1U);
    EXPECT_EQ(el.children()[0].text(), "content");
}

TEST(TuiElementTest, CopyElement) {
    auto el1 = tui::Element::text("original");
    auto el2 = el1;
    EXPECT_EQ(el1.text(), el2.text());
    EXPECT_EQ(el2.kind(), tui::ElementKind::Text);
}

TEST(TuiElementTest, MoveElement) {
    auto el1 = tui::Element::text("move");
    auto el2 = _NEFORCE move(el1);
    EXPECT_EQ(el2.kind(), tui::ElementKind::Text);
    EXPECT_EQ(el2.text(), "move");
}

TEST(TuiLayoutTest, EmptyElement) {
    auto el = tui::Element::empty();
    auto result = computeLayout(el, 80, 24);
    EXPECT_EQ(result.size(), 0U);
}

TEST(TuiLayoutTest, SingleText) {
    auto el = tui::Element::text("hello");
    auto result = computeLayout(el, 80, 24);
    ASSERT_EQ(result.size(), 1U);
    EXPECT_EQ(result[0].w, 5);
    EXPECT_EQ(result[0].h, 1);
}

TEST(TuiLayoutTest, VBoxMultipleChildren) {
    auto c1 = tui::Element::text("line1");
    auto c2 = tui::Element::text("line2");
    vector<tui::Element> children;
    children.push_back(c1);
    children.push_back(c2);

    auto el = tui::Element::vbox(_NEFORCE move(children));
    auto result = computeLayout(el, 80, 24);
    ASSERT_EQ(result.size(), 2U);
    EXPECT_EQ(result[0].h, 1);
    EXPECT_EQ(result[1].h, 1);
    EXPECT_EQ(result[0].y + result[0].h, result[1].y);
}

TEST(TuiLayoutTest, VBoxWithGap) {
    auto c1 = tui::Element::text("a");
    auto c2 = tui::Element::text("b");
    vector<tui::Element> children;
    children.push_back(c1);
    children.push_back(c2);

    tui::BoxProps props;
    props.gap = 2;

    auto el = tui::Element::vbox(_NEFORCE move(children), props);
    auto result = computeLayout(el, 80, 24);
    ASSERT_EQ(result.size(), 2U);
    EXPECT_EQ(result[0].y + result[0].h + 2, result[1].y);
}

TEST(TuiLayoutTest, HBoxMultipleChildren) {
    auto c1 = tui::Element::text("ab");
    auto c2 = tui::Element::text("cd");
    vector<tui::Element> children;
    children.push_back(c1);
    children.push_back(c2);

    auto el = tui::Element::hbox(_NEFORCE move(children));
    auto result = computeLayout(el, 80, 24);
    ASSERT_EQ(result.size(), 2U);
    EXPECT_EQ(result[0].h, 1);
    EXPECT_EQ(result[1].h, 1);
    EXPECT_EQ(result[0].x + result[0].w, result[1].x);
}

TEST(TuiLayoutTest, HBoxWithGap) {
    auto c1 = tui::Element::text("a");
    auto c2 = tui::Element::text("b");
    vector<tui::Element> children;
    children.push_back(c1);
    children.push_back(c2);

    tui::BoxProps props;
    props.gap = 3;

    auto el = tui::Element::hbox(_NEFORCE move(children), props);
    auto result = computeLayout(el, 80, 24);
    ASSERT_EQ(result.size(), 2U);
    EXPECT_EQ(result[0].x + result[0].w + 3, result[1].x);
}

TEST(TuiLayoutTest, NestedLayout) {
    auto inner1 = tui::Element::text("1");
    auto inner2 = tui::Element::text("2");
    vector<tui::Element> innerChildren;
    innerChildren.push_back(inner1);
    innerChildren.push_back(inner2);

    auto hbox = tui::Element::hbox(_NEFORCE move(innerChildren));
    auto outerText = tui::Element::text("top");

    vector<tui::Element> outerChildren;
    outerChildren.push_back(outerText);
    outerChildren.push_back(hbox);

    auto vbox = tui::Element::vbox(_NEFORCE move(outerChildren));
    auto result = computeLayout(vbox, 80, 24);
    ASSERT_EQ(result.size(), 3U);
}

TEST(TuiLayoutTest, SpacerElement) {
    auto c1 = tui::Element::text("a");
    auto sp = tui::Element::spacer(1);
    auto c2 = tui::Element::text("b");
    vector<tui::Element> children;
    children.push_back(c1);
    children.push_back(sp);
    children.push_back(c2);

    auto el = tui::Element::hbox(_NEFORCE move(children));
    auto result = computeLayout(el, 80, 24);
    ASSERT_EQ(result.size(), 3U);
    EXPECT_EQ(result[1].w, 0);
}

TEST(TuiLayoutTest, PaddingOffsetsChildren) {
    auto inner = tui::Element::text("hello");
    vector<tui::Element> children;
    children.push_back(inner);

    tui::BoxProps props;
    props.padding = tui::Padding{2, 3, 1, 4};

    auto el = tui::Element::vbox(_NEFORCE move(children), props);
    auto result = computeLayout(el, 80, 24);
    ASSERT_EQ(result.size(), 1U);
    EXPECT_EQ(result[0].x, 4);
    EXPECT_EQ(result[0].y, 2);
}

TEST(TuiLayoutTest, FixedWidth) {
    tui::Style s;
    s.width = tui::SizeHint{tui::SizeHint::Fixed, 10};

    auto el = tui::Element::text("hello", s);
    auto result = computeLayout(el, 80, 24);
    ASSERT_EQ(result.size(), 1U);
    EXPECT_EQ(result[0].w, 10);
}

TEST(TuiEventTest, KeyEventPrintable) {
    tui::KeyEvent e;
    e.key = tui::KeyEvent::Key::Printable;
    e.ch = U'A';
    EXPECT_TRUE(e.isPrintable());
    EXPECT_EQ(e.ch, U'A');
}

TEST(TuiEventTest, KeyEventSpecial) {
    tui::KeyEvent e;
    e.key = tui::KeyEvent::Key::Escape;
    EXPECT_FALSE(e.isPrintable());
}

TEST(TuiEventTest, KeyEventWithModifier) {
    tui::KeyEvent e;
    e.key = tui::KeyEvent::Key::Printable;
    e.ch = U'c';
    e.mods = tui::Modifier::Ctrl;
    EXPECT_TRUE(e.isPrintable());
    EXPECT_EQ(e.mods, tui::Modifier::Ctrl);
}

TEST(TuiEventTest, MouseEventDefaults) {
    tui::MouseEvent e;
    EXPECT_EQ(e.button, tui::MouseButton::None);
    EXPECT_EQ(e.action, tui::MouseAction::Move);
    EXPECT_EQ(e.x, 0);
    EXPECT_EQ(e.y, 0);
}

TEST(TuiComponentTest, EmptyPropsDefault) {
    TestComponent comp;
    EXPECT_EQ(comp.parent(), nullptr);
}

TEST(TuiComponentTest, OnKeyReturnsFalseByDefault) {
    TestComponent comp;
    tui::KeyEvent e;
    EXPECT_FALSE(comp.onKey(e));
}

TEST(TuiComponentTest, OnMouseReturnsFalseByDefault) {
    TestComponent comp;
    tui::MouseEvent e;
    EXPECT_FALSE(comp.onMouse(e));
}

TEST(TuiComponentTest, RenderCreatesElement) {
    TestComponent comp;
    auto el = comp.render();
    EXPECT_EQ(el.kind(), tui::ElementKind::Text);
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

    state.setQuiet(100);
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
    state.onChange([&](const int& v) { received = v; });

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
