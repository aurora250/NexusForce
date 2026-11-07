#include "NeForce/core/file/json/json_value.hpp"
#include "NeForce/core/memory/memory.hpp"
#include "NeForce/core/string/format.hpp"
#include "NeForce/core/string/to_string.hpp"
NEFORCE_BEGIN_NAMESPACE__

namespace {
    constexpr string_view k_digit_pairs =
            "00010203040506070809101112131415161718192021222324252627282930313233343536373839"
            "40414243444546474849505152535455565758596061626364656667686970717273747576777879"
            "8081828384858687888990919293949596979899";

    constexpr size_t k_max_integer_chars = 20;
    constexpr size_t k_number_buffer_size = k_max_integer_chars + 12;

    struct json_output {
        char* data = nullptr;
        size_t size = 0;
        size_t capacity = 0;

        explicit json_output(const size_t initial) {
            capacity = initial < 64 ? 64 : initial;
            data = static_cast<char*>(::operator new(capacity));
        }

        ~json_output() { ::operator delete(data); }

        json_output(const json_output&) = delete;
        json_output& operator=(const json_output&) = delete;

        void ensure(const size_t extra) {
            if (size + extra <= capacity) {
                return;
            }
            size_t next = capacity * 2;
            while (next < size + extra) {
                next *= 2;
            }
            auto* grown = static_cast<char*>(::operator new(next));
            memory_copy(grown, data, size);
            ::operator delete(data);
            data = grown;
            capacity = next;
        }

        void put(const char c) {
            if (size == capacity) {
                ensure(1);
            }
            data[size++] = c;
        }

        void write(const char* source, const size_t count) {
            if (size + count > capacity) {
                ensure(count);
            }
            memory_copy(data + size, source, count);
            size += count;
        }
    };

    constexpr string_view k_hex_digits = "0123456789abcdef";
    constexpr double k_pow10[] = {1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6};

    size_t write_uinteger(char* buffer, uint64_t value) noexcept;

    constexpr bool escape_table_entry(const byte_t c) noexcept { return c < 0x20 || c == '"' || c == '\\'; }

    struct escape_table_holder {
        bool table[256]{};
        constexpr escape_table_holder() noexcept {
            for (size_t i = 0; i < 256; ++i) {
                table[i] = escape_table_entry(static_cast<byte_t>(i));
            }
        }
    };

    constexpr escape_table_holder k_escape_table{};

    size_t write_real_fast(char* buffer, const double value) noexcept {
        if (value < -9007199254740992.0 || value > 9007199254740992.0) {
            return 0;
        }
        const bool negative = value < 0;
        const double magnitude = negative ? -value : value;
        for (size_t k = 0; k < sizeof(k_pow10) / sizeof(k_pow10[0]); ++k) {
            const double scaled = magnitude * k_pow10[k];
            if (!(scaled <= 9007199254740992.0)) {
                break;
            }
            const auto units = static_cast<uint64_t>(scaled + 0.5);
            if (units > (1ULL << 53)) {
                continue;
            }
            if (static_cast<double>(units) / k_pow10[k] != magnitude) {
                continue;
            }
            char digits[k_max_integer_chars];
            const size_t count = write_uinteger(digits, units);
            size_t written = 0;
            if (negative) {
                buffer[written++] = '-';
            }
            if (k == 0) {
                for (size_t i = 0; i < count; ++i) {
                    buffer[written++] = digits[i];
                }
                buffer[written++] = '.';
                buffer[written++] = '0';
                return written;
            }
            if (count <= k) {
                buffer[written++] = '0';
                buffer[written++] = '.';
                for (size_t zeros = 0; zeros < k - count; ++zeros) {
                    buffer[written++] = '0';
                }
                for (size_t i = 0; i < count; ++i) {
                    buffer[written++] = digits[i];
                }
                return written;
            }
            const size_t integer_digits = count - k;
            for (size_t i = 0; i < integer_digits; ++i) {
                buffer[written++] = digits[i];
            }
            buffer[written++] = '.';
            for (size_t i = integer_digits; i < count; ++i) {
                buffer[written++] = digits[i];
            }
            return written;
        }
        return 0;
    }

