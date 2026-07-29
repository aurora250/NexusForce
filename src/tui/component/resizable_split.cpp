#include <NeForce/tui/component/resizable_split.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__
NEFORCE_BEGIN_COMPONENTS__

namespace {
    enum class split_direction {
        left,
        right,
        top,
        bottom,
    };

    class resizable_split_component final : public component_base {
    public:
        resizable_split_component(unique_ptr<component_base> main, unique_ptr<component_base> back, int* main_size,
                                  split_direction dir) :
        main_size_(main_size),
        direction_(dir) {
            component_base* raw = main.get();
            add_child(move(main));
            add_child(move(back));
            set_active_child(raw);
        }

        element render() override {
            const element main_el = children_[0]->render();
            const element back_el = children_[1]->render();

            const string sep_char =
                    (direction_ == split_direction::left || direction_ == split_direction::right) ? "│" : "─";

            if (direction_ == split_direction::left || direction_ == split_direction::right) {
                element left = (direction_ == split_direction::left) ? main_el : back_el;
                element right = (direction_ == split_direction::right) ? main_el : back_el;
                return element::hbox({left, element::text(sep_char), right});
            }
            element top = (direction_ == split_direction::top) ? main_el : back_el;
            element bottom = (direction_ == split_direction::bottom) ? main_el : back_el;
            return element::vbox({top, element::text(sep_char), bottom});
        }

        bool on_key(const key_event& e) override {
            using K = key_event::type;
            const bool is_vertical = (direction_ == split_direction::left || direction_ == split_direction::right);

            if (e.key == K::F6 || (e.key == K::tab && (e.mods & key_modifier::none) == key_modifier::none)) {
                if (active_child() == children_[0].get()) {
                    set_active_child(children_[1].get());
                } else {
                    set_active_child(children_[0].get());
                }
                schedule_render();
                return true;
            }

            if ((e.mods & key_modifier::ctrl) != key_modifier::none) {
                int delta = 0;
                if (is_vertical) {
                    if (e.key == K::left) {
                        delta = -1;
                    } else if (e.key == K::right) {
                        delta = 1;
                    }
                } else {
                    if (e.key == K::up) {
                        delta = -1;
                    } else if (e.key == K::down) {
                        delta = 1;
                    }
                }
                if (delta != 0 && main_size_ != nullptr) {
                    *main_size_ += delta;
                    *main_size_ = max(*main_size_, 1);
                    schedule_render();
                    return true;
                }
            }

            return active_child()->on_key(e);
        }

        NEFORCE_NODISCARD bool focusable() const override { return true; }

    private:
        int* main_size_;
        split_direction direction_;
    };
} // anonymous namespace


unique_ptr<component_base> resizable_split_left(unique_ptr<component_base> main, unique_ptr<component_base> back,
                                                int* main_size) {
    return make_unique<resizable_split_component>(move(main), move(back), main_size, split_direction::left);
}

unique_ptr<component_base> resizable_split_right(unique_ptr<component_base> main, unique_ptr<component_base> back,
                                                 int* main_size) {
    return make_unique<resizable_split_component>(move(main), move(back), main_size, split_direction::right);
}

unique_ptr<component_base> resizable_split_top(unique_ptr<component_base> main, unique_ptr<component_base> back,
                                               int* main_size) {
    return make_unique<resizable_split_component>(move(main), move(back), main_size, split_direction::top);
}

unique_ptr<component_base> resizable_split_bottom(unique_ptr<component_base> main, unique_ptr<component_base> back,
                                                  int* main_size) {
    return make_unique<resizable_split_component>(move(main), move(back), main_size, split_direction::bottom);
}

NEFORCE_END_COMPONENTS__
NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
