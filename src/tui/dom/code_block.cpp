#include <NeForce/core/string/to_string.hpp>
#include <NeForce/tui/dom/highlight.hpp>
#include <NeForce/core/utility/packages.hpp>
#include <NeForce/tui/dom/code_block.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

namespace {
    bool is_ident_start(const char c) { return is_alpha(c) || c == '_'; }

    bool is_ident_char(const char c) { return is_ident_start(c) || is_digit(c); }

    bool is_keyword(const string_view word) {
        static constexpr string_view keywords[] = {
                "alignas",  "auto",      "bool",     "break",   "case",      "catch",  "char",     "class",
                "const",    "constexpr", "continue", "default", "delete",    "do",     "double",   "else",
                "enum",     "explicit",  "extern",   "false",   "float",     "for",    "friend",   "goto",
                "if",       "inline",    "int",      "long",    "namespace", "new",    "nullptr",  "operator",
                "private",  "protected", "public",   "return",  "short",     "signed", "sizeof",   "static",
                "struct",   "switch",    "template", "this",    "throw",     "true",   "try",      "typedef",
                "typename", "union",     "unsigned", "using",   "virtual",   "void",   "volatile", "while"};

        for (const string_view kw: keywords) {
            if (word == kw) {
                return true;
            }
        }
        return false;
    }

    vector<text_span> highlight_line(const string_view line, const tui::style& base, const tui::style& string_style,
                                     const tui::style& comment_style, const tui::style& number_style,
                                     const tui::style& keyword_style) {
        vector<text_span> spans;
        const size_t len = line.size();
        size_t pos = 0;

        auto push = [&spans](const string_view text, const tui::style& st) {
            if (!text.empty()) {
                spans.push_back(text_span{string(text), st});
            }
        };

        while (pos < len) {
            const char c = line[pos];

            if ((c == '/' && pos + 1 < len && line[pos + 1] == '/') || c == '#') {
                push(line.substr(pos), comment_style);
                break;
            }

            if (c == '"' || c == '\'') {
                const char quote = c;
                size_t end = pos + 1;
                while (end < len) {
                    if (line[end] == '\\' && end + 1 < len) {
                        end += 2;
                        continue;
                    }
                    if (line[end] == quote) {
                        ++end;
                        break;
                    }
                    ++end;
                }
                push(line.substr(pos, end - pos), string_style);
                pos = end;
                continue;
            }

            if (is_digit(c)) {
                size_t end = pos;
                while (end < len &&
                       (is_digit(line[end]) || line[end] == '.' || line[end] == 'x' || line[end] == 'X' ||
                        (line[end] >= 'a' && line[end] <= 'f') || (line[end] >= 'A' && line[end] <= 'F'))) {
                    ++end;
                }
                push(line.substr(pos, end - pos), number_style);
                pos = end;
                continue;
            }

            if (is_ident_start(c)) {
                size_t end = pos;
                while (end < len && is_ident_char(line[end])) {
                    ++end;
                }
                const string_view word = line.substr(pos, end - pos);
                push(word, is_keyword(word) ? keyword_style : base);
                pos = end;
                continue;
            }

            size_t end = pos;
            while (end < len && !is_ident_start(line[end]) && !is_digit(line[end]) && line[end] != '"' &&
                   line[end] != '\'' && line[end] != '/' && line[end] != '#') {
                ++end;
            }
            if (end == pos) {
                ++end;
            }
            push(line.substr(pos, end - pos), base);
            pos = end;
        }

        if (spans.empty()) {
            spans.push_back(text_span{string(" "), base});
        }
        return spans;
    }

    vector<string_view> split_lines(const string_view code) {
        vector<string_view> lines;
        size_t start = 0;
        for (size_t i = 0; i <= code.size(); ++i) {
            if (i == code.size() || code[i] == '\n') {
                lines.push_back(code.substr(start, i - start));
                start = i + 1;
            }
        }
        if (!lines.empty() && lines.back().empty() && !code.empty() && code.back() == '\n') {
            lines.pop_back();
        }
        return lines;
    }

