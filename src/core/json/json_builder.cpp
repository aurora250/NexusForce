#include <MSTL/core/json/json_builder.hpp>
MSTL_BEGIN_NAMESPACE__

json_builder& json_builder::begin_object() {
    auto new_object = make_unique<json_object>();
    json_object* obj_ptr = new_object.get();

    if (contexts.empty()) {
        if (root) {
            Exception(JsonOperateError("Root value already set"));
        }
        root = _MSTL move(new_object);
    } else {
        const auto & current = contexts.top();
        if (current.type == ARRAY) {
            current.array_ptr->add_element(_MSTL move(new_object));
        } else if (current.type == OBJECT) {
            if (current_key.empty()) {
                Exception(JsonOperateError("No key set for object value"));
            }
            current.object_ptr->add_member(current_key, _MSTL move(new_object));
            current_key.clear();
        }
    }

    contexts.push(frame(OBJECT, obj_ptr));
    return *this;
}

json_builder& json_builder::begin_array() {
    auto new_array = make_unique<json_array>();
    json_array* arr_ptr = new_array.get();

    if (contexts.empty()) {
        if (root) {
            Exception(JsonOperateError("Root value already set"));
        }
        root = _MSTL move(new_array);
    } else {
        const auto & current = contexts.top();
        if (current.type == ARRAY) {
            current.array_ptr->add_element(_MSTL move(new_array));
        } else if (current.type == OBJECT) {
            if (current_key.empty()) {
                Exception(JsonOperateError("No key set for array value"));
            }
            current.object_ptr->add_member(current_key, _MSTL move(new_array));
            current_key.clear();
        }
    }

    contexts.push(frame(ARRAY, arr_ptr));
    return *this;
}

json_builder& json_builder::end_object() {
    if (contexts.empty() || contexts.top().type != OBJECT) {
        Exception(JsonOperateError("No object to close or context mismatch"));
    }
    if (!current_key.empty()) {
        Exception(JsonOperateError("Incomplete key-value pair in object"));
    }
    contexts.pop();
    return *this;
}

json_builder& json_builder::end_array() {
    if (contexts.empty() || contexts.top().type != ARRAY) {
        Exception(JsonOperateError("No array to close or context mismatch"));
    }
    contexts.pop();
    return *this;
}

json_builder& json_builder::key(const string& k) {
    if (contexts.empty() || contexts.top().type != OBJECT) {
        Exception(JsonOperateError("Key can only be set inside an object"));
    }
    if (!current_key.empty()) {
        Exception(JsonOperateError("Key already set without corresponding value"));
    }
    current_key = k;
    return *this;
}

json_builder& json_builder::value_object(_MSTL function<void(json_builder&)>&& build_func) {
    json_builder inner_builder;
    inner_builder.begin_object();
    build_func(inner_builder);
    inner_builder.end_object();
    auto obj = inner_builder.build();
    return value_impl(_MSTL move(obj));
}

json_builder& json_builder::value_array(_MSTL function<void(json_builder&)>&& build_func) {
    json_builder inner_builder;
    inner_builder.begin_array();
    build_func(inner_builder);
    inner_builder.end_array();
    auto arr = inner_builder.build();
    return value_impl(_MSTL move(arr));
}

unique_ptr<json_value> json_builder::build() {
    if (!contexts.empty()) {
        Exception(JsonOperateError("Incomplete JSON structure - unclosed objects or arrays"));
    }
    if (!current_key.empty()) {
        Exception(JsonOperateError("Incomplete key-value pair"));
    }
    if (!root) {
        Exception(JsonOperateError("No JSON value built"));
    }
    return _MSTL move(root);
}

MSTL_END_NAMESPACE__
