#include <NeForce/network/socket/tcp_socket.hpp>
#ifdef NEFORCE_PLATFORM_LINUX
#include <fcntl.h>
#include <errno.h>
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
}


void tcp_socket::open(const int family) {
    if (family != AF_INET && family != AF_INET6) {
        throw_exception(value_exception("Invalid address family for TCP socket"));
    }

    close();

    fd_ = ::socket(family, SOCK_STREAM, IPPROTO_TCP);
    if (!is_open()) {
        throw_exception(socket_exception("Failed to create TCP socket"));
    }
}

void tcp_socket::connect(const ip_address& endpoint) {
    if (!is_open()) {
        throw_exception(value_exception("Socket is not open"));
    }

    if (!endpoint.is_valid()) {
        throw_exception(value_exception("Invalid endpoint"));
    }

    const int result = ::connect(fd_, endpoint.data(), endpoint.size());
    if (result < 0) {
        throw_exception(socket_exception("Failed to connect to remote endpoint"));
    }
}

bool tcp_socket::connect(const ip_address& endpoint, const milliseconds timeout, bool was_blocking) {
    if (!is_open()) {
        throw_exception(value_exception("Socket is not open"));
    }

    if (!endpoint.is_valid()) {
        throw_exception(value_exception("Invalid endpoint"));
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    unsigned long mode = 1;
    ::ioctlsocket(fd_, FIONBIO, &mode);
#else
    const int flags = ::fcntl(fd_, F_GETFL, 0);
    if (flags == -1) {
        throw_exception(socket_exception("Failed to get socket flags"));
    }

    if (flags & O_NONBLOCK) {
        was_blocking = false;
    } else {
        if (::fcntl(fd_, F_SETFL, flags | O_NONBLOCK) == -1) {
            throw_exception(socket_exception("Failed to set non-blocking mode"));
        }
    }
#endif

    int result = ::connect(fd_, endpoint.data(), endpoint.size());
    if (result == 0) {
        if (was_blocking) {
            NEFORCE_IGNORE set_nonblocking(false);
        }
        return true;
    }

    int error = socket_exception::last_error();
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (error != WSAEWOULDBLOCK) {
        if (was_blocking) {
            NEFORCE_IGNORE set_nonblocking(false);
        }
        throw_exception(socket_exception("Connection failed with unexpected error"));
    }
#else
    if (error != EINPROGRESS) {
        if (was_blocking) {
            NEFORCE_IGNORE set_nonblocking(false);
        }
        throw_exception(socket_exception("Connection failed with unexpected error"));
    }
#endif

    fd_set write_fds;
    FD_ZERO(&write_fds);
    FD_SET(fd_, &write_fds);

    timeval tv{};
    tv.tv_sec = static_cast<long>(timeout.count() / 1000);
    tv.tv_usec = static_cast<long>((timeout.count() % 1000) * 1000);

#ifdef NEFORCE_PLATFORM_WINDOWS
    result = ::select(0, nullptr, &write_fds, nullptr, &tv);
#else
    result = ::select(static_cast<int>(fd_ + 1), nullptr, &write_fds, nullptr, &tv);
#endif

    if (result < 0) {
        if (was_blocking) {
            NEFORCE_IGNORE set_nonblocking(false);
        }
        throw_exception(socket_exception("Select operation failed during connection"));
    }

    if (result == 0) {
        if (was_blocking) {
            NEFORCE_IGNORE set_nonblocking(false);
        }
        return false;
    }

    int optval = 0;
    ::socklen_t optlen = sizeof(optval);
    if (!get_option(SOL_SOCKET, SO_ERROR, &optval, &optlen) || optval != 0) {
        if (was_blocking) {
            NEFORCE_IGNORE set_nonblocking(false);
        }
        throw_exception(socket_exception(
            "Failed to get socket options or socket error occurred",
            socket_exception::static_type,
            optval ? optval : socket_exception::last_error())
        );
    }

    if (was_blocking) {
        NEFORCE_IGNORE set_nonblocking(false);
    }
    return true;
}

ssize_t tcp_socket::send(const memory_view<const char> data, const int flags) {
    if (!is_open()) {
        throw_exception(value_exception("Socket is not open"));
    }
    if (data.empty()) return 0;

    const ssize_t result = ::send(fd_, data.data(), data.size(), flags);
    if (result < 0) {
        throw_exception(socket_exception("Failed to send data"));
    }
    return result;
}

ssize_t tcp_socket::send(const memory_view<const char> data, const milliseconds timeout, const int flags) {
    if (!is_open()) {
        throw_exception(value_exception("Socket is not open"));
    }
    if (data.empty()) return 0;

    if (timeout.count() > 0) {
        if (!wait_for_write(fd_, timeout)) {
            throw_exception(socket_exception("Send timeout"));
        }
    }

    const ssize_t result = ::send(fd_, data.data(), data.size(), flags);
    if (result < 0) {
        throw_exception(socket_exception("Failed to send data"));
    }
    return result;
}

ssize_t tcp_socket::receive(memory_view<char> buffer, const int flags) {
    if (!is_open()) {
        throw_exception(value_exception("Socket is not open"));
    }
    if (buffer.empty()) return 0;

    const ssize_t result = ::recv(fd_, buffer.data(), buffer.size(), flags);
    if (result < 0) {
        throw_exception(socket_exception("Failed to receive data"));
    }
    return result;
}

void tcp_socket::send_all(memory_view<const char> data) {
    if (!is_open()) {
        throw_exception(value_exception("Socket is not open"));
    }

    size_t total_sent = 0;
    while (total_sent < data.size()) {
        const ssize_t sent = send(data.view(total_sent));
        if (sent <= 0) {
            throw_exception(socket_exception("Failed to send all data - connection may be closed"));
        }
        total_sent += sent;
    }
}

vector<char> tcp_socket::receive_all(const size_t expected_size) {
    if (!is_open()) {
        throw_exception(value_exception("Socket is not open"));
    }

    vector<char> buffer(expected_size);
    size_t total_received = 0;

    while (total_received < expected_size) {
        const ssize_t received = receive(memory_view<char>(
            buffer.data() + total_received,
            expected_size - total_received
        ));

        if (received == 0) {
            break;
        }
        if (received < 0) {
            throw_exception(socket_exception("Failed to receive all data"));
        }
        total_received += received;
    }

    buffer.resize(total_received);
    return buffer;
}

NEFORCE_END_NAMESPACE__
