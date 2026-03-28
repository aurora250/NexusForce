#ifndef NEFORCE_NETWORK_SOCKET_TCP_SOCKET_HPP__
#define NEFORCE_NETWORK_SOCKET_TCP_SOCKET_HPP__
#include "NeForce/core/container/vector.hpp"
#include "NeForce/network/socket/socket_base.hpp"
NEFORCE_BEGIN_NAMESPACE__

class NEFORCE_API tcp_socket : public socket_base {
public:
    tcp_socket() = default;

    tcp_socket(tcp_socket&& other) = default;
    tcp_socket &operator =(tcp_socket&& other) = default;

    explicit tcp_socket(const native_handle_type fd)
    : socket_base(fd) {}

    ~tcp_socket() override = default;

    void open(int family = AF_INET);

    void connect(const ip_address& endpoint);
    bool connect(const ip_address& endpoint, milliseconds timeout, bool was_blocking = true);

    virtual ssize_t send(memory_view<const char> data, int flags = 0);
    ssize_t send(memory_view<const char> data, milliseconds timeout, int flags = 0);
    void send_all(memory_view<const char> data);

    virtual ssize_t receive(memory_view<char> buffer, int flags = 0);
    vector<char> receive_all(size_t expected_size);

    NEFORCE_NODISCARD virtual bool is_ssl() const noexcept {
        return false;
    }
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_SOCKET_TCP_SOCKET_HPP__
