#include <NeForce/core/algorithm/remove.hpp>
#include <NeForce/core/file/toml/toml_parser.hpp>
#include <NeForce/core/utility/hexadecimal.hpp>
#include <NeForce/core/utility/packages.hpp>
NEFORCE_BEGIN_NAMESPACE__

void toml_parser::skip_whitespace() noexcept {
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
            advance_bulk(32);
        } else {
            advance_bulk(static_cast<size_t>(countr_zero(static_cast<unsigned>(~mask))));
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
            advance_bulk(16);
        } else {
            advance_bulk(static_cast<size_t>(countr_zero(static_cast<unsigned>(~mask))));
            return;
        }
    }
#endif
    while (pos_ < len_ && is_space(text_[pos_])) {
        advance();
    }
}

void toml_parser::skip_comment() noexcept {
    if (current() != '#') {
        return;
    }
#ifdef NEFORCE_SIMD_AVX2
    while (pos_ + 32 <= len_) {
        const simd::vec256_t v = ::_mm256_loadu_si256(reinterpret_cast<const simd::vec256_t*>(text_.data() + pos_));
        const int mask = ::_mm256_movemask_epi8(::_mm256_cmpeq_epi8(v, ::_mm256_set1_epi8('\n')));
        if (mask == 0) {
            advance_bulk(32);
        } else {
            advance_bulk(static_cast<size_t>(countr_zero(static_cast<unsigned>(mask))));
            return;
        }
    }
#endif
#ifdef NEFORCE_SIMD_SSE2
    while (pos_ + 16 <= len_) {
        const simd::vec128_t v = ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(text_.data() + pos_));
        const int mask = ::_mm_movemask_epi8(::_mm_cmpeq_epi8(v, ::_mm_set1_epi8('\n')));
        if (mask == 0) {
            advance_bulk(16);
        } else {
            advance_bulk(static_cast<size_t>(countr_zero(static_cast<unsigned>(mask))));
            return;
        }
    }
#endif
    while (!eof() && current() != '\n') {
        advance();
    }
}

void toml_parser::skip_whitespace_and_comments() noexcept {
    while (!eof()) {
        skip_whitespace();
        if (current() == '#') {
            skip_comment();
        } else if (current() == '\n' || current() == '\r') {
            advance();
        } else {
            break;
        }
    }
}

void toml_parser::skip_newlines() noexcept {
    while (!eof() && (current() == '\n' || current() == '\r')) {
        advance();
    }
}

void toml_parser::skip_whitespace_no_newline() noexcept {
#ifdef NEFORCE_SIMD_AVX2
    while (pos_ + 32 <= len_) {
        const simd::vec256_t v = ::_mm256_loadu_si256(reinterpret_cast<const simd::vec256_t*>(text_.data() + pos_));
        simd::vec256_t ws = ::_mm256_cmpeq_epi8(v, ::_mm256_set1_epi8(' '));
        ws = ::_mm256_or_si256(ws, ::_mm256_cmpeq_epi8(v, ::_mm256_set1_epi8('\t')));
        const int mask = ::_mm256_movemask_epi8(ws);
        if (mask == -1) {
            advance_bulk(32);
        } else {
            advance_bulk(static_cast<size_t>(countr_zero(static_cast<unsigned>(~mask))));
            return;
        }
    }
#endif
#ifdef NEFORCE_SIMD_SSE2
    while (pos_ + 16 <= len_) {
        const simd::vec128_t v = ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(text_.data() + pos_));
        simd::vec128_t ws = ::_mm_cmpeq_epi8(v, ::_mm_set1_epi8(' '));
        ws = ::_mm_or_si128(ws, ::_mm_cmpeq_epi8(v, ::_mm_set1_epi8('\t')));
        const int mask = ::_mm_movemask_epi8(ws);
        if (mask == 0xFFFF) {
            advance_bulk(16);
        } else {
            advance_bulk(static_cast<size_t>(countr_zero(static_cast<unsigned>(~mask))));
            return;
        }
    }
#endif
    while (pos_ < len_) {
        const char ch = text_[pos_];
        if (ch == ' ' || ch == '\t') {
            advance();
        } else {
            break;
        }
    }
}

char toml_parser::current() const noexcept {
    if (pos_ < len_) {
        return text_[pos_];
    }
    return '\0';
}

char toml_parser::peek(const size_t offset) const noexcept {
    if (pos_ + offset < len_) {
        return text_[pos_ + offset];
    }
    return '\0';
}

