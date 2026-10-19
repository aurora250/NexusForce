#include <NeForce/tui/component/status_indicator.hpp>
#include <NeForce/tui/dom/spinner.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__
NEFORCE_BEGIN_COMPONENTS__

status_indicator_view::status_indicator_view(status_indicator_option opt) :
opt_(move(opt)) {}

void status_indicator_view::set_label(string label) {
    opt_.label = move(label);
    schedule_render();
}

void status_indicator_view::set_active(const bool active) {
    if (opt_.active == active) {
        return;
    }
    opt_.active = active;
    if (active) {
        restart();
    }
    schedule_render();
}

void status_indicator_view::restart() {
    frame_ = 0;
    frame_accum_ = 0;
    elapsed_ms_ = 0;
}

void status_indicator_view::on_animation(const int64_t delta) {
    if (!opt_.active || delta <= 0) {
        return;
    }
    elapsed_ms_ += delta;
    frame_accum_ += delta;
    const int interval = max(16, opt_.frame_interval_ms);
    if (frame_accum_ >= interval) {
        frame_accum_ %= interval;
        ++frame_;
        schedule_render();
    }
}

element status_indicator_view::render() {
    if (!opt_.active && !keep_visible_) {
        return element::empty();
    }

    vector<text_span> spans;
    const string frame_text = spinner(opt_.charset, frame_).text();
    spans.push_back(text_span{frame_text, opt_.spinner_style});
    if (!opt_.label.empty()) {
        spans.push_back(text_span{" " + opt_.label, opt_.label_style});
    }
    if (opt_.show_elapsed) {
        const int64_t seconds = elapsed_ms_ / 1000;
        const int64_t tenths = (elapsed_ms_ % 1000) / 100;
        spans.push_back(text_span{" " + to_string(seconds) + "." + to_string(tenths) + "s", opt_.elapsed_style});
    }

    return element::rich_text(move(spans), style{}, style::wrap_mode::none);
}

unique_ptr<component_base> status_indicator(status_indicator_option opt) {
    return make_unique<status_indicator_view>(move(opt));
}

NEFORCE_END_COMPONENTS__
NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
