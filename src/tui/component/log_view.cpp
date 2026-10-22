#include <NeForce/core/string/to_string.hpp>
#include <NeForce/core/utility/packages.hpp>
#include <NeForce/core/system/console.hpp>
#include <NeForce/tui/component/log_view.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__
NEFORCE_BEGIN_COMPONENTS__

namespace {
    log_role_style default_style(const log_role role) {
        log_role_style style;
        style.title.bold = true;
        switch (role) {
            case log_role::user: {
                style.name = "You";
                style.title.fg = _NEFORCE color::cyan();
                style.band.bg = _NEFORCE color{32, 36, 44};
                style.banded = true;
                break;
            }
            case log_role::assistant: {
                style.name = "Assistant";
                style.title.fg = _NEFORCE color::green();
                break;
            }
            case log_role::tool: {
                style.name = "Tool";
                style.title.fg = _NEFORCE color::yellow();
                break;
            }
            case log_role::system: {
                style.name = "System";
                style.title.dim = true;
                break;
            }
            case log_role::notice: {
                style.name = "Notice";
                style.title.fg = _NEFORCE color::yellow();
                break;
            }
        }
        return style;
    }

    element gap_rows(const int gap) {
        if (gap <= 0) {
            return element::empty();
        }
        return element::text(string(static_cast<size_t>(gap - 1), '\n'), style{}, style::wrap_mode::none);
    }
} // namespace


log_view::log_view(log_view_option opt) :
opt_(move(opt)),
cache_(opt_.markdown, opt_.cache_capacity) {}

void log_view::setup() {
    virtual_list_option list_opt;
    list_opt.item_count = 0;
    list_opt.item_height = max(1, opt_.height_hint);
    list_opt.variable_height = true;
    list_opt.auto_follow = opt_.auto_follow;
    list_opt.render_item = [this](const size_t index) { return render_message(index); };
    // A virtual list has no content height of its own;
    // inside a column it has to fill the remaining space explicitly.
    list_opt.style.height = style::size_hint{style::size_hint::fill, 1};

    auto owned = make_unique<virtual_list_view>(list_opt);
    list_ = owned.get();
    add_child(move(owned));
}

const log_role_style& log_view::style_of(const log_role role) const {
    static const log_role_style fallback = default_style(log_role::assistant);
    const auto index = static_cast<size_t>(role);
    if (index < opt_.roles.size()) {
        return opt_.roles[index];
    }
    static const vector<log_role_style> defaults = [] {
        vector<log_role_style> list;
        for (int i = 0; i <= static_cast<int>(log_role::notice); ++i) {
            list.push_back(default_style(static_cast<log_role>(i)));
        }
        return list;
    }();
    if (index < defaults.size()) {
        return defaults[index];
    }
    return fallback;
}

log_view::message* log_view::find(const uint64_t id) {
    for (auto& item: messages_) {
        if (item.id == id) {
            return &item;
        }
    }
    return nullptr;
}

void log_view::mark_arrived() {
    // Count arrivals only while the user has scrolled away from the bottom:
    // with variable item heights the geometric "at bottom" test is still based on
    // estimated heights before the first measurement,
    // so follow intent is the reliable signal here.
    if (opt_.show_pending_hint && list_ != nullptr && !list_->auto_follow()) {
        ++pending_;
    }
}

uint64_t log_view::append(const log_role role, string content, string title) {
    message item;
    item.id = next_id_++;
    item.role = role;
    item.title = move(title);
    item.content = move(content);
    item.streaming = true;

    mark_arrived();
    messages_.push_back(move(item));

    if (list_ != nullptr) {
        list_->set_item_count(messages_.size());
    }
    schedule_render();
    return next_id_ - 1;
}

void log_view::append_chunk(const uint64_t id, const string_view chunk) {
    message* item = find(id);
    if (item == nullptr || chunk.empty()) {
        return;
    }
    item->pending.append(chunk);
    if (list_ != nullptr && list_->at_bottom() && list_->auto_follow()) {
        list_->scroll_to_bottom();
    }
}

void log_view::commit() {
    bool changed = false;
    for (auto& item: messages_) {
        if (item.pending.empty()) {
            continue;
        }
        item.content.append(item.pending);
        item.pending.clear();
        changed = true;
    }
    if (changed) {
        schedule_render();
    }
}

void log_view::finish(const uint64_t id) {
    commit();
    message* item = find(id);
    if (item == nullptr) {
        return;
    }
    item->streaming = false;
    schedule_render();
}

void log_view::set_content(const uint64_t id, string content) {
    message* item = find(id);
    if (item == nullptr) {
        return;
    }
    item->content = move(content);
    item->pending.clear();
    schedule_render();
}

void log_view::set_title(const uint64_t id, string title) {
    message* item = find(id);
    if (item == nullptr) {
        return;
    }
    item->title = move(title);
    schedule_render();
}

