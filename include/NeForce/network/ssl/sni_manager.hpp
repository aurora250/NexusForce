#ifndef NEFORCE_NETWORK_SSL_SNI_MANAGER_HPP__
#define NEFORCE_NETWORK_SSL_SNI_MANAGER_HPP__

/**
 * @file sni_manager.hpp
 * @brief SNI 证书管理器
 *
 * 支持为不同域名配置不同的 SSL 证书。
 * 在 TLS 握手时根据客户端 SNI hostname 自动选择合适的 SSL_CTX。
 */

#include "NeForce/core/container/unordered_map.hpp"
#include "NeForce/network/ssl/ssl_context.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup SSL SSL/TLS
 * @{
 */

/**
 * @class sni_manager
 * @brief SNI (Server Name Indication) 证书管理器
 *
 * 支持为不同域名配置不同的SSL证书。
 * 在TLS握手时根据客户端SNI hostname自动选择合适的SSL_CTX。
 *
 * 使用示例：
 * @code
 * sni_manager sni;
 * sni.add_host("example.com", ssl_context(cert1, key1));
 * sni.add_host("api.example.com", ssl_context(cert2, key2));
 * sni.set_default_context(ssl_context(default_cert, default_key));
 *
 * // 在SSL握手回调中使用:
 * SSL_CTX* ctx = sni.select_context(servername);
 * @endcode
 */
class NEFORCE_API sni_manager {
private:
    /// @brief hostname → SSL 上下文映射
    unordered_map<string, ssl_context> hosts_;
    /// @brief 默认 SSL 上下文（SNI 未匹配时使用）
    ssl_context default_context_;
    /// @brief 是否已设置默认上下文
    bool has_default_ = false;

public:
    sni_manager() = default;

    /**
     * @brief 添加域名及其SSL上下文
     * @param hostname 域名
     * @param ctx SSL上下文
     */
    void add_host(const string& hostname, ssl_context ctx);

    /**
     * @brief 移除域名
     * @param hostname 域名
     */
    void remove_host(const string& hostname);

    /**
     * @brief 设置默认SSL上下文（SNI未匹配时使用）
     * @param ctx 默认SSL上下文
     */
    void set_default_context(ssl_context ctx);

    /**
     * @brief 根据SNI hostname选择SSL_CTX
     * @param server_name 客户端发送的SNI hostname
     * @return 匹配的SSL_CTX指针，未找到返回默认上下文或nullptr
     *
     * 匹配规则：
     * 1. 精确匹配 hostname
     * 2. 通配符匹配 *.domain.com
     * 3. 返回默认上下文
     */
    NEFORCE_NODISCARD void* select_ssl_ctx(const string& server_name) const;

    /**
     * @brief 检查是否有指定域名的证书
     */
    NEFORCE_NODISCARD bool has_host(const string& hostname) const;

    /**
     * @brief 获取配置的域名数量
     */
    NEFORCE_NODISCARD size_t host_count() const noexcept { return hosts_.size(); }

    /**
     * @brief SNI回调函数（用于SSL_CTX_set_tlsext_servername_callback）
     *
     * 此静态函数可作为OpenSSL的SNI回调，自动查询sni_manager实例。
     * 通过SSL_CTX_set_tlsext_servername_arg设置manager指针。
     */
    static int sni_callback(void* ssl, int* alert, void* arg);
};

/** @} */ // SSL/TLS

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_SSL_SNI_MANAGER_HPP__
