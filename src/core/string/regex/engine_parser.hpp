#ifndef NEFORCE_CORE_STRING_REGEX_ENGINE_PARSER_SRC__
#define NEFORCE_CORE_STRING_REGEX_ENGINE_PARSER_SRC__
#include "engine_property.hpp"
NEFORCE_BEGIN_NAMESPACE__

class parser {
public:
    parser(const char* pattern, const size_t length, const uint32_t options) :
    pattern_(pattern),
    length_(length),
    options_(options),
    caseless_((options & opt_caseless) != 0),
    dotall_((options & opt_dotall) != 0),
    multiline_((options & opt_multiline) != 0),
    extended_((options & opt_extended) != 0),
    ungreedy_((options & opt_ungreedy) != 0),
    ucp_((options & opt_ucp) != 0),
    utf_((options & opt_utf) != 0) {}

    int32_t parse() {
        if ((options_ & opt_literal) != 0) {
            return parse_literal_pattern();
        }

        skip_ignored();
        const int32_t root = parse_alternation();
        skip_ignored();

        if (!eof()) {
            fail(err_unmatched_parenthesis, pos_);
        }

        for (const auto& entry: nodes) {
            if (entry.type == node::kind::backref && entry.group > capture_count) {
                fail(err_unknown_group_reference, entry.offset);
            }
            if (entry.type == node::kind::call && entry.group > capture_count) {
                fail(err_unknown_group_reference, entry.offset);
            }
            if (entry.type == node::kind::conditional && entry.group > capture_count) {
                fail(err_unknown_group_reference, entry.offset);
            }
        }

        return root;
    }

    vector<node> nodes;
    vector<char_class> classes;
    vector<string> names;
    int capture_count = 0;

private:
    int32_t parse_literal_pattern() {
        vector<int32_t> items;

        while (!eof()) {
            node entry;
            entry.type = node::kind::literal;
            entry.offset = pos_;
            entry.code_point = decode_literal();
            entry.caseless = caseless_;
            items.push_back(add_node(entry));
        }

        return make_concat(items);
    }

    NEFORCE_NODISCARD bool eof(const size_t ahead = 0) const noexcept { return pos_ + ahead >= length_; }

    NEFORCE_NODISCARD char peek(const size_t ahead = 0) const noexcept {
        return pos_ + ahead < length_ ? pattern_[pos_ + ahead] : '\0';
    }

    void advance(const size_t count = 1) noexcept { pos_ += count; }

    NEFORCE_NORETURN void fail(const int code, const size_t offset) const {
        throw compile_failure{code, offset};
        unreachable();
    }

    void skip_ignored() {
        if (!extended_) {
            return;
        }

        for (;;) {
            while (!eof() && (peek() == ' ' || peek() == '\t' || peek() == '\n' || peek() == '\r' || peek() == '\f' ||
                              peek() == '\v')) {
                advance();
            }

            if (!eof() && peek() == '#') {
                while (!eof() && peek() != '\n') {
                    advance();
                }
                continue;
            }

            break;
        }
    }

    int32_t add_node(node entry) {
        entry.offset = entry.offset == invalid_offset ? pos_ : entry.offset;
        nodes.push_back(entry);
        return static_cast<int32_t>(nodes.size() - 1);
    }

    int32_t make_concat(vector<int32_t>& items) {
        if (items.empty()) {
            return add_node(node{});
        }

        if (items.size() == 1) {
            return items[0];
        }

        node entry;
        entry.type = node::kind::concat;
        entry.children = move(items);
        return add_node(entry);
    }

    uint32_t decode_literal() {
        if (!utf_) {
            const auto value = static_cast<unsigned char>(peek());
            advance();
            return value;
        }

        uint32_t code_point = 0;
        const size_t used =
                utf8_decode(reinterpret_cast<const unsigned char*>(pattern_ + pos_), length_ - pos_, code_point);

        if (used == 0) {
            fail(err_bad_utf8_pattern, pos_);
        }

        advance(used);
        return code_point;
    }

    void expect(const char value, const int code) {
        if (eof() || peek() != value) {
            fail(code, pos_);
        }
        advance();
    }

    NEFORCE_NODISCARD string parse_name() {
        const size_t start = pos_;

        while (!eof() && is_word(peek())) {
            advance();
        }

        if (pos_ == start) {
            fail(err_bad_group_name, start);
        }

        return string(pattern_ + start, pos_ - start);
    }

    NEFORCE_NODISCARD int find_name(const string& name) const noexcept { return find_name(name.view()); }

    NEFORCE_NODISCARD int find_name(const string_view name) const noexcept {
        for (size_t index = 1; index < names.size(); ++index) {
            if (names[index] == name) {
                return static_cast<int>(index);
            }
        }
        return -1;
    }