void log_view::modify(const uint64_t id, const function<void(log_role&, string&, string&, bool&)>& mutate) {
    message* item = find(id);
    if (item == nullptr) {
        return;
    }
    mutate(item->role, item->title, item->content, item->streaming);
    schedule_render();
}

void log_view::clear() {
    messages_.clear();
    pending_ = 0;
    cache_.clear();
    if (list_ != nullptr) {
        list_->set_item_count(0);
        list_->invalidate_heights();
    }
    schedule_render();
}

void log_view::scroll_to_bottom() {
    if (list_ != nullptr) {
        list_->scroll_to_bottom();
    }
    pending_ = 0;
}

void log_view::scroll_to_message(const uint64_t id) {
    for (size_t i = 0; i < messages_.size(); ++i) {
        if (messages_[i].id == id) {
            if (list_ != nullptr) {
                int offset = 0;
                for (size_t j = 0; j < i; ++j) {
                    offset += list_->measured_height(j);
                }
                list_->scroll_to(offset);
            }
            return;
        }
    }
}

bool log_view::at_bottom() const { return list_ == nullptr || list_->at_bottom(); }

void log_view::set_auto_follow(const bool enable) {
    if (list_ != nullptr) {
        list_->set_auto_follow(enable);
    }
}

bool log_view::auto_follow() const { return list_ == nullptr || list_->auto_follow(); }

element log_view::render_message(const size_t index) {
    if (index >= messages_.size()) {
        return element::empty();
    }

    const message& item = messages_[index];
    const auto& role_style = style_of(item.role);

    elements parts;

    const string header = item.title.empty() ? role_style.name : item.title;
    if (!header.empty()) {
        const tui::style title_style = role_style.title;
        parts.push_back(element::rich_text({text_span{header, title_style}}, title_style, style::wrap_mode::none));
    }

    const string_view content = item.content.view();
    if (!content.empty()) {
        const int width = opt_.markdown.width;
        if (item.streaming) {
            parts.push_back(markdown(content, opt_.markdown));
        } else {
            parts.push_back(cache_.render(content, width));
        }
    }

    element body = element::vbox(move(parts));
    if (role_style.banded) {
        tui::style band_style = role_style.band;
        using padding = struct style::padding;
        if (!band_style.padding.has_value()) {
            band_style.padding = padding{0, 1, 0, 1};
        }
        body = element::band(move(body), band_style);
    }

    elements rows;
    rows.push_back(move(body));
    element tail = gap_rows(opt_.gap);
    if (tail.kind() != element::kind::empty) {
        rows.push_back(move(tail));
    }
    return element::vbox(move(rows));
}

element log_view::render() {
    if (list_ == nullptr) {
        return element::empty();
    }

    elements parts;
    if (opt_.show_pending_hint && pending_ > 0) {
        tui::style hint;
        hint.dim = true;
        parts.push_back(element::text("\xe2\x86\x93 " + to_string(static_cast<int64_t>(pending_)) + " new", hint,
                                      style::wrap_mode::none));
    }
    parts.push_back(list_->render());
    return element::vbox(move(parts));
}

bool log_view::on_key(const key_event& e) {
    if (list_ == nullptr) {
        return false;
    }
    const bool handled = list_->on_key(e);
    if (handled && list_->at_bottom()) {
        pending_ = 0;
    }
    return handled;
}

uint64_t log_view::message_at(const int y) const {
    if (list_ == nullptr || messages_.empty()) {
        return 0;
    }

    const auto box = list_->viewport_rect();
    if (box.h <= 0 || y < box.y || y >= box.y + box.h) {
        return 0;
    }

    int offset = 0;
    const int target = list_->scroll_offset() + (y - box.y);
    for (size_t i = 0; i < messages_.size(); ++i) {
        const int measured = list_->measured_height(i);
        const int rows = measured > 0 ? measured : max(1, list_->item_height());
        if (target < offset + rows) {
            return messages_[i].id;
        }
        offset += rows;
    }
    return 0;
}

bool log_view::on_mouse(const mouse_event& e) {
    if (e.action != mouse_action::press || e.button != mouse_button::left) {
        return false;
    }
    const uint64_t id = message_at(e.y);
    if (id == 0) {
        return false;
    }
    selected_ = id;
    if (opt_.on_message_click) {
        opt_.on_message_click(id);
    }
    schedule_render();
    return true;
}

void log_view::select_message(const uint64_t id) {
    selected_ = id;
    schedule_render();
}

bool log_view::copy_message(const uint64_t id) {
    for (const auto& item: messages_) {
        if (item.id != id) {
            continue;
        }
        if (item.content.empty()) {
            return false;
        }
        console.copy_to_clipboard(item.content.view());
        return true;
    }
    return false;
}

bool log_view::copy_selected() { return copy_message(selected_); }

void log_view::on_animation(const int64_t delta) {
    ignore = delta;
    commit();
}

unique_ptr<component_base> log_view_component(log_view_option opt) { return make_unique<log_view>(move(opt)); }

NEFORCE_END_COMPONENTS__
NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