bool toml_parser::eof() const noexcept { return pos_ >= len_; }

void toml_parser::advance() noexcept {
    if (pos_ < len_) {
        if (text_[pos_] == '\n') {
            line_++;
            column_ = 1;
        } else {
            column_++;
        }
        pos_++;
    }
}

void toml_parser::advance_bulk(const size_t count) noexcept {
    size_t newlines = 0;
    size_t last_nl = 0;
    size_t i = 0;

#ifdef NEFORCE_SIMD_AVX2
    while (i + 32 <= count) {
        const simd::vec256_t v = ::_mm256_loadu_si256(reinterpret_cast<const simd::vec256_t*>(text_.data() + pos_ + i));
        const int mask = ::_mm256_movemask_epi8(::_mm256_cmpeq_epi8(v, ::_mm256_set1_epi8('\n')));
        if (mask != 0) {
            newlines += static_cast<size_t>(popcount64(static_cast<unsigned>(mask)));
            last_nl = i + static_cast<size_t>(31 - countl_zero(static_cast<unsigned>(mask)));
        }
        i += 32;
    }
#endif
#ifdef NEFORCE_SIMD_SSE2
    while (i + 16 <= count) {
        const simd::vec128_t v = ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(text_.data() + pos_ + i));
        const int mask = ::_mm_movemask_epi8(::_mm_cmpeq_epi8(v, ::_mm_set1_epi8('\n')));
        if (mask != 0) {
            newlines += static_cast<size_t>(popcount64(static_cast<unsigned>(mask)));
            last_nl = i + static_cast<size_t>(31 - countl_zero(static_cast<unsigned>(mask)));
        }
        i += 16;
    }
#endif
    for (; i < count; ++i) {
        if (text_[pos_ + i] == '\n') {
            newlines++;
            last_nl = i;
        }
    }

    if (newlines > 0) {
        line_ += newlines;
        column_ = count - (last_nl + 1);
    } else {
        column_ += count;
    }
    pos_ += count;
}

bool toml_parser::expect(const char ch) {
    if (current() != ch) {
        throw_parse_error("expected '"_s + ch + "' but got '" + escape(string(1, current())) + "'");
        return false;
    }
    advance();
    return true;
}

bool toml_parser::match(const char ch) noexcept {
    if (current() == ch) {
        advance();
        return true;
    }
    return false;
}

void toml_parser::throw_parse_error(string message) const {
    const string error_msg = "Line " + to_string(line_) + ", Column " + to_string(column_) + ": " + move(message);
    NEFORCE_THROW_EXCEPTION(toml_exception(error_msg.data()));
}

codepoint toml_parser::parse_unicode_escape(const size_t digits) {
    const size_t start_pos = pos_;
    for (size_t i = 0; i < digits; i++) {
        if (eof() || !is_xdigit(current())) {
            throw_parse_error("Invalid unicode escape sequence");
        }
        advance();
    }

    const string_view hex_str = text_.view(start_pos, digits);
    int64_t value = 0;
    try {
        value = hexadecimal(hex_str).value();
    } catch (...) {
        throw_parse_error("Invalid unicode format");
    }
    if (!codepoint::is_valid_codepoint(static_cast<uint32_t>(value))) {
        throw_parse_error("Invalid unicode codepoint");
    }
    return codepoint{static_cast<uint32_t>(value)};
}

unique_ptr<toml_string> toml_parser::parse_string() {
    if (current() == '"') {
        if (peek(1) == '"' && peek(2) == '"') {
            return parse_multiline_basic_string();
        }
        return parse_basic_string();
    }
    if (current() == '\'') {
        if (peek(1) == '\'' && peek(2) == '\'') {
            return parse_multiline_literal_string();
        }
        return parse_literal_string();
    }
    throw_parse_error("Expected string");
    return nullptr;
}

