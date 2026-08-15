#ifndef NEFORCE_CORE_SYSTEM_PROCESS_HPP__
#define NEFORCE_CORE_SYSTEM_PROCESS_HPP__

/**
 * @file process.hpp
 * @brief 进程管理工具
 *
 * 此文件提供了进程创建、管理和监控功能。
 */

#include "NeForce/core/async/atomic.hpp"
#include "NeForce/core/async/thread.hpp"
#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/system/pipe.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Exceptions 异常类集
 * @brief 异常类集
 * @{
 */

/**
 * @struct process_exception
 * @brief 进程操作异常
 */
struct process_exception final : system_exception {
    explicit process_exception(const char* info = "Process Operation Failed.",
                               const error_code code = last_error()) noexcept :
    system_exception(info, code) {}

    explicit process_exception(const exception& e) :
    system_exception(e) {}

    ~process_exception() override = default;

    NEFORCE_NODISCARD const char* type() const noexcept override { return "process_exception"; }
};

/** @} */ // Exceptions

/**
 * @defgroup Process 进程
 * @brief 进程管理工具
 * @{
 */

/**
 * @class process
 * @brief 进程管理类
 *
 * 支持 stdout/stderr 分离捕获、stdin 写入、工作目录和环境变量设置，
 * 外部管道连接、文件重定向和进程链。
 *
 * @code
 * process p;
 * p.set_capture_stdout(true)
 *  .set_capture_stderr(true)
 *  .set_work_dir("/tmp");
 * p.start("/bin/ls", {"-la"});
 * int rc = p.wait(5000);
 * string out = p.stdout_output();
 * @endcode
 */
class NEFORCE_API process {
public:
    /**
     * @brief 进程ID类型
     */
    using native_id_type = native_size_type;

    /**
     * @enum std_stream
     * @brief 标准流标识
     */
    enum class std_stream {
        stdin,  ///< 标准输入
        stdout, ///< 标准输出
        stderr  ///< 标准错误
    };

    /**
     * @struct memory_info
     * @brief 进程内存信息
     *
     * 包含进程的工作集和页面文件使用情况。
     */
    struct memory_info {
        size_t working_set_size{0};      /**< 当前工作集大小（字节） */
        size_t peak_working_set_size{0}; /**< 峰值工作集大小（字节） */
        size_t pagefile_usage{0};        /**< 当前页面文件使用量（字节） */
        size_t peak_pagefile_usage{0};   /**< 峰值页面文件使用量（字节） */
    };

    /**
     * @struct time_info
     * @brief 进程时间信息
     *
     * 包含进程的用户态、内核态和墙上时间。
     */
    struct time_info {
        uint64_t user_time_ms{0};   /**< 用户态 CPU 时间（毫秒） */
        uint64_t kernel_time_ms{0}; /**< 内核态 CPU 时间（毫秒） */
        uint64_t wall_time_ms{0};   /**< 墙上经过时间（毫秒） */
    };

    /**
     * @enum state
     * @brief 进程状态枚举
     */
    enum class state {
        running,   /**< 进程正在运行 */
        suspended, /**< 进程已被挂起 */
        stopped,   /**< 进程已停止 */
        exited,    /**< 进程已退出 */
        unknown    /**< 进程状态未知 */
    };

    /**
     * @enum permission
     * @brief 进程权限枚举
     */
    enum class permission {
        read = 0x01,       /**< 读取权限 */
        write = 0x02,      /**< 写入权限 */
        execute = 0x04,    /**< 执行权限 */
        terminate = 0x08,  /**< 终止权限 */
        query_info = 0x10, /**< 查询信息权限 */
        all = 0xFF         /**< 所有权限 */
    };

    /**
     * @enum privilege_level
     * @brief 进程特权级别
     */
    enum class privilege_level {
        privileged,     /**< 具有管理员/root 权限 */
        not_privileged, /**< 普通权限 */
        unknown         /**< 无法确定权限级别 */
    };

    /**
     * @brief 提权工具选择
     */
    enum class elevation_tool {
        auto_, /**< Windows 忽略此选项；Linux 自动尝试 pkexec，回退至 sudo */
        sudo,  /**< Linux 强制使用 sudo 提权 */
        pkexec /**< Linux 强制使用 pkexec 提权 */
    };

    /**
     * @struct shell_result
     * @brief shell 命令执行结果
     */
    struct shell_result {
        int exit_code; /**< 命令退出码 */
        string output; /**< 合并的标准输出和标准错误 */
    };

private:
    void reader_loop();

    void close_handles() noexcept;

    /**
     * @brief 分离进程句柄，使子进程生命周期不再受 process 对象控制
     */
    void detach_handles() noexcept;

    native_id_type process_id_{0}; /**< 子进程 ID */
    int exit_code_{-1};            /**< 子进程退出码，-1 表示尚未退出 */
    bool started_{false};          /**< 子进程是否已启动 */
    bool finished_{false};         /**< 子进程是否已完成 */

    string work_dir_;                       /**< 子进程工作目录 */
    vector<pair<string, string>> env_vars_; /**< 子进程环境变量列表 */
    bool capture_stdout_{false};            /**< 是否捕获标准输出 */
    bool capture_stderr_{false};            /**< 是否捕获标准错误 */
    string stdin_data_;                     /**< 预设的 stdin 输入数据 */

    string stdout_file_; /**< stdout 重定向文件路径 */
    string stderr_file_; /**< stderr 重定向文件路径 */

    pipe* external_stdin_pipe_{nullptr};  /**< 外部提供的 stdin 管道（不拥有所有权） */
    pipe* external_stdout_pipe_{nullptr}; /**< 外部提供的 stdout 管道（不拥有所有权） */
    pipe* external_stderr_pipe_{nullptr}; /**< 外部提供的 stderr 管道（不拥有所有权） */

    bool detached_{false}; /**< 是否已与子进程分离（分离后析构不终止子进程） */

    pipe stdout_pipe_; /**< stdout 管道 */
    pipe stderr_pipe_; /**< stderr 管道 */
    pipe stdin_pipe_;  /**< stdin 管道 */

    string stdout_buf_; /**< stdout 数据缓冲区 */
    string stderr_buf_; /**< stderr 数据缓冲区 */

    thread reader_thread_;               /**< 异步读取线程 */
    atomic<bool> reader_running_{false}; /**< 读取线程运行标志 */

#ifdef NEFORCE_PLATFORM_WINDOWS
    void* process_handle_{nullptr}; /**< Windows 进程句柄 */
    void* thread_handle_{nullptr};  /**< Windows 主线程句柄 */
#endif

public:
    /**
     * @brief 默认构造
     *
     * 不启动任何进程
     */
    process() = default;

    /**
     * @brief 析构
     *
     * 自动终止未退出的子进程并清理资源
     */
    ~process();

    process(const process&) = delete;
    process& operator=(const process&) = delete;

    /**
     * @brief 移动构造
     */
    process(process&& other) noexcept;

    /**
     * @brief 移动赋值
     */
    process& operator=(process&& other) noexcept;

    /**
     * @brief 设置子进程工作目录
     * @param dir 工作目录路径
     */
    process& set_work_dir(const string& dir);

    /**
     * @brief 设置子进程环境变量
     * @param key 变量名
     * @param value 变量值
     */
    process& set_env(const string& key, const string& value);

    /**
     * @brief 捕获标准输出
     * @param v 是否捕获
     */
    process& set_capture_stdout(bool v = true);

    /**
     * @brief 捕获标准错误
     * @param v 是否捕获，为 true 时 stderr 写入独立缓冲区
     */
    process& set_capture_stderr(bool v = true);

    /**
     * @brief 预设要写入子进程 stdin 的数据
     * @param data 数据内容，在子进程启动后自动写入并关闭 stdin
     */
    process& set_stdin_data(const string& data);

    /**
     * @brief 将 stdout 重定向到文件
     * @param file_path 文件路径
     * @return 自身引用
     */
    process& set_stdout_file(const string& file_path);

    /**
     * @brief 将 stderr 重定向到文件
     * @param file_path 文件路径
     * @return 自身引用
     */
    process& set_stderr_file(const string& file_path);

    /**
     * @brief 设置外部管道作为 stdin（用于进程链）
     * @param external_pipe 上游进程的 stdout 连接的管道
     * @return 自身引用
     * @note 该管道生命周期由调用者管理，process 不拥有所有权
     */
    process& set_external_stdin(pipe& external_pipe);

    /**
     * @brief 将 stdout 连接到外部管道（供下游进程读取）
     * @param external_pipe 外部管道对象
     * @return 自身引用
     * @note 该管道生命周期由调用者管理
     */
    process& set_external_stdout(pipe& external_pipe);

    /**
     * @brief 将 stderr 连接到外部管道（供下游进程读取）
     * @param external_pipe 外部管道对象
     * @return 自身引用
     * @note 该管道生命周期由调用者管理
     */
    process& set_external_stderr(pipe& external_pipe);

    /**
     * @brief 管道连接两个进程（A 的 stdout -> B 的 stdin）
     * @param first 上游进程（已配置但未启动）
     * @param second 下游进程（已配置但未启动）
     *
     * 内部创建管道并将 first 的 stdout 连接到 second 的 stdin。
     * 管道生命周期由下游进程管理。
     */
    static void chain(process& first, process& second);

    /**
     * @brief 管道连接多个进程
     * @param processes 要连接的进程列表（已配置但未启动）
     *
     * 将进程列表中的进程按顺序用管道串联：
     * processes[0] stdout -> processes[1] stdin,
     * processes[1] stdout -> processes[2] stdin, ...
     */
    static void chain(vector<process*>& processes);

    /**
     * @brief 启动子进程
     * @param executable 可执行文件路径
     * @param args 命令行参数列表
     * @throws process_exception 启动失败时抛出
     */
    void start(const string& executable, const vector<string>& args = {});

    /**
     * @brief 以管理员/root 权限启动子进程
     * @param executable 可执行文件路径
     * @param args 命令行参数列表
     * @param tool 提权工具
     * @throws process_exception 启动失败或已设置了管道捕获时抛出
     * @note 不捕获输出
     */
    void start_elevated(const string& executable, const vector<string>& args = {},
                        elevation_tool tool = elevation_tool::auto_);

    /**
     * @brief 等待子进程退出
     * @param timeout_ms 超时毫秒数，-1 表示无限等待
     * @return 退出码，-1 超时
     * @throws process_exception 等待失败时抛出
     */
    int wait(int timeout_ms = -1);

    /** @brief 终止子进程（SIGTERM → 短暂等待 → SIGKILL） */
    void terminate();

    /** @brief 挂起子进程 */
    void suspend();

    /** @brief 恢复子进程 */
    void resume();

    /** @brief 显式关闭进程句柄并清理资源 */
    void close() noexcept;

    /** @brief 子进程是否正在运行 */
    NEFORCE_NODISCARD bool is_running() const;

    /** @brief 获取子进程状态 */
    NEFORCE_NODISCARD state get_state() const;

    /** @brief 获取子进程内存信息 */
    NEFORCE_NODISCARD memory_info get_memory_info() const;

    /** @brief 子进程 ID */
    NEFORCE_NODISCARD native_id_type id() const noexcept { return process_id_; }

    /** @brief 子进程退出码（仅在 wait() 返回后有效） */
    NEFORCE_NODISCARD int exit_code() const noexcept { return exit_code_; }

    /** @brief 获取捕获的标准输出 */
    NEFORCE_NODISCARD const string& stdout_output() const noexcept { return stdout_buf_; }

    /** @brief 获取捕获的标准错误 */
    NEFORCE_NODISCARD const string& stderr_output() const noexcept { return stderr_buf_; }

    /** @brief 向子进程的 stdin 写入数据 */
    void write_stdin(const string& data);

    /** @brief 关闭子进程的 stdin */
    void close_stdin();

    /**
     * @brief 执行 shell 命令并获取输出
     * @param command 命令字符串
     * @param timeout_ms 超时毫秒，-1 无限等待
     * @return 包含退出码和合并的 stdout+stderr 的 shell_result
     * @throws process_exception 执行失败或超时
     */
    NEFORCE_NODISCARD static shell_result execute_shell(const string& command, int timeout_ms = -1);

    /**
     * @brief 以管理员/root 权限执行 shell 命令
     * @param command 命令字符串
     * @param timeout_ms 超时毫秒，-1 无限等待
     * @param tool 提权工具选择
     * @return 包含退出码和空输出的 shell_result
     * @throws process_exception 执行失败或超时
     * @note 不捕获输出
     */
    static shell_result execute_elevated_shell(const string& command, int timeout_ms = -1,
                                               elevation_tool tool = elevation_tool::auto_);

    /**
     * @brief 启动子进程并分离（fire-and-forget）
     * @param executable 可执行文件路径
     * @param args 命令行参数
     * @return 子进程 ID
     *
     * 启动后立即分离，不等待。进程在后台运行。
     */
    static native_id_type spawn(const string& executable, const vector<string>& args = {});

    /**
     * @brief 同步运行进程并返回退出码
     * @param executable 可执行文件路径
     * @param args 命令行参数
     * @param timeout_ms 超时毫秒，-1 无限等待
     * @return 退出码
     * @throws process_exception 超时或执行失败
     */
    static int system(const string& executable, const vector<string>& args = {}, int timeout_ms = -1);

    /**
     * @brief 执行进程并捕获 stdout 输出
     * @param executable 可执行文件路径
     * @param args 命令行参数
     * @param timeout_ms 超时毫秒，-1 无限等待
     * @return 标准输出字符串
     * @throws process_exception 超时或执行失败
     */
    static string capture(const string& executable, const vector<string>& args = {}, int timeout_ms = -1);

    /** @brief 获取当前进程 ID */
    NEFORCE_NODISCARD static native_id_type current_id() noexcept;

    /** @brief 获取当前进程的特权级别 */
    NEFORCE_NODISCARD static privilege_level current_privilege_level() noexcept;

    /**
     * @brief 根据进程 ID 获取进程名称
     * @param process_id 目标进程 ID
     * @return 进程名称，失败返回空字符串
     */
    NEFORCE_NODISCARD static string name(native_id_type process_id);

    /**
     * @brief 查询任意进程的内存信息
     * @param process_id 目标进程 ID
     */
    NEFORCE_NODISCARD static memory_info get_memory_info(native_id_type process_id);

    /**
     * @brief 查询任意进程的运行状态
     * @param process_id 目标进程 ID
     */
    NEFORCE_NODISCARD static state get_state(native_id_type process_id);

    /**
     * @brief 获取指定进程的特权级别
     * @param process_id 目标进程 ID
     */
    NEFORCE_NODISCARD static privilege_level get_privilege_level(native_id_type process_id);

    /**
     * @brief 检查对指定进程的访问权限
     * @param process_id 目标进程 ID
     * @param permission 要检查的权限
     * @return 是否拥有该权限
     */
    NEFORCE_NODISCARD static bool check_permission(native_id_type process_id, permission permission);

    /**
     * @brief 在 PATH 环境变量中搜索可执行文件
     * @param executable 可执行文件名
     * @return 可执行文件完整路径，未找到返回空字符串
     */
    NEFORCE_NODISCARD static string search_path(const string& executable);
};

/** @} */ // Process

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_SYSTEM_PROCESS_HPP__
