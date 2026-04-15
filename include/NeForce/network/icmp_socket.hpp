#ifndef NEFORCE_NETWORK_SOCKET_ICMP_SOCKET_HPP__
#define NEFORCE_NETWORK_SOCKET_ICMP_SOCKET_HPP__

/**
 * @file icmp_socket.hpp
 * @brief ICMP协议Socket实现
 *
 * 此文件提供了ICMP（Internet控制消息协议）Socket的实现，
 * 支持Ping和Traceroute等网络诊断功能。
 */

#include "NeForce/core/container/vector.hpp"
#include "NeForce/network/socket_base.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Network 网络通信
 * @brief 网络通信相关组件
 * @{
 */

/**
 * @struct icmp_header
 * @brief ICMP头部结构定义
 *
 * 定义ICMP数据包的头部格式，用于原始套接字操作。
 *
 * ICMP头部格式：
 * - 类型(8位)：消息类型
 * - 代码(8位)：消息子类型
 * - 校验和(16位)：头部和数据校验和
 * - 标识符(16位)：用于匹配请求和响应
 * - 序列号(16位)：用于匹配请求和响应
 */
#pragma pack(push, 1)
struct icmp_header {
    uint8_t type;      ///< ICMP消息类型
    uint8_t code;      ///< 消息代码
    uint16_t checksum; ///< 校验和
    uint16_t id;       ///< 标识符
    uint16_t sequence; ///< 序列号
};
#pragma pack(pop)

/**
 * @class icmp_socket
 * @brief ICMP Socket类
 *
 * 实现ICMP协议的原始套接字，支持Ping和Traceroute功能。
 *
 * 主要功能：
 * - ICMP Echo请求/回复（Ping）
 * - 网络路径追踪（Traceroute）
 * - ICMP超时消息处理
 * - RTT（往返时间）测量
 * - TTL设置和读取
 *
 * 使用示例：
 * @code
 * // Ping示例
 * icmp_socket sock;
 * sock.open(AF_INET);
 *
 * auto dest = ip_address::parse("8.8.8.8");
 * if (dest) {
 *     auto result = sock.ping(*dest, milliseconds(2000));
 *     if (result.success) {
 *         println("Reply from ", result.destination,
 *                 ": bytes=", result.reply_size,
 *                 " time=", result.rtt.count(), "ms",
 *                 " TTL=", result.reply_ttl);
 *     } else {
 *         println("Request timed out");
 *     }
 * }
 *
 * // Traceroute示例
 * auto hops = sock.traceroute(*dest, 30, milliseconds(1000), 3);
 * for (size_t i = 0; i < hops.size(); ++i) {
 *     const auto& hop = hops[i];
 *     print(i + 1, ". ");
 *     if (hop.address.is_valid()) {
 *         print(hop.address);
 *         for (int j = 0; j < 3; ++j) {
 *             if (hop.rtt[j].count() >= 0) {
 *                 print(" ", hop.rtt[j].count(), "ms");
 *             } else {
 *                 print(" *");
 *             }
 *         }
 *         println();
 *     } else {
 *         println("* * *");
 *     }
 *     if (hop.reached) break;
 * }
 * @endcode
 *
 * @note 在Linux上需要root权限才能创建原始ICMP套接字。
 *       Windows可能需要管理员权限。
 */
class NEFORCE_API icmp_socket final : public socket_base {
public:
    /**
     * @enum icmp_type
     * @brief ICMP消息类型常量
     */
    enum icmp_type : uint8_t {
        ICMP_ECHO_REPLY = 0,    ///< Echo Reply（Ping响应）
        ICMP_ECHO_REQUEST = 8,  ///< Echo Request（Ping请求）
        ICMP_TIME_EXCEEDED = 11 ///< Time Exceeded（TTL超时）
    };

    /**
     * @struct ping_result
     * @brief Ping操作结果
     */
    struct ping_result {
        ip_address destination; ///< 目标地址
        milliseconds rtt;       ///< 往返时间
        size_t reply_size;      ///< 响应数据大小（字节）
        uint8_t reply_ttl;      ///< 响应TTL值
        bool success;           ///< 是否成功收到响应
    };

    /**
     * @struct traceroute_hop
     * @brief Traceroute跳点信息
     */
    struct traceroute_hop {
        ip_address address;  ///< 跳点IP地址
        milliseconds rtt[3]; ///< 三次探测的RTT值（-1超时）
        bool reached;        ///< 是否到达目标
    };

private:
    bool receive_reply(milliseconds timeout, uint16_t expected_id, uint16_t expected_seq, ip_address& sender,
                       icmp_header& out_header, vector<char>& out_data, uint8_t& recv_ttl);

    void send_echo_request(const ip_address& dest, uint16_t id, uint16_t seq, uint8_t ttl, const void* data,
                           size_t data_len);

public:
    /**
     * @brief 默认构造函数
     */
    icmp_socket() = default;

    /**
     * @brief 从原生句柄构造
     * @param fd 原生socket句柄
     */
    explicit icmp_socket(native_handle_type fd) noexcept :
    socket_base(fd) {}

    /**
     * @brief 打开ICMP socket
     * @throws socket_exception 创建失败时抛出
     *
     * 创建原始ICMP套接字，需要适当权限。
     */
    void open();

    /**
     * @brief 执行Ping操作
     * @param dest 目标IPv4地址
     * @param timeout 超时时间
     * @param sequence 序列号（默认0）
     * @param data 附加数据（可选）
     * @param data_len 附加数据长度
     * @return Ping结果
     *
     * 发送ICMP Echo请求并等待响应，测量RTT。
     */
    ping_result ping(const ip_address& dest, milliseconds timeout, uint16_t sequence = 0, const void* data = nullptr,
                     size_t data_len = 0);

    /**
     * @brief 执行Traceroute操作
     * @param dest 目标IPv4地址
     * @param max_hops 最大跳数（默认30）
     * @param probe_timeout 每跳探测超时时间（默认1000ms）
     * @param probes_per_hop 每跳探测次数（默认3）
     * @return 跳点信息列表
     *
     * 通过逐步增加TTL值探测网络路径，
     * 收集每个中间路由器的IP地址和响应时间。
     */
    vector<traceroute_hop> traceroute(const ip_address& dest, int max_hops = 30,
                                      milliseconds probe_timeout = milliseconds(1000), int probes_per_hop = 3);
};

/** @} */ // Network

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_SOCKET_ICMP_SOCKET_HPP__
