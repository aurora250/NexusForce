#include <NeForce/core/async/thread.hpp>
#include <NeForce/core/time/clocks.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#include <windef.h>
#include <process.h>
#include <WinBase.h>
#endif
#ifdef NEFORCE_PLATFORM_LINUX
#include <linux/futex.h>
#include <cerrno>
#include <ctime>
#include <unistd.h>
#ifndef FUTEX_BITSET_MATCH_ANY
#define FUTEX_BITSET_MATCH_ANY 0xffffffff
#endif
#endif
NEFORCE_BEGIN_NAMESPACE__

void thread::start_thread_impl(void* args) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    unsigned int thread_id;
    handle_ = reinterpret_cast<native_handle_type>(
        ::_beginthreadex(nullptr, 0, thread_entry, args, 0, &thread_id)
    );
    if (handle_ == nullptr) {
        throw_exception(thread_exception("Failed to create thread"));
    }
    id_ = id(thread_id);
#else
    native_handle_type tid;
    if (::pthread_create(&tid, nullptr, thread_entry, args) != 0) {
        throw_exception(thread_exception("Failed to create thread"));
    }
    handle_ = tid;
    id_ = id(tid);
#endif
}

thread::thread(thread&& other) noexcept
    : handle_(other.handle_), id_(other.id_), state_(other.state_) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    other.handle_ = nullptr;
#else
    other.handle_ = native_handle_type{};
#endif
    other.id_ = id{};
    other.state_ = NOT_A_THREAD;
}

thread& thread::operator =(thread&& other) noexcept {
    if (this != &other) {
        if (joinable()) {
            _NEFORCE terminate();
        }

        handle_ = other.handle_;
        id_ = other.id_;
        state_ = other.state_;
#ifdef NEFORCE_PLATFORM_WINDOWS
        other.handle_ = nullptr;
#else
        other.handle_ = native_handle_type{};
#endif
        other.id_ = id{};
        other.state_ = NOT_A_THREAD;
    }
    return *this;
}

thread::~thread() {
    if (joinable()) {
        _NEFORCE terminate();
    }
}

void thread::join() {
    if (!joinable()) {
        throw_exception(thread_exception("Thread is not joinable"));
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    if (::WaitForSingleObject(handle_, numeric_traits<::DWORD>::max()) != WAIT_OBJECT_0) {
        throw_exception(thread_exception("Fail to join thread"));
    }
    ::CloseHandle(handle_);
    handle_ = nullptr;
#else
    if (::pthread_join(handle_, nullptr) != 0) {
        throw_exception(thread_exception("Thread is not joinable"));
    }
    handle_ = native_handle_type{};
#endif
    state_ = JOINED;
}

void thread::detach() {
    if (!joinable()) {
        throw_exception(thread_exception("Thread is not detachable"));
    }
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (::CloseHandle(handle_) == FALSE) {
        throw_exception(thread_exception("Fail to detach thread"));
    }
    handle_ = nullptr;
#else
    if (::pthread_detach(handle_) != 0) {
        throw_exception(thread_exception("Fail to detach thread"));
    }
    handle_ = native_handle_type{};
#endif
    state_ = DETACHED;
}

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
        if (ms == 0) return;
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

            auto remaining = _NEFORCE time_cast<milliseconds>(target_time - now);

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
    if (ms == 0) return;

    if (ms >= 1000) {
        sleep_for_ms(static_cast<uint32_t>(ms / 1000), true);
        ms %= 1000;
        if (ms == 0) return;
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
        if (ns == 0) return;
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
    if (priority >= 90) win_priority = THREAD_PRIORITY_TIME_CRITICAL;
    else if (priority >= 70) win_priority = THREAD_PRIORITY_HIGHEST;
    else if (priority >= 50) win_priority = THREAD_PRIORITY_ABOVE_NORMAL;
    else if (priority >= 30) win_priority = THREAD_PRIORITY_NORMAL;
    else if (priority >= 10) win_priority = THREAD_PRIORITY_BELOW_NORMAL;
    else win_priority = THREAD_PRIORITY_LOWEST;
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
