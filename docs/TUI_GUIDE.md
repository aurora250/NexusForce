# TUI 使用指南

NeForce TUI 是一个声明式终端 UI 框架：组件返回不可变元素树，框架负责布局、差分渲染与事件路由。

示例：
`examples/tui/counter.cpp`（基础）
`examples/tui/todo.cpp`（容器与焦点）
`examples/tui/chat.cpp`（对话 + 工具执行 + 补全）

---

## 1. 分层结构

```
你的组件 (component_base)
        │  render() 返回 element 树
        ▼
   element（不可变值类型）──compute_layout──▶ layout_rect 列表
        │                                        │
        ▼                                        ▼
   renderer（绘制文本/边框/背景带/滚动条）    screen（cell 网格）
        │                                        │
        └──────────── reconciler.flush() ────────┘
                              │
                              ▼ 差分 ANSI（整帧用同步输出包裹）
                         sys_console
```

- **element** 纯值类型：深拷贝、不持有终端资源，可以安全缓存。
- **layout** 纯函数：`compute_layout(tree, w, h)` 返回叶子矩形，结果缓存在元素节点上。
- **renderer** 把元素树画进 `screen`。
- **reconciler** 编排"组件树 → 元素树 → 布局 → 绘制 → 差分"流水线。
- **component** 唯一持有状态、处理事件的对象。
- **application** 作为 TUI 程序入口。

---

## 2. 最小可运行程序

```cpp
#include <NeForce/tui/application.hpp>
#include <NeForce/tui/component/component.hpp>

using namespace neforce;
using namespace neforce::tui;

class hello final : public component<> {
    element render() override { return element::text("hello, terminal"); }
};

int main() {
    return application().with_component<hello>().with_title("Hello").run();
}
```

---

## 3. 元素与布局

### 3.1 常用元素

| 工厂                                                           | 说明                                              |
|----------------------------------------------------------------|---------------------------------------------------|
| `element::text(content, style, wrap)`                          | 单一样式文本，`wrap` 为 `none`/`word`/`character` |
| `element::rich_text(spans, base, wrap)`                        | 富文本，一行内混排多种样式与超链接                |
| `element::vbox/hbox/zstack`                                    | 列 / 行 / 叠放容器                                |
| `element::panel(child, style)`                                 | 带边框与内边距的面板（对话框、卡片）              |
| `element::band(child, style)`                                  | 背景色带（代码块、差异、工具卡片的整行底色）      |
| `element::separator()` / `spacer(flex)`                        | 分割线 / 弹性占位                                 |
| `element::scroll_view(child, style, x, y)`                     | 带滚动条的滚动区                                  |
| `element::virtual_list(...)`                                   | 只渲染可视项的列表（见 3.4）                      |
| `element::canvas(draw_fn)`                                     | 直接向 `screen` 绘制（图表、自定义控件）          |
| `element::when(cond, then, else)` / `element::each(items, fn)` | 条件与列表渲染                                    |

样式是 `optional` 字段集合，未设置的字段继承主题；也可用装饰器：
`element::text("x") | bold() | color(color::red())`。

### 3.2 尺寸与对齐

```cpp
box_props props;
props.dir = style::direction::row;
props.justify = style::justify::space_between;
props.align = style::align::center;
props.gap = 1;
props.width = style::size_hint{style::size_hint::fill, 1};   // 占满剩余
props.height = style::size_hint{style::size_hint::fixed, 5}; // 固定 5 行
```

`size_hint::mode`：`auto_`（内容决定）、`fixed`、`fill`、`percent`。

### 3.3 文本排版（`dom/text_layout.hpp`）

渲染器、布局测量与编辑器光标共用同一份分行实现：

```cpp
const text_layout lay = layout_text("aaa bbb ccc", 7, style::wrap_mode::word);
int row = 0, col = 0;
caret_position(lay, text, byte_offset, row, col);  // 字节偏移 → (行, 列宽)
const int width = text_display_width("中文");      // 全角按 2 列
```

`\n` 始终硬换行；`word` 模式在行宽内最后一个空格断行并丢弃该空格；单词超宽退化为按字符断行；
控制字符按一列空白渲染，永远不会进入 cell 网格。

### 3.4 虚拟列表与变高项

聊天记录有几千条时只能渲染可视项。
**虚拟列表自身没有内容高度**，布局按 `item_count × item_height` 估算，因此内嵌在自动高度容器里也不会塌陷，要铺满剩余空间需显式尺寸：

