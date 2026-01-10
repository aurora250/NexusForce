#include <MSTL/core/time/clocks.hpp>
#ifdef MSTL_PLATFORM_WINDOWS__
#include <Windows.h>
#endif
MSTL_BEGIN_NAMESPACE__

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

MSTL_END_NAMESPACE__

