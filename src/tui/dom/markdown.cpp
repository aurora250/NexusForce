#include <NeForce/core/file/md/markdown_parser.hpp>
#include <NeForce/core/string/to_string.hpp>
#include <NeForce/core/string/utf_iterator.hpp>
#include <NeForce/core/utility/packages.hpp>
#include <NeForce/tui/dom/code_block.hpp>
#include <NeForce/tui/dom/latex.hpp>
#include <NeForce/core/file/mermaid/mermaid.hpp>
#include <NeForce/tui/dom/markdown.hpp>
#include <NeForce/tui/dom/text_layout.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

namespace {
    struct markdown_styles {
        tui::style body;
        tui::style heading;
        tui::style code;
        tui::style quote;
        tui::style list;
        tui::style link;
        tui::style gutter;
        tui::style header_cell;
        tui::style math;
        bool parse_math = true;
    };

    void append_span(vector<text_span>& out, const string& content, const tui::style& st) {
        if (content.empty()) {
            return;
        }
        out.push_back(text_span{content, st, string()});
    }

    void append_linked_span(vector<text_span>& out, const string& content, const tui::style& st, const string& link) {
        if (content.empty()) {
            return;
        }
        out.push_back(text_span{content, st, link});
    }

    void append_text_with_math(vector<text_span>& out, const string& text, const tui::style& base,
                               const markdown_styles& styles) {
        if (!styles.parse_math || !text.view().contains("$")) {
            append_span(out, text, base);
            return;
        }
        const auto& raw = text.view();
        string plain;
        size_t index = 0;
        while (index < raw.size()) {
            if (raw[index] != '$') {
                plain.push_back(raw[index]);
                ++index;
                continue;
            }
            size_t close = index + 1;
            while (close < raw.size() && raw[close] != '$') {
                ++close;
            }
            if (close >= raw.size() || close == index + 1) {
                plain.append(raw.data() + index, 1);
                ++index;
                continue;
            }
            const string_view tex = raw.substr(index + 1, close - index - 1);
            bool rendered_ok = false;
            if (const auto layout = render_latex(tex, false); layout.has_value() && layout->lines.size() == 1) {
                if (!plain.empty()) {
                    append_span(out, plain, base);
                    plain.clear();
                }
                append_span(out, layout->lines[0], styles.math);
                rendered_ok = true;
            }
            if (!rendered_ok) {
                plain.append(raw.data() + index, close - index + 1);
            }
            index = close + 1;
        }
        if (!plain.empty()) {
            append_span(out, plain, base);
        }
    }

    void append_inline_spans(const md_inlines& inlines, const tui::style& base, const markdown_styles& styles,
                             vector<text_span>& out) {
        for (const auto& node: inlines) {
            switch (node.kind) {
                case md_inline_kind::text: {
                    append_text_with_math(out, node.text, base, styles);
                    break;
                }
                case md_inline_kind::code: {
                    tui::style st = base;
                    st.reverse = styles.code.reverse.value_or(true);
                    if (styles.code.fg.has_value()) {
                        st.fg = styles.code.fg;
                    }
                    if (styles.code.bg.has_value()) {
                        st.bg = styles.code.bg;
                    }
                    append_span(out, node.text, st);
                    break;
                }
                case md_inline_kind::strong: {
                    tui::style st = base;
                    st.bold = true;
                    append_inline_spans(node.children, st, styles, out);
                    break;
                }
                case md_inline_kind::emphasis: {
                    tui::style st = base;
                    st.italic = true;
                    append_inline_spans(node.children, st, styles, out);
                    break;
                }
                case md_inline_kind::strikethrough: {
                    tui::style st = base;
                    st.strikethrough = true;
                    append_inline_spans(node.children, st, styles, out);
                    break;
                }
                case md_inline_kind::link:
                case md_inline_kind::autolink: {
                    tui::style st = base;
                    st.underline = true;
                    if (styles.link.fg.has_value()) {
                        st.fg = styles.link.fg;
                    }
                    if (node.kind == md_inline_kind::autolink) {
                        append_linked_span(out, node.text, st, node.destination);
                    } else {
                        vector<text_span> inner;
                        append_inline_spans(node.children, st, styles, inner);
                        for (auto& span: inner) {
                            if (span.link.empty()) {
                                span.link = node.destination;
                            }
                            out.push_back(_NEFORCE move(span));
                        }
                    }
                    break;
                }
                case md_inline_kind::image: {
                    tui::style st = base;
                    st.dim = true;
                    const string& alt = node.text.empty() ? node.destination : node.text;
                    append_span(out, "[img] ", st);
                    if (!node.destination.empty()) {
                        append_linked_span(out, alt, st, node.destination);
                    } else {
                        append_span(out, alt, st);
                    }
                    break;
                }
                case md_inline_kind::html: {
                    tui::style st = base;
                    st.dim = true;
                    append_span(out, node.text, st);
                    break;
                }
                case md_inline_kind::line_break: {
                    append_span(out, string("\n"), base);
                    break;
                }
            }
        }
    }

