#include <NeForce/network/socket_base.hpp>
#ifdef NEFORCE_PLATFORM_LINUX
#    include <cerrno>
#    include <fcntl.h>
#    include <netinet/tcp.h>
#    include <unistd.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

#ifdef NEFORCE_PLATFORM_WINDOWS
namespace {
    // NOLINTNEXTLINE(cppcoreguidelines-special-member-functions,hicpp-special-member-functions)
    struct winsock_guard {
        winsock_guard() {
            ::WSADATA wsa_data;
            if (::WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
                NEFORCE_THROW_EXCEPTION(system_exception("WSAStartup failed"));
            }
        }
        ~winsock_guard() { ::WSACleanup(); }
        winsock_guard(const winsock_guard&) = delete;
        winsock_guard& operator=(const winsock_guard&) = delete;
    };

    void ensure_winsock() { static winsock_guard guard{}; }
} // namespace
#endif

#ifndef NEFORCE_STANDARD_17
constexpr socket_base::native_handle_type socket_base::invalid_handle;
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
    ensure_winsock();
#endif
}

socket_base& socket_base::operator=(socket_base&& other) noexcept {
    if (addressof(other) == this) {
        return *this;
    }
    close();
    fd_ = exchange(other.fd_, invalid_handle);
    nonblocking_ = exchange(other.nonblocking_, false);
    return *this;
}

void socket_base::open(const family f, const type t, const protocol p) {
    if (f != family::INET4 && f != family::INET6) {
        NEFORCE_THROW_EXCEPTION(value_exception("Invalid address family for socket"));
    }

    close();

    fd_ = ::socket(static_cast<int>(f), static_cast<int>(t), static_cast<int>(p));
    if (!is_open()) {
        NEFORCE_THROW_EXCEPTION(socket_exception("Failed to create socket"));
    }
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
    nonblocking_ = false;
    return success;
}

bool socket_base::try_open(const family f, const type t, const protocol p) noexcept {
    if (f != family::INET4 && f != family::INET6) {
        return false;
    }

    close();
    fd_ = ::socket(static_cast<int>(f), static_cast<int>(t), static_cast<int>(p));
    return is_open();
}

bool socket_base::set_nonblocking(const bool enable) noexcept {
    if (!is_open()) {
        return false;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    ::u_long mode = enable ? 1 : 0;
    if (::ioctlsocket(fd_, FIONBIO, &mode) != 0) {
        return false;
    }
#else
    int flags = ::fcntl(fd_, F_GETFL, 0);
    if (flags == -1) {
        return false;
    }

    if (enable) {
        flags |= O_NONBLOCK;
    } else {
        flags &= ~O_NONBLOCK;
    }
    if (::fcntl(fd_, F_SETFL, flags) == -1) {
        return false;
    }
#endif
    nonblocking_ = enable;
    return true;
}

bool socket_base::is_nonblocking() const noexcept {
    if (!is_open()) {
        return false;
    }
#ifdef NEFORCE_PLATFORM_WINDOWS
    return nonblocking_;
#else
    const int flags = ::fcntl(fd_, F_GETFL, 0);
    return (flags != -1) && (flags & O_NONBLOCK);
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
    if (!is_open()) {
        return false;
    }
    const int value = enable ? 1 : 0;
    return set_option(SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value));
}

bool socket_base::get_reuse_address() const noexcept {
    if (!is_open()) {
        return false;
    }
    int value = 0;
    ::socklen_t optlen = sizeof(value);
    if (!get_option(SOL_SOCKET, SO_REUSEADDR, &value, &optlen)) {
        return false;
    }
    return value != 0;
}

bool socket_base::set_reuse_port(const bool enable) noexcept {
    if (!is_open()) {
        return false;
    }
#ifdef SO_REUSEPORT
    const int value = enable ? 1 : 0;
    return set_option(SOL_SOCKET, SO_REUSEPORT, &value, sizeof(value));
#else
    ignore = enable;
    return false;
#endif
}

bool socket_base::get_reuse_port() const noexcept {
    if (!is_open()) {
        return false;
    }
#ifdef SO_REUSEPORT
    int value = 0;
    ::socklen_t optlen = sizeof(value);
    if (!get_option(SOL_SOCKET, SO_REUSEPORT, &value, &optlen)) {
        return false;
    }
    return value != 0;
#else
    return false;
#endif
}

bool socket_base::set_keep_alive(const bool enable) noexcept {
    if (!is_open()) {
        return false;
    }
    const ::socklen_t value = enable ? 1 : 0;
    return set_option(SOL_SOCKET, SO_KEEPALIVE, &value, sizeof(value));
}

bool socket_base::get_keep_alive() const noexcept {
    if (!is_open()) {
        return false;
    }
    int value = 0;
    ::socklen_t optlen = sizeof(value);
    if (!get_option(SOL_SOCKET, SO_KEEPALIVE, &value, &optlen)) {
        return false;
    }
    return value != 0;
}

