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

optional<ssl_socket> ssl_acceptor::accept_ssl_nonblock() {
    if (!is_open()) {
        return none;
    }
    if (!ctx_.is_valid()) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("SSL context is invalid"));
    }

    auto client = tcp_acceptor::accept_nonblock();
    if (!client) {
        return none;
    }

    ssl_socket ssl_client(client->release());

    try {
        ssl_client.init_server_ssl(ctx_);
    } catch (...) {
        ssl_client.close();
        throw;
    }

    return ssl_client;
}

namespace {
    struct ssl_accept_op : enable_shared_from_this<ssl_accept_op> {
        io_context* ctx;
        ssl_acceptor* acceptor;
        function<void(error_code, ssl_socket)> handler;
        cancellation_slot* cancel_slot{nullptr};
        unique_ptr<ssl_socket> pending;

        void start() {
            if (cancel_slot != nullptr && cancel_slot->is_cancelled()) {
                handler(make_operation_aborted(), ssl_socket{});
                return;
            }

            auto self = shared_from_this();
            if (cancel_slot != nullptr) {
                acceptor->tcp_acceptor::async_accept(
                        *ctx, *cancel_slot,
                        [self](error_code ec, tcp_socket sock) mutable { self->on_accepted(ec, move(sock)); });
            } else {
                acceptor->tcp_acceptor::async_accept(
                        *ctx, [self](error_code ec, tcp_socket sock) mutable { self->on_accepted(ec, move(sock)); });
            }
        }

        void on_accepted(error_code ec, tcp_socket sock) {
            if (ec) {
                handler(ec, ssl_socket{});
                return;
            }

            pending = make_unique<ssl_socket>(move(sock));
            // The accepted socket must stay non-blocking so the async handshake and
            // subsequent async I/O are event driven and never block the io_context thread.
            pending->set_nonblocking(true);
            try {
                pending->prepare_server_ssl(acceptor->context());
            } catch (const system_exception& e) {
                handler(e.code(), ssl_socket{});
                return;
            } catch (const exception&) {
                handler(error_code(static_cast<int>(errc::invalid_argument), error_category::system()), ssl_socket{});
                return;
            }

            auto self = shared_from_this();
            if (cancel_slot != nullptr) {
                pending->async_handshake(*ctx, *cancel_slot, [self](error_code e) mutable { self->on_handshaken(e); });
            } else {
                pending->async_handshake(*ctx, [self](error_code e) mutable { self->on_handshaken(e); });
            }
        }

        void on_handshaken(error_code ec) {
            if (ec) {
                handler(ec, ssl_socket{});
                return;
            }
            handler(error_code{}, move(*pending));
        }
    };
} // namespace

void ssl_acceptor::async_accept(io_context& ctx, function<void(error_code, ssl_socket)> handler) {
    const auto op = make_shared<ssl_accept_op>();
    op->ctx = &ctx;
    op->acceptor = this;
    op->handler = move(handler);
    op->start();
}

void ssl_acceptor::async_accept(io_context& ctx, cancellation_slot& slot,
                                function<void(error_code, ssl_socket)> handler) {
    const auto op = make_shared<ssl_accept_op>();
    op->ctx = &ctx;
    op->acceptor = this;
    op->handler = move(handler);
    op->cancel_slot = &slot;
    op->start();
}

NEFORCE_END_NAMESPACE__
