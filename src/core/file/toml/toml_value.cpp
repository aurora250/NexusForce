#include "NeForce/core/file/toml/toml_value.hpp"
#include "NeForce/core/memory/memory.hpp"
#include "NeForce/core/string/to_string.hpp"
NEFORCE_BEGIN_NAMESPACE__

namespace {
    constexpr string_view k_digit_pairs =
            "00010203040506070809101112131415161718192021222324252627282930313233343536373839"
            "40414243444546474849505152535455565758596061626364656667686970717273747576777879"
            "8081828384858687888990919293949596979899";

    constexpr size_t k_max_integer_chars = 20;
    constexpr size_t k_number_buffer_size = k_max_integer_chars + 12;
    constexpr string_view k_hex_digits = "0123456789abcdef";

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

    struct toml_output {
        char* data = nullptr;
        size_t size = 0;
        size_t capacity = 0;
        string float_scratch;

        explicit toml_output(const size_t initial) {
            capacity = initial < 64 ? 64 : initial;
            data = static_cast<char*>(::operator new(capacity));
        }

        ~toml_output() { ::operator delete(data); }

        toml_output(const toml_output&) = delete;
        toml_output& operator=(const toml_output&) = delete;

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

        void write(const string_view text) { write(text.data(), text.size()); }
    };

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

    NEFORCE_NODISCARD bool needs_escape(const byte_t c) noexcept { return k_escape_table.table[c]; }

