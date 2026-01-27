#include <MSTL/core/async/futex.hpp>
#include <MSTL/core/numeric/numeric_traits.hpp>
#include <MSTL/core/exception/terminate.hpp>
#ifdef MSTL_PLATFORM_WINDOWS__
#include <MSTL/core/time/clocks.hpp>
#include <Windows.h>
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
#endif
#ifdef MSTL_PLATFORM_LINUX__
#include <linux/futex.h>
#include <syscall.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#endif
MSTL_BEGIN_NAMESPACE__

void futex_wait(void* addr, const int32_t value) noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
    const platform_wait_t pval(value);
    auto p = static_cast<volatile platform_wait_t*>(addr);
    const ::BOOL result = ::WaitOnAddress(
        p,
        const_cast<platform_wait_t*>(&pval),
        sizeof(platform_wait_t),
        _MSTL numeric_traits<::DWORD>::max());

    if (result == 0) {
        ::DWORD err = ::GetLastError();
        if (err != 0 && err != ERROR_TIMEOUT) {
            _MSTL terminate();
        }
    }
#else
    const auto err = ::syscall(
        SYS_futex, addr,
        static_cast<platform_wait_t>(futex_wait_flags::wait_private),
        value, nullptr);

    if (!err) return;
    if (errno == EAGAIN) return;
    _MSTL terminate();
#endif
}

bool futex_wait_until(void* addr, int32_t value,
    const bool has_timeout, const int64_t sec, const int64_t ns) {
#ifdef MSTL_PLATFORM_WINDOWS__
    if (!has_timeout) {
        return ::WaitOnAddress(
            addr, &value, sizeof(::DWORD),
            numeric_traits<::DWORD>::max()) == 1;
    }
    const auto tp =
            system_clock::from_seconds(0_s) + seconds(sec) + nanoseconds(ns);
    const auto now = system_clock::now();

    if (tp <= now) {
        return false;
    }

    const milliseconds dur = time_cast<milliseconds>(tp - now).to_milli();
    ::DWORD ms;

    if (dur.count() < 0) {
        ms = 0;
    } else if (dur.count() > numeric_traits<::DWORD>::max() - 1) {
        ms = numeric_traits<::DWORD>::max() - 1;
    } else {
        ms = static_cast<::DWORD>(dur.count());
    }

    if (::WaitOnAddress(addr, &value, sizeof(::DWORD), ms)) {
        return true;
    }
    const ::DWORD err = ::GetLastError();
    if (err == ERROR_TIMEOUT) {
        return false;
    }
    return false;
#else
    constexpr int oper = FUTEX_WAIT_BITSET | FUTEX_PRIVATE_FLAG | FUTEX_CLOCK_REALTIME;
    if (has_timeout) {
        ::timespec ts{ sec, ns };

        const long ret = ::syscall(
            SYS_futex, addr, oper, value, &ts, nullptr,
            FUTEX_BITSET_MATCH_ANY);

        if (ret == -1 && errno == ETIMEDOUT) {
            return false;
        }
        return true;
    }
    ::syscall(SYS_futex, addr, oper, value, nullptr, nullptr,
              FUTEX_BITSET_MATCH_ANY);
    return true;
#endif
}

bool futex_wait_until_steady(void* addr, int32_t value,
    const bool has_timeout, const int64_t sec, const int64_t ns) {
#ifdef MSTL_PLATFORM_WINDOWS__
    if (!has_timeout) {
        return ::WaitOnAddress(
            addr, &value, sizeof(::DWORD),
            numeric_traits<::DWORD>::max()) == 1;
    }
    const steady_clock::time_point tp =
        steady_clock::time_point(seconds(sec)) + nanoseconds(ns);
    const auto now = steady_clock::now();

    if (tp <= now) {
        return false;
    }

    const milliseconds dur = time_cast<milliseconds>(tp - now).to_milli();
    ::DWORD ms;

    if (dur.count() < 0) {
        ms = 0;
    } else if (dur.count() > numeric_traits<::DWORD>::max() - 1) {
        ms = numeric_traits<::DWORD>::max() - 1;
    } else {
        ms = static_cast<::DWORD>(dur.count());
    }

    if (::WaitOnAddress(addr, &value, sizeof(::DWORD), ms)) {
        return true;
    }
    const ::DWORD err = ::GetLastError();
    if (err == ERROR_TIMEOUT) {
        return false;
    }
    return false;
#else
    constexpr int oper = FUTEX_WAIT_BITSET | FUTEX_PRIVATE_FLAG;
    if (has_timeout) {
        ::timespec ts{ sec, ns };

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

void futex_notify(void* addr, const bool all) noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
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

MSTL_END_NAMESPACE__
