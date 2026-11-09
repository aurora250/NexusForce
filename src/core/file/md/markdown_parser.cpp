#include <NeForce/core/file/md/markdown_parser.hpp>
#include <NeForce/core/string/char_types.hpp>
#include <NeForce/core/string/codepoint.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    bool is_blank(const string_view line) noexcept { return line.trim_left_if(_NEFORCE is_blank<char>).empty(); }

    int leading_spaces(const string_view line) noexcept {
        int count = 0;
        for (size_t i = 0; i < line.size(); ++i) {
            if (line[i] == ' ') {
                ++count;
            } else if (line[i] == '\t') {
                count += 4 - (count % 4);
            } else {
                break;
            }
        }
        return count;
    }

    char lower_of(const char c) noexcept { return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c; }

    bool starts_with_ci(const string_view text, const string_view prefix) noexcept {
        if (text.size() < prefix.size()) {
            return false;
        }
        for (size_t i = 0; i < prefix.size(); ++i) {
            if (lower_of(text[i]) != lower_of(prefix[i])) {
                return false;
            }
        }
        return true;
    }

    using line_views = md_list<string_view>;

    string_view fence_of(string_view line) noexcept;

    struct line_table {
        line_views lines;
        vector<string> owned;
        md_list<size_t> reference_lines;
    };

    string expand_tabs(const string_view line) {
        string expanded;
        expanded.reserve(line.size());
        int column = 0;
        size_t start = 0;
        while (start < line.size()) {
            const size_t tab = line.find('\t', start);
            const size_t stop = (tab == string_view::npos) ? line.size() : tab;
            expanded.append(line.substr(start, stop - start));
            column += static_cast<int>(stop - start);
            if (tab == string_view::npos) {
                break;
            }
            const int spaces = 4 - (column % 4);
            expanded.append(static_cast<size_t>(spaces), ' ');
            column += spaces;
            start = tab + 1;
        }
        return expanded;
    }

    string_view arena_copy(const string_view text) {
        if (text.empty()) {
            return string_view();
        }
        bump_arena* arena = inner::md_arena_slot();
        if (arena == nullptr) {
            arena = &inner::md_fallback_arena();
        }
        auto* data = static_cast<char*>(arena->allocate(text.size(), 1, 1));
        memory_copy(data, text.data(), text.size());
        return string_view(data, text.size());
    }

    bump_arena& block_arena() {
        bump_arena* arena = inner::md_arena_slot();
        return arena != nullptr ? *arena : inner::md_fallback_arena();
    }

    md_link_data* make_link_data() {
        bump_arena& arena = block_arena();
        auto* link = static_cast<md_link_data*>(arena.allocate(1, sizeof(md_link_data), alignof(md_link_data)));
        link->destination = string_view();
        link->title = string_view();
        link->reference = string_view();
        link->children = md_list<md_inline>();
        return link;
    }

    line_table split_lines(const string_view text) {
        line_table table;
        if (text.empty()) {
            return table;
        }

        const bool has_tab = text.contains('\t');
        table.lines.reserve(text.size() / 16 + 1);
        if (has_tab) {
            table.owned.reserve(table.lines.capacity());
        }

        bool in_fence = false;
        string_view fence;
        const auto* cursor = reinterpret_cast<const byte_t*>(text.data());
        const auto* const end = cursor + text.size();
        while (cursor < end) {
            const auto* newline = static_cast<const byte_t*>(
                    memory_find(cursor, static_cast<byte_t>('\n'), static_cast<size_t>(end - cursor)));
            const auto* stop = (newline != nullptr) ? newline : end;
            if (newline != nullptr && stop > cursor && stop[-1] == '\r') {
                --stop;
            }

            const string_view line(reinterpret_cast<const char*>(cursor), static_cast<size_t>(stop - cursor));
            string_view stored = line;
            if (has_tab) {
                table.owned.push_back(expand_tabs(line));
                stored = table.owned.back().view();
            }
            table.lines.push_back(stored);

            const char* first = stored.data();
            const char* const line_end = first + stored.size();
            while (first < line_end && (*first == ' ' || *first == '\t')) {
                ++first;
            }
            const char first_char = first < line_end ? *first : '\0';
            if (first_char == '`' || first_char == '~') {
                const string_view marker = fence_of(stored);
                if (in_fence) {
                    if (!marker.empty() && marker.size() >= fence.size() && marker[0] == fence[0] &&
                        stored.trim_left().substr(marker.size()).trim().empty()) {
                        in_fence = false;
                    }
                } else if (marker.size() >= 3) {
                    in_fence = true;
                    fence = marker;
                }
            } else if (!in_fence && first_char == '[' && first - stored.data() <= 3) {
                table.reference_lines.push_back(table.lines.size() - 1);
            }

            if (newline == nullptr) {
                break;
            }
            cursor = newline + 1;
        }
        return table;
    }

    constexpr string_view block_tags[] = {
            "address",  "article",    "aside",  "base",    "basefont", "blockquote", "body",     "caption",  "center",
            "col",      "colgroup",   "dd",     "details", "dialog",   "dir",        "div",      "dl",       "dt",
            "fieldset", "figcaption", "figure", "footer",  "form",     "frame",      "frameset", "h1",       "h2",
            "h3",       "h4",         "h5",     "h6",      "head",     "header",     "hr",       "html",     "iframe",
            "legend",   "li",         "link",   "main",    "menu",     "menuitem",   "nav",      "noframes", "ol",
            "optgroup", "option",     "p",      "param",   "search",   "section",    "summary",  "table",    "tbody",
            "td",       "tfoot",      "th",     "thead",   "title",    "tr",         "track",    "ul"};

    bool is_block_tag(const string_view name) noexcept {
        for (const string_view tag: block_tags) {
            if (name.size() == tag.size() && starts_with_ci(name, tag)) {
                return true;
            }
        }
        return false;
    }

    string_view tag_name(const string_view text, bool& closing) noexcept {
        closing = false;
        size_t pos = 1;
        if (pos < text.size() && text[pos] == '/') {
            closing = true;
            ++pos;
        }
        const size_t begin = pos;
        while (pos < text.size() && (is_alpha_or_digit(text[pos]) || text[pos] == '-')) {
            ++pos;
        }
        return text.substr(begin, pos - begin);
    }

    bool is_thematic_break(const string_view line) noexcept {
        const char* cursor = line.data();
        const char* const end = cursor + line.size();
        while (cursor < end && (*cursor == ' ' || *cursor == '\t')) {
            ++cursor;
        }
        if (end - cursor < 3) {
            return false;
        }
        const char marker = *cursor;
        if (marker != '-' && marker != '*' && marker != '_') {
            return false;
        }
        const char* last = end - 1;
        while (last > cursor && (*last == ' ' || *last == '\t')) {
            --last;
        }
        for (const char* position = cursor; position <= last; ++position) {
            if (*position != marker && *position != ' ' && *position != '\t') {
                return false;
            }
        }
        return true;
    }

    int heading_level(const string_view line) noexcept {
        const char* cursor = line.data();
        const char* const end = cursor + line.size();
        while (cursor < end && (*cursor == ' ' || *cursor == '\t')) {
            ++cursor;
        }
        int level = 0;
        while (level < 7 && cursor + level < end && cursor[level] == '#') {
            ++level;
        }
        if (level == 0 || level > 6) {
            return 0;
        }
        if (cursor + level < end && cursor[level] != ' ') {
            return 0;
        }
        return level;
    }

    int setext_level(const string_view line) noexcept {
        const auto trimmed = line.trim();
        if (trimmed.empty()) {
            return 0;
        }
        const char marker = trimmed[0];
        if (marker != '=' && marker != '-') {
            return 0;
        }
        for (size_t i = 0; i < trimmed.size(); ++i) {
            if (trimmed[i] != marker) {
                return 0;
            }
        }
        return marker == '=' ? 1 : 2;
    }

    string_view fence_of(const string_view line) noexcept {
        const char* cursor = line.data();
        const char* const end = cursor + line.size();
        while (cursor < end && (*cursor == ' ' || *cursor == '\t')) {
            ++cursor;
        }
        if (static_cast<size_t>(end - cursor) < 3 || (*cursor != '`' && *cursor != '~')) {
            return {};
        }
        const char marker = *cursor;
        const char* start = cursor;
        while (cursor < end && *cursor == marker) {
            ++cursor;
        }
        const auto run = static_cast<size_t>(cursor - start);
        return run >= 3 ? string_view(start, run) : string_view();
    }

    int html_block_start(const string_view line) noexcept {
        const auto trimmed = line.trim_left();
        if (trimmed.empty() || trimmed[0] != '<') {
            return 0;
        }
        if (starts_with_ci(trimmed, "<!--")) {
            return 2;
        }
        if (trimmed.starts_with("<?")) {
            return 3;
        }
        if (starts_with_ci(trimmed, "<![CDATA[")) {
            return 5;
        }
        if (trimmed.starts_with("<!")) {
            return 4;
        }
        for (const string_view tag: {"script", "pre", "style", "textarea"}) {
            if (trimmed.size() > tag.size() + 1 && starts_with_ci(trimmed.substr(1), tag)) {
                const char next = trimmed[tag.size() + 1];
                if (next == ' ' || next == '>' || next == '\t') {
                    return 1;
                }
            }
        }

        bool closing = false;
        const auto name = tag_name(trimmed, closing);
        if (name.empty()) {
            return 0;
        }
        if (is_block_tag(name)) {
            const size_t after = closing ? name.size() + 2 : name.size() + 1;
            if (after >= trimmed.size() || trimmed[after] == '>' || trimmed[after] == ' ' || trimmed[after] == '\t') {
                return 6;
            }
            return 0;
        }
        if (!closing && !is_alpha_or_digit(trimmed[1])) {
            return 0;
        }
        const size_t close = trimmed.find('>');
        if (close == string_view::npos) {
            return 0;
        }
        return trimmed.substr(close + 1).trim().empty() ? 7 : 0;
    }

    bool html_block_end(const int type, const string_view line) noexcept {
        switch (type) {
            case 1: {
                const auto trimmed = line.trim_left();
                return starts_with_ci(trimmed, "</script") || starts_with_ci(trimmed, "</pre") ||
                       starts_with_ci(trimmed, "</style") || starts_with_ci(trimmed, "</textarea");
            }
            case 2: {
                const auto trimmed = line.trim_right();
                return trimmed.size() >= 2 && trimmed.substr(trimmed.size() - 2) == "-->"_sv;
            }
            case 3: {
                const auto trimmed = line.trim_right();
                return trimmed.size() >= 2 && trimmed.substr(trimmed.size() - 2) == "?>"_sv;
            }
            case 4: {
                return line.contains('>');
            }
            case 5: {
                const auto trimmed = line.trim_right();
                return trimmed.size() >= 3 && trimmed.substr(trimmed.size() - 3) == "]]>"_sv;
            }
            default: {
                return false;
            }
        }
    }

    struct list_marker {
        bool valid = false;
        int indent = 0;
        bool ordered = false;
        int number = 1;
        char delimiter = '.';
        char bullet = '-';
        size_t content_offset = 0;
        bool task = false;
        bool checked = false;
    };

    list_marker scan_list_marker(const string_view line, const bool allow_task) noexcept {
        list_marker marker;
        const int indent = leading_spaces(line);
        const auto body = line.substr(static_cast<size_t>(min(indent, static_cast<int>(line.size()))));
        if (body.empty()) {
            return marker;
        }

        size_t marker_width = 0;
        if (body[0] == '-' || body[0] == '*' || body[0] == '+') {
            if (body.size() > 1 && body[1] != ' ' && body[1] != '\t') {
                return marker;
            }
            marker.ordered = false;
            marker.bullet = body[0];
            marker_width = 1;
        } else {
            size_t digits = 0;
            while (digits < body.size() && is_digit(body[digits])) {
                ++digits;
            }
            if (digits == 0 || digits > 9 || digits >= body.size()) {
                return marker;
            }
            if (body[digits] != '.' && body[digits] != ')') {
                return marker;
            }
            if (digits + 1 < body.size() && body[digits + 1] != ' ' && body[digits + 1] != '\t') {
                return marker;
            }
            int number = 0;
            for (size_t i = 0; i < digits; ++i) {
                number = number * 10 + (body[i] - '0');
            }
            marker.ordered = true;
            marker.number = number;
            marker.delimiter = body[digits];
            marker_width = digits + 1;
        }

        size_t spaces = 0;
        while (marker_width + spaces < body.size() && body[marker_width + spaces] == ' ') {
            ++spaces;
        }
        size_t content = marker_width + spaces;
        if (spaces == 0) {
            content = marker_width;
        } else if (spaces > 4) {
            content = marker_width + 1;
        }

        marker.valid = true;
        marker.indent = indent;
        marker.content_offset = static_cast<size_t>(indent) + content;

        if (allow_task && marker.content_offset + 2 < line.size() && line[marker.content_offset] == '[' &&
            line[marker.content_offset + 2] == ']') {
            const char state = line[marker.content_offset + 1];
            if (state == ' ' || state == 'x' || state == 'X') {
                marker.task = true;
                marker.checked = (state == 'x' || state == 'X');
                size_t next = marker.content_offset + 3;
                while (next < line.size() && line[next] == ' ') {
                    ++next;
                }
                marker.content_offset = next;
            }
        }

        return marker;
    }

    bool is_delimiter_cell(const string_view cell) noexcept {
        const auto trimmed = cell.trim();
        if (trimmed.empty()) {
            return false;
        }
        size_t pos = 0;
        if (trimmed[pos] == ':') {
            ++pos;
        }
        size_t dashes = 0;
        while (pos < trimmed.size() && trimmed[pos] == '-') {
            ++dashes;
            ++pos;
        }
        if (dashes == 0) {
            return false;
        }
        if (pos < trimmed.size() && trimmed[pos] == ':') {
            ++pos;
        }
        return pos == trimmed.size();
    }

    md_align alignment_of(const string_view cell) noexcept {
        const auto trimmed = cell.trim();
        if (trimmed.empty()) {
            return md_align::none;
        }
        const bool left = trimmed[0] == ':';
        const bool right = trimmed[trimmed.size() - 1] == ':';
        if (left && right) {
            return md_align::center;
        }
        if (left) {
            return md_align::left;
        }
        if (right) {
            return md_align::right;
        }
        return md_align::none;
    }

    vector<string_view> split_pipe_row(const string_view line) {
        vector<string_view> cells;
        string_view trimmed = line.trim();
        if (!trimmed.empty() && trimmed[0] == '|') {
            trimmed = trimmed.substr(1);
        }
        if (!trimmed.empty() && trimmed[trimmed.size() - 1] == '|' &&
            (trimmed.size() < 2 || trimmed[trimmed.size() - 2] != '\\')) {
            trimmed = trimmed.substr(0, trimmed.size() - 1);
        }
        size_t start = 0;
        for (size_t i = 0; i <= trimmed.size(); ++i) {
            if (i == trimmed.size() || (trimmed[i] == '|' && (i == 0 || trimmed[i - 1] != '\\'))) {
                cells.push_back(trimmed.substr(start, i - start).trim());
                start = i + 1;
            }
        }
        return cells;
    }

    bool is_table_start(const line_views& lines, const size_t index) {
        if (index + 1 >= lines.size()) {
            return false;
        }
        const auto delimiter_line = lines[index + 1];
        const char* cursor = delimiter_line.data();
        const char* const end = cursor + delimiter_line.size();
        while (cursor < end && (*cursor == ' ' || *cursor == '\t')) {
            ++cursor;
        }
        if (cursor >= end || (*cursor != '|' && *cursor != ':' && *cursor != '-')) {
            return false;
        }
        const auto header = lines[index];
        if (!header.contains('|')) {
            return false;
        }
        const auto delimiter_cells = split_pipe_row(delimiter_line);
        if (delimiter_cells.empty()) {
            return false;
        }
        for (const auto& cell: delimiter_cells) {
            if (!is_delimiter_cell(cell)) {
                return false;
            }
        }
        return split_pipe_row(header).size() == delimiter_cells.size();
    }

    constexpr uint64_t inline_special_mask[] = {
            0x1000044200000400ULL, // 0..63   : \n(10) !(33) &(38) *(42) <(60)
            0x4000000198000000ULL, // 64..127 : [(91) \(92) _(95) `(96) ~(126)
            0,                     // 128..191
            0                      // 192..255
    };

    bool is_inline_special(const char chr) noexcept {
        const auto c = static_cast<byte_t>(chr);
        return ((inline_special_mask[c >> 6] >> (c & 63)) & 1U) != 0U;
    }

    struct inline_context {
        const markdown_parser::option* opt = nullptr;
        const markdown_document* document = nullptr;
    };

    md_inlines parse_inline_range(string_view text, size_t begin, size_t end, const inline_context& ctx, int depth);

    void push_text(md_inlines& out, string& buffer) {
        if (buffer.empty()) {
            return;
        }
        if (!out.empty() && out.back().kind == md_inline_kind::text) {
            const string_view previous = out.back().text;
            bump_arena* arena = inner::md_arena_slot();
            if (arena == nullptr) {
                arena = &inner::md_fallback_arena();
            }
            auto* data = static_cast<char*>(arena->allocate(previous.size() + buffer.size(), 1, 1));
            if (!previous.empty()) {
                memory_copy(data, previous.data(), previous.size());
            }
            if (!buffer.empty()) {
                memory_copy(data + previous.size(), buffer.data(), buffer.size());
            }
            out.back().text = string_view(data, previous.size() + buffer.size());
            buffer.clear();
            return;
        }
        md_inline node;
        node.kind = md_inline_kind::text;
        node.text = arena_copy(buffer.view());
        buffer.clear();
        out.push_back(move(node));
    }

    void push_text_view(md_inlines& out, const string_view view) {
        if (view.empty()) {
            return;
        }
        if (!out.empty() && out.back().kind == md_inline_kind::text) {
            const string_view previous = out.back().text;
            bump_arena* arena = inner::md_arena_slot();
            if (arena == nullptr) {
                arena = &inner::md_fallback_arena();
            }
            auto* data = static_cast<char*>(arena->allocate(previous.size() + view.size(), 1, 1));
            if (!previous.empty()) {
                memory_copy(data, previous.data(), previous.size());
            }
            memory_copy(data + previous.size(), view.data(), view.size());
            out.back().text = string_view(data, previous.size() + view.size());
            return;
        }
        md_inline node;
        node.kind = md_inline_kind::text;
        node.text = view;
        out.push_back(node);
    }

    size_t find_unescaped(const string_view text, const size_t from, const size_t end,
                          const string_view needle) noexcept {
        if (needle.empty()) {
            return string_view::npos;
        }
        const size_t width = needle.size();
        size_t pos = from;
        while (pos + width <= end) {
            const auto* hit =
                    static_cast<const char*>(memory_find(text.data() + pos, static_cast<byte_t>(needle[0]), end - pos));
            if (hit == nullptr) {
                return string_view::npos;
            }
            pos = static_cast<size_t>(hit - text.data());
            if (pos + width > end) {
                return string_view::npos;
            }
            if (pos > from && text[pos - 1] == '\\') {
                size_t backslashes = 0;
                for (size_t i = pos; i > from && text[i - 1] == '\\'; --i) {
                    ++backslashes;
                }
                if ((backslashes & 1U) != 0U) {
                    ++pos;
                    continue;
                }
            }
            if (text.substr(pos, width) == needle) {
                return pos;
            }
            ++pos;
        }
        return string_view::npos;
    }

    size_t match_bracket(const string_view text, const size_t open, const size_t end) noexcept {
        if (open + 1 < end) {
            const auto* close = static_cast<const char*>(
                    memory_find(text.data() + open + 1, static_cast<byte_t>(']'), end - open - 1));
            if (close != nullptr) {
                const auto candidate = static_cast<size_t>(close - text.data());
                size_t backslashes = 0;
                for (size_t i = candidate; i > open && text[i - 1] == '\\'; --i) {
                    ++backslashes;
                }
                if ((backslashes & 1U) == 0U) {
                    const auto* nested = static_cast<const char*>(
                            memory_find(text.data() + open + 1, static_cast<byte_t>('['), candidate - open - 1));
                    const auto* tick = static_cast<const char*>(
                            memory_find(text.data() + open + 1, static_cast<byte_t>('`'), candidate - open - 1));
                    if (nested == nullptr && tick == nullptr) {
                        return candidate;
                    }
                }
            }
        }
        int depth = 0;
        for (size_t pos = open; pos < end; ++pos) {
            const char c = text[pos];
            if (c == '\\') {
                ++pos;
                continue;
            }
            if (c == '`') {
                size_t run = 0;
                while (pos + run < end && text[pos + run] == '`') {
                    ++run;
                }
                const auto closer = find_unescaped(text, pos + run, end, text.substr(pos, run));
                if (closer != string_view::npos) {
                    pos = closer + run - 1;
                } else {
                    pos += run - 1;
                }
                continue;
            }
            if (c == '[') {
                ++depth;
            } else if (c == ']') {
                --depth;
                if (depth == 0) {
                    return pos;
                }
            }
        }
        return string_view::npos;
    }

    bool decode_entity(const string_view text, const size_t pos, size_t& consumed, string& out) {
        if (text[pos] != '&') {
            return false;
        }
        size_t cursor = pos + 1;
        if (cursor >= text.size()) {
            return false;
        }
        if (text[cursor] == '#') {
            ++cursor;
            bool hex = false;
            if (cursor < text.size() && (text[cursor] == 'x' || text[cursor] == 'X')) {
                hex = true;
                ++cursor;
            }
            const size_t digits_begin = cursor;
            uint32_t value = 0;
            while (cursor < text.size() && (hex ? is_xdigit(text[cursor]) : is_digit(text[cursor]))) {
                const char c = text[cursor];
                const uint32_t digit =
                        is_digit(c) ? static_cast<uint32_t>(c - '0') : static_cast<uint32_t>(lower_of(c) - 'a' + 10);
                value = value * (hex ? 16U : 10U) + digit;
                ++cursor;
            }
            if (cursor == digits_begin || cursor >= text.size() || text[cursor] != ';') {
                return false;
            }
            ++cursor;
            codepoint(value).append_to(out);
            consumed = cursor - pos;
            return true;
        }

        const size_t name_begin = cursor;
        while (cursor < text.size() && is_alpha_or_digit(text[cursor])) {
            ++cursor;
        }
        if (cursor == name_begin || cursor >= text.size() || text[cursor] != ';') {
            return false;
        }

        const string_view name = text.substr(name_begin, cursor - name_begin);

        static constexpr pair<string_view, string_view> entities[] = {
                {"amp", "&"},     {"lt", "<"},       {"gt", ">"},      {"quot", "\""},   {"apos", "'"},
                {"nbsp", " "},    {"copy", "c"},     {"reg", "R"},     {"trade", "TM"},  {"hellip", "..."},
                {"mdash", "-"},   {"ndash", "-"},    {"laquo", "<<"},  {"raquo", ">>"},  {"ldquo", "\""},
                {"rdquo", "\""},  {"lsquo", "'"},    {"rsquo", "'"},   {"times", "x"},   {"divide", "/"},
                {"deg", "deg"},   {"plusmn", "+-"},  {"middot", "."},  {"bull", "*"},    {"dagger", "+"},
                {"sect", "S"},    {"para", "P"},     {"euro", "EUR"},  {"pound", "GBP"}, {"yen", "JPY"},
                {"cent", "c"},    {"larr", "<-"},    {"rarr", "->"},   {"uarr", "^"},    {"darr", "v"},
                {"check", "v"},   {"star", "*"},     {"hearts", "<3"}, {"harr", "<->"},  {"infin", "inf"},
                {"ne", "!="},     {"le", "<="},      {"ge", ">="},     {"asymp", "~="},  {"sum", "sum"},
                {"prod", "prod"}, {"radic", "sqrt"}, {"micro", "u"},   {"alpha", "a"},   {"beta", "b"},
                {"gamma", "g"},   {"delta", "d"},    {"pi", "pi"},     {"sigma", "s"},   {"omega", "w"}};

        for (const auto& entry: entities) {
            const string_view candidate = entry.first;
            if (name.size() == candidate.size() && starts_with_ci(name, candidate)) {
                out += entry.second;
                consumed = cursor + 1 - pos;
                return true;
            }
        }
        return false;
    }

    bool is_autolink(const string_view text, const size_t pos, const size_t end, size_t& close, bool& email) {
        email = false;
        const auto inner_end = text.substr(pos + 1, end - pos - 1).find('>');
        if (inner_end == string_view::npos) {
            return false;
        }
        const size_t at = pos + 1 + inner_end;
        const string_view inner = text.substr(pos + 1, at - pos - 1);
        if (inner.empty() || inner.contains(' ') || inner.contains('<')) {
            return false;
        }
        const auto colon = inner.find(':');
        if (colon != string_view::npos && colon >= 2 && colon <= 32) {
            bool scheme_ok = true;
            for (size_t i = 0; i < colon; ++i) {
                const char c = inner[i];
                if (!is_alpha_or_digit(c) && c != '+' && c != '.' && c != '-') {
                    scheme_ok = false;
                    break;
                }
            }
            if (scheme_ok && is_alpha_or_digit(inner[0])) {
                close = at;
                return true;
            }
        }
        if (inner.contains('@') && !inner.contains('/')) {
            close = at;
            email = true;
            return true;
        }
        return false;
    }

    bool parse_inline_html(const string_view text, const size_t pos, const size_t end, size_t& consumed) {
        const auto rest = text.substr(pos, end - pos);
        if (rest.starts_with("<!--")) {
            const auto close = rest.find("-->");
            if (close == string_view::npos) {
                return false;
            }
            consumed = close + 3;
            return true;
        }
        if (rest.starts_with("<?")) {
            const auto close = rest.find("?>");
            if (close == string_view::npos) {
                return false;
            }
            consumed = close + 2;
            return true;
        }
        if (starts_with_ci(rest, "<![CDATA[")) {
            const auto close = rest.find("]]>");
            if (close == string_view::npos) {
                return false;
            }
            consumed = close + 3;
            return true;
        }
        if (rest.size() < 2) {
            return false;
        }

        size_t cursor = 1;
        if (rest[cursor] == '/') {
            ++cursor;
        }
        if (cursor >= rest.size() || !is_alpha_or_digit(rest[cursor])) {
            return false;
        }
        const size_t tag_begin = cursor;
        while (cursor < rest.size() && (is_alpha_or_digit(rest[cursor]) || rest[cursor] == '-')) {
            ++cursor;
        }
        if (cursor == tag_begin) {
            return false;
        }
        const auto close = rest.substr(cursor).find('>');
        if (close == string_view::npos) {
            return false;
        }
        const auto attributes = rest.substr(cursor, close);
        if (attributes.contains('<')) {
            return false;
        }
        consumed = cursor + close + 1;
        return true;
    }

    bool parse_link_destination(const string_view text, size_t& pos, const size_t end, string_view& destination) {
        const size_t begin = pos;
        if (pos < end && text[pos] == '<') {
            ++pos;
            while (pos < end && text[pos] != '>') {
                if (text[pos] == '\n' || text[pos] == '<') {
                    return false;
                }
                if (text[pos] == '\\' && pos + 1 < end) {
                    ++pos;
                }
                ++pos;
            }
            if (pos >= end) {
                return false;
            }
            destination = text.substr(begin + 1, pos - begin - 1);
            ++pos;
            return true;
        }
        int depth = 0;
        while (pos < end) {
            const char c = text[pos];
            if (c == '\\' && pos + 1 < end) {
                pos += 2;
                continue;
            }
            if (c == '(') {
                ++depth;
            } else if (c == ')') {
                if (depth == 0) {
                    break;
                }
                --depth;
            } else if (c == ' ' || c == '\t' || c == '\n') {
                break;
            }
            ++pos;
        }
        if (pos == begin) {
            return false;
        }
        destination = text.substr(begin, pos - begin);
        return true;
    }

    bool parse_link_title(const string_view text, size_t& pos, const size_t end, string_view& title) {
        if (pos >= end) {
            return false;
        }
        const char open = text[pos];
        if (open != '"' && open != '\'' && open != '(') {
            return false;
        }
        const char close = (open == '(') ? ')' : open;
        ++pos;
        const size_t begin = pos;
        while (pos < end && text[pos] != close) {
            if (text[pos] == '\\' && pos + 1 < end) {
                ++pos;
            }
            ++pos;
        }
        if (pos >= end) {
            return false;
        }
        title = text.substr(begin, pos - begin);
        ++pos;
        return true;
    }

    struct delimiter_run {
        size_t length = 0;
        bool can_open = false;
        bool can_close = false;
    };

    delimiter_run analyse_run(const string_view text, const size_t pos, const size_t end, const char ch) {
        delimiter_run run;
        size_t length = 0;
        while (pos + length < end && text[pos + length] == ch) {
            ++length;
        }
        run.length = length;

        const char prev = (pos > 0) ? text[pos - 1] : '\n';
        const char next = (pos + length < end) ? text[pos + length] : '\n';
        const bool prev_space = is_space(prev);
        const bool next_space = is_space(next);
        const bool prev_punct = is_punct(prev);
        const bool next_punct = is_punct(next);

        run.can_open = !next_space && (!next_punct || prev_space || prev_punct);
        run.can_close = !prev_space && (!prev_punct || next_space || next_punct);
        if (ch == '_') {
            if (is_alpha_or_digit(prev)) {
                run.can_open = false;
            }
            if (is_alpha_or_digit(next)) {
                run.can_close = false;
            }
        }
        return run;
    }

    struct parse_context {
        markdown_parser::option opt;
        markdown_document* document = nullptr;
        bool tables_present = true;
    };

    bool starts_new_block(const string_view line, const parse_context& ctx, const line_views* lines,
                          const size_t index) {
        const char* cursor = line.data();
        const char* const end = cursor + line.size();
        while (cursor < end && (*cursor == ' ' || *cursor == '\t')) {
            ++cursor;
        }
        if (cursor >= end) {
            return false;
        }
        const char first = *cursor;
        if ((first == '`' || first == '~') && fence_of(line).size() >= 3) {
            return true;
        }
        if (first == '#' && heading_level(line) > 0) {
            return true;
        }
        if ((first == '-' || first == '*' || first == '_' || first == '+') && is_thematic_break(line)) {
            return true;
        }
        if (first == '>' && line.trim_left().starts_with(">")) {
            return true;
        }
        if ((first == '-' || first == '+' || first == '*' || (first >= '0' && first <= '9')) &&
            scan_list_marker(line, ctx.opt.task_lists).valid) {
            return true;
        }
        if (first == '<' && ctx.opt.html_blocks && html_block_start(line) > 0) {
            return true;
        }
        if (lines != nullptr && ctx.opt.tables && ctx.tables_present && line.contains('|') &&
            is_table_start(*lines, index)) {
            return true;
        }
        return false;
    }

    md_blocks parse_block_lines(const line_views& lines, parse_context& ctx, int depth);

    bool parse_reference_definition(const line_views& lines, const size_t index, const int indent,
                                    md_reference* reference, size_t* consumed) {
        if (consumed != nullptr) {
            *consumed = 0;
        }
        const auto line = lines[index];
        if (indent > 3) {
            return false;
        }
        const auto body = line.substr(static_cast<size_t>(indent));
        if (body.empty() || body[0] != '[') {
            return false;
        }
        const auto close = match_bracket(body, 0, body.size());
        if (close == string_view::npos || close + 1 >= body.size() || body[close + 1] != ':') {
            return false;
        }
        const auto label = body.substr(1, close - 1).trim();
        if (label.empty() || label.size() > 999) {
            return false;
        }

        size_t used_lines = 1;
        size_t pos = close + 2;
        while (pos < body.size() && body[pos] == ' ') {
            ++pos;
        }

        string_view destination;
        if (!parse_link_destination(body, pos, body.size(), destination)) {
            if (index + 1 >= lines.size()) {
                return false;
            }
            const auto next = lines[index + 1];
            if (leading_spaces(next) > 3) {
                return false;
            }
            auto next_pos = static_cast<size_t>(leading_spaces(next));
            if (!parse_link_destination(next, next_pos, next.size(), destination)) {
                return false;
            }
            used_lines = 2;
            pos = next_pos;
            while (pos < next.size() && next[pos] == ' ') {
                ++pos;
            }
            if (pos != next.size()) {
                return false;
            }
            string_view next_title;
            if (index + 2 < lines.size()) {
                const auto third = lines[index + 2];
                if (leading_spaces(third) <= 3) {
                    auto third_pos = static_cast<size_t>(leading_spaces(third));
                    if (parse_link_title(third, third_pos, third.size(), next_title) &&
                        third.substr(third_pos).trim().empty()) {
                        used_lines = 3;
                    } else {
                        next_title = string_view();
                    }
                }
            }
            if (reference != nullptr) {
                reference->label = label;
                reference->destination = destination;
                reference->title = next_title;
            }
            if (consumed != nullptr) {
                *consumed = used_lines;
            }
            return true;
        }

        while (pos < body.size() && body[pos] == ' ') {
            ++pos;
        }

        string_view title;
        if (pos < body.size()) {
            if (!parse_link_title(body, pos, body.size(), title)) {
                return false;
            }
            while (pos < body.size() && body[pos] == ' ') {
                ++pos;
            }
            if (pos != body.size()) {
                return false;
            }
        } else if (index + 1 < lines.size()) {
            const auto next = lines[index + 1];
            if (leading_spaces(next) <= 3) {
                auto next_pos = static_cast<size_t>(leading_spaces(next));
                if (parse_link_title(next, next_pos, next.size(), title) && next.substr(next_pos).trim().empty()) {
                    used_lines = 2;
                } else {
                    title = string_view();
                }
            }
        }

        if (reference != nullptr) {
            reference->label = label;
            reference->destination = destination;
            reference->title = title;
        }
        if (consumed != nullptr) {
            *consumed = used_lines;
        }
        return true;
    }

    void collect_references(const line_views& lines, const md_list<size_t>& candidates, markdown_document& document) {
        size_t skip_until = 0;
        for (const auto index: candidates) {
            if (index < skip_until) {
                continue;
            }
            const auto line = lines[index];
            const char* first = line.data();
            const char* const end = first + line.size();
            while (first < end && (*first == ' ' || *first == '\t')) {
                ++first;
            }
            if (first >= end || *first != '[') {
                continue;
            }
            md_reference reference;
            size_t consumed = 0;
            if (!parse_reference_definition(lines, index, static_cast<int>(first - line.data()), &reference,
                                            &consumed)) {
                continue;
            }
            const string normalised = md_normalise_label(reference.label);
            bool duplicate = false;
            for (const auto& existing: document.references) {
                if (existing.normalized == normalised.view()) {
                    duplicate = true;
                    break;
                }
            }
            if (!duplicate) {
                auto* data = static_cast<char*>(document.arena().allocate(normalised.size(), 1, 1));
                memory_copy(data, normalised.data(), normalised.size());
                reference.normalized = string_view(data, normalised.size());
                document.references.push_back(reference);
            }
            skip_until = index + max<size_t>(1, consumed);
        }
    }

    md_block make_paragraph(const line_views& paragraph, const inline_context& inline_ctx) {
        md_block block;
        block.kind = md_block_kind::paragraph;
        if (paragraph.size() == 1) {
            const auto line = paragraph[0];
            block.inlines = parse_inline_range(line, 0, line.size(), inline_ctx, 0);
            return block;
        }
        size_t total = paragraph.size() - 1;
        for (const auto& line: paragraph) {
            total += line.size();
        }
        auto* data = static_cast<char*>(block_arena().allocate(total, 1, 1));
        if (data == nullptr) {
            return block;
        }
        char* out = data;
        for (size_t i = 0; i < paragraph.size(); ++i) {
            if (i > 0) {
                *out++ = '\n';
            }
            memory_copy(out, paragraph[i].data(), paragraph[i].size());
            out += paragraph[i].size();
        }
        const string_view paragraph_text(data, total);
        block.inlines = parse_inline_range(paragraph_text, 0, paragraph_text.size(), inline_ctx, 0);
        return block;
    }

    md_block parse_fenced_code(const line_views& lines, size_t& index, const string_view fence) {
        md_block block;
        block.kind = md_block_kind::code_block;
        md_code_data* code = block.make_code(block_arena());
        code->fenced = true;

        code->fence_char = fence[0];
        code->fence_length = static_cast<int>(fence.size());
        code->info = lines[index].trim_left().substr(fence.size()).trim();
        {
            const auto info = code->info;
            const auto space = info.find(' ');
            code->language = space == string_view::npos ? info : info.substr(0, space);
        }
        ++index;

        size_t content_end = index;
        size_t total = 0;
        while (content_end < lines.size()) {
            const auto line = lines[content_end];
            const auto trimmed = line.trim_left();
            if (trimmed.size() >= fence.size() && trimmed[0] == fence[0]) {
                size_t run = 0;
                while (run < trimmed.size() && trimmed[run] == fence[0]) {
                    ++run;
                }
                if (run >= fence.size() && trimmed.substr(run).trim().empty()) {
                    break;
                }
            }
            total += line.size() + 1;
            ++content_end;
        }
        if (total != 0) {
            auto* data = static_cast<char*>(block_arena().allocate(total, 1, 1));
            char* out = data;
            for (size_t i = index; i < content_end; ++i) {
                const auto line = lines[i];
                memory_copy(out, line.data(), line.size());
                out += line.size();
                *out++ = '\n';
            }
            code->literal = string_view(data, total);
        }
        index = content_end;
        if (index < lines.size()) {
            ++index;
        }
        return block;
    }

    md_block parse_html_block(const line_views& lines, size_t& index, const int type) {
        md_block block;
        block.kind = md_block_kind::html_block;
        md_code_data* code = block.make_code(block_arena());
        code->html_type = type;

        size_t html_end = index;
        size_t total = 0;
        while (html_end < lines.size()) {
            const auto line = lines[html_end];
            if (type >= 6 && is_blank(line)) {
                break;
            }
            total += line.size() + 1;
            const bool ended = html_block_end(type, line);
            ++html_end;
            if (type < 6 && ended) {
                break;
            }
        }
        if (total != 0) {
            auto* data = static_cast<char*>(block_arena().allocate(total, 1, 1));
            char* out = data;
            for (size_t i = index; i < html_end; ++i) {
                const auto line = lines[i];
                memory_copy(out, line.data(), line.size());
                out += line.size();
                *out++ = '\n';
            }
            code->literal = string_view(data, total);
        }
        index = html_end;
        return block;
    }

    md_block parse_block_quote(const line_views& lines, size_t& index, parse_context& ctx, const int depth) {
        line_views inner;
        while (index < lines.size()) {
            const auto line = lines[index];
            const auto trimmed = line.trim_left();
            if (trimmed.starts_with(">")) {
                auto rest = trimmed.substr(1);
                if (!rest.empty() && rest[0] == ' ') {
                    rest = rest.substr(1);
                }
                inner.push_back(rest);
                ++index;
                continue;
            }
            if (is_blank(line)) {
                break;
            }
            if (!inner.empty() && !starts_new_block(line, ctx, &lines, index)) {
                inner.push_back(line.trim_left());
                ++index;
                continue;
            }
            break;
        }

        md_block block;
        block.kind = md_block_kind::block_quote;
        block.children = parse_block_lines(inner, ctx, depth + 1);
        return block;
    }

    md_block parse_table(const line_views& lines, size_t& index, const parse_context& ctx) {
        const inline_context inline_ctx{&ctx.opt, ctx.document};

        md_block block;
        block.kind = md_block_kind::table;
        md_table_data* table = block.make_table(block_arena());

        const auto header_cells = split_pipe_row(lines[index]);
        const auto delimiter_cells = split_pipe_row(lines[index + 1]);
        for (const auto& cell: delimiter_cells) {
            table->alignments.push_back(alignment_of(cell));
        }
        for (const auto& cell: header_cells) {
            table->header.push_back(parse_inline_range(cell, 0, cell.size(), inline_ctx, 0));
        }
        index += 2;

        while (index < lines.size()) {
            const auto line = lines[index];
            if (is_blank(line)) {
                break;
            }
            if (!line.contains('|')) {
                break;
            }
            const auto cells = split_pipe_row(line);
            md_list<md_inlines> row;
            for (size_t i = 0; i < table->alignments.size(); ++i) {
                if (i < cells.size()) {
                    row.push_back(parse_inline_range(cells[i], 0, cells[i].size(), inline_ctx, 0));
                } else {
                    row.emplace_back();
                }
            }
            table->rows.push_back(move(row));
            ++index;
        }
        return block;
    }

    bool is_simple_paragraph_item(const line_views& item) {
        if (item.empty()) {
            return false;
        }
        for (const auto& current: item) {
            const char* cursor = current.data();
            const char* const end = cursor + current.size();
            while (cursor < end && (*cursor == ' ' || *cursor == '\t')) {
                ++cursor;
            }
            if (cursor >= end) {
                return false;
            }
            if (static_cast<size_t>(cursor - current.data()) >= 4) {
                return false;
            }
            const char first = *cursor;
            switch (first) {
                case '#':
                case '`':
                case '~':
                case '>':
                case '<':
                case '-':
                case '+':
                case '*':
                case '_':
                case '=':
                case '[':
                case '|':
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9': {
                    return false;
                }
                default: {
                    break;
                }
            }
            if (current.contains('|')) {
                return false;
            }
        }
        return true;
    }

    bool is_paragraph_line(const string_view line) {
        const char* cursor = line.data();
        const char* const end = cursor + line.size();
        while (cursor < end && (*cursor == ' ' || *cursor == '\t')) {
            ++cursor;
        }
        if (cursor >= end || static_cast<size_t>(cursor - line.data()) >= 4) {
            return false;
        }
        const char first = *cursor;
        if (first == '#' || first == '`' || first == '~' || first == '>' || first == '<' || first == '-' ||
            first == '+' || first == '*' || first == '_' || first == '=' || first == '[' || first == '|' ||
            (first >= '0' && first <= '9')) {
            return false;
        }
        return !line.contains('|');
    }

    md_block parse_list(const line_views& lines, size_t& index, parse_context& ctx, const int depth) {
        const auto first = scan_list_marker(lines[index], ctx.opt.task_lists);
        md_block list;
        list.kind = md_block_kind::list;
        list.ordered = first.ordered;
        list.start = first.ordered ? first.number : 1;
        list.delimiter = first.delimiter;
        list.bullet = first.bullet;

        const int base_indent = first.indent;
        bool loose = false;
        int counter = list.start;

        line_views item_views;

        while (index < lines.size()) {
            const auto line = lines[index];
            const auto marker = scan_list_marker(line, ctx.opt.task_lists);
            if (!marker.valid || marker.indent != base_indent) {
                break;
            }
            if (marker.ordered != list.ordered) {
                break;
            }
            if (list.ordered ? (marker.delimiter != list.delimiter) : (marker.bullet != list.bullet)) {
                break;
            }

            md_block item;
            item.kind = md_block_kind::list_item;
            item.ordered = list.ordered;
            item.number = list.ordered ? counter : 0;
            item.task = marker.task;
            item.checked = marker.checked;
            if (list.ordered) {
                ++counter;
            }

            const int content_indent = static_cast<int>(marker.content_offset);
            item_views.clear();
            item_views.push_back(line.substr(marker.content_offset));
            ++index;

            bool item_has_blank = false;
            while (index < lines.size()) {
                const auto next = lines[index];
                if (is_blank(next)) {
                    size_t peek = index + 1;
                    while (peek < lines.size() && is_blank(lines[peek])) {
                        ++peek;
                    }
                    if (peek >= lines.size()) {
                        index = peek;
                        break;
                    }
                    const auto peek_line = lines[peek];
                    const auto peek_marker = scan_list_marker(peek_line, ctx.opt.task_lists);
                    if (peek_marker.valid && peek_marker.indent == base_indent) {
                        loose = true;
                        index = peek;
                        break;
                    }
                    if (leading_spaces(peek_line) >= content_indent) {
                        item_views.emplace_back();
                        item_has_blank = true;
                        index = peek;
                        continue;
                    }
                    index = peek;
                    break;
                }

                const int indent = leading_spaces(next);
                if (indent >= content_indent) {
                    item_views.push_back(next.substr(static_cast<size_t>(content_indent)));
                    ++index;
                    continue;
                }
                if (!starts_new_block(next, ctx, &lines, index)) {
                    item_views.push_back(next);
                    ++index;
                    continue;
                }
                break;
            }

            bool parsed_fast = false;
            if (!item_has_blank && item_views.size() >= 2 && is_paragraph_line(item_views[0]) &&
                scan_list_marker(item_views[1], ctx.opt.task_lists).valid) {
                const inline_context inline_ctx{&ctx.opt, ctx.document};
                line_views first_line;
                first_line.push_back(item_views[0]);
                item.children.push_back(make_paragraph(first_line, inline_ctx));
                size_t sub_index = 1;
                md_block nested = parse_list(item_views, sub_index, ctx, depth + 1);
                if (sub_index == item_views.size()) {
                    item.children.push_back(move(nested));
                    parsed_fast = true;
                }
            }
            if (!parsed_fast) {
                if (!item_has_blank && is_simple_paragraph_item(item_views)) {
                    const inline_context inline_ctx{&ctx.opt, ctx.document};
                    item.children.push_back(make_paragraph(item_views, inline_ctx));
                } else {
                    item.children = parse_block_lines(item_views, ctx, depth + 1);
                }
            }
            if (item_has_blank) {
                loose = true;
            }
            list.children.push_back(move(item));
        }

        list.tight = !loose;
        return list;
    }

    md_blocks parse_block_lines(const line_views& lines, parse_context& ctx, const int depth) {
        md_blocks blocks;
        if (depth > 64) {
            return blocks;
        }
        const inline_context inline_ctx{&ctx.opt, ctx.document};
        if (depth == 0 && lines.size() > 2000) {
            blocks.reserve(lines.size() / 7);
        }
        size_t index = 0;

        while (index < lines.size()) {
            const auto line = lines[index];
            const char* first_cursor = line.data();
            const char* const line_end = first_cursor + line.size();
            while (first_cursor < line_end && (*first_cursor == ' ' || *first_cursor == '\t')) {
                ++first_cursor;
            }
            if (first_cursor >= line_end) {
                ++index;
                continue;
            }
            const char first = *first_cursor;
            const auto indent = static_cast<uint32_t>(first_cursor - line.data());

            if (first == '[') {
                md_reference probe;
                size_t consumed = 0;
                if (parse_reference_definition(lines, index, static_cast<int>(indent), &probe, &consumed)) {
                    index += max<size_t>(1, consumed);
                    continue;
                }
            }

            if (first == '<' && ctx.opt.html_blocks) {
                const int html_type = html_block_start(line);
                if (html_type > 0) {
                    blocks.push_back(parse_html_block(lines, index, html_type));
                    continue;
                }
            }

            if (first == '`' || first == '~') {
                const string_view fence = fence_of(line);
                if (fence.size() >= 3) {
                    blocks.push_back(parse_fenced_code(lines, index, fence));
                    continue;
                }
            }

            const int level = first == '#' ? heading_level(line) : 0;
            if (level > 0) {
                md_block block;
                block.kind = md_block_kind::heading;
                block.level = level;
                auto body = line.trim_left().substr(static_cast<size_t>(level)).trim();
                size_t tail = body.size();
                while (tail > 0 && body[tail - 1] == '#') {
                    --tail;
                }
                if (tail < body.size() && (tail == 0 || body[tail - 1] == ' ')) {
                    body = body.head(tail).trim();
                }
                block.inlines = parse_inline_range(body, 0, body.size(), inline_ctx, 0);
                blocks.push_back(move(block));
                ++index;
                continue;
            }

            if ((first == '-' || first == '*' || first == '_' || first == '+') && is_thematic_break(line)) {
                md_block block;
                block.kind = md_block_kind::thematic_break;
                blocks.push_back(move(block));
                ++index;
                continue;
            }

            if (first == '>') {
                blocks.push_back(parse_block_quote(lines, index, ctx, depth));
                continue;
            }

            if (ctx.opt.tables && ctx.tables_present && is_table_start(lines, index)) {
                blocks.push_back(parse_table(lines, index, ctx));
                continue;
            }

            if ((first == '-' || first == '+' || first == '*' || (first >= '0' && first <= '9')) &&
                scan_list_marker(line, ctx.opt.task_lists).valid) {
                blocks.push_back(parse_list(lines, index, ctx, depth));
                continue;
            }

            if (ctx.opt.indented_code && indent >= 4) {
                md_block block;
                block.kind = md_block_kind::code_block;
                size_t code_end = index;
                while (code_end < lines.size()) {
                    const auto current = lines[code_end];
                    if (!is_blank(current) && leading_spaces(current) < 4) {
                        break;
                    }
                    ++code_end;
                }
                while (code_end > index && is_blank(lines[code_end - 1])) {
                    --code_end;
                }
                size_t total = 0;
                for (size_t i = index; i < code_end; ++i) {
                    const auto current = lines[i];
                    total += is_blank(current) ? 1 : current.size() - 3;
                }
                if (total != 0) {
                    auto* data = static_cast<char*>(block_arena().allocate(total, 1, 1));
                    char* out = data;
                    for (size_t i = index; i < code_end; ++i) {
                        const auto current = lines[i];
                        if (!is_blank(current)) {
                            memory_copy(out, current.data() + 4, current.size() - 4);
                            out += current.size() - 4;
                        }
                        *out++ = '\n';
                    }
                    block.make_code(block_arena())->literal = string_view(data, total);
                }
                index = code_end;
                blocks.push_back(move(block));
                continue;
            }

            line_views paragraph;
            bool promoted = false;
            while (index < lines.size()) {
                const auto current = lines[index];
                const char* current_first = current.data();
                const char* const current_end = current_first + current.size();
                while (current_first < current_end && (*current_first == ' ' || *current_first == '\t')) {
                    ++current_first;
                }
                if (current_first >= current_end) {
                    break;
                }
                const char current_char = *current_first;
                const auto current_indent = static_cast<size_t>(current_first - current.data());
                bool candidate = current_indent >= 4 || current_char == '#' || current_char == '`' ||
                                 current_char == '~' || current_char == '>' || current_char == '<' ||
                                 current_char == '-' || current_char == '+' || current_char == '*' ||
                                 current_char == '_' || current_char == '=' || current_char == '[' ||
                                 current_char == '|' || (current_char >= '0' && current_char <= '9');
                if (!candidate && ctx.tables_present) {
                    candidate = current.contains('|');
                }
                if (ctx.opt.setext_headings && (current_char == '=' || current_char == '-')) {
                    const int setext = setext_level(current);
                    if (setext > 0 && !paragraph.empty()) {
                        md_block block;
                        block.kind = md_block_kind::heading;
                        block.level = setext;
                        block.setext = true;
                        block.inlines = make_paragraph(paragraph, inline_ctx).inlines;
                        blocks.push_back(move(block));
                        promoted = true;
                        ++index;
                        break;
                    }
                }
                if (!paragraph.empty() && candidate && starts_new_block(current, ctx, &lines, index)) {
                    break;
                }
                paragraph.push_back(current);
                ++index;
            }
            if (!promoted && !paragraph.empty()) {
                blocks.push_back(make_paragraph(paragraph, inline_ctx));
            }
        }

        return blocks;
    }

    md_inlines parse_inline_range(const string_view text, const size_t begin, const size_t end,
                                  const inline_context& ctx, const int depth) {
        if (depth > 32) {
            md_inlines deep;
            string deep_text = text.substr(begin, end - begin);
            push_text(deep, deep_text);
            return deep;
        }

        size_t plain_start = begin;
        while (plain_start < end && !is_inline_special(text[plain_start])) {
            ++plain_start;
        }
        if (plain_start == end) {
            md_inlines fast;
            if (end > begin) {
                md_inline node;
                node.text = text.substr(begin, end - begin);
                fast.push_back(node);
            }
            return fast;
        }

        md_inlines out;
        out.reserve(6);
        string buffer;
        size_t plain_begin = begin;
        size_t plain_end = plain_start;
        bool plain = plain_start > begin;
        const auto materialize = [&]() {
            if (plain) {
                buffer.append(text.substr(plain_begin, plain_end - plain_begin));
                plain = false;
            }
        };
        const auto flush = [&]() {
            if (plain) {
                push_text_view(out, text.substr(plain_begin, plain_end - plain_begin));
                plain = false;
            }
            if (!buffer.empty()) {
                push_text(out, buffer);
            }
        };
        size_t pos = plain_start;

        while (pos < end) {
            // Fast path: append the longest run of characters without inline
            // significance as a single slice instead of byte by byte.
            {
                const size_t run_begin = pos;
                while (pos < end && !is_inline_special(text[pos])) {
                    ++pos;
                }
                if (pos > run_begin) {
                    if (buffer.empty()) {
                        if (plain) {
                            plain_end = pos;
                        } else {
                            plain_begin = run_begin;
                            plain_end = pos;
                            plain = true;
                        }
                    } else {
                        buffer.append(text.substr(run_begin, pos - run_begin));
                    }
                    if (pos >= end) {
                        break;
                    }
                }
            }

            const char c = text[pos];

            if (c == '\\') {
                if (pos + 1 < end && text[pos + 1] == '\n') {
                    flush();
                    md_inline node;
                    node.kind = md_inline_kind::line_break;
                    out.push_back(move(node));
                    pos += 2;
                    continue;
                }
                if (pos + 1 < end && is_punct(text[pos + 1])) {
                    materialize();
                    buffer += text[pos + 1];
                    pos += 2;
                    continue;
                }
                materialize();
                buffer += c;
                ++pos;
                continue;
            }

            if (c == '\n') {
                if (buffer.empty() && plain) {
                    size_t trimmed = plain_end;
                    while (trimmed > plain_begin && text[trimmed - 1] == ' ') {
                        --trimmed;
                    }
                    const bool hard = plain_end - trimmed >= 2;
                    plain_end = trimmed;
                    if (hard) {
                        flush();
                        md_inline node;
                        node.kind = md_inline_kind::line_break;
                        out.push_back(move(node));
                        ++pos;
                        continue;
                    }
                    materialize();
                    buffer += ' ';
                    ++pos;
                    continue;
                }
                materialize();
                const bool hard =
                        buffer.size() >= 2 && buffer[buffer.size() - 1] == ' ' && buffer[buffer.size() - 2] == ' ';
                while (!buffer.empty() && buffer.back() == ' ') {
                    buffer.pop_back();
                }
                if (hard) {
                    flush();
                    md_inline node;
                    node.kind = md_inline_kind::line_break;
                    out.push_back(move(node));
                } else {
                    buffer += ' ';
                }
                ++pos;
                continue;
            }

            if (c == '&' && ctx.opt->entities) {
                size_t consumed = 0;
                string decoded;
                if (decode_entity(text, pos, consumed, decoded)) {
                    materialize();
                    buffer += decoded;
                    pos += consumed;
                    continue;
                }
            }

            if (c == '`') {
                size_t fence = 0;
                while (pos + fence < end && text[pos + fence] == '`') {
                    ++fence;
                }
                const auto closer = find_unescaped(text, pos + fence, end, text.substr(pos, fence));
                if (closer != string_view::npos) {
                    string_view content = text.substr(pos + fence, closer - pos - fence);
                    if (content.size() >= 2 && content[0] == ' ' && content[content.size() - 1] == ' ' &&
                        !content.trim().empty()) {
                        content = content.substr(1, content.size() - 2);
                    }
                    md_inline node;
                    node.kind = md_inline_kind::code;
                    node.text = content;
                    flush();
                    out.push_back(move(node));
                    pos = closer + fence;
                    continue;
                }
                buffer += text.substr(pos, fence);
                pos += fence;
                continue;
            }

            if (c == '<') {
                size_t close = 0;
                bool email = false;
                if (ctx.opt->autolinks && is_autolink(text, pos, end, close, email)) {
                    const string_view inner = text.substr(pos + 1, close - pos - 1);
                    md_inline node;
                    node.kind = md_inline_kind::autolink;
                    node.text = inner;
                    static_cast<void>(email);
                    flush();
                    out.push_back(move(node));
                    pos = close + 1;
                    continue;
                }
                if (ctx.opt->inline_html) {
                    size_t consumed = 0;
                    if (parse_inline_html(text, pos, end, consumed)) {
                        md_inline node;
                        node.kind = md_inline_kind::html;
                        node.text = text.substr(pos, consumed);
                        flush();
                        out.push_back(move(node));
                        pos += consumed;
                        continue;
                    }
                }
                materialize();
                buffer += c;
                ++pos;
                continue;
            }

            if ((c == '!' && pos + 1 < end && text[pos + 1] == '[') || c == '[') {
                const bool image = (c == '!');
                const size_t open = image ? pos + 1 : pos;
                const auto close = match_bracket(text, open, end);
                if (close != string_view::npos) {
                    const size_t label_begin = open + 1;
                    const string_view label = text.substr(label_begin, close - label_begin);
                    size_t cursor = close + 1;
                    string_view destination;
                    string_view title;
                    string_view reference;
                    bool resolved = false;

                    if (cursor < end && text[cursor] == '(') {
                        ++cursor;
                        while (cursor < end && (text[cursor] == ' ' || text[cursor] == '\t' || text[cursor] == '\n')) {
                            ++cursor;
                        }
                        if (parse_link_destination(text, cursor, end, destination)) {
                            while (cursor < end &&
                                   (text[cursor] == ' ' || text[cursor] == '\t' || text[cursor] == '\n')) {
                                ++cursor;
                            }
                            parse_link_title(text, cursor, end, title);
                            while (cursor < end && (text[cursor] == ' ' || text[cursor] == '\t')) {
                                ++cursor;
                            }
                            if (cursor < end && text[cursor] == ')') {
                                ++cursor;
                                resolved = true;
                            }
                        }
                    } else if (cursor < end && text[cursor] == '[') {
                        const auto label_close = match_bracket(text, cursor, end);
                        if (label_close != string_view::npos) {
                            const auto raw = text.substr(cursor + 1, label_close - cursor - 1).trim();
                            reference = raw.empty() ? label : raw;
                            if (ctx.document != nullptr) {
                                const auto* definition = md_find_reference(*ctx.document, reference);
                                if (definition != nullptr) {
                                    destination = definition->destination;
                                    title = definition->title;
                                    resolved = true;
                                    cursor = label_close + 1;
                                }
                            }
                        }
                    } else if (ctx.document != nullptr) {
                        reference = label;
                        const auto* definition = md_find_reference(*ctx.document, reference);
                        if (definition != nullptr) {
                            destination = definition->destination;
                            title = definition->title;
                            resolved = true;
                        }
                    }

                    if (resolved) {
                        md_inline node;
                        md_link_data* link = make_link_data();
                        link->destination = destination;
                        link->title = title;
                        link->reference = reference;
                        if (image) {
                            node.kind = md_inline_kind::image;
                            link->alt = label;
                        } else {
                            node.kind = md_inline_kind::link;
                            link->children = parse_inline_range(text, label_begin, close, ctx, depth + 1);
                        }
                        node.link = link;
                        flush();
                        out.push_back(move(node));
                        pos = cursor;
                        continue;
                    }
                }
                materialize();
                buffer += c;
                ++pos;
                continue;
            }

            const bool emphasis_char = (c == '*' || c == '_');
            const bool strike_char = (c == '~' && ctx.opt->strikethrough);
            if (emphasis_char || strike_char) {
                const auto run = analyse_run(text, pos, end, c);
                const size_t need = (c == '~') ? 2 : 1;
                if (run.can_open && run.length >= need) {
                    size_t scan = pos + run.length;
                    size_t found = string_view::npos;
                    size_t found_length = 0;
                    while (scan < end) {
                        const auto* hit = static_cast<const char*>(
                                memory_find(text.data() + scan, static_cast<byte_t>(c), end - scan));
                        if (hit == nullptr) {
                            break;
                        }
                        scan = static_cast<size_t>(hit - text.data());
                        size_t backslashes = 0;
                        for (size_t i = scan; i > pos && text[i - 1] == '\\'; --i) {
                            ++backslashes;
                        }
                        if ((backslashes & 1U) != 0U) {
                            ++scan;
                            continue;
                        }
                        const auto closer = analyse_run(text, scan, end, c);
                        if (closer.can_close && closer.length >= need) {
                            found = scan;
                            found_length = closer.length;
                            break;
                        }
                        scan += max<size_t>(1, closer.length);
                    }

                    if (found != string_view::npos) {
                        size_t use = min(run.length, found_length);
                        if (c == '~') {
                            use = min<size_t>(2, use);
                        } else if (use > 3) {
                            use = 3;
                        }

                        md_inlines inner = parse_inline_range(text, pos + use, found, ctx, depth + 1);
                        md_inline wrapper;
                        if (use >= 3) {
                            md_inline strong;
                            strong.kind = md_inline_kind::strong;
                            strong.children = move(inner);
                            wrapper.kind = md_inline_kind::emphasis;
                            wrapper.children.push_back(move(strong));
                        } else {
                            wrapper.kind = (c == '~')   ? md_inline_kind::strikethrough
                                           : (use == 2) ? md_inline_kind::strong
                                                        : md_inline_kind::emphasis;
                            wrapper.children = move(inner);
                        }

                        if (run.length > use) {
                            string leftover(static_cast<size_t>(run.length - use), c);
                            materialize();
                            buffer += leftover;
                        }
                        flush();
                        out.push_back(move(wrapper));
                        pos = found + found_length;
                        if (found_length > use) {
                            materialize();
                            buffer += string(static_cast<size_t>(found_length - use), c);
                        }
                        continue;
                    }
                }
                const size_t width = max<size_t>(1, run.length);
                materialize();
                buffer += string(text.substr(pos, min(width, end - pos)));
                pos += width;
                continue;
            }

            materialize();
            buffer += c;
            ++pos;
        }

        flush();
        return out;
    }
} // namespace