    void append_escape(toml_output& out, const byte_t c) {
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
            default: {
                const char unicode[6] = {'\\', 'u', '0', '0', k_hex_digits[(c >> 4) & 0x0F], k_hex_digits[c & 0x0F]};
                out.write(unicode, 6);
                break;
            }
        }
    }

    void append_escaped_body(toml_output& out, const string_view text) {
        const char* data = text.data();
        size_t run = 0;
        for (size_t i = 0; i < text.size(); ++i) {
            const auto c = static_cast<byte_t>(data[i]);
            if (!needs_escape(c)) {
                continue;
            }
            if (i > run) {
                out.write(data + run, i - run);
            }
            append_escape(out, c);
            run = i + 1;
        }
        if (text.size() > run) {
            out.write(data + run, text.size() - run);
        }
    }

    void append_escaped(toml_output& out, const string_view text) {
        out.put('"');
        append_escaped_body(out, text);
        out.put('"');
    }

    NEFORCE_NODISCARD bool bare_key(const string_view key) noexcept {
        if (key.empty()) {
            return false;
        }
        for (const char c: key) {
            if (!is_alpha_or_digit(c) && c != '_' && c != '-') {
                return false;
            }
        }
        return true;
    }

    void append_key(toml_output& out, const string_view key) {
        if (bare_key(key)) {
            out.write(key);
            return;
        }
        append_escaped(out, key);
    }

    void write_value(toml_output& out, const toml_value& value, size_t depth);

    void write_string(toml_output& out, const toml_string& text) {
        const string_view content = text.get_value();
        switch (text.get_string_type()) {
            case toml_string::Basic: {
                append_escaped(out, content);
                break;
            }
            case toml_string::Literal: {
                if (!content.contains('\'')) {
                    out.put('\'');
                    out.write(content);
                    out.put('\'');
                } else {
                    append_escaped(out, content);
                }
                break;
            }
            case toml_string::MultiBasic: {
                out.write(R"(""")");
                append_escaped_body(out, content);
                out.write(R"(""")");
                break;
            }
            case toml_string::MultiLiteral: {
                if (!content.contains("'''")) {
                    out.write("'''");
                    out.write(content);
                    out.write("'''");
                } else {
                    append_escaped(out, content);
                }
                break;
            }
            default: {
                append_escaped(out, content);
                break;
            }
        }
    }

    void write_float(toml_output& out, const double value) {
        string& scratch = out.float_scratch;
        scratch.clear();
        inner::append_float_shortest(scratch, value);
        if (!scratch.contains('.') && !scratch.contains('e') && !scratch.contains('E') && !scratch.contains("inf") &&
            !scratch.contains("nan")) {
            scratch += ".0";
        }
        out.write(scratch.data(), scratch.size());
    }

    void write_array(toml_output& out, const toml_array& array, const size_t depth) {
        out.put('[');
        for (size_t i = 0; i < array.size(); ++i) {
            if (i != 0) {
                out.write(", ", 2);
            }
            write_value(out, *array.get_element(i), depth + 1);
        }
        out.put(']');
    }

    void write_inline_table(toml_output& out, const toml_table& table, const size_t depth) {
        if (table.empty()) {
            out.write("{}", 2);
            return;
        }
        out.write("{ ", 2);
        bool first = true;
        for (const toml_member& member: table) {
            if (!first) {
                out.write(", ", 2);
            }
            first = false;
            append_key(out, member.key());
            out.write(" = ", 3);
            write_value(out, member.value, depth + 1);
        }
        out.write(" }", 2);
    }

    void write_value(toml_output& out, const toml_value& value, const size_t depth) {
        char buffer[k_number_buffer_size];
        switch (value.kind()) {
            case toml_kind::null_:
                break;
            case toml_kind::boolean:
                if (value.as_boolean().get_value()) {
                    out.write("true", 4);
                } else {
                    out.write("false", 5);
                }
                break;
            case toml_kind::integer:
                out.write(buffer, write_integer(buffer, value.as_integer().get_value()));
                break;
            case toml_kind::floating:
                write_float(out, value.as_float().get_value());
                break;
            case toml_kind::string:
                write_string(out, value.as_string());
                break;
            case toml_kind::datetime: {
                const string text = value.as_datetime().get_string_value();
                out.write(text.data(), text.size());
                break;
            }
            case toml_kind::array:
                write_array(out, value.as_array(), depth);
                break;
            case toml_kind::table:
                write_inline_table(out, value.as_table(), depth);
                break;
        }
    }

    NEFORCE_NODISCARD bool table_array(const toml_array& array) noexcept {
        if (array.empty()) {
            return false;
        }
        for (size_t i = 0; i < array.size(); ++i) {
            if (!array.get_element(i)->is_table()) {
                return false;
            }
        }
        return true;
    }

    void write_table_document(toml_output& out, const toml_table& table, string& path);

    void write_member_document(toml_output& out, const toml_value& value) {
        if (value.is_table() && !value.as_table().is_inline()) {
            out.write("{}", 2);
            return;
        }
        write_value(out, value, 0);
    }

    void write_table_document(toml_output& out, const toml_table& table, string& path) {
        for (const toml_member& member: table) {
            const toml_value& value = member.value;
            if (value.is_array() && table_array(value.as_array())) {
                continue;
            }
            if (value.is_table() && !value.as_table().is_inline()) {
                continue;
            }
            append_key(out, member.key());
            out.write(" = ", 3);
            write_member_document(out, value);
            out.put('\n');
        }

        for (const toml_member& member: table) {
            const toml_value& value = member.value;
            if ((!value.is_table() || value.as_table().is_inline()) &&
                !(value.is_array() && table_array(value.as_array()))) {
                continue;
            }
            const size_t previous = path.size();
            path += '.';
            path.append(member.key().data(), member.key().size());
            if (value.is_table()) {
                const toml_table nested = value.as_table();
                out.put('\n');
                out.put('[');
                out.write(path.data() + 1, path.size() - 1);
                out.write("]\n", 2);
                write_table_document(out, nested, path);
            } else {
                const toml_array array = value.as_array();
                for (size_t i = 0; i < array.size(); ++i) {
                    out.write("\n[[", 3);
                    out.write(path.data() + 1, path.size() - 1);
                    out.write("]]\n", 3);
                    write_table_document(out, array.get_element(i)->as_table(), path);
                }
            }
            path.resize(previous);
        }
    }

    NEFORCE_NODISCARD size_t reserve_hint(const toml_value& value) noexcept {
        constexpr size_t base = 256;
        constexpr size_t per_child = 48;
        constexpr size_t limit = static_cast<size_t>(256) * 1024;

        size_t children = 0;
        if (value.is_table() || value.is_array()) {
            children = value.element_count();
        }
        const size_t estimate = base + children * per_child;
        return estimate > limit ? limit : estimate;
    }

    constexpr size_t k_reserved_limit = 65535;

    NEFORCE_NODISCARD bool container_full(const uint32_t count, const uint32_t reserved) noexcept {
        if (reserved != 0) {
            return count >= reserved;
        }
        return count == 0 || (count >= 4 && (count & (count - 1)) == 0);
    }

    NEFORCE_NODISCARD size_t container_growth(const uint32_t count) noexcept {
        return count < 4 ? static_cast<size_t>(4) : static_cast<size_t>(count) * 2;
    }

    void set_capacity(uint16_t& reserved, const size_t capacity) noexcept {
        reserved = capacity <= k_reserved_limit ? static_cast<uint16_t>(capacity) : 0;
    }

    constexpr uint32_t k_empty_index = 0xFFFFFFFFU;
    constexpr uint32_t k_index_threshold = 16;
    constexpr uint32_t k_min_index_capacity = 32;

    NEFORCE_NODISCARD uint32_t hash_key(const string_view key) noexcept {
        uint64_t hash = 1469598103934665603ULL;
        for (const char c: key) {
            hash ^= static_cast<unsigned char>(c);
            hash *= 1099511628211ULL;
        }
        return static_cast<uint32_t>(hash ^ (hash >> 32));
    }

    NEFORCE_NODISCARD bool index_should_grow(const toml_container& container) noexcept {
        return container.index != nullptr &&
               (static_cast<size_t>(container.count) + 1) * 4 > static_cast<size_t>(container.index_capacity) * 3;
    }

    NEFORCE_NODISCARD bool index_should_build(const toml_container& container) noexcept {
        return container.index == nullptr && container.count >= k_index_threshold;
    }
} // namespace


