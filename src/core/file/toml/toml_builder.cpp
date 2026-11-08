#include "NeForce/core/file/toml/toml_builder.hpp"
NEFORCE_BEGIN_NAMESPACE__

toml_builder::toml_builder() {
    document_.make_table();
    frames_.push_back(frame{table, &document_.root(), string()});
}

toml_builder& toml_builder::key(string key) {
    if (frames_.empty()) {
        NEFORCE_THROW_EXCEPTION(toml_exception("Cannot set key outside of a table context"));
    }
    frame& top = frames_.back();
    if (top.type != table && top.type != inline_table) {
        NEFORCE_THROW_EXCEPTION(toml_exception("Cannot set key in non-table context"));
    }
    top.pending_key = _NEFORCE move(key);
    return *this;
}

toml_value* toml_builder::open_container(const bool is_table, const bool is_inline) {
    toml_value* node = document_.new_container(is_table, is_inline);
    push_value(*node);
    return node;
}

void toml_builder::close_container(const range_type expected) {
    if (frames_.empty()) {
        NEFORCE_THROW_EXCEPTION(toml_exception("No container to end"));
    }
    const frame& top = frames_.back();
    if (top.type != expected) {
        if (expected == table) {
            NEFORCE_THROW_EXCEPTION(toml_exception("Current context is not a table"));
        }
        if (expected == inline_table) {
            NEFORCE_THROW_EXCEPTION(toml_exception("Current context is not an inline table"));
        }
        NEFORCE_THROW_EXCEPTION(toml_exception("Current context is not an array"));
    }
    if (expected == table && frames_.size() == 1) {
        NEFORCE_THROW_EXCEPTION(toml_exception("Cannot end root table"));
    }
    frames_.pop_back();
}

void toml_builder::push_value(toml_value value) {
    if (frames_.empty()) {
        document_.root() = value;
        return;
    }
    frame& top = frames_.back();
    if (top.type == array) {
        document_.array_append(*top.node, value);
        return;
    }
    if (top.pending_key.empty()) {
        NEFORCE_THROW_EXCEPTION(toml_exception("No key set for value in table"));
    }
    if (top.node->as_table().has_member(top.pending_key)) {
        NEFORCE_THROW_EXCEPTION(toml_exception(("Duplicate key: " + top.pending_key).data()));
    }
    document_.table_append(*top.node, string_view(top.pending_key.data(), top.pending_key.size()), value, true);
    top.pending_key.clear();
}

toml_builder& toml_builder::begin_table(const string& name) { return begin_table(vector<string>{name}); }

toml_builder& toml_builder::begin_table(const vector<string>& path) {
    if (path.empty()) {
        NEFORCE_THROW_EXCEPTION(toml_exception("Table path cannot be empty"));
    }
    toml_value* table_ptr = get_or_create_table_path(path);
    if (!frames_.empty()) {
        frames_.back().pending_key.clear();
    }
    frames_.push_back(frame{table, table_ptr, string()});
    return *this;
}

toml_builder& toml_builder::end_table() {
    close_container(table);
    return *this;
}

toml_builder& toml_builder::begin_inline_table() {
    if (frames_.empty()) {
        NEFORCE_THROW_EXCEPTION(toml_exception("Cannot create inline table at root"));
    }
    toml_value* table_ptr = open_container(true, true);
    frames_.push_back(frame{inline_table, table_ptr, string()});
    return *this;
}

toml_builder& toml_builder::end_inline_table() {
    close_container(inline_table);
    return *this;
}

toml_builder& toml_builder::begin_array() {
    if (frames_.empty()) {
        NEFORCE_THROW_EXCEPTION(toml_exception("Cannot create array at root"));
    }
    toml_value* array_ptr = open_container(false, false);
    frames_.push_back(frame{array, array_ptr, string()});
    return *this;
}

toml_builder& toml_builder::end_array() {
    close_container(array);
    return *this;
}

toml_builder& toml_builder::begin_array_table(const string& name) { return begin_array_table(vector<string>{name}); }

toml_builder& toml_builder::begin_array_table(const vector<string>& path) {
    if (path.empty()) {
        NEFORCE_THROW_EXCEPTION(toml_exception("Array table path cannot be empty"));
    }
    toml_value* array = get_or_create_array_for_array_table(path);
    if (!frames_.empty()) {
        frames_.back().pending_key.clear();
    }
    toml_value* table_ptr = document_.new_container(true);
    document_.array_append(*array, *table_ptr);
    frames_.push_back(frame{table, table_ptr, string()});
    return *this;
}

toml_builder& toml_builder::end_array_table() { return end_table(); }

toml_builder& toml_builder::value_table(const function<void(toml_builder&)>& build_func) {
    if (frames_.empty()) {
        NEFORCE_THROW_EXCEPTION(toml_exception("Cannot create table at root using value_table"));
    }
    toml_value* table_ptr = open_container(true, false);
    frames_.push_back(frame{table, table_ptr, string()});
    build_func(*this);
    frames_.pop_back();
    return *this;
}

toml_builder& toml_builder::value_inline_table(const function<void(toml_builder&)>& build_func) {
    begin_inline_table();
    build_func(*this);
    end_inline_table();
    return *this;
}

toml_builder& toml_builder::value_array(const function<void(toml_builder&)>& build_func) {
    begin_array();
    build_func(*this);
    end_array();
    return *this;
}

toml_document toml_builder::build() {
    if (frames_.size() != 1) {
        NEFORCE_THROW_EXCEPTION(toml_exception("Unclosed table or array context"));
    }
    return _NEFORCE move(document_);
}

toml_value* toml_builder::get_or_create_table_path(const vector<string>& path) {
    toml_value* current = &document_.root();
    for (const string& key: path) {
        const toml_value* existing = current->as_table().get_member(key);
        if (existing != nullptr) {
            if (!existing->is_table()) {
                NEFORCE_THROW_EXCEPTION(toml_exception(("Key '" + key + "' already exists and is not a table").data()));
            }
            current = const_cast<toml_value*>(existing);
            continue;
        }
        toml_value* created = document_.new_container(true);
        document_.table_append(*current, string_view(key.data(), key.size()), *created, true);
        current = created;
    }
    return current;
}

toml_value* toml_builder::get_or_create_array_for_array_table(const vector<string>& path) {
    if (path.empty()) {
        NEFORCE_THROW_EXCEPTION(toml_exception("Array table path cannot be empty"));
    }
    const vector<string> parent_path(path.begin(), path.end() - 1);
    toml_value* parent = parent_path.empty() ? &document_.root() : get_or_create_table_path(parent_path);
    const string& name = path.back();
    const toml_value* existing = parent->as_table().get_member(name);
    if (existing != nullptr) {
        if (!existing->is_array()) {
            NEFORCE_THROW_EXCEPTION(toml_exception(("Key '" + name + "' already exists and is not an array").data()));
        }
        return const_cast<toml_value*>(existing);
    }
    toml_value* created = document_.new_container(false);
    document_.table_append(*parent, string_view(name.data(), name.size()), *created, true);
    return created;
}

NEFORCE_END_NAMESPACE__
