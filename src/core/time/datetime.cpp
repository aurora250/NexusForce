#include <MSTL/core/time/datetime.hpp>
#ifdef MSTL_PLATFORM_WINDOWS__
#include <MSTL/core/config/windef.hpp>
#include <sysinfoapi.h>
#else
#include <ctime>
#endif
MSTL_BEGIN_NAMESPACE__

MSTL_NODISCARD datetime datetime::now() noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
    ::SYSTEMTIME st{};
    ::GetLocalTime(&st);
    return datetime(st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
#elif defined(MSTL_PLATFORM_LINUX__)
    const std::time_t now_time = std::time(nullptr);
    std::tm local_tm{};
    ::localtime_r(&now_time, &local_tm);
    return datetime(local_tm.tm_year + 1900, local_tm.tm_mon + 1,
        local_tm.tm_mday, local_tm.tm_hour, local_tm.tm_min, local_tm.tm_sec, local_tm.tm_gmtoff);
#else
    return datetime();
#endif
}

MSTL_END_NAMESPACE__