datetime toml_datetime::get_value() const noexcept {
    const auto year = static_cast<date::date_type>(raw_ & ((1ULL << toml_value::datetime_year_bits) - 1));
    const auto month = static_cast<date::date_type>((raw_ >> toml_value::datetime_month_shift) & 0x0FULL);
    const auto day = static_cast<date::date_type>((raw_ >> toml_value::datetime_day_shift) & 0x1FULL);
    const auto hour = static_cast<time::time_type>((raw_ >> toml_value::datetime_hour_shift) & 0x1FULL);
    const auto minute = static_cast<time::time_type>((raw_ >> toml_value::datetime_minute_shift) & 0x3FULL);
    const auto second = static_cast<time::time_type>((raw_ >> toml_value::datetime_second_shift) & 0x3FULL);
    const auto offset =
            static_cast<int64_t>((raw_ >> toml_value::datetime_offset_shift) & toml_value::datetime_offset_mask) -
            toml_value::datetime_offset_bias;
    switch (type_) {
        case OffsetDateTime:
            return datetime(year, month, day, hour, minute, second, offset);
        case LocalDateTime:
            return datetime(year, month, day, hour, minute, second);
        case LocalDate:
            return datetime(date(year, month, day));
        case LocalTime:
            return datetime(time(hour, minute, second));
        default:
            unreachable();
    }
}

string toml_datetime::get_string_value() const {
    const datetime value = get_value();
    switch (type_) {
        case OffsetDateTime:
            return value.to_RFC3339();
        case LocalDateTime:
            return value.to_ISO8601();
        case LocalDate:
            return value.date().to_string();
        case LocalTime:
            return value.time().to_string();
        default:
            unreachable();
    }
}

string toml_value::to_string() const {
    toml_output out(reserve_hint(*this));
    write_value(out, *this, 0);
    return string(out.data, out.size);
}

string toml_value::to_document() const {
    toml_output out(reserve_hint(*this));
    if (is_table() && !as_table().is_inline()) {
        string path;
        write_table_document(out, as_table(), path);
    } else {
        write_value(out, *this, 0);
    }
    return string(out.data, out.size);
}

string_view toml_document::store_string(const string_view text) {
    if (text.empty()) {
        return "";
    }
    auto* buffer = static_cast<char*>(arena_.allocate(text.size(), sizeof(char), alignof(char)));
    memory_copy(buffer, text.data(), text.size());
    return string_view(buffer, text.size());
}

toml_value toml_value::make_inline_string_unchecked(const string_view text, const toml_string_type type) noexcept {
    toml_value result;
    result.storage_.ext_.tag_ = toml_kind::string;
    result.storage_.ext_.flags_ =
            static_cast<uint8_t>(flag_inline_string | (static_cast<uint8_t>(type) << string_type_shift) |
                                 (text.size() & mask_inline_length));
    if (!text.empty()) {
        memory_copy(result.storage_.inl_.chars_, text.data(), text.size());
    }
    return result;
}

