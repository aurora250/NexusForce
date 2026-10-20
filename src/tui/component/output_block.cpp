#include <NeForce/core/string/to_string.hpp>
#include <NeForce/core/utility/packages.hpp>
#include <NeForce/tui/component/output_block.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__
NEFORCE_BEGIN_COMPONENTS__


output_block::output_block(output_block_option opt) :
opt_(move(opt)) {}

void output_block::setup() { rect_ = &create_state<layout_rect>(layout_rect{}); }

void output_block::set_header(element header) {
    opt_.header = move(header);
    schedule_render();
}

void output_block::set_body(element body) {
    opt_.body = move(body);
    schedule_render();
}

void output_block::set_collapsed(const bool collapsed) {
    if (opt_.collapsed == collapsed) {
        return;
    }
    opt_.collapsed = collapsed;
    schedule_render();
}

void output_block::toggle() { set_collapsed(!opt_.collapsed); }

void output_block::set_hidden_lines(const size_t lines) {
    if (opt_.hidden_lines == lines) {
        return;
    }
    opt_.hidden_lines = lines;
    schedule_render();
}

element output_block::render_hint() const {
    tui::style st = opt_.hint_style;
    if (!st.dim.has_value()) {
        st.dim = true;
    }
    string text;
    const auto& raw = opt_.collapsed_format.view();
    for (size_t i = 0; i < raw.size(); ++i) {
        if (raw[i] == '%' && i + 1 < raw.size() && raw[i + 1] == 'd') {
            text += to_string(static_cast<int64_t>(opt_.hidden_lines));
            ++i;
            continue;
        }
        text += raw[i];
    }
    return element::rich_text({text_span{" " + text, st}}, st, style::wrap_mode::none);
}

element output_block::render() {
    elements parts;

    if (opt_.header.kind() != element::kind::empty) {
        elements head_cells;
        const string marker = opt_.collapsed ? "\xe2\x96\xb8 " : "\xe2\x96\xbe ";
        head_cells.push_back(element::text(marker, opt_.hint_style, style::wrap_mode::none));
        head_cells.push_back(opt_.header);
        parts.push_back(element::hbox(move(head_cells)));
    }

    if (opt_.collapsed) {
        if (opt_.hidden_lines > 0) {
            parts.push_back(render_hint());
        }
    } else if (opt_.body.kind() != element::kind::empty) {
        parts.push_back(opt_.body);
    }

    tui::style style = opt_.style;
    using padding = struct style::padding;
    if (!style.padding.has_value()) {
        style.padding = padding{0, 1, 0, 1};
    }

    element body = element::band(element::vbox(move(parts)), style);
    body = body.with_rect_state(rect_);
    return body;
}

bool output_block::on_key(const key_event& e) {
    using K = key_event::type;
    switch (e.key) {
        case K::enter:
        case K::right: {
            set_collapsed(false);
            return true;
        }
        case K::left: {
            set_collapsed(true);
            return true;
        }
        case K::printable: {
            if (e.cp.value() == ' ') {
                toggle();
                return true;
            }
            return false;
        }
        default: {
            return false;
        }
    }
}

bool output_block::on_mouse(const mouse_event& e) {
    if (e.action != mouse_action::press) {
        return false;
    }
    toggle();
    return true;
}

unique_ptr<component_base> output_block_component(output_block_option opt) {
    return make_unique<output_block>(move(opt));
}

NEFORCE_END_COMPONENTS__
NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
