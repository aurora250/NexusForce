#include <NeForce/core/async/atomic.hpp>
#include <NeForce/core/async/mutex.hpp>
#include <NeForce/core/container/array.hpp>
#include <NeForce/core/exception/terminate.hpp>
#include <cstdio> // ::fflush
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <errhandlingapi.h>
#endif
#ifdef NEFORCE_PLATFORM_LINUX
#    include <csignal>

#    ifndef SYS_exit
#        ifdef NEFORCE_ARCH_X86_32
#            define SYS_exit 1
#            define SYS_exit_group 252
#            define SYS_close 6
#        elif defined(NEFORCE_ARCH_X86_64)
#            define SYS_exit 60
#            define SYS_exit_group 231
#            define SYS_close 3
#        elif defined(NEFORCE_ARCH_ARM32)
#            define SYS_exit 1
#            define SYS_exit_group 248
#            define SYS_close 6
#        elif defined(NEFORCE_ARCH_AARCH64)
#            define SYS_exit 93
#            define SYS_exit_group 94
#            define SYS_close 57
#        elif defined(NEFORCE_ARCH_RISCV32) || defined(NEFORCE_ARCH_RISCV64)
#            define SYS_exit 93
#            define SYS_exit_group 94
#            define SYS_close 57
#        elif defined(NEFORCE_ARCH_LOONGARCH32) || defined(NEFORCE_ARCH_LOONGARCH64)
#            define SYS_exit 93
#            define SYS_exit_group 94
#            define SYS_close 57
#        endif
#    endif

#endif
NEFORCE_BEGIN_NAMESPACE__

