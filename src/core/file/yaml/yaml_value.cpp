#include <NeForce/core/algorithm/sort.hpp>
#include <NeForce/core/file/yaml/yaml_value.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    constexpr uint32_t index_empty = 0xFFFFFFFFU;

    NEFORCE_NODISCARD uint32_t hash_key(const string_view key) noexcept {
        uint32_t hash = 2166136261U;
        hash = (hash ^ static_cast<uint32_t>(key.size())) * 16777619U;
        if (key.size() >= 4) {
            uint32_t first = 0;
            uint32_t last = 0;
            memory_copy(&first, key.data(), sizeof(first));
            memory_copy(&last, key.data() + key.size() - 4, sizeof(last));
            hash = (hash ^ first) * 16777619U;
            hash = (hash ^ last) * 16777619U;
        } else {
            for (const char ch: key) {
                hash = (hash ^ static_cast<uint8_t>(ch)) * 16777619U;
            }
        }
        return hash;
    }

    NEFORCE_NODISCARD bool key_equal(const yaml_member& member, const string_view key) noexcept {
        return member.key_size == key.size() &&
               (key.empty() || memory_compare(member.key_data, key.data(), key.size()) == 0);
    }

    NEFORCE_NODISCARD uint32_t* index_slots(const yaml_container& container) noexcept {
        auto* base = reinterpret_cast<byte_t*>(container.data.members);
        return reinterpret_cast<uint32_t*>(base + static_cast<size_t>(container.capacity) * sizeof(yaml_member));
    }

    void build_index(yaml_container& container) noexcept {
        const uint32_t slots = container.capacity * 2;
        uint32_t* index = index_slots(container);
        memory_set(index, 0xFF, static_cast<size_t>(slots) * sizeof(uint32_t));
        for (uint32_t i = 0; i < container.count; ++i) {
            const yaml_member& member = container.data.members[i];
            uint32_t slot = member.key_hash & (slots - 1);
            for (;;) {
                uint32_t& entry = index[slot];
                if (entry == index_empty) {
                    entry = i;
                    break;
                }
                const yaml_member& existing = container.data.members[entry];
                if (key_equal(existing, member.key())) {
                    entry = i;
                    break;
                }
                slot = (slot + 1) & (slots - 1);
            }
        }
        container.index = index;
        container.index_capacity = slots;
    }

    NEFORCE_NODISCARD const yaml_member* find_member(const yaml_container& container, const string_view key) noexcept {
        auto* mutable_container = const_cast<yaml_container*>(&container);
        if (mutable_container->index == nullptr && mutable_container->count > 16) {
            build_index(*mutable_container);
        }
        if (container.index != nullptr) {
            uint32_t slot = hash_key(key) & (container.index_capacity - 1);
            for (;;) {
                const uint32_t entry = container.index[slot];
                if (entry == index_empty) {
                    return nullptr;
                }
                const yaml_member& member = container.data.members[entry];
                if (key_equal(member, key)) {
                    return &member;
                }
                slot = (slot + 1) & (container.index_capacity - 1);
            }
        }
        for (uint32_t i = container.count; i != 0; --i) {
            const yaml_member& member = container.data.members[i - 1];
            if (key_equal(member, key)) {
                return &member;
            }
        }
        return nullptr;
    }

    struct quote_class {
        uint8_t flags[256];

        constexpr quote_class() noexcept :
        flags{} {
            constexpr string_view special = ":#[]{}&*!|>'\"%@`\n\r\t";
            for (size_t i = 0; special[i] != '\0'; ++i) {
                flags[static_cast<uint8_t>(special[i])] |= 1U;
            }
            constexpr string_view keyword_letters = "truefalsnoyTRUE FALSNOY";
            bool keyword[256] = {};
            for (size_t i = 0; keyword_letters[i] != '\0'; ++i) {
                keyword[static_cast<uint8_t>(keyword_letters[i])] = true;
            }
            keyword[static_cast<uint8_t>('~')] = true;
            for (size_t i = 0; i < 256; ++i) {
                if (!keyword[i]) {
                    flags[i] |= 2U;
                }
            }
        }
    };

    constexpr quote_class quote_class_table{};

    NEFORCE_NODISCARD bool needs_quotes(const string_view str) noexcept {
        if (str.empty()) {
            return true;
        }
        const char first = str[0];
        if ((first >= '0' && first <= '9') || first == '-' || first == '+' || first == '.') {
            return true;
        }
        uint8_t flags = 0;
        for (const char ch: str) {
            flags |= quote_class_table.flags[static_cast<uint8_t>(ch)];
        }
        if ((flags & 1U) != 0) {
            return true;
        }
        if ((flags & 2U) != 0) {
            return false;
        }
        if (first == 'n' || first == 'N') {
            if (str.size() == 2 && str[1] == 'o') {
                return true;
            }
            if (str.size() == 4 && (str[1] == 'u' || str[1] == 'U')) {
                return str == "null" || str == "Null" || str == "NULL";
            }
            return false;
        }
        if (first == 't' || first == 'T') {
            return str == "true" || str == "True" || str == "TRUE";
        }
        if (first == 'f' || first == 'F') {
            return str == "false" || str == "False" || str == "FALSE";
        }
        if (first == 'y' || first == 'Y') {
            return str == "yes";
        }
        if (first == 'o' || first == 'O') {
            return str == "on" || str == "off";
        }
        return first == '~';
    }

    NEFORCE_NODISCARD bool key_less(const yaml_member& lhs, const yaml_member& rhs) noexcept {
        const string_view left = lhs.key();
        const string_view right = rhs.key();
        const size_t common = left.size() < right.size() ? left.size() : right.size();
        const int comparison = common == 0 ? 0 : memory_compare(left.data(), right.data(), common);
        if (comparison != 0) {
            return comparison < 0;
        }
        return left.size() < right.size();
    }

    void append_escaped(string& out, const string_view text) {
        for (const char ch: text) {
            switch (ch) {
                case '\\':
                    out += "\\\\";
                    break;
                case '"':
                    out += "\\\"";
                    break;
                case '\n':
                    out += "\\n";
                    break;
                case '\r':
                    out += "\\r";
                    break;
                case '\t':
                    out += "\\t";
                    break;
                case '\b':
                    out += "\\b";
                    break;
                case '\f':
                    out += "\\f";
                    break;
                default: {
                    if (static_cast<uint8_t>(ch) < 0x20) {
                        out += "\\u00";
                        constexpr char digits[] = "0123456789ABCDEF";
                        out += digits[(static_cast<uint8_t>(ch) >> 4) & 0x0F];
                        out += digits[static_cast<uint8_t>(ch) & 0x0F];
                    } else {
                        out += ch;
                    }
                    break;
                }
            }
        }
    }

    constexpr string_view decimal_pairs = "00010203040506070809"
                                          "10111213141516171819"
                                          "20212223242526272829"
                                          "30313233343536373839"
                                          "40414243444546474849"
                                          "50515253545556575859"
                                          "60616263646566676869"
                                          "70717273747576777879"
                                          "80818283848586878889"
                                          "90919293949596979899";

    NEFORCE_ALWAYS_INLINE_INLINE void append_integer(string& out, const int64_t value) {
        char buffer[21];
        char* position = buffer + sizeof(buffer);
        uint64_t magnitude = value < 0 ? 0ULL - static_cast<uint64_t>(value) : static_cast<uint64_t>(value);
        while (magnitude >= 100) {
            const uint64_t quotient = magnitude / 100;
            const auto remainder = static_cast<size_t>(magnitude - quotient * 100);
            position -= 2;
            position[0] = decimal_pairs[remainder * 2UL];
            position[1] = decimal_pairs[remainder * 2UL + 1UL];
            magnitude = quotient;
        }
        if (magnitude >= 10) {
            position -= 2;
            position[0] = decimal_pairs[magnitude * 2];
            position[1] = decimal_pairs[magnitude * 2 + 1];
        } else {
            *--position = static_cast<char>('0' + static_cast<int>(magnitude));
        }
        if (value < 0) {
            *--position = '-';
        }
        out.append(position, static_cast<size_t>(buffer + sizeof(buffer) - position));
    }

    void append_quoted_key(string& out, const string_view key) {
        if (needs_quotes(key)) {
            out += '"';
            append_escaped(out, key);
            out += '"';
        } else {
            out += key;
        }
    }

    void stringify_value(string& out, const yaml_value& value, size_t indent, bool inline_context);

    NEFORCE_ALWAYS_INLINE_INLINE void append_float(string& out, const double value) {
        const double scaled = value * 10.0;
        if (scaled >= -9000000000000000.0 && scaled <= 9000000000000000.0 &&
            scaled == static_cast<double>(static_cast<int64_t>(scaled))) {
            const auto fixed = static_cast<int64_t>(scaled);
            const bool negative = fixed < 0;
            const uint64_t magnitude = negative ? 0ULL - static_cast<uint64_t>(fixed) : static_cast<uint64_t>(fixed);
            const uint64_t whole = magnitude / 10;
            const uint64_t fraction = magnitude % 10;
            if (negative) {
                out += '-';
            }
            append_integer(out, static_cast<int64_t>(whole));
            if (fraction != 0) {
                out += '.';
                out += static_cast<char>('0' + static_cast<int>(fraction));
            }
            return;
        }
        string text = to_string(value);
        const auto dot = text.find('.');
        if (dot != string::npos) {
            size_t end = text.size();
            while (end > dot + 1 && text[end - 1] == '0') {
                --end;
            }
            if (end == dot + 1) {
                --end;
            }
            text.resize(end);
        }
        out += text;
    }

    NEFORCE_ALWAYS_INLINE_INLINE char* write_integer(char* dest, const int64_t value) {
        char buffer[21];
        char* position = buffer + sizeof(buffer);
        uint64_t magnitude = value < 0 ? 0ULL - static_cast<uint64_t>(value) : static_cast<uint64_t>(value);
        while (magnitude >= 100) {
            const uint64_t quotient = magnitude / 100;
            const auto remainder = static_cast<size_t>(magnitude - quotient * 100);
            position -= 2;
            position[0] = decimal_pairs[remainder * 2UL];
            position[1] = decimal_pairs[remainder * 2UL + 1UL];
            magnitude = quotient;
        }
        if (magnitude >= 10) {
            position -= 2;
            position[0] = decimal_pairs[magnitude * 2];
            position[1] = decimal_pairs[magnitude * 2 + 1];
        } else {
            *--position = static_cast<char>('0' + static_cast<int>(magnitude));
        }
        if (value < 0) {
            *--position = '-';
        }
        const auto length = static_cast<size_t>(buffer + sizeof(buffer) - position);
        memory_copy(dest, position, length);
        return dest + length;
    }

    void append_indent(string& out, const size_t indent) {
        if (indent <= 4) {
            for (size_t i = 0; i < indent; ++i) {
                out += ' ';
            }
            return;
        }
        static constexpr string_view spaces = "                                ";
        if (indent <= 32) {
            out.append(spaces, indent);
        } else {
            out.append(indent, ' ');
        }
    }

    void append_string_body(string& out, const yaml_string str, const size_t indent) {
        const string_view text = str.get_value();
        switch (str.get_style()) {
            case yaml_string::SingleQuoted: {
                out += '\'';
                for (const char ch: text) {
                    out += ch;
                    if (ch == '\'') {
                        out += '\'';
                    }
                }
                out += '\'';
                break;
            }
            case yaml_string::Literal: {
                out += "|\n";
                size_t line_start = 0;
                while (line_start <= text.size()) {
                    const size_t newline = text.find('\n', line_start);
                    const size_t line_end = newline == string_view::npos ? text.size() : newline;
                    append_indent(out, indent);
                    out.append(text.data() + line_start, line_end - line_start);
                    out += '\n';
                    if (newline == string_view::npos) {
                        break;
                    }
                    line_start = newline + 1;
                }
                break;
            }
            case yaml_string::Folded: {
                out += ">\n";
                out.append(indent, ' ');
                out += text;
                out += '\n';
                break;
            }
            case yaml_string::DoubleQuoted:
            case yaml_string::Plain:
            default: {
                if (str.get_style() == yaml_string::DoubleQuoted || needs_quotes(text)) {
                    out += '"';
                    append_escaped(out, text);
                    out += '"';
                } else {
                    out += text;
                }
                break;
            }
        }
    }

    void append_scalar(string& out, const yaml_value& value, const size_t indent) {
        switch (value.kind()) {
            case yaml_kind::null_:
                out += "null";
                break;
            case yaml_kind::boolean:
                out += value.as_boolean().get_value() ? "true" : "false";
                break;
            case yaml_kind::integer:
                append_integer(out, value.as_integer().get_value());
                break;
            case yaml_kind::floating:
                append_float(out, value.as_float().get_value());
                break;
            case yaml_kind::string:
                append_string_body(out, value.as_string(), indent);
                break;
            case yaml_kind::timestamp:
                out += value.as_timestamp().get_string_value();
                break;
            default:
                break;
        }
    }

    NEFORCE_NODISCARD bool has_meta(const yaml_value& value) noexcept { return value.has_meta(); }

    NEFORCE_ALWAYS_INLINE_INLINE void append_value(string& out, const yaml_value& value, const size_t indent,
                                                   const bool inline_context) {
        if (!has_meta(value) && !value.is_sequence() && !value.is_mapping()) {
            append_scalar(out, value, indent);
            return;
        }
        stringify_value(out, value, indent, inline_context);
    }

    void stringify_sequence(string& out, const yaml_sequence& sequence, const size_t indent,
                            const bool inline_context) {
        const yaml_value* elements = sequence.data();
        const size_t count = sequence.size();
        if (sequence.get_style() == yaml_sequence::Flow || inline_context) {
            out += '[';
            size_t index = 0;
            if (count != 0) {
                char chunk[128];
                char* position = chunk;
                for (; index < count; ++index) {
                    const yaml_value& element = elements[index];
                    if (element.has_meta() || !element.is_integer()) {
                        break;
                    }
                    if (index != 0) {
                        *position++ = ',';
                        *position++ = ' ';
                    }
                    position = write_integer(position, element.as_integer().get_value());
                    if (position >= chunk + 100) {
                        out.append(chunk, static_cast<size_t>(position - chunk));
                        position = chunk;
                    }
                }
                if (position != chunk) {
                    out.append(chunk, static_cast<size_t>(position - chunk));
                }
            }
            for (; index < count; ++index) {
                if (index != 0) {
                    out += ',';
                    out += ' ';
                }
                append_value(out, elements[index], indent, true);
            }
            out += ']';
            return;
        }
        for (size_t i = 0; i < count; ++i) {
            out += '\n';
            append_indent(out, indent);
            out += '-';
            out += ' ';
            const yaml_value& element = elements[i];
            if (!element.has_meta() && element.is_integer()) {
                append_integer(out, element.as_integer().get_value());
            } else {
                append_value(out, element, indent + 2, false);
            }
        }
    }

    void stringify_mapping(string& out, const yaml_mapping& mapping, const size_t indent, const bool inline_context) {
        const size_t count = mapping.size();
        const bool flow = mapping.get_style() == yaml_mapping::Flow || inline_context;
        if (count == 0) {
            out += "{}";
            return;
        }
        if (flow) {
            out += '{';
        }
        const yaml_member* members = mapping.data();
        const uint32_t* order = mapping.get_sorted_indices();
        for (size_t position = 0; position < count; ++position) {
            const uint32_t i = order != nullptr ? order[position] : static_cast<uint32_t>(position);
            const yaml_member& member = members[i];
            if (flow) {
                if (position != 0) {
                    out += ',';
                    out += ' ';
                }
            } else {
                if (position != 0) {
                    out += '\n';
                }
                append_indent(out, indent);
            }
            append_quoted_key(out, member.key());
            out += ':';
            out += ' ';
            append_value(out, member.value, indent + 2, flow);
        }
        if (flow) {
            out += '}';
        }
    }

    void stringify_value(string& out, const yaml_value& value, const size_t indent, const bool inline_context) {
        if (value.has_meta()) {
            const string_view anchor = value.get_anchor();
            if (!anchor.empty()) {
                out += '&';
                out += anchor;
                out += ' ';
            }
            const string_view tag = value.get_tag();
            if (!tag.empty()) {
                out += tag;
                out += ' ';
            }
        }
        if (value.is_sequence()) {
            stringify_sequence(out, value.as_sequence(), indent, inline_context);
        } else if (value.is_mapping()) {
            const yaml_mapping mapping = value.as_mapping();
            if (!inline_context && mapping.get_style() != yaml_mapping::Flow) {
                out += '\n';
            }
            stringify_mapping(out, mapping, indent, inline_context);
        } else {
            append_scalar(out, value, indent);
        }
    }
} // namespace


