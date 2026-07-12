#include <NeForce/network/udp_socket.hpp>
#include <NeForce/core/memory/shared_ptr.hpp>
NEFORCE_BEGIN_NAMESPACE__

void udp_socket::open(const family f) { open_ip(f, type::DGRAM, protocol::UDP); }

ssize_t udp_socket::send_to(memory_view<const char> data, const ip_address& endpoint, const int flags) {
    if (!is_open()) {
        NEFORCE_THROW_EXCEPTION(value_exception("Socket is not open"));
    }

    if (!endpoint.is_valid()) {
        NEFORCE_THROW_EXCEPTION(value_exception("Invalid destination endpoint for UDP send"));
    }
    if (data.empty()) {
        return 0;
    }

    const ssize_t result =
            ::sendto(fd_, data.data(), static_cast<int>(data.size()), flags, endpoint.data(), endpoint.size());

    if (result < 0) {
#ifdef NEFORCE_PLATFORM_WINDOWS
        const int err = ::WSAGetLastError();
        if (err == WSAEWOULDBLOCK) {
            return 0;
        }
        NEFORCE_THROW_EXCEPTION(socket_exception("Failed to send UDP datagram to specified endpoint",
                                                 error_code(err, system_category())));
#else
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        }
        NEFORCE_THROW_EXCEPTION(socket_exception("Failed to send UDP datagram to specified endpoint"));
#endif
    }
    return result;
}

ssize_t udp_socket::send(memory_view<const char> data, const int flags) {
    if (!is_open()) {
        NEFORCE_THROW_EXCEPTION(value_exception("Invalid socket fd for UDP send"));
    }

    const ssize_t result = ::send(fd_, data.data(), static_cast<int>(data.size()), flags);
    if (result < 0) {
#ifdef NEFORCE_PLATFORM_WINDOWS
        const int err = ::WSAGetLastError();
        if (err == WSAEWOULDBLOCK) {
            return 0;
        }
        NEFORCE_THROW_EXCEPTION(socket_exception("Failed to send UDP datagram to connected endpoint",
                                                 error_code(err, system_category())));
#else
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        }
        NEFORCE_THROW_EXCEPTION(socket_exception("Failed to send UDP datagram to connected endpoint"));
#endif
    }
    return result;
}

pair<ssize_t, ip_address> udp_socket::receive_from(memory_view<char> buffer, const int flags) {
    if (!is_open()) {
        NEFORCE_THROW_EXCEPTION(value_exception("Invalid socket fd for UDP receive"));
    }

    if (buffer.empty()) {
        NEFORCE_THROW_EXCEPTION(value_exception("Receive buffer cannot be empty"));
    }

    ::sockaddr_storage addr_storage;
    ::socklen_t addrlen = sizeof(addr_storage);

    ssize_t result = ::recvfrom(fd_, buffer.data(), static_cast<int>(buffer.size()), flags,
                                reinterpret_cast<struct sockaddr*>(&addr_storage), &addrlen);

    if (result < 0) {
#ifdef NEFORCE_PLATFORM_WINDOWS
        const int err = ::WSAGetLastError();
        if (err == WSAEWOULDBLOCK) {
            return {0, ip_address{}};
        }
        NEFORCE_THROW_EXCEPTION(socket_exception("Failed to receive UDP datagram", error_code(err, system_category())));
#else
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return {0, ip_address{}};
        }
        NEFORCE_THROW_EXCEPTION(socket_exception("Failed to receive UDP datagram"));
#endif
    }

    ip_address sender;
    if (addr_storage.ss_family == AF_INET) {
        sender = ip_address(*reinterpret_cast<const sockaddr_in*>(&addr_storage));
    } else if (addr_storage.ss_family == AF_INET6) {
        sender = ip_address(*reinterpret_cast<const sockaddr_in6*>(&addr_storage));
    } else {
        NEFORCE_THROW_EXCEPTION(socket_exception("Unsupport socket type"));
    }

    return {result, move(sender)};
}

