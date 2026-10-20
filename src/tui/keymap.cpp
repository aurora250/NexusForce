#include <NeForce/tui/keymap.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

vector<key_binding>& keymap::layer_of(const shortcut_layer layer) noexcept {
    switch (layer) {
        case shortcut_layer::overlay: {
            return overlay_;
        }
        case shortcut_layer::focus: {
            return focus_;
        }
        case shortcut_layer::global:
        default: {
            return global_;
        }
    }
}

const vector<key_binding>& keymap::layer_of(const shortcut_layer layer) const noexcept {
    switch (layer) {
        case shortcut_layer::overlay: {
            return overlay_;
        }
        case shortcut_layer::focus: {
            return focus_;
        }
        case shortcut_layer::global:
        default: {
            return global_;
        }
    }
}

void keymap::bind(const shortcut_layer layer, key_binding binding) { layer_of(layer).push_back(move(binding)); }

bool keymap::matches(const key_binding& binding, const key_event& e) noexcept {
    if (binding.key != key_event::type::unknown && binding.key != e.key) {
        return false;
    }
    if (binding.character != 0) {
        if (e.key != key_event::type::printable || e.cp.value() != static_cast<uint32_t>(binding.character)) {
            return false;
        }
    }
    const auto flags = static_cast<uint8_t>(e.mods);
    const auto required = static_cast<uint8_t>(binding.required);
    const auto forbidden = static_cast<uint8_t>(binding.forbidden);
    if ((flags & required) != required) {
        return false;
    }
    if ((flags & forbidden) != 0) {
        return false;
    }
    return true;
}

bool keymap::dispatch(const key_event& e) {
    for (const auto* list: {&global_, &overlay_, &focus_}) {
        for (auto it = list->rbegin(); it != list->rend(); ++it) {
            if (!matches(*it, e)) {
                continue;
            }
            if (!it->handler) {
                continue;
            }
            if (it->handler(e)) {
                return true;
            }
        }
    }
    return false;
}

void keymap::clear(const shortcut_layer layer) { layer_of(layer).clear(); }

void keymap::clear() {
    global_.clear();
    overlay_.clear();
    focus_.clear();
}

size_t keymap::count(const shortcut_layer layer) const noexcept { return layer_of(layer).size(); }

size_t keymap::count() const noexcept { return global_.size() + overlay_.size() + focus_.size(); }

vector<string> keymap::hints(const shortcut_layer layer) const {
    vector<string> result;
    for (const auto& binding: layer_of(layer)) {
        if (!binding.description.empty()) {
            result.push_back(binding.description);
        }
    }
    return result;
}

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
