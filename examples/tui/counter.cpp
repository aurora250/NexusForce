/**
 * @example counter.cpp
 * @brief TUI计数器示例
 *
 * 演示 NeForce TUI 框架的核心功能：
 * - 声明式 element DSL (vbox/hbox/text/button/spacer/separator)
 * - 响应式 state<T> 状态管理
 * - component 生命周期 (setup/render)
 * - 键盘事件处理 (on_key)
 * - application 入口组装
 */

#include <NeForce/tui/application.hpp>
#include <NeForce/tui/component/component.hpp>

using namespace neforce;
using namespace neforce::tui;

class counter_component final : public component<> {
    state<int>* count_ = nullptr;

    // ========== 生命周期 ==========
    void setup() override { count_ = &create_state<int>(0); }

    // ========== 声明式渲染 ==========
    element render() override {
        const string count_str = to_string(**count_);

        style title_style;
        title_style.fg = color::cyan();
        title_style.bold = true;

        style count_style;
        count_style.fg = color::white();

        box_props btn_row;
        btn_row.gap = 1;
        btn_row.justify = style::justify::center;

        box_props root_props;
        root_props.padding = {1, 2};
        root_props.justify = style::justify::center;
        root_props.align = style::align::center;

        return element::vbox(
                {
                        element::text("NexusForce TUI - Counter", title_style),
                        element::separator(),
                        element::spacer(),
                        element::text("Count: " + count_str, count_style),
                        element::spacer(),
                        element::hbox(
                                {
                                        element::button(
                                                " +1 ", [this] { *count_ = **count_ + 1; }, {},
                                                style::variant::primary),
                                        element::button(" -1 ", [this] { *count_ = **count_ - 1; }),
                                        element::button(
                                                "Reset", [this] { *count_ = 0; }, {}, style::variant::danger),
                                        element::button(" Quit ", [this] { ctx_->stop(); }),
                                },
                                btn_row),
                        element::spacer(),
                },
                root_props);
    }

    // ========== 键盘事件 ==========
    bool on_key(const key_event& e) override {
        if (e.key == key_event::type::printable && e.cp == U'+') {
            *count_ = **count_ + 1;
            return true;
        }
        if (e.key == key_event::type::printable && e.cp == U'-') {
            *count_ = **count_ - 1;
            return true;
        }
        if (e.key == key_event::type::printable && e.cp == U'r') {
            *count_ = 0;
            return true;
        }
        if (e.key == key_event::type::escape || (e.key == key_event::type::printable && e.cp == U'q')) {
            ctx_->stop();
            return true;
        }
        return false;
    }
};

// ========== 入口 ==========
int main() {
    return application()
            .with_component<counter_component>()
            .with_theme(dark_theme)
            .with_fps(30)
            .with_title("NexusForce Counter")
            .run();
}
