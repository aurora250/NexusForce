#include <NeForce/tui/component/text_input.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

namespace {
    size_t utf8_next(const string& s, size_t pos) {
        if (pos >= s.size()) {
            return s.size();
        }
        const auto b = static_cast<unsigned char>(s[pos]);
        if ((b & 0x80) == 0) {
            return pos + 1;
        }
        if ((b & 0xE0) == 0xC0) {
            return pos + 2;
        }
        if ((b & 0xF0) == 0xE0) {
            return pos + 3;
        }
        if ((b & 0xF8) == 0xF0) {
            return pos + 4;
        }
        return pos + 1;
    }

    // retreat byte position by one UTF-8 codepoint
    size_t utf8_prev(const string& s, size_t pos) {
        if (pos == 0) {
            return 0;
        }
        size_t p = pos - 1;
        while (p > 0 && (static_cast<unsigned char>(s[p]) & 0xC0) == 0x80) {
            --p;
        }
        return p;
    }

    class text_input_component final : public component<> {
    public:
        explicit text_input_component(text_input_option opt) :
        opt_(move(opt)) {}

        void setup() override {
            cursor_visible_ = &create_state<bool>(true);
            cursor_pos_ = &create_state<size_t>(0);
        }

        element render() override {
            style s = opt_.style;
            if (opt_.wrap != style::wrap_mode::none) {
                s.text_wrap = opt_.wrap;
            }
            element el = element::text_input(*opt_.text, s, opt_.placeholder);
            el.set_owner(this);

            if (cursor_pos_ != nullptr) {
                const size_t len = opt_.text->value().size();
                if (cursor_pos_->value() > len) {
                    *cursor_pos_ = len;
                }
                el = el.with_cursor_pos(cursor_pos_->value());
            }

            if (has_focus() && cursor_visible_ != nullptr && cursor_visible_->value()) {
                el = el.with_cursor_visible(true);
            }
            return el;
        }

        void take_focus() override {
            if (cursor_pos_ != nullptr) {
                *cursor_pos_ = opt_.text->value().size();
            }
        }

        bool on_key(const key_event& e) override {
            if (!has_focus()) {
                return false;
            }
            using K = key_event::type;

            if (cursor_pos_ != nullptr) {
                const size_t len = opt_.text->value().size();
                if (cursor_pos_->value() > len) {
                    *cursor_pos_ = len;
                }
            }

            if (e.key == K::left) {
                if (cursor_pos_ != nullptr) {
                    const string& val = opt_.text->value();
                    *cursor_pos_ = utf8_prev(val, cursor_pos_->value());
                }
                return true;
            }
            if (e.key == K::right) {
                if (cursor_pos_ != nullptr) {
                    const string& val = opt_.text->value();
                    *cursor_pos_ = utf8_next(val, cursor_pos_->value());
                }
                return true;
            }
            if (e.key == K::home) {
                if (cursor_pos_ != nullptr) {
                    *cursor_pos_ = 0;
                }
                return true;
            }
            if (e.key == K::end) {
                if (cursor_pos_ != nullptr) {
                    *cursor_pos_ = opt_.text->value().size();
                }
                return true;
            }
            if (e.key == K::backspace) {
                string val = opt_.text->value();
                if (!val.empty()) {
                    size_t cp = cursor_pos_ != nullptr ? cursor_pos_->value() : val.size();
                    if (cp == 0) {
                        return false;
                    }
                    const size_t prev = utf8_prev(val, cp);
                    val.erase(prev, cp - prev);
                    *opt_.text = val;
                    if (cursor_pos_ != nullptr) {
                        *cursor_pos_ = prev;
                    }
                    return true;
                }
                return false;
            }
            if (e.key == K::delete_) {
                string val = opt_.text->value();
                if (!val.empty()) {
                    size_t cp = cursor_pos_ != nullptr ? cursor_pos_->value() : 0;
                    if (cp >= val.size()) {
                        return false;
                    }
                    const size_t next = utf8_next(val, cp);
                    val.erase(cp, next - cp);
                    *opt_.text = val;
                    return true;
                }
                return false;
            }
            if (e.key == K::enter) {
                if (opt_.on_enter) {
                    opt_.on_enter();
                }
                return true;
            }
            if (e.key == K::printable && e.cp.value() >= 0x20) {
                string val = opt_.text->value();
                const size_t cp = cursor_pos_ != nullptr ? cursor_pos_->value() : val.size();
                string before = val.substr(0, cp);
                string after = val.substr(cp);
                e.cp.append_to(before);
                *opt_.text = before + after;
                if (cursor_pos_ != nullptr) {
                    *cursor_pos_ = before.size();
                }
                return true;
            }
            return false;
        }

        void on_animation(int64_t delta) override {
            if (cursor_visible_ == nullptr) {
                return;
            }
            blink_accum_ += delta;
            if (blink_accum_ >= 530) {
                blink_accum_ = 0;
                *cursor_visible_ = !cursor_visible_->value();
            }
        }

        NEFORCE_NODISCARD bool focusable() const override { return true; }

    private:
        text_input_option opt_;
        state<bool>* cursor_visible_ = nullptr;
        state<size_t>* cursor_pos_ = nullptr;
        int64_t blink_accum_ = 0;
    };
} // namespace


unique_ptr<component_base> text_input(text_input_option opt) { return make_unique<text_input_component>(move(opt)); }

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
