#ifndef NEFORCE_NETWORK_UTIL_IP_ADDRESS_HPP__
#define NEFORCE_NETWORK_UTIL_IP_ADDRESS_HPP__

/**
 * @file ip_address.hpp
 * @brief IP地址封装类
 *
 * 此文件提供了IP地址的封装类，支持IPv4和IPv6地址。
 */

#include "NeForce/core/interface/istringify.hpp"
#include "NeForce/core/utility/optional.hpp"
#include "NeForce/core/utility/variant.hpp"
#include "NeForce/network/util/ports.hpp"
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <ws2tcpip.h>
#    ifdef max
#        undef max
#    endif
#    ifdef min
#        undef min
#    endif
#endif
#ifdef NEFORCE_PLATFORM_LINUX
#    include <netinet/in.h>
#endif
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
 * @class ip_address
 * @brief IP地址封装类
 *
 * 封装IPv4和IPv6地址，提供统一的访问接口。
 *
 * 主要功能：
 * - IPv4和IPv6地址的统一表示
 * - 地址解析
 * - 地址格式化
 * - 获取地址族、端口等信息
 * - 通配地址和回环地址生成
 * - 地址比较
 */
class NEFORCE_API ip_address : public istringify<ip_address> {
public:
    using address_type = variant<none_t, ::sockaddr_in, ::sockaddr_in6>; ///< 地址存储类型

private:
    address_type addr_; ///< 存储的地址

public:
    /**
     * @brief 默认构造函数
     *
     * 创建无效的IP地址对象。
     */
    ip_address() noexcept = default;

    /**
     * @brief 从IPv4地址构造
     * @param addr4 IPv4地址结构
     */
    explicit ip_address(const ::sockaddr_in& addr4) noexcept :
    addr_(addr4) {}

    /**
     * @brief 从IPv6地址构造
     * @param addr6 IPv6地址结构
     */
    explicit ip_address(const ::sockaddr_in6& addr6) noexcept :
    addr_(addr6) {}

    ip_address(const ip_address& other) noexcept = default;
    ip_address& operator=(const ip_address& other) noexcept = default;

    ip_address(ip_address&& other) noexcept = default;
    ip_address& operator=(ip_address&& other) noexcept = default;

    /**
     * @brief 检查地址是否有效
     * @return 有效返回true
     */
    NEFORCE_NODISCARD bool is_valid() const noexcept { return !addr_.holds_alternative<none_t>(); }

    /**
     * @brief 检查是否为IPv4地址
     * @return IPv4返回true
     */
    NEFORCE_NODISCARD bool is_ipv4() const noexcept { return addr_.holds_alternative<::sockaddr_in>(); }

    /**
     * @brief 检查是否为IPv6地址
     * @return IPv6返回true
     */
    NEFORCE_NODISCARD bool is_ipv6() const noexcept { return addr_.holds_alternative<::sockaddr_in6>(); }

    /**
     * @brief 获取通配地址
     * @param port 端口号
     * @param family 地址族，默认为IPv4
     * @return 通配地址对象
     *
     * 通配地址用于绑定到所有网络接口。
     */
    NEFORCE_NODISCARD static ip_address any(ports port = ports::UNDEF, int family = AF_INET) noexcept;

    /**
     * @brief 获取回环地址
     * @param port 端口号
     * @param family 地址族（AF_INET或AF_INET6），默认为IPv4
     * @return 回环地址对象
     */
    NEFORCE_NODISCARD static ip_address loopback(ports port = ports::UNDEF, int family = AF_INET) noexcept;

    /**
     * @brief 获取底层指针
     * @return sockaddr指针，无效地址返回nullptr
     */
    NEFORCE_NODISCARD const ::sockaddr* data() const noexcept;

    /**
     * @brief 获取底层指针
     * @return sockaddr指针，无效地址返回nullptr
     */
    NEFORCE_NODISCARD ::sockaddr* data() noexcept;

    /**
     * @brief 获取地址结构大小
     * @return 字节数，无效地址返回0
     */
    NEFORCE_NODISCARD int size() const noexcept;

    /**
     * @brief 获取内部地址存储的常量引用
     * @return variant地址存储
     */
    NEFORCE_NODISCARD const address_type& address() const noexcept { return addr_; }

    /**
     * @brief 获取地址族
     * @return 地址族
     */
    NEFORCE_NODISCARD int family() const noexcept;

    /**
     * @brief 获取端口号
     * @return 端口号
     */
    NEFORCE_NODISCARD ports port() const noexcept;

    /**
     * @brief 转换为字符串表示
     * @return 格式化的IP:端口字符串
     *
     * IPv4格式：192.168.1.1:8080
     * IPv6格式：[2001:db8::1]:443
     */
    NEFORCE_NODISCARD string to_string() const;

    /**
     * @brief 从字符串解析IP地址
     * @param host IP地址字符串
     * @param port 端口号
     * @return 解析成功返回ip_address对象，失败返回none
     *
     * 支持IPv4点分十进制和IPv6十六进制格式。
     * 不支持域名解析（如有需要请使用dns_client）。
     */
    NEFORCE_NODISCARD static optional<ip_address> parse(const string& host, ports port = ports{}) noexcept;

    /**
     * @brief 相等比较运算符
     * @param other 另一个IP地址
     * @return 地址和端口都相等返回true
     *
     * 比较IP地址和端口，不比较地址族以外的字段。
     */
    NEFORCE_NODISCARD bool operator==(const ip_address& other) const;

    /**
     * @brief 不等比较运算符
     * @param other 另一个IP地址
     * @return 不相等返回true
     */
    NEFORCE_NODISCARD bool operator!=(const ip_address& other) const { return !(*this == other); }
};

/** @} */ // NetworkUtil

/** @} */ // Network

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_UTIL_IP_ADDRESS_HPP__
