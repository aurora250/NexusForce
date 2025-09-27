#include <MSTL/core/datetime.hpp>
MSTL_BEGIN_NAMESPACE__

MSTL_NODISCARD string date::to_string() const noexcept {
    char buf[11];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d",
        static_cast<int>(year_),
        static_cast<int>(month_),
        static_cast<int>(day_));
    return {buf};
}

MSTL_NODISCARD date date::from_string(const string& str) noexcept {
    if (str.size() != 10 || str[4] != '-' || str[7] != '-') {
        return date{};
    }
    try {
        const date_type year = _MSTL to_int32(str.substr(0, 4).c_str());
        const date_type month = _MSTL to_int32(str.substr(5, 2).c_str());
        const date_type day = _MSTL to_int32(str.substr(8, 2).c_str());
        return date(year, month, day);
    } catch (...) {
        return date{};
    }
}

string to_string(const date& date) noexcept {
    return date.to_string();
}

MSTL_NODISCARD string time::to_string() const noexcept {
    char buf[9];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
        static_cast<int>(hours_),
        static_cast<int>(minutes_),
        static_cast<int>(seconds_));
    return {buf};
}

string to_string(const time& time) noexcept {
    return time.to_string();
}

MSTL_NODISCARD time time::from_string(const string& str) noexcept {
    if (str.size() != 8 || str[2] != ':' || str[5] != ':') {
        return time{};
    }
    try {
        const time_type h = _MSTL to_int32(str.substr(0, 2).c_str());
        const time_type m = _MSTL to_int32(str.substr(3, 2).c_str());
        const time_type s = _MSTL to_int32(str.substr(6, 2).c_str());
        return time(h, m, s);
    } catch (...) {
        return time{};
    }
}

MSTL_NODISCARD datetime datetime::now() noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
    ::SYSTEMTIME st{};
    ::GetLocalTime(&st);
    return datetime(st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
#elif defined(MSTL_PLATFORM_LINUX__)
    const ::time_t now_time = ::time(nullptr);
    ::tm local_tm{};
    localtime_r(&now_time, &local_tm);
    return datetime(local_tm.tm_year + 1900, local_tm.tm_mon + 1,
        local_tm.tm_mday, local_tm.tm_hour, local_tm.tm_min, local_tm.tm_sec);
#else
    return datetime();
#endif
}

datetime datetime::from_utc(const datetime& utc_dt) noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
    ::SYSTEMTIME st_utc;
    st_utc.wYear = static_cast<::WORD>(utc_dt.get_year());
    st_utc.wMonth = static_cast<::WORD>(utc_dt.get_month());
    st_utc.wDay = static_cast<::WORD>(utc_dt.get_day());
    st_utc.wHour = static_cast<::WORD>(utc_dt.get_hours());
    st_utc.wMinute = static_cast<::WORD>(utc_dt.get_minutes());
    st_utc.wSecond = static_cast<::WORD>(utc_dt.get_seconds());
    st_utc.wMilliseconds = 0;

    ::SYSTEMTIME st;
    if (!::SystemTimeToTzSpecificLocalTime(nullptr, &st_utc, &st)) {
        return datetime::epoch();
    }
    return datetime(st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
#elif defined(MSTL_PLATFORM_LINUX__)
    ::tm utc_tm{};
    utc_tm.tm_year = utc_dt.get_year() - 1900;
    utc_tm.tm_mon = utc_dt.get_month() - 1;
    utc_tm.tm_mday = utc_dt.get_day();
    utc_tm.tm_hour = utc_dt.get_hours();
    utc_tm.tm_min = utc_dt.get_minutes();
    utc_tm.tm_sec = utc_dt.get_seconds();
    utc_tm.tm_isdst = -1;

    const ::time_t t = ::timegm(&utc_tm);
    if (t == -1) {
        return datetime::epoch();
    }
    ::tm local_tm{};
    if (!localtime_r(&t, &local_tm)) {
        return datetime::epoch();
    }
    return datetime(local_tm.tm_year + 1900, local_tm.tm_mon + 1,
        local_tm.tm_mday, local_tm.tm_hour, local_tm.tm_min, local_tm.tm_sec);
#else
    return utc_dt;
#endif
}

