#include <NeForce/tui/component/window.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

namespace {
    class window_component final : public component_base {
    public:
        explicit window_component(window_options opt) :
        opt_(move(opt)),
        owned_left_(opt_.left == nullptr ? 2 : 0),
        owned_top_(opt_.top == nullptr ? 1 : 0),
        owned_width_(opt_.width == nullptr ? 40 : 0),
        owned_height_(opt_.height == nullptr ? 10 : 0) {
            if (opt_.left == nullptr) {
                opt_.left = &owned_left_;
            }
            if (opt_.top == nullptr) {
                opt_.top = &owned_top_;
            }
            if (opt_.width == nullptr) {
                opt_.width = &owned_width_;
            }
            if (opt_.height == nullptr) {
                opt_.height = &owned_height_;
            }
            if (opt_.inner != nullptr) {
                add_child(move(opt_.inner));
            }
        }

        element render() override {
            element inner_el = (active_child() != nullptr) ? active_child()->render() : element::text("");

            if (opt_.render) {
                window_options::render_state state;
                state.inner = inner_el;
                state.title = opt_.title;
                state.active = focused();
                return opt_.render(state);
            }

            style border_style;
            border_style.border = style::border::rounded;
            border_style.borderColor = focused() ? _NEFORCE color::cyan() : _NEFORCE color { 128, 128, 128 };
            using Padding = struct style::padding;
            border_style.padding = Padding{1, 1, 0, 1};

            style tmp;
            tmp.bold = true;
            element title_bar = element::text(opt_.title, tmp);
            element content = element::vbox({title_bar, element::separator(), inner_el});

            style window_style;
            window_style.border = border_style.border;
            window_style.borderColor = border_style.borderColor;
            window_style.padding = border_style.padding;
            window_style.width = style::size_hint{style::size_hint::fixed, *opt_.width};
            window_style.height = style::size_hint{style::size_hint::fixed, *opt_.height};
            using Margin = struct style::margin;
            window_style.margin = Margin{*opt_.top, 0, 0, *opt_.left};

            return content.with_style(window_style);
        }

        bool on_key(const key_event& e) override {
            using K = key_event::type;

            if ((e.mods & key_modifier::ctrl) != key_modifier::none) {
                int dx = 0, dy = 0;
                if (e.key == K::left) {
                    dx = -1;
                } else if (e.key == K::right) {
                    dx = 1;
                } else if (e.key == K::up) {
                    dy = -1;
                } else if (e.key == K::down) {
                    dy = 1;
                }

                if (dx != 0 || dy != 0) {
                    *opt_.left += dx;
                    *opt_.top += dy;
                    *opt_.left = max(*opt_.left, 0);
                    *opt_.top = max(*opt_.top, 0);
                    schedule_render();
                    return true;
                }
            }

            if ((e.mods & key_modifier::ctrl) != key_modifier::none &&
                (e.mods & key_modifier::shift) != key_modifier::none) {
                int dw = 0, dh = 0;
                if (e.key == K::left) {
                    dw = -1;
                } else if (e.key == K::right) {
                    dw = 1;
                } else if (e.key == K::up) {
                    dh = -1;
                } else if (e.key == K::down) {
                    dh = 1;
                }

                if (dw != 0 || dh != 0) {
                    *opt_.width += dw;
                    *opt_.height += dh;
                    *opt_.width = max(*opt_.width, 5);
                    *opt_.height = max(*opt_.height, 3);
                    schedule_render();
                    return true;
                }
            }

            if (active_child() != nullptr) {
                return active_child()->on_key(e);
            }
            return false;
        }

        bool on_mouse(const mouse_event& e) override {
            if (active_child() != nullptr) {
                return active_child()->on_mouse(e);
            }
            return false;
        }

        NEFORCE_NODISCARD bool focusable() const override { return true; }

    private:
        window_options opt_;
        int owned_left_;
        int owned_top_;
        int owned_width_;
        int owned_height_;
    };
} // anonymous namespace


unique_ptr<component_base> window(window_options options) { return make_unique<window_component>(move(options)); }

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
