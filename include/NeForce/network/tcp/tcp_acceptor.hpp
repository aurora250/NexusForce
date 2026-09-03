#ifndef NEFORCE_NETWORK_TCP_TCP_ACCEPTOR_HPP__
#define NEFORCE_NETWORK_TCP_TCP_ACCEPTOR_HPP__

/**
 * @file tcp_acceptor.hpp
 * @brief TCP Acceptor实现
 *
 * 此文件提供了TCP Acceptor的实现。
 */

#include "NeForce/network/tcp/tcp_socket.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup TCP TCP
 * @brief TCP通信相关组件
 * @{
 */

/**
 * @class tcp_acceptor
 * @brief TCP Acceptor类
 *
 * 实现TCP服务器的连接接受功能，负责监听端口并接受客户端连接。
 * 继承自ip_socket，提供专门用于服务器端的操作。
 *
 * 主要功能：
 * - 绑定到本地端点
 * - 开始监听连接
 * - 接受客户端连接
 *
 * 使用示例：
 * @code
 * // TCP服务器示例
 * tcp_acceptor acceptor;
 *
 * // 绑定并监听端口
 * auto endpoint = ip_address::any(ports(8080));
 * acceptor.open(endpoint, 128);
 * acceptor.set_nonblocking(true);
 *
 * println("Server listening on ", endpoint);
 *
 * while (true) {
 *     try {
 *         // 接受客户端连接
 *         auto client = acceptor.accept_nonblock();
 *         if (client) {
 *             // 获取客户端地址
 *             auto client_addr = client.remote_endpoint();
 *             if (client_addr) {
 *                 println("New connection from ", client_addr);
 *             }
 *
 *             // 处理客户端请求
 *             char buffer[1024];
 *             ssize_t received = client.receive({buffer, sizeof(buffer)});
 *             if (received > 0) {
 *                 client.send_all({"Hello from server!"});
 *             }
 *         }
 *     } catch (const exception& e) {
 *         println("Accept error: ", e.what());
 *     }
 * }
 * @endcode
 *
 * @note 默认启用地址重用选项，允许在关闭后立即重用端口。
 */
class NEFORCE_API tcp_acceptor : public ip_socket {
public:
    using socket_base::open;

private:
    using ip_socket::connect;
    using socket_base::shutdown_both;
    using socket_base::shutdown_receive;
    using socket_base::shutdown_send;

public:
    /**
     * @brief 默认构造函数
     */
    tcp_acceptor() = default;

    /**
     * @brief 打开并开始监听
     * @param endpoint 要绑定的本地端点地址
     * @param backlog 连接队列大小（默认SOMAXCONN）
     * @throws socket_exception 绑定或监听失败时抛出
     * @throws value_exception 端点无效时抛出
     */
    void open(const ip_address& endpoint, int backlog = SOMAXCONN);

    /**
     * @brief 接受客户端连接（阻塞模式）
     * @return 新的TCP socket对象
     * @throws socket_exception 接受失败时抛出
     * @throws value_exception acceptor未打开时抛出
     *
     * 阻塞等待直到有新连接到达，然后返回代表该连接的socket。
     * 返回的socket可用于与客户端通信。
     */
    NEFORCE_NODISCARD tcp_socket accept();

    /**
     * @brief 接受客户端连接（非阻塞模式）
     * @return 有新连接返回socket对象，无连接返回none
     * @throws socket_exception 发生错误时抛出
     *
     * 非阻塞地尝试接受新连接。
     * 如果没有待处理的连接，立即返回none。
     * 需要先设置socket为非阻塞模式。
     */
    NEFORCE_NODISCARD optional<tcp_socket> accept_nonblock();

    /**
     * @brief 异步接受客户端连接
     * @param ctx 异步 I/O 执行上下文
     * @param handler 完成回调 void(error_code, tcp_socket)
     *
     * 异步等待新连接。新连接到达时 handler 在 io_context::run() 线程中执行。
     * acceptor 必须是已打开且处于监听状态。
     */
    void async_accept(io_context& ctx, function<void(error_code, tcp_socket)> handler);

    /**
     * @brief 异步接受连接（带取消槽）
     * @param ctx 异步 I/O 执行上下文
     * @param slot 取消槽
     * @param handler 完成回调 void(error_code, tcp_socket)
     */
    void async_accept(io_context& ctx, cancellation_slot& slot, function<void(error_code, tcp_socket)> handler);

    /**
     * @brief 异步接受—任意可调用对象
     * @tparam Token 可调用对象类型，需满足 void(error_code, tcp_socket) 签名
     * @param ctx 异步 I/O 执行上下文
     * @param token 完成令牌
     */
    template <typename Token, enable_if_t<!is_same_v<decay_t<Token>, function<void(error_code, tcp_socket)>>, int> = 0>
    void async_accept(io_context& ctx, Token&& token) {
        async_accept(ctx, function<void(error_code, tcp_socket)>(forward<Token>(token)));
    }

    /**
     * @brief 异步接受—use_future
     * @param ctx 异步 I/O 执行上下文
     * @return 已连接的 socket
     */
    future<tcp_socket> async_accept(io_context& ctx, use_future_t /*unused*/) {
        async_result<use_future_t, void(error_code, tcp_socket)> result(use_future);
        async_accept(ctx, function<void(error_code, tcp_socket)>(result.get_handler()));
        return result.get();
    }

    /**
     * @brief 异步接受—detached（即发即忘）
     * @param ctx 异步 I/O 执行上下文
     */
    void async_accept(io_context& ctx, detached_t /*unused*/) {
        async_accept(ctx, function<void(error_code, tcp_socket)>([](error_code, tcp_socket) {}));
    }

#ifdef NEFORCE_STANDARD_20
    /**
     * @brief 异步接受—use_awaitable
     * @param ctx 异步 I/O 执行上下文
     * @return 协程等待结果
     */
    awaitable<error_code, tcp_socket> async_accept(io_context& ctx, use_awaitable_t /*unused*/) {
        async_result<use_awaitable_t, void(error_code, tcp_socket)> result(use_awaitable);
        async_accept(ctx, function<void(error_code, tcp_socket)>(result.get_handler()));
        return result.get();
    }
#endif
};

/** @} */ // TCP

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_TCP_TCP_ACCEPTOR_HPP__
