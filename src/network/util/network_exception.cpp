#include <NeForce/network/util/network_exception.hpp>
#include <NeForce/core/utility/packages.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <NeForce/core/config/windef.hpp>
#    include <WinSock2.h>
#endif
#ifdef NEFORCE_PLATFORM_LINUX
#    include <cerrno>
#endif
NEFORCE_BEGIN_NAMESPACE__

error_code network_exception::last_error() noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    return {static_cast<int32_t>(::WSAGetLastError()), system_category()};
#else
    return {errno, system_category()};
#endif
}

string network_error_category::message(const int32_t ev) const {
    if (ev == 0) {
        return "";
    }
    switch (static_cast<network_errc>(ev)) {
        case network_errc::timeout:
            return "Network operation timed out";
        case network_errc::network_error:
            return "Network error";
        case network_errc::parse_error:
            return "Network message parse error";
        case network_errc::server_failure:
            return "Remote server failure";
        case network_errc::truncated:
            return "Response truncated";
        case network_errc::no_record:
            return "No record found";
        default:
            return "unknown network error " + to_string(ev);
    }
}

const error_category& network_category() noexcept {
    static network_error_category instance;
    return instance;
}

NEFORCE_END_NAMESPACE__
