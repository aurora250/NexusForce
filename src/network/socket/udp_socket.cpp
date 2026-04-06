#include <NeForce/network/socket/udp_socket.hpp>
NEFORCE_BEGIN_NAMESPACE__

void udp_socket::open(const int family) { open_ip(family, SOCK_DGRAM, IPPROTO_UDP); }

ssize_t udp_socket::send_to(memory_view<const char> data, const ip_address& endpoint, const int flags) {
    if (!is_open()) {
        NEFORCE_THROW_EXCEPTION(value_exception("Socket is not open"));
    }

    if (!endpoint.is_valid()) {
        NEFORCE_THROW_EXCEPTION(value_exception("Invalid destination endpoint for UDP send"));
    }
    if (data.empty()) {
        return 0;
    }

    const ssize_t result =
            ::sendto(fd_, data.data(), static_cast<int>(data.size()), flags, endpoint.data(), endpoint.size());

    if (result < 0) {
        NEFORCE_THROW_EXCEPTION(socket_exception("Failed to send UDP datagram to specified endpoint"));
    }
    return result;
}

ssize_t udp_socket::send(memory_view<const char> data, const int flags) {
    if (!is_open()) {
        NEFORCE_THROW_EXCEPTION(value_exception("Invalid socket fd for UDP send"));
    }

    const ssize_t result = ::send(fd_, data.data(), static_cast<int>(data.size()), flags);
    if (result < 0) {
        NEFORCE_THROW_EXCEPTION(socket_exception("Failed to send UDP datagram to connected endpoint"));
    }
    return result;
}

pair<ssize_t, ip_address> udp_socket::receive_from(memory_view<char> buffer, const int flags) {
    if (!is_open()) {
        NEFORCE_THROW_EXCEPTION(value_exception("Invalid socket fd for UDP receive"));
    }

    if (buffer.empty()) {
        NEFORCE_THROW_EXCEPTION(value_exception("Receive buffer cannot be empty"));
    }

    ::sockaddr_storage addr_storage;
    ::socklen_t addrlen = sizeof(addr_storage);

    ssize_t result = ::recvfrom(fd_, buffer.data(), static_cast<int>(buffer.size()), flags,
                                reinterpret_cast<struct sockaddr*>(&addr_storage), &addrlen);

    if (result < 0) {
        NEFORCE_THROW_EXCEPTION(socket_exception("Failed to receive UDP datagram"));
    }

    ip_address sender;
    if (addr_storage.ss_family == AF_INET) {
        sender = ip_address(*reinterpret_cast<const sockaddr_in*>(&addr_storage));
    } else if (addr_storage.ss_family == AF_INET6) {
        sender = ip_address(*reinterpret_cast<const sockaddr_in6*>(&addr_storage));
    } else {
        NEFORCE_THROW_EXCEPTION(socket_exception("Unsupport socket type"));
    }

    return {result, move(sender)};
}

ssize_t udp_socket::receive(memory_view<char> buffer, const int flags) {
    if (!is_open()) {
        NEFORCE_THROW_EXCEPTION(value_exception("Invalid socket fd for UDP receive"));
    }

    if (buffer.empty()) {
        NEFORCE_THROW_EXCEPTION(value_exception("Receive buffer cannot be empty"));
    }

    const ssize_t result = ::recv(fd_, buffer.data(), static_cast<int>(buffer.size()), flags);
    if (result < 0) {
        NEFORCE_THROW_EXCEPTION(socket_exception("Failed to receive UDP datagram from connected endpoint"));
    }
    return result;
}

NEFORCE_END_NAMESPACE__
