#ifndef NEFORCE_NETWORK_SSL_SSL_CONTEXT_HPP__
#define NEFORCE_NETWORK_SSL_SSL_CONTEXT_HPP__

/**
 * @file ssl_context.hpp
 * @brief SSL/TLS上下文管理
 *
 * 此文件提供了SSL/TLS上下文的封装类，用于管理SSL连接的配置和状态。
 * 基于OpenSSL库实现，支持TLS服务器和客户端模式。
 */

#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/memory/shared_ptr.hpp"
#include "NeForce/network/ssl/ssl_exception.hpp"
#include <openssl/ssl.h>
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup SSL SSL/TLS
 * @brief SSL/TLS相关组件
 * @{
 */

/**
 * @enum ssl_method
 * @brief SSL/TLS方法类型
 *
 * 定义SSL上下文的角色和协议类型。
 */
enum class ssl_method {
    TLS_SERVER,      ///< TLS服务器端
    TLS_CLIENT,      ///< TLS客户端端
    TLS_SERVER_DTLS, ///< DTLS服务器端（数据报TLS）
    TLS_CLIENT_DTLS  ///< DTLS客户端端（数据报TLS）
};

/**
 * @class ssl_context
 * @brief SSL/TLS上下文管理类
 *
 * 封装OpenSSL的SSL_CTX对象，提供SSL连接的配置和管理功能。
 * 支持证书加载、验证配置、密码套件设置等。
 *
 * 主要功能：
 * - SSL上下文创建和配置
 * - 证书和私钥加载
 * - CA证书验证配置
 * - 密码套件和协议版本设置
 * - ALPN（应用层协议协商）支持
 * - 会话缓存和超时配置
 *
 * @note 上下文对象是线程安全的，可以被多个SSL连接共享。
 */
class NEFORCE_API ssl_context {
private:
    struct ctx_deleter {
        void operator()(::SSL_CTX* ctx) const noexcept {
            if (ctx != nullptr) {
                ::SSL_CTX_free(ctx);
            }
        }
    };

    unique_ptr<::SSL_CTX, ctx_deleter> ctx_; ///< OpenSSL SSL_CTX对象
    ssl_method method_;                      ///< 记录创建时使用的方法
    bool cert_loaded_{false};                ///< 证书是否已加载

    ssl_context(ssl_method method, ssl_context* /*tag*/) :
    method_(method) {}

public:
    /**
     * @brief 构造函数
     * @param method SSL/TLS方法类型（默认为TLS服务器）
     * @throws ssl_exception SSL上下文创建失败时抛出
     *
     * 创建SSL上下文并设置基本选项：
     * - 禁用SSLv2/SSLv3（不安全）
     * - 设置默认密码套件
     * - 设置默认CA验证路径
     */
    explicit ssl_context(ssl_method method = ssl_method::TLS_SERVER);

    ~ssl_context() = default;

    ssl_context(const ssl_context&) = delete;
    ssl_context& operator=(const ssl_context&) = delete;

    ssl_context(ssl_context&& other) noexcept = default;
    ssl_context& operator=(ssl_context&& other) noexcept = default;

    /**
     * @brief 克隆当前SSL上下文
     * @return 新的ssl_context实例
     * @throws ssl_exception 克隆失败时抛出
     * @note 克隆的上下文与原上下文共享同一底层SSL_CTX，对任意一个的配置修改将影响所有克隆实例。
     */
    NEFORCE_NODISCARD ssl_context clone() const;

    /**
     * @brief 克隆为共享指针
     * @return 克隆的上下文共享指针
     * @throws ssl_exception 克隆失败时抛出
     */
    NEFORCE_NODISCARD shared_ptr<ssl_context> clone_shared() const;

    /**
     * @brief 加载证书和私钥（从文件）
     * @param cert_file 证书文件路径（PEM格式）
     * @param key_file 私钥文件路径（PEM格式）
     * @return 加载成功返回true
     *
     * 加载服务器或客户端的证书和对应的私钥。
     * 会自动检查私钥是否与证书匹配。
     */
    bool load_certificate(const string& cert_file, const string& key_file);

    /**
     * @brief 加载证书和私钥（从内存）
     * @param cert_pem 证书PEM数据
     * @param key_pem 私钥PEM数据
     * @throws ssl_exception SSL上下文为空或解析、设置失败时抛出
     * @throws value_exception 证书或密钥数据为空时抛出
     *
     * 从内存中的PEM数据加载证书和私钥，适用于嵌入式证书。
     */
    void load_certificate_from_memory(const string& cert_pem, const string& key_pem);

    /**
     * @brief 加载CA证书用于验证
     * @param ca_file CA证书文件路径
     * @param ca_path CA证书目录路径（可选）
     * @return 加载成功返回true
     *
     * 设置用于验证对等方证书的CA证书。
     * 可以指定单个文件或目录（目录中的证书会被自动加载）。
     */
    bool load_verify_locations(const string& ca_file, const string& ca_path = "");

    /**
     * @brief 设置SSL选项
     * @param options OpenSSL SSL_OP_*标志位组合
     * @throws ssl_exception SSL上下文为空时抛出
     *
     * 设置SSL上下文的选项，如禁用特定协议版本、启用特定特性等。
     */
    void set_options(long options);

    /**
     * @brief 设置验证模式
     * @param mode OpenSSL SSL_VERIFY_*模式
     * @throws ssl_exception SSL上下文为空时抛出
     *
     * 设置对等方证书的验证模式。
     * 常用模式：
     * - SSL_VERIFY_NONE：不验证客户端证书
     * - SSL_VERIFY_PEER：验证客户端证书
     * - SSL_VERIFY_FAIL_IF_NO_PEER_CERT：客户端必须提供证书
     */
    void set_verify_mode(int mode);

    /**
     * @brief 要求客户端提供证书
     * @throws ssl_exception SSL上下文为空时抛出
     *
     * 服务器端使用，要求客户端必须提供有效证书。
     */
    void require_client_certificate();

    /**
     * @brief 设置密码套件列表（TLS 1.2及以下）
     * @param ciphers OpenSSL密码套件字符串
     * @throws ssl_exception SSL上下文为空或设置失败时抛出
     *
     * 设置可用的密码套件列表，格式为OpenSSL密码套件字符串。
     * 例如："ECDHE-RSA-AES128-GCM-SHA256:ECDHE-RSA-AES256-GCM-SHA384"
     */
    void set_cipher_list(const string& ciphers);

    /**
     * @brief 设置密码套件列表（TLS 1.3）
     * @param ciphersuites OpenSSL TLS 1.3密码套件字符串
     * @throws ssl_exception SSL上下文为空或设置失败时抛出
     *
     * 设置TLS 1.3的密码套件列表。
     * 例如："TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256"
     */
    void set_ciphersuites(const string& ciphersuites);

    /**
     * @brief 设置安全的默认选项
     * @throws ssl_exception SSL上下文为空时抛出
     *
     * 配置推荐的现代安全选项：
     * - 禁用SSLv2/SSLv3/TLSv1/TLSv1.1
     * - 优先使用服务器密码套件
     * - 禁用压缩
     * - 设置最小TLS版本为1.2
     * - 设置安全的密码套件列表
     */
    void set_default_options();

    /**
     * @brief 设置会话缓存大小
     * @param size 缓存大小（会话数）
     * @throws ssl_exception SSL上下文为空时抛出
     *
     * 设置SSL会话缓存的最大会话数。
     * 会话缓存可以提高重复连接的性能。
     */
    void set_session_cache_size(long size);

    /**
     * @brief 设置会话超时时间
     * @param seconds 超时时间
     * @throws ssl_exception SSL上下文为空时抛出
     *
     * 设置SSL会话缓存中会话的超时时间。
     */
    void set_timeout(long seconds);

    /**
     * @brief 设置ALPN协议列表
     * @param protocols 协议名称列表
     * @throws ssl_exception SSL上下文为空或设置失败时抛出
     * @throws value_exception 协议名称长度无效时抛出
     *
     * 设置应用层协议协商（ALPN）支持的协议列表。
     * 用于在TLS握手时协商应用层协议（如HTTP/2）。
     */
    void set_alpn_protos(const vector<string>& protocols);

    /**
     * @brief 获取原生SSL_CTX句柄
     * @return SSL_CTX指针
     */
    NEFORCE_NODISCARD ::SSL_CTX* native_handle() const noexcept { return ctx_.get(); }

    /**
     * @brief 布尔转换运算符
     * @return 上下文有效返回true
     */
    explicit operator bool() const noexcept { return ctx_ != nullptr; }

    /**
     * @brief 检查上下文是否有效
     * @return 有效返回true
     */
    NEFORCE_NODISCARD bool is_valid() const noexcept { return ctx_ != nullptr; }

    /**
     * @brief 检查是否已加载证书
     * @return 已加载证书返回true
     */
    NEFORCE_NODISCARD bool has_certificate() const noexcept { return cert_loaded_; }
};

/** @} */ // SSL/TLS

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_SSL_SSL_CONTEXT_HPP__
