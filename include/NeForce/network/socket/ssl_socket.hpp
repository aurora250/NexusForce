#ifndef NEFORCE_NETWORK_SSL_SOCKET_HPP__
#define NEFORCE_NETWORK_SSL_SOCKET_HPP__
#include "NeForce/network/socket/tcp_socket.hpp"
#include "NeForce/network/ssl/ssl_stream.hpp"
NEFORCE_BEGIN_NAMESPACE__

class NEFORCE_API ssl_socket final : public tcp_socket {
private:
    optional<ssl_stream> ssl_;

public:
    ssl_socket() = default;

    explicit ssl_socket(const native_handle_type fd) :
    tcp_socket(fd) {}

    explicit ssl_socket(tcp_socket sock) :
    tcp_socket(move(sock)) {}

    ssl_socket(ssl_socket&& other) noexcept = default;
    ssl_socket& operator=(ssl_socket&& other) noexcept = default;

    ~ssl_socket() override = default;

    void init_server_ssl(const ssl_context& ctx);
    void init_client_ssl(const ssl_context& ctx, const string& hostname = "");

    NEFORCE_NODISCARD string peer_certificate_info() const;

    ssize_t send(memory_view<const char> data, int flags = 0) override;
    ssize_t receive(memory_view<char> buffer, int flags = 0) override;

    NEFORCE_NODISCARD bool is_ssl() const noexcept override { return ssl_.has_value(); }

    NEFORCE_NODISCARD ssl_stream& ssl() noexcept { return *ssl_; }

    NEFORCE_NODISCARD const ssl_stream& ssl() const noexcept { return *ssl_; }
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_SSL_SOCKET_HPP__
