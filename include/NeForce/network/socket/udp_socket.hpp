#ifndef NEFORCE_NETWORK_SOCKET_UDP_SOCKET_HPP__
#define NEFORCE_NETWORK_SOCKET_UDP_SOCKET_HPP__
#include "NeForce/core/memory/memory_view.hpp"
#include "NeForce/network/socket/ip_socket.hpp"
NEFORCE_BEGIN_NAMESPACE__

class NEFORCE_API udp_socket final : public ip_socket {
public:
    udp_socket() = default;

    udp_socket(udp_socket&&) = default;
    udp_socket& operator=(udp_socket&&) = default;

    void open(int family = AF_INET);

    ssize_t send_to(memory_view<const char> data, const ip_address& endpoint, int flags = 0);
    ssize_t send(memory_view<const char> data, int flags = 0);

    pair<ssize_t, ip_address> receive_from(memory_view<char> buffer, int flags = 0);
    ssize_t receive(memory_view<char> buffer, int flags = 0);
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_SOCKET_UDP_SOCKET_HPP__
