#include <NeForce/tui/app.hpp>
#include <NeForce/tui/input.hpp>
#include <NeForce/tui/reconciler.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    ifdef NEFORCE_COMPILER_MSVC
#        include <consoleapi.h>
#    endif
#    include <WinBase.h>
#endif
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

Application::Application() :
console_(sys_console::instance()) {}

Application::~Application() = default;

Application& Application::withTheme(const Theme& theme) {
    theme_ = theme;
    return *this;
}

Application& Application::withFps(int fps) {
    fps_ = fps;
    return *this;
}

int Application::run() {
    if (root_ == nullptr) {
        return 1;
    }
    if (running_) {
        return 1;
    }
    running_ = true;

#ifdef NEFORCE_PLATFORM_WINDOWS
    ::HANDLE hOut = ::GetStdHandle(STD_OUTPUT_HANDLE);
    ::DWORD mode = 0;
    ::GetConsoleMode(hOut, &mode);
    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN;
    ::SetConsoleMode(hOut, mode);
#endif

    console_.enable_alternate_screen_buffer();
    console_.hide_cursor();
    console_.enable_mouse();
    console_.set_window_title("NexusForce TUI");

    reconiler_ = make_unique<Reconciler>(console_, renderStrand_, ctx_);
    reconiler_->setTheme(theme_);
    input_ = make_unique<InputDriver>(ctx_, *reconiler_, renderStrand_);

    reconiler_->mount(root_.get());

    input_->start();

    auto resizeCheck = make_shared<function<void()>>();
    *resizeCheck = [this, resizeCheck] {
        if (InputDriver::checkResizeFlag() || console_.is_terminal_resized()) {
            reconiler_->markDirty();
        }
        ctx_.schedule_timer(200, *resizeCheck);
    };
    ctx_.schedule_timer(200, *resizeCheck);

    if (fps_ > 0) {
        int intervalMs = 1000 / fps_;
        auto tick = make_shared<function<void()>>();
        *tick = [this, intervalMs, tick] {
            reconiler_->flush();
            ctx_.schedule_timer(intervalMs, *tick);
        };
        ctx_.schedule_timer(intervalMs, *tick);
    }

    {
        io_context::work guard{ctx_};
        ctx_.run();
    }

    input_->stop();
    console_.show_cursor();
    console_.disable_mouse();
    console_.disable_alternate_screen_buffer();

    reconiler_.reset();
    input_.reset();

    running_ = false;
    return exitCode_;
}

void Application::quit(int exitCode) {
    exitCode_ = exitCode;
    ctx_.stop();
}

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