unique_ptr<toml_string> toml_parser::parse_basic_string() {
    expect('"');
    string result;

    while (pos_ < len_) {
#ifdef NEFORCE_SIMD_AVX2
        while (pos_ + 32 <= len_) {
            const simd::vec256_t v = ::_mm256_loadu_si256(reinterpret_cast<const simd::vec256_t*>(text_.data() + pos_));
            const simd::vec256_t is_quote = ::_mm256_cmpeq_epi8(v, ::_mm256_set1_epi8('"'));
            const simd::vec256_t is_bs = ::_mm256_cmpeq_epi8(v, ::_mm256_set1_epi8('\\'));
            const simd::vec256_t is_nl = ::_mm256_cmpeq_epi8(v, ::_mm256_set1_epi8('\n'));
            const simd::vec256_t special = ::_mm256_or_si256(::_mm256_or_si256(is_quote, is_bs), is_nl);
            const int mask = ::_mm256_movemask_epi8(special);
            if (mask == 0) {
                result.append(text_.data() + pos_, 32);
                column_ += 32;
                pos_ += 32;
            } else {
                const int advance_len = countr_zero(static_cast<unsigned>(mask));
                if (advance_len > 0) {
                    result.append(text_.data() + pos_, static_cast<size_t>(advance_len));
                    column_ += advance_len;
                    pos_ += static_cast<size_t>(advance_len);
                }
                break;
            }
        }
#endif
#ifdef NEFORCE_SIMD_SSE2
        while (pos_ + 16 <= len_) {
            const simd::vec128_t v = ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(text_.data() + pos_));
            const simd::vec128_t is_quote = ::_mm_cmpeq_epi8(v, ::_mm_set1_epi8('"'));
            const simd::vec128_t is_bs = ::_mm_cmpeq_epi8(v, ::_mm_set1_epi8('\\'));
            const simd::vec128_t is_nl = ::_mm_cmpeq_epi8(v, ::_mm_set1_epi8('\n'));
            const simd::vec128_t special = ::_mm_or_si128(::_mm_or_si128(is_quote, is_bs), is_nl);
            const int mask = ::_mm_movemask_epi8(special);
            if (mask == 0) {
                result.append(text_.data() + pos_, 16);
                column_ += 16;
                pos_ += 16;
            } else {
                const int advance_len = countr_zero(static_cast<unsigned>(mask));
                if (advance_len > 0) {
                    result.append(text_.data() + pos_, static_cast<size_t>(advance_len));
                    column_ += advance_len;
                    pos_ += static_cast<size_t>(advance_len);
                }
                break;
            }
        }
#endif
        const char c = text_[pos_];
        if (c == '"') {
            advance();
            return make_unique<toml_string>(move(result), toml_string::Basic);
        }
        if (c == '\n') {
            throw_parse_error("Unescaped newline in basic string");
        }
        if (c == '\\') {
            advance();
            if (eof()) {
                throw_parse_error("Unexpected end in string escape");
            }

            switch (current()) {
                case 'b':
                    result += '\b';
                    break;
                case 't':
                    result += '\t';
                    break;
                case 'n':
                    result += '\n';
                    break;
                case 'f':
                    result += '\f';
                    break;
                case 'r':
                    result += '\r';
                    break;
                case '"':
                    result += '"';
                    break;
                case '\\':
                    result += '\\';
                    break;
                case 'u': {
                    advance();
                    if (eof()) {
                        throw_parse_error("Unexpected end after \\u");
                    }
                    const codepoint cp = parse_unicode_escape(4);
                    cp.append_to(result);
                    continue;
                }
                case 'U': {
                    advance();
                    if (eof()) {
                        throw_parse_error("Unexpected end after \\U");
                    }
                    const codepoint cp = parse_unicode_escape(8);
                    cp.append_to(result);
                    continue;
                }
                default:
                    throw_parse_error(R"(Invalid escape sequence: \)"_s + current());
            }
            advance();
            continue;
        }
        result += c;
        advance();
    }

    expect('"');
    return make_unique<toml_string>(move(result), toml_string::Basic);
}

