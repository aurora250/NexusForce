#include "NeForce/core/file/json/json_parser.hpp"
#include "NeForce/core/memory/bit.hpp"
#include "NeForce/core/memory/memory.hpp"
#include "NeForce/core/numeric/numeric_traits.hpp"
#include "NeForce/core/string/to_numerics.hpp"
NEFORCE_BEGIN_NAMESPACE__

namespace {
    constexpr double k_pow10[] = {1e0,  1e1,  1e2,  1e3,  1e4,  1e5,  1e6,  1e7,  1e8,  1e9,  1e10, 1e11,
                                  1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19, 1e20, 1e21, 1e22};

    constexpr uint64_t k_exact_mantissa = 1ULL << 53;
    constexpr uint64_t k_magnitude_limit = 1844674407370955161ULL;
    constexpr uint64_t k_magnitude_last = 5;
    constexpr int k_fast_digits = 18;

    NEFORCE_NODISCARD uint64_t special_mask(const uint64_t word) noexcept {
        constexpr uint64_t ones = 0x0101010101010101ULL;
        constexpr uint64_t highs = 0x8080808080808080ULL;
        constexpr uint64_t quotes = 0x2222222222222222ULL;
        constexpr uint64_t slashes = 0x5C5C5C5C5C5C5C5CULL;
        constexpr uint64_t controls = 0xE0E0E0E0E0E0E0E0ULL;
        const auto has_zero = [](const uint64_t value) { return (value - ones) & ~value & highs; };
        return has_zero(word ^ quotes) | has_zero(word ^ slashes) | has_zero(word & controls);
    }

