#include "regex_builtin.hpp"
#include "regex_ucd_tables.hpp"
#include <NeForce/core/algorithm/sort.hpp>
#include <NeForce/core/string/string.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    constexpr size_t invalid_offset = numeric_traits<size_t>::max();
    constexpr int maximum_steps = 20000000;
    constexpr int maximum_recursion = 1000;

    enum : uint32_t {
        opt_allow_empty_class = 0x00000001U,
        opt_alt_bsux = 0x00000002U,
        opt_caseless = 0x00000008U,
        opt_dollar_endonly = 0x00000010U,
        opt_dotall = 0x00000020U,
        opt_dupnames = 0x00000040U,
        opt_extended = 0x00000080U,
        opt_firstline = 0x00000100U,
        opt_match_unset_backref = 0x00000200U,
        opt_multiline = 0x00000400U,
        opt_no_auto_capture = 0x00002000U,
        opt_no_auto_possess = 0x00004000U,
        opt_no_dotstar_anchor = 0x00008000U,
        opt_no_start_optimize = 0x00010000U,
        opt_ucp = 0x00020000U,
        opt_ungreedy = 0x00040000U,
        opt_utf = 0x00080000U,
        opt_never_backslash_c = 0x00100000U,
        opt_alt_circumflex = 0x00200000U,
        opt_alt_verbnames = 0x00400000U,
        opt_use_offset_limit = 0x00800000U,
        opt_extended_more = 0x01000000U,
        opt_literal = 0x02000000U,
    };

    constexpr uint32_t accepted_options = opt_allow_empty_class | opt_alt_bsux | opt_caseless | opt_dollar_endonly |
                                          opt_dotall | opt_dupnames | opt_extended | opt_firstline |
                                          opt_match_unset_backref | opt_multiline | opt_no_auto_capture | opt_ucp |
                                          opt_ungreedy | opt_utf | opt_alt_circumflex | opt_literal;

    enum : int {
        err_none = 0,
        err_missing_parenthesis = 1,
        err_unmatched_parenthesis = 2,
        err_quantifier_without_item = 3,
        err_missing_class_bracket = 4,
        err_class_range_order = 5,
        err_unrecognized_group = 6,
        err_quantifier_too_big = 7,
        err_bad_escape = 8,
        err_bad_hex_escape = 9,
        err_bad_backref = 10,
        err_unknown_group_reference = 11,
        err_lookbehind_not_fixed = 12,
        err_unsupported_feature = 13,
        err_unsupported_option = 14,
        err_bad_utf8_pattern = 15,
        err_duplicate_group_name = 16,
        err_unknown_property = 17,
        err_bad_group_name = 18,
        err_invalid_codepoint = 19,
        err_pattern_too_large = 20,
        err_nested_quantifier = 21,
        err_unsupported_verb = 22,
        err_octal_too_big = 23,
    };

    NEFORCE_NODISCARD const char* error_message(const int code) noexcept {
        switch (code) {
            case err_missing_parenthesis:
                return "missing closing parenthesis";
            case err_unmatched_parenthesis:
                return "unmatched closing parenthesis";
            case err_quantifier_without_item:
                return "quantifier does not follow a repeatable item";
            case err_missing_class_bracket:
                return "missing terminating ] for character class";
            case err_class_range_order:
                return "range out of order in character class";
            case err_unrecognized_group:
                return "unrecognized character after (? or (?-";
            case err_quantifier_too_big:
                return "number too big in {} quantifier";
            case err_bad_escape:
                return "unrecognized character follows backslash";
            case err_bad_hex_escape:
                return "invalid hexadecimal escape sequence";
            case err_bad_backref:
                return "invalid back reference";
            case err_unknown_group_reference:
                return "reference to a non-existent subpattern";
            case err_lookbehind_not_fixed:
                return "lookbehind assertion is not bounded";
            case err_unsupported_feature:
                return "pattern uses a construct the builtin engine does not support";
            case err_unsupported_option:
                return "unsupported regular expression option";
            case err_bad_utf8_pattern:
                return "invalid UTF-8 sequence in pattern";
            case err_duplicate_group_name:
                return "two named subpatterns have the same name";
            case err_unknown_property:
                return "unknown property name after \\P or \\p";
            case err_bad_group_name:
                return "invalid subpattern name";
            case err_invalid_codepoint:
                return "code point value is greater than the maximum";
            case err_pattern_too_large:
                return "regular expression is too large";
            case err_nested_quantifier:
                return "nested quantifier";
            case err_unsupported_verb:
                return "unsupported backtracking verb";
            case err_octal_too_big:
                return "octal value is greater than \\377";
            default:
                return "regular expression compilation failed";
        }
    }

    struct compile_failure {
        int code;
        size_t offset;
    };

    NEFORCE_NODISCARD constexpr bool is_utf8_trail(const unsigned char byte) noexcept {
        return (byte & 0xC0U) == 0x80U;
    }

    NEFORCE_NODISCARD size_t utf8_sequence_length(const unsigned char lead) noexcept {
        if (lead < 0x80U) {
            return 1;
        }
        if ((lead & 0xE0U) == 0xC0U) {
            return 2;
        }
        if ((lead & 0xF0U) == 0xE0U) {
            return 3;
        }
        if ((lead & 0xF8U) == 0xF0U) {
            return 4;
        }
        return 0;
    }

    NEFORCE_NODISCARD size_t utf8_decode(const unsigned char* const data, const size_t available,
                                         uint32_t& code_point) noexcept {
        if (available == 0) {
            return 0;
        }

        const unsigned char lead = data[0];
        const size_t length = utf8_sequence_length(lead);

        if (length == 0 || available < length) {
            return 0;
        }

        if (length == 1) {
            code_point = lead;
            return 1;
        }

        uint32_t value = lead & static_cast<unsigned char>((0x7FU >> length));
        for (size_t index = 1; index < length; ++index) {
            if (!is_utf8_trail(data[index])) {
                return 0;
            }
            value = (value << 6U) | static_cast<uint32_t>(data[index] & 0x3FU);
        }

        if ((length == 2 && value < 0x80U) || (length == 3 && value < 0x800U) || (length == 4 && value < 0x10000U) ||
            value > 0x10FFFFU || (value >= 0xD800U && value <= 0xDFFFU)) {
            return 0;
        }

        code_point = value;
        return length;
    }

    NEFORCE_NODISCARD size_t utf8_encode(const uint32_t code_point, char* const output) noexcept {
        if (code_point < 0x80U) {
            output[0] = static_cast<char>(code_point);
            return 1;
        }
        if (code_point < 0x800U) {
            output[0] = static_cast<char>(0xC0U | (code_point >> 6U));
            output[1] = static_cast<char>(0x80U | (code_point & 0x3FU));
            return 2;
        }
        if (code_point < 0x10000U) {
            output[0] = static_cast<char>(0xE0U | (code_point >> 12U));
            output[1] = static_cast<char>(0x80U | ((code_point >> 6U) & 0x3FU));
            output[2] = static_cast<char>(0x80U | (code_point & 0x3FU));
            return 3;
        }
        output[0] = static_cast<char>(0xF0U | (code_point >> 18U));
        output[1] = static_cast<char>(0x80U | ((code_point >> 12U) & 0x3FU));
        output[2] = static_cast<char>(0x80U | ((code_point >> 6U) & 0x3FU));
        output[3] = static_cast<char>(0x80U | (code_point & 0x3FU));
        return 4;
    }

    NEFORCE_NODISCARD uint16_t category_of(const uint32_t code_point) noexcept {
        size_t low = 0;
        size_t high = sizeof(regex_ucd::ranges) / sizeof(regex_ucd::ranges[0]);

        while (low < high) {
            const size_t middle = low + (high - low) / 2;

            if (code_point < regex_ucd::ranges[middle].start) {
                high = middle;
            } else if (code_point > regex_ucd::ranges[middle].end) {
                low = middle + 1;
            } else {
                return regex_ucd::ranges[middle].category;
            }
        }

        return regex_ucd::category_cn;
    }

    template <size_t Length>
    NEFORCE_NODISCARD uint32_t case_lookup(const regex_ucd::case_entry (&table)[Length], const uint32_t code_point,
                                           const uint32_t fallback) noexcept {
        size_t low = 0;
        size_t high = Length;

        while (low < high) {
            const size_t middle = low + (high - low) / 2;

            if (code_point < table[middle].source) {
                high = middle;
            } else if (code_point > table[middle].source) {
                low = middle + 1;
            } else {
                return table[middle].target;
            }
        }

        return fallback;
    }

    NEFORCE_NODISCARD uint32_t to_lower(const uint32_t code_point) noexcept {
        return case_lookup(regex_ucd::lower_map, code_point, code_point);
    }

    NEFORCE_NODISCARD uint32_t to_upper(const uint32_t code_point) noexcept {
        return case_lookup(regex_ucd::upper_map, code_point, code_point);
    }

    NEFORCE_NODISCARD bool is_ascii_digit(const uint32_t code_point) noexcept {
        return code_point >= '0' && code_point <= '9';
    }

    NEFORCE_NODISCARD bool is_ascii_word(const uint32_t code_point) noexcept {
        return (code_point >= 'a' && code_point <= 'z') || (code_point >= 'A' && code_point <= 'Z') ||
               is_ascii_digit(code_point) || code_point == '_';
    }

    NEFORCE_NODISCARD bool is_ascii_space(const uint32_t code_point) noexcept {
        return code_point == ' ' || code_point == '\t' || code_point == '\n' || code_point == '\r' ||
               code_point == '\f' || code_point == '\v';
    }

    NEFORCE_NODISCARD bool is_unicode_word(const uint32_t code_point) noexcept {
        if (code_point < 0x80U) {
            return is_ascii_word(code_point);
        }

        const uint16_t category = category_of(code_point);
        return category >= 1 && category <= 11;
    }

    NEFORCE_NODISCARD bool is_unicode_space(const uint32_t code_point) noexcept {
        if (code_point < 0x80U) {
            return is_ascii_space(code_point);
        }

        if (code_point == 0x85U) {
            return true;
        }

        const uint16_t category = category_of(code_point);
        return category >= 23 && category <= 25;
    }

    struct code_range {
        uint32_t first;
        uint32_t last;
    };

    struct char_class {
        bool negate = false;
        bool caseless = false;
        vector<code_range> ranges;

        void add(const uint32_t first, const uint32_t last) { ranges.push_back(code_range{first, last}); }

        void add_ascii_class(char kind);
        void add_categories(uint32_t mask);
        void add_script(int script);
        void normalize();
        NEFORCE_NODISCARD bool contains(const uint32_t code_point) const noexcept;
    };

    struct node {
        enum class kind : uint8_t {
            empty,
            literal,
            any,
            char_class,
            concat,
            alternate,
            repeat,
            capture,
            group,
            look,
            atomic,
            backref,
            begin_line,
            end_line,
            begin_subject,
            end_subject,
            end_subject_final,
            word_boundary,
            non_word_boundary,
            cluster,
            call,
        };

        enum class repeat_mode : uint8_t {
            greedy = 0,
            lazy = 1,
            possessive = 2,
        };

        kind type = kind::empty;
        uint32_t code_point = 0;
        int32_t class_index = -1;
        int32_t capture = -1;
        int32_t minimum = 0;
        int32_t maximum = -1;
        repeat_mode mode = repeat_mode::greedy;
        bool caseless = false;
        bool dotall = false;
        bool multiline = false;
        bool ucp = false;
        int32_t look_kind = 0;
        int32_t group = 0;
        size_t offset = 0;
        vector<int32_t> children;
    };

    enum class op : uint8_t {
        fail = 0,
        match,
        accept,
        char_match,
        any,
        char_class,
        split,
        jump,
        save,
        begin_line,
        end_line,
        begin_subject,
        end_subject,
        end_subject_final,
        word_boundary,
        non_word_boundary,
        backref,
        look,
        atomic,
        call,
        cluster,
        loop_enter,
        loop_check,
    };

    struct instruction {
        op code = op::fail;
        int32_t a = 0;
        int32_t b = 0;
        int32_t c = 0;
        int32_t d = 0;
        int32_t e = 0;
    };

    void char_class::normalize() {
        if (ranges.empty()) {
            return;
        }

        sort(ranges.begin(), ranges.end(),
             [](const code_range& left, const code_range& right) { return left.first < right.first; });

        size_t merged = 1;
        for (size_t index = 1; index < ranges.size(); ++index) {
            if (ranges[index].first <= ranges[merged - 1].last + 1) {
                ranges[merged - 1].last = max(ranges[index].last, ranges[merged - 1].last);
            } else {
                ranges[merged++] = ranges[index];
            }
        }

        ranges.resize(merged);
    }

    NEFORCE_NODISCARD bool char_class::contains(const uint32_t code_point) const noexcept {
        const auto search = [this](const uint32_t value) noexcept -> bool {
            size_t low = 0;
            size_t high = ranges.size();

            while (low < high) {
                const size_t middle = low + (high - low) / 2;

                if (value < ranges[middle].first) {
                    high = middle;
                } else if (value > ranges[middle].last) {
                    low = middle + 1;
                } else {
                    return true;
                }
            }

            return false;
        };

        bool hit = search(code_point);

        if (!hit && caseless) {
            hit = search(to_lower(code_point)) || search(to_upper(code_point));
        }

        return hit != negate;
    }

    void char_class::add_ascii_class(const char kind) {
        switch (kind) {
            case 'd':
                add('0', '9');
                break;
            case 'w':
                add('a', 'z');
                add('A', 'Z');
                add('0', '9');
                add('_', '_');
                break;
            default:
                add(0x09, 0x0D);
                add(' ', ' ');
                break;
        }
    }

    void char_class::add_categories(const uint32_t mask) {
        for (const auto& entry: regex_ucd::ranges) {
            if ((mask & (1U << (entry.category - 1U))) != 0U) {
                add(entry.start, entry.end);
            }
        }
    }

    void char_class::add_script(const int script) {
        switch (script) {
            case 1: // Han
                add(0x2E80, 0x2EF3);
                add(0x2F00, 0x2FD5);
                add(0x3005, 0x3005);
                add(0x3007, 0x3007);
                add(0x3021, 0x3029);
                add(0x3038, 0x303B);
                add(0x3400, 0x4DBF);
                add(0x4E00, 0x9FFF);
                add(0xF900, 0xFAFF);
                add(0x20000, 0x2A6DF);
                add(0x2A700, 0x2EBEF);
                add(0x2F800, 0x2FA1F);
                add(0x30000, 0x3134F);
                break;
            case 2: // Hiragana
                add(0x3041, 0x309F);
                add(0x1B001, 0x1B11F);
                break;
            case 3: // Katakana
                add(0x30A1, 0x30FF);
                add(0x31F0, 0x31FF);
                add(0xFF66, 0xFF9D);
                add(0x1B000, 0x1B000);
                break;
            case 4: // Hangul
                add(0x1100, 0x11FF);
                add(0x3131, 0x318E);
                add(0xA960, 0xA97F);
                add(0xAC00, 0xD7A3);
                add(0xD7B0, 0xD7FF);
                break;
            default:
                break;
        }
    }

    enum : uint32_t {
        cat_lu = 1U << 0U,
        cat_ll = 1U << 1U,
        cat_lt = 1U << 2U,
        cat_lm = 1U << 3U,
        cat_lo = 1U << 4U,
        cat_mn = 1U << 5U,
        cat_mc = 1U << 6U,
        cat_me = 1U << 7U,
        cat_nd = 1U << 8U,
        cat_nl = 1U << 9U,
        cat_no = 1U << 10U,
        cat_pc = 1U << 11U,
        cat_pd = 1U << 12U,
        cat_ps = 1U << 13U,
        cat_pe = 1U << 14U,
        cat_pi = 1U << 15U,
        cat_pf = 1U << 16U,
        cat_po = 1U << 17U,
        cat_sm = 1U << 18U,
        cat_sc = 1U << 19U,
        cat_sk = 1U << 20U,
        cat_so = 1U << 21U,
        cat_zs = 1U << 22U,
        cat_zl = 1U << 23U,
        cat_zp = 1U << 24U,
        cat_cc = 1U << 25U,
        cat_cf = 1U << 26U,
        cat_co = 1U << 27U,
        cat_cs = 1U << 28U,
        cat_cn = 1U << 29U,
    };

    constexpr uint32_t cat_letters = cat_lu | cat_ll | cat_lt | cat_lm | cat_lo;
    constexpr uint32_t cat_numbers = cat_nd | cat_nl | cat_no;
    constexpr uint32_t cat_marks = cat_mn | cat_mc | cat_me;
    constexpr uint32_t cat_punctuation = cat_pc | cat_pd | cat_ps | cat_pe | cat_pi | cat_pf | cat_po;
    constexpr uint32_t cat_symbols = cat_sm | cat_sc | cat_sk | cat_so;
    constexpr uint32_t cat_separators = cat_zs | cat_zl | cat_zp;
    constexpr uint32_t cat_others = cat_cc | cat_cf | cat_co | cat_cs | cat_cn;

    NEFORCE_NODISCARD bool property_categories(const string& name, uint32_t& mask) noexcept {
        if (name == "l" || name == "letter") {
            mask = cat_letters;
        } else if (name == "lu" || name == "uppercase_letter") {
            mask = cat_lu;
        } else if (name == "ll" || name == "lowercase_letter") {
            mask = cat_ll;
        } else if (name == "lt" || name == "titlecase_letter") {
            mask = cat_lt;
        } else if (name == "lm" || name == "modifier_letter") {
            mask = cat_lm;
        } else if (name == "lo" || name == "other_letter") {
            mask = cat_lo;
        } else if (name == "lc" || name == "cased_letter") {
            mask = cat_lu | cat_ll | cat_lt;
        } else if (name == "m" || name == "mark") {
            mask = cat_marks;
        } else if (name == "mn" || name == "nonspacing_mark") {
            mask = cat_mn;
        } else if (name == "mc" || name == "spacing_mark") {
            mask = cat_mc;
        } else if (name == "me" || name == "enclosing_mark") {
            mask = cat_me;
        } else if (name == "n" || name == "number") {
            mask = cat_numbers;
        } else if (name == "nd" || name == "decimal_number") {
            mask = cat_nd;
        } else if (name == "nl" || name == "letter_number") {
            mask = cat_nl;
        } else if (name == "no" || name == "other_number") {
            mask = cat_no;
        } else if (name == "p" || name == "punctuation") {
            mask = cat_punctuation;
        } else if (name == "pc" || name == "connector_punctuation") {
            mask = cat_pc;
        } else if (name == "pd" || name == "dash_punctuation") {
            mask = cat_pd;
        } else if (name == "ps" || name == "open_punctuation") {
            mask = cat_ps;
        } else if (name == "pe" || name == "close_punctuation") {
            mask = cat_pe;
        } else if (name == "pi" || name == "initial_punctuation") {
            mask = cat_pi;
        } else if (name == "pf" || name == "final_punctuation") {
            mask = cat_pf;
        } else if (name == "po" || name == "other_punctuation") {
            mask = cat_po;
        } else if (name == "s" || name == "symbol") {
            mask = cat_symbols;
        } else if (name == "sm" || name == "math_symbol") {
            mask = cat_sm;
        } else if (name == "sc" || name == "currency_symbol") {
            mask = cat_sc;
        } else if (name == "sk" || name == "modifier_symbol") {
            mask = cat_sk;
        } else if (name == "so" || name == "other_symbol") {
            mask = cat_so;
        } else if (name == "z" || name == "separator") {
            mask = cat_separators;
        } else if (name == "zs" || name == "space_separator") {
            mask = cat_zs;
        } else if (name == "zl" || name == "line_separator") {
            mask = cat_zl;
        } else if (name == "zp" || name == "paragraph_separator") {
            mask = cat_zp;
        } else if (name == "c" || name == "other") {
            mask = cat_others;
        } else if (name == "cc" || name == "control") {
            mask = cat_cc;
        } else if (name == "cf" || name == "format") {
            mask = cat_cf;
        } else if (name == "co" || name == "private_use") {
            mask = cat_co;
        } else if (name == "cs" || name == "surrogate") {
            mask = cat_cs;
        } else if (name == "cn" || name == "unassigned") {
            mask = cat_cn;
        } else {
            return false;
        }

        return true;
    }

    NEFORCE_NODISCARD int property_script(const string& name) noexcept {
        if (name == "han") {
            return 1;
        }
        if (name == "hiragana") {
            return 2;
        }
        if (name == "katakana") {
            return 3;
        }
        if (name == "hangul") {
            return 4;
        }
        return 0;
    }

    void apply_property(char_class& target, const string& name) {
        if (name == "any") {
            target.add(0, 0x10FFFF);
            return;
        }
        if (name == "ascii") {
            target.add(0, 0x7F);
            return;
        }

        const int script = property_script(name);
        if (script != 0) {
            target.add_script(script);
            return;
        }

        uint32_t mask = 0;
        if (property_categories(name, mask)) {
            target.add_categories(mask);
            return;
        }

        if (name == "xan" || name == "alnum") {
            target.add_categories(cat_letters | cat_numbers);
            return;
        }
        if (name == "xwd" || name == "word") {
            target.add_categories(cat_letters | cat_numbers);
            target.add('_', '_');
            return;
        }
        if (name == "xsp" || name == "space") {
            target.add_categories(cat_separators);
            target.add_ascii_class('s');
            target.add(0x85, 0x85);
            return;
        }
        if (name == "alpha") {
            target.add_categories(cat_letters);
            return;
        }
        if (name == "digit") {
            target.add_categories(cat_nd);
            return;
        }
        if (name == "upper") {
            target.add_categories(cat_lu);
            return;
        }
        if (name == "lower") {
            target.add_categories(cat_ll);
            return;
        }
        if (name == "punct") {
            target.add_categories(cat_punctuation | cat_symbols);
            return;
        }
        if (name == "graph") {
            target.add(0x21, 0x7E);
            return;
        }
        if (name == "print") {
            target.add(0x20, 0x7E);
            return;
        }
        if (name == "blank") {
            target.add(' ', ' ');
            target.add('\t', '\t');
            return;
        }
        if (name == "cntrl") {
            target.add_categories(cat_cc);
            return;
        }
        if (name == "xdigit") {
            target.add('0', '9');
            target.add('a', 'f');
            target.add('A', 'F');
            return;
        }

        throw compile_failure{err_unknown_property, invalid_offset};
    }


    NEFORCE_NODISCARD constexpr bool is_digit_char(const char value) noexcept { return value >= '0' && value <= '9'; }

    NEFORCE_NODISCARD constexpr bool is_name_char(const char value) noexcept {
        return (value >= 'a' && value <= 'z') || (value >= 'A' && value <= 'Z') || is_digit_char(value) || value == '_';
    }

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
                while (!eof() && (peek() == ' ' || peek() == '\t' || peek() == '\n' || peek() == '\r' ||
                                  peek() == '\f' || peek() == '\v')) {
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

            while (!eof() && is_name_char(peek())) {
                advance();
            }

            if (pos_ == start) {
                fail(err_bad_group_name, start);
            }

            return string(pattern_ + start, pos_ - start);
        }

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

            if (eof() || !is_digit_char(peek())) {
                pos_ = start;
                return false;
            }

            int32_t value = 0;
            while (!eof() && is_digit_char(peek())) {
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

                if (!eof() && is_digit_char(peek())) {
                    int32_t upper = 0;
                    while (!eof() && is_digit_char(peek())) {
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
            return is_digit_char(value) || (value >= 'a' && value <= 'f') || (value >= 'A' && value <= 'F');
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

                        if (!name.empty() && is_digit_char(name[0])) {
                            number = 0;
                            for (size_t index = 0; index < name.length(); ++index) {
                                if (!is_digit_char(name[index])) {
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
                        if (!name.empty() && is_digit_char(name[0])) {
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
                case 'G':
                case 'K':
                case 'R':
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
                while (!eof() && is_digit_char(peek())) {
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

            if (is_name_char(kind)) {
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

            if (is_digit_char(kind)) {
                advance();
                int number = 0;

                while (!eof() && is_digit_char(peek())) {
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
    class compiler {
    public:
        compiler(const vector<node>& entries, const vector<char_class>& classes, const int capture_count) :
        nodes_(entries),
        classes_(classes),
        capture_entry_(static_cast<size_t>(capture_count) + 1, -1) {}

        void configure(const uint32_t options) noexcept {
            utf_ = (options & opt_utf) != 0;
            dollar_endonly_ = (options & opt_dollar_endonly) != 0;
            multiline_circumflex_ = (options & opt_alt_circumflex) != 0;
        }

        void compile(const int32_t root) {
            emit(op::save, 0);
            compile_node(root);
            emit(op::save, 1);
            emit(op::match);

            for (const auto& fixup: call_fixups_) {
                int32_t target = 0;

                if (fixup.group != 0) {
                    if (static_cast<size_t>(fixup.group) >= capture_entry_.size() ||
                        capture_entry_[static_cast<size_t>(fixup.group)] < 0) {
                        throw compile_failure{err_unknown_group_reference, 0};
                    }
                    target = capture_entry_[static_cast<size_t>(fixup.group)];
                }

                code[static_cast<size_t>(fixup.ip)].a = target;
            }
        }

        vector<instruction> code;
        int loop_count = 0;

    private:
        struct call_fixup {
            int32_t ip;
            int32_t group;
        };

        int32_t emit(const op kind, const int32_t a = 0, const int32_t b = 0, const int32_t c = 0, const int32_t d = 0,
                     const int32_t e = 0) {
            instruction entry;
            entry.code = kind;
            entry.a = a;
            entry.b = b;
            entry.c = c;
            entry.d = d;
            entry.e = e;
            code.push_back(entry);
            return static_cast<int32_t>(code.size() - 1);
        }

        void measure(const int32_t index, int64_t& minimum, int64_t& maximum) const {
            const node& entry = nodes_[static_cast<size_t>(index)];
            minimum = 0;
            maximum = 0;

            switch (entry.type) {
                case node::kind::empty:
                case node::kind::begin_line:
                case node::kind::end_line:
                case node::kind::begin_subject:
                case node::kind::end_subject:
                case node::kind::end_subject_final:
                case node::kind::word_boundary:
                case node::kind::non_word_boundary:
                case node::kind::look:
                case node::kind::atomic:
                    return;
                case node::kind::literal:
                    minimum = 1;
                    maximum = utf_ ? 4 : 1;
                    return;
                case node::kind::any:
                case node::kind::char_class:
                case node::kind::cluster:
                    minimum = 1;
                    maximum = utf_ ? 32 : 1;
                    return;
                case node::kind::backref:
                case node::kind::call:
                    maximum = -1;
                    return;
                case node::kind::concat:
                    for (const int32_t child: entry.children) {
                        int64_t child_minimum = 0;
                        int64_t child_maximum = 0;
                        measure(child, child_minimum, child_maximum);
                        minimum += child_minimum;

                        if (maximum < 0 || child_maximum < 0) {
                            maximum = -1;
                        } else {
                            maximum += child_maximum;
                        }
                    }
                    return;
                case node::kind::alternate: {
                    bool first = true;

                    for (const int32_t child: entry.children) {
                        int64_t child_minimum = 0;
                        int64_t child_maximum = 0;
                        measure(child, child_minimum, child_maximum);

                        if (first || child_minimum < minimum) {
                            minimum = child_minimum;
                        }

                        if (first || maximum < 0 || child_maximum < 0) {
                            maximum = child_maximum;
                        } else if (child_maximum > maximum) {
                            maximum = child_maximum;
                        }

                        first = false;
                    }
                    return;
                }
                case node::kind::repeat: {
                    int64_t child_minimum = 0;
                    int64_t child_maximum = 0;
                    measure(entry.children[0], child_minimum, child_maximum);
                    minimum = child_minimum * entry.minimum;

                    if (entry.maximum < 0 || child_maximum < 0) {
                        maximum = -1;
                    } else {
                        maximum = child_maximum * entry.maximum;
                    }
                    return;
                }
                case node::kind::capture:
                case node::kind::group:
                    measure(entry.children[0], minimum, maximum);
                    return;
            }
        }

        void compile_alternate(const node& entry) {
            vector<int32_t> jumps;

            for (size_t index = 0; index + 1 < entry.children.size(); ++index) {
                const auto split = emit(op::split);
                const auto branch = static_cast<int32_t>(code.size());
                compile_node(entry.children[index]);
                jumps.push_back(emit(op::jump));
                const auto following = static_cast<int32_t>(code.size());
                code[static_cast<size_t>(split)].a = branch;
                code[static_cast<size_t>(split)].b = following;
            }

            compile_node(entry.children.back());

            const auto end = static_cast<int32_t>(code.size());

            for (const int32_t jump: jumps) {
                code[static_cast<size_t>(jump)].a = end;
            }
        }

        void compile_loop_body(const node& entry, const bool lazy) {
            const auto child = entry.children[0];

            for (int32_t count = 0; count < entry.minimum; ++count) {
                compile_node(child);
            }

            if (entry.maximum < 0) {
                const auto slot = loop_count++;
                const auto split = emit(op::split);
                const auto body = static_cast<int32_t>(code.size());
                emit(op::loop_enter, slot);
                compile_node(child);
                const auto check = emit(op::loop_check, slot);
                emit(op::jump, split);
                const auto after = static_cast<int32_t>(code.size());

                if (lazy) {
                    code[static_cast<size_t>(split)].a = after;
                    code[static_cast<size_t>(split)].b = body;
                } else {
                    code[static_cast<size_t>(split)].a = body;
                    code[static_cast<size_t>(split)].b = after;
                }

                code[static_cast<size_t>(check)].b = after;
                return;
            }

            for (int32_t count = entry.minimum; count < entry.maximum; ++count) {
                const int32_t split = emit(op::split);
                const auto body = static_cast<int32_t>(code.size());
                compile_node(child);
                const auto after = static_cast<int32_t>(code.size());

                if (lazy) {
                    code[static_cast<size_t>(split)].a = after;
                    code[static_cast<size_t>(split)].b = body;
                } else {
                    code[static_cast<size_t>(split)].a = body;
                    code[static_cast<size_t>(split)].b = after;
                }
            }
        }

        void compile_wrapped(const op kind, const int32_t index, const int32_t look_kind) {
            const node& entry = nodes_[static_cast<size_t>(index)];

            int64_t minimum = 0;
            int64_t maximum = 0;

            if (kind == op::look && (look_kind == 3 || look_kind == 4)) {
                measure(entry.children[0], minimum, maximum);

                if (maximum < 0) {
                    throw compile_failure{err_lookbehind_not_fixed, entry.offset};
                }

                maximum = min<int64_t>(maximum, 0x7FFF);
            }

            const int32_t wrapper =
                    emit(kind, 0, look_kind, static_cast<int32_t>(minimum), static_cast<int32_t>(maximum));
            emit(op::jump);
            const auto body = static_cast<int32_t>(code.size());
            compile_node(entry.children[0]);
            emit(op::accept);
            const auto after = static_cast<int32_t>(code.size());

            code[static_cast<size_t>(wrapper)].a = body;
            code[static_cast<size_t>(wrapper)].e = after;
            code[static_cast<size_t>(wrapper) + 1UL].a = after;
        }

        void compile_node(const int32_t index) {
            const node& entry = nodes_[static_cast<size_t>(index)];

            switch (entry.type) {
                case node::kind::empty:
                    break;
                case node::kind::literal:
                    emit(op::char_match, static_cast<int32_t>(entry.code_point), entry.caseless ? 1 : 0);
                    break;
                case node::kind::any:
                    emit(op::any, 0, entry.dotall ? 1 : 0);
                    break;
                case node::kind::char_class:
                    emit(op::char_class, entry.class_index);
                    break;
                case node::kind::concat:
                    for (const int32_t child: entry.children) {
                        compile_node(child);
                    }
                    break;
                case node::kind::alternate:
                    compile_alternate(entry);
                    break;
                case node::kind::repeat:
                    if (entry.maximum == 0 && entry.minimum == 0) {
                        break;
                    }

                    if (entry.mode == node::repeat_mode::possessive) {
                        const auto wrapper = emit(op::atomic);
                        emit(op::jump);
                        const auto body = static_cast<int32_t>(code.size());
                        compile_loop_body(entry, false);
                        emit(op::accept);
                        const auto after = static_cast<int32_t>(code.size());
                        code[static_cast<size_t>(wrapper)].a = body;
                        code[static_cast<size_t>(wrapper)].e = after;
                        code[static_cast<size_t>(wrapper) + 1UL].a = after;
                        break;
                    }

                    compile_loop_body(entry, entry.mode == node::repeat_mode::lazy);
                    break;
                case node::kind::capture: {
                    const int32_t slot = entry.capture * 2;
                    emit(op::save, slot);

                    if (static_cast<size_t>(entry.capture) < capture_entry_.size()) {
                        capture_entry_[static_cast<size_t>(entry.capture)] = static_cast<int32_t>(code.size());
                    }

                    compile_node(entry.children[0]);
                    emit(op::save, slot + 1);
                    break;
                }
                case node::kind::group:
                    compile_node(entry.children[0]);
                    break;
                case node::kind::look:
                    compile_wrapped(op::look, index, entry.look_kind);
                    break;
                case node::kind::atomic:
                    compile_wrapped(op::atomic, index, 0);
                    break;
                case node::kind::backref:
                    emit(op::backref, entry.group, entry.caseless ? 1 : 0);
                    break;
                case node::kind::begin_line:
                    emit(op::begin_line, entry.multiline ? 1 : 0, multiline_circumflex_ ? 1 : 0);
                    break;
                case node::kind::end_line:
                    emit(op::end_line, entry.multiline ? 1 : 0, dollar_endonly_ ? 1 : 0);
                    break;
                case node::kind::begin_subject:
                    emit(op::begin_subject);
                    break;
                case node::kind::end_subject:
                    emit(op::end_subject);
                    break;
                case node::kind::end_subject_final:
                    emit(op::end_subject_final);
                    break;
                case node::kind::word_boundary:
                    emit(op::word_boundary, entry.ucp ? 1 : 0);
                    break;
                case node::kind::non_word_boundary:
                    emit(op::non_word_boundary, entry.ucp ? 1 : 0);
                    break;
                case node::kind::cluster:
                    emit(op::cluster);
                    break;
                case node::kind::call: {
                    const int32_t slot = emit(op::call);
                    call_fixups_.push_back(call_fixup{slot, entry.group});
                    break;
                }
            }
        }

        const vector<node>& nodes_;
        const vector<char_class>& classes_;
        vector<int32_t> capture_entry_;
        vector<call_fixup> call_fixups_;
        bool utf_ = false;
        bool dollar_endonly_ = false;
        bool multiline_circumflex_ = false;
    };
    struct match_state {
        const vector<instruction>* code = nullptr;
        const vector<char_class>* classes = nullptr;
        const char* subject = nullptr;
        size_t length = 0;
        bool utf = false;
        bool ucp = false;
        bool match_unset_backref = false;
        vector<size_t> slots;
        vector<size_t> loops;
        struct call_frame {
            int32_t return_ip;
            vector<size_t> saved_loops;
            size_t saved_start;
            size_t saved_end;
        };

        vector<call_frame> calls;
        vector<size_t> undo_index;
        vector<size_t> undo_value;
        long long steps = 0;
        int error = 0;

        struct frame {
            int32_t ip;
            size_t pos;
            size_t undo;
            size_t ret;
        };
    };

    NEFORCE_NODISCARD size_t decode_at(const match_state& state, const size_t pos, uint32_t& code_point) noexcept {
        if (pos >= state.length) {
            return 0;
        }

        if (!state.utf) {
            code_point = static_cast<unsigned char>(state.subject[pos]);
            return 1;
        }

        return utf8_decode(reinterpret_cast<const unsigned char*>(state.subject) + pos, state.length - pos, code_point);
    }

    NEFORCE_NODISCARD bool is_boundary(const match_state& state, const size_t pos) noexcept {
        if (!state.utf || pos == 0 || pos >= state.length) {
            return true;
        }

        return !is_utf8_trail(static_cast<unsigned char>(state.subject[pos]));
    }

    NEFORCE_NODISCARD uint32_t fold_code_point(const match_state& state, const uint32_t code_point) noexcept {
        if (!state.ucp) {
            return (code_point >= 'A' && code_point <= 'Z') ? code_point + 32 : code_point;
        }

        return to_lower(code_point);
    }

    NEFORCE_NODISCARD bool is_word_at(const match_state& state, const size_t pos, const bool ucp) noexcept {
        uint32_t code_point = 0;

        if (decode_at(state, pos, code_point) == 0) {
            return false;
        }

        return ucp ? is_unicode_word(code_point) : is_ascii_word(code_point);
    }

    NEFORCE_NODISCARD bool is_word_before(const match_state& state, const size_t pos, const bool ucp) noexcept {
        if (pos == 0) {
            return false;
        }

        size_t start = pos - 1;

        if (state.utf) {
            while (start > 0 && is_utf8_trail(static_cast<unsigned char>(state.subject[start]))) {
                --start;
            }
        }

        return is_word_at(state, start, ucp);
    }

    void rollback(match_state& state, const size_t mark) {
        while (state.undo_index.size() > mark) {
            const size_t index = state.undo_index.back();
            const size_t value = state.undo_value.back();
            state.undo_index.pop_back();
            state.undo_value.pop_back();

            if (index < state.slots.size()) {
                state.slots[index] = value;
            } else {
                state.loops[index - state.slots.size()] = value;
            }
        }
    }

    void unwind_calls(match_state& state, const size_t mark) {
        while (state.calls.size() > mark) {
            state.loops = move(state.calls.back().saved_loops);
            state.calls.pop_back();
        }
    }

    void assign_slot(match_state& state, const size_t index, const size_t value) {
        state.undo_index.push_back(index);

        if (index < state.slots.size()) {
            state.undo_value.push_back(state.slots[index]);
            state.slots[index] = value;
        } else {
            const size_t loop = index - state.slots.size();
            state.undo_value.push_back(state.loops[loop]);
            state.loops[loop] = value;
        }
    }

    NEFORCE_NODISCARD bool is_grapheme_extend(const uint32_t code_point) noexcept {
        const uint16_t category = category_of(code_point);
        return category >= 6 && category <= 8;
    }

    NEFORCE_NODISCARD size_t cluster_length(const match_state& state, const size_t pos) noexcept {
        uint32_t code_point = 0;
        const size_t used = decode_at(state, pos, code_point);

        if (used == 0) {
            return 0;
        }

        size_t end = pos + used;

        if (code_point == 0x0DU && end < state.length && state.subject[end] == '\n') {
            return 2;
        }

        if (code_point >= 0x1100U && code_point <= 0x115FU) {
            uint32_t next = 0;
            const size_t next_used = decode_at(state, end, next);

            if (next_used != 0 && next >= 0x1160U && next <= 0x11A7U) {
                end += next_used;
            }
        }

        for (;;) {
            uint32_t next = 0;
            const size_t next_used = decode_at(state, end, next);

            if (next_used == 0) {
                break;
            }

            if (is_grapheme_extend(next)) {
                end += next_used;
                continue;
            }

            if (next == 0x200DU) {
                uint32_t joined = 0;
                const size_t joined_used = decode_at(state, end + next_used, joined);

                if (joined_used == 0) {
                    break;
                }

                end += next_used + joined_used;
                continue;
            }

            break;
        }

        return end - pos;
    }

    bool run(match_state& state, int32_t entry_ip, size_t start_pos, size_t& end_pos);

    bool lookbehind_check(match_state& state, const size_t pos, const int32_t kind, const int32_t sub,
                          const size_t minimum, const size_t maximum) {
        if (pos < minimum) {
            return kind == 4;
        }

        const size_t low = pos >= maximum ? pos - maximum : 0;
        const size_t high = pos - minimum;

        for (size_t start = low;; ++start) {
            if (!state.utf || is_boundary(state, start)) {
                const size_t mark = state.undo_index.size();
                size_t end = 0;

                if (run(state, sub, start, end) && end == pos) {
                    if (kind == 4) {
                        rollback(state, mark);
                        return false;
                    }
                    return true;
                }

                rollback(state, mark);
            }

            if (start >= high) {
                break;
            }
        }

        return kind == 4;
    }
    bool run(match_state& state, const int32_t entry_ip, const size_t start_pos, size_t& end_pos) {
        const vector<instruction>& code = *state.code;

        vector<match_state::frame> stack;
        const size_t entry_undo = state.undo_index.size();
        const size_t entry_calls = state.calls.size();

        int32_t ip = entry_ip;
        size_t pos = start_pos;

        for (;;) {
            if (++state.steps > maximum_steps) {
                state.error = match_result_too_complex;
                break;
            }

            const instruction& current = code[static_cast<size_t>(ip)];

            switch (current.code) {
                case op::fail:
                    goto backtrack;
                case op::match:
                    if (!state.calls.empty()) {
                        match_state::call_frame frame = move(state.calls.back());
                        state.calls.pop_back();
                        ip = frame.return_ip;
                        assign_slot(state, 0, frame.saved_start);
                        assign_slot(state, 1, frame.saved_end);
                        state.loops = move(frame.saved_loops);
                    } else {
                        end_pos = pos;
                        return true;
                    }
                    break;
                case op::accept:
                    end_pos = pos;
                    unwind_calls(state, entry_calls);
                    return true;
                case op::char_match: {
                    uint32_t code_point = 0;
                    const size_t used = decode_at(state, pos, code_point);

                    if (used == 0) {
                        goto backtrack;
                    }

                    const auto expected = static_cast<uint32_t>(current.a);
                    bool equal = code_point == expected;

                    if (!equal && current.b != 0) {
                        equal = fold_code_point(state, code_point) == fold_code_point(state, expected);
                    }

                    if (!equal) {
                        goto backtrack;
                    }

                    pos += used;
                    ++ip;
                    break;
                }
                case op::any: {
                    uint32_t code_point = 0;
                    const size_t used = decode_at(state, pos, code_point);

                    if (used == 0) {
                        goto backtrack;
                    }

                    if (current.b == 0 && code_point == '\n') {
                        goto backtrack;
                    }

                    pos += used;
                    ++ip;
                    break;
                }
                case op::char_class: {
                    uint32_t code_point = 0;
                    const size_t used = decode_at(state, pos, code_point);

                    if (used == 0) {
                        goto backtrack;
                    }

                    if (!(*state.classes)[static_cast<size_t>(current.a)].contains(code_point)) {
                        goto backtrack;
                    }

                    pos += used;
                    ++ip;
                    break;
                }
                case op::split:
                    stack.push_back(match_state::frame{current.b, pos, state.undo_index.size(), state.calls.size()});
                    ip = current.a;
                    break;
                case op::jump:
                    ip = current.a;
                    break;
                case op::save:
                    assign_slot(state, static_cast<size_t>(current.a), pos);
                    ++ip;
                    break;
                case op::begin_line: {
                    bool matched = pos == 0;

                    if (!matched && current.a != 0) {
                        matched = state.subject[pos - 1] == '\n';
                    }

                    if (current.b != 0 && pos != 0) {
                        matched = false;
                    }

                    if (!matched) {
                        goto backtrack;
                    }

                    ++ip;
                    break;
                }
                case op::end_line: {
                    bool matched = false;

                    if (current.c != 0) {
                        matched = pos == state.length;
                    } else if (current.a != 0) {
                        matched = pos == state.length || state.subject[pos] == '\n';
                    } else {
                        matched = pos == state.length || (pos + 1 == state.length && state.subject[pos] == '\n');
                    }

                    if (!matched) {
                        goto backtrack;
                    }

                    ++ip;
                    break;
                }
                case op::begin_subject:
                    if (pos != 0) {
                        goto backtrack;
                    }
                    ++ip;
                    break;
                case op::end_subject:
                    if (pos != state.length) {
                        goto backtrack;
                    }
                    ++ip;
                    break;
                case op::end_subject_final:
                    if (pos != state.length && (pos + 1 != state.length || state.subject[pos] != '\n')) {
                        goto backtrack;
                    }
                    ++ip;
                    break;
                case op::word_boundary:
                    if (is_word_before(state, pos, current.a != 0) == is_word_at(state, pos, current.a != 0)) {
                        goto backtrack;
                    }
                    ++ip;
                    break;
                case op::non_word_boundary:
                    if (is_word_before(state, pos, current.a != 0) != is_word_at(state, pos, current.a != 0)) {
                        goto backtrack;
                    }
                    ++ip;
                    break;
                case op::backref: {
                    const auto group = static_cast<size_t>(current.a);

                    if (group * 2 + 1 >= state.slots.size()) {
                        goto backtrack;
                    }

                    const auto begin = state.slots[group * 2];
                    const auto finish = state.slots[group * 2 + 1];

                    if (begin == invalid_offset || finish == invalid_offset) {
                        if (!state.match_unset_backref) {
                            goto backtrack;
                        }
                        ++ip;
                        break;
                    }

                    size_t left = begin;
                    size_t right = pos;

                    while (left < finish) {
                        uint32_t subject_point = 0;
                        uint32_t group_point = 0;
                        const size_t subject_used = decode_at(state, right, subject_point);
                        const size_t group_used = decode_at(state, left, group_point);

                        if (subject_used == 0 || group_used == 0) {
                            goto backtrack;
                        }

                        if (subject_point != group_point &&
                            (current.b == 0 ||
                             fold_code_point(state, subject_point) != fold_code_point(state, group_point))) {
                            goto backtrack;
                        }

                        left += group_used;
                        right += subject_used;
                    }

                    pos = right;
                    ++ip;
                    break;
                }
                case op::look: {
                    const int32_t kind = current.b;

                    if (kind == 1 || kind == 2) {
                        const size_t mark = state.undo_index.size();
                        size_t end = 0;
                        const bool matched = run(state, current.a, pos, end);

                        if (kind == 1) {
                            if (!matched) {
                                goto backtrack;
                            }
                        } else if (matched) {
                            rollback(state, mark);
                            goto backtrack;
                        }
                    } else if (!lookbehind_check(state, pos, kind, current.a, static_cast<size_t>(current.c),
                                                 static_cast<size_t>(current.d))) {
                        goto backtrack;
                    }

                    ip = current.e;
                    break;
                }
                case op::atomic: {
                    size_t end = 0;

                    if (!run(state, current.a, pos, end)) {
                        goto backtrack;
                    }

                    pos = end;
                    ip = current.e;
                    break;
                }
                case op::call:
                    if (state.calls.size() >= static_cast<size_t>(maximum_recursion)) {
                        state.error = match_result_too_complex;
                        goto abort;
                    }
                    state.calls.push_back(match_state::call_frame{ip + 1, state.loops, state.slots[0], state.slots[1]});
                    ip = current.a;
                    break;
                case op::cluster: {
                    const size_t used = cluster_length(state, pos);

                    if (used == 0) {
                        goto backtrack;
                    }

                    pos += used;
                    ++ip;
                    break;
                }
                case op::loop_enter:
                    assign_slot(state, state.slots.size() + static_cast<size_t>(current.a), pos);
                    ++ip;
                    break;
                case op::loop_check:
                    if (state.loops[static_cast<size_t>(current.a)] == pos) {
                        ip = current.b;
                    } else {
                        ++ip;
                    }
                    break;
            }

            continue;

        backtrack:
            if (stack.empty()) {
                rollback(state, entry_undo);
                unwind_calls(state, entry_calls);
                return false;
            }

            const match_state::frame frame = stack.back();
            stack.pop_back();
            rollback(state, frame.undo);
            unwind_calls(state, frame.ret);
            ip = frame.ip;
            pos = frame.pos;
        }

    abort:
        rollback(state, entry_undo);
        unwind_calls(state, entry_calls);
        return false;
    }
} // namespace

struct regex_program {
    vector<instruction> code;
    vector<char_class> classes;
    int capture_count = 0;
    int loop_count = 0;
    uint32_t options = 0;
};

struct regex_match {
    vector<size_t> ovector;
};

regex_program* regex_compile(const char* pattern, const size_t length, const uint32_t options, int* error_code,
                             size_t* error_offset, char* message, const size_t message_size) {
    if (error_code != nullptr) {
        *error_code = 0;
    }
    if (error_offset != nullptr) {
        *error_offset = 0;
    }

    try {
        if ((options & ~accepted_options) != 0U) {
            throw compile_failure{err_unsupported_option, 0};
        }

        parser parsed(pattern, length, options);
        const int32_t root = parsed.parse();

        compiler builder(parsed.nodes, parsed.classes, parsed.capture_count);
        builder.configure(options);
        builder.compile(root);

        auto* program = new regex_program;
        program->code = move(builder.code);
        program->classes = move(parsed.classes);
        program->capture_count = parsed.capture_count;
        program->loop_count = builder.loop_count;
        program->options = options;
        return program;
    } catch (const compile_failure& failure) {
        if (error_code != nullptr) {
            *error_code = failure.code;
        }
        if (error_offset != nullptr) {
            *error_offset = failure.offset;
        }
        if (message != nullptr && message_size > 0) {
            const char* text = error_message(failure.code);
            string_copy(message, text, message_size - 1);
            message[message_size - 1] = '\0';
        }
        return nullptr;
    } catch (...) {
        if (error_code != nullptr) {
            *error_code = err_unsupported_feature;
        }
        if (message != nullptr && message_size > 0) {
            string_copy(message, error_message(err_unsupported_feature), message_size - 1);
            message[message_size - 1] = '\0';
        }
        return nullptr;
    }
}

void regex_free(regex_program* program) noexcept { delete program; }

int regex_capture_count(const regex_program* program) noexcept {
    return program != nullptr ? program->capture_count : 0;
}

regex_match* regex_match_create(const regex_program* program) {
    if (program == nullptr) {
        return nullptr;
    }

    auto* data = new regex_match;
    data->ovector.assign((static_cast<size_t>(program->capture_count) + 1) * 2, invalid_offset);
    return data;
}

void regex_match_free(regex_match* data) noexcept { delete data; }

int regex_exec(const regex_program* program, const char* subject, const size_t length, const size_t start_offset,
               const uint32_t options, regex_match* data) {
    if (program == nullptr || data == nullptr || subject == nullptr) {
        return match_result_nomatch;
    }

    const size_t capture_slots = (static_cast<size_t>(program->capture_count) + 1) * 2;

    match_state state;
    state.code = &program->code;
    state.classes = &program->classes;
    state.subject = subject;
    state.length = length;
    state.utf = (program->options & opt_utf) != 0;
    state.ucp = (program->options & opt_ucp) != 0;
    state.match_unset_backref = (program->options & opt_match_unset_backref) != 0;
    state.slots.assign(capture_slots, invalid_offset);
    state.loops.assign(static_cast<size_t>(program->loop_count), 0);

    const bool anchored = (options & match_anchored) != 0;
    const bool end_anchored = (options & match_end_anchored) != 0;
    const bool first_line = (program->options & opt_firstline) != 0;

    size_t start = start_offset <= length ? start_offset : length;

    while (!anchored || start == start_offset) {
        if (first_line) {
            bool stopped = false;

            for (size_t index = 0; index < start; ++index) {
                if (subject[index] == '\n') {
                    stopped = true;
                    break;
                }
            }

            if (stopped) {
                break;
            }
        }

        if (!state.utf || is_boundary(state, start)) {
            const size_t mark = state.undo_index.size();
            size_t end = 0;

            if (run(state, 0, start, end)) {
                if (!end_anchored || end == length) {
                    data->ovector.assign(capture_slots, invalid_offset);

                    int highest = 0;
                    for (size_t group = 0; group < capture_slots / 2; ++group) {
                        data->ovector[group * 2] = state.slots[group * 2];
                        data->ovector[group * 2 + 1] = state.slots[group * 2 + 1];

                        if (state.slots[group * 2] != invalid_offset) {
                            highest = static_cast<int>(group);
                        }
                    }

                    return highest + 1;
                }

                rollback(state, mark);
            } else if (state.error != 0) {
                return state.error;
            }
        }

        if (start >= length) {
            break;
        }

        size_t step = 1;

        if (state.utf) {
            uint32_t code_point = 0;
            const size_t used = decode_at(state, start, code_point);
            step = used == 0 ? 1 : used;
        }

        start += step;
    }

    return match_result_nomatch;
}

const size_t* regex_ovector(const regex_match* data) noexcept {
    return data != nullptr ? data->ovector.data() : nullptr;
}

NEFORCE_END_NAMESPACE__
