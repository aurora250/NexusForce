#include <NeForce/tui/component/containers.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__
NEFORCE_BEGIN_COMPONENTS__

namespace {
    class container_impl final : public component_base {
    public:
        enum class layout_mode {
            vertical,
            horizontal,
            stacked
        };

        container_impl(vector<unique_ptr<component_base>> children, int* selected_ptr, const layout_mode mode) :
        selected_ptr_(selected_ptr),
        mode_(mode) {
            for (auto& child: children) {
                add_child(move(child));
            }
            if (!children_.empty()) {
                set_active_child(children_[0].get());
                if (selected_ptr_ != nullptr) {
                    *selected_ptr_ = 0;
                }
            }
        }

        element render() override {
            elements elements;
            for (size_t i = 0; i < children_.size(); ++i) {
                elements.push_back(children_[i]->render());
            }
            switch (mode_) {
                case layout_mode::horizontal:
                    return element::hbox(move(elements));
                case layout_mode::stacked:
                    return element::zstack(move(elements));
                case layout_mode::vertical:
                default:
                    return element::vbox(move(elements));
            }
        }

        bool on_key(const key_event& e) override {
            if (active_child() != nullptr && active_child()->on_key(e)) {
                return true;
            }

            int delta = 0;
            using K = key_event::type;

            if (mode_ == layout_mode::stacked) {
                if (e.key == K::right || e.key == K::down) {
                    delta = +1;
                } else if (e.key == K::left || e.key == K::up) {
                    delta = -1;
                }
            } else if (mode_ == layout_mode::vertical) {
                if (e.key == K::up) {
                    delta = -1;
                } else if (e.key == K::down) {
                    delta = +1;
                }
            } else {
                if (e.key == K::left) {
                    delta = -1;
                } else if (e.key == K::right) {
                    delta = +1;
                }
            }

            if (delta != 0 && !children_.empty()) {
                const auto* const active = active_child();
                int idx = 0;
                for (size_t i = 0; i < children_.size(); ++i) {
                    if (children_[i].get() == active) {
                        idx = static_cast<int>(i);
                        break;
                    }
                }
                idx = (idx + delta + static_cast<int>(children_.size())) % static_cast<int>(children_.size());
                set_active_child(children_[idx].get());
                if (selected_ptr_ != nullptr) {
                    *selected_ptr_ = idx;
                }
                schedule_render();
                return true;
            }

            return false;
        }

        NEFORCE_NODISCARD bool focusable() const override { return true; }

    private:
        int* selected_ptr_;
        layout_mode mode_;
    };
} // anonymous namespace


unique_ptr<component_base> container::vertical(vector<unique_ptr<component_base>> children, int* selected) {
    return make_unique<container_impl>(move(children), selected, container_impl::layout_mode::vertical);
}

unique_ptr<component_base> container::horizontal(vector<unique_ptr<component_base>> children, int* selected) {
    return make_unique<container_impl>(move(children), selected, container_impl::layout_mode::horizontal);
}

unique_ptr<component_base> container::tab(vector<unique_ptr<component_base>> children, int* selected) {
    return make_unique<container_impl>(move(children), selected, container_impl::layout_mode::horizontal);
}

unique_ptr<component_base> container::stacked(vector<unique_ptr<component_base>> children, int* selected) {
    return make_unique<container_impl>(move(children), selected, container_impl::layout_mode::stacked);
}

NEFORCE_END_COMPONENTS__
NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
