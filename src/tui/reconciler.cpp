#include <NeForce/tui/reconciler.hpp>
#include <NeForce/core/string/utf_iterator.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

namespace {
    void tag_owners(element& el, component_base* comp) {
        if (el.owner() == nullptr) {
            el.set_owner(comp);
        }
        for (size_t i = 0; i < el.children().size(); ++i) {
            tag_owners(el.children()[i], comp);
        }
    }
} // namespace


reconciler::reconciler(sys_console& console, strand& s, io_context& ctx) :
console_(console),
strand_(s),
ctx_(ctx) {
    refresh_term_size();
    current_screen_.resize(term_w_, term_h_);
    prev_screen_.resize(term_w_, term_h_);
    prev_tree_ = element::empty();
}

reconciler::~reconciler() {
    mounted_ = false;
    if (root_ != nullptr) {
        root_->cleanup();
        root_ = nullptr;
    }
}

function<void(component_base*)> reconciler::schedule_render_callback() {
    return [this](component_base* comp) { schedule_update(comp); };
}

void reconciler::refresh_term_size() {
    const auto sz = console_.get_console_size();
    if (sz.width > 0 && sz.height > 0) {
        term_w_ = sz.width;
        term_h_ = sz.height;
    }
}

void reconciler::setup_tree(component_base* comp) {
    comp->strand_ = &strand_;
    comp->ctx_ = &ctx_;
    comp->schedule_render_cb_ = schedule_render_callback();
    comp->setup();
}

void reconciler::build_focus_chain() {
    focus_chain_.clear();
    if (root_ == nullptr) {
        return;
    }

    vector<component_base*> stack;
    stack.push_back(root_);
    while (!stack.empty()) {
        auto* comp = stack.back();
        stack.pop_back();
        comp->collect_focusable(focus_chain_);
    }
}

void reconciler::set_focus(component_base* comp) {
    if (focused_ == comp) {
        return;
    }
    if (focused_ != nullptr) {
        focused_->set_has_focus(false);
    }
    focused_ = comp;
    if (focused_ != nullptr) {
        focused_->set_has_focus(true);
        focused_->take_focus();
    }
}

void reconciler::focus_next() {
    if (focus_chain_.empty()) {
        return;
    }
    size_t idx = 0;
    if (focused_ != nullptr) {
        const auto it = find(focus_chain_.begin(), focus_chain_.end(), focused_);
        if (it != focus_chain_.end()) {
            idx = static_cast<size_t>(it - focus_chain_.begin()) + 1;
        }
    }
    if (idx >= focus_chain_.size()) {
        idx = 0;
    }
    set_focus(focus_chain_[idx]);
}

void reconciler::focus_prev() {
    if (focus_chain_.empty()) {
        return;
    }
    size_t idx = focus_chain_.size() - 1;
    if (focused_ != nullptr) {
        const auto it = find(focus_chain_.begin(), focus_chain_.end(), focused_);
        if (it != focus_chain_.end() && it != focus_chain_.begin()) {
            idx = static_cast<size_t>(it - focus_chain_.begin()) - 1;
        }
    }
    set_focus(focus_chain_[idx]);
}

void reconciler::mount(component_base* root) {
    root_ = root;
    root_->parent_ = nullptr;
    setup_tree(root_);
    mounted_ = true;
    mark_dirty();
    flush();
}

void reconciler::schedule_update(component_base* comp) {
    if (!mounted_ || update_pending_) {
        return;
    }
    if (comp != nullptr && !comp->should_update()) {
        return;
    }
    update_pending_ = true;
    strand_.post([this] {
        update_pending_ = false;
        dirty_ = true;
        ctx_.post([this] { flush(); });
    });
}

void reconciler::mark_dirty() { dirty_ = true; }

bool reconciler::dispatch_key(const key_event& e) {
    if (root_ == nullptr || !mounted_) {
        return false;
    }

    if (e.key == key_event::type::tab || e.key == key_event::type::tab_reverse) {
        if (e.key == key_event::type::tab_reverse || (e.mods & key_modifier::shift) != key_modifier::none) {
            focus_prev();
        } else {
            focus_next();
        }
        mark_dirty();
        return true;
    }

    if (focused_ != nullptr) {
        if (focused_->on_key(e)) {
            return true;
        }
        if (!focused_->pass_through()) {
            return false;
        }
    }
    if (root_->on_key(e)) {
        return true;
    }
    return false;
}