ssize_t udp_socket::receive(memory_view<char> buffer, const int flags) {
    if (!is_open()) {
        NEFORCE_THROW_EXCEPTION(value_exception("Invalid socket fd for UDP receive"));
    }

    if (buffer.empty()) {
        NEFORCE_THROW_EXCEPTION(value_exception("Receive buffer cannot be empty"));
    }

    const ssize_t result = ::recv(fd_, buffer.data(), static_cast<int>(buffer.size()), flags);
    if (result < 0) {
#ifdef NEFORCE_PLATFORM_WINDOWS
        const int err = ::WSAGetLastError();
        if (err == WSAEWOULDBLOCK) {
            return 0;
        }
        NEFORCE_THROW_EXCEPTION(socket_exception("Failed to receive UDP datagram from connected endpoint",
                                                 error_code(err, system_category())));
#else
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        }
        NEFORCE_THROW_EXCEPTION(socket_exception("Failed to receive UDP datagram from connected endpoint"));
#endif
    }
    return result;
}

namespace {
    struct udp_recv_op : enable_shared_from_this<udp_recv_op> {
        io_context* ctx;
        udp_socket* sock;
        memory_view<char> buffer;
        function<void(error_code, size_t, ip_address)> handler;
        cancellation_slot* cancel_slot{nullptr};
        atomic<bool> fired_{false};

