#ifndef MSTL_CORE_FILE_TOML_BUILDER_HPP__
#define MSTL_CORE_FILE_TOML_BUILDER_HPP__
#include "MSTL/core/container/stack.hpp"
#include "MSTL/core/functional/function.hpp"
#include "toml_value.hpp"
MSTL_BEGIN_NAMESPACE__

class MSTL_API toml_builder {
private:
    enum RANGE_TYPE { TABLE, INLINE_TABLE, ARRAY };

    struct frame {
        RANGE_TYPE type = TABLE;
        union {
            toml_table* table_ptr = nullptr;
            toml_array* array_ptr;
        };

        frame() = default;
        frame(const RANGE_TYPE t, toml_table* tbl) : type(t), table_ptr(tbl) {}
        frame(const RANGE_TYPE t, toml_array* arr) : type(t), array_ptr(arr) {}

        frame(const frame&) = default;
        frame& operator=(const frame&) = default;
        frame(frame&&) = default;
        frame& operator=(frame&&) = default;
        ~frame() = default;
    };

    _MSTL stack<frame> contexts_;
    unique_ptr<toml_table> root_;
    string current_key_;

private:
    template <typename T>
    toml_builder& value_impl(unique_ptr<T> v) {
        if (contexts_.empty()) {
            throw_exception(toml_exception("Cannot add value to root (root must be a table)"));
        }

        const auto& top = contexts_.top();
        if (top.type == ARRAY) {
            top.array_ptr->add_element(_MSTL move(v));
        } else if (top.type == TABLE || top.type == INLINE_TABLE) {
            if (current_key_.empty()) {
                throw_exception(toml_exception("No key set for value in table"));
            }
            if (top.table_ptr->has_member(current_key_)) {
                throw_exception(toml_exception("Duplicate key: " + current_key_));
            }
            top.table_ptr->add_member(current_key_, _MSTL move(v));
            current_key_.clear();
        }
        return *this;
    }

    template <typename Iterable, enable_if_t<is_iterable_v<Iterable>, int> = 0>
    toml_builder& value_iterable_dispatch(const Iterable& t) {
        return this->value_iterable_impl(t);
    }

    template <typename Map, enable_if_t<is_maplike_v<Map>, int> = 0>
    toml_builder& value_iterable_impl(const Map& map) {
        begin_inline_table();
        for (const auto& pair : map) {
            this->key(pair.first).value(pair.second);
        }
        end_inline_table();
        return *this;
    }

    template <typename Iterable, enable_if_t<!is_maplike_v<Iterable>, int> = 0>
    toml_builder& value_iterable_impl(const Iterable& t) {
        begin_array();
        for (const auto& element : t) {
            this->value(element);
        }
        end_array();
        return *this;
    }

    toml_table* get_or_create_table_path(const vector<string>& path) const;
    toml_array* get_or_create_array_for_array_table(const vector<string>& path) const;

public:
    toml_builder();
    toml_builder(const toml_builder&) = delete;
    toml_builder& operator =(const toml_builder&) = delete;
    toml_builder(toml_builder&&) = default;
    toml_builder& operator =(toml_builder&&) = default;

    toml_builder& key(const string& k);

    toml_builder& begin_table(const string& table_name);
    toml_builder& begin_table(const vector<string>& table_path);
    toml_builder& end_table();

    toml_builder& begin_inline_table();
    toml_builder& end_inline_table();

    toml_builder& begin_array();
    toml_builder& end_array();

    toml_builder& begin_array_table(const string& array_table_name);
    toml_builder& begin_array_table(const vector<string>& array_table_path);
    toml_builder& end_array_table();

    toml_builder& value(nullptr_t) { return value_impl(make_unique<toml_boolean>(false)); }
    toml_builder& value(const bool v) { return value_impl(make_unique<toml_boolean>(v)); }
    toml_builder& value(const int64_t v) { return value_impl(make_unique<toml_integer>(v)); }
    toml_builder& value(const int v) { return value(static_cast<int64_t>(v)); }
    toml_builder& value(const double v) { return value_impl(make_unique<toml_float>(v)); }
    toml_builder& value(const string& v) {
        return value_impl(make_unique<toml_string>(v, toml_string::Basic));
    }
    toml_builder& value(const char* v) { return value(string(v)); }
    toml_builder& value(const string_view v) { return value(string(v)); }
    toml_builder& value(unique_ptr<toml_value>&& v) { return value_impl(_MSTL move(v)); }

    toml_builder& value_string(const string& v, toml_string::string_type type) {
        return value_impl(make_unique<toml_string>(v, type));
    }

    toml_builder& value_datetime(const string_view v, toml_datetime::datetime_type type) {
        return value_impl(make_unique<toml_datetime>(v, type));
    }

    template <typename Iterable>
    toml_builder& value(const Iterable& t) {
        return this->value_iterable_dispatch(t);
    }

    toml_builder& value_table(_MSTL function<void(toml_builder&)>&& build_func);
    toml_builder& value_inline_table(_MSTL function<void(toml_builder&)>&& build_func);
    toml_builder& value_array(_MSTL function<void(toml_builder&)>&& build_func);

    unique_ptr<toml_table> build();
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_FILE_TOML_BUILDER_HPP__
