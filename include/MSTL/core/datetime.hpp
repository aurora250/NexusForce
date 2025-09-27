#ifndef MSTL_DATETIME_HPP__
#define MSTL_DATETIME_HPP__
#include "string.hpp"
MSTL_BEGIN_NAMESPACE__

namespace constants {
    MSTL_INLINE17 static constexpr int32_t MONTH_DAYS[12] = {
        31, 28, 31, 30, 31, 30,
        31, 31, 30, 31, 30, 31
    };
}

class MSTL_API date {
public:
    using date_type = int32_t;

private:
    date_type year_ = 1970;
    date_type month_ = 1;
    date_type day_ = 1;

public:
    constexpr date() noexcept = default;

    constexpr explicit date(
        const date_type year, const date_type month, const date_type day) noexcept {
        if (year >= 0 && (month > 0 && month < 13) &&
            (day > 0 && day <= get_month_day(year, month))) {
            year_ = year;
            month_ = month;
            day_ = day;
        }
    }

    constexpr date(const date& d) noexcept : year_(d.year_), month_(d.month_), day_(d.day_) {}

    constexpr date& operator =(const date& d) noexcept {
        year_ = d.year_;
        month_ = d.month_;
        day_ = d.day_;
        return *this;
    }

    constexpr date(date&& d) noexcept : year_(d.year_), month_(d.month_), day_(d.day_) {
        d.clear();
    }
    constexpr date& operator =(date&& d) noexcept {
        year_ = d.year_;
        month_ = d.month_;
        day_ = d.day_;
        d.clear();
        return *this;
    }

    MSTL_CONSTEXPR20 ~date() = default;


    MSTL_NODISCARD constexpr date_type get_year() const noexcept { return year_; }
    MSTL_NODISCARD constexpr date_type get_month() const noexcept { return month_; }
    MSTL_NODISCARD constexpr date_type get_day() const noexcept { return day_; }


    static constexpr date epoch() noexcept {
        return date{};
    }

    static constexpr bool is_leap_year(const date_type year) noexcept {
        return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    }

    static constexpr date_type get_month_day(
        const date_type year, const date_type month) noexcept {
        date_type day = constants::MONTH_DAYS[month - 1];
        if (month == 2 && is_leap_year(year)) {
            day += 1;
        }
        return day;
    }

    MSTL_NODISCARD constexpr date_type day_of_week() const noexcept {
        date_type y = year_;
        date_type m = month_;
        date_type d = day_;
        if (m < 3) {
            y--;
            m += 12;
        }
        return (d + 2 * m + 3 * (m + 1) / 5 + y + y / 4 - y / 100 + y / 400 + 1) % 7;
    }

    MSTL_NODISCARD constexpr date_type day_of_year() const noexcept {
        date_type days = 0;
        for (date_type i = 1; i < month_; ++i) {
            days += get_month_day(year_, i);
        }
        return days + day_;
    }


    constexpr void clear() noexcept {
        year_ = 1970;
        month_ = 1;
        day_ = 1;
    }


    constexpr bool operator ==(const date& d) const noexcept {
        return year_ == d.year_ && month_ == d.month_ && day_ == d.day_;
    }
    constexpr bool operator !=(const date& d) const noexcept {
        return !(*this == d);
    }

    constexpr bool operator >(const date& d) const noexcept {
        if (year_ > d.year_) return true;
        if (year_ == d.year_ && month_ > d.month_) return true;
        if (year_ == d.year_ && month_ == d.month_ && day_ > d.day_) return true;
        return false;
    }
    constexpr bool operator >=(const date& d) const noexcept {
        return *this > d || *this == d;
    }
    constexpr bool operator <(const date& d) const noexcept {
        return !(*this >= d);
    }
    constexpr bool operator <=(const date& d) const noexcept {
        return !(*this > d);
    }


