#include <NeForce/tui/component/toggle.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

namespace {
    class toggle_component final : public component_base {
    public:
        explicit toggle_component(toggle_option opt) :
        opt_(move(opt)) {
            if (opt_.selected != nullptr) {
                selected_ = *opt_.selected;
            }
        }

        element render() override {
            elements items;
            for (size_t i = 0; i < opt_.entries.size(); ++i) {
                const bool sel = (static_cast<int>(i) == selected_);
                string label;
                if (sel) {
                    label = "[" + opt_.entries[i] + "]";
                } else {
                    label = " " + opt_.entries[i] + " ";
                }
                items.push_back(element::text(label));
            }
            if (opt_.horizontal) {
                return element::hbox(move(items));
            }
            return element::vbox(move(items));
        }

        bool on_key(const key_event& e) override {
            using K = key_event::type;
            if (e.key == K::enter || (e.is_printable() && e.cp == ' ') || e.key == K::right || e.key == K::down) {
                if (!opt_.entries.empty()) {
                    const int n = static_cast<int>(opt_.entries.size());
                    selected_ = (selected_ + 1) % n;
                    if (opt_.selected != nullptr) {
                        *opt_.selected = selected_;
                    }
                    if (opt_.on_change) {
                        opt_.on_change();
                    }
                    schedule_render();
                }
                return true;
            }
            if (e.key == K::left || e.key == K::up) {
                if (!opt_.entries.empty()) {
                    const int n = static_cast<int>(opt_.entries.size());
                    selected_ = (selected_ - 1 + n) % n;
                    if (opt_.selected != nullptr) {
                        *opt_.selected = selected_;
                    }
                    if (opt_.on_change) {
                        opt_.on_change();
                    }
                    schedule_render();
                }
                return true;
            }
            return false;
        }

        NEFORCE_NODISCARD bool focusable() const override { return true; }

    private:
        toggle_option opt_;
        int selected_ = 0;
    };
} // anonymous namespace


unique_ptr<component_base> toggle(toggle_option option) { return make_unique<toggle_component>(move(option)); }

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