bool reconciler::dispatch_mouse(const mouse_event& e) {
    if (root_ == nullptr || !mounted_) {
        return false;
    }
    if (dirty_) {
        flush();
    }

    if (dragging_scroll_state_ != nullptr && e.action == mouse_action::release) {
        dragging_scroll_state_ = nullptr;
        return true;
    }

    if (dragging_scroll_state_ != nullptr) {
        for (const auto& info: scrollbar_hits_) {
            if (info.scroll_state == dragging_scroll_state_) {
                const int max_scroll = max(1, info.content_h - info.track_h);
                const int track_range = max(1, info.track_h - info.thumb_size);
                auto* st = static_cast<state<int>*>(dragging_scroll_state_);
                if (info.vertical) {
                    int rel = e.y - info.track_y - info.thumb_size / 2;
                    rel = max(rel, 0);
                    rel = min(rel, track_range);
                    *st = rel * max_scroll / track_range;
                } else {
                    int rel = e.x - info.track_x - info.thumb_size / 2;
                    rel = max(rel, 0);
                    rel = min(rel, track_range);
                    *st = rel * max_scroll / track_range;
                }
                mark_dirty();
                return true;
            }
        }
    }

    if (e.action == mouse_action::wheel) {
        const int wheel_delta = (e.button == mouse_button::wheelup)     ? -3
                                : (e.button == mouse_button::wheeldown) ? 3
                                                                        : 0;
        if (wheel_delta != 0) {
            for (const auto& info: scrollbar_hits_) {
                if (!info.vertical) {
                    continue;
                }
                if (e.x >= info.sv_x && e.x < info.sv_x + info.sv_w && e.y >= info.track_y &&
                    e.y < info.track_y + info.track_h) {
                    auto* st = static_cast<state<int>*>(info.scroll_state);
                    const int max_scroll = max(0, info.content_h - info.track_h);
                    const int new_y = st->value() + wheel_delta;
                    *st = max(0, min(new_y, max_scroll));
                    mark_dirty();
                    return true;
                }
            }
        }
    }

    if (e.action == mouse_action::press) {
        for (const auto& info: scrollbar_hits_) {
            if (e.x == info.track_x && e.y >= info.track_y && e.y < info.track_y + info.track_h) {
                const int rel_y = e.y - info.track_y;
                const int max_scroll = max(1, info.content_h - info.track_h);
                const int track_range = max(1, info.track_h - info.thumb_size);
                if (rel_y >= info.thumb_pos && rel_y < info.thumb_pos + info.thumb_size) {
                    dragging_scroll_state_ = info.scroll_state;
                    drag_anchor_y_ = e.y;
                    auto* st = static_cast<state<int>*>(info.scroll_state);
                    drag_anchor_scroll_ = st->value();
                    return true;
                }
                int target_rel = rel_y - info.thumb_size / 2;
                target_rel = max(target_rel, 0);
                target_rel = min(target_rel, track_range);
                const int new_sy = target_rel * max_scroll / track_range;
                auto* st = static_cast<state<int>*>(info.scroll_state);
                *st = new_sy;
                dragging_scroll_state_ = info.scroll_state;
                drag_anchor_y_ = e.y;
                drag_anchor_scroll_ = new_sy;
                mark_dirty();
                return true;
            }

            if (e.y == info.track_y && e.x >= info.track_x && e.x < info.track_x + info.track_h) {
                const int rel_x = e.x - info.track_x;
                const int max_scroll = max(1, info.content_h - info.track_h);
                const int track_range = max(1, info.track_h - info.thumb_size);
                if (rel_x >= info.thumb_pos && rel_x < info.thumb_pos + info.thumb_size) {
                    dragging_scroll_state_ = info.scroll_state;
                    drag_anchor_y_ = e.x;
                    auto* st = static_cast<state<int>*>(info.scroll_state);
                    drag_anchor_scroll_ = st->value();
                    return true;
                }
                int target_rel = rel_x - info.thumb_size / 2;
                target_rel = max(target_rel, 0);
                target_rel = min(target_rel, track_range);
                const int new_sx = target_rel * max_scroll / track_range;
                auto* st = static_cast<state<int>*>(info.scroll_state);
                *st = new_sx;
                dragging_scroll_state_ = info.scroll_state;
                drag_anchor_y_ = e.x;
                drag_anchor_scroll_ = new_sx;
                mark_dirty();
                return true;
            }
        }
    }

    int idx = 0;
    const element* hitEl = find_element_at(prev_layout_, prev_tree_, e.x, e.y, idx);
    if (hitEl != nullptr && hitEl->on_click()) {
        hitEl->on_click()();
        if (root_ != nullptr && mounted_) {
            mark_dirty();
        }
        return true;
    }
    idx = 0;
    component_base* target = hit_test_at(prev_layout_, prev_tree_, e.x, e.y, idx);
    if (target != last_mouse_target_) {
        if (last_mouse_target_ != nullptr) {
            last_mouse_target_->on_mouse_leave();
        }
        last_mouse_target_ = target;
    }
    if (target != nullptr && root_ != nullptr) {
        if (e.action == mouse_action::press) {
            if (target->focusable()) {
                set_focus(target);
                mark_dirty();
            } else if (focused_ != nullptr) {
                set_focus(nullptr);
                mark_dirty();
            }
        }
        return target->on_mouse(e);
    } else if (e.action == mouse_action::press && focused_ != nullptr) {
        set_focus(nullptr);
        mark_dirty();
    }
    return false;
}

