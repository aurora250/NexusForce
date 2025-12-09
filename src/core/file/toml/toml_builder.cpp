#include <MSTL/core/file/toml/toml_builder.hpp>
MSTL_BEGIN_NAMESPACE__

toml_builder::toml_builder() {
    root_ = make_unique<toml_table>();
    contexts_.push(frame(TABLE, root_.get()));
}

toml_builder& toml_builder::key(const string& k) {
    if (contexts_.empty()) {
        throw_exception(toml_exception("Cannot set key outside of a table context"));
    }

    const auto& top = contexts_.top();
    if (top.type != TABLE && top.type != INLINE_TABLE) {
        throw_exception(toml_exception("Cannot set key in non-table context"));
    }

    current_key_ = k;
    return *this;
}

toml_builder& toml_builder::begin_table(const string& table_name) {
    return begin_table(vector<string>{table_name});
}

toml_builder& toml_builder::begin_table(const vector<string>& table_path) {
    if (table_path.empty()) {
        throw_exception(toml_exception("Table path cannot be empty"));
    }

    toml_table* table = get_or_create_table_path(table_path);

    contexts_.push(frame(TABLE, table));
    current_key_.clear();

    return *this;
}

toml_builder& toml_builder::end_table() {
    if (contexts_.empty()) {
        throw_exception(toml_exception("No table to end"));
    }

    const auto& top = contexts_.top();
    if (top.type != TABLE) {
        throw_exception(toml_exception("Current context is not a table"));
    }

    if (contexts_.size() == 1) {
        throw_exception(toml_exception("Cannot end root table"));
    }

    contexts_.pop();
    current_key_.clear();

    return *this;
}

toml_builder& toml_builder::begin_inline_table() {
    if (contexts_.empty()) {
        throw_exception(toml_exception("Cannot create inline table at root"));
    }

    auto inline_table = make_unique<toml_table>(true);
    toml_table* inline_table_ptr = inline_table.get();

    const auto& top = contexts_.top();
    if (top.type == ARRAY) {
        top.array_ptr->add_element(_MSTL move(inline_table));
    } else if (top.type == TABLE || top.type == INLINE_TABLE) {
        if (current_key_.empty()) {
            throw_exception(toml_exception("No key set for inline table"));
        }
        if (top.table_ptr->has_member(current_key_)) {
            throw_exception(toml_exception("Duplicate key: " + current_key_));
        }
        top.table_ptr->add_member(current_key_, _MSTL move(inline_table));
        current_key_.clear();
    }

    contexts_.push(frame(INLINE_TABLE, inline_table_ptr));

    return *this;
}

toml_builder& toml_builder::end_inline_table() {
    if (contexts_.empty()) {
        throw_exception(toml_exception("No inline table to end"));
    }

    const auto& top = contexts_.top();
    if (top.type != INLINE_TABLE) {
        throw_exception(toml_exception("Current context is not an inline table"));
    }

    contexts_.pop();
    current_key_.clear();

    return *this;
}

toml_builder& toml_builder::begin_array() {
    if (contexts_.empty()) {
        throw_exception(toml_exception("Cannot create array at root"));
    }

    auto arr = make_unique<toml_array>();
    toml_array* arr_ptr = arr.get();

    const auto& top = contexts_.top();
    if (top.type == ARRAY) {
        top.array_ptr->add_element(_MSTL move(arr));
    } else if (top.type == TABLE || top.type == INLINE_TABLE) {
        if (current_key_.empty()) {
            throw_exception(toml_exception("No key set for array"));
        }
        if (top.table_ptr->has_member(current_key_)) {
            throw_exception(toml_exception("Duplicate key: " + current_key_));
        }
        top.table_ptr->add_member(current_key_, _MSTL move(arr));
        current_key_.clear();
    }

    contexts_.push(frame(ARRAY, arr_ptr));

    return *this;
}

toml_builder& toml_builder::end_array() {
    if (contexts_.empty()) {
        throw_exception(toml_exception("No array to end"));
    }

    const auto& top = contexts_.top();
    if (top.type != ARRAY) {
        throw_exception(toml_exception("Current context is not an array"));
    }

    contexts_.pop();
    current_key_.clear();

    return *this;
}

