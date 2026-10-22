#include <NeForce/tui/component/virtual_list.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__
NEFORCE_BEGIN_COMPONENTS__

virtual_list_view::virtual_list_view(virtual_list_option opt) :
opt_(move(opt)),
follow_(opt_.auto_follow) {}

void virtual_list_view::setup() {
    scroll_ = &create_state<int>(0);
    viewport_ = &create_state<int>(0);
    rect_ = &create_state<layout_rect>(layout_rect{});
    heights_.assign(opt_.item_count, 0);
    heights_snapshot_ = heights_;
}

int virtual_list_view::rows_of(const size_t index) const noexcept {
    if (!opt_.variable_height) {
        return max(1, opt_.item_height);
    }
    if (index < heights_.size() && heights_[index] > 0) {
        return heights_[index];
    }
    return max(1, opt_.item_height);
}

void virtual_list_view::set_variable_height(const bool enable) {
    if (opt_.variable_height == enable) {
        return;
    }
    opt_.variable_height = enable;
    invalidate_heights();
}

void virtual_list_view::invalidate_heights() {
    heights_.assign(opt_.item_count, 0);
    heights_snapshot_ = heights_;
    schedule_render();
}

void virtual_list_view::set_item_height(const int height) {
    const int clamped = max(1, height);
    if (clamped == opt_.item_height) {
        return;
    }
    opt_.item_height = clamped;
    follow_pending_ = follow_;
    schedule_render();
}

int virtual_list_view::measured_height(const size_t index) const noexcept {
    return (index < heights_.size()) ? heights_[index] : 0;
}

size_t virtual_list_view::measured_count() const noexcept {
    size_t count = 0;
    for (const int height: heights_) {
        if (height > 0) {
            ++count;
        }
    }
    return count;
}

void virtual_list_view::set_item_renderer(function<element(size_t)> render_item) {
    opt_.render_item = move(render_item);
    schedule_render();
}

layout_rect virtual_list_view::viewport_rect() const noexcept {
    return (rect_ != nullptr) ? rect_->value() : layout_rect{};
}

int virtual_list_view::viewport_rows() const noexcept { return viewport_ != nullptr ? viewport_->value() : 0; }

int virtual_list_view::content_rows() const noexcept {
    if (!opt_.variable_height) {
        return static_cast<int>(opt_.item_count) * max(1, opt_.item_height);
    }
    int total = 0;
    for (size_t i = 0; i < opt_.item_count; ++i) {
        total += rows_of(i);
    }
    return total;
}

int virtual_list_view::scroll_offset() const noexcept { return scroll_ != nullptr ? scroll_->value() : 0; }

bool virtual_list_view::at_bottom() const noexcept {
    if (scroll_ == nullptr) {
        return true;
    }
    return scroll_->value() + max(0, viewport_rows()) >= content_rows();
}

void virtual_list_view::clamp_scroll() {
    if (scroll_ == nullptr) {
        return;
    }
    const int limit = max(0, content_rows() - max(0, viewport_rows()));
    if (scroll_->value() > limit) {
        scroll_->set_quiet(limit);
    }
    if (scroll_->value() < 0) {
        scroll_->set_quiet(0);
    }
}

void virtual_list_view::set_item_count(const size_t count) {
    if (count == opt_.item_count) {
        return;
    }
    const bool was_at_bottom = at_bottom();
    opt_.item_count = count;
    heights_.resize(count, 0);
    heights_snapshot_.resize(count, 0);
    if (follow_ && was_at_bottom) {
        follow_pending_ = true;
    }
    schedule_render();
}

void virtual_list_view::set_auto_follow(const bool enable) {
    follow_ = enable;
    if (follow_) {
        follow_pending_ = true;
        schedule_render();
    }
}

void virtual_list_view::scroll_to(const int offset) {
    if (scroll_ == nullptr) {
        return;
    }
    *scroll_ = max(0, offset);
    clamp_scroll();
    follow_ = at_bottom();
    follow_pending_ = false;
}

void virtual_list_view::scroll_by(const int delta) { scroll_to(scroll_offset() + delta); }

void virtual_list_view::scroll_to_item(const size_t index) {
    scroll_to(static_cast<int>(index) * max(1, opt_.item_height));
}

void virtual_list_view::scroll_to_bottom() {
    scroll_to(max(0, content_rows() - max(0, viewport_rows())));
    follow_ = true;
    follow_pending_ = false;
}

element virtual_list_view::render() {
    if (scroll_ == nullptr || viewport_ == nullptr) {
        return element::empty();
    }

    // While following, keep the viewport pinned to the bottom.
    // This must be re-evaluated every frame:
    // item heights are estimates until the renderer measures them,
    // so pinning once on append can leave the view mid-content.
    if (follow_) {
        const int target = max(0, content_rows() - max(0, viewport_rows()));
        if (target != scroll_->value()) {
            scroll_->set_quiet(target);
        }
    }
    follow_pending_ = false;

    if (opt_.variable_height) {
        // A width change invalidates every measured height.
        const int width = (rect_ != nullptr) ? rect_->value().w : 0;
        if (width > 0 && width != layout_width_) {
            layout_width_ = width;
            heights_.assign(opt_.item_count, 0);
            heights_snapshot_ = heights_;
        }
    }

    element el = opt_.variable_height ? element::virtual_list(opt_.item_count, max(1, opt_.item_height), &heights_,
                                                              opt_.render_item, scroll_->value())
                                      : element::virtual_list(opt_.item_count, max(1, opt_.item_height),
                                                              opt_.render_item, scroll_->value());
    el = el.with_scroll_y_state(scroll_).with_viewport_state(viewport_).with_viewport_rect_state(rect_).with_style(
            opt_.style);
    el.set_owner(this);

    // The renderer writes measured heights back while drawing;
    // if they changed, request one more frame so the scroll metrics settle immediately.
    if (opt_.variable_height && rect_ != nullptr && rect_->value().h > 0 && heights_ != heights_snapshot_) {
        heights_snapshot_ = heights_;
        schedule_render();
    }
    return move(el);
}

bool virtual_list_view::on_key(const key_event& e) {
    using K = key_event::type;
    const int page = max(1, viewport_rows());

    switch (e.key) {
        case K::up: {
            scroll_by(-1);
            return true;
        }
        case K::down: {
            scroll_by(1);
            return true;
        }
        case K::page_up: {
            scroll_by(-page);
            return true;
        }
        case K::page_down: {
            scroll_by(page);
            return true;
        }
        case K::home: {
            scroll_to(0);
            return true;
        }
        case K::end: {
            scroll_to_bottom();
            return true;
        }
        default: {
            break;
        }
    }
    return false;
}

unique_ptr<component_base> virtual_list(virtual_list_option opt) { return make_unique<virtual_list_view>(move(opt)); }

NEFORCE_END_COMPONENTS__
NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