    vector<text_span> inline_spans(const md_inlines& inlines, const tui::style& base, const markdown_styles& styles,
                                   const string& prefix) {
        vector<text_span> spans;
        if (!prefix.empty()) {
            append_span(spans, prefix, styles.quote);
        }
        append_inline_spans(inlines, base, styles, spans);
        if (spans.empty()) {
            append_span(spans, string(), base);
        }
        return spans;
    }

    string cell_text(const md_inlines& inlines) { return md_plain_text(inlines); }

    string spaces_of(const size_t count) {
        string result;
        for (size_t i = 0; i < count; ++i) {
            result += ' ';
        }
        return result;
    }

    string pad_cell(const string& text, const int width, const md_align align) {
        const int text_width = text_display_width(text.view());
        const int padding = max(0, width - text_width);
        if (padding == 0) {
            return text;
        }
        switch (align) {
            case md_align::right: {
                string result = spaces_of(static_cast<size_t>(padding));
                return move(result) + text;
            }
            case md_align::center: {
                string left = spaces_of(static_cast<size_t>(padding / 2));
                string right = spaces_of(static_cast<size_t>(padding - padding / 2));
                return move(left) + text + move(right);
            }
            default: {
                return text + spaces_of(static_cast<size_t>(padding));
            }
        }
    }

    string repeat_glyph(const char* glyph, const int count) {
        string result;
        for (int i = 0; i < count; ++i) {
            result += glyph;
        }
        return result;
    }

    int longest_word_width(const string& text) {
        int best = 0;
        int current = 0;
        const auto& raw = text.view();
        for (utf8_iterator it(reinterpret_cast<const byte_t*>(raw.data()), raw.size()); it != utf8_iterator(); ++it) {
            if (it->value() == ' ') {
                best = max(best, current);
                current = 0;
            } else {
                current += max(1, it->display_width());
            }
        }
        return max(best, current);
    }

    vector<string> wrap_cell(const string& text, const int width) {
        vector<string> lines;
        if (width <= 0) {
            lines.push_back(text);
            return lines;
        }
        const text_layout soft = layout_text(text.view(), width, style::wrap_mode::word);
        for (const auto& line: soft.lines) {
            string piece{text.view().substr(line.begin, line.end - line.begin)};
            if (text_display_width(piece.view()) <= width) {
                lines.push_back(_NEFORCE move(piece));
                continue;
            }
            // A single word wider than the column: break it per character.
            const text_layout hard = layout_text(piece.view(), width, style::wrap_mode::character);
            for (const auto& part: hard.lines) {
                lines.push_back(string{piece.view().substr(part.begin, part.end - part.begin)});
            }
        }
        if (lines.empty()) {
            lines.push_back(string());
        }
        return lines;
    }