string_view yaml_value::get_anchor() const noexcept {
    const yaml_meta* meta = is_inline_string() ? form_.inlined.meta : form_.extended.meta;
    return meta == nullptr ? string_view() : meta->anchor;
}

string_view yaml_value::get_tag() const noexcept {
    const yaml_meta* meta = is_inline_string() ? form_.inlined.meta : form_.extended.meta;
    return meta == nullptr ? string_view() : meta->tag;
}

yaml_null yaml_value::as_null() const noexcept { return yaml_null(); }

yaml_boolean yaml_value::as_boolean() const noexcept {
    return kind() == yaml_kind::boolean ? yaml_boolean(form_.extended.data.boolean) : yaml_boolean();
}

yaml_integer yaml_value::as_integer() const noexcept {
    return kind() == yaml_kind::integer ? yaml_integer(form_.extended.data.integer) : yaml_integer();
}

yaml_float yaml_value::as_float() const noexcept {
    return kind() == yaml_kind::floating ? yaml_float(form_.extended.data.floating) : yaml_float();
}

yaml_string yaml_value::as_string() const noexcept {
    if (kind() != yaml_kind::string) {
        return yaml_string();
    }
    if (is_inline_string()) {
        return yaml_string(form_.inlined.chars, form_.inlined.size,
                           static_cast<yaml_string::string_style>(form_.inlined.flags & style_mask));
    }
    return yaml_string(form_.extended.data.chars, form_.extended.count,
                       static_cast<yaml_string::string_style>(form_.extended.flags & style_mask));
}

