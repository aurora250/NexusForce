#include <NeForce/tui/headless.hpp>
#include <NeForce/tui/recorder.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

headless::headless(component_base* root, const int width, const int height) :
root_(root),
width_(max(1, width)),
height_(max(1, height)) {
    auto& console = sys_console::instance();
    reconciler_ = make_unique<reconciler>(console, strand_, ctx_);
    reconciler_->set_term_size(width_, height_);
    reconciler_->set_output_sink([this](const string_view ansi) { ansi_ = string(ansi); });
    reconciler_->mount(root_);
}

headless::~headless() = default;

void headless::set_theme(const theme& t) { reconciler_->set_theme(t); }

void headless::resize(const int width, const int height) {
    width_ = max(1, width);
    height_ = max(1, height);
    reconciler_->set_term_size(width_, height_);
}

void headless::feed(const string_view bytes) {
    if (recorder_ != nullptr) {
        recorder_->record_input(bytes);
    }
    for (size_t i = 0; i < bytes.size(); ++i) {
        parsed_input input;
        if (!parser_.feed(static_cast<byte_t>(bytes[i]), input)) {
            continue;
        }
        switch (input.type) {
            case parsed_input::kind::key: {
                reconciler_->dispatch_key(input.key);
                break;
            }
            case parsed_input::kind::mouse: {
                reconciler_->dispatch_mouse(input.mouse);
                break;
            }
            case parsed_input::kind::paste: {
                paste_event pe;
                pe.text = input.paste_text;
                reconciler_->dispatch_paste(pe);
                break;
            }
            case parsed_input::kind::none:
            default: {
                break;
            }
        }
    }

    parsed_input pending;
    if (parser_.flush(pending) && pending.type == parsed_input::kind::key) {
        reconciler_->dispatch_key(pending.key);
    }
}

void headless::feed_key(const key_event& e) { reconciler_->dispatch_key(e); }

size_t headless::pump(const size_t max_rounds) {
    size_t handled = 0;
    for (size_t round = 0; round < max_rounds; ++round) {
        const size_t n = ctx_.poll();
        if (n == 0) {
            break;
        }
        handled += n;
    }
    return handled;
}

void headless::render() {
    reconciler_->mark_dirty();
    reconciler_->flush();
    if (recorder_ != nullptr) {
        recorder_->record_frame(plaintext().view());
    }
}

void headless::tick() {
    pump();
    render();
}

string headless::plaintext() const { return reconciler_->frame().to_plaintext(); }

const screen& headless::frame() const noexcept { return reconciler_->frame(); }

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
