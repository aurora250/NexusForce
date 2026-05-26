#ifndef NEFORCE_NETWORK_UTIL_MAC_ADDRESS_HPP__
#define NEFORCE_NETWORK_UTIL_MAC_ADDRESS_HPP__

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
 * @addtogroup NetworkUtil 网络工具
 * @{
 */

/**
 * @class mac_address
 * @brief MAC地址封装类
 *
 * 支持MAC地址的解析、格式化和通过ARP协议从IP地址获取MAC地址。
 *
 * @section standards 遵循的国际标准
 * 本实现严格遵循以下网络寻址与协议相关标准规范：
 *
 * **MAC 地址标准（IEEE 802 系列）：**
 * - **IEEE 802-2014**：IEEE 局域网和城域网标准 — 概览与架构
 *   https://standards.ieee.org/ieee/802/3714/
 * - **IEEE 802.3-2022**：以太网标准（MAC 地址格式定义）
 *   https://standards.ieee.org/ieee/802.3/10422/
 *
 * **地址表示格式标准：**
 * - **IEEE 802.3 §3.2.4**：MAC 地址的十六进制表示规范
 * - **IETF RFC 7042**：IANA 参数与 IETF 协议使用中的常见 MAC 地址约定
 *   https://www.rfc-editor.org/rfc/rfc7042.html
 *
 * @section mac_address_structure MAC 地址结构
 * 根据 IEEE 802.3，MAC 地址（48 位 EUI-48）由以下部分组成：
 *
 * | 字段         | 位数 | 说明                                           |
 * |--------------|------|------------------------------------------------|
 * | OUI          | 24   | 组织唯一标识符，由 IEEE 分配给制造商           |
 * | NIC Specific | 24   | 网络接口卡特定部分，由制造商分配               |
 *
 * **地址类型标识**（第一个字节的最低位）：
 * | 位值 | 类型           | 说明                           |
 * |------|----------------|--------------------------------|
 * | 0    | 单播地址       | 发送到单个网络接口             |
 * | 1    | 多播地址       | 发送到一组网络接口             |
 *
 * **本地管理标识**（第一个字节的次低位）：
 * | 位值 | 类型           | 说明                           |
 * |------|----------------|--------------------------------|
 * | 0    | 全局唯一地址   | IEEE 分配的 OUI               |
 * | 1    | 本地管理地址   | 由网络管理员分配               |
 *
 * @section mac_address_format MAC 地址表示格式
 * 根据 IEEE 802.3 §3.2.4 和 RFC 7042，MAC 地址的标准表示为：
 * - 6 组十六进制数字，每组 2 位
 * - 常用分隔符：冒号（:）、连字符（-）、点号（.）或无分隔符
 * - 字母使用大写（推荐）或小写
 *
 * **支持的格式示例**：
 * | 格式               | 示例                   | 说明               |
 * |--------------------|------------------------|--------------------|
 * | 冒号分隔（推荐）   | 00:11:22:33:44:55      | 本实现默认格式     |
 * | 连字符分隔         | 00-11-22-33-44-55      | 常见于某些操作系统 |
 * | 无分隔符           | 001122334455           | 紧凑格式           |
 *
 * @note MAC 地址格式遵循 IEEE 802.3 标准，通常表示为 6 组十六进制数字。
 *       ARP 查询仅在本地网络有效，且需要目标主机在线。
 *
 * @warning ARP 查询需要适当的系统权限（在某些系统上可能需要 root/管理员权限）。
 *          跨网段的 IP 地址无法通过 ARP 解析 MAC 地址（需要使用默认网关的 MAC 地址）。
 *          ARP 协议无认证机制，存在 ARP 欺骗攻击风险。
 *
 * @see https://standards.ieee.org/products-services/regauth/
 * @see https://www.rfc-editor.org/rfc/rfc826.html
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
    NEFORCE_NODISCARD string to_string() const;

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
    NEFORCE_NODISCARD const bytes_type& bytes() const noexcept { return bytes_; }
};

/** @} */ // NetworkUtil

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_UTIL_MAC_ADDRESS_HPP__
