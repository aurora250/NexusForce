#ifndef NEFORCE_TUI_COMPONENT_TOOL_RUNNER_HPP__
#define NEFORCE_TUI_COMPONENT_TOOL_RUNNER_HPP__

/**
 * @file tool_runner.hpp
 * @brief 工具执行组件
 *
 * 把一次真实子进程调用包装成工具卡片：
 * 输出流式汇入可折叠输出区，状态与耗时自动更新，可被 Esc / 中断按钮打断。
 */

#include "NeForce/core/async/io_context.hpp"
#include "NeForce/core/async/process_watcher.hpp"
#include "NeForce/core/async/strand.hpp"
#include "NeForce/tui/component/component.hpp"
#include "NeForce/tui/component/tool_card.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__
NEFORCE_BEGIN_COMPONENTS__

/**
 * @addtogroup TUI TUI
 * @{
 */

/**
 * @addtogroup Components 用户组件
 * @{
 */

/**
 * @brief 工具执行配置
 */
struct tool_runner_option {
    tool_card_option card;           ///< 卡片外观与初始状态
    process_watcher::option process; ///< 进程启动配置
    size_t max_output_lines = 400;   ///< 保留的输出行数上限
    bool auto_scroll = true;         ///< 输出到达时是否自动展开输出区
    function<void(int)> on_exit;     ///< 退出回调
};

/**
 * @brief 工具执行组件
 *
 * @code
 * tool_runner_option opt;
 * opt.card.title = "bash";
 * opt.card.summary = "make -j8";
 * tool_runner runner(ctx, strand, opt);
 * runner.start("make -j8");
 * // 中断：runner.interrupt();
 * @endcode
 */
class NEFORCE_API tool_runner final : public component<> {
public:
    /**
     * @brief 构造函数
     * @param ctx 事件循环上下文
     * @param strand 串行执行器
     * @param opt 配置
     */
    tool_runner(io_context& ctx, strand& strand, tool_runner_option opt);

    /**
     * @brief 通过 shell 启动命令
     * @param command_line 命令行
     * @return 是否启动成功
     */
    bool start(const string& command_line);

    /**
     * @brief 中断执行
     */
    void interrupt();

    /**
     * @brief 是否仍在运行
     * @return 是否运行中
     */
    NEFORCE_NODISCARD bool running() const noexcept;

    /**
     * @brief 退出码
     * @return 退出码，未退出时为 -1
     */
    NEFORCE_NODISCARD int exit_code() const noexcept;

    /**
     * @brief 累计输出
     * @return 已保留的输出文本
     */
    NEFORCE_NODISCARD const string& output() const noexcept { return output_; }

    /**
     * @brief 内部卡片
     * @return 卡片引用
     */
    NEFORCE_NODISCARD tool_card& card() noexcept { return *card_; }

    /**
     * @brief 内部进程监视器
     * @return 监视器引用
     */
    NEFORCE_NODISCARD process_watcher& watcher() noexcept { return *watcher_; }

    /**
     * @brief 追加一段输出
     * @param chunk 文本
     */
    void append_output(string_view chunk);

    void setup() override;

    element render() override;

    bool on_key(const key_event& e) override;

    bool on_mouse(const mouse_event& e) override;

    void on_animation(int64_t delta) override;

    /**
     * @brief 卸载回调
     *
     * 结束仍在运行的子进程并注销事件循环句柄。
     */
    void cleanup() noexcept override;

    NEFORCE_NODISCARD bool focusable() const override { return true; }

private:
    io_context& ctx_;
    strand& strand_;
    tool_runner_option opt_;
    tool_card* card_ = nullptr;
    unique_ptr<process_watcher> watcher_;
    string output_;
    string pending_;
    bool dirty_output_ = false;
    bool interrupted_ = false;
    size_t dropped_lines_ = 0;

    void flush_output();
    void trim_output();
};

/**
 * @brief 创建工具执行组件
 * @param ctx 事件循环上下文
 * @param strand 串行执行器
 * @param opt 配置
 * @return 组件指针
 */
unique_ptr<component_base> NEFORCE_API tool_runner_component(io_context& ctx, strand& strand, tool_runner_option opt);

/** @} */ // Components

/** @} */ // TUI

NEFORCE_END_COMPONENTS__
NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_COMPONENT_TOOL_RUNNER_HPP__
