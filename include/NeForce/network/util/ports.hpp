#ifndef NEFORCE_NETWORK_PORTS_HPP__
#define NEFORCE_NETWORK_PORTS_HPP__

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
 * @defgroup Network 网络通信
 * @brief 网络通信相关组件
 * @{
 */

/**
 * @defgroup NetworkUtil 网络通信工具
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
 * | SSH           | 22     | RFC 4251 §4.1（安全 Shell 协议架构）                   | 安全 Shell 协议          |
 * | Telnet        | 23     | RFC 854（Telnet 协议规范）                             | 远程登录协议             |
 * | SMTP          | 25     | RFC 5321 §4.5.3.2（简单邮件传输协议）                  | 电子邮件传输             |
 * | DNS           | 53     | RFC 1035 §4.2（域名实现与规范）                        | 域名系统                 |
 * | TFTP          | 69     | RFC 1350 §5（简单文件传输协议）                        | 简单文件传输协议         |
 * | POP3          | 110    | RFC 1939 §4（邮局协议版本3）                           | 电子邮件接收协议         |
 * | IMAP          | 143    | RFC 3501 §2.1（互联网消息访问协议）                    | 电子邮件访问协议         |
 *
 * **WebSocket 协议端口规范：**
 * | 协议          | 端口号 | 标准引用                                               | 说明                     |
 * |---------------|--------|--------------------------------------------------------|--------------------------|
 * | WS            | 80     | RFC 6455 §11.1（WebSocket 协议）                       | WebSocket over HTTP      |
 * | WSS           | 443    | RFC 6455 §11.1（WebSocket 协议）                       | WebSocket over TLS/SSL   |
 *
 * **相关安全标准：**
 * - **IETF RFC 2818**：HTTP Over TLS（HTTPS 端口定义）
 *   https://www.rfc-editor.org/rfc/rfc2818.html
 * - **IETF RFC 6455**：WebSocket 协议（WS/WSS 端口定义）
 *   https://www.rfc-editor.org/rfc/rfc6455.html
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
 * 本文件中定义的所有端口均属于知名端口（0-1023）范围。
 *
 * @section protocol_mapping 协议名称与端口映射
 * 本实现支持的协议名称到端口号的映射关系：
 *
 * | 协议名称 | 端口号 | 别名          | 传输协议 | 说明                     |
 * |----------|--------|---------------|----------|--------------------------|
 * | http     | 80     | -             | TCP      | 超文本传输协议           |
 * | ws       | 80     | http（端口相同）| TCP    | WebSocket 协议           |
 * | https    | 443    | -             | TCP      | HTTP over TLS/SSL        |
 * | wss      | 443    | https（端口相同）| TCP    | WebSocket Secure         |
 * | ftp      | 21     | -             | TCP      | 文件传输协议             |
 * | tftp     | 69     | -             | UDP      | 简单文件传输协议         |
 * | ssh      | 22     | -             | TCP      | 安全 Shell               |
 * | telnet   | 23     | -             | TCP      | 远程登录                 |
 * | smtp     | 25     | -             | TCP      | 简单邮件传输协议         |
 * | dns      | 53     | -             | TCP/UDP  | 域名系统                 |
 * | pop3     | 110    | -             | TCP      | 邮局协议版本3            |
 * | imap     | 143    | -             | TCP      | 互联网消息访问协议       |
 *
 * @section implementation_details 实现细节
 * | 特性              | 规范参数                                  |
 * |-------------------|-------------------------------------------|
 * | 端口数值类型      | uint16_t（0-65535）                       |
 * | 知名端口范围      | 0-1023                                   |
 * | 默认端口值        | undef（0）                                |
 * | 大小              | 2 字节（uint16_t）                        |
 * | 协议名称解析      | 大小写不敏感                              |
 * | 未知协议处理      | 返回 undef                               |
 *
 * @note 本实现中定义的端口号均取自 IANA 注册表。
 *
 * @warning 端口号 80 和 443 同时服务于 HTTP/HTTPS 和 WebSocket（WS/WSS）协议。
 *          使用 `to_string(bool is_ws)` 方法可明确区分协议类型。
 *          动态/私有端口（49152-65535）不在本文件中预定义，可使用 `ports(uint16_t)` 构造。
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
        undef = 0,   ///< 未定义/无效端口
        http = 80,   ///< HTTP协议端口
        ws = 80,     ///< WebSocket协议端口（与HTTP共用）
        https = 443, ///< HTTPS协议端口
        wss = 443,   ///< WebSocket Secure端口（与HTTPS共用）
        ftp = 21,    ///< FTP协议端口
        tftp = 69,   ///< TFTP协议端口
        ssh = 22,    ///< SSH协议端口
        telnet = 23, ///< Telnet协议端口
        smtp = 25,   ///< SMTP协议端口
        dns = 53,    ///< DNS协议端口
        pop3 = 110,  ///< POP3协议端口
        imap = 143   ///< IMAP协议端口
    };

    raw port{raw::undef}; ///< 端口值

    /**
     * @brief 默认构造函数
     *
     * 创建未定义端口的对象。
     */
    constexpr ports() noexcept = default;

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
    port(static_cast<raw>(port)) {}

    /**
     * @brief 布尔转换运算符
     * @return 端口有效返回true
     *
     * 检查端口是否为undef。
     */
    constexpr explicit operator bool() const noexcept { return port != ports::undef; }

    /**
     * @brief uint16_t转换运算符
     * @return 端口的数值表示
     */
    constexpr explicit operator uint16_t() const noexcept { return static_cast<uint16_t>(port); }

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
    string to_string() const;

    /**
     * @brief 转换为协议名称字符串（支持WebSocket）
     * @param is_ws 是否为WebSocket协议
     * @return 协议名称
     *
     * 当is_ws为true时，80端口返回"ws"，443端口返回"wss"；
     * 否则返回"http"/"https"。
     */
    string to_string(bool is_ws) const;
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

/** @} */ // Network

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_PORTS_HPP__
