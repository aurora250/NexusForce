#include <NeForce/tui/component/text_area.hpp>
#include <NeForce/core/system/console.hpp>
#include <NeForce/tui/dom/text_layout.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__
NEFORCE_BEGIN_COMPONENTS__

namespace {
    size_t utf8_next(const string_view text, const size_t pos) noexcept {
        if (pos >= text.size()) {
            return text.size();
        }
        const auto lead = static_cast<byte_t>(text[pos]);
        if ((lead & 0x80) == 0) {
            return pos + 1;
        }
        if ((lead & 0xE0) == 0xC0) {
            return min(pos + 2, text.size());
        }
        if ((lead & 0xF0) == 0xE0) {
            return min(pos + 3, text.size());
        }
        if ((lead & 0xF8) == 0xF0) {
            return min(pos + 4, text.size());
        }
        return pos + 1;
    }

    size_t utf8_prev(const string_view text, const size_t pos) noexcept {
        if (pos == 0) {
            return 0;
        }
        size_t p = pos - 1;
        while (p > 0 && (static_cast<byte_t>(text[p]) & 0xC0) == 0x80) {
            --p;
        }
        return p;
    }

    bool is_space_at(const string_view text, const size_t pos) noexcept {
        return pos < text.size() && (text[pos] == ' ' || text[pos] == '\t' || text[pos] == '\n');
    }

    bool is_word_at(const string_view text, const size_t pos) noexcept {
        return pos < text.size() && !is_space_at(text, pos);
    }
} // namespace


text_area_view::text_area_view(text_area_option opt) :
opt_(move(opt)) {}

string_view text_area_view::text() const noexcept { return (opt_.text != nullptr) ? opt_.text->value().view() : ""; }

size_t text_area_view::selection_begin() const noexcept { return min(anchor_, cursor_); }

size_t text_area_view::selection_end() const noexcept { return max(anchor_, cursor_); }

void text_area_view::setup() {
    rect_ = &create_state<layout_rect>(layout_rect{});
    clamp_cursor();
}

void text_area_view::clamp_cursor() {
    const size_t len = text().size();
    cursor_ = min(cursor_, len);
    anchor_ = min(anchor_, len);
}

size_t text_area_view::offset_at(const int row, const int column) const {
    const layout_rect box = (rect_ != nullptr) ? rect_->value() : layout_rect{};
    const enum style::border border = opt_.style.border.value_or(style::border::none);
    const int border_w = (border == style::border::none) ? 0 : 1;
    const int inner_w = max(1, box.w - 2 * border_w);

    const string_view value = text();
    const text_layout lay = layout_text(value, inner_w, opt_.wrap);
    if (lay.lines.empty()) {
        return 0;
    }

    const size_t line_index =
            static_cast<size_t>(max(0, min(row + scroll_row_, static_cast<int>(lay.lines.size()) - 1)));
    const auto& line = lay.lines[line_index];

    size_t pos = line.begin;
    int walked = 0;
    while (pos < line.end && walked < max(0, column)) {
        const size_t next = utf8_next(value, pos);
        walked += max(1, text_display_width(value.substr(pos, next - pos)));
        pos = next;
    }
    return pos;
}

bool text_area_view::copy_selection() {
    if (!has_selection()) {
        return false;
    }
    const string_view value = text();
    const size_t begin = min(selection_begin(), value.size());
    const size_t end = min(selection_end(), value.size());
    if (end <= begin) {
        return false;
    }
    console.copy_to_clipboard(value.substr(begin, end - begin));
    return true;
}

bool text_area_view::on_mouse(const mouse_event& e) {
    if (opt_.text == nullptr) {
        return false;
    }
    if (e.action != mouse_action::press && e.action != mouse_action::move) {
        return false;
    }

    const layout_rect box = (rect_ != nullptr) ? rect_->value() : layout_rect{};
    const enum style::border border = opt_.style.border.value_or(style::border::none);
    const int border_w = (border == style::border::none) ? 0 : 1;
    const int inner_x = box.x + border_w;
    const int inner_y = box.y + border_w;

    const size_t offset = offset_at(e.y - inner_y, e.x - inner_x);
    if (e.action == mouse_action::press) {
        cursor_ = offset;
        anchor_ = offset;
        dragging_ = true;
    } else {
        if (!dragging_) {
            return false;
        }
        cursor_ = offset;
    }
    schedule_render();
    return true;
}

void text_area_view::notify_changed() { schedule_render(); }

