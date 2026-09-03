#include <NeForce/core/file/json/json_parser.hpp>
#include <NeForce/core/memory/bit.hpp>
#include <NeForce/core/simd/types.hpp>
#include <NeForce/core/string/codepoint.hpp>
#include <NeForce/core/utility/hexadecimal.hpp>
#include <NeForce/core/utility/packages.hpp>
NEFORCE_BEGIN_NAMESPACE__

void json_parser::skip_space() noexcept {
#ifdef NEFORCE_SIMD_AVX2
    while (pos_ + 32 <= len_) {
        const simd::vec256_t v = ::_mm256_loadu_si256(reinterpret_cast<const simd::vec256_t*>(text_.data() + pos_));
        simd::vec256_t ws = ::_mm256_cmpeq_epi8(v, ::_mm256_set1_epi8(' '));
        ws = ::_mm256_or_si256(ws, ::_mm256_cmpeq_epi8(v, ::_mm256_set1_epi8('\t')));
        ws = ::_mm256_or_si256(ws, ::_mm256_cmpeq_epi8(v, ::_mm256_set1_epi8('\n')));
        ws = ::_mm256_or_si256(ws, ::_mm256_cmpeq_epi8(v, ::_mm256_set1_epi8('\v')));
        ws = ::_mm256_or_si256(ws, ::_mm256_cmpeq_epi8(v, ::_mm256_set1_epi8('\f')));
        ws = ::_mm256_or_si256(ws, ::_mm256_cmpeq_epi8(v, ::_mm256_set1_epi8('\r')));
        const int mask = ::_mm256_movemask_epi8(ws);
        if (mask == -1) {
            pos_ += 32;
        } else {
            pos_ += static_cast<size_t>(countr_zero(static_cast<unsigned>(~mask)));
            return;
        }
    }
#endif
#ifdef NEFORCE_SIMD_SSE2
    while (pos_ + 16 <= len_) {
        const simd::vec128_t v = ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(text_.data() + pos_));
        simd::vec128_t ws = ::_mm_cmpeq_epi8(v, ::_mm_set1_epi8(' '));
        ws = ::_mm_or_si128(ws, ::_mm_cmpeq_epi8(v, ::_mm_set1_epi8('\t')));
        ws = ::_mm_or_si128(ws, ::_mm_cmpeq_epi8(v, ::_mm_set1_epi8('\n')));
        ws = ::_mm_or_si128(ws, ::_mm_cmpeq_epi8(v, ::_mm_set1_epi8('\v')));
        ws = ::_mm_or_si128(ws, ::_mm_cmpeq_epi8(v, ::_mm_set1_epi8('\f')));
        ws = ::_mm_or_si128(ws, ::_mm_cmpeq_epi8(v, ::_mm_set1_epi8('\r')));
        const int mask = ::_mm_movemask_epi8(ws);
        if (mask == 0xFFFF) {
            pos_ += 16;
        } else {
            // NOLINTNEXTLINE(bugprone-misplaced-widening-cast)
            pos_ += static_cast<size_t>(countr_zero(static_cast<uintptr_t>(~mask)));
            return;
        }
    }
#endif
    while (pos_ < len_ && is_space(text_[pos_])) {
        pos_++;
    }
}

char json_parser::current() const noexcept {
    if (pos_ < len_) {
        return text_[pos_];
    }
    return '\0';
}

bool json_parser::eof() const noexcept { return pos_ >= len_; }

