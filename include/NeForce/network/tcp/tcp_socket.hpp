#ifndef NEFORCE_NETWORK_TCP_TCP_SOCKET_HPP__
#define NEFORCE_NETWORK_TCP_TCP_SOCKET_HPP__

/**
 * @file tcp_socket.hpp
 * @brief TCP Socket实现
 *
 * 此文件提供了TCP Socket的实现。
 */

#include "NeForce/core/async/async_stream.hpp"
#include "NeForce/core/async/cancellation_slot.hpp"
#include "NeForce/core/async/io_context.hpp"
#include "NeForce/core/async/use_awaitable.hpp"
#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/functional/function.hpp"
#include "NeForce/network/ip_socket.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup TCP TCP
 * @brief TCP通信相关组件
 * @{
 */

/**
 * @class tcp_socket
 * @brief TCP Socket类
 *
 * 实现TCP协议的流式socket，
 * TCP是面向连接的协议，提供可靠的数据传输、流量控制和拥塞控制。
 *
 * 主要功能：
 * - TCP socket创建和打开
 * - 连接建立（支持超时）
 * - 数据发送（普通、带超时、全部发送）
 * - 数据接收（普通、全部接收）
 * - SSL/TLS支持接口
 *
 * 使用示例：
 * @code
 * // TCP客户端示例
 * tcp_socket client;
 * client.open(AF_INET);
 *
 * auto server = ip_address::parse("192.168.1.100", ports(8080));
 * if (client.connect(*server, milliseconds(3000))) {
 *     string request = "GET / HTTP/1.1\r\nHost: example.com\r\n\r\n";
 *     client.send_all(request.view());
 *
 *     char buffer[4096];
 *     ssize_t received = client.receive({buffer, sizeof(buffer)});
 *     if (received > 0) {
 *         println(string_view(buffer, received));
 *     }
 * }
 *
 * // TCP服务器示例
 * tcp_socket server;
 * server.open(AF_INET);
 * auto local = ip_address::any(ports(8080));
 * server.bind(local);
 * server.listen(128);
 *
 * auto client_sock = server.accept();  // 接受连接
 * // 与客户端通信...
 * @endcode
 */
class NEFORCE_API tcp_socket : public ip_socket, public async_stream {
public:
    /**
     * @brief 默认构造函数
     */
    tcp_socket() = default;

    /**
     * @brief 移动构造函数
     */
    tcp_socket(tcp_socket&&) = default;

    /**
     * @brief 移动赋值运算符
     */
    tcp_socket& operator=(tcp_socket&&) = default;

    /**
     * @brief 从原生句柄构造
     * @param fd 原生socket句柄
     */
    explicit tcp_socket(native_handle_type fd) :
    ip_socket(fd) {}

    /**
     * @brief 析构函数
     */
    ~tcp_socket() override = default;

    /**
     * @brief 打开TCP socket
     * @param f 地址族，默认IPv4
     * @throws socket_exception 创建失败时抛出
     *
     * 创建TCP协议的socket，使用SOCK_STREAM类型和IPPROTO_TCP协议。
     */
    void open(family f = family::INET4);

    /**
     * @brief 连接到远程服务器
     * @param endpoint 远程端点地址
     * @param timeout 连接超时时间
     * @param was_blocking 连接前是否处于阻塞模式
     * @return 连接成功返回true，超时返回false
     * @throws socket_exception 连接失败时抛出
     * @throws value_exception socket未打开或端点无效时抛出
     *
     * 发起TCP连接到指定的远程服务器，支持超时控制。
     * 非阻塞模式下，此函数会等待连接完成或超时。
     */
    bool connect(const ip_address& endpoint, milliseconds timeout, bool was_blocking = true);

    /**
     * @brief 发送数据
     * @param data 要发送的数据
     * @param flags 发送标志（通常为0）
     * @return 实际发送的字节数
     * @throws socket_exception 发送失败时抛出
     * @throws value_exception socket未打开时抛出
     *
     * 发送数据到已连接的远程端点。
     * 可能只发送部分数据，需要循环调用直到全部发送完成。
     */
    virtual ssize_t send(memory_view<const char> data, int flags = 0);

    /**
     * @brief 带超时的发送数据
     * @param data 要发送的数据
     * @param timeout 发送超时时间
     * @param flags 发送标志（通常为0）
     * @return 实际发送的字节数
     * @throws socket_exception 发送失败或超时时抛出
     *
     * 在指定超时时间内等待socket可写，然后发送数据。
     */
    ssize_t send(memory_view<const char> data, milliseconds timeout, int flags = 0);

    /**
     * @brief 发送所有数据
     * @param data 要发送的数据
     * @throws socket_exception 发送失败时抛出
     *
     * 循环发送直到所有数据发送完毕。
     * 适用于阻塞模式下的完整数据发送。
     */
    void send_all(memory_view<const char> data);

    /**
     * @brief 接收数据
     * @param buffer 接收缓冲区
     * @param flags 接收标志（通常为0）
     * @return 实际接收的字节数（0表示连接关闭）
     * @throws socket_exception 接收失败时抛出
     * @throws value_exception socket未打开时抛出
     *
     * 从已连接的远程端点接收数据。
     * 返回0表示对端已关闭连接。
     */
    virtual ssize_t receive(memory_view<char> buffer, int flags = 0);

    /**
     * @brief 接收指定大小的所有数据
     * @param expected_size 期望接收的字节数
     * @return 实际接收的数据（可能小于expected_size）
     * @throws socket_exception 接收失败时抛出
     *
     * 循环接收直到达到期望大小或连接关闭。
     * 适用于需要完整接收特定大小数据的场景。
     */
    vector<char> receive_all(size_t expected_size);

    /**
     * @brief 检查是否为SSL/TLS socket
     * @return 始终返回false（基类实现）
     */
    NEFORCE_NODISCARD virtual bool is_ssl() const noexcept { return false; }

    /**
     * @brief 异步连接到远程端点
     * @param ctx 异步 I/O 执行上下文
     * @param endpoint 远程端点地址
     * @param handler 完成回调 void(error_code)
     *
     * 发起异步TCP连接。socket 会被设为非阻塞模式。
     * handler 在 io_context::run() 线程中执行。
     */
    void async_connect(io_context& ctx, const ip_address& endpoint, function<void(error_code)> handler);

    /**
     * @brief 异步连接（带取消槽）
     * @param ctx 异步 I/O 执行上下文
     * @param endpoint 远程端点地址
     * @param slot 取消槽
     * @param handler 完成回调 void(error_code)
     */
    void async_connect(io_context& ctx, const ip_address& endpoint, cancellation_slot& slot,
                       function<void(error_code)> handler);

    /**
     * @brief 异步读取数据
     * @param ctx 异步 I/O 执行上下文
     * @param buffer 接收缓冲区
     * @param handler 完成回调 void(error_code, size_t bytes_transferred)
     */
    void async_read(io_context& ctx, memory_view<char> buffer, function<void(error_code, size_t)> handler) override;

    /**
     * @brief 异步读取（带取消槽）
     * @param ctx 异步 I/O 执行上下文
     * @param buffer 接收缓冲区
     * @param slot 取消槽
     * @param handler 完成回调 void(error_code, size_t bytes_transferred)
     */
    void async_read(io_context& ctx, memory_view<char> buffer, cancellation_slot& slot,
                    function<void(error_code, size_t)> handler) override;

    /**
     * @brief 异步写入数据
     * @param ctx 异步 I/O 执行上下文
     * @param buffer 发送缓冲区
     * @param handler 完成回调 void(error_code, size_t bytes_transferred)
     */
    void async_write(io_context& ctx, memory_view<const char> buffer,
                     function<void(error_code, size_t)> handler) override;

    /**
     * @brief 异步写入（带取消槽）
     * @param ctx 异步 I/O 执行上下文
     * @param buffer 发送缓冲区
     * @param slot 取消槽
     * @param handler 完成回调 void(error_code, size_t bytes_transferred)
     */
    void async_write(io_context& ctx, memory_view<const char> buffer, cancellation_slot& slot,
                     function<void(error_code, size_t)> handler) override;

    /**
     * @brief 异步接收（转发到 async_read）
     * @param ctx 异步 I/O 执行上下文
     * @param buffer 接收缓冲区
     * @param handler 完成回调 void(error_code, size_t bytes_transferred)
     */
    void async_receive(io_context& ctx, memory_view<char> buffer, function<void(error_code, size_t)> handler) {
        async_read(ctx, buffer, move(handler));
    }

    /**
     * @brief 异步接收（带取消槽，转发到 async_read）
     * @param ctx 异步 I/O 执行上下文
     * @param buffer 接收缓冲区
     * @param slot 取消槽
     * @param handler 完成回调 void(error_code, size_t bytes_transferred)
     */
    void async_receive(io_context& ctx, memory_view<char> buffer, cancellation_slot& slot,
                       function<void(error_code, size_t)> handler) {
        async_read(ctx, buffer, slot, move(handler));
    }

    /**
     * @brief 异步发送（转发到 async_write）
     * @param ctx 异步 I/O 执行上下文
     * @param buffer 发送缓冲区
     * @param handler 完成回调 void(error_code, size_t bytes_transferred)
     */
    void async_send(io_context& ctx, memory_view<const char> buffer, function<void(error_code, size_t)> handler) {
        async_write(ctx, buffer, move(handler));
    }

    /**
     * @brief 异步发送（带取消槽，转发到 async_write）
     * @param ctx 异步 I/O 执行上下文
     * @param buffer 发送缓冲区
     * @param slot 取消槽
     * @param handler 完成回调 void(error_code, size_t bytes_transferred)
     */
    void async_send(io_context& ctx, memory_view<const char> buffer, cancellation_slot& slot,
                    function<void(error_code, size_t)> handler) {
        async_write(ctx, buffer, slot, move(handler));
    }

    /**
     * @brief 异步接收—模板令牌版本（转发到 async_read）
     * @tparam Token 完成令牌类型
     * @param ctx 异步 I/O 执行上下文
     * @param buffer 接收缓冲区
     * @param token 完成令牌
     */
    template <typename Token, enable_if_t<!is_same_v<decay_t<Token>, function<void(error_code, size_t)>>, int> = 0>
    decltype(auto) async_receive(io_context& ctx, memory_view<char> buffer, Token&& token) {
        return async_stream::async_read(ctx, buffer, forward<Token>(token));
    }

    /**
     * @brief 异步发送—模板令牌版本（转发到 async_write）
     * @tparam Token 完成令牌类型
     * @param ctx 异步 I/O 执行上下文
     * @param buffer 发送缓冲区
     * @param token 完成令牌
     */
    template <typename Token, enable_if_t<!is_same_v<decay_t<Token>, function<void(error_code, size_t)>>, int> = 0>
    decltype(auto) async_send(io_context& ctx, memory_view<const char> buffer, Token&& token) {
        return async_stream::async_write(ctx, buffer, forward<Token>(token));
    }

    /**
     * @brief 异步连接—任意可调用对象版本
     * @tparam Token 可调用对象类型，需满足 void(error_code) 签名
     * @param ctx 异步 I/O 执行上下文
     * @param endpoint 远程端点地址
     * @param token 完成令牌
     *
     * handler 可以是任意满足 void(error_code) 的可调用对象。
     */
    template <typename Token, enable_if_t<!is_same_v<decay_t<Token>, function<void(error_code)>>, int> = 0>
    void async_connect(io_context& ctx, const ip_address& endpoint, Token&& token) {
        async_connect(ctx, endpoint, function<void(error_code)>(forward<Token>(token)));
    }

    /**
     * @brief 异步连接—use_future
     * @param ctx 异步 I/O 执行上下文
     * @param endpoint 远程端点地址
     * @return future<void> 异步操作结果
     */
    auto async_connect(io_context& ctx, const ip_address& endpoint, use_future_t /*unused*/) {
        async_result<use_future_t, void(error_code)> result(use_future);
        async_connect(ctx, endpoint, function<void(error_code)>(result.get_handler()));
        return result.get();
    }

    /**
     * @brief 异步连接—detached（即发即忘）
     * @param ctx 异步 I/O 执行上下文
     * @param endpoint 远程端点地址
     */
    void async_connect(io_context& ctx, const ip_address& endpoint, detached_t /*unused*/) {
        async_connect(ctx, endpoint, function<void(error_code)>([](error_code) {}));
    }

#ifdef NEFORCE_STANDARD_20
    /**
     * @brief 异步连接—use_awaitable
     * @param ctx 异步 I/O 执行上下文
     * @param endpoint 远程端点地址
     * @return awaitable<void> 可协程等待的结果
     */
    auto async_connect(io_context& ctx, const ip_address& endpoint, use_awaitable_t /*unused*/) {
        async_result<use_awaitable_t, void(error_code)> result(use_awaitable);
        async_connect(ctx, endpoint, function<void(error_code)>(result.get_handler()));
        return result.get();
    }
#endif
};


NEFORCE_BEGIN_INNER__

template <>
struct future_handler<error_code, tcp_socket> {
    shared_ptr<promise<tcp_socket>> promise_;

    void operator()(error_code ec, tcp_socket sock) {
        if (ec) {
            promise_->set_exception(_NEFORCE make_exception_ptr(system_exception(ec)));
        } else {
            promise_->set_value(move(sock));
        }
    }
};

NEFORCE_END_INNER__

template <>
struct async_result<use_future_t, void(error_code, tcp_socket)> {
    using handler_type = inner::future_handler<error_code, tcp_socket>;
    using return_type = future<tcp_socket>;
    handler_type handler_;
    explicit async_result(use_future_t /*unused*/) { handler_.promise_ = make_shared<promise<tcp_socket>>(); }
    handler_type get_handler() { return handler_; }
    return_type get() { return handler_.promise_->get_future(); }
};

/** @} */ // TCP

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_TCP_TCP_SOCKET_HPP__
