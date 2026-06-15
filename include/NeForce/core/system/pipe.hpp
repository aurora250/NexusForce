#ifndef NEFORCE_CORE_SYSTEM_PIPE_HPP__
#define NEFORCE_CORE_SYSTEM_PIPE_HPP__

/**
 * @file pipe.hpp
 * @brief 跨平台管道封装
 *
 * 提供管道创建、读写和管理功能。
 */

#include "NeForce/core/string/string.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Exceptions 异常类集
 * @brief 异常类集
 * @{
 */

/**
 * @struct pipe_exception
 * @brief 管道操作异常
 */
struct pipe_exception final : system_exception {
    explicit pipe_exception(const char* info = "Pipe Operation Failed.", const char* type = static_type,
                            const int code = 0) noexcept :
    system_exception(info, type, code) {}

    explicit pipe_exception(const exception& e) :
    system_exception(e) {}

    ~pipe_exception() override = default;
    static constexpr auto static_type = "pipe_exception";
};

/** @} */ // Exceptions

/**
 * @defgroup Pipe 管道
 * @brief 管道管理工具
 * @{
 */

/**
 * @class pipe
 * @brief 匿名管道类
 *
 * 支持管道的创建、读写和关闭。
 * 默认创建阻塞式管道，可通过参数或 set_nonblocking() 切换非阻塞模式。
 */
class NEFORCE_API pipe {
public:
    using native_handle_type = _NEFORCE native_handle_type;

private:
#ifdef NEFORCE_PLATFORM_WINDOWS
    native_handle_type read_handle_ = nullptr;
    native_handle_type write_handle_ = nullptr;
#else
    native_handle_type fds_[2] = {-1, -1};
#endif
    bool nonblocking_{false}; ///< 非阻塞模式标志

public:
    /**
     * @brief 默认构造函数
     *
     * 创建一个未初始化的管道对象。
     */
    pipe() noexcept = default;

    /**
     * @brief 创建管道并设置非阻塞模式
     * @param inheritable 子进程是否可继承（Windows）
     * @param nonblocking 是否启用非阻塞模式
     * @throws pipe_exception 创建失败时抛出
     *
     * 非阻塞模式下，read() 在无数据可用时返回 0 而非阻塞等待，
     * write() 在缓冲区满时返回 0 而非阻塞等待。
     */
    explicit pipe(bool inheritable, bool nonblocking = false);

    /**
     * @brief 析构函数
     *
     * 自动关闭管道句柄。
     */
    ~pipe();

    pipe(const pipe&) = delete;
    pipe& operator=(const pipe&) = delete;

    /**
     * @brief 移动构造函数
     */
    pipe(pipe&& other) noexcept;

    /**
     * @brief 移动赋值运算符
     */
    pipe& operator=(pipe&& other) noexcept;

    /**
     * @brief 忽略 SIGPIPE 信号
     * @note 在 POSIX 系统上，向没有读端的管道写入会触发 SIGPIPE 信号，默认行为是终止进程。
     *       调用此函数后，系统将忽略该信号，改为由 write() 返回 -1 并设置 errno = EPIPE。
     *       此操作是进程全局的，但也是线程安全的传统做法。
     *       在 Windows 上该函数无任何效果。
     */
    static void ignore_sigpipe() noexcept;

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
    NEFORCE_NODISCARD string read_available();

    /**
     * @brief 设置管道非阻塞模式
     * @param v 是否启用非阻塞模式
     * @return 是否设置成功
     *
     * 设置后 read() 在无数据可用时立即返回 0 而非阻塞，
     * write() 在缓冲区满时立即返回 0 而非阻塞。
     */
    bool set_nonblocking(bool v) noexcept;

    /**
     * @brief 查询管道是否处于非阻塞模式
     * @return 是否非阻塞
     */
    NEFORCE_NODISCARD bool is_nonblocking() const noexcept { return nonblocking_; }

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
    NEFORCE_NODISCARD bool is_valid() const noexcept;

    /**
     * @brief 获取读端句柄
     */
    NEFORCE_NODISCARD native_handle_type native_read_handle() const noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
        return read_handle_;
#else
        return fds_[0];
#endif
    }

    /**
     * @brief 获取写端句柄
     */
    NEFORCE_NODISCARD native_handle_type native_write_handle() const noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
        return write_handle_;
#else
        return fds_[1];
#endif
    }

    /**
     * @brief 分离读端句柄（调用者负责关闭）
     */
    NEFORCE_NODISCARD native_handle_type detach_read_handle() noexcept;

    /**
     * @brief 分离写端句柄（调用者负责关闭）
     */
    NEFORCE_NODISCARD native_handle_type detach_write_handle() noexcept;
};


