#include <NeForce/core/async/atomic.hpp>
#include <NeForce/core/container/array.hpp>
#include <NeForce/core/exception/exception.hpp>
#include <NeForce/core/exception/terminate.hpp>
#include <NeForce/core/system/console.hpp>
#include <NeForce/core/system/stacktrace.hpp>
#include <cstdio> // ::fflush
#ifdef NEFORCE_PLATFORM_WINDOWS
#include <errhandlingapi.h>
#endif
#ifdef NEFORCE_PLATFORM_LINUX
#include <signal.h>

#ifndef SYS_exit
#if defined(NEFORCE_ARCH_X86_32)
    #define SYS_exit 1
    #define SYS_exit_group 252
    #define SYS_close 6
#elif defined(NEFORCE_ARCH_X86_64)
    #define SYS_exit 60
    #define SYS_exit_group 231
    #define SYS_close 3
#elif defined(NEFORCE_ARCH_ARM32)
    #define SYS_exit 1
    #define SYS_exit_group 248
    #define SYS_close 6
#elif defined(NEFORCE_ARCH_AARCH64)
    #define SYS_exit 93
    #define SYS_exit_group 94
    #define SYS_close 57
#elif defined(NEFORCE_ARCH_RISCV32) || defined(NEFORCE_ARCH_RISCV64)
    #define SYS_exit 93
    #define SYS_exit_group 94
    #define SYS_close 57
#elif defined(NEFORCE_ARCH_LOONGARCH32) || defined(NEFORCE_ARCH_LOONGARCH64)
    #define SYS_exit 93
    #define SYS_exit_group 94
    #define SYS_close 57
#endif
#endif

#endif
NEFORCE_BEGIN_NAMESPACE__

#ifdef NEFORCE_PLATFORM_LINUX

#if defined(NEFORCE_ARCH_X86_64)

static inline long syscall_exit(int status) {
    long ret;
    __asm__ volatile (
        "syscall"
        : "=a"(ret)
        : "a"(SYS_exit), "D"(status)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static inline long syscall_exit_group(int status) {
    long ret;
    __asm__ volatile (
        "syscall"
        : "=a"(ret)
        : "a"(SYS_exit_group), "D"(status)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static inline long syscall_close(int fd) {
    long ret;
    __asm__ volatile (
        "syscall"
        : "=a"(ret)
        : "a"(SYS_close), "D"(fd)
        : "rcx", "r11", "memory"
    );
    return ret;
}

#elif defined(NEFORCE_ARCH_X86_32)

static inline long syscall_exit(int status) {
    long ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_exit), "b"(status)
        : "memory"
    );
    return ret;
}

static inline long syscall_exit_group(int status) {
    long ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_exit_group), "b"(status)
        : "memory"
    );
    return ret;
}

static inline long syscall_close(int fd) {
    long ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_close), "b"(fd)
        : "memory"
    );
    return ret;
}

#elif defined(NEFORCE_ARCH_ARM32)

static inline long syscall_exit(int status) {
    register long r7 __asm__("r7") = SYS_exit;
    register long r0 __asm__("r0") = status;
    __asm__ volatile (
        "swi 0x0"
        : "=r"(r0)
        : "r"(r0), "r"(r7)
        : "memory"
    );
    return r0;
}

static inline long syscall_exit_group(int status) {
    register long r7 __asm__("r7") = SYS_exit_group;
    register long r0 __asm__("r0") = status;
    __asm__ volatile (
        "swi 0x0"
        : "=r"(r0)
        : "r"(r0), "r"(r7)
        : "memory"
    );
    return r0;
}

static inline long syscall_close(int fd) {
    register long r7 __asm__("r7") = SYS_close;
    register long r0 __asm__("r0") = fd;
    __asm__ volatile (
        "swi 0x0"
        : "=r"(r0)
        : "r"(r0), "r"(r7)
        : "memory"
    );
    return r0;
}

#elif defined(NEFORCE_ARCH_AARCH64)

static inline long syscall_exit(int status) {
    register long x8 __asm__("x8") = SYS_exit;
    register long x0 __asm__("x0") = status;
    __asm__ volatile (
        "svc #0"
        : "=r"(x0)
        : "r"(x0), "r"(x8)
        : "memory"
    );
    return x0;
}

static inline long syscall_exit_group(int status) {
    register long x8 __asm__("x8") = SYS_exit_group;
    register long x0 __asm__("x0") = status;
    __asm__ volatile (
        "svc #0"
        : "=r"(x0)
        : "r"(x0), "r"(x8)
        : "memory"
    );
    return x0;
}

static inline long syscall_close(int fd) {
    register long x8 __asm__("x8") = SYS_close;
    register long x0 __asm__("x0") = fd;
    __asm__ volatile (
        "svc #0"
        : "=r"(x0)
        : "r"(x0), "r"(x8)
        : "memory"
    );
    return x0;
}

#elif defined(NEFORCE_ARCH_RISCV)

static inline long syscall_exit(int status) {
    register long a7 __asm__("a7") = SYS_exit;
    register long a0 __asm__("a0") = status;
    __asm__ volatile (
        "ecall"
        : "=r"(a0)
        : "r"(a0), "r"(a7)
        : "memory"
    );
    return a0;
}

static inline long syscall_exit_group(int status) {
    register long a7 __asm__("a7") = SYS_exit_group;
    register long a0 __asm__("a0") = status;
    __asm__ volatile (
        "ecall"
        : "=r"(a0)
        : "r"(a0), "r"(a7)
        : "memory"
    );
    return a0;
}

static inline long syscall_close(int fd) {
    register long a7 __asm__("a7") = SYS_close;
    register long a0 __asm__("a0") = fd;
    __asm__ volatile (
        "ecall"
        : "=r"(a0)
        : "r"(a0), "r"(a7)
        : "memory"
    );
    return a0;
}

