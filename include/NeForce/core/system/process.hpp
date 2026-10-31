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
#include "NeForce/core/container/map.hpp"
#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/file/path.hpp"
#include "NeForce/core/string/string_view.hpp"
#include "NeForce/core/system/pipe.hpp"
#include "NeForce/core/utility/optional.hpp"
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
NEFORCE_ERROR_BUILD_SYSTEM_CLASS(process_exception, "Process Operation Failed.")

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
    using native_id_type = _NEFORCE native_id_type;

    /**
     * @enum std_stream
     * @brief 标准流标识
     */
    enum class std_stream {
        in,  ///< 标准输入
        out, ///< 标准输出
        err  ///< 标准错误
    };

    /**
     * @struct memory_info
     * @brief 进程内存信息
     *
     * 包含进程的工作集和页面文件使用情况。
     */
    struct memory_info {
        size_t virtual_size{0};          /**< 虚拟内存大小（字节） */
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
        uint64_t wall_time_ms{0};   /**< 自进程启动以来的墙上时间（毫秒） */
    };

    /**
     * @enum state
     * @brief 进程状态枚举
     */
    enum class state {
        running,   /**< 进程正在运行 */
        suspended, /**< 进程已被挂起 */
        stopped,   /**< 进程被调试器跟踪而暂停（Linux） */
        exited,    /**< 进程已退出 */
        unknown    /**< 进程状态未知 */
    };

    /**
     * @enum permission
     * @brief 进程访问权限位
     *
     * 各平台语义不完全等价：
     * - Windows：
     *   - read/write 对应进程虚拟内存读写
     *   - query_info 对应信息查询
     *   - terminate 对应终止权限
     *   - execute 无对应项而被忽略
     * - Linux：
     *   - read/write 表示能否访问目标进程内存
     *   - query_info 表示能否读取 /proc 信息
     *   - terminate 表示能否发送信号
     *   - execute 恒为 false
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
     * @enum signal
     * @brief 跨平台进程信号
     */
    enum class signal : int32_t {
        terminate, ///< 请求终止
        kill,      ///< 强制终止
        interrupt, ///< 中断（LINUX）
        quit,      ///< 退出（LINUX）
        hangup,    ///< 挂断（LINUX）
        stop,      ///< 暂停（LINUX）
        resume,    ///< 继续（LINUX）
        user1,     ///< 用户信号 1（LINUX）
        user2      ///< 用户信号 2（LINUX）
    };

    /**
     * @enum priority
     * @brief 进程调度优先级
     */
    enum class priority : int32_t {
        idle,         ///< 最低优先级
        below_normal, ///< 低于普通优先级
        normal,       ///< 普通优先级
        normal_above, ///< 高于普通优先级
        high,         ///< 高优先级
        realtime      ///< 实时优先级，可能影响系统响应
    };

    /**
     * @struct io_info
     * @brief 进程 I/O 统计
     */
    struct io_info {
        uint64_t read_bytes{0};       ///< 累计读取字节
        uint64_t write_bytes{0};      ///< 累计写入字节
        uint64_t read_operations{0};  ///< 读操作次数
        uint64_t write_operations{0}; ///< 写操作次数
        uint64_t other_bytes{0};      ///< 其它 I/O 字节（Windows）
    };

    /**
     * @struct thread_info
     * @brief 线程信息
     */
    struct thread_info {
        uint32_t tid{0};              ///< 线程 ID
        string name;                  ///< 线程名，无法获取时为空
        state status{state::unknown}; ///< 线程状态
        uint64_t user_time_ms{0};     ///< 用户态 CPU 时间（毫秒）
        uint64_t system_time_ms{0};   ///< 内核态 CPU 时间（毫秒）
        uint64_t start_time_ms{0};    ///< 启动时刻（Unix 纪元毫秒）
    };

    /**
     * @struct shell_result
     * @brief shell 命令执行结果
     */
    struct shell_result {
        int exit_code{-1}; /**< 命令退出码 */
        string output;     /**< 标准输出 */
        string error;      /**< 错误输出 */
    };

    /**
     * @struct snapshot_entry
     * @brief 进程快照条目
     */
    struct snapshot_entry {
        uint32_t pid{0};              ///< 进程 ID
        uint32_t parent_pid{0};       ///< 父进程 ID
        string name;                  ///< 进程名
        string command;               ///< 命令行
        state status{state::unknown}; ///< 运行状态
        char state_code{0};           ///< 原始状态字符（Linux）
        uint32_t uid{0};              ///< 属主用户 ID（Linux）
        string user;                  ///< 属主用户名，无法解析时为空
        uint32_t threads{0};          ///< 线程数
        int32_t nice{0};              ///< nice 值（Linux）
        bool kernel_thread{false};    ///< 是否为内核线程（Linux）
        uint64_t rss_bytes{0};        ///< 常驻内存（字节）
        uint64_t virtual_bytes{0};    ///< 虚拟内存（字节，Linux）
        uint64_t user_time_ms{0};     ///< 用户态 CPU 时间（毫秒）
        uint64_t system_time_ms{0};   ///< 内核态 CPU 时间（毫秒）
        uint64_t start_time_ms{0};    ///< 启动时刻（Unix 纪元毫秒）
        io_info io;                   ///< 进程 I/O 统计
    };

