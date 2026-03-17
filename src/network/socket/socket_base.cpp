#include <NeForce/network/socket/socket_base.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#include <NeForce/core/async/atomic.hpp>
#endif
#ifdef NEFORCE_PLATFORM_LINUX
#include <netinet/tcp.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

#ifdef NEFORCE_PLATFORM_WINDOWS
namespace {
    struct winsock_initializer {
        static atomic<int> ref_count;

        winsock_initializer() {
            const int prev = ref_count.fetch_add(1);
            if (prev == 0) {
                ::WSADATA wsa_data;
                if (::WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
                    ref_count.fetch_sub(1);
                    throw_exception(system_exception("WSAStartup failed"));
                }
            }
        }

        ~winsock_initializer() {
            const int prev = ref_count.fetch_sub(1);
            if (prev == 1) {
                ::WSACleanup();
            }
        }
    };

    atomic<int> winsock_initializer::ref_count{0};
}
#endif


int socket_exception::last_error() noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    return ::WSAGetLastError();
#else
    return errno;
#endif
}

bool socket_exception::is_would_block(int error) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    return error == WSAEWOULDBLOCK;
#else
    return error == EWOULDBLOCK;
#endif
}

socket_base::socket_base() {
#ifdef NEFORCE_PLATFORM_WINDOWS
    static winsock_initializer win_sock{};
#endif
}

socket_base& socket_base::operator =(socket_base&& other) noexcept {
    if (this != &other) {
        close();
        fd_ = exchange(other.fd_, invalid_handle);
    }
    return *this;
}

bool socket_base::close() noexcept {
    if (!is_open()) {
        return true;
    }

    bool success = false;
#ifdef NEFORCE_PLATFORM_WINDOWS
    success = (::closesocket(fd_) == 0);
#else
    success = (::close(fd_) == 0);
#endif

    fd_ = invalid_handle;
    return success;
}

bool socket_base::set_nonblocking(const bool enable) noexcept {
    if (!is_open()) {
        return false;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    unsigned long mode = enable ? 1 : 0;
    return ::ioctlsocket(fd_, FIONBIO, &mode) == 0;
#else
    int flags = ::fcntl(fd_, F_GETFL, 0);
    if (flags == -1) return false;

    if (enable) {
        flags |= O_NONBLOCK;
    } else {
        flags &= ~O_NONBLOCK;
    }
    return ::fcntl(fd_, F_SETFL, flags) == 0;
#endif
}

bool socket_base::shutdown_send() noexcept {
    if (!is_open()) {
        return false;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    return ::shutdown(fd_, SD_SEND) == 0;
#else
    return ::shutdown(fd_, SHUT_WR) == 0;
#endif
}

bool socket_base::shutdown_receive() noexcept {
    if (!is_open()) {
        return false;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    return ::shutdown(fd_, SD_RECEIVE) == 0;
#else
    return ::shutdown(fd_, SHUT_RD) == 0;
#endif
}

bool socket_base::shutdown_both() noexcept {
    if (!is_open()) {
        return false;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    return ::shutdown(fd_, SD_BOTH) == 0;
#else
    return ::shutdown(fd_, SHUT_RDWR) == 0;
#endif
}

bool socket_base::set_option(const int level, const int optname, const void* value, const ::socklen_t len) noexcept {
    return ::setsockopt(fd_, level, optname, static_cast<const char*>(value), len) == 0;
}

bool socket_base::get_option(const int level, const int optname, void* optval, ::socklen_t* optlen) const noexcept {
    if (!is_open()) {
        return false;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    return ::getsockopt(fd_, level, optname, static_cast<char*>(optval), optlen) == 0;
#else
    return ::getsockopt(fd_, level, optname, optval, optlen) == 0;
#endif
}

bool socket_base::set_reuse_address(const bool enable) noexcept {
    const int value = enable ? 1 : 0;
    return set_option(SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value));
}

bool socket_base::set_reuse_port(const bool enable) noexcept {
#ifdef SO_REUSEPORT
    const int value = enable ? 1 : 0;
    return set_option(SOL_SOCKET, SO_REUSEPORT, &value, sizeof(value));
#else
    return false;
#endif
}

bool socket_base::set_keep_alive(const bool enable) noexcept {
    const int value = enable ? 1 : 0;
    return set_option(SOL_SOCKET, SO_KEEPALIVE, &value, sizeof(value));
}

bool socket_base::set_tcp_nodelay(const bool enable) noexcept {
    const int value = enable ? 1 : 0;
    return set_option(IPPROTO_TCP, TCP_NODELAY, &value, sizeof(value));
}

bool socket_base::set_receive_buffer_size(const int size) noexcept {
    return set_option(SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
}

bool socket_base::set_send_buffer_size(const int size) noexcept {
    return set_option(SOL_SOCKET, SO_SNDBUF, &size, sizeof(size));
}

bool socket_base::set_send_timeout(const milliseconds timeout) noexcept {
    if (!is_open()) {
        return false;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    const ::DWORD ms = static_cast<::DWORD>(timeout.count());
    return set_option(SOL_SOCKET, SO_SNDTIMEO, &ms, sizeof(ms));
#else
    struct ::timeval tv;
    tv.tv_sec = timeout.count() / 1000;
    tv.tv_usec = (timeout.count() % 1000) * 1000;
    return set_option(SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
#endif
}

bool socket_base::set_receive_timeout(const milliseconds timeout) noexcept {
    if (!is_open()) {
        return false;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    const ::DWORD ms = static_cast<::DWORD>(timeout.count());
    return set_option(SOL_SOCKET, SO_RCVTIMEO, &ms, sizeof(ms));
#else
    struct ::timeval tv;
    tv.tv_sec = timeout.count() / 1000;
    tv.tv_usec = (timeout.count() % 1000) * 1000;
    return set_option(SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif
}

optional<ip_address> socket_base::local_endpoint() const {
    if (!is_open()) {
        return none;
    }

    ::sockaddr_storage storage{};
    ::socklen_t len = sizeof(storage);

    if (::getsockname(fd_, reinterpret_cast<::sockaddr*>(&storage), &len) != 0) {
        return none;
    }

    if (storage.ss_family == AF_INET) {
        return ip_address(*reinterpret_cast<::sockaddr_in*>(&storage));
    } else if (storage.ss_family == AF_INET6) {
        return ip_address(*reinterpret_cast<::sockaddr_in6*>(&storage));
    }

    return none;
}

optional<ip_address> socket_base::remote_endpoint() const {
    if (!is_open()) {
        return none;
    }

    ::sockaddr_storage storage{};
    ::socklen_t len = sizeof(storage);

    if (::getpeername(fd_, reinterpret_cast<::sockaddr*>(&storage), &len) != 0) {
        return none;
    }

    if (storage.ss_family == AF_INET) {
        return ip_address(*reinterpret_cast<::sockaddr_in*>(&storage));
    } else if (storage.ss_family == AF_INET6) {
        return ip_address(*reinterpret_cast<::sockaddr_in6*>(&storage));
    }

    return none;
}

NEFORCE_END_NAMESPACE__
