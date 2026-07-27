#include <NeForce/tui/component/modal.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

namespace {
    class modal_component final : public component_base {
    public:
        modal_component(unique_ptr<component_base> main, unique_ptr<component_base> overlay, const bool* show) :
        show_(show) {
            component_base* raw = main.get();
            add_child(move(main));
            add_child(move(overlay));
            set_active_child(raw);
        }

        element render() override {
            element main_el = children_[0]->render();
            if (show_ != nullptr && *show_) {
                element overlay_el = children_[1]->render();
                return element::zstack({main_el, overlay_el});
            }
            return main_el;
        }

        bool on_key(const key_event& e) override {
            if (show_ != nullptr && *show_) {
                if (children_[1]->on_key(e)) {
                    return true;
                }
                return true;
            }
            return children_[0]->on_key(e);
        }

        bool on_mouse(const mouse_event& e) override {
            if (show_ != nullptr && *show_) {
                if (children_[1]->on_mouse(e)) {
                    return true;
                }
                return true;
            }
            return children_[0]->on_mouse(e);
        }

        NEFORCE_NODISCARD bool focusable() const override { return true; }

    private:
        const bool* show_;
    };
} // anonymous namespace


unique_ptr<component_base> modal(unique_ptr<component_base> main, unique_ptr<component_base> overlay,
                                 const bool* show_modal) {
    return make_unique<modal_component>(move(main), move(overlay), show_modal);
}

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
