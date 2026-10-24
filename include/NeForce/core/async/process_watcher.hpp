#ifndef NEFORCE_CORE_ASYNC_PROCESS_WATCHER_HPP__
#define NEFORCE_CORE_ASYNC_PROCESS_WATCHER_HPP__

/**
 * @file process_watcher.hpp
 * @brief 进程异步监视器
 *
 * 把子进程接入 `io_context`，不阻塞事件循环。支持管道模式与伪终端模式。
 */

#include "NeForce/core/async/io_context.hpp"
#include "NeForce/core/async/strand.hpp"
#include "NeForce/core/memory/unique_ptr.hpp"
#include "NeForce/core/system/pipe.hpp"
#include "NeForce/core/system/process.hpp"
#include "NeForce/core/system/pty.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup AsyncComponents 异步组件
 * @brief 异步编程相关组件
 * @{
 */

/**
 * @class process_watcher
 * @brief 进程异步监视器
 *
 * @code
 * process_watcher watcher(ctx, strand);
 * watcher.set_on_output([](string_view chunk) { ... });
 * watcher.set_on_exit([](int code) { ... });
 * watcher.start_shell("ls --color=auto");
 * ctx.run();
 * @endcode
 */
class NEFORCE_API process_watcher {
public:
    /**
     * @brief 启动配置
     */
    struct option {
        bool use_pty = false;      ///< 是否使用伪终端
        bool merge_stderr = true;  ///< 是否把 stderr 合并到输出流
        int pty_columns = 120;     ///< 伪终端列数
        int pty_rows = 40;         ///< 伪终端行数
        int poll_interval_ms = 30; ///< 退出轮询间隔
        string work_dir;           ///< 工作目录，空表示继承
        vector<string> env;        ///< 追加环境变量

        // NOLINTNEXTLINE(hicpp-use-equals-default,modernize-use-equals-default)
        option() {}
    };

    /**
     * @brief 构造函数
     * @param ctx 事件循环上下文
     * @param strand 串行执行器
     */
    process_watcher(io_context& ctx, strand& strand);

    /**
     * @brief 析构
     *
     * 结束仍在运行的子进程。
     */
    ~process_watcher();

    process_watcher(const process_watcher&) = delete;
    process_watcher& operator=(const process_watcher&) = delete;
    process_watcher(process_watcher&&) = delete;
    process_watcher& operator=(process_watcher&&) = delete;

    /**
     * @brief 当前环境是否支持伪终端
     * @return 是否支持
     */
    NEFORCE_NODISCARD static bool supports_pty() noexcept;

    /**
     * @brief 启动子进程
     * @param executable 可执行文件
     * @param args 参数
     * @param opt 启动配置
     * @return 是否启动成功
     */
    bool start(const string& executable, const vector<string>& args = vector<string>{}, option opt = option{});

    /**
     * @brief 通过 shell 启动命令行
     * @param command_line 命令行
     * @param opt 启动配置
     * @return 是否启动成功
     */
    bool start_shell(const string& command_line, option opt = option{});

    /**
     * @brief 是否仍在运行
     * @return 是否运行中
     */
    NEFORCE_NODISCARD bool running() const noexcept { return running_; }

    /**
     * @brief 是否使用伪终端
     * @return 是否伪终端模式
     */
    NEFORCE_NODISCARD bool uses_pty() const noexcept { return term_ != nullptr; }

    /**
     * @brief 退出码
     * @return 退出码，未退出时为 -1
     */
    NEFORCE_NODISCARD int exit_code() const noexcept { return exit_code_; }

    /**
     * @brief 写入子进程标准输入
     * @param data 数据
     */
    void write(string_view data);

    /**
     * @brief 调整伪终端尺寸
     * @param columns 列数
     * @param rows 行数
     */
    void resize(int columns, int rows);

    /**
     * @brief 向伪终端发送控制字符
     * @param byte 控制字节
     */
    void send_control(char byte);

    /**
     * @brief 终止子进程
     */
    void terminate();

    /**
     * @brief 设置输出回调
     * @param cb 回调，参数为本次读到的数据
     */
    void set_on_output(function<void(string_view)> cb) { on_output_ = _NEFORCE move(cb); }

    /**
     * @brief 设置退出回调
     * @param cb 回调，参数为退出码
     */
    void set_on_exit(function<void(int)> cb) { on_exit_ = _NEFORCE move(cb); }

private:
    io_context& ctx_;
    strand& strand_;
    unique_ptr<process> proc_;
    unique_ptr<pipe> out_pipe_;
    unique_ptr<pty> term_;
    function<void(string_view)> on_output_;
    function<void(int)> on_exit_;
    int poll_interval_ms_ = 30;
    int exit_code_ = -1;
    int settle_polls_ = 0;
    bool running_ = false;
    bool term_sent_ = false;
    shared_ptr<bool> alive_ = make_shared<bool>(true);

    void attach_pipe();
    void attach_pty();
    void schedule_poll(const shared_ptr<bool>& guard);
    bool drain_pipe();
    bool drain_pty();
    void poll_exit();
    void finish(int code);
    void release_handles();
};

/** @} */ // Async

/** @} */ // AsyncComponents

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_PROCESS_WATCHER_HPP__
