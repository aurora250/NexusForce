#include <NeForce/tui/element.hpp>
#include <NeForce/tui/state.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

class ComponentBase;

struct Element::Node {
    ElementKind kind = ElementKind::Empty;
    size_t key = 0;

    string text;
    Style style;
    BoxProps layout;

    vector<Element> children;
    function<void()> onClick;
    function<void(int, int, int, int)> drawFn;
    void* stateRef = nullptr;
    int flex = 0;
    Variant variant = Variant::Default;
    ComponentBase* owner = nullptr;
    int scrollX = 0;
    int scrollY = 0;

    Node() = default;
    ~Node() = default;
    Node(const Node&) = default;
    Node& operator=(const Node&) = default;
    Node(Node&&) noexcept = default;
    Node& operator=(Node&&) noexcept = default;
};

Element::Element() :
node_(make_shared<Node>()) {}
Element::~Element() = default;

Element::Element(const Element& other) :
node_(make_shared<Node>(*other.node_)) {}

Element& Element::operator=(const Element& other) {
    if (&other == this) {
        return *this;
    }
    node_ = make_shared<Node>(*other.node_);
    return *this;
}

Element::Element(Element&& other) noexcept :
node_(move(other.node_)) {
    other.node_ = make_shared<Node>();
}

Element& Element::operator=(Element&& other) noexcept {
    if (&other == this) {
        return *this;
    }
    node_ = move(other.node_);
    return *this;
}

Element& Element::withKey(size_t k) {
    node_->key = k;
    return *this;
}

ElementKind Element::kind() const noexcept { return node_->kind; }

size_t Element::key() const noexcept { return node_->key; }

const vector<Element>& Element::children() const { return node_->children; }

const Style& Element::style() const { return node_->style; }

const BoxProps& Element::layout() const { return node_->layout; }

const string& Element::text() const { return node_->text; }

const function<void()>& Element::onClick() const { return node_->onClick; }

int Element::flex() const noexcept { return node_->flex; }

void* Element::stateRef() const noexcept { return node_->stateRef; }

Variant Element::variant() const noexcept { return node_->variant; }

const function<void(int, int, int, int)>& Element::drawFn() const { return node_->drawFn; }
void* Element::owner() const noexcept { return node_->owner; }
void Element::setOwner(void* o) { node_->owner = static_cast<ComponentBase*>(o); }
int Element::scrollX() const noexcept { return node_->scrollX; }
int Element::scrollY() const noexcept { return node_->scrollY; }

void Element::ensureNode() {
    if (!node_) {
        node_ = make_shared<Node>();
    }
}

void Element::initKind(ElementKind k) {
    ensureNode();
    node_->kind = k;
}

void Element::reserveChildren(size_t n) {
    ensureNode();
    node_->children.reserve(n);
}

void Element::addChild(Element child) { node_->children.push_back(move(child)); }

Element Element::empty() { return {}; }

Element Element::vbox(vector<Element> children, BoxProps props) {
    Element el;
    el.node_->kind = ElementKind::VBox;
    el.node_->children = move(children);
    el.node_->layout = props;
    return el;
}

Element Element::hbox(vector<Element> children, BoxProps props) {
    Element el;
    el.node_->kind = ElementKind::HBox;
    el.node_->children = move(children);
    el.node_->layout = props;
    el.node_->layout.dir = Direction::Row;
    return el;
}

Element Element::zstack(vector<Element> children) {
    Element el;
    el.node_->kind = ElementKind::ZStack;
    el.node_->children = move(children);
    return el;
}

Element Element::text(string content, Style style) {
    Element el;
    el.node_->kind = ElementKind::Text;
    el.node_->text = move(content);
    el.node_->style = style;
    return el;
}

Element Element::spacer(int flex) {
    Element el;
    el.node_->kind = ElementKind::Spacer;
    el.node_->flex = flex;
    return el;
}

Element Element::separator() {
    Element el;
    el.node_->kind = ElementKind::Separator;
    return el;
}

Element Element::button(string label, function<void()> onClick, Style style, Variant variant) {
    Element el;
    el.node_->kind = ElementKind::Button;
    el.node_->text = move(label);
    el.node_->onClick = move(onClick);
    el.node_->style = move(style);
    el.node_->variant = variant;
    return el;
}

Element Element::textInput(State<string>& text, Style style) {
    Element el;
    el.node_->kind = ElementKind::TextInput;
    el.node_->stateRef = &text;
    el.node_->style = move(style);
    el.node_->text = text.value();
    return el;
}

Element Element::checkbox(string label, State<bool>& checked, Style style) {
    Element el;
    el.node_->kind = ElementKind::Checkbox;
    el.node_->text = move(label);
    el.node_->stateRef = &checked;
    el.node_->style = move(style);
    return el;
}

Element Element::scrollView(Element child, Style style, int scrollX, int scrollY) {
    Element el;
    el.node_->kind = ElementKind::ScrollView;
    el.node_->children.push_back(move(child));
    el.node_->style = move(style);
    el.node_->scrollX = scrollX;
    el.node_->scrollY = scrollY;
    return el;
}

Element Element::canvas(function<void(int, int, int, int)> drawFn, Style style) {
    Element el;
    el.node_->kind = ElementKind::Canvas;
    el.node_->drawFn = move(drawFn);
    el.node_->style = move(style);
    return el;
}

Element Element::when(bool cond, function<Element()> then, function<Element()> otherwise) {
    Element el;
    el.node_->kind = ElementKind::When;
    el.node_->children.push_back(cond ? then() : otherwise());
    return el;
}

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
