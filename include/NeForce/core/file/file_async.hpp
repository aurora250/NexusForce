#ifndef NEFORCE_CORE_FILE_FILE_ASYNC_HPP__
#define NEFORCE_CORE_FILE_FILE_ASYNC_HPP__

/**
 * @file file_async.hpp
 * @brief 异步文件I/O操作类
 *
 * 此文件提供了异步文件读写操作的支持。
 */

#include "NeForce/core/async/mutex.hpp"
#include "NeForce/core/container/unordered_map.hpp"
#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/file/file_constants.hpp"
#include "NeForce/core/string/string.hpp"
#ifdef NEFORCE_PLATFORM_LINUX
#include <aio.h>
#endif
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
 * 提供对文件句柄的异步读写操作：
 * - 非阻塞异步读写
 * - 支持超时等待
 * - 支持取消操作
 * - 线程安全的操作提交
 *
 * @note 对同一文件的并发操作顺序由操作系统决定。
 * @note 不持有文件句柄所有权，句柄生命周期由调用方保证。
 */
class NEFORCE_API file_async {
public:
#ifdef NEFORCE_PLATFORM_WINDOWS
    using size_type          = ::DWORD;      ///< 大小类型
    using difference_type    = ::LONGLONG;   ///< 偏移量类型
    using aiocb_type         = ::OVERLAPPED; ///< 异步I/O控制块类型
#else
    using size_type          = size_t;       ///< 大小类型
    using difference_type    = ::off_t;      ///< 偏移量类型
    using aiocb_type         = ::aiocb;      ///< 异步I/O控制块类型
#endif

    using native_handle_type = _NEFORCE native_handle_type; ///< 原生文件句柄类型

private:
    /**
     * @struct async_context
     * @brief 异步操作上下文
     */
    struct async_context {
        string data{};            ///< 写操作的数据副本
        string* buffer = nullptr; ///< 读操作的目标缓冲区
        aiocb_type* cb = nullptr; ///< 异步I/O控制块
        bool is_write = false;    ///< 是否为写操作

        /**
         * @brief 写操作上下文构造函数
         * @param d 要写入的数据
         */
        explicit async_context(string&& d);

        /**
         * @brief 读操作上下文构造函数
         * @param buf 目标缓冲区
         */
        explicit async_context(string* buf);

        /**
         * @brief 析构函数
         */
        ~async_context();
    };

public:
    /**
     * @struct async_result
     * @brief 异步操作结果句柄
     *
     * 表示一个异步操作的状态，用于等待操作完成和获取结果。
     */
    struct async_result {
        bool completed = false;           ///< 操作是否已完成
        size_t bytes_transferred = 0;     ///< 实际传输的字节数
        int error_code = 0;               ///< 错误码（0表示成功）
        aiocb_type* cb = nullptr;         ///< 异步I/O控制块指针
        async_context* user_context = nullptr; ///< 用户上下文指针
    };

private:
    native_handle_type handle_;                    ///< 文件句柄
    mutable mutex mutex_;                          ///< 保护操作列表的互斥锁
    mutable vector<aiocb_type*> operations_;       ///< 进行中的异步操作列表
    mutable unordered_map<aiocb_type*, async_context*> contexts_; ///< 操作上下文映射

    bool complete_result(async_result& result, size_type bytes) noexcept;
    bool check_completion(async_result& result) noexcept;

public:
    /**
     * @brief 构造函数
     * @param handle 已打开的文件句柄
     *
     * 关联指定的文件句柄。
     */
    explicit file_async(native_handle_type handle);

    /**
     * @brief 析构函数
     *
     * 取消所有进行中的异步操作并释放资源。
     */
    ~file_async();

    file_async(const file_async&) = delete;
    file_async& operator =(const file_async&) = delete;

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
    file_async& operator =(file_async&& other) noexcept;

    /**
     * @brief 提交异步读取操作
     * @param buffer 输出缓冲区（会预先分配空间）
     * @param size 要读取的字节数
     * @param offset 文件偏移量，-1表示使用当前文件位置
     * @return 异步操作结果句柄
     *
     * 从文件读取数据到指定缓冲区。如果size为0，立即返回已完成的结果。
     * 如果offset为-1，使用当前文件指针位置；否则从指定偏移量读取。
     */
    async_result read(string& buffer, size_type size, difference_type offset = -1) const;

    /**
     * @brief 提交异步写入操作
     * @param data 要写入的数据
     * @param size 要写入的字节数，最大值表示写入全部数据
     * @param offset 文件偏移量，-1表示使用当前文件位置
     * @return 异步操作结果句柄
     *
     * 将数据写入文件。数据会被复制到内部存储，调用者可以立即释放。
     * 如果size为最大值，写入整个字符串；否则写入指定长度。
     */
    async_result write(string data, size_type size, difference_type offset = -1);

    /**
     * @brief 等待异步操作完成
     * @param result 异步操作结果句柄
     * @param timeout_ms 超时时间（毫秒），最大值表示无限等待
     * @return 操作完成返回true，超时或失败返回false
     *
     * 阻塞当前线程直到异步操作完成或超时。
     * 如果操作已完成，立即返回。
     */
    bool wait(async_result& result, uint32_t timeout_ms = numeric_traits<uint32_t>::max());

    /**
     * @brief 取消异步操作
     * @param result 异步操作结果句柄
     *
     * 尝试取消进行中的异步操作。取消成功后，操作状态会被标记为已完成，
     * error_code会被设置为取消错误码。
     */
    void cancel(async_result& result) noexcept;
};

/** @} */ // File

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_FILE_ASYNC_HPP__
