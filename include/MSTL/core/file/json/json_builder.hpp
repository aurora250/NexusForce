#ifndef MSTL_CORE_FILE_JSON_JSON_BUILDER_HPP__
#define MSTL_CORE_FILE_JSON_JSON_BUILDER_HPP__
#include "MSTL/core/container/stack.hpp"
#include "MSTL/core/functional/function.hpp"
#include "json_value.hpp"
MSTL_BEGIN_NAMESPACE__

class MSTL_API json_builder {
private:
    enum RANGE_TYPE { OBJECT, ARRAY };

    struct frame {
        RANGE_TYPE type = OBJECT;
        union {
            json_object* object_ptr = nullptr;
            json_array* array_ptr;
        };

        frame() = default;
        frame(const RANGE_TYPE t, json_object* obj) : type(t), object_ptr(obj) {}
        frame(const RANGE_TYPE t, json_array* arr) : type(t), array_ptr(arr) {}

        frame(const frame&) = default;
        frame& operator =(const frame&) = default;
        frame(frame&&) = default;
        frame& operator =(frame&&) = default;
        ~frame() = default;
    };

    _MSTL stack<frame> contexts;
    unique_ptr<json_value> root;
    string current_key;

private:
    template <typename T>
    json_builder& value_impl(unique_ptr<T> v) {
        if (contexts.empty()) {
            if (root) {
                throw_exception(json_exception("Multiple root values not allowed"));
            }
            root = _MSTL move(v);
        } else {
            const auto& top = contexts.top();
            if (top.type == ARRAY) {
                top.array_ptr->add_element(_MSTL move(v));
            } else if (top.type == OBJECT) {
                if (current_key.empty()) {
                    throw_exception(json_exception("No key set for value in object"));
                }
                top.object_ptr->add_member(current_key, _MSTL move(v));
                current_key.clear();
            }
        }
        return *this;
    }

    template <typename Iterable, enable_if_t<is_iterable_v<Iterable>, int> = 0>
    json_builder& value_iterable_dispatch(const Iterable& t) {
        return this->value_iterable_impl(t);
    }

    template <typename Map, enable_if_t<is_maplike_v<Map>, int> = 0>
    json_builder& value_iterable_impl(const Map& map) {
        begin_object();
        for (const auto& pair : map) {
            this->key(pair.first).value(pair.second);
        }
        end_object();
        return *this;
    }

    template <typename Iterable, enable_if_t<!is_maplike_v<Iterable>, int> = 0>
    json_builder& value_iterable_impl(const Iterable& t) {
        begin_array();
        for (const auto& element : t) {
            this->value(element);
        }
        end_array();
        return *this;
    }

public:
    json_builder() = default;
    json_builder(const json_builder&) = delete;
    json_builder& operator =(const json_builder&) = delete;
    json_builder(json_builder&&) = default;
    json_builder& operator =(json_builder&&) = default;

    json_builder& begin_object();
    json_builder& begin_array();

    json_builder& end_object();
    json_builder& end_array();

    json_builder& key(const string& k);

    json_builder& value(nullptr_t) { return value_impl(make_unique<json_null>()); }
    json_builder& value(const string& v) { return value_impl(make_unique<json_string>(v)); }
    json_builder& value(const char* v) { return value(string(v)); }
    json_builder& value(const string_view v) { return value(string(v)); }
    json_builder& value(const double v) { return value_impl(make_unique<json_number>(v)); }
    json_builder& value(const int v) { return value_impl(make_unique<json_number>(static_cast<double>(v))); }
    json_builder& value(const bool v) { return value_impl(make_unique<json_bool>(v)); }
    json_builder& value(unique_ptr<json_value>&& v) { return value_impl(_MSTL move(v)); }

    template <typename Iterable>
    json_builder& value(const Iterable& t) {
        return this->value_iterable_dispatch(t);
    }

    json_builder& value_object(_MSTL function<void(json_builder&)>&& build_func);
    json_builder& value_array(_MSTL function<void(json_builder&)>&& build_func);

    unique_ptr<json_value> build();
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_FILE_JSON_JSON_BUILDER_HPP__