yaml_timestamp yaml_value::as_timestamp() const noexcept {
    if (kind() != yaml_kind::timestamp) {
        return yaml_timestamp();
    }
    return yaml_timestamp(form_.extended.data.timestamp, static_cast<yaml_timestamp::timestamp_type>(
                                                                 (form_.extended.flags & time_mask) >> time_shift));
}

yaml_sequence yaml_value::as_sequence() const noexcept {
    return kind() == yaml_kind::sequence ? yaml_sequence(const_cast<yaml_value*>(this)) : yaml_sequence();
}

yaml_mapping yaml_value::as_mapping() const noexcept {
    return kind() == yaml_kind::mapping ? yaml_mapping(const_cast<yaml_value*>(this)) : yaml_mapping();
}

string yaml_value::to_string() const {
    string result;
    stringify_value(result, *this, 0, true);
    return result;
}

string yaml_value::to_document() const {
    string result;
    size_t hint = 0;
    if (is_sequence() || is_mapping()) {
        const yaml_container* container = form_.extended.data.container;
        hint = container->flags >> 8;
        if (hint != 0) {
            hint += hint / 8 + 64;
        }
    }
    if (hint < 256) {
        if (is_sequence()) {
            hint = as_sequence().size() * 8 + 256;
        } else if (is_mapping()) {
            hint = as_mapping().size() * 24 + 256;
        } else {
            hint = 256;
        }
    }
    result.reserve(hint);
    if (is_sequence() || is_mapping()) {
        stringify_value(result, *this, 0, false);
    } else {
        stringify_value(result, *this, 0, true);
    }
    result += '\n';
    return result;
}