    constexpr date& operator +=(const date_type day) {
        if (day < 0) return *this -= -day;

        day_ += day;
        while (day_ > get_month_day(year_, month_)) {
            day_ -= get_month_day(year_, month_);
            if (++month_ == 13) {
                month_ = 1;
                year_++;
            }
        }
        return *this;
    }

    constexpr date& operator -=(const date_type day) noexcept {
        if (day < 0) return *this += -day;

        day_ -= day;
        while (day_ <= 0) {
            if (--month_ == 0) {
                month_ = 12;
                year_--;
            }
            day_ += get_month_day(year_, month_);
        }
        return *this;
    }

    constexpr date operator +(const date_type day) const noexcept {
        date ret(*this);
        ret += day;
        return ret;
    }

    constexpr date operator -(const date_type day) const noexcept {
        date ret(*this);
        ret -= day;
        return ret;
    }

    constexpr date& operator ++() {
        *this += 1;
        return *this;
    }
    constexpr date operator ++(int) {
        const date ret(*this);
        *this += 1;
        return ret;
    }

    constexpr date_type operator -(const date& d) const noexcept {
        date max = *this;
        date min = d;
        int flag = 1;

        if (*this < d) {
            max = d;
            min = *this;
            flag = -1;
        }

        date_type count = 0;
        while (min != max) {
            ++min;
            count++;
        }
        return count * flag;
    }

    MSTL_NODISCARD string to_string() const noexcept;
    MSTL_NODISCARD static date from_string(const string& str) noexcept;
};

string to_string(const date& date) noexcept;


class MSTL_API time {
public:
    using time_type = int32_t;

private:
    time_type hours_ = 0;
    time_type minutes_ = 0;
    time_type seconds_ = 0;

public:
    constexpr explicit time(const time_type h = 0,
        const time_type m = 0, const time_type s = 0) noexcept {
        if (h >= 0 && h < 24 && m >= 0 && m < 60 && s >= 0 && s < 60) {
            hours_ = h;
            minutes_ = m;
            seconds_ = s;
        }
    }

    constexpr time(const time& t) noexcept : hours_(t.hours_), minutes_(t.minutes_), seconds_(t.seconds_) {}
    constexpr time& operator =(const time& t) noexcept {
        hours_ = t.hours_;
        minutes_ = t.minutes_;
        seconds_ = t.seconds_;
        return *this;
    }

    constexpr time(time&& t) noexcept : hours_(t.hours_), minutes_(t.minutes_), seconds_(t.seconds_) {
        t.clear();
    }
    constexpr time& operator =(time&& t) noexcept {
        hours_ = t.hours_;
        minutes_ = t.minutes_;
        seconds_ = t.seconds_;
        t.clear();
        return *this;
    }

    MSTL_CONSTEXPR20 ~time() = default;


    MSTL_NODISCARD constexpr time_type get_hours() const noexcept { return hours_; }
    MSTL_NODISCARD constexpr time_type get_minutes() const noexcept { return minutes_; }
    MSTL_NODISCARD constexpr time_type get_seconds() const noexcept { return seconds_; }

    constexpr void clear() noexcept {
        hours_ = 0;
        minutes_ = 0;
        seconds_ = 0;
    }

    constexpr bool operator ==(const time& other) const noexcept {
        return hours_ == other.hours_ &&
               minutes_ == other.minutes_ &&
               seconds_ == other.seconds_;
    }
    constexpr bool operator !=(const time& other) const noexcept { return !(*this == other); }

    constexpr bool operator >(const time& other) const noexcept {
        if (hours_ > other.hours_) return true;
        if (hours_ == other.hours_) {
            if (minutes_ > other.minutes_) return true;
            if (minutes_ == other.minutes_ && seconds_ > other.seconds_) return true;
        }
        return false;
    }
    constexpr bool operator >=(const time& other) const noexcept { return *this > other || *this == other; }
    constexpr bool operator <(const time& other) const noexcept { return !(*this >= other); }
    constexpr bool operator <=(const time& other) const noexcept { return !(*this > other); }


