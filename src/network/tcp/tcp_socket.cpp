#include <NeForce/network/tcp/tcp_socket.hpp>
#ifdef NEFORCE_PLATFORM_LINUX
#    include <cerrno>
#    include <fcntl.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

namespace {
    bool wait_for_write(tcp_socket::native_handle_type fd, const milliseconds timeout) {
        ::fd_set write_fds;
        FD_ZERO(&write_fds);
        FD_SET(fd, &write_fds);

        ::timeval tv{};
        tv.tv_sec = static_cast<long>(timeout.count() / 1000);
        tv.tv_usec = static_cast<long>((timeout.count() % 1000) * 1000);

#ifdef NEFORCE_PLATFORM_WINDOWS
        return ::select(0, nullptr, &write_fds, nullptr, &tv) > 0;
#else
        return ::select(static_cast<int>(fd + 1), nullptr, &write_fds, nullptr, &tv) > 0;
#endif
    }
} // namespace


void tcp_socket::open(const family f) { open_ip(f, type::STREAM, protocol::TCP); }

bool tcp_socket::connect(const ip_address& endpoint, const milliseconds timeout, bool was_blocking) {
    if (!is_open()) {
        NEFORCE_THROW_EXCEPTION(value_exception("Socket is not open"));
    }

    if (!endpoint.is_valid()) {
        NEFORCE_THROW_EXCEPTION(value_exception("Invalid endpoint"));
    }

#ifndef NEFORCE_PLATFORM_WINDOWS
    if (fd_ >= FD_SETSIZE) {
        NEFORCE_THROW_EXCEPTION(socket_exception("Socket fd exceeds FD_SETSIZE"));
    }
#endif

#ifdef NEFORCE_PLATFORM_WINDOWS
    unsigned long mode = 1;
    ::ioctlsocket(fd_, FIONBIO, &mode);
#else
    const int flags = ::fcntl(fd_, F_GETFL, 0);
    if (flags == -1) {
        NEFORCE_THROW_EXCEPTION(socket_exception("Failed to get socket flags"));
    }

    if ((flags & O_NONBLOCK) != 0) {
        was_blocking = false;
    } else {
        if (::fcntl(fd_, F_SETFL, flags | O_NONBLOCK) == -1) {
            NEFORCE_THROW_EXCEPTION(socket_exception("Failed to set non-blocking mode"));
        }
    }
#endif

    int result = ::connect(fd_, endpoint.data(), endpoint.size());
    if (result == 0) {
        if (was_blocking) {
            ignore = set_nonblocking(false);
        }
        return true;
    }

    int error = network_exception::last_error().value();
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (error != WSAEWOULDBLOCK) {
        if (was_blocking) {
            ignore = set_nonblocking(false);
        }
        NEFORCE_THROW_EXCEPTION(socket_exception("Connection failed with unexpected error"));
    }
#else
    if (error != EINPROGRESS) {
        if (was_blocking) {
            ignore = set_nonblocking(false);
        }
        NEFORCE_THROW_EXCEPTION(socket_exception("Connection failed with unexpected error"));
    }
#endif

    ::fd_set write_fds;
    FD_ZERO(&write_fds);
    FD_SET(fd_, &write_fds);

    ::timeval tv{};
    tv.tv_sec = static_cast<long>(timeout.count() / 1000);
    tv.tv_usec = static_cast<long>((timeout.count() % 1000) * 1000);

#ifdef NEFORCE_PLATFORM_WINDOWS
    result = ::select(0, nullptr, &write_fds, nullptr, &tv);
#else
    result = ::select(static_cast<int>(fd_ + 1), nullptr, &write_fds, nullptr, &tv);
#endif

    if (result < 0) {
        if (was_blocking) {
            ignore = set_nonblocking(false);
        }
        NEFORCE_THROW_EXCEPTION(socket_exception("Select operation failed during connection"));
    }

    if (result == 0) {
        if (was_blocking) {
            ignore = set_nonblocking(false);
        }
        return false;
    }

    int optval = 0;
    ::socklen_t optlen = sizeof(optval);
    if (!get_option(SOL_SOCKET, SO_ERROR, &optval, &optlen) || optval != 0) {
        if (was_blocking) {
            ignore = set_nonblocking(false);
        }
        const auto code = error_code{optval != 0 ? optval : network_exception::last_error().value(), system_category()};
        NEFORCE_THROW_EXCEPTION(socket_exception("Failed to get socket options or socket error occurred", code));
    }

    if (was_blocking) {
        ignore = set_nonblocking(false);
    }
    return true;
}

