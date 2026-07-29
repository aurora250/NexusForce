#include <NeForce/tui/component/hoverable.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__
NEFORCE_BEGIN_COMPONENTS__

namespace {
    class hoverable_component final : public component_base {
    public:
        hoverable_component(unique_ptr<component_base> child, function<void()> on_enter, function<void()> on_leave,
                            bool* hovered) :
        on_enter_(move(on_enter)),
        on_leave_(move(on_leave)),
        hovered_ptr_(hovered) {
            add_child(move(child));
        }

        element render() override { return active_child()->render(); }

        bool on_key(const key_event& e) override { return active_child()->on_key(e); }

        bool on_mouse(const mouse_event& e) override {
            const bool was_hovered = hovered_;

            if (e.action == mouse_action::move || e.action == mouse_action::press) {
                hovered_ = true;
            }

            if (!was_hovered && hovered_ && on_enter_) {
                on_enter_();
            } else if (was_hovered && !hovered_ && on_leave_) {
                on_leave_();
            }

            if (hovered_ptr_ != nullptr) {
                *hovered_ptr_ = hovered_;
            }

            return active_child()->on_mouse(e);
        }

        NEFORCE_NODISCARD bool focusable() const override { return active_child()->focusable(); }

        void on_mouse_leave() override {
            if (hovered_ && on_leave_) {
                on_leave_();
            }
            hovered_ = false;
            if (hovered_ptr_ != nullptr) {
                *hovered_ptr_ = false;
            }
        }

        void cleanup() noexcept override { hovered_ = false; }

    private:
        function<void()> on_enter_;
        function<void()> on_leave_;
        bool* hovered_ptr_;
        bool hovered_ = false;
    };
} // anonymous namespace


unique_ptr<component_base> hoverable(unique_ptr<component_base> child, function<void()> on_enter,
                                     function<void()> on_leave) {
    return make_unique<hoverable_component>(move(child), move(on_enter), move(on_leave), nullptr);
}

unique_ptr<component_base> hoverable(unique_ptr<component_base> child, bool* hovered) {
    return make_unique<hoverable_component>(move(child), nullptr, nullptr, hovered);
}

NEFORCE_END_COMPONENTS__
NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
