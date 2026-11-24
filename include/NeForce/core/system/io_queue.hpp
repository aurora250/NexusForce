#ifndef NEFORCE_CORE_SYSTEM_IO_QUEUE_HPP__
#define NEFORCE_CORE_SYSTEM_IO_QUEUE_HPP__

/**
 * @file io_queue.hpp
 * @brief Linux 异步 I/O 提交/完成队列
 *
 * 直接在 io_uring 内核接口之上实现提交队列（SQ）与完成队列（CQ）的管理如
 * 环形缓冲区映射、SQE 填充、内存屏障、io_uring_setup/io_uring_enter 系统调用。
 */

#include "NeForce/core/exception/error_code.hpp"
#include "NeForce/core/functional/function.hpp"
#include "NeForce/core/memory/unique_ptr.hpp"
#ifdef NEFORCE_PLATFORM_LINUX
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup IOQueue Linux IO 队列
 * @brief Linux 异步 I/O 队列
 * @{
 */

/**
 * @class io_queue
 * @brief Linux I/O 队列
 *
 * 管理一个内核异步 I/O 环，对外提供定长读取与写入的提交接口，
 * 完成项由 drain() 回收并就地调用各自的回调。
 *
 * @code
 * io_queue queue;
 * queue.submit_read(fd, buffer.data(), size, offset, [](error_code ec, size_t n) {});
 * @endcode
 */
class NEFORCE_API io_queue {
public:
    /// @brief 提交令牌，0 表示无效
    using token = uint64_t;

    /// @brief 完成回调类型
    using completion_handler = function<void(error_code, size_t)>;

    /// @brief 默认环容量
    static constexpr size_t default_entries = 512;

private:
    struct impl;
    unique_ptr<impl> impl_; ///< 内核环状态

public:
    /**
     * @brief 构造队列并创建内核环
     * @param entries 环容量，会向上取整到 2 的幂
     */
    explicit io_queue(size_t entries = default_entries);

    /**
     * @brief 析构队列
     *
     * 关闭环并释放映射，未完成的回调被丢弃。
     */
    ~io_queue();

    io_queue(const io_queue&) = delete;
    io_queue& operator=(const io_queue&) = delete;

    /**
     * @brief 探测当前内核是否支持异步 I/O 环
     * @return 支持返回 true
     */
    NEFORCE_NODISCARD static bool available() noexcept;

    /**
     * @brief 查询队列是否可用
     * @return 内核环创建成功返回 true
     */
    NEFORCE_NODISCARD bool is_open() const noexcept;

    /**
     * @brief 获取环的文件描述符
     * @return 环句柄，未打开时返回 -1
     */
    NEFORCE_NODISCARD native_handle_type native_handle() const noexcept;

    /**
     * @brief 查询未完成的提交数量
     * @return 已提交但尚未回收的操作数
     */
    NEFORCE_NODISCARD size_t pending() const noexcept;

    /**
     * @brief 提交一次定长读取
     * @param fd 文件描述符
     * @param buffer 读入缓冲区，至少 size 字节
     * @param size 要读取的字节数
     * @param offset 文件偏移量
     * @param handler 完成回调
     * @return 提交令牌，0 表示提交失败
     */
    token submit_read(native_handle_type fd, void* buffer, size_t size, uint64_t offset, completion_handler handler);

    /**
     * @brief 提交一次定长写入
     * @param fd 文件描述符
     * @param buffer 要写入的数据，至少 size 字节
     * @param size 要写入的字节数
     * @param offset 文件偏移量
     * @param handler 完成回调
     * @return 提交令牌，0 表示提交失败
     */
    token submit_write(native_handle_type fd, const void* buffer, size_t size, uint64_t offset,
                       completion_handler handler);

    /**
     * @brief 取消一次尚未完成的提交
     * @param id 提交时返回的令牌
     * @return 令牌有效并且取消请求已提交返回 true
     */
    bool cancel(token id) noexcept;

    /**
     * @brief 回收完成队列中的全部完成项并调用各自回调
     * @return 本次回收的完成项数量
     */
    size_t drain();

    /**
     * @brief 关闭内核环并释放映射
     */
    void close() noexcept;
};

/** @} */ // IOQueue

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_PLATFORM_LINUX
#endif // NEFORCE_CORE_SYSTEM_IO_QUEUE_HPP__
