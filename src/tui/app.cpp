#include <NeForce/tui/app.hpp>
#include <NeForce/tui/input.hpp>
#include <NeForce/tui/reconciler.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

application& application::with_theme(const theme& theme) {
    theme_ = theme;
    return *this;
}

application& application::with_fps(int fps) {
    fps_ = fps;
    return *this;
}

application& application::with_title(string title) {
    title_ = title;
    return *this;
}

int application::run() {
    if (root_ == nullptr) {
        return 1;
    }
    if (running_) {
        return 1;
    }
    running_ = true;

    console_.enable_virtual_terminal_processing();
    console_.enable_alternate_screen_buffer();
    console_.hide_cursor();
    console_.enable_mouse();
    console_.set_window_title(title_.empty() ? "NexusForce TUI"_sv : title_.view());

    reconiler_ = make_unique<reconciler>(console_, render_strand_, ctx_);
    reconiler_->set_theme(theme_);
    input_ = make_unique<input_driver>(ctx_, *reconiler_, render_strand_);

    reconiler_->mount(root_.get());

    input_->start();

    auto resize_check = make_shared<function<void()>>();
    *resize_check = [this, resize_check] {
        if (input_driver::check_resize_flag() || console_.is_terminal_resized()) {
            reconiler_->mark_dirty();
        }
        ctx_.schedule_timer(200, *resize_check);
    };
    ctx_.schedule_timer(200, *resize_check);

    if (fps_ > 0) {
        int interval_ms = 1000 / fps_;
        auto tick = make_shared<function<void()>>();
        auto last_time = make_shared<int64_t>(0);
        *tick = [this, interval_ms, tick, last_time] {
            const auto now = static_cast<int64_t>(interval_ms);
            const auto delta = (*last_time == 0) ? interval_ms : now;
            *last_time = now;

            if (root_ != nullptr && reconiler_ != nullptr) {
                vector<component_base*> stack;
                stack.push_back(root_.get());
                while (!stack.empty()) {
                    auto* comp = stack.back();
                    stack.pop_back();
                    comp->on_animation(delta);
                    for (size_t i = 0; i < comp->child_count(); ++i) {
                        stack.push_back(comp->child_at(i));
                    }
                }
                reconiler_->mark_dirty();
            }

            reconiler_->flush();
            ctx_.schedule_timer(interval_ms, *tick);
        };
        ctx_.schedule_timer(interval_ms, *tick);
    }

    {
        io_context::work guard{ctx_};
        ctx_.run();
    }

    input_->stop();
    console_.hide_cursor(false);
    console_.enable_mouse(false);
    console_.enable_alternate_screen_buffer(false);

    reconiler_.reset();
    input_.reset();

    running_ = false;
    return exit_code_;
}

void application::quit(int exit_code) {
    exit_code_ = exit_code;
    ctx_.stop();
}

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
