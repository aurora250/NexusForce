#include <NeForce/tui/component/component.hpp>
#include <NeForce/tui/dom/text_layout.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

namespace {
    const vector<element>& no_children() {
        static const vector<element> value;
        return value;
    }
    const style& no_style() {
        static const style value;
        return value;
    }
    const box_props& no_layout() noexcept {
        static constexpr box_props value;
        return value;
    }
    const string& no_text() {
        static const string value;
        return value;
    }
    const vector<text_span>& no_spans() {
        static const vector<text_span> value;
        return value;
    }
} // namespace


struct element::node {
    string text;
    vector<text_span> spans;
    vector<text_span> hang;
    int hang_width = 0;
    tui::style style;
    box_props layout;
    vector<element> children;
    function<void()> on_click;
    function<void(int, int, int, int)> draw_function;
    function<void(canvas_painter&)> painter_function;
    function<element(size_t)> item_builder;
    string placeholder;
    mutable vector<layout_rect> cached_layout_;
    mutable measure_cache measure_;

    void* state_ref = nullptr;
    component_base* owner = nullptr;
    void* scroll_x_state = nullptr;
    void* scroll_y_state = nullptr;
    void* viewport_state = nullptr;
    void* viewport_rect_state = nullptr;
    vector<int>* item_heights = nullptr;
    void* rect_state = nullptr;

    size_t key = 0;
    size_t cursor_pos = 0;
    size_t selection_begin = 0;
    size_t selection_end = 0;
    size_t item_count = 0;

    int flex = 0;
    int scroll_x = 0;
    int scroll_y = 0;
    int grid_columns = 0;
    int item_height = 1;
    int item_height_hint = 1;
    mutable int cached_constraint_w_ = -1;
    mutable int cached_constraint_h_ = -1;

    enum kind kind = kind::empty;
    style::variant variant = style::variant::default_;

    bool cursor_visible = false;
    mutable bool layout_dirty_ = true;

    node() = default;
    ~node() = default;
    node(const node&) = default;
    node& operator=(const node&) = default;
    node(node&&) noexcept = default;
    node& operator=(node&&) noexcept = default;
};

const measure_cache& element::cached_measure() const noexcept {
    static constexpr measure_cache empty;
    return node_ ? node_->measure_ : empty;
}

measure_cache& element::mutable_measure() const noexcept {
    static measure_cache fallback;
    if (node_ == nullptr) {
        return fallback;
    }
    return node_->measure_;
}

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
    try {
        other.node_ = make_shared<node>();
        // NOLINTNEXTLINE(bugprone-empty-catch)
    } catch (const exception& e) {
        NEFORCE_REPORT_EXCEPTION(e);
    }
}

element& element::operator=(element&& other) noexcept {
    if (&other == this) {
        return *this;
    }
    node_ = move(other.node_);
    return *this;
}

element& element::with_key(const size_t k) {
    node_->key = k;
    return *this;
}

enum element::kind element::kind() const noexcept { return node_ ? node_->kind : kind::empty; }

size_t element::key() const noexcept { return node_ ? node_->key : 0; }

const vector<element>& element::children() const { return node_ ? node_->children : no_children(); }

vector<element>& element::children() {
    ensure_node();
    return node_->children;
}

const style& element::style() const { return node_ ? node_->style : no_style(); }

const box_props& element::layout() const { return node_ ? node_->layout : no_layout(); }

const string& element::text() const { return node_ ? node_->text : no_text(); }

const vector<text_span>& element::spans() const { return node_ ? node_->spans : no_spans(); }

const vector<text_span>& element::hang() const { return node_ ? node_->hang : no_spans(); }

int element::hang_width() const noexcept { return node_ ? node_->hang_width : 0; }

element& element::with_hang(vector<text_span> prefix) {
    node_->hang = _NEFORCE move(prefix);
    int width = 0;
    for (const auto& span: node_->hang) {
        width += text_display_width(span.content.view());
    }
    node_->hang_width = width;
    node_->layout_dirty_ = true;
    return *this;
}

size_t element::item_count() const noexcept { return node_->item_count; }

int element::item_height() const noexcept { return node_->item_height; }

const function<element(size_t)>& element::item_builder() const { return node_->item_builder; }

const vector<int>* element::item_heights() const noexcept { return node_->item_heights; }

int element::item_height_hint() const noexcept { return node_->item_height_hint; }

void* element::viewport_rect_state() const noexcept { return node_->viewport_rect_state; }

element& element::with_viewport_rect_state(void* s) {
    ensure_node();
    node_->viewport_rect_state = s;
    return *this;
}

void* element::viewport_state() const noexcept { return node_->viewport_state; }

void* element::rect_state() const noexcept { return node_->rect_state; }

element& element::with_rect_state(void* s) {
    ensure_node();
    node_->rect_state = s;
    return *this;
}

element& element::with_scroll_y(const int y) {
    ensure_node();
    node_->scroll_y = y;
    return *this;
}

size_t element::selection_begin() const noexcept { return node_->selection_begin; }

size_t element::selection_end() const noexcept { return node_->selection_end; }

element& element::with_selection(const size_t begin, const size_t end) {
    ensure_node();
    node_->selection_begin = begin;
    node_->selection_end = end;
    return *this;
}

element& element::with_viewport_state(void* s) {
    ensure_node();
    node_->viewport_state = s;
    return *this;
}

const function<void()>& element::on_click() const { return node_->on_click; }

int element::flex() const noexcept { return node_ ? node_->flex : 0; }

void* element::state_ref() const noexcept { return node_ ? node_->state_ref : nullptr; }

style::variant element::variant() const noexcept { return node_ ? node_->variant : style::variant{}; }

const function<void(int, int, int, int)>& element::draw_function() const { return node_->draw_function; }