private:
    void reader_loop();

    void close_handles() noexcept;
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
     * @return 自身引用
     */
    process& set_work_dir(const string& dir);

    /**
     * @brief 设置子进程工作目录
     * @param dir 工作目录路径
     * @return 自身引用
     */
    process& set_work_dir(const path& dir) { return set_work_dir(dir.str()); }

    /**
     * @brief 设置子进程环境变量
     * @param key 变量名
     * @param value 变量值
     * @return 自身引用
     */
    process& set_env(const string& key, const string& value);

    /**
     * @brief 捕获标准输出
     * @param v 是否捕获
     * @return 自身引用
     */
    process& set_capture_stdout(bool v = true);

    /**
     * @brief 捕获标准错误
     * @param v 是否捕获，为 true 时 stderr 写入独立缓冲区
     * @return 自身引用
     */
    process& set_capture_stderr(bool v = true);

    /**
     * @brief 预设要写入子进程 stdin 的数据
     * @param data 数据内容，在子进程启动后自动写入并关闭 stdin
     * @return 自身引用
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
     * @brief 设置外部管道作为 stdin
     * @param external_pipe 上游进程的 stdout 连接的管道
     * @return 自身引用
     * @note 该管道生命周期由调用者管理
     */
    process& set_external_stdin(pipe& external_pipe);

    /**
     * @brief 将 stdout 连接到外部管道
     * @param external_pipe 外部管道对象
     * @return 自身引用
     * @note 该管道生命周期由调用者管理
     */
    process& set_external_stdout(pipe& external_pipe);

    /**
     * @brief 将 stderr 连接到外部管道
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
    static void chain(const vector<process*>& processes);

    /**
     * @brief 启动子进程
     * @param executable 可执行文件路径
     * @param args 命令行参数列表
     * @throws process_exception 启动失败时抛出
     */
    void start(const string& executable, const vector<string>& args = {});

    /**
     * @brief 启动子进程
     * @param executable 可执行文件路径
     * @param args 命令行参数列表
     * @throws process_exception 启动失败时抛出
     */
    void start(const path& executable, const vector<string>& args = {}) { start(executable.str(), args); }

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
     * @return 退出码，-1 表示超时
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

    /** @brief 获取子进程时间信息 */
    NEFORCE_NODISCARD time_info get_time_info() const;

    /** @brief 获取子进程 I/O 统计 */
    NEFORCE_NODISCARD io_info get_io_info() const;

    /** @brief 子进程 ID */
    NEFORCE_NODISCARD native_id_type id() const noexcept { return process_id_; }

    /** @brief 子进程退出码（仅在 wait() 返回后有效） */
    NEFORCE_NODISCARD int exit_code() const noexcept { return exit_code_; }

    /** @brief 获取捕获的标准输出 */
    NEFORCE_NODISCARD const string& stdout_output() const noexcept { return stdout_buf_; }

    /** @brief 获取捕获的标准错误 */
    NEFORCE_NODISCARD const string& stderr_output() const noexcept { return stderr_buf_; }

    /**
     * @brief 向子进程的 stdin 写入数据
     * @param data 数据内容
     * @throws process_exception 子进程未运行或 stdin 未打开时抛出
     */
    void write_stdin(const string& data);

    /** @brief 关闭子进程的 stdin */
    void close_stdin();

    /**
     * @brief 执行 shell 命令并获取输出
     * @param command 命令字符串
     * @param timeout_ms 超时毫秒，-1 无限等待
     * @return 包含退出码和 stdout+stderr 的 shell_result
     * @throws process_exception 命令为空、执行失败或超时
     */
    NEFORCE_NODISCARD static shell_result execute_shell(const string& command, int timeout_ms = -1);

    /**
     * @brief 以管理员/root 权限执行 shell 命令
     * @param command 命令字符串
     * @param timeout_ms 超时毫秒，-1 无限等待
     * @param tool 提权工具选择
     * @return 包含退出码的 shell_result
     * @throws process_exception 命令为空、执行失败或超时
     * @note 提权执行不进行输出捕获
     */
    NEFORCE_NODISCARD static shell_result execute_elevated_shell(const string& command, int timeout_ms = -1,
                                                                 elevation_tool tool = elevation_tool::auto_);

    /**
     * @brief 启动子进程并分离（fire-and-forget）
     * @param executable 可执行文件路径
     * @param args 命令行参数
     * @return 子进程 ID
     * @throws process_exception 启动失败时抛出
     */
    NEFORCE_NODISCARD static native_id_type spawn(const string& executable, const vector<string>& args = {});

    /**
     * @brief 同步运行进程并返回退出码
     * @param executable 可执行文件路径
     * @param args 命令行参数
     * @param timeout_ms 超时毫秒，-1 无限等待
     * @return 退出码
     * @throws process_exception 超时或执行失败
     * @note 不捕获子进程输出，输出直接写入当前进程的 stdout/stderr
     */
    NEFORCE_NODISCARD static int run(const string& executable, const vector<string>& args = {}, int timeout_ms = -1);

    /**
     * @brief 同步运行进程并捕获 stdout
     * @param executable 可执行文件路径
     * @param args 命令行参数
     * @param timeout_ms 超时毫秒，-1 无限等待
     * @return 退出码与捕获的标准输出
     * @throws process_exception 超时或执行失败
     */
    NEFORCE_NODISCARD static shell_result run_capture(const string& executable, const vector<string>& args = {},
                                                      int timeout_ms = -1);

    /**
     * @brief 获取当前进程 ID
     */
    NEFORCE_NODISCARD static native_id_type current_id() noexcept;

    /**
     * @brief 获取当前进程名称
     * @return 进程名，失败返回空字符串
     */
    NEFORCE_NODISCARD static string current_name();

    /**
     * @brief 获取当前进程运行状态
     * @return 当前进程状态
     */
    NEFORCE_NODISCARD static state current_state();

    /**
     * @brief 获取当前进程的内存信息
     * @return 内存信息
     */
    NEFORCE_NODISCARD static memory_info current_memory_info();

    /**
     * @brief 获取当前进程的时间信息
     * @return 时间信息
     */
    NEFORCE_NODISCARD static time_info current_time_info();

    /**
     * @brief 获取当前进程的特权级别
     */
    NEFORCE_NODISCARD static privilege_level current_privilege_level() noexcept;

    /**
     * @brief 获取当前进程的命令行
     * @return 以空格分隔的参数串，失败返回空字符串
     */
    NEFORCE_NODISCARD static string current_command_line();

    /**
     * @brief 以同一命令行重新启动当前进程
     * @return 成功创建/执行子进程返回 true
     * @note 新进程与当前进程并行运行，本进程不会退出也不会等待新进程
     */
    static bool relaunch_current();

    /**
     * @brief 根据进程 ID 获取进程名称
     * @param process_id 目标进程 ID
     * @return 进程名，失败返回空字符串
     */
    NEFORCE_NODISCARD static string get_name(native_id_type process_id);

    /**
     * @brief 查询任意进程的内存信息
     * @param process_id 目标进程 ID
     * @return 内存信息，查询失败时各字段为 0
     */
    NEFORCE_NODISCARD static memory_info get_memory_info(native_id_type process_id);

    /**
     * @brief 查询任意进程的时间信息
     * @param process_id 目标进程 ID
     * @return 时间信息，查询失败时各字段为 0
     */
    NEFORCE_NODISCARD static time_info get_time_info(native_id_type process_id);

    /**
     * @brief 查询任意进程的 I/O 统计
     * @param process_id 目标进程 ID
     * @return I/O 统计，查询失败时各字段为 0
     */
    NEFORCE_NODISCARD static io_info get_io_info(native_id_type process_id);

    /**
     * @brief 查询当前进程的 I/O 统计
     * @return I/O 统计，查询失败时各字段为 0
     */
    NEFORCE_NODISCARD static io_info current_io_info();

    /**
     * @brief 查询任意进程的属主用户名
     * @param process_id 目标进程 ID
     * @return 用户名，无法解析时返回空字符串
     */
    NEFORCE_NODISCARD static string get_user(native_id_type process_id);

    /**
     * @brief 枚举指定进程的线程
     * @param process_id 目标进程 ID
     * @return 线程列表，进程不存在或无权限时返回空列表
     * @note 包含主线程（tid 等于进程 ID）
     */
    NEFORCE_NODISCARD static vector<thread_info> threads(native_id_type process_id);

    /**
     * @struct socket_owner_scan
     * @brief socket inode 到属主进程的扫描结果
     */
    struct socket_owner_scan {
        map<uint32_t, vector<native_id_type>> owners; ///< inode 到进程 ID 列表的映射
        bool complete{true};                          ///< 是否在时限内完成扫描
        uint64_t unreadable_processes{0};             ///< 无法读取 fd 目录的进程数
        bool supported{true};                         ///< 当前平台是否支持扫描
    };

    /**
     * @brief 建立 socket inode 到属主进程的映射
     * @return inode 到进程 ID 列表的映射；Windows 无 inode 概念，返回空映射
     * @note Linux 需要遍历 /proc/<pid>/fd，进程较多时开销明显
     */
    NEFORCE_NODISCARD static map<uint32_t, vector<native_id_type>> socket_owners();

    /**
     * @brief 扫描 socket inode 到属主进程的映射
     * @param timeout_ms 扫描时限（毫秒），-1 表示不限制
     * @return 扫描结果，包含完成状态与不可读进程数
     */
    NEFORCE_NODISCARD static socket_owner_scan scan_socket_owners(int timeout_ms = -1);

    /**
     * @brief 向指定进程发送信号
     * @param process_id 目标进程 ID
     * @param sig 跨平台信号
     * @return 发送成功返回 true
     * @note Linux 使用 pidfd 避免 PID 复用竞态；Windows 只映射 terminate/kill
     */
    static bool send_signal(native_id_type process_id, signal sig) noexcept;

    /**
     * @brief 向指定进程发送平台原生信号
     * @param process_id 目标进程 ID
     * @param native_signal 原生信号编号（POSIX），Windows 只接受 9 与 15
     * @return 发送成功返回 true
     */
    static bool send_signal(native_id_type process_id, int native_signal) noexcept;

    /**
     * @brief 获取信号的名称
     * @param sig 跨平台信号
     * @return 不含 SIG 前缀的名称，如 "TERM"
     */
    NEFORCE_NODISCARD static string_view signal_name(signal sig) noexcept;

    /**
     * @brief 查询进程调度优先级
     * @param process_id 目标进程 ID
     * @return 优先级；进程不存在或权限不足返回 none
     */
    NEFORCE_NODISCARD static optional<priority> get_priority(native_id_type process_id);

    /**
     * @brief 设置进程调度优先级
     * @param process_id 目标进程 ID
     * @param level 目标优先级
     * @return 设置成功返回 true
     * @note Linux 降低 nice 值需要特权；Windows 设置高优先级可能需要特权
     */
    static bool set_priority(native_id_type process_id, priority level) noexcept;

    /**
     * @brief 设置进程 nice 值
     * @param process_id 目标进程 ID
     * @param nice nice 值，范围 -20..19，超出范围会被截断
     * @return 设置成功返回 true
     * @note Linux 精确设置；Windows 按区间映射到进程优先级
     */
    static bool set_nice(native_id_type process_id, int32_t nice) noexcept;

    /**
     * @brief 查询进程 CPU 亲和性
     * @param process_id 目标进程 ID
     * @return 位掩码，bit N 表示第 N 个逻辑处理器；失败返回 none
     * @note 仅覆盖前 64 个逻辑处理器
     */
    NEFORCE_NODISCARD static optional<uint64_t> get_affinity(native_id_type process_id);

    /**
     * @brief 设置进程 CPU 亲和性
     * @param process_id 目标进程 ID
     * @param mask 位掩码，bit N 表示第 N 个逻辑处理器
     * @return 设置成功返回 true
     * @note 仅覆盖前 64 个逻辑处理器，Windows 多处理器组下行为受限
     */
    static bool set_affinity(native_id_type process_id, uint64_t mask) noexcept;

    /**
     * @brief 查询任意进程的运行状态
     * @param process_id 目标进程 ID
     * @return 进程状态，无法确定或进程不存在时返回 unknown 或 exited
     */
    NEFORCE_NODISCARD static state get_state(native_id_type process_id);

    /**
     * @brief 获取指定进程的特权级别
     * @param process_id 目标进程 ID
     * @return 特权级别，无法查询时返回 unknown
     */
    NEFORCE_NODISCARD static privilege_level get_privilege_level(native_id_type process_id);

    /**
     * @brief 获取指定进程的命令行
     * @param process_id 目标进程 ID
     * @return 以空格分隔的参数串，失败或权限不足返回空字符串
     */
    NEFORCE_NODISCARD static string get_command_line(native_id_type process_id);

    /**
     * @brief 获取指定进程的可执行文件完整路径
     * @param process_id 目标进程 ID
     * @return 可执行文件路径，失败或权限不足返回空字符串
     */
    NEFORCE_NODISCARD static string get_executable_path(native_id_type process_id);

    /**
     * @brief 检查对指定进程的访问权限
     * @param process_id 目标进程 ID
     * @param perm 要检查的权限，可为多个权限位的组合
     * @return 是否拥有全部请求的权限
     * @note 平台语义差异见 permission；perm 为 0 时等价于检查进程是否存在
     */
    NEFORCE_NODISCARD static bool check_permission(native_id_type process_id, permission perm);

    /**
     * @brief 在 PATH 环境变量中搜索可执行文件
     * @param executable 可执行文件名
     * @return 可执行文件完整路径，未找到返回空字符串
     */
    NEFORCE_NODISCARD static string search_path(const string& executable);

    /**
     * @brief 把可执行文件与参数拼成 Windows 命令行字符串
     * @param executable 可执行文件路径
     * @param args 参数列表
     * @return 命令行字符串，含空格或 shell 特殊字符的参数会被双引号包裹
     */
    NEFORCE_NODISCARD static string build_command_line(const string& executable, const vector<string>& args);

    /**
     * @brief 枚举全系统进程快照
     * @return 快照条目列表，按 /proc 或系统快照的枚举顺序排列
     */
    NEFORCE_NODISCARD static vector<snapshot_entry> snapshot();
};

/** @} */ // Process

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_SYSTEM_PROCESS_HPP__
