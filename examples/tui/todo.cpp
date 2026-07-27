/**
 * @example todo.cpp
 * @brief TUI待办列表示例
 *
 * 演示 NeForce TUI 框架的高级功能：
 * - text_input 焦点输入 + 光标闪烁
 * - 列表渲染与 key-based 差分 (element::each + with_key)
 * - 复杂 state 管理 (vector<T> 批量更新)
 * - 文本输入框 placeholder 提示
 * - 条件样式 (done/undone 任务颜色区分)
 * - scroll_view 裁剪溢出内容 + 自动滚动条
 * - up/down 键盘导航 + 选中高亮
 * - 按钮按下动画反馈
 */

#include <NeForce/tui/app.hpp>
#include <NeForce/tui/component/scroll_view.hpp>
#include <NeForce/tui/component/text_input.hpp>

using namespace neforce;
using namespace neforce::tui;

class todo_component final : public component<> {
public:
    struct task {
        size_t id;
        string title;
        bool done;
    };

private:
    state<vector<task>>* tasks_ = nullptr;
    state<string>* input_ = nullptr;
    state<size_t>* selected_ = nullptr;
    state<int>* btn_flash_ = nullptr;
    component_base* input_comp_ = nullptr;
    component_base* sv_comp_ = nullptr;
    size_t next_id_ = 1;

public:
    void setup() override {
        tasks_ = &create_state<vector<task>>({});
        input_ = &create_state<string>("");
        selected_ = &create_state<size_t>(0);
        btn_flash_ = &create_state<int>(0);

        text_input_option input_opt;
        input_opt.text = input_;
        input_opt.placeholder = "Add a new task...";
        input_opt.on_enter = [this] { add_task(); };
        auto ti_comp = text_input(input_opt);
        input_comp_ = ti_comp.get();
        add_child(move(ti_comp));

        scroll_view_option sv_opt;
        sv_opt.content = [this] {
            return element::each<task>(tasks_->value(),
                                       [this](const task& t, size_t idx) { return render_task_row(t, idx); });
        };
        const int list_height = max(5, sys_console::instance().get_console_size().height - 7);
        sv_opt.style.border = style::border::rounded;
        sv_opt.style.height = style::size_hint{style::size_hint::fixed, list_height};
        using padding = struct style::padding;
        sv_opt.style.padding = padding{0, 1};
        auto sv = scroll_view(sv_opt);
        sv_comp_ = sv.get();
        add_child(move(sv));
    }

    void add_task() {
        const string title = input_->value();
        if (title.empty()) {
            return;
        }
        tasks_->modify([&](vector<task>& v) { v.emplace_back(next_id_++, title, false); });
        *input_ = "";
        *btn_flash_ = 1;
    }

    void toggle_task(size_t id) {
        tasks_->modify([id](vector<task>& v) {
            for (auto& t: v) {
                if (t.id == id) {
                    t.done = !t.done;
                    break;
                }
            }
        });
    }

    void remove_task(size_t id) {
        tasks_->modify([id](vector<task>& v) {
            v.erase(remove_if(v.begin(), v.end(), [id](const task& t) { return t.id == id; }), v.end());
        });
        const size_t selected = selected_->value();
        if (selected > 0 && selected >= tasks_->value().size()) {
            *selected_ = tasks_->value().empty() ? 0 : tasks_->value().size() - 1;
        }
    }

    element render_task_row(const task& t, size_t idx) {
        const string prefix = t.done ? "[x] " : "[ ] ";
        style task_style;
        task_style.fg = t.done ? color::gray() : color::white();
        if (idx == selected_->value()) {
            task_style.bg = color::blue();
            task_style.fg = color::white();
            task_style.bold = true;
        }

        return element::hbox({
                                     element::text(prefix + t.title, task_style),
                                     element::spacer(),
                                     element::button(
                                             " x ", [this, id = t.id] { remove_task(id); }, {}, style::variant::danger),
                             })
                .with_key(t.id);
    }

    element render() override {
        style title_style;
        title_style.fg = color::cyan();
        title_style.bold = true;

        style help_style;
        help_style.fg = color::gray();

        box_props input_row;
        input_row.gap = 1;

        box_props root_props;
        root_props.padding = {1, 2};

        element input_el = input_comp_->render();

        const int flash = btn_flash_->value();
        style add_btn_style{};
        if (flash > 0) {
            add_btn_style.bg = color::green();
            add_btn_style.fg = color::black();
            add_btn_style.bold = true;
        }

        return element::vbox(
                {
                        element::text("NexusForce TUI - Todo List", title_style),
                        element::separator(),

                        element::hbox(
                                {
                                        input_el,
                                        element::spacer(),
                                        element::button(
                                                "Add", [this] { add_task(); }, add_btn_style, style::variant::primary),
                                },
                                input_row),

                        element::separator(),

                        sv_comp_->render(),

                        element::separator(),
                        element::text("Enter=Add  Up/Down=Select  Space=Toggle  Backspace=Remove  q/Esc=Quit",
                                      help_style),
                },
                root_props);
    }

    void on_animation(int64_t delta) override {
        int flash = btn_flash_->value();
        if (flash > 0) {
            flash -= static_cast<int>(delta);
            flash = max(flash, 0);
            *btn_flash_ = flash;
        }

        for (size_t i = 0; i < child_count(); ++i) {
            child_at(i)->on_animation(delta);
        }
    }

    bool on_key(const key_event& e) override {
        using K = key_event::type;

        if (e.key == K::up) {
            const size_t selected = selected_->value();
            if (selected > 0) {
                *selected_ = selected - 1;
            }
            return true;
        }
        if (e.key == K::down) {
            const size_t selected = selected_->value();
            const auto& v = tasks_->value();
            if (!v.empty() && selected + 1 < v.size()) {
                *selected_ = selected + 1;
            }
            return true;
        }
        if (e.key == K::printable && e.cp == U' ') {
            const auto& v = tasks_->value();
            const size_t selected = selected_->value();
            if (!v.empty() && selected < v.size()) {
                toggle_task(v[selected].id);
            }
            return true;
        }
        if (e.key == K::backspace) {
            const auto& v = tasks_->value();
            const size_t selected = selected_->value();
            if (!v.empty() && selected < v.size()) {
                remove_task(v[selected].id);
            }
            return true;
        }
        if (e.key == K::enter) {
            add_task();
            return true;
        }
        if (e.key == K::escape || (e.key == K::printable && e.cp == U'q')) {
            ctx_->stop();
            return true;
        }
        return false;
    }
};

int main() {
    return application()
            .with_component<todo_component>()
            .with_theme(dark_theme)
            .with_fps(30)
            .with_title("NexusForce TODO List")
            .run();
}