unique_ptr<toml_string> toml_parser::parse_literal_string() {
    expect('\'');
    string result;

    while (pos_ < len_) {
#ifdef NEFORCE_SIMD_AVX2
        while (pos_ + 32 <= len_) {
            const simd::vec256_t v = ::_mm256_loadu_si256(reinterpret_cast<const simd::vec256_t*>(text_.data() + pos_));
            const simd::vec256_t is_quote = ::_mm256_cmpeq_epi8(v, ::_mm256_set1_epi8('\''));
            const simd::vec256_t is_nl = ::_mm256_cmpeq_epi8(v, ::_mm256_set1_epi8('\n'));
            const simd::vec256_t special = ::_mm256_or_si256(is_quote, is_nl);
            const int mask = ::_mm256_movemask_epi8(special);
            if (mask == 0) {
                result.append(text_.data() + pos_, 32);
                column_ += 32;
                pos_ += 32;
            } else {
                const int advance_len = countr_zero(static_cast<unsigned>(mask));
                if (advance_len > 0) {
                    result.append(text_.data() + pos_, static_cast<size_t>(advance_len));
                    column_ += advance_len;
                    pos_ += static_cast<size_t>(advance_len);
                }
                break;
            }
        }
#endif
#ifdef NEFORCE_SIMD_SSE2
        while (pos_ + 16 <= len_) {
            const simd::vec128_t v = ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(text_.data() + pos_));
            const simd::vec128_t is_quote = ::_mm_cmpeq_epi8(v, ::_mm_set1_epi8('\''));
            const simd::vec128_t is_nl = ::_mm_cmpeq_epi8(v, ::_mm_set1_epi8('\n'));
            const simd::vec128_t special = ::_mm_or_si128(is_quote, is_nl);
            const int mask = ::_mm_movemask_epi8(special);
            if (mask == 0) {
                result.append(text_.data() + pos_, 16);
                column_ += 16;
                pos_ += 16;
            } else {
                const int advance_len = countr_zero(static_cast<unsigned>(mask));
                if (advance_len > 0) {
                    result.append(text_.data() + pos_, static_cast<size_t>(advance_len));
                    column_ += advance_len;
                    pos_ += static_cast<size_t>(advance_len);
                }
                break;
            }
        }
#endif
        const char c = text_[pos_];
        if (c == '\'') {
            advance();
            return make_unique<toml_string>(move(result), toml_string::Literal);
        }
        if (c == '\n') {
            throw_parse_error("Unescaped newline in literal string");
        }
        result += c;
        advance();
    }

    expect('\'');
    return make_unique<toml_string>(move(result), toml_string::Literal);
}

unique_ptr<toml_string> toml_parser::parse_multiline_basic_string() {
    expect('"');
    expect('"');
    expect('"');

    if (current() == '\n') {
        advance();
    } else if (current() == '\r' && peek() == '\n') {
        advance();
        advance();
    }

    string result;

    while (pos_ < len_) {
#ifdef NEFORCE_SIMD_AVX2
        while (pos_ + 32 <= len_) {
            const simd::vec256_t v = ::_mm256_loadu_si256(reinterpret_cast<const simd::vec256_t*>(text_.data() + pos_));
            const simd::vec256_t is_quote = ::_mm256_cmpeq_epi8(v, ::_mm256_set1_epi8('"'));
            const simd::vec256_t is_bs = ::_mm256_cmpeq_epi8(v, ::_mm256_set1_epi8('\\'));
            const simd::vec256_t special = ::_mm256_or_si256(is_quote, is_bs);
            const int mask = ::_mm256_movemask_epi8(special);
            if (mask == 0) {
                result.append(text_.data() + pos_, 32);
                advance_bulk(32);
            } else {
                const int advance_len = countr_zero(static_cast<unsigned>(mask));
                if (advance_len > 0) {
                    result.append(text_.data() + pos_, static_cast<size_t>(advance_len));
                    advance_bulk(static_cast<size_t>(advance_len));
                }
                break;
            }
        }
#endif
#ifdef NEFORCE_SIMD_SSE2
        while (pos_ + 16 <= len_) {
            const simd::vec128_t v = ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(text_.data() + pos_));
            const simd::vec128_t is_quote = ::_mm_cmpeq_epi8(v, ::_mm_set1_epi8('"'));
            const simd::vec128_t is_bs = ::_mm_cmpeq_epi8(v, ::_mm_set1_epi8('\\'));
            const simd::vec128_t special = ::_mm_or_si128(is_quote, is_bs);
            const int mask = ::_mm_movemask_epi8(special);
            if (mask == 0) {
                result.append(text_.data() + pos_, 16);
                advance_bulk(16);
            } else {
                const int advance_len = countr_zero(static_cast<unsigned>(mask));
                if (advance_len > 0) {
                    result.append(text_.data() + pos_, static_cast<size_t>(advance_len));
                    advance_bulk(static_cast<size_t>(advance_len));
                }
                break;
            }
        }
#endif
        const char c = text_[pos_];
        if (c == '"') {
            if (peek() == '"' && peek(2) == '"') {
                advance();
                advance();
                advance();
                break;
            }
            result += '"';
            advance();
            continue;
        }

        if (c == '\\') {
            advance();
            if (eof()) {
                throw_parse_error("Unexpected end in string escape");
            }

            if (current() == '\n') {
                advance();
                skip_whitespace();
                continue;
            }
            if (current() == '\r' && peek() == '\n') {
                advance();
                advance();
                skip_whitespace();
                continue;
            }
            if (current() == ' ' || current() == '\t') {
                skip_whitespace();
                if (current() == '\n') {
                    advance();
                    skip_whitespace();
                    continue;
                }
                if (current() == '\r' && peek() == '\n') {
                    advance();
                    advance();
                    skip_whitespace();
                    continue;
                }
            }

            switch (current()) {
                case 'b':
                    result += '\b';
                    break;
                case 't':
                    result += '\t';
                    break;
                case 'n':
                    result += '\n';
                    break;
                case 'f':
                    result += '\f';
                    break;
                case 'r':
                    result += '\r';
                    break;
                case '"':
                    result += '"';
                    break;
                case '\\':
                    result += '\\';
                    break;
                case 'u': {
                    advance();
                    const codepoint cp = parse_unicode_escape(4);
                    cp.append_to(result);
                    continue;
                }
                case 'U': {
                    advance();
                    const codepoint cp = parse_unicode_escape(8);
                    cp.append_to(result);
                    continue;
                }
                default:
                    throw_parse_error(R"(Invalid escape sequence: \)"_s + current());
            }
            advance();
            continue;
        }
        result += c;
        advance();
    }

    return make_unique<toml_string>(move(result), toml_string::MultiBasic);
}

