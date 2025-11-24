#ifndef MSTL_CORE_TIME_DATETIME_HPP__
#define MSTL_CORE_TIME_DATETIME_HPP__
#include "../string/format.hpp"
#include "../utility/packages.hpp"
MSTL_BEGIN_NAMESPACE__

MSTL_BEGIN_CONSTANTS__
MSTL_INLINE17 static constexpr int32_t MONTH_DAYS[12] = {
    31, 28, 31, 30, 31, 30,
    31, 31, 30, 31, 30, 31
};
MSTL_END_CONSTANTS__

class MSTL_API date : public iserialize<date> {
public:
    using date_type = int32_t;
    using self = date;

private:
    date_type year_ = 1970;
    date_type month_ = 1;
    date_type day_ = 1;

public:
    constexpr date() noexcept = default;

    constexpr explicit date(
        const date_type year, const date_type month, const date_type day) noexcept {
        if (year >= 0 && (month > 0 && month < 13) &&
            (day > 0 && day <= days_of_month(year, month))) {
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


    MSTL_NODISCARD constexpr date_type year() const noexcept { return year_; }
    MSTL_NODISCARD constexpr date_type month() const noexcept { return month_; }
    MSTL_NODISCARD constexpr date_type day() const noexcept { return day_; }


    static constexpr date epoch() noexcept {
        return date{};
    }

    static constexpr bool is_leap_year(const date_type year) noexcept {
        return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    }


    MSTL_NODISCARD constexpr date_type days_of_week() const noexcept {
        date_type y = year_;
        date_type m = month_;
        date_type d = day_;
        if (m < 3) {
            y--;
            m += 12;
        }
        return (d + 2 * m + 3 * (m + 1) / 5 + y + y / 4 - y / 100 + y / 400 + 1) % 7;
    }

    static constexpr date_type days_of_month(
        const date_type year, const date_type month) noexcept {
        date_type day = _CONSTANTS MONTH_DAYS[month - 1];
        if (month == 2 && is_leap_year(year)) {
            day += 1;
        }
        return day;
    }

    MSTL_NODISCARD constexpr date_type days_of_year() const noexcept {
        date_type days = 0;
        for (date_type i = 1; i < month_; ++i) {
            days += days_of_month(year_, i);
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
        while (day_ > days_of_month(year_, month_)) {
            day_ -= days_of_month(year_, month_);
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
            day_ += days_of_month(year_, month_);
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

    MSTL_NODISCARD constexpr size_t to_hash() const noexcept {
        constexpr hash<date_type> hasher;
        return hasher(day()) ^ hasher(month()) ^ hasher(year());
    }

    MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string() const {
        return _MSTL format("{:04d}-{:02d}-{:02d}", year(), month(), day());
    }

    MSTL_NODISCARD MSTL_CONSTEXPR20 static self parse(const string_view str) {
        if (str.size() != 10 || str[4] != '-' || str[7] != '-') {
            throw_exception(value_exception("Wrong string formation."));
        }
        const date_type year = integer32::parse(str.substr(0, 4));
        const date_type month = integer32::parse(str.substr(5, 2));
        const date_type day = integer32::parse(str.substr(8, 2));
        return date(year, month, day);
    }

    MSTL_CONSTEXPR20 bool try_parse(const string_view str) noexcept {
        self tmp;
        try {
            tmp = self::parse(str);
        } catch (...) {
            return false;
        }
        *this = _MSTL move(tmp);
        return true;
    }

    constexpr void swap(date& d) noexcept {
        _MSTL swap(year_, d.year_);
        _MSTL swap(month_, d.month_);
        _MSTL swap(day_, d.day_);
    }
};


class MSTL_API time : public iserialize<time> {
public:
    using time_type = int32_t;
    using self = _MSTL time;

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


    MSTL_NODISCARD constexpr time_type hours() const noexcept { return hours_; }
    MSTL_NODISCARD constexpr time_type minutes() const noexcept { return minutes_; }
    MSTL_NODISCARD constexpr time_type seconds() const noexcept { return seconds_; }

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

    MSTL_NODISCARD constexpr size_t to_hash() const noexcept {
        constexpr hash<time_type> hasher;
        return hasher(hours()) ^ hasher(minutes()) ^ hasher(seconds());
    }

    MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string() const {
        return _MSTL format("{:02d}:{:02d}:{:02d}", hours(), minutes(), seconds());
    }

    MSTL_NODISCARD static MSTL_CONSTEXPR20 time parse(const string_view str) {
        if (str.size() != 8 || str[2] != ':' || str[5] != ':') {
            throw_exception(value_exception("Wrong string formation."));
        }
        const time_type h = integer32::parse(str.substr(0, 2));
        const time_type m = integer32::parse(str.substr(3, 2));
        const time_type s = integer32::parse(str.substr(6, 2));
        return time(h, m, s);
    }

    MSTL_CONSTEXPR20 bool try_parse(const string_view str) noexcept {
        self tmp;
        try {
            tmp = self::parse(str);
        } catch (...) {
            return false;
        }
        *this = _MSTL move(tmp);
        return true;
    }

    constexpr void swap(time& other) noexcept {
        _MSTL swap(seconds_, other.seconds_);
        _MSTL swap(hours_, other.hours_);
        _MSTL swap(minutes_, other.minutes_);
    }
};


class MSTL_API datetime : public iserialize<datetime> {
public:
    using date_type = date::date_type;
    using time_type = time::time_type;
    using self = datetime;

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


    MSTL_NODISCARD constexpr const _MSTL date& dates() const noexcept { return date_; }
    MSTL_NODISCARD constexpr const _MSTL time& times() const noexcept { return time_; }

    MSTL_NODISCARD constexpr time_type hours() const noexcept { return time_.hours(); }
    MSTL_NODISCARD constexpr time_type minutes() const noexcept { return time_.minutes(); }
    MSTL_NODISCARD constexpr time_type seconds() const noexcept { return time_.seconds(); }
    MSTL_NODISCARD constexpr date_type year() const noexcept { return date_.year(); }
    MSTL_NODISCARD constexpr date_type month() const noexcept { return date_.month(); }
    MSTL_NODISCARD constexpr date_type day() const noexcept { return date_.day(); }

    MSTL_NODISCARD static constexpr datetime epoch() noexcept { return datetime{}; }
    MSTL_NODISCARD static datetime now() noexcept;

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

        const _MSTL time temp_time = time_;
        const time_type max_seconds_without_day_change = 86400 -
            (temp_time.hours() * 3600 + temp_time.minutes() * 60 + temp_time.seconds());

        if (seconds < max_seconds_without_day_change) {
            time_ += seconds;
        }
        else {
            const time_type remaining_seconds = seconds - max_seconds_without_day_change - 1;
            date_ += 1;
            time_ = _MSTL time(0, 0, 0) + (remaining_seconds + 1);
        }
        return *this;
    }

    constexpr datetime& operator -=(const time_type seconds) noexcept {
        if (seconds < 0) return *this += -seconds;

        const  time_type current_seconds =
            time_.hours() * 3600 + time_.minutes() * 60 + time_.seconds();

        if (seconds < current_seconds) {
            time_ -= seconds;
        }
        else {
            const time_type remaining_seconds = seconds - current_seconds - 1;
            date_ -= 1;
            time_ = _MSTL time(23, 59, 59) - remaining_seconds;
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


    MSTL_NODISCARD static datetime parse_UTC(const datetime& utc_dt) noexcept;
    MSTL_NODISCARD static datetime to_UTC(const datetime& local_dt) noexcept;

    MSTL_NODISCARD string to_GMT() const noexcept;

    MSTL_NODISCARD string to_ISO_UTC() const {
        const datetime utc_dt = datetime::to_UTC(*this);
        return utc_dt.dates().to_string() + "T" + utc_dt.times().to_string() + "Z";
    }

    MSTL_NODISCARD MSTL_CONSTEXPR20 string to_ISO() const {
        return date_.to_string() + "T" + time_.to_string() + "Z";
    }


    MSTL_NODISCARD constexpr size_t to_hash() const noexcept {
        return date_.to_hash() ^ time_.to_hash();
    }

    MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string() const {
        return date_.to_string() + " " + time_.to_string();
    }

    MSTL_NODISCARD static MSTL_CONSTEXPR20 datetime parse(const string_view str) {
        if (str.size() != 19 || str[10] != ' ') {
            throw_exception(value_exception("Wrong string formation."));
        }
        const _MSTL date d = date::parse(str.substr(0, 10));
        const _MSTL time t = time::parse(str.substr(11, 8));
        return datetime(d, t);
    }

    MSTL_CONSTEXPR20 bool try_parse(const string_view str) noexcept {
        self tmp;
        try {
            tmp = self::parse(str);
        } catch (...) {
            return false;
        }
        *this = _MSTL move(tmp);
        return true;
    }

    constexpr void swap(datetime& other) noexcept {
        _MSTL swap(date_, other.date_);
        _MSTL swap(time_, other.time_);
    }
};


class MSTL_API timestamp : public iserialize<timestamp> {
public:
    using value_type = int64_t;
    using self = timestamp;

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


    MSTL_NODISCARD static timestamp now() noexcept { return timestamp(datetime::now()); }

    MSTL_NODISCARD constexpr datetime to_datetime() const noexcept {
        return datetime::epoch() + sec_since_epoch_;
    }

    constexpr size_t to_hash() const noexcept {
        return hash<value_type>()(sec_since_epoch_);
    }

    MSTL_CONSTEXPR20 string to_string() const {
        return integer64(sec_since_epoch_).to_string();
    }

    MSTL_NODISCARD static MSTL_CONSTEXPR20 self parse(const string_view str) {
        return self{integer64::parse(str)};
    }

    MSTL_CONSTEXPR20 bool try_parse(const string_view str) noexcept {
        self tmp;
        try {
            tmp = self::parse(str);
        } catch (...) {
            return false;
        }
        *this = _MSTL move(tmp);
        return true;
    }

    constexpr void swap(self& other) noexcept {
        _MSTL swap(sec_since_epoch_, other.sec_since_epoch_);
    }

    MSTL_NODISCARD constexpr value_type seconds() const noexcept {
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

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_TIME_DATETIME_HPP__