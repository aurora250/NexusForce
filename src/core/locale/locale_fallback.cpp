#include <NeForce/core/numeric/math.hpp>
#include <NeForce/core/locale/locale.hpp>
#include <NeForce/core/string/codepoint.hpp>
#include <NeForce/core/system/environment.hpp>
#include <NeForce/core/time/datetime.hpp>
#include "locale_data.hpp"
#include "locale_tables.hpp"
#ifndef NEFORCE_SUPPORT_ICU
#    ifdef NEFORCE_PLATFORM_WINDOWS
#        include <WinNls.h>
#    endif
NEFORCE_BEGIN_NAMESPACE__

namespace {
    constexpr uint32_t ASCII_CASE_DISTANCE = 32;
    constexpr int32_t DEFAULT_FRACTION_DIGITS = 6;
    constexpr size_t KEY_LEVEL_STRIDE = 4;
    constexpr uint32_t NON_LOWER_CASE_BIT = 0x800000;
    constexpr size_t MIN_PADDED_YEAR_DIGITS = 4;

    using locale_tables::ALPHA;
    using locale_tables::BASE_ENTRIES;
    using locale_tables::BASE_ENTRY_COUNT;
    using locale_tables::CASE_ENTRIES;
    using locale_tables::CASE_ENTRY_COUNT;
    using locale_tables::CLASS_RANGE_COUNT;
    using locale_tables::CLASS_RANGES;
    using locale_tables::DIGIT;
    using locale_tables::LOWER;
    using locale_tables::PRINTABLE;
    using locale_tables::PUNCT;
    using locale_tables::SPACE;
    using locale_tables::TITLE_ENTRIES;
    using locale_tables::TITLE_ENTRY_COUNT;
    using locale_tables::TITLECASE;
    using locale_tables::UPPER;
    using locale_tables::WHITE_SPACE;

    NEFORCE_NODISCARD uint16_t class_mask(const char32_t codepoint) noexcept {
        const auto value = static_cast<uint32_t>(codepoint);
        uint32_t low = 0;
        uint32_t high = CLASS_RANGE_COUNT;

        while (low < high) {
            const uint32_t middle = low + (high - low) / 2;
            const auto& range = CLASS_RANGES[middle];

            if (value < range.first) {
                high = middle;
            } else if (value > range.last) {
                low = middle + 1;
            } else {
                return range.mask;
            }
        }
        return 0;
    }

    NEFORCE_NODISCARD bool flagged(const uint16_t mask, const uint16_t flag) noexcept { return (mask & flag) != 0; }

    NEFORCE_NODISCARD uint32_t mapped_case(const uint32_t codepoint, const bool want_upper) noexcept {
        uint32_t low = 0;
        uint32_t high = CASE_ENTRY_COUNT;

        while (low < high) {
            const uint32_t middle = low + (high - low) / 2;
            const auto& entry = CASE_ENTRIES[middle];

            if (codepoint < entry.code) {
                high = middle;
            } else if (codepoint > entry.code) {
                low = middle + 1;
            } else {
                return want_upper ? entry.upper : entry.lower;
            }
        }
        return codepoint;
    }

    NEFORCE_NODISCARD uint32_t upper_of(const uint32_t codepoint) noexcept {
        if (codepoint < 0x80U) {
            return (codepoint >= 'a' && codepoint <= 'z') ? codepoint - ASCII_CASE_DISTANCE : codepoint;
        }
        return mapped_case(codepoint, true);
    }

    NEFORCE_NODISCARD uint32_t lower_of(const uint32_t codepoint) noexcept {
        if (codepoint < 0x80U) {
            return (codepoint >= 'A' && codepoint <= 'Z') ? codepoint + ASCII_CASE_DISTANCE : codepoint;
        }
        return mapped_case(codepoint, false);
    }

    NEFORCE_NODISCARD uint32_t title_of(const uint32_t codepoint) noexcept {
        uint32_t low = 0;
        uint32_t high = TITLE_ENTRY_COUNT;

        while (low < high) {
            const uint32_t middle = low + (high - low) / 2;
            const auto& entry = TITLE_ENTRIES[middle];

            if (codepoint < entry.code) {
                high = middle;
            } else if (codepoint > entry.code) {
                low = middle + 1;
            } else {
                return entry.title;
            }
        }
        return upper_of(codepoint);
    }

    NEFORCE_NODISCARD uint32_t accent_base(const uint32_t codepoint) noexcept {
        uint32_t low = 0;
        uint32_t high = BASE_ENTRY_COUNT;

        while (low < high) {
            const uint32_t middle = low + (high - low) / 2;
            const auto& entry = BASE_ENTRIES[middle];

            if (codepoint < entry.code) {
                high = middle;
            } else if (codepoint > entry.code) {
                low = middle + 1;
            } else {
                return entry.base;
            }
        }
        return codepoint;
    }

    NEFORCE_NODISCARD char32_t first_codepoint_of(string_view text) noexcept {
        if (text.empty() || text[0] == '\0') {
            return U'\0';
        }

        size_t index = 0;
        const size_t length = text.length();
        return codepoint::decode_utf8(reinterpret_cast<const byte_t*>(text.data()), index, length).to_char32();
    }