datetime yaml_timestamp::get_value() const noexcept {
    const auto year = static_cast<int32_t>(packed_ & 0xFFFFU);
    const auto month = static_cast<int32_t>((packed_ >> 16) & 0xFU);
    const auto day = static_cast<int32_t>((packed_ >> 20) & 0x1FU);
    const auto hour = static_cast<int32_t>((packed_ >> 25) & 0x1FU);
    const auto minute = static_cast<int32_t>((packed_ >> 30) & 0x3FU);
    const auto second = static_cast<int32_t>((packed_ >> 36) & 0x3FU);
    const auto offset = static_cast<int64_t>((packed_ >> 42) & 0x3FFFFU) - 131071;
    if (type_ == DateTimeTz) {
        return datetime(year, month, day, hour, minute, second, offset);
    }
    return datetime(year, month, day, hour, minute, second);
}

size_t yaml_sequence::size() const noexcept { return node_ == nullptr ? 0 : node_->ext().data.container->count; }

const yaml_value* yaml_sequence::get_element(const size_t index) const noexcept {
    if (node_ == nullptr) {
        return nullptr;
    }
    const yaml_container* container = node_->ext().data.container;
    return index < container->count ? &container->data.elements[index] : nullptr;
}

const yaml_value* yaml_sequence::data() const noexcept {
    return node_ == nullptr ? nullptr : node_->ext().data.container->data.elements;
}