    size_t write_uinteger(char* buffer, uint64_t value) noexcept {
        char reversed[k_max_integer_chars];
        size_t count = 0;
        while (value >= 100) {
            const size_t pair = static_cast<size_t>(value % 100) * 2;
            value /= 100;
            reversed[count++] = k_digit_pairs[pair + 1];
            reversed[count++] = k_digit_pairs[pair];
        }
        if (value >= 10) {
            const size_t pair = static_cast<size_t>(value) * 2;
            reversed[count++] = k_digit_pairs[pair + 1];
            reversed[count++] = k_digit_pairs[pair];
        } else {
            reversed[count++] = static_cast<char>('0' + static_cast<char>(value));
        }
        for (size_t i = 0; i < count; ++i) {
            buffer[i] = reversed[count - 1 - i];
        }
        return count;
    }

    size_t write_integer(char* buffer, const int64_t value) noexcept {
        if (value < 0) {
            buffer[0] = '-';
            return 1 + write_uinteger(buffer + 1, static_cast<uint64_t>(0) - static_cast<uint64_t>(value));
        }
        return write_uinteger(buffer, static_cast<uint64_t>(value));
    }

    bool needs_escape(const byte_t c) noexcept { return k_escape_table.table[c]; }

    bool word_needs_escape(const uint64_t word) noexcept {
        constexpr uint64_t ones = 0x0101010101010101ULL;
        constexpr uint64_t highs = 0x8080808080808080ULL;
        constexpr uint64_t quotes = 0x2222222222222222ULL;
        constexpr uint64_t slashes = 0x5C5C5C5C5C5C5C5CULL;
        const auto has_zero = [](const uint64_t value) { return (value - ones) & ~value & highs; };
        const uint64_t control = (word - (0x20ULL * ones)) & ~word & highs;
        return (control | has_zero(word ^ quotes) | has_zero(word ^ slashes)) != 0;
    }

    void append_escape(json_output& out, const byte_t c) {
        switch (c) {
            case '"':
                out.write("\\\"", 2);
                break;
            case '\\':
                out.write("\\\\", 2);
                break;
            case '\b':
                out.write("\\b", 2);
                break;
            case '\f':
                out.write("\\f", 2);
                break;
            case '\n':
                out.write("\\n", 2);
                break;
            case '\r':
                out.write("\\r", 2);
                break;
            case '\t':
                out.write("\\t", 2);
                break;
            default:
                const char unicode[6] = {'\\', 'u', '0', '0', k_hex_digits[(c >> 4) & 0x0F], k_hex_digits[c & 0x0F]};
                out.write(unicode, 6);
                break;
        }
    }

    constexpr size_t k_short_string_limit = 16;
    constexpr size_t k_short_string_scratch = 256;

    void build_quoted(char* scratch, size_t& count, const string_view text) {
        scratch[count++] = '"';
        for (size_t i = 0; i < text.size(); ++i) {
            const auto c = static_cast<byte_t>(text[i]);
            if (!needs_escape(c)) {
                scratch[count++] = static_cast<char>(c);
                continue;
            }
            switch (c) {
                case '"':
                    scratch[count++] = '\\';
                    scratch[count++] = '"';
                    break;
                case '\\':
                    scratch[count++] = '\\';
                    scratch[count++] = '\\';
                    break;
                case '\b':
                    scratch[count++] = '\\';
                    scratch[count++] = 'b';
                    break;
                case '\f':
                    scratch[count++] = '\\';
                    scratch[count++] = 'f';
                    break;
                case '\n':
                    scratch[count++] = '\\';
                    scratch[count++] = 'n';
                    break;
                case '\r':
                    scratch[count++] = '\\';
                    scratch[count++] = 'r';
                    break;
                case '\t':
                    scratch[count++] = '\\';
                    scratch[count++] = 't';
                    break;
                default:
                    scratch[count++] = '\\';
                    scratch[count++] = 'u';
                    scratch[count++] = '0';
                    scratch[count++] = '0';
                    scratch[count++] = k_hex_digits[(c >> 4) & 0x0F];
                    scratch[count++] = k_hex_digits[c & 0x0F];
                    break;
            }
        }
        scratch[count++] = '"';
    }

    void append_escaped_short(json_output& out, const string_view text) {
        char scratch[k_short_string_scratch];
        size_t count = 0;
        build_quoted(scratch, count, text);
        out.write(scratch, count);
    }

