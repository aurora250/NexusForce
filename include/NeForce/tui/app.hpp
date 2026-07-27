#ifndef NEFORCE_TUI_APP_HPP__
#define NEFORCE_TUI_APP_HPP__

/**
 * @file app.hpp
 * @brief TUI应用程序入口
 *
 * 驱动主事件循环。
 */

#include "NeForce/core/async/strand.hpp"
#include "NeForce/core/system/console.hpp"
#include "NeForce/tui/dom/style.hpp"
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
};

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_APP_HPP__
