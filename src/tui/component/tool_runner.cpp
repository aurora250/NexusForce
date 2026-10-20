#include <NeForce/tui/component/tool_runner.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__
NEFORCE_BEGIN_COMPONENTS__

namespace {
    size_t count_lines(const string_view text) {
        size_t lines = 1;
        for (size_t i = 0; i < text.size(); ++i) {
            if (text[i] == '\n') {
                ++lines;
            }
        }
        return lines;
    }
} // namespace


tool_runner::tool_runner(io_context& ctx, strand& strand, tool_runner_option opt) :
ctx_(ctx),
strand_(strand),
opt_(move(opt)) {}

void tool_runner::setup() {
    auto owned = make_unique<tool_card>(opt_.card);
    card_ = owned.get();
    add_child(move(owned));

    watcher_ = make_unique<process_watcher>(ctx_, strand_);
    watcher_->set_on_output([this](const string_view chunk) { append_output(chunk); });
    watcher_->set_on_exit([this](const int code) {
        flush_output();
        const tool_status status = interrupted_  ? tool_status::interrupted
                                   : (code == 0) ? tool_status::success
                                                 : tool_status::failed;
        if (card_ != nullptr) {
            card_->set_status(status, static_cast<int64_t>(0));
            if (status == tool_status::failed && dropped_lines_ > 0) {
                card_->set_output(output_);
            }
        }
        if (opt_.on_exit) {
            opt_.on_exit(code);
        }
        schedule_render();
    });
}

bool tool_runner::start(const string& command_line) {
    if (watcher_ == nullptr) {
        return false;
    }
    output_.clear();
    pending_.clear();
    dropped_lines_ = 0;
    interrupted_ = false;
    dirty_output_ = true;

    if (card_ != nullptr) {
        card_->set_status(tool_status::running, static_cast<int64_t>(0));
        card_->set_output(string());
    }

    process_watcher::option proc = opt_.process;
    if (proc.use_pty && !process_watcher::supports_pty()) {
        // No devpts (container / sandbox): fall back to pipes so the tool still runs.
        proc.use_pty = false;
    }
    if (!watcher_->start_shell(command_line, proc)) {
        if (card_ != nullptr) {
            card_->set_status(tool_status::failed, static_cast<int64_t>(0));
        }
        schedule_render();
        return false;
    }
    schedule_render();
    return true;
}

void tool_runner::interrupt() {
    if (watcher_ == nullptr || !watcher_->running()) {
        return;
    }
    interrupted_ = true;
    if (watcher_->uses_pty()) {
        watcher_->send_control(static_cast<char>(0x03));
        watcher_->terminate();
    } else {
        watcher_->terminate();
    }
    schedule_render();
}

bool tool_runner::running() const noexcept { return watcher_ != nullptr && watcher_->running(); }

int tool_runner::exit_code() const noexcept { return watcher_ != nullptr ? watcher_->exit_code() : -1; }

void tool_runner::append_output(const string_view chunk) {
    if (chunk.empty()) {
        return;
    }
    pending_.append(chunk);
    dirty_output_ = true;
    // Output is flushed once per frame from on_animation();
    // a card that never gets animation ticks (fps == 0) still flushes on exit.
}

void tool_runner::trim_output() {
    if (opt_.max_output_lines == 0) {
        return;
    }
    size_t lines = count_lines(output_.view());
    while (lines > opt_.max_output_lines) {
        const auto newline = output_.view().find('\n');
        if (newline == string_view::npos) {
            break;
        }
        output_ = output_.substr(newline + 1);
        --lines;
        ++dropped_lines_;
    }
}

void tool_runner::flush_output() {
    if (!dirty_output_) {
        return;
    }
    dirty_output_ = false;
    if (!pending_.empty()) {
        output_.append(pending_);
        pending_.clear();
        trim_output();
    }
    if (card_ != nullptr) {
        card_->set_output(output_);
        if (opt_.auto_scroll && card_->running()) {
            card_->set_collapsed(false);
        }
    }
    schedule_render();
}

element tool_runner::render() {
    if (card_ == nullptr) {
        return element::empty();
    }
    return card_->render();
}

bool tool_runner::on_key(const key_event& e) {
    if (card_ == nullptr) {
        return false;
    }
    return card_->on_key(e);
}

bool tool_runner::on_mouse(const mouse_event& e) {
    if (card_ == nullptr) {
        return false;
    }
    return card_->on_mouse(e);
}

void tool_runner::on_animation(const int64_t delta) {
    if (card_ != nullptr) {
        card_->on_animation(delta);
    }
    flush_output();
}

void tool_runner::cleanup() noexcept {
    if (watcher_ == nullptr) {
        return;
    }
    if (watcher_->running()) {
        watcher_->terminate();
    }
    watcher_.reset();
}

unique_ptr<component_base> tool_runner_component(io_context& ctx, strand& strand, tool_runner_option opt) {
    return make_unique<tool_runner>(ctx, strand, move(opt));
}

NEFORCE_END_COMPONENTS__
NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
