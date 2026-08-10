#include <NeForce/core/string/utf_iterator.hpp>
#include <NeForce/tui/dom/layout.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

namespace {
    int string_display_width(const string& text) {
        int width = 0;
        const auto& raw = text.view();
        for (utf8_iterator it(reinterpret_cast<const byte_t*>(raw.data()), raw.size()); it != utf8_iterator(); ++it) {
            width += it->display_width();
        }
        return width;
    }

    int measure_text_width(const string& text) { return string_display_width(text); }

    int compute_wrapped_height(const string& text, int max_width, style::wrap_mode wm) {
        if (max_width <= 0 || wm == style::wrap_mode::none) {
            return 1;
        }
        if (text.empty()) {
            return 1;
        }

        if (wm == style::wrap_mode::character) {
            const int total_w = string_display_width(text);
            return max(1, (total_w + max_width - 1) / max_width);
        }

        int lines = 1;
        int line_w = 0;
        const auto& raw = text.view();
        const auto* data = reinterpret_cast<const byte_t*>(raw.data());
        const size_t len = raw.size();

        size_t pos = 0;
        while (pos < len) {
            if (line_w == 0) {
                while (pos < len && data[pos] == ' ') {
                    ++pos;
                }
                if (pos >= len) {
                    break;
                }
            }

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

            const string word_sv = raw.view(pos, word_end - pos);
            const int word_w = string_display_width(word_sv);

            if (line_w == 0) {
                if (word_w > max_width) {
                    lines += (word_w - 1) / max_width;
                    line_w = word_w % max_width;
                    if (line_w == 0) {
                        line_w = 0;
                    }
                } else {
                    line_w = word_w;
                }
            } else {
                if (line_w + 1 + word_w > max_width) {
                    ++lines;
                    line_w = word_w;
                    if (word_w > max_width) {
                        lines += (word_w - 1) / max_width;
                        line_w = word_w % max_width;
                        if (line_w == 0) {
                            line_w = 0;
                        }
                    }
                } else {
                    line_w += 1 + word_w;
                }
            }

            pos = word_end;
            while (pos < len && data[pos] == ' ') {
                ++pos;
            }
        }

        return lines;
    }

    int measure_element_width(const element& el);
    int measure_element_height(const element& el, int allocated_width = -1);
    void assign_layout(const element& el, vector<layout_rect>& out, int x, int y, int total_w, int total_h);

    int child_flex_weight(const element& child, bool is_row) {
        const auto& cs = child.style();
        if (child.kind() == element::kind::spacer) {
            return child.flex();
        }
        const float fg = cs.flex_grow.value_or(0.0F);
        if (fg > 0.0F) {
            return static_cast<int>(fg * 100.0F);
        }
        const style::size_hint hint = cs.width.value_or(style::size_hint{});
        if (is_row && hint.mode == style::size_hint::fill) {
            return hint.value > 0 ? hint.value : 1;
        }
        const style::size_hint h_hint = cs.height.value_or(style::size_hint{});
        if (!is_row && h_hint.mode == style::size_hint::fill) {
            return h_hint.value > 0 ? h_hint.value : 1;
        }
        return 0;
    }

