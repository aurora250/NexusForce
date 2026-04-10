#include <NeForce/network/ssl/ssl_acceptor.hpp>
NEFORCE_BEGIN_NAMESPACE__

void ssl_acceptor::set_ssl_context(ssl_context ctx) {
    if (!ctx.is_valid()) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("Invalid SSL context"));
    }
    ctx_ = move(ctx);
}

ssl_socket ssl_acceptor::accept_ssl() {
    if (!is_open()) {
        NEFORCE_THROW_EXCEPTION(value_exception("Acceptor socket is not open"));
    }
    if (!ctx_.is_valid()) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("SSL context is invalid"));
    }

    tcp_socket client = tcp_acceptor::accept();
    if (!client.is_open()) {
        NEFORCE_THROW_EXCEPTION(socket_exception("Failed to accept client connection"));
    }
    ssl_socket ssl_client(client.release());

    try {
        ssl_client.init_server_ssl(ctx_);
    } catch (...) {
        ssl_client.close();
        throw;
    }

    return ssl_client;
}

NEFORCE_END_NAMESPACE__