    void append_escaped(json_output& out, const string_view text) {
        if (text.size() <= k_short_string_limit) {
            append_escaped_short(out, text);
            return;
        }
        out.put('"');
        const char* data = text.data();
        const size_t size = text.size();
        size_t index = 0;
        size_t run = 0;
        while (index < size) {
            while (index + 8 <= size) {
                uint64_t word = 0;
                memory_copy(&word, data + index, sizeof(word));
                if (word_needs_escape(word)) {
                    break;
                }
                index += 8;
            }
            if (index >= size) {
                break;
            }
            if (index + 8 <= size) {
                while (index < size && !needs_escape(static_cast<byte_t>(data[index]))) {
                    ++index;
                }
                if (index >= size) {
                    break;
                }
            }
            const auto c = static_cast<byte_t>(data[index]);
            if (!needs_escape(c)) {
                ++index;
                continue;
            }
            if (index > run) {
                out.write(data + run, index - run);
            }
            append_escape(out, c);
            ++index;
            run = index;
        }
        if (size > run) {
            out.write(data + run, size - run);
        }
        out.put('"');
    }

    void append_indent(json_output& out, const size_t depth, const size_t width) {
        out.put('\n');
        for (size_t i = 0; i < depth * width; ++i) {
            out.put(' ');
        }
    }

    void write_value(json_output& out, const json_value& value, size_t depth, size_t indent);

    void write_object(json_output& out, const json_object& object, const size_t depth, const size_t indent) {
        if (object.empty()) {
            out.write("{}", 2);
            return;
        }
        out.put('{');
        bool first = true;
        for (const json_member& member: object) {
            const string_view key = member.key();
            if (indent == 0 && member.key_inlined()) {
                char head[16];
                size_t count = 0;
                if (!first) {
                    head[count++] = ',';
                }
                head[count++] = '"';
                const size_t length = key.size();
                uint64_t word = 0;
                memory_copy(&word, &member.key_data, sizeof(word));
                const uint64_t mask =
                        length == 8 ? ~static_cast<uint64_t>(0) : ((static_cast<uint64_t>(1) << (length * 8)) - 1);
                const uint64_t probe = (word & mask) | (~mask & 0x7878787878787878ULL);
                if (!word_needs_escape(probe)) {
                    memory_copy(head + count, &word, sizeof(word));
                    count += length;
                } else {
                    count = 0;
                    if (!first) {
                        head[count++] = ',';
                    }
                    build_quoted(head, count, key);
                }
                head[count++] = '"';
                head[count++] = ':';
                out.write(head, count);
            } else if (indent == 0 && key.size() <= k_short_string_limit) {
                char head[k_short_string_scratch];
                size_t count = 0;
                if (!first) {
                    head[count++] = ',';
                }
                build_quoted(head, count, key);
                head[count++] = ':';
                out.write(head, count);
            } else {
                if (!first) {
                    out.put(',');
                }
                if (indent != 0) {
                    append_indent(out, depth + 1, indent);
                }
                append_escaped(out, key);
                out.put(':');
                if (indent != 0) {
                    out.put(' ');
                }
            }
            first = false;
            write_value(out, member.value, depth + 1, indent);
        }
        if (indent != 0) {
            append_indent(out, depth, indent);
        }
        out.put('}');
    }

    void write_array(json_output& out, const json_array& array, const size_t depth, const size_t indent) {
        if (array.empty()) {
            out.write("[]", 2);
            return;
        }
        out.put('[');
        bool first = true;
        for (const json_value& element: array) {
            if (!first) {
                out.put(',');
            }
            first = false;
            if (indent != 0) {
                append_indent(out, depth + 1, indent);
            }
            write_value(out, element, depth + 1, indent);
        }
        if (indent != 0) {
            append_indent(out, depth, indent);
        }
        out.put(']');
    }