    void register_name(const string_view name, const int number) {
        if (find_name(name) >= 0 && (options_ & opt_dupnames) == 0) {
            fail(err_duplicate_group_name, pos_);
        }

        if (names.size() <= static_cast<size_t>(number)) {
            names.resize(static_cast<size_t>(number) + 1);
        }

        names[static_cast<size_t>(number)] = name;
    }

    int32_t parse_alternation() {
        vector<int32_t> branches;
        branches.push_back(parse_concat());

        while (!eof() && peek() == '|') {
            advance();
            branches.push_back(parse_concat());
        }

        if (branches.size() == 1) {
            return branches[0];
        }

        node entry;
        entry.type = node::kind::alternate;
        entry.children = move(branches);
        return add_node(entry);
    }

    int32_t parse_concat() {
        vector<int32_t> items;

        for (;;) {
            skip_ignored();

            if (eof()) {
                break;
            }

            const char next = peek();

            if (next == '|' || next == ')') {
                break;
            }

            const int32_t atom = parse_atom();

            if (atom < 0) {
                continue;
            }

            items.push_back(parse_quantifier(atom));
        }

        return make_concat(items);
    }

    int32_t parse_quantifier(const int32_t atom) {
        if (eof()) {
            return atom;
        }

        int32_t minimum = 0;
        int32_t maximum = 0;
        bool present = true;

        switch (peek()) {
            case '*':
                advance();
                maximum = -1;
                break;
            case '+':
                advance();
                minimum = 1;
                maximum = -1;
                break;
            case '?':
                advance();
                maximum = 1;
                break;
            case '{':
                present = parse_braces(minimum, maximum);
                break;
            default:
                present = false;
                break;
        }

        if (!present) {
            return atom;
        }

        node::repeat_mode mode = ungreedy_ ? node::repeat_mode::lazy : node::repeat_mode::greedy;

        if (!eof() && peek() == '?') {
            advance();
            mode = node::repeat_mode::lazy;
        } else if (!eof() && peek() == '+') {
            advance();
            mode = node::repeat_mode::possessive;
        }

        if (!eof() && (peek() == '*' || peek() == '+' || peek() == '?')) {
            fail(err_nested_quantifier, pos_);
        }

        node entry;
        entry.type = node::kind::repeat;
        entry.minimum = minimum;
        entry.maximum = maximum;
        entry.mode = mode;
        entry.children.push_back(atom);
        return add_node(entry);
    }

    bool parse_braces(int32_t& minimum, int32_t& maximum) {
        const size_t start = pos_;
        advance();

        if (eof() || !is_digit(peek())) {
            pos_ = start;
            return false;
        }

        int32_t value = 0;
        while (!eof() && is_digit(peek())) {
            value = value * 10 + (peek() - '0');
            if (value > 65535) {
                fail(err_quantifier_too_big, pos_);
            }
            advance();
        }

        minimum = value;
        maximum = value;

        if (!eof() && peek() == ',') {
            advance();

            if (!eof() && is_digit(peek())) {
                int32_t upper = 0;
                while (!eof() && is_digit(peek())) {
                    upper = upper * 10 + (peek() - '0');
                    if (upper > 65535) {
                        fail(err_quantifier_too_big, pos_);
                    }
                    advance();
                }
                maximum = upper;
            } else {
                maximum = -1;
            }
        }

        if (eof() || peek() != '}') {
            pos_ = start;
            return false;
        }

        advance();

        if (maximum != -1 && maximum < minimum) {
            fail(err_quantifier_too_big, start);
        }

        return true;
    }

    int32_t parse_atom() {
        const char next = peek();

        switch (next) {
            case '(':
                return parse_group();
            case '[':
                return parse_class();
            case '.': {
                advance();
                node entry;
                entry.type = node::kind::any;
                entry.dotall = dotall_;
                return add_node(entry);
            }
            case '^': {
                advance();
                node entry;
                entry.type = node::kind::begin_line;
                entry.multiline = multiline_;
                return add_node(entry);
            }
            case '$': {
                advance();
                node entry;
                entry.type = node::kind::end_line;
                entry.multiline = multiline_;
                return add_node(entry);
            }
            case '\\': {
                const int32_t escaped = parse_escape(false, nullptr);

                if (escaped <= -2) {
                    node entry;
                    entry.type = node::kind::literal;
                    entry.code_point = static_cast<uint32_t>(-2 - escaped);
                    entry.caseless = caseless_;
                    return add_node(entry);
                }

                return escaped;
            }
            case '*':
            case '+':
            case '?':
                fail(err_quantifier_without_item, pos_);
            default: {
                node entry;
                entry.type = node::kind::literal;
                entry.code_point = decode_literal();
                entry.caseless = caseless_;
                return add_node(entry);
            }
        }
    }


