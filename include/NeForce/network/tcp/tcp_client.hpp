#ifndef NEFORCE_NETWORK_TCP_TCP_CLIENT_HPP__
#define NEFORCE_NETWORK_TCP_TCP_CLIENT_HPP__

/**
 * @file tcp_client.hpp
 * @brief TCP客户端实现
 *
 * 此文件提供了TCP客户端的完整实现，支持普通TCP和SSL/TLS加密连接。
 * 支持DNS解析、自动重连、超时控制、SSL证书验证等功能。
 *
 * 主要功能：
 * - TCP/SSL客户端连接
 * - DNS解析
 * - 自动重连机制
 * - 超时控制
 * - SSL/TLS支持
 * - 数据发送接收
 * - 连接状态回调
 */

#include "NeForce/network/dns/dns_client.hpp"
#include "NeForce/network/ssl/ssl_socket.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup TCP
 * @{
 */

/**
 * @class tcp_client_base
 * @brief TCP客户端基类
 *
 * 提供TCP客户端的核心功能，包括连接管理、DNS解析、
 * 数据收发和自动重连。派生类实现具体的socket创建。
 */
class NEFORCE_API tcp_client_base {
public:
    using exception_handler_t = function<void(const exception&)>;    ///< 异常处理器类型
    using connect_callback_t = function<void(const string&, ports)>; ///< 连接回调类型
    using disconnect_callback_t = function<void()>;                  ///< 断开回调类型

protected:
    dns_client dns_;                ///< DNS解析客户端
    unique_ptr<tcp_socket> socket_; ///< TCP socket

    string connected_host_;                    ///< 已连接的主机名
    ports connected_port_;                     ///< 已连接的端口
    int reconnect_attempts_ = 3;               ///< 最大重连次数
    atomic<int> current_reconnect_attempt_{0}; ///< 当前重连次数

    milliseconds connect_timeout_{5000}; ///< 连接超时
    milliseconds send_timeout_{5000};    ///< 发送超时
    milliseconds recv_timeout_{5000};    ///< 接收超时
    milliseconds reconnect_delay_{1000}; ///< 重连延迟

    bool auto_reconnect_ = false;         ///< 是否自动重连
    bool prefer_ipv6_ = false;            ///< 是否优先IPv6
    atomic<bool> is_reconnecting_{false}; ///< 是否正在重连

    connect_callback_t connect_callback_;       ///< 连接成功回调
    disconnect_callback_t disconnect_callback_; ///< 断开连接回调
    exception_handler_t exception_handler_;     ///< 异常处理器

    /**
     * @brief 创建新的socket对象
     * @return socket智能指针
     */
    virtual unique_ptr<tcp_socket> create_socket() = 0;

    /**
     * @brief 连接建立后的额外处理
     * @return 处理成功返回true
     */
    virtual bool post_connect() { return true; }

    /**
     * @brief 连接断开前的清理工作
     */
    virtual void pre_disconnect() {}

    /**
     * @brief 尝试连接到指定IP
     * @param ip IP地址
     * @param port 端口
     * @return 连接成功返回true
     */
    bool try_connect_to_ip(const string& ip, ports port);

    /**
     * @brief 需要时执行重连
     * @return 重连成功返回true
     */
    bool reconnect_if_needed();

    /**
     * @brief 处理异常
     * @param e 异常对象
     */
    NEFORCE_ALWAYS_INLINE void handle_exception(const exception& e) const {
        if (exception_handler_) {
            exception_handler_(e);
        }
    }

public:
    /**
     * @brief 默认构造函数
     */
    tcp_client_base() = default;

    /**
     * @brief 构造函数（带DNS客户端）
     * @param dns DNS客户端
     */
    explicit tcp_client_base(dns_client dns) :
    dns_(_NEFORCE move(dns)) {}

    /**
     * @brief 析构函数
     */
    virtual ~tcp_client_base() { disconnect(); }

    tcp_client_base(const tcp_client_base&) = delete;
    tcp_client_base& operator=(const tcp_client_base&) = delete;
    tcp_client_base(tcp_client_base&&) noexcept = default;
    tcp_client_base& operator=(tcp_client_base&&) noexcept = default;

    /**
     * @brief 设置连接超时时间
     * @param timeout 超时时间（毫秒）
     * @throws value_exception timeout为0或负数时抛出
     */
    void set_connect_timeout(milliseconds timeout);

    /**
     * @brief 获取连接超时时间
     * @return 超时时间
     */
    NEFORCE_NODISCARD milliseconds connect_timeout() const noexcept { return connect_timeout_; }

    /**
     * @brief 设置发送超时时间
     * @param timeout 超时时间（毫秒）
     * @throws value_exception timeout为0或负数时抛出
     */
    void set_send_timeout(milliseconds timeout);

    /**
     * @brief 获取发送超时时间
     * @return 超时时间
     */
    NEFORCE_NODISCARD milliseconds send_timeout() const noexcept { return send_timeout_; }

    /**
     * @brief 设置接收超时时间
     * @param timeout 超时时间（毫秒）
     * @throws value_exception timeout为0或负数时抛出
     */
    void set_recv_timeout(milliseconds timeout);

    /**
     * @brief 获取接收超时时间
     * @return 超时时间
     */
    NEFORCE_NODISCARD milliseconds recv_timeout() const noexcept { return recv_timeout_; }

    /**
     * @brief 设置自动重连
     * @param enable 是否启用
     * @param max_attempts 最大重连次数（默认3）
     * @throws value_exception max_attempts为0或负数时抛出
     */
    void set_auto_reconnect(bool enable, int max_attempts = 3);

    /**
     * @brief 检查是否启用自动重连
     * @return 启用返回true
     */
    NEFORCE_NODISCARD bool is_auto_reconnect() const noexcept { return auto_reconnect_; }

    /**
     * @brief 获取最大重连次数
     * @return 重连次数
     */
    NEFORCE_NODISCARD int reconnect_attempts() const noexcept { return reconnect_attempts_; }

    /**
     * @brief 获取当前重连尝试次数
     * @return 当前次数
     */
    NEFORCE_NODISCARD int current_reconnect_attempt() const noexcept { return current_reconnect_attempt_; }

    /**
     * @brief 设置重连延迟时间
     * @param delay 延迟时间
     * @throws value_exception delay为负数时抛出
     */
    void set_reconnect_delay(milliseconds delay);

    /**
     * @brief 获取重连延迟时间
     * @return 延迟时间
     */
    NEFORCE_NODISCARD milliseconds reconnect_delay() const noexcept { return reconnect_delay_; }

    /**
     * @brief 设置是否优先使用IPv6
     * @param prefer 是否优先
     */
    void set_prefer_ipv6(bool prefer) noexcept { prefer_ipv6_ = prefer; }

    /**
     * @brief 检查是否优先使用IPv6
     * @return 优先返回true
     */
    NEFORCE_NODISCARD bool prefer_ipv6() const noexcept { return prefer_ipv6_; }

    /**
     * @brief 设置DNS服务器配置
     * @param cfg DNS配置
     */
    void set_dns_server(dns_client::config cfg) { dns_.set_config(move(cfg)); }

    /**
     * @brief 设置异常处理器
     * @param handler 处理函数
     */
    void set_exception_handler(exception_handler_t handler) { exception_handler_ = move(handler); }

    /**
     * @brief 设置连接成功回调
     * @param callback 回调函数
     */
    void set_connect_callback(connect_callback_t callback) { connect_callback_ = move(callback); }

    /**
     * @brief 设置断开连接回调
     * @param callback 回调函数
     */
    void set_disconnect_callback(disconnect_callback_t callback) { disconnect_callback_ = move(callback); }

    /**
     * @brief 连接到服务器
     * @param host 主机名或IP地址
     * @param port 端口
     * @return 连接成功返回true
     *
     * 支持域名解析，自动尝试所有解析到的IP地址。
     * 根据prefer_ipv6_设置决定IPv4/IPv6优先级。
     */
    virtual bool connect(const string& host, ports port);

    /**
     * @brief 断开连接
     */
    void disconnect() noexcept;

    /**
     * @brief 发送数据
     * @param data 数据指针
     * @param length 数据长度
     * @return 实际发送字节数，失败返回-1
     *
     * 自动重连机制生效时，连接断开会尝试重连后发送。
     */
    ssize_t send(const void* data, size_t length);

    /**
     * @brief 发送字符串数据
     * @param data 字符串视图
     * @return 实际发送字节数，失败返回-1
     */
    ssize_t send(string_view data) { return send(data.data(), data.size()); }

    /**
     * @brief 发送所有数据（保证全部发送）
     * @param data 数据指针
     * @param length 数据长度
     * @return 发送成功返回true
     *
     * 循环调用send()直到所有数据发送完毕。
     */
    bool send_all(const void* data, size_t length);

    /**
     * @brief 发送所有字符串数据
     * @param data 字符串视图
     * @return 发送成功返回true
     */
    bool send_all(string_view data) { return send_all(data.data(), data.size()); }

    /**
     * @brief 接收数据
     * @param buffer 接收缓冲区
     * @param length 缓冲区大小
     * @return 实际接收字节数，0表示连接关闭，-1表示错误
     */
    ssize_t receive(void* buffer, size_t length);

    /**
     * @brief 接收所有可用数据
     * @param max_size 最大接收字节数（0表示无限制）
     * @return 接收到的数据
     *
     * 持续接收直到连接关闭或达到最大大小。
     */
    vector<char> receive_all(size_t max_size = 0);

    /**
     * @brief 接收指定大小的数据
     * @param buffer 接收缓冲区（指定大小）
     * @return 接收成功返回true
     *
     * 循环接收直到缓冲区填满或连接关闭。
     */
    bool receive_exact(memory_view<char> buffer);

    /**
     * @brief 接收一行数据（以\\n结尾）
     * @param max_length 最大行长度
     * @return 行字符串（不含\\n），失败返回none
     *
     * 支持\\r\\n和\\n换行符。
     */
    optional<string> receive_line(size_t max_length = 8192);

    /**
     * @brief 检查是否已连接
     * @return 已连接返回true
     */
    NEFORCE_NODISCARD bool is_connected() const noexcept { return socket_ && socket_->is_open(); }

    /**
     * @brief 检查是否正在重连
     * @return 正在重连返回true
     */
    NEFORCE_NODISCARD bool is_reconnecting() const noexcept { return is_reconnecting_; }

    /**
     * @brief 获取当前连接的主机名
     * @return 主机名
     */
    NEFORCE_NODISCARD const string& connected_host() const noexcept { return connected_host_; }

    /**
     * @brief 获取当前连接的端口
     * @return 端口
     */
    NEFORCE_NODISCARD ports connected_port() const noexcept { return connected_port_; }

    /**
     * @brief 获取底层socket引用
     * @return socket引用
     * @throws value_exception 未连接时抛出
     */
    NEFORCE_NODISCARD tcp_socket& socket();

    /**
     * @brief 获取底层socket常量引用
     * @return socket常量引用
     * @throws value_exception 未连接时抛出
     */
    NEFORCE_NODISCARD const tcp_socket& socket() const;

    /**
     * @brief 获取DNS客户端
     * @return DNS客户端引用
     */
    NEFORCE_NODISCARD dns_client& get_dns_client() noexcept { return dns_; }

    /**
     * @brief 获取DNS客户端常量引用
     * @return DNS客户端常量引用
     */
    NEFORCE_NODISCARD const dns_client& get_dns_client() const noexcept { return dns_; }
};

/**
 * @class tcp_client
 * @brief TCP客户端类
 *
 * 普通TCP客户端实现，不加密。
 */
class NEFORCE_API tcp_client final : public tcp_client_base {
public:
    using tcp_client_base::tcp_client_base;

protected:
    unique_ptr<tcp_socket> create_socket() override { return make_unique<tcp_socket>(); }
};

/** @} */ // TCP

/**
 * @addtogroup SSL SSL/TLS
 * @{
 */

/**
 * @class ssl_client
 * @brief SSL/TLS客户端类
 *
 * 实现SSL/TLS加密的TCP客户端，支持证书验证和SNI。
 *
 * 使用示例：
 * @code
 * // 创建SSL客户端
 * ssl_client client;
 *
 * // 加载CA证书（可选）
 * client.load_ca_file("ca-bundle.crt");
 *
 * // 设置SNI主机名
 * client.set_sni_hostname("example.com");
 *
 * // 设置验证模式
 * client.set_verify_peer(true);
 *
 * // 连接HTTPS服务器
 * if (client.connect("example.com", 443)) {
 *     // 发送HTTPS请求
 *     client.send_all("GET / HTTP/1.1\r\nHost: example.com\r\n\r\n");
 *
 *     // 获取证书信息
 *     println("Cipher: ", client.cipher_name());
 *     println("Protocol: ", client.protocol_version());
 * }
 * @endcode
 */
class NEFORCE_API ssl_client final : public tcp_client_base {
private:
    optional<ssl_context> ssl_ctx_; ///< SSL上下文
    string sni_hostname_;           ///< SNI主机名
    bool verify_peer_ = true;       ///< 是否验证对等方证书
    bool ssl_initialized_ = false;  ///< SSL是否已初始化

protected:
    unique_ptr<tcp_socket> create_socket() override { return make_unique<ssl_socket>(); }

    bool post_connect() override;
    void pre_disconnect() override { ssl_initialized_ = false; }

public:
    /**
     * @brief 默认构造函数
     */
    ssl_client() = default;

    /**
     * @brief 构造函数（带SSL上下文）
     * @param ctx SSL上下文
     */
    explicit ssl_client(ssl_context ctx);

    /**
     * @brief 设置SSL上下文
     * @param ctx SSL上下文
     * @throws ssl_exception 已连接或上下文无效时抛出
     */
    void set_ssl_context(ssl_context ctx);

    /**
     * @brief 设置是否验证对等方证书
     * @param verify 是否验证
     * @throws ssl_exception 已连接时抛出
     */
    void set_verify_peer(bool verify);
    NEFORCE_NODISCARD bool get_verify_peer() const noexcept { return verify_peer_; }

    /**
     * @brief 设置SNI主机名
     * @param hostname 主机名
     * @throws ssl_exception 已连接时抛出
     */
    void set_sni_hostname(string hostname);
    NEFORCE_NODISCARD const string& sni_hostname() const noexcept { return sni_hostname_; }

    /**
     * @brief 加载CA证书文件
     * @param ca_file CA证书文件路径
     * @return 加载成功返回true
     */
    bool load_ca_file(const string& ca_file);

    /**
     * @brief 加载CA证书目录
     * @param ca_path CA证书目录路径
     * @return 加载成功返回true
     */
    bool load_ca_path(const string& ca_path);

    /**
     * @brief 获取对等方证书信息
     * @return 证书信息字符串
     */
    NEFORCE_NODISCARD string_view peer_certificate_info() const;

    /**
     * @brief 获取当前密码套件名称
     * @return 密码套件名称
     */
    NEFORCE_NODISCARD string_view cipher_name() const;

    /**
     * @brief 获取当前协议版本
     * @return 协议版本字符串
     */
    NEFORCE_NODISCARD string_view protocol_version() const;

    /**
     * @brief 检查是否有SSL上下文
     * @return 有返回true
     */
    NEFORCE_NODISCARD bool has_ssl_context() const noexcept { return ssl_ctx_.has_value() && ssl_ctx_->is_valid(); }

    /**
     * @brief 检查SSL是否已初始化
     * @return 已初始化返回true
     */
    NEFORCE_NODISCARD bool is_ssl_initialized() const noexcept { return ssl_initialized_; }

    /**
     * @brief 获取SSL socket引用
     * @return SSL socket引用
     * @throws value_exception SSL未就绪时抛出
     */
    NEFORCE_NODISCARD ssl_socket& ssl_socket_ref();

    /**
     * @brief 获取SSL socket常量引用
     * @return SSL socket常量引用
     * @throws value_exception SSL未就绪时抛出
     */
    NEFORCE_NODISCARD const ssl_socket& ssl_socket_ref() const;
};

/** @} */ // SSL/TLS

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_TCP_TCP_CLIENT_HPP__
