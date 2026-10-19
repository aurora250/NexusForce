#include <NeForce/tui/component/select_list.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__
NEFORCE_BEGIN_COMPONENTS__

namespace {
    bool contains_ignore_case(const string& haystack, const string_view needle) {
        if (needle.empty()) {
            return true;
        }
        const auto& raw = haystack.view();
        if (needle.size() > raw.size()) {
            return false;
        }
        for (size_t i = 0; i + needle.size() <= raw.size(); ++i) {
            bool match = true;
            for (size_t j = 0; j < needle.size(); ++j) {
                const auto a = static_cast<unsigned char>(raw[i + j]);
                const auto b = static_cast<unsigned char>(needle[j]);
                const unsigned char la = (a >= 'A' && a <= 'Z') ? static_cast<unsigned char>(a - 'A' + 'a') : a;
                const unsigned char lb = (b >= 'A' && b <= 'Z') ? static_cast<unsigned char>(b - 'A' + 'a') : b;
                if (la != lb) {
                    match = false;
                    break;
                }
            }
            if (match) {
                return true;
            }
        }
        return false;
    }
} // namespace


select_list_view::select_list_view(select_list_option opt) :
opt_(move(opt)) {
    rebuild_matches();
}

void select_list_view::setup() {
    viewport_ = &create_state<int>(0);
    scroll_state_ = &create_state<int>(0);
}

void select_list_view::rebuild_matches() {
    matches_.clear();
    match_indices_.clear();
    for (size_t i = 0; i < opt_.items.size(); ++i) {
        if (contains_ignore_case(opt_.items[i], filter_.view())) {
            matches_.push_back(opt_.items[i]);
            match_indices_.push_back(i);
        }
    }
    position_ = max(0, min(position_, static_cast<int>(matches_.size()) - 1));
    if (scroll_state_ != nullptr) {
        scroll_state_->set_quiet(0);
    }
    scroll_ = 0;
}

void select_list_view::set_items(vector<string> items) {
    opt_.items = move(items);
    rebuild_matches();
    schedule_render();
}

void select_list_view::set_filter(string filter) {
    filter_ = move(filter);
    rebuild_matches();
    schedule_render();
}

int select_list_view::selected_index() const noexcept {
    if (position_ < 0 || position_ >= static_cast<int>(match_indices_.size())) {
        return -1;
    }
    return static_cast<int>(match_indices_[static_cast<size_t>(position_)]);
}

void select_list_view::set_selected_position(const int position) {
    if (matches_.empty()) {
        position_ = 0;
        return;
    }
    const int last = static_cast<int>(matches_.size()) - 1;
    position_ = max(0, min(position, last));
    if (opt_.selected != nullptr) {
        *opt_.selected = selected_index();
    }
    if (opt_.on_change) {
        opt_.on_change(selected_index());
    }
    schedule_render();
}

void select_list_view::move_by(const int delta) {
    if (matches_.empty()) {
        return;
    }
    const int count = static_cast<int>(matches_.size());
    set_selected_position((position_ + delta + count) % count);
}

bool select_list_view::confirm() {
    const int index = selected_index();
    if (index < 0) {
        return false;
    }
    if (opt_.selected != nullptr) {
        *opt_.selected = index;
    }
    if (opt_.on_select) {
        opt_.on_select(index);
    }
    return true;
}

element select_list_view::render() {
    elements rows;

    if (opt_.filterable) {
        tui::style fst = opt_.filter_style;
        if (!fst.fg.has_value()) {
            fst.fg = opt_.filter_style.fg;
        }
        rows.push_back(element::rich_text({text_span{opt_.filter_prefix, opt_.filter_style},
                                           text_span{filter_, opt_.filter_style},
                                           text_span{matches_.empty() ? opt_.empty_text : string(), opt_.filter_style}},
                                          style{}, style::wrap_mode::none));
    }

    if (matches_.empty()) {
        return element::vbox(move(rows));
    }

    // Keep the highlighted row inside the viewport.
    const int viewport_rows = max(1, viewport_ != nullptr ? viewport_->value() : opt_.visible_rows);
    int target = scroll_;
    target = min(position_, target);
    if (position_ >= target + viewport_rows) {
        target = position_ - viewport_rows + 1;
    }
    target = max(0, min(target, static_cast<int>(matches_.size()) - viewport_rows));
    if (target != scroll_) {
        scroll_ = target;
        if (scroll_state_ != nullptr) {
            scroll_state_->set_quiet(target);
        }
    }

    tui::style list_style;
    list_style.height = style::size_hint{style::size_hint::fixed, max(1, opt_.visible_rows)};

    const int selected_pos = position_;
    element list = element::virtual_list(
            matches_.size(), 1,
            [this, selected_pos](const size_t index) {
                const bool is_selected = (compare_equal(index, selected_pos));
                tui::style st = is_selected ? opt_.selected_style : opt_.item_style;
                if (is_selected && !st.reverse.has_value()) {
                    st.reverse = true;
                }
                return element::text(matches_[index], st, style::wrap_mode::none);
            },
            scroll_);
    list = list.with_style(list_style).with_viewport_state(viewport_).with_scroll_y_state(scroll_state_);
    list.set_owner(this);
    rows.push_back(move(list));

    return element::vbox(move(rows));
}

bool select_list_view::on_key(const key_event& e) {
    using K = key_event::type;
    const int rows = max(1, opt_.visible_rows);

    switch (e.key) {
        case K::up: {
            move_by(-1);
            return true;
        }
        case K::down: {
            move_by(1);
            return true;
        }
        case K::page_up: {
            move_by(-rows);
            return true;
        }
        case K::page_down: {
            move_by(rows);
            return true;
        }
        case K::home: {
            set_selected_position(0);
            return true;
        }
        case K::end: {
            set_selected_position(static_cast<int>(matches_.size()) - 1);
            return true;
        }
        case K::enter: {
            return confirm();
        }
        case K::escape: {
            if (!filter_.empty()) {
                set_filter(string());
                return true;
            }
            if (opt_.on_cancel) {
                opt_.on_cancel();
                return true;
            }
            return false;
        }
        case K::backspace: {
            if (!opt_.filterable || filter_.empty()) {
                return false;
            }
            string next = filter_.substr(0, filter_.size() - 1);
            set_filter(move(next));
            return true;
        }
        case K::printable: {
            if (!opt_.filterable || e.cp.value() < 0x20 || (e.mods & key_modifier::ctrl) != key_modifier::none) {
                return false;
            }
            string next = filter_;
            e.cp.append_to(next);
            set_filter(move(next));
            return true;
        }
        default: {
            return false;
        }
    }
}

unique_ptr<component_base> select_list(select_list_option opt) { return make_unique<select_list_view>(move(opt)); }

NEFORCE_END_COMPONENTS__
NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
