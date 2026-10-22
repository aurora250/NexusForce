#include <NeForce/core/async/process_watcher.hpp>
NEFORCE_BEGIN_NAMESPACE__

process_watcher::process_watcher(io_context& ctx, strand& strand) :
ctx_(ctx),
strand_(strand) {}

process_watcher::~process_watcher() {
    *alive_ = false;
    try {
        if (running_) {
            terminate();
        }
        release_handles();
        // NOLINTNEXTLINE(bugprone-empty-catch)
    } catch (const exception& e) {
        NEFORCE_REPORT_EXCEPTION(e);
    }
}

bool process_watcher::supports_pty() noexcept { return pty::is_available(); }

void process_watcher::release_handles() {
#ifndef NEFORCE_PLATFORM_WINDOWS
    if (term_ != nullptr) {
        ctx_.remove_fd(static_cast<io_context::native_handle_type>(term_->master_handle()));
    }
    if (out_pipe_ != nullptr) {
        ctx_.remove_fd(static_cast<io_context::native_handle_type>(out_pipe_->native_read_handle()));
    }
#endif
    term_.reset();
    out_pipe_.reset();
    proc_.reset();
}

void process_watcher::attach_pipe() {
#ifndef NEFORCE_PLATFORM_WINDOWS
    const auto fd = static_cast<io_context::native_handle_type>(out_pipe_->native_read_handle());
    ctx_.add_fd(
            fd, epoll_event::in, [this](int /*fd*/, epoll_event /*events*/, error_code /*ec*/) { drain_pipe(); },
            false);
#endif
}

void process_watcher::attach_pty() {
#ifndef NEFORCE_PLATFORM_WINDOWS
    const auto fd = static_cast<io_context::native_handle_type>(term_->master_handle());
    ctx_.add_fd(
            fd, epoll_event::in, [this](int /*fd*/, epoll_event /*events*/, error_code /*ec*/) { drain_pty(); }, false);
#endif
}

bool process_watcher::start(const string& executable, const vector<string>& args, option opt) {
    release_handles();

    exit_code_ = -1;
    term_sent_ = false;
    settle_polls_ = 0;
    poll_interval_ms_ = max(1, opt.poll_interval_ms);

    const bool want_pty = opt.use_pty && pty::is_available();
    if (want_pty) {
        term_ = make_unique<pty>();
        try {
            term_->open(pty::console_size{opt.pty_columns, opt.pty_rows});
            term_->start(executable, args, opt.work_dir, opt.env, opt.merge_stderr);
        } catch (...) {
            term_.reset();
            return false;
        }
        attach_pty();
    } else {
        proc_ = make_unique<process>();
        out_pipe_ = make_unique<pipe>(true, true);
        proc_->set_capture_stdout(false);
        proc_->set_capture_stderr(false);
        proc_->set_external_stdout(*out_pipe_);
        if (opt.merge_stderr) {
            proc_->set_external_stderr(*out_pipe_);
        }
        if (!opt.work_dir.empty()) {
            proc_->set_work_dir(opt.work_dir);
        }
        for (const auto& item: opt.env) {
            const auto separator = item.find('=');
            if (separator != string::npos && separator > 0) {
                proc_->set_env(string(item.view().substr(0, separator)), string(item.view().substr(separator + 1)));
            }
        }
        try {
            proc_->start(executable, args);
        } catch (...) {
            out_pipe_.reset();
            proc_.reset();
            return false;
        }
        // The parent must drop its own write end,
        // otherwise the read end never reports EOF after the child exits.
        out_pipe_->close_write();
        attach_pipe();
    }

    running_ = true;

    auto tick = make_shared<function<void()>>();
    *tick = [this, tick, guard = alive_] {
        if (!*guard || !running_) {
            return;
        }
#ifdef NEFORCE_PLATFORM_WINDOWS
        if (term_ != nullptr) {
            drain_pty();
        } else if (out_pipe_ != nullptr) {
            drain_pipe();
        }
#endif
        poll_exit();
        if (running_) {
            ctx_.schedule_timer(poll_interval_ms_, *tick);
        }
    };
    ctx_.schedule_timer(poll_interval_ms_, *tick);

    return true;
}

bool process_watcher::start_shell(const string& command_line, option opt) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    return start("cmd.exe", {"/C", command_line}, move(opt));
#else
    return start("/bin/sh", {"-c", command_line}, move(opt));
#endif
}

bool process_watcher::drain_pipe() {
    if (out_pipe_ == nullptr) {
        return false;
    }
    const string data = out_pipe_->read_available();
    if (data.empty()) {
        return false;
    }
    if (on_output_) {
        on_output_(data.view());
    }
    return true;
}

bool process_watcher::drain_pty() {
    if (term_ == nullptr) {
        return false;
    }
    const string data = term_->read_available();
    if (data.empty()) {
        return false;
    }
    if (on_output_) {
        on_output_(data.view());
    }
    return true;
}

void process_watcher::poll_exit() {
    if (!running_) {
        return;
    }

    bool exited = false;
    int code = -1;

    if (term_ != nullptr) {
        exited = term_->poll_exit();
        if (exited) {
            code = term_->exit_code();
            drain_pty();
        }
    } else if (proc_ != nullptr) {
        exited = !proc_->is_running();
        if (exited) {
            // A finished child is already a zombie, so this returns immediately;
            // `wait(0)` would report a timeout without ever reaping it.
            code = proc_->wait(-1);
            drain_pipe();
        }
    } else {
        exited = true;
    }

    if (exited) {
#ifdef NEFORCE_PLATFORM_WINDOWS
        // ConPTY and the stdout pipe deliver their tail after the child has been reaped,
        // so the first exit poll can still miss the final chunk.
        // Keep polling until two consecutive polls read nothing, then report the exit.
        if (settle_polls_ == 0) {
            settle_polls_ = 2;
        }
        if (settle_polls_ > 0) {
            const bool more = (term_ != nullptr) ? drain_pty() : drain_pipe();
            settle_polls_ = more ? 2 : settle_polls_ - 1;
            if (settle_polls_ > 0) {
                return;
            }
        }
#endif
        finish(code);
    }
}

void process_watcher::finish(const int code) {
    if (!running_) {
        return;
    }
    running_ = false;
    exit_code_ = code;
    if (on_exit_) {
        on_exit_(code);
    }
}

void process_watcher::write(const string_view data) {
    if (!running_) {
        return;
    }
    if (term_ != nullptr) {
        (void) term_->write(data);
    } else if (proc_ != nullptr) {
        proc_->write_stdin(string(data));
    }
}

void process_watcher::resize(const int columns, const int rows) {
    if (term_ != nullptr) {
        term_->resize(columns, rows);
    }
}

void process_watcher::send_control(const char byte) {
    if (term_ != nullptr) {
        term_->send_control(byte);
        return;
    }
    write(string_view(&byte, 1));
}

void process_watcher::terminate() {
    if (!running_) {
        return;
    }
    if (term_ != nullptr) {
        term_->terminate();
    } else if (proc_ != nullptr) {
        proc_->terminate();
    }
    term_sent_ = true;

    const auto escalate = make_shared<function<void()>>();
    *escalate = [this, guard = alive_] {
        if (!*guard || !running_) {
            return;
        }
        if (term_ != nullptr) {
            term_->kill();
        } else if (proc_ != nullptr) {
            proc_->terminate();
        }
        poll_exit();
    };
    ctx_.schedule_timer(300, *escalate);
}

NEFORCE_END_NAMESPACE__
