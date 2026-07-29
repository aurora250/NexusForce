#include <NeForce/tui/dom/layout.hpp>
#include <NeForce/tui/dom/state.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

struct element::node {
    enum kind kind = kind::empty;
    size_t key = 0;

    string text;
    tui::style style;
    box_props layout;

    vector<element> children;
    function<void()> on_click;
    function<void(int, int, int, int)> draw_function;
    void* state_ref = nullptr;
    int flex = 0;
    style::variant variant = style::variant::default_;
    component_base* owner = nullptr;
    int scroll_x = 0;
    int scroll_y = 0;
    int grid_columns = 0;
    string placeholder;
    bool cursor_visible = false;
    size_t cursor_pos = 0;
    void* scroll_x_state = nullptr;
    void* scroll_y_state = nullptr;

    mutable bool layout_dirty_ = true;
    mutable vector<layout_rect> cached_layout_;
    mutable int cached_constraint_w_ = -1;
    mutable int cached_constraint_h_ = -1;

    node() = default;
    ~node() = default;
    node(const node&) = default;
    node& operator=(const node&) = default;
    node(node&&) noexcept = default;
    node& operator=(node&&) noexcept = default;
};

element::element() :
node_(make_shared<node>()) {}
element::~element() = default;

element::element(const element& other) :
node_(make_shared<node>(*other.node_)) {}

element& element::operator=(const element& other) {
    if (&other == this) {
        return *this;
    }
    node_ = make_shared<node>(*other.node_);
    return *this;
}

element::element(element&& other) noexcept :
node_(move(other.node_)) {
    other.node_ = make_shared<node>();
}

element& element::operator=(element&& other) noexcept {
    if (&other == this) {
        return *this;
    }
    node_ = move(other.node_);
    return *this;
}

element& element::with_key(size_t k) {
    node_->key = k;
    return *this;
}

enum element::kind element::kind() const noexcept { return node_->kind; }

size_t element::key() const noexcept { return node_->key; }

const vector<element>& element::children() const { return node_->children; }

vector<element>& element::children() { return node_->children; }

const style& element::style() const { return node_->style; }

const box_props& element::layout() const { return node_->layout; }

const string& element::text() const { return node_->text; }

const function<void()>& element::on_click() const { return node_->on_click; }

int element::flex() const noexcept { return node_->flex; }

void* element::state_ref() const noexcept { return node_->state_ref; }

style::variant element::variant() const noexcept { return node_->variant; }

const function<void(int, int, int, int)>& element::draw_function() const { return node_->draw_function; }

void* element::owner() const noexcept { return node_->owner; }

void element::set_owner(void* o) { node_->owner = static_cast<component_base*>(o); }

int element::scroll_x() const noexcept { return node_->scroll_x; }

int element::scroll_y() const noexcept { return node_->scroll_y; }

int element::grid_columns() const noexcept { return node_->grid_columns; }

const string& element::placeholder() const noexcept { return node_->placeholder; }

bool element::cursor_visible() const noexcept { return node_->cursor_visible; }

style::wrap_mode element::wrap_mode() const noexcept { return node_->style.text_wrap.value_or(style::wrap_mode::none); }

element& element::with_cursor_visible(bool v) {
    ensure_node();
    node_->cursor_visible = v;
    return *this;
}

size_t element::cursor_pos() const noexcept { return node_->cursor_pos; }

element& element::with_cursor_pos(size_t pos) {
    ensure_node();
    node_->cursor_pos = pos;
    return *this;
}

void* element::scroll_x_state() const noexcept { return node_->scroll_x_state; }

element& element::with_scroll_x_state(void* s) {
    ensure_node();
    node_->scroll_x_state = s;
    return *this;
}

void* element::scroll_y_state() const noexcept { return node_->scroll_y_state; }

element& element::with_scroll_y_state(void* s) {
    ensure_node();
    node_->scroll_y_state = s;
    return *this;
}

void element::ensure_node() {
    if (!node_) {
        node_ = make_shared<node>();
    }
}

void element::init_kind(enum kind k) {
    ensure_node();
    node_->kind = k;
}

void element::reserve_children(size_t n) {
    ensure_node();
    node_->children.reserve(n);
}

void element::add_child(element child) { node_->children.push_back(move(child)); }

element& element::with_style(const tui::style& s) {
    ensure_node();
    node_->style = style::merge(node_->style, s);
    node_->layout_dirty_ = true;
    return *this;
}

element element::empty() { return {}; }

