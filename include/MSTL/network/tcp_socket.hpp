#ifndef MSTL_NETWORK_SOCKET_HPP__
#define MSTL_NETWORK_SOCKET_HPP__
#include "MSTL/core/time/duration.hpp"
#ifdef MSTL_PLATFORM_WINDOWS__
#include <WinSock2.h>
#include <ws2tcpip.h>
#include "MSTL/core/config/undef_cmacro.hpp"
#elif defined(MSTL_PLATFORM_LINUX__)
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif
MSTL_BEGIN_NAMESPACE__

enum class SOCKET_DOMAIN : uint16_t {
    IPV4 = AF_INET, IPV6 = AF_INET6,
    UNIX = AF_UNIX,
#ifdef MSTL_PLATFORM_LINUX__
    LOCAL = AF_LOCAL,
    NETLINK = AF_NETLINK, BLUETOOTH = AF_BLUETOOTH
#endif
};

enum class SOCKET_TYPE : uint16_t {
    STREAM = SOCK_STREAM, DATAGRAM = SOCK_DGRAM,
    RAW = SOCK_RAW, SEQPACKET = SOCK_SEQPACKET
};

enum class SOCKET_PROTOCOL : uint16_t {
    TCP = IPPROTO_TCP, UDP = IPPROTO_UDP,
    IPV4 = IPPROTO_IP, IPV6 = IPPROTO_IPV6,
    ICMP = IPPROTO_ICMP, ICMP6 = IPPROTO_ICMPV6,
    SCTP = IPPROTO_SCTP, AUTO = 0
};


#ifdef MSTL_PLATFORM_WINDOWS__
bool MSTL_API winsock_initialized();
#endif


class tcp_socket {
public:
#ifdef MSTL_PLATFORM_WINDOWS__
    using socket_t = ::SOCKET;
    static constexpr socket_t INVALID_MARK = INVALID_SOCKET;
#elif MSTL_PLATFORM_LINUX__
    using socket_t = int;
    static constexpr socket_t INVALID_MARK = -1;
#endif

private:
    socket_t sockfd_ = INVALID_MARK;

public:
    tcp_socket() noexcept = default;
    explicit tcp_socket(const socket_t fd) : sockfd_(fd) {}

    explicit tcp_socket(const SOCKET_DOMAIN domain,
        const SOCKET_TYPE type = SOCKET_TYPE::STREAM,
        const SOCKET_PROTOCOL protocol = SOCKET_PROTOCOL::AUTO) noexcept {
        this->open(domain, type, protocol);
    }

    tcp_socket(const tcp_socket&) = delete;
    tcp_socket& operator =(const tcp_socket&) = delete;

    tcp_socket(tcp_socket&& other) noexcept : sockfd_(other.sockfd_) {
        other.sockfd_ = INVALID_MARK;
    }
    tcp_socket& operator =(tcp_socket&& other) noexcept {
        if (this != &other) {
            close();
            sockfd_ = other.sockfd_;
            other.sockfd_ = INVALID_MARK;
        }
        return *this;
    }

    ~tcp_socket() {
        close();
    }

    MSTL_NODISCARD const socket_t& sockfd() const noexcept { return sockfd_; }
    MSTL_NODISCARD bool is_valid() const noexcept { return sockfd_ != INVALID_MARK; }

    MSTL_NODISCARD bool ssl_valid() const noexcept { return false; }
    MSTL_NODISCARD bool tcp_valid() const noexcept { return sockfd_ != INVALID_MARK; }

    bool open(const SOCKET_DOMAIN domain, const SOCKET_TYPE type = SOCKET_TYPE::STREAM,
        const SOCKET_PROTOCOL protocol = SOCKET_PROTOCOL::AUTO) noexcept {
        sockfd_ = ::socket(
            static_cast<uint16_t>(domain),
            static_cast<uint16_t>(type),
            static_cast<uint16_t>(protocol)
            );
        return is_valid();
    }

