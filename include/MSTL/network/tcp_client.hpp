#ifndef MSTL_NETWORK_TCP_TCP_CLIENT_HPP__
#define MSTL_NETWORK_TCP_TCP_CLIENT_HPP__
#include "./dns/dns_client.hpp"
#include "MSTL/core/functional/function.hpp"
#include "ssl_socket.hpp"
MSTL_BEGIN_NAMESPACE__

class MSTL_API tcp_client {
private:
#ifdef MSTL_SUPPORT_OPENSSL__
    using handle_sock_t = ssl_socket;
#else
    using handle_sock_t = tcp_socket;
#endif

    dns_client dns_{};
#ifdef MSTL_SUPPORT_OPENSSL__
    ssl_context ssl_ctx_{};
#endif
    handle_sock_t socket_{};

    string connected_host_;
    uint16_t connected_port_ = 0;

    milliseconds receive_timeout_{5000};
    milliseconds send_timeout_{5000};

    bool try_connect(const string& host, uint16_t port, const string& ip, bool ipv6);

public:
    explicit tcp_client() = default;
    explicit tcp_client(dns_client dns);
#ifdef MSTL_SUPPORT_OPENSSL__
    explicit tcp_client(ssl_context ctx);
#endif
    ~tcp_client();

    void set_recv_timeout(const milliseconds& timeout) { receive_timeout_ = timeout; }
    void set_send_timeout(const milliseconds& timeout) { send_timeout_ = timeout; }
#ifdef MSTL_SUPPORT_OPENSSL__
    void set_ssl_context(ssl_context ctx) { ssl_ctx_ = _MSTL move(ctx); }
#endif

    milliseconds recv_timeout() const { return receive_timeout_; }
    milliseconds send_timeout() const { return send_timeout_; }

    bool connect(const string& host, uint16_t port);
    void close() noexcept;

    MSTL_NODISCARD bool is_connected() const noexcept { return socket_.is_valid(); }
    MSTL_NODISCARD const string& connected_host() const noexcept { return connected_host_; }
    MSTL_NODISCARD uint16_t connected_port() const noexcept { return connected_port_; }

    ssize_t send(const void* data, size_t length);
    ssize_t send(string_view data);
    ssize_t receive(void* buffer, size_t length) const;

    bool receive_all(string& out_data, size_t expected_length = 0);
    void receive_with_callback(function<void(const string&)> callback, size_t buffer_size = 8192);
};

MSTL_END_NAMESPACE__
#endif // MSTL_NETWORK_TCP_TCP_CLIENT_HPP__