    element render_table(const md_block& block, const markdown_option& opt, const markdown_styles& styles) {
        const size_t columns = block.header.size();
        if (columns == 0) {
            return element::text(string());
        }

        vector<vector<string>> rows_text;
        rows_text.push_back(vector<string>());
        for (size_t c = 0; c < columns; ++c) {
            rows_text.back().push_back(block.header[c].empty() ? string() : cell_text(block.header[c]));
        }
        for (const auto& row: block.rows) {
            rows_text.push_back(vector<string>());
            for (size_t c = 0; c < columns; ++c) {
                rows_text.back().push_back(c < row.size() ? cell_text(row[c]) : string());
            }
        }

        vector<int> minima(columns, 3);
        vector<int> ideals(columns, 3);
        for (const auto& row: rows_text) {
            for (size_t c = 0; c < columns; ++c) {
                ideals[c] = max(ideals[c], text_display_width(row[c].view()));
                minima[c] = max(minima[c], longest_word_width(row[c]));
            }
        }

        int total_min = 0;
        int total_ideal = 0;
        for (size_t c = 0; c < columns; ++c) {
            total_min += minima[c];
            total_ideal += ideals[c];
        }

        const int budget = opt.width > 0 ? opt.width - static_cast<int>(1 + 3 * columns) : -1;
        vector<int> widths(columns, 0);
        if (budget <= 0 || total_ideal <= budget) {
            widths = ideals;
        } else if (total_min > budget) {
            widths = minima;
        } else {
            widths = minima;
            vector<int> room(columns, 0);
            for (size_t c = 0; c < columns; ++c) {
                room[c] = ideals[c] - minima[c];
            }
            int remaining = budget - total_min;
            while (remaining > 0) {
                size_t best = columns;
                for (size_t c = 0; c < columns; ++c) {
                    if (room[c] <= 0) {
                        continue;
                    }
                    if (best == columns || room[c] > room[best]) {
                        best = c;
                    }
                }
                if (best == columns) {
                    break;
                }
                ++widths[best];
                --room[best];
                --remaining;
            }
        }

        vector<vector<vector<string>>> wrapped(rows_text.size());
        for (size_t r = 0; r < rows_text.size(); ++r) {
            wrapped[r].resize(columns);
            for (size_t c = 0; c < columns; ++c) {
                wrapped[r][c] = wrap_cell(rows_text[r][c], widths[c]);
            }
        }

        const auto align_of = [&block](const size_t c) {
            return c < block.alignments.size() ? block.alignments[c] : md_align::none;
        };

        const auto border_row = [&](const char* left, const char* middle, const char* right) {
            string line{left};
            for (size_t c = 0; c < columns; ++c) {
                line += repeat_glyph("\xe2\x94\x80", widths[c] + 2);
                line += (c + 1 < columns) ? middle : right;
            }
            return element::text(line, styles.gutter, style::wrap_mode::none);
        };

        const auto cell_row = [&](const size_t index, const tui::style& style, const bool header) {
            size_t row_height = 1;
            for (size_t c = 0; c < columns; ++c) {
                row_height = max(row_height, wrapped[index][c].size());
            }
            elements rows;
            for (size_t line_index = 0; line_index < row_height; ++line_index) {
                elements cells;
                cells.push_back(element::text("\xe2\x94\x82", styles.gutter, style::wrap_mode::none));
                for (size_t c = 0; c < columns; ++c) {
                    const auto& cell_lines = wrapped[index][c];
                    string content;
                    if (line_index < cell_lines.size()) {
                        content = cell_lines[line_index];
                    }
                    const auto align = header ? md_align::center : align_of(c);
                    content = pad_cell(content, widths[c], align);
                    cells.push_back(element::text(" " + content + " ", style, style::wrap_mode::none));
                    cells.push_back(element::text("\xe2\x94\x82", styles.gutter, style::wrap_mode::none));
                }
                rows.push_back(element::hbox(_NEFORCE move(cells)));
            }
            return element::vbox(_NEFORCE move(rows));
        };

        elements rows;
        rows.push_back(border_row("\xe2\x94\x8c", "\xe2\x94\xac", "\xe2\x94\x90"));
        tui::style header_style = styles.header_cell;
        header_style.bold = true;
        rows.push_back(cell_row(0, header_style, true));
        if (rows_text.size() > 1) {
            rows.push_back(border_row("\xe2\x94\x9c", "\xe2\x94\xbc", "\xe2\x94\xa4"));
            for (size_t r = 1; r < rows_text.size(); ++r) {
                rows.push_back(cell_row(r, opt.style, false));
                if (r + 1 < rows_text.size()) {
                    rows.push_back(border_row("\xe2\x94\x9c", "\xe2\x94\xbc", "\xe2\x94\xa4"));
                }
            }
        }
        rows.push_back(border_row("\xe2\x94\x94", "\xe2\x94\xb4", "\xe2\x94\x98"));

        element table = element::vbox(_NEFORCE move(rows));
        if (opt.width > 0) {
            tui::style width_style;
            width_style.width = style::size_hint{style::size_hint::fixed, opt.width};
            table = table.with_style(width_style);
        }
        return table;
    }