    NEFORCE_NODISCARD static string normalize_property(string name) {
        string normalized;
        normalized.reserve(name.length());

        for (size_t index = 0; index < name.length(); ++index) {
            char value = name[index];

            if (value == '_' || value == '-' || value == ' ') {
                continue;
            }

            if (value >= 'A' && value <= 'Z') {
                value = static_cast<char>(value + 32);
            }

            normalized += value;
        }

        return normalized;
    }

    void add_escaped_class(char_class& target, const char base) {
        if (!ucp_) {
            target.add_ascii_class(base);
            return;
        }

        if (base == 'd') {
            target.add_categories(cat_nd);
        } else if (base == 'w') {
            target.add_categories(cat_letters | cat_numbers);
            target.add('_', '_');
        } else {
            target.add_categories(cat_separators);
            target.add_ascii_class('s');
            target.add(0x85, 0x85);
        }
    }

    static void add_complement(char_class& target, const char_class& source) {
        uint32_t start = 0;

        for (const auto& entry: source.ranges) {
            if (entry.first > start) {
                target.add(start, entry.first - 1);
            }
            start = entry.last + 1;
        }

        if (start <= 0x10FFFFU) {
            target.add(start, 0x10FFFFU);
        }
    }

    void add_class_escape(char_class& target, const char kind) {
        const bool negated = kind >= 'A' && kind <= 'Z';
        const char base = negated ? static_cast<char>(kind + 32) : kind;

        char_class temporary;
        add_escaped_class(temporary, base);
        temporary.normalize();

        if (negated) {
            add_complement(target, temporary);
        } else {
            for (const auto& entry: temporary.ranges) {
                target.add(entry.first, entry.last);
            }
        }
    }

    int32_t make_class_escape_node(const char kind) {
        char_class entry;
        add_class_escape(entry, kind);
        entry.normalize();
        classes.push_back(move(entry));
        return static_cast<int32_t>(classes.size() - 1);
    }

    void add_property_to_class(char_class& target, const string& name, const bool negate) {
        char_class temporary;
        apply_property(temporary, name);
        temporary.normalize();

        if (negate) {
            add_complement(target, temporary);
        } else {
            for (const auto& entry: temporary.ranges) {
                target.add(entry.first, entry.last);
            }
        }
    }

    uint32_t parse_hex_value(const size_t digits, const bool braced) {
        uint32_t value = 0;
        size_t used = 0;

        while (!eof() && used < digits && isxdigit_char(peek())) {
            value = value * 16U + hex_value(peek());
            advance();
            ++used;
        }

        if (used == 0) {
            fail(err_bad_hex_escape, pos_);
        }

        if (braced) {
            expect('}', err_bad_hex_escape);
        }

        if (value > 0x10FFFFU) {
            fail(err_invalid_codepoint, pos_);
        }

        return value;
    }

    static constexpr bool isxdigit_char(const char value) noexcept {
        return is_digit(value) || (value >= 'a' && value <= 'f') || (value >= 'A' && value <= 'F');
    }

    static constexpr uint32_t hex_value(const char value) noexcept {
        if (value >= '0' && value <= '9') {
            return static_cast<uint32_t>(value - '0');
        }
        if (value >= 'a' && value <= 'f') {
            return static_cast<uint32_t>(value - 'a' + 10);
        }
        return static_cast<uint32_t>(value - 'A' + 10);
    }

    NEFORCE_NODISCARD static int32_t escape_value(const uint32_t code_point, const bool in_class) noexcept {
        return in_class ? static_cast<int32_t>(code_point) : -2 - static_cast<int32_t>(code_point);
    }

