#ifndef NEFORCE_CORE_SYSTEM_PTY_HPP__
#define NEFORCE_CORE_SYSTEM_PTY_HPP__

/**
 * @file pty.hpp
 * @brief 伪终端
 *
 * 在 POSIX 系统上创建伪终端并在其上启动子进程，使子进程获得真正的终端语义。
 */

#include "console.hpp"
#include "NeForce/core/system/signal.hpp"
#include "NeForce/core/exception/system_exception.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Exceptions 异常类集
 * @brief 异常类集
 * @{
 */

/**
 * @struct pty_exception
 * @brief 伪终端操作失败
 */
NEFORCE_ERROR_BUILD_SYSTEM_CLASS(pty_exception, "Pseudo Terminal Operation Failed.")

/** @} */ // Exceptions

/**
 * @addtogroup System 系统平台
 * @{
 */

/**
 * @class pty
 * @brief 伪终端会话
 *
 * @code
 * pty session;
 * session.open(pty::console_size{120, 40});  // 终端尺寸
 * session.start("/bin/bash", {"-lc", "ls --color=auto"});
 * const string output = session.read_available();  // 或接入 io_context
 * const int code = session.wait(5000);
 * @endcode
 */
class NEFORCE_API pty {
public:
    using console_size = sys_console::console_size;

    /**
     * @brief 创建未初始化的伪终端
     */
    pty() noexcept;

    /**
     * @brief 析构
     *
     * 关闭主端并回收子进程。
     */
    ~pty();

    pty(const pty&) = delete;
    pty& operator=(const pty&) = delete;
    pty(pty&& other) noexcept;
    pty& operator=(pty&& other) noexcept;

    /**
     * @brief 当前平台是否支持伪终端
     * @return 是否支持
     */
    NEFORCE_NODISCARD static bool is_supported() noexcept;

    /**
     * @brief 当前环境是否真的能打开伪终端
     * @return 是否可用
     */
    NEFORCE_NODISCARD static bool is_available() noexcept;

    /**
     * @brief 打开伪终端
     * @param size 终端大小
     * @throws pty_exception 平台不支持或打开失败
     */
    void open(console_size size = console_size{80, 24});

    /**
     * @brief 在伪终端上启动子进程
     * @param executable 可执行文件路径
     * @param args 参数列表，`argv[0]` 为可执行文件名本身
     * @param work_dir 工作目录，空表示继承
     * @param env `键=值` 形式的环境变量
     * @param merged_stderr 保留参数以与管道模式对齐
     * @throws pty_exception 未打开、可执行文件不存在、`fork`/`chdir`/`exec` 失败时抛出
     *
     * 子进程成为新会话的首进程并把主端从端设为控制终端。父子之间有一根 close-on-exec 的通报管道，
     * `exec` 之前的任何失败都会带回父进程并由本函数抛出，而不是变成子进程的 127 退出码。
     */
    void start(const string& executable, const vector<string>& args = {}, const string& work_dir = {},
               const vector<string>& env = {}, bool merged_stderr = true);

    /**
     * @brief 是否已启动
     * @return 是否已启动
     */
    NEFORCE_NODISCARD bool is_running() const noexcept;

    /**
     * @brief 主端文件描述符
     * @return 可读写的描述符，未打开时为 -1
     */
    NEFORCE_NODISCARD native_handle_type master_handle() const noexcept;

    /**
     * @brief 子进程 id
     * @return 进程 id，未启动时为 0
     */
    NEFORCE_NODISCARD native_id_type process_id() const noexcept;

    /**
     * @brief 读取全部可用输出
     * @return 数据，无数据时为空串
     */
    NEFORCE_NODISCARD string read_available();

    /**
     * @brief 读取至多指定字节的输出
     * @param buffer 缓冲区
     * @param size 缓冲区大小
     * @return 实际读取的字节数，-1 表示错误或已关闭
     */
    int read(void* buffer, size_t size) noexcept;

    /**
     * @brief 写入数据
     * @param data 数据
     * @return 实际写入字节数，-1 表示失败
     */
    int write(string_view data) noexcept;

    /**
     * @brief 调整窗口尺寸
     * @param columns 列数
     * @param rows 行数
     */
    void resize(int columns, int rows);

    /**
     * @brief 向子进程终端发送控制字符
     * @param byte 控制字节
     */
    void send_control(char byte);

    /**
     * @brief 终止子进程
     * @param signal 发送的信号
     *
     * 信号发送给以子进程为组长的整个进程组，避免后代进程逃逸。
     */
    void terminate(system_signal::event signal = system_signal::event::TERMINATE);

    /**
     * @brief 强制终止子进程
     */
    void kill();

    /**
     * @brief 等待子进程退出
     * @param timeout_ms 超时毫秒，负数表示一直等待
     * @return 退出码；超时返回 -1
     *
     * 退出码与 `process::wait()` 一致：正常退出为退出状态，被信号终止为负的信号值。
     */
    int wait(int timeout_ms = -1);

    /**
     * @brief 退出码
     * @return 退出码，尚未退出时为 -1
     */
    NEFORCE_NODISCARD int exit_code() const noexcept;

    /**
     * @brief 非阻塞检查子进程是否已退出
     * @return 是否已退出
     */
    NEFORCE_NODISCARD bool poll_exit() noexcept;

    /**
     * @brief 关闭主端并回收子进程
     */
    void close() noexcept;

private:
    int master_{-1};
    int child_id_{0};
    int exit_code_{-1};
    bool finished_{false};
    int columns_{80};
    int rows_{24};

    void reap(bool blocking, int timeout_ms);
};

/** @} */ // System

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_SYSTEM_PTY_HPP__
