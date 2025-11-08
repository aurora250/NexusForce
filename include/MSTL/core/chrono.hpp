#ifndef MSTL_CHRONO_HPP__
#define MSTL_CHRONO_HPP__
#include "ratio.hpp"
#include "mathlib.hpp"
#include <ctime> // std::time_t
#ifdef MSTL_PLATFORM_WINDOWS__
#include <Windows.h>
#include "undef_cmacro.hpp"
#endif
MSTL_BEGIN_NAMESPACE__

MSTL_BEGIN_CHRONO__

template <typename Rep, typename Period = ratio<1>>
struct duration;

template <typename Clock, typename Dur = typename Clock::duration>
struct time_point;

MSTL_END_CHRONO__


MSTL_BEGIN_INNER__
template <typename, typename, typename, typename = void>
struct __duration_common_type {};

template <typename CommonT, typename Period1, typename Period2>
struct __duration_common_type<CommonT, Period1, Period2, void_t<typename CommonT::type>> {
private:
	using gcd_numerator = static_gcd<Period1::num, Period2::num>;
	using gcd_denominator = static_gcd<Period1::den, Period2::den>;
	using common_rep = typename CommonT::type;
	using result_ratio = ratio<
	    gcd_numerator::value,
	    (Period1::den / gcd_denominator::value) * Period2::den
	>;

public:
	using type = _MSTL_CHRONO duration<common_rep, typename result_ratio::type>;
};
MSTL_END_INNER__

template <typename Rep1, typename Period1, typename Rep2, typename Period2>
struct common_type<_MSTL_CHRONO duration<Rep1, Period1>, _MSTL_CHRONO duration<Rep2, Period2>>
    : _INNER __duration_common_type<common_type<Rep1, Rep2>, typename Period1::type, typename Period2::type>
{};

template <typename Rep, typename Period>
struct common_type<_MSTL_CHRONO duration<Rep, Period>, _MSTL_CHRONO duration<Rep, Period>> {
	using type = _MSTL_CHRONO duration<common_type_t<Rep>, typename Period::type>;
};

template <typename Rep, typename Period>
struct common_type<_MSTL_CHRONO duration<Rep, Period>> {
	using type = _MSTL_CHRONO duration<common_type_t<Rep>, typename Period::type>;
};


MSTL_BEGIN_INNER__
template <typename, typename, typename = void>
struct __timepoint_common_type {};

template <typename CommonT, typename Clock>
struct __timepoint_common_type<CommonT, Clock, void_t<typename CommonT::type>> {
	using type = _MSTL_CHRONO time_point<Clock, typename CommonT::type>;
};
MSTL_END_INNER__

template <typename Clock, typename Dur1, typename Dur2>
struct common_type<_MSTL_CHRONO time_point<Clock, Dur1>, _MSTL_CHRONO time_point<Clock, Dur2>>
    : _INNER __timepoint_common_type<common_type<Dur1, Dur2>, Clock>
{};

template <typename Clock, typename Dur>
struct common_type<_MSTL_CHRONO time_point<Clock, Dur>, _MSTL_CHRONO time_point<Clock, Dur>> {
	using type = _MSTL_CHRONO time_point<Clock, Dur>;
};

template <typename Clock, typename Dur>
struct common_type<_MSTL_CHRONO time_point<Clock, Dur>> {
	using type = _MSTL_CHRONO time_point<Clock, Dur>;
};


MSTL_BEGIN_INNER__

template <typename ToDur, typename ConvFactor, typename CommonRep, bool NumIsOne = false, bool DenIsOne = false>
struct __duration_cast_impl {
	template<typename Rep, typename Period>
	static constexpr ToDur __cast(const _MSTL_CHRONO duration<Rep, Period>& value) {
		return ToDur(static_cast<typename ToDur::rep>(static_cast<CommonRep>(value.count())
		    * static_cast<CommonRep>(ConvFactor::num) / static_cast<CommonRep>(ConvFactor::den)));
	}
};

