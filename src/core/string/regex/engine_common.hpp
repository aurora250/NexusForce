#ifndef NEFORCE_CORE_STRING_REGEX_ENGINE_COMMON_SRC__
#define NEFORCE_CORE_STRING_REGEX_ENGINE_COMMON_SRC__
#include <NeForce/core/numeric/numeric_traits.hpp>
#include "ucd_tables.hpp"
NEFORCE_BEGIN_NAMESPACE__

constexpr uint32_t match_anchored = 0x80000000U;
constexpr uint32_t match_end_anchored = 0x20000000U;

constexpr int match_result_nomatch = -1;
constexpr int match_result_too_complex = -2;
constexpr int match_result_bad_utf = -3;

constexpr size_t invalid_offset = numeric_traits<size_t>::max();
constexpr ssize_t maximum_steps = 20000000;
constexpr ssize_t attempt_steps = 1000000;
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

NEFORCE_NODISCARD inline const char* error_message(const int code) noexcept {
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

NEFORCE_NODISCARD constexpr bool is_utf8_trail(const unsigned char byte) noexcept { return (byte & 0xC0U) == 0x80U; }

NEFORCE_NODISCARD inline size_t utf8_sequence_length(const unsigned char lead) noexcept {
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

NEFORCE_NODISCARD inline size_t utf8_decode(const unsigned char* const data, const size_t available,
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

NEFORCE_NODISCARD inline size_t utf8_encode(const uint32_t code_point, char* const output) noexcept {
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

NEFORCE_NODISCARD inline uint16_t category_of(const uint32_t code_point) noexcept {
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

NEFORCE_NODISCARD inline uint32_t to_lower(const uint32_t code_point) noexcept {
    return case_lookup(regex_ucd::lower_map, code_point, code_point);
}

NEFORCE_NODISCARD inline uint32_t to_upper(const uint32_t code_point) noexcept {
    return case_lookup(regex_ucd::upper_map, code_point, code_point);
}

NEFORCE_NODISCARD inline bool is_ascii_digit(const uint32_t code_point) noexcept {
    return code_point >= '0' && code_point <= '9';
}

NEFORCE_NODISCARD inline bool is_ascii_word(const uint32_t code_point) noexcept {
    return (code_point >= 'a' && code_point <= 'z') || (code_point >= 'A' && code_point <= 'Z') ||
           is_ascii_digit(code_point) || code_point == '_';
}

NEFORCE_NODISCARD inline bool is_ascii_space(const uint32_t code_point) noexcept {
    return code_point == ' ' || code_point == '\t' || code_point == '\n' || code_point == '\r' || code_point == '\f' ||
           code_point == '\v';
}

NEFORCE_NODISCARD inline bool is_unicode_word(const uint32_t code_point) noexcept {
    if (code_point < 0x80U) {
        return is_ascii_word(code_point);
    }

    const uint16_t category = category_of(code_point);
    return category >= 1 && category <= 11;
}

NEFORCE_NODISCARD inline bool is_unicode_space(const uint32_t code_point) noexcept {
    if (code_point < 0x80U) {
        return is_ascii_space(code_point);
    }

    if (code_point == 0x85U) {
        return true;
    }

    const uint16_t category = category_of(code_point);
    return category >= 23 && category <= 25;
}

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_STRING_REGEX_ENGINE_COMMON_SRC__