    int32_t parse_escape(const bool in_class, char_class* const target) {
        const size_t start = pos_;
        advance();

        if (eof()) {
            fail(err_bad_escape, start);
        }

        const char kind = peek();

        switch (kind) {
            case 'd':
            case 'D':
            case 'w':
            case 'W':
            case 's':
            case 'S': {
                advance();
                if (in_class) {
                    add_class_escape(*target, kind);
                    return -1;
                }
                node entry;
                entry.type = node::kind::char_class;
                entry.class_index = make_class_escape_node(kind);
                return add_node(entry);
            }
            case 'p':
            case 'P': {
                advance();
                string name;

                if (!eof() && peek() == '{') {
                    advance();
                    const size_t begin = pos_;
                    while (!eof() && peek() != '}') {
                        advance();
                    }
                    expect('}', err_unknown_property);
                    name = string(pattern_ + begin, pos_ - begin - 1);
                } else {
                    if (eof()) {
                        fail(err_unknown_property, start);
                    }
                    name = string(1, peek());
                    advance();
                }

                const string normalized = normalize_property(move(name));
                const bool negate = kind == 'P';

                if (in_class) {
                    try {
                        add_property_to_class(*target, normalized, negate);
                    } catch (const compile_failure& failure) {
                        fail(failure.code, start);
                    }
                    return -1;
                }

                char_class entry_class;
                try {
                    add_property_to_class(entry_class, normalized, negate);
                } catch (const compile_failure& failure) {
                    fail(failure.code, start);
                }
                entry_class.normalize();
                classes.push_back(move(entry_class));

                node entry;
                entry.type = node::kind::char_class;
                entry.class_index = static_cast<int32_t>(classes.size() - 1);
                return add_node(entry);
            }
            case 'Q': {
                advance();
                vector<int32_t> items;

                while (!eof()) {
                    if (peek() == '\\' && peek(1) == 'E') {
                        advance(2);
                        break;
                    }

                    node entry;
                    entry.type = node::kind::literal;
                    entry.code_point = decode_literal();
                    entry.caseless = caseless_;
                    items.push_back(add_node(entry));
                }

                return make_concat(items);
            }
            case 'E':
                advance();
                return -1;

            case 'b':
                advance();
                if (in_class) {
                    return escape_value(0x08, in_class);
                }
                {
                    node entry;
                    entry.type = node::kind::word_boundary;
                    entry.ucp = ucp_;
                    return add_node(entry);
                }
            case 'B': {
                advance();
                if (in_class) {
                    fail(err_bad_escape, start);
                }
                node entry;
                entry.type = node::kind::non_word_boundary;
                entry.ucp = ucp_;
                return add_node(entry);
            }
            case 'A': {
                advance();
                node entry;
                entry.type = node::kind::begin_subject;
                return add_node(entry);
            }
            case 'z': {
                advance();
                node entry;
                entry.type = node::kind::end_subject;
                return add_node(entry);
            }
            case 'Z': {
                advance();
                node entry;
                entry.type = node::kind::end_subject_final;
                return add_node(entry);
            }
            case 'X': {
                advance();
                if (in_class) {
                    fail(err_bad_escape, start);
                }
                node entry;
                entry.type = node::kind::cluster;
                return add_node(entry);
            }
            case 'N': {
                advance();
                node entry;
                entry.type = node::kind::any;
                entry.dotall = false;
                return add_node(entry);
            }
            case 'k': {
                advance();
                if (eof() || (peek() != '<' && peek() != '{' && peek() != '\'')) {
                    fail(err_bad_backref, start);
                }

                const char opener = peek();
                const char closer = opener == '<' ? '>' : (opener == '{' ? '}' : '\'');
                advance();

                const size_t begin = pos_;
                while (!eof() && peek() != closer) {
                    advance();
                }

                if (eof()) {
                    fail(err_bad_backref, start);
                }

                const auto name = string_view(pattern_ + begin, pos_ - begin);
                advance();

                const int number = find_name(name);

                if (number < 0) {
                    fail(err_unknown_group_reference, start);
                }

                node entry;
                entry.type = node::kind::backref;
                entry.group = number;
                entry.caseless = caseless_;
                return add_node(entry);
            }
            case 'g': {
                advance();

                if (!eof() && peek() == '{') {
                    advance();
                    const size_t begin = pos_;
                    while (!eof() && peek() != '}') {
                        advance();
                    }
                    expect('}', err_bad_backref);

                    const auto name = string_view(pattern_ + begin, pos_ - begin - 1);
                    int number = 0;

                    if (!name.empty() && is_digit(name[0])) {
                        number = 0;
                        for (size_t index = 0; index < name.length(); ++index) {
                            if (!is_digit(name[index])) {
                                fail(err_bad_backref, start);
                            }
                            number = number * 10 + (name[index] - '0');
                        }
                    } else {
                        number = find_name(name);
                    }

                    if (number < 0) {
                        fail(err_unknown_group_reference, start);
                    }

                    node entry;
                    entry.type = node::kind::backref;
                    entry.group = number;
                    entry.caseless = caseless_;
                    return add_node(entry);
                }

                if (!eof() && (peek() == '<' || peek() == '\'')) {
                    const char opener = peek();
                    const char closer = opener == '<' ? '>' : '\'';
                    advance();

                    const size_t begin = pos_;
                    while (!eof() && peek() != closer) {
                        advance();
                    }

                    if (eof()) {
                        fail(err_bad_backref, start);
                    }

                    const auto name = string_view(pattern_ + begin, pos_ - begin);
                    advance();

                    int number = 0;
                    if (!name.empty() && is_digit(name[0])) {
                        for (size_t index = 0; index < name.length(); ++index) {
                            number = number * 10 + (name[index] - '0');
                        }
                    } else {
                        number = find_name(name);
                    }

                    if (number < 0) {
                        fail(err_unknown_group_reference, start);
                    }

                    node entry;
                    entry.type = node::kind::call;
                    entry.group = number;
                    return add_node(entry);
                }

                fail(err_bad_backref, start);
            }
            case 'x': {
                advance();

                if (!eof() && peek() == '{') {
                    advance();
                    return escape_value(parse_hex_value(6, true), in_class);
                }

                return escape_value(parse_hex_value(2, false), in_class);
            }
            case 'u': {
                advance();

                if ((options_ & opt_alt_bsux) == 0) {
                    fail(err_bad_escape, start);
                }

                if (!eof() && peek() == '{') {
                    advance();
                    return escape_value(parse_hex_value(6, true), in_class);
                }

                return escape_value(parse_hex_value(4, false), in_class);
            }
            case 'o': {
                advance();
                expect('{', err_bad_escape);

                uint32_t value = 0;
                size_t digits = 0;
                while (!eof() && peek() >= '0' && peek() <= '7') {
                    value = value * 8U + static_cast<uint32_t>(peek() - '0');
                    advance();
                    ++digits;
                    if (value > 0x10FFFFU) {
                        fail(err_invalid_codepoint, start);
                    }
                }

                if (digits == 0) {
                    fail(err_bad_escape, start);
                }

                expect('}', err_bad_escape);
                return escape_value(value, in_class);
            }
            case 'c': {
                advance();
                if (eof()) {
                    fail(err_bad_escape, start);
                }
                const uint32_t control = static_cast<unsigned char>(peek()) & 0x1FU;
                advance();
                return escape_value(control, in_class);
            }
            case 'a':
                advance();
                return escape_value(0x07, in_class);
            case 'e':
                advance();
                return 0x1B;
            case 'f':
                advance();
                return escape_value(0x0C, in_class);
            case 'n':
                advance();
                return escape_value(0x0A, in_class);
            case 'r':
                advance();
                return escape_value(0x0D, in_class);
            case 't':
                advance();
                return escape_value(0x09, in_class);
            case 'v':
                advance();
                return escape_value(0x0B, in_class);
            case 'G': {
                if (in_class) {
                    fail(err_bad_escape, start);
                }

                advance();
                node entry;
                entry.type = node::kind::begin_search;
                return add_node(entry);
            }
            case 'K': {
                if (in_class) {
                    fail(err_bad_escape, start);
                }

                advance();
                node entry;
                entry.type = node::kind::keep;
                return add_node(entry);
            }
            case 'R': {
                if (in_class) {
                    fail(err_bad_escape, start);
                }

                advance();

                char_class newline_class;
                newline_class.add(0x0A, 0x0D);
                newline_class.add(0x85, 0x85);
                newline_class.add(0x2028, 0x2029);
                newline_class.normalize();
                classes.push_back(move(newline_class));

                node carriage;
                carriage.type = node::kind::literal;
                carriage.code_point = '\r';

                node line_feed;
                line_feed.type = node::kind::literal;
                line_feed.code_point = '\n';

                node crlf;
                crlf.type = node::kind::concat;
                crlf.children.push_back(add_node(carriage));
                crlf.children.push_back(add_node(line_feed));

                node single;
                single.type = node::kind::char_class;
                single.class_index = static_cast<int32_t>(classes.size() - 1);

                node alternative;
                alternative.type = node::kind::alternate;
                alternative.children.push_back(add_node(crlf));
                alternative.children.push_back(add_node(single));

                node wrapped;
                wrapped.type = node::kind::atomic;
                wrapped.children.push_back(add_node(alternative));
                return add_node(wrapped);
            }
            case 'C':
                fail(err_unsupported_feature, start);
            default:
                break;
        }

        if (kind >= '0' && kind <= '9') {
            if (kind == '0') {
                uint32_t value = 0;
                size_t digits = 0;
                while (!eof() && peek() >= '0' && peek() <= '7' && digits < 3) {
                    value = value * 8U + static_cast<uint32_t>(peek() - '0');
                    advance();
                    ++digits;
                }
                if (value > 0xFFU) {
                    fail(err_octal_too_big, start);
                }
                return escape_value(value, in_class);
            }

            int32_t number = 0;
            while (!eof() && is_digit(peek())) {
                number = number * 10 + (peek() - '0');
                if (number > 100000) {
                    fail(err_bad_backref, start);
                }
                advance();
            }

            if (in_class) {
                return number;
            }

            node entry;
            entry.type = node::kind::backref;
            entry.group = number;
            entry.caseless = caseless_;
            return add_node(entry);
        }

        if (is_word(kind)) {
            fail(err_bad_escape, start);
        }

        return escape_value(decode_literal(), in_class);
    }

