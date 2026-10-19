#include <NeForce/tui/component/approve_prompt.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__
NEFORCE_BEGIN_COMPONENTS__

namespace {
    approve_result choice_result(const int index) noexcept {
        if (index == 0) {
            return approve_result::approved;
        }
        if (index == 1) {
            return approve_result::rejected;
        }
        return approve_result::approved_always;
    }
} // namespace


approve_prompt_view::approve_prompt_view(approve_prompt_option opt) :
opt_(move(opt)),
highlight_(opt_.default_approve ? 0 : 1) {}

void approve_prompt_view::set_message(string message) {
    opt_.message = move(message);
    schedule_render();
}

approve_result approve_prompt_view::result() const noexcept {
    return (opt_.result != nullptr) ? opt_.result->value() : own_result_;
}

void approve_prompt_view::decide(const approve_result value) {
    own_result_ = value;
    if (opt_.result != nullptr) {
        *opt_.result = value;
    }
    if (opt_.on_decide) {
        opt_.on_decide(value);
    }
    schedule_render();
}

element approve_prompt_view::render_choice(const int index) const {
    string label;
    switch (index) {
        case 0: {
            label = opt_.approve_label;
            break;
        }
        case 1: {
            label = opt_.reject_label;
            break;
        }
        default: {
            label = opt_.always_label;
            break;
        }
    }

    tui::style st;
    if (index == highlight_) {
        st.reverse = true;
        st.bold = true;
    } else {
        st.dim = true;
    }
    return element::text("  " + label + "  ", st, style::wrap_mode::none);
}

element approve_prompt_view::render() {
    elements children;

    tui::style title_style = opt_.style;
    title_style.bold = true;
    children.push_back(element::text(opt_.title, title_style));

    if (!opt_.message.empty()) {
        tui::style message_style;
        message_style.dim = true;
        children.push_back(
                element::rich_text({text_span{opt_.message, message_style}}, style{}, style::wrap_mode::word));
    }

    elements choices;
    for (int i = 0; i < choice_count(); ++i) {
        choices.push_back(render_choice(i));
    }
    children.push_back(element::hbox(move(choices)));

    tui::style panel_style = opt_.style;
    if (!panel_style.border.has_value()) {
        panel_style.border = style::border::rounded;
    }
    if (!panel_style.width.has_value()) {
        panel_style.width = style::size_hint{style::size_hint::fixed, max(10, opt_.width)};
    }
    using padding = struct style::padding;
    if (!panel_style.padding.has_value()) {
        panel_style.padding = padding{1, 1, 1, 1};
    }

    return element::panel(element::vbox(move(children)), panel_style);
}

bool approve_prompt_view::on_key(const key_event& e) {
    using K = key_event::type;
    const int count = choice_count();

    switch (e.key) {
        case K::left: {
            highlight_ = (highlight_ - 1 + count) % count;
            schedule_render();
            return true;
        }
        case K::right:
        case K::tab: {
            highlight_ = (highlight_ + 1) % count;
            schedule_render();
            return true;
        }
        case K::enter: {
            decide(choice_result(highlight_));
            return true;
        }
        case K::escape: {
            decide(approve_result::rejected);
            return true;
        }
        case K::printable: {
            if ((e.mods & key_modifier::ctrl) != key_modifier::none) {
                return false;
            }
            const auto value = e.cp.value();
            if (value >= '1' && compare_less(value, '1' + count)) {
                const int index = static_cast<int>(value - '1');
                highlight_ = index;
                decide(choice_result(index));
                return true;
            }
            return false;
        }
        default: {
            return false;
        }
    }
}

bool approve_prompt_view::on_mouse(const mouse_event& e) {
    if (e.action != mouse_action::press) {
        return false;
    }
    if (e.y < 0) {
        return false;
    }
    highlight_ = (highlight_ + 1) % choice_count();
    schedule_render();
    return true;
}

unique_ptr<component_base> approve_prompt(approve_prompt_option opt) {
    return make_unique<approve_prompt_view>(move(opt));
}

NEFORCE_END_COMPONENTS__
NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