    void append_codepoint(string& target, const uint32_t value) { codepoint(value).append_to(target); }

    NEFORCE_NODISCARD int compare_codepoints(const u32string& left, const u32string& right) noexcept {
        const size_t shared = (left.size() < right.size()) ? left.size() : right.size();

        for (size_t i = 0; i < shared; ++i) {
            if (left[i] != right[i]) {
                return (left[i] < right[i]) ? -1 : 1;
            }
        }
        if (left.size() == right.size()) {
            return 0;
        }
        return (left.size() < right.size()) ? -1 : 1;
    }

    // Primary level: case folded and accent stripped, so "HELLO" compares equal to "hello" and
    // "resume" compares equal to "résumé".
    NEFORCE_NODISCARD u32string primary_form(const string& text) {
        u32string result;
        result.reserve(text.size());

        const auto* bytes = reinterpret_cast<const byte_t*>(text.data());
        size_t index = 0;
        while (index < text.size()) {
            const uint32_t value = codepoint::decode_utf8(bytes, index, text.size()).value();
            result.push_back(static_cast<char32_t>(upper_of(accent_base(value))));
        }
        return result;
    }

    // Secondary level: case folded but accent sensitive.
    NEFORCE_NODISCARD u32string secondary_form(const string& text) {
        u32string result;
        result.reserve(text.size());

        const auto* bytes = reinterpret_cast<const byte_t*>(text.data());
        size_t index = 0;
        while (index < text.size()) {
            const uint32_t value = codepoint::decode_utf8(bytes, index, text.size()).value();
            result.push_back(static_cast<char32_t>(lower_of(value)));
        }
        return result;
    }

    // Tertiary level: lowercase weighs less than uppercase, which is the order the root collation
    // uses, so "hello" sorts before "HELLO". The bit is above every codepoint, so it never collides
    // with a real character.
    NEFORCE_NODISCARD u32string tertiary_form(const string& text) {
        u32string result;
        result.reserve(text.size());

        const auto* bytes = reinterpret_cast<const byte_t*>(text.data());
        size_t index = 0;
        while (index < text.size()) {
            const uint32_t value = codepoint::decode_utf8(bytes, index, text.size()).value();
            const bool lowercase = flagged(class_mask(static_cast<char32_t>(value)), LOWER);
            result.push_back(static_cast<char32_t>(lowercase ? value : (value | NON_LOWER_CASE_BIT)));
        }
        return result;
    }

    // Sort keys are built level by level with a zero terminator after each level: a zero byte is
    // smaller than every encoded codepoint, so a plain byte comparison of two keys reproduces the
    // level wise comparison performed by compare().
    void append_key_level(string& target, const u32string& level) {
        for (const char32_t codepoint: level) {
            const auto value = static_cast<uint32_t>(codepoint) + 1;
            target.push_back(static_cast<char>((value >> 24) & 0xFF));
            target.push_back(static_cast<char>((value >> 16) & 0xFF));
            target.push_back(static_cast<char>((value >> 8) & 0xFF));
            target.push_back(static_cast<char>(value & 0xFF));
        }
        target.append(KEY_LEVEL_STRIDE, '\0');
    }

    NEFORCE_NODISCARD bool ascii_alpha(const char value) noexcept {
        return (value >= 'a' && value <= 'z') || (value >= 'A' && value <= 'Z');
    }

    NEFORCE_NODISCARD bool ascii_digit(const char value) noexcept { return value >= '0' && value <= '9'; }

    void append_lowered(string& target, const string& text) {
        for (const char value: text) {
            target.push_back((value >= 'A' && value <= 'Z') ? static_cast<char>(value + ASCII_CASE_DISTANCE) : value);
        }
    }

    void append_uppered(string& target, const string& text) {
        for (const char value: text) {
            target.push_back((value >= 'a' && value <= 'z') ? static_cast<char>(value - ASCII_CASE_DISTANCE) : value);
        }
    }

    void append_titled(string& target, const string& text) {
        if (text.empty()) {
            return;
        }

        const char head = text[0];
        target.push_back((head >= 'a' && head <= 'z') ? static_cast<char>(head - ASCII_CASE_DISTANCE) : head);
        for (size_t i = 1; i < text.size(); ++i) {
            const char value = text[i];
            target.push_back((value >= 'A' && value <= 'Z') ? static_cast<char>(value + ASCII_CASE_DISTANCE) : value);
        }
    }

    NEFORCE_NODISCARD bool all_alpha(const string& text) noexcept {
        for (const char value: text) {
            if (!ascii_alpha(value)) {
                return false;
            }
        }
        return !text.empty();
    }

    NEFORCE_NODISCARD bool all_digit(const string& text) noexcept {
        for (const char value: text) {
            if (!ascii_digit(value)) {
                return false;
            }
        }
        return !text.empty();
    }

    struct name_parts {
        string language;
        string script;
        string country;
        string variant;
    };

