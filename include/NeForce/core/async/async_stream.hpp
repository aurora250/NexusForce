#ifndef NEFORCE_CORE_ASYNC_ASYNC_STREAM_HPP__
#define NEFORCE_CORE_ASYNC_ASYNC_STREAM_HPP__

/**
 * @file async_stream.hpp
 * @brief 异步流抽象基类
 *
 * 为可异步读写的流类型提供统一多态接口。
 * tcp_socket、ssl_stream 等实现纯虚方法即可获得完整的完成令牌支持。
 */

#include "NeForce/core/async/cancellation_slot.hpp"
#include "NeForce/core/async/use_awaitable.hpp"
#include "NeForce/core/async/io_context.hpp"
#include "NeForce/core/memory/buffer.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup AsyncStream 异步流
 * @brief 异步 I/O 流抽象
 * @{
 */

/**
 * @class async_stream
 * @brief 异步流抽象基类
 *
 * 定义异步读写的纯虚接口。派生类实现四个纯虚方法后，
 * 自动获得 use_future、detached、use_awaitable 等完成令牌支持。
 *
 * @note 仅抽象异步 read/write 行为——连接（connect/accept）和地址操作留在具体类中。
 */
class NEFORCE_API async_stream {
public:
    /**
     * @brief 析构函数
     */
    virtual ~async_stream() = default;

    /**
     * @brief 异步读取数据
     * @param ctx 异步 I/O 执行上下文
     * @param buffer 接收缓冲区
     * @param handler 完成回调 void(error_code, size_t bytes_transferred)
     */
    virtual void async_read(io_context& ctx, memory_view<char> buffer, function<void(error_code, size_t)> handler) = 0;

    /**
     * @brief 异步读取（带取消槽）
     * @param ctx 异步 I/O 执行上下文
     * @param buffer 接收缓冲区
     * @param slot 取消槽
     * @param handler 完成回调 void(error_code, size_t bytes_transferred)
     */
    virtual void async_read(io_context& ctx, memory_view<char> buffer, cancellation_slot& slot,
                            function<void(error_code, size_t)> handler) = 0;

    /**
     * @brief 异步写入数据
     * @param ctx 异步 I/O 执行上下文
     * @param buffer 发送缓冲区
     * @param handler 完成回调 void(error_code, size_t bytes_transferred)
     */
    virtual void async_write(io_context& ctx, memory_view<const char> buffer,
                             function<void(error_code, size_t)> handler) = 0;

    /**
     * @brief 异步写入（带取消槽）
     * @param ctx 异步 I/O 执行上下文
     * @param buffer 发送缓冲区
     * @param slot 取消槽
     * @param handler 完成回调 void(error_code, size_t bytes_transferred)
     */
    virtual void async_write(io_context& ctx, memory_view<const char> buffer, cancellation_slot& slot,
                             function<void(error_code, size_t)> handler) = 0;

    /**
     * @brief 异步读取—任意可调用对象
     * @tparam Token 可调用对象类型，需满足 void(error_code, size_t) 签名
     * @param ctx 异步 I/O 执行上下文
     * @param buffer 接收缓冲区
     * @param token 完成令牌
     *
     * 委托到纯虚 async_read(ctx, buffer, function<...>)。
     */
    template <typename Token, enable_if_t<!is_same_v<decay_t<Token>, function<void(error_code, size_t)>>, int> = 0>
    void async_read(io_context& ctx, memory_view<char> buffer, Token&& token) {
        async_read(ctx, buffer, function<void(error_code, size_t)>(forward<Token>(token)));
    }

    /**
     * @brief 异步读取—use_future
     * @param ctx 异步 I/O 执行上下文
     * @param buffer 接收缓冲区
     * @return future<size_t> 读取字节数
     */
    auto async_read(io_context& ctx, memory_view<char> buffer, use_future_t /*unused*/) {
        async_result<use_future_t, void(error_code, size_t)> result(use_future);
        async_read(ctx, buffer, function<void(error_code, size_t)>(result.get_handler()));
        return result.get();
    }

    /**
     * @brief 异步读取—detached（即发即忘）
     * @param ctx 异步 I/O 执行上下文
     * @param buffer 接收缓冲区
     */
    void async_read(io_context& ctx, memory_view<char> buffer, detached_t /*unused*/) {
        async_read(ctx, buffer, function<void(error_code, size_t)>([](error_code, size_t) {}));
    }

#ifdef NEFORCE_STANDARD_20
    /**
     * @brief 异步读取—use_awaitable
     * @param ctx 异步 I/O 执行上下文
     * @param buffer 接收缓冲区
     * @return awaitable<size_t> 可协程等待的结果
     */
    auto async_read(io_context& ctx, memory_view<char> buffer, use_awaitable_t /*unused*/) {
        async_result<use_awaitable_t, void(error_code, size_t)> result(use_awaitable);
        async_read(ctx, buffer, function<void(error_code, size_t)>(result.get_handler()));
        return result.get();
    }
#endif

    /**
     * @brief 异步写入—任意可调用对象
     * @tparam Token 可调用对象类型，需满足 void(error_code, size_t) 签名
     * @param ctx 异步 I/O 执行上下文
     * @param buffer 发送缓冲区
     * @param token 完成令牌
     *
     * 委托到纯虚 async_write(ctx, buffer, function<...>)。
     */
    template <typename Token, enable_if_t<!is_same_v<decay_t<Token>, function<void(error_code, size_t)>>, int> = 0>
    void async_write(io_context& ctx, memory_view<const char> buffer, Token&& token) {
        async_write(ctx, buffer, function<void(error_code, size_t)>(forward<Token>(token)));
    }

    /**
     * @brief 异步写入—use_future
     * @param ctx 异步 I/O 执行上下文
     * @param buffer 发送缓冲区
     * @return future<size_t> 写入字节数
     */
    auto async_write(io_context& ctx, memory_view<const char> buffer, use_future_t /*unused*/) {
        async_result<use_future_t, void(error_code, size_t)> result(use_future);
        async_write(ctx, buffer, function<void(error_code, size_t)>(result.get_handler()));
        return result.get();
    }

    /**
     * @brief 异步写入—detached（即发即忘）
     * @param ctx 异步 I/O 执行上下文
     * @param buffer 发送缓冲区
     */
    void async_write(io_context& ctx, memory_view<const char> buffer, detached_t /*unused*/) {
        async_write(ctx, buffer, function<void(error_code, size_t)>([](error_code, size_t) {}));
    }

#ifdef NEFORCE_STANDARD_20
    /**
     * @brief 异步写入—use_awaitable
     * @param ctx 异步 I/O 执行上下文
     * @param buffer 发送缓冲区
     * @return awaitable<size_t> 可协程等待的结果
     */
    auto async_write(io_context& ctx, memory_view<const char> buffer, use_awaitable_t /*unused*/) {
        async_result<use_awaitable_t, void(error_code, size_t)> result(use_awaitable);
        async_write(ctx, buffer, function<void(error_code, size_t)>(result.get_handler()));
        return result.get();
    }
#endif

    /**
     * @brief 异步读取—scatter-gather
     * @param ctx 异步 I/O 执行上下文
     * @param bufs 多个接收缓冲区
     * @param handler 完成回调 void(error_code, size_t total_read)
     *
     * 依次填满每个缓冲区，委托到纯虚 async_read。
     */
    void async_read(io_context& ctx, mutable_buffers& bufs, function<void(error_code, size_t)> handler);

    /**
     * @brief 异步写入—scatter-gather
     * @param ctx 异步 I/O 执行上下文
     * @param bufs 多个发送缓冲区
     * @param handler 完成回调 void(error_code, size_t total_written)
     *
     * 依次发送每个缓冲区，委托到纯虚 async_write。
     */
    void async_write(io_context& ctx, const_buffers& bufs, function<void(error_code, size_t)> handler);

    /**
     * @brief 异步读取到 dynamic_buffer
     * @param ctx 异步 I/O 执行上下文
     * @param buf 动态缓冲区（prepare/commit/consume）
     * @param n 最少读取字节数
     * @param handler 完成回调 void(error_code, size_t bytes_read)
     *
     * 调用 buf.prepare(n) 获取写入区域，完成后 buf.commit(n)。
     */
    void async_read(io_context& ctx, dynamic_buffer& buf, size_t n, function<void(error_code, size_t)> handler) {
        auto region = buf.prepare(n);
        async_read(ctx, region, [&buf, h = move(handler)](error_code ec, size_t bytes) mutable {
            if (!ec) {
                buf.commit(bytes);
            }
            h(ec, bytes);
        });
    }
};

/** @} */ // AsyncStream

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_ASYNC_STREAM_HPP__
