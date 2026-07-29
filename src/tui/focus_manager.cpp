#include <NeForce/tui/focus_manager.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

void focus_manager::rebuild_chain(component_base* root) {
    if (!chain_dirty_) {
        return;
    }
    focus_chain_.clear();
    if (root == nullptr) {
        chain_dirty_ = false;
        return;
    }

    vector<component_base*> stack;
    stack.push_back(root);
    while (!stack.empty()) {
        auto* comp = stack.back();
        stack.pop_back();
        comp->collect_focusable(focus_chain_);
    }
    chain_dirty_ = false;
}

void focus_manager::set_focus(component_base* comp) {
    if (focused_ == comp) {
        return;
    }
    if (focused_ != nullptr) {
        focused_->set_has_focus(false);
    }
    focused_ = comp;
    if (focused_ != nullptr) {
        focused_->set_has_focus(true);
        focused_->take_focus();
    }
}

void focus_manager::focus_next() {
    if (focus_chain_.empty()) {
        return;
    }
    size_t idx = 0;
    if (focused_ != nullptr) {
        const auto it = find(focus_chain_.begin(), focus_chain_.end(), focused_);
        if (it != focus_chain_.end()) {
            idx = static_cast<size_t>(it - focus_chain_.begin()) + 1;
        }
    }
    if (idx >= focus_chain_.size()) {
        idx = 0;
    }
    set_focus(focus_chain_[idx]);
}

void focus_manager::focus_prev() {
    if (focus_chain_.empty()) {
        return;
    }
    size_t idx = focus_chain_.size() - 1;
    if (focused_ != nullptr) {
        const auto it = find(focus_chain_.begin(), focus_chain_.end(), focused_);
        if (it != focus_chain_.end() && it != focus_chain_.begin()) {
            idx = static_cast<size_t>(it - focus_chain_.begin()) - 1;
        }
    }
    set_focus(focus_chain_[idx]);
}

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
