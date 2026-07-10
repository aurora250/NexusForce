#ifndef NEFORCE_NETWORK_UDP_SOCKET_HPP__
#define NEFORCE_NETWORK_UDP_SOCKET_HPP__

/**
 * @file udp_socket.hpp
 * @brief UDP Socket实现
 *
 * 此文件提供了UDP Socket的完整实现，继承自ip_socket类。
 * 支持无连接的数据报发送和接收，以及已连接UDP socket的操作。
 */

#include "NeForce/core/async/cancellation_slot.hpp"
#include "NeForce/core/async/use_awaitable.hpp"
#include "NeForce/core/async/io_context.hpp"
#include "NeForce/core/functional/function.hpp"
#include "NeForce/core/memory/memory_view.hpp"
#include "NeForce/network/ip_socket.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup UDP UDP
 * @{
 */

/**
 * @class udp_socket
 * @brief UDP Socket类
 *
 * 实现UDP协议的数据报socket，支持无连接和已连接两种模式。
 * UDP是面向数据报的协议，提供不可靠、无连接的服务。
 *
 * 主要功能：
 * - UDP socket创建和打开
 * - 向指定端点发送数据报（send_to）
 * - 从任意端点接收数据报（receive_from）
 * - 已连接UDP socket的发送（send）和接收（receive）
 * - 获取发送方地址信息
 *
 * @note 数据报大小受MTU限制，通常建议不超过65507字节。
 */
class NEFORCE_API udp_socket final : public ip_socket {
public:
    /**
     * @brief 默认构造函数
     */
    udp_socket() = default;

    udp_socket(udp_socket&&) = default;
    udp_socket& operator=(udp_socket&&) = default;

    /**
     * @brief 打开UDP socket
     * @param f 地址族，默认IPv4
     * @throws socket_exception 创建失败时抛出
     *
     * 创建UDP协议的socket，使用SOCK_DGRAM类型和IPPROTO_UDP协议。
     */
    void open(family f = family::INET4);

    /**
     * @brief 向指定端点发送数据报
     * @param data 要发送的数据
     * @param endpoint 目标端点地址
     * @param flags 发送标志（通常为0）
     * @return 实际发送的字节数
     * @throws socket_exception 发送失败时抛出
     * @throws value_exception socket未打开或端点无效时抛出
     *
     * 向指定的远程端点发送UDP数据报，不需要socket处于连接状态。
     */
    ssize_t send_to(memory_view<const char> data, const ip_address& endpoint, int flags = 0);

    /**
     * @brief 向已连接的端点发送数据
     * @param data 要发送的数据
     * @param flags 发送标志（通常为0）
     * @return 实际发送的字节数
     * @throws socket_exception 发送失败时抛出
     * @throws value_exception socket未打开时抛出
     *
     * 向connect()关联的远程端点发送数据，需要先调用connect()。
     * 性能优于send_to()，因为不需要每次指定目标地址。
     */
    ssize_t send(memory_view<const char> data, int flags = 0);

    /**
     * @brief 接收数据报并获取发送方地址
     * @param buffer 接收缓冲区
     * @param flags 接收标志（通常为0）
     * @return 接收的字节数和发送方地址的pair
     * @throws socket_exception 接收失败时抛出
     * @throws value_exception socket未打开或缓冲区为空时抛出
     *
     * 接收UDP数据报，同时返回发送方的IP地址和端口。
     * 适用于无连接模式的UDP服务器。
     */
    pair<ssize_t, ip_address> receive_from(memory_view<char> buffer, int flags = 0);

    /**
     * @brief 从已连接的端点接收数据
     * @param buffer 接收缓冲区
     * @param flags 接收标志（通常为0）
     * @return 实际接收的字节数
     * @throws socket_exception 接收失败时抛出
     * @throws value_exception socket未打开或缓冲区为空时抛出
     *
     * 从connect()关联的远程端点接收数据，需要先调用connect()。
     * 只能接收来自该端点的数据。
     */
    ssize_t receive(memory_view<char> buffer, int flags = 0);

    /**
     * @brief 异步接收数据报
     * @param ctx 异步 I/O 执行上下文
     * @param buffer 接收缓冲区
     * @param handler 完成回调 void(error_code, size_t bytes, ip_address sender)
     *
     * 异步等待接收UDP数据报，返回发送方地址。
     * handler 在 io_context::run() 线程中执行。
     */
    void async_receive_from(io_context& ctx, memory_view<char> buffer,
                            function<void(error_code, size_t, ip_address)> handler);

    /**
     * @brief 异步接收数据报（带取消槽）
     * @param ctx 异步 I/O 执行上下文
     * @param buffer 接收缓冲区
     * @param slot 取消槽
     * @param handler 完成回调 void(error_code, size_t bytes, ip_address sender)
     */
    void async_receive_from(io_context& ctx, memory_view<char> buffer, cancellation_slot& slot,
                            function<void(error_code, size_t, ip_address)> handler);

    /**
     * @brief 异步发送数据报
     * @param ctx 异步 I/O 执行上下文
     * @param data 要发送的数据
     * @param endpoint 目标端点地址
     * @param handler 完成回调 void(error_code, size_t bytes_sent)
     *
     * 异步向指定端点发送UDP数据报。
     * handler 在 io_context::run() 线程中执行。
     */
    void async_send_to(io_context& ctx, memory_view<const char> data, const ip_address& endpoint,
                       function<void(error_code, size_t)> handler);

    /**
     * @brief 异步发送数据报（带取消槽）
     * @param ctx 异步 I/O 执行上下文
     * @param data 要发送的数据
     * @param endpoint 目标端点地址
     * @param slot 取消槽
     * @param handler 完成回调 void(error_code, size_t bytes_sent)
     */
    void async_send_to(io_context& ctx, memory_view<const char> data, const ip_address& endpoint,
                       cancellation_slot& slot, function<void(error_code, size_t)> handler);

    /**
     * @brief 异步接收数据报—任意可调用对象
     * @tparam Token 可调用对象类型，需满足 void(error_code, size_t, ip_address) 签名
     * @param ctx 异步 I/O 执行上下文
     * @param buffer 接收缓冲区
     * @param token 完成令牌
     */
    template <typename Token,
              enable_if_t<!is_same_v<decay_t<Token>, function<void(error_code, size_t, ip_address)>>, int> = 0>
    void async_receive_from(io_context& ctx, memory_view<char> buffer, Token&& token) {
        async_receive_from(ctx, buffer, function<void(error_code, size_t, ip_address)>(forward<Token>(token)));
    }

    /**
     * @brief 异步接收数据报—use_future
     * @param ctx 异步 I/O 执行上下文
     * @param buffer 接收缓冲区
     * @return future<pair<size_t, ip_address>> 接收字节数和发送方地址
     */
    auto async_receive_from(io_context& ctx, memory_view<char> buffer, use_future_t /*unused*/) {
        async_result<use_future_t, void(error_code, size_t, ip_address)> result(use_future);
        async_receive_from(ctx, buffer, function<void(error_code, size_t, ip_address)>(result.get_handler()));
        return result.get();
    }

    /**
     * @brief 异步接收数据报—detached（即发即忘）
     * @param ctx 异步 I/O 执行上下文
     * @param buffer 接收缓冲区
     */
    void async_receive_from(io_context& ctx, memory_view<char> buffer, detached_t /*unused*/) {
        async_receive_from(ctx, buffer,
                           function<void(error_code, size_t, ip_address)>([](error_code, size_t, ip_address) {}));
    }

#ifdef NEFORCE_STANDARD_20
    /**
     * @brief 异步接收数据报—use_awaitable
     * @param ctx 异步 I/O 执行上下文
     * @param buffer 接收缓冲区
     * @return awaitable<pair<size_t, ip_address>> 可协程等待的结果
     */
    auto async_receive_from(io_context& ctx, memory_view<char> buffer, use_awaitable_t /*unused*/) {
        async_result<use_awaitable_t, void(error_code, size_t, ip_address)> result(use_awaitable);
        async_receive_from(ctx, buffer, function<void(error_code, size_t, ip_address)>(result.get_handler()));
        return result.get();
    }
#endif

    /**
     * @brief 异步发送数据报—任意可调用对象
     * @tparam Token 可调用对象类型，需满足 void(error_code, size_t) 签名
     * @param ctx 异步 I/O 执行上下文
     * @param data 要发送的数据
     * @param endpoint 目标端点地址
     * @param token 完成令牌
     */
    template <typename Token, enable_if_t<!is_same_v<decay_t<Token>, function<void(error_code, size_t)>>, int> = 0>
    void async_send_to(io_context& ctx, memory_view<const char> data, const ip_address& endpoint, Token&& token) {
        async_send_to(ctx, data, endpoint, function<void(error_code, size_t)>(forward<Token>(token)));
    }

    /**
     * @brief 异步发送数据报—use_future
     * @param ctx 异步 I/O 执行上下文
     * @param data 要发送的数据
     * @param endpoint 目标端点地址
     * @return future<size_t> 发送字节数
     */
    auto async_send_to(io_context& ctx, memory_view<const char> data, const ip_address& endpoint,
                       use_future_t /*unused*/) {
        async_result<use_future_t, void(error_code, size_t)> result(use_future);
        async_send_to(ctx, data, endpoint, function<void(error_code, size_t)>(result.get_handler()));
        return result.get();
    }

    /**
     * @brief 异步发送数据报—detached（即发即忘）
     * @param ctx 异步 I/O 执行上下文
     * @param data 要发送的数据
     * @param endpoint 目标端点地址
     */
    void async_send_to(io_context& ctx, memory_view<const char> data, const ip_address& endpoint,
                       detached_t /*unused*/) {
        async_send_to(ctx, data, endpoint, function<void(error_code, size_t)>([](error_code, size_t) {}));
    }

#ifdef NEFORCE_STANDARD_20
    /**
     * @brief 异步发送数据报—use_awaitable
     * @param ctx 异步 I/O 执行上下文
     * @param data 要发送的数据
     * @param endpoint 目标端点地址
     * @return awaitable<size_t> 可协程等待的结果
     */
    auto async_send_to(io_context& ctx, memory_view<const char> data, const ip_address& endpoint,
                       use_awaitable_t /*unused*/) {
        async_result<use_awaitable_t, void(error_code, size_t)> result(use_awaitable);
        async_send_to(ctx, data, endpoint, function<void(error_code, size_t)>(result.get_handler()));
        return result.get();
    }
#endif
};

/** @} */ // UDP

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_UDP_SOCKET_HPP__
