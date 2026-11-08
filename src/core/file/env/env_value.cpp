#include "NeForce/core/file/env/env_value.hpp"
#include "NeForce/core/utility/packages.hpp"
NEFORCE_BEGIN_NAMESPACE__

namespace {
    constexpr uint32_t k_reserved_limit = 4096;
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

    NEFORCE_NODISCARD bool container_full(const uint32_t count, const uint32_t reserved) noexcept {
        if (reserved != 0) {
            return count >= reserved;
        }
        return count == 0 || (count >= 4 && (count & (count - 1)) == 0);
    }

    NEFORCE_NODISCARD size_t container_growth(const uint32_t count) noexcept {
        return count < 4 ? static_cast<size_t>(4) : static_cast<size_t>(count) * 2;
    }

    void set_capacity(uint32_t& reserved, const size_t capacity) noexcept {
        reserved = capacity <= k_reserved_limit ? static_cast<uint32_t>(capacity) : 0;
    }

    string escape_quoted(const string_view value, const env_variable::quote_type quote) {
        if (quote == env_variable::None) {
            return string(value.data(), value.size());
        }
        if (quote == env_variable::Single) {
            string result;
            for (const char c: value) {
                if (c == '\'') {
                    result += "'\\''";
                } else {
                    result += c;
                }
            }
            return result;
        }
        return escape(value);
    }

    string variable_to_string(const string_view key, const env_variable variable) {
        string result;
        if (variable.is_exported()) {
            result += "export ";
        }
        if (!key.empty()) {
            result += key;
            result += '=';
        }
        const env_variable::quote_type quote = variable.get_quote_type();
        if (quote == env_variable::None) {
            result += variable.get_value();
            return result;
        }
        result += quote == env_variable::Single ? '\'' : '"';
        result += escape_quoted(variable.get_value(), quote);
        result += quote == env_variable::Single ? '\'' : '"';
        return result;
    }
} // namespace


env_value env_value::make_inline_variable_unchecked(const string_view text, const env_quote quote,
                                                    const bool exported) noexcept {
    env_value result;
    result.storage_.ext_.tag_ = kind::variable;
    result.storage_.ext_.flags_ =
            static_cast<uint8_t>(flag_inline_value | pack_flags(quote, exported) | (text.size() & mask_inline_length));
    for (size_t i = 0; i < text.size(); ++i) {
        result.storage_.inl_.chars_[i] = text[i];
    }
    return result;
}

env_variable env_value::as_variable() const noexcept {
    if (is_inline()) {
        return env_variable(storage_.inl_.chars_, storage_.ext_.flags_ & mask_inline_length, storage_.ext_.flags_);
    }
    return env_variable(storage_.ext_.payload_.chars_, storage_.ext_.count_, storage_.ext_.flags_);
}

string env_value::to_string() const { return variable_to_string(string_view(), as_variable()); }

string env_value::to_document() const { return to_string(); }

env_variable::quote_type env_variable::get_quote_type() const noexcept {
    switch ((flags_ & env_value::mask_quote) >> env_value::quote_shift) {
        case 1:
            return Single;
        case 2:
            return Double;
        default:
            return None;
    }
}

int env_variable::get_int(const int default_value) const noexcept {
    try {
        return integer32::parse(get_value()).value();
    } catch (...) {
        return default_value;
    }
}

int64_t env_variable::get_int64(const int64_t default_value) const noexcept {
    try {
        return integer64::parse(get_value()).value();
    } catch (...) {
        return default_value;
    }
}

double env_variable::get_double(const double default_value) const noexcept {
    try {
        return float64::parse(get_value()).value();
    } catch (...) {
        return default_value;
    }
}

bool env_variable::get_bool(const bool default_value) const noexcept {
    try {
        return boolean::parse(get_value()).value();
    } catch (...) {
        return default_value;
    }
}

string env_variable::to_string() const { return string(data_, size_); }

string env_variable::to_document() const { return to_string(); }