    element render_block(const md_block& block, const markdown_option& opt, const markdown_styles& styles,
                         const string& quote_prefix);

    bool is_mermaid_language(const string& language) {
        return language == string{"mermaid"} || language == string{"mmd"};
    }

    element render_mermaid_block(const md_block& block, const markdown_option& opt, const markdown_styles& styles) {
        const optional<mermaid_art> art = render_mermaid(block.literal.view());
        if (!art.has_value() || art->lines.empty()) {
            return element::empty();
        }
        if (opt.width > 0 && art->width > opt.width - 6) {
            return element::empty();
        }

        elements rows;
        for (size_t i = 0; i < art->lines.size(); ++i) {
            vector<text_span> spans;
            const auto& runs = (i < art->styled.size()) ? art->styled[i] : vector<mermaid_run>{};
            if (runs.empty()) {
                spans.push_back(text_span{string{"  "} + art->lines[i], opt.style, string()});
            } else {
                spans.push_back(text_span{string{"  "}, opt.style, string()});
                for (const auto& run: runs) {
                    tui::style st = opt.style;
                    switch (run.role) {
                        case mermaid_role::border:
                            st = styles.gutter;
                            break;
                        case mermaid_role::edge:
                            st.fg = styles.list.fg;
                            break;
                        case mermaid_role::edge_label:
                        case mermaid_role::title:
                            st = styles.quote;
                            break;
                        case mermaid_role::text:
                        case mermaid_role::plain:
                        default:
                            break;
                    }
                    const size_t begin = min(run.begin, art->lines[i].size());
                    const size_t end = min(run.end, art->lines[i].size());
                    if (end > begin) {
                        spans.push_back(text_span{art->lines[i].substr(begin, end - begin), st, string()});
                    }
                }
            }
            rows.push_back(element::rich_text(_NEFORCE move(spans), opt.style, style::wrap_mode::none));
        }
        return element::vbox(_NEFORCE move(rows));
    }

    element render_list(const md_block& list, const markdown_option& opt, const markdown_styles& styles,
                        const string& quote_prefix) {
        elements rows;

        for (const auto& item: list.children) {
            const md_block* first_paragraph = nullptr;
            for (const auto& child: item.children) {
                if (child.kind == md_block_kind::paragraph) {
                    first_paragraph = &child;
                    break;
                }
            }

            string marker;
            if (item.task) {
                marker = item.checked ? "[\xe2\x9c\x93] " : "[ ] ";
            } else if (item.ordered) {
                marker = to_string(static_cast<int64_t>(item.number)) + ". ";
            } else {
                marker = "\xe2\x80\xa2 ";
            }

            vector<text_span> spans;
            if (first_paragraph != nullptr) {
                append_inline_spans(first_paragraph->inlines, opt.style, styles, spans);
            }
            if (spans.empty()) {
                append_span(spans, string(), opt.style);
            }

            elements cells;
            cells.push_back(element::text(quote_prefix + marker, styles.list, style::wrap_mode::none));
            cells.push_back(element::rich_text(move(spans), opt.style, style::wrap_mode::word));
            rows.push_back(element::hbox(move(cells)));

            const int content_width = text_display_width(marker.view());
            for (const auto& child: item.children) {
                if (&child == first_paragraph) {
                    continue;
                }
                element inner = (child.kind == md_block_kind::list) ? render_list(child, opt, styles, quote_prefix)
                                                                    : render_block(child, opt, styles, quote_prefix);
                elements nested;
                nested.push_back(element::text(spaces_of(static_cast<size_t>(content_width)), opt.style,
                                               style::wrap_mode::none));
                nested.push_back(move(inner));
                rows.push_back(element::hbox(move(nested)));
            }
        }

        return element::vbox(move(rows));
    }

