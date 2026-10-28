#include <NeForce/core/string/utf_iterator.hpp>
#include <NeForce/core/string/width.hpp>
#include <NeForce/tui/component/component.hpp>
#include <NeForce/tui/dom/layout.hpp>
#include <NeForce/tui/dom/text_layout.hpp>
#include <NeForce/tui/renderer.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

namespace {
    string truncate_to_cols(const string_view text, const int max_cols) {
        if (max_cols <= 0 || text.empty()) {
            return string();
        }
        string out;
        int used = 0;
        for (utf8_iterator it(reinterpret_cast<const byte_t*>(text.data()), text.size()); it != utf8_iterator(); ++it) {
            const codepoint& cp = *it;
            const int width = cp.display_width();
            if (used + width > max_cols) {
                break;
            }
            cp.append_to(out);
            used += width;
        }
        return out;
    }

    void put_codepoint(const codepoint& cp, const int width, cell& target) {
        const uint32_t value = cp.value();
        if (value < 0x20 || value == 0x7F) {
            target.character = " ";
            return;
        }
        target.character.set(cp, width);
    }

    struct virtual_window {
        int first;      // index of the first visible item
        int skip;       // rows of the first item that are scrolled out
        int visible;    // number of items to render
        int max_scroll; // largest valid scroll offset
        int content_h;  // total content height in rows
    };

    int item_rows_at(const element& el, const size_t index) {
        const auto* heights = el.item_heights();
        if (heights == nullptr || index >= heights->size()) {
            return max(1, el.item_height_hint());
        }
        const int measured = (*heights)[index];
        return (measured > 0) ? measured : max(1, el.item_height_hint());
    }

    virtual_window compute_virtual_window(const element& el, const int area_h) {
        const int count = static_cast<int>(el.item_count());
        const int area = max(0, area_h);

        virtual_window win{};
        if (el.item_heights() == nullptr) {
            const int item_h = max(1, el.item_height());
            win.content_h = count * item_h;
            win.max_scroll = max(0, win.content_h - area);
            const int scroll = (el.scroll_y() < 0) ? 0 : min(el.scroll_y(), win.max_scroll);
            win.first = scroll / item_h;
            win.skip = scroll % item_h;
            win.visible = (area + item_h - 1) / item_h + 1;
            return win;
        }

        int total = 0;
        for (int i = 0; i < count; ++i) {
            total += item_rows_at(el, static_cast<size_t>(i));
        }
        win.content_h = total;
        win.max_scroll = max(0, total - area);
        const int scroll = (el.scroll_y() < 0) ? 0 : min(el.scroll_y(), win.max_scroll);

        int consumed = 0;
        int first = 0;
        while (first < count) {
            const int rows = item_rows_at(el, static_cast<size_t>(first));
            if (consumed + rows > scroll) {
                break;
            }
            consumed += rows;
            ++first;
        }
        win.first = min(first, max(0, count - 1));
        win.skip = max(0, scroll - consumed);
        win.visible = 0;
        int filled = (win.skip > 0) ? win.skip : 0;
        for (int i = win.first; i < count && filled < area; ++i) {
            filled += item_rows_at(el, static_cast<size_t>(i));
            ++win.visible;
        }
        win.visible = (win.visible == 0) ? 1 : win.visible + 1;
        return win;
    }

} // namespace


renderer::renderer(screen& target, const theme& t, const int term_w, const int term_h) :
screen_(target),
theme_(t),
term_w_(term_w),
term_h_(term_h) {}

void renderer::render(const element& tree, const vector<layout_rect>& layout, const bool has_focus) {
    ++stats_.frames;
    painted_this_frame_ = 0;
    has_focus_ = has_focus;
    scrollbar_hits_.clear();
    int idx = 0;
    render_subtree(tree, layout, idx);
    stats_.last_frame_cells = painted_this_frame_;
    stats_.total_cells += painted_this_frame_;
}