        void start() {
            if (cancel_slot != nullptr && cancel_slot->is_cancelled()) {
                handler(make_operation_aborted(), 0, ip_address{});
                return;
            }

            const auto fd = sock->native_handle();
            ::sockaddr_storage addr_storage{};
            ::socklen_t addrlen = sizeof(addr_storage);
            const ssize_t result = ::recvfrom(fd, buffer.data(), static_cast<int>(buffer.size()), 0,
                                              reinterpret_cast<struct sockaddr*>(&addr_storage), &addrlen);
            if (result > 0) {
                ip_address sender;
                if (addr_storage.ss_family == AF_INET) {
                    sender = ip_address(*reinterpret_cast<const sockaddr_in*>(&addr_storage));
                } else if (addr_storage.ss_family == AF_INET6) {
                    sender = ip_address(*reinterpret_cast<const sockaddr_in6*>(&addr_storage));
                }
                handler(error_code{}, static_cast<size_t>(result), move(sender));
                return;
            }
            const int err = socket_exception::last_error();
            if (!socket_exception::is_would_block(err)) {
                handler(error_code(err, error_category::system()), 0, ip_address{});
                return;
            }
            auto self = shared_from_this();
            if (cancel_slot != nullptr) {
                cancel_slot->assign([self]() mutable {
                    self->ctx->remove_fd(static_cast<int>(self->sock->native_handle()));
                    bool expected = false;
                    if (self->fired_.compare_exchange_strong(expected, true)) {
                        self->handler(make_operation_aborted(), 0, ip_address{});
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
                handler(ec, 0, ip_address{});
                return;
            }
            fired_.store(false, memory_order_release);
            start();
        }
    };
} // namespace

void udp_socket::async_receive_from(io_context& ctx, memory_view<char> buffer,
                                    function<void(error_code, size_t, ip_address)> handler) {
    auto op = make_shared<udp_recv_op>();
    op->ctx = &ctx;
    op->sock = this;
    op->buffer = buffer;
    op->handler = move(handler);
    op->start();
}

void udp_socket::async_receive_from(io_context& ctx, memory_view<char> buffer, cancellation_slot& slot,
                                    function<void(error_code, size_t, ip_address)> handler) {
    auto op = make_shared<udp_recv_op>();
    op->ctx = &ctx;
    op->sock = this;
    op->buffer = buffer;
    op->handler = move(handler);
    op->cancel_slot = &slot;
    op->start();
}

void udp_socket::async_send_to(io_context& ctx, memory_view<const char> data, const ip_address& endpoint,
                               function<void(error_code, size_t)> handler) {
    const ssize_t result =
            ::sendto(native_handle(), data.data(), static_cast<int>(data.size()), 0, endpoint.data(), endpoint.size());
    if (result > 0) {
        handler(error_code{}, static_cast<size_t>(result));
        return;
    }
    const int err = socket_exception::last_error();
    if (!socket_exception::is_would_block(err)) {
        handler(error_code(err, error_category::system()), 0);
        return;
    }

    auto handler_ptr = make_shared<function<void(error_code, size_t)>>(move(handler));
    auto data_copy = make_shared<string>(data.data(), data.size());
    auto ep_copy = make_shared<ip_address>(endpoint);
    auto fired = make_shared<atomic<bool>>(false);
    native_handle_type fd = native_handle();
    ctx.add_fd(fd, epoll_out, [handler_ptr, data_copy, ep_copy, fd, fired](int, uint32_t, error_code ec) {
        bool expected = false;
        if (!fired->compare_exchange_strong(expected, true)) {
            return;
        }
        if (ec) {
            (*handler_ptr)(ec, 0);
            return;
        }
        const ssize_t r = ::sendto(fd, data_copy->data(), static_cast<int>(data_copy->size()), 0, ep_copy->data(),
                                   ep_copy->size());
        if (r > 0) {
            (*handler_ptr)(error_code{}, static_cast<size_t>(r));
        } else {
            (*handler_ptr)(error_code{socket_exception::last_error(), error_category::system()}, 0);
        }
    });

    ::fd_set wfds;
    FD_ZERO(&wfds);
    FD_SET(fd, &wfds);
    ::timeval tv{};
    if (::select(static_cast<int>(fd) + 1, nullptr, &wfds, nullptr, &tv) > 0) {
        bool expected = false;
        if (fired->compare_exchange_strong(expected, true)) {
            ctx.remove_fd(static_cast<int>(fd));
            const ssize_t r = ::sendto(fd, data_copy->data(), static_cast<int>(data_copy->size()), 0, ep_copy->data(),
                                       ep_copy->size());
            if (r > 0) {
                (*handler_ptr)(error_code{}, static_cast<size_t>(r));
            } else {
                (*handler_ptr)(error_code{socket_exception::last_error(), error_category::system()}, 0);
            }
        }
    }
}

void udp_socket::async_send_to(io_context& ctx, memory_view<const char> data, const ip_address& endpoint,
                               cancellation_slot& slot, function<void(error_code, size_t)> handler) {
    if (slot.is_cancelled()) {
        handler(make_operation_aborted(), 0);
        return;
    }

    const ssize_t result =
            ::sendto(native_handle(), data.data(), static_cast<int>(data.size()), 0, endpoint.data(), endpoint.size());
    if (result > 0) {
        handler(error_code{}, static_cast<size_t>(result));
        return;
    }
    const int err = socket_exception::last_error();
    if (!socket_exception::is_would_block(err)) {
        handler(error_code(err, error_category::system()), 0);
        return;
    }

    auto handler_ptr = make_shared<function<void(error_code, size_t)>>(move(handler));
    auto data_copy = make_shared<string>(data.data(), data.size());
    auto ep_copy = make_shared<ip_address>(endpoint);
    auto fired = make_shared<atomic<bool>>(false);
    native_handle_type fd = native_handle();

    slot.assign([handler_ptr, fired]() mutable {
        bool expected = false;
        if (fired->compare_exchange_strong(expected, true)) {
            (*handler_ptr)(make_operation_aborted(), 0);
        }
    });

    ctx.add_fd(fd, epoll_out, [handler_ptr, data_copy, ep_copy, fd, fired](int, uint32_t, error_code ec) {
        bool expected = false;
        if (!fired->compare_exchange_strong(expected, true)) {
            return;
        }
        if (ec) {
            (*handler_ptr)(ec, 0);
            return;
        }
        const ssize_t r = ::sendto(fd, data_copy->data(), static_cast<int>(data_copy->size()), 0, ep_copy->data(),
                                   ep_copy->size());
        if (r > 0) {
            (*handler_ptr)(error_code{}, static_cast<size_t>(r));
        } else {
            (*handler_ptr)(error_code{socket_exception::last_error(), error_category::system()}, 0);
        }
    });

    ::fd_set wfds;
    FD_ZERO(&wfds);
    FD_SET(fd, &wfds);
    ::timeval tv{};
    if (::select(static_cast<int>(fd) + 1, nullptr, &wfds, nullptr, &tv) > 0) {
        bool expected = false;
        if (fired->compare_exchange_strong(expected, true)) {
            ctx.remove_fd(static_cast<int>(fd));
            const ssize_t r = ::sendto(fd, data_copy->data(), static_cast<int>(data_copy->size()), 0, ep_copy->data(),
                                       ep_copy->size());
            if (r > 0) {
                (*handler_ptr)(error_code{}, static_cast<size_t>(r));
            } else {
                (*handler_ptr)(error_code{socket_exception::last_error(), error_category::system()}, 0);
            }
        }
    }
}

NEFORCE_END_NAMESPACE__