```cpp
// 固定高度项
element list = element::virtual_list(count, 3, [](size_t i) { return row_of(i); })
                       .with_scroll_y_state(&scroll)         // 绑定滚动状态（滚轮/拖拽）
                       .with_viewport_state(&viewport_rows); // 回写可视行数
list = list.with_style([] {
    tui::style st;
    st.height = style::size_hint{style::size_hint::fixed, 6};
    return st;
}());

// 变高项：高度表由调用方持有，渲染器写回实测高度
element msgs = element::virtual_list(count, 4, &heights, [](size_t i) { return msg_of(i); })
                       .with_scroll_y_state(&scroll)
                       .with_viewport_rect_state(&rect); // 回写视口矩形
```

通常直接用组件更省事：

```cpp
virtual_list_option opt;
opt.variable_height = true;  // 按实测高度定位
opt.item_height = 4;         // 未测量项的估算高度
opt.render_item = [this](size_t i) { return render_message(i); };
opt.style.height = style::size_hint{style::size_hint::fill, 1};

auto list = make_unique<virtual_list_view>(opt);
list->set_item_count(n);      // 追加后调用，自动贴底跟随
list->invalidate_heights();   // 宽度/内容变化后重测
```

变高项的实测只覆盖**曾经可见**的项，因此贴底场景下尾部先收敛，这是设计使然而非缺陷。

虚拟列表**没有内容高度**，布局时按 `item_count × item_height` 给出一个估算固有高度：
这样 `vbox({header, list})` 这类自动高度容器不会把它压成 0 行，剩余空间仍由 `style.height = fill` 决定。
需要精确铺满时应显式给 `fill`，估算值只用于自动尺寸的父容器。

---

## 4. 组件与状态

### 4.1 生命周期

| 钩子                       | 时机                               | 该做什么                                            |
|----------------------------|------------------------------------|-----------------------------------------------------|
| `setup()`                  | 首次 render 之前、`add_child()` 时 | `create_state<T>()`、启动异步、订阅                 |
| `render()`                 | 每次 flush                         | **纯函数**：只构建 element 树，禁止 I/O、禁止改状态 |
| `on_key/on_mouse/on_paste` | 事件派发                           | 消费返回 `true`                                     |
| `on_animation(delta)`      | 每帧（`fps > 0`）                  | 推进动画/计时、提交流式缓冲                         |
| `cleanup()`                | 从父组件移除时、宿主销毁时         | 断开连接、释放资源（同步句柄/子进程）               |

> ⚠️ **不要在 `render()` 里无条件调用子组件的 setter**。子组件的 setter 会 `schedule_render()`，
> 而 render 本身又在 flush 中执行，于是每一帧又排一帧，`headless::tick()` / `application::run()` 永不收敛。
> 正确做法是把要下发的数据设计成签名，只在签名变化时回写。

`cleanup()` 由 `component_base::release_tree()` 在宿主销毁时递归调用（`reconciler` 析构 → 根组件树），
因此持有 `pipe` / `io_context` 句柄的组件要在这里释放， 否则组件比事件循环活得久时会访问已销毁的 `io_context`：

```cpp
void my_runner::cleanup() noexcept {
    if (watcher_ != nullptr) {          // 结束子进程并注销 fd
        if (watcher_->running()) { watcher_->terminate(); }
        watcher_.reset();
    }
}
```

如果直接使用 `process_watcher`，请保证它在 `io_context` 之前析构；它的定时回调带有
生存标志，销毁后到期不会访问自身，但 `remove_fd()` 需要事件循环仍然存在。

### 4.2 响应式状态

```cpp
class counter final : public component<> {
    state<int>* count_ = nullptr;
    void setup() override { count_ = &create_state<int>(0); }
    element render() override { return element::text(to_string(**count_)); }
};

*count_ = 5;                                              // 整值替换，自动重渲染
count_->modify([](int& v) { ++v; });                      // 就地修改
count_->set_quiet(10); /* 批量 */ count_->notify();       // 静默写 + 一次通知
count_->on_change([](const int& v) { /* ... */ });        // 订阅变化
```

高频写入务必批量提交，见 9.2 的 `log_view::append_chunk` + `commit`。

### 4.3 组合与上下文

```cpp
add_child(make_unique<my_child>());     // 父组件接管生命周期与焦点链
child_at(0)->render();                  // 在父组件 render 里内联子树

provide_context<app_state>(state);      // 向下注入
const auto& ctx = context<app_state>(); // 沿父链查找（找不到触发 value_exception）
```

---

## 5. 事件与焦点

### 5.1 事件

```cpp
struct key_event   { type key; codepoint cp; key_modifier mods; };
struct mouse_event { int x, y; mouse_button button; mouse_action action; key_modifier mods; };
struct paste_event { string text; };
```

