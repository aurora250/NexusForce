#include <NeForce/network/ip_socket.hpp>
NEFORCE_BEGIN_NAMESPACE__

void ip_socket::open_ip(const ip_family f, const type t, const protocol p) {
    if (f != ip_family::inet4 && f != ip_family::inet6) {
        NEFORCE_THROW_EXCEPTION(value_exception("Invalid address family: only INET4 / INET6 are supported"));
    }

    close();

    fd_ = ::socket(static_cast<int>(f), static_cast<int>(t), static_cast<int>(p));
    if (!is_open()) {
        NEFORCE_THROW_EXCEPTION(socket_exception("Failed to create IP socket"));
    }

    family_ = f;
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
