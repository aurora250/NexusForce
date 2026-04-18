#ifndef NEFORCE_NETWORK_DNS_DNS_MESSAGE_HPP__
#define NEFORCE_NETWORK_DNS_DNS_MESSAGE_HPP__

/**
 * @file dns_message.hpp
 * @brief DNS协议消息定义
 *
 * 此文件定义了DNS协议的消息结构和相关枚举类型，用于DNS查询和响应。
 * 包括DNS操作码、查询类型、响应码、记录类型等核心定义。
 *
 * 主要功能：
 * - DNS操作码定义（标准查询、反向查询、状态查询等）
 * - DNS查询类型定义（Internet、CHAOS、HESIOD等）
 * - DNS响应码定义（无错误、格式错误、服务器失败等）
 * - DNS记录结构（A、NS、CNAME、MX、TXT、AAAA、SRV等）
 * - DNS查询结果结构（答案、权威、附加记录）
 * - DNS异常类定义
 */

#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/string/string.hpp"
#include "NeForce/core/time/duration.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Network 网络通信
 * @brief 网络通信相关组件
 * @{
 */

/**
 * @defgroup DNS DNS
 * @brief DNS组件
 *
 * 本模块提供了 DNS（域名系统）协议的完整客户端实现，支持消息构建、查询发送、
 * 响应解析、缓存管理以及多种记录类型的查询。
 *
 * @section standards 遵循的国际标准
 * 本实现严格遵循以下 IETF RFC 规范与相关标准：
 *
 * **DNS 核心协议规范：**
 * - **IETF STD 13 / RFC 1034**：域名 — 概念与设施
 *   https://www.rfc-editor.org/rfc/rfc1034.html
 * - **IETF STD 13 / RFC 1035**：域名 — 实现与规范
 *   https://www.rfc-editor.org/rfc/rfc1035.html
 *
 * **DNS 扩展与更新：**
 * - **IETF RFC 2181**：DNS 规范的澄清
 *   https://www.rfc-editor.org/rfc/rfc2181.html
 * - **IETF RFC 6891**：DNS 扩展机制 (EDNS0)
 *   https://www.rfc-editor.org/rfc/rfc6891.html
 *
 * **DNS 记录类型定义：**
 * - **IANA DNS Parameters Registry**：DNS 参数注册表
 *   https://www.iana.org/assignments/dns-parameters/dns-parameters.xhtml
 *
 * **DNS 传输协议：**
 * - **IETF RFC 7766**：DNS over TCP 实现要求
 *   https://www.rfc-editor.org/rfc/rfc7766.html
 * - **IETF RFC 8484**：DNS over HTTPS (DoH)（参考）
 *   https://www.rfc-editor.org/rfc/rfc8484.html
 *
 * **IPv6 地址记录：**
 * - **IETF RFC 3596**：DNS 对 IPv6 的扩展 (AAAA 记录)
 *   https://www.rfc-editor.org/rfc/rfc3596.html
 *
 * **邮件交换记录：**
 * - **IETF RFC 1035 §3.3.9**：MX 记录定义
 * - **IETF RFC 7505**：空 MX 记录（"." 表示不接收邮件）
 *   https://www.rfc-editor.org/rfc/rfc7505.html
 *
 * **服务定位记录：**
 * - **IETF RFC 2782**：DNS SRV 记录
 *   https://www.rfc-editor.org/rfc/rfc2782.html
 *
 * @section dns_message_structure DNS 消息结构
 * 根据 RFC 1035 §4.1，DNS 消息由以下部分组成：
 *
 * | 部分           | 大小       | 说明                                           |
 * |----------------|------------|------------------------------------------------|
 * | Header         | 12 字节    | 包含 ID、标志位、各段计数                       |
 * | Question       | 可变       | 查询的域名、类型、类                           |
 * | Answer         | 可变       | 回答的资源记录                                 |
 * | Authority      | 可变       | 指向权威名称服务器的资源记录                   |
 * | Additional     | 可变       | 附加信息记录（如 EDNS0）                       |
 *
 * **头部标志位**（RFC 1035 §4.1.1）：
 * | 位偏移 | 名称  | 说明                                                         |
 * |--------|-------|--------------------------------------------------------------|
 * | 0      | QR    | 0=查询，1=响应                                                |
 * | 1-4    | OPCODE| 操作码：0=QUERY, 1=IQUERY, 2=STATUS, 4=NOTIFY, 5=UPDATE      |
 * | 5      | AA    | 权威回答                                                      |
 * | 6      | TC    | 截断标志（响应被截断，需使用 TCP 重试）                       |
 * | 7      | RD    | 期望递归查询                                                  |
 * | 8      | RA    | 递归可用                                                      |
 * | 9-11   | Z     | 保留位（必须为0）                                             |
 * | 12-15  | RCODE | 响应码：0=无错误, 1=格式错误, 2=服务器失败, 3=名称错误等      |
 *
 * @section dns_record_types DNS 记录类型
 * 根据 IANA DNS 参数注册表，本模块支持以下常见记录类型：
 *
 * | 类型   | 值  | RFC 引用      | 说明                                   |
 * |--------|-----|---------------|----------------------------------------|
 * | A      | 1   | RFC 1035      | 主机地址（IPv4）                       |
 * | NS     | 2   | RFC 1035      | 权威名称服务器                         |
 * | CNAME  | 5   | RFC 1035      | 规范名称（别名）                       |
 * | SOA    | 6   | RFC 1035      | 授权区域起始                           |
 * | PTR    | 12  | RFC 1035      | 域名指针（反向查询）                   |
 * | MX     | 15  | RFC 1035      | 邮件交换                               |
 * | TXT    | 16  | RFC 1035      | 文本字符串                             |
 * | AAAA   | 28  | RFC 3596      | IPv6 地址                              |
 * | SRV    | 33  | RFC 2782      | 服务定位器                             |
 *
 * @section dns_opcodes 操作码与响应码
 * **操作码（OPCODE）**（RFC 1035 §4.1.1，RFC 1996，RFC 2136）：
 * | 值 | 名称     | 说明                     |
 * |----|----------|--------------------------|
 * | 0  | QUERY    | 标准查询                 |
 * | 1  | IQUERY   | 反向查询（已废弃）       |
 * | 2  | STATUS   | 服务器状态请求           |
 * | 4  | NOTIFY   | 区域变更通知             |
 * | 5  | UPDATE   | 动态更新                 |
 *
 * **响应码（RCODE）**（RFC 1035 §4.1.1，RFC 6891 扩展）：
 * | 值 | 名称             | 说明                           |
 * |----|------------------|--------------------------------|
 * | 0  | NoError          | 无错误                         |
 * | 1  | FormErr          | 格式错误                       |
 * | 2  | ServFail         | 服务器失败                     |
 * | 3  | NXDomain         | 不存在的域名                   |
 * | 4  | NotImp           | 未实现                         |
 * | 5  | Refused          | 拒绝查询                       |
 *
 * @section dns_classes 查询类（CLASS）
 * 根据 RFC 1035 §3.2.4：
 * | 值  | 名称     | 说明               |
 * |-----|----------|--------------------|
 * | 1   | IN       | Internet（最常用） |
 * | 3   | CH       | CHAOS 类           |
 * | 4   | HS       | Hesiod             |
 * | 255 | ANY      | 任何类（通配）     |
 *
 * @section dns_transport DNS 传输协议
 * 根据 RFC 1035 §4.2 和 RFC 7766：
 * - **UDP**：默认传输协议，消息大小限制为 512 字节（EDNS0 可扩展）
 * - **TCP**：当响应被截断（TC 标志置位）或消息超过 UDP 限制时自动切换
 * - **本实现**：优先使用 UDP，若响应截断则自动通过 TCP 重试
 *
 * @section dns_caching DNS 缓存机制
 * 根据 RFC 1035 §7.2 的 TTL 规范：
 * - 每条资源记录包含 TTL（生存时间，秒）
 * - 缓存条目按记录的 TTL 值过期
 * - 本实现支持自定义缓存 TTL 上限（默认 300 秒）
 * - 缓存条目在查询前检查有效期，过期后自动刷新
 *
 * @section reverse_dns 反向 DNS 查询
 * 根据 RFC 1035 §3.5，PTR 记录用于 IP 地址到域名的映射：
 * - IPv4 地址转换为特殊域名：`x.x.x.x.in-addr.arpa`
 * - 例如 IP `8.8.8.8` 查询 `8.8.8.8.in-addr.arpa` 的 PTR 记录
 *
 * @note DNS 消息使用大端字节序编码，本实现自动处理字节序转换。
 *       对于超过 512 字节的 UDP 响应，建议启用 EDNS0 或依赖 TCP 自动切换机制。
 *
 * @warning 缓存 TTL 应根据实际业务需求设置，过长的 TTL 可能导致 IP 变更后无法及时更新。
 *          反向查询（PTR）并非所有 IP 地址都有对应的记录。
 *
 * @see https://www.iana.org/assignments/dns-parameters/dns-parameters.xhtml
 * @see https://www.rfc-editor.org/rfc/rfc1034.html
 * @see https://www.rfc-editor.org/rfc/rfc1035.html
 * @see https://developers.google.com/speed/public-dns
 * @see https://www.cloudflare.com/learning/dns/what-is-dns/
 * @{
 */

/**
 * @enum dns_opcode
 * @brief DNS操作码枚举
 *
 * 定义DNS消息的操作类型，指定查询或响应的目的。
 */
enum class dns_opcode : uint8_t {
    QUERY = 0,  ///< 标准查询
    IQUERY = 1, ///< 反向查询
    STATUS = 2, ///< 服务器状态请求
    NOTIFY = 4, ///< 区域通知
    UPDATE = 5  ///< 区域更新
};

/**
 * @enum dns_query
 * @brief DNS查询类型枚举
 *
 * 定义DNS查询的类别，指定查询的协议族或类型。
 */
enum class dns_query : uint16_t {
    INTERNET = 1, ///< Internet类
    CHAOS = 3,    ///< CHAOS类
    HESIOD = 4,   ///< HESIOD类
    ANY = 255     ///< 任何类
};

/**
 * @enum dns_response
 * @brief DNS响应码枚举
 *
 * 定义DNS服务器返回的响应状态码。
 */
enum class dns_response : uint8_t {
    NON_ERROR = 0,       ///< 无错误，查询成功
    FORMAT_ERROR = 1,    ///< 格式错误，服务器无法解析查询
    SERVER_FAILURE = 2,  ///< 服务器失败，无法处理查询
    NAME_ERROR = 3,      ///< 名称错误，域名不存在
    NOT_IMPLEMENTED = 4, ///< 未实现，服务器不支持该查询类型
    REFUSED = 5          ///< 拒绝，服务器拒绝执行查询
};

/** @} */ // DNS

/** @} */ // Network

/**
 * @addtogroup Exceptions 异常类集
 * @{
 */

/**
 * @class dns_exception
 * @brief DNS异常类
 *
 * DNS操作失败时抛出的异常，包含错误类型和详细信息。
 */
class dns_exception final : public network_exception {
public:
    /**
     * @enum code
     * @brief DNS异常错误码
     */
    enum class code {
        TIMEOUT,        ///< 查询超时
        NETWORK_ERROR,  ///< 网络错误
        PARSE_ERROR,    ///< 解析错误
        SERVER_FAILURE, ///< 服务器失败
        TRUNCATED,      ///< 响应被截断
        NO_RECORD       ///< 无记录
    };

    explicit dns_exception(const string& what) :
    network_exception(what.data()) {}

    dns_exception(const string& what, const code code) :
    network_exception(what.data(), static_type, static_cast<int>(code)) {}

    /**
     * @brief 创建超时异常
     * @return DNS异常对象
     */
    static dns_exception timeout() { return dns_exception("DNS query timeout", code::TIMEOUT); }

    /**
     * @brief 创建网络错误异常
     * @param detail 错误详情
     * @param code 错误码
     * @return DNS异常对象
     */
    static dns_exception network_error(const string& detail, const code code = code::NETWORK_ERROR) {
        return dns_exception("Network error: " + detail, code);
    }

    /**
     * @brief 创建解析错误异常
     * @param detail 错误详情
     * @return DNS异常对象
     */
    static dns_exception parse_error(const string& detail) {
        return dns_exception("Parse error: " + detail, code::PARSE_ERROR);
    }

    static constexpr auto static_type = "dns_exception";
};

/** @} */ // Exceptions

/**
 * @addtogroup Network 网络通信
 * @{
 */

/**
 * @addtogroup DNS DNS
 * @{
 */

/**
 * @struct dns_record
 * @brief DNS资源记录结构
 *
 * 表示一个DNS资源记录，包含域名、类型、数据、TTL等信息。
 */
struct dns_record {
    /**
     * @enum raw
     * @brief DNS记录类型枚举
     *
     * 定义常见的DNS记录类型。
     */
    enum raw : uint16_t {
        A = 1,     ///< IPv4地址记录
        NS = 2,    ///< 名称服务器记录
        CNAME = 5, ///< 规范名称记录
        SOA = 6,   ///< 授权开始记录
        PTR = 12,  ///< 指针记录
        MX = 15,   ///< 邮件交换记录
        TXT = 16,  ///< 文本记录
        AAAA = 28, ///< IPv6地址记录
        SRV = 33   ///< 服务定位记录
    };

    string name;          ///< 记录名称
    string data;          ///< 记录数据
    uint32_t ttl;         ///< 生存时间（秒）
    raw type{raw::A};     ///< 记录类型
    dns_query class_type; ///< 查询类

    /**
     * @brief 默认构造函数
     */
    dns_record() = default;

    /**
     * @brief 构造函数
     * @param n 记录名称
     * @param t 记录类型
     * @param c 查询类
     * @param ttl_val 生存时间
     * @param d 记录数据
     */
    dns_record(string n, const raw t, const dns_query c, const uint32_t ttl_val, string d) noexcept :
    name(_NEFORCE move(n)),
    data(_NEFORCE move(d)),
    ttl(ttl_val),
    type(t),
    class_type(c) {}
};

/**
 * @struct dns_query_result
 * @brief DNS查询结果结构
 *
 * 包含DNS查询的完整响应数据，包括答案记录、权威记录、附加记录等。
 */
struct dns_query_result {
    vector<dns_record> answers;                          ///< 答案记录
    vector<dns_record> authorities;                      ///< 权威记录
    vector<dns_record> additional;                       ///< 附加记录
    milliseconds query_time;                             ///< 查询耗时
    dns_response response_code{dns_response::NON_ERROR}; ///< 响应码
    bool truncated;                                      ///< 响应是否被截断
    bool recursive_available;                            ///< 递归查询是否可用

    /**
     * @brief 检查查询是否成功
     * @return 成功返回true
     */
    NEFORCE_NODISCARD bool is_success() const noexcept { return response_code == dns_response::NON_ERROR; }
};

/**
 * @struct dns_header
 * @brief DNS消息头部结构
 *
 * 表示DNS消息的12字节头部，包含ID、标志、计数等字段。
 */
struct dns_header {
    uint16_t id = 0;      ///< 消息ID
    uint16_t flags = 0;   ///< 标志位
    uint16_t qdcount = 0; ///< 问题计数
    uint16_t ancount = 0; ///< 答案计数
    uint16_t nscount = 0; ///< 权威计数
    uint16_t arcount = 0; ///< 附加计数
};

/** @} */ // DNS

/** @} */ // Network

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_DNS_DNS_MESSAGE_HPP__