    void assign_layout_flex_line(const vector<size_t>& indices, const vector<element>& children, const box_props& lp,
                                 const vector<int>& child_sizes, int content_x, int content_y, int content_w,
                                 int content_h, bool is_row, int line_cross_pos, int line_cross_size,
                                 vector<layout_rect>& out) {
        const int n = static_cast<int>(indices.size());
        int total_content_size = 0;
        for (size_t i = 0; i < indices.size(); ++i) {
            total_content_size += child_sizes[i];
        }
        const int total_gap = max(0, n - 1) * lp.gap;
        total_content_size += total_gap;

        const int available_size = is_row ? content_w : content_h;
        const int free_space = max(0, available_size - total_content_size);

        int justify_offset = 0;
        int adjusted_gap = lp.gap;
        switch (lp.justify) {
            case style::justify::center:
                justify_offset = free_space / 2;
                break;
            case style::justify::end:
                justify_offset = free_space;
                break;
            case style::justify::space_between:
                if (n > 1) {
                    adjusted_gap = lp.gap + free_space / (n - 1);
                }
                break;
            case style::justify::space_around:
                if (n > 0) {
                    const int space = free_space / n;
                    justify_offset = space / 2;
                    adjusted_gap = lp.gap + space;
                }
                break;
            default:
                break;
        }

        int offset = is_row ? content_x + justify_offset : content_y + justify_offset;
        for (size_t i = 0; i < indices.size(); ++i) {
            const auto& child = children[indices[i]];
            const auto& cs = child.style();
            const int child_alloc_w = is_row ? child_sizes[i] : content_w;
            const int child_alloc_h = is_row ? line_cross_size : child_sizes[i];

            const enum style::align child_align = cs.align.value_or(lp.align);
            const bool is_stretch = (child_align == style::align::stretch);
            int cross_x = content_x;
            int cross_y = line_cross_pos;
            if (!is_stretch) {
                if (is_row) {
                    switch (child_align) {
                        case style::align::center:
                            cross_y = line_cross_pos + (line_cross_size - child_alloc_h) / 2;
                            break;
                        case style::align::end:
                            cross_y = line_cross_pos + line_cross_size - child_alloc_h;
                            break;
                        default:
                            break;
                    }
                } else {
                    switch (child_align) {
                        case style::align::center:
                            cross_x = line_cross_pos + (line_cross_size - child_alloc_w) / 2;
                            break;
                        case style::align::end:
                            cross_x = line_cross_pos + line_cross_size - child_alloc_w;
                            break;
                        default:
                            break;
                    }
                }
            }

            if (is_row) {
                assign_layout(child, out, offset, is_stretch ? content_y : cross_y, child_alloc_w,
                              is_stretch ? content_h : child_alloc_h);
            } else {
                assign_layout(child, out, is_stretch ? content_x : cross_x, offset,
                              is_stretch ? content_w : child_alloc_w, child_alloc_h);
            }
            offset += child_sizes[i] + adjusted_gap;
        }
    }

    vector<int> compute_flex_sizes(const vector<size_t>& indices, const vector<element>& children, const box_props& lp,
                                   int main_axis_space, bool is_row) {
        const int n = static_cast<int>(indices.size());
        vector<int> sizes(n);

        int total_flex = 0;
        int fixed_size = 0;
        for (size_t i = 0; i < indices.size(); ++i) {
            const auto& child = children[indices[i]];
            const int fw = child_flex_weight(child, is_row);
            if (fw > 0) {
                total_flex += fw;
            } else {
                fixed_size += is_row ? measure_element_width(child) : measure_element_height(child);
            }
        }

        const int total_gap = max(0, n - 1) * lp.gap;
        const int flex_available = max(0, main_axis_space - fixed_size - total_gap);

        int remaining_flex = total_flex;
        int remaining_space = flex_available;
        for (size_t i = 0; i < indices.size(); ++i) {
            const auto& child = children[indices[i]];
            const int fw = child_flex_weight(child, is_row);
            if (fw > 0 && total_flex > 0) {
                sizes[i] = (i == static_cast<size_t>(n - 1)) ? remaining_space : flex_available * fw / total_flex;
                remaining_flex -= fw;
                remaining_space -= sizes[i];
            } else {
                sizes[i] = is_row ? measure_element_width(child) : measure_element_height(child);
            }
        }
        return sizes;
    }

