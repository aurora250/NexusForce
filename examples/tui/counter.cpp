/**
 * @example counter.cpp
 * @brief TUI计数器示例
 *
 * 演示 NeForce TUI 框架的核心功能：
 * - 声明式 Element DSL (vbox/hbox/text/button/spacer/separator)
 * - 响应式 State<T> 状态管理
 * - Component 生命周期 (setup/render)
 * - 键盘事件处理 (onKey)
 * - Application 入口组装
 */

#include <NeForce/tui/app.hpp>
#include <NeForce/tui/component.hpp>
#include <NeForce/tui/element.hpp>
#include <NeForce/tui/state.hpp>
#include <NeForce/tui/style.hpp>

using namespace neforce;
using namespace neforce::tui;

class CounterApp : public Component<EmptyProps> {
    State<int>* count_ = nullptr;

    // ========== 生命周期 ==========
    void setup() override { count_ = &createState<int>(0); }

    // ========== 声明式渲染 ==========
    Element render() override {
        string countStr = to_string(**count_);

        Style titleStyle;
        titleStyle.fg = color::cyan();
        titleStyle.bold = true;

        Style countStyle;
        countStyle.fg = color::white();

        BoxProps btnRow;
        btnRow.gap = 1;
        btnRow.justify = Justify::Center;

        BoxProps rootProps;
        rootProps.padding = {1, 2};
        rootProps.justify = Justify::Center;
        rootProps.align = Align::Center;

        return Element::vbox(
                {
                        Element::text("NexusForce TUI - Counter", titleStyle),
                        Element::separator(),
                        Element::spacer(),
                        Element::text("Count: " + countStr, countStyle),
                        Element::spacer(),
                        Element::hbox(
                                {
                                        Element::button(
                                                " +1 ", [this] { *count_ = **count_ + 1; }, {}, Variant::Primary),
                                        Element::button(" -1 ", [this] { *count_ = **count_ - 1; }),
                                        Element::button(
                                                "Reset", [this] { *count_ = 0; }, {}, Variant::Danger),
                                        Element::button(" Quit ", [this] { ctx_->stop(); }),
                                },
                                btnRow),
                        Element::spacer(),
                },
                rootProps);
    }

    // ========== 键盘事件 ==========
    bool onKey(const KeyEvent& e) override {
        if (e.key == KeyEvent::Key::Printable && e.ch == U'+') {
            *count_ = **count_ + 1;
            return true;
        }
        if (e.key == KeyEvent::Key::Printable && e.ch == U'-') {
            *count_ = **count_ - 1;
            return true;
        }
        if (e.key == KeyEvent::Key::Printable && e.ch == U'r') {
            *count_ = 0;
            return true;
        }
        if (e.key == KeyEvent::Key::Escape || (e.key == KeyEvent::Key::Printable && e.ch == U'q')) {
            ctx_->stop();
            return true;
        }
        return false;
    }
};

// ========== 入口 ==========
int main() { return Application().withComponent<CounterApp>().withTheme(dark_theme).withFps(30).run(); }
