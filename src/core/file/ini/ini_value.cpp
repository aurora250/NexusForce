#include "NeForce/core/file/ini/ini_value.hpp"
#include "NeForce/core/utility/packages.hpp"
NEFORCE_BEGIN_NAMESPACE__

namespace {
    constexpr uint32_t k_reserved_limit = 4096;

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

    NEFORCE_NODISCARD bool section_index_should_build(const ini_section_data& section) noexcept {
        return section.index == nullptr && section.count + 1 >= k_index_threshold;
    }

    NEFORCE_NODISCARD bool section_index_should_grow(const ini_section_data& section) noexcept {
        return section.index != nullptr &&
               (static_cast<uint64_t>(section.count) + 1) * 4 > static_cast<uint64_t>(section.index_capacity) * 3;
    }

    void append_property_line(string& out, const string_view key, const string_view value) {
        out += key;
        out += " = ";
        out += value;
        out += '\n';
    }

    void append_section_body(string& out, const ini_section& section) {
        for (const ini_member& member: section) {
            append_property_line(out, member.key(), member.value.as_property().get_value());
        }
    }
} // namespace


ini_value ini_value::make_inline_property_unchecked(const string_view text) noexcept {
    ini_value result;
    result.storage_.ext_.tag_ = ini_kind::property;
    result.storage_.ext_.flags_ = static_cast<uint8_t>(flag_inline_property | (text.size() & mask_inline_length));
    for (size_t i = 0; i < text.size(); ++i) {
        result.storage_.inl_.chars_[i] = text[i];
    }
    return result;
}

ini_property ini_value::as_property() const noexcept {
    if (is_inline_property()) {
        return ini_property(storage_.inl_.chars_, storage_.ext_.flags_ & mask_inline_length);
    }
    if (is_section()) {
        return ini_property("", 0);
    }
    return ini_property(storage_.ext_.payload_.chars_, storage_.ext_.count_);
}

ini_section ini_value::as_section() const noexcept {
    return ini_section(is_section() ? storage_.ext_.payload_.section_ : nullptr);
}

string ini_value::to_string() const {
    if (is_section()) {
        return as_section().to_string();
    }
    const ini_property property = as_property();
    return string(property.data(), property.size());
}

string ini_value::to_document() const { return to_string(); }