    // Accepts both BCP-47 tags ("zh-Hans-CN") and POSIX names ("zh_CN.UTF-8"), and canonicalises
    // the case of every subtag. The codeset and the modifier of a POSIX name are dropped.
    NEFORCE_NODISCARD bool parse_locale_name(const string& name, name_parts& parts) {
        string normalized;
        normalized.reserve(name.size());

        for (const char value: name) {
            if (value == '.' || value == '@') {
                break;
            }
            normalized.push_back(value == '_' ? '-' : value);
        }

        size_t index = 0;
        int32_t position = 0;
        while (index <= normalized.size() && index != string::npos) {
            const size_t separator = normalized.find('-', index);
            const size_t end = (separator == string::npos) ? normalized.size() : separator;
            const string subtag = normalized.substr(index, end - index);
            index = (separator == string::npos) ? normalized.size() + 1 : separator + 1;

            if (subtag.empty()) {
                if (position == 0) {
                    return false;
                }
                ++position;
                continue;
            }

            if (position == 0) {
                if (!all_alpha(subtag)) {
                    return false;
                }
                append_lowered(parts.language, subtag);
            } else if (parts.script.empty() && subtag.size() == 4 && all_alpha(subtag)) {
                append_titled(parts.script, subtag);
            } else if (parts.country.empty() &&
                       ((subtag.size() == 2 && all_alpha(subtag)) || (subtag.size() == 3 && all_digit(subtag)))) {
                append_uppered(parts.country, subtag);
            } else if (parts.variant.empty()) {
                append_uppered(parts.variant, subtag);
            }
            ++position;
        }

        return !parts.language.empty();
    }

    NEFORCE_NODISCARD string bcp47_of(const name_parts& parts) {
        string result = parts.language;
        if (!parts.script.empty()) {
            result += "-";
            result += parts.script;
        }
        if (!parts.country.empty()) {
            result += "-";
            result += parts.country;
        }
        if (!parts.variant.empty()) {
            result += "-";
            result += parts.variant;
        }
        return result;
    }

    NEFORCE_NODISCARD string posix_of(const name_parts& parts) {
        string result = parts.language;
        if (!parts.script.empty()) {
            result += "_";
            result += parts.script;
        }
        if (!parts.country.empty()) {
            result += "_";
            result += parts.country;
        }
        if (!parts.variant.empty()) {
            result += "_";
            result += parts.variant;
        }
        return result;
    }

    void push_unique(vector<string>& target, const string& value) {
        for (const auto& existing: target) {
            if (existing == value) {
                return;
            }
        }
        target.push_back(value);
    }

    // A locale without a grouping separator (the POSIX locale) reports no grouping at all; the
    // Indian system reports the secondary group size as well.
    NEFORCE_NODISCARD const char* grouping_of(const locale_data::number_style& style) noexcept {
        if (style.group_separator == nullptr || style.group_separator[0] == '\0') {
            return "";
        }
        return style.indian_grouping ? "3;2" : "3";
    }

    // The POSIX locale is the classic C locale: numbers carry no grouping and the currency symbol
    // is separated by a plain space.
    NEFORCE_NODISCARD const locale_data::number_style& number_style_for(const string& language, const string& variant) {
        static constexpr locale_data::number_style posix_style = {".", "", 0, false, true, " "};
        if (variant == "POSIX") {
            return posix_style;
        }
        return locale_data::number_style_of(language.data());
    }

    // A digit_zero of zero means the ASCII digits, every other value is the codepoint of the local
    // zero (arabic-indic and extended arabic-indic digits, for example).
    NEFORCE_NODISCARD u32string digits_of(const uint64_t magnitude, uint32_t digit_zero) {
        if (digit_zero == 0) {
            digit_zero = '0';
        }

        u32string reversed;
        uint64_t rest = magnitude;

        do {
            reversed.push_back(static_cast<char32_t>(digit_zero + (rest % 10)));
            rest /= 10;
        } while (rest != 0);

        u32string result;
        result.reserve(reversed.size());
        for (size_t i = reversed.size(); i > 0; --i) {
            result.push_back(reversed[i - 1]);
        }
        return result;
    }

    // Western grouping inserts a separator every three digits; the Indian system keeps the last
    // three digits together and groups the remaining digits in pairs (12,34,567).
    NEFORCE_NODISCARD u32string group_digits(const u32string& digits, const locale_data::number_style& style) {
        const auto separator = static_cast<char32_t>(first_codepoint_of(style.group_separator));
        const size_t size = digits.size();
        if (separator == U'\0') {
            return digits;
        }

        u32string result;
        result.reserve(size + size / 2 + 1);

        for (size_t i = 0; i < size; ++i) {
            result.push_back(digits[i]);

            const size_t rest = size - 1 - i;
            if (rest == 0) {
                break;
            }
            if (style.indian_grouping) {
                if (rest == 3 || (rest > 3 && (rest - 3) % 2 == 0)) {
                    result.push_back(separator);
                }
            } else if (rest % 3 == 0) {
                result.push_back(separator);
            }
        }
        return result;
    }

    NEFORCE_NODISCARD string encode(const u32string& text) {
        string result;
        result.reserve(text.size());
        for (const char32_t value: text) {
            codepoint(value).append_to(result);
        }
        return result;
    }