    constexpr time& operator +=(const time_type seconds) {
        if (seconds < 0) return *this -= -seconds;

        seconds_ += seconds;
        const time_type extra_min = seconds_ / 60;
        seconds_ %= 60;

        minutes_ += extra_min;
        const time_type extra_hour = minutes_ / 60;
        minutes_ %= 60;

        hours_ += extra_hour;
        hours_ %= 24;

        return *this;
    }

    constexpr time& operator -=(const time_type seconds) noexcept {
        if (seconds < 0) return *this += -seconds;

        seconds_ -= seconds;
        while (seconds_ < 0) {
            seconds_ += 60;
            minutes_--;
        }

        while (minutes_ < 0) {
            minutes_ += 60;
            hours_--;
        }

        while (hours_ < 0) {
            hours_ += 24;
        }
        return *this;
    }

    constexpr time operator +(const time_type seconds) const noexcept {
        time ret(*this);
        ret += seconds;
        return ret;
    }

    constexpr time operator -(const time_type seconds) const noexcept {
        time ret(*this);
        ret -= seconds;
        return ret;
    }

    constexpr time& operator ++() { return *this += 1; }
    constexpr time operator ++(int) {
        const time ret(*this);
        *this += 1;
        return ret;
    }
    constexpr time& operator --() { return *this -= 1; }
    constexpr time operator --(int) {
        const time ret(*this);
        *this -= 1;
        return ret;
    }

    constexpr time_type operator -(const time& other) const noexcept {
        time_type sec_diff = (hours_ - other.hours_) * 3600;
        sec_diff += (minutes_ - other.minutes_) * 60;
        sec_diff += (seconds_ - other.seconds_);
        return sec_diff;
    }


    MSTL_NODISCARD constexpr time_type to_seconds() const noexcept {
        return hours_ * 3600 + minutes_ * 60 + seconds_;
    }

    MSTL_NODISCARD string to_string() const noexcept;
    MSTL_NODISCARD static time from_string(const string& str) noexcept;
};

string to_string(const time &time) noexcept;


class timestamp;

class MSTL_API datetime {
public:
    using date_type = date::date_type;
    using time_type = time::time_type;

private:
    date date_{};
    time time_{};

public:
    constexpr datetime() noexcept = default;

    constexpr datetime(const datetime& dt) noexcept : date_(dt.date_), time_(dt.time_) {}
    constexpr datetime& operator =(const datetime& dt) noexcept {
        date_ = dt.date_;
        time_ = dt.time_;
        return *this;
    }
    constexpr datetime& operator =(const date& dt) noexcept {
        date_ = dt;
        return *this;
    }

    constexpr datetime(datetime&& dt) noexcept : date_(_MSTL move(dt.date_)), time_(_MSTL move(dt.time_)) {}
    constexpr datetime& operator =(datetime&& dt) noexcept {
        date_ = _MSTL move(dt.date_);
        time_ = _MSTL move(dt.time_);
        return *this;
    }

    constexpr explicit datetime(const date& d, const time& t) noexcept
        : date_(d), time_(t) {}

    constexpr explicit datetime(date&& d, time&& t) noexcept
        : date_(_MSTL move(d)), time_(_MSTL move(t)) {}


    constexpr explicit datetime(const date& d) noexcept : date_(d) {}

    constexpr explicit datetime(
        const date& d, const time_type hour, const time_type minute, const time_type second) noexcept
        : date_(d), time_(hour, minute, second) {}

    constexpr explicit datetime(date&& d) noexcept : date_(_MSTL move(d)) {}

    constexpr explicit datetime(
        date&& d, const time_type hour, const time_type minute, const time_type second) noexcept
        : date_(_MSTL move(d)), time_(hour, minute, second) {}

    constexpr explicit datetime(
        const date_type year, const date_type month, const date_type day) noexcept
        : date_(year, month, day) {}

