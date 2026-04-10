#ifndef NEFORCE_NETWORK_SOCKET_IP_SOCKET_HPP__
#define NEFORCE_NETWORK_SOCKET_IP_SOCKET_HPP__
#include "NeForce/network/socket_base.hpp"
NEFORCE_BEGIN_NAMESPACE__

#pragma pack(push, 1)
struct ip_header {
    uint8_t ihl : 4;
    uint8_t version : 4;
    uint8_t tos;
    uint16_t total_len;
    uint16_t id;
    uint16_t frag_off;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint32_t src_addr;
    uint32_t dest_addr;
};
#pragma pack(pop)


/**
 * @class ip_socket
 * @extends socket_base
 * @brief 面向 IP 协议族（IPv4 / IPv6）的 socket 基类
 *
 * 在 socket_base 的基础上增加：
 *  - 地址族约束（仅允许 AF_INET / AF_INET6）
 *  - 统一的 open(family, type, protocol) 实现
 *  - connect / accept 公共接口声明
 */
class NEFORCE_API ip_socket : public socket_base {
protected:
    int family_ = AF_UNSPEC;

    void open_ip(int family, int type, int protocol);

public:
    ip_socket() = default;

    explicit ip_socket(const native_handle_type fd) noexcept :
    socket_base(fd) {}

    ip_socket(ip_socket&&) noexcept = default;
    ip_socket& operator=(ip_socket&&) noexcept = default;

    ip_socket(const ip_socket&) = delete;
    ip_socket& operator=(const ip_socket&) = delete;

    ~ip_socket() override = default;

    NEFORCE_NODISCARD int address_family() const noexcept { return family_; }

    NEFORCE_NODISCARD bool is_ipv4() const noexcept { return family_ == AF_INET; }

    NEFORCE_NODISCARD bool is_ipv6() const noexcept { return family_ == AF_INET6; }

    virtual void connect(const ip_address& endpoint);

    bool close() noexcept override {
        family_ = AF_UNSPEC;
        return socket_base::close();
    }
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_SOCKET_IP_SOCKET_HPP__
