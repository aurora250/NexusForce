#include "NeForce/core/file/json/json_builder.hpp"
NEFORCE_BEGIN_NAMESPACE__

json_builder& json_builder::begin_object() {
    frame next;
    next.is_object = true;
    next.node = document_.new_container(true);
    frames_.emplace_back(move(next));
    return *this;
}

json_builder& json_builder::begin_array() {
    frame next;
    next.is_object = false;
    next.node = document_.new_container(false);
    frames_.emplace_back(move(next));
    return *this;
}

json_builder& json_builder::end_object() {
    if (frames_.empty() || !frames_.back().is_object) {
        NEFORCE_THROW_EXCEPTION(json_exception("json_builder: end_object without matching begin_object."));
    }
    frame current = move(frames_.back());
    frames_.pop_back();
    push_value(*current.node);
    return *this;
}

json_builder& json_builder::end_array() {
    if (frames_.empty() || frames_.back().is_object) {
        NEFORCE_THROW_EXCEPTION(json_exception("json_builder: end_array without matching begin_array."));
    }
    frame current = move(frames_.back());
    frames_.pop_back();
    push_value(*current.node);
    return *this;
}

json_builder& json_builder::key(const string& key) {
    if (frames_.empty() || !frames_.back().is_object) {
        NEFORCE_THROW_EXCEPTION(json_exception("json_builder: key used outside of object."));
    }
    frames_.back().pending_key = key;
    return *this;
}

json_builder& json_builder::value(const json_value& value) {
    push_value(value);
    return *this;
}

json_builder& json_builder::value_string(const string_view text) {
    push_value(document_.make_string_value(text));
    return *this;
}

json_builder& json_builder::value_object(const function<void(json_builder&)>& build_func) {
    this->begin_object();
    build_func(*this);
    return this->end_object();
}

json_builder& json_builder::value_array(const function<void(json_builder&)>& build_func) {
    this->begin_array();
    build_func(*this);
    return this->end_array();
}

void json_builder::push_value(const json_value value) {
    if (frames_.empty()) {
        document_.root() = value;
        has_root_ = true;
        return;
    }
    frame& current = frames_.back();
    if (current.is_object) {
        document_.object_append(*current.node, string_view(current.pending_key.data(), current.pending_key.size()),
                                value);
        current.pending_key.clear();
    } else {
        document_.array_append(*current.node, value);
    }
}

json_document json_builder::build() {
    if (!frames_.empty()) {
        NEFORCE_THROW_EXCEPTION(json_exception("json_builder: unclosed container."));
    }
    if (!has_root_) {
        document_.root() = json_value::make_null();
    }
    return move(document_);
}

NEFORCE_END_NAMESPACE__