/**
 * @class named_pipe
 * @brief 命名管道类
 *
 * 提供跨平台的命名管道支持，可用于任意进程间通信。
 * 使用服务端/客户端模式：一端创建并监听，另一端连接。
 */
class NEFORCE_API named_pipe {
public:
    using native_handle_type = _NEFORCE native_handle_type;

private:
#ifdef NEFORCE_PLATFORM_WINDOWS
    native_handle_type pipe_handle_;
    bool is_server_{false};
#else
    native_handle_type fd_{-1};
    string fifo_path_;
#endif
    string name_;
    bool nonblocking_{false};

public:
    /**
     * @brief 默认构造函数
     */
    named_pipe();

    /**
     * @brief 析构函数
     */
    ~named_pipe();

    named_pipe(const named_pipe&) = delete;
    named_pipe& operator=(const named_pipe&) = delete;

    /**
     * @brief 移动构造函数
     */
    named_pipe(named_pipe&& other) noexcept;

    /**
     * @brief 移动赋值运算符
     */
    named_pipe& operator=(named_pipe&& other) noexcept;

    /**
     * @brief 创建命名管道服务端
     * @param name 管道名称（不含平台前缀）
     * @param nonblocking 是否非阻塞模式
     * @return 是否创建成功
     * @throws pipe_exception 创建失败时抛出
     */
    bool create(const string& name, bool nonblocking = false);

    /**
     * @brief 连接到已存在的命名管道（客户端）
     * @param name 管道名称
     * @param timeout_ms 连接超时毫秒（仅 Windows），-1 表示无限等待
     * @return 是否连接成功
     * @throws pipe_exception 连接失败时抛出
     */
    bool connect(const string& name, int timeout_ms = -1);

    /**
     * @brief 等待客户端连接（服务端）
     * @return 是否成功接受连接
     * @throws pipe_exception 等待失败时抛出
     */
    bool wait_for_client();

    /**
     * @brief 从管道读取数据
     * @param buffer 缓冲区
     * @param size 缓冲区大小
     * @return 实际读取的字节数，-1表示错误
     */
    int read(void* buffer, size_t size) noexcept;

    /**
     * @brief 向管道写入数据
     * @param data 要写入的数据
     * @param size 数据大小
     * @return 实际写入的字节数，-1表示错误
     */
    int write(const void* data, size_t size) noexcept;

    /**
     * @brief 关闭管道
     */
    void close() noexcept;

    /**
     * @brief 检查管道是否有效
     * @return 是否已打开
     */
    NEFORCE_NODISCARD bool is_valid() const noexcept;

    /**
     * @brief 设置非阻塞模式
     * @param v 是否非阻塞
     * @return 是否设置成功
     */
    bool set_nonblocking(bool v) noexcept;

    /**
     * @brief 查询是否非阻塞
     * @return 是否非阻塞
     */
    NEFORCE_NODISCARD bool is_nonblocking() const noexcept { return nonblocking_; }

    /**
     * @brief 获取管道名称
     * @return 管道名称
     */
    NEFORCE_NODISCARD const string& name() const noexcept { return name_; }

    /**
     * @brief 获取原生句柄
     * @return 原生句柄
     */
    NEFORCE_NODISCARD native_handle_type native_handle() const noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
        return pipe_handle_;
#else
        return fd_;
#endif
    }

    /**
     * @brief 删除命名管道（仅 Linux FIFO）
     * @param path FIFO 文件路径
     * @return 是否删除成功
     */
    static bool remove(const string& path);
};

/** @} */ // Pipe

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_SYSTEM_PIPE_HPP__