    constexpr explicit datetime(
        const date_type year, const date_type month, const date_type day,
        const time_type hour, const time_type minute, const time_type second) noexcept
        : date_(year, month, day), time_(hour, minute, second) {}


    MSTL_CONSTEXPR20 ~datetime() = default;


    MSTL_NODISCARD constexpr const date& get_date() const noexcept { return date_; }
    MSTL_NODISCARD constexpr const time& get_time() const noexcept { return time_; }

    MSTL_NODISCARD constexpr time_type get_hours() const noexcept { return time_.get_hours(); }
    MSTL_NODISCARD constexpr time_type get_minutes() const noexcept { return time_.get_minutes(); }
    MSTL_NODISCARD constexpr time_type get_seconds() const noexcept { return time_.get_seconds(); }
    MSTL_NODISCARD constexpr date_type get_year() const noexcept { return date_.get_year(); }
    MSTL_NODISCARD constexpr date_type get_month() const noexcept { return date_.get_month(); }
    MSTL_NODISCARD constexpr date_type get_day() const noexcept { return date_.get_day(); }


    MSTL_NODISCARD static constexpr datetime epoch() noexcept { return datetime{}; }
    MSTL_NODISCARD static datetime now() noexcept;
    MSTL_NODISCARD static datetime from_utc(const datetime& utc_dt) noexcept;
    MSTL_NODISCARD static datetime to_utc(const datetime& local_dt) noexcept;
    MSTL_NODISCARD constexpr timestamp to_timestamp() const noexcept;


    constexpr void clear() noexcept {
        date_.clear();
        time_.clear();
    }


    constexpr bool operator ==(const datetime& other) const noexcept {
        return date_ == other.date_ && time_ == other.time_;
    }
    constexpr bool operator !=(const datetime& other) const noexcept { return !(*this == other); }

    constexpr bool operator >(const datetime& other) const noexcept {
        if (date_ > other.date_) return true;
        if (date_ == other.date_ && time_ > other.time_) return true;
        return false;
    }
    constexpr bool operator >=(const datetime& other) const noexcept {
        return *this > other || *this == other;
    }
    constexpr bool operator <(const datetime& other) const noexcept { return !(*this >= other); }
    constexpr bool operator <=(const datetime& other) const noexcept { return !(*this > other); }


    constexpr datetime& operator +=(const time_type seconds) {
        if (seconds < 0) return *this -= -seconds;

        const time temp_time = time_;
        const time_type max_seconds_without_day_change = 86400 -
            (temp_time.get_hours() * 3600 + temp_time.get_minutes() * 60 + temp_time.get_seconds());

        if (seconds < max_seconds_without_day_change) {
            time_ += seconds;
        }
        else {
            const time_type remaining_seconds = seconds - max_seconds_without_day_change - 1;
            date_ += 1;
            time_ = time(0, 0, 0) + (remaining_seconds + 1);
        }
        return *this;
    }

    constexpr datetime& operator -=(const time_type seconds) noexcept {
        if (seconds < 0) return *this += -seconds;

        const  time_type current_seconds =
            time_.get_hours() * 3600 + time_.get_minutes() * 60 + time_.get_seconds();

        if (seconds < current_seconds) {
            time_ -= seconds;
        }
        else {
            const time_type remaining_seconds = seconds - current_seconds - 1;
            date_ -= 1;
            time_ = time(23, 59, 59) - remaining_seconds;
        }
        return *this;
    }

    constexpr datetime operator +(const time_type seconds) const noexcept {
        datetime ret(*this);
        ret += seconds;
        return ret;
    }

    constexpr datetime operator -(const time_type seconds) const noexcept {
        datetime ret(*this);
        ret -= seconds;
        return ret;
    }

    constexpr datetime& operator ++() { return *this += 1; }
    constexpr datetime operator ++(int) {
        const datetime ret(*this);
        *this += 1;
        return ret;
    }
    constexpr datetime& operator --() { return *this -= 1; }
    constexpr datetime operator --(int) {
        const datetime ret(*this);
        *this -= 1;
        return ret;
    }