void reconciler::flush() {
    if (!mounted_ || root_ == nullptr) {
        return;
    }
    if (!dirty_) {
        return;
    }
    if (rendering_) {
        return;
    }
    rendering_ = true;
    dirty_ = false;

    refresh_term_size();

    if (current_screen_.dimx() != term_w_ || current_screen_.dimy() != term_h_) {
        current_screen_.resize(term_w_, term_h_);
        prev_screen_.resize(term_w_, term_h_);
    }

    element new_tree = root_->render();

    build_focus_chain();

    tag_owners(new_tree, root_);

    auto newLayout = compute_layout(new_tree, term_w_, term_h_);

    current_screen_.clear();

    scrollbar_hits_.clear();

    {
        int idx = 0;
        render_subtree(new_tree, newLayout, idx);
    }

    const string output = current_screen_.to_string(prev_screen_);

    prev_screen_ = current_screen_;
    prev_tree_ = move(new_tree);
    prev_layout_ = move(newLayout);

    if (!output.empty()) {
        console_.print_string(output);
    }
    rendering_ = false;
}

void reconciler::render_subtree(const element& el, const vector<layout_rect>& layout, int& idx) {
    switch (el.kind()) {
        case element::kind::empty: {
            return;
        }
        case element::kind::text: {
            if (idx < layout.size()) {
                const auto& rect = layout[static_cast<size_t>(idx)];
                render_text_block(rect.x, rect.y, rect.w, rect.h, el.text(), el.style(), el.wrap_mode());
                ++idx;
            }
            return;
        }
        case element::kind::button: {
            if (idx < layout.size()) {
                const auto& rect = layout[static_cast<size_t>(idx)];
                render_button(rect.x, rect.y, rect.w, rect.h, el.text(), el.style(), theme_, el.variant());
                ++idx;
            }
            return;
        }
        case element::kind::checkbox: {
            if (idx < layout.size()) {
                const auto& rect = layout[static_cast<size_t>(idx)];
                bool checked = false;
                if (el.state_ref() != nullptr) {
                    const auto* s = static_cast<state<bool>*>(el.state_ref());
                    checked = s->value();
                }
                render_checkbox(rect.x, rect.y, rect.w, rect.h, el.text(), checked, el.style());
                ++idx;
            }
            return;
        }
        case element::kind::text_input: {
            if (idx < layout.size()) {
                const auto& rect = layout[static_cast<size_t>(idx)];
                const bool focused = (focused_ != nullptr);

                string display;
                if (el.state_ref() != nullptr) {
                    const auto* s = static_cast<state<string>*>(el.state_ref());
                    display = s->value();
                } else {
                    display = el.text();
                }

                const style::wrap_mode wm = el.style().text_wrap.value_or(style::wrap_mode::none);
                const bool show_cursor = el.cursor_visible() && focused;

                const auto border_color = focused ? theme_.primary : theme_.border;
                apply_border(rect.x, rect.y, rect.w, rect.h, style::border::single, border_color);

                const bool tall = (rect.h >= 3);
                const int inner_x = rect.x + 1;
                const int inner_y = tall ? rect.y + 1 : rect.y;
                const int inner_w = max(1, rect.w - 2);
                const int inner_h = tall ? rect.h - 2 : 1;

                if (!display.empty()) {
                    int end_x = inner_x;
                    int end_y = inner_y;
                    render_text_block(inner_x, inner_y, inner_w, inner_h, display, el.style(), wm, &end_x, &end_y);
                    if (show_cursor) {
                        const size_t cp = min(el.cursor_pos(), display.size());
                        int cx = inner_x;
                        int cy = inner_y;
                        const auto& raw = display.view();
                        const auto* data = reinterpret_cast<const byte_t*>(raw.data());
                        for (utf8_iterator it(data, cp); it != utf8_iterator(); ++it) {
                            const int cw = max(1, it->display_width());
                            if (cx + cw > inner_x + inner_w) {
                                cx = inner_x;
                                cy += 1;
                            }
                            cx += cw;
                        }
                        if (cx < rect.x + rect.w) {
                            apply_text(cx, cy, "\xe2\x96\x8c", el.style());
                        }
                    }
                } else if (!el.placeholder().empty()) {
                    style ph_style;
                    ph_style.fg = theme_.muted;
                    apply_text(inner_x, inner_y, el.placeholder(), ph_style);
                    if (show_cursor) {
                        apply_text(inner_x, inner_y, "\xe2\x96\x8c", ph_style);
                    }
                } else if (show_cursor) {
                    apply_text(inner_x, inner_y, "\xe2\x96\x8c", el.style());
                }
                ++idx;
            }
            return;
        }
        case element::kind::separator: {
            if (idx < layout.size()) {
                const auto& rect = layout[static_cast<size_t>(idx)];
                render_separator(rect.x, rect.y, rect.w);
                ++idx;
            }
            return;
        }
        case element::kind::spacer: {
            ++idx;
            return;
        }
        case element::kind::scroll_view: {
            if (idx < layout.size()) {
                const auto& rect = layout[static_cast<size_t>(idx)];
                ++idx;
                const style& sv_style = el.style();
                const enum style::border sv_border = sv_style.border.value_or(style::border::single);
                apply_border(rect.x, rect.y, rect.w, rect.h, sv_border, theme_.border);
                if (!el.children().empty() && rect.w > 2 && rect.h > 2) {
                    using padding = struct style::padding;
                    const padding pad = sv_style.padding.value_or(padding{});
                    const clip_rect clip{rect.x + 1 + pad.left, rect.y + 1 + pad.top, rect.w - 2 - pad.left - pad.right,
                                         rect.h - 2 - pad.top - pad.bottom};
                    clip_stack_.push_back(clip);
                    apply_clear(clip.x, clip.y, clip.w, clip.h);

                    const int child_start_idx = idx;
                    render_subtree(el.children()[0], layout, idx);
                    const int child_end_idx = idx;

                    clip_stack_.pop_back();

                    int content_w = 0;
                    int content_h = 0;
                    for (int i = child_start_idx; i < child_end_idx && i < static_cast<int>(layout.size()); ++i) {
                        const auto& cr = layout[i];
                        content_w = max(content_w, cr.x + cr.w);
                        content_h = max(content_h, cr.y + cr.h);
                    }
                    content_w = max(0, content_w - clip.x);
                    content_h = max(0, content_h - clip.y);

                    const int max_scroll_x = max(0, content_w - clip.w);
                    const int max_scroll_y = max(0, content_h - clip.h);
                    const int sx = min(el.scroll_x(), max_scroll_x);
                    const int sy = min(el.scroll_y(), max_scroll_y);

                    // keep component state in sync with clamped display values
                    if (el.scroll_x_state() != nullptr && el.scroll_x() != sx) {
                        *static_cast<state<int>*>(el.scroll_x_state()) = sx;
                    }
                    if (el.scroll_y_state() != nullptr && el.scroll_y() != sy) {
                        *static_cast<state<int>*>(el.scroll_y_state()) = sy;
                    }

                    if (content_h > clip.h) {
                        const int bar_x = rect.x + rect.w - 1;
                        const int thumb_size = max(1, clip.h * clip.h / max(1, content_h));
                        int thumb_pos = 0;
                        {
                            const int ms = max(1, content_h - clip.h);
                            thumb_pos = (clip.h - thumb_size) * sy / ms;
                        }

                        if (el.scroll_y_state() != nullptr) {
                            scrollbar_hits_.push_back({el.scroll_y_state(), bar_x, clip.y, clip.h, thumb_pos,
                                                       thumb_size, content_h, rect.x, rect.w, true});
                        }
                        for (int i = 0; i < clip.h; ++i) {
                            if (bar_x >= 0 && bar_x < term_w_ && clip.y + i >= 0 && clip.y + i < term_h_) {
                                auto& cell = current_screen_.fast_cell_at(bar_x, clip.y + i);
                                cell.character = (i >= thumb_pos && i < thumb_pos + thumb_size) ? "\xe2\x96\x88"
                                                                                                : "\xe2\x94\x82";
                                cell.foreground = theme_.border;
                            }
                        }
                    }

                    if (content_w > clip.w) {
                        const int barY = rect.y + rect.h - 1;
                        const int thumb_size = max(1, clip.w * clip.w / max(1, content_w));
                        int thumb_pos = 0;
                        {
                            const int ms = max(1, content_w - clip.w);
                            thumb_pos = (clip.w - thumb_size) * sx / ms;
                        }

                        if (el.scroll_x_state() != nullptr) {
                            scrollbar_hits_.push_back({el.scroll_x_state(), clip.x, barY, clip.w, thumb_pos, thumb_size,
                                                       content_w, rect.x, rect.w, false});
                        }
                        for (int i = 0; i < clip.w; ++i) {
                            if (clip.x + i >= 0 && clip.x + i < term_w_ && barY >= 0 && barY < term_h_) {
                                auto& cell = current_screen_.fast_cell_at(clip.x + i, barY);
                                cell.character = (i >= thumb_pos && i < thumb_pos + thumb_size) ? "\xe2\x96\x88"
                                                                                                : "\xe2\x94\x80";
                                cell.foreground = theme_.border;
                            }
                        }
                    }
                }
            }
            return;
        }
        case element::kind::vbox:
        case element::kind::hbox:
        case element::kind::zstack:
        case element::kind::each:
        case element::kind::when:
        case element::kind::flexbox:
        case element::kind::gridbox: {
            for (const auto& child: el.children()) {
                render_subtree(child, layout, idx);
            }
            return;
        }
        case element::kind::canvas: {
            if (idx < layout.size()) {
                const auto& rect = layout[static_cast<size_t>(idx)];
                const auto& fn = el.draw_function();
                if (fn) {
                    fn(rect.x, rect.y, rect.w, rect.h);
                }
                ++idx;
            }
            return;
        }
        default: {
            return;
        }
    }
}

