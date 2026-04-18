#ifndef NEFORCE_NETWORK_SSL_SSL_SOCKET_HPP__
#define NEFORCE_NETWORK_SSL_SSL_SOCKET_HPP__

/**
 * @file ssl_socket.hpp
 * @brief SSL/TLS安全Socket实现
 *
 * 此文件提供了SSL/TLS安全Socket的实现。
 */

#include "NeForce/network/tcp/tcp_socket.hpp"
#include "NeForce/network/ssl/ssl_stream.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup Network 网络通信
 * @{
 */

/**
 * @addtogroup SSL SSL/TLS
 * @{
 */

/**
 * @class ssl_socket
 * @brief SSL/TLS安全Socket类
 *
 * 实现SSL/TLS加密的TCP socket，提供安全的网络通信。
 * 继承自tcp_socket，可以透明地替换普通TCP socket。
 *
 * 主要功能：
 * - SSL/TLS客户端初始化
 * - SSL/TLS服务器端初始化
 * - 加密数据发送和接收
 * - 对等方证书信息获取
 * - 无缝透明替换TCP socket
 *
 * 使用示例：
 * @code
 * ssl_context ctx(ssl_method::TLS_CLIENT);
 * ctx.load_verify_locations("ca-bundle.crt");
 *
 * ssl_socket client;
 * client.open(AF_INET);
 *
 * auto server = ip_address::parse("example.com", ports::https);
 * client.connect(*server, milliseconds(3000));
 * client.init_client_ssl(ctx, "example.com");
 *
 * // 发送HTTPS请求
 * string request = "GET / HTTP/1.1\r\nHost: example.com\r\n\r\n";
 * client.send_all(request.view());
 *
 * // 接收响应
 * char buffer[4096];
 * ssize_t received = client.receive({buffer, sizeof(buffer)});
 * if (received > 0) {
 *     println(string_view(buffer, received));
 * }
 *
 * // 获取证书信息
 * println(client.peer_certificate_info());
 *
 * // SSL服务器示例
 * ssl_context server_ctx(ssl_method::TLS_SERVER);
 * server_ctx.load_certificate("server.crt", "server.key");
 *
 * ssl_acceptor acceptor;
 * acceptor.open(ip_address::any(ports::https));
 *
 * auto client_sock = acceptor.accept_ssl();
 * client_sock.set_ssl_context(server_ctx);
 *
 * // 与客户端进行加密通信
 * @endcode
 */
class NEFORCE_API ssl_socket final : public tcp_socket {
private:
    optional<ssl_stream> ssl_; ///< SSL流对象（仅TLS激活时）

public:
    /**
     * @brief 默认构造函数
     */
    ssl_socket() = default;

    /**
     * @brief 从原生句柄构造
     * @param fd 原生socket句柄
     */
    explicit ssl_socket(const native_handle_type fd) :
    tcp_socket(fd) {}

    /**
     * @brief 从TCP socket转移构造
     * @param sock TCP socket对象
     *
     * 将普通TCP socket转换为SSL socket。
     * 适用于接受连接后升级为TLS的场景。
     */
    explicit ssl_socket(tcp_socket sock) :
    tcp_socket(move(sock)) {}

    ssl_socket(ssl_socket&& other) noexcept = default;
    ssl_socket& operator=(ssl_socket&& other) noexcept = default;

    /**
     * @brief 析构函数
     */
    ~ssl_socket() override = default;

    /**
     * @brief 初始化服务器端SSL
     * @param ctx SSL上下文（需已加载CA证书）
     * @throws ssl_exception SSL握手失败时抛出
     * @throws value_exception socket未打开时抛出
     *
     * 作为服务器端执行SSL/TLS握手。需要在连接建立后调用。
     */
    void init_server_ssl(const ssl_context& ctx);

    /**
     * @brief 初始化客户端端SSL
     * @param ctx SSL上下文（需已加载CA证书）
     * @param hostname 服务器主机名（用于SNI和证书验证）
     * @throws ssl_exception SSL握手失败时抛出
     * @throws value_exception socket未打开时抛出
     *
     * 作为客户端发起SSL/TLS握手。需要在连接建立后调用。
     */
    void init_client_ssl(const ssl_context& ctx, const string& hostname = "");

    /**
     * @brief 获取对等方证书信息
     * @return 证书信息字符串（主题和颁发者）
     *
     * 返回对等方证书的可读信息，用于调试和验证。
     */
    NEFORCE_NODISCARD string peer_certificate_info() const;

    /**
     * @brief 发送加密数据
     * @param data 要发送的数据
     * @param flags 发送标志（忽略）
     * @return 实际发送的字节数
     * @throws ssl_exception 发送失败时抛出
     *
     * 如果TLS已激活，通过ssl_stream发送加密数据；否则回退到普通发送。
     */
    ssize_t send(memory_view<const char> data, int flags = 0) override;

    /**
     * @brief 接收解密数据
     * @param buffer 接收缓冲区
     * @param flags 接收标志（忽略）
     * @return 实际接收的字节数
     * @throws ssl_exception 接收失败时抛出
     *
     * 如果TLS已激活，通过ssl_stream接收解密数据；否则回退到普通接收。
     */
    ssize_t receive(memory_view<char> buffer, int flags = 0) override;

    /**
     * @brief 检查是否为SSL/TLS socket
     * @return 始终返回true（SSL/TLS激活时）
     */
    NEFORCE_NODISCARD bool is_ssl() const noexcept override { return ssl_.has_value(); }

    /**
     * @brief 获取SSL流对象的引用
     * @return SSL流引用
     */
    NEFORCE_NODISCARD ssl_stream& ssl() noexcept { return *ssl_; }

    /**
     * @brief 获取SSL流对象的常量引用
     * @return SSL流常量引用
     */
    NEFORCE_NODISCARD const ssl_stream& ssl() const noexcept { return *ssl_; }
};

/** @} */ // SSL/TLS

/** @} */ // Network

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_SSL_SSL_SOCKET_HPP__
