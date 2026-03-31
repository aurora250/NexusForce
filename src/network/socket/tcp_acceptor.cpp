#include <NeForce/network/socket/tcp_acceptor.hpp>
NEFORCE_BEGIN_NAMESPACE__

void tcp_acceptor::open(const ip_address& endpoint, const int backlog) {
    if (!endpoint.is_valid()) {
        NEFORCE_THROW_EXCEPTION(value_exception("Invalid endpoint for TCP acceptor"));
    }

    open_ip(endpoint.family(), SOCK_STREAM, IPPROTO_TCP);

    if (!set_reuse_address(true)) {
        close();
        NEFORCE_THROW_EXCEPTION(socket_exception("Failed to set SO_REUSEADDR on acceptor socket"));
    }

    if (::bind(fd_, endpoint.data(), endpoint.size()) < 0) {
        close();
        NEFORCE_THROW_EXCEPTION(socket_exception("Failed to bind acceptor socket to endpoint"));
    }

    if (::listen(fd_, backlog) < 0) {
        close();
        NEFORCE_THROW_EXCEPTION(socket_exception("Failed to listen on acceptor socket"));
    }
}

tcp_socket tcp_acceptor::accept() {
    if (!is_open()) {
        NEFORCE_THROW_EXCEPTION(value_exception("Acceptor socket is not open"));
    }

    ::sockaddr_storage client_storage{};
    ::socklen_t addrlen = sizeof(client_storage);

    const native_handle_type client_fd = ::accept(fd_, reinterpret_cast<::sockaddr*>(&client_storage), &addrlen);
    if (client_fd == invalid_handle) {
        NEFORCE_THROW_EXCEPTION(socket_exception("Failed to accept incoming connection"));
    }

    return tcp_socket(client_fd);
}

optional<tcp_socket> tcp_acceptor::accept_nonblock() {
    if (!is_open()) {
        return none;
    }

    ::sockaddr_storage client_storage{};
    ::socklen_t addrlen = sizeof(client_storage);

    const native_handle_type client_fd = ::accept(fd_, reinterpret_cast<::sockaddr*>(&client_storage), &addrlen);
    if (client_fd == invalid_handle) {
        const int error = socket_exception::last_error();
        if (socket_exception::is_would_block(error)) {
            return none;
        }
        NEFORCE_THROW_EXCEPTION(socket_exception("Failed to accept incoming connection in non-blocking mode"));
    }

    return tcp_socket(client_fd);
}

NEFORCE_END_NAMESPACE__