    tui::style class_style(const syntax_class cls, const tui::style& base, const code_block_option& opt) {
        tui::style st = base;
        const syntax_palette& palette = opt.syntax;
        switch (cls) {
            case syntax_class::keyword:
                st.fg = palette.keyword.value_or(_NEFORCE color::cyan());
                break;
            case syntax_class::string:
                st.fg = palette.string_.value_or(_NEFORCE color::green());
                break;
            case syntax_class::comment:
                if (palette.comment.has_value()) {
                    st.fg = palette.comment;
                } else {
                    st.dim = true;
                }
                break;
            case syntax_class::number:
                st.fg = palette.number.value_or(_NEFORCE color::yellow());
                break;
            case syntax_class::function:
                st.fg = palette.function.value_or(_NEFORCE color::cyan());
                break;
            case syntax_class::type:
                st.fg = palette.type.value_or(_NEFORCE color::magenta());
                break;
            case syntax_class::variable:
                if (palette.variable.has_value()) {
                    st.fg = palette.variable;
                }
                break;
            case syntax_class::op:
                if (palette.op.has_value()) {
                    st.fg = palette.op;
                }
                break;
            case syntax_class::punctuation:
                if (palette.punctuation.has_value()) {
                    st.fg = palette.punctuation;
                } else {
                    st.dim = true;
                }
                break;
            case syntax_class::constant:
                st.fg = palette.constant.value_or(_NEFORCE color::magenta());
                break;
            case syntax_class::plain:
            default:
                break;
        }
        return st;
    }

    // Styles for the ten token classes, built once per code block instead of once per run.
    struct syntax_styles {
        tui::style plain;
        tui::style values[10];

        syntax_styles(const tui::style& base, const code_block_option& opt) :
        plain(base) {
            for (size_t i = 0; i < 10; ++i) {
                values[i] = class_style(static_cast<syntax_class>(i), base, opt);
            }
        }

        NEFORCE_NODISCARD const tui::style& of(const syntax_class cls) const noexcept {
            const auto index = static_cast<size_t>(cls);
            return index < 10 ? values[index] : plain;
        }
    };

    vector<text_span> runs_to_spans(const string_view line, const vector<syntax_run>& runs,
                                    const syntax_styles& styles) {
        vector<text_span> spans;
        spans.reserve(runs.size() * 2 + 1);
        size_t cursor = 0;
        for (const auto& run: runs) {
            if (run.begin > cursor) {
                spans.push_back(text_span{string{line.substr(cursor, run.begin - cursor)}, styles.plain, string()});
            }
            if (run.end > run.begin) {
                spans.push_back(
                        text_span{string{line.substr(run.begin, run.end - run.begin)}, styles.of(run.cls), string()});
            }
            cursor = max(cursor, run.end);
        }
        if (cursor < line.size()) {
            spans.push_back(text_span{string{line.substr(cursor, line.size() - cursor)}, styles.plain, string()});
        }
        return spans;
    }
} // namespace