    NEFORCE_NODISCARD string format_integer(const int64_t value, const locale_data::number_style& style) {
        const bool negative = value < 0;
        const auto magnitude = negative ? (~static_cast<uint64_t>(value) + 1) : static_cast<uint64_t>(value);

        string result;
        if (negative) {
            result.push_back('-');
        }
        result += encode(group_digits(digits_of(magnitude, style.digit_zero), style));
        return result;
    }

    NEFORCE_NODISCARD string format_decimal(const double value, const int32_t fraction_digits, const bool trim,
                                            const locale_data::number_style& style) {
        if (is_nan(value)) {
            return "NaN";
        }
        if (is_infinity(value)) {
            string result;
            if (value < 0) {
                result.push_back('-');
            }
            result += "\xE2\x88\x9E";
            return result;
        }

        const bool negative = value < 0;
        const double magnitude = negative ? -value : value;

        const string digits = to_string_fixed(magnitude, fraction_digits);
        const size_t dot = digits.find('.');
        const string integer_part = (dot == string::npos) ? digits : digits.substr(0, dot);
        string fraction_part = (dot == string::npos) ? string{} : digits.substr(dot + 1);

        if (trim) {
            size_t keep = fraction_part.size();
            while (keep > 0 && fraction_part[keep - 1] == '0') {
                --keep;
            }
            fraction_part = fraction_part.substr(0, keep);
        }

        uint64_t integer_value = 0;
        for (const char digit: integer_part) {
            if (ascii_digit(digit)) {
                integer_value = integer_value * 10 + static_cast<uint64_t>(digit - '0');
            }
        }

        string result;
        if (negative) {
            result.push_back('-');
        }
        result += encode(group_digits(digits_of(integer_value, style.digit_zero), style));

        if (!fraction_part.empty()) {
            result += style.decimal_point;
            for (const char digit: fraction_part) {
                if (ascii_digit(digit)) {
                    append_codepoint(result, (style.digit_zero == 0 ? static_cast<uint32_t>('0') : style.digit_zero) +
                                                     static_cast<uint32_t>(digit - '0'));
                }
            }
        }
        return result;
    }

    struct date_fields {
        int32_t year{1970};
        int32_t month{1};
        int32_t day{1};
        int32_t hour{0};
        int32_t minute{0};
        int32_t second{0};
        int32_t weekday{0};
        int32_t day_of_year{1};
        int64_t offset_seconds{0};
    };

    NEFORCE_NODISCARD date_fields fields_of(const datetime& value) {
        date_fields fields;
        fields.year = value.year();
        fields.month = value.month();
        fields.day = value.day();
        fields.hour = value.hours();
        fields.minute = value.minutes();
        fields.second = value.seconds();
        fields.weekday = value.date().days_of_week();
        fields.day_of_year = value.date().days_of_year();
        fields.offset_seconds = value.offset_seconds();
        return fields;
    }

    void append_padded(string& target, const int64_t value, const size_t width) {
        const bool negative = value < 0;
        auto magnitude = static_cast<uint64_t>(negative ? -value : value);

        char buffer[24] = {};
        size_t written = 0;
        do {
            buffer[written] = static_cast<char>('0' + (magnitude % 10));
            ++written;
            magnitude /= 10;
        } while (magnitude != 0);

        if (negative) {
            target.push_back('-');
        }
        for (size_t padding = written; padding < width; ++padding) {
            target.push_back('0');
        }
        for (size_t i = written; i > 0; --i) {
            target.push_back(buffer[i - 1]);
        }
    }

    void append_narrow(string& target, string_view name) {
        const char32_t head = first_codepoint_of(name);
        if (head != U'\0') {
            codepoint(head).append_to(target);
        }
    }

    void append_offset(string& target, const int64_t offset_seconds, const bool compact) {
        const int64_t total_minutes = offset_seconds / 60;
        const int64_t hours = total_minutes / 60;
        const int64_t minutes = total_minutes % 60;

        if (compact && hours == 0 && minutes == 0) {
            target.push_back('Z');
            return;
        }

        target += "GMT";
        target.push_back(hours < 0 ? '-' : '+');
        append_padded(target, hours < 0 ? -hours : hours, 2);
        if (!compact) {
            target.push_back(':');
        }
        append_padded(target, minutes < 0 ? -minutes : minutes, 2);
    }

    NEFORCE_NODISCARD bool pattern_letter(const char value) noexcept {
        constexpr string_view letters = "GyYuUrQqMLlwWdDFgEecabBhHKkmsSAzZOvVXx";
        for (size_t i = 0; letters[i] != '\0'; ++i) {
            if (letters[i] == value) {
                return true;
            }
        }
        return false;
    }

    NEFORCE_NODISCARD int32_t week_of_year(const date_fields& fields) noexcept {
        const int32_t first_weekday = ((fields.day_of_year - fields.weekday) % 7 + 7) % 7;
        return (fields.day_of_year + first_weekday - 1) / 7 + 1;
    }