    void parse_posix_class(char_class& target, const size_t start) {
        advance(2);

        bool negate = false;

        if (!eof() && peek() == '^') {
            negate = true;
            advance();
        }

        const size_t begin = pos_;
        while (!eof() && peek() != ':') {
            advance();
        }

        if (eof()) {
            fail(err_missing_class_bracket, start);
        }

        const string name(pattern_ + begin, pos_ - begin);
        advance();
        expect(']', err_missing_class_bracket);

        char_class temporary;

        if (name == "alpha") {
            if (ucp_) {
                temporary.add_categories(cat_letters);
            } else {
                temporary.add('a', 'z');
                temporary.add('A', 'Z');
            }
        } else if (name == "digit") {
            if (ucp_) {
                temporary.add_categories(cat_nd);
            } else {
                temporary.add('0', '9');
            }
        } else if (name == "alnum") {
            if (ucp_) {
                temporary.add_categories(cat_letters | cat_numbers);
            } else {
                temporary.add('a', 'z');
                temporary.add('A', 'Z');
                temporary.add('0', '9');
            }
        } else if (name == "lower") {
            if (ucp_) {
                temporary.add_categories(cat_ll);
            } else {
                temporary.add('a', 'z');
            }
        } else if (name == "upper") {
            if (ucp_) {
                temporary.add_categories(cat_lu);
            } else {
                temporary.add('A', 'Z');
            }
        } else if (name == "space") {
            add_escaped_class(temporary, 's');
        } else if (name == "blank") {
            temporary.add(' ', ' ');
            temporary.add('\t', '\t');
        } else if (name == "cntrl") {
            temporary.add(0, 0x1F);
            temporary.add(0x7F, 0x7F);
        } else if (name == "graph") {
            temporary.add(0x21, 0x7E);
        } else if (name == "print") {
            temporary.add(0x20, 0x7E);
        } else if (name == "punct") {
            temporary.add('!', '/');
            temporary.add(':', '@');
            temporary.add('[', '[');
            temporary.add('{', '~');
        } else if (name == "word") {
            add_escaped_class(temporary, 'w');
        } else if (name == "xdigit") {
            temporary.add('0', '9');
            temporary.add('a', 'f');
            temporary.add('A', 'F');
        } else if (name == "ascii") {
            temporary.add(0, 0x7F);
        } else {
            fail(err_missing_class_bracket, start);
        }

        temporary.normalize();

        if (negate) {
            add_complement(target, temporary);
        } else {
            for (const auto& entry: temporary.ranges) {
                target.add(entry.first, entry.last);
            }
        }
    }

