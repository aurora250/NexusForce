#ifndef NEFORCE_CORE_FILE_FILE_ASYNC_HPP__
#define NEFORCE_CORE_FILE_FILE_ASYNC_HPP__
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
 * @class file_async
 * @brief 文件异步 I/O 管理类
 *
 * 不持有文件所有权。
 */
class NEFORCE_API file_async {
public:
#ifdef NEFORCE_PLATFORM_WINDOWS
    using size_type          = ::DWORD;
    using difference_type    = ::LONGLONG;
    using aiocb_type         = ::OVERLAPPED;
#else
    using size_type          = size_t;
    using difference_type    = ::off_t;
    using aiocb_type         = ::aiocb;
#endif

    using native_handle_type = _NEFORCE native_handle_type;

private:
    /**
     * @struct async_context
     * @brief 异步操作上下文
     */
    struct async_context {
        string data{};
        string* buffer = nullptr;
        aiocb_type* cb = nullptr;
        bool is_write = false;

        explicit async_context(string&& d);
        explicit async_context(string* buf);
        ~async_context();
    };

public:
    /**
     * @struct async_result
     * @brief 异步操作结果句柄
     */
    struct async_result {
        bool completed = false;
        size_t bytes_transferred = 0;
        int error_code = 0;
        aiocb_type* cb = nullptr;
        async_context* user_context = nullptr;
    };

private:
    native_handle_type handle_;
    mutable mutex mutex_;
    mutable vector<aiocb_type*> operations_;
    mutable unordered_map<aiocb_type*, async_context*> contexts_;

    bool complete_result(async_result& result, size_type bytes) noexcept;
    bool check_completion(async_result& result) noexcept;

public:
    explicit file_async(native_handle_type handle);
    ~file_async();

    file_async(const file_async&) = delete;
    file_async& operator =(const file_async&) = delete;
    file_async(file_async&& other) noexcept;
    file_async& operator =(file_async&& other) noexcept;

    /**
     * @brief 提交异步读取
     * @param buffer 输出缓冲区
     * @param size 读取字节数
     * @param offset 文件偏移（-1 表示当前位置）
     */
    async_result read(string& buffer, size_type size, difference_type offset = -1) const;

    /**
     * @brief 提交异步写入
     * @param data 写入数据
     * @param size 写入字节数
     * @param offset 文件偏移（-1 表示当前位置）
     */
    async_result write(string data, size_type size, difference_type offset = -1);

    /**
     * @brief 等待异步操作完成
     * @param result 异步结果句柄
     * @param timeout_ms 毫秒超时，默认无限等待
     */
    bool wait(async_result& result, uint32_t timeout_ms = numeric_traits<uint32_t>::max());

    /**
     * @brief 取消异步操作
     */
    void cancel(async_result& result) noexcept;
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_FILE_ASYNC_HPP__
