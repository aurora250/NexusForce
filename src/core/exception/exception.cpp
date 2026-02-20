#include <MSTL/core/exception/exception.hpp>
#include <MSTL/core/async/atomic.hpp>
#include <MSTL/core/system/console.hpp>
#include <MSTL/core/system/stacktrace.hpp>
#include <MSTL/core/exception/terminate.hpp>
#include <stdio.h>
#include <stdlib.h>
#ifdef MSTL_PLATFORM_LINUX__
#include <signal.h>
#endif
MSTL_BEGIN_NAMESPACE__

void throw_with_stack(const exception& err) {
    printcln(color::red(), "\nException : (", err.type(), ") ", err.what());
    printcln(color::red(), stacktrace());
    throw err;
}

static atomic<terminate_handler>& get_terminate_handler() noexcept {
    static atomic<terminate_handler> handler{nullptr};
    return handler;
}

void set_terminate(terminate_handler handler) noexcept {
    get_terminate_handler().store(handler, memory_order_release);
}

void terminate() {
    const auto handler = get_terminate_handler().load(memory_order_acquire);
    if (handler) handler();
    std::abort();
}

void abort() {
    ::fflush(nullptr);

#ifdef MSTL_PLATFORM_WINDOWS__
    ::SetUnhandledExceptionFilter(nullptr);

    // STATUS_FATAL_APP_EXIT
    ::PVOID handlers[1];
    handlers[0] = ::AddVectoredExceptionHandler(1, nullptr);

    constexpr ::ULONG_PTR args[1] = { 0xC0000409 }; // STATUS_STACK_BUFFER_OVERRUN

    ::RaiseException(0xC0000409, EXCEPTION_NONCONTINUABLE, 1, args);
    ::TerminateProcess(::GetCurrentProcess(), 3);
    if (handlers[0]) ::RemoveVectoredExceptionHandler(handlers[0]);
#else
    ::sigset_t mask;
    ::sigemptyset(&mask);
    ::sigprocmask(SIG_SETMASK, &mask, nullptr);

    struct ::sigaction sa;
    sa.sa_handler = SIG_DFL;
    ::sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    struct ::sigaction old_sa;
    ::sigaction(SIGABRT, &sa, &old_sa);

    // SIGABRT
    ::kill(getpid(), SIGABRT);

    constexpr ::timespec ts = { .tv_sec = 0, .tv_nsec = 1000000 }; // 1ms
    ::nanosleep(&ts, nullptr);

    sa.sa_handler = SIG_IGN;
    ::sigaction(SIGABRT, &sa, nullptr);
    ::kill(::getpid(), SIGABRT);

    ::_exit(1);
#endif
    MSTL_UNREACHABLE;
}

MSTL_END_NAMESPACE__