ssize_t tcp_socket::send(const memory_view<const char> data, const int flags) {
    if (!is_open()) {
        NEFORCE_THROW_EXCEPTION(value_exception("Socket is not open"));
    }
    if (data.empty()) {
        return 0;
    }

#ifdef NEFORCE_PLATFORM_LINUX
    const int send_flags = flags | MSG_NOSIGNAL;
#else
    const int send_flags = flags;
#endif

    const ssize_t result = ::send(fd_, data.data(), static_cast<int>(data.size()), send_flags);
    if (result < 0) {
        NEFORCE_THROW_EXCEPTION(socket_exception("Failed to send data"));
    }
    return result;
}

ssize_t tcp_socket::send(const memory_view<const char> data, const milliseconds timeout, const int flags) {
    if (!is_open()) {
        NEFORCE_THROW_EXCEPTION(value_exception("Socket is not open"));
    }
    if (data.empty()) {
        return 0;
    }

    if (timeout.count() > 0) {
        if (!wait_for_write(fd_, timeout)) {
            NEFORCE_THROW_EXCEPTION(socket_exception("Send timeout"));
        }
    }

#ifdef NEFORCE_PLATFORM_LINUX
    const int send_flags = flags | MSG_NOSIGNAL;
#else
    const int send_flags = flags;
#endif

    const ssize_t result = ::send(fd_, data.data(), static_cast<int>(data.size()), send_flags);
    if (result < 0) {
        NEFORCE_THROW_EXCEPTION(socket_exception("Failed to send data"));
    }
    return result;
}

ssize_t tcp_socket::receive(memory_view<char> buffer, const int flags) {
    if (!is_open()) {
        NEFORCE_THROW_EXCEPTION(value_exception("Socket is not open"));
    }
    if (buffer.empty()) {
        return 0;
    }

    const ssize_t result = ::recv(fd_, buffer.data(), static_cast<int>(buffer.size()), flags);
    if (result < 0) {
#ifdef NEFORCE_PLATFORM_WINDOWS
        const int err = ::WSAGetLastError();
        if (err == WSAEWOULDBLOCK) {
            return 0;
        }
        NEFORCE_THROW_EXCEPTION(socket_exception("Failed to receive data", error_code(err, system_category())));
#else
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        }
        NEFORCE_THROW_EXCEPTION(socket_exception("Failed to receive data"));
#endif
    }
    return result;
}

void tcp_socket::send_all(memory_view<const char> data) {
    if (!is_open()) {
        NEFORCE_THROW_EXCEPTION(value_exception("Socket is not open"));
    }

    size_t total_sent = 0;
    while (total_sent < data.size()) {
        const ssize_t sent = send(data.view(total_sent));
        if (sent <= 0) {
            NEFORCE_THROW_EXCEPTION(socket_exception("Failed to send all data - connection may be closed"));
        }
        total_sent += sent;
    }
}

vector<char> tcp_socket::receive_all(const size_t expected_size) {
    if (!is_open()) {
        NEFORCE_THROW_EXCEPTION(value_exception("Socket is not open"));
    }

    vector<char> buffer(expected_size);
    size_t total_received = 0;

    while (total_received < expected_size) {
        const ssize_t received =
                receive(memory_view<char>(buffer.data() + total_received, expected_size - total_received));

        if (received == 0) {
            break;
        }
        if (received < 0) {
            NEFORCE_THROW_EXCEPTION(socket_exception("Failed to receive all data"));
        }
        total_received += received;
    }

    buffer.resize(total_received);
    return buffer;
}

namespace {
    template <typename Handler>
    struct connect_op : enable_shared_from_this<connect_op<Handler>> {
        io_context* ctx;
        tcp_socket* sock;
        ip_address endpoint;
        Handler handler;
        bool was_blocking{true};
        cancellation_slot* cancel_slot{nullptr};
        atomic<bool> fired_{false};