    int32_t parse_class() {
        const size_t start = pos_;
        advance();

        char_class entry;
        entry.caseless = caseless_;

        if (!eof() && peek() == '^') {
            entry.negate = true;
            advance();
        }

        bool first = true;
        bool closed = false;

        while (!eof()) {
            if (peek() == ']' && first && pos_ + 1 >= length_) {
                advance();
                closed = true;
                break;
            }

            if (peek() == ']' && !first) {
                advance();
                closed = true;
                break;
            }

            if (peek() == ']' && first) {
                advance();
                entry.add(']', ']');
                first = false;
                continue;
            }

            if (peek() == '[' && peek(1) == ':') {
                parse_posix_class(entry, start);
                first = false;
                continue;
            }

            uint32_t low = 0;

            if (peek() == '\\') {
                const int32_t escaped = parse_escape(true, &entry);

                if (escaped < 0) {
                    first = false;
                    continue;
                }

                low = static_cast<uint32_t>(escaped);
            } else {
                low = decode_literal();
            }

            if (!eof(1) && peek() == '-' && peek(1) != ']') {
                advance();

                uint32_t high = 0;

                if (peek() == '\\') {
                    const int32_t escaped = parse_escape(true, &entry);

                    if (escaped < 0) {
                        entry.add(low, low);
                        first = false;
                        continue;
                    }

                    high = static_cast<uint32_t>(escaped);
                } else {
                    high = decode_literal();
                }

                if (high < low) {
                    fail(err_class_range_order, pos_);
                }

                entry.add(low, high);
            } else {
                entry.add(low, low);
            }

            first = false;
        }

        if (!closed) {
            fail(err_missing_class_bracket, start);
        }

        if (entry.ranges.empty() && (options_ & opt_allow_empty_class) == 0) {
            fail(err_missing_class_bracket, start);
        }

        entry.normalize();
        classes.push_back(move(entry));

        node result;
        result.type = node::kind::char_class;
        result.class_index = static_cast<int32_t>(classes.size() - 1);
        return add_node(result);
    }