yaml_sequence::sequence_style yaml_sequence::get_style() const noexcept {
    if (node_ == nullptr) {
        return Block;
    }
    return (node_->ext().flags & yaml_value::flow_flag) != 0 ? Flow : Block;
}

size_t yaml_mapping::size() const noexcept { return node_ == nullptr ? 0 : node_->ext().data.container->count; }

const yaml_value* yaml_mapping::get_member(const string_view key) const noexcept {
    if (node_ == nullptr) {
        return nullptr;
    }
    const yaml_member* member = find_member(*node_->ext().data.container, key);
    return member == nullptr ? nullptr : &member->value;
}

const yaml_member* yaml_mapping::data() const noexcept {
    return node_ == nullptr ? nullptr : node_->ext().data.container->data.members;
}

const yaml_member* yaml_mapping::end_data() const noexcept {
    if (node_ == nullptr) {
        return nullptr;
    }
    const yaml_container* container = node_->ext().data.container;
    return container->data.members + container->count;
}

const uint32_t* yaml_mapping::get_sorted_indices() const noexcept {
    if (node_ == nullptr) {
        return nullptr;
    }
    yaml_container* container = node_->ext().data.container;
    if (container->count <= 1) {
        return nullptr;
    }
    if (container->count > container->capacity) {
        return nullptr;
    }
    // Members, hash slots and sort indices share the block allocated by mapping_append().
    auto* order = reinterpret_cast<uint32_t*>(reinterpret_cast<byte_t*>(container->data.members) +
                                              static_cast<size_t>(container->capacity) *
                                                      (sizeof(yaml_member) + 2 * sizeof(uint32_t)));
    if ((container->flags & 1U) == 0) {
        for (uint32_t i = 0; i < container->count; ++i) {
            order[i] = i;
        }
        const yaml_member* members = container->data.members;
        if (container->count <= 8) {
            for (uint32_t i = 1; i < container->count; ++i) {
                const uint32_t current = order[i];
                uint32_t j = i;
                while (j > 0 && key_less(members[current], members[order[j - 1]])) {
                    order[j] = order[j - 1];
                    --j;
                }
                order[j] = current;
            }
        } else {
            sort(order, &order[container->count],
                 [members](const uint32_t lhs, const uint32_t rhs) { return key_less(members[lhs], members[rhs]); });
        }
        container->flags |= 1U;
    }
    return order;
}

