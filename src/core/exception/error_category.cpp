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

error_condition error_category::default_error_condition(const int32_t ev) const noexcept { return {ev, *this}; }

bool error_category::equivalent(const int code, const error_condition& condition) const noexcept {
    return default_error_condition(code) == condition;
}

bool error_category::equivalent(const error_code& code, const int condition) const noexcept {
    return *this == code.category() && code.value() == condition;
}

string generic_error_category::message(const int32_t ev) const {
#ifdef NEFORCE_PLATFORM_WINDOWS
    char buf[256];
    if (::strerror_s(buf, sizeof(buf), ev) == 0) {
        return {buf};
    }
    return "unknown error " + to_string(ev);
#else
    char buf[256];
#    if (_POSIX_C_SOURCE >= 200112L) && !defined(_GNU_SOURCE)
    if (::strerror_r(ev, buf, sizeof(buf)) == 0) {
        return string(buf);
    }
    return "unknown error " + to_string(ev);
#    else
    const char* s = ::strerror_r(ev, buf, sizeof(buf));
    return s != nullptr ? string(s) : "unknown error " + to_string(ev);
#    endif
#endif
}

error_condition generic_error_category::default_error_condition(const int32_t ev) const noexcept { return {ev, *this}; }

const error_category& generic_category() noexcept {
    static generic_error_category instance;
    return instance;
}

string system_error_category::message(const int32_t ev) const {
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (ev == 0) {
        return "success";
    }
    const ::LPSTR buf = nullptr;
    ::FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                     nullptr, static_cast<::DWORD>(ev), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf, 0, nullptr);
    string result;
    if (buf != nullptr) {
        result = buf;
    }
    ::LocalFree(buf);
    return result.empty() ? "unknown system error " + to_string(ev) : result;

#else
    char buf[256];
#    if (_POSIX_C_SOURCE >= 200112L) && !defined(_GNU_SOURCE)
    if (::strerror_r(ev, buf, sizeof(buf)) == 0) {
        return string(buf);
    }
    return "unknown error " + to_string(ev);
#    else
    const char* s = ::strerror_r(ev, buf, sizeof(buf));
    return s != nullptr ? string(s) : "unknown error " + to_string(ev);
#    endif
#endif
}

