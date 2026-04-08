#include <NeForce/core/async/this_thread.hpp>
#include <NeForce/core/time/clocks.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <windef.h>
#    include <WinBase.h>
#endif
#ifdef NEFORCE_PLATFORM_LINUX
#    include <ctime>
#    include <pthread.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

namespace {
    int64_t qpc_frequency() noexcept {
        static const int64_t freq = []() {
            ::LARGE_INTEGER f;
            ::QueryPerformanceFrequency(&f);
            return f.QuadPart;
        }();
        return freq;
    }
} // namespace


NEFORCE_BEGIN_THIS_THREAD__

void sleep_for_ms(const uint32_t ms, const bool busy_wait) noexcept {
    if (!busy_wait) {
#ifdef NEFORCE_PLATFORM_WINDOWS
        ::Sleep(ms);
#else
        ::timespec ts;
        ts.tv_sec = ms / 1000;
        ts.tv_nsec = static_cast<long>((ms % 1000) * 1000000L);
        ::nanosleep(&ts, nullptr);
#endif
        return;
    }

    if (ms == 0) {
        return;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    const int64_t freq = qpc_frequency();

    ::LARGE_INTEGER start{};
    ::LARGE_INTEGER now{};
    ::QueryPerformanceCounter(&start);

    while (true) {
        ::QueryPerformanceCounter(&now);
        const auto elapsed_ms = static_cast<uint64_t>((now.QuadPart - start.QuadPart) * 1000 / freq);

        if (elapsed_ms >= ms) {
            break;
        }

        const uint64_t remaining = ms - elapsed_ms;
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
    const auto target_time = clock::now() + milliseconds(ms);

    while (true) {
        const auto now = clock::now();
        if (now >= target_time) {
            break;
        }

        const auto remaining = time_cast<milliseconds>(target_time - now);

        if (remaining.count() > 1) {
            const auto half = remaining.count() / 2;
            if (half > 0) {
                ::timespec ts;
                ts.tv_sec = static_cast<time_t>(half / 1000);
                ts.tv_nsec = static_cast<long>((half % 1000) * 1000000L);
                ::nanosleep(&ts, nullptr);
            }
        } else {
            this_thread::relax();
        }
    }
#endif
}

void sleep_for_us(uint64_t us) noexcept {
    if (us == 0) {
        return;
    }

    if (us >= 1000) {
        sleep_for_ms(static_cast<uint32_t>(us / 1000), true);
        us %= 1000;
        if (us == 0) {
            return;
        }
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    const int64_t freq = qpc_frequency();

    ::LARGE_INTEGER start{};
    ::QueryPerformanceCounter(&start);
    const int64_t target_ticks = start.QuadPart + (freq / 1000000LL * static_cast<int64_t>(us));

    ::LARGE_INTEGER now{};
    do {
        ::QueryPerformanceCounter(&now);
        this_thread::relax();
    } while (now.QuadPart < target_ticks);
#else
    using clock = steady_clock;
    const auto target_time = clock::now() + microseconds(us);

    while (clock::now() < target_time) {
        this_thread::relax();
    }
#endif
}

void sleep_for_ns(uint64_t ns) noexcept {
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
        ns %= 1000000;
        if (ns == 0) {
            return;
        }
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    const int64_t freq = qpc_frequency();

    ::LARGE_INTEGER start{};
    ::QueryPerformanceCounter(&start);
    const int64_t target_ticks = start.QuadPart + (freq * static_cast<int64_t>(ns) / 1000000000LL);

    ::LARGE_INTEGER now{};
    do {
        ::QueryPerformanceCounter(&now);
        ::_mm_pause();
    } while (now.QuadPart < target_ticks);
#else
    using clock = steady_clock;
    const auto target_time = clock::now() + nanoseconds(ns);

    while (clock::now() < target_time) {
        this_thread::relax();
    }
#endif
}

bool affinity(size_t cpu_mask) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    return ::SetThreadAffinityMask(::GetCurrentThread(), static_cast<::DWORD_PTR>(cpu_mask)) != 0;
#else
    ::cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    for (size_t i = 0; i < sizeof(cpu_mask) * 8; ++i) {
        if (cpu_mask & (static_cast<size_t>(1) << i)) {
            CPU_SET(i, &cpuset);
        }
    }
    return ::pthread_setaffinity_np(::pthread_self(), sizeof(::cpu_set_t), &cpuset) == 0;
#endif
}

bool priority(int priority) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    int win_priority = THREAD_PRIORITY_LOWEST;
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
    }
    return ::SetThreadPriority(::GetCurrentThread(), win_priority) != 0;
#else
    int policy;
    ::sched_param param;
    if (::pthread_getschedparam(::pthread_self(), &policy, &param) != 0) {
        return false;
    }
    const int min_p = ::sched_get_priority_min(policy);
    const int max_p = ::sched_get_priority_max(policy);
    param.sched_priority = min_p + (priority * (max_p - min_p)) / 100;
    return ::pthread_setschedparam(::pthread_self(), policy, &param) == 0;
#endif
}

NEFORCE_END_THIS_THREAD__
NEFORCE_END_NAMESPACE__
