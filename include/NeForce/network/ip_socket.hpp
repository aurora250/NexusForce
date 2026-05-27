#ifndef NEFORCE_NETWORK_IP_SOCKET_HPP__
#define NEFORCE_NETWORK_IP_SOCKET_HPP__

/**
 * @file ip_socket.hpp
 * @brief IP协议族Socket基类
 *
 * 此文件提供了面向IP协议族（IPv4/IPv6）的Socket基类实现。
 * 在socket_base的基础上增加了地址族约束和连接功能。
 */

#include "NeForce/network/socket_base.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup SocketBase socket基类
 * @{
 */

#pragma pack(push, 1)
/**
 * @struct ip_header
 * @brief IPv4头部结构定义
 *
 * 定义IPv4数据包的头部格式，用于原始套接字操作。
 * 使用位域精确匹配IPv4头部字段布局。
 *
 * IPv4头部格式：
 * - 版本(4位) + 头部长度(4位)
 * - 服务类型(8位)
 * - 总长度(16位)
 * - 标识(16位)
 * - 标志(3位) + 片偏移(13位)
 * - 生存时间(8位)
 * - 协议(8位)
 * - 头部校验和(16位)
 * - 源地址(32位)
 * - 目的地址(32位)
 */
struct ip_header {
    uint8_t version_ihl; ///< 版本(高4位) + 头部长度(低4位)
    uint8_t tos;         ///< 服务类型
    uint16_t total_len;  ///< IP数据包总长度
    uint16_t id;         ///< 标识符
    uint16_t frag_off;   ///< 标志和片偏移
    uint8_t ttl;         ///< 生存时间
    uint8_t protocol;    ///< 上层协议类型
    uint16_t checksum;   ///< 头部校验和
    uint32_t src_addr;   ///< 源IP地址
    uint32_t dest_addr;  ///< 目的IP地址

    NEFORCE_NODISCARD uint8_t version() const noexcept { return version_ihl >> 4; }
    NEFORCE_NODISCARD uint8_t ihl() const noexcept { return version_ihl & 0x0F; }
};
#pragma pack(pop)


/**
 * @class ip_socket
 * @brief IP协议族Socket基类
 *
 * 继承自socket_base，专门用于IP协议族（IPv4/IPv6）的Socket操作。
 * 增加了地址族约束、连接功能和地址族查询。
 *
 * 主要功能：
 * - 地址族约束
 * - IPv4/IPv6地址族检测
 * - 连接操作
 * - IP头结构定义
 *
 * @note 此类是抽象基类，具体使用TCP或UDP时应使用tcp_socket或udp_socket。
 */
class NEFORCE_API ip_socket : public socket_base {
protected:
    family family_ = family::UNDEF; ///< 地址族

    void open_ip(family f, type t, protocol p);

public:
    /**
     * @brief 默认构造函数
     */
    ip_socket() = default;

    /**
     * @brief 从原生句柄构造
     * @param fd 原生socket句柄
     */
    explicit ip_socket(const native_handle_type fd) noexcept :
    socket_base(fd) {}

    ip_socket(ip_socket&&) noexcept = default;
    ip_socket& operator=(ip_socket&&) noexcept = default;

    ip_socket(const ip_socket&) = delete;
    ip_socket& operator=(const ip_socket&) = delete;

    /**
     * @brief 析构函数
     */
    ~ip_socket() override = default;

    /**
     * @brief 获取地址族
     */
    NEFORCE_NODISCARD family address_family() const noexcept { return family_; }

    /**
     * @brief 检查是否为IPv4 socket
     * @return IPv4返回true
     */
    NEFORCE_NODISCARD bool is_ipv4() const noexcept { return family_ == family::INET4; }

    /**
     * @brief 检查是否为IPv6 socket
     * @return IPv6返回true
     */
    NEFORCE_NODISCARD bool is_ipv6() const noexcept { return family_ == family::INET6; }

    /**
     * @brief 连接到远程端点（TCP客户端）
     * @param endpoint 远程IP地址和端口
     * @throws socket_exception 连接失败时抛出
     * @throws value_exception socket未打开或端点无效时抛出
     *
     * 发起TCP连接到指定的远程服务器。
     * 对于非阻塞socket，连接可能不会立即完成。
     */
    virtual void connect(const ip_address& endpoint);

    /**
     * @brief 关闭socket
     * @return 关闭成功返回true
     *
     * 重置地址族为AF_UNSPEC，然后关闭socket。
     */
    bool close() noexcept override {
        family_ = family::UNDEF;
        return socket_base::close();
    }
};

/** @} */ // SocketBase

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_IP_SOCKET_HPP__
