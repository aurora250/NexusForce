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

class MSTL_API date : public iobject<date>, public icommon<date> {
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
        if (is_valid(year, month, day)) {
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

    static constexpr bool is_valid(date_type y, date_type m, date_type d) noexcept {
        if (y < 1900 || y > 9999) return false;
        if (m < 1 || m > 12) return false;
        return d > 0 && d <= days_of_month(y, m);
    }


    static constexpr date epoch() noexcept {
        return date{};
    }

    static constexpr bool is_leap_year(const date_type year) noexcept {
        return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    }


    MSTL_NODISCARD constexpr date_type days_of_week() const noexcept {
        date_type y = year_;
        date_type m = month_;
        const date_type d = day_;
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

    constexpr bool operator <(const date& d) const noexcept {
        if (year_ < d.year_) return true;
        if (year_ == d.year_ && month_ < d.month_) return true;
        if (year_ == d.year_ && month_ == d.month_ && day_ < d.day_) return true;
        return false;
    }

    constexpr date& operator +=(const date_type day) noexcept {
        if (day == 0) return *this;
        if (day < 0) return *this -= -day;

        if (day > 365) {
            const int64_t jd = to_julian_day(year_, month_, day_) + day;
            *this = from_julian_day(jd);
            return *this;
        }

        date_type remaining = day;
        while (remaining > 0) {
            const date_type days_in_month = days_of_month(year_, month_);
            const date_type available = days_in_month - day_ + 1;

            if (remaining < available) {
                day_ += remaining;
                break;
            }

            remaining -= available;
            day_ = 1;
            if (++month_ > 12) {
                month_ = 1;
                ++year_;
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
        return static_cast<date_type>(to_julian_day(year_, month_, day_) - to_julian_day(d.year_, d.month_, d.day_));
    }

    MSTL_NODISCARD constexpr size_t to_hash() const noexcept {
        constexpr hash<date_type> hasher;
        return hasher(day()) ^ hasher(month()) ^ hasher(year());
    }

    MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string() const {
        return _MSTL format("{:04d}-{:02d}-{:02d}", year(), month(), day());
    }

    MSTL_NODISCARD constexpr static date parse(const string_view str) {
        if (str.size() != 10 || str[4] != '-' || str[7] != '-') {
            throw_exception(value_exception("Wrong string formation."));
        }
        const date_type year = integer32::parse(str.substr(0, 4));
        const date_type month = integer32::parse(str.substr(5, 2));
        const date_type day = integer32::parse(str.substr(8, 2));
        return date(year, month, day);
    }

    constexpr void swap(date& d) noexcept {
        _MSTL swap(year_, d.year_);
        _MSTL swap(month_, d.month_);
        _MSTL swap(day_, d.day_);
    }

    static constexpr int64_t to_julian_day(
        const date_type y, const date_type m, const date_type d) noexcept {
        const int64_t a = (14 - m) / 12;
        const int64_t year = y + 4800 - a;
        const int64_t month = m + 12 * a - 3;
        return d + (153 * month + 2) / 5 + 365 * year + year / 4 - year / 100 + year / 400 - 32045;
    }

    static constexpr date from_julian_day(const int64_t jd) noexcept {
        const int64_t a = jd + 32044;
        const int64_t b = (4 * a + 3) / 146097;
        const int64_t c = a - (146097 * b) / 4;
        const int64_t d = (4 * c + 3) / 1461;
        const int64_t e = c - (1461 * d) / 4;
        const int64_t m = (5 * e + 2) / 153;

        const date_type day = static_cast<date_type>(e - (153 * m + 2) / 5 + 1);
        const date_type month = static_cast<date_type>(m + 3 - 12 * (m / 10));
        const date_type year = static_cast<date_type>(100 * b + d - 4800 + (m / 10));

        return date(year, month, day);
    }
};


class MSTL_API time : public iobject<time>, public icommon<time> {
public:
    using time_type = int32_t;

private:
    time_type hours_ = 0;
    time_type minutes_ = 0;
    time_type seconds_ = 0;

public:
    constexpr explicit time(const time_type h = 0,
        const time_type m = 0, const time_type s = 0) noexcept {
        if (is_valid(h, m, s)) {
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

    static constexpr bool is_valid(time_type h, time_type m, time_type s) noexcept {
        return h >= 0 && h < 24 && m >= 0 && m < 60 && s >= 0 && s < 60;
    }

    constexpr void clear() noexcept {
        hours_ = 0;
        minutes_ = 0;
        seconds_ = 0;
    }


    constexpr bool operator ==(const time& other) const noexcept {
        return hours_ == other.hours_ && minutes_ == other.minutes_ && seconds_ == other.seconds_;
    }

    constexpr bool operator <(const time& other) const noexcept {
        if (hours_ < other.hours_) return true;
        if (hours_ == other.hours_) {
            if (minutes_ < other.minutes_) return true;
            if (minutes_ == other.minutes_ && seconds_ < other.seconds_) return true;
        }
        return false;
    }


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

        int64_t total_sec = to_seconds() - seconds;
        total_sec %= 86400;
        if (total_sec < 0) total_sec += 86400;

        hours_   = static_cast<time_type>(total_sec / 3600);
        minutes_ = static_cast<time_type>((total_sec % 3600) / 60);
        seconds_ = static_cast<time_type>(total_sec % 60);
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

    MSTL_NODISCARD static constexpr time parse(const string_view str) {
        if (str.size() != 8 || str[2] != ':' || str[5] != ':') {
            throw_exception(value_exception("Wrong string formation."));
        }
        const time_type h = integer32::parse(str.substr(0, 2));
        const time_type m = integer32::parse(str.substr(3, 2));
        const time_type s = integer32::parse(str.substr(6, 2));
        return time(h, m, s);
    }

    constexpr void swap(time& other) noexcept {
        _MSTL swap(hours_, other.hours_);
        _MSTL swap(minutes_, other.minutes_);
        _MSTL swap(seconds_, other.seconds_);
    }
};


MSTL_BEGIN_CONSTANTS__

MSTL_INLINE17 constexpr const char* const WEEKDAYS_STRING[] =
    {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

MSTL_INLINE17 constexpr const char* const MONTHS_STRING[] =
    {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

MSTL_END_CONSTANTS__

constexpr int months_to_int(const string_view sv) {
    for (int i = 0; i < 12; ++i) {
        if (sv == _CONSTANTS MONTHS_STRING[i]) return i + 1;
    }
    return 0;
}


class MSTL_API datetime : public iobject<datetime>, public icommon<datetime> {
public:
    using date_type = _MSTL date::date_type;
    using time_type = _MSTL time::time_type;

private:
    _MSTL date date_{};
    _MSTL time time_{};
    int64_t offset_seconds_ = 0;
    bool has_timezone_ = false;

public:
    constexpr datetime() noexcept = default;

    constexpr datetime(const datetime& dt) noexcept
    : date_(dt.date_), time_(dt.time_),
    offset_seconds_(dt.offset_seconds_), has_timezone_(dt.has_timezone_) {}

    constexpr datetime& operator =(const datetime& dt) noexcept {
        date_ = dt.date_;
        time_ = dt.time_;
        offset_seconds_ = dt.offset_seconds_;
        has_timezone_ = dt.has_timezone_;
        return *this;
    }

    constexpr datetime(datetime&& dt) noexcept
    : date_(dt.date_), time_(dt.time_), offset_seconds_(dt.offset_seconds_),
    has_timezone_(dt.has_timezone_) {
        dt.clear();
    }

    constexpr datetime& operator =(datetime&& dt) noexcept {
        date_ = dt.date_;
        time_ = dt.time_;
        offset_seconds_ = dt.offset_seconds_;
        has_timezone_ = dt.has_timezone_;
        dt.clear();
        return *this;
    }

    constexpr explicit datetime(const date_type year, const date_type month, const date_type day,
        const time_type hour, const time_type minute, const time_type second) noexcept
    : date_(year, month, day), time_(hour, minute, second) {}

    constexpr explicit datetime(const date_type year, const date_type month, const date_type day,
        const time_type hour, const time_type minute, const time_type second, const int64_t offset) noexcept
    : date_(year, month, day), time_(hour, minute, second), offset_seconds_(offset), has_timezone_(true) {}

    constexpr explicit datetime(const _MSTL date& d, const _MSTL time& t) noexcept
    : date_(d), time_(t) {}

    constexpr explicit datetime(const _MSTL date& d, const _MSTL time& t, const int64_t offset) noexcept
    : date_(d), time_(t), offset_seconds_(offset), has_timezone_(true) {}

    constexpr explicit datetime(_MSTL date&& d, _MSTL time&& t) noexcept
    : date_(d), time_(t) {
        d.clear();
        t.clear();
    }

    constexpr explicit datetime(_MSTL date&& d, _MSTL time&& t, const int64_t offset) noexcept
    : date_(d), time_(t), offset_seconds_(offset), has_timezone_(true) {
        d.clear();
        t.clear();
    }

    constexpr explicit datetime(const _MSTL date& d) noexcept : date_(d) {}

    constexpr datetime& operator =(const _MSTL date& d) noexcept {
        date_ = d;
        return *this;
    }

    constexpr explicit datetime(_MSTL date&& d) noexcept : date_(d) {
        d.clear();
    }

    constexpr datetime& operator =(_MSTL date&& d) noexcept {
        date_ = d;
        d.clear();
        return *this;
    }

    constexpr explicit datetime(const _MSTL time& t) noexcept : time_(t) {}

    constexpr datetime& operator =(const _MSTL time& t) noexcept {
        time_ = t;
        return *this;
    }

    constexpr explicit datetime(_MSTL time&& t) noexcept : time_(t) {
        t.clear();
    }

    constexpr datetime& operator =(_MSTL time&& t) noexcept {
        time_ = t;
        t.clear();
        return *this;
    }

    MSTL_CONSTEXPR20 ~datetime() = default;


    MSTL_NODISCARD constexpr const _MSTL date& date() const noexcept { return date_; }
    MSTL_NODISCARD constexpr const _MSTL time& time() const noexcept { return time_; }

    MSTL_NODISCARD constexpr time_type hours() const noexcept { return time_.hours(); }
    MSTL_NODISCARD constexpr time_type minutes() const noexcept { return time_.minutes(); }
    MSTL_NODISCARD constexpr time_type seconds() const noexcept { return time_.seconds(); }
    MSTL_NODISCARD constexpr date_type year() const noexcept { return date_.year(); }
    MSTL_NODISCARD constexpr date_type month() const noexcept { return date_.month(); }
    MSTL_NODISCARD constexpr date_type day() const noexcept { return date_.day(); }

    MSTL_NODISCARD constexpr bool has_timezone() const noexcept { return has_timezone_; }
    MSTL_NODISCARD constexpr int64_t offset_seconds() const noexcept { return offset_seconds_; }

    MSTL_NODISCARD static constexpr datetime epoch() noexcept { return datetime{}; }
    MSTL_NODISCARD static datetime now() noexcept;

    constexpr void clear() noexcept {
        date_.clear();
        time_.clear();
        offset_seconds_ = 0;
        has_timezone_ = false;
    }


    constexpr bool operator ==(const datetime& other) const noexcept {
        return date_ == other.date_ && time_ == other.time_
            && has_timezone_ == other.has_timezone_
            && offset_seconds_ == other.offset_seconds_;
    }

    constexpr bool operator <(const datetime& other) const noexcept {
        if (date_ < other.date_) {
            return true;
        } else if (date_ == other.date_) {
            if (time_ < other.time_) {
                return true;
            } else if (time_ == other.time_) {
                if (has_timezone_ && other.has_timezone_ &&
                    offset_seconds_ < other.offset_seconds_) {
                    return true;
                }
            }
        }
        return false;
    }


    constexpr datetime& operator +=(const int64_t seconds) {
        if (seconds < 0) return *this -= -seconds;

        const int64_t current_total_sec =
            static_cast<int64_t>(time_.hours()) * 3600 +
            static_cast<int64_t>(time_.minutes()) * 60 +
            static_cast<int64_t>(time_.seconds());

        int64_t new_total_sec = current_total_sec + seconds;
        int64_t days_to_add = new_total_sec / 86400;
        new_total_sec %= 86400;

        if (new_total_sec < 0) {
            new_total_sec += 86400;
            days_to_add--;
        }

        date_ += static_cast<date_type>(days_to_add);
        time_ = _MSTL time(
            static_cast<time_type>(new_total_sec / 3600),
            static_cast<time_type>((new_total_sec % 3600) / 60),
            static_cast<time_type>(new_total_sec % 60)
        );
        return *this;
    }

    constexpr datetime& operator -=(const int64_t seconds) noexcept {
        if (seconds < 0) return *this += -seconds;

        const int64_t current_total_sec =
            static_cast<int64_t>(time_.hours()) * 3600 +
            static_cast<int64_t>(time_.minutes()) * 60 +
            static_cast<int64_t>(time_.seconds());

        int64_t new_total_sec = current_total_sec - seconds;
        int64_t days_to_subtract = 0;

        if (new_total_sec < 0) {
            days_to_subtract = (-new_total_sec + 86399) / 86400;
            new_total_sec += days_to_subtract * 86400;
        }

        date_ -= static_cast<date_type>(days_to_subtract);
        time_ = _MSTL time(
            static_cast<time_type>(new_total_sec / 3600),
            static_cast<time_type>((new_total_sec % 3600) / 60),
            static_cast<time_type>(new_total_sec % 60)
        );
        return *this;
    }

    constexpr datetime operator +(const int64_t seconds) const noexcept {
        datetime ret(*this);
        ret += seconds;
        return ret;
    }

    constexpr datetime operator -(const int64_t seconds) const noexcept {
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


    MSTL_NODISCARD MSTL_CONSTEXPR20 string to_offset_string() const {
        if (!has_timezone_) return {};
        if (offset_seconds_ == 0) return "Z";
        int64_t total_sec = offset_seconds_;
        const char sign = total_sec >= 0 ? '+' : '-';
        total_sec = total_sec >= 0 ? total_sec : -total_sec;
        const int64_t hours = total_sec / 3600;
        const int64_t minutes = (total_sec % 3600) / 60;
        return _MSTL format("{}{:02d}:{:02d}", sign, hours, minutes);
    }


    static constexpr datetime from_UTC(const datetime& utc_dt, const int32_t offset_sec = 0) noexcept {
        datetime utc_time = utc_dt;
        if (utc_dt.has_timezone_ && utc_dt.offset_seconds_ != 0) {
            utc_time = utc_dt.to_UTC();
        }
        datetime local = utc_time + offset_sec;
        local.offset_seconds_ = offset_sec;
        local.has_timezone_ = true;
        return local;
    }

    MSTL_NODISCARD constexpr datetime to_UTC() const noexcept {
        if (!has_timezone_) {
            return *this;
        }
        datetime utc = *this;
        utc -= offset_seconds_;
        utc.offset_seconds_ = 0;
        utc.has_timezone_ = true;
        return utc;
    }


    MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string_ISO_UTC() const {
        if (has_timezone_) {
            return date_.to_string() + "T" + time_.to_string() + to_offset_string();
        } else {
            return date_.to_string() + "T" + time_.to_string();
        }
    }

    MSTL_NODISCARD static constexpr datetime parse_ISO_UTC(const string_view str) {
        if (str.size() < 20 || str[10] != 'T') {
            throw_exception(value_exception("Invalid ISO UTC datetime format."));
        }

        const _MSTL date d = date::parse(str.substr(0, 10));
        const _MSTL time t = time::parse(str.substr(11, 8));

        if (str.size() > 19 && str[19] == 'Z') {
            return datetime(d, t, 0);
        } else if (str.size() > 19 && (str[19] == '+' || str[19] == '-')) {
            const char sign = str[19];
            int hours = 0, minutes = 0;
            size_t pos = 20;
            if (str.size() >= pos + 2) {
                hours = integer32::parse(str.substr(pos, 2));
                pos += 2;
                if (str.size() >= pos + 3 && str[pos] == ':') {
                    pos++;
                    if (str.size() >= pos + 2) {
                        minutes = integer32::parse(str.substr(pos, 2));
                    }
                }
            }

            int32_t total_offset = hours * 3600 + minutes * 60;
            if (sign == '-') total_offset = -total_offset;
            return datetime(d, t, total_offset);
        }
        return datetime(d, t);
    }

    MSTL_CONSTEXPR20 bool try_parse_ISO_UTC(string_view str) noexcept {
        try {
            datetime tmp = datetime::parse_ISO_UTC(str);
            this->swap(tmp);
        } catch (...) {
            return false;
        }
        return true;
    }


    MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string_GMT() const noexcept {
        const _MSTL date utc_date = date();
        const _MSTL time utc_time = time();

        int wday = utc_date.days_of_week();
        if (wday < 0 || wday >= 7) wday = 0;

        int mon_idx = utc_date.month() - 1;
        if (mon_idx < 0 || mon_idx >= 12) mon_idx = 0;

        return _MSTL format("{}, {:02d} {} {} {} GMT",
            _CONSTANTS WEEKDAYS_STRING[wday],
            utc_date.day(),
            _CONSTANTS MONTHS_STRING[mon_idx],
            utc_date.year(),
            utc_time.to_string()
        );
    }

    MSTL_NODISCARD static constexpr datetime parse_GMT(string_view str) {
        if (str.size() < 29) {
            throw_exception(value_exception("Invalid date length."));
        }
        if (str.substr(3, 2) != ", ") {
            throw_exception(value_exception("Invalid date format"));
        }

        str.remove_prefix(5);
        const int day = integer32::parse(str.substr(0, 2));
        str.remove_prefix(3);
        const int mon = months_to_int(str.substr(0, 3));
        if (mon == 0) throw_exception(value_exception("Invalid month in date"));
        str.remove_prefix(4);
        const int year = integer32::parse(str.substr(0, 4));
        str.remove_prefix(5);
        const int hour = integer32::parse(str.substr(0, 2));
        str.remove_prefix(3);
        const int minute = integer32::parse(str.substr(0, 2));
        str.remove_prefix(3);
        const int second = integer32::parse(str.substr(0, 2));
        str.remove_prefix(3);

        if (str != "GMT") {
            throw_exception(value_exception("Invalid timezone in date"));
        }
        return datetime(year, mon, day, hour, minute, second);
    }

    MSTL_CONSTEXPR20 bool try_parse_GMT(const string_view str) noexcept {
        try {
            datetime tmp = datetime::parse_GMT(str);
            this->swap(tmp);
        } catch (...) {
            return false;
        }
        return true;
    }


    MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string_ISO() const {
        return date_.to_string() + "T" + time_.to_string();
    }

    MSTL_NODISCARD static constexpr datetime parse_ISO(const string_view str) {
        if (str.size() < 19 || str[10] != 'T') {
            throw_exception(value_exception("Invalid ISO datetime format."));
        }
        const _MSTL date d = _MSTL date::parse(str.substr(0, 10));
        size_t time_len = 8;
        if (str.size() >= 19) {
            time_len = 8;
        }
        const _MSTL time t = _MSTL time::parse(str.substr(11, time_len));
        return datetime(d, t);
    }

    MSTL_CONSTEXPR20 bool try_parse_ISO(const string_view str) noexcept {
        try {
            datetime tmp = datetime::parse_ISO(str);
            this->swap(tmp);
        } catch (...) {
            return false;
        }
        return true;
    }


    MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string() const {
        return date_.to_string() + " " + time_.to_string();
    }

    MSTL_NODISCARD static constexpr datetime parse(const string_view str) {
        if (str.size() != 19 || str[10] != ' ') {
            throw_exception(value_exception("Wrong string formation."));
        }
        const _MSTL date d = date::parse(str.substr(0, 10));
        const _MSTL time t = time::parse(str.substr(11, 8));
        return datetime(d, t);
    }

    MSTL_NODISCARD constexpr size_t to_hash() const noexcept {
        return date_.to_hash() ^ time_.to_hash() ^
            hash<bool>()(has_timezone_) ^ hash<int64_t>()(offset_seconds_);
    }

    constexpr void swap(datetime& other) noexcept {
        _MSTL swap(date_, other.date_);
        _MSTL swap(time_, other.time_);
        _MSTL swap(offset_seconds_, other.offset_seconds_);
        _MSTL swap(has_timezone_, other.has_timezone_);
    }
};


class MSTL_API timestamp : public iobject<timestamp>, public ipackage<timestamp, int64_t> {
public:
    using value_type = int64_t;

    constexpr timestamp() noexcept = default;

    constexpr timestamp(const timestamp &timestamp) noexcept
    : ipackage(timestamp.value_) {}

    constexpr timestamp& operator =(const timestamp &timestamp) noexcept {
        value_ = timestamp.value_;
        return *this;
    }

    constexpr timestamp(timestamp&& timestamp) noexcept
    : ipackage(timestamp.value_) {
        timestamp.clear();
    }

    constexpr timestamp& operator =(timestamp&& timestamp) noexcept {
        value_ = timestamp.value_;
        timestamp.clear();
        return *this;
    }

    constexpr explicit timestamp(const value_type sec) noexcept
    : ipackage(sec) {}

    constexpr explicit timestamp(const datetime& dt) noexcept {
        value_ = dt - datetime::epoch();
    }

    MSTL_CONSTEXPR20 ~timestamp() = default;


    MSTL_NODISCARD static timestamp now() noexcept {
        return timestamp(datetime::now());
    }

    MSTL_NODISCARD constexpr datetime to_datetime() const noexcept {
        return datetime::epoch() + value_;
    }

    MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string() const {
        return integer64(value_).to_string();
    }

    MSTL_NODISCARD static constexpr timestamp parse(const string_view str) {
        return timestamp{integer64::parse(str)};
    }

    MSTL_NODISCARD constexpr value_type seconds() const noexcept {
        return value_;
    }
    
    constexpr void clear() noexcept {
        value_ = 0;
    }
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_TIME_DATETIME_HPP__
