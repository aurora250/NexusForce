#include <NeForce/core/async/futex.hpp>
#include <NeForce/core/numeric/numeric_traits.hpp>
#include <NeForce/core/exception/terminate.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#include <NeForce/core/time/clocks.hpp>
#include <NeForce/core/config/windef.hpp>
#include <errhandlingapi.h>
#include <synchapi.h>
#include <winerror.h>
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
#endif
#ifdef NEFORCE_PLATFORM_LINUX
#include <linux/futex.h>
#include <syscall.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

bool futex_wait_until(
    void* addr, platform_wait_t value,
    const bool has_timeout, const int64_t sec, const int64_t ns,
    const bool is_monotonic) {

#ifdef NEFORCE_PLATFORM_WINDOWS
    if (!has_timeout) {
        return ::WaitOnAddress(
            addr, &value, sizeof(::DWORD),
            numeric_traits<::DWORD>::max()) == 1;
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
        ::timespec ts{ static_cast<ssize_t>(sec), static_cast<ssize_t>(ns) };

        const long ret = ::syscall(
            SYS_futex, addr, oper, value, &ts, nullptr,
            FUTEX_BITSET_MATCH_ANY);

        if (ret == -1 && errno == ETIMEDOUT) {
            return false;
        }
        return true;
    }

    ::syscall(
        SYS_futex, addr, oper, value, nullptr, nullptr,
        FUTEX_BITSET_MATCH_ANY);
    return true;
#endif
}

void futex_wait(void* addr, const platform_wait_t value) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    auto p = static_cast<volatile platform_wait_t*>(addr);
    const ::BOOL result = ::WaitOnAddress(
        p,
        const_cast<platform_wait_t*>(&value),
        sizeof(platform_wait_t),
        _NEFORCE numeric_traits<::DWORD>::max());

    if (result == 0) {
        ::DWORD err = ::GetLastError();
        if (err != 0 && err != ERROR_TIMEOUT) {
            _NEFORCE terminate();
        }
    }
#else
    const auto err = ::syscall(
        SYS_futex, addr,
        static_cast<platform_wait_t>(futex_wait_flags::wait_private),
        value, nullptr);

    if (!err) return;
    if (errno == EAGAIN) return;
    _NEFORCE terminate();
#endif
}

void futex_notify(void* addr, const bool all) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    const auto p = static_cast<volatile platform_wait_t*>(addr);
    if (all) {
        ::WakeByAddressAll(const_cast<platform_wait_t*>(p));
    } else {
        ::WakeByAddressSingle(const_cast<platform_wait_t*>(p));
    }
#else
    ::syscall(
        SYS_futex, addr,
        static_cast<platform_wait_t>(futex_wait_flags::wake_private),
        all ? numeric_traits<platform_wait_t>::max() : 1);
#endif
}

NEFORCE_END_NAMESPACE__