toml_value toml_document::make_string_value(const string_view text, const toml_string_type type) {
    if (text.size() <= toml_value::inline_string_capacity) {
        return toml_value::make_inline_string_unchecked(text, type);
    }
    return toml_value::make_string(store_string(text), type);
}

toml_value toml_document::make_datetime_value(const string_view text, const toml_datetime_type type) noexcept {
    datetime value;
    switch (type) {
        case toml_offset_datetime: {
            value.try_parse_RFC3339(text);
            break;
        }
        case toml_local_datetime: {
            value.try_parse_ISO8601(text);
            break;
        }
        case toml_local_date: {
            date part{};
            part.try_parse(text);
            value = part;
            break;
        }
        case toml_local_time: {
            time part{};
            part.try_parse(text);
            value = part;
            break;
        }
        default:
            unreachable();
    }
    return toml_value::make_datetime(value, type);
}

void toml_document::table_append(toml_value& node, const string_view key, const toml_value value, const bool copy_key) {
    toml_container* container = node.storage_.ext_.payload_.container_;
    if (index_should_build(*container) || index_should_grow(*container)) {
        const size_t capacity =
                container->index == nullptr ? k_min_index_capacity : static_cast<size_t>(container->index_capacity) * 2;
        auto* slots = static_cast<uint32_t*>(arena_.allocate(capacity, sizeof(uint32_t), alignof(uint32_t)));
        for (size_t i = 0; i < capacity; ++i) {
            slots[i] = k_empty_index;
        }
        const size_t mask = capacity - 1;
        for (uint32_t i = 0; i < container->count; ++i) {
            const string_view member_key = container->data.members[i].key();
            size_t slot = hash_key(member_key) & mask;
            while (slots[slot] != k_empty_index) {
                slot = (slot + 1) & mask;
            }
            slots[slot] = i;
        }
        container->index = slots;
        container->index_capacity = static_cast<uint32_t>(capacity);
    }
    if (container_full(container->count, container->reserved)) {
        const size_t next = container_growth(container->count);
        toml_member* members = container->data.members;
        const size_t old_bytes = static_cast<size_t>(container->count) * sizeof(toml_member);
        const size_t new_bytes = next * sizeof(toml_member);
        if (!arena_.try_extend(members, old_bytes, new_bytes)) {
            auto* grown = static_cast<toml_member*>(arena_.allocate(next, sizeof(toml_member), alignof(toml_member)));
            if (container->count != 0) {
                memory_copy(grown, members, old_bytes);
            }
            members = grown;
            container->data.members = members;
        }
        set_capacity(container->reserved, next);
    }
    toml_member& member = container->data.members[container->count];
    if (copy_key && key.size() > toml_member::inline_key_capacity) {
        member.assign_key(store_string(key));
    } else {
        member.assign_key(key);
    }
    member.value = value;
    if (container->index != nullptr) {
        const size_t mask = static_cast<size_t>(container->index_capacity) - 1;
        size_t slot = hash_key(key) & mask;
        while (container->index[slot] != k_empty_index) {
            slot = (slot + 1) & mask;
        }
        container->index[slot] = container->count;
    }
    ++container->count;
}

void toml_document::array_append(toml_value& node, const toml_value value) {
    toml_container* container = node.storage_.ext_.payload_.container_;
    if (container_full(container->count, container->reserved)) {
        const size_t next = container_growth(container->count);
        toml_value* elements = container->data.elements;
        const size_t old_bytes = static_cast<size_t>(container->count) * sizeof(toml_value);
        const size_t new_bytes = next * sizeof(toml_value);
        if (!arena_.try_extend(elements, old_bytes, new_bytes)) {
            auto* grown = static_cast<toml_value*>(arena_.allocate(next, sizeof(toml_value), alignof(toml_value)));
            if (container->count != 0) {
                memory_copy(grown, elements, old_bytes);
            }
            elements = grown;
            container->data.elements = elements;
        }
        set_capacity(container->reserved, next);
    }
    container->data.elements[container->count] = value;
    ++container->count;
}

