#include <NeForce/core/file/file_info.hpp>
#ifdef NEFORCE_PLATFORM_LINUX
#    include <sys/stat.h>
#    include <sys/time.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

namespace {
#ifdef NEFORCE_PLATFORM_WINDOWS
    datetime filetime_to_datetime(const ::FILETIME& ft) noexcept {
        ::ULARGE_INTEGER ull;
        ull.LowPart = ft.dwLowDateTime;
        ull.HighPart = ft.dwHighDateTime;
        constexpr uint64_t EPOCH_DIFF = 116444736000000000ULL;
        const uint64_t unix_100ns = ull.QuadPart - EPOCH_DIFF;
        const datetime dt = timestamp(static_cast<int64_t>(unix_100ns / 10000000ULL)).to_datetime();
        return datetime(dt.date(), dt.time(), 0);
    }

    ::FILETIME datetime_to_filetime(const datetime& dt) noexcept {
        constexpr uint64_t EPOCH_DIFF = 116444736000000000ULL;
        const uint64_t val = static_cast<uint64_t>(timestamp(dt).value()) * 10000000ULL + EPOCH_DIFF;
        ::FILETIME ft;
        ft.dwLowDateTime = static_cast<::DWORD>(val & 0xFFFFFFFF);
        ft.dwHighDateTime = static_cast<::DWORD>(val >> 32);
        return ft;
    }
#else
    datetime filetime_to_datetime(const ::time_t t) noexcept {
        const datetime dt = timestamp(static_cast<int64_t>(t)).to_datetime();
        return datetime(dt.date(), dt.time(), 0);
    }
#endif
} // namespace


file_info::file_info(const native_handle_type handle) noexcept :
handle_(handle) {}

file_attri file_info::attributes() const noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::BY_HANDLE_FILE_INFORMATION info{};
    if (::GetFileInformationByHandle(handle_, &info) == FALSE) {
        return file_attri::OTHERS;
    }
    return static_cast<file_attri>(info.dwFileAttributes);
#else
    struct ::stat64 st{};
    if (::fstat64(handle_, &st) == -1) {
        return file_attri::OTHERS;
    }
    return static_cast<file_attri>(st.st_mode);
#endif
}

bool file_info::set_attributes(const file_attri attr) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    char path_buf[MAX_PATH]{};
    if (::GetFinalPathNameByHandleA(handle_, path_buf, MAX_PATH, FILE_NAME_NORMALIZED | VOLUME_NAME_DOS) == 0) {
        return false;
    }
    const char* p = path_buf;
    if (string_compare(p, R"(\\?\)", 4) == 0) {
        p += 4;
    }
    return ::SetFileAttributesA(p, static_cast<::DWORD>(attr)) != 0;
#else
    struct ::stat64 st{};
    if (::fstat64(handle_, &st) == -1) {
        return false;
    }
    constexpr ::mode_t perm_mask = S_IRWXU | S_IRWXG | S_IRWXO;
    const ::mode_t new_perm = static_cast<::mode_t>(attr) & perm_mask;
    const ::mode_t new_mode = (st.st_mode & ~perm_mask) | new_perm;
    return ::fchmod(handle_, new_mode) == 0;
#endif
}

file_info::size_type file_info::size() const noexcept {
    size_type out = 0;
    size(out);
    return out;
}

uint64_t file_info::size64() const noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::LARGE_INTEGER li{};
    if (::GetFileSizeEx(handle_, &li) == FALSE) {
        return 0;
    }
    return static_cast<uint64_t>(li.QuadPart);
#else
    struct ::stat64 st{};
    if (::fstat64(handle_, &st) == -1) {
        return 0;
    }
    return static_cast<uint64_t>(st.st_size);
#endif
}

