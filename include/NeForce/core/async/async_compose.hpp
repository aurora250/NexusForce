#ifndef NEFORCE_CORE_ASYNC_ASYNC_COMPOSE_HPP__
#define NEFORCE_CORE_ASYNC_ASYNC_COMPOSE_HPP__

/**
 * @file async_compose.hpp
 * @brief 组合异步操作
 *
 * 提供将多个基础异步操作组合为高层操作的基础设施。
 */

#include "NeForce/core/async/async_stream.hpp"
#include "NeForce/core/exception/error_code.hpp"
#include "NeForce/core/functional/function.hpp"
#include "NeForce/core/container/vector.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup AsyncCompose 组合操作
 * @brief 异步组合操作
 * @{
 */

/**
 * @brief 异步读取指定字节数
 * @param stream 异步流
 * @param ctx 异步 I/O 执行上下文
 * @param buffer 接收缓冲区（大小决定读取量）
 * @param handler 完成回调 void(error_code, size_t total_read)
 *
 * 持续调用 async_stream::async_read 直到填满 buffer 或连接关闭。
 */
void async_read(async_stream& stream, io_context& ctx, vector<char>& buffer,
                function<void(error_code, size_t)> handler);

/**
 * @brief 异步写入指定字节数
 * @param stream 异步流
 * @param ctx 异步 I/O 执行上下文
 * @param data 要发送的数据
 * @param size 数据大小
 * @param handler 完成回调 void(error_code, size_t total_written)
 *
 * 持续调用 async_stream::async_write 直到所有数据发送完毕。
 */
void async_write(async_stream& stream, io_context& ctx, const void* data, size_t size,
                 function<void(error_code, size_t)> handler);

/** @} */ // AsyncCompose

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_ASYNC_COMPOSE_HPP__