unique_ptr<json_string> json_parser::parse_string() {
    pos_++;
    string result;

    while (pos_ < len_) {
#ifdef NEFORCE_SIMD_AVX2
        while (pos_ + 32 <= len_) {
            const simd::vec256_t v = ::_mm256_loadu_si256(reinterpret_cast<const simd::vec256_t*>(text_.data() + pos_));
            const simd::vec256_t is_quote = ::_mm256_cmpeq_epi8(v, ::_mm256_set1_epi8('"'));
            const simd::vec256_t is_bs = ::_mm256_cmpeq_epi8(v, ::_mm256_set1_epi8('\\'));
            const simd::vec256_t is_ctrl = ::_mm256_cmpeq_epi8(::_mm256_min_epu8(v, ::_mm256_set1_epi8(0x1F)), v);
            const simd::vec256_t special = ::_mm256_or_si256(::_mm256_or_si256(is_quote, is_bs), is_ctrl);
            const int mask = ::_mm256_movemask_epi8(special);
            if (mask == 0) {
                result.append(text_.data() + pos_, 32);
                pos_ += 32;
            } else {
                const int advance = countr_zero(static_cast<unsigned>(mask));
                if (advance > 0) {
                    result.append(text_.data() + pos_, static_cast<size_t>(advance));
                    pos_ += static_cast<size_t>(advance);
                }
                break;
            }
        }
        if (pos_ >= len_) {
            break;
        }
#endif
#ifdef NEFORCE_SIMD_SSE2
        while (pos_ + 16 <= len_) {
            const simd::vec128_t v = ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(text_.data() + pos_));
            const simd::vec128_t is_quote = ::_mm_cmpeq_epi8(v, ::_mm_set1_epi8('"'));
            const simd::vec128_t is_bs = ::_mm_cmpeq_epi8(v, ::_mm_set1_epi8('\\'));
            const simd::vec128_t is_ctrl = ::_mm_cmpeq_epi8(::_mm_min_epu8(v, ::_mm_set1_epi8(0x1F)), v);
            const simd::vec128_t special = ::_mm_or_si128(::_mm_or_si128(is_quote, is_bs), is_ctrl);
            const int mask = ::_mm_movemask_epi8(special);
            if (mask == 0) {
                result.append(text_.data() + pos_, 16);
                pos_ += 16;
            } else {
                const int advance = countr_zero(static_cast<uintptr_t>(mask));
                if (advance > 0) {
                    result.append(text_.data() + pos_, static_cast<size_t>(advance));
                    pos_ += static_cast<size_t>(advance);
                }
                break;
            }
        }
        if (pos_ >= len_) {
            break;
        }
#endif
        const char c = text_[pos_++];
        if (c == '"') {
            return make_unique<json_string>(move(result));
        }
        if (c == '\\') {
            if (pos_ >= len_) {
                NEFORCE_THROW_EXCEPTION(json_exception("Unterminated escape sequence in string"));
            }
            const char esc = text_[pos_++];
            switch (esc) {
                case '"':
                    result += '"';
                    break;
                case '\\':
                    result += '\\';
                    break;
                case '/':
                    result += '/';
                    break;
                case 'b':
                    result += '\b';
                    break;
                case 'f':
                    result += '\f';
                    break;
                case 'n':
                    result += '\n';
                    break;
                case 'r':
                    result += '\r';
                    break;
                case 't':
                    result += '\t';
                    break;
                case 'u': {
                    if (pos_ + 4 > len_) {
                        NEFORCE_THROW_EXCEPTION(json_exception("Unterminated unicode escape sequence"));
                    }
                    const string_view hex_str = text_.view(pos_, 4);
                    for (size_t i = 0; i < 4; i++) {
                        if (!is_xdigit(hex_str[i])) {
                            NEFORCE_THROW_EXCEPTION(json_exception("Invalid unicode escape sequence"));
                        }
                    }
                    pos_ += 4;
                    try {
                        const uint32_t cp_val = static_cast<uint32_t>(hexadecimal(hex_str).value());
                        const auto high = static_cast<char16_t>(cp_val);

                        if (codepoint::is_high_surrogate(high)) {
                            if (pos_ + 6 <= len_ && text_[pos_] == '\\' && text_[pos_ + 1] == 'u') {
                                const string_view hex_str2 = text_.view(pos_ + 2, 4);
                                bool valid_low = true;
                                for (size_t i = 0; i < 4 && valid_low; i++) {
                                    if (!is_xdigit(hex_str2[i])) {
                                        valid_low = false;
                                    }
                                }
                                if (valid_low) {
                                    const uint32_t low_val = static_cast<uint32_t>(hexadecimal(hex_str2).value());
                                    const auto low = static_cast<char16_t>(low_val);
                                    if (codepoint::is_low_surrogate(low)) {
                                        pos_ += 6;
                                        codepoint::combine_surrogates(high, low).append_to(result);
                                        break;
                                    }
                                }
                            }
                            NEFORCE_THROW_EXCEPTION(json_exception("Unpaired high surrogate in unicode escape"));
                        }
                        if (codepoint::is_low_surrogate(high)) {
                            NEFORCE_THROW_EXCEPTION(json_exception("Unpaired low surrogate in unicode escape"));
                        }
                        if (!codepoint::is_valid_codepoint(cp_val)) {
                            NEFORCE_THROW_EXCEPTION(json_exception("Unicode codepoint out of range"));
                        }
                        codepoint{cp_val}.append_to(result);
                    } catch (const exception&) {
                        throw;
                    } catch (...) {
                        NEFORCE_THROW_EXCEPTION(json_exception("Invalid unicode escape value"));
                    }
                    break;
                }
                default:
                    NEFORCE_THROW_EXCEPTION(json_exception("Invalid escape sequence in string"));
            }
        } else if (static_cast<uint8_t>(c) < 0x20) {
            NEFORCE_THROW_EXCEPTION(json_exception("Unescaped control character in string"));
        } else {
            result += c;
        }
    }
    NEFORCE_THROW_EXCEPTION(json_exception("Unterminated string"));
    unreachable();
}

