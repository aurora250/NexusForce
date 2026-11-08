#include <NeForce/core/file/yaml/yaml_builder.hpp>
NEFORCE_BEGIN_NAMESPACE__

yaml_builder::yaml_builder() { (void) new_root_mapping(); }

yaml_value* yaml_builder::new_root_mapping() {
    document_ = yaml_document();
    yaml_value* root = document_.new_mapping(false);
    document_.root() = *root;
    contexts_.clear();
    contexts_.push_back(frame{mapping, root, string()});
    pending_anchor_.clear();
    pending_tag_.clear();
    return root;
}

void yaml_builder::apply_pending_metadata(yaml_value& value) {
    if (pending_anchor_.empty() && pending_tag_.empty()) {
        return;
    }
    document_.set_meta(value, string_view(pending_anchor_.data(), pending_anchor_.size()),
                       string_view(pending_tag_.data(), pending_tag_.size()), true);
    if (!pending_anchor_.empty()) {
        anchors_.push_back(anchor_entry{pending_anchor_, value});
    }
    pending_anchor_.clear();
    pending_tag_.clear();
}

yaml_builder& yaml_builder::push_value(yaml_value value) {
    apply_pending_metadata(value);
    if (contexts_.empty()) {
        NEFORCE_THROW_EXCEPTION(yaml_exception("No active context"));
    }
    frame& current = contexts_.back();
    if (current.type == sequence) {
        document_.sequence_append(*current.node, value);
        return *this;
    }
    if (current.pending_key.empty()) {
        NEFORCE_THROW_EXCEPTION(yaml_exception("No key in mapping"));
    }
    if (current.node->as_mapping().has_member(string_view(current.pending_key.data(), current.pending_key.size()))) {
        NEFORCE_THROW_EXCEPTION(yaml_exception("Duplicate key"));
    }
    document_.mapping_append(*current.node, string_view(current.pending_key.data(), current.pending_key.size()), value,
                             true);
    current.pending_key.clear();
    return *this;
}

yaml_builder& yaml_builder::key(string key) {
    if (contexts_.empty() || contexts_.back().type != mapping) {
        NEFORCE_THROW_EXCEPTION(yaml_exception("Cannot set key outside mapping"));
    }
    contexts_.back().pending_key = _NEFORCE move(key);
    return *this;
}

void yaml_builder::begin_container(const range_type type, const bool flow) {
    if (contexts_.empty()) {
        NEFORCE_THROW_EXCEPTION(yaml_exception("No active context"));
    }
    if (type == mapping) {
        yaml_value* node = document_.new_mapping(flow);
        push_value(*node);
        contexts_.push_back(frame{mapping, node, string()});
    } else {
        yaml_value* node = document_.new_sequence(flow);
        push_value(*node);
        contexts_.push_back(frame{sequence, node, string()});
    }
}

void yaml_builder::end_container(const range_type type) {
    if (contexts_.size() <= 1) {
        NEFORCE_THROW_EXCEPTION(yaml_exception("Cannot end root context"));
    }
    if (contexts_.back().type != type) {
        NEFORCE_THROW_EXCEPTION(yaml_exception("Mismatched end context"));
    }
    contexts_.pop_back();
}

yaml_builder& yaml_builder::begin_mapping() {
    begin_container(mapping, false);
    return *this;
}

yaml_builder& yaml_builder::begin_block_mapping() {
    begin_container(mapping, false);
    return *this;
}

yaml_builder& yaml_builder::begin_flow_mapping() {
    begin_container(mapping, true);
    return *this;
}

yaml_builder& yaml_builder::end_mapping() {
    end_container(mapping);
    return *this;
}

yaml_builder& yaml_builder::begin_sequence() {
    begin_container(sequence, false);
    return *this;
}

yaml_builder& yaml_builder::begin_block_sequence() {
    begin_container(sequence, false);
    return *this;
}

yaml_builder& yaml_builder::begin_flow_sequence() {
    begin_container(sequence, true);
    return *this;
}

yaml_builder& yaml_builder::end_sequence() {
    end_container(sequence);
    return *this;
}

yaml_builder& yaml_builder::value_mapping(const function<void(yaml_builder&)>& build_func) {
    begin_mapping();
    build_func(*this);
    end_mapping();
    return *this;
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
    begin_sequence();
    build_func(*this);
    end_sequence();
    return *this;
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
    pending_anchor_ = _NEFORCE move(name);
    return *this;
}

yaml_builder& yaml_builder::tag(string t) {
    pending_tag_ = _NEFORCE move(t);
    return *this;
}

yaml_builder& yaml_builder::alias(const string& name) {
    for (const anchor_entry& entry: anchors_) {
        if (entry.name == name) {
            return push_value(entry.value);
        }
    }
    NEFORCE_THROW_EXCEPTION(yaml_exception("Unknown anchor"));
}

yaml_builder& yaml_builder::begin_document() {
    if (contexts_.size() != 1) {
        NEFORCE_THROW_EXCEPTION(yaml_exception("Unclosed contexts before beginning new document"));
    }
    documents_.push_back(_NEFORCE move(document_));
    anchors_.clear();
    (void) new_root_mapping();
    return *this;
}

yaml_document yaml_builder::build() {
    if (contexts_.size() != 1) {
        NEFORCE_THROW_EXCEPTION(yaml_exception("Unclosed mapping or sequence context"));
    }
    return _NEFORCE move(document_);
}

vector<yaml_document> yaml_builder::build_documents() {
    if (contexts_.size() != 1) {
        NEFORCE_THROW_EXCEPTION(yaml_exception("Unclosed mapping or sequence context"));
    }
    vector<yaml_document> result = _NEFORCE move(documents_);
    result.push_back(_NEFORCE move(document_));
    return result;
}

NEFORCE_END_NAMESPACE__