template <typename ToDur, typename ConvFactor, typename CommonRep>
struct __duration_cast_impl<ToDur, ConvFactor, CommonRep, true, true> {
	template <typename Rep, typename Period>
	static constexpr ToDur __cast(const _MSTL_CHRONO duration<Rep, Period>& value) {
		return ToDur(static_cast<typename ToDur::rep>(value.count()));
	}
};

template <typename ToDur, typename ConvFactor, typename CommonRep>
struct __duration_cast_impl<ToDur, ConvFactor, CommonRep, true, false> {
	template<typename Rep, typename Period>
	static constexpr ToDur __cast(const _MSTL_CHRONO duration<Rep, Period>& value) {
		return ToDur(static_cast<typename ToDur::rep>(
		    static_cast<CommonRep>(value.count()) / static_cast<CommonRep>(ConvFactor::den)));
	}
};

template <typename ToDur, typename ConvFactor, typename CommonRep>
struct __duration_cast_impl<ToDur, ConvFactor, CommonRep, false, true> {
	template<typename Rep, typename Period>
	static constexpr ToDur __cast(const _MSTL_CHRONO duration<Rep, Period>& value) {
		return ToDur(static_cast<typename ToDur::rep>(
		    static_cast<CommonRep>(value.count()) * static_cast<CommonRep>(ConvFactor::num)));
	}
};

MSTL_END_INNER__


template <typename>
struct is_duration : false_type {};

template <typename Rep, typename Period>
struct is_duration<_MSTL_CHRONO duration<Rep, Period>> : true_type {};

template <typename T>
MSTL_INLINE17 constexpr bool is_duration_v = is_duration<T>::value;


MSTL_BEGIN_CHRONO__

template <typename ToDur, typename Rep, typename Period, enable_if_t<is_duration<ToDur>::value, int> = 0>
constexpr ToDur duration_cast(const duration<Rep, Period>& value) {
	using to_period = typename ToDur::period;
	using to_rep = typename ToDur::rep;
	using conversion_factor = ratio_divide<Period, to_period>;
	using common_rep = common_type_t<to_rep, Rep, intmax_t>;
	using duration_caster = _INNER __duration_cast_impl<ToDur, conversion_factor, common_rep,
		conversion_factor::num == 1, conversion_factor::den == 1>;
	return duration_caster::__cast(value);
}


template <typename Rep, typename Period>
struct duration {
	static_assert(!is_duration_v<Rep>, "rep cannot be a duration");
	static_assert(is_ratio_v<Period>, "period must be a specialization of ratio");
	static_assert(Period::num > 0, "period must be positive");

private:
	template <typename R1, typename R2,
		 intmax_t Gcd1 = _MSTL gcd(R1::num, R2::num),
		 intmax_t Gcd2 = _MSTL gcd(R1::den, R2::den)>
	using divide = ratio<(R1::num / Gcd1) * (R2::den / Gcd2), (R1::den / Gcd2) * (R2::num / Gcd1)>;

	template <typename Period2>
	using is_harmonic = bool_constant<divide<Period2, Period>::den == 1>;

public:
	using rep = Rep;
	using period = typename Period::type;

private:
	rep rep_ = _MSTL initialize<rep>();

public:
	constexpr duration() = default;

	duration(const duration&) = default;
	constexpr duration& operator =(const duration&) = default;

	template <typename Rep2, typename = enable_if_t<conjunction_v<
		 is_convertible<const Rep2&, rep>, disjunction<is_floating_point<rep>, negation<is_floating_point<Rep2>>>>>
	>
	constexpr explicit duration(const Rep2& rep2) : rep_(static_cast<rep>(rep2)) {}