    int measure_element_width(const element& el) {
        const auto k = el.kind();
        const auto& style = el.style();

        int content_w = 0;
        switch (k) {
            case element::kind::text:
            case element::kind::checkbox: {
                content_w = measure_text_width(el.text());
                break;
            }
            case element::kind::button: {
                content_w = measure_text_width(el.text()) + 2;
                break;
            }
            case element::kind::spacer: {
                content_w = 0;
                break;
            }
            case element::kind::separator: {
                content_w = 0;
                break;
            }
            case element::kind::text_input: {
                content_w = el.text().empty() ? 10 : measure_text_width(el.text()) + 2;
                break;
            }
            case element::kind::vbox:
            case element::kind::hbox:
            case element::kind::zstack:
            case element::kind::flexbox: {
                int max_child_w = 0;
                int total_child_w = 0;
                for (const auto& child: el.children()) {
                    int cw = measure_element_width(child);
                    max_child_w = max(max_child_w, cw);
                    total_child_w += cw;
                }
                const auto& lp = el.layout();
                const int n = static_cast<int>(el.children().size());
                if (lp.dir == style::direction::row && !lp.wrap) {
                    content_w = total_child_w + max(0, n - 1) * lp.gap;
                } else {
                    content_w = max_child_w;
                }
                break;
            }
            case element::kind::gridbox: {
                const int cols = el.grid_columns();
                const auto& children = el.children();
                if (cols > 0 && !children.empty()) {
                    const int rows = static_cast<int>(children.size()) / cols;
                    for (int c = 0; c < cols; ++c) {
                        int colW = 0;
                        for (int r = 0; r < rows; ++r) {
                            const size_t idx = static_cast<size_t>(r) * cols + c;
                            if (idx < children.size()) {
                                colW = max(colW, measure_element_width(children[idx]));
                            }
                        }
                        content_w += colW;
                    }
                }
                break;
            }
            case element::kind::scroll_view: {
                content_w = el.children().empty() ? 0 : measure_element_width(el.children()[0]);
                break;
            }
            case element::kind::each:
            case element::kind::when: {
                int max_child_w = 0;
                for (const auto& child: el.children()) {
                    max_child_w = max(max_child_w, measure_element_width(child));
                }
                content_w = max_child_w;
                break;
            }
            case element::kind::empty:
            default: {
                break;
            }
        }

        const style::size_hint width_hint = style.width.value_or(style::size_hint{});
        using padding = struct style::padding;
        using margin = struct style::margin;
        const padding pad = style.padding.value_or(padding{});
        const margin mar_w = style.margin.value_or(margin{});
        if (width_hint.mode == style::size_hint::fixed) {
            return width_hint.value + pad.left + pad.right + mar_w.left + mar_w.right;
        }

        content_w += pad.left + pad.right + mar_w.left + mar_w.right;
        return content_w;
    }

