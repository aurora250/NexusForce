#include <NeForce/core/string/utf_iterator.hpp>
#include <NeForce/tui/dom/state.hpp>
#include <NeForce/tui/renderer.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

renderer::renderer(screen& target, const theme& t, const int& term_w, const int& term_h) :
screen_(target),
theme_(t),
term_w_(term_w),
term_h_(term_h) {}

void renderer::render(const element& tree, const vector<layout_rect>& layout, bool has_focus) {
    has_focus_ = has_focus;
    scrollbar_hits_.clear();
    int idx = 0;
    render_subtree(tree, layout, idx);
}

void renderer::render_subtree(const element& el, const vector<layout_rect>& layout, int& idx) {
    switch (el.kind()) {
        case element::kind::empty: {
            return;
        }
        case element::kind::text: {
            if (idx < static_cast<int>(layout.size())) {
                const auto& rect = layout[static_cast<size_t>(idx)];
                render_text_block(rect.x, rect.y, rect.w, rect.h, el.text(), el.style(), el.wrap_mode());
                ++idx;
            }
            return;
        }
        case element::kind::button: {
            if (idx < static_cast<int>(layout.size())) {
                const auto& rect = layout[static_cast<size_t>(idx)];
                render_button(rect.x, rect.y, rect.w, rect.h, el.text(), el.style(), theme_, el.variant());
                ++idx;
            }
            return;
        }
        case element::kind::checkbox: {
            if (idx < static_cast<int>(layout.size())) {
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
            if (idx < static_cast<int>(layout.size())) {
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

                const auto border_color = has_focus_ ? theme_.primary : theme_.border;
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
            if (idx < static_cast<int>(layout.size())) {
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
            if (idx < static_cast<int>(layout.size())) {
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
                    for (int i = child_start_idx; i < child_end_idx && i < static_cast<int>(layout.size()); ++i) {
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

                    if (content_w > clip.w) {
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
            if (idx < static_cast<int>(layout.size())) {
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

void renderer::clear_element_area(const element& el, const vector<layout_rect>& layout, int& idx) {
    switch (el.kind()) {
        case element::kind::text:
        case element::kind::button:
        case element::kind::checkbox:
        case element::kind::text_input:
        case element::kind::separator:
        case element::kind::spacer: {
            if (idx < static_cast<int>(layout.size())) {
                const auto& rect = layout[static_cast<size_t>(idx)];
                apply_clear(rect.x, rect.y, rect.w, rect.h);
                ++idx;
            }
            return;
        }
        case element::kind::scroll_view: {
            if (idx < static_cast<int>(layout.size())) {
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
        case element::kind::button:
        case element::kind::checkbox:
        case element::kind::text_input:
        case element::kind::separator:
        case element::kind::spacer: {
            if (idx < static_cast<int>(layout.size())) {
                const auto& rect = layout[static_cast<size_t>(idx)];
                ++idx;
                if (mx >= rect.x && mx < rect.x + rect.w && my >= rect.y && my < rect.y + rect.h) {
                    return &tree;
                }
            }
            return nullptr;
        }
        case element::kind::scroll_view: {
            if (idx < static_cast<int>(layout.size())) {
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

component_base* hit_test_at(const vector<layout_rect>& layout, const element& tree, int mx, int my, int& idx,
                            component_base* fallback) {
    switch (tree.kind()) {
        case element::kind::text:
        case element::kind::button:
        case element::kind::checkbox:
        case element::kind::text_input:
        case element::kind::separator:
        case element::kind::spacer: {
            if (idx < static_cast<int>(layout.size())) {
                const auto& rect = layout[static_cast<size_t>(idx)];
                ++idx;
                if (mx >= rect.x && mx < rect.x + rect.w && my >= rect.y && my < rect.y + rect.h) {
                    auto* owner = static_cast<component_base*>(tree.owner());
                    return owner != nullptr ? owner : fallback;
                }
            }
            return nullptr;
        }
        case element::kind::scroll_view: {
            if (idx < static_cast<int>(layout.size())) {
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
    } else {
        cell.background = theme_.bg;
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

void renderer::apply_text(int x, int y, const string& text, const style& style) { apply_text(x, y, text, style, -1); }

void renderer::apply_text(int x, int y, const string& text, const style& style, int max_x) {
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
            it->append_to(cell.character);
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

void renderer::apply_border(int x, int y, int w, int h, enum style::border border, const _NEFORCE color& c) {
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
    for (int i = 0; i < h; ++i) {
        for (int j = 0; j < w; ++j) {
            const int cx = x + j;
            const int cy = y + i;
            if (cx >= 0 && cx < term_w_ && cy >= 0 && cy < term_h_) {
                auto& cell = screen_.fast_cell_at(cx, cy);
                cell.reset();
                cell.character = " ";
            }
        }
    }
}

void renderer::render_button(int x, int y, int w, int h, const string& label, const style& style, const theme& theme,
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

void renderer::render_checkbox(int x, int y, int w, int h, const string& label, bool checked, const style& style) {
    string display = checked ? "[x] " : "[ ] ";
    display += label;
    render_text_block(x, y, w, h, display, style, style::wrap_mode::word);
}

void renderer::render_separator(int x, int y, int w) {
    if (w <= 0) {
        return;
    }
    for (int i = 0; i < w && x + i < term_w_ && y < term_h_; ++i) {
        auto& cell = screen_.fast_cell_at(x + i, y);
        cell.character = "-";
        cell.foreground = theme_.border;
    }
}

void renderer::render_text_block(int x, int y, int w, int h, const string& text, const tui::style& style,
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
                auto& cell = screen_.fast_cell_at(px, py);
                cell.reset();
                it->append_to(cell.character);
                apply_style_to_cell(cell, style);
            }
        }
        const int cw = it->display_width();
        for (int ww = 1; ww < cw && px + ww < term_w_; ++ww) {
            auto& next = screen_.fast_cell_at(px + ww, py);
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
