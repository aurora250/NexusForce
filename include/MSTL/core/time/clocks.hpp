#ifndef MSTL_CORE_TIME_CLOCKS_HPP__
#define MSTL_CORE_TIME_CLOCKS_HPP__
#include "time_point.hpp"
MSTL_BEGIN_NAMESPACE__
MSTL_BEGIN_CHRONO__

struct MSTL_API system_clock {
    using duration = _MSTL_CHRONO nanoseconds;
    using rep = duration::rep;
    using period = duration::period;
    using time_point = _MSTL_CHRONO time_point<system_clock>;

    static_assert(system_clock::duration::min() < system_clock::duration::zero(),
        "a clock's minimum duration cannot be less than its epoch");


    static constexpr bool is_steady = false;


    static time_point now() noexcept;

    static std::time_t to_time_t(const time_point& time_point_value) noexcept {
        return duration_cast<_MSTL_CHRONO seconds>(time_point_value.time_since_epoch()).count();
    }

    static time_point from_time_t(const std::time_t time_value) noexcept {
        using from_time_point = _MSTL_CHRONO time_point<system_clock, seconds>;
        return time_point_cast<system_clock::duration>(from_time_point(_MSTL_CHRONO seconds(time_value)));
    }
};

using high_resolution_clock = system_clock;


struct MSTL_API steady_clock {
    using duration = _MSTL_CHRONO nanoseconds;
    using rep = duration::rep;
    using period = duration::period;
    using time_point = _MSTL_CHRONO time_point<steady_clock, duration>;

    static constexpr bool is_steady = true;

    static time_point now() noexcept;
};

MSTL_END_CHRONO__

template <typename T>
struct is_clock;
template <>
struct is_clock<_MSTL_CHRONO system_clock> : true_type {};
template <>
struct is_clock<_MSTL_CHRONO steady_clock> : true_type {};

template <typename T>
MSTL_INLINE17 constexpr bool is_clock_v = is_clock<T>::value;


MSTL_BEGIN_THIS_THREAD__

template <typename Clock, typename Dur>
void sleep_until(const _MSTL_CHRONO time_point<Clock, Dur>& time) {
    auto current = Clock::now();
    if (Clock::is_steady) {
        if (current < time) {
            _THIS_THREAD sleep_for(time - current);
        }
        return;
    }
    while (current < time) {
        _THIS_THREAD sleep_for(time - current);
        current = Clock::now();
    }
}

MSTL_END_THIS_THREAD__

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_TIME_CLOCKS_HPP__