#elif defined(NEFORCE_ARCH_LOONGARCH)

static inline long syscall_exit(int status) {
    register long a7 __asm__("a7") = SYS_exit;
    register long a0 __asm__("a0") = status;
    __asm__ volatile (
        "syscall 0"
        : "=r"(a0)
        : "r"(a0), "r"(a7)
        : "memory"
    );
    return a0;
}

static inline long syscall_exit_group(int status) {
    register long a7 __asm__("a7") = SYS_exit_group;
    register long a0 __asm__("a0") = status;
    __asm__ volatile (
        "syscall 0"
        : "=r"(a0)
        : "r"(a0), "r"(a7)
        : "memory"
    );
    return a0;
}

static inline long syscall_close(int fd) {
    register long a7 __asm__("a7") = SYS_close;
    register long a0 __asm__("a0") = fd;
    __asm__ volatile (
        "syscall 0"
        : "=r"(a0)
        : "r"(a0), "r"(a7)
        : "memory"
    );
    return a0;
}

#endif

constexpr void validate_syscall_numbers() {
#if defined(SYS_exit)
    static_assert(SYS_exit > 0, "SYS_exit must be defined");
#endif
#if defined(SYS_exit_group)
    static_assert(SYS_exit_group > 0, "SYS_exit_group must be defined");
#endif
#if defined(SYS_close)
    static_assert(SYS_close > 0, "SYS_close must be defined");
#endif
}

#endif

void throw_with_stack(const exception& err) {
    printcln(color::red(), "\nException : (", err.type(), ") ", err.what());
    printcln(color::red(), stacktrace());
    throw err;
}

static atomic<terminate_handler>& get_terminate_handler() noexcept {
    static atomic<terminate_handler> handler{nullptr};
    return handler;
}

void set_terminate(const terminate_handler handler) noexcept {
    get_terminate_handler().store(handler, memory_order_release);
}

void terminate() {
    const auto handler = get_terminate_handler().load(memory_order_acquire);
    if (handler) handler();
    _NEFORCE abort();
}

void abort() {
    ::fflush(nullptr);

#ifdef NEFORCE_PLATFORM_WINDOWS
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

    immediate_exit(1);
#endif
    NEFORCE_UNREACHABLE;
}


class exit_handler_manager {
private:
    static constexpr size_t max_handler_threshhold = 32;

    struct handler_entry {
        exit_handler func;
        bool is_used;
    };

    array<handler_entry, max_handler_threshhold> atexit_handlers;
    array<handler_entry, max_handler_threshhold> quick_exit_handlers;
    size_t atexit_count;
    size_t quick_exit_count;
    mutex mtx;

    exit_handler_manager()
    : atexit_count(0), quick_exit_count(0) {
        fill(atexit_handlers.begin(), atexit_handlers.end(), handler_entry{nullptr, false});
        fill(quick_exit_handlers.begin(), quick_exit_handlers.end(), handler_entry{nullptr, false});
    }

public:
    static exit_handler_manager& instance() {
        static exit_handler_manager manager;
        return manager;
    }

    int register_atexit(const exit_handler handler) {
        if (!handler) return -1;

        lock<mutex> lock(mtx);
        if (atexit_count >= max_handler_threshhold) return -1;

        for (auto& entry : atexit_handlers) {
            if (!entry.is_used) {
                entry.func = handler;
                entry.is_used = true;
                ++atexit_count;
                return 0;
            }
        }
        return -1;
    }

    int register_quick_exit(const exit_handler handler) {
        if (!handler) return -1;

        lock<mutex> lock(mtx);
        if (quick_exit_count >= max_handler_threshhold) return -1;

        for (auto& entry : quick_exit_handlers) {
            if (!entry.is_used) {
                entry.func = handler;
                entry.is_used = true;
                ++quick_exit_count;
                return 0;
            }
        }
        return -1;
    }

    void execute_atexit_handlers() {
        for (int i = static_cast<int>(atexit_handlers.size()) - 1; i >= 0; --i) {
            if (atexit_handlers[i].is_used) {
                atexit_handlers[i].func();
                atexit_handlers[i].is_used = false;
            }
        }
        atexit_count = 0;
    }

    void execute_quick_exit_handlers() {
        for (int i = static_cast<int>(quick_exit_handlers.size()) - 1; i >= 0; --i) {
            if (quick_exit_handlers[i].is_used) {
                quick_exit_handlers[i].func();
                quick_exit_handlers[i].is_used = false;
            }
        }
        quick_exit_count = 0;
    }
};


int set_exit(const exit_handler handler) noexcept {
    return exit_handler_manager::instance().register_atexit(handler);
}

int set_quick_exit(const exit_handler handler) noexcept {
    return exit_handler_manager::instance().register_quick_exit(handler);
}

void exit(const int status) {
    exit_handler_manager::instance().execute_atexit_handlers();

    ::fflush(nullptr);

#ifdef NEFORCE_PLATFORM_WINDOWS
    ::ExitProcess(static_cast<::UINT>(status));
#else
    syscall_exit_group(status);
#endif
    NEFORCE_UNREACHABLE;
}

void immediate_exit(const int status) noexcept {
    ::fflush(nullptr);

#ifdef NEFORCE_PLATFORM_WINDOWS
    ::TerminateProcess(::GetCurrentProcess(), static_cast<::UINT>(status));
#else
    syscall_exit(status);
#endif
    NEFORCE_UNREACHABLE;
}

void quick_exit(const int status) noexcept {
    exit_handler_manager::instance().execute_quick_exit_handlers();
    immediate_exit(status);
}

NEFORCE_END_NAMESPACE__