    void write_value(json_output& out, const json_value& value, const size_t depth, const size_t indent) {
        char buffer[k_number_buffer_size];
        switch (value.kind()) {
            case json_kind::null_:
                out.write("null", 4);
                break;
            case json_kind::boolean: {
                const bool state = value.as_bool().get_value();
                out.write(state ? "true" : "false", state ? 4 : 5);
                break;
            }
            case json_kind::integer:
                out.write(buffer, write_integer(buffer, value.as_number().as_integer()));
                break;
            case json_kind::uinteger:
                out.write(buffer, write_uinteger(buffer, value.as_number().as_uinteger()));
                break;
            case json_kind::real: {
                const double real = value.as_number().as_real();
                const size_t written = write_real_fast(buffer, real);
                if (written != 0) {
                    out.write(buffer, written);
                } else {
                    string text;
                    inner::append_float_shortest(text, real);
                    out.write(text.data(), text.size());
                }
                break;
            }
            case json_kind::string:
                append_escaped(out, value.as_string().get_value());
                break;
            case json_kind::array:
                write_array(out, value.as_array(), depth, indent);
                break;
            case json_kind::object:
                write_object(out, value.as_object(), depth, indent);
                break;
        }
    }

    size_t reserve_hint(const json_value& value) noexcept {
        constexpr size_t base = 256;
        constexpr size_t per_child = 32;
        constexpr size_t limit = 64 * 1024;
        size_t children = 0;
        if (value.is_object() || value.is_array()) {
            children = value.element_count();
        }
        const size_t estimate = base + children * per_child;
        return estimate > limit ? limit : estimate;
    }

    constexpr size_t k_reserved_limit = 65535;

    bool container_full(const uint32_t count, const uint32_t reserved) noexcept {
        if (reserved != 0) {
            return count >= reserved;
        }
        return count == 0 || (count >= 4 && (count & (count - 1)) == 0);
    }

    size_t container_growth(const uint32_t count) noexcept {
        return count < 4 ? static_cast<size_t>(4) : static_cast<size_t>(count) * 2;
    }

    void set_capacity(uint16_t& reserved, const size_t capacity) noexcept {
        reserved = capacity <= k_reserved_limit ? static_cast<uint16_t>(capacity) : 0;
    }
} // namespace


string json_value::to_string() const {
    json_output out(reserve_hint(*this));
    write_value(out, *this, 0, 0);
    return string(out.data, out.size);
}

string json_value::to_indented_string(const size_t indent) const {
    json_output out(reserve_hint(*this));
    write_value(out, *this, 0, indent);
    return string(out.data, out.size);
}

string_view json_document::store_string(const string_view text) {
    if (text.empty()) {
        return "";
    }
    auto* buffer = static_cast<char*>(arena_.allocate(text.size(), sizeof(char)));
    memory_copy(buffer, text.data(), text.size());
    return string_view(buffer, text.size());
}

json_value json_document::make_string_value(const string_view text) {
    if (text.size() <= json_value::inline_string_capacity) {
        return json_value::make_inline_string_unchecked(text);
    }
    return json_value::make_string(store_string(text));
}

void json_document::object_append(json_value& node, const string_view key, const json_value value,
                                  const bool copy_key) {
    uint32_t& count = node.storage_.ext_.count_;
    if (container_full(count, node.storage_.ext_.reserved_)) {
        const size_t next = container_growth(count);
        json_member*& members = node.storage_.ext_.payload_.members_;
        const size_t old_bytes = static_cast<size_t>(count) * sizeof(json_member);
        const size_t new_bytes = next * sizeof(json_member);
        if (!arena_.try_extend(members, old_bytes, new_bytes)) {
            auto* grown = static_cast<json_member*>(arena_.allocate(next, sizeof(json_member)));
            if (count != 0) {
                memory_copy(grown, members, old_bytes);
            }
            members = grown;
        }
        set_capacity(node.storage_.ext_.reserved_, next);
    }
    json_member& member = node.storage_.ext_.payload_.members_[count];
    if (copy_key && key.size() > json_member::inline_key_capacity) {
        member.assign_key(store_string(key));
    } else {
        member.assign_key(key);
    }
    member.value = value;
    ++count;
}

void json_document::array_append(json_value& node, const json_value value) {
    uint32_t& count = node.storage_.ext_.count_;
    if (container_full(count, node.storage_.ext_.reserved_)) {
        const size_t next = container_growth(count);
        json_value*& elements = node.storage_.ext_.payload_.elements_;
        const size_t old_bytes = static_cast<size_t>(count) * sizeof(json_value);
        const size_t new_bytes = next * sizeof(json_value);
        if (!arena_.try_extend(elements, old_bytes, new_bytes)) {
            auto* grown = static_cast<json_value*>(arena_.allocate(next, sizeof(json_value)));
            if (count != 0) {
                memory_copy(grown, elements, old_bytes);
            }
            elements = grown;
        }
        set_capacity(node.storage_.ext_.reserved_, next);
    }
    node.storage_.ext_.payload_.elements_[count] = value;
    ++count;
}