unique_ptr<json_number> json_parser::parse_number() {
    const size_t start = pos_;
    if (current() == '-') {
        pos_++;
    }
    if (current() == '0') {
        pos_++;
        if (pos_ < len_ && text_[pos_] == '.') {
            pos_++;
            if (pos_ >= len_ || !is_digit(text_[pos_])) {
                NEFORCE_THROW_EXCEPTION(json_exception("Invalid decimal part"));
            }
            while (pos_ < len_ && is_digit(text_[pos_])) {
                pos_++;
            }
        } else if (pos_ < len_ && is_digit(text_[pos_])) {
            NEFORCE_THROW_EXCEPTION(json_exception("Leading zeros are not allowed"));
        }
    } else if (is_digit(current())) {
        while (pos_ < len_ && is_digit(text_[pos_])) {
            pos_++;
        }
        if (pos_ < len_ && text_[pos_] == '.') {
            pos_++;
            if (pos_ >= len_ || !is_digit(text_[pos_])) {
                NEFORCE_THROW_EXCEPTION(json_exception("Invalid decimal part"));
            }
            while (pos_ < len_ && is_digit(text_[pos_])) {
                pos_++;
            }
        }
    } else {
        NEFORCE_THROW_EXCEPTION(json_exception("Invalid number format"));
    }

    if (pos_ < len_ && (text_[pos_] == 'e' || text_[pos_] == 'E')) {
        pos_++;
        if (pos_ < len_ && (text_[pos_] == '+' || text_[pos_] == '-')) {
            pos_++;
        }
        if (pos_ >= len_ || !is_digit(text_[pos_])) {
            NEFORCE_THROW_EXCEPTION(json_exception("Invalid exponent part"));
        }
        while (pos_ < len_ && is_digit(text_[pos_])) {
            pos_++;
        }
    }

    try {
        double value = float64::parse(text_.view(start, pos_ - start)).value();
        return make_unique<json_number>(value);
    } catch (...) {
        NEFORCE_THROW_EXCEPTION(json_exception("Invalid number value"));
    }
    unreachable();
}

unique_ptr<json_value> json_parser::parse_keyword() {
    const size_t start = pos_;
    while (pos_ < len_ && is_alpha(text_[pos_])) {
        pos_++;
    }

    const string_view keyword = text_.view(start, pos_ - start);
    if (keyword == "true") {
        return make_unique<json_bool>(true);
    }
    if (keyword == "false") {
        return make_unique<json_bool>(false);
    }
    if (keyword == "null") {
        return make_unique<json_null>();
    }
    NEFORCE_THROW_EXCEPTION(json_exception("Invalid keyword"));
    unreachable();
}

unique_ptr<json_array> json_parser::parse_array() {
    pos_++;
    auto array = make_unique<json_array>();

    skip_space();
    if (current() == ']') {
        pos_++;
        return array;
    }

    while (true) {
        skip_space();
        auto element = parse_value();
        array->add_element(move(element));
        skip_space();

        if (current() == ']') {
            pos_++;
            break;
        } else if (current() == ',') {
            pos_++;
            skip_space();
        } else {
            NEFORCE_THROW_EXCEPTION(json_exception("Expected comma or closing bracket in array"));
        }
    }
    return array;
}

unique_ptr<json_object> json_parser::parse_object() {
    pos_++;
    auto object = make_unique<json_object>();

    skip_space();
    if (current() == '}') {
        pos_++;
        return object;
    }

    while (true) {
        skip_space();
        if (current() != '"') {
            NEFORCE_THROW_EXCEPTION(json_exception("Expected string key in object"));
        }
        const auto key_obj = parse_string();
        string key = key_obj->get_value();

        skip_space();
        if (current() != ':') {
            NEFORCE_THROW_EXCEPTION(json_exception("Expected colon after key in object"));
        }
        pos_++;
        skip_space();

        auto value = parse_value();
        object->add_member(key, move(value));

        skip_space();
        if (current() == '}') {
            pos_++;
            break;
        } else if (current() == ',') {
            pos_++;
            skip_space();
        } else {
            NEFORCE_THROW_EXCEPTION(json_exception("Expected comma or closing brace in object"));
        }
    }

    return object;
}

unique_ptr<json_value> json_parser::parse_value() {
    skip_space();
    if (eof()) {
        NEFORCE_THROW_EXCEPTION(json_exception("Unexpected end of input"));
    }

    const char c = current();
    switch (c) {
        case '{':
            return parse_object();
        case '[':
            return parse_array();
        case '"':
            return parse_string();
        case '-':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            return parse_number();
        case 't':
        case 'f':
        case 'n':
            return parse_keyword();
        default:
            NEFORCE_THROW_EXCEPTION(json_exception("Unexpected character"));
    }
    unreachable();
}


unique_ptr<json_value> json_parser::parse() {
    auto value = parse_value();
    skip_space();
    if (!eof()) {
        NEFORCE_THROW_EXCEPTION(json_exception("Unexpected characters after JSON value"));
    }
    return value;
}

optional<unique_ptr<json_value>> json_parser::try_parse() {
    try {
        return parse();
    } catch (...) {
        return {};
    }
}

NEFORCE_END_NAMESPACE__