	template <typename Rep2, typename Period2, typename = enable_if_t<conjunction_v<
		 is_convertible<const Rep2&, rep>,
		 disjunction<is_floating_point<rep>, conjunction<is_harmonic<Period2>, negation<is_floating_point<Rep2>>>>>>
	>
	constexpr duration(const duration<Rep2, Period2>& dur) : rep_(duration_cast<duration>(dur).count()) {}


	constexpr rep count() const noexcept { return rep_; }


	constexpr duration<common_type_t<rep>, period>
	operator +() const {
		return duration<common_type_t<rep>, period>(rep_);
	}

	constexpr duration<common_type_t<rep>, period>
	operator -() const {
		return duration<common_type_t<rep>, period>(-rep_);
	}


	constexpr duration& operator ++() {
		++rep_;
		return *this;
	}

	constexpr duration operator ++(int) {
		return duration(rep_++);
	}

	constexpr duration& operator --() {
		--rep_;
		return *this;
	}

	constexpr duration  operator --(int) {
		return duration(rep_--);
	}


	constexpr duration& operator +=(const duration& dur) {
		rep_ += dur.count();
		return *this;
	}

	constexpr duration& operator -=(const duration& dur) {
		rep_ -= dur.count();
		return *this;
	}

	constexpr duration& operator *=(const rep& rhs) {
		rep_ *= rhs;
		return *this;
	}

	constexpr duration& operator /=(const rep& rhs) {
		rep_ /= rhs;
		return *this;
	}

	template <typename U = rep, enable_if_t<!is_floating_point_v<U>, int> = 0>
	constexpr duration& operator %=(const rep& rhs) {
	    rep_ %= rhs;
	    return *this;
	}

	template <typename U = rep, enable_if_t<!is_floating_point_v<U>, int> = 0>
	constexpr duration& operator %=(const duration& rhs) {
	    rep_ %= rhs.count();
	    return *this;
	}


	static constexpr duration zero() noexcept {
		return duration(rep(0));
	}
	static constexpr duration min() noexcept {
		return duration(numeric_limits<Rep>::lowest());
	}
	static constexpr duration max() noexcept {
		return duration(numeric_limits<Rep>::max());
	}
};

using nanoseconds	= duration<int64_t, nano>;
using microseconds	= duration<int64_t, micro>;
using milliseconds	= duration<int64_t, milli>;
using seconds		= duration<int64_t>;
using minutes		= duration<int64_t, ratio<60>>;
using hours			= duration<int64_t, ratio<3600>>;
using days			= duration<int64_t, ratio<86400>>;
using weeks			= duration<int64_t, ratio<604800>>;
using years			= duration<int64_t, ratio<31556952>>;
using months		= duration<int64_t, ratio<2629746>>;

MSTL_END_CHRONO__

MSTL_BEGIN_INNER__
template <typename Rep1, typename Rep2,
    typename CommonRep = common_type_t<Rep1, Rep2>>
using __common_rep_t = enable_if_t<is_convertible_v<const Rep2&, CommonRep>, CommonRep>;
MSTL_END_INNER__

MSTL_BEGIN_CHRONO__

template <typename Rep1, typename Period1, typename Rep2, typename Period2>
constexpr common_type_t<duration<Rep1, Period1>, duration<Rep2, Period2>>
operator +(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs) {
	using duration1 = duration<Rep1, Period1>;
	using duration2 = duration<Rep2, Period2>;
	using common_duration = common_type_t<duration1, duration2>;
	return common_duration(common_duration(lhs).count() + common_duration(rhs).count());
}

template <typename Rep1, typename Period1, typename Rep2, typename Period2>
constexpr common_type_t<duration<Rep1, Period1>, duration<Rep2, Period2>>
operator -(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs) {
	using duration1 = duration<Rep1, Period1>;
	using duration2 = duration<Rep2, Period2>;
	using common_duration = common_type_t<duration1, duration2>;
	return common_duration(common_duration(lhs).count() - common_duration(rhs).count());
}