    void append_pattern_field(string& target, const char letter, const size_t width, const date_fields& fields,
                              const locale_data::calendar_style& style) {
        switch (letter) {
            case 'G':
                target += (fields.year >= 1) ? "AD" : "BC";
                return;
            case 'y':
            case 'u': {
                const int32_t year = (fields.year >= 1) ? fields.year : (1 - fields.year);
                if (width == 1) {
                    append_padded(target, year, 1);
                } else if (width == 2) {
                    append_padded(target, year % 100, 2);
                } else {
                    append_padded(target, year, (width < MIN_PADDED_YEAR_DIGITS) ? MIN_PADDED_YEAR_DIGITS : width);
                }
                return;
            }
            case 'M':
            case 'L': {
                if (fields.month < 1 || fields.month > 12) {
                    append_padded(target, fields.month, (width == 1) ? 1 : 2);
                    return;
                }

                const auto month = static_cast<size_t>(fields.month - 1);
                if (width <= 1) {
                    append_padded(target, fields.month, 1);
                } else if (width == 2) {
                    append_padded(target, fields.month, 2);
                } else if (width == 3) {
                    target += style.short_months[month];
                } else if (width == 4) {
                    target += style.long_months[month];
                } else {
                    append_narrow(target, style.long_months[month]);
                }
                return;
            }
            case 'Q':
            case 'q': {
                const int32_t quarter = (fields.month - 1) / 3 + 1;
                if (width <= 2) {
                    append_padded(target, quarter, (width == 1) ? 1 : 2);
                } else {
                    target.push_back('Q');
                    append_padded(target, quarter, 1);
                }
                return;
            }
            case 'd':
                append_padded(target, fields.day, (width == 1) ? 1 : 2);
                return;
            case 'D':
                append_padded(target, fields.day_of_year, (width == 1) ? 1 : width);
                return;
            case 'F':
                append_padded(target, (fields.day - 1) / 7 + 1, 1);
                return;
            case 'E':
            case 'e':
            case 'c': {
                const auto weekday = static_cast<size_t>((fields.weekday < 0) ? 0 : fields.weekday % 7);
                if (width == 4) {
                    target += style.long_days[weekday];
                } else if (width == 5) {
                    append_narrow(target, style.short_days[weekday]);
                } else {
                    target += style.short_days[weekday];
                }
                return;
            }
            case 'a':
            case 'b':
            case 'B':
                target += (fields.hour < 12) ? style.am : style.pm;
                return;
            case 'h':
                append_padded(target, (fields.hour % 12 == 0) ? 12 : fields.hour % 12, (width == 1) ? 1 : 2);
                return;
            case 'H':
                append_padded(target, fields.hour, (width == 1) ? 1 : 2);
                return;
            case 'K':
                append_padded(target, fields.hour % 12, (width == 1) ? 1 : 2);
                return;
            case 'k':
                append_padded(target, (fields.hour == 0) ? 24 : fields.hour, (width == 1) ? 1 : 2);
                return;
            case 'm':
                append_padded(target, fields.minute, (width == 1) ? 1 : 2);
                return;
            case 's':
                append_padded(target, fields.second, (width == 1) ? 1 : 2);
                return;
            case 'S':
                target.append(width, '0');
                return;
            case 'w':
            case 'W':
                append_padded(target, week_of_year(fields), (width == 1) ? 1 : 2);
                return;
            case 'z':
            case 'O':
            case 'v':
            case 'V':
                append_offset(target, fields.offset_seconds, false);
                return;
            case 'Z':
            case 'X':
            case 'x':
                append_offset(target, fields.offset_seconds, true);
                return;
            default:
                target.append(width, letter);
        }
    }

    NEFORCE_NODISCARD string apply_pattern(const string& pattern, const date_fields& fields,
                                           const locale_data::calendar_style& style) {
        string result;
        result.reserve(pattern.size() + 16);

        size_t index = 0;
        while (index < pattern.size()) {
            const char value = pattern[index];

            if (value == '\'') {
                if (index + 1 < pattern.size() && pattern[index + 1] == '\'') {
                    result.push_back('\'');
                    index += 2;
                    continue;
                }

                ++index;
                while (index < pattern.size()) {
                    if (pattern[index] == '\'') {
                        if (index + 1 < pattern.size() && pattern[index + 1] == '\'') {
                            result.push_back('\'');
                            index += 2;
                            continue;
                        }
                        ++index;
                        break;
                    }
                    result.push_back(pattern[index]);
                    ++index;
                }
                continue;
            }

            if (pattern_letter(value)) {
                size_t width = 1;
                while (index + width < pattern.size() && pattern[index + width] == value) {
                    ++width;
                }
                append_pattern_field(result, value, width, fields, style);
                index += width;
                continue;
            }

            result.push_back(value);
            ++index;
        }
        return result;
    }

    NEFORCE_NODISCARD string_view date_pattern_of(const locale_data::calendar_style& style,
                                                  const locale::date_style kind) noexcept {
        switch (kind) {
            case locale::date_style::none:
                return nullptr;
            case locale::date_style::long_fmt:
                return style.long_date;
            case locale::date_style::medium:
                return style.medium_date;
            case locale::date_style::short_fmt:
                return style.short_date;
            case locale::date_style::full:
            case locale::date_style::relative:
                return style.full_date;
        }
        return style.medium_date;
    }

