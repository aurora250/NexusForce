#ifndef MSTL_NETWORK_TCP_TCP_CLIENT_HPP__
#define MSTL_NETWORK_TCP_TCP_CLIENT_HPP__
#include "../dns/dns_client.hpp"
#include "MSTL/core/functional/function.hpp"
#include "tcp_socket.hpp"
MSTL_BEGIN_NAMESPACE__

class MSTL_API tcp_client {
private:
    dns_client dns_{};
    tcp_socket sock_{};
    bool connected_ = false;
    string connected_host_;
    uint16_t connected_port_ = 0;

    milliseconds read_timeout_{5000};
    milliseconds send_timeout_{5000};

    bool try_connect(const string& host, uint16_t port, const string& ip, bool ipv6);
    bool connect_domain(const string& host, uint16_t port);
    void close_connection() noexcept;

public:
    explicit tcp_client() = default;
    explicit tcp_client(dns_client dns) : dns_(_MSTL move(dns)) {}
    ~tcp_client() { close(); }

    void set_read_timeout(const milliseconds& timeout) { read_timeout_ = timeout; }
    void set_send_timeout(const milliseconds& timeout) { send_timeout_ = timeout; }

    bool connect(const string& host, uint16_t port);
    MSTL_NODISCARD bool is_connected() const noexcept { return connected_; }
    void close() noexcept { close_connection(); }

    MSTL_NODISCARD const string& connected_host() const noexcept { return connected_host_; }
    MSTL_NODISCARD uint16_t connected_port() const noexcept { return connected_port_; }

    ssize_t send(const void* data, size_t length);
    ssize_t send(const string& data);
    ssize_t receive(void* buffer, size_t length) const;
    bool receive_all(string& out_data, size_t expected_length = 0);

    tcp_client& operator <<(const string& data);
    tcp_client& operator >>(string& data);

    void receive_with_callback(function<void(const string&)> callback, size_t buffer_size = 8192);
};

MSTL_END_NAMESPACE__
#endif // MSTL_NETWORK_TCP_TCP_CLIENT_HPP__