error_condition system_error_category::default_error_condition(const int32_t ev) const noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    switch (static_cast<::DWORD>(ev)) {
        case ERROR_SUCCESS:
            return {0, generic_category()};
        case ERROR_ACCESS_DENIED:
            return {static_cast<int32_t>(errc::permission_denied), generic_category()};
        case ERROR_ALREADY_EXISTS:
            return {static_cast<int32_t>(errc::file_exists), generic_category()};
        case ERROR_BAD_UNIT:
            return {static_cast<int32_t>(errc::no_such_device), generic_category()};
        case ERROR_BROKEN_PIPE:
            return {static_cast<int32_t>(errc::broken_pipe), generic_category()};
        case ERROR_BUFFER_OVERFLOW:
            return {static_cast<int32_t>(errc::filename_too_long), generic_category()};
        case ERROR_BUSY:
        case ERROR_BUSY_DRIVE:
            return {static_cast<int32_t>(errc::device_or_resource_busy), generic_category()};
        case ERROR_CANNOT_MAKE:
            return {static_cast<int32_t>(errc::permission_denied), generic_category()};
        case ERROR_CANTOPEN:
        case ERROR_CANTREAD:
        case ERROR_CANTWRITE:
            return {static_cast<int32_t>(errc::io_error), generic_category()};
        case ERROR_CURRENT_DIRECTORY:
            return {static_cast<int32_t>(errc::permission_denied), generic_category()};
        case ERROR_DEV_NOT_EXIST:
            return {static_cast<int32_t>(errc::no_such_device), generic_category()};
        case ERROR_DEVICE_IN_USE:
            return {static_cast<int32_t>(errc::device_or_resource_busy), generic_category()};
        case ERROR_DIR_NOT_EMPTY:
            return {static_cast<int32_t>(errc::directory_not_empty), generic_category()};
        case ERROR_DIRECTORY:
            return {static_cast<int32_t>(errc::invalid_argument), generic_category()};
        case ERROR_DISK_FULL:
            return {static_cast<int32_t>(errc::no_space_on_device), generic_category()};
        case ERROR_FILE_EXISTS:
            return {static_cast<int32_t>(errc::file_exists), generic_category()};
        case ERROR_FILE_NOT_FOUND:
            return {static_cast<int32_t>(errc::no_such_file_or_directory), generic_category()};
        case ERROR_HANDLE_DISK_FULL:
            return {static_cast<int32_t>(errc::no_space_on_device), generic_category()};
        case ERROR_HANDLE_EOF:
            return {static_cast<int32_t>(errc::value_too_large), generic_category()};
        case ERROR_INVALID_ACCESS:
            return {static_cast<int32_t>(errc::permission_denied), generic_category()};
        case ERROR_INVALID_DRIVE:
            return {static_cast<int32_t>(errc::no_such_device), generic_category()};
        case ERROR_INVALID_FUNCTION:
            return {static_cast<int32_t>(errc::function_not_supported), generic_category()};
        case ERROR_INVALID_HANDLE:
        case ERROR_INVALID_NAME:
        case ERROR_INVALID_PARAMETER:
            return {static_cast<int32_t>(errc::invalid_argument), generic_category()};
        case ERROR_LOCK_VIOLATION:
        case ERROR_LOCKED:
            return {static_cast<int32_t>(errc::no_lock_available), generic_category()};
        case ERROR_NEGATIVE_SEEK:
            return {static_cast<int32_t>(errc::invalid_argument), generic_category()};
        case ERROR_NOACCESS:
            return {static_cast<int32_t>(errc::permission_denied), generic_category()};
        case ERROR_NOT_ENOUGH_MEMORY:
            return {static_cast<int32_t>(errc::not_enough_memory), generic_category()};
        case ERROR_NOT_READY:
            return {static_cast<int32_t>(errc::resource_unavailable_try_again), generic_category()};
        case ERROR_NOT_SAME_DEVICE:
            return {static_cast<int32_t>(errc::cross_device_link), generic_category()};
        case ERROR_NOT_SUPPORTED:
            return {static_cast<int32_t>(errc::not_supported), generic_category()};
        case ERROR_OPEN_FAILED:
            return {static_cast<int32_t>(errc::io_error), generic_category()};
        case ERROR_OPERATION_ABORTED:
            return {static_cast<int32_t>(errc::operation_canceled), generic_category()};
        case ERROR_OUTOFMEMORY:
            return {static_cast<int32_t>(errc::not_enough_memory), generic_category()};
        case ERROR_PATH_NOT_FOUND:
            return {static_cast<int32_t>(errc::no_such_file_or_directory), generic_category()};
        case ERROR_READ_FAULT:
            return {static_cast<int32_t>(errc::io_error), generic_category()};
        case ERROR_RETRY:
            return {static_cast<int32_t>(errc::resource_unavailable_try_again), generic_category()};
        case ERROR_SEEK:
            return {static_cast<int32_t>(errc::io_error), generic_category()};
        case ERROR_SHARING_VIOLATION:
            return {static_cast<int32_t>(errc::permission_denied), generic_category()};
        case ERROR_TOO_MANY_OPEN_FILES:
            return {static_cast<int32_t>(errc::too_many_files_open), generic_category()};
        case ERROR_WRITE_FAULT:
            return {static_cast<int32_t>(errc::io_error), generic_category()};
        case ERROR_WRITE_PROTECT:
        case WSAEACCES:
            return {static_cast<int32_t>(errc::permission_denied), generic_category()};
        case WSAEADDRINUSE:
            return {static_cast<int32_t>(errc::address_in_use), generic_category()};
        case WSAEADDRNOTAVAIL:
            return {static_cast<int32_t>(errc::address_not_available), generic_category()};
        case WSAEAFNOSUPPORT:
            return {static_cast<int32_t>(errc::address_family_not_supported), generic_category()};
        case WSAEALREADY:
            return {static_cast<int32_t>(errc::connection_already_in_progress), generic_category()};
        case WSAECONNABORTED:
            return {static_cast<int32_t>(errc::connection_aborted), generic_category()};
        case WSAECONNREFUSED:
            return {static_cast<int32_t>(errc::connection_refused), generic_category()};
        case WSAECONNRESET:
            return {static_cast<int32_t>(errc::connection_reset), generic_category()};
        case WSAEDESTADDRREQ:
            return {static_cast<int32_t>(errc::destination_address_required), generic_category()};
        case WSAEHOSTUNREACH:
            return {static_cast<int32_t>(errc::host_unreachable), generic_category()};
        case WSAEINPROGRESS:
            return {static_cast<int32_t>(errc::operation_in_progress), generic_category()};
        case WSAEINTR:
            return {static_cast<int32_t>(errc::interrupted), generic_category()};
        case WSAEINVAL:
            return {static_cast<int32_t>(errc::invalid_argument), generic_category()};
        case WSAEISCONN:
            return {static_cast<int32_t>(errc::already_connected), generic_category()};
        case WSAEMSGSIZE:
            return {static_cast<int32_t>(errc::message_size), generic_category()};
        case WSAENETDOWN:
            return {static_cast<int32_t>(errc::network_down), generic_category()};
        case WSAENETRESET:
            return {static_cast<int32_t>(errc::network_reset), generic_category()};
        case WSAENETUNREACH:
            return {static_cast<int32_t>(errc::network_unreachable), generic_category()};
        case WSAENOBUFS:
            return {static_cast<int32_t>(errc::no_buffer_space), generic_category()};
        case WSAENOPROTOOPT:
            return {static_cast<int32_t>(errc::no_protocol_option), generic_category()};
        case WSAENOTCONN:
            return {static_cast<int32_t>(errc::not_connected), generic_category()};
        case WSAENOTSOCK:
            return {static_cast<int32_t>(errc::not_a_socket), generic_category()};
        case WSAEOPNOTSUPP:
            return {static_cast<int32_t>(errc::operation_not_supported), generic_category()};
        case WSAEPROTONOSUPPORT:
            return {static_cast<int32_t>(errc::protocol_not_supported), generic_category()};
        case WSAEPROTOTYPE:
            return {static_cast<int32_t>(errc::wrong_protocol_type), generic_category()};
        case WSAETIMEDOUT:
            return {static_cast<int32_t>(errc::timed_out), generic_category()};
        case WSAEWOULDBLOCK:
            return {static_cast<int32_t>(errc::operation_would_block), generic_category()};
        default:
            return {ev, *this};
    }
#else
    return {ev, generic_category()};
#endif
}

const error_category& system_category() noexcept {
    static system_error_category instance;
    return instance;
}

NEFORCE_END_NAMESPACE__
