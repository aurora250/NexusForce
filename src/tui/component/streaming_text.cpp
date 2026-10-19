#include <NeForce/tui/component/streaming_text.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__
NEFORCE_BEGIN_COMPONENTS__

streaming_text_view::streaming_text_view(streaming_text_option opt) :
opt_(move(opt)) {}

void streaming_text_view::setup() { text_ = &create_state<string>(string()); }

void streaming_text_view::append(const string_view chunk) {
    if (chunk.empty()) {
        return;
    }
    pending_.append(chunk);
    if (pending_.size() > max<size_t>(1, opt_.max_bytes)) {
        pending_ = pending_.substr(pending_.size() - max<size_t>(1, opt_.max_bytes));
    }
}

void streaming_text_view::commit() {
    if (text_ == nullptr || pending_.empty()) {
        return;
    }
    string next = text_->value();
    next.append(pending_);
    pending_.clear();
    if (next.size() > max<size_t>(1, opt_.max_bytes)) {
        next = next.substr(next.size() - max<size_t>(1, opt_.max_bytes));
    }
    *text_ = move(next);
}

void streaming_text_view::clear() {
    pending_.clear();
    if (text_ != nullptr) {
        *text_ = string();
    }
}

string_view streaming_text_view::text() const noexcept { return (text_ != nullptr) ? text_->value().view() : ""; }

element streaming_text_view::render() {
    if (text_ == nullptr) {
        return element::empty();
    }
    element el = element::text(text_->value(), opt_.style, opt_.wrap);
    el.set_owner(this);
    return el;
}

void streaming_text_view::on_animation(const int64_t delta) {
    ignore = delta;
    commit();
}

unique_ptr<component_base> streaming_text(streaming_text_option opt) {
    return make_unique<streaming_text_view>(move(opt));
}

NEFORCE_END_COMPONENTS__
NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
