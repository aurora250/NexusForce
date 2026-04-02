#include <NeForce/core/time/datetime.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <NeForce/core/config/windef.hpp>
#    include <sysinfoapi.h>
#    include <timezoneapi.h>
#else
#    include <time.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

NEFORCE_NODISCARD datetime datetime::now() noexcept {
    try {
#ifdef NEFORCE_PLATFORM_WINDOWS
        ::SYSTEMTIME st{};
        ::GetLocalTime(&st);
        ::TIME_ZONE_INFORMATION tzi{};
        ::DWORD tzResult = ::GetTimeZoneInformation(&tzi);
        int64_t offset = 0;
        if (tzResult != TIME_ZONE_ID_INVALID) {
            offset = -(tzi.Bias + (tzResult == TIME_ZONE_ID_DAYLIGHT ? tzi.DaylightBias : tzi.StandardBias)) * 60;
        }
        return datetime(st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, offset);
#elif defined(NEFORCE_PLATFORM_LINUX)
        const ::time_t now_time = ::time(nullptr);
        if (now_time == static_cast<::time_t>(-1)) {
            return datetime();
        }
        ::tm local_tm{};
        ::localtime_r(&now_time, &local_tm);
        return datetime(local_tm.tm_year + 1900, local_tm.tm_mon + 1, local_tm.tm_mday, local_tm.tm_hour,
                        local_tm.tm_min, local_tm.tm_sec, local_tm.tm_gmtoff);
#endif
    } catch (...) {
        return datetime();
    }
}

NEFORCE_END_NAMESPACE__