void text_area_view::notify_content_changed() {
    schedule_render();
    if (opt_.on_change && opt_.text != nullptr) {
        opt_.on_change(opt_.text->value());
    }
}

void text_area_view::record_undo() {
    if (opt_.text == nullptr) {
        return;
    }
    if (opt_.word_undo && undo_coalescing_ && !undo_.empty()) {
        redo_.clear();
        return;
    }
    undo_.push_back(snapshot{opt_.text->value(), cursor_, anchor_});
    if (undo_.size() > max<size_t>(1, opt_.undo_limit)) {
        undo_.erase(undo_.begin());
    }
    redo_.clear();
}

void text_area_view::insert_text(const string_view chunk) {
    if (opt_.text == nullptr || chunk.empty()) {
        return;
    }
    record_undo();
    undo_coalescing_ = opt_.word_undo;

    string value = opt_.text->value();
    erase_selection();
    value = opt_.text->value();

    const size_t at = min(cursor_, value.size());
    string before = value.substr(0, at);
    const string_view after = value.view(at);
    before.append(chunk);
    cursor_ = before.size();
    anchor_ = cursor_;
    *opt_.text = before + after;
    notify_content_changed();
}

void text_area_view::erase_selection() {
    if (opt_.text == nullptr) {
        return;
    }
    const size_t begin = selection_begin();
    const size_t end = selection_end();
    if (begin == end) {
        return;
    }
    const string value = opt_.text->value();
    const string head = value.substr(0, min(begin, value.size()));
    const string_view tail = value.view(min(end, value.size()));
    cursor_ = head.size();
    anchor_ = cursor_;
    *opt_.text = head + tail;
}

void text_area_view::set_text(string value) {
    if (opt_.text == nullptr) {
        return;
    }
    record_undo();
    *opt_.text = move(value);
    cursor_ = text().size();
    anchor_ = cursor_;
    scroll_row_ = 0;
    notify_content_changed();
}

void text_area_view::set_cursor(const size_t offset) {
    cursor_ = min(offset, text().size());
    anchor_ = cursor_;
    scroll_row_ = 0;
    notify_changed();
}

void text_area_view::select_all() {
    anchor_ = 0;
    cursor_ = text().size();
    notify_changed();
}

bool text_area_view::undo() {
    undo_coalescing_ = false;
    if (undo_.empty() || opt_.text == nullptr) {
        return false;
    }
    redo_.push_back(snapshot{opt_.text->value(), cursor_, anchor_});
    const snapshot last = undo_.back();
    undo_.pop_back();
    *opt_.text = last.text;
    cursor_ = min(last.cursor, text().size());
    anchor_ = min(last.anchor, text().size());
    notify_content_changed();
    return true;
}

bool text_area_view::redo() {
    undo_coalescing_ = false;
    if (redo_.empty() || opt_.text == nullptr) {
        return false;
    }
    undo_.push_back(snapshot{opt_.text->value(), cursor_, anchor_});
    const snapshot next = redo_.back();
    redo_.pop_back();
    *opt_.text = next.text;
    cursor_ = min(next.cursor, text().size());
    anchor_ = min(next.anchor, text().size());
    notify_content_changed();
    return true;
}

void text_area_view::submit() {
    if (opt_.text == nullptr) {
        return;
    }
    if (opt_.history_limit > 0 && !opt_.text->value().empty()) {
        history_.push_back(opt_.text->value());
        if (history_.size() > opt_.history_limit) {
            history_.erase(history_.begin());
        }
    }
    history_pos_ = history_.size();
    if (opt_.on_submit) {
        opt_.on_submit();
    }
}

void text_area_view::jump_history(const int delta) {
    if (history_.empty() || opt_.text == nullptr) {
        return;
    }
    if (delta < 0) {
        if (history_pos_ == 0) {
            return;
        }
        --history_pos_;
    } else {
        if (history_pos_ >= history_.size()) {
            return;
        }
        ++history_pos_;
    }

    record_undo();
    if (history_pos_ >= history_.size()) {
        // Stepping past the newest entry returns to an empty draft.
        opt_.text->set_quiet(string());
        cursor_ = 0;
    } else {
        opt_.text->set_quiet(history_[history_pos_]);
        cursor_ = text().size();
    }
    anchor_ = cursor_;
    notify_content_changed();
}

