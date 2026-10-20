/**
 * @example chat.cpp
 * @brief TUI 对话应用示例
 *
 * 演示 NeForce TUI 终端对话组件栈：
 * - log_view：变高消息、流式合并、贴底跟随与"N 条新消息"提示
 * - composer：多行输入 + 斜杠命令补全 + 状态提示行
 * - tool_runner + process_watcher：真实子进程执行、输出流式汇入卡片、可中断
 * - status_indicator：运行状态行
 * - markdown / markdown_cache：模型回复的 Markdown 渲染与缓存
 * - application 默认全局快捷键：`Ctrl+C` 两段式退出、`Ctrl+L` 重绘
 *
 * 输入 `/` 打开命令补全；`/help` 查看命令；`/run <cmd>` 或 `!<cmd>` 执行
 * shell 命令并在卡片里流式显示输出；`Esc` 中断正在运行的工具。
 */

#include <NeForce/tui/application.hpp>
#include <NeForce/tui/component/composer.hpp>
#include <NeForce/tui/component/log_view.hpp>
#include <NeForce/tui/component/status_indicator.hpp>
#include <NeForce/tui/component/tool_runner.hpp>

using namespace neforce;
using namespace neforce::tui;
using namespace neforce::tui::components;

/**
 * @brief 对话应用根组件
 */
class chat_component final : public component<> {
public:
    void setup() override {
        log_view_option log_opt;
        log_opt.gap = 1;
        log_opt.markdown.width = 72;
        auto log = make_unique<log_view>(log_opt);
        log_ = log.get();
        add_child(move(log));

        tool_runner_option tool_opt;
        tool_opt.card.title = "bash";
        tool_opt.card.collapsed = false;
        tool_opt.process.use_pty = true; // 有 PTY 时保留颜色与进度条
        tool_opt.max_output_lines = 200;
        tool_opt.on_exit = [this](int) { set_status("ready"); };
        auto runner = make_unique<tool_runner>(*ctx_, *strand_, tool_opt);
        runner_ = runner.get();
        add_child(move(runner));

        status_indicator_option status_opt;
        status_opt.label = "ready";
        status_opt.show_elapsed = false;
        auto status = make_unique<status_indicator_view>(status_opt);
        status_ = status.get();
        add_child(move(status));

        auto input = make_unique<composer>(make_composer_option());
        input_ = input.get();
        add_child(move(input));

        log_->append(log_role::system, "NeForce chat demo \xe2\x80\x94 type a message, `/help` for commands, "
                                       "`!<cmd>` to run a shell tool.");
    }

    element render() override {
        elements parts;
        parts.push_back(log_->render());
        parts.push_back(runner_->render());

        box_props filler;
        filler.height = style::size_hint{style::size_hint::fill, 1};
        parts.push_back(element::vbox({}, filler));
        parts.push_back(status_->render());
        parts.push_back(input_->render());
        return element::vbox(move(parts));
    }

    bool on_key(const key_event& e) override {
        if (e.key == key_event::type::escape && runner_->running()) {
            interrupt_tool();
            return true;
        }
        return false;
    }

    void on_animation(int64_t delta) override {
        ignore = delta;
        if (!streaming_) {
            return;
        }
        if (++stream_tick_ % 5 != 0) {
            return;
        }
        push_stream_chunk();
    }

private:
    log_view* log_ = nullptr;
    composer* input_ = nullptr;
    status_indicator_view* status_ = nullptr;
    tool_runner* runner_ = nullptr;

    string reply_text_;
    uint64_t stream_message_ = 0;
    size_t stream_pos_ = 0;
    size_t stream_tick_ = 0;
    bool streaming_ = false;

    void set_status(const string& label) {
        if (status_ != nullptr) {
            status_->set_label(label);
        }
    }