void reconciler::clear_element_area(const element& el, const vector<layout_rect>& layout, int& idx) {
    switch (el.kind()) {
        case element::kind::text:
        case element::kind::button:
        case element::kind::checkbox:
        case element::kind::text_input:
        case element::kind::separator:
        case element::kind::spacer: {
            if (idx < layout.size()) {
                const auto& rect = layout[static_cast<size_t>(idx)];
                apply_clear(rect.x, rect.y, rect.w, rect.h);
                ++idx;
            }
            return;
        }
        case element::kind::scroll_view: {
            if (idx < layout.size()) {
                const auto& rect = layout[static_cast<size_t>(idx)];
                apply_clear(rect.x, rect.y, rect.w, rect.h);
                ++idx;
            }
            for (const auto& child: el.children()) {
                clear_element_area(child, layout, idx);
            }
            return;
        }
        case element::kind::vbox:
        case element::kind::hbox:
        case element::kind::zstack:
        case element::kind::each:
        case element::kind::when:
        case element::kind::flexbox:
        case element::kind::gridbox: {
            for (const auto& child: el.children()) {
                clear_element_area(child, layout, idx);
            }
            return;
        }
        default: {
            return;
        }
    }
}

const element* reconciler::find_element_at(const vector<layout_rect>& layout, const element& tree, int mx, int my,
                                           int& idx) {
    switch (tree.kind()) {
        case element::kind::text:
        case element::kind::button:
        case element::kind::checkbox:
        case element::kind::text_input:
        case element::kind::separator:
        case element::kind::spacer: {
            if (idx < layout.size()) {
                const auto& rect = layout[static_cast<size_t>(idx)];
                ++idx;
                if (mx >= rect.x && mx < rect.x + rect.w && my >= rect.y && my < rect.y + rect.h) {
                    return &tree;
                }
            }
            return nullptr;
        }
        case element::kind::scroll_view: {
            if (idx < layout.size()) {
                const auto& rect = layout[static_cast<size_t>(idx)];
                ++idx;
                if (mx >= rect.x && mx < rect.x + rect.w && my >= rect.y && my < rect.y + rect.h) {
                    if (tree.on_click()) {
                        return &tree;
                    }
                }
            }
            for (const auto& child: tree.children()) {
                const element* hit = find_element_at(layout, child, mx, my, idx);
                if (hit != nullptr) {
                    return hit;
                }
            }
            return nullptr;
        }
        case element::kind::vbox:
        case element::kind::hbox:
        case element::kind::zstack:
        case element::kind::each:
        case element::kind::when:
        case element::kind::flexbox:
        case element::kind::gridbox: {
            for (const auto& child: tree.children()) {
                const element* hit = find_element_at(layout, child, mx, my, idx);
                if (hit != nullptr) {
                    return hit;
                }
            }
            return nullptr;
        }
        default: {
            return nullptr;
        }
    }
}

