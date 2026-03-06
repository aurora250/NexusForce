#include <NeForce/core/file/json/json_builder.hpp>
NEFORCE_BEGIN_NAMESPACE__

json_builder& json_builder::begin_object() {
    auto new_object = make_unique<json_object>();
    json_object* obj_ptr = new_object.get();

    if (contexts_.empty()) {
        if (root_) {
            throw_exception(json_exception("Root value already set"));
        }
        root_ = _NEFORCE move(new_object);
    } else {
        const auto & current = contexts_.top();
        if (current.type == array) {
            current.array_ptr->add_element(_NEFORCE move(new_object));
        } else if (current.type == object) {
            if (current_key_.empty()) {
                throw_exception(json_exception("No key set for object value"));
            }
            current.object_ptr->add_member(current_key_, _NEFORCE move(new_object));
            current_key_.clear();
        }
    }

    contexts_.push(frame(object, obj_ptr));
    return *this;
}

json_builder& json_builder::begin_array() {
    auto new_array = make_unique<json_array>();
    json_array* arr_ptr = new_array.get();

    if (contexts_.empty()) {
        if (root_) {
            throw_exception(json_exception("Root value already set"));
        }
        root_ = _NEFORCE move(new_array);
    } else {
        const auto & current = contexts_.top();
        if (current.type == array) {
            current.array_ptr->add_element(_NEFORCE move(new_array));
        } else if (current.type == object) {
            if (current_key_.empty()) {
                throw_exception(json_exception("No key set for array value"));
            }
            current.object_ptr->add_member(current_key_, _NEFORCE move(new_array));
            current_key_.clear();
        }
    }

    contexts_.push(frame(array, arr_ptr));
    return *this;
}

json_builder& json_builder::end_object() {
    if (contexts_.empty() || contexts_.top().type != object) {
        throw_exception(json_exception("No object to close or context mismatch"));
    }
    if (!current_key_.empty()) {
        throw_exception(json_exception("Incomplete key-value pair in object"));
    }
    contexts_.pop();
    return *this;
}

json_builder& json_builder::end_array() {
    if (contexts_.empty() || contexts_.top().type != array) {
        throw_exception(json_exception("No array to close or context mismatch"));
    }
    contexts_.pop();
    return *this;
}

json_builder& json_builder::key(const string& key) {
    if (contexts_.empty() || contexts_.top().type != object) {
        throw_exception(json_exception("Key can only be set inside an object"));
    }
    if (!current_key_.empty()) {
        throw_exception(json_exception("Key already set without corresponding value"));
    }
    current_key_ = key;
    return *this;
}

json_builder& json_builder::value_object(_NEFORCE function<void(json_builder&)>&& build_func) {
    json_builder inner_builder;
    inner_builder.begin_object();
    build_func(inner_builder);
    inner_builder.end_object();
    auto obj = inner_builder.build();
    return value_impl(_NEFORCE move(obj));
}

json_builder& json_builder::value_array(_NEFORCE function<void(json_builder&)>&& build_func) {
    json_builder inner_builder;
    inner_builder.begin_array();
    build_func(inner_builder);
    inner_builder.end_array();
    auto arr = inner_builder.build();
    return value_impl(_NEFORCE move(arr));
}

unique_ptr<json_value> json_builder::build() {
    if (!contexts_.empty()) {
        throw_exception(json_exception("Incomplete JSON structure - unclosed objects or arrays"));
    }
    if (!current_key_.empty()) {
        throw_exception(json_exception("Incomplete key-value pair"));
    }
    if (!root_) {
        throw_exception(json_exception("No JSON value built"));
    }
    return _NEFORCE move(root_);
}

NEFORCE_END_NAMESPACE__
