#ifndef NEFORCE_TESTS_INTEGRATION_UTILS_HPP__
#define NEFORCE_TESTS_INTEGRATION_UTILS_HPP__
#include <NeForce/network/udp_socket.hpp>

inline bool network_available() {
    using namespace neforce;

    udp_socket sock;
    if (!sock.try_open(ip_family::inet4, socket_type::dgram)) {
        return false;
    }
    sock.set_reuse_address(true);
    auto addr = ip_address::any();
    sock.bind(addr);
    auto local = sock.local_endpoint();
    sock.close();
    return local.has_value();
}

#ifdef NEFORCE_PLATFORM_WINDOWS
NEFORCE_INLINE17 constexpr const char* SERVER_CERT = "D:/OpenSSL/server.crt";
NEFORCE_INLINE17 constexpr const char* SERVER_KEY = "D:/OpenSSL/server.key";
#else
NEFORCE_INLINE17 constexpr const char* SERVER_CERT = "/home/huenqi/server.crt";
NEFORCE_INLINE17 constexpr const char* SERVER_KEY = "/home/huenqi/server.key";
#endif

#endif // NEFORCE_TESTS_INTEGRATION_UTILS_HPP__