yaml_mapping::mapping_style yaml_mapping::get_style() const noexcept {
    if (node_ == nullptr) {
        return Block;
    }
    return (node_->ext().flags & yaml_value::flow_flag) != 0 ? Flow : Block;
}

yaml_value* yaml_document::new_sequence(const bool flow) {
    auto* node = static_cast<yaml_value*>(arena_.allocate(1, sizeof(yaml_value), alignof(yaml_value)));
    node->form_ = yaml_value::storage_form();
    node->ext().tag = yaml_kind::sequence;
    node->ext().flags = flow ? yaml_value::flow_flag : 0;
    auto* container = static_cast<yaml_container*>(arena_.allocate(1, sizeof(yaml_container), alignof(yaml_container)));
    *container = yaml_container();
    node->ext().data.container = container;
    return node;
}

yaml_value* yaml_document::new_mapping(const bool flow) {
    auto* node = static_cast<yaml_value*>(arena_.allocate(1, sizeof(yaml_value), alignof(yaml_value)));
    node->form_ = yaml_value::storage_form();
    node->ext().tag = yaml_kind::mapping;
    node->ext().flags = flow ? yaml_value::flow_flag : 0;
    auto* container = static_cast<yaml_container*>(arena_.allocate(1, sizeof(yaml_container), alignof(yaml_container)));
    *container = yaml_container();
    node->ext().data.container = container;
    return node;
}