bool file_info::size(size_type& out_size) const noexcept {
    out_size = 0;
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::LARGE_INTEGER li{};
    if (::GetFileSizeEx(handle_, &li) == FALSE) {
        return false;
    }
    if (li.QuadPart > static_cast<::LONGLONG>(numeric_traits<size_type>::max())) {
        return false;
    }
    out_size = static_cast<size_type>(li.QuadPart);
    return true;
#else
    struct ::stat64 st{};
    if (::fstat64(handle_, &st) == -1) {
        return false;
    }
    if (static_cast<uint64_t>(st.st_size) > numeric_traits<size_type>::max()) {
        return false;
    }
    out_size = static_cast<size_type>(st.st_size);
    return true;
#endif
}

datetime file_info::last_access_time() const noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::FILETIME ftCreate{}, ftAccess{}, ftWrite{};
    if (::GetFileTime(handle_, &ftCreate, &ftAccess, &ftWrite) == FALSE) {
        return datetime::epoch();
    }
    return filetime_to_datetime(ftAccess);
#else
    struct ::stat64 st{};
    if (::fstat64(handle_, &st) == -1) {
        return datetime::epoch();
    }
    return filetime_to_datetime(st.st_atime);
#endif
}

bool file_info::set_last_access_time(const datetime& dt) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    const ::FILETIME ftA = datetime_to_filetime(dt);
    return ::SetFileTime(handle_, nullptr, &ftA, nullptr) != 0;
#else
    ::timespec times[2]{};
    times[0].tv_sec = static_cast<::time_t>(timestamp(dt).value());
    times[0].tv_nsec = 0;
    times[1].tv_nsec = UTIME_OMIT;
    return ::futimens(handle_, times) == 0;
#endif
}

datetime file_info::last_write_time() const noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::FILETIME ftCreate{}, ftAccess{}, ftWrite{};
    if (::GetFileTime(handle_, &ftCreate, &ftAccess, &ftWrite) == FALSE) {
        return datetime::epoch();
    }
    return filetime_to_datetime(ftWrite);
#else
    struct ::stat64 st{};
    if (::fstat64(handle_, &st) == -1) {
        return datetime::epoch();
    }
    return filetime_to_datetime(st.st_mtime);
#endif
}

bool file_info::set_last_write_time(const datetime& dt) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    const ::FILETIME ftW = datetime_to_filetime(dt);
    return ::SetFileTime(handle_, nullptr, nullptr, &ftW) != 0;
#else
    ::timespec times[2]{};
    times[0].tv_nsec = UTIME_OMIT;
    times[1].tv_sec = static_cast<::time_t>(timestamp(dt).value());
    times[1].tv_nsec = 0;
    return ::futimens(handle_, times) == 0;
#endif
}

#ifdef NEFORCE_PLATFORM_WINDOWS
datetime file_info::creation_time() const noexcept {
    ::FILETIME ftCreate{}, ftAccess{}, ftWrite{};
    if (::GetFileTime(handle_, &ftCreate, &ftAccess, &ftWrite) == FALSE) {
        return datetime::epoch();
    }
    return filetime_to_datetime(ftCreate);
}

bool file_info::set_creation_time(const datetime& dt) noexcept {
    const ::FILETIME ftC = datetime_to_filetime(dt);
    return ::SetFileTime(handle_, &ftC, nullptr, nullptr) != 0;
}
#endif

#ifdef NEFORCE_PLATFORM_WINDOWS
bool file_info::set_all_times(const datetime& create, const datetime& access, const datetime& write) noexcept {
    const ::FILETIME ftC = datetime_to_filetime(create);
    const ::FILETIME ftA = datetime_to_filetime(access);
    const ::FILETIME ftW = datetime_to_filetime(write);
    return ::SetFileTime(handle_, &ftC, &ftA, &ftW) != 0;
}
#else
bool file_info::set_all_times(const datetime& access, const datetime& write) noexcept {
    ::timespec times[2]{};
    times[0].tv_sec = static_cast<::time_t>(timestamp(access).value());
    times[0].tv_nsec = 0;
    times[1].tv_sec = static_cast<::time_t>(timestamp(write).value());
    times[1].tv_nsec = 0;
    return ::futimens(handle_, times) == 0;
}
#endif

NEFORCE_END_NAMESPACE__
