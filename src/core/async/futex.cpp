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
#    include <errno.h>
#    include <linux/futex.h>
#    include <syscall.h>
#    include <time.h>
#    include <unistd.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

bool futex_wait_until(void* addr, platform_wait_t value, const bool has_timeout, const int64_t sec, const int64_t ns,
                      const bool is_monotonic) {

#ifdef NEFORCE_PLATFORM_WINDOWS
    if (!has_timeout) {
        return ::WaitOnAddress(addr, &value, sizeof(::DWORD), numeric_traits<::DWORD>::max()) == 1;
    }

    const auto ms = relative_time(sec, ns, is_monotonic);
    if (::WaitOnAddress(addr, &value, sizeof(::DWORD), ms.count())) {
        return true;
    }

    const ::DWORD err = ::GetLastError();
    if (err == ERROR_TIMEOUT) {
        return false;
    }
    return false;

#else
    int oper = FUTEX_WAIT_BITSET | FUTEX_PRIVATE_FLAG;
    if (!is_monotonic) {
        oper |= FUTEX_CLOCK_REALTIME;
    }

    if (has_timeout) {
        ::timespec ts{static_cast<ssize_t>(sec), static_cast<ssize_t>(ns)};

        const long ret = ::syscall(SYS_futex, addr, oper, value, &ts, nullptr, FUTEX_BITSET_MATCH_ANY);

        if (ret == -1 && errno == ETIMEDOUT) {
            return false;
        }
        return true;
    }

    ::syscall(SYS_futex, addr, oper, value, nullptr, nullptr, FUTEX_BITSET_MATCH_ANY);
    return true;
#endif
}

void futex_wait(void* addr, platform_wait_t value) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    auto p = static_cast<volatile platform_wait_t*>(addr);
    const ::BOOL result = ::WaitOnAddress(p, &value, sizeof(platform_wait_t), numeric_traits<::DWORD>::max());

    if (result == 0) {
        ::DWORD err = ::GetLastError();
        if (err != 0 && err != ERROR_TIMEOUT) {
            terminate();
        }
    }
#else
    const auto err =
            ::syscall(SYS_futex, addr, static_cast<platform_wait_t>(futex_wait_flags::wait_private), value, nullptr);

    if (!err) {
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
    auto p = static_cast<platform_wait_t*>(addr);
    if (all) {
        ::WakeByAddressAll(p);
    } else {
        ::WakeByAddressSingle(p);
    }
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

int futex_cmp_requeue(void* wait_addr, int wake_count, void* requeue_addr, int requeue_count, int cmp_value) noexcept {
    const long ret =
            ::syscall(SYS_futex, wait_addr, static_cast<int>(futex_wait_flags::cmp_requeue) | FUTEX_PRIVATE_FLAG,
                      wake_count, requeue_count, requeue_addr, cmp_value);
    if (ret == -1) {
        return -errno;
    }
    return 0;
}

int futex_wake_op(void* addr, int wake_count, void* op_addr, int op_arg, int op, int cmp, int cmp_arg) noexcept {
    const unsigned int val3 = (op << 28) | (cmp << 24) | (op_arg << 12) | (cmp_arg);
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

int futex_cmp_requeue_pi(void* wait_addr, int wake_count, void* requeue_addr, int cmp_value) noexcept {
    const long ret =
            ::syscall(SYS_futex, wait_addr, static_cast<int>(futex_wait_flags::cmp_requeue_pi) | FUTEX_PRIVATE_FLAG,
                      wake_count, requeue_addr, nullptr, cmp_value);
    if (ret == -1) {
        return -errno;
    }
    return 0;
}

#endif // NEFORCE_PLATFORM_LINUX

NEFORCE_END_NAMESPACE__
