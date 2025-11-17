#ifndef MSTL_SOCKET_HPP__
#define MSTL_SOCKET_HPP__
#include "../core/datetime/chrono.hpp"
#ifdef MSTL_PLATFORM_WINDOWS__
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")
#include <ws2tcpip.h>
#elif defined(MSTL_PLATFORM_LINUX__)
#include <sys/socket.h>
#include <netinet/in.h>
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


class socket {
public:
#ifdef MSTL_PLATFORM_WINDOWS__
    using socket_t = SOCKET;
    static constexpr socket_t INVALID_MARK = INVALID_SOCKET;
#elif MSTL_PLATFORM_LINUX__
    using socket_t = int;
    static constexpr socket_t INVALID_MARK = -1;
#endif

private:
    socket_t sockfd_ = INVALID_MARK;

public:
    explicit socket(const SOCKET_DOMAIN domain,
        const SOCKET_TYPE type = SOCKET_TYPE::STREAM,
        const SOCKET_PROTOCOL protocol = SOCKET_PROTOCOL::AUTO) noexcept {
        this->open(domain, type, protocol);
    }

    explicit socket(const socket_t fd) : sockfd_(_MSTL move(fd)) {}

    socket() noexcept = default;

    ~socket() {
        close();
    }

    socket(const socket&) = delete;
    socket& operator =(const socket&) = delete;

    socket(socket&& other) noexcept : sockfd_(other.sockfd_) {
        other.sockfd_ = INVALID_MARK;
    }
    socket& operator =(socket&& other) noexcept {
        if (this != &other) {
            close();
            sockfd_ = other.sockfd_;
            other.sockfd_ = INVALID_MARK;
        }
        return *this;
    }

    MSTL_NODISCARD socket_t sockfd() const noexcept { return sockfd_; }
    MSTL_NODISCARD bool is_valid() const noexcept { return sockfd_ != INVALID_MARK; }


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
        if (is_valid()) {
#ifdef MSTL_PLATFORM_WINDOWS__
            ::closesocket(sockfd_);
#elif defined(MSTL_PLATFORM_LINUX__)
            ::close(sockfd_);
#endif
            sockfd_ = INVALID_MARK;
        }
    }

    MSTL_NODISCARD socket accept() const noexcept {
        ::sockaddr_in client_addr{};
        ::socklen_t client_len = sizeof(client_addr);
        return socket{::accept(sockfd_, reinterpret_cast<::sockaddr *>(&client_addr), &client_len)};
    }

    bool listen(const int backlog) const noexcept {
        return ::listen(sockfd_, backlog) == -1;
    }

    int bind(::sockaddr_in& addr) const noexcept {
        return ::bind(sockfd_, reinterpret_cast<::sockaddr*>(&addr), sizeof(addr));
    }

    int reuse_addr() const noexcept {
        constexpr int opt = 1;
        return ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));
    }

    bool set_receive_timeout(const chrono::milliseconds timeout) const noexcept {
        ::timeval tv{};
        tv.tv_sec = timeout.count() / 1000;
        tv.tv_usec = (timeout.count() % 1000) * 1000;
        return ::setsockopt(sockfd_, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&tv), sizeof(tv)) == 0;
    }

    bool set_send_timeout(const chrono::milliseconds timeout) const noexcept {
        ::timeval tv{};
        tv.tv_sec = timeout.count() / 1000;
        tv.tv_usec = (timeout.count() % 1000) * 1000;
        return ::setsockopt(sockfd_, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&tv), sizeof(tv)) == 0;
    }

    MSTL_NODISCARD bool connect(const ::sockaddr* addr, const ::socklen_t addrlen) const noexcept {
        return ::connect(sockfd_, addr, addrlen) == 0;
    }

    MSTL_NODISCARD bool connect(const ::sockaddr_in& addr) const noexcept {
        return socket::connect(reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
    }

    MSTL_NODISCARD ssize_t send(const void* buf, const size_t len, const int flags = 0) const noexcept {
        return ::send(sockfd_, static_cast<const char*>(buf), len, flags);
    }

    MSTL_NODISCARD ssize_t receive(void* buf, const size_t len, const int flags = 0) const noexcept {
        return ::recv(sockfd_, static_cast<char*>(buf), len, flags);
    }

    MSTL_NODISCARD ssize_t send_to(const void* buf, const size_t len, const ::sockaddr* dest_addr,
        const ::socklen_t addrlen, const int flags = 0) const noexcept {
        return ::sendto(sockfd_, static_cast<const char*>(buf), len, flags, dest_addr, addrlen);
    }

    MSTL_NODISCARD ssize_t send_to(const void* buf, const size_t len,
        const ::sockaddr_in& dest_addr, const int flags = 0) const noexcept {
        return ::sendto(sockfd_, static_cast<const char*>(buf), len, flags,
            reinterpret_cast<const sockaddr*>(&dest_addr), sizeof(dest_addr));
    }

    MSTL_NODISCARD ssize_t receive_from(void* buf, const size_t len, ::sockaddr* src_addr,
        ::socklen_t* addrlen, const int flags = 0) const noexcept {
        return ::recvfrom(sockfd_, static_cast<char*>(buf), len, flags, src_addr, addrlen);
    }

    MSTL_NODISCARD ssize_t receive_from(void* buf, const size_t len, const int flags = 0) const noexcept {
        sockaddr_in from_addr{};
        socklen_t from_len = sizeof(from_addr);
        return ::recvfrom(sockfd_, static_cast<char*>(buf), len, flags, reinterpret_cast<sockaddr*>(&from_addr), &from_len);
    }
};

MSTL_END_NAMESPACE__
#endif // MSTL_SOCKET_HPP__