    element render_block(const md_block& block, const markdown_option& opt, const markdown_styles& styles,
                         const string& quote_prefix) {
        switch (block.kind) {
            case md_block_kind::paragraph: {
                element paragraph = element::rich_text(inline_spans(block.inlines, opt.style, styles, quote_prefix),
                                                       opt.style, style::wrap_mode::word);
                if (!quote_prefix.empty()) {
                    paragraph.with_hang(vector<text_span>{text_span{quote_prefix, styles.quote, string()}});
                }
                return paragraph;
            }
            case md_block_kind::heading: {
                tui::style st = styles.heading;
                st.bold = true;
                element heading = element::rich_text(inline_spans(block.inlines, st, styles, quote_prefix), st,
                                                     style::wrap_mode::word);
                if (!quote_prefix.empty()) {
                    heading.with_hang(vector<text_span>{text_span{quote_prefix, styles.quote, string()}});
                }
                return heading;
            }
            case md_block_kind::code_block: {
                if (is_mermaid_language(block.language)) {
                    element diagram = render_mermaid_block(block, opt, styles);
                    if (diagram.kind() != element::kind::empty) {
                        return diagram;
                    }
                }
                code_block_option cb;
                cb.code = block.literal;
                cb.language = block.language;
                cb.width = opt.width;
                cb.highlight = opt.highlight_code;
                cb.style = opt.style;
                if (styles.code.bg.has_value()) {
                    cb.style.bg = styles.code.bg;
                    cb.band_style.bg = styles.code.bg;
                }
                return code_block(move(cb));
            }
            case md_block_kind::html_block: {
                elements lines;
                const auto& literal = block.literal;
                size_t start = 0;
                for (size_t i = 0; i <= literal.size(); ++i) {
                    if (i == literal.size() || literal[i] == '\n') {
                        if (i > start) {
                            lines.push_back(element::text(string(literal.view().substr(start, i - start)), styles.quote,
                                                          style::wrap_mode::none));
                        }
                        start = i + 1;
                    }
                }
                if (lines.empty()) {
                    return element::text(string(), styles.quote);
                }
                return element::vbox(move(lines));
            }
            case md_block_kind::thematic_break: {
                string rule;
                const int count = max(1, opt.width > 0 ? opt.width : 20);
                for (int i = 0; i < count; ++i) {
                    rule += "\xe2\x94\x80";
                }
                return element::text(rule, styles.quote, style::wrap_mode::none);
            }
            case md_block_kind::block_quote: {
                elements children;
                // Nesting depth is encoded in the accumulated prefix
                // (one level is four bytes: a three-byte rail glyph plus a space).
                const size_t depth = quote_prefix.size() / 4;
                const char* rail = depth >= 2 ? "\xe2\x96\x8f " : "\xe2\x96\x8e ";
                const string prefix = quote_prefix + rail;
                for (const auto& child: block.children) {
                    children.push_back(render_block(child, opt, styles, prefix));
                }
                if (children.empty()) {
                    children.push_back(element::text(quote_prefix, styles.quote, style::wrap_mode::none));
                }
                return element::vbox(move(children));
            }
            case md_block_kind::list: {
                return render_list(block, opt, styles, quote_prefix);
            }
            case md_block_kind::list_item: {
                md_block single;
                single.kind = md_block_kind::list;
                single.tight = true;
                single.children.push_back(block);
                return render_list(single, opt, styles, quote_prefix);
            }
            case md_block_kind::table: {
                return render_table(block, opt, styles);
            }
        }
        return element::text(string());
    }

    markdown_styles make_styles(const markdown_option& opt) {
        markdown_styles styles;
        styles.body = opt.style;
        styles.heading = opt.heading_style;
        if (!styles.heading.fg.has_value()) {
            styles.heading.fg = _NEFORCE color::cyan();
        }
        styles.code = opt.code_style;
        styles.quote = opt.quote_style;
        if (!styles.quote.dim.has_value()) {
            styles.quote.dim = true;
        }
        styles.list = opt.list_style;
        if (!styles.list.fg.has_value()) {
            styles.list.fg = _NEFORCE color::cyan();
        }
        styles.link = opt.link_style;
        styles.math = opt.math_style;
        if (!styles.math.italic.has_value()) {
            styles.math.italic = true;
        }
        styles.parse_math = opt.parse_math;
        styles.gutter = opt.quote_style;
        if (!styles.gutter.dim.has_value()) {
            styles.gutter.dim = true;
        }
        styles.header_cell = opt.style;
        styles.header_cell.bold = true;
        return styles;
    }
} // namespace


