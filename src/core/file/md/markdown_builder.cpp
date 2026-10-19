#include <NeForce/core/file/md/markdown_builder.hpp>
#include <NeForce/core/string/to_string.hpp>
#include <NeForce/core/string/char_types.hpp>
#include <NeForce/core/utility/packages.hpp>
NEFORCE_BEGIN_NAMESPACE__

string markdown_document::to_string() const { return markdown_builder::dump(*this); }


namespace {
    bool needs_escape(const char chr) noexcept {
        const auto c = static_cast<byte_t>(chr);
        if (c < 0x21 || c > 0x7E) {
            return false;
        }
        const uint32_t idx = c - 0x21;
        if (idx < 64) {
            // 0x21..0x60
            constexpr uint64_t lo = 0xFC00000008000225ULL;
            return ((lo >> idx) & 1) != 0U;
        }
        // 0x61..0x7E
        constexpr uint32_t hi = 0x28000000U;
        return ((hi >> (idx - 64)) & 1) != 0U;
    }

    bool is_line_start_special(const char chr) noexcept {
        const auto c = static_cast<byte_t>(chr);
        const uint32_t t = c - 0x2A; // 0x2A == '*'
        if (t < 21) {                // 0x2A..0x3E
            // bits: '*'=0, '+'=1, '-'=3, '='=19, '>'=20
            return ((0x18000BULL >> t) & 1) != 0U;
        }
        return c == '#' || c == '_' || c == '|';
    }

    void append_escaped(const string& text, string& out, const bool at_line_start) {
        const string_view view = text.view();
        bool line_start = at_line_start;
        size_t pos = 0;

        while (pos < view.size()) {
            if (view[pos] == '\n') {
                out += '\n';
                line_start = true;
                ++pos;
                continue;
            }

            if (line_start) {
                line_start = false;
                if (is_digit(view[pos])) {
                    size_t cursor = pos;
                    while (cursor < view.size() && is_digit(view[cursor])) {
                        ++cursor;
                    }
                    if (cursor < view.size() && (view[cursor] == '.' || view[cursor] == ')')) {
                        out.append(view.substr(pos, cursor - pos));
                        out += '\\';
                        out += view[cursor];
                        pos = cursor + 1;
                        continue;
                    }
                } else if (is_line_start_special(view[pos]) && !needs_escape(view[pos])) {
                    // '-', '+', '>' and '=' are not part of the inline escape set,
                    // so the line-start backslash is emitted here.
                    out += '\\';
                }
            }

            const size_t run_begin = pos;
            while (pos < view.size() && view[pos] != '\n' && !needs_escape(view[pos])) {
                ++pos;
            }
            if (pos > run_begin) {
                out.append(view.substr(run_begin, pos - run_begin));
            }
            if (pos >= view.size() || view[pos] == '\n') {
                continue;
            }
            out += '\\';
            out += view[pos];
            ++pos;
        }
    }

    string inline_code_fence(const string& text) {
        int longest = 0;
        int current = 0;
        for (size_t i = 0; i < text.size(); ++i) {
            if (text[i] == '`') {
                ++current;
                longest = max(longest, current);
            } else {
                current = 0;
            }
        }
        return string::repeat('`', max(1, longest + 1));
    }

    string block_fence(const string& literal, const char preferred) {
        const char marker = (preferred == '`' || preferred == '~') ? preferred : '`';
        int longest = 0;
        int current = 0;
        for (size_t i = 0; i < literal.size(); ++i) {
            if (literal[i] == marker) {
                ++current;
                longest = max(longest, current);
            } else {
                current = 0;
            }
        }
        return string::repeat(marker, max(3, longest + 1));
    }