        void start() {
            if (cancel_slot != nullptr && cancel_slot->is_cancelled()) {
                handler(make_operation_aborted());
                return;
            }

#ifdef NEFORCE_PLATFORM_WINDOWS
            unsigned long mode = 1;
            ::ioctlsocket(sock->native_handle(), FIONBIO, &mode);
#else
            const int flags = ::fcntl(static_cast<int>(sock->native_handle()), F_GETFL, 0);
            if (flags >= 0 && (flags & O_NONBLOCK) == 0) {
                was_blocking = true;
                ::fcntl(static_cast<int>(sock->native_handle()), F_SETFL, flags | O_NONBLOCK);
            }
#endif
            const int result = ::connect(sock->native_handle(), endpoint.data(), static_cast<int>(endpoint.size()));
            if (result == 0) {
                restore_blocking();
                handler(error_code{});
                return;
            }

            const int err = network_exception::last_error().value();
#ifdef NEFORCE_PLATFORM_WINDOWS
            if (err != WSAEWOULDBLOCK && err != WSAEALREADY && err != WSAEINPROGRESS) {
#else
            if (err != EINPROGRESS && err != EALREADY) {
#endif
                restore_blocking();
                handler(error_code(err, error_category::system()));
                return;
            }

            auto self = this->shared_from_this();
            if (cancel_slot != nullptr) {
                cancel_slot->assign([self]() mutable {
                    self->ctx->remove_fd(self->sock->native_handle());
                    bool expected = false;
                    if (self->fired_.compare_exchange_strong(expected, true)) {
                        self->restore_blocking();
                        self->handler(make_operation_aborted());
                    }
                });
            }
            ctx->add_fd(sock->native_handle(), epoll_out, [self](int, uint32_t, error_code ec) {
                bool expected = false;
                if (self->fired_.compare_exchange_strong(expected, true)) {
                    self->on_ready(ec);
                }
            });

            int optval = 0;
            ::socklen_t optlen = sizeof(optval);
            if (sock->get_option(SOL_SOCKET, SO_ERROR, &optval, &optlen)) {
                bool still_in_progress = false;
#ifdef NEFORCE_PLATFORM_WINDOWS
                still_in_progress = (optval == WSAEWOULDBLOCK || optval == WSAEINPROGRESS);
#else
                still_in_progress = (optval == EINPROGRESS);
#endif
                if (!still_in_progress) {
                    bool expected = false;
                    if (fired_.compare_exchange_strong(expected, true)) {
                        ctx->remove_fd(sock->native_handle());
                        restore_blocking();
                        handler(error_code(optval, error_category::system()));
                    }
                }
            }
        }

        void on_ready(error_code ec) {
            if (ec) {
                restore_blocking();
                handler(ec);
                return;
            }

            int optval = 0;
            ::socklen_t optlen = sizeof(optval);
            if (sock->get_option(SOL_SOCKET, SO_ERROR, &optval, &optlen) && optval == 0) {
                restore_blocking();
                handler(error_code{});
            } else {
                restore_blocking();
                handler(error_code(optval != 0 ? optval : network_exception::last_error().value(),
                                   error_category::system()));
            }
        }

        void restore_blocking() {
            if (was_blocking) {
                sock->set_nonblocking(false);
            }
        }
    };
} // namespace

void tcp_socket::async_connect(io_context& ctx, const ip_address& endpoint, function<void(error_code)> handler) {
    auto op = make_shared<connect_op<function<void(error_code)>>>();
    op->ctx = &ctx;
    op->sock = this;
    op->endpoint = endpoint;
    op->handler = move(handler);
    op->start();
}

void tcp_socket::async_connect(io_context& ctx, const ip_address& endpoint, cancellation_slot& slot,
                               function<void(error_code)> handler) {
    auto op = make_shared<connect_op<function<void(error_code)>>>();
    op->ctx = &ctx;
    op->sock = this;
    op->endpoint = endpoint;
    op->handler = move(handler);
    op->cancel_slot = &slot;
    op->start();
}

namespace {
    template <typename Handler>
    struct read_op : enable_shared_from_this<read_op<Handler>> {
        io_context* ctx;
        tcp_socket* sock;
        memory_view<char> buffer;
        Handler handler;
        cancellation_slot* cancel_slot{nullptr};
        atomic<bool> fired_{false};

