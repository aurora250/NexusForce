#ifndef NEFORCE_NETWORK_SSL_SSL_SOCKET_HPP__
#define NEFORCE_NETWORK_SSL_SSL_SOCKET_HPP__

/**
 * @file ssl_socket.hpp
 * @brief SSL/TLS安全Socket实现
 *
 * 此文件提供了SSL/TLS安全Socket的实现。
 */

#include "NeForce/network/tcp/tcp_socket.hpp"
#include "NeForce/network/ssl/ssl_exception.hpp"
#include "NeForce/network/ssl/ssl_stream.hpp"
NEFORCE_BEGIN_NAMESPACE__

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
     * @param ctx 已加载CA证书的SSL上下文
     * @throws ssl_exception SSL握手失败时抛出
     * @throws value_exception socket未打开时抛出
     */
    void init_server_ssl(const ssl_context& ctx);

    /**
     * @brief 初始化客户端端SSL
     * @param ctx 已加载CA证书的SSL上下文
     * @param hostname 服务器主机名
     * @throws ssl_exception SSL握手失败时抛出
     * @throws value_exception socket未打开时抛出
     */
    void init_client_ssl(const ssl_context& ctx, const string& hostname = "", void* session = nullptr);

    /**
     * @brief 准备服务器端 SSL
     * @param ctx 已加载CA证书的SSL上下文
     * @throws ssl_exception SSL对象创建失败时抛出
     * @throws value_exception socket未打开时抛出
     *
     * 将 SSL 置于接受状态，但不执行握手。
     */
    void prepare_server_ssl(const ssl_context& ctx);

    /**
     * @brief 准备客户端端 SSL
     * @param ctx 已加载CA证书的SSL上下文
     * @param hostname 服务器主机名
     * @throws ssl_exception SSL对象创建失败时抛出
     * @throws value_exception socket未打开时抛出
     *
     * 将 SSL 置于连接状态，但不执行握手。
     */
    void prepare_client_ssl(const ssl_context& ctx, const string& hostname = "");

    /**
     * @brief 异步执行 SSL/TLS 握手
     * @param ctx 异步 I/O 执行上下文
     * @param handler 完成回调 void(error_code)
     * @note 需要先完成 SSL 准备
     */
    void async_handshake(io_context& ctx, function<void(error_code)> handler);

    /**
     * @brief 带取消槽的异步执行 SSL/TLS 握手
     * @param ctx 异步 I/O 执行上下文
     * @param slot 取消槽
     * @param handler 完成回调 void(error_code)
     * @note 需要先完成 SSL 准备
     */
    void async_handshake(io_context& ctx, cancellation_slot& slot, function<void(error_code)> handler);

    /**
     * @brief 可调用对象的异步握手
     * @tparam Token 可调用对象类型，需满足 void(error_code) 签名
     * @param ctx 异步 I/O 执行上下文
     * @param token 完成令牌
     */
    template <typename Token, enable_if_t<!is_same_v<decay_t<Token>, function<void(error_code)>>, int> = 0>
    void async_handshake(io_context& ctx, Token&& token) {
        async_handshake(ctx, function<void(error_code)>(forward<Token>(token)));
    }

    /**
     * @brief future 异步握手
     * @param ctx 异步 I/O 执行上下文
     * @return 异步操作结果
     */
    future<void> async_handshake(io_context& ctx, use_future_t /*unused*/) {
        async_result<use_future_t, void(error_code)> result(use_future);
        async_handshake(ctx, function<void(error_code)>(result.get_handler()));
        return result.get();
    }

    /**
     * @brief detached 异步握手
     * @param ctx 异步 I/O 执行上下文
     */
    void async_handshake(io_context& ctx, detached_t /*unused*/) {
        async_handshake(ctx, function<void(error_code)>([](error_code) {}));
    }

#ifdef NEFORCE_STANDARD_20
    /**
     * @brief awaitable 异步握手
     * @param ctx 异步 I/O 执行上下文
     * @return 可协程等待的结果
     */
    awaitable<error_code> async_handshake(io_context& ctx, use_awaitable_t /*unused*/) {
        async_result<use_awaitable_t, void(error_code)> result(use_awaitable);
        async_handshake(ctx, function<void(error_code)>(result.get_handler()));
        return result.get();
    }