输入由 `tui::input_parser` 解析：ASCII/UTF-8、控制键、CSI/SS3、带修饰键序列（`ESC[1;5C`）、
`Alt+字符`、SGR 鼠标、`ESC[200~…201~` 括号粘贴。

### 5.2 焦点

- 可聚焦组件实现 `focusable() { return true; }`；
- `Tab` 先交给焦点组件，未被消费才做焦点导航；
- `set_pass_through(true)` 让未消费的按键继续冒泡到根组件；
- 组件动态增删会标记结构变化，焦点链自动重建，被移除的焦点组件会被安全清理。

### 5.3 命中与鼠标

`reconciler` 用上一帧布局做命中测试：点击先命中 `element::on_click()`（按钮），
再命中组件 `on_mouse()`；滚动条拖拽与滚轮由渲染器收集的命中信息驱动。
`output_block`/`tool_card` 的点击折叠依赖"事件已命中本组件"这一前提，组件内无需算坐标。

文本编辑器的鼠标支持：点击定位光标、按住拖动扩展选区（`text_area::on_mouse`）。

---

## 6. 快捷键

```cpp
keymap map;
map.bind({.key = key_event::type::printable, .required = key_modifier::ctrl, .character = U'p',
          .handler = [](const key_event&) { /* ... */ return true; }, .description = "ctrl+p palette"});

if (!map.dispatch(event)) {
    // 未被任何绑定消费 → 继续派发给组件树
}
```

三层：`global` → `overlay` → `focus`，同层内**后注册者优先**；
`required`/`forbidden` 描述 "必须按下"/"必须未按下" 的修饰键；`hints()` 则可直接用于状态行。

---

## 7. Markdown

实现 AST → 元素映射 流程。

```cpp
markdown_option opt;
opt.width = 72;              // 0 表示由布局决定
opt.highlight_code = true;
opt.link_style.fg = color::blue();

element body = markdown(source, opt);   // 直接渲染

markdown_cache cache(opt, 64);          // 或走缓存
const element& cached = cache.render(source, 72);
```

链接会渲染成 OSC-8 超链接（`text_span::link`），终端支持时可直接点击。

代码块与差异自带整块背景、左侧竖线与折叠：

```cpp
code_block_option cb;
cb.code = source;
cb.language = "cpp";
cb.collapse_after = 20;   // 超过 20 行折叠中间部分，保留首尾并提示"… N 行已折叠"

diff_view_option dv;
dv.group_by_file = true;        // 按文件分组，文件头带 +N -M 统计
dv.collapse_hunk_after = 40;    // 单个 hunk 过长时折叠
element patch = diff_view(unified_diff_text, dv);
```

---

## 8. 主题与颜色

- 预置 `dark_theme` / `light_theme`，`application::with_theme(...)` 切换；
- 输出颜色深度由终端能力决定（`color_depth::ansi16/ansi256/truecolor`），可用
  `reconciler::set_color_depth()` 显式覆盖；
- 整帧用 `ESC[?2026h/l` 包裹（同步输出），支持该扩展的终端会原子提交整帧；
  `set_synchronized_output(false)` 关闭。

---

## 9. 对话界面

### 9.1 `log_view`：消息记录

```cpp
log_view log;
const uint64_t id = log.append(log_role::assistant, "partial");
log.append_chunk(id, " answer");   // 流式追加
log.commit();                      // 每帧提交一次
log.finish(id);                    // 走 markdown_cache
```

- 消息高度按实测值定位，每帧只渲染可视窗口；
- 贴底自动跟随；用户上滚后停止跟随并累计"↓ N new"，回到底部清零；
- 点击消息触发 `log_view_option::on_message_click`，`copy_message(id)` 把消息文本通过 OSC 52 写入剪贴板；
- **流式中的消息绕过缓存**，finish 后才会写入。

### 9.2 高频写入的正确姿势

```cpp
void on_token(string_view token) {
    log.append_chunk(id, token);   // 只进缓冲
}
void on_animation(int64_t) {
    log.commit();                  // 每帧一次状态写入 → 一帧一次重渲染
}
```

### 9.3 `composer`：输入区

```cpp
composer_option opt;
opt.complete = [](const completion_request& req, vector<completion_item>& out) {
    // req.query 是触发符之后、光标之前的文本
};
opt.on_submit = [](const string& text) { /* 提交 */ };
opt.on_interrupt = [] { /* 运行中按 Esc */ };
auto input = make_unique<composer>(opt);
```

