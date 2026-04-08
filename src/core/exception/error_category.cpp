#include <NeForce/core/exception/error_code.hpp>
#include <NeForce/core/utility/packages.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <NeForce/core/config/windef.hpp>
#    include <windef.h>
#    include <minwindef.h>
#    include <WinBase.h>
#endif
#ifdef NEFORCE_PLATFORM_LINUX
#    include <cstring>
#endif
NEFORCE_BEGIN_NAMESPACE__

error_code last_error() noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    return {static_cast<int>(::GetLastError()), system_category()};
#else
    return {errno, system_category()};
#endif
}

error_condition error_category::default_error_condition(int ev) const noexcept { return {ev, *this}; }

bool error_category::equivalent(int code, const error_condition& condition) const noexcept {
    return default_error_condition(code) == condition;
}

bool error_category::equivalent(const error_code& code, int condition) const noexcept {
    return *this == code.category() && code.value() == condition;
}

string generic_error_category::message(int ev) const {
#ifdef NEFORCE_PLATFORM_WINDOWS
    char buf[256];
    if (::strerror_s(buf, sizeof(buf), ev) == 0) {
        return string(buf);
    }
    return "unknown error " + _NEFORCE to_string(ev);
#else
    char buf[256];
#    if (_POSIX_C_SOURCE >= 200112L) && !defined(_GNU_SOURCE)
    if (::strerror_r(ev, buf, sizeof(buf)) == 0) {
        return string(buf);
    }
    return "unknown error " + _NEFORCE to_string(ev);
#    else
    const char* s = ::strerror_r(ev, buf, sizeof(buf));
    return s != nullptr ? string(s) : "unknown error " + _NEFORCE to_string(ev);
#    endif
#endif
}

error_condition generic_error_category::default_error_condition(int ev) const noexcept { return {ev, *this}; }

const error_category& generic_category() noexcept {
    static generic_error_category instance;
    return instance;
}


#ifdef NEFORCE_PLATFORM_WINDOWS

string system_error_category::message(int ev) const {
    if (ev == 0) {
        return "success";
    }

    ::LPSTR buf = nullptr;
    ::FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                     nullptr, static_cast<DWORD>(ev), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf, 0, nullptr);

    string result{buf};
    ::LocalFree(buf);

    return result.empty() ? "unknown system error " + to_string(ev) : result;
}

