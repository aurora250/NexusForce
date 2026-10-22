#include <NeForce/core/file/md/markdown_parser.hpp>
#include <NeForce/core/string/to_string.hpp>
#include <NeForce/core/utility/packages.hpp>
#include <NeForce/tui/dom/code_block.hpp>
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
    };

    void append_span(vector<text_span>& out, const string& content, const tui::style& st) {
        if (content.empty()) {
            return;
        }
        out.push_back(text_span{content, st});
    }

    void append_inline_spans(const md_inlines& inlines, const tui::style& base, const markdown_styles& styles,
                             vector<text_span>& out) {
        for (const auto& node: inlines) {
            switch (node.kind) {
                case md_inline_kind::text: {
                    append_span(out, node.text, base);
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
                        append_span(out, node.text, st);
                    } else {
                        append_inline_spans(node.children, st, styles, out);
                    }
                    break;
                }
                case md_inline_kind::image: {
                    tui::style st = base;
                    st.dim = true;
                    append_span(out, node.text.empty() ? string("image") : node.text, st);
                    if (!node.destination.empty()) {
                        append_span(out, " (" + node.destination + ")", st);
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

    element render_table(const md_block& block, const markdown_option& opt, const markdown_styles& styles) {
        const size_t columns = block.header.size();
        if (columns == 0) {
            return element::text(string());
        }

        vector<int> widths(columns, 0);
        for (size_t c = 0; c < columns; ++c) {
            widths[c] = text_display_width(cell_text(block.header[c]).view());
        }
        for (const auto& row: block.rows) {
            for (size_t c = 0; c < columns && c < row.size(); ++c) {
                widths[c] = max(widths[c], text_display_width(cell_text(row[c]).view()));
            }
        }

        const auto align_of = [&block](const size_t c) {
            return c < block.alignments.size() ? block.alignments[c] : md_align::none;
        };

        elements rows;
        const auto build_row = [&](const vector<md_inlines>& cells, const tui::style& style) {
            vector<text_span> spans;
            for (size_t c = 0; c < columns; ++c) {
                if (c > 0) {
                    append_span(spans, " \xe2\x94\x82 ", styles.gutter);
                }
                const string text = (c < cells.size()) ? cell_text(cells[c]) : string();
                append_span(spans, pad_cell(text, widths[c], align_of(c)), style);
            }
            return element::rich_text(move(spans), style, style::wrap_mode::none);
        };

        rows.push_back(build_row(block.header, styles.header_cell));

        string separator;
        for (size_t c = 0; c < columns; ++c) {
            if (c > 0) {
                separator += "\xe2\x94\xbc";
            }
            for (int i = 0; i < widths[c]; ++i) {
                separator += "\xe2\x94\x80";
            }
        }
        rows.push_back(element::text(separator, styles.gutter, style::wrap_mode::none));

        for (const auto& row: block.rows) {
            rows.push_back(build_row(row, opt.style));
        }

        element table = element::vbox(move(rows));
        if (opt.width > 0) {
            tui::style width_style;
            width_style.width = style::size_hint{style::size_hint::fixed, opt.width};
            table = table.with_style(width_style);
        }
        return table;
    }

    element render_block(const md_block& block, const markdown_option& opt, const markdown_styles& styles,
                         const string& quote_prefix);

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
                return element::rich_text(inline_spans(block.inlines, opt.style, styles, quote_prefix), opt.style,
                                          style::wrap_mode::word);
            }
            case md_block_kind::heading: {
                tui::style st = styles.heading;
                st.bold = true;
                return element::rich_text(inline_spans(block.inlines, st, styles, quote_prefix), st,
                                          style::wrap_mode::word);
            }
            case md_block_kind::code_block: {
                code_block_option cb;
                cb.code = block.literal;
                cb.language = block.language;
                cb.width = opt.width;
                cb.highlight = opt.highlight_code;
                cb.style = opt.style;
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
                const string prefix = quote_prefix + "\xe2\x94\x82 ";
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

element markdown(const string_view text, markdown_option opt) {
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

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
