#include <NeForce/tui/component/radiobox.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__
NEFORCE_BEGIN_COMPONENTS__

namespace {
    class radiobox_component final : public component_base {
    public:
        explicit radiobox_component(radiobox_option opt) :
        opt_(move(opt)) {
            if (opt_.selected != nullptr) {
                selected_ = *opt_.selected;
            }
        }

        element render() override {
            elements items;
            for (size_t i = 0; i < opt_.entries.size(); ++i) {
                const bool sel = (static_cast<int>(i) == selected_);
                string label = sel ? "(*) " : "( ) ";
                label += opt_.entries[i];
                items.push_back(element::text(label));
            }
            return element::vbox(move(items));
        }

        bool on_key(const key_event& e) override {
            using K = key_event::type;
            int delta = 0;
            if (e.key == K::up) {
                delta = -1;
            } else if (e.key == K::down) {
                delta = 1;
            }

            if (delta != 0 && !opt_.entries.empty()) {
                const int n = static_cast<int>(opt_.entries.size());
                selected_ = (selected_ + delta + n) % n;
                if (opt_.selected != nullptr) {
                    *opt_.selected = selected_;
                }
                if (opt_.on_change) {
                    opt_.on_change();
                }
                schedule_render();
                return true;
            }
            return false;
        }

        NEFORCE_NODISCARD bool focusable() const override { return true; }

    private:
        radiobox_option opt_;
        int selected_ = 0;
    };
} // anonymous namespace


unique_ptr<component_base> radiobox(radiobox_option option) { return make_unique<radiobox_component>(move(option)); }

NEFORCE_END_COMPONENTS__
NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
