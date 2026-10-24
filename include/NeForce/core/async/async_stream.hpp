#ifndef NEFORCE_CORE_ASYNC_ASYNC_STREAM_HPP__
#define NEFORCE_CORE_ASYNC_ASYNC_STREAM_HPP__

/**
 * @file async_stream.hpp
 * @brief 异步流抽象基类
 *
 * 为可异步读写的流类型提供统一多态接口。
 */

#include "NeForce/core/async/cancellation_slot.hpp"
#include "NeForce/core/async/use_awaitable.hpp"
#include "NeForce/core/async/io_context.hpp"
#include "NeForce/core/memory/buffer.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup AsyncComponents 异步组件
 * @brief 异步编程相关组件
 * @{
 */

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
 */
class NEFORCE_API async_stream {
public:
    /**
     * @brief 析构函数
     */
    virtual ~async_stream() = default;

    /**
     * @brief 异步读取
     * @param ctx 异步 I/O 执行上下文
     * @param buffer 接收缓冲区
     * @param handler 完成回调 void(error_code, size_t bytes_transferred)
     */
    virtual void async_read(io_context& ctx, memory_view<char> buffer, function<void(error_code, size_t)> handler) = 0;

    /**
     * @brief 带取消槽的异步读取
     * @param ctx 异步 I/O 执行上下文
     * @param buffer 接收缓冲区
     * @param slot 取消槽
     * @param handler 完成回调 void(error_code, size_t bytes_transferred)
     */
    virtual void async_read(io_context& ctx, memory_view<char> buffer, cancellation_slot& slot,
                            function<void(error_code, size_t)> handler) = 0;

    /**
     * @brief 异步写入
     * @param ctx 异步 I/O 执行上下文
     * @param buffer 发送缓冲区
     * @param handler 完成回调 void(error_code, size_t bytes_transferred)
     */
    virtual void async_write(io_context& ctx, memory_view<const char> buffer,
                             function<void(error_code, size_t)> handler) = 0;

    /**
     * @brief 带取消槽的异步写入
     * @param ctx 异步 I/O 执行上下文
     * @param buffer 发送缓冲区
     * @param slot 取消槽
     * @param handler 完成回调 void(error_code, size_t bytes_transferred)
     */
    virtual void async_write(io_context& ctx, memory_view<const char> buffer, cancellation_slot& slot,
                             function<void(error_code, size_t)> handler) = 0;

    /**
     * @brief 可调用对象的异步读取
     * @tparam Token 可调用对象类型，需满足 void(error_code, size_t) 签名
     * @param ctx 异步 I/O 执行上下文
     * @param buffer 接收缓冲区
     * @param token 完成令牌
     */
    template <typename Token, enable_if_t<!is_same_v<decay_t<Token>, function<void(error_code, size_t)>>, int> = 0>
    void async_read(io_context& ctx, memory_view<char> buffer, Token&& token) {
        async_read(ctx, buffer, function<void(error_code, size_t)>(_NEFORCE forward<Token>(token)));
    }

    /**
     * @brief future 异步读取
     * @param ctx 异步 I/O 执行上下文
     * @param buffer 接收缓冲区
     * @return 读取字节数
     */
    future<size_t> async_read(io_context& ctx, memory_view<char> buffer, use_future_t /*unused*/) {
        async_result<use_future_t, void(error_code, size_t)> result(use_future);
        async_read(ctx, buffer, function<void(error_code, size_t)>(result.get_handler()));
        return result.get();
    }

    /**
     * @brief detached 异步读取
     * @param ctx 异步 I/O 执行上下文
     * @param buffer 接收缓冲区
     */
    void async_read(io_context& ctx, memory_view<char> buffer, detached_t /*unused*/) {
        async_read(ctx, buffer, function<void(error_code, size_t)>([](error_code, size_t) {}));
    }

#ifdef NEFORCE_STANDARD_20
    /**
     * @brief awaitable 异步读取
     * @param ctx 异步 I/O 执行上下文
     * @param buffer 接收缓冲区
     * @return 可协程等待的结果
     */
    awaitable<error_code, size_t> async_read(io_context& ctx, memory_view<char> buffer, use_awaitable_t /*unused*/) {
        async_result<use_awaitable_t, void(error_code, size_t)> result(use_awaitable);
        async_read(ctx, buffer, function<void(error_code, size_t)>(result.get_handler()));
        return result.get();
    }
#endif

    /**
     * @brief 可调用对象的异步写入
     * @tparam Token 可调用对象类型，需满足 void(error_code, size_t) 签名
     * @param ctx 异步 I/O 执行上下文
     * @param buffer 发送缓冲区
     * @param token 完成令牌
     */
    template <typename Token, enable_if_t<!is_same_v<decay_t<Token>, function<void(error_code, size_t)>>, int> = 0>
    void async_write(io_context& ctx, memory_view<const char> buffer, Token&& token) {
        async_write(ctx, buffer, function<void(error_code, size_t)>(forward<Token>(token)));
    }

    /**
     * @brief future 异步写入
     * @param ctx 异步 I/O 执行上下文
     * @param buffer 发送缓冲区
     * @return 写入字节数
     */
    future<size_t> async_write(io_context& ctx, memory_view<const char> buffer, use_future_t /*unused*/) {
        async_result<use_future_t, void(error_code, size_t)> result(use_future);
        async_write(ctx, buffer, function<void(error_code, size_t)>(result.get_handler()));
        return result.get();
    }

    /**
     * @brief detached 异步写入
     * @param ctx 异步 I/O 执行上下文
     * @param buffer 发送缓冲区
     */
    void async_write(io_context& ctx, memory_view<const char> buffer, detached_t /*unused*/) {
        async_write(ctx, buffer, function<void(error_code, size_t)>([](error_code, size_t) {}));
    }

#ifdef NEFORCE_STANDARD_20
    /**
     * @brief awaitable 异步写入
     * @param ctx 异步 I/O 执行上下文
     * @param buffer 发送缓冲区
     * @return 可协程等待的结果
     */
    awaitable<error_code, size_t> async_write(io_context& ctx, memory_view<const char> buffer,
                                              use_awaitable_t /*unused*/) {
        async_result<use_awaitable_t, void(error_code, size_t)> result(use_awaitable);
        async_write(ctx, buffer, function<void(error_code, size_t)>(result.get_handler()));
        return result.get();
    }
#endif

    /**
     * @brief scatter-gather 异步读取
     * @param ctx 异步 I/O 执行上下文
     * @param bufs 多个接收缓冲区
     * @param handler 完成回调 void(error_code, size_t total_read)
     */
    void async_read(io_context& ctx, mutable_buffers& bufs, function<void(error_code, size_t)> handler);

    /**
     * @brief scatter-gather 异步写入
     * @param ctx 异步 I/O 执行上下文
     * @param bufs 多个发送缓冲区
     * @param handler 完成回调 void(error_code, size_t total_written)
     */
    void async_write(io_context& ctx, const_buffers& bufs, function<void(error_code, size_t)> handler);

    /**
     * @brief 异步读取到 dynamic_buffer
     * @param ctx 异步 I/O 执行上下文
     * @param buf 动态缓冲区
     * @param n 最少读取字节数
     * @param handler 完成回调 void(error_code, size_t bytes_read)
     */
    void async_read(io_context& ctx, dynamic_buffer& buf, size_t n, function<void(error_code, size_t)> handler);
};

/** @} */ // AsyncStream

/** @} */ // AsyncComponents

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_ASYNC_STREAM_HPP__