element code_block(const code_block_option& opt) {
    const tui::style base_style = opt.style;
    tui::style gutter = opt.gutter_style;
    if (!gutter.dim.has_value()) {
        gutter.dim = true;
    }

    tui::style string_style = base_style;
    string_style.fg = _NEFORCE color::green();
    tui::style comment_style = base_style;
    comment_style.dim = true;
    tui::style number_style = base_style;
    number_style.fg = _NEFORCE color::yellow();
    tui::style keyword_style = base_style;
    keyword_style.fg = _NEFORCE color::cyan();

    const highlighter* grammar = opt.highlight ? find_highlighter(opt.language.view()) : nullptr;
    // Ten class styles are worth building only when a grammar will actually be used.
    unique_ptr<syntax_styles> syntax;
    if (grammar != nullptr) {
        syntax = make_unique<syntax_styles>(base_style, opt);
    }
    highlight_state lexer_state;

    const auto lines = split_lines(opt.code.view());
    int gutter_width = 1;
    {
        const int last = max(1, opt.start_line) + static_cast<int>(lines.size()) - 1;
        gutter_width = static_cast<int>(to_string(last).size());
    }

    elements rows;
    for (size_t i = 0; i < lines.size(); ++i) {
        // The gutter is the first span of the line rather than a sibling element:
        // one rich text node per line instead of a text node, a rich text node and an hbox.
        vector<text_span> spans;
        if (opt.show_gutter) {
            const string number = to_string(max(1, opt.start_line) + static_cast<int>(i));
            string padded = number;
            while (compare_less(padded.size(), gutter_width)) {
                padded = " " + padded;
            }
            padded += " ";
            spans.push_back(text_span{move(padded), gutter, string()});
        }

        if (grammar != nullptr) {
            vector<syntax_run> runs;
            grammar->highlight_line(lines[i], lexer_state, runs);
            vector<text_span> code_spans = runs_to_spans(lines[i], runs, *syntax);
            for (auto& span: code_spans) {
                spans.push_back(move(span));
            }
        } else if (opt.highlight) {
            vector<text_span> code_spans =
                    highlight_line(lines[i], base_style, string_style, comment_style, number_style, keyword_style);
            for (auto& span: code_spans) {
                spans.push_back(move(span));
            }
        } else {
            spans.push_back(text_span{string(lines[i]), base_style, string()});
        }
        rows.push_back(element::rich_text(move(spans), base_style, style::wrap_mode::none));
    }

    if (rows.empty()) {
        rows.push_back(element::text(" "));
    }

    if (opt.collapse_after > 0 && rows.size() > opt.collapse_after) {
        const size_t head = max<size_t>(1, opt.collapse_after / 2);
        const size_t tail = max<size_t>(1, opt.collapse_after - head);
        const size_t hidden = rows.size() - head - tail;

        elements collapsed;
        for (size_t i = 0; i < head && i < rows.size(); ++i) {
            collapsed.push_back(rows[i]);
        }

        tui::style hint_style = opt.gutter_style;
        if (!hint_style.dim.has_value()) {
            hint_style.dim = true;
        }
        string hint = "\xe2\x80\xa6 " + to_string(static_cast<int64_t>(hidden)) + " lines hidden \xc2\xb7 expand";
        collapsed.push_back(element::text(move(hint), hint_style, style::wrap_mode::none));

        for (size_t i = rows.size() - tail; i < rows.size(); ++i) {
            collapsed.push_back(rows[i]);
        }
        rows = move(collapsed);
    }

    element body = element::vbox(move(rows));
    if (opt.width > 0) {
        tui::style width_style;
        width_style.width = style::size_hint{style::size_hint::fixed, opt.width};
        body = body.with_style(width_style);
    }

    if (opt.left_bar) {
        const tui::style bar_style = opt.gutter_style;
        // Built explicitly instead of from an initializer list: a list would copy each
        // element (and deep-copy its node) before moving it into the row.
        elements bar;
        bar.reserve(3);
        bar.push_back(element::text("\xe2\x94\x82", bar_style, style::wrap_mode::none));
        bar.push_back(element::text(" ", bar_style, style::wrap_mode::none));
        bar.push_back(move(body));
        body = element::hbox(move(bar));
    }

    if (opt.background) {
        tui::style band_style = opt.band_style;
        if (!band_style.bg.has_value()) {
            band_style.bg = base_style.bg.has_value() ? *base_style.bg : _NEFORCE color{26, 26, 30};
        }
        using padding = struct style::padding;
        if (!band_style.padding.has_value()) {
            band_style.padding = padding{0, 1, 0, 1};
        }
        body = element::band(move(body), band_style);
    }

    return body;
}

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
