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
 * @struct ports
 * @brief 网络端口封装类
 *
 * 提供类型安全的端口表示，封装了常见协议的端口号，支持从协议名称解析和反向转换。
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

/** @} */ // Network

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_PORTS_HPP__