unique_ptr<toml_string> toml_parser::parse_multiline_literal_string() {
    expect('\'');
    expect('\'');
    expect('\'');

    if (current() == '\n') {
        advance();
    } else if (current() == '\r' && peek() == '\n') {
        advance();
        advance();
    }

    string result;

    while (pos_ < len_) {
#ifdef NEFORCE_SIMD_AVX2
        while (pos_ + 32 <= len_) {
            const simd::vec256_t v = ::_mm256_loadu_si256(reinterpret_cast<const simd::vec256_t*>(text_.data() + pos_));
            const int mask = ::_mm256_movemask_epi8(::_mm256_cmpeq_epi8(v, ::_mm256_set1_epi8('\'')));
            if (mask == 0) {
                result.append(text_.data() + pos_, 32);
                advance_bulk(32);
            } else {
                const int advance_len = countr_zero(static_cast<unsigned>(mask));
                if (advance_len > 0) {
                    result.append(text_.data() + pos_, static_cast<size_t>(advance_len));
                    advance_bulk(static_cast<size_t>(advance_len));
                }
                break;
            }
        }
#endif
#ifdef NEFORCE_SIMD_SSE2
        while (pos_ + 16 <= len_) {
            const simd::vec128_t v = ::_mm_loadu_si128(reinterpret_cast<const simd::vec128_t*>(text_.data() + pos_));
            const int mask = ::_mm_movemask_epi8(::_mm_cmpeq_epi8(v, ::_mm_set1_epi8('\'')));
            if (mask == 0) {
                result.append(text_.data() + pos_, 16);
                advance_bulk(16);
            } else {
                const int advance_len = countr_zero(static_cast<unsigned>(mask));
                if (advance_len > 0) {
                    result.append(text_.data() + pos_, static_cast<size_t>(advance_len));
                    advance_bulk(static_cast<size_t>(advance_len));
                }
                break;
            }
        }
#endif
        const char c = text_[pos_];
        if (c == '\'' && peek() == '\'' && peek(2) == '\'') {
            advance();
            advance();
            advance();
            break;
        }
        result += c;
        advance();
    }

    return make_unique<toml_string>(move(result), toml_string::MultiLiteral);
}

