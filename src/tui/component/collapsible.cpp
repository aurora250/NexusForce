#include <NeForce/tui/component/collapsible.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

namespace {
    class collapsible_component final : public component_base {
    public:
        collapsible_component(string title, unique_ptr<component_base> child, bool* show) :
        title_(move(title)) {
            owned_show_ = (show != nullptr) ? *show : true;
            show_ptr_ = (show != nullptr) ? show : &owned_show_;
            add_child(_NEFORCE move(child));
        }

        element render() override {
            const string arrow = *show_ptr_ ? "v" : ">";
            const element header = element::text(arrow + " " + title_);
            elements parts;
            parts.emplace_back(move(header));
            if (*show_ptr_) {
                parts.push_back(active_child()->render());
            }
            return element::vbox(_NEFORCE move(parts));
        }

        bool on_key(const key_event& e) override {
            using K = key_event::type;
            if (e.key == K::enter || (e.is_printable() && e.cp == ' ')) {
                *show_ptr_ = !*show_ptr_;
                schedule_render();
                return true;
            }
            if (*show_ptr_ && active_child() != nullptr) {
                return active_child()->on_key(e);
            }
            return false;
        }

        NEFORCE_NODISCARD bool focusable() const override { return true; }

    private:
        string title_;
        bool* show_ptr_;
        bool owned_show_ = true;
    };
} // anonymous namespace


unique_ptr<component_base> collapsible(const string& title, unique_ptr<component_base> child, bool* show) {
    return make_unique<collapsible_component>(title, move(child), show);
}

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
