#ifndef NEFORCE_CORE_FILE_FILE_ASYNC_HPP__
#define NEFORCE_CORE_FILE_FILE_ASYNC_HPP__

/**
 * @file file_async.hpp
 * @brief 异步文件I/O操作类
 *
 * 提供通过 io_context 驱动的异步文件读写操作。
 * Windows 使用 IOCP，Linux 使用 io_uring（内核 5.1+）。
 */

#include "NeForce/core/async/cancellation_slot.hpp"
#include "NeForce/core/async/io_context.hpp"
#include "NeForce/core/exception/error_code.hpp"
#include "NeForce/core/file/file_constants.hpp"
#include "NeForce/core/string/string.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup File 文件
 * @brief 文件操作
 * @{
 */

/**
 * @class file_async
 * @brief 文件异步I/O管理类
 *
 * 提供对文件句柄的异步读写操作，由 io_context 事件循环驱动。
 * handler 在 io_context::run() 线程中执行。
 *
 * @note 不持有文件句柄所有权，句柄生命周期由调用方保证。
 */
class NEFORCE_API file_async {
public:
#ifdef NEFORCE_PLATFORM_WINDOWS
    using size_type = ::DWORD;          ///< 大小类型
    using difference_type = ::LONGLONG; ///< 偏移量类型
#else
    using size_type = size_t;        ///< 大小类型
    using difference_type = ::off_t; ///< 偏移量类型
#endif

    using native_handle_type = _NEFORCE native_handle_type; ///< 原生文件句柄类型

private:
    native_handle_type handle_;
    io_context* ctx_{nullptr};
    function<void(error_code, size_type)> pending_handler_;

    void ensure_iocp(io_context& ctx);

    void do_async_read(io_context& ctx, string& buffer, size_type size, difference_type offset, cancellation_slot* slot,
                       function<void(error_code, size_type)> handler);

    void do_async_write(io_context& ctx, string data, size_type size, difference_type offset, cancellation_slot* slot,
                        function<void(error_code, size_type)> handler);

public:
    /**
     * @brief 构造函数
     * @param handle 已打开的文件句柄
     */
    explicit file_async(native_handle_type handle);

    /**
     * @brief 析构函数
     */
    ~file_async();

    file_async(const file_async&) = delete;
    file_async& operator=(const file_async&) = delete;

    /**
     * @brief 移动构造函数
     * @param other 要移动的对象
     */
    file_async(file_async&& other) noexcept;

    /**
     * @brief 移动赋值运算符
     * @param other 要移动的对象
     * @return 自身引用
     */
    file_async& operator=(file_async&& other) noexcept;

    /**
     * @brief 异步读取数据
     * @param ctx 异步 I/O 执行上下文
     * @param buffer 输出缓冲区
     * @param size 要读取的字节数
     * @param handler 完成回调 void(error_code, size_type bytes_read)
     */
    void async_read(io_context& ctx, string& buffer, size_type size, function<void(error_code, size_type)> handler) {
        do_async_read(ctx, buffer, size, -1, nullptr, move(handler));
    }

    /**
     * @brief 指定偏移量异步读取数据
     * @param ctx 异步 I/O 执行上下文
     * @param buffer 输出缓冲区
     * @param size 要读取的字节数
     * @param offset 文件偏移量，-1 使用当前位置
     * @param handler 完成回调 void(error_code, size_type bytes_read)
     */
    void async_read(io_context& ctx, string& buffer, size_type size, difference_type offset,
                    function<void(error_code, size_type)> handler) {
        do_async_read(ctx, buffer, size, offset, nullptr, move(handler));
    }

    /**
     * @brief 带取消槽异步读取数据
     * @param ctx 异步 I/O 执行上下文
     * @param buffer 输出缓冲区
     * @param size 要读取的字节数
     * @param slot 取消槽
     * @param handler 完成回调 void(error_code, size_type bytes_read)
     */
    void async_read(io_context& ctx, string& buffer, size_type size, cancellation_slot& slot,
                    function<void(error_code, size_type)> handler) {
        do_async_read(ctx, buffer, size, -1, &slot, move(handler));
    }

    /**
     * @brief 异步写入数据
     * @param ctx 异步 I/O 执行上下文
     * @param data 要写入的数据
     * @param size 要写入的字节数
     * @param handler 完成回调 void(error_code, size_type bytes_written)
     *
     * 数据被内部拷贝，调用者可以立即释放。
     */
    void async_write(io_context& ctx, string data, size_type size, function<void(error_code, size_type)> handler) {
        do_async_write(ctx, move(data), size, -1, nullptr, move(handler));
    }

    /**
     * @brief 指定偏移量异步写入数据
     * @param ctx 异步 I/O 执行上下文
     * @param data 要写入的数据
     * @param size 要写入的字节数
     * @param offset 文件偏移量，-1 使用当前位置
     * @param handler 完成回调 void(error_code, size_type bytes_written)
     */
    void async_write(io_context& ctx, string data, size_type size, difference_type offset,
                     function<void(error_code, size_type)> handler) {
        do_async_write(ctx, move(data), size, offset, nullptr, move(handler));
    }

    /**
     * @brief 带取消槽异步写入数据
     * @param ctx 异步 I/O 执行上下文
     * @param data 要写入的数据
     * @param size 要写入的字节数
     * @param slot 取消槽
     * @param handler 完成回调 void(error_code, size_type bytes_written)
     */
    void async_write(io_context& ctx, string data, size_type size, cancellation_slot& slot,
                     function<void(error_code, size_type)> handler) {
        do_async_write(ctx, move(data), size, -1, &slot, move(handler));
    }

    /**
     * @brief 获取原生文件句柄
     * @return 原生文件句柄
     */
    NEFORCE_NODISCARD native_handle_type native_handle() const noexcept { return handle_; }
};

/** @} */ // File

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_FILE_ASYNC_HPP__