vector<text_span> parse_inline_markdown(const string_view text, const tui::style& base, const tui::style& code_style) {
    markdown_option opt;
    opt.style = base;
    opt.code_style = code_style;
    const auto styles = make_styles(opt);

    vector<text_span> spans;
    append_inline_spans(markdown_parser(string(text)).parse_inline(), base, styles, spans);
    if (spans.empty()) {
        spans.push_back(text_span{string(), base});
    }
    return spans;
}


namespace {
    struct math_segment {
        bool is_math = false;
        string content;
    };

    vector<math_segment> split_math_blocks(const string_view text) {
        vector<math_segment> segments;
        segments.push_back(math_segment{});

        bool in_fence = false;
        char fence_char = 0;
        size_t fence_len = 0;

        size_t cursor = 0;
        while (cursor < text.size()) {
            size_t line_end = cursor;
            while (line_end < text.size() && text[line_end] != '\n') {
                ++line_end;
            }
            string_view line = text.substr(cursor, line_end - cursor);

            size_t indent = 0;
            while (indent < line.size() && (line[indent] == ' ' || line[indent] == '\t')) {
                ++indent;
            }

            // Fenced code blocks are opaque: `$$` inside them is source text.
            size_t run = 0;
            const char first = (indent < line.size()) ? line[indent] : '\0';
            if (first == '`' || first == '~') {
                while (indent + run < line.size() && line[indent + run] == first) {
                    ++run;
                }
            }
            if (!in_fence && run >= 3) {
                in_fence = true;
                fence_char = first;
                fence_len = run;
            } else if (in_fence && first == fence_char && run >= fence_len) {
                in_fence = false;
            }

            const bool opens = !in_fence && line.size() >= indent + 2 && line.substr(indent, 2) == string_view{"$$"};
            if (opens) {
                // $$$...$$$ on one line: the closer must end the line.
                size_t close = string_view::npos;
                for (size_t i = indent + 2; i + 1 < line.size(); ++i) {
                    if (line[i] == '$' && line[i + 1] == '$') {
                        close = i;
                        break;
                    }
                }
                if (close != string_view::npos) {
                    math_segment math;
                    math.is_math = true;
                    math.content = string{line.substr(indent + 2, close - indent - 2)};
                    segments.push_back(move(math));
                    segments.push_back(math_segment{});
                    cursor = line_end + 1;
                    continue;
                }

                // Multi-line block: collect until a closing `$$` line.
                string body;
                size_t scan = line_end + 1;
                size_t after = text.size();
                bool closed = false;
                while (scan < text.size()) {
                    size_t end = scan;
                    while (end < text.size() && text[end] != '\n') {
                        ++end;
                    }
                    const string_view inner = text.substr(scan, end - scan);
                    size_t inner_indent = 0;
                    while (inner_indent < inner.size() && (inner[inner_indent] == ' ' || inner[inner_indent] == '\t')) {
                        ++inner_indent;
                    }
                    if (inner.size() >= inner_indent + 2 && inner.substr(inner_indent, 2) == string_view{"$$"}) {
                        after = end + 1;
                        closed = true;
                        break;
                    }
                    body.append(inner.data(), inner.size());
                    body.push_back('\n');
                    scan = end + 1;
                }
                if (closed) {
                    math_segment math;
                    math.is_math = true;
                    math.content = move(body);
                    segments.push_back(move(math));
                    segments.push_back(math_segment{});
                    cursor = after;
                    continue;
                }
            }

            segments.back().content.append(line.data(), line.size());
            if (line_end < text.size()) {
                segments.back().content.push_back('\n');
            }
            cursor = line_end + 1;
        }
        return segments;
    }
} // namespace


