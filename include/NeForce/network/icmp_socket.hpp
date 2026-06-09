#ifndef NEFORCE_NETWORK_ICMP_SOCKET_HPP__
#define NEFORCE_NETWORK_ICMP_SOCKET_HPP__

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
 * @defgroup ICMP ICMP
 * @brief ICMP协议实现
 *
 * 提供了ICMP（Internet控制消息协议）Socket的实现，
 * 支持Ping和Traceroute等网络诊断功能。
 *
 * @section standards 遵循的国际标准
 * 本实现严格遵循以下网络诊断协议与相关标准规范：
 *
 * **ICMP 核心协议规范：**
 * - **IETF STD 5 / RFC 792**：Internet 控制消息协议（ICMP）
 *   https://www.rfc-editor.org/rfc/rfc792.html
 * - **IETF RFC 1122**：Internet 主机要求 — 通信层（§3.2.2 ICMP 要求）
 *   https://www.rfc-editor.org/rfc/rfc1122.html
 * - **IETF RFC 1812**：IPv4 路由器要求（§4.3 ICMP 协议处理）
 *   https://www.rfc-editor.org/rfc/rfc1812.html
 *
 * **ICMP 扩展与更新：**
 * - **IETF RFC 4884**：ICMP 多部分消息扩展
 *   https://www.rfc-editor.org/rfc/rfc4884.html
 * - **IETF RFC 6918**：ICMP 代码注册表更新
 *   https://www.rfc-editor.org/rfc/rfc6918.html
 *
 * **ICMP 参数注册：**
 * - **IANA ICMP Parameters Registry**：ICMP 类型和代码注册表
 *   https://www.iana.org/assignments/icmp-parameters/icmp-parameters.xhtml
 *
 * **IPv4 协议标准：**
 * - **IETF RFC 791**：Internet 协议（IPv4）
 *   https://www.rfc-editor.org/rfc/rfc791.html
 *
 * @section icmp_message_types ICMP 消息类型
 * 根据 RFC 792 和 IANA 注册表，ICMP 消息类型分类如下：
 *
 * | 类型值 | 名称                     | 用途                           | 本实现支持 |
 * |--------|--------------------------|--------------------------------|------------|
 * | 0      | Echo Reply               | Ping 响应                      | ✓          |
 * | 3      | Destination Unreachable  | 目标不可达                     | -          |
 * | 5      | Redirect                 | 路由重定向                     | -          |
 * | 8      | Echo Request             | Ping 请求                      | ✓          |
 * | 11     | Time Exceeded            | TTL 超时（Traceroute 依赖）    | ✓          |
 * | 13     | Timestamp                | 时间戳请求                     | -          |
 * | 14     | Timestamp Reply          | 时间戳响应                     | -          |
 *
 * @section icmp_header_format ICMP 头部格式
 * 根据 RFC 792 §2，ICMP 头部由以下字段组成：
 *
 * | 字段     | 大小   | 说明                                           |
 * |----------|--------|------------------------------------------------|
 * | Type     | 8 位   | 消息类型（0=Echo Reply, 8=Echo Request, 11=TTL Exceeded）|
 * | Code     | 8 位   | 消息子类型（通常为 0）                         |
 * | Checksum | 16 位  | 整个 ICMP 消息的校验和（RFC 1071 算法）        |
 * | ID       | 16 位  | 标识符，用于匹配请求和响应                     |
 * | Sequence | 16 位  | 序列号，用于匹配请求和响应                     |
 *
 * **Echo Request/Reply 消息**（RFC 792 §3.2）：
 * - 数据部分可选，内容任意
 * - 响应的标识符和序列号必须与请求匹配
 * - 数据部分原样返回
 *
 * **Time Exceeded 消息**（RFC 792 §3.3）：
 * - Code 0：TTL 在传输中耗尽
 * - Code 1：分片重组超时
 * - 包含原始数据包的 IP 头部和前 8 字节
 *
 * @section checksum_algorithm 校验和算法
 * 根据 RFC 1071，ICMP 校验和计算步骤：
 * 1. 将校验和字段清零
 * 2. 将数据按 16 位字累加
 * 3. 将进位加到低 16 位
 * 4. 结果取反码
 *
 * @section ping_operation Ping 操作原理
 * Ping 使用 ICMP Echo Request/Reply 消息测试网络连通性：
 *
 * **工作流程**：
 * 1. 发送方构造 ICMP Echo Request（Type=8），包含 ID 和序列号
 * 2. 目标主机收到后返回 ICMP Echo Reply（Type=0）
 * 3. 发送方根据 ID 和序列号匹配响应
 * 4. 计算往返时间（RTT = 接收时间 - 发送时间）
 *
 * **测量指标**：
 * | 指标       | 说明                           |
 * |------------|--------------------------------|
 * | RTT        | 往返时间（毫秒）               |
 * | Reply TTL  | 响应数据包的剩余 TTL 值        |
 * | Reply Size | 响应数据大小（字节）           |
 *
 * @section traceroute_operation Traceroute 操作原理
 * Traceroute 利用 IP TTL 和 ICMP Time Exceeded 探测网络路径：
 *
 * **工作流程**：
 * 1. 发送 TTL=1 的 UDP 或 ICMP Echo Request 数据包
 * 2. 第一跳路由器丢弃数据包（TTL=0），返回 ICMP Time Exceeded（Type=11）
 * 3. 发送方从 Time Exceeded 消息中提取路由器 IP 地址
 * 4. 逐步增加 TTL（2, 3, ...），重复以上步骤
 * 5. 当数据包到达目标主机时，返回 ICMP Echo Reply（Type=0）或 UDP Port Unreachable
 *
 * **跳点信息**：
 * | 字段     | 说明                           |
 * |----------|--------------------------------|
 * | address  | 该跳路由器的 IP 地址           |
 * | rtt[3]   | 三次探测的往返时间             |
 * | reached  | 是否已到达目标                 |
 *
 * @section usage_examples 使用示例
 * Ping 操作：
 * ```cpp
 * icmp_socket sock;
 * sock.open();
 * auto dest = ip_address::parse("8.8.8.8");
 * if (dest) {
 *     auto result = sock.ping(*dest, milliseconds(2000));
 *     if (result.success) {
 *         println("{}: bytes={} time={}ms TTL={}",
 *                 dest->to_string(), result.reply_size,
 *                 result.rtt.count(), result.reply_ttl);
 *     }
 * }
 * ```
 *
 * Traceroute 操作：
 * ```cpp
 * auto hops = sock.traceroute(*dest, 30, milliseconds(1000), 3);
 * for (size_t i = 0; i < hops.size(); ++i) {
 *     println("{}: {} (reached={})", i + 1,
 *             hops[i].address.is_valid() ? hops[i].address.to_string() : "*",
 *             hops[i].reached);
 * }
 * ```
 *
 * @note Ping 和 Traceroute 是网络诊断中最常用的工具，
 *       Ping 用于测试连通性和延迟，Traceroute 用于分析网络路径。
 *
 * @warning 原始套接字操作需要 root/管理员权限。
 *          某些网络环境可能过滤 ICMP 数据包，导致 Ping/Traceroute 结果不准确。
 *          Traceroute 的中间路由器可能不响应 ICMP Time Exceeded（显示为 *）。
 *
 * @see https://www.rfc-editor.org/rfc/rfc792.html
 * @see https://www.iana.org/assignments/icmp-parameters/icmp-parameters.xhtml
 * @see https://en.wikipedia.org/wiki/Internet_Control_Message_Protocol
 * @see https://en.wikipedia.org/wiki/Ping_(networking_utility)
 * @see https://en.wikipedia.org/wiki/Traceroute
 * @{
 */

#pragma pack(push, 1)
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
 * @note 在Linux上需要root权限才能创建原始ICMP套接字。
 *       Windows可能需要管理员权限。
 */
// TODO: Impl full ICMP socket
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
        ip_address address;       ///< 跳点IP地址
        vector<milliseconds> rtt; ///< 每次探测的RTT值（-1超时）
        bool reached;             ///< 是否到达目标
    };

private:
    bool receive_reply(milliseconds timeout, uint16_t expected_id, uint16_t expected_seq, ip_address& sender,
                       icmp_header& out_header, vector<char>& out_data, uint8_t& recv_ttl);

    void send_echo_request(const ip_address& dest, uint16_t id, uint16_t seq, int ttl, const void* data,
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

/** @} */ // ICMP

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_ICMP_SOCKET_HPP__