element element::vbox(vector<element> children, box_props props) {
    element el;
    el.node_->kind = kind::vbox;
    el.node_->children = move(children);
    el.node_->layout = props;
    return el;
}

element element::hbox(vector<element> children, box_props props) {
    element el;
    el.node_->kind = kind::hbox;
    el.node_->children = move(children);
    el.node_->layout = props;
    el.node_->layout.dir = style::direction::row;
    return el;
}

element element::zstack(vector<element> children) {
    element el;
    el.node_->kind = kind::zstack;
    el.node_->children = move(children);
    return el;
}

element element::text(string content, tui::style style, style::wrap_mode wrap) {
    element el;
    el.node_->kind = kind::text;
    el.node_->text = move(content);
    el.node_->style = move(style);
    if (wrap != style::wrap_mode::none) {
        el.node_->style.text_wrap = wrap;
    }
    return el;
}

element element::spacer(int flex) {
    element el;
    el.node_->kind = kind::spacer;
    el.node_->flex = flex;
    return el;
}

element element::separator() {
    element el;
    el.node_->kind = kind::separator;
    return el;
}

element element::button(string label, function<void()> on_click, tui::style style, style::variant variant) {
    element el;
    el.node_->kind = kind::button;
    el.node_->text = move(label);
    el.node_->on_click = move(on_click);
    el.node_->style = move(style);
    el.node_->variant = variant;
    return el;
}

element element::text_input(state<string>& text, tui::style style, string placeholder) {
    element el;
    el.node_->kind = kind::text_input;
    el.node_->state_ref = &text;
    el.node_->style = move(style);
    el.node_->text = text.value();
    el.node_->placeholder = move(placeholder);
    if (!el.node_->style.text_wrap.has_value()) {
        el.node_->style.text_wrap = style::wrap_mode::word;
    }
    return el;
}

element element::checkbox(string label, state<bool>& checked, tui::style style) {
    element el;
    el.node_->kind = kind::checkbox;
    el.node_->text = move(label);
    el.node_->state_ref = &checked;
    el.node_->style = move(style);
    return el;
}

element element::scroll_view(element child, tui::style style, int scroll_x, int scroll_y) {
    element el;
    el.node_->kind = kind::scroll_view;
    el.node_->children.push_back(move(child));
    el.node_->style = move(style);
    el.node_->scroll_x = scroll_x;
    el.node_->scroll_y = scroll_y;
    return el;
}

element element::canvas(function<void(int, int, int, int)> draw_function, tui::style style) {
    element el;
    el.node_->kind = kind::canvas;
    el.node_->draw_function = move(draw_function);
    el.node_->style = move(style);
    return el;
}

element element::flexbox(vector<element> children, box_props config) {
    element el;
    el.node_->kind = kind::flexbox;
    el.node_->children = move(children);
    el.node_->layout = config;
    el.node_->layout.dir = style::direction::row;
    el.node_->layout.wrap = config.wrap;
    return el;
}

element element::gridbox(vector<vector<element>> grid) {
    element el;
    el.node_->kind = kind::gridbox;
    const int rows = static_cast<int>(grid.size());
    int cols = 0;
    for (const auto& row: grid) {
        cols = max(cols, static_cast<int>(row.size()));
    }
    el.node_->grid_columns = cols;
    el.node_->children.reserve(static_cast<size_t>(rows) * cols);
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            if (c < static_cast<int>(grid[r].size())) {
                el.node_->children.push_back(grid[r][c]);
            } else {
                el.node_->children.push_back(element::text(""));
            }
        }
    }
    return el;
}

element element::when(bool cond, function<element()> then, function<element()> otherwise) {
    element el;
    el.node_->kind = kind::when;
    el.node_->children.push_back(cond ? then() : otherwise());
    return el;
}

bool element::is_layout_dirty() const noexcept { return node_->layout_dirty_; }

void element::set_layout_dirty(bool dirty) { node_->layout_dirty_ = dirty; }

const vector<layout_rect>& element::cached_layout() const noexcept { return node_->cached_layout_; }

void element::set_cached_layout(const vector<layout_rect>& layout, int constraint_w, int constraint_h) const {
    node_->cached_layout_ = layout;
    node_->cached_constraint_w_ = constraint_w;
    node_->cached_constraint_h_ = constraint_h;
    node_->layout_dirty_ = false;
}

int element::cached_constraint_w() const noexcept { return node_->cached_constraint_w_; }

int element::cached_constraint_h() const noexcept { return node_->cached_constraint_h_; }

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