component_base* reconciler::hit_test_at(const vector<layout_rect>& layout, const element& tree, int mx, int my,
                                        int& idx) {
    switch (tree.kind()) {
        case element::kind::text:
        case element::kind::button:
        case element::kind::checkbox:
        case element::kind::text_input:
        case element::kind::separator:
        case element::kind::spacer: {
            if (idx < layout.size()) {
                const auto& rect = layout[static_cast<size_t>(idx)];
                ++idx;
                if (mx >= rect.x && mx < rect.x + rect.w && my >= rect.y && my < rect.y + rect.h) {
                    auto* owner = static_cast<component_base*>(tree.owner());
                    return owner != nullptr ? owner : root_;
                }
            }
            return nullptr;
        }
        case element::kind::scroll_view: {
            if (idx < layout.size()) {
                const auto& rect = layout[static_cast<size_t>(idx)];
                ++idx;
                if (mx >= rect.x && mx < rect.x + rect.w && my >= rect.y && my < rect.y + rect.h) {
                    auto* owner = static_cast<component_base*>(tree.owner());
                    if (owner != nullptr) {
                        return owner;
                    }
                }
            }
            for (const auto& child: tree.children()) {
                auto* hit = hit_test_at(layout, child, mx, my, idx);
                if (hit != nullptr) {
                    return hit;
                }
            }
            return nullptr;
        }
        case element::kind::vbox:
        case element::kind::hbox:
        case element::kind::zstack:
        case element::kind::each:
        case element::kind::when:
        case element::kind::flexbox:
        case element::kind::gridbox: {
            for (const auto& child: tree.children()) {
                auto* hit = hit_test_at(layout, child, mx, my, idx);
                if (hit != nullptr) {
                    return hit;
                }
            }
            return nullptr;
        }
        default: {
            return nullptr;
        }
    }
}