void json_document::object_reserve(json_value& node, const size_t count) {
    if (count == 0 || count > k_reserved_limit || node.storage_.ext_.count_ != 0 ||
        node.storage_.ext_.payload_.members_ != nullptr) {
        return;
    }
    node.storage_.ext_.payload_.members_ = static_cast<json_member*>(arena_.allocate(count, sizeof(json_member)));
    set_capacity(node.storage_.ext_.reserved_, count);
}

void json_document::array_reserve(json_value& node, const size_t count) {
    if (count == 0 || count > k_reserved_limit || node.storage_.ext_.count_ != 0 ||
        node.storage_.ext_.payload_.elements_ != nullptr) {
        return;
    }
    node.storage_.ext_.payload_.elements_ = static_cast<json_value*>(arena_.allocate(count, sizeof(json_value)));
    set_capacity(node.storage_.ext_.reserved_, count);
}

json_object_mut json_document::make_object() {
    root_ = json_value::make_null();
    root_.storage_.ext_.tag_ = json_kind::object;
    return json_object_mut(this, &root_);
}

json_array_mut json_document::make_array() {
    root_ = json_value::make_null();
    root_.storage_.ext_.tag_ = json_kind::array;
    return json_array_mut(this, &root_);
}

json_value* json_document::new_container(const bool is_object, const size_t reserve) {
    const size_t slots = reserve == 0 || reserve > k_reserved_limit ? 0 : reserve;
    const size_t slot_size = is_object ? sizeof(json_member) : sizeof(json_value);
    auto* memory = static_cast<byte_t*>(arena_.allocate(sizeof(json_value) + slots * slot_size, 1));
    auto* node = reinterpret_cast<json_value*>(memory);
    *node = json_value::make_null();
    node->storage_.ext_.tag_ = is_object ? json_kind::object : json_kind::array;
    if (slots != 0) {
        if (is_object) {
            node->storage_.ext_.payload_.members_ = reinterpret_cast<json_member*>(memory + sizeof(json_value));
        } else {
            node->storage_.ext_.payload_.elements_ = reinterpret_cast<json_value*>(memory + sizeof(json_value));
        }
        node->storage_.ext_.reserved_ = static_cast<uint16_t>(slots);
    }
    return node;
}

json_object_mut json_document::new_object() { return json_object_mut(this, new_container(true)); }

json_array_mut json_document::new_array() { return json_array_mut(this, new_container(false)); }

json_object_mut& json_object_mut::add_member(const string_view key, const json_value value) {
    doc_->object_append(*node_, key, value, true);
    return *this;
}

json_array_mut& json_array_mut::add_element(const json_value value) {
    doc_->array_append(*node_, value);
    return *this;
}

json_value json_value::make_inline_string(const string_view text) {
    if (text.size() > inline_string_capacity) {
        NEFORCE_THROW_EXCEPTION(json_exception("JSON inline string exceeds 14 bytes."));
    }
    return make_inline_string_unchecked(text);
}

json_value json_value::make_inline_string_unchecked(const string_view text) noexcept {
    json_value result;
    result.storage_.ext_.tag_ = json_kind::string;
    result.storage_.ext_.flags_ = static_cast<uint8_t>(flag_inline_string | (text.size() & mask_inline_length));
    if (!text.empty()) {
        memory_copy(result.storage_.inl_.chars_, text.data(), text.size());
    }
    return result;
}

void json_member::assign_key(const string_view text) noexcept {
    const auto length = static_cast<uint32_t>(text.size());
    if (text.size() <= inline_key_capacity) {
        key_size = flag_inline_key | length;
        if (length == 0) {
            key_data = nullptr;
            key_prefix = 0;
            return;
        }
        memory_copy(&key_data, text.data(), text.size());
        memory_copy(&key_prefix, &key_data, key_prefix_size);
        if (length < key_prefix_size) {
            key_prefix &= (1U << (length * 8)) - 1U;
        }
        return;
    }
    key_data = text.data();
    key_size = length;
    memory_copy(&key_prefix, text.data(), key_prefix_size);
}

NEFORCE_END_NAMESPACE__