    int measure_element_height(const element& el, int allocated_width) {
        const auto k = el.kind();
        const auto& style = el.style();
        int content_h = 0;

        switch (k) {
            case element::kind::text: {
                const style::wrap_mode wm = el.wrap_mode();
                if (wm != style::wrap_mode::none && allocated_width > 0) {
                    content_h = compute_wrapped_height(el.text(), allocated_width, wm);
                } else {
                    content_h = 1;
                }
                break;
            }
            case element::kind::button:
            case element::kind::checkbox:
            case element::kind::separator: {
                content_h = 1;
                break;
            }
            case element::kind::text_input: {
                const style::wrap_mode wm = el.style().text_wrap.value_or(style::wrap_mode::none);
                if (wm != style::wrap_mode::none) {
                    int effective_w = allocated_width > 0 ? allocated_width : measure_element_width(el);
                    if (effective_w > 0) {
                        const int wrapped = compute_wrapped_height(el.text(), effective_w, wm);
                        content_h = (wrapped > 1) ? wrapped + 2 : 1;
                    } else {
                        content_h = 1;
                    }
                } else {
                    content_h = 1;
                }
                break;
            }
            case element::kind::spacer: {
                content_h = 0;
                break;
            }
            case element::kind::vbox:
            case element::kind::hbox:
            case element::kind::zstack:
            case element::kind::flexbox: {
                const auto& children = el.children();
                const auto& lp = el.layout();
                const int n = static_cast<int>(children.size());
                const bool is_row = (lp.dir == style::direction::row || k == element::kind::hbox);

                int max_child_h = 0;
                int total_child_h = 0;

                if (is_row && allocated_width > 0 && n > 0) {
                    int total_flex = 0;
                    int fixed_w = 0;
                    for (const auto& child: children) {
                        const int fw = child_flex_weight(child, true);
                        if (fw > 0) {
                            total_flex += fw;
                        } else {
                            fixed_w += measure_element_width(child);
                        }
                    }
                    const int total_gap = max(0, n - 1) * lp.gap;
                    const int flex_avail = max(0, allocated_width - fixed_w - total_gap);

                    int rem_flex = total_flex;
                    int rem_space = flex_avail;
                    for (size_t i = 0; i < static_cast<size_t>(n); ++i) {
                        const auto& child = children[i];
                        const int fw = child_flex_weight(child, true);
                        int child_w = 0;
                        if (fw > 0 && total_flex > 0) {
                            child_w = (i == static_cast<size_t>(n - 1)) ? rem_space : flex_avail * fw / total_flex;
                            rem_flex -= fw;
                            rem_space -= child_w;
                        } else {
                            child_w = measure_element_width(child);
                        }
                        int ch = measure_element_height(child, child_w);
                        max_child_h = max(max_child_h, ch);
                        total_child_h += ch;
                    }
                } else {
                    for (const auto& child: children) {
                        const int child_w_ctx = is_row ? measure_element_width(child) : allocated_width;
                        int ch = measure_element_height(child, child_w_ctx);
                        max_child_h = max(max_child_h, ch);
                        total_child_h += ch;
                    }
                }

                if (lp.dir == style::direction::column && !lp.wrap) {
                    content_h = total_child_h + max(0, n - 1) * lp.gap;
                } else {
                    content_h = max_child_h;
                }
                break;
            }
            case element::kind::gridbox: {
                const int cols = el.grid_columns();
                const auto& children = el.children();
                if (cols > 0 && !children.empty()) {
                    const int rows = static_cast<int>(children.size()) / cols;
                    for (int r = 0; r < rows; ++r) {
                        int rowH = 0;
                        for (int c = 0; c < cols; ++c) {
                            const size_t idx = static_cast<size_t>(r) * cols + c;
                            if (idx < children.size()) {
                                rowH = max(rowH, measure_element_height(children[idx]));
                            }
                        }
                        content_h += rowH;
                    }
                }
                break;
            }
            case element::kind::scroll_view: {
                content_h = el.children().empty() ? 1 : measure_element_height(el.children()[0]);
                break;
            }
            case element::kind::each:
            case element::kind::when: {
                int totalH = 0;
                for (const auto& child: el.children()) {
                    totalH += measure_element_height(child);
                }
                content_h = totalH;
                break;
            }
            case element::kind::empty:
            default: {
                break;
            }
        }

        const style::size_hint height_hint = style.height.value_or(style::size_hint{});
        using padding = struct style::padding;
        using margin = struct style::margin;
        const padding pad_h = style.padding.value_or(padding{});
        const margin mar_h = style.margin.value_or(margin{});
        if (height_hint.mode == style::size_hint::fixed) {
            return height_hint.value + pad_h.top + pad_h.bottom + mar_h.top + mar_h.bottom;
        }

        content_h += pad_h.top + pad_h.bottom + mar_h.top + mar_h.bottom;
        return content_h;
    }