void reconciler::apply_style_to_cell(cell& cell, const style& style) {
    if (style.fg.has_value()) {
        cell.foreground = style.fg.value();
    } else {
        cell.foreground = theme_.fg;
    }
    if (style.bg.has_value()) {
        cell.background = style.bg.value();
    }
    if (style.bold.has_value()) {
        cell.bold = style.bold.value();
    }
    if (style.dim.has_value()) {
        cell.dim = style.dim.value();
    }
    if (style.underline.has_value()) {
        cell.underlined = style.underline.value();
    }
    if (style.underlined_double.has_value()) {
        cell.underlined_double = style.underlined_double.value();
    }
    if (style.italic.has_value()) {
        cell.italic = style.italic.value();
    }
    if (style.reverse.has_value()) {
        cell.inverted = style.reverse.value();
    }
    if (style.blink.has_value()) {
        cell.blink = style.blink.value();
    }
    if (style.strikethrough.has_value()) {
        cell.strikethrough = style.strikethrough.value();
    }
}

void reconciler::apply_text(int x, int y, const string& text, const style& style) { apply_text(x, y, text, style, -1); }

void reconciler::apply_text(int x, int y, const string& text, const style& style, int max_x) {
    if (x < 0 || y < 0 || x >= term_w_ || y >= term_h_) {
        return;
    }
    const int clip_limit = (max_x >= 0) ? min(max_x, term_w_) : term_w_;
    const auto& raw = text.view();
    int cx = x;
    for (utf8_iterator it(reinterpret_cast<const byte_t*>(raw.data()), raw.size()); it != utf8_iterator(); ++it) {
        if (cx >= clip_limit) {
            break;
        }
        bool skip = false;
        for (const auto& clip: clip_stack_) {
            if (cx < clip.x || cx >= clip.x + clip.w || y < clip.y || y >= clip.y + clip.h) {
                skip = true;
                break;
            }
        }
        if (!skip) {
            auto& cell = current_screen_.fast_cell_at(cx, y);
            cell.reset();
            it->append_to(cell.character);
            apply_style_to_cell(cell, style);
        }
        const int cw = it->display_width();
        for (int w = 1; w < cw && cx + w < term_w_; ++w) {
            auto& next = current_screen_.fast_cell_at(cx + w, y);
            next.reset();
            next.automerge = true;
        }
        cx += max(1, cw);
    }
}