element markdown(const string_view text, markdown_option opt) {
    if (opt.parse_math) {
        vector<math_segment> segments = split_math_blocks(text);
        bool has_math = false;
        for (const auto& segment: segments) {
            has_math = has_math || segment.is_math;
        }
        if (has_math) {
            elements parts;
            bool first = true;
            markdown_option plain_opt = opt;
            plain_opt.parse_math = false;
            for (const auto& segment: segments) {
                if (segment.is_math) {
                    const auto layout = render_latex(segment.content.view(), true);
                    bool fits = layout.has_value() && !layout->lines.empty();
                    if (fits && opt.width > 0) {
                        for (const auto& line: layout->lines) {
                            if (text_display_width(line.view()) > opt.width - 6) {
                                fits = false;
                                break;
                            }
                        }
                    }
                    if (fits) {
                        if (!first) {
                            parts.push_back(element::text(string(), opt.style, style::wrap_mode::none));
                        }
                        string rendered;
                        for (const auto& line: layout->lines) {
                            if (!rendered.empty()) {
                                rendered.push_back('\n');
                            }
                            rendered += "  ";
                            rendered += line;
                        }
                        parts.push_back(element::text(rendered, opt.math_style, style::wrap_mode::none));
                        first = false;
                    } else {
                        if (!first) {
                            parts.push_back(element::text(string(), opt.style, style::wrap_mode::none));
                        }
                        string literal{"$$"};
                        literal += segment.content;
                        literal += "$$";
                        parts.push_back(markdown(literal.view(), plain_opt));
                        first = false;
                    }
                    continue;
                }
                if (segment.content.empty()) {
                    continue;
                }
                if (!first) {
                    parts.push_back(element::text(string(), opt.style, style::wrap_mode::none));
                }
                parts.push_back(markdown(segment.content.view(), plain_opt));
                first = false;
            }
            if (parts.empty()) {
                parts.push_back(element::text(string(), opt.style));
            }
            element composed = element::vbox(move(parts));
            if (opt.width > 0) {
                tui::style width_style;
                width_style.width = style::size_hint{style::size_hint::fixed, opt.width};
                composed = composed.with_style(width_style);
            }
            return composed;
        }
    }

    const auto document = markdown_parser(string(text)).parse();
    const auto styles = make_styles(opt);

    elements blocks;
    for (size_t i = 0; i < document.blocks.size(); ++i) {
        if (i > 0) {
            blocks.push_back(element::text(string(), opt.style, style::wrap_mode::none));
        }
        blocks.push_back(render_block(document.blocks[i], opt, styles, string()));
    }

    if (blocks.empty()) {
        blocks.push_back(element::text(string(), opt.style));
    }

    element body = element::vbox(move(blocks));
    if (opt.width > 0) {
        tui::style width_style;
        width_style.width = style::size_hint{style::size_hint::fixed, opt.width};
        body = body.with_style(width_style);
    }
    return body;
}

struct markdown_cache::entry {
    size_t hash = 0;
    int width = 0;
    uint64_t stamp = 0;
    element tree = element::empty();
};

markdown_cache::~markdown_cache() = default;

markdown_cache::markdown_cache(markdown_option opt, const size_t capacity) :
entries_(make_unique<entry[]>(max<size_t>(1, capacity))),
capacity_(max<size_t>(1, capacity)),
opt_(move(opt)) {}

const element& markdown_cache::render(const string_view text, const int width) {
    const int effective_width = (width > 0) ? width : opt_.width;
    const size_t hash = FNV_hash_string32(text.data(), text.size());
    const uint64_t stamp = ++stamp_;

    for (size_t i = 0; i < count_; ++i) {
        entry& candidate = entries_[i];
        if (candidate.hash == hash && candidate.width == effective_width) {
            candidate.stamp = stamp;
            ++hits_;
            return candidate.tree;
        }
    }

    ++misses_;

    markdown_option opt = opt_;
    opt.width = effective_width;
    element tree = markdown(text, move(opt));

    if (count_ < capacity_) {
        entry& slot = entries_[count_++];
        slot.hash = hash;
        slot.width = effective_width;
        slot.stamp = stamp;
        slot.tree = move(tree);
        return slot.tree;
    }

    size_t victim = 0;
    for (size_t i = 1; i < count_; ++i) {
        if (entries_[i].stamp < entries_[victim].stamp) {
            victim = i;
        }
    }
    entry& slot = entries_[victim];
    slot.hash = hash;
    slot.width = effective_width;
    slot.stamp = stamp;
    slot.tree = move(tree);
    return slot.tree;
}

void markdown_cache::clear() { count_ = 0; }

size_t markdown_cache::size() const noexcept { return count_; }

