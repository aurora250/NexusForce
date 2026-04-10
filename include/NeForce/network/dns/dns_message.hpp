#ifndef NEFORCE_NETWORK_DNS_MESSAGE_HPP
#define NEFORCE_NETWORK_DNS_MESSAGE_HPP

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

/** @} */ // Network

/**
 * @defgroup Exceptions 异常类集
 * @brief 异常类集
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

    /**
     * @brief 构造函数
     * @param what 错误描述
     */
    explicit dns_exception(const string& what) :
    network_exception(what.data()) {}

    /**
     * @brief 构造函数
     * @param what 错误描述
     * @param code 错误码
     */
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
 * @defgroup Network 网络通信
 * @brief 网络通信相关组件
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

/** @} */ // Network

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_DNS_MESSAGE_HPP