template <typename Rep1, typename Period, typename Rep2>
constexpr duration<_INNER __common_rep_t<Rep1, Rep2>, Period>
operator *(const duration<Rep1, Period>& value, const Rep2& scalar) {
    using common_duration = duration<common_type_t<Rep1, Rep2>, Period>;
    return common_duration(common_duration(value).count() * scalar);
}

template <typename Rep1, typename Rep2, typename Period>
constexpr duration<_INNER __common_rep_t<Rep2, Rep1>, Period>
operator *(const Rep1& scalar, const duration<Rep2, Period>& value) {
    return value * scalar;
}

template <typename Rep1, typename Period, typename Rep2>
constexpr duration<_INNER __common_rep_t<Rep1, enable_if_t<!is_duration_v<Rep2>, Rep2>>, Period>
operator /(const duration<Rep1, Period>& value, const Rep2& scalar) {
    using common_duration = duration<common_type_t<Rep1, Rep2>, Period>;
    return common_duration(common_duration(value).count() / scalar);
}

template <typename Rep1, typename Period1, typename Rep2, typename Period2>
constexpr common_type_t<Rep1, Rep2>
operator /(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs) {
    using duration1 = duration<Rep1, Period1>;
    using duration2 = duration<Rep2, Period2>;
    using common_duration = common_type_t<duration1, duration2>;
    return common_duration(lhs).count() / common_duration(rhs).count();
}

template <typename Rep1, typename Period, typename Rep2>
constexpr duration<_INNER __common_rep_t<Rep1, enable_if_t<!is_duration_v<Rep2>, Rep2>>, Period>
operator %(const duration<Rep1, Period>& value, const Rep2& scalar) {
    using common_duration = duration<common_type_t<Rep1, Rep2>, Period>;
    return common_duration(common_duration(value).count() % scalar);
}

template <typename Rep1, typename Period1, typename Rep2, typename Period2>
constexpr common_type_t<duration<Rep1, Period1>, duration<Rep2, Period2>>
operator %(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs) {
    using duration1 = duration<Rep1, Period1>;
    using duration2 = duration<Rep2, Period2>;
    using common_duration = common_type_t<duration1, duration2>;
    return common_duration(common_duration(lhs).count() % common_duration(rhs).count());
}

template <typename Rep1, typename Period1, typename Rep2, typename Period2>
constexpr bool operator ==(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs) {
    using duration1 = duration<Rep1, Period1>;
    using duration2 = duration<Rep2, Period2>;
    using common_duration = common_type_t<duration1, duration2>;
    return common_duration(lhs).count() == common_duration(rhs).count();
}

template <typename Rep1, typename Period1, typename Rep2, typename Period2>
constexpr bool operator !=(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs) {
    return !(lhs == rhs);
}

template<typename Rep1, typename Period1, typename Rep2, typename Period2>
constexpr bool operator <(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs) {
    using duration1 = duration<Rep1, Period1>;
    using duration2 = duration<Rep2, Period2>;
    using common_duration = common_type_t<duration1, duration2>;
    return common_duration(lhs).count() < common_duration(rhs).count();
}

template <typename Rep1, typename Period1, typename Rep2, typename Period2>
constexpr bool operator <=(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs) {
    return !(rhs < lhs);
}

template <typename Rep1, typename Period1, typename Rep2, typename Period2>
constexpr bool operator >(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs) {
    return rhs < lhs;
}

template <typename Rep1, typename Period1, typename Rep2, typename Period2>
constexpr bool operator >=(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs) {
    return !(lhs < rhs);
}


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

MSTL_BEGIN_INNER__
template <typename Dur, char... Digits>
constexpr Dur __check_overflow() noexcept {
	using value_type = static_parse_int<Digits...>;
	constexpr typename Dur::rep rep = value_type::value;
	static_assert(rep >= 0 && rep == value_type::value,
	    "literal value cannot be represented by duration type");
	return Dur(rep);
}
MSTL_END_INNER__