    NEFORCE_NODISCARD const char* skip_space(const char* cursor, const char* end) noexcept {
        if (cursor == end) {
            return cursor;
        }
        const char first = *cursor;
        if (first != ' ' && first != '\t' && first != '\n' && first != '\r') {
            return cursor;
        }
        ++cursor;
        while (cursor != end) {
            const char c = *cursor;
            if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
                break;
            }
            ++cursor;
        }
        return cursor;
    }

    int hex_value(const char c) noexcept {
        if (c >= '0' && c <= '9') {
            return c - '0';
        }
        if (c >= 'a' && c <= 'f') {
            return c - 'a' + 10;
        }
        if (c >= 'A' && c <= 'F') {
            return c - 'A' + 10;
        }
        return -1;
    }

    void append_utf8(string& out, const unsigned int code_point) {
        if (code_point <= 0x7F) {
            out += static_cast<char>(code_point);
        } else if (code_point <= 0x7FF) {
            out += static_cast<char>(0xC0 | (code_point >> 6));
            out += static_cast<char>(0x80 | (code_point & 0x3F));
        } else if (code_point <= 0xFFFF) {
            out += static_cast<char>(0xE0 | (code_point >> 12));
            out += static_cast<char>(0x80 | ((code_point >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (code_point & 0x3F));
        } else {
            out += static_cast<char>(0xF0 | (code_point >> 18));
            out += static_cast<char>(0x80 | ((code_point >> 12) & 0x3F));
            out += static_cast<char>(0x80 | ((code_point >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (code_point & 0x3F));
        }
    }

    enum class scan_status : uint8_t {
        ok,
        control_character,
        unterminated
    };

    NEFORCE_NODISCARD scan_status scan_string(const char*& cursor, const char* end, const char*& begin,
                                              const char*& finish, bool& escaped) noexcept {
        ++cursor;
        begin = cursor;
        escaped = false;
        while (true) {
            if (end - cursor >= 8) {
                uint64_t word = 0;
                memory_copy(&word, cursor, sizeof(word));
                const uint64_t mask = special_mask(word);
                if (mask == 0) {
                    cursor += 8;
                    continue;
                }
                const size_t offset = static_cast<size_t>(countr_zero(static_cast<uintptr_t>(mask))) >> 3;
                if (offset != 0) {
                    cursor += offset;
                }
            } else if (cursor == end) {
                return scan_status::unterminated;
            }
            const char c = *cursor;
            if (c == '"') {
                finish = cursor;
                ++cursor;
                return scan_status::ok;
            }
            if (c == '\\') {
                escaped = true;
                ++cursor;
                if (cursor != end) {
                    ++cursor;
                }
                continue;
            }
            if (static_cast<unsigned char>(c) < 0x20) {
                return scan_status::control_character;
            }
            ++cursor;
        }
    }
} // namespace


void json_parser::decode_string(const char* begin, const char* finish, string& out) {
    out.clear();
    out.reserve(static_cast<size_t>(finish - begin));
    for (const char* cursor = begin; cursor != finish; ++cursor) {
        const char c = *cursor;
        if (c != '\\') {
            out += c;
            continue;
        }
        ++cursor;
        if (cursor == finish) {
            position_ = offset_of(cursor);
            NEFORCE_THROW_EXCEPTION(json_exception("JSON invalid escape."));
        }
        switch (*cursor) {
            case '"':
                out += '"';
                break;
            case '\\':
                out += '\\';
                break;
            case '/':
                out += '/';
                break;
            case 'b':
                out += '\b';
                break;
            case 'f':
                out += '\f';
                break;
            case 'n':
                out += '\n';
                break;
            case 'r':
                out += '\r';
                break;
            case 't':
                out += '\t';
                break;
            case 'u': {
                if (finish - cursor < 5) {
                    position_ = offset_of(cursor);
                    NEFORCE_THROW_EXCEPTION(json_exception("JSON invalid unicode escape."));
                }
                unsigned int code = 0;
                for (int k = 1; k <= 4; ++k) {
                    const int v = hex_value(cursor[k]);
                    if (v < 0) {
                        position_ = offset_of(cursor);
                        NEFORCE_THROW_EXCEPTION(json_exception("JSON invalid unicode escape."));
                    }
                    code = (code << 4) | static_cast<unsigned int>(v);
                }
                cursor += 4;
                if (code >= 0xD800 && code <= 0xDBFF) {
                    if (finish - cursor <= 6 || cursor[1] != '\\' || cursor[2] != 'u') {
                        position_ = offset_of(cursor);
                        NEFORCE_THROW_EXCEPTION(json_exception("JSON unpaired high surrogate."));
                    }
                    unsigned int low = 0;
                    for (int k = 3; k <= 6; ++k) {
                        const int v = hex_value(cursor[k]);
                        if (v < 0) {
                            position_ = offset_of(cursor);
                            NEFORCE_THROW_EXCEPTION(json_exception("JSON invalid unicode escape."));
                        }
                        low = (low << 4) | static_cast<unsigned int>(v);
                    }
                    if (low < 0xDC00 || low > 0xDFFF) {
                        position_ = offset_of(cursor);
                        NEFORCE_THROW_EXCEPTION(json_exception("JSON invalid low surrogate."));
                    }
                    code = 0x10000 + ((code - 0xD800) << 10) + (low - 0xDC00);
                    cursor += 6;
                } else if (code >= 0xDC00 && code <= 0xDFFF) {
                    position_ = offset_of(cursor);
                    NEFORCE_THROW_EXCEPTION(json_exception("JSON unpaired low surrogate."));
                }
                append_utf8(out, code);
                break;
            }
            default:
                position_ = offset_of(cursor);
                NEFORCE_THROW_EXCEPTION(json_exception("JSON invalid escape."));
        }
    }
}

json_value json_parser::parse_string(json_document& doc, const char*& cursor, const char* end) {
    if (cursor == end || *cursor != '"') {
        position_ = offset_of(cursor);
        NEFORCE_THROW_EXCEPTION(json_exception("JSON string expected."));
    }
    const char* begin = nullptr;
    const char* finish = nullptr;
    bool escaped = false;
    const scan_status status = scan_string(cursor, end, begin, finish, escaped);
    if (status != scan_status::ok) {
        position_ = offset_of(cursor);
        if (status == scan_status::control_character) {
            NEFORCE_THROW_EXCEPTION(json_exception("JSON string contains control character."));
        }
        NEFORCE_THROW_EXCEPTION(json_exception("JSON string not terminated."));
    }
    const string_view text(begin, static_cast<size_t>(finish - begin));
    if (!escaped) {
        if (owning_) {
            return doc.make_string_value(text);
        }
        return json_value::make_string(text);
    }
    decode_string(begin, finish, decode_scratch_);
    return doc.make_string_value(string_view(decode_scratch_.data(), decode_scratch_.size()));
}

json_value json_parser::parse_number(json_document& /*unused*/, const char*& cursor, const char* end) {
    const char* const begin = cursor;
    bool negative = false;
    if (*cursor == '-') {
        negative = true;
        ++cursor;
    }
    if (cursor == end || !is_digit(*cursor)) {
        position_ = offset_of(cursor);
        NEFORCE_THROW_EXCEPTION(json_exception("JSON invalid number."));
    }

    uint64_t significand = 0;
    int exponent = 0;
    bool overflow = false;
    bool truncated = false;
    bool real = false;

    if (*cursor == '0') {
        ++cursor;
        if (cursor != end && is_digit(*cursor)) {
            position_ = offset_of(cursor);
            NEFORCE_THROW_EXCEPTION(json_exception("JSON number has leading zero."));
        }
    } else {
        int budget = k_fast_digits;
        while (budget != 0 && cursor != end && is_digit(*cursor)) {
            significand = significand * 10 + static_cast<uint64_t>(*cursor - '0');
            --budget;
            ++cursor;
        }
        while (cursor != end && is_digit(*cursor)) {
            const auto digit = static_cast<uint64_t>(*cursor - '0');
            if (overflow) {
                ++exponent;
                truncated = truncated || digit != 0;
            } else if (significand > k_magnitude_limit ||
                       (significand == k_magnitude_limit && digit > k_magnitude_last)) {
                overflow = true;
                ++exponent;
                truncated = digit != 0;
            } else {
                significand = significand * 10 + digit;
            }
            ++cursor;
        }
    }

    if (cursor != end && *cursor == '.') {
        real = true;
        ++cursor;
        if (cursor == end || !is_digit(*cursor)) {
            position_ = offset_of(cursor);
            NEFORCE_THROW_EXCEPTION(json_exception("JSON fraction requires digits."));
        }
        while (cursor != end && is_digit(*cursor)) {
            const auto digit = static_cast<uint64_t>(*cursor - '0');
            if (overflow) {
                truncated = truncated || digit != 0;
            } else if (significand > k_magnitude_limit ||
                       (significand == k_magnitude_limit && digit > k_magnitude_last)) {
                overflow = true;
                truncated = digit != 0;
            } else {
                significand = significand * 10 + digit;
                --exponent;
            }
            ++cursor;
        }
    }

    if (cursor != end && (*cursor == 'e' || *cursor == 'E')) {
        real = true;
        ++cursor;
        bool exponent_negative = false;
        if (cursor != end && (*cursor == '+' || *cursor == '-')) {
            exponent_negative = *cursor == '-';
            ++cursor;
        }
        if (cursor == end || !is_digit(*cursor)) {
            position_ = offset_of(cursor);
            NEFORCE_THROW_EXCEPTION(json_exception("JSON exponent requires digits."));
        }
        int written = 0;
        while (cursor != end && is_digit(*cursor)) {
            if (written < 100000) {
                written = written * 10 + (*cursor - '0');
            }
            ++cursor;
        }
        exponent += exponent_negative ? -written : written;
    }

    if (!real && !overflow) {
        constexpr auto int_max = static_cast<uint64_t>(numeric_traits<int64_t>::max());
        if (negative) {
            if (significand <= int_max + 1ULL) {
                return json_value::make_integer(static_cast<int64_t>(0ULL - significand));
            }
        } else if (significand <= int_max) {
            return json_value::make_integer(static_cast<int64_t>(significand));
        } else {
            return json_value::make_uinteger(significand);
        }
    }

    if (!overflow && !truncated && significand <= k_exact_mantissa && exponent >= -22 && exponent <= 22) {
        auto value = static_cast<double>(significand);
        value = exponent >= 0 ? value * k_pow10[exponent] : value / k_pow10[-exponent];
        return json_value::make_real(negative ? -value : value);
    }
    const string_view text(begin, static_cast<size_t>(cursor - begin));
    double converted = 0.0;
    if (inner::parse_double_exact(text, converted)) {
        return json_value::make_real(converted);
    }
    return json_value::make_real(to_float64(text));
}

json_value json_parser::parse_keyword(json_document& /*unused*/, const char*& cursor, const char* end) {
    const auto remaining = static_cast<size_t>(end - cursor);
    switch (*cursor) {
        case 't':
            if (remaining >= 4 && cursor[1] == 'r' && cursor[2] == 'u' && cursor[3] == 'e') {
                cursor += 4;
                return json_value::make_bool(true);
            }
            break;
        case 'f':
            if (remaining >= 5 && cursor[1] == 'a' && cursor[2] == 'l' && cursor[3] == 's' && cursor[4] == 'e') {
                cursor += 5;
                return json_value::make_bool(false);
            }
            break;
        case 'n':
            if (remaining >= 4 && cursor[1] == 'u' && cursor[2] == 'l' && cursor[3] == 'l') {
                cursor += 4;
                return json_value::make_null();
            }
            break;
        default:
            break;
    }
    position_ = offset_of(cursor);
    NEFORCE_THROW_EXCEPTION(json_exception("JSON invalid keyword."));
}

json_value json_parser::parse_array(json_document& doc, const char*& cursor, const char* end, const size_t depth) {
    ++cursor;
    json_value* node = doc.new_container(false, reserve_hint(array_sizes_, array_confirms_, depth));
    const char* body = skip_space(cursor, end);
    if (body != end && *body == ']') {
        cursor = body + 1;
        update_reserve_hint(array_sizes_, array_confirms_, depth, 0);
        return *node;
    }
    while (true) {
        doc.array_append(*node, parse_value(doc, cursor, end, depth + 1));
        cursor = skip_space(cursor, end);
        if (cursor != end && *cursor == ',') {
            ++cursor;
            continue;
        }
        if (cursor != end && *cursor == ']') {
            ++cursor;
            break;
        }
        position_ = offset_of(cursor);
        NEFORCE_THROW_EXCEPTION(json_exception("JSON array not terminated."));
    }
    update_reserve_hint(array_sizes_, array_confirms_, depth, node->element_count());
    return *node;
}

json_value json_parser::parse_object(json_document& doc, const char*& cursor, const char* end, const size_t depth) {
    ++cursor;
    json_value* node = doc.new_container(true, reserve_hint(object_sizes_, object_confirms_, depth));
    const char* body = skip_space(cursor, end);
    if (body != end && *body == '}') {
        cursor = body + 1;
        update_reserve_hint(object_sizes_, object_confirms_, depth, 0);
        return *node;
    }
    while (true) {
        cursor = skip_space(cursor, end);
        if (cursor == end || *cursor != '"') {
            position_ = offset_of(cursor);
            NEFORCE_THROW_EXCEPTION(json_exception("JSON object key expected."));
        }
        const char* key_begin = nullptr;
        const char* key_finish = nullptr;
        bool key_escaped = false;
        const scan_status key_status = scan_string(cursor, end, key_begin, key_finish, key_escaped);
        if (key_status != scan_status::ok) {
            position_ = offset_of(cursor);
            if (key_status == scan_status::control_character) {
                NEFORCE_THROW_EXCEPTION(json_exception("JSON string contains control character."));
            }
            NEFORCE_THROW_EXCEPTION(json_exception("JSON string not terminated."));
        }
        string_view key;
        if (key_escaped) {
            decode_string(key_begin, key_finish, decode_scratch_);
            key = doc.store_string(string_view(decode_scratch_.data(), decode_scratch_.size()));
        } else {
            key = string_view(key_begin, static_cast<size_t>(key_finish - key_begin));
        }
        cursor = skip_space(cursor, end);
        if (cursor == end || *cursor != ':') {
            position_ = offset_of(cursor);
            NEFORCE_THROW_EXCEPTION(json_exception("JSON object key not followed by colon."));
        }
        ++cursor;
        const json_value value = parse_value(doc, cursor, end, depth + 1);
        doc.object_append(*node, key, value, owning_);
        cursor = skip_space(cursor, end);
        if (cursor != end && *cursor == ',') {
            ++cursor;
            continue;
        }
        if (cursor != end && *cursor == '}') {
            ++cursor;
            break;
        }
        position_ = offset_of(cursor);
        NEFORCE_THROW_EXCEPTION(json_exception("JSON object not terminated."));
    }
    update_reserve_hint(object_sizes_, object_confirms_, depth, node->element_count());
    return *node;
}

json_value json_parser::parse_value(json_document& doc, const char*& cursor, const char* end, const size_t depth) {
    cursor = skip_space(cursor, end);
    if (cursor == end) {
        position_ = offset_of(cursor);
        NEFORCE_THROW_EXCEPTION(json_exception("JSON value expected."));
    }
    switch (*cursor) {
        case '{':
            return parse_object(doc, cursor, end, depth);
        case '[':
            return parse_array(doc, cursor, end, depth);
        case '"':
            return parse_string(doc, cursor, end);
        case 't':
        case 'f':
        case 'n':
            return parse_keyword(doc, cursor, end);
        default:
            return parse_number(doc, cursor, end);
    }
}

json_document json_parser::parse() {
    json_document doc;
    doc.reserve(text_.size() * 3 + 65536);
    const char* data = text_.data();
    const char* end = data + text_.size();
    const char* cursor = data;
    const json_value value = parse_value(doc, cursor, end, 0);
    cursor = skip_space(cursor, end);
    if (cursor != end) {
        position_ = offset_of(cursor);
        NEFORCE_THROW_EXCEPTION(json_exception("JSON trailing content."));
    }
    doc.root_ = value;
    return doc;
}

optional<json_document> json_parser::try_parse() noexcept {
    try {
        return optional<json_document>(parse());
    } catch (...) {
        return optional<json_document>();
    }
}

NEFORCE_END_NAMESPACE__