void toml_document::table_reserve(toml_value& node, const size_t count) {
    toml_container* container = node.storage_.ext_.payload_.container_;
    if (count == 0 || count > k_reserved_limit || container->count != 0 || container->data.members != nullptr) {
        return;
    }
    container->data.members =
            static_cast<toml_member*>(arena_.allocate(count, sizeof(toml_member), alignof(toml_member)));
    set_capacity(container->reserved, count);
}

void toml_document::array_reserve(toml_value& node, const size_t count) {
    toml_container* container = node.storage_.ext_.payload_.container_;
    if (count == 0 || count > k_reserved_limit || container->count != 0 || container->data.elements != nullptr) {
        return;
    }
    container->data.elements =
            static_cast<toml_value*>(arena_.allocate(count, sizeof(toml_value), alignof(toml_value)));
    set_capacity(container->reserved, count);
}

toml_table_mut toml_document::make_table(const bool is_inline) {
    root_ = *new_container(true, is_inline);
    return toml_table_mut(this, &root_);
}

toml_array_mut toml_document::make_array() {
    root_ = *new_container(false);
    return toml_array_mut(this, &root_);
}

toml_value* toml_document::new_container(const bool is_table, const bool is_inline, const size_t reserve) {
    const size_t slots = reserve == 0 || reserve > k_reserved_limit ? 0 : reserve;
    const size_t slot_size = is_table ? sizeof(toml_member) : sizeof(toml_value);
    auto* memory = static_cast<byte_t*>(
            arena_.allocate(sizeof(toml_value) + sizeof(toml_container) + slots * slot_size, 1, alignof(toml_value)));
    auto* node = reinterpret_cast<toml_value*>(memory);
    auto* container = reinterpret_cast<toml_container*>(memory + sizeof(toml_value));
    *node = toml_value();
    node->storage_.ext_.tag_ = is_table ? toml_kind::table : toml_kind::array;
    node->storage_.ext_.payload_.container_ = container;
    *container = toml_container();
    container->inline_table = is_table && is_inline;
    if (slots != 0) {
        if (is_table) {
            container->data.members =
                    reinterpret_cast<toml_member*>(memory + sizeof(toml_value) + sizeof(toml_container));
        } else {
            container->data.elements =
                    reinterpret_cast<toml_value*>(memory + sizeof(toml_value) + sizeof(toml_container));
        }
        container->reserved = static_cast<uint16_t>(slots);
    }
    return node;
}

toml_table_mut toml_document::new_table(const bool is_inline) {
    return toml_table_mut(this, new_container(true, is_inline));
}

toml_array_mut toml_document::new_array() { return toml_array_mut(this, new_container(false)); }

toml_table_mut& toml_table_mut::add_member(const string_view key, const toml_value value) {
    doc_->table_append(*node_, key, value, true);
    return *this;
}

toml_array_mut& toml_array_mut::add_element(const toml_value value) {
    doc_->array_append(*node_, value);
    return *this;
}

const toml_value* toml_table::get_member(const string_view key) const noexcept {
    const toml_container* container = container_;
    if (container->count == 0) {
        return nullptr;
    }
    const uint32_t prefix = toml_member::prefix_of(key);
    if (container->index != nullptr) {
        const size_t mask = static_cast<size_t>(container->index_capacity) - 1;
        size_t slot = hash_key(key) & mask;
        while (true) {
            const uint32_t entry = container->index[slot];
            if (entry == k_empty_index) {
                return nullptr;
            }
            if (container->data.members[entry].key_equal(prefix, key)) {
                return &container->data.members[entry].value;
            }
            slot = (slot + 1) & mask;
        }
    }
    for (uint32_t i = 0; i < container->count; ++i) {
        if (container->data.members[i].key_equal(prefix, key)) {
            return &container->data.members[i].value;
        }
    }
    return nullptr;
}

void toml_member::assign_key(const string_view text) noexcept {
    const auto length = static_cast<uint32_t>(text.size());
    if (text.size() <= inline_key_capacity) {
        key_size = flag_inline_key | length;
        if (length == 0) {
            key_data = nullptr;
            key_prefix = 0;
            return;
        }
        memory_copy(static_cast<void*>(&key_data), text.data(), text.size());
        memory_copy(&key_prefix, static_cast<const void*>(&key_data), key_prefix_size);
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
