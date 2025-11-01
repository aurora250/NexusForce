#include <MSTL/core/datetime.hpp>
#include <MSTL/core/vsprintf.hpp>
#ifdef MSTL_PLATFORM_WINDOWS__
#include <Windows.h>
#else
#include <time.h>
#endif
MSTL_BEGIN_NAMESPACE__

MSTL_NODISCARD datetime datetime::now() noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
    ::SYSTEMTIME st{};
    ::GetLocalTime(&st);
    return datetime(st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
#elif defined(MSTL_PLATFORM_LINUX__)
    const ::time_t now_time = ::time(nullptr);
    ::tm local_tm{};
    ::localtime_r(&now_time, &local_tm);
    return datetime(local_tm.tm_year + 1900, local_tm.tm_mon + 1,
        local_tm.tm_mday, local_tm.tm_hour, local_tm.tm_min, local_tm.tm_sec);
#else
    return datetime();
#endif
}

datetime datetime::parse_utc(const datetime& utc_dt) noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
    ::SYSTEMTIME st_utc;
    st_utc.wYear = static_cast<::WORD>(utc_dt.year());
    st_utc.wMonth = static_cast<::WORD>(utc_dt.month());
    st_utc.wDay = static_cast<::WORD>(utc_dt.day());
    st_utc.wHour = static_cast<::WORD>(utc_dt.hours());
    st_utc.wMinute = static_cast<::WORD>(utc_dt.minutes());
    st_utc.wSecond = static_cast<::WORD>(utc_dt.seconds());
    st_utc.wMilliseconds = 0;

    ::SYSTEMTIME st;
    if (!::SystemTimeToTzSpecificLocalTime(nullptr, &st_utc, &st)) {
        return datetime::epoch();
    }
    return datetime(st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
#elif defined(MSTL_PLATFORM_LINUX__)
    ::tm utc_tm{};
    utc_tm.tm_year = utc_dt.year() - 1900;
    utc_tm.tm_mon = utc_dt.month() - 1;
    utc_tm.tm_mday = utc_dt.day();
    utc_tm.tm_hour = utc_dt.hours();
    utc_tm.tm_min = utc_dt.minutes();
    utc_tm.tm_sec = utc_dt.seconds();
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
    st_local.wYear = static_cast<::WORD>(local_dt.year());
    st_local.wMonth = static_cast<::WORD>(local_dt.month());
    st_local.wDay = static_cast<::WORD>(local_dt.day());
    st_local.wHour = static_cast<::WORD>(local_dt.hours());
    st_local.wMinute = static_cast<::WORD>(local_dt.minutes());
    st_local.wSecond = static_cast<::WORD>(local_dt.seconds());
    st_local.wMilliseconds = 0;

    ::SYSTEMTIME st;
    if (!::TzSpecificLocalTimeToSystemTime(nullptr, &st_local, &st)) {
        return datetime::epoch();
    }
    return datetime(st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
#elif defined(MSTL_PLATFORM_LINUX__)
    ::tm local_tm{};
    local_tm.tm_year = local_dt.year() - 1900;
    local_tm.tm_mon = local_dt.month() - 1;
    local_tm.tm_mday = local_dt.day();
    local_tm.tm_hour = local_dt.hours();
    local_tm.tm_min = local_dt.minutes();
    local_tm.tm_sec = local_dt.seconds();
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
    const _MSTL date& utc_date = utc_dt.dates();
    const _MSTL time& utc_time = utc_dt.times();
    static const char* weekdays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

    static const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

    int wday = utc_date.days_of_week();
    if (wday < 0 || wday >= 7) wday = 0;

    int mon_idx = utc_date.month() - 1;
    if (mon_idx < 0 || mon_idx >= 12) mon_idx = 0;

    char buf[30];
    _MSTL snprintf(buf, sizeof(buf), "%s, %02d %s %d %s GMT",
        weekdays[wday],
        utc_date.day(),
        months[mon_idx],
        utc_date.year(),
        utc_time.to_string().c_str());
    return string(buf);
}

MSTL_END_NAMESPACE__