void yaml_document::sequence_append(yaml_value& node, const yaml_value value) {
    yaml_container* container = node.ext().data.container;
    if (container->count == container->capacity) {
        const uint32_t capacity = container->capacity == 0 ? 4 : container->capacity * 2;
        auto* elements = static_cast<yaml_value*>(arena_.allocate(capacity, sizeof(yaml_value), alignof(yaml_value)));
        if (container->count != 0) {
            memory_copy(elements, container->data.elements, container->count * sizeof(yaml_value));
        }
        container->data.elements = elements;
        container->capacity = capacity;
    }
    container->data.elements[container->count++] = value;
}

void yaml_document::mapping_append(yaml_value& node, const string_view key, const yaml_value value,
                                   const bool copy_key) {
    yaml_container* container = node.ext().data.container;
    bool grew = false;
    if (container->count == container->capacity || container->data.members == nullptr) {
        grew = true;
        const uint32_t capacity = container->capacity == 0 ? 4 : container->capacity * 2;
        const size_t bytes = static_cast<size_t>(capacity) * sizeof(yaml_member) +
                             static_cast<size_t>(capacity) * 3 * sizeof(uint32_t);
        auto* members = static_cast<yaml_member*>(arena_.allocate(1, bytes, alignof(yaml_member)));
        if (container->count != 0) {
            memory_copy(members, container->data.members, container->count * sizeof(yaml_member));
        }
        container->data.members = members;
        container->capacity = capacity;
    }
    yaml_member& member = container->data.members[container->count];
    if (key.empty()) {
        member.key_data = "";
    } else if (copy_key) {
        auto* data = static_cast<char*>(arena_.allocate(key.size(), 1, 1));
        memory_copy(data, key.data(), key.size());
        member.key_data = data;
    } else {
        member.key_data = key.data();
    }
    member.key_size = static_cast<uint32_t>(key.size());
    member.key_hash = hash_key(key);
    member.value = value;
    ++container->count;
    if (grew) {
        container->index = nullptr;
        container->index_capacity = 0;
    } else if (container->index != nullptr) {
        uint32_t slot = member.key_hash & (container->index_capacity - 1);
        while (container->index[slot] != index_empty) {
            slot = (slot + 1) & (container->index_capacity - 1);
        }
        container->index[slot] = container->count - 1;
    }
    container->flags = 0;
}

