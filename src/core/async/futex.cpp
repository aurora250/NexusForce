#include <NeForce/core/async/futex.hpp>
#include <NeForce/core/exception/terminate.hpp>
#include <NeForce/core/numeric/numeric_traits.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <NeForce/core/config/windef.hpp>
#    include <NeForce/core/time/clocks.hpp>
#    include <errhandlingapi.h>
#    include <synchapi.h>
#    include <winerror.h>
#    ifdef max
#        undef max
#    endif
#    ifdef min
#        undef min
#    endif
#endif
#ifdef NEFORCE_PLATFORM_LINUX
#    include <linux/futex.h>
#    include <syscall.h>
#endif
#ifdef NEFORCE_PLATFORM_POSIX
#    include <NeForce/core/algorithm/compare.hpp>
#    include <cerrno>
#    include <unistd.h>
#    include <ctime>
#endif
#ifdef NEFORCE_PLATFORM_BSD_LIKE
#    include <cstdint>
#    include <pthread.h>
#endif
#ifdef NEFORCE_PLATFORM_FREEBSD
#    include <sys/types.h>
#    include <sys/umtx.h>
#endif
#ifdef NEFORCE_PLATFORM_MACOS
extern "C" int __ulock_wait2(::uint32_t operation, void* addr, ::uint64_t value, ::uint64_t timeout_ns,
                             ::uint64_t value2);
extern "C" int __ulock_wake(::uint32_t operation, void* addr, ::uint64_t wake_value);
#    ifndef UL_COMPARE_AND_WAIT
#        define UL_COMPARE_AND_WAIT 1
#    endif
#    ifndef ULF_WAKE_ALL
#        define ULF_WAKE_ALL 0x00000100
#    endif
#endif
NEFORCE_BEGIN_NAMESPACE__

// Native FreeBSD umtx / macOS ulock is the default.
// Define NEFORCE_ASYNC_BSD_COND_FALLBACK to build the diagnostic condition-variable path
// instead when a host's wait/wake notifications are known to be broken.
#if defined(NEFORCE_PLATFORM_BSD_LIKE) && defined(NEFORCE_ASYNC_BSD_COND_FALLBACK)
namespace {
    ::pthread_mutex_t& timed_wait_mutex() noexcept {
        static ::pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
        return mutex;
    }

    ::pthread_cond_t& timed_wait_cond() noexcept {
        static ::pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
        return cond;
    }

    int& timed_waiter_count() noexcept {
        static int count = 0;
        return count;
    }

    bool realtime_deadline(const bool is_monotonic, const int64_t sec, const int64_t ns, ::timespec& out) noexcept {
        if (!is_monotonic) {
            out = ::timespec{static_cast<::time_t>(sec), static_cast<long>(ns)};
            return true;
        }
        ::timespec mono{};
        ::timespec real{};
        if (::clock_gettime(CLOCK_MONOTONIC, &mono) != 0 || ::clock_gettime(CLOCK_REALTIME, &real) != 0) {
            return false;
        }
        int64_t remaining =
                (sec - static_cast<int64_t>(mono.tv_sec)) * 1000000000LL + (ns - static_cast<int64_t>(mono.tv_nsec));
        if (remaining < 0) {
            remaining = 0;
        }
        const int64_t total =
                static_cast<int64_t>(real.tv_sec) * 1000000000LL + static_cast<int64_t>(real.tv_nsec) + remaining;
        out = ::timespec{static_cast<::time_t>(total / 1000000000LL), static_cast<long>(total % 1000000000LL)};
        return true;
    }

    void wake_timed_waiters() noexcept {
        if (__atomic_load_n(&timed_waiter_count(), __ATOMIC_SEQ_CST) <= 0) {
            return;
        }
        ::pthread_mutex_lock(&timed_wait_mutex());
        ::pthread_cond_broadcast(&timed_wait_cond());
        ::pthread_mutex_unlock(&timed_wait_mutex());
    }

