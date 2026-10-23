#include <NeForce/core/string/codepoint.hpp>
#include <NeForce/core/string/string.hpp>
#include <NeForce/core/utility/packages.hpp>
#include <NeForce/core/file/mermaid/mermaid.hpp>
#include <NeForce/core/string/width.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    constexpr size_t MERMAID_SOURCE_LIMIT = 20000;
    constexpr size_t MERMAID_MAX_NODES = 512;
    constexpr size_t MERMAID_MAX_EDGES = 2048;
    constexpr size_t MERMAID_MAX_PARTICIPANTS = 128;
    constexpr size_t MERMAID_MAX_EVENTS = 512;
    constexpr size_t MERMAID_MAX_CLASS_MEMBERS = 8;
    constexpr size_t MERMAID_MAX_ER_ATTRIBUTES = 8;
    constexpr size_t MERMAID_MAX_PIE_ENTRIES = 64;
    constexpr size_t MERMAID_MAX_RELATIONS = 512;
    constexpr size_t MERMAID_MAX_GIT_BRANCHES = 32;
    constexpr size_t MERMAID_MAX_BYPASS_RELATIONS = 6;
    constexpr size_t MERMAID_MAX_MINDMAP_DEPTH = 16;
    constexpr size_t MERMAID_MAX_TIMELINE_ENTRIES = 256;
    constexpr size_t MERMAID_MAX_JOURNEY_ENTRIES = 256;
    constexpr size_t MERMAID_MAX_GANTT_TASKS = 256;
    constexpr size_t MERMAID_MAX_QUADRANT_POINTS = 64;
    constexpr size_t MERMAID_MAX_REQUIREMENTS = 64;
    constexpr int MERMAID_PIE_BAR_WIDTH = 24;
    constexpr int MERMAID_GANTT_BAR_WIDTH = 30;
    constexpr int MERMAID_QUADRANT_WIDTH = 32;
    constexpr int MERMAID_QUADRANT_HEIGHT = 8;

    constexpr uint8_t DIR_UP = 1;
    constexpr uint8_t DIR_DOWN = 2;
    constexpr uint8_t DIR_LEFT = 4;
    constexpr uint8_t DIR_RIGHT = 8;

    enum class node_shape : uint8_t {
        rectangle = 0,
        rounded,
        diamond,
        circle,
    };

    enum class edge_style : uint8_t {
        solid = 0,
        dotted,
        thick,
    };

    enum class flow_direction : uint8_t {
        top_down = 0,
        left_right,
    };

    enum class diagram_kind : uint8_t {
        none = 0,
        flow,
        sequence,
        state,
        class_diagram,
        entity_relationship,
        git_graph,
        pie_chart,
        mindmap,
        timeline,
        gantt,
        journey,
        quadrant,
        requirement,
    };

    struct header_info {
        diagram_kind kind = diagram_kind::none;
        flow_direction direction = flow_direction::top_down;
        string_view remainder;
    };

    struct mermaid_document {
        vector<string_view> lines;
        size_t header = 0;
        string title;
    };

    bool is_id_char(const char character) noexcept {
        const auto value = static_cast<byte_t>(character);
        return is_digit_or_alpha(character) || value == '_' || value >= 0x80;
    }

    bool is_arrow_char(const char character) noexcept {
        return character == '-' || character == '.' || character == '=' || character == '<' || character == '>';
    }

    size_t skip_spaces(const string_view text, size_t position) noexcept {
        while (position < text.size() && is_space(text[position])) {
            ++position;
        }
        return position;
    }

    string trim_copy(const string_view text) { return string(text.trim()); }

    string_view strip_comment(const string_view line) noexcept {
        const size_t marker = line.find("%%");
        return marker == string_view::npos ? line : line.substr(0, marker);
    }

    bool matches_keyword(const string_view text, const string_view keyword) noexcept {
        if (!text.starts_with(keyword)) {
            return false;
        }
        if (text.size() == keyword.size()) {
            return true;
        }
        const char next = text[keyword.size()];
        return is_space(next) || next == ';';
    }

    string upper_ascii(const string_view text) {
        string result;
        result.reserve(text.size());
        for (size_t i = 0; i < text.size(); ++i) {
            const char character = text[i];
            result += (character >= 'a' && character <= 'z') ? static_cast<char>(character - 'a' + 'A') : character;
        }
        return result;
    }

    string strip_quotes(const string_view value) {
        const string_view text = value.trim();
        if (text.size() >= 2) {
            const char first = text.front();
            const char last = text.back();
            if ((first == '"' && last == '"') || (first == '\'' && last == '\'')) {
                return string(text.substr(1, text.size() - 2));
            }
        }
        return string(text);
    }

    string sanitize_label(const string_view value) {
        const string trimmed = strip_quotes(value);
        string result;
        result.reserve(trimmed.size());
        for (size_t i = 0; i < trimmed.size(); ++i) {
            const auto character = static_cast<byte_t>(trimmed[i]);
            if (character == '\t') {
                result += ' ';
            } else if (character < 0x20 || character == 0x7F) {
                result += ' ';
            } else {
                result += trimmed[i];
            }
        }
        return result;
    }

    vector<string_view> split_lines(const string_view source) {
        vector<string_view> lines;
        size_t begin = 0;
        while (begin <= source.size()) {
            const size_t end = source.find('\n', begin);
            if (end == string_view::npos) {
                lines.push_back(source.substr(begin));
                break;
            }
            lines.push_back(source.substr(begin, end - begin));
            begin = end + 1;
        }
        return lines;
    }

    void split_statements(const string_view line, vector<string_view>& statements) {
        size_t begin = 0;
        int depth = 0;
        bool quoted = false;
        for (size_t i = 0; i < line.size(); ++i) {
            const char character = line[i];
            if (character == '"') {
                quoted = !quoted;
            } else if (!quoted && (character == '[' || character == '(' || character == '{')) {
                ++depth;
            } else if (!quoted && (character == ']' || character == ')' || character == '}')) {
                depth = depth > 0 ? depth - 1 : 0;
            } else if (character == ';' && !quoted && depth == 0) {
                statements.push_back(line.substr(begin, i - begin));
                begin = i + 1;
            }
        }
        statements.push_back(line.substr(begin));
    }

    bool take_title_line(const string_view text, string& title) {
        const string_view trimmed = text.trim();
        if (!trimmed.starts_with("title:")) {
            return false;
        }
        string candidate = sanitize_label(trimmed.substr(6));
        if (!candidate.empty() && title.empty()) {
            title = move(candidate);
        }
        return true;
    }

    bool is_directive_line(const string_view text) noexcept {
        constexpr string_view directives[] = {
                "subgraph", "end",       "classDef", "class",    "style",   "linkStyle",
                "click",    "direction", "accTitle", "accDescr", "section",
        };
        for (const string_view directive: directives) {
            if (matches_keyword(text, directive)) {
                return true;
            }
        }
        return false;
    }

    bool parse_direction_token(const string_view text, flow_direction& direction) {
        const string upper = upper_ascii(text);
        if (upper == "TD" || upper == "TB" || upper == "BT") {
            direction = flow_direction::top_down;
            return true;
        }
        if (upper == "LR" || upper == "RL") {
            direction = flow_direction::left_right;
            return true;
        }
        return false;
    }

    uint8_t glyph_directions(const string_view glyph) noexcept {
        if (glyph == "─" || glyph == "━" || glyph == "┄") {
            return DIR_LEFT | DIR_RIGHT;
        }
        if (glyph == "│" || glyph == "┃" || glyph == "┆") {
            return DIR_UP | DIR_DOWN;
        }
        if (glyph == "┌" || glyph == "┏") {
            return DIR_DOWN | DIR_RIGHT;
        }
        if (glyph == "┐" || glyph == "┓") {
            return DIR_DOWN | DIR_LEFT;
        }
        if (glyph == "└" || glyph == "┗") {
            return DIR_UP | DIR_RIGHT;
        }
        if (glyph == "┘" || glyph == "┛") {
            return DIR_UP | DIR_LEFT;
        }
        if (glyph == "├" || glyph == "┣") {
            return DIR_UP | DIR_DOWN | DIR_RIGHT;
        }
        if (glyph == "┤" || glyph == "┫") {
            return DIR_UP | DIR_DOWN | DIR_LEFT;
        }
        if (glyph == "┬" || glyph == "┳") {
            return DIR_DOWN | DIR_LEFT | DIR_RIGHT;
        }
        if (glyph == "┴" || glyph == "┻") {
            return DIR_UP | DIR_LEFT | DIR_RIGHT;
        }
        if (glyph == "┼" || glyph == "╋") {
            return DIR_UP | DIR_DOWN | DIR_LEFT | DIR_RIGHT;
        }
        return 0;
    }

    const char* directions_glyph(const edge_style style, const uint8_t mask) noexcept {
        if (style == edge_style::thick) {
            switch (mask) {
                case DIR_UP | DIR_DOWN:
                    return "┃";
                case DIR_LEFT | DIR_RIGHT:
                    return "━";
                case DIR_DOWN | DIR_RIGHT:
                    return "┏";
                case DIR_DOWN | DIR_LEFT:
                    return "┓";
                case DIR_UP | DIR_RIGHT:
                    return "┗";
                case DIR_UP | DIR_LEFT:
                    return "┛";
                case DIR_UP | DIR_DOWN | DIR_RIGHT:
                    return "┣";
                case DIR_UP | DIR_DOWN | DIR_LEFT:
                    return "┫";
                case DIR_DOWN | DIR_LEFT | DIR_RIGHT:
                    return "┳";
                case DIR_UP | DIR_LEFT | DIR_RIGHT:
                    return "┻";
                case DIR_UP | DIR_DOWN | DIR_LEFT | DIR_RIGHT:
                    return "╋";
                default:
                    break;
            }
        }
        if (style == edge_style::dotted) {
            if (mask == (DIR_LEFT | DIR_RIGHT)) {
                return "┄";
            }
            if (mask == (DIR_UP | DIR_DOWN)) {
                return "┆";
            }
        }
        switch (mask) {
            case DIR_UP | DIR_DOWN:
                return "│";
            case DIR_LEFT | DIR_RIGHT:
                return "─";
            case DIR_DOWN | DIR_RIGHT:
                return "┌";
            case DIR_DOWN | DIR_LEFT:
                return "┐";
            case DIR_UP | DIR_RIGHT:
                return "└";
            case DIR_UP | DIR_LEFT:
                return "┘";
            case DIR_UP | DIR_DOWN | DIR_RIGHT:
                return "├";
            case DIR_UP | DIR_DOWN | DIR_LEFT:
                return "┤";
            case DIR_DOWN | DIR_LEFT | DIR_RIGHT:
                return "┬";
            case DIR_UP | DIR_LEFT | DIR_RIGHT:
                return "┴";
            case DIR_UP | DIR_DOWN | DIR_LEFT | DIR_RIGHT:
                return "┼";
            default:
                return nullptr;
        }
    }

    size_t codepoint_bytes(const char lead) noexcept {
        const auto value = static_cast<byte_t>(lead);
        if ((value & 0x80U) == 0) {
            return 1;
        }
        if ((value & 0xE0U) == 0xC0U) {
            return 2;
        }
        if ((value & 0xF0U) == 0xE0U) {
            return 3;
        }
        if ((value & 0xF8U) == 0xF0U) {
            return 4;
        }
        return 1;
    }

    uint32_t decode_codepoint(const string_view text) noexcept {
        if (text.empty()) {
            return 0;
        }
        size_t offset = 0;
        const codepoint value =
                codepoint::decode_utf8(reinterpret_cast<const byte_t*>(text.data()), offset, text.size());
        return value.value();
    }

    int codepoint_columns(const uint32_t value) noexcept {
        if (value < 0x20 || value == 0x7F) {
            return 1;
        }
        return codepoint(static_cast<char32_t>(value)).display_width();
    }

    struct art_cell {
        string glyph;
        mermaid_role role = mermaid_role::plain;

        art_cell() :
        glyph(" ") {}
    };

    class art_canvas {
    public:
        void put(const int row, const int col, const string_view glyph, const mermaid_role role) {
            if (row < 0 || col < 0 || glyph.empty()) {
                return;
            }
            ensure(row, col);
            clear_cell(row, col);
            const int width = text_display_width(glyph);
            if (width > 1) {
                ensure(row, col + width - 1);
                for (int k = 1; k < width; ++k) {
                    clear_cell(row, col + k);
                }
            }
            rows_[static_cast<size_t>(row)][static_cast<size_t>(col)].glyph = string(glyph);
            rows_[static_cast<size_t>(row)][static_cast<size_t>(col)].role = role;
            for (int k = 1; k < width; ++k) {
                art_cell& continuation = rows_[static_cast<size_t>(row)][static_cast<size_t>(col) + k];
                continuation.glyph.clear();
                continuation.role = role;
            }
        }

        int put_text(const int row, int col, const string_view text, const mermaid_role role) {
            size_t position = 0;
            while (position < text.size()) {
                const size_t bytes = codepoint_bytes(text[position]);
                const size_t end = min(text.size(), position + bytes);
                const string_view piece = text.substr(position, end - position);
                const int width = codepoint_columns(decode_codepoint(piece));
                if (width <= 0) {
                    append_combining(row, col, piece);
                } else {
                    put(row, col, piece, role);
                    col += width;
                }
                position = end;
            }
            return col;
        }

        void add_connection(const int row, const int col, const uint8_t mask, const edge_style style,
                            const mermaid_role role) {
            if (row < 0 || col < 0 || mask == 0) {
                return;
            }
            ensure(row, col);
            art_cell& cell = rows_[static_cast<size_t>(row)][static_cast<size_t>(col)];
            const uint8_t existing = glyph_directions(cell.glyph.view());
            if (existing == 0 && cell.glyph != " ") {
                return;
            }
            const char* glyph = directions_glyph(style, static_cast<uint8_t>(existing | mask));
            if (glyph == nullptr) {
                return;
            }
            cell.glyph = glyph;
            cell.role = role;
        }

        NEFORCE_NODISCARD bool has_vertical(const int row, const int col) const noexcept {
            if (row < 0 || col < 0) {
                return false;
            }
            const auto row_index = static_cast<size_t>(row);
            if (row_index >= rows_.size()) {
                return false;
            }
            const auto& line = rows_[row_index];
            const auto col_index = static_cast<size_t>(col);
            return col_index < line.size() &&
                   (glyph_directions(line[col_index].glyph.view()) & (DIR_UP | DIR_DOWN)) == (DIR_UP | DIR_DOWN);
        }

        NEFORCE_NODISCARD bool region_is_blank(const int row, const int col, const int width) const {
            if (row < 0 || col < 0 || width <= 0) {
                return false;
            }
            const auto row_index = static_cast<size_t>(row);
            if (row_index >= rows_.size()) {
                return true;
            }
            const auto& line = rows_[row_index];
            for (int offset = 0; offset < width; ++offset) {
                const auto index = static_cast<size_t>(col) + static_cast<size_t>(offset);
                if (index >= line.size()) {
                    continue;
                }
                const art_cell& cell = line[index];
                if (!cell.glyph.empty() && cell.glyph != " ") {
                    return false;
                }
                if (cell.glyph.empty() && index > 0) {
                    const art_cell& previous = line[index - 1];
                    if (!previous.glyph.empty() && text_display_width(previous.glyph.view()) > 1) {
                        return false;
                    }
                }
            }
            return true;
        }

        void put_edge_label(const int row, const int col, const int alternate_row, const int alternate_col,
                            const string_view label) {
            if (label.empty()) {
                return;
            }
            const int width = text_display_width(label);
            const int xs[3] = {col, alternate_col, min(col, alternate_col)};
            const int ys[2] = {row, alternate_row};
            for (int attempt_y = 0; attempt_y < 2; ++attempt_y) {
                for (int attempt_x = 0; attempt_x < 3; ++attempt_x) {
                    if (xs[attempt_x] >= 0 && region_is_blank(ys[attempt_y], xs[attempt_x], width)) {
                        put_text(ys[attempt_y], xs[attempt_x], label, mermaid_role::edge_label);
                        return;
                    }
                }
            }
            if (col >= 0) {
                put_text(row, col, label, mermaid_role::edge_label);
            }
        }

        void horizontal_run(const int row, const int first, const int last, const bool dotted,
                            const mermaid_role role) {
            for (int x = first; x <= last; ++x) {
                if (has_vertical(row, x)) {
                    add_connection(row, x, DIR_LEFT | DIR_RIGHT, edge_style::solid, role);
                } else {
                    put(row, x, dotted ? "·" : "─", role);
                }
            }
        }

        NEFORCE_NODISCARD const vector<vector<art_cell>>& rows() const noexcept { return rows_; }

    private:
        vector<vector<art_cell>> rows_;

        void ensure(const int row, const int col) {
            while (rows_.size() <= static_cast<size_t>(row)) {
                rows_.push_back(vector<art_cell>());
            }
            auto& line = rows_[static_cast<size_t>(row)];
            if (line.size() <= static_cast<size_t>(col)) {
                line.resize(static_cast<size_t>(col) + 1);
            }
        }

        void clear_cell(const int row, const int col) {
            auto& line = rows_[static_cast<size_t>(row)];
            const auto index = static_cast<size_t>(col);
            if (index >= line.size()) {
                return;
            }
            art_cell& cell = line[index];
            if (cell.glyph.empty()) {
                if (index > 0) {
                    art_cell& previous = line[index - 1];
                    if (!previous.glyph.empty() && text_display_width(previous.glyph.view()) > 1) {
                        previous.glyph = " ";
                        previous.role = mermaid_role::plain;
                    }
                }
                cell.glyph = " ";
                cell.role = mermaid_role::plain;
                return;
            }
            const int width = text_display_width(cell.glyph.view());
            if (width > 1) {
                for (int k = 1; k < width && index + static_cast<size_t>(k) < line.size(); ++k) {
                    line[index + static_cast<size_t>(k)].glyph = " ";
                    line[index + static_cast<size_t>(k)].role = mermaid_role::plain;
                }
            }
            cell.glyph = " ";
            cell.role = mermaid_role::plain;
        }

        void append_combining(const int row, const int col, const string_view bytes) {
            if (row < 0 || col <= 0 || bytes.empty()) {
                return;
            }
            ensure(row, col - 1);
            auto& line = rows_[static_cast<size_t>(row)];
            int index = col - 1;
            if (index > 0 && static_cast<size_t>(index) < line.size() &&
                line[static_cast<size_t>(index)].glyph.empty()) {
                --index;
            }
            if (index >= 0 && static_cast<size_t>(index) < line.size() &&
                !line[static_cast<size_t>(index)].glyph.empty()) {
                line[static_cast<size_t>(index)].glyph.append(bytes);
            }
        }
    };

    void pad_line_to(string& line, vector<mermaid_run>& runs, const int width) {
        const int current = text_display_width(line.view());
        if (current >= width) {
            return;
        }
        const auto padding = static_cast<size_t>(width - current);
        const size_t begin = line.size();
        line.append(padding, ' ');
        if (!runs.empty() && runs.back().role == mermaid_role::plain) {
            runs.back().end = line.size();
        } else {
            mermaid_run run;
            run.begin = begin;
            run.end = line.size();
            run.role = mermaid_role::plain;
            runs.push_back(run);
        }
    }

    bool row_is_blank(const vector<art_cell>& row) {
        for (const art_cell& cell: row) {
            if (!cell.glyph.empty() && cell.glyph != " ") {
                return false;
            }
        }
        return true;
    }

    mermaid_art build_art(const art_canvas& canvas) {
        mermaid_art art;
        const auto& rows = canvas.rows();
        size_t first_row = 0;
        while (first_row < rows.size() && row_is_blank(rows[first_row])) {
            ++first_row;
        }
        size_t last_row = rows.size();
        while (last_row > first_row && row_is_blank(rows[last_row - 1])) {
            --last_row;
        }
        if (first_row >= last_row) {
            return art;
        }

        vector<string> raw_lines;
        vector<vector<mermaid_run>> raw_runs;
        raw_lines.reserve(last_row - first_row);
        raw_runs.reserve(last_row - first_row);
        int max_width = 0;
        for (size_t row_index = first_row; row_index < last_row; ++row_index) {
            const vector<art_cell>& row = rows[row_index];
            size_t last_col = 0;
            bool has_content = false;
            for (size_t col = 0; col < row.size(); ++col) {
                if (!row[col].glyph.empty() && row[col].glyph != " ") {
                    last_col = col;
                    has_content = true;
                }
            }
            string line;
            vector<mermaid_run> runs;
            if (has_content) {
                bool open = false;
                size_t run_begin = 0;
                auto current = mermaid_role::plain;
                for (size_t col = 0; col <= last_col; ++col) {
                    const art_cell& cell = row[col];
                    if (cell.glyph.empty()) {
                        continue;
                    }
                    if (!open) {
                        open = true;
                        current = cell.role;
                        run_begin = line.size();
                    } else if (cell.role != current) {
                        if (line.size() > run_begin) {
                            mermaid_run run;
                            run.begin = run_begin;
                            run.end = line.size();
                            run.role = current;
                            runs.push_back(run);
                        }
                        run_begin = line.size();
                        current = cell.role;
                    }
                    line += cell.glyph;
                }
                if (line.size() > run_begin) {
                    mermaid_run run;
                    run.begin = run_begin;
                    run.end = line.size();
                    run.role = current;
                    runs.push_back(run);
                }
            }
            max_width = max(max_width, text_display_width(line.view()));
            raw_lines.push_back(move(line));
            raw_runs.push_back(move(runs));
        }

        if (max_width <= 0) {
            return art;
        }

        art.width = max_width;
        art.lines.reserve(raw_lines.size());
        art.styled.reserve(raw_runs.size());
        for (size_t i = 0; i < raw_lines.size(); ++i) {
            string line = move(raw_lines[i]);
            vector<mermaid_run> runs = move(raw_runs[i]);
            pad_line_to(line, runs, max_width);
            art.lines.push_back(move(line));
            art.styled.push_back(move(runs));
        }
        return art;
    }

    void prepend_title(mermaid_art& art, const string& title) {
        if (title.empty()) {
            return;
        }
        const int title_width = text_display_width(title.view());
        const int width = max(art.width, title_width);
        for (size_t i = 0; i < art.lines.size(); ++i) {
            pad_line_to(art.lines[i], art.styled[i], width);
        }

        string line = title;
        vector<mermaid_run> runs;
        if (!title.empty()) {
            mermaid_run run;
            run.begin = 0;
            run.end = title.size();
            run.role = mermaid_role::title;
            runs.push_back(run);
        }
        pad_line_to(line, runs, width);

        vector<string> lines;
        vector<vector<mermaid_run>> styled;
        lines.reserve(art.lines.size() + 1);
        styled.reserve(art.styled.size() + 1);
        lines.push_back(move(line));
        styled.push_back(move(runs));
        for (size_t i = 0; i < art.lines.size(); ++i) {
            lines.push_back(move(art.lines[i]));
            styled.push_back(move(art.styled[i]));
        }
        art.lines = move(lines);
        art.styled = move(styled);
        art.width = width;
    }

    struct text_segment {
        string text;
        mermaid_role role = mermaid_role::text;
    };

    string repeat_char(const size_t count, const char value) {
        string result;
        result.append(count, value);
        return result;
    }

    string repeat_glyph(const size_t count, const string_view glyph) {
        string result;
        for (size_t i = 0; i < count; ++i) {
            result.append(glyph);
        }
        return result;
    }

    string format_integer(const long long value) {
        if (value == 0) {
            return string("0");
        }
        const bool negative = value < 0;
        unsigned long long magnitude = negative ? static_cast<unsigned long long>(-(value + 1)) + 1ULL
                                                : static_cast<unsigned long long>(value);
        char digits[24];
        size_t count = 0;
        while (magnitude > 0 && count < sizeof(digits)) {
            digits[count++] = static_cast<char>('0' + static_cast<char>(magnitude % 10ULL));
            magnitude /= 10ULL;
        }
        string result;
        if (negative) {
            result += '-';
        }
        while (count > 0) {
            result += digits[--count];
        }
        return result;
    }

    string zero_pad(const int value, const int width) {
        string digits = format_integer(value);
        if (compare_greater_equal(digits.size(), width)) {
            return digits;
        }
        string result;
        result.append(static_cast<size_t>(width - static_cast<int>(digits.size())), '0');
        result += digits;
        return result;
    }

    string pad_to_width(const string_view text, const int width) {
        const int current = text_display_width(text);
        if (current >= width) {
            return string(text);
        }
        string result(text);
        result.append(static_cast<size_t>(width - current), ' ');
        return result;
    }

    string align_right_width(const string_view text, const int width) {
        const int current = text_display_width(text);
        if (current >= width) {
            return string(text);
        }
        string result;
        result.append(static_cast<size_t>(width - current), ' ');
        result.append(text);
        return result;
    }

    bool parse_integer(const string_view text, long long& value) {
        size_t position = skip_spaces(text, 0);
        bool negative = false;
        if (position < text.size() && (text[position] == '+' || text[position] == '-')) {
            negative = text[position] == '-';
            ++position;
        }
        const size_t start = position;
        long long result = 0;
        while (position < text.size() && text[position] >= '0' && text[position] <= '9') {
            result = result * 10 + static_cast<long long>(text[position] - '0');
            result = min(result, 1000000000LL);
            ++position;
        }
        if (position == start) {
            return false;
        }
        value = negative ? -result : result;
        return true;
    }

    bool parse_fraction(const string_view text, double& value) {
        const string_view trimmed = text.trim();
        if (trimmed.empty()) {
            return false;
        }
        size_t position = 0;
        bool negative = false;
        if (trimmed[position] == '+' || trimmed[position] == '-') {
            negative = trimmed[position] == '-';
            ++position;
        }
        double result = 0.0;
        bool any = false;
        while (position < trimmed.size() && trimmed[position] >= '0' && trimmed[position] <= '9') {
            result = result * 10.0 + static_cast<double>(trimmed[position] - '0');
            ++position;
            any = true;
        }
        if (position < trimmed.size() && trimmed[position] == '.') {
            ++position;
            double scale = 0.1;
            while (position < trimmed.size() && trimmed[position] >= '0' && trimmed[position] <= '9') {
                result += static_cast<double>(trimmed[position] - '0') * scale;
                scale *= 0.1;
                ++position;
                any = true;
            }
        }
        if (!any) {
            return false;
        }
        value = negative ? -result : result;
        return true;
    }

    string truncate_display(const string_view text, const int max_columns) {
        if (max_columns <= 0) {
            return string();
        }
        string result;
        size_t position = 0;
        int used = 0;
        while (position < text.size()) {
            const size_t bytes = codepoint_bytes(text[position]);
            const size_t end = min(text.size(), position + bytes);
            const string_view piece = text.substr(position, end - position);
            const int width = codepoint_columns(decode_codepoint(piece));
            if (width <= 0) {
                result.append(piece);
                position = end;
                continue;
            }
            if (used + width > max_columns) {
                break;
            }
            result.append(piece);
            used += width;
            position = end;
        }
        return result;
    }

    void add_text_segment(vector<text_segment>& segments, const string_view text, const mermaid_role role) {
        text_segment segment;
        segment.text = string(text);
        segment.role = role;
        segments.push_back(move(segment));
    }

    void split_on_char(const string_view text, const char delimiter, vector<string_view>& parts) {
        size_t begin = 0;
        while (begin <= text.size()) {
            const size_t end = text.find(delimiter, begin);
            parts.push_back(end == string_view::npos ? text.substr(begin) : text.substr(begin, end - begin));
            if (end == string_view::npos) {
                break;
            }
            begin = end + 1;
        }
    }

    bool take_plain_title(const string_view text, string& title) {
        if (!matches_keyword(text, "title")) {
            return false;
        }
        string candidate = sanitize_label(text.substr(5));
        if (!candidate.empty() && title.empty()) {
            title = move(candidate);
        }
        return true;
    }

    class text_art_builder {
    public:
        void add_line(const string_view text, const mermaid_role role) {
            vector<text_segment> segments;
            text_segment segment;
            segment.text = string(text);
            segment.role = role;
            segments.push_back(move(segment));
            add_line(segments);
        }

        void add_line(const vector<text_segment>& segments) {
            string line;
            vector<mermaid_run> runs;
            auto current = mermaid_role::plain;
            bool open = false;
            for (const text_segment& segment: segments) {
                if (segment.text.empty()) {
                    continue;
                }
                if (open && segment.role == current) {
                    line.append(segment.text);
                    runs.back().end = line.size();
                    continue;
                }
                mermaid_run run;
                run.begin = line.size();
                line.append(segment.text);
                run.end = line.size();
                run.role = segment.role;
                runs.push_back(run);
                current = segment.role;
                open = true;
            }
            lines_.push_back(move(line));
            styled_.push_back(move(runs));
        }

        void add_blank_line() { add_line(string_view(), mermaid_role::plain); }

        void finish(mermaid_art& art) const {
            size_t first = 0;
            while (first < lines_.size() && trim_copy(lines_[first].view()).empty()) {
                ++first;
            }
            size_t last = lines_.size();
            while (last > first && trim_copy(lines_[last - 1].view()).empty()) {
                --last;
            }
            if (first >= last) {
                return;
            }
            int width = 0;
            for (size_t i = first; i < last; ++i) {
                width = max(width, text_display_width(lines_[i].view()));
            }
            if (width <= 0) {
                return;
            }
            art.width = width;
            art.lines.reserve(last - first);
            art.styled.reserve(last - first);
            for (size_t i = first; i < last; ++i) {
                string line = lines_[i];
                vector<mermaid_run> runs = styled_[i];
                pad_line_to(line, runs, width);
                art.lines.push_back(move(line));
                art.styled.push_back(move(runs));
            }
        }

    private:
        vector<string> lines_;
        vector<vector<mermaid_run>> styled_;
    };

    struct stacked_box {
        string title;
        vector<string> members;
        int left = 0;
        int top = 0;
        int width = 0;
    };

    int stacked_box_height(const stacked_box& box) noexcept {
        return box.members.empty() ? 3 : 4 + static_cast<int>(box.members.size());
    }

    int stacked_box_content_width(const string_view title, const vector<string>& members) {
        int width = text_display_width(title);
        for (const string& member: members) {
            width = max(width, text_display_width(member.view()));
        }
        return width;
    }

    void draw_stacked_box(art_canvas& canvas, const stacked_box& box) {
        if (box.width < 4) {
            return;
        }
        const int right = box.left + box.width - 1;
        const int bottom = box.top + stacked_box_height(box) - 1;
        constexpr auto border = mermaid_role::border;
        canvas.put(box.top, box.left, "┌", border);
        for (int x = box.left + 1; x < right; ++x) {
            canvas.put(box.top, x, "─", border);
        }
        canvas.put(box.top, right, "┐", border);
        canvas.put(box.top + 1, box.left, "│", border);
        canvas.put(box.top + 1, right, "│", border);
        const int title_width = text_display_width(box.title.view());
        int title_left = box.left + 1 + (box.width - 2 - title_width) / 2;
        title_left = max(title_left, box.left + 1);
        canvas.put_text(box.top + 1, title_left, box.title.view(), mermaid_role::text);
        int row = box.top + 2;
        if (!box.members.empty()) {
            canvas.put(row, box.left, "├", border);
            for (int x = box.left + 1; x < right; ++x) {
                canvas.put(row, x, "─", border);
            }
            canvas.put(row, right, "┤", border);
            ++row;
            for (const string& member: box.members) {
                canvas.put(row, box.left, "│", border);
                canvas.put(row, right, "│", border);
                canvas.put_text(row, box.left + 2, member.view(), mermaid_role::text);
                ++row;
            }
        }
        canvas.put(bottom, box.left, "└", border);
        for (int x = box.left + 1; x < right; ++x) {
            canvas.put(bottom, x, "─", border);
        }
        canvas.put(bottom, right, "┘", border);
    }

    enum class relation_head : uint8_t {
        none = 0,
        arrow,
        triangle,
        diamond,
        hollow_diamond,
    };

    string_view relation_head_glyph(const relation_head head, const bool dotted, const bool upward) noexcept {
        switch (head) {
            case relation_head::arrow:
                if (dotted) {
                    return upward ? "△" : "▽";
                }
                return upward ? "▲" : "▼";
            case relation_head::triangle:
                return "△";
            case relation_head::diamond:
                return "◆";
            case relation_head::hollow_diamond:
                return "◇";
            case relation_head::none:
            default:
                return nullptr;
        }
    }

    void draw_relation_connector(art_canvas& canvas, const int center_x, const int gap_top, const int gap_height,
                                 const bool dotted, const relation_head head, const bool head_at_top,
                                 const string_view label) {
        if (gap_height <= 0) {
            return;
        }
        constexpr auto role = mermaid_role::edge;
        const string_view line_glyph = dotted ? "┆" : "│";
        for (int offset = 0; offset < gap_height; ++offset) {
            canvas.put(gap_top + offset, center_x, line_glyph, role);
        }
        if (head != relation_head::none) {
            const string_view glyph = relation_head_glyph(head, dotted, head_at_top);
            if (!glyph.empty()) {
                canvas.put(head_at_top ? gap_top : gap_top + gap_height - 1, center_x, glyph, role);
            }
        }
        if (!label.empty()) {
            int row = gap_top + (head_at_top ? 2 : 1);
            if (row >= gap_top + gap_height) {
                row = gap_top + gap_height - 1;
            }
            canvas.put_text(row, center_x + 2, label, mermaid_role::edge_label);
        }
    }

    struct bypass_style {
        bool dotted = false;
        relation_head head = relation_head::none;
        bool marker_at_source = true;
        string label;
        string source_cardinality;
        string target_cardinality;
    };

    void draw_bypass_relation(art_canvas& canvas, const int channel, const int box_right, const int source_row,
                              const int target_row, const bypass_style& style) {
        if (source_row == target_row) {
            return;
        }
        constexpr auto edge = mermaid_role::edge;
        const string_view line_glyph = style.dotted ? "┆" : "│";
        for (int x = box_right; x < channel; ++x) {
            canvas.put(source_row, x, "─", edge);
            canvas.put(target_row, x, "─", edge);
        }
        canvas.put(source_row, channel, source_row < target_row ? "┐" : "┘", edge);
        canvas.put(target_row, channel, source_row < target_row ? "┘" : "┐", edge);
        const int first_row = min(source_row, target_row);
        const int last_row = max(source_row, target_row);
        for (int row = first_row + 1; row < last_row; ++row) {
            canvas.put(row, channel, line_glyph, edge);
        }
        if (!style.source_cardinality.empty()) {
            canvas.put_text(source_row, channel + 2, style.source_cardinality.view(), mermaid_role::edge_label);
        }
        if (!style.target_cardinality.empty()) {
            canvas.put_text(target_row, channel + 2, style.target_cardinality.view(), mermaid_role::edge_label);
        }
        if (!style.label.empty()) {
            int label_row = (source_row + target_row) / 2;
            if (label_row == source_row) {
                ++label_row;
            }
            if (label_row == target_row) {
                --label_row;
            }
            canvas.put_text(label_row, channel + 2, style.label.view(), mermaid_role::edge_label);
        }
        const int marker_row = style.marker_at_source ? source_row : target_row;
        if (style.head == relation_head::arrow) {
            canvas.put(marker_row, box_right, style.dotted ? "◁" : "◀", edge);
        } else if (style.head != relation_head::none) {
            const string_view glyph = relation_head_glyph(style.head, style.dotted, false);
            if (!glyph.empty()) {
                canvas.put(marker_row, box_right, glyph, edge);
            }
        }
    }

    struct class_box {
        string name;
        vector<string> members;
    };

    struct raw_class_relation {
        string from_name;
        string to_name;
        string label;
        relation_head head = relation_head::none;
        bool dotted = false;
    };

    struct class_relation {
        size_t from = 0;
        size_t to = 0;
        string label;
        relation_head head = relation_head::none;
        bool dotted = false;
    };

    struct class_data {
        vector<class_box> classes;
        vector<raw_class_relation> raw_relations;
    };

    size_t find_class(const class_data& data, const string_view name) noexcept {
        for (size_t i = 0; i < data.classes.size(); ++i) {
            if (data.classes[i].name.view() == name) {
                return i;
            }
        }
        return string_view::npos;
    }

    void add_class_members(class_box& box, const string_view body) {
        size_t begin = 0;
        while (begin <= body.size() && box.members.size() < MERMAID_MAX_CLASS_MEMBERS) {
            const size_t end = body.find(';', begin);
            const string_view piece = end == string_view::npos ? body.substr(begin) : body.substr(begin, end - begin);
            const string member = sanitize_label(piece);
            if (!member.empty() && box.members.size() < MERMAID_MAX_CLASS_MEMBERS) {
                box.members.push_back(member);
            }
            if (end == string_view::npos) {
                break;
            }
            begin = end + 1;
        }
    }

    string clean_class_name(const string_view text) {
        string name = sanitize_label(text);
        const size_t css = name.view().find(":::");
        if (css != string_view::npos) {
            name = string(name.view().substr(0, css));
        }
        const size_t space = name.view().find(' ');
        if (space != string_view::npos) {
            name = string(name.view().substr(0, space));
        }
        return name;
    }

    void parse_class_declaration(const string_view text, class_data& data, size_t& body_target) {
        const string_view rest = text.trim();
        if (rest.empty()) {
            return;
        }
        const size_t brace = rest.find('{');
        const string name = clean_class_name(brace == string_view::npos ? rest : rest.substr(0, brace));
        if (name.empty()) {
            return;
        }
        size_t index = find_class(data, name.view());
        if (index == string_view::npos) {
            if (data.classes.size() >= MERMAID_MAX_NODES) {
                return;
            }
            class_box box;
            box.name = name;
            data.classes.push_back(move(box));
            index = data.classes.size() - 1;
        }
        if (brace == string_view::npos) {
            return;
        }
        const size_t close = rest.find('}', brace + 1);
        if (close == string_view::npos) {
            add_class_members(data.classes[index], rest.substr(brace + 1));
            body_target = index;
        } else {
            add_class_members(data.classes[index], rest.substr(brace + 1, close - brace - 1));
        }
    }

    bool parse_class_relation(const string_view text, raw_class_relation& result) {
        struct marker_info {
            string_view marker;
            bool dotted;
            relation_head head;
            bool owner_on_left;
        };

        constexpr marker_info markers[] = {
                {"<|--", false, relation_head::triangle, true},
                {"--|>", false, relation_head::triangle, false},
                {"<|..", true, relation_head::triangle, true},
                {"..|>", true, relation_head::triangle, false},
                {"*--", false, relation_head::diamond, true},
                {"--*", false, relation_head::diamond, false},
                {"o--", false, relation_head::hollow_diamond, true},
                {"--o", false, relation_head::hollow_diamond, false},
                {"-->", false, relation_head::arrow, true},
                {"<--", false, relation_head::arrow, false},
                {"..>", true, relation_head::arrow, true},
                {"<..", true, relation_head::arrow, false},
                {"---", false, relation_head::none, true},
                {"...", true, relation_head::none, true},
                {"--", false, relation_head::none, true},
                {"..", true, relation_head::none, true},
        };

        size_t best_position = string_view::npos;
        size_t best_length = 0;
        const marker_info* best = nullptr;
        for (const marker_info& marker: markers) {
            const string_view token(marker.marker);
            const size_t position = text.find(token);
            if (position == string_view::npos) {
                continue;
            }
            if (best_position == string_view::npos || position < best_position ||
                (position == best_position && token.size() > best_length)) {
                best_position = position;
                best_length = token.size();
                best = &marker;
            }
        }
        if (best == nullptr) {
            return false;
        }
        const string_view left = text.substr(0, best_position).trim();
        const string_view right_part = text.substr(best_position + best_length);
        string_view right = right_part;
        string label;
        const size_t colon = right_part.find(':');
        if (colon != string_view::npos) {
            label = sanitize_label(right_part.substr(colon + 1));
            right = right_part.substr(0, colon).trim();
        }
        const string left_name = sanitize_label(left);
        const string right_name = sanitize_label(right);
        if (left_name.empty() || right_name.empty()) {
            return false;
        }
        if (left_name.view().contains(' ') || right_name.view().contains(' ')) {
            return false;
        }
        result.from_name = best->owner_on_left ? left_name : right_name;
        result.to_name = best->owner_on_left ? right_name : left_name;
        result.label = label;
        result.head = best->head;
        result.dotted = best->dotted;
        return true;
    }

    bool is_class_directive(const string_view text) noexcept {
        constexpr string_view directives[] = {
                "classDef", "style", "linkStyle", "click", "cssClass", "note", "direction", "accTitle", "accDescr",
        };

        for (const string_view directive: directives) {
            if (matches_keyword(text, directive)) {
                return true;
            }
        }
        return false;
    }

    void process_class_line(const string_view line, class_data& data, string& title, size_t& body_target) {
        if (body_target != string_view::npos) {
            const size_t close = line.find('}');
            add_class_members(data.classes[body_target], close == string_view::npos ? line : line.substr(0, close));
            if (close != string_view::npos) {
                body_target = string_view::npos;
            }
            return;
        }
        vector<string_view> statements;
        split_statements(line, statements);
        for (const string_view raw: statements) {
            const string_view text = raw.trim();
            if (text.empty()) {
                continue;
            }
            if (take_title_line(text, title) || take_plain_title(text, title)) {
                continue;
            }
            if (is_class_directive(text)) {
                continue;
            }
            if (text.starts_with("}")) {
                continue;
            }
            if (matches_keyword(text, "class")) {
                parse_class_declaration(text.substr(5), data, body_target);
                continue;
            }
            raw_class_relation relation;
            if (parse_class_relation(text, relation)) {
                if (data.raw_relations.size() < MERMAID_MAX_RELATIONS) {
                    data.raw_relations.push_back(move(relation));
                }
                continue;
            }
            const size_t colon = text.find(':');
            if (colon != string_view::npos) {
                const string name = sanitize_label(text.substr(0, colon));
                const size_t index = find_class(data, name.view());
                if (index != string_view::npos) {
                    const string member = sanitize_label(text.substr(colon + 1));
                    if (!member.empty() && data.classes[index].members.size() < MERMAID_MAX_CLASS_MEMBERS) {
                        data.classes[index].members.push_back(member);
                    }
                }
            }
        }
    }

    bool build_class_diagram(const mermaid_document& document, const header_info& header, string& title,
                             class_data& data) {
        size_t body_target = string_view::npos;
        process_class_line(header.remainder, data, title, body_target);
        for (size_t i = document.header + 1; i < document.lines.size(); ++i) {
            process_class_line(strip_comment(document.lines[i]), data, title, body_target);
        }
        return !data.classes.empty();
    }

    bool layout_class_diagram(class_data& data, mermaid_art& art) {
        const size_t count = data.classes.size();
        if (count == 0) {
            return false;
        }
        vector<class_relation> relations;
        for (const raw_class_relation& raw: data.raw_relations) {
            const size_t from = find_class(data, raw.from_name.view());
            const size_t to = find_class(data, raw.to_name.view());
            if (from == string_view::npos || to == string_view::npos || from == to) {
                continue;
            }
            if (relations.size() >= MERMAID_MAX_RELATIONS) {
                break;
            }
            class_relation relation;
            relation.from = from;
            relation.to = to;
            relation.label = raw.label;
            relation.head = raw.head;
            relation.dotted = raw.dotted;
            relations.push_back(move(relation));
        }

        vector<vector<size_t>> outgoing(count);
        for (const class_relation& relation: relations) {
            outgoing[relation.from].push_back(relation.to);
        }
        vector<size_t> order;
        vector<bool> visited(count, false);
        order.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            if (visited[i]) {
                continue;
            }
            size_t current = i;
            while (current != string_view::npos && !visited[current]) {
                visited[current] = true;
                order.push_back(current);
                size_t next = string_view::npos;
                for (const size_t successor: outgoing[current]) {
                    if (!visited[successor]) {
                        next = successor;
                        break;
                    }
                }
                current = next;
            }
        }

        vector<size_t> position(count, string_view::npos);
        for (size_t i = 0; i < order.size(); ++i) {
            position[order[i]] = i;
        }

        int box_width = 8;
        for (const class_box& box: data.classes) {
            box_width = max(box_width, stacked_box_content_width(box.name.view(), box.members) + 4);
        }
        vector<int> tops(count, 0);
        vector<int> heights(count, 3);
        int cursor = 0;
        for (const size_t index: order) {
            stacked_box probe;
            probe.title = data.classes[index].name;
            probe.members = data.classes[index].members;
            tops[index] = cursor;
            heights[index] = stacked_box_height(probe);
            cursor += heights[index] + 4;
        }

        struct relation_group {
            size_t upper = 0;
            size_t lower = 0;
            relation_head head = relation_head::none;
            bool dotted = false;
            bool head_at_top = false;
            string label;
        };
        vector<relation_group> groups;
        vector<relation_group> bypass_groups;
        for (const class_relation& relation: relations) {
            const size_t from_position = position[relation.from];
            const size_t to_position = position[relation.to];
            if (from_position == string_view::npos || to_position == string_view::npos) {
                continue;
            }
            size_t upper = 0;
            size_t lower = 0;
            bool bypass = false;
            if (from_position + 1 == to_position) {
                upper = relation.from;
                lower = relation.to;
            } else if (to_position + 1 == from_position) {
                upper = relation.to;
                lower = relation.from;
            } else if (from_position < to_position) {
                upper = relation.from;
                lower = relation.to;
                bypass = true;
            } else if (to_position < from_position) {
                upper = relation.to;
                lower = relation.from;
                bypass = true;
            } else {
                continue;
            }
            if (bypass && bypass_groups.size() >= MERMAID_MAX_BYPASS_RELATIONS) {
                continue;
            }
            const size_t head_box = relation.head == relation_head::arrow ? relation.to : relation.from;
            const bool head_at_top = head_box == upper;
            vector<relation_group>& target_groups = bypass ? bypass_groups : groups;
            bool merged = false;
            for (relation_group& group: target_groups) {
                if (group.upper == upper && group.lower == lower) {
                    if (!relation.label.empty()) {
                        if (!group.label.empty()) {
                            group.label += ", ";
                        }
                        group.label += relation.label;
                    }
                    merged = true;
                    break;
                }
            }
            if (merged) {
                continue;
            }
            relation_group group;
            group.upper = upper;
            group.lower = lower;
            group.head = relation.head;
            group.dotted = relation.dotted;
            group.head_at_top = head_at_top;
            group.label = relation.label;
            target_groups.push_back(move(group));
        }

        art_canvas canvas;
        for (size_t i = 0; i < count; ++i) {
            stacked_box box;
            box.title = data.classes[i].name;
            box.members = data.classes[i].members;
            box.left = 0;
            box.top = tops[i];
            box.width = box_width;
            draw_stacked_box(canvas, box);
        }
        for (const relation_group& group: groups) {
            const int upper_bottom = tops[group.upper] + heights[group.upper] - 1;
            draw_relation_connector(canvas, box_width / 2, upper_bottom + 1, 4, group.dotted, group.head,
                                    group.head_at_top, group.label.view());
        }
        int bypass_label_width = 0;
        int adjacent_label_width = 0;
        for (const relation_group& group: bypass_groups) {
            bypass_label_width = max(bypass_label_width, text_display_width(group.label.view()));
        }
        for (const relation_group& group: groups) {
            adjacent_label_width = max(adjacent_label_width, text_display_width(group.label.view()));
        }
        const int bypass_spacing = max(4, bypass_label_width + 3);
        const int bypass_base = max(box_width + 2, box_width / 2 + 3 + adjacent_label_width);
        for (size_t lane = 0; lane < bypass_groups.size(); ++lane) {
            const relation_group& group = bypass_groups[lane];
            bypass_style style;
            style.dotted = group.dotted;
            style.head = group.head;
            style.marker_at_source = group.head_at_top;
            style.label = group.label;
            draw_bypass_relation(canvas, bypass_base + static_cast<int>(lane) * bypass_spacing, box_width,
                                 tops[group.upper] + 1, tops[group.lower] + 1, style);
        }
        art = build_art(canvas);
        return !art.lines.empty();
    }

    struct er_entity {
        string name;
        string title;
        vector<string> attributes;
    };

    struct er_raw_relation {
        string from_name;
        string to_name;
        string label;
        string from_cardinality;
        string to_cardinality;
        bool dotted = false;
    };

    struct er_relation {
        size_t from = 0;
        size_t to = 0;
        string label;
        string from_cardinality;
        string to_cardinality;
        bool dotted = false;
    };

    struct er_data {
        vector<er_entity> entities;
        vector<er_raw_relation> raw_relations;
    };

    size_t find_er_entity(const er_data& data, const string_view name) noexcept {
        for (size_t i = 0; i < data.entities.size(); ++i) {
            if (data.entities[i].name.view() == name) {
                return i;
            }
        }
        return string_view::npos;
    }

    size_t ensure_er_entity(er_data& data, const string_view name) {
        if (name.empty()) {
            return string_view::npos;
        }
        const size_t existing = find_er_entity(data, name);
        if (existing != string_view::npos) {
            return existing;
        }
        if (data.entities.size() >= MERMAID_MAX_NODES) {
            return string_view::npos;
        }
        er_entity entity;
        entity.name = string(name);
        entity.title = string(name);
        data.entities.push_back(move(entity));
        return data.entities.size() - 1;
    }

    bool parse_er_entity_head(const string_view text, string& name, string& title) {
        const string_view head = text.trim();
        if (head.empty()) {
            return false;
        }
        const size_t open = head.find('[');
        if (open != string_view::npos) {
            const size_t close = head.find(']', open + 1);
            name = sanitize_label(head.substr(0, open));
            title = close == string_view::npos ? name : sanitize_label(head.substr(open + 1, close - open - 1));
        } else {
            name = sanitize_label(head);
            title = name;
        }
        return !name.empty();
    }

    void add_er_attribute(er_entity& entity, const string_view body) {
        const string attribute = sanitize_label(body);
        if (attribute.empty()) {
            return;
        }
        if (entity.attributes.size() >= MERMAID_MAX_ER_ATTRIBUTES) {
            return;
        }
        entity.attributes.push_back(attribute);
    }

    string er_cardinality_text(const string_view value) {
        if (value == "||") {
            return string("1");
        }
        if (value == "|o" || value == "o|") {
            return string("0..1");
        }
        if (value == "}|" || value == "|{") {
            return string("1..*");
        }
        if (value == "}o" || value == "o{") {
            return string("0..*");
        }
        if (value == "|" || value == "{") {
            return string("1");
        }
        if (value == "o") {
            return string("0..1");
        }
        return sanitize_label(value);
    }

    bool parse_er_relation(const string_view text, er_raw_relation& result) {
        size_t dash = string_view::npos;
        for (size_t i = 0; i + 1 < text.size(); ++i) {
            if ((text[i] == '-' && text[i + 1] == '-') || (text[i] == '.' && text[i + 1] == '.')) {
                dash = i;
                break;
            }
        }
        if (dash == string_view::npos || dash == 0) {
            return false;
        }
        size_t left_begin = dash;
        while (left_begin > 0 && (text[left_begin - 1] == '|' || text[left_begin - 1] == 'o' ||
                                  text[left_begin - 1] == '{' || text[left_begin - 1] == '}')) {
            --left_begin;
        }
        if (left_begin == 0) {
            return false;
        }
        const bool dotted = text[dash] == '.';
        size_t dash_end = dash + 2;
        while (dash_end < text.size() && (text[dash_end] == '-' || text[dash_end] == '.')) {
            ++dash_end;
        }
        size_t right_card_end = dash_end;
        while (right_card_end < text.size() && (text[right_card_end] == '|' || text[right_card_end] == 'o' ||
                                                text[right_card_end] == '{' || text[right_card_end] == '}')) {
            ++right_card_end;
        }
        const string_view left_name = text.substr(0, left_begin).trim();
        string_view right = text.substr(right_card_end).trim();
        if (left_name.empty() || right.empty()) {
            return false;
        }
        string label;
        const size_t colon = right.find(':');
        if (colon != string_view::npos) {
            label = sanitize_label(right.substr(colon + 1));
            right = right.substr(0, colon).trim();
        }
        const string from_name = sanitize_label(left_name);
        const string to_name = sanitize_label(right);
        if (from_name.empty() || to_name.empty()) {
            return false;
        }
        result.from_name = from_name;
        result.to_name = to_name;
        result.label = label;
        result.from_cardinality = er_cardinality_text(text.substr(left_begin, dash - left_begin));
        result.to_cardinality = er_cardinality_text(text.substr(dash_end, right_card_end - dash_end));
        result.dotted = dotted;
        return true;
    }

    bool is_er_directive(const string_view text) noexcept {
        constexpr string_view directives[] = {
                "accTitle", "accDescr", "direction", "style", "classDef", "click", "cssClass", "note",
        };
        for (const string_view directive: directives) {
            if (matches_keyword(text, directive)) {
                return true;
            }
        }
        return false;
    }

    void process_er_line(const string_view line, er_data& data, string& title, size_t& body_entity) {
        if (body_entity != string_view::npos) {
            const size_t close = line.find('}');
            if (body_entity < data.entities.size()) {
                add_er_attribute(data.entities[body_entity], close == string_view::npos ? line : line.substr(0, close));
            }
            if (close != string_view::npos) {
                body_entity = string_view::npos;
            }
            return;
        }
        vector<string_view> statements;
        split_statements(line, statements);
        for (const string_view raw: statements) {
            const string_view text = raw.trim();
            if (text.empty()) {
                continue;
            }
            if (take_title_line(text, title) || take_plain_title(text, title)) {
                continue;
            }
            if (is_er_directive(text)) {
                continue;
            }
            const size_t brace = text.find('{');
            if (brace != string_view::npos && !text.contains("--") && !text.contains("..")) {
                string name;
                string display;
                if (parse_er_entity_head(text.substr(0, brace), name, display)) {
                    const size_t index = ensure_er_entity(data, name.view());
                    if (index != string_view::npos) {
                        data.entities[index].title = display;
                        const size_t close = text.find('}', brace + 1);
                        if (close == string_view::npos) {
                            body_entity = index;
                            add_er_attribute(data.entities[index], text.substr(brace + 1));
                        } else {
                            vector<string_view> attributes;
                            split_on_char(text.substr(brace + 1, close - brace - 1), ';', attributes);
                            for (const string_view attribute: attributes) {
                                add_er_attribute(data.entities[index], attribute);
                            }
                        }
                    }
                    continue;
                }
            }
            er_raw_relation relation;
            if (parse_er_relation(text, relation)) {
                if (data.raw_relations.size() < MERMAID_MAX_RELATIONS) {
                    data.raw_relations.push_back(move(relation));
                }
                continue;
            }
            if (!text.contains(' ') && text.back() != '}') {
                ensure_er_entity(data, text);
            }
        }
    }

    bool build_er_diagram(const mermaid_document& document, const header_info& header, string& title, er_data& data) {
        size_t body_entity = string_view::npos;
        process_er_line(header.remainder, data, title, body_entity);
        for (size_t i = document.header + 1; i < document.lines.size(); ++i) {
            process_er_line(strip_comment(document.lines[i]), data, title, body_entity);
        }
        return !data.entities.empty() || !data.raw_relations.empty();
    }

    void put_right_aligned_text(art_canvas& canvas, const int row, const int end_column, const string_view text,
                                const mermaid_role role) {
        if (text.empty()) {
            return;
        }
        const int width = text_display_width(text);
        int start = end_column - width;
        start = max(start, 0);
        canvas.put_text(row, start, text, role);
    }

    bool layout_er_diagram(er_data& data, mermaid_art& art) {
        if (data.entities.empty() && data.raw_relations.empty()) {
            return false;
        }
        vector<er_relation> relations;
        for (const er_raw_relation& raw: data.raw_relations) {
            const size_t from = ensure_er_entity(data, raw.from_name.view());
            const size_t to = ensure_er_entity(data, raw.to_name.view());
            if (from == string_view::npos || to == string_view::npos || from == to) {
                continue;
            }
            if (relations.size() >= MERMAID_MAX_RELATIONS) {
                break;
            }
            er_relation relation;
            relation.from = from;
            relation.to = to;
            relation.label = raw.label;
            relation.from_cardinality = raw.from_cardinality;
            relation.to_cardinality = raw.to_cardinality;
            relation.dotted = raw.dotted;
            relations.push_back(move(relation));
        }
        const size_t count = data.entities.size();

        vector<vector<size_t>> outgoing(count);
        for (const er_relation& relation: relations) {
            outgoing[relation.from].push_back(relation.to);
        }
        vector<size_t> order;
        vector<bool> visited(count, false);
        order.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            if (visited[i]) {
                continue;
            }
            size_t current = i;
            while (current != string_view::npos && !visited[current]) {
                visited[current] = true;
                order.push_back(current);
                size_t next = string_view::npos;
                for (const size_t successor: outgoing[current]) {
                    if (!visited[successor]) {
                        next = successor;
                        break;
                    }
                }
                current = next;
            }
        }
        vector<size_t> position(count, string_view::npos);
        for (size_t i = 0; i < order.size(); ++i) {
            position[order[i]] = i;
        }

        int box_width = 8;
        for (const er_entity& entity: data.entities) {
            box_width = max(box_width, stacked_box_content_width(entity.title.view(), entity.attributes) + 4);
        }
        vector<int> tops(count, 0);
        vector<int> heights(count, 3);
        int cursor = 0;
        for (const size_t index: order) {
            stacked_box probe;
            probe.title = data.entities[index].title;
            probe.members = data.entities[index].attributes;
            tops[index] = cursor;
            heights[index] = stacked_box_height(probe);
            cursor += heights[index] + 4;
        }

        struct er_group {
            size_t upper = 0;
            size_t lower = 0;
            string label;
            string from_cardinality;
            string to_cardinality;
            bool dotted = false;
        };

        vector<er_group> groups;
        vector<er_group> bypass_groups;

        for (const er_relation& relation: relations) {
            const size_t from_position = position[relation.from];
            const size_t to_position = position[relation.to];
            if (from_position == string_view::npos || to_position == string_view::npos) {
                continue;
            }
            size_t upper = 0;
            size_t lower = 0;
            string upper_cardinality;
            string lower_cardinality;
            bool bypass = false;
            if (from_position + 1 == to_position) {
                upper = relation.from;
                lower = relation.to;
                upper_cardinality = relation.from_cardinality;
                lower_cardinality = relation.to_cardinality;
            } else if (to_position + 1 == from_position) {
                upper = relation.to;
                lower = relation.from;
                upper_cardinality = relation.to_cardinality;
                lower_cardinality = relation.from_cardinality;
            } else if (from_position < to_position) {
                upper = relation.from;
                lower = relation.to;
                upper_cardinality = relation.from_cardinality;
                lower_cardinality = relation.to_cardinality;
                bypass = true;
            } else if (to_position < from_position) {
                upper = relation.to;
                lower = relation.from;
                upper_cardinality = relation.to_cardinality;
                lower_cardinality = relation.from_cardinality;
                bypass = true;
            } else {
                continue;
            }
            if (bypass && bypass_groups.size() >= MERMAID_MAX_BYPASS_RELATIONS) {
                continue;
            }
            vector<er_group>& target_groups = bypass ? bypass_groups : groups;
            bool merged = false;
            for (er_group& group: target_groups) {
                if (group.upper == upper && group.lower == lower) {
                    if (!relation.label.empty()) {
                        if (!group.label.empty()) {
                            group.label += ", ";
                        }
                        group.label += relation.label;
                    }
                    merged = true;
                    break;
                }
            }
            if (merged) {
                continue;
            }
            er_group group;
            group.upper = upper;
            group.lower = lower;
            group.label = relation.label;
            group.from_cardinality = move(upper_cardinality);
            group.to_cardinality = move(lower_cardinality);
            group.dotted = relation.dotted;
            target_groups.push_back(move(group));
        }

        art_canvas canvas;
        for (size_t i = 0; i < count; ++i) {
            stacked_box box;
            box.title = data.entities[i].title;
            box.members = data.entities[i].attributes;
            box.left = 0;
            box.top = tops[i];
            box.width = box_width;
            draw_stacked_box(canvas, box);
        }
        const int center = box_width / 2;
        for (const er_group& group: groups) {
            const int upper_bottom = tops[group.upper] + heights[group.upper] - 1;
            const int gap_top = upper_bottom + 1;
            draw_relation_connector(canvas, center, gap_top, 4, group.dotted, relation_head::arrow, false,
                                    group.label.view());
            put_right_aligned_text(canvas, gap_top, center - 1, group.from_cardinality.view(),
                                   mermaid_role::edge_label);
            put_right_aligned_text(canvas, gap_top + 3, center - 1, group.to_cardinality.view(),
                                   mermaid_role::edge_label);
        }
        int bypass_text_width = 0;
        int adjacent_label_width = 0;
        for (const er_group& group: bypass_groups) {
            bypass_text_width = max(bypass_text_width, text_display_width(group.label.view()));
            bypass_text_width = max(bypass_text_width, text_display_width(group.from_cardinality.view()));
            bypass_text_width = max(bypass_text_width, text_display_width(group.to_cardinality.view()));
        }
        for (const er_group& group: groups) {
            adjacent_label_width = max(adjacent_label_width, text_display_width(group.label.view()));
        }
        const int bypass_spacing = max(4, bypass_text_width + 3);
        const int bypass_base = max(box_width + 2, box_width / 2 + 3 + adjacent_label_width);
        for (size_t lane = 0; lane < bypass_groups.size(); ++lane) {
            const er_group& group = bypass_groups[lane];
            bypass_style style;
            style.dotted = group.dotted;
            style.head = relation_head::arrow;
            style.marker_at_source = false;
            style.label = group.label;
            style.source_cardinality = group.from_cardinality;
            style.target_cardinality = group.to_cardinality;
            draw_bypass_relation(canvas, bypass_base + static_cast<int>(lane) * bypass_spacing, box_width,
                                 tops[group.upper] + 1, tops[group.lower] + 1, style);
        }
        art = build_art(canvas);
        return !art.lines.empty();
    }

    struct git_branch_entry {
        string name;
        size_t lane = 0;
        size_t source_lane = 0;
        int creation_row = 0;
        int last_row = 0;
        int end_row = 0;
        bool explicit_creation = false;
        bool merged = false;
    };

    struct git_commit_entry {
        int row = 0;
        size_t lane = 0;
        string label;
    };

    struct git_merge_entry {
        int row = 0;
        size_t from_lane = 0;
        size_t to_lane = 0;
        string label;
        bool has_id = false;
    };

    struct git_data {
        vector<git_branch_entry> branches;
        vector<git_commit_entry> commits;
        vector<git_merge_entry> merges;
        int max_row = 0;
    };

    string_view first_word_token(const string_view text) noexcept {
        size_t end = 0;
        while (end < text.size() && !is_space(text[end])) {
            ++end;
        }
        return text.substr(0, end);
    }

    string extract_quoted_option(const string_view text, const string_view key) {
        const size_t position = text.find(key);
        if (position == string_view::npos) {
            return string();
        }
        const string_view rest = text.substr(position + key.size()).trim();
        if (rest.empty()) {
            return string();
        }
        if (rest.front() == '"' || rest.front() == '\'') {
            const size_t close = rest.find(rest.front(), 1);
            if (close != string_view::npos) {
                return sanitize_label(rest.substr(1, close - 1));
            }
        }
        return sanitize_label(first_word_token(rest));
    }

    size_t find_git_branch(const git_data& data, const string_view name) noexcept {
        for (size_t i = 0; i < data.branches.size(); ++i) {
            if (data.branches[i].name.view() == name) {
                return i;
            }
        }
        return string_view::npos;
    }

    size_t ensure_git_branch(git_data& data, const string_view name, const int fallback_row) {
        const size_t existing = find_git_branch(data, name);
        if (existing != string_view::npos) {
            return existing;
        }
        if (name.empty() || data.branches.size() >= MERMAID_MAX_GIT_BRANCHES) {
            return string_view::npos;
        }
        git_branch_entry branch;
        branch.name = string(name);
        branch.lane = data.branches.size();
        branch.source_lane = data.branches.empty() ? 0 : data.branches.back().lane;
        branch.creation_row = fallback_row;
        branch.last_row = fallback_row;
        branch.end_row = fallback_row;
        data.branches.push_back(move(branch));
        return data.branches.size() - 1;
    }

    void process_git_line(const string_view line, git_data& data, size_t& current, int& next_row, string& title) {
        const string_view text = line.trim();
        if (text.empty()) {
            return;
        }
        if (take_title_line(text, title) || take_plain_title(text, title)) {
            return;
        }
        if (matches_keyword(text, "commit") || matches_keyword(text, "cherry-pick")) {
            if (data.commits.size() >= MERMAID_MAX_EVENTS) {
                return;
            }
            const bool cherry = matches_keyword(text, "cherry-pick");
            const string_view rest = text.substr(cherry ? 11 : 6);
            const string id = extract_quoted_option(rest, "id:");
            const string tag = extract_quoted_option(rest, "tag:");
            git_commit_entry entry;
            entry.row = next_row;
            entry.lane = data.branches[current].lane;
            entry.label = id;
            if (entry.label.empty() && cherry) {
                entry.label = "cherry-pick";
            }
            if (!tag.empty()) {
                if (!entry.label.empty()) {
                    entry.label += ' ';
                }
                entry.label += tag;
            }
            data.commits.push_back(move(entry));
            data.branches[current].last_row = next_row;
            data.max_row = max(data.max_row, next_row);
            next_row += 2;
            return;
        }
        if (matches_keyword(text, "branch")) {
            const string name = sanitize_label(first_word_token(text.substr(6).trim()));
            if (name.empty()) {
                return;
            }
            const size_t existing = find_git_branch(data, name.view());
            if (existing != string_view::npos) {
                current = existing;
                return;
            }
            if (data.branches.size() >= MERMAID_MAX_GIT_BRANCHES) {
                return;
            }
            git_branch_entry branch;
            branch.name = name;
            branch.lane = data.branches.size();
            branch.source_lane = data.branches[current].lane;
            branch.creation_row = next_row;
            branch.last_row = next_row;
            branch.end_row = next_row;
            branch.explicit_creation = true;
            data.branches.push_back(move(branch));
            current = data.branches.size() - 1;
            data.max_row = max(data.max_row, next_row);
            next_row += 2;
            return;
        }
        if (matches_keyword(text, "checkout") || matches_keyword(text, "switch")) {
            const bool checkout = matches_keyword(text, "checkout");
            const string name = sanitize_label(first_word_token(text.substr(checkout ? 8 : 6).trim()));
            if (name.empty()) {
                return;
            }
            const size_t index = ensure_git_branch(data, name.view(), max(0, next_row - 2));
            if (index != string_view::npos) {
                current = index;
            }
            return;
        }
        if (matches_keyword(text, "merge")) {
            if (data.merges.size() >= MERMAID_MAX_EVENTS) {
                return;
            }
            const string_view rest = text.substr(5).trim();
            const string name = sanitize_label(first_word_token(rest));
            if (name.empty()) {
                return;
            }
            const size_t index = ensure_git_branch(data, name.view(), max(0, next_row - 2));
            if (index == string_view::npos) {
                return;
            }
            const string id = extract_quoted_option(rest, "id:");
            git_merge_entry entry;
            entry.row = next_row;
            entry.from_lane = data.branches[index].lane;
            entry.to_lane = data.branches[current].lane;
            entry.label = id.empty() ? (string("merge ") + name) : id;
            entry.has_id = !id.empty();
            data.merges.push_back(move(entry));
            data.branches[index].merged = true;
            data.branches[index].end_row = next_row - 1;
            data.branches[current].last_row = next_row;
            data.max_row = max(data.max_row, next_row);
            next_row += 2;
        }
    }

    bool build_git_graph(const mermaid_document& document, const header_info& header, string& title, git_data& data) {
        git_branch_entry main_branch;
        main_branch.name = "main";
        main_branch.lane = 0;
        main_branch.source_lane = 0;
        main_branch.creation_row = 0;
        data.branches.push_back(move(main_branch));
        size_t current = 0;
        int next_row = 0;
        process_git_line(header.remainder, data, current, next_row, title);
        for (size_t i = document.header + 1; i < document.lines.size(); ++i) {
            process_git_line(strip_comment(document.lines[i]), data, current, next_row, title);
        }
        return !data.commits.empty();
    }

    bool layout_git_graph(const git_data& data, mermaid_art& art) {
        if (data.commits.empty() || data.branches.empty()) {
            return false;
        }
        int max_label_width = 0;
        for (const git_commit_entry& commit: data.commits) {
            max_label_width = max(max_label_width, text_display_width(commit.label.view()));
        }
        for (const git_merge_entry& merge: data.merges) {
            if (!merge.has_id) {
                max_label_width = max(max_label_width, text_display_width(merge.label.view()));
            }
        }
        for (const git_branch_entry& branch: data.branches) {
            if (branch.explicit_creation) {
                max_label_width = max(max_label_width, text_display_width(branch.name.view()));
            }
        }
        const int spacing = max(6, max_label_width + 4);
        const auto lane_x = [spacing](const size_t lane) { return static_cast<int>(lane) * spacing; };
        constexpr auto edge = mermaid_role::edge;

        art_canvas canvas;
        for (const git_branch_entry& branch: data.branches) {
            const int end_row = branch.merged ? branch.end_row : data.max_row;
            for (int row = branch.creation_row + 1; row <= end_row; ++row) {
                canvas.add_connection(row, lane_x(branch.lane), DIR_UP | DIR_DOWN, edge_style::solid, edge);
            }
            if (branch.lane == 0 && branch.creation_row == 0) {
                canvas.add_connection(0, 0, DIR_UP | DIR_DOWN, edge_style::solid, edge);
            }
        }
        for (const git_branch_entry& branch: data.branches) {
            if (!branch.explicit_creation) {
                continue;
            }
            const int row = branch.creation_row;
            const int source_x = lane_x(branch.source_lane);
            const int target_x = lane_x(branch.lane);
            if (source_x == target_x) {
                continue;
            }
            canvas.add_connection(row, source_x, DIR_UP | DIR_DOWN | DIR_RIGHT, edge_style::solid, edge);
            for (int x = source_x + 1; x < target_x; ++x) {
                canvas.add_connection(row, x, DIR_LEFT | DIR_RIGHT, edge_style::solid, edge);
            }
            canvas.put(row, target_x, "┐", edge);
            canvas.put_text(row, target_x + 2, branch.name.view(), mermaid_role::text);
        }
        for (const git_merge_entry& merge: data.merges) {
            const int row = merge.row - 1;
            if (row < 0) {
                continue;
            }
            const int from_x = lane_x(merge.from_lane);
            const int to_x = lane_x(merge.to_lane);
            if (from_x == to_x) {
                continue;
            }
            if (from_x > to_x) {
                canvas.put(row, from_x, "┘", edge);
                for (int x = to_x + 1; x < from_x; ++x) {
                    canvas.put(row, x, "─", edge);
                }
                canvas.put(row, to_x, "┌", edge);
            } else {
                canvas.put(row, from_x, "└", edge);
                for (int x = from_x + 1; x < to_x; ++x) {
                    canvas.put(row, x, "─", edge);
                }
                canvas.put(row, to_x, "┐", edge);
            }
        }
        for (const git_commit_entry& commit: data.commits) {
            canvas.put(commit.row, lane_x(commit.lane), "●", edge);
            if (!commit.label.empty()) {
                canvas.put_text(commit.row, lane_x(commit.lane) + 2, commit.label.view(), mermaid_role::text);
            }
        }
        for (const git_merge_entry& merge: data.merges) {
            canvas.put(merge.row, lane_x(merge.to_lane), "●", edge);
            if (!merge.label.empty()) {
                canvas.put_text(merge.row, lane_x(merge.to_lane) + 2, merge.label.view(), mermaid_role::text);
            }
        }
        art = build_art(canvas);
        return !art.lines.empty();
    }

    struct pie_entry {
        string label;
        long long value = 0;
    };

    struct pie_data {
        vector<pie_entry> entries;
    };

    void parse_pie_header_remainder(const string_view text, string& title) {
        string_view rest = text.trim();
        if (matches_keyword(rest, "showData")) {
            rest = rest.substr(8).trim();
        }
        if (matches_keyword(rest, "title")) {
            string candidate = sanitize_label(rest.substr(5));
            if (!candidate.empty() && title.empty()) {
                title = move(candidate);
            }
        }
    }

    bool parse_pie_entry(const string_view text, pie_entry& entry) {
        const string_view trimmed = text.trim();
        if (trimmed.empty()) {
            return false;
        }
        const char quote = trimmed.front();
        if (quote != '"' && quote != '\'') {
            return false;
        }
        const size_t close = trimmed.find(quote, 1);
        if (close == string_view::npos) {
            return false;
        }
        string_view rest = trimmed.substr(close + 1).trim();
        if (rest.empty() || rest.front() != ':') {
            return false;
        }
        rest = rest.substr(1).trim();
        long long value = 0;
        size_t position = 0;
        bool any = false;
        while (position < rest.size() && rest[position] >= '0' && rest[position] <= '9') {
            value = value * 10 + static_cast<long long>(rest[position] - '0');
            value = min(value, 1000000000LL);
            ++position;
            any = true;
        }
        if (!any) {
            return false;
        }
        const string label = sanitize_label(trimmed.substr(1, close - 1));
        if (label.empty()) {
            return false;
        }
        entry.label = label;
        entry.value = value;
        return true;
    }

    bool is_pie_directive(const string_view text) noexcept {
        constexpr string_view directives[] = {"accTitle", "accDescr"};
        for (const string_view directive: directives) {
            if (matches_keyword(text, directive)) {
                return true;
            }
        }
        return false;
    }

    bool build_pie_chart(const mermaid_document& document, const header_info& header, string& title, pie_data& data) {
        parse_pie_header_remainder(header.remainder, title);
        for (size_t i = document.header + 1; i < document.lines.size(); ++i) {
            const string_view text = strip_comment(document.lines[i]).trim();
            if (text.empty()) {
                continue;
            }
            if (take_title_line(text, title) || take_plain_title(text, title)) {
                continue;
            }
            if (is_pie_directive(text)) {
                continue;
            }
            pie_entry entry;
            if (parse_pie_entry(text, entry)) {
                if (data.entries.size() < MERMAID_MAX_PIE_ENTRIES) {
                    data.entries.push_back(move(entry));
                }
            }
        }
        return !data.entries.empty();
    }

    bool layout_pie_chart(const pie_data& data, mermaid_art& art) {
        if (data.entries.empty()) {
            return false;
        }
        long long total = 0;
        long long max_value = 0;
        int label_width = 0;
        for (const pie_entry& entry: data.entries) {
            total += entry.value;
            max_value = max(max_value, entry.value);
            label_width = max(label_width, text_display_width(entry.label.view()));
        }
        if (total <= 0) {
            return false;
        }
        const string max_value_text = format_integer(max_value);
        const int value_width = text_display_width(max_value_text.view());
        text_art_builder builder;
        for (const pie_entry& entry: data.entries) {
            const long long scaled_twice =
                    (entry.value * 2 * static_cast<long long>(MERMAID_PIE_BAR_WIDTH) + total) / total;
            int full = static_cast<int>(scaled_twice / 2);
            bool half = (scaled_twice % 2) != 0;
            if (full >= MERMAID_PIE_BAR_WIDTH) {
                full = MERMAID_PIE_BAR_WIDTH;
                half = false;
            }
            const int cells = full + (half ? 1 : 0);
            string bar = repeat_glyph(static_cast<size_t>(full), "█");
            if (half) {
                bar += "▉";
            }
            bar.append(static_cast<size_t>(MERMAID_PIE_BAR_WIDTH - cells), ' ');
            const long long percent = (entry.value * 100 + total / 2) / total;
            const string value_text = format_integer(entry.value);
            const string percent_text = format_integer(percent) + "%";
            vector<text_segment> segments;
            add_text_segment(segments, pad_to_width(entry.label.view(), label_width).view(), mermaid_role::text);
            add_text_segment(segments, " │ ", mermaid_role::border);
            add_text_segment(segments, bar.view(), mermaid_role::text);
            add_text_segment(segments, " ", mermaid_role::plain);
            add_text_segment(segments, align_right_width(value_text.view(), value_width).view(), mermaid_role::text);
            add_text_segment(segments, "  ", mermaid_role::plain);
            add_text_segment(segments, align_right_width(percent_text.view(), 4).view(), mermaid_role::text);
            builder.add_line(segments);
        }
        builder.finish(art);
        return !art.lines.empty();
    }

    struct mindmap_node {
        string label;
        size_t parent = string_view::npos;
        vector<size_t> children;
        int depth = 0;
    };

    struct mindmap_data {
        vector<mindmap_node> nodes;
    };

    string mindmap_label(const string_view text) {
        string label = sanitize_label(text);
        const size_t icon = label.view().find("::icon(");
        if (icon != string_view::npos) {
            label = string(label.view().substr(0, icon));
        }
        const size_t css = label.view().find(":::");
        if (css != string_view::npos) {
            label = string(label.view().substr(0, css));
        }
        label = trim_copy(label.view());

        struct wrapper_info {
            const string_view open;
            const string_view close;
        };
        constexpr wrapper_info wrappers[] = {
                {"((", "))"}, {"{{", "}}"}, {"))", "(("}, {"[", "]"}, {"(", ")"}, {")", "("},
        };

        for (const wrapper_info& wrapper: wrappers) {
            const string_view open = wrapper.open;
            const string_view close = wrapper.close;
            const size_t open_position = label.view().find(open);
            if (open_position == string_view::npos || !label.view().ends_with(close)) {
                continue;
            }
            if (label.size() < open_position + open.size() + close.size()) {
                continue;
            }
            const string prefix = trim_copy(label.view().substr(0, open_position));
            if (!prefix.empty() && prefix.view().contains(' ')) {
                continue;
            }
            const string inner = trim_copy(label.view().substr(
                    open_position + open.size(), label.size() - open_position - open.size() - close.size()));
            if (!inner.empty()) {
                label = inner;
                break;
            }
        }
        return trim_copy(label.view());
    }

    bool build_mindmap(const mermaid_document& document, const header_info& header, mindmap_data& data) {
        (void) header;
        vector<size_t> node_stack;
        vector<int> indent_stack;
        for (size_t i = document.header + 1; i < document.lines.size(); ++i) {
            const string_view raw = strip_comment(document.lines[i]);
            if (raw.trim().empty()) {
                continue;
            }
            int indent = 0;
            size_t position = 0;
            while (position < raw.size() && (raw[position] == ' ' || raw[position] == '\t')) {
                indent += raw[position] == '\t' ? 4 : 1;
                indent = min(indent, 256);
                ++position;
            }
            const string label = mindmap_label(raw.substr(position));
            if (label.empty()) {
                continue;
            }
            if (data.nodes.size() >= MERMAID_MAX_NODES) {
                break;
            }
            if (data.nodes.empty()) {
                mindmap_node root;
                root.label = label;
                data.nodes.push_back(move(root));
                node_stack.push_back(0);
                indent_stack.push_back(indent);
                continue;
            }
            while (indent_stack.size() > 1 && indent <= indent_stack.back()) {
                node_stack.pop_back();
                indent_stack.pop_back();
            }
            const size_t parent = node_stack.back();
            if (compare_greater_equal(data.nodes[parent].depth, MERMAID_MAX_MINDMAP_DEPTH)) {
                continue;
            }
            mindmap_node node;
            node.label = label;
            node.parent = parent;
            node.depth = data.nodes[parent].depth + 1;
            data.nodes.push_back(move(node));
            const size_t index = data.nodes.size() - 1;
            data.nodes[parent].children.push_back(index);
            node_stack.push_back(index);
            indent_stack.push_back(indent);
        }
        return !data.nodes.empty();
    }

    bool layout_mindmap(const mindmap_data& data, mermaid_art& art) {
        if (data.nodes.empty()) {
            return false;
        }

        struct render_task {
            size_t index = 0;
            string prefix;
            bool last = true;
        };

        vector<render_task> stack;
        render_task root;
        root.index = 0;
        stack.push_back(move(root));
        text_art_builder builder;
        while (!stack.empty()) {
            const render_task task = move(stack.back());
            stack.pop_back();
            vector<text_segment> segments;
            if (task.index == 0) {
                add_text_segment(segments, data.nodes[0].label.view(), mermaid_role::text);
            } else {
                string connector = task.prefix;
                connector += task.last ? "└─ " : "├─ ";
                add_text_segment(segments, connector.view(), mermaid_role::edge);
                add_text_segment(segments, data.nodes[task.index].label.view(), mermaid_role::text);
            }
            builder.add_line(segments);
            string child_prefix = task.prefix;
            if (task.index != 0) {
                child_prefix += task.last ? "   " : "│  ";
            }
            const vector<size_t>& children = data.nodes[task.index].children;
            for (size_t i = children.size(); i > 0; --i) {
                render_task child;
                child.index = children[i - 1];
                child.prefix = child_prefix;
                child.last = i == children.size();
                stack.push_back(move(child));
            }
        }
        builder.finish(art);
        return !art.lines.empty();
    }

    struct timeline_entry {
        string period;
        string section;
        vector<string> events;
    };

    struct timeline_data {
        vector<timeline_entry> entries;
    };

    bool is_timeline_directive(const string_view text) noexcept {
        constexpr string_view directives[] = {"accTitle", "accDescr"};
        for (const string_view directive: directives) {
            if (matches_keyword(text, directive)) {
                return true;
            }
        }
        return false;
    }

    void append_timeline_events(timeline_entry& entry, const string_view body) {
        vector<string_view> parts;
        split_on_char(body, ':', parts);
        for (const string_view part: parts) {
            const string event = sanitize_label(part);
            if (!event.empty()) {
                entry.events.push_back(event);
            }
        }
    }

    bool build_timeline(const mermaid_document& document, const header_info& header, string& title,
                        timeline_data& data) {
        string current_section;
        const string_view first = header.remainder.trim();
        if (!first.empty()) {
            if (matches_keyword(first, "title")) {
                const string candidate = sanitize_label(first.substr(5));
                if (!candidate.empty() && title.empty()) {
                    title = candidate;
                }
            } else if (matches_keyword(first, "section")) {
                current_section = sanitize_label(first.substr(7));
            }
        }
        for (size_t i = document.header + 1; i < document.lines.size(); ++i) {
            const string_view text = strip_comment(document.lines[i]).trim();
            if (text.empty()) {
                continue;
            }
            if (take_title_line(text, title) || take_plain_title(text, title)) {
                continue;
            }
            if (is_timeline_directive(text)) {
                continue;
            }
            if (matches_keyword(text, "section")) {
                current_section = sanitize_label(text.substr(7));
                continue;
            }
            if (text.front() == ':') {
                if (!data.entries.empty()) {
                    append_timeline_events(data.entries.back(), text.substr(1));
                }
                continue;
            }
            const size_t colon = text.find(':');
            if (colon == string_view::npos) {
                continue;
            }
            if (data.entries.size() >= MERMAID_MAX_TIMELINE_ENTRIES) {
                continue;
            }
            timeline_entry entry;
            entry.period = sanitize_label(text.substr(0, colon));
            entry.section = current_section;
            if (entry.period.empty()) {
                continue;
            }
            append_timeline_events(entry, text.substr(colon + 1));
            if (entry.events.empty()) {
                continue;
            }
            data.entries.push_back(move(entry));
        }
        return !data.entries.empty();
    }

    void append_section_header(text_art_builder& builder, const string_view name) {
        vector<text_segment> segments;
        add_text_segment(segments, "── ", mermaid_role::edge);
        add_text_segment(segments, name, mermaid_role::text);
        add_text_segment(segments, " ──", mermaid_role::edge);
        builder.add_line(segments);
    }

    bool layout_timeline(const timeline_data& data, mermaid_art& art) {
        if (data.entries.empty()) {
            return false;
        }
        int period_width = 0;
        for (const timeline_entry& entry: data.entries) {
            period_width = max(period_width, text_display_width(entry.period.view()));
        }
        text_art_builder builder;
        string previous_section;
        bool started = false;
        for (const timeline_entry& entry: data.entries) {
            if (entry.section != previous_section) {
                if (!entry.section.empty()) {
                    if (started) {
                        builder.add_blank_line();
                    }
                    append_section_header(builder, entry.section.view());
                    started = true;
                }
                previous_section = entry.section;
            }
            for (size_t i = 0; i < entry.events.size(); ++i) {
                vector<text_segment> segments;
                if (i == 0) {
                    add_text_segment(segments, pad_to_width(entry.period.view(), period_width).view(),
                                     mermaid_role::text);
                } else {
                    add_text_segment(segments, repeat_char(static_cast<size_t>(period_width), ' ').view(),
                                     mermaid_role::plain);
                }
                add_text_segment(segments, " ─ ", mermaid_role::edge);
                add_text_segment(segments, entry.events[i].view(), mermaid_role::text);
                builder.add_line(segments);
                started = true;
            }
        }
        builder.finish(art);
        return !art.lines.empty();
    }

    int days_from_civil(int year, const int month, const int day) noexcept {
        year -= month <= 2 ? 1 : 0;
        const long long era = (year >= 0 ? year : year - 399) / 400;
        const auto yoe = static_cast<unsigned>(year - static_cast<int>(era) * 400);
        const auto doy = static_cast<unsigned>((153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1);
        const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
        return static_cast<int>(era * 146097 + static_cast<long long>(doe) - 719468);
    }

    void civil_from_days(const int days, int& year, int& month, int& day) noexcept {
        const int z = days + 719468;
        const int era = (z >= 0 ? z : z - 146096) / 146097;
        const auto doe = static_cast<unsigned>(z - era * 146097);
        const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
        const int y = static_cast<int>(yoe) + era * 400;
        const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
        const unsigned mp = (5 * doy + 2) / 153;
        const unsigned d = doy - (153 * mp + 2) / 5 + 1;
        const unsigned m = mp < 10 ? mp + 3 : mp - 9;
        day = static_cast<int>(d);
        month = static_cast<int>(m);
        year = y + (m <= 2 ? 1 : 0);
    }

    bool parse_iso_date(const string_view text, int& days) noexcept {
        if (text.size() != 10 || text[4] != '-' || text[7] != '-') {
            return false;
        }
        int year = 0;
        int month = 0;
        int day = 0;
        for (size_t i = 0; i < 4; ++i) {
            if (text[i] < '0' || text[i] > '9') {
                return false;
            }
            year = year * 10 + static_cast<int>(text[i] - '0');
        }
        for (size_t i = 5; i < 7; ++i) {
            if (text[i] < '0' || text[i] > '9') {
                return false;
            }
            month = month * 10 + static_cast<int>(text[i] - '0');
        }
        for (size_t i = 8; i < 10; ++i) {
            if (text[i] < '0' || text[i] > '9') {
                return false;
            }
            day = day * 10 + static_cast<int>(text[i] - '0');
        }
        if (year < 1 || month < 1 || month > 12 || day < 1 || day > 31) {
            return false;
        }
        days = days_from_civil(year, month, day);
        return true;
    }

    string format_iso_date(const int days) {
        int year = 1970;
        int month = 1;
        int day = 1;
        civil_from_days(days, year, month, day);
        string result;
        if (year < 0) {
            result += '-';
            year = -year;
        }
        result += zero_pad(year, 4);
        result += '-';
        result += zero_pad(month, 2);
        result += '-';
        result += zero_pad(day, 2);
        return result;
    }

    long long parse_gantt_duration(const string_view token) noexcept {
        if (token.size() < 2) {
            return -1;
        }
        size_t position = 0;
        long long value = 0;
        while (position < token.size() && token[position] >= '0' && token[position] <= '9') {
            value = value * 10 + static_cast<long long>(token[position] - '0');
            if (value > 1000000LL) {
                return -1;
            }
            ++position;
        }
        if (position == 0 || position + 1 != token.size()) {
            return -1;
        }
        const char unit = token[position];
        if (unit == 'd') {
            return value;
        }
        if (unit == 'w') {
            return value * 7;
        }
        if (unit == 'h') {
            return value == 0 ? 0 : (value + 23) / 24;
        }
        return -1;
    }

    struct gantt_task {
        string name;
        string section;
        int start = 0;
        int end = 0;
        string start_text;
        string end_text;
        bool milestone = false;
        bool done = false;
    };

    struct gantt_data {
        vector<gantt_task> tasks;
    };

    bool parse_gantt_task(const string_view text, const string& section, gantt_task& task) {
        const size_t colon = text.find(':');
        if (colon == string_view::npos) {
            return false;
        }
        task.name = sanitize_label(text.substr(0, colon));
        if (task.name.empty()) {
            return false;
        }
        vector<string_view> tokens;
        split_on_char(text.substr(colon + 1), ',', tokens);
        bool has_start = false;
        bool has_end = false;
        bool has_duration = false;
        long long duration = 0;
        string id;
        for (const string_view raw: tokens) {
            const string_view token = raw.trim();
            if (token.empty()) {
                continue;
            }
            const string lowered = upper_ascii(token);
            int parsed_days = 0;
            if (parse_iso_date(token, parsed_days)) {
                if (!has_start) {
                    task.start = parsed_days;
                    task.start_text = string(token);
                    has_start = true;
                } else if (!has_end) {
                    task.end = parsed_days;
                    task.end_text = string(token);
                    has_end = true;
                }
                continue;
            }
            const long long parsed_duration = parse_gantt_duration(token);
            if (parsed_duration >= 0) {
                duration = parsed_duration;
                has_duration = true;
                continue;
            }
            if (lowered.view() == "MILESTONE") {
                task.milestone = true;
                continue;
            }
            if (lowered.view() == "DONE" || lowered.view() == "ACTIVE" || lowered.view() == "CRIT") {
                if (lowered.view() == "DONE") {
                    task.done = true;
                }
                continue;
            }
            if (id.empty()) {
                id = string(token);
            }
        }
        if (!has_start) {
            return false;
        }
        if (!has_end) {
            if (!has_duration) {
                return false;
            }
            task.end = task.start + static_cast<int>(duration);
            task.end_text = format_iso_date(task.end);
        }
        if (task.end < task.start) {
            return false;
        }
        if (task.milestone) {
            task.end = task.start;
            task.end_text = task.start_text;
        }
        task.section = section;
        (void) id;
        return true;
    }

    bool is_gantt_directive(const string_view text) noexcept {
        constexpr string_view directives[] = {
                "dateFormat", "axisFormat", "excludes", "includes", "todayMarker", "tickInterval",
                "weekday",    "inclusive",  "click",    "accTitle", "accDescr",    "displayMode",
        };
        for (const string_view directive: directives) {
            if (matches_keyword(text, directive)) {
                return true;
            }
        }
        return false;
    }

    bool build_gantt(const mermaid_document& document, const header_info& header, string& title, gantt_data& data) {
        string current_section;
        const string_view first = header.remainder.trim();
        if (!first.empty() && matches_keyword(first, "title")) {
            const string candidate = sanitize_label(first.substr(5));
            if (!candidate.empty() && title.empty()) {
                title = candidate;
            }
        }
        for (size_t i = document.header + 1; i < document.lines.size(); ++i) {
            const string_view text = strip_comment(document.lines[i]).trim();
            if (text.empty()) {
                continue;
            }
            if (take_title_line(text, title) || take_plain_title(text, title)) {
                continue;
            }
            if (matches_keyword(text, "section")) {
                current_section = sanitize_label(text.substr(7));
                continue;
            }
            if (is_gantt_directive(text)) {
                continue;
            }
            if (data.tasks.size() >= MERMAID_MAX_GANTT_TASKS) {
                continue;
            }
            gantt_task task;
            if (parse_gantt_task(text, current_section, task)) {
                data.tasks.push_back(move(task));
            }
        }
        return !data.tasks.empty();
    }

    bool layout_gantt(const gantt_data& data, mermaid_art& art) {
        if (data.tasks.empty()) {
            return false;
        }
        int name_width = 0;
        int max_duration = 1;
        for (const gantt_task& task: data.tasks) {
            name_width = max(name_width, text_display_width(task.name.view()));
            if (!task.milestone) {
                max_duration = max(max_duration, task.end - task.start);
            }
        }
        if (max_duration <= 0) {
            max_duration = 1;
        }
        text_art_builder builder;
        string previous_section;
        bool started = false;
        for (const gantt_task& task: data.tasks) {
            if (task.section != previous_section) {
                if (!task.section.empty()) {
                    if (started) {
                        builder.add_blank_line();
                    }
                    append_section_header(builder, task.section.view());
                    started = true;
                }
                previous_section = task.section;
            }
            string bar;
            if (task.milestone) {
                bar += "◆";
                bar.append(static_cast<size_t>(MERMAID_GANTT_BAR_WIDTH - 1), ' ');
            } else {
                const int duration = task.end - task.start;
                int cells = 0;
                if (duration <= 0) {
                    cells = 1;
                } else {
                    cells = static_cast<int>(
                            (static_cast<long long>(duration) * MERMAID_GANTT_BAR_WIDTH * 2 + max_duration) /
                            (static_cast<long long>(max_duration) * 2));
                }
                cells = max(cells, 1);
                cells = min(cells, MERMAID_GANTT_BAR_WIDTH);
                bar = repeat_glyph(static_cast<size_t>(cells), "█");
                bar.append(static_cast<size_t>(MERMAID_GANTT_BAR_WIDTH - cells), ' ');
            }
            string dates = task.start_text;
            if (!task.milestone) {
                dates += " → ";
                dates += task.end_text;
            }
            vector<text_segment> segments;
            add_text_segment(segments, pad_to_width(task.name.view(), name_width).view(), mermaid_role::text);
            add_text_segment(segments, "  ", mermaid_role::plain);
            add_text_segment(segments, bar.view(), mermaid_role::text);
            add_text_segment(segments, "  ", mermaid_role::plain);
            add_text_segment(segments, dates.view(), mermaid_role::edge_label);
            builder.add_line(segments);
            started = true;
        }
        builder.finish(art);
        return !art.lines.empty();
    }

    struct journey_entry {
        bool is_section = false;
        string name;
        string actor;
        string section;
        int score = 0;
    };

    struct journey_data {
        vector<journey_entry> entries;
        size_t task_count = 0;
    };

    bool is_journey_directive(const string_view text) noexcept {
        constexpr string_view directives[] = {"accTitle", "accDescr"};
        for (const string_view directive: directives) {
            if (matches_keyword(text, directive)) {
                return true;
            }
        }
        return false;
    }

    bool build_journey(const mermaid_document& document, const header_info& header, string& title, journey_data& data) {
        string current_section;
        const string_view first = header.remainder.trim();
        if (!first.empty() && matches_keyword(first, "title")) {
            const string candidate = sanitize_label(first.substr(5));
            if (!candidate.empty() && title.empty()) {
                title = candidate;
            }
        }
        for (size_t i = document.header + 1; i < document.lines.size(); ++i) {
            const string_view text = strip_comment(document.lines[i]).trim();
            if (text.empty()) {
                continue;
            }
            if (take_title_line(text, title) || take_plain_title(text, title)) {
                continue;
            }
            if (is_journey_directive(text)) {
                continue;
            }
            if (matches_keyword(text, "section")) {
                current_section = sanitize_label(text.substr(7));
                if (data.entries.size() < MERMAID_MAX_JOURNEY_ENTRIES) {
                    journey_entry entry;
                    entry.is_section = true;
                    entry.name = current_section;
                    data.entries.push_back(move(entry));
                }
                continue;
            }
            vector<string_view> parts;
            split_on_char(text, ':', parts);
            if (parts.size() < 2) {
                continue;
            }
            journey_entry entry;
            entry.name = sanitize_label(parts[0]);
            if (entry.name.empty()) {
                continue;
            }
            long long score = 0;
            if (!parse_integer(parts[1], score) || score < 1 || score > 5) {
                continue;
            }
            entry.score = static_cast<int>(score);
            if (parts.size() > 2) {
                entry.actor = sanitize_label(parts[2]);
            }
            entry.section = current_section;
            if (data.entries.size() >= MERMAID_MAX_JOURNEY_ENTRIES) {
                continue;
            }
            data.entries.push_back(move(entry));
            ++data.task_count;
        }
        return data.task_count > 0;
    }

    bool layout_journey(const journey_data& data, mermaid_art& art) {
        if (data.entries.empty() || data.task_count == 0) {
            return false;
        }
        int name_width = 0;
        for (const journey_entry& entry: data.entries) {
            if (!entry.is_section) {
                name_width = max(name_width, text_display_width(entry.name.view()));
            }
        }
        text_art_builder builder;
        bool started = false;
        for (const journey_entry& entry: data.entries) {
            if (entry.is_section) {
                if (!entry.name.empty()) {
                    if (started) {
                        builder.add_blank_line();
                    }
                    append_section_header(builder, entry.name.view());
                    started = true;
                }
                continue;
            }
            string bar = repeat_glyph(static_cast<size_t>(entry.score), "█");
            bar += repeat_glyph(static_cast<size_t>(5 - entry.score), "░");
            const string score_text = format_integer(entry.score);
            vector<text_segment> segments;
            add_text_segment(segments, pad_to_width(entry.name.view(), name_width).view(), mermaid_role::text);
            add_text_segment(segments, "  ", mermaid_role::plain);
            add_text_segment(segments, bar.view(), mermaid_role::text);
            add_text_segment(segments, "  ", mermaid_role::plain);
            add_text_segment(segments, score_text.view(), mermaid_role::text);
            if (!entry.actor.empty()) {
                string actor = "  (";
                actor += entry.actor;
                actor += ')';
                add_text_segment(segments, actor.view(), mermaid_role::edge_label);
            }
            builder.add_line(segments);
            started = true;
        }
        builder.finish(art);
        return !art.lines.empty();
    }

    struct quadrant_point {
        string label;
        double x = 0.0;
        double y = 0.0;
    };

    struct quadrant_data {
        string x_low;
        string x_high;
        string y_low;
        string y_high;
        vector<quadrant_point> points;
    };

    void parse_quadrant_axis(const string_view text, string& low, string& high) {
        const size_t arrow = text.find("-->");
        if (arrow == string_view::npos) {
            return;
        }
        const string parsed_low = sanitize_label(text.substr(0, arrow));
        const string parsed_high = sanitize_label(text.substr(arrow + 3));
        if (!parsed_low.empty()) {
            low = parsed_low;
        }
        if (!parsed_high.empty()) {
            high = parsed_high;
        }
    }

    bool parse_quadrant_point(const string_view text, quadrant_point& point) {
        const size_t open = text.find('[');
        if (open == string_view::npos) {
            return false;
        }
        const size_t close = text.find(']', open + 1);
        if (close == string_view::npos) {
            return false;
        }
        string_view label = text.substr(0, open).trim();
        if (!label.empty() && label.back() == ':') {
            label = label.substr(0, label.size() - 1).trim();
        }
        string name = sanitize_label(label);
        if (name.empty()) {
            return false;
        }
        vector<string_view> coordinates;
        split_on_char(text.substr(open + 1, close - open - 1), ',', coordinates);
        if (coordinates.size() < 2) {
            return false;
        }
        double x = 0.0;
        double y = 0.0;
        if (!parse_fraction(coordinates[0], x) || !parse_fraction(coordinates[1], y)) {
            return false;
        }
        point.label = move(name);
        point.x = x < 0.0 ? 0.0 : (x > 1.0 ? 1.0 : x);
        point.y = y < 0.0 ? 0.0 : (y > 1.0 ? 1.0 : y);
        return true;
    }

    bool is_quadrant_directive(const string_view text) noexcept {
        constexpr string_view directives[] = {"accTitle", "accDescr"};
        for (const string_view directive: directives) {
            if (matches_keyword(text, directive)) {
                return true;
            }
        }
        return false;
    }

    bool build_quadrant_chart(const mermaid_document& document, const header_info& header, string& title,
                              quadrant_data& data) {
        (void) header;
        for (size_t i = document.header + 1; i < document.lines.size(); ++i) {
            const string_view text = strip_comment(document.lines[i]).trim();
            if (text.empty()) {
                continue;
            }
            if (take_title_line(text, title) || take_plain_title(text, title)) {
                continue;
            }
            if (is_quadrant_directive(text)) {
                continue;
            }
            if (matches_keyword(text, "x-axis")) {
                parse_quadrant_axis(text.substr(6), data.x_low, data.x_high);
                continue;
            }
            if (matches_keyword(text, "y-axis")) {
                parse_quadrant_axis(text.substr(6), data.y_low, data.y_high);
                continue;
            }
            if (text.starts_with("quadrant-")) {
                continue;
            }
            quadrant_point point;
            if (parse_quadrant_point(text, point)) {
                if (data.points.size() < MERMAID_MAX_QUADRANT_POINTS) {
                    data.points.push_back(move(point));
                }
            }
        }
        return !data.points.empty();
    }

    bool layout_quadrant_chart(const quadrant_data& data, mermaid_art& art) {
        if (data.points.empty()) {
            return false;
        }
        constexpr int inner_width = MERMAID_QUADRANT_WIDTH;
        constexpr int inner_height = MERMAID_QUADRANT_HEIGHT;
        constexpr int total_width = inner_width + 2;
        const bool show_y = !data.y_low.empty() || !data.y_high.empty();
        const bool show_x = !data.x_low.empty() || !data.x_high.empty();
        const int top = show_y ? 1 : 0;
        constexpr auto border = mermaid_role::border;
        constexpr auto edge = mermaid_role::edge;

        art_canvas canvas;
        if (show_y) {
            string label = "y: ";
            label += data.y_low.empty() ? string("0") : data.y_low;
            label += " → ";
            label += data.y_high.empty() ? string("1") : data.y_high;
            canvas.put_text(0, 0, label.view(), mermaid_role::text);
        }
        for (int x = 0; x < total_width; ++x) {
            canvas.put(top, x, x == 0 ? "┌" : (x == total_width - 1 ? "┐" : "─"), border);
        }
        for (int row = 1; row <= inner_height; ++row) {
            canvas.put(top + row, 0, "│", border);
            canvas.put(top + row, total_width - 1, "│", border);
        }
        const int bottom_row = top + inner_height + 1;
        for (int x = 0; x < total_width; ++x) {
            canvas.put(bottom_row, x, x == 0 ? "└" : (x == total_width - 1 ? "┘" : "─"), border);
        }

        constexpr int center_column = 1 + inner_width / 2;
        const int center_row = top + 1 + inner_height / 2;
        for (int row = top + 1; row <= top + inner_height; ++row) {
            if (row != center_row) {
                canvas.put(row, center_column, "┆", edge);
            }
        }
        for (int column = 1; column <= inner_width; ++column) {
            if (column != center_column) {
                canvas.put(center_row, column, "┄", edge);
            }
        }
        canvas.put(center_row, center_column, "┼", edge);

        for (const quadrant_point& point: data.points) {
            int column = 1 + static_cast<int>(round(point.x * static_cast<double>(inner_width - 1)));
            int row = top + 1 + static_cast<int>(round((1.0 - point.y) * static_cast<double>(inner_height - 1)));
            column = max(column, 1);
            column = min(column, inner_width);
            row = max(row, top + 1);
            row = min(row, top + inner_height);
            canvas.put(row, column, "●", mermaid_role::text);
            const int available = inner_width - (column - 1) - 2;
            if (available > 0) {
                const string label = truncate_display(point.label.view(), available);
                if (!label.empty()) {
                    canvas.put_text(row, column + 2, label.view(), mermaid_role::text);
                }
            }
        }
        if (show_x) {
            string label = "x: ";
            label += data.x_low.empty() ? string("0") : data.x_low;
            label += " → ";
            label += data.x_high.empty() ? string("1") : data.x_high;
            canvas.put_text(bottom_row + 1, 0, label.view(), mermaid_role::text);
        }
        art = build_art(canvas);
        return !art.lines.empty();
    }

    struct requirement_block {
        string name;
        vector<string> fields;
    };

    struct requirement_data {
        vector<requirement_block> blocks;
    };

    bool is_requirement_keyword(const string_view text, size_t& keyword_size) noexcept {
        constexpr string_view keywords[] = {
                "functionalRequirement", "performanceRequirement", "interfaceRequirement",
                "physicalRequirement",   "designConstraint",       "requirement",
        };
        for (const string_view keyword: keywords) {
            if (matches_keyword(text, keyword)) {
                keyword_size = keyword.size();
                return true;
            }
        }
        return false;
    }

    void split_requirement_fields(const string_view body, requirement_block& block) {
        struct key_position {
            string_view key = "";
            size_t position = 0;
        };
        constexpr string_view keys[] = {"id:", "text:", "risk:", "verifymethod:"};
        key_position found[4];
        size_t found_count = 0;
        for (const string_view key: keys) {
            const size_t position = body.find(key);
            if (position == string_view::npos) {
                continue;
            }
            if (position > 0 && !is_space(body[position - 1])) {
                continue;
            }
            found[found_count].key = key;
            found[found_count].position = position;
            ++found_count;
        }
        if (found_count == 0) {
            const string field = sanitize_label(body);
            if (!field.empty() && block.fields.size() < MERMAID_MAX_ER_ATTRIBUTES) {
                block.fields.push_back(field);
            }
            return;
        }
        for (size_t i = 1; i < found_count; ++i) {
            const key_position current = found[i];
            size_t position = i;
            while (position > 0 && found[position - 1].position > current.position) {
                found[position] = found[position - 1];
                --position;
            }
            found[position] = current;
        }
        for (size_t i = 0; i < found_count && block.fields.size() < MERMAID_MAX_ER_ATTRIBUTES; ++i) {
            const size_t key_length = string_view(found[i].key).size();
            const size_t value_begin = found[i].position + key_length;
            const size_t value_end = i + 1 < found_count ? found[i + 1].position : body.size();
            const string value = sanitize_label(body.substr(value_begin, value_end - value_begin));
            string field(found[i].key);
            if (!value.empty()) {
                field += ' ';
                field += value;
            }
            block.fields.push_back(move(field));
        }
    }

    void process_requirement_line(const string_view line, requirement_data& data, size_t& body_index, string& title) {
        if (body_index != string_view::npos) {
            const size_t close = line.find('}');
            const string field = sanitize_label(close == string_view::npos ? line : line.substr(0, close));
            if (!field.empty() && body_index < data.blocks.size()) {
                if (data.blocks[body_index].fields.size() < MERMAID_MAX_ER_ATTRIBUTES) {
                    data.blocks[body_index].fields.push_back(field);
                }
            }
            if (close != string_view::npos) {
                body_index = string_view::npos;
            }
            return;
        }
        const string_view text = line.trim();
        if (text.empty()) {
            return;
        }
        if (take_title_line(text, title) || take_plain_title(text, title)) {
            return;
        }
        size_t keyword_size = 0;
        if (!is_requirement_keyword(text, keyword_size)) {
            return;
        }
        const string_view rest = text.substr(keyword_size).trim();
        const size_t brace = rest.find('{');
        string name = sanitize_label(brace == string_view::npos ? rest : rest.substr(0, brace));
        const size_t space = name.view().find(' ');
        if (space != string_view::npos) {
            name = string(name.view().substr(0, space));
        }
        if (name.empty() || data.blocks.size() >= MERMAID_MAX_REQUIREMENTS) {
            return;
        }
        requirement_block block;
        block.name = name;
        if (brace != string_view::npos) {
            const size_t close = rest.find('}', brace + 1);
            if (close == string_view::npos) {
                data.blocks.push_back(move(block));
                body_index = data.blocks.size() - 1;
                const string field = sanitize_label(rest.substr(brace + 1));
                if (!field.empty()) {
                    data.blocks[body_index].fields.push_back(field);
                }
                return;
            }
            const string_view body = rest.substr(brace + 1, close - brace - 1);
            vector<string_view> parts;
            split_on_char(body, ';', parts);
            if (parts.size() > 1) {
                for (const string_view part: parts) {
                    const string field = sanitize_label(part);
                    if (!field.empty() && block.fields.size() < MERMAID_MAX_ER_ATTRIBUTES) {
                        block.fields.push_back(field);
                    }
                }
            } else {
                split_requirement_fields(body, block);
            }
        }
        data.blocks.push_back(move(block));
    }

    bool build_requirement_diagram(const mermaid_document& document, const header_info& header, string& title,
                                   requirement_data& data) {
        size_t body_index = string_view::npos;
        (void) header;
        for (size_t i = document.header + 1; i < document.lines.size(); ++i) {
            process_requirement_line(strip_comment(document.lines[i]), data, body_index, title);
        }
        return !data.blocks.empty();
    }

    bool layout_requirement_diagram(const requirement_data& data, mermaid_art& art) {
        if (data.blocks.empty()) {
            return false;
        }
        art_canvas canvas;
        int top = 0;
        for (const requirement_block& block: data.blocks) {
            stacked_box box;
            box.title = block.name;
            box.members = block.fields;
            box.left = 0;
            box.top = top;
            box.width = max(8, stacked_box_content_width(box.title.view(), box.members) + 4);
            draw_stacked_box(canvas, box);
            top += stacked_box_height(box) + 1;
        }
        art = build_art(canvas);
        return !art.lines.empty();
    }

    struct diagram_node {
        string id;
        string label;
        node_shape shape = node_shape::rectangle;
        int rank = 0;
        int left = 0;
        int top = 0;
        int width = 0;
    };

    struct diagram_edge {
        size_t from = 0;
        size_t to = 0;
        string label;
        edge_style style = edge_style::solid;
        bool arrow = true;
    };

    struct diagram_graph {
        vector<diagram_node> nodes;
        vector<diagram_edge> edges;
    };

    size_t find_node(const diagram_graph& graph, const string_view id) noexcept {
        for (size_t i = 0; i < graph.nodes.size(); ++i) {
            if (graph.nodes[i].id.view() == id) {
                return i;
            }
        }
        return string_view::npos;
    }

    size_t ensure_node(diagram_graph& graph, const string_view id, const string_view label, const node_shape shape,
                       const bool explicit_declaration) {
        if (id.empty()) {
            return string_view::npos;
        }
        const size_t existing = find_node(graph, id);
        if (existing != string_view::npos) {
            if (explicit_declaration) {
                graph.nodes[existing].label = sanitize_label(label);
                graph.nodes[existing].shape = shape;
            }
            return existing;
        }
        if (graph.nodes.size() >= MERMAID_MAX_NODES) {
            return string_view::npos;
        }
        diagram_node node;
        node.id = string(id);
        node.label = explicit_declaration ? sanitize_label(label) : sanitize_label(id);
        node.shape = shape;
        graph.nodes.push_back(move(node));
        return graph.nodes.size() - 1;
    }

    struct parsed_node_ref {
        string id;
        string label;
        node_shape shape = node_shape::rectangle;
        bool explicit_declaration = false;
    };

    bool parse_node_ref(const string_view text, size_t& position, parsed_node_ref& result) {
        position = skip_spaces(text, position);
        const size_t start = position;
        while (position < text.size() && is_id_char(text[position])) {
            ++position;
        }
        if (position == start) {
            return false;
        }
        result.id = string(text.substr(start, position - start));
        result.label = result.id;
        result.shape = node_shape::rectangle;
        result.explicit_declaration = false;
        if (position >= text.size()) {
            return true;
        }
        const char open = text[position];
        if (open == '(') {
            if (position + 1 < text.size() && text[position + 1] == '(') {
                const size_t close = text.find("))", position + 2);
                if (close == string_view::npos) {
                    return false;
                }
                result.label = trim_copy(text.substr(position + 2, close - position - 2));
                result.shape = node_shape::circle;
                result.explicit_declaration = true;
                position = close + 2;
            } else {
                const size_t close = text.find(')', position + 1);
                if (close == string_view::npos) {
                    return false;
                }
                result.label = trim_copy(text.substr(position + 1, close - position - 1));
                result.shape = node_shape::rounded;
                result.explicit_declaration = true;
                position = close + 1;
            }
        } else if (open == '[') {
            const size_t close = text.find(']', position + 1);
            if (close == string_view::npos) {
                return false;
            }
            result.label = trim_copy(text.substr(position + 1, close - position - 1));
            result.shape = node_shape::rectangle;
            result.explicit_declaration = true;
            position = close + 1;
        } else if (open == '{') {
            const size_t close = text.find('}', position + 1);
            if (close == string_view::npos) {
                return false;
            }
            result.label = trim_copy(text.substr(position + 1, close - position - 1));
            result.shape = node_shape::diamond;
            result.explicit_declaration = true;
            position = close + 1;
        }
        if (result.explicit_declaration) {
            result.label = sanitize_label(result.label.view());
        }
        return true;
    }

    edge_style classify_edge_style(const string_view marker) noexcept {
        if (marker.contains('=')) {
            return edge_style::thick;
        }
        if (marker.contains('.')) {
            return edge_style::dotted;
        }
        return edge_style::solid;
    }

    bool parse_edge_operator(const string_view text, size_t& position, string& label, edge_style& style, bool& arrow) {
        position = skip_spaces(text, position);
        const size_t start = position;
        while (position < text.size() && is_arrow_char(text[position])) {
            ++position;
        }
        if (position == start) {
            return false;
        }
        const string_view marker = text.substr(start, position - start);
        const bool has_head = marker.back() == '>' || marker.back() == '<';
        if (!has_head) {
            size_t search = position;
            while (search < text.size()) {
                if (!is_arrow_char(text[search])) {
                    ++search;
                    continue;
                }
                size_t tail_end = search;
                while (tail_end < text.size() && is_arrow_char(text[tail_end])) {
                    ++tail_end;
                }
                const string_view tail = text.substr(search, tail_end - search);
                const bool tail_head = tail.back() == '>' || tail.back() == '<';
                if (tail_head || tail.size() >= 3) {
                    size_t label_begin = position;
                    while (label_begin < search && is_space(text[label_begin])) {
                        ++label_begin;
                    }
                    size_t label_end = search;
                    while (label_end > label_begin && is_space(text[label_end - 1])) {
                        --label_end;
                    }
                    if (label_end > label_begin) {
                        label = trim_copy(text.substr(label_begin, label_end - label_begin));
                        style = classify_edge_style(tail);
                        arrow = tail_head;
                        position = tail_end;
                        const size_t after = skip_spaces(text, position);
                        if (after < text.size() && text[after] == '|') {
                            const size_t close = text.find('|', after + 1);
                            if (close == string_view::npos) {
                                return false;
                            }
                            const string post = trim_copy(text.substr(after + 1, close - after - 1));
                            if (!post.empty()) {
                                label = post;
                            }
                            position = close + 1;
                        }
                        return true;
                    }
                }
                search = tail_end;
            }
        }
        style = classify_edge_style(marker);
        arrow = has_head;
        const size_t after = skip_spaces(text, position);
        if (after < text.size() && text[after] == '|') {
            const size_t close = text.find('|', after + 1);
            if (close == string_view::npos) {
                return false;
            }
            label = trim_copy(text.substr(after + 1, close - after - 1));
            position = close + 1;
        }
        return true;
    }

    size_t ensure_local_node(vector<parsed_node_ref>& nodes, const parsed_node_ref& value) {
        for (size_t i = 0; i < nodes.size(); ++i) {
            if (nodes[i].id.view() == value.id.view()) {
                if (value.explicit_declaration) {
                    nodes[i].label = value.label;
                    nodes[i].shape = value.shape;
                    nodes[i].explicit_declaration = true;
                }
                return i;
            }
        }
        if (nodes.size() >= MERMAID_MAX_NODES) {
            return string_view::npos;
        }
        nodes.push_back(value);
        return nodes.size() - 1;
    }

    struct local_edge {
        size_t from = 0;
        size_t to = 0;
        string label;
        edge_style style = edge_style::solid;
        bool arrow = true;
    };

    bool commit_statement(const vector<parsed_node_ref>& local_nodes, const vector<local_edge>& local_edges,
                          diagram_graph& graph) {
        if (local_nodes.empty() && local_edges.empty()) {
            return false;
        }
        if (graph.nodes.size() + local_nodes.size() > MERMAID_MAX_NODES ||
            graph.edges.size() + local_edges.size() > MERMAID_MAX_EDGES) {
            return false;
        }
        vector<size_t> mapping;
        mapping.reserve(local_nodes.size());
        for (const parsed_node_ref& local: local_nodes) {
            const size_t index =
                    ensure_node(graph, local.id.view(), local.label.view(), local.shape, local.explicit_declaration);
            if (index == string_view::npos) {
                return false;
            }
            mapping.push_back(index);
        }
        for (const local_edge& local: local_edges) {
            if (local.from >= mapping.size() || local.to >= mapping.size()) {
                return false;
            }
            diagram_edge edge;
            edge.from = mapping[local.from];
            edge.to = mapping[local.to];
            edge.label = local.label;
            edge.style = local.style;
            edge.arrow = local.arrow;
            graph.edges.push_back(move(edge));
        }
        return true;
    }

    bool parse_flow_statement(const string_view statement, diagram_graph& graph) {
        size_t position = 0;
        parsed_node_ref first;
        if (!parse_node_ref(statement, position, first)) {
            return false;
        }
        vector<parsed_node_ref> local_nodes;
        vector<local_edge> local_edges;
        const size_t first_index = ensure_local_node(local_nodes, first);
        if (first_index == string_view::npos) {
            return false;
        }
        size_t current = first_index;
        while (true) {
            position = skip_spaces(statement, position);
            if (position >= statement.size()) {
                break;
            }
            string label;
            auto style = edge_style::solid;
            bool arrow = true;
            if (!parse_edge_operator(statement, position, label, style, arrow)) {
                return false;
            }
            parsed_node_ref target;
            if (!parse_node_ref(statement, position, target)) {
                return false;
            }
            const size_t target_index = ensure_local_node(local_nodes, target);
            if (target_index == string_view::npos) {
                return false;
            }
            local_edge edge;
            edge.from = current;
            edge.to = target_index;
            edge.label = label;
            edge.style = style;
            edge.arrow = arrow;
            local_edges.push_back(move(edge));
            current = target_index;
            if (local_edges.size() > MERMAID_MAX_EDGES) {
                return false;
            }
        }
        return commit_statement(local_nodes, local_edges, graph);
    }

    bool handle_direction_line(const string_view text, flow_direction& direction) {
        if (!matches_keyword(text, "direction")) {
            return false;
        }
        const string_view rest = text.substr(9).trim();
        if (rest.empty()) {
            return true;
        }
        const size_t end = rest.find_first_of(" \t;");
        const string_view token = end == string_view::npos ? rest : rest.substr(0, end);
        parse_direction_token(token, direction);
        return true;
    }

    void process_flow_line(const string_view line, diagram_graph& graph, flow_direction& direction, string& title) {
        vector<string_view> statements;
        split_statements(line, statements);
        for (const string_view statement: statements) {
            const string_view text = statement.trim();
            if (text.empty()) {
                continue;
            }
            if (take_title_line(text, title)) {
                continue;
            }
            if (handle_direction_line(text, direction)) {
                continue;
            }
            if (is_directive_line(text)) {
                continue;
            }
            parse_flow_statement(text, graph);
        }
    }

    void assign_ranks(diagram_graph& graph) {
        const size_t count = graph.nodes.size();
        vector<vector<size_t>> outgoing(count);
        vector<int> indegree(count, 0);
        for (const diagram_edge& edge: graph.edges) {
            if (edge.from >= count || edge.to >= count) {
                continue;
            }
            outgoing[edge.from].push_back(edge.to);
            ++indegree[edge.to];
        }

        vector<int> ranks(count, -1);
        vector<size_t> queue;
        queue.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            if (indegree[i] == 0) {
                ranks[i] = 0;
                queue.push_back(i);
            }
        }
        size_t head = 0;
        while (head < queue.size()) {
            const size_t node = queue[head++];
            for (const size_t next: outgoing[node]) {
                if (ranks[next] < 0) {
                    ranks[next] = ranks[node] + 1;
                    queue.push_back(next);
                }
            }
        }
        for (size_t i = 0; i < count; ++i) {
            if (ranks[i] < 0) {
                ranks[i] = 0;
                queue.clear();
                queue.push_back(i);
                head = 0;
                while (head < queue.size()) {
                    const size_t node = queue[head++];
                    for (const size_t next: outgoing[node]) {
                        if (ranks[next] < 0) {
                            ranks[next] = ranks[node] + 1;
                            queue.push_back(next);
                        }
                    }
                }
            }
        }
        for (size_t i = 0; i < count; ++i) {
            graph.nodes[i].rank = ranks[i];
        }
    }

    void draw_node_box(art_canvas& canvas, const diagram_node& node) {
        string_view top_left = "┌";
        string_view top_right = "┐";
        string_view bottom_left = "└";
        string_view bottom_right = "┘";
        switch (node.shape) {
            case node_shape::rounded:
                top_left = "╭";
                top_right = "╮";
                bottom_left = "╰";
                bottom_right = "╯";
                break;
            case node_shape::circle:
                top_left = "╭";
                top_right = "╮";
                bottom_left = "╰";
                bottom_right = "╯";
                break;
            case node_shape::diamond:
                top_left = "╱";
                top_right = "╲";
                bottom_left = "╲";
                bottom_right = "╱";
                break;
            case node_shape::rectangle:
            default:
                break;
        }

        const int right = node.left + node.width - 1;
        const int bottom = node.top + 2;
        constexpr auto border = mermaid_role::border;
        canvas.put(node.top, node.left, top_left, border);
        for (int x = node.left + 1; x < right; ++x) {
            canvas.put(node.top, x, "─", border);
        }
        canvas.put(node.top, right, top_right, border);
        canvas.put(node.top + 1, node.left, "│", border);
        canvas.put_text(node.top + 1, node.left + 2, node.label.view(), mermaid_role::text);
        canvas.put(node.top + 1, right, "│", border);
        canvas.put(bottom, node.left, bottom_left, border);
        for (int x = node.left + 1; x < right; ++x) {
            canvas.put(bottom, x, "─", border);
        }
        canvas.put(bottom, right, bottom_right, border);
    }

    void draw_stacked_edge(art_canvas& canvas, const diagram_node& from, const diagram_node& to,
                           const diagram_edge& edge) {
        const int source_x = from.left + from.width / 2;
        const int target_x = to.left + to.width / 2;
        const int bottom = from.top + 2;
        constexpr auto role = mermaid_role::edge;
        canvas.add_connection(bottom, source_x, DIR_DOWN, edge.style, role);
        canvas.add_connection(bottom + 1, source_x, DIR_UP | DIR_DOWN, edge.style, role);
        canvas.add_connection(bottom + 2, source_x, DIR_UP | DIR_DOWN, edge.style, role);
        if (source_x == target_x) {
            canvas.add_connection(bottom + 3, source_x, DIR_UP | DIR_DOWN, edge.style, role);
        } else {
            const bool towards_right = target_x > source_x;
            const uint8_t source_direction = towards_right ? DIR_RIGHT : DIR_LEFT;
            const uint8_t target_direction = towards_right ? DIR_LEFT : DIR_RIGHT;
            const int step = towards_right ? 1 : -1;
            canvas.add_connection(bottom + 3, source_x, DIR_UP | source_direction, edge.style, role);
            for (int x = source_x + step; x != target_x; x += step) {
                canvas.add_connection(bottom + 3, x, DIR_LEFT | DIR_RIGHT, edge.style, role);
            }
            canvas.add_connection(bottom + 3, target_x, target_direction | DIR_DOWN, edge.style, role);
        }
        if (edge.arrow) {
            canvas.put(bottom + 4, target_x, "▼", role);
        } else {
            canvas.add_connection(bottom + 4, target_x, DIR_UP | DIR_DOWN, edge.style, role);
        }
        canvas.add_connection(to.top, target_x, DIR_UP, edge.style, role);
        if (!edge.label.empty()) {
            canvas.put_edge_label(bottom + 2, target_x + 2, bottom + 1, source_x + 2, edge.label.view());
        }
    }

    void draw_stacked_edge_up(art_canvas& canvas, const diagram_node& from, const diagram_node& to,
                              const diagram_edge& edge) {
        const int source_x = from.left + from.width / 2;
        const int target_x = to.left + to.width / 2;
        const int source_top = from.top;
        const int target_bottom = to.top + 2;
        constexpr auto role = mermaid_role::edge;
        canvas.add_connection(source_top, source_x, DIR_UP, edge.style, role);
        canvas.add_connection(source_top - 1, source_x, DIR_UP | DIR_DOWN, edge.style, role);
        canvas.add_connection(source_top - 2, source_x, DIR_UP | DIR_DOWN, edge.style, role);
        if (source_x == target_x) {
            canvas.add_connection(source_top - 3, source_x, DIR_UP | DIR_DOWN, edge.style, role);
        } else {
            const bool towards_left = target_x < source_x;
            const uint8_t source_direction = towards_left ? DIR_LEFT : DIR_RIGHT;
            const uint8_t target_direction = towards_left ? DIR_RIGHT : DIR_LEFT;
            const int step = towards_left ? -1 : 1;
            canvas.add_connection(source_top - 3, source_x, DIR_DOWN | source_direction, edge.style, role);
            for (int x = source_x + step; x != target_x; x += step) {
                canvas.add_connection(source_top - 3, x, DIR_LEFT | DIR_RIGHT, edge.style, role);
            }
            canvas.add_connection(source_top - 3, target_x, target_direction | DIR_UP, edge.style, role);
        }
        if (edge.arrow) {
            canvas.put(source_top - 4, target_x, "▲", role);
        } else {
            canvas.add_connection(source_top - 4, target_x, DIR_UP | DIR_DOWN, edge.style, role);
        }
        canvas.add_connection(target_bottom, target_x, DIR_DOWN, edge.style, role);
        if (!edge.label.empty()) {
            canvas.put_edge_label(source_top - 2, target_x + 2, source_top - 1, source_x + 2, edge.label.view());
        }
    }

    void draw_same_rank_edge(art_canvas& canvas, const diagram_node& from, const diagram_node& to,
                             const diagram_edge& edge) {
        if (from.left == to.left) {
            return;
        }
        const int row = from.top + 1;
        constexpr auto role = mermaid_role::edge;
        if (from.left < to.left) {
            const int source_x = from.left + from.width - 1;
            const int target_x = to.left;
            const int gap = target_x - source_x - 1;
            if (gap < 2) {
                return;
            }
            canvas.add_connection(row, source_x, DIR_RIGHT, edge.style, role);
            for (int x = source_x + 1; x < target_x - 1; ++x) {
                canvas.add_connection(row, x, DIR_LEFT | DIR_RIGHT, edge.style, role);
            }
            if (edge.arrow) {
                canvas.put(row, target_x - 1, "▶", role);
            } else {
                canvas.add_connection(row, target_x - 1, DIR_LEFT | DIR_RIGHT, edge.style, role);
                canvas.add_connection(row, target_x, DIR_LEFT, edge.style, role);
            }
            if (!edge.label.empty()) {
                const int label_width = text_display_width(edge.label.view());
                if (label_width + 2 <= gap) {
                    canvas.put_edge_label(row - 1, source_x + 2, row + 1, source_x + 2, edge.label.view());
                }
            }
        } else {
            const int source_x = from.left;
            const int target_x = to.left + to.width - 1;
            const int gap = source_x - target_x - 1;
            if (gap < 2) {
                return;
            }
            canvas.add_connection(row, source_x, DIR_LEFT, edge.style, role);
            for (int x = target_x + 1; x < source_x - 1; ++x) {
                canvas.add_connection(row, x, DIR_LEFT | DIR_RIGHT, edge.style, role);
            }
            if (edge.arrow) {
                canvas.put(row, target_x + 1, "◀", role);
            } else {
                canvas.add_connection(row, target_x + 1, DIR_LEFT | DIR_RIGHT, edge.style, role);
                canvas.add_connection(row, target_x, DIR_RIGHT, edge.style, role);
            }
            if (!edge.label.empty()) {
                const int label_width = text_display_width(edge.label.view());
                if (label_width + 2 <= gap) {
                    canvas.put_edge_label(row - 1, target_x + 2, row + 1, target_x + 2, edge.label.view());
                }
            }
        }
    }

    void draw_horizontal_edge(art_canvas& canvas, const diagram_node& from, const diagram_node& to,
                              const diagram_edge& edge, const int column_gap) {
        const int source_y = from.top + 1;
        const int target_y = to.top + 1;
        const int source_x = from.left + from.width - 1;
        const int target_x = to.left;
        constexpr auto role = mermaid_role::edge;
        canvas.add_connection(source_y, source_x, DIR_RIGHT, edge.style, role);
        if (source_y == target_y) {
            for (int x = source_x + 1; x < target_x - 1; ++x) {
                canvas.add_connection(source_y, x, DIR_LEFT | DIR_RIGHT, edge.style, role);
            }
            if (edge.arrow) {
                canvas.put(source_y, target_x - 1, "▶", role);
            } else {
                canvas.add_connection(source_y, target_x - 1, DIR_LEFT | DIR_RIGHT, edge.style, role);
                canvas.add_connection(source_y, target_x, DIR_LEFT, edge.style, role);
            }
        } else {
            int middle = target_x - 2;
            if (middle <= source_x + 1) {
                middle = source_x + 2;
            }
            for (int x = source_x + 1; x < middle; ++x) {
                canvas.add_connection(source_y, x, DIR_LEFT | DIR_RIGHT, edge.style, role);
            }
            const bool downwards = target_y > source_y;
            canvas.add_connection(source_y, middle, DIR_LEFT | (downwards ? DIR_DOWN : DIR_UP), edge.style, role);
            const int step = downwards ? 1 : -1;
            for (int y = source_y + step; y != target_y; y += step) {
                canvas.add_connection(y, middle, DIR_UP | DIR_DOWN, edge.style, role);
            }
            canvas.add_connection(target_y, middle, (downwards ? DIR_UP : DIR_DOWN) | DIR_RIGHT, edge.style, role);
            for (int x = middle + 1; x < target_x - 1; ++x) {
                canvas.add_connection(target_y, x, DIR_LEFT | DIR_RIGHT, edge.style, role);
            }
            if (edge.arrow) {
                canvas.put(target_y, target_x - 1, "▶", role);
            } else {
                canvas.add_connection(target_y, target_x - 1, DIR_LEFT | DIR_RIGHT, edge.style, role);
                canvas.add_connection(target_y, target_x, DIR_LEFT, edge.style, role);
            }
        }
        if (!edge.label.empty()) {
            const int label_width = text_display_width(edge.label.view());
            if (source_x + 2 + label_width < target_x) {
                canvas.put_edge_label(source_y - 1, source_x + 2, source_y + 2, target_x - label_width - 2,
                                      edge.label.view());
            }
            (void) column_gap;
        }
    }

    bool layout_top_down(diagram_graph& graph, mermaid_art& art) {
        int max_rank = 0;
        for (const diagram_node& node: graph.nodes) {
            max_rank = max(max_rank, node.rank);
        }
        vector<vector<size_t>> layers(static_cast<size_t>(max_rank) + 1);
        for (size_t i = 0; i < graph.nodes.size(); ++i) {
            layers[static_cast<size_t>(graph.nodes[i].rank)].push_back(i);
        }

        constexpr int node_gap = 6;
        int widest_layer = 0;
        for (const vector<size_t>& layer: layers) {
            int width = 0;
            for (size_t i = 0; i < layer.size(); ++i) {
                const diagram_node& node = graph.nodes[layer[i]];
                width += node.width;
                if (i + 1 < layer.size()) {
                    width += node_gap;
                }
            }
            widest_layer = max(widest_layer, width);
        }

        for (size_t rank = 0; rank < layers.size(); ++rank) {
            int width = 0;
            for (size_t i = 0; i < layers[rank].size(); ++i) {
                width += graph.nodes[layers[rank][i]].width;
                if (i + 1 < layers[rank].size()) {
                    width += node_gap;
                }
            }
            int x = (widest_layer - width) / 2;
            for (const size_t index: layers[rank]) {
                diagram_node& node = graph.nodes[index];
                node.left = x;
                node.top = static_cast<int>(rank) * 7;
                x += node.width + node_gap;
            }
        }

        art_canvas canvas;
        for (const diagram_node& node: graph.nodes) {
            draw_node_box(canvas, node);
        }
        for (const diagram_edge& edge: graph.edges) {
            if (edge.from >= graph.nodes.size() || edge.to >= graph.nodes.size()) {
                continue;
            }
            const diagram_node& from = graph.nodes[edge.from];
            const diagram_node& to = graph.nodes[edge.to];
            if (to.rank == from.rank + 1) {
                draw_stacked_edge(canvas, from, to, edge);
            } else if (from.rank == to.rank + 1 && from.top == to.top + 7) {
                draw_stacked_edge_up(canvas, from, to, edge);
            } else if (to.rank == from.rank) {
                const vector<size_t>& layer = layers[static_cast<size_t>(from.rank)];
                size_t from_position = string_view::npos;
                size_t to_position = string_view::npos;
                for (size_t i = 0; i < layer.size(); ++i) {
                    if (layer[i] == edge.from) {
                        from_position = i;
                    }
                    if (layer[i] == edge.to) {
                        to_position = i;
                    }
                }
                if (from_position != string_view::npos && to_position != string_view::npos) {
                    const size_t distance =
                            from_position > to_position ? from_position - to_position : to_position - from_position;
                    if (distance == 1) {
                        draw_same_rank_edge(canvas, from, to, edge);
                    }
                }
            }
        }
        art = build_art(canvas);
        return !art.lines.empty();
    }

    bool layout_left_right(diagram_graph& graph, mermaid_art& art) {
        int max_rank = 0;
        for (const diagram_node& node: graph.nodes) {
            max_rank = max(max_rank, node.rank);
        }
        vector<vector<size_t>> layers(static_cast<size_t>(max_rank) + 1);
        for (size_t i = 0; i < graph.nodes.size(); ++i) {
            layers[static_cast<size_t>(graph.nodes[i].rank)].push_back(i);
        }

        int max_label_width = 0;
        for (const diagram_edge& edge: graph.edges) {
            max_label_width = max(max_label_width, text_display_width(edge.label.view()));
        }
        const int column_gap = max(8, max_label_width + 4);

        vector<int> column_widths(layers.size(), 0);
        for (size_t rank = 0; rank < layers.size(); ++rank) {
            for (const size_t index: layers[rank]) {
                column_widths[rank] = max(column_widths[rank], graph.nodes[index].width);
            }
        }
        vector<int> column_lefts(layers.size(), 0);
        int cursor = 0;
        for (size_t rank = 0; rank < layers.size(); ++rank) {
            column_lefts[rank] = cursor;
            cursor += column_widths[rank] + column_gap;
        }

        for (size_t rank = 0; rank < layers.size(); ++rank) {
            int y = 0;
            for (const size_t index: layers[rank]) {
                diagram_node& node = graph.nodes[index];
                node.left = column_lefts[rank] + (column_widths[rank] - node.width) / 2;
                node.top = y;
                y += 7;
            }
        }

        art_canvas canvas;
        for (const diagram_node& node: graph.nodes) {
            draw_node_box(canvas, node);
        }
        for (const diagram_edge& edge: graph.edges) {
            if (edge.from >= graph.nodes.size() || edge.to >= graph.nodes.size()) {
                continue;
            }
            const diagram_node& from = graph.nodes[edge.from];
            const diagram_node& to = graph.nodes[edge.to];
            if (to.rank == from.rank + 1) {
                draw_horizontal_edge(canvas, from, to, edge, column_gap);
            } else if (to.rank == from.rank && to.top == from.top + 7) {
                draw_stacked_edge(canvas, from, to, edge);
            } else if (to.rank == from.rank && from.top == to.top + 7) {
                draw_stacked_edge_up(canvas, from, to, edge);
            }
        }
        art = build_art(canvas);
        return !art.lines.empty();
    }

    bool layout_graph(diagram_graph& graph, const flow_direction direction, mermaid_art& art) {
        if (graph.nodes.empty()) {
            return false;
        }
        assign_ranks(graph);
        for (diagram_node& node: graph.nodes) {
            node.width = text_display_width(node.label.view()) + 4;
            if (node.shape == node_shape::diamond) {
                node.width += 2;
            }
            node.width = max(node.width, 4);
        }
        if (direction == flow_direction::left_right) {
            return layout_left_right(graph, art);
        }
        return layout_top_down(graph, art);
    }

    bool build_flowchart(const mermaid_document& document, const header_info& header, string& title,
                         diagram_graph& graph, flow_direction& direction) {
        direction = header.direction;
        process_flow_line(header.remainder, graph, direction, title);
        for (size_t i = document.header + 1; i < document.lines.size(); ++i) {
            process_flow_line(strip_comment(document.lines[i]), graph, direction, title);
        }
        return !graph.nodes.empty();
    }

    size_t resolve_state(diagram_graph& graph, const string_view reference, const bool as_target) {
        if (reference == "[*]") {
            if (as_target) {
                return ensure_node(graph, "__mermaid_end__", "●", node_shape::circle, true);
            }
            return ensure_node(graph, "__mermaid_start__", "●", node_shape::circle, true);
        }
        return ensure_node(graph, reference, reference, node_shape::rectangle, false);
    }

    void parse_state_declaration(const string_view text, diagram_graph& graph) {
        string_view rest = text.trim();
        if (rest.empty()) {
            return;
        }
        if (rest.front() == '"') {
            const size_t close = rest.find('"', 1);
            if (close == string_view::npos) {
                return;
            }
            const string label = sanitize_label(rest.substr(1, close - 1));
            rest = rest.substr(close + 1).trim();
            string id = label;
            if (matches_keyword(rest, "as")) {
                id = trim_copy(rest.substr(2));
            }
            if (!id.empty()) {
                ensure_node(graph, id.view(), label.view(), node_shape::rectangle, true);
            }
            return;
        }

        const size_t as_position = rest.find(" as ");
        if (as_position != string_view::npos) {
            const string id = trim_copy(rest.substr(0, as_position));
            const string alias = trim_copy(rest.substr(as_position + 4));
            if (!id.empty()) {
                const string_view display = alias.empty() ? id.view() : alias.view();
                ensure_node(graph, id.view(), display, node_shape::rectangle, true);
            }
            return;
        }

        size_t end = 0;
        while (end < rest.size() && !is_space(rest[end]) && rest[end] != '{' && rest[end] != ':') {
            ++end;
        }
        if (end == 0) {
            return;
        }
        const string_view id = rest.substr(0, end);
        ensure_node(graph, id, id, node_shape::rectangle, true);
    }

    void parse_state_transition(const string_view text, diagram_graph& graph) {
        const size_t arrow = text.find("-->");
        if (arrow == string_view::npos) {
            return;
        }
        const string_view left = text.substr(0, arrow).trim();
        string_view right = text.substr(arrow + 3).trim();
        if (left.empty() || right.empty()) {
            return;
        }
        string label;
        const size_t colon = right.find(':');
        if (colon != string_view::npos) {
            label = sanitize_label(right.substr(colon + 1));
            right = right.substr(0, colon).trim();
        }
        if (right.empty()) {
            return;
        }
        const size_t from = resolve_state(graph, left, false);
        const size_t to = resolve_state(graph, right, true);
        if (from == string_view::npos || to == string_view::npos) {
            return;
        }
        if (graph.edges.size() >= MERMAID_MAX_EDGES) {
            return;
        }
        diagram_edge edge;
        edge.from = from;
        edge.to = to;
        edge.label = label;
        graph.edges.push_back(move(edge));
    }

    bool is_state_directive(const string_view text) noexcept {
        if (text.starts_with("}") || text == "--") {
            return true;
        }
        constexpr string_view directives[] = {
                "note",      "end",  "classDef", "class",    "style",    "click",
                "direction", "hide", "scale",    "accTitle", "accDescr",
        };
        for (const string_view directive: directives) {
            if (matches_keyword(text, directive)) {
                return true;
            }
        }
        return false;
    }

    void process_state_line(const string_view line, string& title, diagram_graph& graph) {
        vector<string_view> statements;
        split_statements(line, statements);
        for (const string_view statement: statements) {
            const string_view text = statement.trim();
            if (text.empty()) {
                continue;
            }
            if (take_title_line(text, title)) {
                continue;
            }
            if (is_state_directive(text)) {
                continue;
            }
            if (matches_keyword(text, "state")) {
                parse_state_declaration(text.substr(5), graph);
                continue;
            }
            parse_state_transition(text, graph);
        }
    }

    bool build_state_diagram(const mermaid_document& document, const header_info& header, string& title,
                             diagram_graph& graph) {
        process_state_line(header.remainder, title, graph);
        for (size_t i = document.header + 1; i < document.lines.size(); ++i) {
            process_state_line(strip_comment(document.lines[i]), title, graph);
        }
        return !graph.nodes.empty();
    }

    struct sequence_participant {
        string id;
        string name;
    };

    struct sequence_event {
        enum class kind : uint8_t {
            message = 0,
            note,
        };

        kind type = kind::message;
        string from_id;
        string to_id;
        string text;
        bool dotted = false;
        size_t from = 0;
        size_t to = 0;
    };

    struct sequence_data {
        vector<sequence_participant> participants;
        vector<sequence_event> events;
    };

    size_t find_participant(const sequence_data& data, const string_view reference) noexcept {
        for (size_t i = 0; i < data.participants.size(); ++i) {
            if (data.participants[i].id.view() == reference) {
                return i;
            }
        }
        for (size_t i = 0; i < data.participants.size(); ++i) {
            if (data.participants[i].name.view() == reference) {
                return i;
            }
        }
        return string_view::npos;
    }

    size_t ensure_participant(sequence_data& data, const string_view id, const string_view name,
                              const bool explicit_declaration) {
        if (id.empty()) {
            return string_view::npos;
        }
        const size_t existing = find_participant(data, id);
        if (existing != string_view::npos) {
            if (explicit_declaration) {
                data.participants[existing].name = sanitize_label(name.empty() ? id : name);
            }
            return existing;
        }
        if (data.participants.size() >= MERMAID_MAX_PARTICIPANTS) {
            return string_view::npos;
        }
        sequence_participant participant;
        participant.id = string(id);
        participant.name = sanitize_label(name.empty() ? id : name);
        data.participants.push_back(move(participant));
        return data.participants.size() - 1;
    }

    bool parse_participant_line(const string_view text, string& id, string& name) {
        const string_view rest = text.trim();
        if (rest.empty()) {
            return false;
        }
        const size_t as_position = rest.find(" as ");
        if (as_position != string_view::npos) {
            id = strip_quotes(rest.substr(0, as_position));
            name = strip_quotes(rest.substr(as_position + 4));
        } else {
            id = strip_quotes(rest);
            name = id;
        }
        return !id.empty();
    }

    struct sequence_arrow_pattern {
        string_view token;
        bool dotted;
    };

    bool parse_message_line(const string_view text, string& from, string& to, string& body, bool& dotted) {
        constexpr sequence_arrow_pattern patterns[] = {
                {"-->>", true}, {"->>", false}, {"-->", true}, {"->", false},
                {"--x", true},  {"-x", false},  {"--)", true}, {"-)", false},
        };

        size_t best = string_view::npos;
        size_t best_length = 0;
        bool best_dotted = false;
        for (const sequence_arrow_pattern& pattern: patterns) {
            const string_view token(pattern.token);
            const size_t position = text.find(token);
            if (position == string_view::npos) {
                continue;
            }
            if (position < best || (position == best && token.size() > best_length)) {
                best = position;
                best_length = token.size();
                best_dotted = pattern.dotted;
            }
        }
        if (best == string_view::npos || best_length == 0) {
            return false;
        }
        const string_view sender = text.substr(0, best).trim();
        const string_view right = text.substr(best + best_length).trim();
        const size_t colon = right.find(':');
        string_view receiver = right;
        string message_body;
        if (colon != string_view::npos) {
            receiver = right.substr(0, colon).trim();
            message_body = trim_copy(right.substr(colon + 1));
        }
        while (!receiver.empty() && (receiver.front() == '+' || receiver.front() == '-')) {
            receiver.remove_prefix(1);
        }
        receiver = receiver.trim();
        if (sender.empty() || receiver.empty()) {
            return false;
        }
        from = string(sender);
        to = string(receiver);
        body = move(message_body);
        dotted = best_dotted;
        return true;
    }

    bool is_note_line(const string_view text) {
        const string_view trimmed = text.trim();
        if (trimmed.size() < 4) {
            return false;
        }
        const char first = trimmed[0];
        const char second = trimmed[1];
        const char third = trimmed[2];
        const char fourth = trimmed[3];
        if ((first != 'N' && first != 'n') || (second != 'o' && second != 'O') || (third != 't' && third != 'T') ||
            (fourth != 'e' && fourth != 'E')) {
            return false;
        }
        return trimmed.size() == 4 || is_space(trimmed[4]) || trimmed[4] == ':';
    }

    bool parse_note_line(const string_view text, string& body) {
        const string_view trimmed = text.trim();
        const size_t colon = trimmed.find(':');
        if (colon != string_view::npos) {
            body = trim_copy(trimmed.substr(colon + 1));
        } else {
            body = trim_copy(trimmed.substr(4));
        }
        return true;
    }

    bool is_sequence_directive(const string_view text) noexcept {
        if (text == "end" || text.starts_with("}") || text == "--") {
            return true;
        }
        constexpr string_view directives[] = {
                "autonumber", "activate", "deactivate", "loop", "alt", "else",   "opt",     "par", "and",
                "critical",   "option",   "break",      "rect", "box", "create", "destroy", "end",
        };
        for (const string_view directive: directives) {
            if (matches_keyword(text, directive)) {
                return true;
            }
        }
        return false;
    }

    bool build_sequence_diagram(const mermaid_document& document, const header_info& header, string& title,
                                sequence_data& data) {
        vector<sequence_participant> declarations;
        vector<sequence_event> raw_events;

        const auto process_line = [&](const string_view line) {
            vector<string_view> statements;
            split_statements(line, statements);
            for (const string_view statement: statements) {
                const string_view text = statement.trim();
                if (text.empty()) {
                    continue;
                }
                if (take_title_line(text, title)) {
                    continue;
                }
                if (matches_keyword(text, "participant") || matches_keyword(text, "actor")) {
                    const size_t keyword_size = matches_keyword(text, "participant") ? 11 : 5;
                    string id;
                    string name;
                    if (parse_participant_line(text.substr(keyword_size), id, name)) {
                        sequence_participant participant;
                        participant.id = move(id);
                        participant.name = sanitize_label(name.view());
                        declarations.push_back(move(participant));
                    }
                    continue;
                }
                if (is_note_line(text)) {
                    if (raw_events.size() >= MERMAID_MAX_EVENTS) {
                        continue;
                    }
                    sequence_event event;
                    event.type = sequence_event::kind::note;
                    parse_note_line(text, event.text);
                    raw_events.push_back(move(event));
                    continue;
                }
                if (is_sequence_directive(text)) {
                    continue;
                }
                string from;
                string to;
                string body;
                bool dotted = false;
                if (parse_message_line(text, from, to, body, dotted)) {
                    if (raw_events.size() >= MERMAID_MAX_EVENTS) {
                        continue;
                    }
                    sequence_event event;
                    event.type = sequence_event::kind::message;
                    event.from_id = move(from);
                    event.to_id = move(to);
                    event.text = move(body);
                    event.dotted = dotted;
                    raw_events.push_back(move(event));
                }
            }
        };

        process_line(header.remainder);
        for (size_t i = document.header + 1; i < document.lines.size(); ++i) {
            process_line(strip_comment(document.lines[i]));
        }

        for (const sequence_participant& declaration: declarations) {
            ensure_participant(data, declaration.id.view(), declaration.name.view(), true);
        }
        for (sequence_event& event: raw_events) {
            if (event.type == sequence_event::kind::message) {
                const size_t from = ensure_participant(data, event.from_id.view(), event.from_id.view(), false);
                const size_t to = ensure_participant(data, event.to_id.view(), event.to_id.view(), false);
                if (from == string_view::npos || to == string_view::npos) {
                    continue;
                }
                event.from = from;
                event.to = to;
            }
            data.events.push_back(move(event));
        }
        return !data.participants.empty();
    }

    void draw_participant_box(art_canvas& canvas, const int left, const int width, const string_view name) {
        const int right = left + width - 1;
        constexpr auto border = mermaid_role::border;
        canvas.put(0, left, "┌", border);
        for (int x = left + 1; x < right; ++x) {
            canvas.put(0, x, "─", border);
        }
        canvas.put(0, right, "┐", border);
        canvas.put(1, left, "│", border);
        canvas.put(1, right, "│", border);
        canvas.put(2, left, "└", border);
        for (int x = left + 1; x < right; ++x) {
            canvas.put(2, x, "─", border);
        }
        canvas.put(2, right, "┘", border);
        const int text_width = text_display_width(name);
        int text_left = left + (width - text_width) / 2;
        text_left = max(text_left, left + 1);
        canvas.put_text(1, text_left, name, mermaid_role::text);
    }

    bool layout_sequence_diagram(const sequence_data& data, mermaid_art& art) {
        if (data.participants.empty()) {
            return false;
        }
        const size_t count = data.participants.size();
        vector<int> widths(count, 0);
        vector<int> lefts(count, 0);
        vector<int> centers(count, 0);
        vector<int> gaps(count > 0 ? count - 1 : 0, 8);

        for (size_t i = 0; i < count; ++i) {
            widths[i] = max(5, text_display_width(data.participants[i].name.view()) + 4);
        }

        const auto compute_positions = [&]() {
            for (size_t i = 0; i < count; ++i) {
                if (i == 0) {
                    lefts[i] = 0;
                } else {
                    lefts[i] = lefts[i - 1] + widths[i - 1] + gaps[i - 1];
                }
                centers[i] = lefts[i] + widths[i] / 2;
            }
        };
        compute_positions();

        for (const sequence_event& event: data.events) {
            if (event.type != sequence_event::kind::message) {
                continue;
            }
            if (event.from >= count || event.to >= count) {
                continue;
            }
            const int label_width = text_display_width(event.text.view());
            if (event.from == event.to) {
                if (event.from + 1 < count) {
                    gaps[event.from] = max(gaps[event.from], label_width + 6);
                    compute_positions();
                }
                continue;
            }
            const size_t low = min(event.from, event.to);
            const size_t high = max(event.from, event.to);
            const int span = centers[high] - centers[low];
            const int needed = label_width + 4;
            if (span < needed) {
                const int deficit = needed - span;
                const size_t gap_count = high - low;
                for (size_t i = 0; i < gap_count; ++i) {
                    gaps[low + i] += deficit / static_cast<int>(gap_count);
                }
                gaps[low] += deficit % static_cast<int>(gap_count);
                compute_positions();
            }
        }

        art_canvas canvas;
        for (size_t i = 0; i < count; ++i) {
            draw_participant_box(canvas, lefts[i], widths[i], data.participants[i].name.view());
        }

        int y = 3;
        for (const sequence_event& event: data.events) {
            if (event.type == sequence_event::kind::note) {
                y += 2;
            } else if (event.from == event.to) {
                y += 3;
            } else {
                y += 2;
            }
        }

        const int total_rows = max(y, 4) + 1;
        constexpr auto edge = mermaid_role::edge;
        for (size_t i = 0; i < count; ++i) {
            canvas.add_connection(2, centers[i], DIR_DOWN, edge_style::solid, edge);
            for (int row = 3; row < total_rows; ++row) {
                canvas.put(row, centers[i], "│", edge);
            }
        }

        y = 3;
        for (const sequence_event& event: data.events) {
            if (event.type == sequence_event::kind::note) {
                string note = "Note: ";
                note += event.text;
                canvas.put_text(y, 2, note.view(), mermaid_role::text);
                y += 2;
                continue;
            }
            if (event.from >= count || event.to >= count) {
                continue;
            }
            const int from_center = centers[event.from];
            const int to_center = centers[event.to];
            if (event.from == event.to) {
                if (!event.text.empty()) {
                    canvas.put_text(y, from_center + 2, event.text.view(), mermaid_role::edge_label);
                }
                canvas.add_connection(y + 1, from_center, DIR_RIGHT, edge_style::solid, edge);
                for (int x = from_center + 1; x <= from_center + 3; ++x) {
                    canvas.add_connection(y + 1, x, DIR_LEFT | DIR_RIGHT, edge_style::solid, edge);
                }
                canvas.add_connection(y + 1, from_center + 4, DIR_LEFT | DIR_DOWN, edge_style::solid, edge);
                canvas.put(y + 2, from_center, "◀", edge);
                for (int x = from_center + 1; x <= from_center + 3; ++x) {
                    canvas.add_connection(y + 2, x, DIR_LEFT | DIR_RIGHT, edge_style::solid, edge);
                }
                canvas.add_connection(y + 2, from_center + 4, DIR_LEFT | DIR_UP, edge_style::solid, edge);
                y += 3;
                continue;
            }
            if (!event.text.empty()) {
                const int label_width = text_display_width(event.text.view());
                const int span = to_center > from_center ? to_center - from_center : from_center - to_center;
                int start = min(from_center, to_center) + (span - label_width) / 2;
                start = max(start, min(from_center, to_center) + 2);
                canvas.put_text(y, start, event.text.view(), mermaid_role::edge_label);
            }
            const int row = y + 1;
            if (from_center < to_center) {
                canvas.add_connection(row, from_center, DIR_RIGHT, edge_style::solid, edge);
                canvas.horizontal_run(row, from_center + 1, to_center - 1, event.dotted, edge);
                canvas.put(row, to_center, "▶", edge);
            } else {
                canvas.add_connection(row, from_center, DIR_LEFT, edge_style::solid, edge);
                canvas.horizontal_run(row, to_center + 1, from_center - 1, event.dotted, edge);
                canvas.put(row, to_center, "◀", edge);
            }
            y += 2;
        }

        art = build_art(canvas);
        return !art.lines.empty();
    }

    bool matches_diagram_header(const string_view text, const string_view keyword) noexcept {
        if (!text.starts_with(keyword)) {
            return false;
        }
        if (text.size() == keyword.size()) {
            return true;
        }
        const char next = text[keyword.size()];
        return is_space(next) || next == ';' || next == ':';
    }

    string_view diagram_header_remainder(const string_view text, const size_t keyword_size) noexcept {
        string_view rest = text.substr(keyword_size).trim();
        if (!rest.empty() && rest.front() == ';') {
            rest.remove_prefix(1);
            rest = rest.trim();
        }
        return rest;
    }

    bool parse_header(const string_view line, header_info& result) {
        const string_view text = line.trim();
        if (text.empty()) {
            return false;
        }
        if (matches_keyword(text, "flowchart") || matches_keyword(text, "graph")) {
            const size_t keyword_size = text.starts_with("flowchart") ? 9 : 5;
            result.kind = diagram_kind::flow;
            string_view rest = text.substr(keyword_size).trim();
            if (!rest.empty() && rest.front() == ';') {
                rest.remove_prefix(1);
                rest = rest.trim();
            }
            if (!rest.empty()) {
                size_t end = 0;
                while (end < rest.size() && !is_space(rest[end]) && rest[end] != ';') {
                    ++end;
                }
                const string_view token = rest.substr(0, end);
                if (!token.empty() && parse_direction_token(token, result.direction)) {
                    rest.remove_prefix(end);
                    rest = rest.trim();
                    if (!rest.empty() && rest.front() == ';') {
                        rest.remove_prefix(1);
                        rest = rest.trim();
                    }
                    result.remainder = rest;
                } else {
                    result.remainder = rest;
                }
            }
            return true;
        }
        if (matches_keyword(text, "sequenceDiagram")) {
            result.kind = diagram_kind::sequence;
            string_view rest = text.substr(15).trim();
            if (!rest.empty() && rest.front() == ';') {
                rest.remove_prefix(1);
                rest = rest.trim();
            }
            result.remainder = rest;
            return true;
        }
        if (text.starts_with("stateDiagram-v2") && matches_keyword(text, "stateDiagram-v2")) {
            result.kind = diagram_kind::state;
            string_view rest = text.substr(15).trim();
            if (!rest.empty() && rest.front() == ';') {
                rest.remove_prefix(1);
                rest = rest.trim();
            }
            result.remainder = rest;
            return true;
        }
        if (matches_keyword(text, "stateDiagram")) {
            result.kind = diagram_kind::state;
            string_view rest = text.substr(12).trim();
            if (!rest.empty() && rest.front() == ';') {
                rest.remove_prefix(1);
                rest = rest.trim();
            }
            result.remainder = rest;
            return true;
        }
        if (matches_diagram_header(text, "classDiagram-v2")) {
            result.kind = diagram_kind::class_diagram;
            result.remainder = diagram_header_remainder(text, 15);
            return true;
        }
        if (matches_diagram_header(text, "classDiagram")) {
            result.kind = diagram_kind::class_diagram;
            result.remainder = diagram_header_remainder(text, 12);
            return true;
        }
        if (matches_diagram_header(text, "erDiagram")) {
            result.kind = diagram_kind::entity_relationship;
            result.remainder = diagram_header_remainder(text, 9);
            return true;
        }
        if (matches_diagram_header(text, "gitGraph")) {
            result.kind = diagram_kind::git_graph;
            result.remainder = diagram_header_remainder(text, 8);
            return true;
        }
        if (matches_diagram_header(text, "pie")) {
            result.kind = diagram_kind::pie_chart;
            result.remainder = diagram_header_remainder(text, 3);
            return true;
        }
        if (matches_diagram_header(text, "mindmap")) {
            result.kind = diagram_kind::mindmap;
            result.remainder = diagram_header_remainder(text, 7);
            return true;
        }
        if (matches_diagram_header(text, "timeline")) {
            result.kind = diagram_kind::timeline;
            result.remainder = diagram_header_remainder(text, 8);
            return true;
        }
        if (matches_diagram_header(text, "gantt")) {
            result.kind = diagram_kind::gantt;
            result.remainder = diagram_header_remainder(text, 5);
            return true;
        }
        if (matches_diagram_header(text, "journey")) {
            result.kind = diagram_kind::journey;
            result.remainder = diagram_header_remainder(text, 7);
            return true;
        }
        if (matches_diagram_header(text, "quadrantChart")) {
            result.kind = diagram_kind::quadrant;
            result.remainder = diagram_header_remainder(text, 13);
            return true;
        }
        if (matches_diagram_header(text, "requirementDiagram")) {
            result.kind = diagram_kind::requirement;
            result.remainder = diagram_header_remainder(text, 18);
            return true;
        }
        return false;
    }

    bool prepare_document(const string_view source, mermaid_document& document) {
        string_view text = source;
        if (text.size() >= 3 && static_cast<byte_t>(text[0]) == 0xEF && static_cast<byte_t>(text[1]) == 0xBB &&
            static_cast<byte_t>(text[2]) == 0xBF) {
            text.remove_prefix(3);
        }
        document.lines = split_lines(text);

        size_t index = 0;
        while (index < document.lines.size() && strip_comment(document.lines[index]).trim().empty()) {
            ++index;
        }
        if (index < document.lines.size() && strip_comment(document.lines[index]).trim() == "---") {
            size_t close = index + 1;
            while (close < document.lines.size() && strip_comment(document.lines[close]).trim() != "---") {
                ++close;
            }
            if (close < document.lines.size()) {
                for (size_t i = index + 1; i < close; ++i) {
                    const string_view front = strip_comment(document.lines[i]).trim();
                    if (!front.starts_with("title:")) {
                        continue;
                    }
                    const string candidate = sanitize_label(front.substr(6));
                    if (!candidate.empty()) {
                        document.title = candidate;
                    }
                }
                index = close + 1;
            }
        }

        while (index < document.lines.size() && strip_comment(document.lines[index]).trim().empty()) {
            ++index;
        }
        if (index >= document.lines.size()) {
            return false;
        }
        document.header = index;
        return true;
    }
} // namespace


