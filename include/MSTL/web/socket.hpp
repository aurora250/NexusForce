#ifndef MSTL_SOCKET_HPP__
#define MSTL_SOCKET_HPP__
#include "MSTL/core/environment.hpp"
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
    socket(const SOCKET_DOMAIN domain,
        const SOCKET_TYPE type = SOCKET_TYPE::STREAM,
        const SOCKET_PROTOCOL protocol = SOCKET_PROTOCOL::AUTO) {
        open(domain, type, protocol);
    }

    socket() = default;

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

    socket_t get() const { return sockfd_; }
    bool is_valid() const { return sockfd_ != INVALID_MARK; }


    bool open(const SOCKET_DOMAIN domain,
        const SOCKET_TYPE type = SOCKET_TYPE::STREAM,
        const SOCKET_PROTOCOL protocol = SOCKET_PROTOCOL::AUTO) {

        sockfd_ = ::socket(
            static_cast<uint16_t>(domain),
            static_cast<uint16_t>(type),
            static_cast<uint16_t>(protocol));

        if (sockfd_ == INVALID_MARK) {
            return false;
        }
        return true;
    }

    void close() {
        if (sockfd_ != INVALID_MARK) {
#ifdef MSTL_PLATFORM_WINDOWS__
            ::closesocket(sockfd_);
#elif defined(MSTL_PLATFORM_LINUX__)
            ::close(sockfd_);
#endif
            sockfd_ = INVALID_MARK;
        }
    }
};

MSTL_END_NAMESPACE__
#endif // MSTL_SOCKET_HPP__