datetime datetime::to_utc(const datetime& local_dt) noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
    ::SYSTEMTIME st_local;
    st_local.wYear = static_cast<::WORD>(local_dt.get_year());
    st_local.wMonth = static_cast<::WORD>(local_dt.get_month());
    st_local.wDay = static_cast<::WORD>(local_dt.get_day());
    st_local.wHour = static_cast<::WORD>(local_dt.get_hours());
    st_local.wMinute = static_cast<::WORD>(local_dt.get_minutes());
    st_local.wSecond = static_cast<::WORD>(local_dt.get_seconds());
    st_local.wMilliseconds = 0;

    ::SYSTEMTIME st;
    if (!::TzSpecificLocalTimeToSystemTime(nullptr, &st_local, &st)) {
        return datetime::epoch();
    }
    return datetime(st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
#elif defined(MSTL_PLATFORM_LINUX__)
    ::tm local_tm{};
    local_tm.tm_year = local_dt.get_year() - 1900;
    local_tm.tm_mon = local_dt.get_month() - 1;
    local_tm.tm_mday = local_dt.get_day();
    local_tm.tm_hour = local_dt.get_hours();
    local_tm.tm_min = local_dt.get_minutes();
    local_tm.tm_sec = local_dt.get_seconds();
    local_tm.tm_isdst = -1;

    const ::time_t t = ::mktime(&local_tm);
    if (t == -1) {
        return datetime::epoch();
    }
    ::tm utc_tm{};
    if (!gmtime_r(&t, &utc_tm)) {
        return datetime::epoch();
    }
    return datetime(utc_tm.tm_year + 1900, utc_tm.tm_mon + 1,
        utc_tm.tm_mday, utc_tm.tm_hour, utc_tm.tm_min, utc_tm.tm_sec);
#else
    return *this;
#endif
}

MSTL_NODISCARD string datetime::to_gmt() const noexcept {
    const datetime utc_dt = datetime::to_utc(*this);
    const date& utc_date = utc_dt.get_date();
    const time& utc_time = utc_dt.get_time();
    static const char* weekdays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

    static const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

    int wday = utc_date.day_of_week();
    if (wday < 0 || wday >= 7) wday = 0;

    int mon_idx = utc_date.get_month() - 1;
    if (mon_idx < 0 || mon_idx >= 12) mon_idx = 0;

    char buf[30];
    std::snprintf(buf, sizeof(buf), "%s, %02d %s %d %s GMT",
        weekdays[wday],
        utc_date.get_day(),
        months[mon_idx],
        utc_date.get_year(),
        utc_time.to_string().c_str());
    return string(buf);
}

MSTL_NODISCARD string datetime::to_iso() const noexcept {
    return date_.to_string() + "T" + time_.to_string() + "Z";
}

MSTL_NODISCARD string datetime::to_iso_utc() const noexcept {
    const datetime utc_dt = datetime::to_utc(*this);
    return utc_dt.get_date().to_string() + "T" + utc_dt.get_time().to_string() + "Z";
}

MSTL_NODISCARD string datetime::to_string() const noexcept {
    return date_.to_string() + " " + time_.to_string();
}

MSTL_NODISCARD datetime datetime::from_string(const string& str) noexcept {
    if (str.size() != 19 || str[10] != ' ') {
        return datetime{};
    }
    const date d = date::from_string(str.substr(0, 10));
    const time t = time::from_string(str.substr(11, 8));
    return datetime(d, t);
}

string to_string(const datetime& datetime) noexcept {
    return datetime.to_string();
}

MSTL_NODISCARD timestamp timestamp::now() noexcept {
    return timestamp(datetime::now());
}

MSTL_END_NAMESPACE__