void env_member::assign_key(const string_view text) noexcept {
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

string_view env_document::store_string(const string_view text) {
    if (text.empty()) {
        return "";
    }
    auto* buffer = static_cast<char*>(arena_.allocate(text.size(), sizeof(char), alignof(char)));
    memory_copy(buffer, text.data(), text.size());
    return string_view(buffer, text.size());
}

env_value env_document::make_value(const string_view text, const env_quote quote, const bool exported,
                                   const bool copy) {
    if (text.size() <= env_value::inline_value_capacity) {
        return env_value::make_inline_variable_unchecked(text, quote, exported);
    }
    return env_value::make_variable(copy ? store_string(text) : text, quote, exported);
}

env_member* env_document::find_member(const string_view name) const noexcept {
    if (member_count_ == 0) {
        return nullptr;
    }
    const uint32_t prefix = env_member::prefix_of(name);
    if (index_ != nullptr) {
        const size_t mask = static_cast<size_t>(index_capacity_) - 1;
        size_t slot = static_cast<size_t>(hash_key(name)) & mask;
        while (true) {
            const uint32_t entry = index_[slot];
            if (entry == k_empty_index) {
                return nullptr;
            }
            if (members_[entry].key_equal(prefix, name)) {
                return const_cast<env_member*>(&members_[entry]);
            }
            slot = (slot + 1) & mask;
        }
    }
    for (uint32_t i = 0; i < member_count_; ++i) {
        if (members_[i].key_equal(prefix, name)) {
            return const_cast<env_member*>(&members_[i]);
        }
    }
    return nullptr;
}

void env_document::set_variable_impl(const string_view name, const string_view value, const env_quote quote,
                                     const bool exported, const bool copy) {
    env_value variable = make_value(value, quote, exported, copy);
    const bool should_build = index_ == nullptr && member_count_ + 1 >= k_index_threshold;
    const bool should_grow = index_ != nullptr && (static_cast<uint64_t>(member_count_) + 1) * 4 >
                                                          static_cast<uint64_t>(index_capacity_) * 3;
    if (should_build || should_grow) {
        const size_t capacity = index_ == nullptr ? k_min_index_capacity : static_cast<size_t>(index_capacity_) * 2;
        auto* slots = static_cast<uint32_t*>(arena_.allocate(capacity, sizeof(uint32_t), alignof(uint32_t)));
        for (size_t i = 0; i < capacity; ++i) {
            slots[i] = k_empty_index;
        }
        const size_t mask = capacity - 1;
        for (uint32_t i = 0; i < member_count_; ++i) {
            const string_view member_key = members_[i].key();
            size_t slot = static_cast<size_t>(hash_key(member_key)) & mask;
            while (slots[slot] != k_empty_index) {
                slot = (slot + 1) & mask;
            }
            slots[slot] = i;
        }
        index_ = slots;
        index_capacity_ = static_cast<uint32_t>(capacity);
    }
    if (env_member* existing = find_member(name); existing != nullptr) {
        existing->value = variable;
        return;
    }
    if (container_full(member_count_, member_reserved_)) {
        const size_t next = container_growth(member_count_);
        const size_t old_bytes = static_cast<size_t>(member_count_) * sizeof(env_member);
        const size_t new_bytes = next * sizeof(env_member);
        if (!arena_.try_extend(members_, old_bytes, new_bytes)) {
            auto* grown = static_cast<env_member*>(arena_.allocate(next, sizeof(env_member), alignof(env_member)));
            if (member_count_ != 0) {
                memory_copy(grown, members_, old_bytes);
            }
            members_ = grown;
        }
        set_capacity(member_reserved_, next);
    }
    env_member& member = members_[member_count_];
    member.assign_key(copy && name.size() > env_member::inline_key_capacity ? store_string(name) : name);
    member.value = variable;
    if (index_ != nullptr) {
        const size_t mask = static_cast<size_t>(index_capacity_) - 1;
        size_t slot = static_cast<size_t>(hash_key(name)) & mask;
        while (index_[slot] != k_empty_index) {
            slot = (slot + 1) & mask;
        }
        index_[slot] = member_count_;
    }
    ++member_count_;
}

void env_document::set_variable(const string_view name, const string_view value, const env_variable::quote_type quote,
                                const bool exported) {
    env_quote parsed = env_quote::none;
    switch (quote) {
        case env_variable::Single:
            parsed = env_quote::single;
            break;
        case env_variable::Double:
            parsed = env_quote::double_quote;
            break;
        default:
            break;
    }
    set_variable_impl(name, value, parsed, exported, true);
}

const env_value* env_document::get_variable(const string_view name) const noexcept {
    const env_member* member = find_member(name);
    return member == nullptr ? nullptr : &member->value;
}

void env_document::remove_variable(const string_view name) noexcept {
    const uint32_t prefix = env_member::prefix_of(name);
    for (uint32_t i = 0; i < member_count_; ++i) {
        if (!members_[i].key_equal(prefix, name)) {
            continue;
        }
        for (uint32_t j = i + 1; j < member_count_; ++j) {
            members_[j - 1] = members_[j];
        }
        --member_count_;
        index_ = nullptr;
        index_capacity_ = 0;
        return;
    }
}

void env_document::add_comment_impl(const string_view text, const bool copy) {
    if (container_full(comment_count_, comment_reserved_)) {
        const size_t next = container_growth(comment_count_);
        const size_t old_bytes = static_cast<size_t>(comment_count_) * sizeof(string_view);
        const size_t new_bytes = next * sizeof(string_view);
        if (!arena_.try_extend(comments_, old_bytes, new_bytes)) {
            auto* grown = static_cast<string_view*>(arena_.allocate(next, sizeof(string_view), alignof(string_view)));
            if (comment_count_ != 0) {
                memory_copy(grown, comments_, old_bytes);
            }
            comments_ = grown;
        }
        set_capacity(comment_reserved_, next);
    }
    comments_[comment_count_++] = copy ? store_string(text) : text;
}

void env_document::add_comment(const string_view text) { add_comment_impl(text, true); }

string env_document::to_string() const {
    string result;
    for (const string_view comment: get_comments()) {
        result += "# ";
        result += comment;
        result += '\n';
    }
    if (!get_comments().empty()) {
        result += '\n';
    }
    for (const env_member& member: get_variables()) {
        result += variable_to_string(member.key(), member.variable());
        result += '\n';
    }
    return result;
}

void env_document::reserve(const size_t bytes) { arena_.reserve(bytes); }

NEFORCE_END_NAMESPACE__
