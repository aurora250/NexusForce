#ifndef NEFORCE_CORE_SYSTEM_DAEMON_HPP__
#define NEFORCE_CORE_SYSTEM_DAEMON_HPP__

/**
 * @file daemon.hpp
 * @brief 守护进程框架
 */

#include "NeForce/core/async/atomic.hpp"
#include "NeForce/core/async/mutex.hpp"
#include "NeForce/core/async/thread.hpp"
#include "NeForce/core/container/unordered_map.hpp"
#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/functional/function.hpp"
#include "NeForce/core/memory/unique_ptr.hpp"
#include "NeForce/core/system/pipe.hpp"
#include "NeForce/core/system/process.hpp"
#include "NeForce/core/system/signal.hpp"
#include "NeForce/core/system/system_event.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Daemon 守护进程
 * @brief 守护进程框架
 * @{
 */

/**
 * @struct daemon_exception
 * @brief 守护进程操作异常
 */
struct daemon_exception final : system_exception {
    explicit daemon_exception(const char* info = "Daemon Operation Failed.", const char* type = static_type,
                              const int code = 0) noexcept :
    system_exception(info, type, code) {}

    explicit daemon_exception(const exception& e) :
    system_exception(e) {}

    ~daemon_exception() override = default;
    static constexpr auto static_type = "daemon_exception";
};

/**
 * @class daemon
 * @brief 守护进程
 *
 * 封装了守护进程的完整生命周期：后台化 → PID 文件锁 → 子进程管理 → 信号驱动主循环 → 优雅关闭。
 *
 * @code
 * daemon d;
 * d.on_start([] { logger::info("starting"); });
 * d.on_stop([] { logger::info("stopping"); });
 * d.daemonize("/");
 * d.write_pid_file("/var/run/myapp.pid");
 *
 * daemon::child_config cfg;
 * cfg.name = "worker";
 * cfg.executable = "/usr/bin/myworker";
 * cfg.auto_restart = true;
 * d.add_child(cfg);
 * return d.run();
 * @endcode
 */
class NEFORCE_API daemon {
public:
    /** @brief 守护进程运行状态 */
    enum class daemon_state {
        stopped,   ///< 未启动
        starting,  ///< 正在启动
        running,   ///< 正常运行
        reloading, ///< 正在重载配置
        stopping   ///< 正在停止
    };

    /** @brief 子进程配置 */
    struct child_config {
        string name;                       ///< 唯一名称
        string executable;                 ///< 可执行文件路径
        vector<string> args;               ///< 命令行参数
        string work_dir;                   ///< 工作目录（空=继承）
        int max_restarts{5};               ///< 最大自动重启次数
        int restart_delay_ms{1000};        ///< 重启前等待时间
        int health_check_interval_ms{0};   ///< 健康检查间隔
        int graceful_timeout_ms{5000};     ///< 停止时的优雅超时
        bool capture_output{false};        ///< 是否捕获子进程输出
        vector<pair<string, string>> envs; ///< 额外环境变量
    };

    /** @brief 子进程运行时状态 */
    struct child_status {
        string name;                    ///< 名称
        process::native_id_type pid{0}; ///< 进程 ID
        int exit_code{0};               ///< 最后退出码
        int restart_count{0};           ///< 已重启次数
        bool running{false};            ///< 是否正在运行
    };

    /** @brief 启动回调，run() 进入主循环前调用 */
    using start_callback = function<void()>;

    /** @brief 停止回调，收到终止信号后调用 */
    using stop_callback = function<void()>;

    /** @brief 重载回调，收到 SIGHUP 后调用。返回 true 继续运行，false 请求终止 */
    using reload_callback = function<bool()>;

    /** @brief 子进程退出回调 */
    using child_exit_callback = function<void(const string& name, int exit_code)>;

private:
    atomic<daemon_state> state_{daemon_state::stopped};

#ifdef NEFORCE_PLATFORM_WINDOWS
    void* pid_handle_{nullptr};
#else
    int pid_fd_{-1};
#endif
    string pid_path_;
    string work_dir_{"/"};

    unique_ptr<signal_guard> signal_guard_;

    system_event shutdown_event_{false, system_event::type::manual_reset};
    system_event reload_event_{false, system_event::type::auto_reset};

    struct child_entry {
        child_config config;
        unique_ptr<process> proc;
        int restart_count{0};
        int exit_code{0};
        bool running{false};
    };
    mutable mutex children_mutex_;
    unordered_map<string, child_entry> children_;

    start_callback on_start_;
    stop_callback on_stop_;
    reload_callback on_reload_;
    child_exit_callback on_child_exit_;

    atomic<bool> watchdog_enabled_{false};
    atomic<int> watchdog_timeout_ms_{0};
    atomic<int64_t> last_ping_{0};
    thread watchdog_thread_;

    bool daemonize_impl() noexcept;
    void setup_signal_handlers();
    void start_all_children();
    void stop_all_children();
    void check_and_restart_children();
    static void start_child(child_entry& entry);
    void watchdog_loop();

public:
    daemon() = default;
    ~daemon();

    daemon(const daemon&) = delete;
    daemon& operator=(const daemon&) = delete;
    daemon(daemon&&) = delete;
    daemon& operator=(daemon&&) = delete;

    /** @brief 注册启动回调 */
    void on_start(start_callback cb) { on_start_ = move(cb); }

    /** @brief 注册停止回调 */
    void on_stop(stop_callback cb) { on_stop_ = move(cb); }

    /** @brief 注册重载回调 */
    void on_reload(reload_callback cb) { on_reload_ = move(cb); }

    /** @brief 注册子进程退出回调 */
    void on_child_exit(child_exit_callback cb) { on_child_exit_ = move(cb); }

    /**
     * @brief 将当前进程转为守护进程（Linux 双 fork）
     * @param work_dir 守护进程工作目录
     * @return 父进程返回 false，子进程返回 true
     * @note Windows 上始终返回 true
     */
    bool daemonize(const string& work_dir = "/");

    /**
     * @brief 创建并锁定 PID 文件（单实例保护）
     * @param path PID 文件路径
     * @return true 创建成功；false 已被其他实例锁定
     * @throws daemon_exception 创建失败时抛出
     */
    bool write_pid_file(const string& path);

    /** @brief 删除 PID 文件并释放锁 */
    void remove_pid_file() noexcept;

    /** @brief 检查 PID 文件是否被锁定 */
    NEFORCE_NODISCARD static bool is_pid_file_locked(const string& path) noexcept;

    /**
     * @brief 添加受管理的子进程
     * @param cfg 子进程配置
     * @return true 添加成功，false 同名已存在
     */
    bool add_child(const child_config& cfg);

    /**
     * @brief 移除子进程（先优雅终止）
     * @param name 子进程名称
     */
    void remove_child(const string& name);

    /** @brief 获取子进程运行状态 */
    NEFORCE_NODISCARD child_status get_child_status(const string& name) const;

    /** @brief 获取所有子进程状态 */
    NEFORCE_NODISCARD vector<child_status> all_child_statuses() const;

    /**
     * @brief 进入信号驱动主循环（阻塞）
     * @return 退出码，0 为正常退出
     */
    int run();

    /** @brief 请求优雅关闭（可从任何线程调用） */
    void request_shutdown();

    /** @brief 请求重载配置（可从任何线程调用） */
    void request_reload();

    /** @brief 获取当前状态 */
    NEFORCE_NODISCARD daemon_state state() const noexcept { return state_.load(); }

    /**
     * @brief 看门狗喂狗（从工作线程周期性调用）
     *
     * 若超过 set_watchdog_timeout() 设置的时长未调用此方法，守护进程将强制退出。
     */
    void watchdog_ping();

    /**
     * @brief 设置看门狗超时
     * @param timeout_ms 超时毫秒数，0 表示禁用
     */
    void set_watchdog_timeout(int timeout_ms);
};

/** @} */ // Daemon

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_SYSTEM_DAEMON_HPP__
