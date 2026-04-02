#include <NeForce/core/async/this_thread.hpp>
#include <NeForce/core/time/clocks.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <WinBase.h>
#    include <windef.h>
#endif
#ifdef NEFORCE_PLATFORM_LINUX
#    include <ctime>
#    include <pthread.h>
#endif
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_THIS_THREAD__

void NEFORCE_API sleep_for_ms(const uint32_t ms, const bool busy_wait) noexcept {
    if (!busy_wait) {
#ifdef NEFORCE_PLATFORM_WINDOWS
        ::Sleep(ms);
#else
        ::timespec ts;
        ts.tv_sec = ms / 1000;
        ts.tv_nsec = (ms % 1000) * 1000000L;
        ::nanosleep(&ts, nullptr);
#endif
    } else {
        if (ms == 0) {
            return;
        }
#ifdef NEFORCE_PLATFORM_WINDOWS
        static ::LARGE_INTEGER frequency{0};
        if (frequency.QuadPart == 0) {
            ::QueryPerformanceFrequency(&frequency);
        }

        ::LARGE_INTEGER start, now;
        ::QueryPerformanceCounter(&start);

        while (true) {
            ::QueryPerformanceCounter(&now);
            const uint64_t elapsed = now.QuadPart - start.QuadPart;
            const uint64_t elapsed_ms = (elapsed * 1000) / frequency.QuadPart;

            if (elapsed_ms >= ms) {
                break;
            }

            uint64_t remaining = ms - elapsed_ms;
            if (remaining > 1) {
                ::Sleep(static_cast<::DWORD>(remaining / 2));
            } else {
                for (int i = 0; i < 100; ++i) {
                    this_thread::relax();
                }
            }
        }
#else
        using clock = steady_clock;
        const auto start = clock::now();
        const auto target_time = start + milliseconds(ms);

        while (true) {
            auto now = clock::now();
            if (now >= target_time) {
                break;
            }

            auto remaining = time_cast<milliseconds>(target_time - now);

            if (remaining.count() > 1) {
                ::timespec ts;
                ts.tv_sec = remaining.count() / 1000 / 2;
                ts.tv_nsec = ((remaining.count() / 2) % 1000) * 1000000L;
                ::nanosleep(&ts, nullptr);
            } else {
                this_thread::relax();
            }
        }
#endif
    }
}

void NEFORCE_API sleep_for_us(uint64_t ms) noexcept {
    if (ms == 0) {
        return;
    }

    if (ms >= 1000) {
        sleep_for_ms(static_cast<uint32_t>(ms / 1000), true);
        ms %= 1000;
        if (ms == 0) {
            return;
        }
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    static ::LARGE_INTEGER frequency{0};
    if (frequency.QuadPart == 0) {
        ::QueryPerformanceFrequency(&frequency);
    }

    ::LARGE_INTEGER start;
    ::QueryPerformanceCounter(&start);

    const uint64_t target_ticks = start.QuadPart + (frequency.QuadPart * ms) / 1000000;
    ::LARGE_INTEGER now;
    do {
        ::QueryPerformanceCounter(&now);
        this_thread::relax();
    } while (now.QuadPart < target_ticks);
#else
    using clock = system_clock;
    const auto start = clock::now();
    const auto target_time = start + microseconds(ms);

    while (clock::now() < target_time) {
        this_thread::relax();
    }
#endif
}

void NEFORCE_API sleep_for_ns(uint64_t ns) noexcept {
    if (ns < 1000) {
        for (uint64_t i = 0; i < ns / 10; ++i) {
#ifdef NEFORCE_COMPILER_MSVC
            ::_mm_mfence();
#else
            asm volatile("" ::: "memory");
#endif
            this_thread::relax();
        }
        return;
    }

    if (ns >= 1000000) {
        sleep_for_us(ns / 1000);
        ns %= 1000;
        if (ns == 0) {
            return;
        }
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    static ::LARGE_INTEGER frequency{0};
    if (frequency.QuadPart == 0) {
        ::QueryPerformanceFrequency(&frequency);
    }

    ::LARGE_INTEGER start;
    ::QueryPerformanceCounter(&start);
    const uint64_t target_ticks = start.QuadPart + (frequency.QuadPart * ns) / 1000000000;

    ::LARGE_INTEGER now;
    do {
        ::QueryPerformanceCounter(&now);
        ::_mm_pause();
    } while (now.QuadPart < target_ticks);
#else
    using clock = system_clock;
    auto start = clock::now();
    auto target_time = start + nanoseconds(ns);

    while (clock::now() < target_time) {
        asm volatile("pause" ::: "memory");
    }
#endif
}

bool NEFORCE_API affinity(size_t cpu_mask) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    return ::SetThreadAffinityMask(::GetCurrentThread(), cpu_mask) != 0;
#else
    ::cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    for (size_t i = 0; i < sizeof(cpu_mask) * 8; ++i) {
        if (cpu_mask & (1ULL << i)) {
            CPU_SET(i, &cpuset);
        }
    }
    return ::pthread_setaffinity_np(::pthread_self(), sizeof(cpu_set_t), &cpuset) == 0;
#endif
}

bool NEFORCE_API priority(int priority) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    int win_priority;
    if (priority >= 90) {
        win_priority = THREAD_PRIORITY_TIME_CRITICAL;
    } else if (priority >= 70) {
        win_priority = THREAD_PRIORITY_HIGHEST;
    } else if (priority >= 50) {
        win_priority = THREAD_PRIORITY_ABOVE_NORMAL;
    } else if (priority >= 30) {
        win_priority = THREAD_PRIORITY_NORMAL;
    } else if (priority >= 10) {
        win_priority = THREAD_PRIORITY_BELOW_NORMAL;
    } else {
        win_priority = THREAD_PRIORITY_LOWEST;
    }
    return ::SetThreadPriority(::GetCurrentThread(), win_priority) != 0;
#else
    int policy;
    ::sched_param param;
    if (::pthread_getschedparam(::pthread_self(), &policy, &param) != 0) {
        return false;
    }
    const int min_priority = ::sched_get_priority_min(policy);
    const int max_priority = ::sched_get_priority_max(policy);
    param.sched_priority = min_priority + (priority * (max_priority - min_priority)) / 100;
    return ::pthread_setschedparam(::pthread_self(), policy, &param) == 0;
#endif
}

NEFORCE_END_THIS_THREAD__
NEFORCE_END_NAMESPACE__