unique_ptr<toml_value> toml_parser::parse_number() {
    const size_t start_pos = pos_;
    bool is_float = false;

    const bool has_sign = current() == '+' || current() == '-';
    if (has_sign) {
        advance();
    }

    if (current() == 'i' || current() == 'n') {
        while (!eof() && is_alpha(current())) {
            advance();
        }
        const string_view num_str = text_.view(start_pos, pos_ - start_pos);

        try {
            double val = float64::parse(num_str).value();
            return make_unique<toml_float>(val);
        } catch (...) {
            throw_parse_error("Invalid special float value: "_s + num_str);
        }
    }

    if (current() == '0' && !eof()) {
        const char next = peek();
        if (next == 'x' || next == 'X') {
            if (has_sign) {
                throw_parse_error("Sign not allowed for hexadecimal integer");
            }
            advance();
            advance();
            return parse_integer(16);
        }
        if (next == 'o' || next == 'O') {
            if (has_sign) {
                throw_parse_error("Sign not allowed for octal integer");
            }
            advance();
            advance();
            return parse_integer(8);
        }
        if (next == 'b' || next == 'B') {
            if (has_sign) {
                throw_parse_error("Sign not allowed for binary integer");
            }
            advance();
            advance();
            return parse_integer(2);
        }
    }

    bool has_digits = false;
    while (!eof() && (is_digit(current()) || current() == '_')) {
        if (is_digit(current())) {
            has_digits = true;
        }
        advance();
    }

    if (!has_digits) {
        throw_parse_error("Expected digit in number");
    }

    if (current() == '.') {
        is_float = true;
        advance();
        if (!is_digit(current())) {
            throw_parse_error("Expected digit after decimal point");
        }
        while (!eof() && (is_digit(current()) || current() == '_')) {
            advance();
        }
    }

    if (current() == 'e' || current() == 'E') {
        is_float = true;
        advance();
        if (current() == '+' || current() == '-') {
            advance();
        }
        if (!is_digit(current())) {
            throw_parse_error("Expected digit in exponent");
        }
        while (!eof() && (is_digit(current()) || current() == '_')) {
            advance();
        }
    }

    string num_str = text_.substr(start_pos, pos_ - start_pos);
    num_str.erase(remove(num_str.begin(), num_str.end(), '_'), num_str.end());

    if (!is_float) {
        string_view digits = num_str.view();
        if (!digits.empty() && (digits[0] == '+' || digits[0] == '-')) {
            digits = digits.substr(1);
        }
        if (digits.size() > 1 && digits[0] == '0') {
            throw_parse_error("Leading zeros are not allowed in decimal integers");
        }
    }

    try {
        if (is_float) {
            double val = float64::parse(num_str.view()).value();
            return make_unique<toml_float>(val);
        }
        int64_t val = to_int64(num_str.view(), nullptr, 10);
        return make_unique<toml_integer>(val);
    } catch (...) {
        throw_parse_error("Invalid numeric value: " + num_str);
    }
    return nullptr;
}

unique_ptr<toml_integer> toml_parser::parse_integer(const int base) {
    const size_t start_pos = pos_;
    bool has_digits = false;

    while (!eof()) {
        const char ch = current();
        if (ch == '_') {
            advance();
            continue;
        }

        bool valid = false;
        if (base == 2) {
            valid = (ch == '0' || ch == '1');
        } else if (base == 8) {
            valid = (ch >= '0' && ch <= '7');
        } else if (base == 16) {
            valid = is_xdigit(ch);
        } else {
            valid = is_digit(ch);
        }

        if (valid) {
            has_digits = true;
            advance();
        } else {
            break;
        }
    }

    if (!has_digits) {
        throw_parse_error("Expected digit in integer");
    }

    string num_str = text_.substr(start_pos, pos_ - start_pos);
    num_str.erase(remove(num_str.begin(), num_str.end(), '_'), num_str.end());

    try {
        int64_t val = 0;
        if (base == 2 || base == 8 || base == 16) {
            val = to_int64(num_str.view(), nullptr, base);
        } else {
            val = to_int64(num_str.view(), nullptr, 10);
        }
        return make_unique<toml_integer>(val);
    } catch (...) {
        throw_parse_error("Invalid integer value: " + num_str);
    }
    return nullptr;
}

unique_ptr<toml_boolean> toml_parser::parse_boolean() {
    if (current() == 't' && peek() == 'r' && peek(2) == 'u' && peek(3) == 'e') {
        advance();
        advance();
        advance();
        advance();
        return make_unique<toml_boolean>(true);
    }
    if (current() == 'f' && peek() == 'a' && peek(2) == 'l' && peek(3) == 's' && peek(4) == 'e') {
        advance();
        advance();
        advance();
        advance();
        advance();
        return make_unique<toml_boolean>(false);
    }
    throw_parse_error("Expected boolean value");
    return nullptr;
}

unique_ptr<toml_datetime> toml_parser::parse_datetime() {
    const size_t start_pos = pos_;

    while (!eof() && (is_digit(current()) || current() == '-' || current() == ':' || current() == 'T' ||
                      current() == 'Z' || current() == '+' || current() == '.' || current() == ' ')) {
        advance();
    }

    const string_view dt_str = text_.view(start_pos, pos_ - start_pos);

    const bool has_date_sep = dt_str.contains('-');
    const bool has_time_sep = dt_str.contains(':');
    const bool has_datetime_sep = dt_str.contains('T') || dt_str.contains(' ');

    if (!has_date_sep && !has_time_sep) {
        throw_parse_error("Not a valid datetime format");
    }

    toml_datetime::datetime_type dt_type{toml_datetime::LocalDate};
    if (has_datetime_sep) {
        if (dt_str.contains('Z') || dt_str.contains('+') || dt_str.rfind('-') > 10) {
            dt_type = toml_datetime::OffsetDateTime;
        } else {
            dt_type = toml_datetime::LocalDateTime;
        }
    } else if (has_time_sep) {
        dt_type = toml_datetime::LocalTime;
    }

    return make_unique<toml_datetime>(dt_str, dt_type);
}