namespace {
#ifdef NEFORCE_PLATFORM_LINUX
#    ifdef NEFORCE_ARCH_X86_64
    long syscall_exit(int status) {
        long ret = 0;
        __asm__ volatile("syscall" : "=a"(ret) : "a"(SYS_exit), "D"(status) : "rcx", "r11", "memory");
        return ret;
    }
    long syscall_exit_group(int status) {
        long ret = 0;
        __asm__ volatile("syscall" : "=a"(ret) : "a"(SYS_exit_group), "D"(status) : "rcx", "r11", "memory");
        return ret;
    }
    long syscall_close(int fd) {
        long ret = 0;
        __asm__ volatile("syscall" : "=a"(ret) : "a"(SYS_close), "D"(fd) : "rcx", "r11", "memory");
        return ret;
    }
#    elif defined(NEFORCE_ARCH_X86_32)
    long syscall_exit(int status) {
        long ret = 0;
        __asm__ volatile("int $0x80" : "=a"(ret) : "a"(SYS_exit), "b"(status) : "memory");
        return ret;
    }
    long syscall_exit_group(int status) {
        long ret = 0;
        __asm__ volatile("int $0x80" : "=a"(ret) : "a"(SYS_exit_group), "b"(status) : "memory");
        return ret;
    }
    long syscall_close(int fd) {
        long ret = 0;
        __asm__ volatile("int $0x80" : "=a"(ret) : "a"(SYS_close), "b"(fd) : "memory");
        return ret;
    }
#    elif defined(NEFORCE_ARCH_ARM32)
    long syscall_exit(int status) {
        register long r7 __asm__("r7") = SYS_exit;
        register long r0 __asm__("r0") = status;
        __asm__ volatile("swi 0x0" : "=r"(r0) : "r"(r0), "r"(r7) : "memory");
        return r0;
    }
    long syscall_exit_group(int status) {
        register long r7 __asm__("r7") = SYS_exit_group;
        register long r0 __asm__("r0") = status;
        __asm__ volatile("swi 0x0" : "=r"(r0) : "r"(r0), "r"(r7) : "memory");
        return r0;
    }
    long syscall_close(int fd) {
        register long r7 __asm__("r7") = SYS_close;
        register long r0 __asm__("r0") = fd;
        __asm__ volatile("swi 0x0" : "=r"(r0) : "r"(r0), "r"(r7) : "memory");
        return r0;
    }
#    elif defined(NEFORCE_ARCH_AARCH64)
    long syscall_exit(int status) {
        register long x8 __asm__("x8") = SYS_exit;
        register long x0 __asm__("x0") = status;
        __asm__ volatile("svc #0" : "=r"(x0) : "r"(x0), "r"(x8) : "memory");
        return x0;
    }
    long syscall_exit_group(int status) {
        register long x8 __asm__("x8") = SYS_exit_group;
        register long x0 __asm__("x0") = status;
        __asm__ volatile("svc #0" : "=r"(x0) : "r"(x0), "r"(x8) : "memory");
        return x0;
    }
    long syscall_close(int fd) {
        register long x8 __asm__("x8") = SYS_close;
        register long x0 __asm__("x0") = fd;
        __asm__ volatile("svc #0" : "=r"(x0) : "r"(x0), "r"(x8) : "memory");
        return x0;
    }
#    elif defined(NEFORCE_ARCH_RISCV)
    long syscall_exit(int status) {
        register long a7 __asm__("a7") = SYS_exit;
        register long a0 __asm__("a0") = status;
        __asm__ volatile("ecall" : "=r"(a0) : "r"(a0), "r"(a7) : "memory");
        return a0;
    }
    long syscall_exit_group(int status) {
        register long a7 __asm__("a7") = SYS_exit_group;
        register long a0 __asm__("a0") = status;
        __asm__ volatile("ecall" : "=r"(a0) : "r"(a0), "r"(a7) : "memory");
        return a0;
    }
    long syscall_close(int fd) {
        register long a7 __asm__("a7") = SYS_close;
        register long a0 __asm__("a0") = fd;
        __asm__ volatile("ecall" : "=r"(a0) : "r"(a0), "r"(a7) : "memory");
        return a0;
    }
#    elif defined(NEFORCE_ARCH_LOONGARCH)
    long syscall_exit(int status) {
        register long a7 __asm__("a7") = SYS_exit;
        register long a0 __asm__("a0") = status;
        __asm__ volatile("syscall 0" : "=r"(a0) : "r"(a0), "r"(a7) : "memory");
        return a0;
    }
    long syscall_exit_group(int status) {
        register long a7 __asm__("a7") = SYS_exit_group;
        register long a0 __asm__("a0") = status;
        __asm__ volatile("syscall 0" : "=r"(a0) : "r"(a0), "r"(a7) : "memory");
        return a0;
    }
    long syscall_close(int fd) {
        register long a7 __asm__("a7") = SYS_close;
        register long a0 __asm__("a0") = fd;
        __asm__ volatile("syscall 0" : "=r"(a0) : "r"(a0), "r"(a7) : "memory");
        return a0;
    }
#    endif
#endif

    atomic<terminate_handler>& get_terminate_handler() noexcept {
        static atomic<terminate_handler> handler{nullptr};
        return handler;
    }

    class exit_handler_manager {
    private:
        static constexpr size_t max_handler_threshhold = 32;

        struct handler_entry {
            exit_handler func;
            bool is_used;
        };

        array<handler_entry, max_handler_threshhold> exit_handlers_{};
        array<handler_entry, max_handler_threshhold> quick_exit_handlers_{};
        size_t exit_count_{0};
        size_t quick_exit_count_{0};
        mutex mtx_;

        exit_handler_manager() {
            fill(exit_handlers_.begin(), exit_handlers_.end(), handler_entry{nullptr, false});
            fill(quick_exit_handlers_.begin(), quick_exit_handlers_.end(), handler_entry{nullptr, false});
        }

    public:
        static exit_handler_manager& instance() {
            static exit_handler_manager manager;
            return manager;
        }

