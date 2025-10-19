#include <MSTL/core/memory.hpp>
#ifdef MSTL_PLATFORM_LINUX__
#include <sys/sysinfo.h>
#endif
#ifdef MSTL_PLATFORM_WINDOWS__
#include <Windows.h>
#endif
MSTL_BEGIN_NAMESPACE__

int popcountll(const size_t x) {
#ifdef MSTL_COMPILER_GNUC__
    return __builtin_popcountll(x);
#elif defined(MSTL_COMPILER_MSVC__)
#ifdef MSTL_DATA_BUS_WIDTH_64__
    return static_cast<int>(__popcnt64(x));
#else
    return static_cast<int>(__popcnt(static_cast<uint32_t>(x)));
#endif
#else
    return
        _CONSTANTS POPCOUNT_TABLE[x & 0xFF] +
        _CONSTANTS POPCOUNT_TABLE[(x >> 8) & 0xFF] +
        _CONSTANTS POPCOUNT_TABLE[(x >> 16) & 0xFF] +
        _CONSTANTS POPCOUNT_TABLE[(x >> 24) & 0xFF] +
        _CONSTANTS POPCOUNT_TABLE[(x >> 32) & 0xFF] +
        _CONSTANTS POPCOUNT_TABLE[(x >> 40) & 0xFF] +
        _CONSTANTS POPCOUNT_TABLE[(x >> 48) & 0xFF] +
        _CONSTANTS POPCOUNT_TABLE[(x >> 56) & 0xFF];
#endif
}

int clzll(const size_t x) {
    if (x == 0) return 64;
#ifdef MSTL_COMPILER_GNUC__
    return __builtin_clzll(x);
#elif defined(MSTL_COMPILER_MSVC__)
    unsigned long index;
#ifdef MSTL_DATA_BUS_WIDTH_64__
    ::_BitScanReverse64(&index, x);
#else
    ::_BitScanReverse(&index, x);
#endif
    return 63 - static_cast<int>(index);
#else
    if (x == 0) return 64;

    int n = 0;
    if (x <= 0x00000000FFFFFFFFULL) {
        n += 32;
        x <<= 32;
    }
    if (x <= 0x0000FFFFFFFFFFFFULL) {
        n += 16;
        x <<= 16;
    }
    if (x <= 0x00FFFFFFFFFFFFFFULL) {
        n += 8;
        x <<= 8;
    }
    if (x <= 0x0FFFFFFFFFFFFFFFULL) {
        n += 4;
        x <<= 4;
    }
    if (x <= 0x3FFFFFFFFFFFFFFFULL) {
        n += 2;
        x <<= 2;
    }
    if (x <= 0x7FFFFFFFFFFFFFFFULL) {
        n += 1;
    }
    return n;
#endif
}

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