        void start() {
            if (cancel_slot != nullptr && cancel_slot->is_cancelled()) {
                handler(make_operation_aborted(), 0);
                return;
            }

            const auto fd = sock->native_handle();
            const ssize_t result = ::recv(fd, buffer.data(), static_cast<int>(buffer.size()), 0);
            if (result > 0) {
                handler(error_code{}, static_cast<size_t>(result));
                return;
            }
            if (result == 0) {
                handler(error_code{make_error_code(errc::connection_reset)}, 0);
                return;
            }
#ifdef NEFORCE_PLATFORM_WINDOWS
            const int err = ::WSAGetLastError();
            if (err != WSAEWOULDBLOCK) {
#else
            const int err = errno;
            if (err != EAGAIN && err != EWOULDBLOCK) {
#endif
                handler(error_code(err, error_category::system()), 0);
                return;
            }

            auto self = this->shared_from_this();
            if (cancel_slot != nullptr) {
                cancel_slot->assign([self]() mutable {
                    self->ctx->remove_fd(self->sock->native_handle());
                    bool expected = false;
                    if (self->fired_.compare_exchange_strong(expected, true)) {
                        self->handler(make_operation_aborted(), 0);
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
                handler(ec, 0);
                return;
            }
            const ssize_t result = ::recv(sock->native_handle(), buffer.data(), static_cast<int>(buffer.size()), 0);
            if (result > 0) {
                handler(error_code{}, static_cast<size_t>(result));
            } else if (result == 0) {
                handler(error_code{make_error_code(errc::connection_reset)}, 0);
            } else {
                handler(error_code{static_cast<int>(network_exception::last_error().value()), error_category::system()},
                        0);
            }
        }
    };
} // namespace

void tcp_socket::async_read(io_context& ctx, memory_view<char> buffer, function<void(error_code, size_t)> handler) {
    auto op = make_shared<read_op<function<void(error_code, size_t)>>>();
    op->ctx = &ctx;
    op->sock = this;
    op->buffer = buffer;
    op->handler = move(handler);
    op->start();
}

void tcp_socket::async_read(io_context& ctx, memory_view<char> buffer, cancellation_slot& slot,
                            function<void(error_code, size_t)> handler) {
    auto op = make_shared<read_op<function<void(error_code, size_t)>>>();
    op->ctx = &ctx;
    op->sock = this;
    op->buffer = buffer;
    op->handler = move(handler);
    op->cancel_slot = &slot;
    op->start();
}

namespace {
    template <typename Handler>
    struct write_op : enable_shared_from_this<write_op<Handler>> {
        io_context* ctx;
        tcp_socket* sock;
        memory_view<const char> buffer;
        Handler handler;
        cancellation_slot* cancel_slot{nullptr};
        atomic<bool> fired_{false};

        void start() {
            if (cancel_slot != nullptr && cancel_slot->is_cancelled()) {
                handler(make_operation_aborted(), 0);
                return;
            }

            const auto fd = sock->native_handle();
#ifdef NEFORCE_PLATFORM_LINUX
            constexpr int send_flags = MSG_NOSIGNAL;
#else
            constexpr int send_flags = 0;
#endif
            const ssize_t result = ::send(fd, buffer.data(), static_cast<int>(buffer.size()), send_flags);
            if (result > 0) {
                handler(error_code{}, static_cast<size_t>(result));
                return;
            }
#ifdef NEFORCE_PLATFORM_WINDOWS
            const int err = ::WSAGetLastError();
            if (err != WSAEWOULDBLOCK) {
#else
            const int err = errno;
            if (err != EAGAIN && err != EWOULDBLOCK) {
#endif
                handler(error_code(err, error_category::system()), 0);
                return;
            }

            auto self = this->shared_from_this();
            if (cancel_slot != nullptr) {
                cancel_slot->assign([self]() mutable {
                    self->ctx->remove_fd(self->sock->native_handle());
                    bool expected = false;
                    if (self->fired_.compare_exchange_strong(expected, true)) {
                        self->handler(make_operation_aborted(), 0);
                    }
                });
            }
            ctx->add_fd(fd, epoll_out, [self](int, uint32_t, error_code ec) {
                bool expected = false;
                if (self->fired_.compare_exchange_strong(expected, true)) {
                    self->on_ready(ec);
                }
            });

            ::fd_set wfds;
            FD_ZERO(&wfds);
            FD_SET(fd, &wfds);
            ::timeval tv{};
            if (::select(static_cast<int>(fd) + 1, nullptr, &wfds, nullptr, &tv) > 0) {
                bool expected = false;
                if (fired_.compare_exchange_strong(expected, true)) {
                    ctx->remove_fd(static_cast<int>(fd));
                    on_ready(error_code{});
                }
            }
        }

        void on_ready(error_code ec) {
            if (ec) {
                handler(ec, 0);
                return;
            }
#ifdef NEFORCE_PLATFORM_LINUX
            constexpr int send_flags = MSG_NOSIGNAL;
#else
            constexpr int send_flags = 0;
#endif
            const ssize_t result =
                    ::send(sock->native_handle(), buffer.data(), static_cast<int>(buffer.size()), send_flags);
            if (result > 0) {
                handler(error_code{}, static_cast<size_t>(result));
            } else {
                handler(error_code{static_cast<int>(network_exception::last_error().value()), error_category::system()},
                        0);
            }
        }
    };
} // namespace


void tcp_socket::async_write(io_context& ctx, memory_view<const char> buffer,
                             function<void(error_code, size_t)> handler) {
    auto op = make_shared<write_op<function<void(error_code, size_t)>>>();
    op->ctx = &ctx;
    op->sock = this;
    op->buffer = buffer;
    op->handler = move(handler);
    op->start();
}

void tcp_socket::async_write(io_context& ctx, memory_view<const char> buffer, cancellation_slot& slot,
                             function<void(error_code, size_t)> handler) {
    auto op = make_shared<write_op<function<void(error_code, size_t)>>>();
    op->ctx = &ctx;
    op->sock = this;
    op->buffer = buffer;
    op->handler = move(handler);
    op->cancel_slot = &slot;
    op->start();
}

NEFORCE_END_NAMESPACE__
