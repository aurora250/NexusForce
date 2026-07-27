#include <NeForce/tui/component/slider.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

namespace {
    class slider_component final : public component_base {
    public:
        explicit slider_component(slider_options options) :
        opt_(move(options)) {}

        element render() override {
            if (opt_.value == nullptr) {
                return element::text(opt_.label + " [error: null value]");
            }
            const int range = opt_.max_value - opt_.min_value;
            const float progress =
                    (range > 0) ? static_cast<float>(*opt_.value - opt_.min_value) / static_cast<float>(range) : 0.0F;

            string bar;
            bar.reserve(32);
            bar += "[";
            constexpr int bar_width = 16;
            const int filled = static_cast<int>(progress * bar_width);
            for (int i = 0; i < bar_width; ++i) {
                bar += (i < filled) ? '=' : ' ';
            }
            bar += "] " + to_string(*opt_.value);

            return element::hbox({element::text(opt_.label + " "), element::text(bar)});
        }

        bool on_key(const key_event& e) override {
            if (opt_.value == nullptr) {
                return false;
            }
            using K = key_event::type;
            if (e.key == K::left || e.key == K::down) {
                *opt_.value =
                        (*opt_.value - opt_.increment < opt_.min_value) ? opt_.min_value : *opt_.value - opt_.increment;
                schedule_render();
                return true;
            }
            if (e.key == K::right || e.key == K::up) {
                *opt_.value =
                        (*opt_.value + opt_.increment > opt_.max_value) ? opt_.max_value : *opt_.value + opt_.increment;
                schedule_render();
                return true;
            }
            return false;
        }

        NEFORCE_NODISCARD bool focusable() const override { return true; }

    private:
        slider_options opt_;
    };
} // namespace


unique_ptr<component_base> slider(slider_options options) { return make_unique<slider_component>(move(options)); }

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
