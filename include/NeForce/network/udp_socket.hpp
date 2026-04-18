#ifndef NEFORCE_NETWORK_UDP_SOCKET_HPP__
#define NEFORCE_NETWORK_UDP_SOCKET_HPP__

/**
 * @file udp_socket.hpp
 * @brief UDP Socket实现
 *
 * 此文件提供了UDP Socket的完整实现，继承自ip_socket类。
 * 支持无连接的数据报发送和接收，以及已连接UDP socket的操作。
 */

#include "NeForce/core/memory/memory_view.hpp"
#include "NeForce/network/ip_socket.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup Network 网络通信
 * @{
 */

/**
 * @class udp_socket
 * @brief UDP Socket类
 *
 * 实现UDP协议的数据报socket，支持无连接和已连接两种模式。
 * UDP是面向数据报的协议，提供不可靠、无连接的服务。
 *
 * 主要功能：
 * - UDP socket创建和打开
 * - 向指定端点发送数据报（send_to）
 * - 从任意端点接收数据报（receive_from）
 * - 已连接UDP socket的发送（send）和接收（receive）
 * - 获取发送方地址信息
 *
 * @note 数据报大小受MTU限制，通常建议不超过65507字节。
 */
class NEFORCE_API udp_socket final : public ip_socket {
public:
    /**
     * @brief 默认构造函数
     */
    udp_socket() = default;

    udp_socket(udp_socket&&) = default;
    udp_socket& operator=(udp_socket&&) = default;

    /**
     * @brief 打开UDP socket
     * @param family 地址族（AF_INET或AF_INET6），默认IPv4
     * @throws socket_exception 创建失败时抛出
     *
     * 创建UDP协议的socket，使用SOCK_DGRAM类型和IPPROTO_UDP协议。
     */
    void open(int family = AF_INET);

    /**
     * @brief 向指定端点发送数据报
     * @param data 要发送的数据
     * @param endpoint 目标端点地址
     * @param flags 发送标志（通常为0）
     * @return 实际发送的字节数
     * @throws socket_exception 发送失败时抛出
     * @throws value_exception socket未打开或端点无效时抛出
     *
     * 向指定的远程端点发送UDP数据报，不需要socket处于连接状态。
     */
    ssize_t send_to(memory_view<const char> data, const ip_address& endpoint, int flags = 0);

    /**
     * @brief 向已连接的端点发送数据
     * @param data 要发送的数据
     * @param flags 发送标志（通常为0）
     * @return 实际发送的字节数
     * @throws socket_exception 发送失败时抛出
     * @throws value_exception socket未打开时抛出
     *
     * 向connect()关联的远程端点发送数据，需要先调用connect()。
     * 性能优于send_to()，因为不需要每次指定目标地址。
     */
    ssize_t send(memory_view<const char> data, int flags = 0);

    /**
     * @brief 接收数据报并获取发送方地址
     * @param buffer 接收缓冲区
     * @param flags 接收标志（通常为0）
     * @return 接收的字节数和发送方地址的pair
     * @throws socket_exception 接收失败时抛出
     * @throws value_exception socket未打开或缓冲区为空时抛出
     *
     * 接收UDP数据报，同时返回发送方的IP地址和端口。
     * 适用于无连接模式的UDP服务器。
     */
    pair<ssize_t, ip_address> receive_from(memory_view<char> buffer, int flags = 0);

    /**
     * @brief 从已连接的端点接收数据
     * @param buffer 接收缓冲区
     * @param flags 接收标志（通常为0）
     * @return 实际接收的字节数
     * @throws socket_exception 接收失败时抛出
     * @throws value_exception socket未打开或缓冲区为空时抛出
     *
     * 从connect()关联的远程端点接收数据，需要先调用connect()。
     * 只能接收来自该端点的数据。
     */
    ssize_t receive(memory_view<char> buffer, int flags = 0);
};

/** @} */ // Network

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_UDP_SOCKET_HPP__
