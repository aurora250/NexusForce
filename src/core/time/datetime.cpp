#include <NeForce/core/time/datetime.hpp>
#include <ctime>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <NeForce/core/config/windef.hpp>
#    include <sysinfoapi.h>
#    include <timezoneapi.h>
#endif
#ifdef NEFORCE_PLATFORM_LINUX
#    include <sys/time.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

timestamp timestamp::now() noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::FILETIME ft;
    ::GetSystemTimeAsFileTime(&ft);
    ::ULARGE_INTEGER uli;
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    constexpr uint64_t EPOCH_DIFF = 116444736000000000ULL;
    return timestamp(static_cast<int64_t>(uli.QuadPart - EPOCH_DIFF) / 10000000LL);
#else
    ::timeval tv = {0, 0};
    ::gettimeofday(&tv, nullptr);
    return timestamp(tv.tv_sec);
#endif
}

NEFORCE_NODISCARD datetime datetime::now() noexcept {
    try {
#ifdef NEFORCE_PLATFORM_WINDOWS
        ::SYSTEMTIME st{};
        ::GetLocalTime(&st);
        ::TIME_ZONE_INFORMATION tzi{};
        const ::DWORD tzResult = ::GetTimeZoneInformation(&tzi);
        int64_t offset = 0;
        if (tzResult != TIME_ZONE_ID_INVALID) {
            offset = -static_cast<int64_t>(tzi.Bias +
                                           (tzResult == TIME_ZONE_ID_DAYLIGHT ? tzi.DaylightBias : tzi.StandardBias)) *
                     60;
        }
        return datetime(st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, offset);
#else
        const ::time_t now_time = ::time(nullptr);
        if (now_time == static_cast<::time_t>(-1)) {
            return {};
        }
        ::tm local_tm{};
        ::localtime_r(&now_time, &local_tm);
        return datetime(local_tm.tm_year + 1900, local_tm.tm_mon + 1, local_tm.tm_mday, local_tm.tm_hour,
                        local_tm.tm_min, local_tm.tm_sec, local_tm.tm_gmtoff);
#endif
    } catch (...) {
        return {};
    }
}

NEFORCE_END_NAMESPACE__
