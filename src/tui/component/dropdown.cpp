#include <NeForce/tui/component/dropdown.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__
NEFORCE_BEGIN_COMPONENTS__

namespace {
    class dropdown_component final : public component_base {
    public:
        explicit dropdown_component(dropdown_option opt) :
        opt_(_NEFORCE move(opt)) {
            if (opt_.open == nullptr) {
                owned_open_ = false;
                open_ptr_ = &owned_open_;
            } else {
                open_ptr_ = opt_.open;
            }
            if (opt_.selected != nullptr) {
                selected_ = *opt_.selected;
            }
        }

        element render() override {
            elements children;
            string header_text = (selected_ >= 0 && static_cast<size_t>(selected_) < opt_.entries.size())
                                         ? opt_.entries[selected_]
                                         : "Select...";
            header_text = (*open_ptr_ ? "v " : "> ") + header_text;
            children.push_back(element::text(header_text));

            if (*open_ptr_) {
                elements items;
                for (size_t i = 0; i < opt_.entries.size(); ++i) {
                    const bool is_sel = (static_cast<int>(i) == selected_);
                    string label = is_sel ? " * " : "   ";
                    label += opt_.entries[i];
                    items.push_back(element::text(label));
                }
                children.push_back(element::vbox(_NEFORCE move(items)));
            }
            return element::vbox(_NEFORCE move(children));
        }

        bool on_key(const key_event& e) override {
            using K = key_event::type;

            if (e.key == K::enter || (e.is_printable() && e.cp == ' ') || e.key == K::down) {
                if (!*open_ptr_ && !opt_.entries.empty()) {
                    *open_ptr_ = true;
                    schedule_render();
                    return true;
                }
            }

            if (*open_ptr_) {
                if (e.key == K::up) {
                    if (selected_ > 0) {
                        selected_--;
                    }
                    if (opt_.selected != nullptr) {
                        *opt_.selected = selected_;
                    }
                    if (opt_.on_change) {
                        opt_.on_change();
                    }
                    schedule_render();
                    return true;
                }
                if (e.key == K::down) {
                    const int n = static_cast<int>(opt_.entries.size());
                    if (selected_ < n - 1) {
                        selected_++;
                    }
                    if (opt_.selected != nullptr) {
                        *opt_.selected = selected_;
                    }
                    if (opt_.on_change) {
                        opt_.on_change();
                    }
                    schedule_render();
                    return true;
                }
                if (e.key == K::enter || e.key == K::escape) {
                    *open_ptr_ = false;
                    if (e.key == K::enter && opt_.on_change) {
                        opt_.on_change();
                    }
                    schedule_render();
                    return true;
                }
            }

            return false;
        }

        NEFORCE_NODISCARD bool focusable() const override { return true; }

    private:
        dropdown_option opt_;
        bool* open_ptr_;
        bool owned_open_{true};
        int selected_ = 0;
    };

} // anonymous namespace


unique_ptr<component_base> dropdown(dropdown_option option) { return make_unique<dropdown_component>(move(option)); }

NEFORCE_END_COMPONENTS__
NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