optional<mermaid_art> render_mermaid(const string_view source) {
    if (source.empty() || source.size() > MERMAID_SOURCE_LIMIT) {
        return none;
    }
    try {
        mermaid_document document;
        if (!prepare_document(source, document)) {
            return none;
        }
        header_info header;
        if (!parse_header(strip_comment(document.lines[document.header]).trim(), header) ||
            header.kind == diagram_kind::none) {
            return none;
        }

        string title = document.title;
        mermaid_art art;
        if (header.kind == diagram_kind::flow) {
            diagram_graph graph;
            flow_direction direction = header.direction;
            if (!build_flowchart(document, header, title, graph, direction)) {
                return none;
            }
            if (!layout_graph(graph, direction, art)) {
                return none;
            }
        } else if (header.kind == diagram_kind::state) {
            diagram_graph graph;
            if (!build_state_diagram(document, header, title, graph)) {
                return none;
            }
            if (!layout_graph(graph, flow_direction::top_down, art)) {
                return none;
            }
        } else if (header.kind == diagram_kind::sequence) {
            sequence_data data;
            if (!build_sequence_diagram(document, header, title, data)) {
                return none;
            }
            if (!layout_sequence_diagram(data, art)) {
                return none;
            }
        } else if (header.kind == diagram_kind::class_diagram) {
            class_data data;
            if (!build_class_diagram(document, header, title, data)) {
                return none;
            }
            if (!layout_class_diagram(data, art)) {
                return none;
            }
        } else if (header.kind == diagram_kind::entity_relationship) {
            er_data data;
            if (!build_er_diagram(document, header, title, data)) {
                return none;
            }
            if (!layout_er_diagram(data, art)) {
                return none;
            }
        } else if (header.kind == diagram_kind::git_graph) {
            git_data data;
            if (!build_git_graph(document, header, title, data)) {
                return none;
            }
            if (!layout_git_graph(data, art)) {
                return none;
            }
        } else if (header.kind == diagram_kind::pie_chart) {
            pie_data data;
            if (!build_pie_chart(document, header, title, data)) {
                return none;
            }
            if (!layout_pie_chart(data, art)) {
                return none;
            }
        } else if (header.kind == diagram_kind::mindmap) {
            mindmap_data data;
            if (!build_mindmap(document, header, data)) {
                return none;
            }
            if (!layout_mindmap(data, art)) {
                return none;
            }
        } else if (header.kind == diagram_kind::timeline) {
            timeline_data data;
            if (!build_timeline(document, header, title, data)) {
                return none;
            }
            if (!layout_timeline(data, art)) {
                return none;
            }
        } else if (header.kind == diagram_kind::gantt) {
            gantt_data data;
            if (!build_gantt(document, header, title, data)) {
                return none;
            }
            if (!layout_gantt(data, art)) {
                return none;
            }
        } else if (header.kind == diagram_kind::journey) {
            journey_data data;
            if (!build_journey(document, header, title, data)) {
                return none;
            }
            if (!layout_journey(data, art)) {
                return none;
            }
        } else if (header.kind == diagram_kind::quadrant) {
            quadrant_data data;
            if (!build_quadrant_chart(document, header, title, data)) {
                return none;
            }
            if (!layout_quadrant_chart(data, art)) {
                return none;
            }
        } else if (header.kind == diagram_kind::requirement) {
            requirement_data data;
            if (!build_requirement_diagram(document, header, title, data)) {
                return none;
            }
            if (!layout_requirement_diagram(data, art)) {
                return none;
            }
        } else {
            return none;
        }
        if (!title.empty()) {
            prepend_title(art, title);
        }
        if (art.lines.empty()) {
            return none;
        }
        return art;
    } catch (...) {
        return none;
    }
}

NEFORCE_END_NAMESPACE__
