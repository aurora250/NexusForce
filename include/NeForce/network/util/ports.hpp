#ifndef NEFORCE_NETWORK_UTIL_PORTS_HPP__
#define NEFORCE_NETWORK_UTIL_PORTS_HPP__

/**
 * @file ports.hpp
 * @brief 网络端口定义和转换工具
 *
 * 此文件提供了常见网络端口的类型安全封装，
 * 支持从协议名称解析端口号，以及将端口号转换为协议名称。
 */

#include "NeForce/core/interface/iobject.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup NetworkUtil 网络工具
 * @brief 网络通信辅助工具组件
 * @{
 */

/**
 * @struct ports
 * @brief 网络端口封装类
 *
 * 提供类型安全的端口表示，封装了常见协议的端口号，支持从协议名称解析和反向转换。
 *
 * @section standards 遵循的国际标准
 * 本实现严格遵循以下网络服务与端口分配相关标准规范：
 *
 * **端口号分配权威机构：**
 * - **IANA Service Name and Transport Protocol Port Number Registry**：服务名称与端口号注册表
 *   https://www.iana.org/assignments/service-names-port-numbers/service-names-port-numbers.xhtml
 *
 * **核心互联网协议标准：**
 * - **IETF RFC 1340**：Assigned Numbers（已废弃，由 IANA 在线数据库取代）
 *   https://www.rfc-editor.org/rfc/rfc1340.html
 * - **IETF RFC 6335**：Internet 号码分配机构（IANA）服务名称和传输协议端口号注册程序
 *   https://www.rfc-editor.org/rfc/rfc6335.html
 * - **IETF STD 2**：Assigned Internet Protocol Numbers（已废弃，由 RFC 3232 取代）
 *   https://www.rfc-editor.org/rfc/rfc3232.html
 *
 * **各协议端口规范：**
 * | 协议          | 端口号 | 标准引用                                               | 说明                     |
 * |---------------|--------|--------------------------------------------------------|--------------------------|
 * | HTTP          | 80     | RFC 7230 §2.7.1（HTTP/1.1 消息语法与路由）             | 万维网协议               |
 * | HTTPS         | 443    | RFC 2818 §2.4（HTTP Over TLS）                         | HTTP over TLS/SSL        |
 * | FTP           | 21     | RFC 959 §3.2（文件传输协议）                           | 文件传输协议（控制端口） |
 * | FTP-DATA      | 20     | RFC 959 §3.2（文件传输协议）                           | 文件传输协议（数据端口） |
 * | SSH           | 22     | RFC 4251 §4.1（安全 Shell 协议架构）                   | 安全 Shell 协议          |
 * | Telnet        | 23     | RFC 854（Telnet 协议规范）                             | 远程登录协议             |
 * | SMTP          | 25     | RFC 5321 §4.5.3.2（简单邮件传输协议）                  | 电子邮件传输             |
 * | DNS           | 53     | RFC 1035 §4.2（域名实现与规范）                        | 域名系统                 |
 * | TFTP          | 69     | RFC 1350 §5（简单文件传输协议）                        | 简单文件传输协议         |
 * | POP3          | 110    | RFC 1939 §4（邮局协议版本3）                           | 电子邮件接收协议         |
 * | IMAP          | 143    | RFC 3501 §2.1（互联网消息访问协议）                    | 电子邮件访问协议         |
 * | NTP           | 123    | RFC 5905 §4（网络时间协议版本4）                       | 网络时间同步协议         |
 * | SNMP          | 161    | RFC 3417 §3（简单网络管理协议）                        | 网络管理协议             |
 * | SNMP-TRAP     | 162    | RFC 3417 §3（简单网络管理协议）                        | SNMP陷阱通知             |
 * | LDAP          | 389    | RFC 4511 §3（轻量级目录访问协议）                      | 目录服务协议             |
 * | LDAPS         | 636    | RFC 4513 §5（LDAP over TLS）                           | LDAP over TLS/SSL        |
 * | SMB           | 445    | Microsoft SMB 协议规范                                 | 服务器消息块协议         |
 * | DHCP-SERVER   | 67     | RFC 2131 §3（动态主机配置协议）                        | DHCP服务器端口            |
 * | DHCP-CLIENT   | 68     | RFC 2131 §3（动态主机配置协议）                        | DHCP客户端端口            |
 * | MYSQL         | 3306   | MySQL 官方文档                                         | MySQL数据库              |
 * | POSTGRESQL    | 5432   | PostgreSQL 官方文档                                    | PostgreSQL数据库         |
 * | REDIS         | 6379   | Redis 官方文档                                         | Redis键值存储            |
 * | MONGODB       | 27017  | MongoDB 官方文档                                       | MongoDB数据库            |
 *
 * **WebSocket 协议端口规范：**
 * | 协议          | 端口号 | 标准引用                                               | 说明                     |
 * |---------------|--------|--------------------------------------------------------|--------------------------|
 * | WS            | 80     | RFC 6455 §11.1（WebSocket 协议）                       | WebSocket over HTTP      |
 * | WSS           | 443    | RFC 6455 §11.1（WebSocket 协议）                       | WebSocket over TLS/SSL   |
 *
 * **邮件协议扩展端口规范：**
 * | 协议          | 端口号 | 标准引用                                               | 说明                     |
 * |---------------|--------|--------------------------------------------------------|--------------------------|
 * | SMTP-SUBMIT   | 587    | RFC 6409 §3（邮件提交协议）                            | 邮件提交                 |
 * | SMTPS         | 465    | 历史惯例（IANA已重新分配）                             | SMTP over TLS（传统）    |
 * | POP3S         | 995    | RFC 2595 §5（POP3 over TLS）                           | POP3 over TLS/SSL        |
 * | IMAPS         | 993    | RFC 2595 §4（IMAP over TLS）                           | IMAP over TLS/SSL        |
 *
 * **相关安全标准：**
 * - **IETF RFC 2818**：HTTP Over TLS（HTTPS 端口定义）
 *   https://www.rfc-editor.org/rfc/rfc2818.html
 * - **IETF RFC 6455**：WebSocket 协议（WS/WSS 端口定义）
 *   https://www.rfc-editor.org/rfc/rfc6455.html
 * - **IETF RFC 2595**：使用 TLS 的 IMAP、POP3 和 ACAP
 *   https://www.rfc-editor.org/rfc/rfc2595.html
 *
 * @section port_registry IANA 端口号注册表
 * 根据 IANA 服务名称和端口号注册表，端口号分为三类：
 *
 * | 范围            | 类别           | 说明                                   | 分配要求                     |
 * |-----------------|----------------|----------------------------------------|------------------------------|
 * | 0-1023          | 知名端口       | 系统端口，由 IANA 分配和管控           | 需要 IETF 共识或 IESG 批准   |
 * | 1024-49151      | 注册端口       | 用户端口，由 IANA 注册                 | 需要 IETF 审核或专家审核     |
 * | 49152-65535     | 动态/私有端口  | 临时使用，自动分配                      | 无需分配                     |
 *
 * 本文件中定义的所有端口包含知名端口（0-1023）和部分常用注册端口（1024-49151）。
 *
 * @section implementation_details 实现细节
 * | 特性              | 规范参数                                  |
 * |-------------------|-------------------------------------------|
 * | 端口数值类型      | uint16_t（0-65535）                       |
 * | 知名端口范围      | 0-1023                                   |
 * | 注册端口范围      | 1024-49151                               |
 * | 默认端口值        | UNDEF（0）                                |
 * | 大小              | 2 字节（uint16_t）                        |
 * | 协议名称解析      | 大小写不敏感                              |
 * | 未知协议处理      | 返回 UNDEF                               |
 *
 * @note 本实现中定义的端口号均取自 IANA 注册表及相关 RFC 标准文档。
 *
 * @warning 端口号 80 和 443 同时服务于 HTTP/HTTPS 和 WebSocket（WS/WSS）协议。
 *          使用 `to_string(bool is_ws)` 方法可明确区分协议类型。
 *          端口号 465（smtps）虽然在实践中广泛使用，但 IANA 已将其重新分配给其他服务，
 *          建议新应用使用端口 587（smtp-submit）配合 STARTTLS。
 *
 * @see https://www.iana.org/assignments/service-names-port-numbers/
 * @see https://www.rfc-editor.org/rfc/rfc6335
 * @see https://en.wikipedia.org/wiki/List_of_TCP_and_UDP_port_numbers
 */
struct NEFORCE_API ports : iobject<ports> {
    /**
     * @enum raw
     * @brief 端口号枚举值
     */
    enum raw : uint16_t {
        UNDEF = 0, ///< 未定义/无效端口

        FTP_DATA = 20,     ///< FTP数据端口
        FTP = 21,          ///< FTP控制端口
        SSH = 22,          ///< SSH协议端口
        TELNET = 23,       ///< Telnet协议端口
        SMTP = 25,         ///< SMTP协议端口
        DNS = 53,          ///< DNS协议端口
        DHCP_SERVER = 67,  ///< DHCP服务器端口
        DHCP_CLIENT = 68,  ///< DHCP客户端端口
        TFTP = 69,         ///< TFTP协议端口
        HTTP = 80,         ///< HTTP协议端口
        WS = 80,           ///< WebSocket协议端口（与HTTP共用）
        POP3 = 110,        ///< POP3协议端口
        NTP = 123,         ///< NTP协议端口
        IMAP = 143,        ///< IMAP协议端口
        SNMP = 161,        ///< SNMP协议端口
        SNMP_TRAP = 162,   ///< SNMP陷阱通知端口
        LDAP = 389,        ///< LDAP协议端口
        HTTPS = 443,       ///< HTTPS协议端口
        WSS = 443,         ///< WebSocket Secure端口（与HTTPS共用）
        SMB = 445,         ///< SMB/CIFS协议端口
        SMTPS = 465,       ///< SMTPS协议端口
        SMTP_SUBMIT = 587, ///< SMTP邮件提交端口
        LDAPS = 636,       ///< LDAPS协议端口
        IMAPS = 993,       ///< IMAPS协议端口
        POP3S = 995,       ///< POP3S协议端口

        MYSQL = 3306,      ///< MySQL数据库端口
        POSTGRESQL = 5432, ///< PostgreSQL数据库端口
        REDIS = 6379,      ///< Redis数据库端口
        MONGODB = 27017    ///< MongoDB数据库端口
    };

    raw port{UNDEF}; ///< 端口值

    /**
     * @brief 默认构造函数
     *
     * 创建未定义端口的对象。
     */
    constexpr ports() noexcept = default;
    NEFORCE_CONSTEXPR20 ~ports() noexcept = default;

    constexpr ports(const ports&) noexcept = default;
    constexpr ports& operator=(const ports&) noexcept = default;

    constexpr ports(ports&&) noexcept = default;
    constexpr ports& operator=(ports&&) noexcept = default;

    /**
     * @brief 从枚举值构造
     * @param port 端口枚举值
     */
    constexpr ports(const raw port) noexcept :
    port(port) {}

    /**
     * @brief 从uint16_t构造
     * @param port 端口数值
     */
    constexpr explicit ports(const uint16_t port) noexcept :
    // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
    port(static_cast<raw>(port)) {}

    /**
     * @brief 布尔转换运算符
     * @return 端口有效返回true
     *
     * 检查端口是否为undef。
     */
    constexpr explicit operator bool() const noexcept { return port != ports::UNDEF; }

    /**
     * @brief uint16_t转换运算符
     * @return 端口的数值表示
     */
    constexpr explicit operator uint16_t() const noexcept { return static_cast<uint16_t>(port); }

    NEFORCE_NODISCARD constexpr uint16_t value() const noexcept { return static_cast<uint16_t>(port); }

    /**
     * @brief 检查端口是否在知名端口范围内
     * @return 若端口号在0-1023范围内返回true
     */
    NEFORCE_NODISCARD constexpr bool is_well_known() const noexcept { return static_cast<uint16_t>(port) <= 1023; }

    /**
     * @brief 检查端口是否在注册端口范围内
     * @return 若端口号在1024-49151范围内返回true
     */
    NEFORCE_NODISCARD constexpr bool is_registered() const noexcept {
        auto p = static_cast<uint16_t>(port);
        return p >= 1024 && p <= 49151;
    }

    /**
     * @brief 检查端口是否在动态/私有端口范围内
     * @return 若端口号在49152-65535范围内返回true
     */
    NEFORCE_NODISCARD constexpr bool is_dynamic() const noexcept { return static_cast<uint16_t>(port) >= 49152; }

    /**
     * @brief 从协议名称解析端口
     * @param scheme 协议名称（如"http"、"https"、"ws"等）
     * @return 对应的端口对象，无法识别返回undef
     *
     * 支持常见协议名称到端口号的映射。
     */
    static ports parse(string_view scheme) noexcept;

    /**
     * @brief 转换为协议名称字符串
     * @return 协议名称，非标准端口返回空字符串
     *
     * 将端口号转换为对应的协议名称。
     * 对于80端口返回"http"，443端口返回"https"。
     */
    NEFORCE_NODISCARD string to_string() const;

    /**
     * @brief 转换为协议名称字符串（支持WebSocket）
     * @param is_ws 是否为WebSocket协议
     * @return 协议名称
     *
     * 当is_ws为true时，80端口返回"ws"，443端口返回"wss"；
     * 否则返回"http"/"https"。
     */
    NEFORCE_NODISCARD string to_string(bool is_ws) const;
};


NEFORCE_NODISCARD NEFORCE_ALWAYS_INLINE_INLINE constexpr bool operator==(const ports lhs, const uint16_t rhs) noexcept {
    return lhs.port == static_cast<ports::raw>(rhs);
}

NEFORCE_NODISCARD NEFORCE_ALWAYS_INLINE_INLINE constexpr bool operator==(const uint16_t lhs, const ports rhs) noexcept {
    return static_cast<ports::raw>(lhs) == rhs.port;
}

NEFORCE_NODISCARD NEFORCE_ALWAYS_INLINE_INLINE constexpr bool operator==(const ports lhs,
                                                                         const ports::raw rhs) noexcept {
    return lhs.port == rhs;
}

NEFORCE_NODISCARD NEFORCE_ALWAYS_INLINE_INLINE constexpr bool operator==(const ports::raw lhs,
                                                                         const ports rhs) noexcept {
    return lhs == rhs.port;
}

NEFORCE_NODISCARD NEFORCE_ALWAYS_INLINE_INLINE constexpr bool operator==(const ports lhs, const ports rhs) noexcept {
    return lhs.port == rhs.port;
}

NEFORCE_NODISCARD NEFORCE_ALWAYS_INLINE_INLINE constexpr bool operator!=(const ports lhs, const uint16_t rhs) noexcept {
    return lhs.port != static_cast<ports::raw>(rhs);
}

NEFORCE_NODISCARD NEFORCE_ALWAYS_INLINE_INLINE constexpr bool operator!=(const uint16_t lhs, const ports rhs) noexcept {
    return static_cast<ports::raw>(lhs) != rhs.port;
}

NEFORCE_NODISCARD NEFORCE_ALWAYS_INLINE_INLINE constexpr bool operator!=(const ports lhs,
                                                                         const ports::raw rhs) noexcept {
    return lhs.port != rhs;
}

NEFORCE_NODISCARD NEFORCE_ALWAYS_INLINE_INLINE constexpr bool operator!=(const ports::raw lhs,
                                                                         const ports rhs) noexcept {
    return lhs != rhs.port;
}

NEFORCE_NODISCARD NEFORCE_ALWAYS_INLINE_INLINE constexpr bool operator!=(const ports lhs, const ports rhs) noexcept {
    return lhs.port != rhs.port;
}

/** @} */ // NetworkUtil

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_UTIL_PORTS_HPP__