void ini_member::assign_key(const string_view text) noexcept {
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

int ini_property::get_int(const int default_value) const noexcept {
    try {
        return integer32::parse(get_value()).value();
    } catch (...) {
        return default_value;
    }
}

double ini_property::get_double(const double default_value) const noexcept {
    try {
        return float64::parse(get_value()).value();
    } catch (...) {
        return default_value;
    }
}

int64_t ini_section::get_int64(const string_view key, const int64_t default_value) const noexcept {
    const ini_value* prop = get_property(key);
    if (prop == nullptr) {
        return default_value;
    }
    try {
        return integer64::parse(prop->as_property().get_value()).value();
    } catch (...) {
        return default_value;
    }
}

int64_t ini_document::get_int64(const string_view section, const string_view key,
                                const int64_t default_value) const noexcept {
    return get_section(section).get_int64(key, default_value);
}

bool ini_property::get_bool(const bool default_value) const noexcept {
    try {
        return boolean::parse(get_value()).value();
    } catch (...) {
        return default_value;
    }
}

string ini_property::to_string() const { return string(data_, size_); }

string ini_property::to_document() const { return to_string(); }

string ini_section::to_string() const {
    string result;
    if (data_ == nullptr) {
        return result;
    }
    result.reserve(64 + static_cast<size_t>(data_->count) * 32);
    if (!data_->name.empty()) {
        result += '[';
        result += data_->name;
        result += "]\n";
    }
    append_section_body(result, *this);
    return result;
}

string ini_section::to_document() const { return to_string(); }

ini_section ini_section_iterator::operator*() const noexcept { return current_->as_section(); }

ini_section_mut& ini_section_mut::set_property(const string_view key, const string_view value) {
    doc_->append_property(data_, key, value, true);
    return *this;
}

ini_section_mut& ini_section_mut::set_name(const string_view name) {
    data_->name = doc_->store_string(name);
    return *this;
}

const ini_value* ini_section::get_property(const string_view key) const noexcept {
    if (data_ == nullptr || data_->count == 0) {
        return nullptr;
    }
    const uint32_t prefix = ini_member::prefix_of(key);
    if (data_->index != nullptr) {
        const size_t mask = static_cast<size_t>(data_->index_capacity) - 1;
        size_t slot = static_cast<size_t>(hash_key(key)) & mask;
        while (true) {
            const uint32_t entry = data_->index[slot];
            if (entry == k_empty_index) {
                return nullptr;
            }
            const ini_member& member = data_->members[entry];
            if (member.key_equal(prefix, key)) {
                return &member.value;
            }
            slot = (slot + 1) & mask;
        }
    }
    const ini_member* members = data_->members;
    for (uint32_t i = 0; i < data_->count; ++i) {
        if (members[i].key_equal(prefix, key)) {
            return &members[i].value;
        }
    }
    return nullptr;
}

ini_document::ini_document() {
    auto* data = new_section_node(string_view(), true, 0);
    root_ = ini_value();
    root_.storage_.ext_.tag_ = ini_kind::section;
    root_.storage_.ext_.payload_.section_ = data;
    sections_ = nullptr;
    section_count_ = 0;
    section_reserved_ = 0;
}

string_view ini_document::store_string(const string_view text) {
    if (text.empty()) {
        return "";
    }
    auto* buffer = static_cast<char*>(arena_.allocate(text.size(), sizeof(char), alignof(char)));
    memory_copy(buffer, text.data(), text.size());
    return string_view(buffer, text.size());
}

ini_value ini_document::make_property_value(const string_view text, const bool copy) {
    if (text.size() <= ini_value::inline_string_capacity) {
        return ini_value::make_inline_property_unchecked(text);
    }
    return ini_value::make_property(copy ? store_string(text) : text);
}

ini_section_data* ini_document::new_section_node(const string_view name, const bool copy, const size_t slots) {
    const size_t slot_count = slots == 0 || slots > k_reserved_limit ? 0 : slots;
    auto* memory = static_cast<byte_t*>(arena_.allocate(
            sizeof(ini_value) + sizeof(ini_section_data) + slot_count * sizeof(ini_member), 1, alignof(ini_value)));
    auto* data = reinterpret_cast<ini_section_data*>(memory + sizeof(ini_value));
    *data = ini_section_data();
    data->name = copy ? store_string(name) : name;
    if (slot_count != 0) {
        data->members = reinterpret_cast<ini_member*>(memory + sizeof(ini_value) + sizeof(ini_section_data));
        data->reserved = static_cast<uint16_t>(slot_count);
    }
    return data;
}

ini_section_data* ini_document::find_section_data(const string_view name) const noexcept {
    for (uint32_t i = 0; i < section_count_; ++i) {
        ini_section_data* data = sections_[i].storage_.ext_.payload_.section_;
        if (data->name == name) {
            return data;
        }
    }
    return nullptr;
}

ini_section_data* ini_document::reset_section(ini_section_data* data) noexcept {
    data->count = 0;
    data->index = nullptr;
    data->index_capacity = 0;
    return data;
}

void ini_document::append_property(ini_section_data* section, const string_view key, const string_view value,
                                   const bool copy) {
    const uint32_t prefix = ini_member::prefix_of(key);
    if (section_index_should_build(*section) || section_index_should_grow(*section)) {
        const size_t capacity =
                section->index == nullptr ? k_min_index_capacity : static_cast<size_t>(section->index_capacity) * 2;
        auto* slots = static_cast<uint32_t*>(arena_.allocate(capacity, sizeof(uint32_t), alignof(uint32_t)));
        for (size_t i = 0; i < capacity; ++i) {
            slots[i] = k_empty_index;
        }
        const size_t mask = capacity - 1;
        for (uint32_t i = 0; i < section->count; ++i) {
            const string_view member_key = section->members[i].key();
            size_t slot = static_cast<size_t>(hash_key(member_key)) & mask;
            while (slots[slot] != k_empty_index) {
                slot = (slot + 1) & mask;
            }
            slots[slot] = i;
        }
        section->index = slots;
        section->index_capacity = static_cast<uint32_t>(capacity);
    }
    if (section->index != nullptr) {
        const size_t mask = static_cast<size_t>(section->index_capacity) - 1;
        size_t slot = static_cast<size_t>(hash_key(key)) & mask;
        while (true) {
            const uint32_t entry = section->index[slot];
            if (entry == k_empty_index) {
                break;
            }
            if (section->members[entry].key_equal(prefix, key)) {
                section->members[entry].value = make_property_value(value, copy);
                return;
            }
            slot = (slot + 1) & mask;
        }
    } else {
        for (uint32_t i = 0; i < section->count; ++i) {
            if (section->members[i].key_equal(prefix, key)) {
                section->members[i].value = make_property_value(value, copy);
                return;
            }
        }
    }
    if (container_full(section->count, section->reserved)) {
        const size_t next = container_growth(section->count);
        ini_member* members = section->members;
        const size_t old_bytes = static_cast<size_t>(section->count) * sizeof(ini_member);
        const size_t new_bytes = next * sizeof(ini_member);
        if (!arena_.try_extend(members, old_bytes, new_bytes)) {
            auto* grown = static_cast<ini_member*>(arena_.allocate(next, sizeof(ini_member), alignof(ini_member)));
            if (section->count != 0) {
                memory_copy(grown, members, old_bytes);
            }
            members = grown;
            section->members = members;
        }
        set_capacity(section->reserved, next);
    }
    ini_member& member = section->members[section->count];
    if (copy && key.size() > ini_member::inline_key_capacity) {
        member.assign_key(store_string(key));
    } else {
        member.assign_key(key);
    }
    member.value = make_property_value(value, copy);
    if (section->index != nullptr) {
        const size_t mask = static_cast<size_t>(section->index_capacity) - 1;
        size_t slot = static_cast<size_t>(hash_key(key)) & mask;
        while (section->index[slot] != k_empty_index) {
            slot = (slot + 1) & mask;
        }
        section->index[slot] = section->count;
    }
    ++section->count;
}

ini_section_mut ini_document::global_section_mut() noexcept {
    return ini_section_mut(this, root_.storage_.ext_.payload_.section_);
}

ini_section_mut ini_document::new_section(const string_view name, const bool copy) {
    if (name.empty()) {
        return ini_section_mut(this, reset_section(root_.storage_.ext_.payload_.section_));
    }
    ini_section_data* existing = find_section_data(name);
    if (existing != nullptr) {
        return ini_section_mut(this, reset_section(existing));
    }
    ini_section_data* data = new_section_node(name, copy, 0);
    if (section_reserved_ == 0 || section_count_ >= section_reserved_) {
        const size_t next = section_count_ < 4 ? static_cast<size_t>(4) : static_cast<size_t>(section_count_) * 2;
        const size_t old_bytes = static_cast<size_t>(section_count_) * sizeof(ini_value);
        const size_t new_bytes = next * sizeof(ini_value);
        if (!arena_.try_extend(sections_, old_bytes, new_bytes)) {
            auto* grown = static_cast<ini_value*>(arena_.allocate(next, sizeof(ini_value), alignof(ini_value)));
            if (section_count_ != 0) {
                memory_copy(grown, sections_, old_bytes);
            }
            sections_ = grown;
        }
        section_reserved_ = static_cast<uint32_t>(next);
    }
    ini_value node;
    node.storage_.ext_.tag_ = ini_kind::section;
    node.storage_.ext_.payload_.section_ = data;
    sections_[section_count_++] = node;
    return ini_section_mut(this, data);
}

ini_section ini_document::get_section(const string_view name) const noexcept {
    if (name.empty()) {
        return root_.as_section();
    }
    return ini_section(find_section_data(name));
}

string ini_document::to_string() const {
    string result;
    const ini_section global = get_global_section();
    if (!global.empty()) {
        append_section_body(result, global);
        result += '\n';
    }
    for (ini_section section: get_sections()) {
        result += '[';
        result += section.get_name();
        result += "]\n";
        append_section_body(result, section);
        result += '\n';
    }
    return result;
}

void ini_document::reserve(const size_t bytes) { arena_.reserve(bytes); }

NEFORCE_END_NAMESPACE__
