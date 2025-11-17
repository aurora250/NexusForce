#ifndef MSTL_CORE_TIME_TIME_POINT_HPP__
#define MSTL_CORE_TIME_TIME_POINT_HPP__
#include "duration.hpp"
MSTL_BEGIN_NAMESPACE__
MSTL_BEGIN_CHRONO__

template <typename Clock, typename Dur>
struct time_point {
    static_assert(is_duration_v<Dur>, "duration must be a specialization of duration");

    using clock_type = Clock;
    using duration_type = Dur;
    using rep = typename duration_type::rep;
    using period = typename duration_type::period;

private:
    duration_type value_;

public:
    constexpr time_point() : value_(duration_type::zero()) {}
    constexpr explicit time_point(const duration_type& dur) : value_(dur) {}

    template <typename Dur2, typename = enable_if_t<is_convertible_v<Dur2, duration_type>>>
    constexpr time_point(const time_point<clock_type, Dur2>& value)
        : value_(value.value_) {}

    constexpr time_point& operator ++() {
        ++value_;
        return *this;
    }

    constexpr time_point operator ++(int) {
        return time_point{value_++};
    }

    constexpr time_point& operator --() {
        --value_;
        return *this;
    }

    constexpr time_point operator --(int) {
        return time_point{value_--};
    }

    constexpr time_point& operator +=(const duration_type& dur) {
        value_ += dur;
        return *this;
    }

    constexpr time_point& operator -=(const duration_type& dur) {
        value_ -= dur;
        return *this;
    }

    constexpr duration_type time_since_epoch() const noexcept {
	    return value_;
    }
    static constexpr time_point min() noexcept {
	    return time_point(duration_type::min());
    }
    static constexpr time_point max() noexcept {
	    return time_point(duration_type::max());
    }
};

template <typename ToDur, typename Clock, typename Dur, enable_if_t<is_duration_v<ToDur>, int> = 0>
constexpr time_point<Clock, ToDur> time_point_cast(const time_point<Clock, Dur>& time_point_value) {
    return time_point<Clock, ToDur>(duration_cast<ToDur>(time_point_value.time_since_epoch()));
}


template <typename Clock, typename Dur1, typename Rep2, typename Period2>
constexpr time_point<Clock, common_type_t<Dur1, duration<Rep2, Period2>>>
operator +(const time_point<Clock, Dur1>& lhs, const duration<Rep2, Period2>& rhs) {
    using duration2 = duration<Rep2, Period2>;
    using common_duration = common_type_t<Dur1, duration2>;
    using result_time_point = time_point<Clock, common_duration>;
    return result_time_point(lhs.time_since_epoch() + rhs);
}

template <typename Rep1, typename Period1, typename Clock, typename Dur2>
constexpr time_point<Clock, common_type_t<duration<Rep1, Period1>, Dur2>>
operator +(const duration<Rep1, Period1>& lhs, const time_point<Clock, Dur2>& rhs) {
    using duration1 = duration<Rep1, Period1>;
    using common_duration = common_type_t<duration1, Dur2>;
    using result_time_point = time_point<Clock, common_duration>;
    return result_time_point(rhs.time_since_epoch() + lhs);
}

template <typename Clock, typename Dur1, typename Rep2, typename Period2>
constexpr time_point<Clock, common_type_t<Dur1, duration<Rep2, Period2>>>
operator -(const time_point<Clock, Dur1>& lhs, const duration<Rep2, Period2>& rhs) {
    using duration2 = duration<Rep2, Period2>;
    using common_duration = common_type_t<Dur1, duration2>;
    using result_time_point = time_point<Clock, common_duration>;
    return result_time_point(lhs.time_since_epoch() - rhs);
}

template <typename Clock, typename Dur1, typename Dur2>
constexpr common_type_t<Dur1, Dur2>
operator -(const time_point<Clock, Dur1>& lhs, const time_point<Clock, Dur2>& rhs) {
    return lhs.time_since_epoch() - rhs.time_since_epoch();
}

template <typename Clock, typename Dur1, typename Dur2>
constexpr bool operator ==(const time_point<Clock, Dur1>& lhs, const time_point<Clock, Dur2>& rhs) {
    return lhs.time_since_epoch() == rhs.time_since_epoch();
}

template <typename Clock, typename Dur1, typename Dur2>
constexpr bool operator !=(const time_point<Clock, Dur1>& lhs, const time_point<Clock, Dur2>& rhs) {
    return !(lhs == rhs);
}

template <typename Clock, typename Dur1, typename Dur2>
constexpr bool operator <(const time_point<Clock, Dur1>& lhs, const time_point<Clock, Dur2>& rhs) {
    return lhs.time_since_epoch() < rhs.time_since_epoch();
}

template <typename Clock, typename Dur1, typename Dur2>
constexpr bool operator <=(const time_point<Clock, Dur1>& lhs, const time_point<Clock, Dur2>& rhs) {
    return !(rhs < lhs);
}

template <typename Clock, typename Dur1, typename Dur2>
constexpr bool operator >(const time_point<Clock, Dur1>& lhs, const time_point<Clock, Dur2>& rhs) {
    return rhs < lhs;
}

template <typename Clock, typename Dur1, typename Dur2>
constexpr bool operator >=(const time_point<Clock, Dur1>& lhs, const time_point<Clock, Dur2>& rhs) {
    return !(lhs < rhs);
}

MSTL_END_CHRONO__
MSTL_END_NAMESPACE__
#endif // MSTL_CORE_TIME_TIME_POINT_HPP__