void yaml_document::set_meta(yaml_value& value, const string_view anchor, const string_view tag, const bool copy) {
    if (anchor.empty() && tag.empty()) {
        return;
    }
    auto* meta = static_cast<yaml_meta*>(arena_.allocate(1, sizeof(yaml_meta), alignof(yaml_meta)));
    meta->anchor = string_view();
    meta->tag = string_view();
    if (!anchor.empty()) {
        if (copy) {
            auto* data = static_cast<char*>(arena_.allocate(anchor.size(), 1, 1));
            memory_copy(data, anchor.data(), anchor.size());
            meta->anchor = string_view(data, anchor.size());
        } else {
            meta->anchor = anchor;
        }
    }
    if (!tag.empty()) {
        if (copy) {
            auto* data = static_cast<char*>(arena_.allocate(tag.size(), 1, 1));
            memory_copy(data, tag.data(), tag.size());
            meta->tag = string_view(data, tag.size());
        } else {
            meta->tag = tag;
        }
    }
    if (value.is_inline_string()) {
        value.form_.inlined.meta = meta;
    } else {
        value.form_.extended.meta = meta;
    }
}

void yaml_document::set_size_hint(yaml_value& root, const size_t size) noexcept {
    if (!root.is_sequence() && !root.is_mapping()) {
        return;
    }
    yaml_container* container = root.ext().data.container;
    const size_t capped = size > 0xFFFFFF ? 0xFFFFFF : size;
    container->flags = (container->flags & 0xFFU) | (static_cast<uint32_t>(capped) << 8);
}

yaml_value yaml_document::make_timestamp_value(const datetime& value) noexcept {
    uint64_t packed = static_cast<uint64_t>(value.year()) & 0xFFFFU;
    packed |= (static_cast<uint64_t>(value.month()) & 0xFU) << 16;
    packed |= (static_cast<uint64_t>(value.day()) & 0x1FU) << 20;
    packed |= (static_cast<uint64_t>(value.hours()) & 0x1FU) << 25;
    packed |= (static_cast<uint64_t>(value.minutes()) & 0x3FU) << 30;
    packed |= (static_cast<uint64_t>(value.seconds()) & 0x3FU) << 36;
    packed |= (static_cast<uint64_t>(value.offset_seconds() + 131071) & 0x3FFFFU) << 42;

    yaml_value result;
    result.form_ = yaml_value::storage_form();
    result.form_.extended.tag = yaml_kind::timestamp;
    result.form_.extended.flags = static_cast<uint8_t>((value.has_timezone() ? 1 : 0) << 4);
    result.form_.extended.data.timestamp = packed;
    return result;
}

string_view yaml_document::store_string(const string_view text, const bool copy) {
    if (!copy || text.empty()) {
        return text;
    }
    auto* data = static_cast<char*>(arena_.allocate(text.size(), 1, 1));
    memory_copy(data, text.data(), text.size());
    return string_view(data, text.size());
}

void yaml_document::append_missing(yaml_value& target, const yaml_mapping& source) {
    const yaml_mapping destination = target.as_mapping();
    for (const yaml_member& member: source) {
        if (!destination.has_member(member.key())) {
            mapping_append(target, member.key(), member.value, false);
        }
    }
}

NEFORCE_END_NAMESPACE__
