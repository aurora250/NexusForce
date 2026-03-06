#ifndef NEFORCE_NETWORK_SOCKET_TCP_ACCEPTOR_HPP__
#define NEFORCE_NETWORK_SOCKET_TCP_ACCEPTOR_HPP__
#include "NeForce/network/socket/tcp_socket.hpp"
NEFORCE_BEGIN_NAMESPACE__

class NEFORCE_API tcp_acceptor : public socket_base {
public:
    tcp_acceptor() = default;

    void open(const ip_address& endpoint, int backlog = SOMAXCONN);

    tcp_socket accept();

    optional<tcp_socket> accept_nonblock();
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_SOCKET_TCP_ACCEPTOR_HPP__
