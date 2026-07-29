#include <NeForce/tui/component/menu.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__
NEFORCE_BEGIN_COMPONENTS__

namespace {
    class menu_component final : public component_base {
    public:
        explicit menu_component(menu_option opt) :
        opt_(move(opt)) {
            if (opt_.selected != nullptr) {
                selected_ = *opt_.selected;
            }
        }

        element render() override {
            elements entries;
            for (size_t i = 0; i < opt_.entries.size(); ++i) {
                const bool is_selected = (static_cast<int>(i) == selected_);
                style s = is_selected ? opt_.focused_style : opt_.normal_style;
                if (is_selected) {
                    style tmp;
                    tmp.bold = true;
                    s = style::merge(s, tmp);
                }
                entries.push_back(element::text(opt_.entries[i], s));
            }
            if (opt_.horizontal) {
                return element::hbox(move(entries));
            }
            return element::vbox(move(entries));
        }

        bool on_key(const key_event& e) override {
            if (opt_.entries.empty()) {
                return false;
            }

            using K = key_event::type;
            int delta = 0;

            if (!opt_.horizontal) {
                if (e.key == K::up) {
                    delta = -1;
                } else if (e.key == K::down) {
                    delta = 1;
                }
            } else {
                if (e.key == K::left) {
                    delta = -1;
                } else if (e.key == K::right) {
                    delta = 1;
                }
            }

            if (delta != 0) {
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

            if (e.key == K::enter && opt_.on_enter) {
                opt_.on_enter();
                return true;
            }

            return false;
        }

        NEFORCE_NODISCARD bool focusable() const override { return true; }

    private:
        menu_option opt_;
        int selected_ = 0;
    };
} // anonymous namespace


unique_ptr<component_base> menu(menu_option option) { return make_unique<menu_component>(move(option)); }

NEFORCE_END_COMPONENTS__
NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