unique_ptr<toml_array> toml_parser::parse_array() {
    expect('[');
    auto arr = make_unique<toml_array>();
    skip_whitespace_and_comments();

    while (!eof() && current() != ']') {
        auto element = parse_value();
        arr->add_element(move(element));
        skip_whitespace_and_comments();

        if (current() == ',') {
            advance();
            skip_whitespace_and_comments();
            if (current() == ']') {
                throw_parse_error("Trailing comma is not allowed in array");
            }
        } else if (current() != ']') {
            throw_parse_error("Expected ',' or ']' in array");
        }
    }

    expect(']');
    return arr;
}

unique_ptr<toml_table> toml_parser::parse_inline_table() {
    expect('{');
    auto table = make_unique<toml_table>(true);

    skip_whitespace_no_newline();

    while (!eof() && current() != '}') {
        string key = parse_key();
        skip_whitespace_no_newline();
        expect('=');
        skip_whitespace_no_newline();

        auto value = parse_value();
        if (table->has_member(key)) {
            throw_parse_error("Duplicate key in inline table: " + key);
        }
        table->add_member(key, move(value));

        skip_whitespace_no_newline();

        if (current() == ',') {
            advance();
            skip_whitespace_no_newline();
            if (current() == '}') {
                throw_parse_error("Trailing comma is not allowed in inline table");
            }
        } else if (current() != '}') {
            throw_parse_error("Expected ',' or '}' in inline table");
        }
    }

    expect('}');
    return table;
}

string toml_parser::parse_key() {
    if (current() == '"' || current() == '\'') {
        return parse_quoted_key();
    }
    return parse_bare_key();
}

string toml_parser::parse_bare_key() {
    const size_t start_pos = pos_;
    while (!eof() && (is_alpha_or_digit(current()) || current() == '_' || current() == '-')) {
        advance();
    }
    if (pos_ == start_pos) {
        throw_parse_error("Expected key");
    }
    return text_.substr(start_pos, pos_ - start_pos);
}

string toml_parser::parse_quoted_key() {
    const auto str_value = parse_string();
    return str_value->get_value();
}

vector<string> toml_parser::parse_dotted_key() {
    vector<string> keys;
    keys.push_back(parse_key());

    while (current() == '.') {
        advance();
        skip_whitespace();
        keys.push_back(parse_key());
        skip_whitespace();
    }

    return keys;
}

unique_ptr<toml_value> toml_parser::parse_value() {
    skip_whitespace_and_comments();

    if (eof()) {
        throw_parse_error("Unexpected end of input when expecting a value");
    }

    const char ch = current();

    if (ch == '"' || ch == '\'') {
        return parse_string();
    }
    if (ch == '[') {
        return parse_array();
    }
    if (ch == '{') {
        return parse_inline_table();
    }
    if (ch == 't' || ch == 'f') {
        return parse_boolean();
    }
    if (ch == '+' || ch == '-' || ch == 'i' || ch == 'n') {
        return parse_number();
    }
    if (is_digit(ch)) {
        const size_t saved_pos = pos_;
        const size_t saved_line = line_;
        const size_t saved_column = column_;

        try {
            return parse_datetime();
        } catch (...) {
            pos_ = saved_pos;
            line_ = saved_line;
            column_ = saved_column;
            try {
                return parse_number();
            } catch (...) {
                throw_parse_error("Failed to parse numeric or datetime value");
            }
        }
    }

    throw_parse_error("Unexpected character '"_s + ch + "' when parsing value");
    return nullptr;
}

