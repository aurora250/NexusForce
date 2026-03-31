#include <NeForce/network/socket/ip_socket.hpp>
NEFORCE_BEGIN_NAMESPACE__

void ip_socket::open_ip(const int family, const int type, const int protocol) {
    if (family != AF_INET && family != AF_INET6) {
        NEFORCE_THROW_EXCEPTION(value_exception("Invalid address family: only AF_INET / AF_INET6 are supported"));
    }

    close();

    fd_ = ::socket(family, type, protocol);
    if (!is_open()) {
        NEFORCE_THROW_EXCEPTION(socket_exception("Failed to create IP socket"));
    }

    family_ = family;
}

void ip_socket::connect(const ip_address& endpoint) {
    if (!is_open()) {
        NEFORCE_THROW_EXCEPTION(value_exception("Socket is not open"));
    }
    if (!endpoint.is_valid()) {
        NEFORCE_THROW_EXCEPTION(value_exception("Invalid endpoint"));
    }
    if (::connect(fd_, endpoint.data(), endpoint.size()) < 0) {
        NEFORCE_THROW_EXCEPTION(socket_exception("Failed to connect to remote endpoint"));
    }
}

NEFORCE_END_NAMESPACE__