error_condition system_error_category::default_error_condition(int ev) const noexcept {
    switch (static_cast<DWORD>(ev)) {
        case ERROR_SUCCESS:
            return {0, generic_category()};
        case ERROR_ACCESS_DENIED:
            return {(int) errc::permission_denied, generic_category()};
        case ERROR_ALREADY_EXISTS:
            return {(int) errc::file_exists, generic_category()};
        case ERROR_BAD_UNIT:
            return {(int) errc::no_such_device, generic_category()};
        case ERROR_BROKEN_PIPE:
            return {(int) errc::broken_pipe, generic_category()};
        case ERROR_BUFFER_OVERFLOW:
            return {(int) errc::filename_too_long, generic_category()};
        case ERROR_BUSY:
            return {(int) errc::device_or_resource_busy, generic_category()};
        case ERROR_BUSY_DRIVE:
            return {(int) errc::device_or_resource_busy, generic_category()};
        case ERROR_CANNOT_MAKE:
            return {(int) errc::permission_denied, generic_category()};
        case ERROR_CANTOPEN:
            return {(int) errc::io_error, generic_category()};
        case ERROR_CANTREAD:
            return {(int) errc::io_error, generic_category()};
        case ERROR_CANTWRITE:
            return {(int) errc::io_error, generic_category()};
        case ERROR_CURRENT_DIRECTORY:
            return {(int) errc::permission_denied, generic_category()};
        case ERROR_DEV_NOT_EXIST:
            return {(int) errc::no_such_device, generic_category()};
        case ERROR_DEVICE_IN_USE:
            return {(int) errc::device_or_resource_busy, generic_category()};
        case ERROR_DIR_NOT_EMPTY:
            return {(int) errc::directory_not_empty, generic_category()};
        case ERROR_DIRECTORY:
            return {(int) errc::invalid_argument, generic_category()};
        case ERROR_DISK_FULL:
            return {(int) errc::no_space_on_device, generic_category()};
        case ERROR_FILE_EXISTS:
            return {(int) errc::file_exists, generic_category()};
        case ERROR_FILE_NOT_FOUND:
            return {(int) errc::no_such_file_or_directory, generic_category()};
        case ERROR_HANDLE_DISK_FULL:
            return {(int) errc::no_space_on_device, generic_category()};
        case ERROR_HANDLE_EOF:
            return {(int) errc::value_too_large, generic_category()};
        case ERROR_INVALID_ACCESS:
            return {(int) errc::permission_denied, generic_category()};
        case ERROR_INVALID_DRIVE:
            return {(int) errc::no_such_device, generic_category()};
        case ERROR_INVALID_FUNCTION:
            return {(int) errc::function_not_supported, generic_category()};
        case ERROR_INVALID_HANDLE:
            return {(int) errc::invalid_argument, generic_category()};
        case ERROR_INVALID_NAME:
            return {(int) errc::invalid_argument, generic_category()};
        case ERROR_INVALID_PARAMETER:
            return {(int) errc::invalid_argument, generic_category()};
        case ERROR_LOCK_VIOLATION:
            return {(int) errc::no_lock_available, generic_category()};
        case ERROR_LOCKED:
            return {(int) errc::no_lock_available, generic_category()};
        case ERROR_NEGATIVE_SEEK:
            return {(int) errc::invalid_argument, generic_category()};
        case ERROR_NOACCESS:
            return {(int) errc::permission_denied, generic_category()};
        case ERROR_NOT_ENOUGH_MEMORY:
            return {(int) errc::not_enough_memory, generic_category()};
        case ERROR_NOT_READY:
            return {(int) errc::resource_unavailable_try_again, generic_category()};
        case ERROR_NOT_SAME_DEVICE:
            return {(int) errc::cross_device_link, generic_category()};
        case ERROR_NOT_SUPPORTED:
            return {(int) errc::not_supported, generic_category()};
        case ERROR_OPEN_FAILED:
            return {(int) errc::io_error, generic_category()};
        case ERROR_OPERATION_ABORTED:
            return {(int) errc::operation_canceled, generic_category()};
        case ERROR_OUTOFMEMORY:
            return {(int) errc::not_enough_memory, generic_category()};
        case ERROR_PATH_NOT_FOUND:
            return {(int) errc::no_such_file_or_directory, generic_category()};
        case ERROR_READ_FAULT:
            return {(int) errc::io_error, generic_category()};
        case ERROR_RETRY:
            return {(int) errc::resource_unavailable_try_again, generic_category()};
        case ERROR_SEEK:
            return {(int) errc::io_error, generic_category()};
        case ERROR_SHARING_VIOLATION:
            return {(int) errc::permission_denied, generic_category()};
        case ERROR_TOO_MANY_OPEN_FILES:
            return {(int) errc::too_many_files_open, generic_category()};
        case ERROR_WRITE_FAULT:
            return {(int) errc::io_error, generic_category()};
        case ERROR_WRITE_PROTECT:
            return {(int) errc::permission_denied, generic_category()};
        case WSAEACCES:
            return {(int) errc::permission_denied, generic_category()};
        case WSAEADDRINUSE:
            return {(int) errc::address_in_use, generic_category()};
        case WSAEADDRNOTAVAIL:
            return {(int) errc::address_not_available, generic_category()};
        case WSAEAFNOSUPPORT:
            return {(int) errc::address_family_not_supported, generic_category()};
        case WSAEALREADY:
            return {(int) errc::connection_already_in_progress, generic_category()};
        case WSAECONNABORTED:
            return {(int) errc::connection_aborted, generic_category()};
        case WSAECONNREFUSED:
            return {(int) errc::connection_refused, generic_category()};
        case WSAECONNRESET:
            return {(int) errc::connection_reset, generic_category()};
        case WSAEDESTADDRREQ:
            return {(int) errc::destination_address_required, generic_category()};
        case WSAEHOSTUNREACH:
            return {(int) errc::host_unreachable, generic_category()};
        case WSAEINPROGRESS:
            return {(int) errc::operation_in_progress, generic_category()};
        case WSAEINTR:
            return {(int) errc::interrupted, generic_category()};
        case WSAEINVAL:
            return {(int) errc::invalid_argument, generic_category()};
        case WSAEISCONN:
            return {(int) errc::already_connected, generic_category()};
        case WSAEMSGSIZE:
            return {(int) errc::message_size, generic_category()};
        case WSAENETDOWN:
            return {(int) errc::network_down, generic_category()};
        case WSAENETRESET:
            return {(int) errc::network_reset, generic_category()};
        case WSAENETUNREACH:
            return {(int) errc::network_unreachable, generic_category()};
        case WSAENOBUFS:
            return {(int) errc::no_buffer_space, generic_category()};
        case WSAENOPROTOOPT:
            return {(int) errc::no_protocol_option, generic_category()};
        case WSAENOTCONN:
            return {(int) errc::not_connected, generic_category()};
        case WSAENOTSOCK:
            return {(int) errc::not_a_socket, generic_category()};
        case WSAEOPNOTSUPP:
            return {(int) errc::operation_not_supported, generic_category()};
        case WSAEPROTONOSUPPORT:
            return {(int) errc::protocol_not_supported, generic_category()};
        case WSAEPROTOTYPE:
            return {(int) errc::wrong_protocol_type, generic_category()};
        case WSAETIMEDOUT:
            return {(int) errc::timed_out, generic_category()};
        case WSAEWOULDBLOCK:
            return {(int) errc::operation_would_block, generic_category()};
        default:
            return {ev, *this};
    }
}

#else

string system_error_category::message(int ev) const {
    char buf[256];
#    if (_POSIX_C_SOURCE >= 200112L) && !defined(_GNU_SOURCE)
    if (::strerror_r(ev, buf, sizeof(buf)) == 0) {
        return string(buf);
    }
    return "unknown error " + to_string(ev);
#    else
    const char* s = ::strerror_r(ev, buf, sizeof(buf));
    return s != nullptr ? string(s) : "unknown error " + _NEFORCE to_string(ev);
#    endif
}

error_condition system_error_category::default_error_condition(int ev) const noexcept {
    return {ev, generic_category()};
}

#endif

const error_category& system_category() noexcept {
    static system_error_category instance;
    return instance;
}

NEFORCE_END_NAMESPACE__
