#ifndef MSTL_CORE_SYSTEM_PIPE_HPP__
#define MSTL_CORE_SYSTEM_PIPE_HPP__

/**
 * @file pipe.hpp
 * @brief 跨平台管道封装
 *
 * 提供管道创建、读写和管理功能。
 */

#include "MSTL/core/string/string.hpp"
MSTL_BEGIN_NAMESPACE__

/**
 * @defgroup Exceptions 异常类集
 * @brief MSTL异常类集
 * @{
 */

/**
 * @struct pipe_exception
 * @extends system_exception
 * @brief 管道操作异常
 */
MSTL_ERROR_BUILD_FINAL_CLASS(pipe_exception, system_exception, "Pipe Operation Failed.")

/** @} */ // Exceptions

/**
 * @defgroup Pipe 管道
 * @brief 管道管理工具
 * @{
 */

/**
 * @class pipe
 * @brief 管道类
 *
 * 支持管道的创建、读写和关闭。
 */
class MSTL_API pipe {
public:
    using native_handle_type = _MSTL native_handle_type;

private:
#ifdef MSTL_PLATFORM_WINDOWS__
    native_handle_type read_handle_ = nullptr;
    native_handle_type write_handle_ = nullptr;
#else
    native_handle_type fds_[2] = {-1, -1};
#endif

public:
    /**
     * @brief 默认构造函数
     *
     * 创建一个未初始化的管道对象。
     */
    pipe() noexcept {}

    /**
     * @brief 创建管道
     * @param inheritable 子进程是否可继承（仅Windows）
     * @throws pipe_exception 创建失败时抛出
     */
    explicit pipe(bool inheritable);

    /**
     * @brief 析构函数
     *
     * 自动关闭管道句柄。
     */
    ~pipe();

    pipe(const pipe&) = delete;
    pipe& operator =(const pipe&) = delete;

    /**
     * @brief 移动构造函数
     */
    pipe(pipe&& other) noexcept;

    /**
     * @brief 移动赋值运算符
     */
    pipe& operator =(pipe&& other) noexcept;

    /**
     * @brief 从管道读取数据
     * @param buffer 缓冲区
     * @param size 缓冲区大小
     * @return 实际读取的字节数，-1表示错误
     */
    int read(void* buffer, size_t size) noexcept;

    /**
     * @brief 从管道读取所有可用数据（非阻塞）
     * @return 读取的数据字符串
     */
    MSTL_NODISCARD string read_available() noexcept;

    /**
     * @brief 向管道写入数据
     * @param data 要写入的数据
     * @param size 数据大小
     * @return 实际写入的字节数，-1表示错误
     */
    int write(const void* data, size_t size) noexcept;

    /**
     * @brief 关闭管道的读端
     */
    void close_read() noexcept;

    /**
     * @brief 关闭管道的写端
     */
    void close_write() noexcept;

    /**
     * @brief 关闭管道的所有端
     */
    void close() noexcept;

    /**
     * @brief 检查管道是否有效
     * @return 管道是否已创建且未关闭
     */
    MSTL_NODISCARD bool is_valid() const noexcept;

    /**
     * @brief 获取读端句柄
     */
    MSTL_NODISCARD native_handle_type native_read_handle() const noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
        return read_handle_;
#else
        return fds_[0];
#endif
    }

    /**
     * @brief 获取写端句柄
     */
    MSTL_NODISCARD native_handle_type native_write_handle() const noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
        return write_handle_;
#else
        return fds_[1];
#endif
    }

    /**
     * @brief 分离读端句柄（调用者负责关闭）
     */
    MSTL_NODISCARD native_handle_type detach_read_handle() noexcept;

    /**
     * @brief 分离写端句柄（调用者负责关闭）
     */
    MSTL_NODISCARD native_handle_type detach_write_handle() noexcept;
};

/** @} */ // Pipe

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_SYSTEM_PIPE_HPP__
