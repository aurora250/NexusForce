#include <NeForce/tui/component/composer.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__
NEFORCE_BEGIN_COMPONENTS__

composer::composer(composer_option opt) :
opt_(move(opt)) {}

void composer::setup() {
    if (opt_.text == nullptr) {
        text_storage_ = &create_state<string>(string());
        opt_.text = text_storage_;
    }

    text_area_option area_opt;
    area_opt.text = opt_.text;
    area_opt.placeholder = opt_.placeholder;
    area_opt.style = opt_.style;
    area_opt.wrap = style::wrap_mode::word;
    area_opt.submit_on_enter = false;
    area_opt.on_change = [this](const string&) {
        if (popup_ != nullptr) {
            refresh_completions();
        }
    };

    auto area = make_unique<text_area_view>(area_opt);
    input_ = area.get();
    add_child(move(area));
}

void composer::take_focus() {
    component_base::take_focus();
    if (input_ != nullptr) {
        input_->set_has_focus(true);
    }
}

const string& composer::text() const {
    static const string empty_text;
    if (input_ == nullptr) {
        return empty_text;
    }
    return input_->text();
}

void composer::set_phase(const composer_phase phase) {
    if (phase_ == phase) {
        return;
    }
    phase_ = phase;
    schedule_render();
}

void composer::set_hint(string hint) {
    hint_override_ = move(hint);
    schedule_render();
}

void composer::set_text(string value) {
    if (input_ != nullptr) {
        input_->set_text(move(value));
    }
}

void composer::submit() {
    if (input_ == nullptr) {
        return;
    }
    const string payload = input_->text();
    if (payload.empty()) {
        return;
    }
    if (opt_.on_submit) {
        opt_.on_submit(payload);
    }
    input_->set_text(string());
    close_completion();
}

bool composer::extract_query(string& query) const {
    if (input_ == nullptr || opt_.trigger == 0) {
        return false;
    }

    const string_view content = input_->text().view();
    const size_t cursor = min(input_->cursor(), content.size());

    size_t begin = cursor;
    while (begin > 0) {
        const char previous = content[begin - 1];
        if (previous == ' ' || previous == '\t' || previous == '\n') {
            return false;
        }
        const auto as_char32 = static_cast<char32_t>(static_cast<unsigned char>(previous));
        if (as_char32 == opt_.trigger) {
            break;
        }
        --begin;
    }
    if (begin == 0) {
        return false;
    }

    const size_t trigger_pos = begin - 1;
    if (content[trigger_pos] != static_cast<char>(opt_.trigger)) {
        return false;
    }
    if (opt_.trigger_at_line_start && trigger_pos > 0 && content[trigger_pos - 1] != '\n') {
        return false;
    }

    query = string(content.substr(begin, cursor - begin));
    return true;
}

void composer::refresh_completions() {
    string query;
    if (!extract_query(query) || !opt_.complete) {
        close_completion();
        return;
    }

    completion_request request;
    request.query = query;
    request.cursor = input_ != nullptr ? input_->cursor() : 0;

    vector<completion_item> items;
    opt_.complete(request, items);
    if (items.empty()) {
        close_completion();
        return;
    }

    vector<string> labels;
    labels.reserve(items.size());
    for (const auto& item: items) {
        labels.push_back(item.detail.empty() ? item.label : item.label + "  " + item.detail);
    }

    if (popup_ == nullptr) {
        select_list_option popup_opt;
        popup_opt.visible_rows = max(1, min(opt_.visible_completions, static_cast<int>(labels.size())));
        popup_opt.items = move(labels);
        popup_opt.filterable = false;
        popup_opt.on_select = [this](const int) { accept_completion(); };
        auto popup = make_unique<select_list_view>(popup_opt);
        popup_ = popup.get();
        add_child(move(popup));
    } else {
        popup_->set_visible_rows(max(1, min(opt_.visible_completions, static_cast<int>(labels.size()))));
        popup_->set_items(move(labels));
    }
    popup_->set_selected_position(0);
}

void composer::close_completion() {
    if (popup_ == nullptr) {
        return;
    }
    auto owned = remove_child(popup_);
    popup_ = nullptr;
    ignore = owned;
    schedule_render();
}

bool composer::request_completion() {
    refresh_completions();
    return popup_ != nullptr;
}

