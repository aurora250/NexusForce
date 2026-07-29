#include <NeForce/tui/component/scroll_view.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__
NEFORCE_BEGIN_COMPONENTS__

namespace {
    class scroll_view_component final : public component<> {
    public:
        explicit scroll_view_component(scroll_view_option opt) :
        opt_(move(opt)) {}

        void setup() override {
            scroll_x_ = &create_state<int>(0);
            if (opt_.external_scroll_y != nullptr) {
                scroll_y_ = opt_.external_scroll_y;
            } else {
                scroll_y_ = &create_state<int>(0);
            }
        }

        element render() override {
            element content = opt_.content();
            element sv = element::scroll_view(move(content), opt_.style, scroll_x_->value(), scroll_y_->value())
                                 .with_scroll_x_state(scroll_x_)
                                 .with_scroll_y_state(scroll_y_);
            sv.set_owner(this);
            return sv;
        }

        NEFORCE_NODISCARD bool focusable() const override { return false; }

    private:
        scroll_view_option opt_;
        state<int>* scroll_x_ = nullptr;
        state<int>* scroll_y_ = nullptr;
    };
} // namespace


unique_ptr<component_base> scroll_view(scroll_view_option opt) { return make_unique<scroll_view_component>(move(opt)); }

NEFORCE_END_COMPONENTS__
NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