    void append_inlines_text(const md_inlines& inlines, string& out, const bool at_line_start) {
        bool line_start = at_line_start;
        for (const auto& node: inlines) {
            switch (node.kind) {
                case md_inline_kind::text: {
                    append_escaped(node.text, out, line_start);
                    line_start = !node.text.empty() && node.text.back() == '\n';
                    break;
                }
                case md_inline_kind::strong: {
                    out += "**";
                    append_inlines_text(node.children, out, false);
                    out += "**";
                    line_start = false;
                    break;
                }
                case md_inline_kind::emphasis: {
                    out += '*';
                    append_inlines_text(node.children, out, false);
                    out += '*';
                    line_start = false;
                    break;
                }
                case md_inline_kind::strikethrough: {
                    out += "~~";
                    append_inlines_text(node.children, out, false);
                    out += "~~";
                    line_start = false;
                    break;
                }
                case md_inline_kind::code: {
                    const string fence = inline_code_fence(node.text);
                    out += fence;
                    const bool padded = !node.text.empty() && (node.text[0] == '`' || node.text.back() == '`');
                    if (padded) {
                        out += ' ';
                    }
                    out += node.text;
                    if (padded) {
                        out += ' ';
                    }
                    out += fence;
                    line_start = false;
                    break;
                }
                case md_inline_kind::link: {
                    out += '[';
                    append_inlines_text(node.children, out, false);
                    out += "](";
                    out += node.destination;
                    if (!node.title.empty()) {
                        out += " \"";
                        out += node.title;
                        out += '"';
                    }
                    out += ')';
                    line_start = false;
                    break;
                }
                case md_inline_kind::image: {
                    out += "![";
                    append_escaped(node.text, out, false);
                    out += "](";
                    out += node.destination;
                    if (!node.title.empty()) {
                        out += " \"";
                        out += node.title;
                        out += '"';
                    }
                    out += ')';
                    line_start = false;
                    break;
                }
                case md_inline_kind::autolink: {
                    out += '<';
                    out += node.text;
                    out += '>';
                    line_start = false;
                    break;
                }
                case md_inline_kind::html: {
                    out += node.text;
                    line_start = false;
                    break;
                }
                case md_inline_kind::line_break: {
                    out += "\\\n";
                    line_start = true;
                    break;
                }
            }
        }
    }

    void append_block(const md_block& block, const markdown_builder::option& opt, string& out, int indent);

    void append_table(const md_block& block, const markdown_builder::option& opt, string& out) {
        ignore = opt;
        out += '|';
        for (const auto& cell: block.header) {
            out += ' ';
            append_inlines_text(cell, out, false);
            out += " |";
        }
        out += '\n';

        out += '|';
        for (const auto& align: block.alignments) {
            switch (align) {
                case md_align::left: {
                    out += " :--- |";
                    break;
                }
                case md_align::center: {
                    out += " :---: |";
                    break;
                }
                case md_align::right: {
                    out += " ---: |";
                    break;
                }
                default: {
                    out += " --- |";
                    break;
                }
            }
        }
        out += '\n';

        for (const auto& row: block.rows) {
            out += '|';
            for (size_t i = 0; i < block.alignments.size(); ++i) {
                out += ' ';
                if (i < row.size()) {
                    append_inlines_text(row[i], out, false);
                }
                out += " |";
            }
            out += '\n';
        }
    }

    string list_marker_text(const md_block& item, const md_block& list, const markdown_builder::option& opt) {
        string marker;
        if (item.ordered) {
            marker = to_string(static_cast<int64_t>(item.number));
            marker += list.delimiter;
            marker += ' ';
        } else {
            marker = opt.bullet.empty() ? (string(1, list.bullet) + " ") : opt.bullet;
        }
        if (item.task) {
            marker += item.checked ? "[x] " : "[ ] ";
        }
        return marker;
    }