    constexpr time_type operator -(const datetime& other) const noexcept {
        const time_type day_diff = date_ - other.date_;
        time_type sec_diff = day_diff * 86400;
        sec_diff += (time_ - other.time_);
        return sec_diff;
    }


    MSTL_NODISCARD string to_gmt() const noexcept;
    MSTL_NODISCARD string to_iso() const noexcept;
    MSTL_NODISCARD string to_iso_utc() const noexcept;
    MSTL_NODISCARD string to_string() const noexcept;

    MSTL_NODISCARD static datetime from_string(const string& str) noexcept;
};

string to_string(const datetime &datetime) noexcept;


class MSTL_API timestamp {
public:
    using value_type = int64_t;

private:
    value_type sec_since_epoch_ = 0;

public:
    constexpr timestamp() noexcept = default;

    constexpr timestamp(const timestamp &timestamp) noexcept
        : sec_since_epoch_(timestamp.sec_since_epoch_) {}

    constexpr timestamp& operator =(const timestamp &timestamp) noexcept {
        sec_since_epoch_ = timestamp.sec_since_epoch_;
        return *this;
    }

    constexpr timestamp(timestamp&& timestamp) noexcept
        : sec_since_epoch_(timestamp.sec_since_epoch_) {
        timestamp.clear();
    }

    constexpr timestamp& operator =(timestamp&& timestamp) noexcept {
        sec_since_epoch_ = timestamp.sec_since_epoch_;
        timestamp.clear();
        return *this;
    }

    constexpr explicit timestamp(const value_type sec) noexcept
        : sec_since_epoch_(sec) {}

    constexpr explicit timestamp(const datetime& dt) noexcept {
        sec_since_epoch_ = dt - datetime::epoch();
    }

    MSTL_CONSTEXPR20 ~timestamp() = default;


    MSTL_NODISCARD static timestamp now() noexcept;

    MSTL_NODISCARD constexpr datetime to_datetime() const noexcept {
        return datetime::epoch() + sec_since_epoch_;
    }

    MSTL_NODISCARD constexpr value_type get_seconds() const noexcept {
        return sec_since_epoch_;
    }


    constexpr void clear() noexcept {
        sec_since_epoch_ = 0;
    }

    constexpr timestamp& operator +=(const value_type sec) noexcept {
        sec_since_epoch_ += sec;
        return *this;
    }
    constexpr timestamp& operator -=(const value_type sec) noexcept {
        sec_since_epoch_ -= sec;
        return *this;
    }

    constexpr timestamp operator +(const value_type sec) const noexcept {
        timestamp ret(*this);
        ret += sec;
        return ret;
    }

    constexpr timestamp operator -(const value_type sec) const noexcept {
        timestamp ret(*this);
        ret -= sec;
        return ret;
    }

    constexpr value_type operator -(const timestamp& other) const noexcept {
        return sec_since_epoch_ - other.sec_since_epoch_;
    }


    constexpr bool operator ==(const timestamp& other) const noexcept {
        return sec_since_epoch_ == other.sec_since_epoch_;
    }
    constexpr bool operator !=(const timestamp& other) const noexcept { return !(*this == other); }

    constexpr bool operator >(const timestamp& other) const noexcept {
        return sec_since_epoch_ > other.sec_since_epoch_;
    }
    constexpr bool operator >=(const timestamp& other) const noexcept {
        return *this > other || *this == other;
    }
    constexpr bool operator <(const timestamp& other) const noexcept { return !(*this >= other); }
    constexpr bool operator <=(const timestamp& other) const noexcept { return !(*this > other); }
};

MSTL_NODISCARD constexpr timestamp datetime::to_timestamp() const noexcept {
    return timestamp(*this);
}

MSTL_END_NAMESPACE__
#endif // MSTL_DATETIME_HPP__