    composer_option make_composer_option() {
        composer_option opt;
        opt.placeholder = "ask anything, or `/help`";
        opt.on_submit = [this](const string& text) { handle_submit(text); };
        opt.on_interrupt = [this] { interrupt_tool(); };
        opt.complete = [](const completion_request& request, vector<completion_item>& items) {
            struct entry {
                const string_view label;
                const string_view detail;
            };
            static constexpr entry commands[] = {{"/help", "show commands"},
                                                 {"/clear", "clear the transcript"},
                                                 {"/run", "run a shell command"},
                                                 {"/md", "render a markdown sample"}};
            for (const auto& command: commands) {
                if (request.query.empty() || command.label.contains(request.query.view())) {
                    completion_item item;
                    item.label = command.label;
                    item.detail = command.detail;
                    items.push_back(move(item));
                }
            }
        };
        return opt;
    }

    void emit(log_role role, const string& text) {
        const uint64_t id = log_->append(role, text);
        log_->finish(id);
    }

    void handle_submit(const string& text) {
        emit(log_role::user, text);

        const auto raw = text.view();
        if (raw == "/help"_sv) {
            emit(log_role::notice, "**Commands**\n\n"
                                   "- `/help` show this help\n"
                                   "- `/clear` clear the transcript\n"
                                   "- `/run <cmd>` run a shell command as a tool\n"
                                   "- `/md` render a markdown sample\n"
                                   "- `!<cmd>` shorthand for `/run`\n\n"
                                   "`esc` interrupts a running tool, `ctrl+c` twice to quit.");
            set_status("ready");
            return;
        }
        if (raw == "/clear"_sv) {
            log_->clear();
            return;
        }
        if (raw == "/md"_sv) {
            stream("## Markdown sample\n\nA table with alignment:\n\n"
                   "| name | value |\n"
                   "| :-- | --: |\n"
                   "| retry | 3 |\n"
                   "| timeout | 30 |\n\n"
                   "Inline `code`, **bold**, *italic* and a [link](https://example.com).\n\n"
                   "```cpp\n"
                   "auto task = retry_async(fn, 3);\n"
                   "```\n");
            return;
        }
        if (raw.size() > 1 && raw[0] == '!') {
            run_tool(string(raw.substr(1)));
            return;
        }
        if (raw.size() > 5 && raw.substr(0, 5) == "/run "_sv) {
            run_tool(string(raw.substr(5)));
            return;
        }

        stream("Echo: " + text +
               "\n\nThis demo streams the reply chunk by chunk, exactly like a model "
               "would: `log_view` coalesces the chunks into one render per frame.");
    }

    void stream(const string& text) {
        reply_text_ = text;
        stream_pos_ = 0;
        stream_tick_ = 0;
        streaming_ = true;
        stream_message_ = log_->append(log_role::assistant, string());
        set_status("thinking");
    }

    void push_stream_chunk() {
        const auto& raw = reply_text_.view();
        if (stream_pos_ >= raw.size()) {
            streaming_ = false;
            log_->finish(stream_message_);
            set_status("ready");
            return;
        }
        const size_t step = min<size_t>(raw.size() - stream_pos_, 5);
        log_->append_chunk(stream_message_, raw.substr(stream_pos_, step));
        stream_pos_ += step;
        log_->commit();
    }

    void run_tool(const string& command) {
        if (runner_ == nullptr || command.empty()) {
            return;
        }
        if (runner_->running()) {
            emit(log_role::notice, "*A tool is already running \xe2\x80\x94 press `esc` first.*");
            return;
        }
        set_status("running \xc2\xb7 esc to interrupt");
        if (!runner_->start(command)) {
            emit(log_role::notice, "*Failed to start the tool.*");
            set_status("ready");
        }
    }

    void interrupt_tool() {
        if (runner_ == nullptr || !runner_->running()) {
            return;
        }
        runner_->interrupt();
        emit(log_role::notice, "*Interrupted.*");
        set_status("ready");
    }
};

int main() { return application().with_component<chat_component>().with_title("NeForce Chat").with_fps(30).run(); }
