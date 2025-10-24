#include <MSTL/core/memory.hpp>
#ifdef MSTL_PLATFORM_LINUX__
#include <sys/sysinfo.h>
#endif
#ifdef MSTL_PLATFORM_WINDOWS__
#include <Windows.h>
#endif
MSTL_BEGIN_NAMESPACE__

size_t get_available_memory() noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
    ::MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    ::GlobalMemoryStatusEx(&statex);
    return statex.ullAvailPhys + statex.ullAvailVirtual;
#elif defined(MSTL_PLATFORM_LINUX__)
    struct ::sysinfo info{};
    ::sysinfo(&info);
    return info.freeram * info.mem_unit + info.freeswap * info.mem_unit;
#else
    return 0;
#endif
}

MSTL_END_NAMESPACE__