const function<void(canvas_painter&)>& element::painter_function() const { return node_->painter_function; }

void* element::owner() const noexcept { return node_ ? node_->owner : nullptr; }

void element::set_owner(void* o) {
    if (node_) {
        node_->owner = static_cast<component_base*>(o);
    }
}

int element::scroll_x() const noexcept { return node_ ? node_->scroll_x : 0; }

int element::scroll_y() const noexcept { return node_ ? node_->scroll_y : 0; }

int element::grid_columns() const noexcept { return node_ ? node_->grid_columns : 0; }

const string& element::placeholder() const noexcept { return node_ ? node_->placeholder : no_text(); }

bool element::cursor_visible() const noexcept { return node_ ? node_->cursor_visible : false; }

style::wrap_mode element::wrap_mode() const noexcept {
    return node_ ? node_->style.text_wrap.value_or(style::wrap_mode::none) : style::wrap_mode::none;
}

element& element::with_cursor_visible(const bool v) {
    ensure_node();
    node_->cursor_visible = v;
    return *this;
}

size_t element::cursor_pos() const noexcept { return node_->cursor_pos; }

element& element::with_cursor_pos(const size_t pos) {
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

void element::init_kind(const enum kind k) {
    ensure_node();
    node_->kind = k;
}

void element::reserve_children(const size_t n) {
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

element element::text(string content, tui::style style, const style::wrap_mode wrap) {
    element el;
    el.node_->kind = kind::text;
    el.node_->text = move(content);
    el.node_->style = move(style);
    if (wrap != style::wrap_mode::none) {
        el.node_->style.text_wrap = wrap;
    }
    return el;
}

element element::rich_text(vector<text_span> spans, tui::style style, const style::wrap_mode wrap) {
    element el;
    el.node_->kind = kind::rich_text;
    el.node_->spans = move(spans);
    el.node_->style = move(style);
    if (wrap != style::wrap_mode::none) {
        el.node_->style.text_wrap = wrap;
    }
    return el;
}

element element::virtual_list(const size_t item_count, const int item_height, function<element(size_t)> builder,
                              const int scroll_y) {
    element el;
    el.node_->kind = kind::virtual_list;
    el.node_->item_count = item_count;
    el.node_->item_height = (item_height > 0) ? item_height : 1;
    el.node_->item_builder = move(builder);
    el.node_->scroll_y = scroll_y;
    return el;
}

element element::virtual_list(const size_t item_count, const int item_height_hint, vector<int>* heights,
                              function<element(size_t)> builder, const int scroll_y) {
    element el = element::virtual_list(item_count, item_height_hint, move(builder), scroll_y);
    el.node_->item_height_hint = (item_height_hint > 0) ? item_height_hint : 1;
    el.node_->item_heights = heights;
    return el;
}

element element::spacer(const int flex) {
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

element element::button(string label, function<void()> on_click, tui::style style, const style::variant variant) {
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

element element::scroll_view(element child, tui::style style, const int scroll_x, const int scroll_y) {
    element el;
    el.node_->kind = kind::scroll_view;
    el.node_->children.push_back(move(child));
    el.node_->style = move(style);
    el.node_->scroll_x = scroll_x;
    el.node_->scroll_y = scroll_y;
    return el;
}

element element::band(element child, tui::style style) {
    element el;
    el.node_->kind = kind::band;
    el.node_->children.push_back(move(child));
    el.node_->style = move(style);
    return el;
}

element element::panel(element child, tui::style style) {
    element el;
    el.node_->kind = kind::panel;
    el.node_->children.push_back(move(child));
    el.node_->style = move(style);
    return el;
}

element element::panel(element child, string title, tui::style style) {
    element el = panel(move(child), move(style));
    el.node_->text = move(title);
    return el;
}

element element::canvas(function<void(int, int, int, int)> draw_function, tui::style style) {
    element el;
    el.node_->kind = kind::canvas;
    el.node_->draw_function = move(draw_function);
    el.node_->style = move(style);
    return el;
}

element element::canvas(function<void(canvas_painter&)> draw_function, tui::style style) {
    element el;
    el.node_->kind = kind::canvas;
    el.node_->painter_function = move(draw_function);
    el.node_->style = move(style);
    return el;
}

element element::flexbox(vector<element> children, box_props config) {
    element el;
    el.node_->kind = kind::flexbox;
    el.node_->children = move(children);
    el.node_->layout = config;
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
            if (compare_less(c, grid[r].size())) {
                el.node_->children.push_back(grid[r][c]);
            } else {
                el.node_->children.push_back(element::text(""));
            }
        }
    }
    return el;
}

element element::when(const bool cond, function<element()> then, function<element()> otherwise) {
    element el;
    el.node_->kind = kind::when;
    el.node_->children.push_back(cond ? then() : otherwise());
    return el;
}

bool element::is_layout_dirty() const noexcept { return !node_ || node_->layout_dirty_; }

void element::set_layout_dirty(const bool dirty) {
    if (node_) {
        node_->layout_dirty_ = dirty;
    }
}

const vector<layout_rect>& element::cached_layout() const {
    static const vector<layout_rect> value;
    return node_ ? node_->cached_layout_ : value;
}

void element::set_cached_layout(const vector<layout_rect>& layout, const int constraint_w,
                                const int constraint_h) const {
    if (!node_) {
        return;
    }
    node_->cached_layout_ = layout;
    node_->cached_constraint_w_ = constraint_w;
    node_->cached_constraint_h_ = constraint_h;
    node_->layout_dirty_ = false;
}

int element::cached_constraint_w() const noexcept { return node_ ? node_->cached_constraint_w_ : 0; }

int element::cached_constraint_h() const noexcept { return node_ ? node_->cached_constraint_h_ : 0; }

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
