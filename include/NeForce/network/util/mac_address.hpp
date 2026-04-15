#ifndef NEFORCE_NETWORK_MAC_ADDRESS_HPP__
#define NEFORCE_NETWORK_MAC_ADDRESS_HPP__

/**
 * @file mac_address.hpp
 * @brief MAC地址封装类
 *
 * 此文件提供了MAC地址的封装类，支持MAC地址的解析、格式化和
 * 通过ARP协议从IP地址获取MAC地址。
 */

#include "NeForce/core/container/array.hpp"
#include "NeForce/network/util/ip_address.hpp"
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
 * @class mac_address
 * @brief MAC地址封装类
 *
 * 封装6字节的MAC地址，提供地址解析、格式化和ARP查询功能。
 *
 * MAC地址格式：XX:XX:XX:XX:XX:XX（6组十六进制数，每组2位）
 *
 * 主要功能：
 * - MAC地址字符串解析
 * - MAC地址格式化输出
 * - 通过ARP协议从IP地址获取MAC地址
 */
class NEFORCE_API mac_address : public istringify<mac_address> {
public:
    static constexpr size_t MAC_LEN = 6;       ///< MAC地址字节长度
    using bytes_type = array<byte_t, MAC_LEN>; ///< MAC地址字节数组类型

private:
    bytes_type bytes_{}; ///< 存储的MAC地址字节

public:
    /**
     * @brief 默认构造函数
     *
     * 创建全零的MAC地址。
     */
    mac_address() noexcept = default;

    /**
     * @brief 从字节数组构造
     * @param bytes 指向6字节数据的指针
     */
    explicit mac_address(const byte_t* bytes) noexcept { copy(bytes, bytes + MAC_LEN, bytes_.begin()); }

    /**
     * @brief 从字节数组容器构造
     * @param bytes MAC地址字节数组
     */
    explicit mac_address(const bytes_type& bytes) noexcept :
    bytes_(bytes) {}

    /**
     * @brief 从字符串解析MAC地址
     * @param str MAC地址字符串
     * @return 解析成功返回MAC地址对象，失败返回none
     *
     * 支持的格式：
     * - "00:11:22:33:44:55"（冒号分隔）
     * - "00-11-22-33-44-55"（连字符分隔）
     */
    static optional<mac_address> parse(string_view str);

    /**
     * @brief 从IP地址获取MAC地址
     * @param ip IP地址对象
     * @param iface 网络接口名称（Linux可选）
     * @return 成功返回MAC地址，失败返回none
     *
     * 通过ARP查询指定IP对应的MAC地址。
     *
     * @note 仅在本地网络中有效，需要目标主机在线且可达。
     * @note 此操作可能需要适当的系统权限。
     */
    static optional<mac_address> parse(const ip_address& ip, const char* iface = nullptr);

    /**
     * @brief 转换为字符串表示
     * @return 格式化的MAC地址字符串
     */
    string to_string() const;

    /**
     * @brief 相等比较运算符
     * @param other 另一个MAC地址
     * @return 地址相同返回true
     */
    bool operator==(const mac_address& other) const noexcept { return bytes_ == other.bytes_; }

    /**
     * @brief 不等比较运算符
     * @param other 另一个MAC地址
     * @return 地址不同返回true
     */
    bool operator!=(const mac_address& other) const noexcept { return !(*this == other); }

    /**
     * @brief 获取MAC地址字节数组
     * @return 6字节数组的常量引用
     */
    const bytes_type& bytes() const noexcept { return bytes_; }
};

/** @} */ // NetworkUtil

/** @} */ // Network

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_MAC_ADDRESS_HPP__