void reconciler::apply_border(int x, int y, int w, int h, enum style::border border, const _NEFORCE color& c) {
    if (border == style::border::none || w < 2 || h < 2) {
        return;
    }
    const auto* tl = "\xe2\x94\x8c"; // ┌
    const auto* tr = "\xe2\x94\x90"; // ┐
    const auto* bl = "\xe2\x94\x94"; // └
    const auto* br = "\xe2\x94\x98"; // ┘
    const auto* hz = "\xe2\x94\x80"; // ─
    const auto* vt = "\xe2\x94\x82"; // │

    switch (border) {
        case style::border::double_: {
            tl = "\xe2\x95\x94";
            tr = "\xe2\x95\x97"; // ╔ ╗
            bl = "\xe2\x95\x9a";
            br = "\xe2\x95\x9d"; // ╚ ╝
            hz = "\xe2\x95\x90";
            vt = "\xe2\x95\x91"; // ═ ║
            break;
        }
        case style::border::rounded: {
            tl = "\xe2\x95\xad";
            tr = "\xe2\x95\xae"; // ╭ ╮
            bl = "\xe2\x95\xb0";
            br = "\xe2\x95\xaf"; // ╰ ╯
            hz = "\xe2\x94\x80";
            vt = "\xe2\x94\x82"; // ─ │
            break;
        }
        default: {
            break;
        }
    }

    auto set_border_cell = [this, &c](int bx, int by, const char* ch) {
        if (bx >= 0 && bx < term_w_ && by >= 0 && by < term_h_) {
            auto& cell = current_screen_.fast_cell_at(bx, by);
            cell.character = ch;
            cell.foreground = c;
        }
    };

    for (int i = 0; i < w; ++i) {
        set_border_cell(x + i, y, i == 0 ? tl : (i == w - 1 ? tr : hz));
    }
    for (int i = 1; i < h - 1; ++i) {
        set_border_cell(x, y + i, vt);
        set_border_cell(x + w - 1, y + i, vt);
    }
    for (int i = 0; i < w; ++i) {
        set_border_cell(x + i, y + h - 1, i == 0 ? bl : (i == w - 1 ? br : hz));
    }
}

void reconciler::apply_clear(int x, int y, int w, int h) {
    if (w <= 0 || h <= 0) {
        return;
    }
    for (const auto& clip: clip_stack_) {
        const int nx = max(x, clip.x);
        const int ny = max(y, clip.y);
        const int nw = min(x + w, clip.x + clip.w) - nx;
        const int nh = min(y + h, clip.y + clip.h) - ny;
        x = nx;
        y = ny;
        w = nw;
        h = nh;
        if (w <= 0 || h <= 0) {
            return;
        }
    }
    for (int i = 0; i < h; ++i) {
        for (int j = 0; j < w; ++j) {
            const int cx = x + j;
            const int cy = y + i;
            if (cx >= 0 && cx < term_w_ && cy >= 0 && cy < term_h_) {
                auto& cell = current_screen_.fast_cell_at(cx, cy);
                cell.reset();
                cell.character = " ";
            }
        }
    }
}

void reconciler::render_button(int x, int y, int w, int h, const string& label, const style& style, const theme& theme,
                               style::variant variant) {
    tui::style s = theme.button_style(variant);
    s = style::merge(s, style);

    const auto borderColor = s.borderColor.value_or(theme.border);
    apply_border(x, y, w, h, style::border::single, borderColor);

    const int inner_w = max(1, w - 2);
    if (h >= 3) {
        render_text_block(x + 1, y + 1, inner_w, h - 2, label, s, style::wrap_mode::none);
    } else {
        int label_x = x + (w - static_cast<int>(label.size())) / 2;
        label_x = max(label_x, x + 1);
        render_text_block(label_x, y, min(inner_w, w - (label_x - x)), 1, label, s, style::wrap_mode::none);
    }
}

void reconciler::render_checkbox(int x, int y, int w, int h, const string& label, bool checked, const style& style) {
    string display = checked ? "[x] " : "[ ] ";
    display += label;
    render_text_block(x, y, w, h, display, style, style::wrap_mode::word);
}

void reconciler::render_separator(int x, int y, int w) {
    if (w <= 0) {
        return;
    }
    for (int i = 0; i < w && x + i < term_w_ && y < term_h_; ++i) {
        auto& cell = current_screen_.fast_cell_at(x + i, y);
        cell.character = "-";
        cell.foreground = theme_.border;
    }
}

