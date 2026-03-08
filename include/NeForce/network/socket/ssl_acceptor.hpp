#ifndef NEFORCE_NETWORK_SSL_SSL_ACCEPTOR_HPP__
#define NEFORCE_NETWORK_SSL_SSL_ACCEPTOR_HPP__
#include "NeForce/network/socket/tcp_acceptor.hpp"
#ifdef NEFORCE_SUPPORT_OPENSSL
#include "NeForce/network/socket/ssl_socket.hpp"
NEFORCE_BEGIN_NAMESPACE__

class NEFORCE_API ssl_acceptor final : public tcp_acceptor {
private:
    ssl_context ctx_;

public:
    ssl_acceptor() = default;

    void set_ssl_context(ssl_context ctx);

    NEFORCE_NODISCARD ssl_socket accept_ssl();

    NEFORCE_NODISCARD ssl_context& context() noexcept {
        return ctx_;
    }

    NEFORCE_NODISCARD const ssl_context& context() const noexcept {
        return ctx_;
    }
};

NEFORCE_END_NAMESPACE__
#endif
#endif // NEFORCE_NETWORK_SSL_SSL_ACCEPTOR_HPP__
