/**
 * @example todo.cpp
 * @brief TUI待办列表示例
 *
 * 演示 NeForce TUI 框架的高级功能：
 * - 列表渲染与 key-based 差分 (Element::each + withKey)
 * - 复杂 State 管理 (vector<T> 批量更新)
 * - 文本输入框 (Element::textInput)
 * - 条件样式 (done/undone 任务颜色区分)
 * - ScrollView 裁剪溢出内容
 * - Up/Down 键盘导航 + 选中高亮
 * - 键盘快捷键 (Enter/Space/Backspace/q/Esc)
 */

#include <NeForce/tui/app.hpp>
#include <NeForce/tui/component.hpp>
#include <NeForce/tui/element.hpp>
#include <NeForce/tui/state.hpp>
#include <NeForce/tui/style.hpp>

using namespace neforce;
using namespace neforce::tui;

struct Task {
    size_t id;
    string title;
    bool done;
};

class TodoApp : public Component<EmptyProps> {
    State<vector<Task>>* tasks_ = nullptr;
    State<string>* input_ = nullptr;
    State<size_t>* selected_ = nullptr;
    size_t nextId_ = 1;

    // ========== 生命周期 ==========
    void setup() override {
        tasks_ = &createState<vector<Task>>({});
        input_ = &createState<string>("");
        selected_ = &createState<size_t>(0);
    }

    // ========== 数据操作 ==========
    void addTask() {
        string title = input_->value();
        if (title.empty()) {
            return;
        }
        tasks_->modify([&](vector<Task>& v) { v.push_back({nextId_++, title, false}); });
        *input_ = "";
    }

    void toggleTask(size_t id) {
        tasks_->modify([id](vector<Task>& v) {
            for (auto& t: v) {
                if (t.id == id) {
                    t.done = !t.done;
                    break;
                }
            }
        });
    }

    void removeTask(size_t id) {
        tasks_->modify([id](vector<Task>& v) {
            v.erase(remove_if(v.begin(), v.end(), [id](const Task& t) { return t.id == id; }), v.end());
        });
        size_t sel = selected_->value();
        if (sel > 0 && sel >= tasks_->value().size()) {
            *selected_ = tasks_->value().empty() ? 0 : tasks_->value().size() - 1;
        }
    }

    // ========== 渲染辅助 ==========
    Element renderTaskRow(const Task& t, size_t idx) {
        string prefix = t.done ? "[x] " : "[ ] ";
        Style taskStyle;
        taskStyle.fg = t.done ? color::gray() : color::white();
        if (idx == selected_->value()) {
            taskStyle.bg = color::blue();
            taskStyle.fg = color::white();
            taskStyle.bold = true;
        }

        return Element::hbox({
                                     Element::text(prefix + t.title, taskStyle),
                                     Element::spacer(),
                                     Element::button(
                                             " x ", [this, id = t.id] { removeTask(id); }, {}, Variant::Danger),
                             })
                .withKey(t.id);
    }

    // ========== 声明式渲染 ==========
    Element render() override {
        Style titleStyle;
        titleStyle.fg = color::cyan();
        titleStyle.bold = true;

        Style helpStyle;
        helpStyle.fg = color::gray();

        BoxProps inputRow;
        inputRow.gap = 1;

        BoxProps rootProps;
        rootProps.padding = {1, 2};

        const auto& taskList = tasks_->value();

        int listHeight = max(5, sys_console::instance().get_console_size().height - 7);

        Style listStyle;
        listStyle.border = Border::Rounded;
        listStyle.height = SizeHint{SizeHint::Fixed, listHeight};
        listStyle.padding = Padding{0, 1};

        return Element::vbox(
                {
                        Element::text("NexusForce TUI - Todo List", titleStyle),
                        Element::separator(),

                        Element::hbox(
                                {
                                        Element::textInput(*input_),
                                        Element::button(
                                                "Add", [this] { addTask(); }, {}, Variant::Primary),
                                },
                                inputRow),

                        Element::separator(),

                        Element::scrollView(
                                Element::each<Task>(
                                        taskList,
                                        [this](const Task& t,
                                               size_t idx) -> Element { return renderTaskRow(t, idx); }),
                                listStyle,
                                0, 0),

                        Element::separator(),
                        Element::text(
                                "↑↓=Select  Enter=Add  Space=Toggle  Backspace=Remove  q/Esc=Quit",
                                helpStyle),
                },
                rootProps);
    }

    // ========== 键盘事件 ==========
    bool onKey(const KeyEvent& e) override {
        if (e.key == KeyEvent::Key::Enter) {
            addTask();
            return true;
        }
        if (e.key == KeyEvent::Key::Up) {
            size_t sel = selected_->value();
            if (sel > 0) {
                *selected_ = sel - 1;
            }
            return true;
        }
        if (e.key == KeyEvent::Key::Down) {
            size_t sel = selected_->value();
            const auto& v = tasks_->value();
            if (!v.empty() && sel + 1 < v.size()) {
                *selected_ = sel + 1;
            }
            return true;
        }
        if (e.key == KeyEvent::Key::Printable && e.ch == U' ') {
            const auto& v = tasks_->value();
            size_t sel = selected_->value();
            if (!v.empty() && sel < v.size()) {
                toggleTask(v[sel].id);
            }
            return true;
        }
        if (e.key == KeyEvent::Key::Backspace) {
            const auto& v = tasks_->value();
            size_t sel = selected_->value();
            if (!v.empty() && sel < v.size()) {
                removeTask(v[sel].id);
            }
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
int main() { return Application().withComponent<TodoApp>().withTheme(dark_theme).withFps(30).run(); }