- `Enter` 提交、`Shift+Enter` 换行、`Ctrl+Enter` 提交；
- `/`（可配 `trigger`）自动唤起补全，`Tab`/`Enter` 接受，`Esc` 先关弹层；
- 运行中 `Esc` 触发 `on_interrupt`；`set_phase(composer_phase::running)` 切换提示行；
- `take_focus()` 会把焦点转交内部编辑器，因此键盘事件统一由 composer 转发。

### 9.4 其它对话组件

- `approve_prompt`：权限确认，方向键 + 数字键 + `Esc` 选择；
- `status_indicator`：旋转 + 文案 + 耗时，由 `on_animation` 驱动；
- `streaming_text`：单段文本的帧内合并提交；
- `select_list`：可过滤候选列表。

---

## 10. 工具执行（子进程）

### 10.1 伪终端

```cpp
pty session;
session.open(pty::console_size{120, 40});                 // 尺寸
session.start("/bin/bash", {"-lc", "ls --color=auto"});   // 拥有 isatty()、颜色、SIGWINCH
const string output = session.read_available();
session.resize(100, 30);        // TIOCSWINSZ + SIGWINCH
session.send_control(0x03);     // 模拟 Ctrl+C
session.terminate();            // 发给整个进程组
const int code = session.wait(5000);
```

### 10.2 异步桥

```cpp
process_watcher watcher(ctx, strand);
watcher.set_on_output([](string_view chunk) { /* io_context 线程回调 */ });
watcher.set_on_exit([](int code) { /* ... */ });

process_watcher::option opt;
opt.use_pty = true;          // 需要终端语义时打开；不可用会自动降级为管道
opt.merge_stderr = true;
watcher.start_shell("make -j8", opt);
```

全程不阻塞事件循环；管道模式下父进程会主动关闭写端 以便读到 EOF。

### 10.3 `tool_runner`：工具卡片

```cpp
tool_runner_option opt;
opt.card.title = "bash";
opt.max_output_lines = 200;    // 环形缓冲上限
opt.on_exit = [](int code) { /* ... */ };
auto runner = make_unique<tool_runner>(ctx, strand, opt);
runner->start("make -j8");     // 输出流式汇入卡片，状态/耗时自动更新
runner->interrupt();           // 先 ^C（PTY）再终止；宽限期后强杀
```

输出先进组件内部缓冲、每帧合并一次写入卡片，因此高频输出不会打爆渲染。

### 10.4 把终端让给子进程

```cpp
app.suspend();     // 退出交替屏幕/raw/鼠标/包裹粘贴，恢复终端
// ... 运行需要独占终端的命令（编辑器、交互式工具）...
app.resume();      // 重新接管并强制整屏重绘
```

---

## 11. 鼠标与剪贴板

- 列表/卡片的点击：`log_view::on_mouse` 按滚动偏移与实测高度反查消息；
- 编辑器：点击定位光标、拖动选择；`Ctrl+C` 复制选区、`Ctrl+X` 剪切、`Ctrl+V` 读取剪贴板；

读取分两条路，全屏应用应当用第一条：

```cpp
sys_console::instance().request_clipboard();  // 只发 OSC 52 查询，不阻塞
```

```cpp
const string text = sys_console::instance().read_clipboard(200);  // 同步读，用于非全屏工具
```

> ⚠️ OSC 52 读取需要终端支持，多数终端（含 Windows Terminal、iTerm2 默认配置、大部分 tmux 版本）出于安全考虑关闭了查询；
> 此时应答为 `ESC ] 52 ; c ; ? BEL`，解析器不会产出事件，`read_clipboard()` 返回空串。
> 粘贴的**主路径仍然是终端的括号粘贴事件**（`Ctrl+Shift+V` / 鼠标右键），不需要任何查询。

---

## 12. 限制

- 管道模式没有终端语义：子进程 `isatty()` 为假时颜色与进度条可能不出现，`\r` 不会被转换；需要这些行为时请用 PTY。
- 沙箱/容器可能没有挂载 `devpts`，用 `pty::is_available()` 提前判断。
- 剪贴板读取依赖终端的 OSC 52 查询支持，终端拒绝时静默返回空；括号粘贴不依赖该能力。

---

## 13. 性能建议

1. **长列表用 `virtual_list`**，别把上千条消息塞进 `vbox`；
2. **Markdown 用 `markdown_cache`**，流式中的消息跳过缓存；
3. **流式写入批量提交**（`set_quiet + notify` 或每帧一次 `commit`）；
4. **元素树尽量稳定**：同一位置复用相同 `kind` 的元素，避免整棵子树重建；
5. 大块文本考虑 `code_block` 的 `collapse_after` 折叠。