MSTL_BEGIN_LITERALS__

constexpr _MSTL_CHRONO duration<decimal_t, ratio<3600, 1>> operator ""_h(const decimal_t hours) noexcept {
	return _MSTL_CHRONO duration<decimal_t, ratio<3600, 1>>{hours};
}

template <char... _Digits>
constexpr _MSTL_CHRONO hours operator ""_h() noexcept {
	return _INNER __check_overflow<_MSTL_CHRONO hours, _Digits...>();
}

constexpr _MSTL_CHRONO duration<decimal_t, ratio<60, 1>> operator ""_min(const decimal_t mins) noexcept {
	return _MSTL_CHRONO duration<decimal_t, ratio<60,1>>{mins};
}

template <char... _Digits>
constexpr _MSTL_CHRONO minutes operator ""_min() noexcept {
	return _INNER __check_overflow<_MSTL_CHRONO minutes, _Digits...>();
}

constexpr _MSTL_CHRONO duration<decimal_t> operator ""_s(const decimal_t secs) noexcept {
	return _MSTL_CHRONO duration<decimal_t>{secs};
}

template <char... Digits>
constexpr _MSTL_CHRONO seconds operator ""_s() noexcept {
	return _INNER __check_overflow<_MSTL_CHRONO seconds, Digits...>();
}

constexpr _MSTL_CHRONO duration<decimal_t, milli> operator ""_ms(const decimal_t msecs) noexcept {
	return _MSTL_CHRONO duration<decimal_t, milli>{msecs};
}

template <char... Digits>
constexpr _MSTL_CHRONO milliseconds operator ""_ms() noexcept {
	return _INNER __check_overflow<_MSTL_CHRONO milliseconds, Digits...>();
}

constexpr _MSTL_CHRONO duration<decimal_t, micro> operator ""_us(const decimal_t usecs) noexcept {
	return _MSTL_CHRONO duration<decimal_t, micro>{usecs};
}

template <char... Digits>
constexpr _MSTL_CHRONO microseconds operator ""_us() noexcept {
	return _INNER __check_overflow<_MSTL_CHRONO microseconds, Digits...>();
}

constexpr _MSTL_CHRONO duration<decimal_t, nano> operator ""_nsecs(const decimal_t nsecs) noexcept {
	return _MSTL_CHRONO duration<decimal_t, nano>{nsecs};
}

template <char... Digits>
constexpr _MSTL_CHRONO nanoseconds operator ""_ns() noexcept {
	return _INNER __check_overflow<_MSTL_CHRONO nanoseconds, Digits...>();
}

MSTL_END_LITERALS__

MSTL_BEGIN_THIS_THREAD__

template <typename Rep, typename Period>
void sleep_for(const _MSTL_CHRONO duration<Rep, Period>& time) {
	if (time <= time.zero()) return;

#ifdef MSTL_PLATFORM_WINDOWS__
	auto ns = _MSTL_CHRONO duration_cast<_MSTL_CHRONO nanoseconds>(time);
	::LARGE_INTEGER li;
	li.QuadPart = -(ns.count() / 100);

	const ::HANDLE timer = ::CreateWaitableTimerW(nullptr, 1, nullptr);
	if (timer) {
		::SetWaitableTimer(timer, &li, 0, nullptr, nullptr, 0);
		::WaitForSingleObject(timer, INFINITE);
		::CloseHandle(timer);
	}
#elif defined(MSTL_PLATFORM_LINUX__)
	auto s = _MSTL_CHRONO duration_cast<_MSTL_CHRONO seconds>(time);
	auto ns = _MSTL_CHRONO duration_cast<_MSTL_CHRONO nanoseconds>(time - s);
	::timespec ts{static_cast<std::time_t>(s.count()), static_cast<long>(ns.count())};
	while (::nanosleep(&ts, &ts) == -1) {}
#endif
}

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
#endif // MSTL_CHRONO_HPP__