toml_builder& toml_builder::begin_array_table(const string& array_table_name) {
    return begin_array_table(vector<string>{array_table_name});
}

toml_builder& toml_builder::begin_array_table(const vector<string>& array_table_path) {
    if (array_table_path.empty()) {
        throw_exception(toml_exception("Array table path cannot be empty"));
    }

    toml_array* arr = get_or_create_array_for_array_table(array_table_path);
    auto new_table = make_unique<toml_table>();
    toml_table* new_table_ptr = new_table.get();
    arr->add_element(_MSTL move(new_table));

    // 压入新的上下文
    contexts_.push(frame(TABLE, new_table_ptr));
    current_key_.clear();

    return *this;
}

toml_builder& toml_builder::end_array_table() {
    return end_table();
}

toml_builder& toml_builder::value_table(_MSTL function<void(toml_builder&)>&& build_func) {
    if (contexts_.empty()) {
        throw_exception(toml_exception("Cannot create table at root using value_table"));
    }

    auto table = make_unique<toml_table>();
    toml_table* table_ptr = table.get();

    const auto& top = contexts_.top();
    if (top.type == ARRAY) {
        top.array_ptr->add_element(_MSTL move(table));
    } else if (top.type == TABLE || top.type == INLINE_TABLE) {
        if (current_key_.empty()) {
            throw_exception(toml_exception("No key set for table"));
        }
        if (top.table_ptr->has_member(current_key_)) {
            throw_exception(toml_exception("Duplicate key: " + current_key_));
        }
        top.table_ptr->add_member(current_key_, _MSTL move(table));
        current_key_.clear();
    }

    contexts_.push(frame(TABLE, table_ptr));
    build_func(*this);
    contexts_.pop();

    return *this;
}

toml_builder& toml_builder::value_inline_table(_MSTL function<void(toml_builder&)>&& build_func) {
    begin_inline_table();
    build_func(*this);
    end_inline_table();
    return *this;
}

toml_builder& toml_builder::value_array(_MSTL function<void(toml_builder&)>&& build_func) {
    begin_array();
    build_func(*this);
    end_array();
    return *this;
}

unique_ptr<toml_table> toml_builder::build() {
    if (contexts_.size() != 1) {
        throw_exception(toml_exception("Unclosed table or array context"));
    }

    return _MSTL move(root_);
}

toml_table* toml_builder::get_or_create_table_path(const vector<string>& path) const {
    toml_table* current = root_.get();

    for (const string& key : path) {
        const toml_value* existing = current->get_member(key);

        if (existing) {
            if (!existing->is_table()) {
                throw_exception(toml_exception(
                    "Key '" + key + "' already exists and is not a table"));
            }
            current = const_cast<toml_table*>(existing->as_table());
        } else {
            auto new_table = make_unique<toml_table>();
            toml_table* new_table_ptr = new_table.get();
            current->add_member(key, _MSTL move(new_table));
            current = new_table_ptr;
        }
    }

    return current;
}

toml_array* toml_builder::get_or_create_array_for_array_table(const vector<string>& path)const {
    if (path.empty()) {
        throw_exception(toml_exception("Array table path cannot be empty"));
    }

    vector<string> parent_path(path.begin(), path.end() - 1);
    const string& array_name = path.back();
    toml_table* parent = parent_path.empty() ? root_.get() : get_or_create_table_path(parent_path);
    const toml_value* existing = parent->get_member(array_name);

    if (existing) {
        if (!existing->is_array()) {
            throw_exception(toml_exception(
                "Key '" + array_name + "' already exists and is not an array"));
        }
        return const_cast<toml_array*>(existing->as_array());
    } else {
        auto new_array = make_unique<toml_array>();
        toml_array* new_array_ptr = new_array.get();
        parent->add_member(array_name, _MSTL move(new_array));
        return new_array_ptr;
    }
}

MSTL_END_NAMESPACE__
