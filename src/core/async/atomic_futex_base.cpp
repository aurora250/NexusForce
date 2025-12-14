#include <MSTL/core/async/atomic_futex_base.hpp>
#ifdef MSTL_PLATFORM_WINDOWS__
#include <Windows.h>
#include <synchapi.h>
#pragma comment(lib, "synchronization.lib")
#include <MSTL/core/config/undef_cmacro.hpp>
#else
#include <cerrno>
#include <linux/futex.h>
#include <unistd.h>
#include <syscall.h>
#ifndef FUTEX_BITSET_MATCH_ANY
#define FUTEX_BITSET_MATCH_ANY 0xffffffff
#endif
#endif

MSTL_BEGIN_NAMESPACE__

bool atomic_futex_base::futex_wait_until(unsigned *addr, const unsigned value,
    const bool has_timeout, const _MSTL_CHRONO seconds sec, const _MSTL_CHRONO nanoseconds ns) {
#ifdef MSTL_PLATFORM_WINDOWS__
    if (!has_timeout) {
        ::BOOL ret = ::WaitOnAddress(
            addr, const_cast<unsigned*>(&value), sizeof(unsigned),
            numeric_limits<uint32_t>::max());
        return ret == 1;
    } else {
        using namespace chrono;

        const auto tp = system_clock::from_time_t(0) + sec + ns;
        const auto now = system_clock::now();

        if (tp <= now) {
            return false;
        }

        const auto dur = duration_cast<milliseconds>(tp - now);
        ::DWORD ms;

        if (dur.count() < 0) {
            ms = 0;
        } else if (static_cast<unsigned long long>(
            dur.count()) > numeric_limits<uint32_t>::max() - 1) {
            ms = numeric_limits<uint32_t>::max() - 1;
        } else {
            ms = static_cast<::DWORD>(dur.count());
        }
        ::BOOL ret = ::WaitOnAddress(
            addr, const_cast<unsigned*>(&value), sizeof(unsigned), ms);

        if (ret) {
            return true;
        } else {
            const ::DWORD err = ::GetLastError();
            if (err == ERROR_TIMEOUT) {
                return false;
            }
            return false;
        }
    }
#else
    constexpr int oper = FUTEX_WAIT_BITSET | FUTEX_PRIVATE_FLAG | FUTEX_CLOCK_REALTIME;
    if (has_timeout) {
        ::timespec ts{};
        ts.tv_sec = sec.count();
        ts.tv_nsec = ns.count();
    
        const long ret = ::syscall(
            SYS_futex, addr, oper, value, &ts, nullptr,
            FUTEX_BITSET_MATCH_ANY);
    
        if (ret == -1 && errno == ETIMEDOUT) {
            return false;
        }
        return true;
    } else {
        ::syscall(
            SYS_futex, addr, oper, value, nullptr, nullptr,
            FUTEX_BITSET_MATCH_ANY);
        return true;
    }
#endif
}

bool atomic_futex_base::futex_wait_until_steady(unsigned *addr, const unsigned value,
    const bool has_timeout, const _MSTL_CHRONO seconds sec, const _MSTL_CHRONO nanoseconds ns) {
#ifdef MSTL_PLATFORM_WINDOWS__
    if (!has_timeout) {
        ::BOOL ret = ::WaitOnAddress(
            addr, const_cast<unsigned*>(&value), sizeof(unsigned),
            numeric_limits<uint32_t>::max());
        return ret == 1;
    } else {
        using namespace chrono;
        const steady_clock::time_point tp = steady_clock::time_point(sec) + ns;
        const auto now = steady_clock::now();

        if (tp <= now) {
            return false;
        }

        const auto dur = duration_cast<milliseconds>(tp - now);
        ::DWORD ms;

        if (dur.count() < 0) {
            ms = 0;
        } else if (static_cast<unsigned long long>(
            dur.count()) > numeric_limits<uint32_t>::max() - 1) {
            ms = numeric_limits<uint32_t>::max() - 1;
        } else {
            ms = static_cast<::DWORD>(dur.count());
        }

        ::BOOL ret = ::WaitOnAddress(
            addr, const_cast<unsigned*>(&value), sizeof(unsigned), ms);

        if (ret) {
            return true;
        } else {
            const ::DWORD err = ::GetLastError();
            if (err == ERROR_TIMEOUT) {
                return false;
            }
            return false;
        }
    }
#else
    constexpr int oper = FUTEX_WAIT_BITSET | FUTEX_PRIVATE_FLAG;
    if (has_timeout) {
        ::timespec ts{};
        ts.tv_sec = sec.count();
        ts.tv_nsec = ns.count();

        const long ret = ::syscall(
            SYS_futex, addr, oper, value, &ts, nullptr,
            FUTEX_BITSET_MATCH_ANY);
    
        if (ret == -1 && errno == ETIMEDOUT) {
            return false;
        }
        return true;
    } else {
        ::syscall(SYS_futex, addr, oper, value, nullptr, nullptr,
            FUTEX_BITSET_MATCH_ANY);
        return true;
    }
#endif
}

void atomic_futex_base::futex_notify_all(unsigned* addr) {
#ifdef MSTL_PLATFORM_WINDOWS__
    ::WakeByAddressAll(addr);
#else
    constexpr int oper = FUTEX_WAKE | FUTEX_PRIVATE_FLAG;
    ::syscall(SYS_futex, addr, oper, numeric_limits<int>::max(), nullptr, nullptr, 0);
#endif
}

MSTL_END_NAMESPACE__
