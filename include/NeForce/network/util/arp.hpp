#ifndef NEFORCE_NETWORK_UTIL_ARP_HPP__
#define NEFORCE_NETWORK_UTIL_ARP_HPP__

/**
 * @file arp.hpp
 * @brief ARP实现
 *
 * 此文件提供了ARP协议的功能实现，用于将IP地址解析为MAC地址，
 * 通过发送ARP请求并接收ARP应答来获取目标主机的MAC地址。
 */

#include "NeForce/core/time/duration.hpp"
#include "NeForce/network/util/mac_address.hpp"
#ifdef NEFORCE_PLATFORM_LINUX
#    include "NeForce/network/socket_base.hpp"
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
 * @class arp
 * @brief ARP协议实现类
 *
 * 提供ARP地址解析功能，将IPv4地址转换为MAC地址。
 * 需要在本地网络中有效，目标主机必须在线且可达。
 *
 * 主要功能：
 * - 从IP地址解析MAC地址
 * - 支持超时控制
 * - 支持指定网络接口
 */
class NEFORCE_API arp {
private:
    string iface_; ///< 网络接口名称

#ifdef NEFORCE_PLATFORM_WINDOWS
    bool opened_ = false; ///< 是否已打开
#else
    socket_base sock_;      ///< 原始套接字
    mac_address local_mac_; ///< 本地网卡MAC地址
    uint32_t local_ip_;     ///< 本地网卡IP地址
    int ifindex_ = -1;      ///< 网络接口索引
#endif

#ifdef NEFORCE_PLATFORM_LINUX
    bool local_info(const char* iface);
#endif

public:
    /**
     * @brief 默认构造函数
     */
    arp() = default;

    /**
     * @brief 析构函数
     */
    ~arp() = default;

    /**
     * @brief 打开ARP解析器
     * @param iface 网络接口名称
     * @return 成功返回true
     *
     * 初始化ARP解析器，获取本地网络接口信息。
     */
    bool open(const char* iface = nullptr);

    /**
     * @brief 关闭ARP解析器
     */
    void close() noexcept;

    /**
     * @brief 解析IP地址对应的MAC地址
     * @param target 目标IPv4地址
     * @param timeout 超时时间（毫秒），默认1000ms
     * @return 成功返回MAC地址，失败返回none
     *
     * 发送ARP请求并等待ARP应答，获取目标IP的MAC地址。
     * 如果超时未收到应答，返回none。
     */
    optional<mac_address> resolve(const ip_address& target, milliseconds timeout = milliseconds(1000));
};

/** @} */ // NetworkUtil

/** @} */ // Network

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_UTIL_ARP_HPP__
