#include <NeForce/core/file/yaml/yaml_builder.hpp>
NEFORCE_BEGIN_NAMESPACE__

yaml_builder::yaml_builder() {
    root_ = make_shared<yaml_mapping>(yaml_mapping::Block);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    contexts_.push(frame(frame::mapping, static_cast<yaml_mapping*>(root_.get())));
}

void yaml_builder::apply_pending_metadata(const shared_ptr<yaml_value>& value) {
    if (!pending_anchor_.empty()) {
        value->anchor = pending_anchor_;
        anchors_[pending_anchor_] = value;
        pending_anchor_.clear();
    }
    if (!pending_tag_.empty()) {
        value->tag = pending_tag_;
        pending_tag_.clear();
    }
}

void yaml_builder::add_to_parent_and_push(const shared_ptr<yaml_value>& container, frame f) {
    apply_pending_metadata(container);

    const auto& top = contexts_.top();
    if (top.type == frame::sequence) {
        top.seq_ptr->add_element(container);
    } else if (top.type == frame::mapping) {
        if (current_key_.empty()) {
            NEFORCE_THROW_EXCEPTION(yaml_exception("No key set for value in mapping"));
        }
        if (top.map_ptr->has_member(current_key_)) {
            NEFORCE_THROW_EXCEPTION(yaml_exception(("Duplicate key: " + current_key_).data()));
        }
        top.map_ptr->add_member(current_key_, container);
        current_key_.clear();
    }

    contexts_.push(f);
}

yaml_builder& yaml_builder::key(string key) {
    if (contexts_.empty()) {
        NEFORCE_THROW_EXCEPTION(yaml_exception("Cannot set key outside of a mapping context"));
    }

    const auto& top = contexts_.top();
    if (top.type != frame::mapping) {
        NEFORCE_THROW_EXCEPTION(yaml_exception("Cannot set key in non-mapping context"));
    }

    current_key_ = move(key);
    return *this;
}

yaml_builder& yaml_builder::begin_mapping() { return begin_block_mapping(); }

yaml_builder& yaml_builder::begin_block_mapping() {
    auto map = make_shared<yaml_mapping>(yaml_mapping::Block);
    yaml_mapping* map_ptr = map.get();
    add_to_parent_and_push(map, frame(frame::mapping, map_ptr));
    return *this;
}

yaml_builder& yaml_builder::begin_flow_mapping() {
    auto map = make_shared<yaml_mapping>(yaml_mapping::Flow);
    yaml_mapping* map_ptr = map.get();
    add_to_parent_and_push(map, frame(frame::mapping, map_ptr));
    return *this;
}

yaml_builder& yaml_builder::end_mapping() {
    if (contexts_.empty()) {
        NEFORCE_THROW_EXCEPTION(yaml_exception("No mapping to end"));
    }

    const auto& top = contexts_.top();
    if (top.type != frame::mapping) {
        NEFORCE_THROW_EXCEPTION(yaml_exception("Current context is not a mapping"));
    }

    if (contexts_.size() == 1) {
        NEFORCE_THROW_EXCEPTION(yaml_exception("Cannot end root mapping"));
    }

    contexts_.pop();
    current_key_.clear();

    return *this;
}

yaml_builder& yaml_builder::begin_sequence() { return begin_block_sequence(); }

yaml_builder& yaml_builder::begin_block_sequence() {
    auto seq = make_shared<yaml_sequence>(yaml_sequence::Block);
    yaml_sequence* seq_ptr = seq.get();
    add_to_parent_and_push(seq, frame(frame::sequence, seq_ptr));
    return *this;
}

yaml_builder& yaml_builder::begin_flow_sequence() {
    auto seq = make_shared<yaml_sequence>(yaml_sequence::Flow);
    yaml_sequence* seq_ptr = seq.get();
    add_to_parent_and_push(seq, frame(frame::sequence, seq_ptr));
    return *this;
}

yaml_builder& yaml_builder::end_sequence() {
    if (contexts_.empty()) {
        NEFORCE_THROW_EXCEPTION(yaml_exception("No sequence to end"));
    }

    const auto& top = contexts_.top();
    if (top.type != frame::sequence) {
        NEFORCE_THROW_EXCEPTION(yaml_exception("Current context is not a sequence"));
    }

    if (contexts_.size() == 1) {
        NEFORCE_THROW_EXCEPTION(yaml_exception("Cannot end root sequence"));
    }

    contexts_.pop();
    current_key_.clear();

    return *this;
}

yaml_builder& yaml_builder::value_datetime(const datetime& dt) { return value_impl(make_shared<yaml_timestamp>(dt)); }

yaml_builder& yaml_builder::value_string(string v, yaml_string::string_style style) {
    return value_impl(make_shared<yaml_string>(_NEFORCE move(v), style));
}

yaml_builder& yaml_builder::value_mapping(const function<void(yaml_builder&)>& build_func) {
    return value_block_mapping(build_func);
}

yaml_builder& yaml_builder::value_block_mapping(const function<void(yaml_builder&)>& build_func) {
    begin_block_mapping();
    build_func(*this);
    end_mapping();
    return *this;
}

yaml_builder& yaml_builder::value_flow_mapping(const function<void(yaml_builder&)>& build_func) {
    begin_flow_mapping();
    build_func(*this);
    end_mapping();
    return *this;
}

yaml_builder& yaml_builder::value_sequence(const function<void(yaml_builder&)>& build_func) {
    return value_block_sequence(build_func);
}

yaml_builder& yaml_builder::value_block_sequence(const function<void(yaml_builder&)>& build_func) {
    begin_block_sequence();
    build_func(*this);
    end_sequence();
    return *this;
}

yaml_builder& yaml_builder::value_flow_sequence(const function<void(yaml_builder&)>& build_func) {
    begin_flow_sequence();
    build_func(*this);
    end_sequence();
    return *this;
}

yaml_builder& yaml_builder::anchor(string name) {
    pending_anchor_ = move(name);
    return *this;
}

yaml_builder& yaml_builder::tag(string t) {
    pending_tag_ = move(t);
    return *this;
}

yaml_builder& yaml_builder::alias(string name) {
    auto it = anchors_.find(name);
    if (it == anchors_.end()) {
        NEFORCE_THROW_EXCEPTION(yaml_exception(("Anchor not found: " + name).data()));
    }

    return value(it->second);
}

yaml_builder& yaml_builder::begin_document() {
    if (contexts_.size() != 1) {
        NEFORCE_THROW_EXCEPTION(yaml_exception("Unclosed contexts before beginning new document"));
    }

    documents_.push_back(root_);
    contexts_.pop();

    auto new_root = make_shared<yaml_mapping>(yaml_mapping::Block);
    root_ = new_root;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    contexts_.push(frame(frame::mapping, static_cast<yaml_mapping*>(root_.get())));
    current_key_.clear();
    pending_anchor_.clear();
    pending_tag_.clear();

    return *this;
}

shared_ptr<yaml_value> yaml_builder::build() {
    if (contexts_.size() != 1) {
        NEFORCE_THROW_EXCEPTION(yaml_exception("Unclosed mapping or sequence context"));
    }

    return root_;
}

vector<shared_ptr<yaml_value>> yaml_builder::build_documents() {
    if (contexts_.size() != 1) {
        NEFORCE_THROW_EXCEPTION(yaml_exception("Unclosed mapping or sequence context"));
    }

    vector<shared_ptr<yaml_value>> docs = documents_;
    docs.push_back(root_);
    return docs;
}

NEFORCE_END_NAMESPACE__