    // macOS and FreeBSD wait/wake interfaces are not reliable under QEMU and the
    // macOS ARM64 runner. Route every BSD-like wait through one process wide
    // condition variable with an explicit realtime deadline. This is slower than
    // the native futex path but has a single, auditable wake-up and timeout path.
    bool wait_on_timed_cond(void* addr, const platform_wait_t value, const bool is_monotonic, const int64_t sec,
                            const int64_t ns) noexcept {
        __atomic_add_fetch(&timed_waiter_count(), 1, __ATOMIC_SEQ_CST);
        ::pthread_mutex_lock(&timed_wait_mutex());
        const auto* word = static_cast<const int*>(addr);
        bool woken = __atomic_load_n(word, __ATOMIC_SEQ_CST) != value;
        ::timespec deadline{};
        if (!woken && !realtime_deadline(is_monotonic, sec, ns, deadline)) {
            woken = true;
        }
        while (!woken) {
            const int wait_status = ::pthread_cond_timedwait(&timed_wait_cond(), &timed_wait_mutex(), &deadline);
            if (wait_status != 0) {
                break;
            }
            woken = __atomic_load_n(word, __ATOMIC_SEQ_CST) != value;
        }
        ::pthread_mutex_unlock(&timed_wait_mutex());
        __atomic_sub_fetch(&timed_waiter_count(), 1, __ATOMIC_SEQ_CST);
        return woken;
    }

    void wait_on_cond(void* addr, const platform_wait_t value) noexcept {
        __atomic_add_fetch(&timed_waiter_count(), 1, __ATOMIC_SEQ_CST);
        ::pthread_mutex_lock(&timed_wait_mutex());
        const auto* word = static_cast<const int*>(addr);
        while (__atomic_load_n(word, __ATOMIC_SEQ_CST) == value) {
            (void) ::pthread_cond_wait(&timed_wait_cond(), &timed_wait_mutex());
        }
        ::pthread_mutex_unlock(&timed_wait_mutex());
        __atomic_sub_fetch(&timed_waiter_count(), 1, __ATOMIC_SEQ_CST);
    }
} // namespace
#endif

#if defined(NEFORCE_PLATFORM_MACOS) && !defined(NEFORCE_ASYNC_BSD_COND_FALLBACK)
namespace {
    bool relative_timeout_ns(const bool is_monotonic, const int64_t sec, const int64_t ns, uint64_t& out) noexcept {
        ::timespec now{};
        if (::clock_gettime(is_monotonic ? CLOCK_MONOTONIC : CLOCK_REALTIME, &now) != 0) {
            out = 0;
            return false;
        }
        const int64_t deadline_ns = sec * 1000000000LL + ns;
        const int64_t now_ns = static_cast<int64_t>(now.tv_sec) * 1000000000LL + static_cast<int64_t>(now.tv_nsec);
        const int64_t remaining = deadline_ns > now_ns ? deadline_ns - now_ns : 0;
        out = static_cast<uint64_t>(remaining);
        return true;
    }
} // namespace
#endif


