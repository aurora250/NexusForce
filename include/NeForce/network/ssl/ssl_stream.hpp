#ifndef NEFORCE_NETWORK_SSL_SSL_STREAM_HPP__
#define NEFORCE_NETWORK_SSL_SSL_STREAM_HPP__

/**
 * @file ssl_stream.hpp
 * @brief SSL/TLS流封装
 *
 * 此文件提供了SSL/TLS流的封装类，用于在已建立的TCP连接上
 * 进行SSL/TLS加密通信。支持服务器端和客户端模式。
 */

#include "NeForce/network/ssl/ssl_context.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup SSL SSL/TLS
 * @{
 */

/**
 * @class ssl_stream
 * @brief SSL/TLS流封装类
 *
 * 提供加密通信流的接口。
 * 需要先建立TCP连接，然后在连接上建立SSL/TLS会话。
 *
 * 主要功能：
 * - SSL/TLS握手
 * - 加密数据读写
 * - 证书验证
 * - SNI（服务器名称指示）支持
 * - 密码套件和协议版本查询
 * - 非阻塞I/O支持
 */
class NEFORCE_API ssl_stream {
public:
    /**
     * @brief 原生文件描述符类型
     */
    using native_handle_type =
#ifdef NEFORCE_PLATFORM_WINDOWS
            uintptr_t;
#else
            int;
#endif

private:
    struct ssl_deleter {
        void operator()(::SSL* ssl) const noexcept {
            if (ssl != nullptr) {
                ::SSL_free(ssl);
            }
        }
    };

    struct x509_deleter {
        void operator()(::X509* cert) const noexcept {
            if (cert != nullptr) {
                ::X509_free(cert);
            }
        }
    };
    using x509_ptr = unique_ptr<::X509, x509_deleter>;

    unique_ptr<::SSL, ssl_deleter> ssl_; ///< SSL对象
    string last_error_;                  ///< 最后错误信息

    void handle_ssl_error(int ret, const char* operation);

public:
    /**
     * @brief 默认构造函数
     *
     * 创建未初始化的SSL流对象。
     */
    ssl_stream() = default;

    /**
     * @brief 从SSL上下文构造
     * @param ctx SSL上下文
     * @throws ssl_exception SSL对象创建失败时抛出
     */
    explicit ssl_stream(const ssl_context& ctx) { reset(ctx); }

    ssl_stream(ssl_stream&& other) noexcept = default;
    ssl_stream& operator=(ssl_stream&& other) noexcept = default;

    /**
     * @brief 重置SSL流
     * @param ctx SSL上下文
     * @throws ssl_exception SSL对象创建失败时抛出
     *
     * 创建新的SSL对象并关联到指定的上下文。
     * 如果已有SSL对象，会先关闭并释放。
     */
    void reset(const ssl_context& ctx);

    /**
     * @brief 设置底层socket文件描述符
     * @param fd socket文件描述符
     * @throws ssl_exception 设置失败时抛出
     * @throws value_exception 文件描述符无效时抛出
     *
     * 将SSL对象关联到已连接的TCP socket。
     * 必须在握手之前调用。
     */
    void set_fd(native_handle_type fd);

    /**
     * @brief 执行服务器端TLS握手
     * @throws ssl_exception 握手失败时抛出
     *
     * 作为服务器端接受TLS连接，执行SSL/TLS握手。
     * 需要在set_fd()之后调用。
     */
    void accept();

    /**
     * @brief 执行客户端TLS握手
     * @return 握手成功返回true
     * @throws ssl_exception 握手失败时抛出
     *
     * 作为客户端发起TLS连接，执行SSL/TLS握手。
     * 需要在set_fd()和set_sni_hostname()（可选）之后调用。
     */
    bool connect();

    /**
     * @brief 关闭SSL连接
     *
     * 发送close_notify告警，释放SSL对象。
     * 底层的socket不会关闭，需要单独关闭。
     */
    void close() noexcept;

    /**
     * @brief 读取加密数据
     * @param buffer 接收缓冲区
     * @param size 要读取的字节数
     * @return 实际读取的字节数，-1表示错误，0表示需要重试（非阻塞模式）
     *
     * 从SSL连接读取解密后的数据。
     * 非阻塞模式下，如果没有数据可读返回0。
     */
    ssize_t read(void* buffer, size_t size);

    /**
     * @brief 写入加密数据
     * @param buffer 要发送的数据
     * @param size 数据大小
     * @return 实际写入的字节数，-1表示错误，0表示需要重试（非阻塞模式）
     *
     * 加密并发送数据到SSL连接。
     * 非阻塞模式下，如果无法立即发送返回0。
     */
    ssize_t write(const void* buffer, size_t size);

    /**
     * @brief 读取所有可用数据
     * @param max_size 最大读取字节数
     * @return 读取的数据
     * @throws ssl_exception 读取失败时抛出
     *
     * 持续读取直到连接关闭或达到最大大小。
     * 适用于阻塞模式下的完整响应读取。
     */
    vector<char> read_all(size_t max_size = 8192);

    /**
     * @brief 写入所有数据
     * @param data 要发送的数据
     * @param size 数据大小
     * @return 成功返回true
     *
     * 持续写入直到所有数据发送完毕。
     * 适用于阻塞模式下的完整数据发送。
     */
    bool write_all(const void* data, size_t size);

    /**
     * @brief 获取缓冲区中可读字节数
     * @return 可读字节数
     *
     * 返回SSL对象内部缓冲区中已解密但未读取的字节数。
     */
    NEFORCE_NODISCARD int pending() const;

    /**
     * @brief 设置SNI主机名（客户端）
     * @param hostname 服务器主机名
     * @throws ssl_exception 设置失败时抛出
     * @throws value_exception 主机名为空时抛出
     *
     * 在客户端TLS握手中设置服务器名称指示（SNI）扩展。
     * 用于指示客户端想要连接的服务器名称。
     */
    void set_sni_hostname(const string& hostname);

    /**
     * @brief 获取对等方证书
     * @return X509证书指针
     *
     * 获取TLS握手时对等方提供的证书。
     */
    NEFORCE_NODISCARD x509_ptr get_peer_certificate() const;

    /**
     * @brief 验证对等方证书
     * @return 验证通过返回true
     *
     * 检查对等方证书是否有效（未过期、链完整、信任CA）。
     * 需要在握手完成后调用。
     */
    NEFORCE_NODISCARD bool verify_peer() const;

    /**
     * @brief 获取密码套件名称
     * @return 密码套件名称
     *
     * 返回当前TLS连接使用的密码套件名称。
     */
    NEFORCE_NODISCARD string get_cipher_name() const;

    /**
     * @brief 获取TLS协议版本
     * @return 协议版本字符串（如"TLSv1.2"）
     */
    NEFORCE_NODISCARD string get_version() const;

    /**
     * @brief 获取ALPN协商的协议名称
     * @return 协议名称字符串（如"h2"、"http/1.1"），未协商返回空字符串
     *
     * 返回TLS握手期间通过ALPN（Application-Layer Protocol Negotiation）
     * 协商确定的应用层协议。需要在TLS握手（accept/connect）完成后调用。
     */
    NEFORCE_NODISCARD string get_alpn_negotiated() const;

    /**
     * @brief 获取最后错误信息
     * @return 错误描述字符串
     */
    NEFORCE_NODISCARD const string& last_error() const { return last_error_; }

    /**
     * @brief 检查SSL对象是否有效
     * @return 有效返回true
     */
    NEFORCE_NODISCARD bool is_valid() const noexcept { return ssl_ != nullptr; }

    /**
     * @brief 布尔转换运算符
     * @return 有效返回true
     */
    explicit operator bool() const noexcept { return is_valid(); }

    /**
     * @brief 获取原生SSL对象指针
     * @return SSL指针
     */
    NEFORCE_NODISCARD ::SSL* native_handle() const noexcept { return ssl_.get(); }

    /**
     * @brief 释放SSL对象所有权
     * @return SSL指针
     *
     * 释放SSL对象的所有权，调用方负责释放。
     */
    NEFORCE_NODISCARD ::SSL* release() noexcept { return ssl_.release(); }
};

/** @} */ // SSL/TLS

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_SSL_SSL_STREAM_HPP__