void composer::accept_completion() {
    if (popup_ == nullptr || input_ == nullptr) {
        return;
    }

    string query;
    if (!extract_query(query)) {
        close_completion();
        return;
    }

    vector<completion_item> items;
    if (opt_.complete) {
        completion_request request;
        request.query = query;
        request.cursor = input_->cursor();
        opt_.complete(request, items);
    }

    const int position = popup_->selected_position();
    if (position < 0 || static_cast<size_t>(position) >= items.size()) {
        close_completion();
        return;
    }

    const auto& item = items[static_cast<size_t>(position)];
    const string insertion = item.insert.empty() ? item.label : item.insert;

    const string_view content = input_->text().view();
    const size_t cursor = min(input_->cursor(), content.size());
    size_t begin = cursor;
    while (begin > 0) {
        const char previous = content[begin - 1];
        if (previous == ' ' || previous == '\t' || previous == '\n') {
            break;
        }
        --begin;
    }
    const size_t replace_from = (begin > 0) ? (begin - 1) : 0;

    string before(content.substr(0, replace_from));
    const string_view tail = content.substr(cursor);
    before.append(insertion.view());
    const size_t new_cursor = before.size();
    before.append(tail);

    input_->set_text(move(before));
    input_->set_cursor(new_cursor);
    close_completion();
}

string composer::current_hint() const {
    if (!hint_override_.empty()) {
        return hint_override_;
    }
    if (phase_ == composer_phase::running) {
        return opt_.running_hint.empty() ? string("esc to interrupt") : opt_.running_hint;
    }
    return opt_.hint.empty() ? string("enter send \xc2\xb7 shift+enter newline") : opt_.hint;
}

element composer::render() {
    if (input_ == nullptr) {
        return element::empty();
    }

    elements parts;

    if (popup_ != nullptr) {
        parts.push_back(popup_->render());
    }

    tui::style input_style = opt_.style;
    using padding = struct style::padding;
    if (!input_style.padding.has_value()) {
        input_style.padding = padding{0, 1, 0, 1};
    }
    if (!input_style.height.has_value()) {
        input_style.height = style::size_hint{style::size_hint::auto_, 0};
    }

    element editor = input_->render();
    editor = element::band(move(editor), input_style);
    parts.push_back(move(editor));

    tui::style bar = opt_.bar_style;
    if (!bar.dim.has_value()) {
        bar.dim = true;
    }
    const auto phase_color = (phase_ == composer_phase::running) ? _NEFORCE color::yellow() : bar.fg;
    tui::style left_style = bar;
    if (phase_color.has_value()) {
        left_style.fg = phase_color;
    }

    parts.push_back(element::rich_text({text_span{" " + current_hint(), left_style}}, bar, style::wrap_mode::none));

    return element::vbox(move(parts));
}

bool composer::on_key(const key_event& e) {
    using K = key_event::type;
    const bool shift = (e.mods & key_modifier::shift) != key_modifier::none;
    const bool ctrl = (e.mods & key_modifier::ctrl) != key_modifier::none;

    if (e.key == K::escape) {
        if (popup_ != nullptr) {
            close_completion();
            return true;
        }
        if (phase_ == composer_phase::running) {
            if (opt_.on_interrupt) {
                opt_.on_interrupt();
            }
            return true;
        }
        return false;
    }

    if (popup_ != nullptr) {
        switch (e.key) {
            case K::up:
            case K::down:
            case K::page_up:
            case K::page_down:
            case K::home:
            case K::end: {
                return popup_->on_key(e);
            }
            case K::tab: {
                accept_completion();
                return true;
            }
            case K::enter: {
                accept_completion();
                return true;
            }
            default: {
                break;
            }
        }
    }

    if (e.key == K::enter) {
        if (shift) {
            return input_->on_key(e);
        }
        submit();
        return true;
    }

    const bool handled = input_->on_key(e);
    if (handled) {
        string query;
        if (extract_query(query)) {
            refresh_completions();
        } else if (popup_ != nullptr) {
            close_completion();
        }
    }
    ignore = ctrl;
    ignore = shift;
    return handled;
}

bool composer::on_paste(const paste_event& e) {
    if (input_ == nullptr) {
        return false;
    }
    return input_->on_paste(e);
}

unique_ptr<component_base> composer_component(composer_option opt) { return make_unique<composer>(move(opt)); }

NEFORCE_END_COMPONENTS__
NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
