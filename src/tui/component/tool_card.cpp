#include <NeForce/core/string/to_string.hpp>
#include <NeForce/core/utility/packages.hpp>
#include <NeForce/tui/component/tool_card.hpp>
#include <NeForce/tui/dom/spinner.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__
NEFORCE_BEGIN_COMPONENTS__

namespace {
    size_t count_newlines(const string_view text) {
        size_t count = 0;
        for (size_t i = 0; i < text.size(); ++i) {
            if (text[i] == '\n') {
                ++count;
            }
        }
        return count;
    }
} // namespace


tool_card::tool_card(tool_card_option opt) :
opt_(move(opt)) {}

void tool_card::setup() {
    output_block_option out_opt;
    out_opt.collapsed = opt_.collapsed;
    out_opt.hidden_lines = opt_.hidden_lines;
    out_opt.collapsed_format = "\xe2\x80\xa6 %d lines hidden \xc2\xb7 enter to expand";

    auto owned = make_unique<output_block>(move(out_opt));
    output_ = owned.get();
    add_child(move(owned));
}

void tool_card::set_status(const tool_status status, const int64_t elapsed_ms) {
    if (elapsed_ms >= 0) {
        opt_.elapsed_ms = elapsed_ms;
    }
    if (opt_.status == status) {
        schedule_render();
        return;
    }
    opt_.status = status;
    if (status != tool_status::running) {
        frame_ = 0;
        frame_accum_ = 0;
    }
    schedule_render();
}

void tool_card::set_summary(string summary) {
    opt_.summary = move(summary);
    schedule_render();
}

void tool_card::set_output(string text) {
    element content = element::text(move(text), tui::style{}, style::wrap_mode::none);
    const size_t lines = content.text().empty() ? 0 : static_cast<size_t>(1 + count_newlines(content.text().view()));
    set_output_element(move(content), lines);
}

void tool_card::set_output_element(element content, const size_t hidden_lines) {
    opt_.hidden_lines = hidden_lines;
    if (output_ != nullptr) {
        output_->set_body(move(content));
        output_->set_hidden_lines(hidden_lines);
    }
    schedule_render();
}

void tool_card::set_collapsed(const bool collapsed) {
    opt_.collapsed = collapsed;
    if (output_ != nullptr) {
        output_->set_collapsed(collapsed);
    }
    schedule_render();
}

string tool_card::status_icon() {
    switch (opt_.status) {
        case tool_status::running: {
            // Take a copy: element::text() returns a reference into the element's shared node,
            // and the temporary returned by spinner() releases that node at the end of this statement,
            // which would leave a dangling reference behind.
            string frame = spinner(3, frame_).text();
            return frame.empty() ? string(".") : move(frame);
        }
        case tool_status::success: {
            return "\xe2\x9c\x94";
        }
        case tool_status::failed: {
            return "\xe2\x9c\x96";
        }
        case tool_status::interrupted: {
            return "\xe2\x8f\xb9";
        }
    }
    return "?";
}

tui::style tool_card::status_style() const {
    tui::style st;
    switch (opt_.status) {
        case tool_status::running: {
            st.fg = _NEFORCE color::yellow();
            break;
        }
        case tool_status::success: {
            st.fg = _NEFORCE color::green();
            break;
        }
        case tool_status::failed: {
            st.fg = _NEFORCE color::red();
            break;
        }
        case tool_status::interrupted: {
            st.fg = _NEFORCE color::yellow();
            break;
        }
    }
    st.bold = true;
    return st;
}

element tool_card::render() {
    elements head;

    const tui::style icon_style = status_style();
    head.push_back(
            element::rich_text({text_span{status_icon() + " ", icon_style}}, icon_style, style::wrap_mode::none));

    tui::style title_style = opt_.title_style;
    if (!title_style.bold.has_value()) {
        title_style.bold = true;
    }
    head.push_back(element::rich_text({text_span{opt_.title, title_style}}, title_style, style::wrap_mode::none));

    if (!opt_.summary.empty()) {
        tui::style summary_style = opt_.summary_style;
        if (!summary_style.dim.has_value()) {
            summary_style.dim = true;
        }
        head.push_back(element::rich_text({text_span{"  " + opt_.summary, summary_style}}, summary_style,
                                          style::wrap_mode::none));
    }

    if (opt_.elapsed_ms > 0) {
        tui::style elapsed_style = opt_.summary_style;
        if (!elapsed_style.dim.has_value()) {
            elapsed_style.dim = true;
        }
        const int64_t seconds = opt_.elapsed_ms / 1000;
        const int64_t tenths = (opt_.elapsed_ms % 1000) / 100;
        head.push_back(element::rich_text(
                {text_span{"  " + to_string(seconds) + "." + to_string(tenths) + "s", elapsed_style}}, elapsed_style,
                style::wrap_mode::none));
    }

    if (output_ != nullptr) {
        // render() must not mutate a child unconditionally:
        // every setter call schedules another frame,
        // and the reconciler would flush without ever converging.
        string key = status_icon();
        key += '\x1f';
        key += opt_.title;
        key += '\x1f';
        key += opt_.summary;
        key += '\x1f';
        key += to_string(static_cast<int64_t>(opt_.status));
        if (opt_.elapsed_ms > 0) {
            key += '\x1f';
            key += to_string(opt_.elapsed_ms);
        }
        if (key != header_key_) {
            header_key_ = move(key);
            output_->set_header(element::hbox(move(head)));
        }
        return output_->render();
    }

    return element::hbox(move(head));
}

bool tool_card::on_key(const key_event& e) {
    if (output_ == nullptr) {
        return false;
    }
    if (output_->on_key(e)) {
        opt_.collapsed = output_->collapsed();
        return true;
    }
    return false;
}

void tool_card::on_animation(const int64_t delta) {
    if (opt_.status != tool_status::running) {
        return;
    }
    opt_.elapsed_ms += delta;
    frame_accum_ += delta;
    constexpr int64_t interval = 90;
    if (frame_accum_ >= interval) {
        frame_accum_ %= interval;
        ++frame_;
        schedule_render();
    }
}

unique_ptr<component_base> tool_card_component(tool_card_option opt) { return make_unique<tool_card>(move(opt)); }

NEFORCE_END_COMPONENTS__
NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