    void append_list_item(const md_block& item, const md_block& list, const markdown_builder::option& opt, string& out,
                          const int indent) {
        const string pad = string::repeat(' ', indent);
        const string marker = list_marker_text(item, list, opt);
        const int marker_width = static_cast<int>(marker.size());
        const int content_indent = indent + marker_width;

        if (item.children.empty()) {
            out += pad;
            out += marker;
            out += '\n';
            return;
        }

        bool first = true;
        for (const auto& child: item.children) {
            if (first) {
                first = false;
                if (child.kind == md_block_kind::paragraph) {
                    out += pad;
                    out += marker;
                    append_inlines_text(child.inlines, out, true);
                    out += '\n';
                    continue;
                }
                // A non-paragraph first block starts on the line after the marker.
                out += pad;
                out += marker;
                out += '\n';
                append_block(child, opt, out, content_indent);
                continue;
            }

            if (!list.tight) {
                out += '\n';
            }
            if (child.kind == md_block_kind::list) {
                // Nested lists use the configured indent unit so that the option is honoured,
                // but never less than the content column of the parent item.
                append_block(child, opt, out, indent + max(opt.list_indent, marker_width));
                continue;
            }
            append_block(child, opt, out, content_indent);
        }
    }

    void append_list(const md_block& block, const markdown_builder::option& opt, string& out, const int indent) {
        for (size_t i = 0; i < block.children.size(); ++i) {
            if (!block.tight && i > 0) {
                out += '\n';
            }
            append_list_item(block.children[i], block, opt, out, indent);
        }
    }

    void append_quoted(const md_block& block, const markdown_builder::option& opt, string& out, const int indent) {
        string inner;
        for (size_t i = 0; i < block.children.size(); ++i) {
            if (i > 0) {
                inner += '\n';
            }
            append_block(block.children[i], opt, inner, 0);
        }

        const string pad = string::repeat(' ', indent);
        size_t start = 0;
        for (size_t i = 0; i <= inner.size(); ++i) {
            if (i == inner.size() || inner[i] == '\n') {
                if (i > start) {
                    out += pad;
                    out += "> ";
                    out += string(inner.view().substr(start, i - start));
                } else if (i == inner.size()) {
                    break;
                } else {
                    out += ">\n";
                    start = i + 1;
                    continue;
                }
                if (i < inner.size()) {
                    out += '\n';
                }
                start = i + 1;
            }
        }
    }

    void append_code_block(const md_block& block, const markdown_builder::option& opt, string& out, const int indent) {
        const string pad = string::repeat(' ', indent);
        if (block.fenced) {
            const string fence = block_fence(block.literal, block.fence_char);
            out += pad;
            out += fence;
            if (!block.info.empty()) {
                out += block.info;
            } else if (!block.language.empty()) {
                out += block.language;
            }
            out += '\n';

            size_t start = 0;
            const auto& literal = block.literal;
            for (size_t i = 0; i <= literal.size(); ++i) {
                if (i == literal.size() || literal[i] == '\n') {
                    if (i > start) {
                        out += pad;
                        out += string(literal.view().substr(start, i - start));
                    }
                    if (i < literal.size()) {
                        out += '\n';
                    }
                    start = i + 1;
                }
            }
            out += pad;
            out += fence;
            out += '\n';
            return;
        }

        const string code_pad = string::repeat(' ', indent + 4);
        size_t start = 0;
        const auto& literal = block.literal;
        for (size_t i = 0; i <= literal.size(); ++i) {
            if (i == literal.size() || literal[i] == '\n') {
                if (i > start) {
                    out += code_pad;
                    out += string(literal.view().substr(start, i - start));
                }
                if (i < literal.size()) {
                    out += '\n';
                }
                start = i + 1;
            }
        }
        ignore = opt;
    }