size_t text_area_view::word_boundary(const size_t from, const bool forward) const {
    const string_view value = text();
    size_t pos = min(from, value.size());
    if (forward) {
        while (pos < value.size() && !is_word_at(value, pos)) {
            pos = utf8_next(value, pos);
        }
        while (pos < value.size() && is_word_at(value, pos)) {
            pos = utf8_next(value, pos);
        }
        return pos;
    }
    while (pos > 0 && !is_word_at(value, utf8_prev(value, pos))) {
        pos = utf8_prev(value, pos);
    }
    while (pos > 0 && is_word_at(value, utf8_prev(value, pos))) {
        pos = utf8_prev(value, pos);
    }
    return pos;
}

void text_area_view::move_line_boundary(const bool to_end, const bool extend) {
    const string_view value = text();
    size_t pos = min(cursor_, value.size());
    if (to_end) {
        while (pos < value.size() && value[pos] != '\n') {
            pos = utf8_next(value, pos);
        }
    } else {
        while (pos > 0 && value[pos - 1] != '\n') {
            pos = utf8_prev(value, pos);
        }
    }
    cursor_ = pos;
    if (!extend) {
        anchor_ = cursor_;
    }
    notify_changed();
}

void text_area_view::move_document_boundary(const bool to_end, const bool extend) {
    cursor_ = to_end ? text().size() : 0;
    if (!extend) {
        anchor_ = cursor_;
    }
    notify_changed();
}

void text_area_view::move_up_or_down(const int delta, const bool extend) {
    const string_view value = text();
    const int visible_w = max(1, rect_ != nullptr ? rect_->value().w : 0);
    const text_layout lay = layout_text(value, visible_w, opt_.wrap);

    int row = 0;
    int col = 0;
    caret_position(lay, value, min(cursor_, value.size()), row, col);
    const int target = row + delta;
    if (target < 0) {
        cursor_ = 0;
    } else if (compare_greater_equal(target, lay.lines.size())) {
        cursor_ = value.size();
    } else {
        const auto& line = lay.lines[static_cast<size_t>(target)];
        size_t pos = line.begin;
        int walked = 0;
        while (pos < line.end) {
            if (walked >= col) {
                break;
            }
            walked += max(1, text_display_width(value.substr(pos, utf8_next(value, pos) - pos)));
            pos = utf8_next(value, pos);
        }
        cursor_ = pos;
    }

    if (!extend) {
        anchor_ = cursor_;
    }
    notify_changed();
}

void text_area_view::move_page(const int direction, const bool extend) {
    const int rows = max(1, rect_ != nullptr ? rect_->value().h - 2 : 1);
    move_up_or_down(direction * rows, extend);
}

element text_area_view::render() {
    if (opt_.text == nullptr) {
        return element::empty();
    }

    tui::style st = opt_.style;
    if (st.text_wrap.value_or(style::wrap_mode::none) == style::wrap_mode::none) {
        st.text_wrap = opt_.wrap;
    }

    if (!st.border.has_value()) {
        st.border = style::border::none;
    }
    const enum style::border border = st.border.value_or(style::border::none);
    const int border_w = (border == style::border::none) ? 0 : 1;
    const layout_rect box = rect_ != nullptr ? rect_->value() : layout_rect{};
    const int inner_w = max(1, box.w - 2 * border_w);
    const int inner_h = max(1, box.h - 2 * border_w);

    const string_view value = text();
    const text_layout lay = layout_text(value, inner_w, st.text_wrap.value_or(style::wrap_mode::word));
    int row = 0;
    int col = 0;
    caret_position(lay, value, min(cursor_, value.size()), row, col);

    const int rows = static_cast<int>(lay.lines.size());
    if (row < scroll_row_) {
        scroll_row_ = row;
    } else if (row >= scroll_row_ + inner_h) {
        scroll_row_ = row - inner_h + 1;
    }
    scroll_row_ = max(0, min(scroll_row_, max(0, rows - inner_h)));

    if (!geometry_known_ && box.w > 0) {
        geometry_known_ = true;
        schedule_render();
    }

    element el = element::text_input(*opt_.text, st, opt_.placeholder)
                         .with_cursor_pos(min(cursor_, value.size()))
                         .with_cursor_visible(has_focus())
                         .with_selection(selection_begin(), selection_end())
                         .with_scroll_y(scroll_row_)
                         .with_rect_state(rect_);
    el.set_owner(this);
    return el;
}

