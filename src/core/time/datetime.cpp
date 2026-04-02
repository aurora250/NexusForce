#include <NeForce/core/time/datetime.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <NeForce/core/config/windef.hpp>
#    include <sysinfoapi.h>
#else
#    include <ctime>
#endif
NEFORCE_BEGIN_NAMESPACE__

NEFORCE_NODISCARD datetime datetime::now() noexcept {
    try {
#ifdef NEFORCE_PLATFORM_WINDOWS
        ::SYSTEMTIME st{};
        ::GetLocalTime(&st);
        return datetime(st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
#elif defined(NEFORCE_PLATFORM_LINUX)
        const std::time_t now_time = std::time(nullptr);
        if (now_time == static_cast<std::time_t>(-1)) {
            return datetime();
        }
        std::tm local_tm{};
        ::localtime_r(&now_time, &local_tm);
        return datetime(local_tm.tm_year + 1900, local_tm.tm_mon + 1, local_tm.tm_mday, local_tm.tm_hour,
                        local_tm.tm_min, local_tm.tm_sec, local_tm.tm_gmtoff);
#endif
    } catch (...) {
        return datetime();
    }
}

NEFORCE_END_NAMESPACE__
