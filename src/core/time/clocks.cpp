#include <MSTL/core/time/clocks.hpp>
#ifdef MSTL_PLATFORM_WINDOWS__
#include <MSTL/core/config/windef.hpp>
#include <handleapi.h>
#include <profileapi.h>
#include <synchapi.h>
#include <sysinfoapi.h>
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
#endif
#ifdef MSTL_PLATFORM_LINUX__
#include <ctime>
#endif
MSTL_BEGIN_NAMESPACE__

MSTL_BEGIN_INNER__

void sleep_for_aux(int64_t s, int64_t ns) {
#ifdef MSTL_PLATFORM_WINDOWS__
    ::LARGE_INTEGER li{};
    li.QuadPart = -(ns / 100);

    const ::HANDLE timer = ::CreateWaitableTimerW(nullptr, 1, nullptr);
    if (timer) {
        ::SetWaitableTimer(timer, &li, 0, nullptr, nullptr, 0);
        ::WaitForSingleObject(timer, numeric_traits<::DWORD>::max());
        ::CloseHandle(timer);
    }
#elif defined(MSTL_PLATFORM_LINUX__)
    ::timespec ts{s, ns};
    while (::nanosleep(&ts, &ts) == -1) {}
#endif
}

MSTL_END_INNER__

system_clock::time_point system_clock::now() noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
    ::FILETIME ft;
    ::GetSystemTimePreciseAsFileTime(&ft);
    const uint64_t file_time = static_cast<uint64_t>(ft.dwHighDateTime) << 32 | ft.dwLowDateTime;
    constexpr uint64_t unix_epoch_offset = 116444736000000000ULL;

    const uint64_t ticks_since_unix_epoch = file_time - unix_epoch_offset;
    const uint64_t nanos = ticks_since_unix_epoch * 100;
    rep total_nanos = static_cast<rep>(nanos);
#elif defined(MSTL_PLATFORM_LINUX__)
    ::timespec ts{};
    ::clock_gettime(CLOCK_REALTIME, &ts);

    using rep = system_clock::rep;
    const rep total_nanos = static_cast<rep>(ts.tv_sec) * 1'000'000'000LL + static_cast<rep>(ts.tv_nsec);
#endif
    return time_point(duration(total_nanos));
}

steady_clock::time_point steady_clock::now() noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
    ::LARGE_INTEGER count;
    ::QueryPerformanceCounter(&count);

    static const ::LARGE_INTEGER freq = []() {
        ::LARGE_INTEGER f;
        ::QueryPerformanceFrequency(&f);
        return f;
    }();

    using rep = steady_clock::rep;
    const rep ticks = static_cast<rep>(count.QuadPart);
    const rep nanos_per_tick = 1'000'000'000LL / static_cast<rep>(freq.QuadPart);
    const rep remainder = 1'000'000'000LL % static_cast<rep>(freq.QuadPart);
    rep total_nanos = ticks * nanos_per_tick + (ticks * remainder) / static_cast<rep>(freq.QuadPart);
#elif defined(MSTL_PLATFORM_LINUX__)
    ::timespec ts{};
    ::clock_gettime(CLOCK_MONOTONIC, &ts);

    using rep = steady_clock::rep;
    const rep total_nanos = static_cast<rep>(ts.tv_sec) * 1'000'000'000LL + static_cast<rep>(ts.tv_nsec);
#endif
    return time_point(duration(total_nanos));
}

milliseconds relative_time(const int64_t sec, const int64_t nsec, const bool is_monotonic) {
    const nanoseconds abs_ns = seconds(sec) + nanoseconds(nsec);

    nanoseconds diff_ns;
    if (is_monotonic) {
        diff_ns = abs_ns - steady_clock::now().since_epoch();
    } else {
        const auto abs_time = system_clock::time_point(abs_ns);
        diff_ns = abs_time - system_clock::now();
    }
    if (diff_ns <= 0_ns) return 0_ms;

    const milliseconds diff_ms = diff_ns.to_milli();
    constexpr int64_t max_uint32 = numeric_traits<uint32_t>::max();
    if (diff_ms.count() > max_uint32 - 1) {
        return milliseconds(max_uint32 - 1);
    }
    return diff_ms;
}

MSTL_END_NAMESPACE__