    NEFORCE_NODISCARD string_view time_pattern_of(const locale_data::calendar_style& style,
                                                  const locale::time_style kind) noexcept {
        switch (kind) {
            case locale::time_style::none:
                return nullptr;
            case locale::time_style::long_fmt:
                return style.long_time;
            case locale::time_style::medium:
                return style.medium_time;
            case locale::time_style::short_fmt:
                return style.short_time;
            case locale::time_style::full:
            case locale::time_style::relative:
                return style.full_time;
        }
        return style.medium_time;
    }

    NEFORCE_NODISCARD bool compact_datetime(const char* language) noexcept {
        return string_compare(language, "zh") == 0 || string_compare(language, "ja") == 0 ||
               string_compare(language, "ko") == 0 || string_compare(language, "th") == 0 ||
               string_compare(language, "my") == 0 || string_compare(language, "km") == 0;
    }
} // namespace

void locale::load(const string& bcp47) {
    cleanup();

    string input = bcp47;
    if (input.empty() || input == "C" || input == "POSIX") {
        input = "en-US-POSIX";
    }

    name_parts parts;
    if (!parse_locale_name(input, parts)) {
        NEFORCE_THROW_EXCEPTION(locale_exception("locale: invalid locale name '" + bcp47 + "'"));
    }

    language_code_ = parts.language;
    script_code_ = parts.script;
    country_code_ = parts.country;
    variant_code_ = parts.variant;
    name_ = bcp47_of(parts);
    icu_name_ = posix_of(parts);
}

void locale::cleanup() noexcept {
    collator_ = nullptr;
    converter_ = nullptr;
    num_fmt_ = nullptr;
    curr_fmt_ = nullptr;
    date_fmt_ = nullptr;
}

locale::locale() { load("en-US-POSIX"); }

locale::locale(const string& name) { load(name); }

locale::~locale() { cleanup(); }

locale::locale(const locale& other) { load(other.name_); }

locale& locale::operator=(const locale& other) {
    if (addressof(other) == this) {
        return *this;
    }
    load(other.name_);
    return *this;
}

locale::locale(locale&& other) noexcept :
name_(move(other.name_)),
icu_name_(move(other.icu_name_)),
language_code_(move(other.language_code_)),
script_code_(move(other.script_code_)),
country_code_(move(other.country_code_)),
variant_code_(move(other.variant_code_)) {
    other.collator_ = nullptr;
    other.converter_ = nullptr;
    other.num_fmt_ = nullptr;
    other.curr_fmt_ = nullptr;
    other.date_fmt_ = nullptr;
}

locale& locale::operator=(locale&& other) noexcept {
    if (addressof(other) == this) {
        return *this;
    }
    cleanup();
    name_ = move(other.name_);
    icu_name_ = move(other.icu_name_);
    language_code_ = move(other.language_code_);
    script_code_ = move(other.script_code_);
    country_code_ = move(other.country_code_);
    variant_code_ = move(other.variant_code_);
    other.collator_ = nullptr;
    other.converter_ = nullptr;
    other.num_fmt_ = nullptr;
    other.curr_fmt_ = nullptr;
    other.date_fmt_ = nullptr;
    return *this;
}

locale locale::classic() { return locale("en-US-POSIX"); }

locale locale::system() {
#    ifdef NEFORCE_PLATFORM_WINDOWS
    wchar_t buffer[LOCALE_NAME_MAX_LENGTH] = {};
    const int written = static_cast<int>(::GetUserDefaultLocaleName(buffer, LOCALE_NAME_MAX_LENGTH));
    if (written <= 1) {
        return locale("en-US-POSIX");
    }
    return locale(wcharacter::to_string({buffer, written}));
#    else
    for (const string_view variable: {"LC_ALL", "LC_CTYPE", "LANG"}) {
        const string value = environment::get(variable);
        if (!value.empty() && value != "C" && value != "POSIX") {
            return locale(value);
        }
    }
    return locale("en-US-POSIX");
#    endif
}

locale locale::from_name(const string& name) { return locale(name); }

string locale::display_name(const locale& display_locale) const {
    const auto* language = locale_data::language_name_of(language_code_.data());
    const auto* country = country_code_.empty() ? nullptr : locale_data::country_name_of(country_code_.data());
    const bool native = (display_locale.language_code_ == language_code_);

    string result;
    if (language != nullptr) {
        result += native ? language->native_name : language->english_name;
    } else {
        result += name_;
    }

    if (country != nullptr) {
        string_view before;
        string_view after;
        locale_data::display_name_brackets(display_locale.language_code_.data(), before, after);
        result += before;
        result += native ? country->native_name : country->english_name;
        result += after;
    }
    return result;
}

string locale::native_language_name() const { return display_name(*this); }

string locale::native_country_name() const {
    if (country_code_.empty()) {
        return {};
    }
    return display_name(*this);
}