markdown_parser::markdown_parser(string text) noexcept :
text_(move(text)) {}

markdown_parser::markdown_parser(string text, const option opt) noexcept :
text_(move(text)),
opt_(opt) {}

markdown_parser& markdown_parser::set_options(const option opt) noexcept {
    opt_ = opt;
    return *this;
}

markdown_document markdown_parser::parse() {
    markdown_document document;
    document.set_source(move(text_));
    inner::md_arena_scope scope(&document.arena());
    const line_table table = split_lines(document.source());
    if (table.lines.empty()) {
        return document;
    }

    collect_references(table.lines, table.reference_lines, document);

    parse_context ctx;
    ctx.opt = opt_;
    ctx.document = &document;
    ctx.tables_present = document.source().contains('|');
    document.blocks = parse_block_lines(table.lines, ctx, 0);
    return document;
}

md_inlines markdown_parser::parse_inline(const markdown_document* document) {
    bump_arena& arena = inner::md_inline_arena();
    arena = bump_arena();
    inner::md_arena_scope scope(&arena);
    const string_view text = text_.view();
    auto* data = static_cast<char*>(arena.allocate(text.size(), 1, 1));
    if (!text.empty()) {
        memory_copy(data, text.data(), text.size());
    }
    const string_view source(data, text.size());
    inline_context ctx;
    ctx.opt = &opt_;
    ctx.document = document;
    return parse_inline_range(source, 0, source.size(), ctx, 0);
}

NEFORCE_END_NAMESPACE__