void renderer::render_subtree(const element& el, const vector<layout_rect>& layout, int& idx) {
    switch (el.kind()) {
        case element::kind::empty: {
            return;
        }
        case element::kind::text: {
            if (compare_less(idx, layout.size())) {
                const auto& rect = layout[static_cast<size_t>(idx)];
                ++idx;
                if (!is_rect_visible(rect.x, rect.y, rect.w, rect.h)) {
                    return;
                }
                render_text_block(rect.x, rect.y, rect.w, rect.h, el.text(), el.style(), el.wrap_mode(), nullptr,
                                  nullptr, 0, &el);
            }
            return;
        }
        case element::kind::button: {
            if (compare_less(idx, layout.size())) {
                const auto& rect = layout[static_cast<size_t>(idx)];
                ++idx;
                if (!is_rect_visible(rect.x, rect.y, rect.w, rect.h)) {
                    return;
                }
                render_button(rect.x, rect.y, rect.w, rect.h, el.text(), el.style(), theme_, el.variant());
            }
            return;
        }
        case element::kind::checkbox: {
            if (compare_less(idx, layout.size())) {
                const auto& rect = layout[static_cast<size_t>(idx)];
                ++idx;
                if (!is_rect_visible(rect.x, rect.y, rect.w, rect.h)) {
                    return;
                }
                bool checked = false;
                if (el.state_ref() != nullptr) {
                    const auto* s = static_cast<state<bool>*>(el.state_ref());
                    checked = s->value();
                }
                render_checkbox(rect.x, rect.y, rect.w, rect.h, el.text(), checked, el.style());
            }
            return;
        }
        case element::kind::text_input: {
            if (compare_less(idx, layout.size())) {
                const auto& rect = layout[static_cast<size_t>(idx)];

                string display;
                if (el.state_ref() != nullptr) {
                    const auto* s = static_cast<state<string>*>(el.state_ref());
                    display = s->value();
                } else {
                    display = el.text();
                }

                const style::wrap_mode wm = el.style().text_wrap.value_or(style::wrap_mode::none);
                const bool show_cursor = el.cursor_visible() && has_focus_;

                if (el.rect_state() != nullptr) {
                    auto* box = static_cast<state<layout_rect>*>(el.rect_state());
                    if (box->value().x != rect.x || box->value().y != rect.y || box->value().w != rect.w ||
                        box->value().h != rect.h) {
                        *box = rect;
                    }
                }

                const enum style::border border_kind = el.style().border.value_or(style::border::single);
                const int border_w = (border_kind == style::border::none) ? 0 : 1;
                const auto border_color = has_focus_ ? theme_.primary : theme_.border;
                if (border_w > 0) {
                    apply_border(rect.x, rect.y, rect.w, rect.h, border_kind, border_color);
                }

                const bool framed = (rect.h >= 2 * border_w + 1);
                const int inner_x = rect.x + border_w;
                const int inner_y = framed ? rect.y + border_w : rect.y;
                const int inner_w = max(1, rect.w - 2 * border_w);
                const int inner_h = framed ? max(1, rect.h - 2 * border_w) : 1;
                const int scroll_row = max(0, el.scroll_y());

                if (!display.empty()) {
                    const text_layout text_lay = layout_text(display.view(), inner_w, wm);
                    render_text_block(inner_x, inner_y, inner_w, inner_h, display, el.style(), wm, nullptr, nullptr,
                                      scroll_row);

                    const size_t sel_begin = min(el.selection_begin(), display.size());
                    const size_t sel_end = min(el.selection_end(), display.size());
                    if (sel_end > sel_begin) {
                        apply_selection_overlay(inner_x, inner_y, inner_w, inner_h, display, text_lay, sel_begin,
                                                sel_end, scroll_row);
                    }

                    if (show_cursor) {
                        int row = 0;
                        int col = 0;
                        caret_position(text_lay, display.view(), min(el.cursor_pos(), display.size()), row, col);
                        const int visible_row = row - scroll_row;
                        const int cx = inner_x + min(col, max(0, inner_w - 1));
                        const int cy = inner_y + visible_row;
                        if (visible_row >= 0 && visible_row < inner_h && cx < rect.x + rect.w) {
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
        case element::kind::band: {
            if (compare_less(idx, layout.size())) {
                const auto& rect = layout[static_cast<size_t>(idx)];
                ++idx;
                if (el.rect_state() != nullptr) {
                    auto* box = static_cast<state<layout_rect>*>(el.rect_state());
                    if (box->value().x != rect.x || box->value().y != rect.y || box->value().w != rect.w ||
                        box->value().h != rect.h) {
                        *box = rect;
                    }
                }
                const auto fill = el.style().bg;
                if (fill.has_value()) {
                    for (int row = 0; row < rect.h; ++row) {
                        const int cy = rect.y + row;
                        if (cy < 0 || cy >= term_h_) {
                            continue;
                        }
                        for (int col = 0; col < rect.w; ++col) {
                            const int cx = rect.x + col;
                            if (cx >= 0 && cx < term_w_ && !is_clipped(cx, cy)) {
                                auto& cell = screen_.fast_cell_at(cx, cy);
                                cell.background = *fill;
                            }
                        }
                    }
                    band_stack_.push_back(*fill);
                }
                for (const auto& child: el.children()) {
                    render_subtree(child, layout, idx);
                }
                if (fill.has_value()) {
                    band_stack_.pop_back();
                }
            }
            return;
        }
        case element::kind::panel: {
            if (compare_less(idx, layout.size())) {
                const auto& rect = layout[static_cast<size_t>(idx)];
                ++idx;
                const auto& panel_style = el.style();
                const optional<_NEFORCE color> fill = panel_style.bg;
                if (fill.has_value()) {
                    for (int row = 0; row < rect.h; ++row) {
                        const int cy = rect.y + row;
                        if (cy < 0 || cy >= term_h_) {
                            continue;
                        }
                        for (int column = 0; column < rect.w; ++column) {
                            const int cx = rect.x + column;
                            if (cx >= 0 && cx < term_w_ && !is_clipped(cx, cy)) {
                                screen_.fast_cell_at(cx, cy).background = *fill;
                            }
                        }
                    }
                    band_stack_.push_back(*fill);
                }
                const enum style::border b = panel_style.border.value_or(style::border::none);
                if (b != style::border::none) {
                    apply_border(rect.x, rect.y, rect.w, rect.h, b, panel_style.borderColor.value_or(theme_.border));
                }
                if (!el.text().empty() && rect.w >= 6) {
                    const string title = truncate_to_cols(el.text().view(), rect.w - 4);
                    const int title_w = text_display_width(title.view());
                    if (title_w > 0) {
                        style clear_style;
                        if (fill.has_value()) {
                            clear_style.bg = *fill;
                        }
                        apply_text(rect.x + 2, rect.y, string(static_cast<size_t>(title_w + 2), ' '), clear_style);
                        style title_style;
                        title_style.fg = panel_style.fg.has_value() ? *panel_style.fg : theme_.primary;
                        title_style.bold = true;
                        if (fill.has_value()) {
                            title_style.bg = *fill;
                        }
                        apply_text(rect.x + 3, rect.y, title, title_style);
                    }
                }
                for (const auto& child: el.children()) {
                    render_subtree(child, layout, idx);
                }
                if (fill.has_value()) {
                    band_stack_.pop_back();
                }
            }
            return;
        }
        case element::kind::rich_text: {
            if (compare_less(idx, layout.size())) {
                const auto& rect = layout[static_cast<size_t>(idx)];
                ++idx;
                if (!is_rect_visible(rect.x, rect.y, rect.w, rect.h)) {
                    return;
                }
                render_rich_text(rect.x, rect.y, rect.w, rect.h, el.spans(), el.style(), el.wrap_mode(), nullptr,
                                 nullptr, el.hang().empty() ? nullptr : &el.hang(), el.hang_width(), &el);
            }
            return;
        }
        case element::kind::virtual_list: {
            if (compare_less(idx, layout.size())) {
                const auto& rect = layout[static_cast<size_t>(idx)];
                ++idx;
                render_virtual_list(el, rect);
            }
            return;
        }
        case element::kind::separator: {
            if (compare_less(idx, layout.size())) {
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
            if (compare_less(idx, layout.size())) {
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

                    int content_left = numeric_traits<int>::max();
                    int content_top = numeric_traits<int>::max();
                    int content_right = 0;
                    int content_bottom = 0;
                    for (int i = child_start_idx; i < child_end_idx && compare_less(i, layout.size()); ++i) {
                        const auto& cr = layout[i];
                        content_left = min(content_left, cr.x);
                        content_top = min(content_top, cr.y);
                        content_right = max(content_right, cr.x + cr.w);
                        content_bottom = max(content_bottom, cr.y + cr.h);
                    }
                    int content_w = max(0, content_right - content_left);
                    int content_h = max(0, content_bottom - content_top);

                    const int max_scroll_x = max(0, content_w - clip.w);
                    const int max_scroll_y = max(0, content_h - clip.h);
                    const int sx = min(el.scroll_x(), max_scroll_x);
                    const int sy = min(el.scroll_y(), max_scroll_y);

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

                        scrollbar_hits_.push_back({el.scroll_y_state(), bar_x, clip.y, clip.h, thumb_pos, thumb_size,
                                                   content_h, rect.x, rect.w, true});
                        for (int i = 0; i < clip.h; ++i) {
                            if (bar_x >= 0 && bar_x < term_w_ && clip.y + i >= 0 && clip.y + i < term_h_) {
                                auto& cell = screen_.fast_cell_at(bar_x, clip.y + i);
                                cell.character = (i >= thumb_pos && i < thumb_pos + thumb_size) ? "\xe2\x96\x88"
                                                                                                : "\xe2\x94\x82";
                                cell.foreground = theme_.border;
                            }
                        }
                    }

                    if (content_w > clip.w && el.style().show_horizontal_scrollbar.value_or(true)) {
                        const int barY = rect.y + rect.h - 1;
                        const int thumb_size = max(1, clip.w * clip.w / max(1, content_w));
                        int thumb_pos = 0;
                        {
                            const int ms = max(1, content_w - clip.w);
                            thumb_pos = (clip.w - thumb_size) * sx / ms;
                        }

                        scrollbar_hits_.push_back({el.scroll_x_state(), clip.x, barY, clip.w, thumb_pos, thumb_size,
                                                   content_w, rect.x, rect.w, false});
                        for (int i = 0; i < clip.w; ++i) {
                            if (clip.x + i >= 0 && clip.x + i < term_w_ && barY >= 0 && barY < term_h_) {
                                auto& cell = screen_.fast_cell_at(clip.x + i, barY);
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
            if (compare_less(idx, layout.size())) {
                const auto& rect = layout[static_cast<size_t>(idx)];
                if (el.rect_state() != nullptr) {
                    auto* box = static_cast<state<layout_rect>*>(el.rect_state());
                    if (box->value().x != rect.x || box->value().y != rect.y || box->value().w != rect.w ||
                        box->value().h != rect.h) {
                        *box = rect;
                    }
                }
                const auto& fn = el.draw_function();
                if (fn) {
                    fn(rect.x, rect.y, rect.w, rect.h);
                }
                const auto& painter_fn = el.painter_function();
                if (painter_fn) {
                    // Local clip is the element rectangle intersected with every active
                    // clip rectangle on the renderer stack.
                    int clip_x = 0;
                    int clip_y = 0;
                    int clip_w = rect.w;
                    int clip_h = rect.h;
                    for (const auto& clip: clip_stack_) {
                        const int lx = clip.x - rect.x;
                        const int ly = clip.y - rect.y;
                        const int x0 = max(clip_x, lx);
                        const int y0 = max(clip_y, ly);
                        const int x1 = min(clip_x + clip_w, lx + clip.w);
                        const int y1 = min(clip_y + clip_h, ly + clip.h);
                        clip_x = x0;
                        clip_y = y0;
                        clip_w = max(0, x1 - x0);
                        clip_h = max(0, y1 - y0);
                    }
                    canvas_painter painter(screen_, theme_, rect.x, rect.y, rect.w, rect.h, el.style(), clip_x, clip_y,
                                           clip_w, clip_h);
                    painter_fn(painter);
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

void renderer::clear_element_area(const element& el, const vector<layout_rect>& layout, int& idx) {
    switch (el.kind()) {
        case element::kind::text:
        case element::kind::rich_text:
        case element::kind::button:
        case element::kind::checkbox:
        case element::kind::text_input:
        case element::kind::separator:
        case element::kind::spacer:
        case element::kind::virtual_list:
        case element::kind::canvas: {
            if (compare_less(idx, layout.size())) {
                const auto& rect = layout[static_cast<size_t>(idx)];
                apply_clear(rect.x, rect.y, rect.w, rect.h);
                ++idx;
            }
            return;
        }
        case element::kind::panel:
        case element::kind::band:
        case element::kind::scroll_view: {
            if (compare_less(idx, layout.size())) {
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

const element* find_element_at(const vector<layout_rect>& layout, const element& tree, int mx, int my, int& idx) {
    switch (tree.kind()) {
        case element::kind::text:
        case element::kind::rich_text:
        case element::kind::button:
        case element::kind::checkbox:
        case element::kind::text_input:
        case element::kind::separator:
        case element::kind::spacer:
        case element::kind::virtual_list:
        case element::kind::canvas: {
            if (compare_less(idx, layout.size())) {
                const auto& rect = layout[static_cast<size_t>(idx)];
                ++idx;
                if (mx >= rect.x && mx < rect.x + rect.w && my >= rect.y && my < rect.y + rect.h) {
                    return &tree;
                }
            }
            return nullptr;
        }
        case element::kind::panel:
        case element::kind::band:
        case element::kind::scroll_view: {
            if (compare_less(idx, layout.size())) {
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

component_base* hit_test_at(const vector<layout_rect>& layout, const element& tree, const int mx, const int my,
                            int& idx, component_base* fallback) {
    switch (tree.kind()) {
        case element::kind::text:
        case element::kind::rich_text:
        case element::kind::button:
        case element::kind::checkbox:
        case element::kind::text_input:
        case element::kind::separator:
        case element::kind::spacer:
        case element::kind::virtual_list:
        case element::kind::canvas: {
            if (compare_less(idx, layout.size())) {
                const auto& rect = layout[static_cast<size_t>(idx)];
                ++idx;
                if (mx >= rect.x && mx < rect.x + rect.w && my >= rect.y && my < rect.y + rect.h) {
                    auto* owner = static_cast<component_base*>(tree.owner());
                    return owner != nullptr ? owner : fallback;
                }
            }
            return nullptr;
        }
        case element::kind::panel:
        case element::kind::band:
        case element::kind::scroll_view: {
            if (compare_less(idx, layout.size())) {
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
                auto* hit = hit_test_at(layout, child, mx, my, idx, fallback);
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
                auto* hit = hit_test_at(layout, child, mx, my, idx, fallback);
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

void renderer::apply_style_to_cell(cell& cell, const style& style) {
    if (style.fg.has_value()) {
        cell.foreground = style.fg.value();
    } else {
        cell.foreground = theme_.fg;
    }
    if (style.bg.has_value()) {
        cell.background = style.bg.value();
    } else if (!band_stack_.empty()) {
        cell.background = band_stack_.back();
    }
    // Unstyled text keeps the terminal's default background. Painting theme_.bg
    // only behind glyphs creates visible blocks around text (the bug this fixes).
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

void renderer::apply_text(const int x, const int y, const string& text, const style& style) {
    apply_text(x, y, text, style, -1);
}

void renderer::apply_text(const int x, const int y, const string& text, const style& style, const int max_x) {
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
            auto& cell = screen_.fast_cell_at(cx, y);
            cell.reset();
            put_codepoint(*it, max(1, it->display_width()), cell);
            apply_style_to_cell(cell, style);
        }
        const int cw = it->display_width();
        for (int w = 1; w < cw && cx + w < term_w_; ++w) {
            auto& next = screen_.fast_cell_at(cx + w, y);
            next.reset();
            next.automerge = true;
        }
        cx += max(1, cw);
    }
}

void renderer::apply_border(const int x, const int y, const int w, const int h, const enum style::border border,
                            const _NEFORCE color& c) {
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
        case style::border::ascii: {
            tl = "+";
            tr = "+";
            bl = "+";
            br = "+";
            hz = "-";
            vt = "|";
            break;
        }
        default: {
            break;
        }
    }

    auto set_border_cell = [this, &c](const int bx, const int by, const char* ch) {
        if (bx >= 0 && bx < term_w_ && by >= 0 && by < term_h_) {
            auto& cell = screen_.fast_cell_at(bx, by);
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

void renderer::apply_clear(int x, int y, int w, int h) {
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
    // One prepared blank cell is copied per column instead of reset() plus a string assignment:
    // clearing a viewport is a hot path for scroll containers.
    cell blank;
    blank.reset();
    blank.character = " ";
    const int x0 = max(x, 0);
    const int x1 = min(x + w, term_w_);
    const int y0 = max(y, 0);
    const int y1 = min(y + h, term_h_);
    for (int cy = y0; cy < y1; ++cy) {
        for (int cx = x0; cx < x1; ++cx) {
            screen_.fast_cell_at(cx, cy) = blank;
        }
    }
}

void renderer::render_button(const int x, const int y, const int w, const int h, const string& label,
                             const style& style, const theme& theme, const style::variant variant) {
    tui::style s = theme.button_style(variant);
    s = style::merge(s, style);

    const auto borderColor = s.borderColor.value_or(theme.border);
    apply_border(x, y, w, h, style::border::single, borderColor);

    const int inner_w = max(1, w - 2);
    if (h >= 3) {
        render_text_block(x + 1, y + 1, inner_w, h - 2, label, s, style::wrap_mode::none);
    } else {
        int label_w = 0;
        const auto& raw = label.view();
        for (utf8_iterator it(reinterpret_cast<const byte_t*>(raw.data()), raw.size()); it != utf8_iterator(); ++it) {
            label_w += max(1, it->display_width());
        }
        int label_x = x + (w - label_w) / 2;
        label_x = max(label_x, x + 1);
        render_text_block(label_x, y, min(inner_w, w - (label_x - x)), 1, label, s, style::wrap_mode::none);
    }
}

void renderer::render_checkbox(const int x, const int y, const int w, const int h, const string& label,
                               const bool checked, const style& style) {
    string display = checked ? "[x] " : "[ ] ";
    display += label;
    render_text_block(x, y, w, h, display, style, style::wrap_mode::word);
}

void renderer::render_separator(const int x, const int y, const int w) {
    if (w <= 0) {
        return;
    }
    for (int i = 0; i < w && x + i < term_w_ && y < term_h_; ++i) {
        auto& cell = screen_.fast_cell_at(x + i, y);
        cell.character = "-";
        cell.foreground = theme_.border;
    }
}

uint8_t renderer::hyperlink_of(const string* link) {
    if (link == nullptr || link->empty()) {
        return 0;
    }
    if (cached_link_ == link) {
        return cached_link_id_;
    }
    cached_link_ = link;
    cached_link_id_ = screen_.register_hyperlink(*link);
    return cached_link_id_;
}

int renderer::draw_glyph(const int x, const int y, const codepoint& cp, const int cw, const cell& stamp,
                         const string* link) {
    if (x >= 0 && x < term_w_ && y >= 0 && y < term_h_) {
        ++painted_this_frame_;
    }
    if (x >= 0 && x < term_w_ && y >= 0 && y < term_h_ && !is_clipped(x, y)) {
        auto& target = screen_.fast_cell_at(x, y);
        target = stamp;
        put_codepoint(cp, cw, target);
        const uint8_t id = hyperlink_of(link);
        if (id != 0) {
            target.hyperlink = id;
        }
    }
    for (int i = 1; i < cw && x + i < term_w_; ++i) {
        // The first cell is bounds-checked above;
        // the wide-glyph continuation columns can still fall outside the screen when content overflows.
        if (y < 0 || y >= term_h_) {
            break;
        }
        auto& next = screen_.fast_cell_at(x + i, y);
        next.reset();
        next.automerge = true;
    }
    return cw;
}

bool renderer::is_clipped(const int x, const int y) const noexcept {
    for (const auto& clip: clip_stack_) {
        if (x < clip.x || x >= clip.x + clip.w || y < clip.y || y >= clip.y + clip.h) {
            return true;
        }
    }
    return false;
}

bool renderer::is_rect_visible(const int x, const int y, const int w, const int h) const noexcept {
    if (w <= 0 || h <= 0 || x >= term_w_ || y >= term_h_ || x + w <= 0 || y + h <= 0) {
        return false;
    }
    for (const auto& clip: clip_stack_) {
        if (x >= clip.x + clip.w || y >= clip.y + clip.h || x + w <= clip.x || y + h <= clip.y) {
            return false;
        }
    }
    return true;
}

void renderer::render_glyph_run(const int x, const int y, const int w, const int h, const vector<run_glyph>& glyphs,
                                const style::wrap_mode wm, const int scroll_row, int* out_end_x, int* out_end_y,
                                const vector<text_span>* hang, const int hang_cols, const element* owner,
                                const tui::style* uniform_style, const vector<text_span>* spans,
                                const vector<tui::style>* span_styles) {
    int cx = x;
    int cy = y;

    const bool cached = owner != nullptr && !owner->cached_glyph_flat().empty();
    const size_t glyph_count = cached ? owner->cached_glyph_codepoints().size() : glyphs.size();
    if (w <= 0 || h <= 0 || glyph_count == 0) {
        if (out_end_x != nullptr) {
            *out_end_x = x;
        }
        if (out_end_y != nullptr) {
            *out_end_y = y;
        }
        return;
    }

    // The flat string and the per-glyph byte offsets only depend on the element content,
    // so they are cached on the node: re-rendering the same text skips the whole UTF-8 pass.
    string flat_storage;
    vector<uint32_t> local_offsets;
    vector<codepoint> local_cps;
    string_view flat;
    const vector<uint32_t>* glyph_offsets = nullptr;
    const vector<codepoint>* glyph_cps = nullptr;
    if (cached) {
        flat = owner->cached_glyph_flat().view();
        glyph_offsets = &owner->cached_glyph_offsets();
        glyph_cps = &owner->cached_glyph_codepoints();
    } else {
        flat_storage.reserve(glyphs.size() * 2);
        local_offsets.reserve(glyphs.size() + 1);
        local_cps.reserve(glyphs.size());
        vector<uint16_t> local_spans;
        const bool rich = spans != nullptr;
        if (rich) {
            local_spans.reserve(glyphs.size());
        }
        for (const auto& g: glyphs) {
            local_offsets.push_back(static_cast<uint32_t>(flat_storage.size()));
            local_cps.push_back(g.cp);
            if (rich) {
                local_spans.push_back(g.span);
            }
            g.cp.append_to(flat_storage);
        }
        local_offsets.push_back(static_cast<uint32_t>(flat_storage.size()));
        flat = flat_storage.view();
        if (owner != nullptr) {
            owner->cache_glyph_run(flat_storage, local_offsets, local_cps, _NEFORCE move(local_spans));
            glyph_offsets = &owner->cached_glyph_offsets();
            glyph_cps = &owner->cached_glyph_codepoints();
        } else {
            glyph_offsets = &local_offsets;
            glyph_cps = &local_cps;
        }
    }
    const vector<uint32_t>& offsets = *glyph_offsets;
    const vector<codepoint>& cps = *glyph_cps;

    // Only the rows that can be painted are wrapped:
    // a long document stops as soon as the viewport is covered instead of laying out to the end.
    const auto max_rows = static_cast<size_t>(max(0, scroll_row) + max(0, h));
    const text_layout lay = layout_text_hang(flat, w, wm, hang_cols, max_rows);

    size_t glyph_index = 0;
    for (size_t row = 0; row < lay.lines.size(); ++row) {
        if (compare_less(row, scroll_row)) {
            continue;
        }
        if (cy >= y + h) {
            break;
        }
        const auto& line = lay.lines[row];
        while (glyph_index + 1 < offsets.size() && offsets[glyph_index] < line.begin) {
            ++glyph_index;
        }
        cx = x;
        if (hang != nullptr && hang_cols > 0 && line.continuation) {
            for (const auto& span: *hang) {
                // One resolved prototype per span instead of ten optional tests per glyph.
                cell stamp;
                stamp.reset();
                apply_style_to_cell(stamp, span.style);
                const auto& raw_hang = span.content.view();
                for (utf8_iterator it(reinterpret_cast<const byte_t*>(raw_hang.data()), raw_hang.size());
                     it != utf8_iterator(); ++it) {
                    const int hang_width = max(1, it->display_width());
                    if (cx + hang_width > x + w) {
                        break;
                    }
                    cx += draw_glyph(cx, cy, *it, hang_width, stamp, span.link.empty() ? nullptr : &span.link);
                }
            }
        }
        cell stamp;
        if (uniform_style != nullptr) {
            stamp.reset();
            apply_style_to_cell(stamp, *uniform_style);
        }
        // Cached rich-text runs resolve the style and link through the originating span;
        // uncached runs carry them per glyph.
        const vector<uint16_t>* span_of =
                (cached && spans != nullptr && span_styles != nullptr) ? &owner->cached_glyph_spans() : nullptr;
        const tui::style* stamp_source = uniform_style;
        while (glyph_index + 1 < offsets.size() && offsets[glyph_index] < line.end) {
            const codepoint& cp = cps[glyph_index];
            const int cw = max(1, cp.display_width());
            if (cx + cw > x + w) {
                break;
            }
            const tui::style* glyph_style = uniform_style;
            const string* link = nullptr;
            if (span_of != nullptr && glyph_index < span_of->size()) {
                const size_t span_index = (*span_of)[glyph_index];
                glyph_style = &(*span_styles)[span_index];
                const string& target = (*spans)[span_index].link;
                link = target.empty() ? nullptr : &target;
            } else if (uniform_style == nullptr) {
                glyph_style = glyphs[glyph_index].style;
                link = glyphs[glyph_index].link;
            }
            if (glyph_style != stamp_source) {
                stamp.reset();
                apply_style_to_cell(stamp, *glyph_style);
                stamp_source = glyph_style;
            }
            cx += draw_glyph(cx, cy, cp, cw, stamp, link);
            ++glyph_index;
        }
        cy += 1;
    }

    if (cy > y) {
        cy -= 1;
    }
    if (out_end_x != nullptr) {
        *out_end_x = cx;
    }
    if (out_end_y != nullptr) {
        *out_end_y = cy;
    }
}

void renderer::render_text_block(const int x, const int y, const int w, const int h, const string& text,
                                 const tui::style& style, const style::wrap_mode wm, int* out_end_x, int* out_end_y,
                                 const int scroll_row, const element* owner) {
    const auto& raw = text.view();
    // Plain ASCII on a single unwrapped line is drawn byte by byte:
    // no glyph vector, no re-encoded flat string and no wrapping pass.
    // This is the common case for labels, table cells and transcript rows.
    bool plain = wm == style::wrap_mode::none && scroll_row == 0 && w > 0 && h > 0;
    if (plain) {
        for (size_t i = 0; i < raw.size(); ++i) {
            const auto ch = static_cast<unsigned char>(raw[i]);
            if (ch >= 0x80 || ch == '\n') {
                plain = false;
                break;
            }
        }
    }
    if (plain) {
        cell stamp;
        stamp.reset();
        apply_style_to_cell(stamp, style);
        int cx = x;
        const int limit = x + w;
        for (size_t i = 0; i < raw.size() && cx < limit; ++i) {
            const auto ch = static_cast<unsigned char>(raw[i]);
            cx += draw_glyph(cx, y, codepoint(static_cast<char32_t>(ch)), 1, stamp, nullptr);
        }
        if (out_end_x != nullptr) {
            *out_end_x = cx;
        }
        if (out_end_y != nullptr) {
            *out_end_y = y;
        }
        return;
    }

    // The glyph run of a text element is immutable, so it is decoded once and cached on the node;
    // later frames only wrap and draw.
    vector<run_glyph> glyphs;
    if (owner == nullptr || owner->cached_glyph_flat().empty()) {
        glyphs.reserve(raw.size());
        for (utf8_iterator it(reinterpret_cast<const byte_t*>(raw.data()), raw.size()); it != utf8_iterator(); ++it) {
            glyphs.push_back(run_glyph{*it, &style, nullptr});
        }
    }
    render_glyph_run(x, y, w, h, glyphs, wm, scroll_row, out_end_x, out_end_y, nullptr, 0, owner, &style);
}

void renderer::apply_selection_overlay(const int x, const int y, const int /*w*/, const int h, const string& text,
                                       const text_layout& lay, const size_t sel_begin, const size_t sel_end,
                                       const int scroll_row) {
    const auto& raw = text.view();
    const auto* data = reinterpret_cast<const byte_t*>(raw.data());

    for (size_t row = 0; row < lay.lines.size(); ++row) {
        const int visible = static_cast<int>(row) - scroll_row;
        if (visible < 0) {
            continue;
        }
        if (visible >= h) {
            break;
        }
        const auto& line = lay.lines[row];
        if (line.end <= sel_begin || line.begin >= sel_end) {
            continue;
        }

        int cx = x;
        size_t pos = line.begin;
        while (pos < line.end) {
            const utf8_iterator it(data + pos, line.end - pos);
            if (it == utf8_iterator()) {
                break;
            }
            const int cw = max(1, it->display_width());
            const bool selected = (pos >= sel_begin && pos < sel_end);
            if (selected && y + visible >= 0 && y + visible < term_h_ && cx >= 0 && cx < term_w_ &&
                !is_clipped(cx, y + visible)) {
                auto& target = screen_.fast_cell_at(cx, y + visible);
                target.inverted = true;
            }
            cx += cw;
            pos += max<size_t>(1, it->utf8_length());
        }
    }
}

void renderer::render_rich_text(const int x, const int y, const int w, const int h, const vector<text_span>& spans,
                                const tui::style& style, const style::wrap_mode wm, int* out_end_x, int* out_end_y,
                                const vector<text_span>* hang, const int hang_cols, const element* owner) {
    // Merging a span style costs a 192-byte copy plus 21 optional merges;
    // the result only depends on the element, so it is cached on the node after the first frame.
    vector<tui::style> merged_local;
    const vector<tui::style>* merged = nullptr;
    const bool own_style = owner != nullptr && &style == &owner->style();
    if (own_style && owner->cached_span_styles().size() == spans.size()) {
        merged = &owner->cached_span_styles();
    } else {
        merged_local.reserve(spans.size());
        for (const auto& span: spans) {
            merged_local.push_back(tui::style::merge(style, span.style));
        }
        if (own_style) {
            owner->cache_span_styles(merged_local);
            merged = &owner->cached_span_styles();
        } else {
            merged = &merged_local;
        }
    }

    // The glyph run of a rich text element only depends on its spans, so it is decoded once
    // and cached on the node; later frames wrap and draw without touching UTF-8 again.
    vector<run_glyph> glyphs;
    if (owner == nullptr || owner->cached_glyph_flat().empty()) {
        for (size_t i = 0; i < spans.size(); ++i) {
            const auto& raw = spans[i].content.view();
            for (utf8_iterator it(reinterpret_cast<const byte_t*>(raw.data()), raw.size()); it != utf8_iterator();
                 ++it) {
                glyphs.push_back(run_glyph{*it, &(*merged)[i], &spans[i].link, static_cast<uint16_t>(i)});
            }
        }
    }
    render_glyph_run(x, y, w, h, glyphs, wm, 0, out_end_x, out_end_y, hang, hang_cols, owner, nullptr, &spans, merged);
}

void renderer::render_virtual_list(const element& el, const layout_rect& rect) {
    if (el.viewport_rect_state() != nullptr) {
        auto* box = static_cast<state<layout_rect>*>(el.viewport_rect_state());
        const auto& current = box->value();
        if (current.x != rect.x || current.y != rect.y || current.w != rect.w || current.h != rect.h) {
            *box = rect;
        }
    }

    if (rect.w <= 0 || rect.h <= 0) {
        if (el.viewport_state() != nullptr) {
            *static_cast<state<int>*>(el.viewport_state()) = 0;
        }
        return;
    }

    const bool variable = (el.item_heights() != nullptr);
    auto* heights = const_cast<vector<int>*>(el.item_heights());

    const auto win = compute_virtual_window(el, rect.h);

    if (el.scroll_y_state() != nullptr && el.scroll_y() != min(max(0, el.scroll_y()), win.max_scroll)) {
        *static_cast<state<int>*>(el.scroll_y_state()) = min(max(0, el.scroll_y()), win.max_scroll);
    }
    if (el.viewport_state() != nullptr) {
        auto* vp = static_cast<state<int>*>(el.viewport_state());
        if (vp->value() != rect.h) {
            *vp = rect.h;
        }
    }

    if (!el.item_builder()) {
        return;
    }

    clip_stack_.push_back(clip_rect{rect.x, rect.y, rect.w, rect.h});
    apply_clear(rect.x, rect.y, rect.w, rect.h);

    constexpr int measure_limit = 100000;
    const int count = static_cast<int>(el.item_count());
    int offset = -win.skip;

    for (int step = 0; step < win.visible; ++step) {
        const int index = win.first + step;
        if (index < 0 || index >= count || offset >= rect.h) {
            break;
        }

        const int hint = item_rows_at(el, static_cast<size_t>(index));
        const element item = el.item_builder()(static_cast<size_t>(index));

        if (item.kind() == element::kind::empty) {
            offset += hint;
            continue;
        }

        // A single-line text item needs no layout pass: it is drawn straight into the row,
        // which is the common case for log lines and message lists.
        const bool single_line = item.kind() == element::kind::text && item.wrap_mode() == style::wrap_mode::none &&
                                 hint == 1 && item.text().view().find('\n') == string_view::npos;
        if (single_line) {
            render_text_block(rect.x, rect.y + offset, rect.w, 1, item.text(), item.style(), style::wrap_mode::none,
                              nullptr, nullptr, 0, &item);
            offset += hint;
            continue;
        }

        const int limit = variable ? measure_limit : hint;
        auto item_layout = compute_layout(item, rect.w, limit);

        int natural = hint;
        if (variable) {
            natural = 0;
            for (const auto& box: item_layout) {
                natural = max(natural, box.y + box.h);
            }
            natural = max(1, natural);
            if (heights != nullptr && static_cast<size_t>(index) < heights->size() && (*heights)[index] != natural) {
                (*heights)[index] = natural;
            }
            if (natural != hint) {
                item_layout = compute_layout(item, rect.w, natural);
            }
        }

        const int offset_y = rect.y + offset;
        for (auto& box: item_layout) {
            box.x += rect.x;
            box.y += offset_y;
        }
        int child_idx = 0;
        render_subtree(item, item_layout, child_idx);

        offset += natural;
    }

    clip_stack_.pop_back();

    if (win.content_h > rect.h && el.scroll_y_state() != nullptr) {
        const int thumb_size = max(1, rect.h * rect.h / max(1, win.content_h));
        const int ms = max(1, win.content_h - rect.h);
        const int thumb_pos = (rect.h - thumb_size) * min(max(0, el.scroll_y()), win.max_scroll) / ms;

        scrollbar_hits_.push_back({el.scroll_y_state(), rect.x + rect.w - 1, rect.y, rect.h, thumb_pos, thumb_size,
                                   win.content_h, rect.x, rect.w, true});
        for (int i = 0; i < rect.h; ++i) {
            const int by = rect.y + i;
            const int bx = rect.x + rect.w - 1;
            if (bx >= 0 && bx < term_w_ && by >= 0 && by < term_h_) {
                auto& cell = screen_.fast_cell_at(bx, by);
                cell.character = (i >= thumb_pos && i < thumb_pos + thumb_size) ? "\xe2\x96\x88" : "\xe2\x94\x82";
                cell.foreground = theme_.border;
            }
        }
    }
}

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