locale::text_direction locale::direction() const {
    return locale_data::is_right_to_left(language_code_.data()) ? text_direction::RTL : text_direction::LTR;
}

locale::measurement_system locale::measurement() const {
    switch (locale_data::measurement_system_of(country_code_.data())) {
        case 1:
            return measurement_system::US;
        case 2:
            return measurement_system::UK;
        default:
            return measurement_system::SI;
    }
}

int32_t locale::first_day_of_week() const {
    return static_cast<int32_t>(locale_data::first_day_of_week_of(country_code_.data()));
}

vector<string> locale::ui_languages() const {
    vector<string> result;
    if (name_.empty()) {
        return result;
    }

    name_parts parts;
    if (!parse_locale_name(name_, parts)) {
        result.push_back(name_);
        return result;
    }

    push_unique(result, bcp47_of(parts));

    if (!parts.variant.empty()) {
        name_parts without_variant = parts;
        without_variant.variant.clear();
        push_unique(result, bcp47_of(without_variant));
    }
    if (!parts.country.empty()) {
        name_parts without_country = parts;
        without_country.country.clear();
        without_country.variant.clear();
        push_unique(result, bcp47_of(without_country));
    }
    if (!parts.script.empty()) {
        name_parts without_script = parts;
        without_script.script.clear();
        without_script.variant.clear();
        push_unique(result, bcp47_of(without_script));
    }

    push_unique(result, parts.language);
    return result;
}

bool locale::is_alpha(const char32_t cp) noexcept { return flagged(class_mask(cp), ALPHA); }
bool locale::is_digit(const char32_t cp) noexcept { return flagged(class_mask(cp), DIGIT); }
bool locale::is_alnum(const char32_t cp) noexcept {
    const uint16_t mask = class_mask(cp);
    return flagged(mask, ALPHA) || flagged(mask, DIGIT);
}
bool locale::is_space(const char32_t cp) noexcept { return flagged(class_mask(cp), SPACE); }
bool locale::is_upper(const char32_t cp) noexcept { return flagged(class_mask(cp), UPPER); }
bool locale::is_lower(const char32_t cp) noexcept { return flagged(class_mask(cp), LOWER); }
bool locale::is_punct(const char32_t cp) noexcept { return flagged(class_mask(cp), PUNCT); }
bool locale::is_print(const char32_t cp) noexcept { return flagged(class_mask(cp), PRINTABLE); }
bool locale::is_titlecase(const char32_t cp) noexcept { return flagged(class_mask(cp), TITLECASE); }
bool locale::is_white_space(const char32_t cp) noexcept { return flagged(class_mask(cp), WHITE_SPACE); }

char32_t locale::to_upper(const char32_t cp) noexcept { return static_cast<char32_t>(upper_of(cp)); }
char32_t locale::to_lower(const char32_t cp) noexcept { return static_cast<char32_t>(lower_of(cp)); }
char32_t locale::to_titlecase(const char32_t cp) noexcept { return static_cast<char32_t>(title_of(cp)); }

int locale::compare(const string& a, const string& b, const collate_strength strength) const {
    const int primary = compare_codepoints(primary_form(a), primary_form(b));
    if (primary != 0 || strength == collate_strength::primary) {
        return primary;
    }

    const int secondary = compare_codepoints(secondary_form(a), secondary_form(b));
    if (secondary != 0 || strength == collate_strength::secondary) {
        return secondary;
    }

    return compare_codepoints(tertiary_form(a), tertiary_form(b));
}

string locale::collation_key(const string& s) const {
    string key;
    key.reserve(s.size() * KEY_LEVEL_STRIDE * 3 + KEY_LEVEL_STRIDE * 3);

    append_key_level(key, primary_form(s));
    append_key_level(key, secondary_form(s));
    append_key_level(key, tertiary_form(s));
    return key;
}

string locale::to_multibyte(const u32string& ucs4) const {
    string result;
    result.reserve(ucs4.size());
    for (const char32_t value: ucs4) {
        codepoint(value).append_to(result);
    }
    return result;
}

u32string locale::to_ucs4(const string& mb) const {
    u32string result;
    result.reserve(mb.size());

    const auto* bytes = reinterpret_cast<const byte_t*>(mb.data());
    size_t index = 0;
    while (index < mb.size()) {
        result.push_back(codepoint::decode_utf8(bytes, index, mb.size()).to_char32());
    }
    return result;
}

string locale::format_number(const int64_t value) const {
    return format_integer(value, number_style_for(language_code_, variant_code_));
}

string locale::format_number(const double value, const int32_t fraction_digits) const {
    const auto& style = number_style_for(language_code_, variant_code_);

    if (fraction_digits < 0) {
        return format_decimal(value, DEFAULT_FRACTION_DIGITS, true, style);
    }
    return format_decimal(value, fraction_digits, false, style);
}

