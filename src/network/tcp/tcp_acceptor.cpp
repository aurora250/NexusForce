#include <NeForce/network/tcp/tcp_acceptor.hpp>
NEFORCE_BEGIN_NAMESPACE__

void tcp_acceptor::open(const ip_address& endpoint, const int backlog) {
    if (!endpoint.is_valid()) {
        NEFORCE_THROW_EXCEPTION(value_exception("Invalid endpoint for TCP acceptor"));
    }

    open_ip(endpoint.address_family(), type::STREAM, protocol::TCP);

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

namespace {
    struct accept_op : enable_shared_from_this<accept_op> {
        io_context* ctx;
        tcp_acceptor* acceptor;
        function<void(error_code, tcp_socket)> handler;
        cancellation_slot* cancel_slot{nullptr};
        atomic<bool> fired_{false};

        void start() {
            if (cancel_slot != nullptr && cancel_slot->is_cancelled()) {
                handler(make_operation_aborted(), tcp_socket{});
                return;
            }

            ::sockaddr_storage client_storage{};
            ::socklen_t addrlen = sizeof(client_storage);
            const auto client_fd =
                    ::accept(acceptor->native_handle(), reinterpret_cast<::sockaddr*>(&client_storage), &addrlen);
            if (client_fd != tcp_acceptor::invalid_handle) {
                handler(error_code{}, tcp_socket(client_fd));
                return;
            }
            const int err = socket_exception::last_error();
            if (!socket_exception::is_would_block(err)) {
                handler(error_code(err, error_category::system()), tcp_socket{});
                return;
            }

            auto self = shared_from_this();
            const auto fd = acceptor->native_handle();
            if (cancel_slot != nullptr) {
                cancel_slot->assign([self]() mutable {
                    self->ctx->remove_fd(static_cast<int>(self->acceptor->native_handle()));
                    bool expected = false;
                    if (self->fired_.compare_exchange_strong(expected, true)) {
                        self->handler(make_operation_aborted(), tcp_socket{});
                    }
                });
            }
            ctx->add_fd(fd, epoll_in, [self](int, uint32_t, error_code ec) {
                bool expected = false;
                if (self->fired_.compare_exchange_strong(expected, true)) {
                    self->on_ready(ec);
                }
            });

            ::fd_set rfds;
            FD_ZERO(&rfds);
            FD_SET(fd, &rfds);
            ::timeval tv{};
            if (::select(static_cast<int>(fd) + 1, &rfds, nullptr, nullptr, &tv) > 0) {
                bool expected = false;
                if (fired_.compare_exchange_strong(expected, true)) {
                    ctx->remove_fd(static_cast<int>(fd));
                    on_ready(error_code{});
                }
            }
        }

        void on_ready(error_code ec) {
            if (ec) {
                handler(ec, tcp_socket{});
                return;
            }
            ::sockaddr_storage client_storage{};
            ::socklen_t addrlen = sizeof(client_storage);
            const auto client_fd =
                    ::accept(acceptor->native_handle(), reinterpret_cast<::sockaddr*>(&client_storage), &addrlen);
            if (client_fd != tcp_acceptor::invalid_handle) {
                handler(error_code{}, tcp_socket(client_fd));
            } else {
                handler(error_code{static_cast<int>(socket_exception::last_error()), error_category::system()},
                        tcp_socket{});
            }
        }
    };
} // namespace

void tcp_acceptor::async_accept(io_context& ctx, function<void(error_code, tcp_socket)> handler) {
    auto op = make_shared<accept_op>();
    op->ctx = &ctx;
    op->acceptor = this;
    op->handler = move(handler);
    op->start();
}

void tcp_acceptor::async_accept(io_context& ctx, cancellation_slot& slot,
                                function<void(error_code, tcp_socket)> handler) {
    auto op = make_shared<accept_op>();
    op->ctx = &ctx;
    op->acceptor = this;
    op->handler = move(handler);
    op->cancel_slot = &slot;
    op->start();
}

NEFORCE_END_NAMESPACE__
