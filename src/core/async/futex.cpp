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
#    include <pthread.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

#ifdef NEFORCE_PLATFORM_FREEBSD
namespace {
    ::pthread_mutex_t& futex_mutex() noexcept {
        static ::pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
        return m;
    }

    ::pthread_cond_t& futex_cond() noexcept {
        static ::pthread_cond_t c = PTHREAD_COND_INITIALIZER;
        return c;
    }

    ::timespec futex_deadline(const bool is_monotonic, const int64_t sec, const int64_t ns) noexcept {
        if (!is_monotonic) {
            return ::timespec{static_cast<::time_t>(sec), static_cast<long>(ns)};
        }
        ::timespec mono{};
        ::timespec real{};
        ::clock_gettime(CLOCK_MONOTONIC, &mono);
        ::clock_gettime(CLOCK_REALTIME, &real);
        int64_t delta =
                (sec - static_cast<int64_t>(mono.tv_sec)) * 1000000000LL + (ns - static_cast<int64_t>(mono.tv_nsec));
        delta = max<int64_t>(delta, 0);
        const int64_t total =
                static_cast<int64_t>(real.tv_sec) * 1000000000LL + static_cast<int64_t>(real.tv_nsec) + delta;
        return ::timespec{static_cast<::time_t>(total / 1000000000LL), static_cast<long>(total % 1000000000LL)};
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
    const auto* word = static_cast<const volatile platform_wait_t*>(addr);
    ::pthread_mutex_lock(&futex_mutex());
    bool timed_out = false;
    const ::timespec deadline = futex_deadline(is_monotonic, sec, ns);
    while (*word == value) {
        if (!has_timeout) {
            ::pthread_cond_wait(&futex_cond(), &futex_mutex());
        } else if (::pthread_cond_timedwait(&futex_cond(), &futex_mutex(), &deadline) == ETIMEDOUT) {
            timed_out = true;
            break;
        }
    }
    ::pthread_mutex_unlock(&futex_mutex());
    return !timed_out;
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
    const auto* word = static_cast<const volatile platform_wait_t*>(addr);
    ::pthread_mutex_lock(&futex_mutex());
    while (*word == value) {
        ::pthread_cond_wait(&futex_cond(), &futex_mutex());
    }
    ::pthread_mutex_unlock(&futex_mutex());
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
    ::pthread_mutex_lock(&futex_mutex());
    if (all) {
        ::pthread_cond_broadcast(&futex_cond());
    } else {
        ::pthread_cond_signal(&futex_cond());
    }
    ::pthread_mutex_unlock(&futex_mutex());
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