        int register_atexit(const exit_handler handler) {
            if (handler == nullptr) {
                return -1;
            }

            lock<mutex> lock(mtx_);
            if (exit_count_ >= max_handler_threshhold) {
                return -1;
            }

            for (auto& entry: exit_handlers_) {
                if (!entry.is_used) {
                    entry.func = handler;
                    entry.is_used = true;
                    ++exit_count_;
                    return 0;
                }
            }
            return -1;
        }

        int register_quick_exit(const exit_handler handler) {
            if (handler == nullptr) {
                return -1;
            }

            lock<mutex> lock(mtx_);
            if (quick_exit_count_ >= max_handler_threshhold) {
                return -1;
            }

            for (auto& entry: quick_exit_handlers_) {
                if (!entry.is_used) {
                    entry.func = handler;
                    entry.is_used = true;
                    ++quick_exit_count_;
                    return 0;
                }
            }
            return -1;
        }

        void execute_exit_handlers() {
            for (int i = static_cast<int>(exit_handlers_.size()) - 1; i >= 0; --i) {
                if (exit_handlers_[i].is_used) {
                    exit_handlers_[i].func();
                    exit_handlers_[i].is_used = false;
                }
            }
            exit_count_ = 0;
        }

        void execute_quick_exit_handlers() {
            for (int i = static_cast<int>(quick_exit_handlers_.size()) - 1; i >= 0; --i) {
                if (quick_exit_handlers_[i].is_used) {
                    quick_exit_handlers_[i].func();
                    quick_exit_handlers_[i].is_used = false;
                }
            }
            quick_exit_count_ = 0;
        }
    };
} // namespace


void set_terminate(const terminate_handler handler) noexcept {
    get_terminate_handler().store(handler, memory_order_release);
}

void terminate() noexcept {
    try {
        const auto handler = get_terminate_handler().load(memory_order_acquire);
        if (handler != nullptr) {
            handler();
        }
        // NOLINTNEXTLINE(bugprone-empty-catch)
    } catch (...) {
        // ignore
    }
    abort();
}

void abort() noexcept {
    ::fflush(nullptr);

#ifdef NEFORCE_PLATFORM_WINDOWS
    ::SetUnhandledExceptionFilter(nullptr);
    ::TerminateProcess(::GetCurrentProcess(), 3);
#else
    ::sigset_t mask;
    ::sigemptyset(&mask);
    ::pthread_sigmask(SIG_SETMASK, &mask, nullptr);

    struct ::sigaction sa;
    sa.sa_handler = SIG_DFL;
    ::sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    struct ::sigaction old_sa;
    ::sigaction(SIGABRT, &sa, &old_sa);

    // SIGABRT
    ::kill(::getpid(), SIGABRT);

    constexpr ::timespec ts = {.tv_sec = 0, .tv_nsec = 1000000}; // 1ms
    ::nanosleep(&ts, nullptr);

    sa.sa_handler = SIG_IGN;
    ::sigaction(SIGABRT, &sa, nullptr);
    ::kill(::getpid(), SIGABRT);

    immediate_exit(1);
#endif
    unreachable();
}

int set_exit(const exit_handler handler) noexcept { return exit_handler_manager::instance().register_atexit(handler); }

int set_quick_exit(const exit_handler handler) noexcept {
    return exit_handler_manager::instance().register_quick_exit(handler);
}

void exit(const int status) {
    exit_handler_manager::instance().execute_exit_handlers();

    ::fflush(nullptr);

#ifdef NEFORCE_PLATFORM_WINDOWS
    ::ExitProcess(static_cast<::UINT>(status));
#else
    syscall_exit_group(status);
#endif
    unreachable();
}

void immediate_exit(const int status) noexcept {
    ::fflush(nullptr);

#ifdef NEFORCE_PLATFORM_WINDOWS
    ::TerminateProcess(::GetCurrentProcess(), static_cast<::UINT>(status));
#else
    syscall_exit(status);
#endif
    unreachable();
}

void quick_exit(const int status) noexcept {
    exit_handler_manager::instance().execute_quick_exit_handlers();
    immediate_exit(status);
}

NEFORCE_END_NAMESPACE__