bool socket_base::set_tcp_nodelay(const bool enable) noexcept {
    if (!is_open()) {
        return false;
    }
    const ::socklen_t value = enable ? 1 : 0;
    return set_option(IPPROTO_TCP, TCP_NODELAY, &value, sizeof(value));
}

bool socket_base::get_tcp_nodelay() const noexcept {
    if (!is_open()) {
        return false;
    }
    int value = 0;
    ::socklen_t optlen = sizeof(value);
    if (!get_option(IPPROTO_TCP, TCP_NODELAY, &value, &optlen)) {
        return false;
    }
    return value != 0;
}

bool socket_base::set_receive_buffer_size(const int size) noexcept {
    if (!is_open()) {
        return false;
    }
    return set_option(SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
}

int socket_base::get_receive_buffer_size() const noexcept {
    if (!is_open()) {
        return -1;
    }
    int value = 0;
    ::socklen_t optlen = sizeof(value);
    if (!get_option(SOL_SOCKET, SO_RCVBUF, &value, &optlen)) {
        return -1;
    }
    return value;
}

bool socket_base::set_send_buffer_size(const int size) noexcept {
    if (!is_open()) {
        return false;
    }
    return set_option(SOL_SOCKET, SO_SNDBUF, &size, sizeof(size));
}

int socket_base::get_send_buffer_size() const noexcept {
    if (!is_open()) {
        return -1;
    }
    int value = 0;
    ::socklen_t optlen = sizeof(value);
    if (!get_option(SOL_SOCKET, SO_SNDBUF, &value, &optlen)) {
        return -1;
    }
    return value;
}

optional<milliseconds> socket_base::get_send_timeout() const noexcept {
    if (!is_open()) {
        return none;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    ::DWORD ms = 0;
    int optlen = sizeof(ms);
    if (!get_option(SOL_SOCKET, SO_SNDTIMEO, &ms, &optlen)) {
        return none;
    }
    return milliseconds(ms);
#else
    ::timeval tv = {};
    ::socklen_t optlen = sizeof(tv);
    if (!get_option(SOL_SOCKET, SO_SNDTIMEO, &tv, &optlen)) {
        return none;
    }
    return milliseconds(tv.tv_sec * 1000 + tv.tv_usec / 1000);
#endif
}

bool socket_base::set_send_timeout(const milliseconds timeout) noexcept {
    if (!is_open()) {
        return false;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    const auto ms = static_cast<::DWORD>(timeout.count());
    return set_option(SOL_SOCKET, SO_SNDTIMEO, &ms, sizeof(ms));
#else
    ::timeval tv;
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
    const auto ms = static_cast<::DWORD>(timeout.count());
    return set_option(SOL_SOCKET, SO_RCVTIMEO, &ms, sizeof(ms));
#else
    ::timeval tv;
    tv.tv_sec = timeout.count() / 1000;
    tv.tv_usec = (timeout.count() % 1000) * 1000;
    return set_option(SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif
}

optional<milliseconds> socket_base::get_receive_timeout() const noexcept {
    if (!is_open()) {
        return none;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    ::DWORD ms = 0;
    int optlen = sizeof(ms);
    if (!get_option(SOL_SOCKET, SO_RCVTIMEO, &ms, &optlen)) {
        return none;
    }
    return milliseconds(ms);
#else
    ::timeval tv = {};
    ::socklen_t optlen = sizeof(tv);
    if (!get_option(SOL_SOCKET, SO_RCVTIMEO, &tv, &optlen)) {
        return none;
    }
    return milliseconds(tv.tv_sec * 1000 + tv.tv_usec / 1000);
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
    }
    if (storage.ss_family == AF_INET6) {
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
    }
    if (storage.ss_family == AF_INET6) {
        return ip_address(*reinterpret_cast<::sockaddr_in6*>(&storage));
    }

    return none;
}

void socket_base::bind(const ip_address& endpoint) {
    if (!is_open()) {
        NEFORCE_THROW_EXCEPTION(value_exception("Socket is not open"));
    }

    if (!endpoint.is_valid()) {
        NEFORCE_THROW_EXCEPTION(value_exception("Invalid endpoint for UDP bind"));
    }

    if (::bind(fd_, endpoint.data(), endpoint.size()) < 0) {
        NEFORCE_THROW_EXCEPTION(socket_exception("Failed to bind UDP socket to endpoint"));
    }
}

void socket_base::listen(const int backlog) {
    if (!is_open()) {
        NEFORCE_THROW_EXCEPTION(value_exception("Socket is not open"));
    }

    if (::listen(fd_, backlog) < 0) {
        NEFORCE_THROW_EXCEPTION(socket_exception("PORT: listen failed"));
    }
}

NEFORCE_END_NAMESPACE__