bool futex_wait_until(void* addr, platform_wait_t value, const bool has_timeout, const int64_t sec, const int64_t ns,
                      const bool is_monotonic) {

#ifdef NEFORCE_PLATFORM_WINDOWS
    if (!has_timeout) {
        return ::WaitOnAddress(addr, &value, sizeof(::DWORD), numeric_traits<::DWORD>::max()) == 1;
    }

    const auto ms = relative_time(sec, ns, is_monotonic);
    if (::WaitOnAddress(addr, &value, sizeof(::DWORD), ms.count()) == TRUE) {
        return true;
    }

    const ::DWORD err = ::GetLastError();
    if (err == ERROR_TIMEOUT) {
        return false;
    }
    return false;

#elif defined(NEFORCE_PLATFORM_FREEBSD)
#    ifdef NEFORCE_ASYNC_BSD_COND_FALLBACK
    if (!has_timeout) {
        wait_on_cond(addr, value);
        return true;
    }
    return wait_on_timed_cond(addr, value, is_monotonic, sec, ns);
#    else
    if (!has_timeout) {
        const int ret = ::_umtx_op(addr, UMTX_OP_WAIT_UINT, static_cast<::u_long>(value), nullptr, nullptr);
        return ret == 0 || errno != ETIMEDOUT;
    }
    ::_umtx_time timeout{};
    timeout._timeout = ::timespec{static_cast<::time_t>(sec), static_cast<long>(ns)};
    timeout._flags = UMTX_ABSTIME;
    timeout._clockid = is_monotonic ? CLOCK_MONOTONIC : CLOCK_REALTIME;
    const int ret = ::_umtx_op(addr, UMTX_OP_WAIT_UINT, static_cast<::u_long>(value),
                               reinterpret_cast<void*>(static_cast<uintptr_t>(sizeof(timeout))), &timeout);
    return ret == 0 || errno != ETIMEDOUT;
#    endif

#elif defined(NEFORCE_PLATFORM_MACOS)
#    ifdef NEFORCE_ASYNC_BSD_COND_FALLBACK
    if (!has_timeout) {
        wait_on_cond(addr, value);
        return true;
    }
    return wait_on_timed_cond(addr, value, is_monotonic, sec, ns);
#    else
    uint64_t timeout_ns = 0;
    if (has_timeout) {
        relative_timeout_ns(is_monotonic, sec, ns, timeout_ns);
        if (timeout_ns == 0) {
            timeout_ns = 1;
        }
    }
    const int ret = ::__ulock_wait2(UL_COMPARE_AND_WAIT, addr, static_cast<uint64_t>(value), timeout_ns, 0);
    return ret == 0 || errno != ETIMEDOUT;
#    endif

#else
    int oper = FUTEX_WAIT_BITSET | FUTEX_PRIVATE_FLAG;
    if (!is_monotonic) {
        oper |= FUTEX_CLOCK_REALTIME;
    }

    if (has_timeout) {
        ::timespec ts{static_cast<ssize_t>(sec), static_cast<ssize_t>(ns)};
        const long ret = ::syscall(SYS_futex, addr, oper, value, &ts, nullptr, FUTEX_BITSET_MATCH_ANY);
        return ret != -1 || errno != ETIMEDOUT;
    }

    const auto ret = ::syscall(SYS_futex, addr, oper, value, nullptr, nullptr, FUTEX_BITSET_MATCH_ANY);
    return ret != -1 || errno != EAGAIN;
#endif
}

void futex_wait(void* addr, platform_wait_t value) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    auto* p = static_cast<volatile platform_wait_t*>(addr);
    const ::BOOL result = ::WaitOnAddress(p, &value, sizeof(platform_wait_t), numeric_traits<::DWORD>::max());

    if (result == 0) {
        const ::DWORD err = ::GetLastError();
        if (err != 0 && err != ERROR_TIMEOUT) {
            terminate();
        }
    }
#elif defined(NEFORCE_PLATFORM_FREEBSD)
#    ifdef NEFORCE_ASYNC_BSD_COND_FALLBACK
    wait_on_cond(addr, value);
#    else
    for (;;) {
        const int ret = ::_umtx_op(addr, UMTX_OP_WAIT_UINT, static_cast<::u_long>(value), nullptr, nullptr);
        if (ret == 0 || errno == EAGAIN) {
            return;
        }
        if (errno != EINTR) {
            return;
        }
    }
#    endif
#elif defined(NEFORCE_PLATFORM_MACOS)
#    ifdef NEFORCE_ASYNC_BSD_COND_FALLBACK
    wait_on_cond(addr, value);
#    else
    for (;;) {
        const int ret = ::__ulock_wait2(UL_COMPARE_AND_WAIT, addr, static_cast<uint64_t>(value), 0, 0);
        if (ret == 0 || errno == EAGAIN) {
            return;
        }
        if (errno != EINTR) {
            return;
        }
    }
#    endif
#else
    const auto err =
            ::syscall(SYS_futex, addr, static_cast<platform_wait_t>(futex_wait_flags::wait_private), value, nullptr);

    if (err == 0) {
        return;
    }
    if (errno == EAGAIN) {
        return;
    }
    terminate();
#endif
}

void futex_notify(void* addr, const bool all) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    auto* p = static_cast<platform_wait_t*>(addr);
    if (all) {
        ::WakeByAddressAll(p);
    } else {
        ::WakeByAddressSingle(p);
    }