void reconciler::render_text_block(int x, int y, int w, int h, const string& text, const tui::style& style,
                                   style::wrap_mode wm, int* out_end_x, int* out_end_y) {
    if (w <= 0 || h <= 0 || text.empty()) {
        if (out_end_x != nullptr) {
            *out_end_x = x;
        }
        if (out_end_y != nullptr) {
            *out_end_y = y;
        }
        return;
    }

    if (wm == style::wrap_mode::none) {
        apply_text(x, y, text, style, x + w);
        if (out_end_x != nullptr) {
            int end_x = x;
            const auto& raw = text.view();
            for (utf8_iterator it(reinterpret_cast<const byte_t*>(raw.data()), raw.size());
                 it != utf8_iterator() && end_x < x + w; ++it) {
                end_x += max(1, it->display_width());
            }
            *out_end_x = end_x;
        }
        if (out_end_y != nullptr) {
            *out_end_y = y;
        }
        return;
    }

    const auto& raw = text.view();
    const auto* data = reinterpret_cast<const byte_t*>(raw.data());
    const size_t len = raw.size();

    int cx = x;
    int cy = y;

    const auto render_codepoint = [this, &style](int px, int py, const utf8_iterator& it) {
        if (px >= 0 && px < term_w_ && py >= 0 && py < term_h_) {
            bool skip = false;
            for (const auto& clip: clip_stack_) {
                if (px < clip.x || px >= clip.x + clip.w || py < clip.y || py >= clip.y + clip.h) {
                    skip = true;
                    break;
                }
            }
            if (!skip) {
                auto& cell = current_screen_.fast_cell_at(px, py);
                cell.reset();
                it->append_to(cell.character);
                apply_style_to_cell(cell, style);
            }
        }
        const int cw = it->display_width();
        for (int ww = 1; ww < cw && px + ww < term_w_; ++ww) {
            auto& next = current_screen_.fast_cell_at(px + ww, py);
            next.reset();
            next.automerge = true;
        }
        return max(1, cw);
    };

    if (wm == style::wrap_mode::character) {
        for (utf8_iterator it(data, len); it != utf8_iterator(); ++it) {
            const int cpw = it->display_width();
            if (cx + max(1, cpw) > x + w) {
                cy += 1;
                cx = x;
                if (cy >= y + h) {
                    break;
                }
            }
            cx += render_codepoint(cx, cy, it);
        }
        if (out_end_x != nullptr) {
            *out_end_x = cx;
        }
        if (out_end_y != nullptr) {
            *out_end_y = cy;
        }
        return;
    }

    size_t pos = 0;
    while (pos < len && cy < y + h) {
        if (cx == x) {
            while (pos < len && data[pos] == ' ') {
                ++pos;
            }
            if (pos >= len) {
                break;
            }
        }

        const size_t word_start = pos;
        size_t word_end = pos;
        while (word_end < len && data[word_end] != ' ') {
            const byte_t lead = data[word_end];
            if ((lead & 0x80) == 0) {
                ++word_end;
            } else if ((lead & 0xE0) == 0xC0) {
                word_end += 2;
            } else if ((lead & 0xF0) == 0xE0) {
                word_end += 3;
            } else if ((lead & 0xF8) == 0xF0) {
                word_end += 4;
            } else {
                ++word_end;
            }
        }

        int word_w = 0;
        for (utf8_iterator it(data + word_start, word_end - word_start); it != utf8_iterator(); ++it) {
            word_w += it->display_width();
        }

        if (word_w == 0 && word_start < len && data[word_start] == ' ') {
            if (cx + 1 > x + w) {
                cy += 1;
                cx = x;
                if (cy >= y + h) {
                    break;
                }
            }
            cx += render_codepoint(cx, cy, utf8_iterator(reinterpret_cast<const byte_t*>(" "), 1));
            ++pos;
            continue;
        }

        const int space_w = (cx > x) ? 1 : 0;
        if (cx > x && cx + space_w + word_w > x + w) {
            cy += 1;
            cx = x;
            if (cy >= y + h) {
                break;
            }
            pos = word_start;
            while (pos < len && data[pos] == ' ') {
                ++pos;
            }
            continue;
        }

        if (cx > x) {
            cx += render_codepoint(cx, cy, utf8_iterator(reinterpret_cast<const byte_t*>(" "), 1));
        }

        if (word_w > w && cx == x) {
            for (utf8_iterator it(data + word_start, word_end - word_start); it != utf8_iterator(); ++it) {
                const int cpw = it->display_width();
                if (cx + max(1, cpw) > x + w) {
                    cy += 1;
                    cx = x;
                    if (cy >= y + h) {
                        break;
                    }
                }
                cx += render_codepoint(cx, cy, it);
            }
        } else {
            for (utf8_iterator it(data + word_start, word_end - word_start); it != utf8_iterator(); ++it) {
                cx += render_codepoint(cx, cy, it);
            }
        }

        pos = word_end;
    }
    if (out_end_x != nullptr) {
        *out_end_x = cx;
    }
    if (out_end_y != nullptr) {
        *out_end_y = cy;
    }
}

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