    int32_t finish_wrapped(const node::kind type, const int look_kind, const size_t start) {
        const int32_t body = parse_alternation();
        expect(')', err_missing_parenthesis);

        node entry;
        entry.type = type;
        entry.look_kind = look_kind;
        entry.offset = start;
        entry.children.push_back(body);
        return add_node(entry);
    }
    int32_t parse_conditional(const size_t start) {
        advance(2);

        bool define_block = false;
        int32_t number = -1;

        if (!eof() && is_digit(peek())) {
            number = 0;

            while (!eof() && is_digit(peek())) {
                number = number * 10 + (peek() - '0');
                advance();
            }
        } else if (!eof() && is_word(peek())) {
            const size_t begin = pos_;

            while (!eof() && is_word(peek())) {
                advance();
            }

            const string name(pattern_ + begin, pos_ - begin);

            if (name == "DEFINE") {
                define_block = true;
            } else {
                number = find_name(name.view());

                if (number < 0) {
                    fail(err_unknown_group_reference, start);
                }
            }
        } else {
            fail(err_unrecognized_group, start);
        }

        expect(')', err_unrecognized_group);

        if (define_block) {
            const int32_t body = parse_alternation();
            expect(')', err_missing_parenthesis);

            node entry;
            entry.type = node::kind::conditional;
            entry.group = -1;
            entry.offset = start;
            entry.children.push_back(body);
            return add_node(entry);
        }

        const int32_t yes_branch = parse_concat();
        int32_t no_branch = -1;

        if (!eof() && peek() == '|') {
            advance();
            no_branch = parse_concat();
        }

        expect(')', err_missing_parenthesis);

        node entry;
        entry.type = node::kind::conditional;
        entry.group = number;
        entry.offset = start;
        entry.children.push_back(yes_branch);

        if (no_branch >= 0) {
            entry.children.push_back(no_branch);
        }

        return add_node(entry);
    }