    void append_block(const md_block& block, const markdown_builder::option& opt, string& out, const int indent) {
        const string pad = string::repeat(' ', indent);
        switch (block.kind) {
            case md_block_kind::paragraph: {
                out += pad;
                append_inlines_text(block.inlines, out, true);
                out += '\n';
                break;
            }
            case md_block_kind::heading: {
                out += pad;
                if (block.setext) {
                    string title;
                    append_inlines_text(block.inlines, title, true);
                    out += title;
                    out += '\n';
                    const char marker = (block.level == 1) ? '=' : '-';
                    out += string::repeat(marker, max<int>(3, static_cast<int>(title.size())));
                    out += '\n';
                } else {
                    out += string::repeat('#', max(1, min(6, block.level)));
                    out += ' ';
                    append_inlines_text(block.inlines, out, false);
                    out += '\n';
                }
                break;
            }
            case md_block_kind::code_block: {
                append_code_block(block, opt, out, indent);
                break;
            }
            case md_block_kind::html_block: {
                out += pad;
                string literal = block.literal;
                if (!literal.empty() && literal.back() == '\n') {
                    literal = literal.substr(0, literal.size() - 1);
                }
                out += literal;
                out += '\n';
                break;
            }
            case md_block_kind::thematic_break: {
                out += pad;
                out += "---\n";
                break;
            }
            case md_block_kind::block_quote: {
                append_quoted(block, opt, out, indent);
                break;
            }
            case md_block_kind::list: {
                append_list(block, opt, out, indent);
                break;
            }
            case md_block_kind::list_item: {
                md_block single;
                single.kind = md_block_kind::list;
                single.tight = true;
                single.children.push_back(block);
                append_list(single, opt, out, indent);
                break;
            }
            case md_block_kind::table: {
                if (opt.tables) {
                    out += pad;
                    append_table(block, opt, out);
                } else {
                    out += pad;
                    for (const auto& cell: block.header) {
                        append_inlines_text(cell, out, false);
                        out += ' ';
                    }
                    out += '\n';
                }
                break;
            }
        }
    }
} // namespace


markdown_builder::markdown_builder(option opt) :
opt_(move(opt)) {}

markdown_builder& markdown_builder::set_options(option opt) {
    opt_ = move(opt);
    return *this;
}

markdown_builder& markdown_builder::append(const md_block& block) {
    document_.blocks.push_back(block);
    return *this;
}

markdown_builder& markdown_builder::append(const markdown_document& document) {
    for (const auto& block: document.blocks) {
        document_.blocks.push_back(block);
    }
    for (const auto& reference: document.references) {
        bool duplicate = false;
        for (const auto& existing: document_.references) {
            if (existing.label == reference.label) {
                duplicate = true;
                break;
            }
        }
        if (!duplicate) {
            document_.references.push_back(reference);
        }
    }
    return *this;
}

markdown_builder& markdown_builder::append_inline(const md_inlines& inlines) {
    md_block block;
    block.kind = md_block_kind::paragraph;
    block.inlines = inlines;
    document_.blocks.push_back(move(block));
    return *this;
}

markdown_builder& markdown_builder::reference(string label, string destination, string title) {
    md_reference entry;
    entry.label = move(label);
    entry.destination = move(destination);
    entry.title = move(title);
    document_.references.push_back(move(entry));
    return *this;
}

markdown_builder& markdown_builder::clear() noexcept {
    document_.blocks.clear();
    document_.references.clear();
    return *this;
}

bool markdown_builder::empty() const noexcept { return document_.blocks.empty() && document_.references.empty(); }

size_t markdown_builder::block_count() const noexcept { return document_.blocks.size(); }

markdown_document markdown_builder::document() const { return document_; }

string markdown_builder::build() const { return dump(document_, opt_); }

string markdown_builder::dump(const md_block& block, option opt) {
    string result;
    append_block(block, opt, result, 0);
    return result;
}

string markdown_builder::dump(const markdown_document& document, option opt) {
    string result;
    for (size_t i = 0; i < document.blocks.size(); ++i) {
        if (opt.blank_between_blocks && i > 0) {
            result += '\n';
        }
        append_block(document.blocks[i], opt, result, 0);
    }

    if (!document.references.empty()) {
        if (!result.empty()) {
            result += '\n';
        }
        for (const auto& reference: document.references) {
            result += '[';
            result += reference.label;
            result += "]: ";
            result += reference.destination;
            if (!reference.title.empty()) {
                result += " \"";
                result += reference.title;
                result += '"';
            }
            result += '\n';
        }
    }
    return result;
}

string markdown_builder::dump_inline(const md_inlines& inlines) {
    string result;
    append_inlines_text(inlines, result, false);
    return result;
}

NEFORCE_END_NAMESPACE__