#endif

    /**
     * @brief 获取对等方证书信息
     * @return 证书信息字符串
     */
    NEFORCE_NODISCARD string peer_certificate_info() const;

    /**
     * @brief 获取ALPN协商的协议名称
     * @return 协议名称字符串
     */
    NEFORCE_NODISCARD string get_alpn_negotiated() const;

    /**
     * @brief 发送加密数据
     * @param data 要发送的数据
     * @param flags 发送标志
     * @return 实际发送的字节数
     * @throws ssl_exception 发送失败时抛出
     */
    ssize_t send(memory_view<const char> data, int flags = 0) override;

    /**
     * @brief 接收解密数据
     * @param buffer 接收缓冲区
     * @param flags 接收标志
     * @return 实际接收的字节数
     * @throws ssl_exception 接收失败时抛出
     */
    ssize_t receive(memory_view<char> buffer, int flags = 0) override;

    /**
     * @brief 异步读取解密数据
     * @param ctx 异步 I/O 执行上下文
     * @param buffer 接收缓冲区
     * @param handler 完成回调 void(error_code, size_t bytes_transferred)
     */
    void async_read(io_context& ctx, memory_view<char> buffer, function<void(error_code, size_t)> handler) override;

    /**
     * @brief 带取消槽的异步读取解密数据
     * @param ctx 异步 I/O 执行上下文
     * @param buffer 接收缓冲区
     * @param slot 取消槽
     * @param handler 完成回调 void(error_code, size_t bytes_transferred)
     */
    void async_read(io_context& ctx, memory_view<char> buffer, cancellation_slot& slot,
                    function<void(error_code, size_t)> handler) override;

    /**
     * @brief 异步写入加密数据
     * @param ctx 异步 I/O 执行上下文
     * @param buffer 发送缓冲区
     * @param handler 完成回调 void(error_code, size_t bytes_transferred)
     */
    void async_write(io_context& ctx, memory_view<const char> buffer,
                     function<void(error_code, size_t)> handler) override;

    /**
     * @brief 带取消槽的异步写入加密数据
     * @param ctx 异步 I/O 执行上下文
     * @param buffer 发送缓冲区
     * @param slot 取消槽
     * @param handler 完成回调 void(error_code, size_t bytes_transferred)
     */
    void async_write(io_context& ctx, memory_view<const char> buffer, cancellation_slot& slot,
                     function<void(error_code, size_t)> handler) override;

    /**
     * @brief 关闭SSL连接和底层socket
     */
    bool close() noexcept override;

    /**
     * @brief 检查是否处于SSL/TLS状态
     * @return SSL/TLS是否激活
     */
    NEFORCE_NODISCARD bool is_ssl() const noexcept override { return ssl_.has_value(); }

    /**
     * @brief 当前连接是否复用TLS会话
     * @return 复用返回true
     */
    NEFORCE_NODISCARD bool session_reused() const noexcept { return ssl_.has_value() && ssl_->session_reused(); }

    /**
     * @brief 处理握手后的会话票据消息
     *
     * 短暂切换为非阻塞读取一个字节，让 OpenSSL 处理 TLS 1.3 NewSessionTicket；
     * 握手完成后、发送应用数据前调用。
     */
    void drain_session_tickets();

    /**
     * @brief 获取SSL流对象的引用
     * @return SSL流引用
     * @throws ssl_exception SSL未初始化时抛出
     */
    NEFORCE_NODISCARD ssl_stream& ssl() {
        if (!ssl_) {
            NEFORCE_THROW_EXCEPTION(ssl_exception("SSL stream not initialized"));
        }
        return *ssl_;
    }

    /**
     * @brief 获取SSL流对象的常量引用
     * @return SSL流常量引用
     * @throws ssl_exception SSL未初始化时抛出
     */
    NEFORCE_NODISCARD const ssl_stream& ssl() const {
        if (!ssl_) {
            NEFORCE_THROW_EXCEPTION(ssl_exception("SSL stream not initialized"));
        }
        return *ssl_;
    }
};

NEFORCE_BEGIN_INNER__

template <>
struct future_handler<error_code, ssl_socket> {
    shared_ptr<promise<ssl_socket>> promise_;

    void operator()(error_code ec, ssl_socket sock) {
        if (ec) {
            promise_->set_exception(_NEFORCE make_exception_ptr(system_exception(ec)));
        } else {
            promise_->set_value(move(sock));
        }
    }
};

NEFORCE_END_INNER__

template <>
struct async_result<use_future_t, void(error_code, ssl_socket)> {
    using handler_type = inner::future_handler<error_code, ssl_socket>;
    using return_type = future<ssl_socket>;
    handler_type handler_;
    explicit async_result(use_future_t /*unused*/) { handler_.promise_ = make_shared<promise<ssl_socket>>(); }
    handler_type get_handler() { return handler_; }
    return_type get() { return handler_.promise_->get_future(); }
};

/** @} */ // SSL/TLS

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_SSL_SSL_SOCKET_HPP__