size_t markdown_cache::hit_count() const noexcept { return hits_; }

size_t markdown_cache::miss_count() const noexcept { return misses_; }

const markdown_option& markdown_cache::options() const noexcept { return opt_; }


namespace {
    // Returns the largest safe seal offset:
    // the end of the last blank line that is not inside a fenced code block,
    // provided at least `min_tail` bytes stay behind.
    // Sealing inside a fence would split a code block in half.
    size_t find_seal_boundary(const string_view text, const size_t from, const size_t min_tail) {
        size_t candidate = from;
        size_t line_start = from;
        bool in_fence = false;
        char fence_char = 0;
        size_t fence_len = 0;

        while (line_start < text.size()) {
            size_t line_end = line_start;
            while (line_end < text.size() && text[line_end] != '\n') {
                ++line_end;
            }

            size_t cursor = line_start;
            size_t indent = 0;
            while (cursor < line_end && text[cursor] == ' ' && indent < 4) {
                ++cursor;
                ++indent;
            }
            size_t run = 0;
            const char first = cursor < line_end ? text[cursor] : '\0';
            if (first == '`' || first == '~') {
                while (cursor + run < line_end && text[cursor + run] == first) {
                    ++run;
                }
            }

            if (!in_fence && run >= 3) {
                in_fence = true;
                fence_char = first;
                fence_len = run;
            } else if (in_fence && first == fence_char && run >= fence_len) {
                in_fence = false;
            } else if (!in_fence && cursor >= line_end) {
                // Blank line outside a fence: a chunk may end here,
                // as long as the unsealed tail keeps at least `min_tail` bytes.
                if (line_end + 1 + min_tail <= text.size()) {
                    candidate = line_end + 1;
                }
            }

            line_start = line_end + 1;
        }

        if (candidate > from && text.size() - candidate >= min_tail) {
            return candidate;
        }
        return from;
    }
} // namespace


struct markdown_stream::impl {
    explicit impl(markdown_option options) :
    opt(move(options)) {}

    markdown_option opt;
    string text;
    size_t sealed = 0;
    vector<element> blocks;
    element rendered;
    int width = 0;
};

markdown_stream::markdown_stream(markdown_option opt) :
impl_(make_unique<impl>(move(opt))) {}

markdown_stream::~markdown_stream() = default;

void markdown_stream::append(const string_view chunk) {
    if (chunk.empty()) {
        return;
    }
    impl_->text.append(chunk);
}

void markdown_stream::set_text(const string_view text) {
    const string_view current{impl_->text.view()};
    const bool extends_prefix = text.size() >= current.size() && text.starts_with(current);
    if (!extends_prefix) {
        reset();
    }
    impl_->text = string{text};
}

void markdown_stream::reset() {
    impl_->text.clear();
    impl_->sealed = 0;
    impl_->blocks.clear();
    impl_->rendered = element::empty();
}

const element& markdown_stream::render(int width) {
    if (width <= 0) {
        width = impl_->opt.width;
    }
    impl_->width = width;

    markdown_option opt = impl_->opt;
    opt.width = width;

    const string_view full{impl_->text.view()};
    const size_t boundary = find_seal_boundary(full, impl_->sealed, 1024);
    if (boundary > impl_->sealed) {
        markdown_option chunk_opt = opt;
        chunk_opt.width = width;
        const string_view chunk = full.substr(impl_->sealed, boundary - impl_->sealed);
        impl_->blocks.push_back(markdown(chunk, chunk_opt));
        impl_->sealed = boundary;
    }

    elements rows;
    rows.reserve(impl_->blocks.size() + 1);
    for (const auto& block: impl_->blocks) {
        rows.push_back(block);
        rows.push_back(element::text(string()));
    }
    const string_view tail = full.substr(impl_->sealed);
    if (!tail.empty()) {
        rows.push_back(markdown(tail, opt));
    }
    impl_->rendered = element::vbox(move(rows));
    return impl_->rendered;
}

string_view markdown_stream::text() const noexcept { return impl_->text.view(); }

size_t markdown_stream::sealed_bytes() const noexcept { return impl_->sealed; }

size_t markdown_stream::sealed_blocks() const noexcept { return impl_->blocks.size(); }

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