    void close() noexcept {
        if (sockfd_ != INVALID_MARK) {
#ifdef MSTL_PLATFORM_WINDOWS__
            ::closesocket(sockfd_);
#elif defined(MSTL_PLATFORM_LINUX__)
            ::close(sockfd_);
#endif
            sockfd_ = INVALID_MARK;
        }
    }

    MSTL_NODISCARD tcp_socket accept() const {
        ::sockaddr_in client_addr{};
        ::socklen_t client_len = sizeof(client_addr);
        return tcp_socket{::accept(
            sockfd_,
            reinterpret_cast<::sockaddr *>(&client_addr),
            &client_len)};
    }

    MSTL_NODISCARD bool listen(const int backlog) const noexcept {
        return ::listen(sockfd_, backlog) == 0;
    }

    MSTL_NODISCARD bool bind(::sockaddr_in& addr) const noexcept {
        return ::bind(sockfd_, reinterpret_cast<::sockaddr*>(&addr), sizeof(addr)) == 0;
    }

    MSTL_NODISCARD bool bind(const uint16_t port) const {
        ::sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = ::htons(port);
        return ::bind(sockfd_, reinterpret_cast<::sockaddr*>(&addr), sizeof(addr)) == 0;
    }

    MSTL_NODISCARD bool reuse_addr() const noexcept {
        constexpr int opt = 1;
        return ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR,
            reinterpret_cast<const char*>(&opt), sizeof(opt)) == 0;
    }

    MSTL_NODISCARD bool set_receive_timeout(const milliseconds timeout) const {
#ifdef MSTL_PLATFORM_WINDOWS__
        ::DWORD tv = static_cast<::DWORD>(timeout.count());
        return ::setsockopt(sockfd_, SOL_SOCKET, SO_RCVTIMEO,
            reinterpret_cast<const char*>(&tv), sizeof(tv)) == 0;
#else
        ::timeval tv{};
        tv.tv_sec = timeout.count() / 1000;
        tv.tv_usec = (timeout.count() % 1000) * 1000;
        return ::setsockopt(sockfd_, SOL_SOCKET, SO_RCVTIMEO,
            reinterpret_cast<const char*>(&tv), sizeof(tv)) == 0;
#endif
    }

    MSTL_NODISCARD bool set_send_timeout(const milliseconds timeout) const {
#ifdef MSTL_PLATFORM_WINDOWS__
        ::DWORD tv = static_cast<::DWORD>(timeout.count());
        return ::setsockopt(sockfd_, SOL_SOCKET, SO_SNDTIMEO,
            reinterpret_cast<const char*>(&tv), sizeof(tv)) == 0;
#else
        ::timeval tv{};
        tv.tv_sec = timeout.count() / 1000;
        tv.tv_usec = (timeout.count() % 1000) * 1000;
        return ::setsockopt(sockfd_, SOL_SOCKET, SO_SNDTIMEO,
            reinterpret_cast<const char*>(&tv), sizeof(tv)) == 0;
#endif
    }

    MSTL_NODISCARD bool connect(const ::sockaddr* addr, const ::socklen_t addr_len) const noexcept {
        return ::connect(sockfd_, addr, addr_len) == 0;
    }

    MSTL_NODISCARD bool connect(const ::sockaddr_in& addr) const noexcept {
        return ::connect(sockfd_, reinterpret_cast<const ::sockaddr*>(&addr), sizeof(addr)) == 0;
    }

    MSTL_NODISCARD bool connect_ipv4(const char* ip, const uint16_t port) const {
        ::sockaddr_storage addr{};
        auto *a4 = reinterpret_cast<::sockaddr_in*>(&addr);
        a4->sin_family = AF_INET;
        if (::inet_pton(AF_INET, ip, &a4->sin_addr) != 1) {
            return false;
        }
        a4->sin_port = ::htons(port);
        constexpr ::socklen_t addr_len = sizeof(::sockaddr_in);
        return ::connect(sockfd_, reinterpret_cast<::sockaddr*>(&addr), addr_len) == 0;
    }

    MSTL_NODISCARD bool connect_ipv6(const char* ip, const uint16_t port) const {
        ::sockaddr_storage addr{};
        auto *a6 = reinterpret_cast<::sockaddr_in6 *>(&addr);
        a6->sin6_family = AF_INET6;
        if (::inet_pton(AF_INET6, ip, &a6->sin6_addr) != 1) {
            return false;
        }
        a6->sin6_port = ::htons(port);
        constexpr ::socklen_t addr_len = sizeof(::sockaddr_in6);
        return ::connect(sockfd_, reinterpret_cast<::sockaddr*>(&addr), addr_len) == 0;
    }

    MSTL_NODISCARD ssize_t send(const void* buf, const size_t len, const int flags = 0) const noexcept {
        return ::send(sockfd_, static_cast<const char*>(buf), len, flags);
    }

    MSTL_NODISCARD ssize_t receive(void* buf, const size_t len, const int flags = 0) const noexcept {
        return ::recv(sockfd_, static_cast<char*>(buf), len, flags);
    }

    MSTL_NODISCARD ssize_t sendto(const void* buf, const size_t len, const ::sockaddr* dest_addr,
        const ::socklen_t addrlen, const int flags = 0) const noexcept {
        return ::sendto(sockfd_, static_cast<const char*>(buf), len, flags, dest_addr, addrlen);
    }

    MSTL_NODISCARD ssize_t sendto(const void* buf, const size_t len,
        const ::sockaddr_in& dest_addr, const int flags = 0) const noexcept {
        return ::sendto(
            sockfd_, static_cast<const char*>(buf), len, flags,
            reinterpret_cast<const ::sockaddr*>(&dest_addr), sizeof(dest_addr));
    }

    MSTL_NODISCARD ssize_t sendto(const void* buf, const size_t len,
        const char* ip, const uint16_t port, const int flags = 0) const {
        ::sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = ::htons(port);
        if (::inet_pton(AF_INET, ip, &addr.sin_addr) != 1) {
            return -1;
        }
        return ::sendto(
            sockfd_, static_cast<const char*>(buf), len, flags,
            reinterpret_cast<const ::sockaddr*>(&addr), sizeof(addr));
    }

    MSTL_NODISCARD ssize_t receive_from(void* buf, const size_t len, ::sockaddr* src_addr,
        ::socklen_t* addrlen, const int flags = 0) const noexcept {
        return ::recvfrom(sockfd_, static_cast<char*>(buf), len, flags, src_addr, addrlen);
    }

    MSTL_NODISCARD ssize_t receive_from(void* buf, const size_t len, const int flags = 0) const {
        ::sockaddr_in from_addr{};
        ::socklen_t from_len = sizeof(from_addr);
        return ::recvfrom(sockfd_, static_cast<char*>(buf), len, flags, reinterpret_cast<sockaddr*>(&from_addr), &from_len);
    }

    MSTL_NODISCARD static bool is_ipv4(const char* host) {
        ::sockaddr_in a4{};
        return ::inet_pton(AF_INET, host, &(a4.sin_addr)) == 1;
    }

    MSTL_NODISCARD static bool is_ipv6(const char* host) {
        ::sockaddr_in6 a6{};
        return ::inet_pton(AF_INET6, host, &(a6.sin6_addr)) == 1;
    }

    MSTL_NODISCARD bool operator ==(const tcp_socket& other) const noexcept {
        return sockfd_ == other.sockfd_;
    }
    MSTL_NODISCARD bool operator !=(const tcp_socket& other) const noexcept {
        return sockfd_ != other.sockfd_;
    }
};

MSTL_END_NAMESPACE__
#endif // MSTL_NETWORK_SOCKET_HPP__
