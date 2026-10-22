#ifndef NEFORCE_TUI_APPLICATION_HPP__
#define NEFORCE_TUI_APPLICATION_HPP__

/**
 * @file application.hpp
 * @brief TUI应用程序入口
 *
 * 驱动主事件循环。
 */

#include "NeForce/core/async/strand.hpp"
#include "NeForce/core/system/console.hpp"
#include "NeForce/core/functional/function.hpp"
#include "NeForce/tui/dom/style.hpp"
#include "NeForce/tui/keymap.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

/**
 * @addtogroup TUI TUI
 * @{
 */

/// @cond
class reconciler;
class input_driver;
class component_base;
/// @endcond

/**
 * @brief TUI应用程序入口
 *
 * 驱动主事件循环。
 *
 * @code
 * int main() {
 *     return application()
 *         .with_component<my_root_component>()
 *         .with_theme(dark_theme)
 *         .with_fps(60)
 *         .run();
 * }
 * @endcode
 */
class NEFORCE_API application {
public:
    application();
    ~application();

    application(const application&) = delete;
    application& operator=(const application&) = delete;
    application(application&&) = delete;
    application& operator=(application&&) = delete;

    /**
     * @brief 设置根组件
     * @tparam RootComponent 根组件类型
     * @tparam Args 构造参数类型
     * @param args 构造参数
     * @return 自身引用
     */
    template <typename RootComponent, typename... Args>
    application& with_component(Args&&... args) {
        root_ = _NEFORCE make_unique<RootComponent>(_NEFORCE forward<Args>(args)...);
        return *this;
    }

    /**
     * @brief 设置主题
     * @param theme 主题
     * @return 自身引用
     */
    application& with_theme(const theme& theme);

    /**
     * @brief 设置最大刷新帧率
     * @param fps 帧率（0 表示仅事件驱动刷新）
     * @return 自身引用
     */
    application& with_fps(int fps);

    /**
     * @brief 设置程序名
     * @param title 程序名
     * @return 自身引用
     */
    application& with_title(string title);

    /**
     * @brief 阻塞运行事件循环
     * @returns 退出码
     */
    int run();

    /**
     * @brief 请求退出应用
     * @param exit_code 退出码
     */
    void quit(int exit_code = 0);

    /**
     * @brief 让出终端
     *
     * 退出交替屏幕缓冲区、关闭鼠标与括号粘贴、恢复终端原始属性并显示光标，
     * 使调用方可以把终端交给交互式子进程。此时应用仍持有组件树与状态。
     */
    void suspend();

    /**
     * @brief 收回终端
     *
     * 重新进入交替屏幕缓冲区、恢复鼠标与括号粘贴、回到原始模式，并强制整屏重绘
     */
    void resume();

    /**
     * @brief 是否处于让出终端状态
     * @return 是否已让出
     */
    NEFORCE_NODISCARD bool suspended() const noexcept { return suspended_; }

    /**
     * @brief 开关两段式 Ctrl+C 退出
     * @param enable 是否启用
     * @return 自身引用
     *
     * 启用后第一次 `Ctrl+C` 只进入"待确认"状态，第二次才真正退出；
     * 两秒内没有第二次按键会自动复位。
     */
    application& with_two_stage_quit(bool enable = true) noexcept;

    /**
     * @brief 是否处于待确认退出的状态
     * @return 是否待确认
     */
    NEFORCE_NODISCARD bool quit_pending() const noexcept { return quit_pending_; }

    /**
     * @brief 设置待确认状态变化回调
     * @param cb 回调，参数为是否待确认
     *
     * 供应用在状态行之类的位置提示。
     */
    application& on_quit_hint(function<void(bool)> cb);

    /**
     * @brief 获取全局快捷键表
     * @return 快捷键表引用，应用可在运行前追加全局绑定
     * @note 仅在 `run()` 之后有效。
     */
    NEFORCE_NODISCARD keymap* shortcuts() noexcept;

private:
    io_context ctx_;
    strand render_strand_{ctx_};
    sys_console& console_{sys_console::instance()};
    unique_ptr<reconciler> reconiler_;
    unique_ptr<input_driver> input_;
    unique_ptr<component_base> root_;
    theme theme_{dark_theme};
    int fps_{60};
    int exit_code_ = 0;
    string title_;
    bool running_ = false;
    bool suspended_ = false;
    bool two_stage_quit_ = true;
    bool quit_pending_ = false;
    function<void(bool)> quit_hint_;

    void schedule_quit_hint_reset();
    void schedule_resize_check();
    void schedule_frame_tick();
};

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_APPLICATION_HPP__