#elif defined(NEFORCE_PLATFORM_FREEBSD)
#    ifdef NEFORCE_ASYNC_BSD_COND_FALLBACK
    (void) addr;
    (void) all;
    wake_timed_waiters();
#    else
    (void) ::_umtx_op(addr, UMTX_OP_WAKE,
                      all ? static_cast<::u_long>(numeric_traits<int32_t>::max()) : static_cast<::u_long>(1), nullptr,
                      nullptr);
#    endif
#elif defined(NEFORCE_PLATFORM_MACOS)
#    ifdef NEFORCE_ASYNC_BSD_COND_FALLBACK
    (void) addr;
    (void) all;
    wake_timed_waiters();
#    else
    (void) ::__ulock_wake(UL_COMPARE_AND_WAIT | (all ? ULF_WAKE_ALL : 0), addr, 0);
#    endif
#else
    ::syscall(SYS_futex, addr, static_cast<platform_wait_t>(futex_wait_flags::wake_private),
              all ? numeric_traits<platform_wait_t>::max() : 1);
#endif
}

#ifdef NEFORCE_PLATFORM_LINUX

int futex(void* wait_addr, futex_wait_flags flags, int wake_count, void* requeue_addr, int requeue_count,
          int cmp_value) noexcept {
    const long ret = ::syscall(SYS_futex, wait_addr, static_cast<int>(flags), wake_count, requeue_count, requeue_addr,
                               cmp_value);
    if (ret == -1) {
        return -errno;
    }
    return 0;
}

int futex_requeue(void* wait_addr, int wake_count, void* requeue_addr, int requeue_count) noexcept {
    const long ret = ::syscall(SYS_futex, wait_addr, static_cast<int>(futex_wait_flags::requeue) | FUTEX_PRIVATE_FLAG,
                               wake_count, requeue_count, requeue_addr, 0);
    if (ret == -1) {
        return -errno;
    }
    return 0;
}

int futex_wake_op(void* addr, int wake_count, void* op_addr, int op_arg, int op, int cmp, int cmp_arg) noexcept {
    const unsigned int val3 = (op << 28) | (cmp << 24) | (op_arg << 12) | cmp_arg;
    const long ret = ::syscall(SYS_futex, addr, static_cast<int>(futex_wait_flags::wake_op) | FUTEX_PRIVATE_FLAG,
                               wake_count, op_addr, nullptr, val3);
    if (ret == -1) {
        return -errno;
    }
    return 0;
}

int futex_lock_pi(void* addr) noexcept {
    const long ret = ::syscall(SYS_futex, addr, static_cast<int>(futex_wait_flags::lock_pi) | FUTEX_PRIVATE_FLAG,
                               nullptr, nullptr, nullptr, 0);
    if (ret == -1) {
        return -errno;
    }
    return 0;
}

int futex_trylock_pi(void* addr) noexcept {
    const long ret = ::syscall(SYS_futex, addr, static_cast<int>(futex_wait_flags::trylock_pi) | FUTEX_PRIVATE_FLAG,
                               nullptr, nullptr, nullptr, 0);
    if (ret == -1) {
        return -errno;
    }
    return 0;
}

int futex_unlock_pi(void* addr) noexcept {
    const long ret = ::syscall(SYS_futex, addr, static_cast<int>(futex_wait_flags::unlock_pi) | FUTEX_PRIVATE_FLAG,
                               nullptr, nullptr, nullptr, 0);
    if (ret == -1) {
        return -errno;
    }
    return 0;
}

int futex_wait_requeue_pi(void* wait_addr, int value, void* requeue_addr) noexcept {
    const long ret =
            ::syscall(SYS_futex, wait_addr, static_cast<int>(futex_wait_flags::wait_requeue_pi) | FUTEX_PRIVATE_FLAG,
                      value, requeue_addr, nullptr, 0);
    if (ret == -1) {
        return -errno;
    }
    return 0;
}

#endif // NEFORCE_PLATFORM_LINUX

NEFORCE_END_NAMESPACE__