    int32_t parse_group() {
        const size_t start = pos_;
        advance();

        const bool saved_caseless = caseless_;
        const bool saved_dotall = dotall_;
        const bool saved_multiline = multiline_;
        const bool saved_extended = extended_;
        const bool saved_ungreedy = ungreedy_;

        const auto restore = [&]() noexcept {
            caseless_ = saved_caseless;
            dotall_ = saved_dotall;
            multiline_ = saved_multiline;
            extended_ = saved_extended;
            ungreedy_ = saved_ungreedy;
        };

        if (eof() || peek() != '?') {
            int capture = -1;

            if ((options_ & opt_no_auto_capture) == 0) {
                capture = ++capture_count;
            }

            const int32_t body = parse_alternation();
            expect(')', err_missing_parenthesis);
            restore();

            if (capture < 0) {
                return body;
            }

            node entry;
            entry.type = node::kind::capture;
            entry.capture = capture;
            entry.offset = start;
            entry.children.push_back(body);
            return add_node(entry);
        }

        if (eof(1)) {
            fail(err_missing_parenthesis, start);
        }

        const char kind = peek(1);

        if (kind == '(') {
            return parse_conditional(start);
        }

        switch (kind) {
            case '#': {
                advance(2);
                while (!eof() && peek() != ')') {
                    advance();
                }
                expect(')', err_missing_parenthesis);
                restore();
                return -1;
            }
            case ':': {
                advance(2);
                const int32_t body = parse_alternation();
                expect(')', err_missing_parenthesis);
                restore();
                return body;
            }
            case '=':
                advance(2);
                return finish_wrapped(node::kind::look, 1, start);
            case '!':
                advance(2);
                return finish_wrapped(node::kind::look, 2, start);
            case '>':
                advance(2);
                return finish_wrapped(node::kind::atomic, 0, start);
            case '|': {
                advance(2);
                const int base = capture_count;
                int highest = base;
                vector<int32_t> branches;

                for (;;) {
                    capture_count = base;
                    branches.push_back(parse_concat());
                    highest = max(capture_count, highest);

                    if (!eof() && peek() == '|') {
                        advance();
                        continue;
                    }

                    break;
                }

                expect(')', err_missing_parenthesis);
                capture_count = highest;
                restore();

                if (branches.size() == 1) {
                    return branches[0];
                }

                node entry;
                entry.type = node::kind::alternate;
                entry.offset = start;
                entry.children = move(branches);
                return add_node(entry);
            }
            case '<': {
                if (!eof(2) && peek(2) == '=') {
                    advance(3);
                    return finish_wrapped(node::kind::look, 3, start);
                }

                if (!eof(2) && peek(2) == '!') {
                    advance(3);
                    return finish_wrapped(node::kind::look, 4, start);
                }

                advance(2);
                const string name = parse_name();
                expect('>', err_bad_group_name);

                const int number = ++capture_count;
                register_name(name.view(), number);

                const int32_t body = parse_alternation();
                expect(')', err_missing_parenthesis);
                restore();

                node entry;
                entry.type = node::kind::capture;
                entry.capture = number;
                entry.offset = start;
                entry.children.push_back(body);
                return add_node(entry);
            }
            case '\'': {
                advance(2);
                const string name = parse_name();
                expect('\'', err_bad_group_name);

                const int number = ++capture_count;
                register_name(name.view(), number);

                const int32_t body = parse_alternation();
                expect(')', err_missing_parenthesis);
                restore();

                node entry;
                entry.type = node::kind::capture;
                entry.capture = number;
                entry.offset = start;
                entry.children.push_back(body);
                return add_node(entry);
            }
            case 'P': {
                if (eof(2)) {
                    fail(err_missing_parenthesis, start);
                }

                const char mode = peek(2);

                if (mode == '<') {
                    advance(3);
                    const string name = parse_name();
                    expect('>', err_bad_group_name);

                    const int number = ++capture_count;
                    register_name(name.view(), number);

                    const int32_t body = parse_alternation();
                    expect(')', err_missing_parenthesis);
                    restore();

                    node entry;
                    entry.type = node::kind::capture;
                    entry.capture = number;
                    entry.offset = start;
                    entry.children.push_back(body);
                    return add_node(entry);
                }

                if (mode == '=') {
                    advance(3);
                    const string name = parse_name();
                    expect(')', err_missing_parenthesis);

                    const int number = find_name(name.view());

                    if (number < 0) {
                        fail(err_unknown_group_reference, start);
                    }

                    node entry;
                    entry.type = node::kind::backref;
                    entry.group = number;
                    entry.caseless = caseless_;
                    entry.offset = start;
                    return add_node(entry);
                }

                if (mode == '>') {
                    advance(3);
                    const string name = parse_name();
                    expect(')', err_missing_parenthesis);

                    const int number = find_name(name.view());

                    if (number < 0) {
                        fail(err_unknown_group_reference, start);
                    }

                    node entry;
                    entry.type = node::kind::call;
                    entry.group = number;
                    entry.offset = start;
                    return add_node(entry);
                }

                fail(err_unrecognized_group, start);
            }
            case 'R':
            case '&': {
                if (kind == 'R') {
                    advance(2);
                    expect(')', err_missing_parenthesis);

                    node entry;
                    entry.type = node::kind::call;
                    entry.group = 0;
                    entry.offset = start;
                    return add_node(entry);
                }

                advance(2);
                const string name = parse_name();
                expect(')', err_missing_parenthesis);

                const int number = find_name(name.view());

                if (number < 0) {
                    fail(err_unknown_group_reference, start);
                }

                node entry;
                entry.type = node::kind::call;
                entry.group = number;
                entry.offset = start;
                return add_node(entry);
            }
            case '*':
                fail(err_unsupported_verb, start);
            default:
                break;
        }

        if (is_digit(kind)) {
            advance();
            int number = 0;

            while (!eof() && is_digit(peek())) {
                number = number * 10 + (peek() - '0');
                advance();
            }

            expect(')', err_missing_parenthesis);

            node entry;
            entry.type = node::kind::call;
            entry.group = number;
            entry.offset = start;
            return add_node(entry);
        }

        if (kind == 'i' || kind == 'm' || kind == 's' || kind == 'x' || kind == 'U' || kind == '-' || kind == '^') {
            advance(1);

            bool enable = true;
            bool seen = false;

            for (;;) {
                if (eof()) {
                    fail(err_missing_parenthesis, start);
                }

                const char flag = peek();

                if (flag == '-') {
                    enable = false;
                    advance();
                    continue;
                }

                if (flag == '^') {
                    caseless_ = false;
                    dotall_ = false;
                    multiline_ = false;
                    extended_ = false;
                    ungreedy_ = false;
                    seen = true;
                    advance();
                    continue;
                }

                if (flag == 'i') {
                    caseless_ = enable;
                } else if (flag == 'm') {
                    multiline_ = enable;
                } else if (flag == 's') {
                    dotall_ = enable;
                } else if (flag == 'x') {
                    extended_ = enable;
                } else if (flag == 'U') {
                    ungreedy_ = enable;
                } else if (flag == ')') {
                    if (!seen) {
                        fail(err_unrecognized_group, start);
                    }
                    advance();
                    return -1;
                } else if (flag == ':') {
                    advance();
                    const int32_t body = parse_alternation();
                    expect(')', err_missing_parenthesis);
                    restore();
                    return body;
                } else {
                    fail(err_unrecognized_group, pos_);
                }

                seen = true;
                advance();
            }
        }

        fail(err_unrecognized_group, start);
    }

    const char* pattern_;
    size_t length_;
    size_t pos_ = 0;
    uint32_t options_;
    bool caseless_;
    bool dotall_;
    bool multiline_;
    bool extended_;
    bool ungreedy_;
    bool ucp_;
    bool utf_;
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_STRING_REGEX_ENGINE_PARSER_SRC__