    void assign_layout(const element& el, vector<layout_rect>& out, int x, int y, int total_w, int total_h) {
        const auto k = el.kind();
        const auto& style = el.style();
        const auto& lp = el.layout();

        using margin = struct style::margin;
        const margin marg = style.margin.value_or(margin{});
        layout_rect rect;
        rect.x = x + marg.left;
        rect.y = y + marg.top;
        rect.w = total_w - marg.left - marg.right;
        rect.h = total_h - marg.top - marg.bottom;

        switch (k) {
            case element::kind::separator:
            case element::kind::spacer: {
                out.push_back(rect);
                return;
            }
            case element::kind::scroll_view: {
                out.push_back(rect);
                if (!el.children().empty()) {
                    const auto& child = el.children()[0];
                    using padding = struct style::padding;
                    const padding pad = el.style().padding.value_or(padding{});
                    int sx = el.scroll_x();
                    int sy = el.scroll_y();
                    int content_x = rect.x + 1 + pad.left - sx;
                    int content_y = rect.y + 1 + pad.top - sy;
                    int content_w = max(measure_element_width(child), rect.w - 2 - pad.left - pad.right);
                    int content_h = max(measure_element_height(child), rect.h - 2 - pad.top - pad.bottom);
                    assign_layout(child, out, content_x, content_y, content_w, content_h);
                }
                return;
            }
            case element::kind::canvas: {
                const int content_w = measure_element_width(el);
                const int content_h = measure_element_height(el);
                if (content_w > 0 && content_w < rect.w) {
                    rect.w = content_w;
                }
                if (content_h > 0 && content_h < rect.h) {
                    rect.h = content_h;
                }
                out.push_back(rect);
                return;
            }
            case element::kind::text:
            case element::kind::button:
            case element::kind::checkbox: {
                const int content_w = measure_element_width(el);
                const int content_h = measure_element_height(el);
                if (content_w > 0 && content_w < rect.w) {
                    rect.w = content_w;
                }
                if (content_h > 0 && content_h < rect.h) {
                    rect.h = content_h;
                }

                const style::wrap_mode wm = k == element::kind::text
                                                    ? el.wrap_mode()
                                                    : el.style().text_wrap.value_or(style::wrap_mode::none);
                if (wm != style::wrap_mode::none && rect.w > 0 && k == element::kind::text) {
                    const int wrapped_h = compute_wrapped_height(el.text(), rect.w, wm);
                    rect.h = max(rect.h, wrapped_h);
                }

                out.push_back(rect);
                return;
            }
            case element::kind::text_input: {
                const int content_w = measure_element_width(el);
                const int content_h = measure_element_height(el, rect.w);
                if (content_w > 0 && content_w < rect.w) {
                    rect.w = content_w;
                }
                if (content_h > 0 && content_h < rect.h) {
                    rect.h = content_h;
                }

                out.push_back(rect);
                return;
            }
            case element::kind::zstack: {
                const int content_x = rect.x + lp.padding.left;
                const int content_y = rect.y + lp.padding.top;
                const int content_w = rect.w - lp.padding.left - lp.padding.right;
                const int content_h = rect.h - lp.padding.top - lp.padding.bottom;

                for (const auto& child: el.children()) {
                    assign_layout(child, out, content_x, content_y, content_w, content_h);
                }
                return;
            }
            case element::kind::vbox:
            case element::kind::hbox:
            case element::kind::each:
            case element::kind::when:
            case element::kind::flexbox: {
                const int content_x = rect.x + lp.padding.left;
                const int content_y = rect.y + lp.padding.top;
                const int content_w = rect.w - lp.padding.left - lp.padding.right;
                const int content_h = rect.h - lp.padding.top - lp.padding.bottom;

                const auto& children = el.children();
                const size_t n = children.size();
                if (n == 0) {
                    return;
                }

                const bool is_row = (lp.dir == style::direction::row || k == element::kind::hbox);
                const bool do_wrap = (k == element::kind::flexbox) && lp.wrap;

                if (do_wrap) {
                    struct line {
                        vector<size_t> indices;
                        int line_size;
                    };
                    vector<line> lines;
                    int main_available = is_row ? content_w : content_h;

                    line cur_line;
                    int cur_main = 0;
                    for (size_t i = 0; i < n; ++i) {
                        const int child_ms =
                                is_row ? measure_element_width(children[i]) : measure_element_height(children[i]);
                        const int gap_before = cur_line.indices.empty() ? 0 : lp.gap;

                        if (!cur_line.indices.empty() && cur_main + gap_before + child_ms > main_available) {
                            lines.push_back(cur_line);
                            cur_line = line{};
                            cur_main = 0;
                        }

                        cur_main += (cur_line.indices.empty() ? 0 : lp.gap) + child_ms;
                        cur_line.indices.push_back(i);
                        cur_line.line_size = max(cur_line.line_size, is_row ? measure_element_height(children[i])
                                                                            : measure_element_width(children[i]));
                    }
                    if (!cur_line.indices.empty()) {
                        lines.push_back(cur_line);
                    }

                    int total_cross = 0;
                    for (const auto& l: lines) {
                        total_cross += l.line_size;
                    }
                    total_cross += max(0, static_cast<int>(lines.size()) - 1) * lp.cross_gap;

                    const int cross_available = is_row ? content_h : content_w;
                    int line_cross_offset = is_row ? content_y : content_x;
                    const int extra_cross = max(0, cross_available - total_cross);
                    switch (lp.align_content) {
                        case style::align::center:
                            line_cross_offset += extra_cross / 2;
                            break;
                        case style::align::end:
                            line_cross_offset += extra_cross;
                            break;
                        case style::align::stretch: {
                            if (!lines.empty() && extra_cross > 0) {
                                const int addPerLine = extra_cross / static_cast<int>(lines.size());
                                for (auto& l: lines) {
                                    l.line_size += addPerLine;
                                }
                            }
                            break;
                        }
                        default:
                            break;
                    }

                    for (const auto& l: lines) {
                        const auto sizes =
                                compute_flex_sizes(l.indices, children, lp, is_row ? content_w : content_h, is_row);
                        assign_layout_flex_line(l.indices, children, lp, sizes, content_x, content_y, content_w,
                                                content_h, is_row, line_cross_offset, l.line_size, out);
                        line_cross_offset += l.line_size + lp.cross_gap;
                    }
                    return;
                }

                int total_flex = 0;
                int fixed_size = 0;
                for (const auto& child: children) {
                    const int fw = child_flex_weight(child, is_row);
                    if (fw > 0) {
                        total_flex += fw;
                    } else {
                        fixed_size += is_row ? measure_element_width(child) : measure_element_height(child, content_w);
                    }
                }

                const int total_gap = max(0, static_cast<int>(n) - 1) * lp.gap;
                const int flex_available = max(0, (is_row ? content_w : content_h) - fixed_size - total_gap);

                vector<int> child_sizes(n);
                {
                    int remaining_flex = total_flex;
                    int remaining_space = flex_available;
                    for (size_t i = 0; i < n; ++i) {
                        const auto& child = children[i];
                        const int fw = child_flex_weight(child, is_row);
                        if (fw > 0 && total_flex > 0) {
                            child_sizes[i] = (i == n - 1) ? remaining_space : flex_available * fw / total_flex;
                            remaining_flex -= fw;
                            remaining_space -= child_sizes[i];
                        } else {
                            child_sizes[i] =
                                    is_row ? measure_element_width(child) : measure_element_height(child, content_w);
                        }
                    }
                }

                int total_content_size = 0;
                for (size_t i = 0; i < n; ++i) {
                    total_content_size += child_sizes[i];
                }
                total_content_size += total_gap;

                const int overflow_limit = is_row ? content_w : content_h;
                if (total_content_size > overflow_limit && flex_available == 0) {
                    int non_flex_total = 0;
                    for (size_t i = 0; i < n; ++i) {
                        if (child_flex_weight(children[i], is_row) == 0) {
                            non_flex_total += child_sizes[i];
                        }
                    }
                    const int budget = max(1, overflow_limit - total_gap);
                    if (non_flex_total > budget) {
                        int remaining = budget;
                        for (size_t i = 0; i < n; ++i) {
                            if (child_flex_weight(children[i], is_row) == 0) {
                                child_sizes[i] = max(1, budget * child_sizes[i] / non_flex_total);
                                remaining -= child_sizes[i];
                            }
                        }
                        for (size_t i = n; i > 0;) {
                            --i;
                            if (child_flex_weight(children[i], is_row) == 0) {
                                child_sizes[i] = max(1, child_sizes[i] + remaining);
                                break;
                            }
                        }
                        total_content_size = budget + total_gap;
                    }
                }

                int justify_offset = 0;
                int adjusted_gap = lp.gap;
                const int available_size = is_row ? content_w : content_h;
                const int free_space = max(0, available_size - total_content_size);
                switch (lp.justify) {
                    case style::justify::center: {
                        justify_offset = free_space / 2;
                        break;
                    }
                    case style::justify::end: {
                        justify_offset = free_space;
                        break;
                    }
                    case style::justify::space_between: {
                        if (n > 1) {
                            adjusted_gap = lp.gap + free_space / static_cast<int>(n - 1);
                        }
                        break;
                    }
                    case style::justify::space_around: {
                        const int space = free_space / static_cast<int>(n);
                        justify_offset = space / 2;
                        adjusted_gap = lp.gap + space;
                        break;
                    }
                    case style::justify::start:
                    default: {
                        break;
                    }
                }

                int offset = is_row ? content_x + justify_offset : content_y + justify_offset;
                for (size_t i = 0; i < n; ++i) {
                    const auto& child = children[i];
                    const auto& cs = child.style();

                    const int child_alloc_w = is_row ? child_sizes[i] : content_w;
                    const int child_alloc_h = is_row ? content_h : child_sizes[i];

                    const enum style::align child_align = cs.align.value_or(lp.align);
                    bool is_stretch = (child_align == style::align::stretch);
                    int cross_x = content_x;
                    int cross_y = content_y;
                    if (!is_stretch) {
                        if (is_row) {
                            switch (child_align) {
                                case style::align::center: {
                                    cross_y = content_y + (content_h - child_alloc_h) / 2;
                                    break;
                                }
                                case style::align::end: {
                                    cross_y = content_y + content_h - child_alloc_h;
                                    break;
                                }
                                default: {
                                    break;
                                }
                            }
                        } else {
                            switch (child_align) {
                                case style::align::center: {
                                    cross_x = content_x + (content_w - child_alloc_w) / 2;
                                    break;
                                }
                                case style::align::end: {
                                    cross_x = content_x + content_w - child_alloc_w;
                                    break;
                                }
                                default: {
                                    break;
                                }
                            }
                        }
                    }

                    if (is_row) {
                        assign_layout(child, out, offset, is_stretch ? content_y : cross_y, child_alloc_w,
                                      is_stretch ? content_h : child_alloc_h);
                    } else {
                        assign_layout(child, out, is_stretch ? content_x : cross_x, offset,
                                      is_stretch ? content_w : child_alloc_w, child_alloc_h);
                    }
                    offset += child_sizes[i] + adjusted_gap;
                }
                return;
            }
            case element::kind::gridbox: {
                const int cols = el.grid_columns();
                const auto& children = el.children();
                if (cols <= 0 || children.empty()) {
                    return;
                }
                const int rows = static_cast<int>(children.size()) / cols;
                if (rows <= 0) {
                    return;
                }

                const int content_x = rect.x + lp.padding.left;
                const int content_y = rect.y + lp.padding.top;
                const int content_w = rect.w - lp.padding.left - lp.padding.right;
                const int content_h = rect.h - lp.padding.top - lp.padding.bottom;

                vector<int> column_widths(cols, 0);
                vector<int> column_flex_grow(cols, 1024);
                for (int c = 0; c < cols; ++c) {
                    for (int r = 0; r < rows; ++r) {
                        const size_t idx = static_cast<size_t>(r) * cols + c;
                        if (idx < children.size()) {
                            column_widths[c] = max(column_widths[c], measure_element_width(children[idx]));
                            const float fg = children[idx].style().flex_grow.value_or(1.0F);
                            column_flex_grow[c] = min(column_flex_grow[c], static_cast<int>(fg * 100));
                        }
                    }
                }

                vector<int> row_heights(rows, 0);
                vector<int> row_flex_grow(rows, 1024);
                for (int r = 0; r < rows; ++r) {
                    for (int c = 0; c < cols; ++c) {
                        const size_t idx = static_cast<size_t>(r) * cols + c;
                        if (idx < children.size()) {
                            row_heights[r] = max(row_heights[r], measure_element_height(children[idx]));
                            const float fg = children[idx].style().flex_grow.value_or(1.0F);
                            row_flex_grow[r] = min(row_flex_grow[r], static_cast<int>(fg * 100));
                        }
                    }
                }

                int total_column_w = 0;
                int total_column_flex = 0;
                for (int c = 0; c < cols; ++c) {
                    total_column_w += column_widths[c];
                    if (column_flex_grow[c] > 0) {
                        total_column_flex += column_flex_grow[c];
                    }
                }
                const int column_gap_total = max(0, cols - 1) * lp.gap;
                int extra_w = max(0, content_w - total_column_w - column_gap_total);
                if (total_column_flex > 0 && extra_w > 0) {
                    int remaining = extra_w;
                    for (int c = 0; c < cols; ++c) {
                        if (column_flex_grow[c] > 0) {
                            const int add =
                                    (c == cols - 1) ? remaining : extra_w * column_flex_grow[c] / total_column_flex;
                            column_widths[c] += add;
                            remaining -= add;
                        }
                    }
                }

                int total_row_h = 0;
                int total_row_flex = 0;
                for (int r = 0; r < rows; ++r) {
                    total_row_h += row_heights[r];
                    if (row_flex_grow[r] > 0) {
                        total_row_flex += row_flex_grow[r];
                    }
                }
                const int row_gap_total = max(0, rows - 1) * lp.cross_gap;
                int extra_h = max(0, content_h - total_row_h - row_gap_total);
                if (total_row_flex > 0 && extra_h > 0) {
                    int remaining = extra_h;
                    for (int r = 0; r < rows; ++r) {
                        if (row_flex_grow[r] > 0) {
                            const int add = (r == rows - 1) ? remaining : extra_h * row_flex_grow[r] / total_row_flex;
                            row_heights[r] += add;
                            remaining -= add;
                        }
                    }
                }

                int y_off = content_y;
                for (int r = 0; r < rows; ++r) {
                    int x_off = content_x;
                    for (int c = 0; c < cols; ++c) {
                        const size_t idx = static_cast<size_t>(r) * cols + c;
                        if (idx < children.size()) {
                            assign_layout(children[idx], out, x_off, y_off, column_widths[c], row_heights[r]);
                        }
                        x_off += column_widths[c] + lp.gap;
                    }
                    y_off += row_heights[r] + lp.cross_gap;
                }
                return;
            }
            case element::kind::empty:
            default: {
                return;
            }
        }
    }
} // namespace


vector<layout_rect> compute_layout(const element& element, int constraint_w, int constraint_h) {
    if (!element.is_layout_dirty() && element.cached_constraint_w() == constraint_w &&
        element.cached_constraint_h() == constraint_h) {
        return element.cached_layout();
    }

    vector<layout_rect> result;
    if (element.kind() == element::kind::empty) {
        element.set_cached_layout(result, constraint_w, constraint_h);
        return result;
    }
    assign_layout(element, result, 0, 0, constraint_w, constraint_h);
    element.set_cached_layout(result, constraint_w, constraint_h);
    return result;
}

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