void toml_parser::parse_key_value() {
    skip_whitespace_and_comments();

    vector<string> key_path = parse_dotted_key();

    skip_whitespace();

    expect('=');
    skip_whitespace();

    unique_ptr<toml_value> val = parse_value();

    skip_whitespace();
    if (current() == '#') {
        skip_comment();
    }

    toml_table* table = ctb_;

    for (size_t i = 0; i + 1 < key_path.size(); i++) {
        const auto& k = key_path[i];
        const toml_value* member = table->get_member(k);
        toml_table* sub_table = nullptr;

        if (member != nullptr && member->is_table()) {
            sub_table = const_cast<toml_table*>(member->as_table());
        } else if (member == nullptr) {
            auto new_table = make_unique<toml_table>();
            sub_table = new_table.get();
            table->add_member(k, move(new_table));
        } else {
            throw_parse_error("Key '" + k + "' already exists but is not a table");
        }

        table = sub_table;
    }

    const string& last_key = key_path.back();

    if (table->has_member(last_key)) {
        throw_parse_error("Duplicate key: " + last_key);
    }
    table->add_member(last_key, move(val));
}

void toml_parser::parse_table_header() {
    expect('[');

    if (current() == '[') {
        throw_parse_error("Use [[table]] for array of tables");
    }

    const vector<string> path = parse_dotted_key();
    skip_whitespace();
    expect(']');

    set_current_table(path);
    is_in_array_table_ = false;
}

void toml_parser::parse_array_table_header() {
    expect('[');
    expect('[');
    vector<string> path = parse_dotted_key();
    skip_whitespace();
    expect(']');
    expect(']');

    if (path.empty()) {
        throw_parse_error("Empty array table path");
    }

    const vector<string> parent_path(path.begin(), path.end() - 1);
    toml_table* parent = parent_path.empty() ? root_.get() : get_or_create_table(parent_path);

    const string& array_key = path.back();
    const toml_value* existing = parent->get_member(array_key);

    toml_array* arr = nullptr;
    if (existing != nullptr) {
        if (!existing->is_array()) {
            throw_parse_error("Key '" + array_key + "' already exists and is not an array");
        }
        arr = const_cast<toml_array*>(existing->as_array());
    } else {
        auto new_array = make_unique<toml_array>();
        arr = new_array.get();
        parent->add_member(array_key, move(new_array));
    }

    auto new_table = make_unique<toml_table>();
    toml_table* new_table_ptr = new_table.get();

    arr->add_element(move(new_table));
    context_stack_.push_back(context{ctb_, ctp_});
    ctb_ = new_table_ptr;
    ctp_ = move(path);
    is_in_array_table_ = true;
}

toml_table* toml_parser::get_or_create_table(const vector<string>& path) const {
    toml_table* tbl = root_.get();
    for (const string& key: path) {
        const auto* member = tbl->get_member(key);
        if (member != nullptr && member->is_table()) {
            tbl = const_cast<toml_table*>(member->as_table());
        } else if (member == nullptr) {
            auto new_table = make_unique<toml_table>();
            toml_table* new_tbl_ptr = new_table.get();
            tbl->add_member(key, move(new_table));
            tbl = new_tbl_ptr;
        } else {
            throw_parse_error("Key '" + key + "' already exists but is not a table");
        }
    }
    return tbl;
}

toml_table* toml_parser::navigate_to_table(const vector<string>& path) const {
    toml_table* tbl = root_.get();
    for (const string& key: path) {
        const toml_value* member = tbl->get_member(key);
        if (member == nullptr || !member->is_table()) {
            return nullptr;
        }
        tbl = const_cast<toml_table*>(member->as_table());
    }
    return tbl;
}

void toml_parser::set_current_table(const vector<string>& path) {
    ctp_ = path;
    ctb_ = get_or_create_table(path);
}

unique_ptr<toml_table> toml_parser::parse() {
    if (pos_ + 3 <= len_ && static_cast<uint8_t>(text_[pos_]) == 0xEF &&
        static_cast<uint8_t>(text_[pos_ + 1]) == 0xBB && static_cast<uint8_t>(text_[pos_ + 2]) == 0xBF) {
        pos_ += 3;
        column_ += 3;
    }

    while (!eof()) {
        skip_whitespace_and_comments();
        if (eof()) {
            break;
        }

        if (current() == '[') {
            if (is_in_array_table_ && !context_stack_.empty()) {
                const auto& prev_context = context_stack_.back();
                ctb_ = prev_context.table;
                ctp_ = prev_context.path;
                context_stack_.pop_back();
                is_in_array_table_ = false;
            }

            if (peek() == '[') {
                parse_array_table_header();
            } else {
                parse_table_header();
            }
        } else {
            if (ctp_.empty()) {
                ctb_ = root_.get();
            }

            parse_key_value();
        }
    }
    return move(root_);
}

optional<unique_ptr<toml_table>> toml_parser::try_parse() {
    try {
        return parse();
    } catch (...) {
        return {};
    }
}

NEFORCE_END_NAMESPACE__