bool text_area_view::on_key(const key_event& e) {
    if (opt_.text == nullptr || !has_focus()) {
        return false;
    }
    clamp_cursor();

    using K = key_event::type;
    const bool shift = (e.mods & key_modifier::shift) != key_modifier::none;
    const bool ctrl = (e.mods & key_modifier::ctrl) != key_modifier::none;
    if (e.key != K::printable && e.key != K::backspace && e.key != K::delete_) {
        undo_coalescing_ = false;
    }
    if (e.key == K::printable && ctrl) {
        undo_coalescing_ = false;
    }

    switch (e.key) {
        case K::printable: {
            const auto value = e.cp.value();
            if (ctrl) {
                const auto is_letter = [value](const char32_t lower) {
                    return value == lower || value == static_cast<char32_t>(lower - 32);
                };
                if (is_letter('a')) {
                    select_all();
                    return true;
                }
                if (is_letter('z')) {
                    return shift ? redo() : undo();
                }
                if (is_letter('y')) {
                    return redo();
                }
                if (is_letter('c')) {
                    return copy_selection();
                }
                if (is_letter('v')) {
                    console.request_clipboard();
                    return true;
                }
                if (is_letter('x')) {
                    if (!copy_selection()) {
                        return false;
                    }
                    record_undo();
                    erase_selection();
                    notify_content_changed();
                    return true;
                }
                if (is_letter('p')) {
                    jump_history(-1);
                    return true;
                }
                if (is_letter('n')) {
                    jump_history(1);
                    return true;
                }
                return false;
            }
            if (value < 0x20) {
                return false;
            }
            string chunk;
            e.cp.append_to(chunk);
            insert_text(chunk.view());
            return true;
        }
        case K::enter: {
            if (opt_.submit_on_enter || ctrl) {
                submit();
                return true;
            }
            insert_text("\n"_sv);
            return true;
        }
        case K::backspace: {
            if (has_selection()) {
                record_undo();
                erase_selection();
                notify_changed();
                return true;
            }
            if (cursor_ == 0) {
                return false;
            }
            record_undo();
            undo_coalescing_ = opt_.word_undo;
            const string& value = opt_.text->value();
            const size_t prev = ctrl ? word_boundary(cursor_, false) : utf8_prev(value.view(), cursor_);
            const string head = value.substr(0, prev);
            const string_view tail = value.view(cursor_);
            cursor_ = head.size();
            anchor_ = cursor_;
            *opt_.text = move(head) + tail;
            notify_changed();
            return true;
        }
        case K::delete_: {
            if (has_selection()) {
                record_undo();
                erase_selection();
                notify_changed();
                return true;
            }
            const string& value = opt_.text->value();
            if (cursor_ >= value.size()) {
                return false;
            }
            record_undo();
            undo_coalescing_ = opt_.word_undo;
            const size_t next = ctrl ? word_boundary(cursor_, true) : utf8_next(value.view(), cursor_);
            string head = value.substr(0, cursor_);
            const string_view tail = value.view(next);
            *opt_.text = move(head) + tail;
            notify_changed();
            return true;
        }
        case K::left: {
            const size_t target = ctrl ? word_boundary(cursor_, false) : utf8_prev(text(), cursor_);
            cursor_ = target;
            if (!shift) {
                anchor_ = cursor_;
            }
            notify_changed();
            return true;
        }
        case K::right: {
            const size_t target = ctrl ? word_boundary(cursor_, true) : utf8_next(text(), cursor_);
            cursor_ = target;
            if (!shift) {
                anchor_ = cursor_;
            }
            notify_changed();
            return true;
        }
        case K::up: {
            move_up_or_down(-1, shift);
            return true;
        }
        case K::down: {
            move_up_or_down(1, shift);
            return true;
        }
        case K::home: {
            if (ctrl) {
                move_document_boundary(false, shift);
            } else {
                move_line_boundary(false, shift);
            }
            return true;
        }
        case K::end: {
            if (ctrl) {
                move_document_boundary(true, shift);
            } else {
                move_line_boundary(true, shift);
            }
            return true;
        }
        case K::page_up: {
            move_page(-1, shift);
            return true;
        }
        case K::page_down: {
            move_page(1, shift);
            return true;
        }
        case K::escape: {
            if (has_selection()) {
                clear_selection();
                notify_changed();
                return true;
            }
            return false;
        }
        default: {
            return false;
        }
    }
}

bool text_area_view::on_paste(const paste_event& e) {
    if (opt_.text == nullptr || !has_focus() || e.text.empty()) {
        return false;
    }
    insert_text(e.text.view());
    return true;
}

unique_ptr<component_base> text_area(text_area_option opt) { return make_unique<text_area_view>(move(opt)); }

NEFORCE_END_COMPONENTS__
NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