string locale::format_currency(const double value, const string& iso_4217_code) const {
    const auto& style = number_style_for(language_code_, variant_code_);
    const auto* currency = locale_data::currency_of(iso_4217_code.data());

    const string symbol = (currency != nullptr) ? string(currency->symbol) : iso_4217_code;
    const auto digits = (currency != nullptr) ? static_cast<int32_t>(currency->fraction_digits) : 2;
    const string amount = format_decimal(value, digits, false, style);

    string result;
    if (style.currency_before) {
        result += symbol;
        result += style.currency_separator;
        result += amount;
    } else {
        result += amount;
        result += style.currency_separator;
        result += symbol;
    }
    return result;
}

string locale::format_datetime(const datetime& dt, const date_style ds, const time_style ts) const {
    const auto& style = locale_data::calendar_style_of(language_code_.data(), country_code_.data());
    const auto fields = fields_of(dt);
    const string_view date_pattern = date_pattern_of(style, ds);
    const string_view time_pattern = time_pattern_of(style, ts);

    string result;
    if (date_pattern != nullptr) {
        result += apply_pattern(date_pattern, fields, style);
    }
    if (date_pattern != nullptr && time_pattern != nullptr) {
        result += compact_datetime(language_code_.data()) ? " " : ", ";
    }
    if (time_pattern != nullptr) {
        result += apply_pattern(time_pattern, fields, style);
    }
    return result;
}

string locale::format_datetime(const datetime& dt, const string& pattern) const {
    const auto& style = locale_data::calendar_style_of(language_code_.data(), country_code_.data());
    return apply_pattern(pattern, fields_of(dt), style);
}

string locale::format_datetime() const {
    return format_datetime(datetime::now(), date_style::medium, time_style::medium);
}

locale::numeric_info locale::numeric() const {
    const auto& style = number_style_for(language_code_, variant_code_);

    numeric_info info;
    info.decimal_point = style.decimal_point;
    info.thousands_sep = style.group_separator;
    info.grouping = grouping_of(style);
    info.percent_sign = locale_data::percent_sign_of(language_code_.data());
    info.minus_sign = "-";
    info.plus_sign = "+";
    info.exponential = "E";
    info.nan_symbol = "NaN";
    info.infinity_symbol = "\xE2\x88\x9E";
    return info;
}

locale::monetary_info locale::monetary() const {
    const auto& style = number_style_for(language_code_, variant_code_);
    const string_view code = locale_data::home_currency_of(country_code_.data());
    const auto* currency = locale_data::currency_of(code);

    monetary_info info;
    info.currency_symbol = (currency != nullptr) ? currency->symbol : code;
    info.int_curr_symbol = code;
    info.mon_decimal_point = style.decimal_point;
    info.mon_thousands_sep = style.group_separator;
    info.mon_grouping = grouping_of(style);
    info.positive_sign = "";
    info.negative_sign = "-";
    info.frac_digits = (currency != nullptr) ? currency->fraction_digits : 2;
    info.int_frac_digits = info.frac_digits;
    info.cash_frac_digits = info.frac_digits;
    info.p_cs_precedes = style.currency_before;
    info.n_cs_precedes = style.currency_before;
    info.p_sep_by_space = (style.currency_separator[0] != '\0') ? 1 : 0;
    info.n_sep_by_space = info.p_sep_by_space;
    return info;
}

locale::time_info locale::time() const {
    const auto& style = locale_data::calendar_style_of(language_code_.data(), country_code_.data());

    time_info info;
    info.date_fmt = style.medium_date;
    info.time_fmt = style.medium_time;

    info.datetime_fmt = style.medium_date;
    info.datetime_fmt += compact_datetime(language_code_.data()) ? " " : ", ";
    info.datetime_fmt += style.medium_time;

    for (size_t i = 0; i < 7; ++i) {
        info.day_names.emplace_back(style.long_days[i]);
        info.abbr_day_names.emplace_back(style.short_days[i]);
    }
    for (size_t i = 0; i < 12; ++i) {
        info.month_names.emplace_back(style.long_months[i]);
        info.abbr_month_names.emplace_back(style.short_months[i]);
    }

    info.am_str = style.am;
    info.pm_str = style.pm;
    return info;
}

vector<string> locale::available_locales() {
    vector<string> result;

    const auto locales = locale_data::available_locales();
    result.reserve(locales.size());
    for (uint32_t i = 0; i < locales.size(); ++i) {
        result.emplace_back(locales[i]);
    }
    return result;
}

vector<string> locale::available_countries(const string& /*language*/) {
    vector<string> result;

    const auto countries = locale_data::iso_countries();
    result.reserve(countries.size());
    for (uint32_t i = 0; i < countries.size(); ++i) {
        result.emplace_back(countries[i]);
    }
    return result;
}

bool locale::is_valid_locale(const string& name) {
    name_parts parts;
    if (!parse_locale_name(name, parts)) {
        return false;
    }
    if (parts.language.size() < 2 || parts.language.size() > 3) {
        return false;
    }
    if (!parts.script.empty() && parts.script.size() != 4) {
        return false;
    }
    if (!parts.country.empty() && parts.country.size() != 2 && !all_digit(parts.country)) {
        return false;
    }
    return locale_data::is_known_language(parts.language.data());
}

NEFORCE_END_NAMESPACE__
#endif // !NEFORCE_SUPPORT_ICU
