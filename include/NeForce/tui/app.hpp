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
#include "NeForce/tui/style.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

/**
 * @addtogroup TUI TUI
 * @{
 */

/// @cond
class Reconciler;
class InputDriver;
class ComponentBase;
/// @endcond

/**
 * @brief TUI应用程序入口
 *
 * 驱动主事件循环。
 *
 * @code
 * int main() {
 *     return Application()
 *         .withComponent<MyRootComponent>()
 *         .withTheme(kDarkTheme)
 *         .withFps(60)
 *         .run();
 * }
 * @endcode
 */
class Application {
public:
    Application();
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

    /**
     * @brief 设置根组件
     * @tparam RootComponent 根组件类型
     * @tparam Args 构造参数类型
     * @param args 构造参数
     * @return 自身引用
     */
    template <typename RootComponent, typename... Args>
    Application& withComponent(Args&&... args) {
        root_ = make_unique<RootComponent>(std::forward<Args>(args)...);
        return *this;
    }

    /**
     * @brief 设置主题
     * @param theme 主题
     * @return 自身引用
     */
    Application& withTheme(const Theme& theme);

    /**
     * @brief 设置最大刷新帧率
     * @param fps 帧率（0 表示仅事件驱动刷新）
     * @return 自身引用
     */
    Application& withFps(int fps);

    /**
     * @brief 阻塞运行事件循环
     * @returns 退出码
     */
    int run();

    /**
     * @brief 请求退出应用
     * @param exitCode 退出码
     */
    void quit(int exitCode = 0);

private:
    io_context ctx_;
    strand renderStrand_{ctx_};
    sys_console& console_;
    unique_ptr<Reconciler> reconiler_;
    unique_ptr<InputDriver> input_;
    unique_ptr<ComponentBase> root_;
    Theme theme_{dark_theme};
    int fps_{60};
    int exitCode_ = 0;
    bool running_ = false;
};

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_APP_HPP__
