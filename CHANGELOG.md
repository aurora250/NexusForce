# CHANGELOG

## [unreleased]

### 🚀 New Features

- `process` 新增 `snapshot()` 与 `process::snapshot_entry`：一次枚举全系统进程，给出父进程、状态、线程数、RSS、CPU 时间与启动时刻；Windows 走 Toolhelp 快照，Linux 读 `/proc`
- `process` 新增 `get_time_info()` / 实例 `get_time_info()` / `current_time_info()`：用户态、内核态 CPU 时间与墙上时间统一为毫秒
- `process` 新增 `get_command_line()` / `current_command_line()`、`get_executable_path()`、`current_name()`、`current_state()`
- `process` 新增进程控制原语：`send_signal()`（Linux 走 pidfd，避免 PID 复用竞态；Windows 映射 terminate/kill）、`signal_name()`、`get_priority()` / `set_priority()` / `set_nice()`、`get_affinity()` / `set_affinity()`
- `process` 新增 `io_info` / `get_io_info()` / `current_io_info()`、`get_user()`、`threads()`、`socket_owners()`；`snapshot_entry` 补充 `user`、`nice`、`kernel_thread` 与 `io` 字段
- `sysinfo` 新增 `cpu_usage_per_core()`、`cpu_usage_breakdown()`、`disk_io_rates()`、`battery_info` / `battery_status()`
- `sysinfo::network_interface` 新增 `operstate`、`mtu`、`speed_bps` 与收发字节/包/错误/丢包计数
- `sysinfo::battery_info` 新增 `rate_mw`、`capacity_mwh`、`full_capacity_mwh`；`snapshot_entry` 在 Windows 上填充 `virtual_bytes`
- `sysinfo` 新增 `mount_points()`；Windows 挂载点以卷根目录实现，Linux 读 `/proc/mounts`
- `sysinfo::disk_info` 新增 `filesystem` 与 `available_bytes`，区分“空闲（含保留块）”与“非特权用户可用”
- `sysinfo::network_interface` 新增 `index`、`prefix_length`、`is_loopback`：Windows 补子网掩码、MAC 与 IPv6 前缀，Linux 补 MAC 与接口索引且每个接口只保留一条记录
- `sysinfo` 新增 `sysinfo::simd_instruction` 与 `sysinfo::simd_info`，并作为 `cpu_info::simd` 随实例提供：运行时探测 SSE/AVX/AVX-512 与 NEON/SVE，AVX 系列同时校验操作系统 XSAVE 状态
- 添加 `tui::chart` 通用图表：`chart_spec` 支持柱状图、折线图、面积图、饼图、环形图、散点图、直方图、热力图与雷达图，含标题、类目标签、多系列、轴/网格/图例选项、揭示进度与初始选中索引
- 添加交互式 `tui::chart_component`：方向键/Home/End 移动选中点，鼠标按下按实际几何命中，`selected_point()` / `selected_points()` 暴露标签与数值，`animate_progress()` 在帧回调中推进进度
- 添加单元格级画布原语 `tui::canvas_painter` 与 `element::canvas(function<void(canvas_painter&)>)` 重载：局部坐标、裁剪、宽字符、文本/填充/线段与主题访问，画布元素同时回写布局矩形
- `path` 新增 `is_absolute`、`canonical`、`weakly_canonical`、`is_within`、`components` 与 `operator+=`（字符串/字符），便于安全路径判断与路径拼接
- `filesystem` 新增 `resize`，用于截断/扩展文件
- `process` 新增 `set_work_dir(const path&)` 与 `start(const path&, ...)` 重载
- `tui::text_area_view` / `tui::composer` 新增 `set_history_limit(size_t)`，可在运行时调整并裁剪提交历史
- 新增 `NeForce/compress/zstd_compress.hpp`：静态压缩/解压、`zstd_exception`、连续帧前缀恢复
- `composer` 支持圆角/单线/双线面板边框（`composer_option::style.border`）
- `tui::style` 新增 `show_horizontal_scrollbar`，允许静态内容关闭水平滚动条
- 新增 `thread_pool::cancel_task()` / `task_info::cancelled()`：跳过排队任务并协作取消执行中任务；不强制终止线程
- `tui::theme` 新增角色表与成员函数，旧版九字段仍保留并从角色派生
- `code_block_option` 新增 `syntax`；`markdown_option` 新增 `math_style` / `parse_math`；`text_line` 新增 `continuation`
- `renderer::render_glyph_run` / `render_rich_text` 追加可选的悬挂前缀参数
- 添加 Kitty 键盘协议与 modifyOtherKeys 解析：`ESC[code;mods[:event]u`、`ESC[27;mods;code~`，`key_event` 新增 `action`（按下/重复/释放）与 `base_cp`
- 添加终端焦点事件：`ESC[I` / `ESC[O` 解析为 `parsed_input::kind::focus`，`reconciler::dispatch_focus()` 遍历组件树回调 `component_base::on_focus()` 并暂停/恢复帧时钟
- 添加根组件按键预览 `component_base::preview_key()`：在焦点组件之前处理必须优先的全局键
- 添加终端画像与协议工具 `tui/terminal_protocol.hpp`：`detect_terminal_profile()`、OSC 11 背景查询/解析、通知（OSC 9/777/99/BEL）、光标形状（DECSCUSR）、焦点事件开关、Kitty 键盘开关
- 添加图像协议 `tui/image.hpp`：PNG 解码（8 位 RGB/RGBA、5 种过滤器、IDAT 拼接、zlib 解压）、Kitty/iTerm2/Sixel 编码与 `detect_image_protocol()`
- 添加 Mermaid 渲染 `core/file/mermaid/mermaid.hpp`：flowchart、sequenceDiagram、stateDiagram、classDiagram、erDiagram、gitGraph、pie、mindmap、timeline、gantt、journey、quadrantChart、requirementDiagram 渲染为框线字符图，并接入 Markdown 的 ` ```mermaid ` 围栏
- 选区支持平移 `screen_selection::translate()` 与 `reconciler::scroll_selection()`：应用滚动转录后选区跟随内容
- 添加 `reconciler::set_copy_on_select()`：拖选结束即写入剪贴板并保留高亮；`reconciler::selection_text()` 取选区文本
- 添加 `application::request_quit()`：组件内部（如 `/quit`）可正常退出事件循环并恢复终端
- 添加 TUI 主题角色系统 `tui::theme_role`：把界面颜色收敛为 81 个语义角色（品牌/焦点、文本、表面、状态、diff、工具点、语法、图表、chrome、子代理），`theme` 新增 `get()` / `set()` 与 `has_palette`
- 添加六套内置调色板 `tui::make_palette()`：雾蓝 `dark` / `light` / `dark-ansi`、樱 `pink-night` / `pink-day` / `pink-ansi`，颜色值逐条取自 dsh-TUI 的 `theme.ts`（MIT），并提供生成脚本 `scripts/gen_theme_palettes.py`
- 添加主题目录 `tui::theme_catalog`：用户主题 JSON（`{name, base, colors}`）加载与局部覆盖、`auto` 伪主题按终端背景明暗解析、同名主题覆盖、坏文档跳过
- 添加颜色工具 `tui::lighten` / `darken` / `parse_color`（`#rgb`、`#rrggbb`、`#rrggbbaa`、`rgb()`、`ansi256()`、`ansi:name`）
- 添加动效原语 `tui::frame_clock`（进程级共享帧时钟，`application` 每帧推进）、`shimmer_color` / `shimmer_sweep`（移动高亮扫光）、`reveal`（平滑揭示，`max(3, 积压/8)` 步长）、旋转动画帧集 `find_spinner` / `spinner_frame`
- 添加 Markdown 悬挂缩进：`element::with_hang()` + `layout_text_hang()` + `text_line::continuation`，引用块与列表的续行按前缀宽度收窄并对齐
- Markdown 表格改为列宽预算算法：最小不可断词宽度、理想宽度、按剩余空间贪心分配、单元格内换行、垂直居中、完整边框与表头居中加粗
- Markdown 链接与图片输出 OSC-8 超链接；图片渲染为 `[img] alt`
- Markdown 支持 `$…$` 与 `$$…$$` 数学公式：块级公式竖排、行内公式压成单行、无法排版时回退原文；`markdown_option::parse_math` 可关闭
- 添加流式 Markdown 渲染器 `tui::markdown_stream`：按围栏代码块之外的空行封口已解析前缀，仅重解析尾部，块间距稳定不闪烁
- 添加表驱动语法高亮 `tui::highlighter`：20 种语言、10 个记号分类、跨行块注释与多行字符串状态；`code_block` 按语言自动着色
- 添加 LaTeX 渲染器 `tui::render_latex()`：TeX 子集（约 220 个符号、分数/根式/上下标/大运算符上下限/矩阵/cases/aligned/重音/间距）排版为多行 Unicode 文本，失败返回 `none` 而不产出半个公式
- 添加 Markdown 渲染缓存与 `code_block` 语法配色 `syntax_palette`
- 显示宽度模型对齐 Unicode 15：东亚宽度 W/F 与 Emoji_Presentation 表按 UCD 重新生成，组合符与变体选择符按零宽处理；`text_display_width` 增加 4096 条哈希缓存
- 添加物化与顺序类惰性算子：`distinct` / `distinct_by` / `order_by` / `order_by_descending` / `then_by` / `then_by_descending` / `group_by` / `chunk` / `sliding` / `zip` / `enumerate` / `scan`，链式 API 与 `views` 管道适配器
- 添加终端算子 `to_map`、与 `to_lookup`
- 新增视图组件：`distinct_view`、`order_by_view`、`group_by_view` 与 `grouping`、`chunk_view`、`sliding_view`、`zip_view`、`enumerate_view`、`scan_view`
- `order_by(...).then_by(...)` 采用复合键 + 逐级比较实现多级排序：主键始终优先、主键相等时按次级键，并保留前一级的升降序方向
- 添加链式查询组件 `query`：`ranges::from` 得到惰性查询链
- 添加链式查询 CRTP 接口 `iqueryable`：派生类型只需提供 `value_type` / `base_view_t` / `view()` / `make()` 即可获得全部链式算子
- 添加 `thread_pool::post_task()`：投递不关心结果的任务，不分配 `task_info`、不创建 future/promise，单次投递开销约为 `submit_task()` 的三分之一（实测 447 ns/任务 vs 1325 ns/任务）
- 添加 `thread_pool::set_task_tracking()` / `task_tracking()`：关闭后提交路径不再为每个任务分配 `task_info`（`submit_result::task_info` 返回共享占位对象）
- 添加 `thread_pool::set_warmup_window()` / `warmup_window()`：定时任务到期前唤醒一个工作线程并保持自旋，把 `submit_after` 的冷唤醒变为热派发
- 添加 `timer_scheduler::set_spin_window()` / `spin_window()`：到期前自旋守时窗口，消除条件变量唤醒的调度延迟
- 添加 `thread_pool::set_cpu_affinity()` / `cpu_affinity()`：工作线程可绑定到进程允许 CPU 集合内的不同核心，默认关闭
- 添加 `launder` 编译器优化阻止屏障函数
- 添加 `likely` / `unlikely`
- 添加 CMake 多架构 SIMD 检测配置
- 添加 PCLMULQDQ 指令集检测宏 `NEFORCE_SIMD_PCLMUL`
- 添加 Sanitizer 构建配置项 `NEXUSFORCE_ENABLE_ASAN` / `NEXUSFORCE_ENABLE_UBSAN` / `NEXUSFORCE_ENABLE_TSAN`
- 添加 Sanitizer CI 工作流 `.github/workflows/sanitizer.yml`
- 添加 `sysinfo::parse_brand_frequency()` 从 CPU 型号字符串解析标称频率
- 添加 PCG 随机数生成器 `random_pcg32`（XSH-RR 64/32，与 PCG 参考实现逐位一致）与 `random_pcg64`（XSL-RR 128/64，与 NumPy PCG64 输出一致），均支持 `(seed, stream)` 双参数播种以派生相互独立的随机序列
- 添加异或移位旋转随机数生成器 `random_xoroshiro128`（2×64 位状态）与 `random_xoroshiro256`（4×64 位状态），模板参数 `xoroshiro_scramble` 可选 `+` / `**` / `++` 三种输出混淆方式
- 添加位生成器适配器 `bit_gen`：在 1-64 位任意位宽上生成无偏均匀随机数，位宽等于引擎字宽时零开销直通
- 所有随机数引擎补齐标准库 UniformRandomBitGenerator 概念接口（`result_type` / `min()` / `max()` / `operator()` / `discard()`），可直接配合标准库与第三方分布函数使用
- 新增随机数分布函数 `uniform_int` / `uniform_real` / `bernoulli` / `normal` / `exponential` / `log_uniform` / `poisson`，全部为无状态自由函数，同一引擎与种子下结果完全可复现
- 随机数组件拆分为 `random/` 子目录（engine / lcg / mt / pcg / xoroshiro / bit_gen / secret / distribution），`random.hpp` 保留为聚合头
- 添加 `random_seed()` 默认种子生成与 `splitmix64()` 种子扩展函数
- 数学库新增 `exponential_e()`（实数指数 e^x）、`logarithm_1p()`（ln(1+x)）、`logarithm_factorial()`（ln(n!)）、`power_of_two()`（2 的整数次幂）与 `normalize_power_of_two()`（按 2 的整数次幂归一化）
- 新增追加式格式化接口 `format_to(string&, fmt, args...)`：与 `format()` 语义一致但不新建字符串，供日志等高频路径复用目标缓冲
- 添加高性能内存池组件 `memory_pool`：小对象由零块头 span 承担，尺寸类与归属通过进程级 64 KiB 槽位地址映射表查询，大对象直接映射操作系统内存并记录区域头部
- 添加分配器 `pool_allocator<T>`，可直接用于容器
- 添加进程级全局覆盖选项 `NEXUSFORCE_MEMORY_POOL_GLOBAL_OVERRIDE`：提供显式的 `NexusForceMemoryPoolOverride`（OBJECT）与 `NexusForceMemoryPoolOverrideStatic`（STATIC）目标，需要整个进程统一分配器的程序由自己把目标链接进每个模块
- 添加内置区域设置后端 `src/core/system/locale_fallback.cpp`：字符分类与大小写转换使用自研 Unicode 表，数字、货币、日期时间格式化与区域名称查询使用自研精简区域数据
- 添加内置后端的区域数据组件 `src/core/system/locale_data.{hpp,cpp}`：数字与货币风格、日期名称与模式、语言与地区名称、本币、可用区域与 ISO 3166 列表
- 添加 Unicode 字符表生成脚本 `scripts/generate_locale_tables.py` 与生成结果 `src/core/system/locale_tables.{hpp,cpp}`：字符分类、大小写映射与规范分解表，按 Unicode 字符数据库生成，分类规则与 ICU4C 一致
- 添加 TUI 输入解析状态机 `tui::input_parser` + `tui::parsed_input`：识别 ASCII/UTF-8 可打印字符、控制字符、CSI 与 SS3 序列、带修饰键的 CSI（`ESC[1;5C` 等）、Alt+字符前缀、SGR 鼠标序列与括号粘贴
- 添加粘贴事件链路 `tui::paste_event` / `component_base::on_paste()` / `event_dispatcher::dispatch_paste()` / `reconciler::dispatch_paste()`：一次粘贴整体送达，行分隔符归一化为 `\n`，不再把多行粘贴中的换行当作回车提交
- 添加 `sys_console::enable_bracketed_paste()` / `is_bracketed_paste_enabled()`（`ESC[?2004h/l`），`tui::application` 在进入与退出时自动开关括号粘贴模式
- 添加 `focus_manager::validate_focus()` 与 `component_base::structure_dirty()` / `clear_structure_dirty()`：组件动态增删后焦点链自动失效重建，焦点组件被移除时不再留下悬空指针
- `screen::to_string()` 现在依据 `cursor()` 输出真实终端光标位置与形状（`DECSCUSR`，块/下划线/竖线 × 闪烁），光标形状为 `hidden` 时保持隐藏
- `element::flexbox()` 尊重 `box_props::dir`，可以构造列方向的弹性容器
- `paragraph()` 系列按显示宽度换行，超长单词按字符断行，且 `max_width == 0` 时仍按显式换行符分行
- 添加 TUI 显式换行支持：`element::text` / 文本输入框中的 `\n` 现在作为硬换行渲染，行高按换行与自动换行合并计算
- 添加富文本元素 `element::rich_text()` + `text_span`：单行内可混排颜色/粗体/斜体/下划线等样式，换行与裁剪规则与普通文本完全一致（标题、diff 着色、行内代码由此解锁）
- 添加文本排版组件 `dom/text_layout.hpp`：`layout_text()` / `text_display_width()` / `caret_position()`，渲染器、布局测量与编辑器光标定位共用同一份分行实现
- 添加虚拟列表元素 `element::virtual_list()` 与组件 `components::virtual_list_view`：只对可视窗口内的项做布局与渲染，支持贴底自动跟随、滚轮/拖拽/键盘滚动与可视高度回写
- 添加面板元素 `element::panel()`：自身占用布局矩形、可绘制边框并把子元素排布在内边距内，用于对话框与卡片
- 添加多行编辑器组件 `components::text_area`：软换行编辑、选区与 `Shift` 扩展、按词移动、`Ctrl+A` 全选、撤销重做、`Ctrl+P/N` 历史、粘贴整体插入、光标随动滚动
- 添加可选择列表组件 `components::select_list`：大小写不敏感过滤、上下/翻页/首尾移动、回车确认与 `Esc` 逐级取消，用于命令面板与候选补全
- 添加确认对话框组件 `components::approve_prompt`：标题 + 说明 + 允许/拒绝/总是允许选项，支持方向键、数字键与 `Esc` 取消，结果可写回 `state<int>`
- 添加状态指示组件 `components::status_indicator`：由 `on_animation()` 驱动的旋转动画 + 文案 + 已耗时，无需外部每帧喂帧号
- 添加流式文本组件 `components::streaming_text`：`append()` 只写内部缓冲，每帧合并提交一次，避免逐 token 触发逐 token 重渲染
- 添加 Markdown 渲染器 `dom/markdown.hpp`：标题、段落、无序/有序列表、引用、分隔线、围栏代码块与行内 `**粗体**` / `*斜体*` / `` `代码` `` / `~~删除线~~`
- 添加代码块渲染器 `dom/code_block.hpp`：行号栏 + 注释/字符串/数字/关键字着色
- 添加差异渲染器 `dom/diff.hpp`：`parse_unified_diff()` 解析 unified diff，`diff_view()` 用双行号栏与 `+`/`-` 标记着色渲染
- 添加无终端驱动 `tui::headless`：固定终端尺寸、帧输出回收到内存、输入按字节注入并支持排队更新泵送，作为快照测试与离屏渲染底座
- 添加终端颜色深度支持 `color_depth`：`screen` 可输出 16 色 / 256 色 / 24 位真彩色，`reconciler` 依据 `sys_console` 能力自动选择，并可显式覆盖
- 添加 `sys_console::supports_256_color()` 与预置亮色主题 `light_theme`
- 添加同步输出（`ESC[?2026h`/`ESC[?2026l`）包裹每帧，支持该扩展的终端会原子提交整帧
- 添加 `application::suspend()` / `resume()`：让出终端给交互式子进程（`$EDITOR`、需要 TTY 的命令）后收回并强制整屏重绘
- 添加 `reconciler::invalidate()`、`frame()`、`frame_layout()`、`frame_tree()`、`set_output_sink()`、`set_term_size()`、`set_color_depth()`、`set_synchronized_output()`：离屏渲染与快照测试所需的观测与控制接口
- 添加 `surface::fast_cell_at()` 的只读重载
- 添加 `md_find_reference()` / `md_plain_text()` / `md_empty()` 文档查询接口
- Markdown 解析器与构建器的分配优化：表格的表头/对齐/数据行按实际数量一次性预留容量、列表解析复用跨项的行暂存缓冲、构建器的文本转义改为**按片段复制**并正确区分行首标记
- 添加对话记录组件 `components::log_view`：变高消息、流式合并追加、贴底自动跟随、上滚期间"N 条新消息"提示、定稿消息走 Markdown 渲染缓存
- 添加输入编排组件 `components::composer`：多行编辑器 + 补全弹层 + 状态提示行，`Enter` 提交、`Shift+Enter` 换行、`Ctrl+Enter` 提交，触发符（默认 `/`）自动唤起补全，`Tab`/`Enter` 接受、`Esc` 关闭，运行中 `Esc` 触发中断
- 添加工具调用卡片 `components::tool_card`：工具名 + 参数摘要 + 状态图标 + 耗时 + 可折叠输出
- 添加可折叠输出块 `components::output_block`：头部一行 + 折叠提示，`Enter`/`Space`/`→` 展开、`←` 折叠，点击切换
- 添加快捷键表 `tui::keymap`：`global`/`overlay`/`focus` 三层绑定，修饰键可要求也可禁止，同层后注册者优先，`hints()` 可复用为状态行说明
- 添加全局快捷键接入：`reconciler::shortcuts()` 在事件抵达组件树之前处理全局层，`application` 默认绑定 `Ctrl+C` 与 `Ctrl+L`
- 添加背景色带元素 `element::band()`：把整个矩形刷成背景色并在内边距内排布子元素，色带内文本自动继承色带底色
- 添加富文本片段超链接 `text_span::link`：渲染为 OSC-8 超链接
- 虚拟列表支持**变高项**：`element::virtual_list()` 新增高度表重载、`components::virtual_list_view` 新增 `variable_height`/`invalidate_heights()`/`measured_height()`，只对可视项做实测并回写，宽度变化自动失效重测
- `code_block()` 新增整块背景带、左侧竖线与 `collapse_after` 折叠
- `diff_view()` 新增按文件分组、hunk 折叠与背景带，`diff_view_option` 增加 `added_style`/`removed_style`/`header_style`/`band_style`
- 添加伪终端组件 `pty`：`open()` 申请主从端、`start()` 以会话首进程启动子进程并把它接到从端
- 添加异步子进程监视器 `process_watcher`：`start()` / `start_shell()` 走 PTY 或管道，输出增量回调、退出回调，伪终端不可用时自动回退到管道
- 添加流式 HTTP 接口 `http_client::request_stream()` / `request_stream_async()`：正文不缓冲，每读到一段就回调 `http_stream_observer::on_data`
- 添加流式请求取消句柄 `http_stream_handle`：`abort()` 可跨线程调用，对底层 socket 执行 `shutdown` 立即唤醒阻塞读取；`on_data` 返回 `false` 等价于 `abort()`
- 添加重试策略 `http_retry_policy`（`config::retry`）：可重试状态码（408/429/500/502/503/504）与传输层失败按倍数退避重试，优先采用响应的 `Retry-After`（秒）作为退避时长，默认不重试
- `http_client::config` 新增 `keep_alive_idle_timeout` 与 `stream_idle_timeout`：空闲超限的连接主动重连，流式读取期间使用独立的接收超时
- 添加屏幕选区 `tui::screen_selection` 与 `paint_selection()` / `selection_text()`：以帧缓冲单元格为坐标跨组件选取，行内尾随空白裁剪、宽字符续列跳过
- `reconciler` 接入鼠标拖选：左键按下拖动即开始选区，`Ctrl+C` 复制并清空选区，其余按键、滚轮、窗口变化都会丢弃选区；`set_clipboard_writer()` 可接管复制目标
- `components::composer` 提交时记录历史，单行编辑下 `Up`/`Down` 浏览提交历史（多行时仍由编辑器移动光标）
- `text_area_view` 新增 `history_previous()` / `history_next()` / `in_history()`：把历史浏览开放给上层组件，方向键可映射到历史

### 🔧 Improvements

- `process::check_permission()` Linux 语义与 Windows 对齐：`query_info` 只需进程存在，读/写/终止要求属主或 root，`execute` 恒为 false，`permission::all` 不再因 `/proc/<pid>` 的目录权限而误判
- `process::get_state()` Linux 把被调试器跟踪的 `t` 状态映射为 `state::stopped`，该枚举值不再恒不可达
- `process::get_memory_info()` Windows 补 `virtual_size`；`get_name()` 与快照在 Windows 改用宽字符 API，非 ASCII 路径不再乱码
- `sysinfo::process_count()` Windows 按需扩容进程缓冲区，不再在 1024 个进程处截断
- `sysinfo::network_interfaces()` Linux 每个接口只保留一条记录，Windows 补子网掩码与 MAC
- `process` 内部读取 `/proc` 改用 `neforce::file`，不再直接使用 `fopen`/`fread`
- `process` Linux 读取自身 `/proc/<pid>/stat` 遇到全零计数器时回退到 `/proc/thread-self/stat`，模拟环境下内存、线程与时间查询不再恒为 0
- `tui::chart` 饼图/环形图改为 2x2 子单元象限抗锯齿，边沿支持双 slice 前背景色
- `band` 在未显式设置 `bg` 时保持透明，不再默认填充 `theme.bg`
- `code_block` 支持继承 `code_style.bg`，不再固定 `#1a1a1e`
- `channel<T>` 数据面改用 `lock_free_queue`（MPMC），无缓冲模式保留同步握手；容量边界仍由条件变量等待
- `http_client` 默认开启连接复用：连续请求不再重复握手，响应带 `Connection: close` 时仍会主动断开
- `http_client` 的 `HEAD`、`204`、`304` 与 1xx 响应不再尝试读取正文，避免复用连接被后续请求阻塞
- 重定向跟随会带上目标 scheme：`http -> https` 跳转不再被当作明文 HTTP 重放
- `text_area_view` 在历史浏览中越过最新一条时清空输入，与 `Ctrl+N` 的既有语义保持一致
- Windows 动态库导出表缩小，加载更快、导入库更小，模板实例与 STL/COMDAT 内部符号不再越界导出
- `process_watcher` 在 Windows 上改由轮询定时器拉取子进程输出，POSIX 仍走 fd 就绪回调
- `pty::wait()` 在无超时直接阻塞于 `waitpid()` 而非 5ms 轮询；退出码解码与 `process::wait()` 统一
- 异步日志的批量写入改为单个 drain 任务循环排空队列：不再每批提交一个新任务，批次的到达顺序稳定，线程池任务数也随之减少
- 异步日志队列满且策略为 `block` 时改用 `notify_all()` 唤醒生产者，多个生产者不再只被唤醒一个
- 新建的子 Logger 立即继承父 Logger 当前生效的级别；此前固定为 INFO，要等父级再次 `set_level()` 才生效
- `locale::monetary()` 报告地区本币而非固定 USD，并补齐 `mon_grouping`、`positive_sign`、`negative_sign`
- `locale::numeric()` 补齐 `exponential`、`nan_symbol`、`infinity_symbol` 与 `grouping`
- 内置后端的一级比较忽略大小写与重音、二级区分重音、三级区分大小写；排序键按级编码，键的字节序与 `compare()` 的结果一致
- 内置后端按地区选择英文日期格式，并区分 POSIX 区域）
- 添加 `thread_pool::wait_for()` / `wait_for_all()`：等待任务结果期间代为出队、窃取并执行池内任务，父任务等待子任务时不再让工作线程空转，嵌套等待扇出随线程数正常扩展
- `standard_allocator` 的分配与释放改由进程级内存池承担，库容器在库与消费者两侧共用同一个导出池单例，跨模块释放天然安全，不再依赖全局 `operator new` 是否被替换
- 共享库不再替换全局 `operator new` / `delete`：第三方 C++ DLL 与消费者不再被隐式接管
- `memory_pool` 新增 `foreign_release_count()` 与 `statistics::foreign_releases`：对外来指针的释放做无条件计数，发布版下的分配器边界失配由静默变为可观测
- `thread_pool` 存活巡检改为按需布防：仅在存在待处理任务或已发布的本地队列深度时向定时器注册巡检项，命中则唤醒全部停驻线程并在仍有工作时续期，空闲池不再持有任何巡检定时项
- `thread_pool` 本地队列可见性补齐：本地提交路径同样布防存活巡检，巡检把各工作线程已发布的本地队列深度并入"仍有工作"的判据，工作线程等待子任务时其本地积压也能被巡检唤醒的兄弟线程窃取
- `thread_pool` 全局队列批量出队：出队后发现积压深于 16 轮全部工作线程时，一次取出至多 8 个任务，首个立即执行、其余落入本地队列按序消费，批量投递场景下共享队列往返次数由每任务一次降为每批一次；阈值刻意设高，长任务积压时不做批量，避免八个长任务集中到一个工作线程而在任务尾部留下空转
- `timer_scheduler` 在到期前 `spin_window` 窗口内提前唤醒并自旋守时，`submit_after` 的平均绝对偏差由约 130 µs 降至 5–36 µs
- `thread_pool` 定时任务预热：`submit_after` 在到期前唤醒一个工作线程保持热态，消除定时任务因工作线程已停驻而产生的约 75 µs 冷唤醒延迟
- `thread_pool` 缓存模式伸缩线程按积压比例一次扩充多个线程，突发负载下更快逼近目标并发度
- `lock_free_queue` 隐式生产者哈希表的墓碑槽位可被复用，反复创建短生命周期生产者线程不再导致哈希表无谓翻倍扩容
- `thread_pool` 提交路径重构：全局队列直接存放任务对象，出队改用非分配的 `try_dequeue`，生产者侧每任务少一次堆分配
- `thread_pool` 工作线程停驻改为等待单调递增的唤醒字，取代 1ms 条件变量轮询与 256 轮 `sched_yield` 阶梯：16 线程空闲 CPU 占用由 11.8% 单核降至 0.05%
- `thread_pool` 自旋预算自适应：取到任务则倍增，确无任务可做时减半并停驻
- `thread_pool` 唤醒策略按积压量决定：已有空闲线程在轮询时提交完全不产生唤醒系统调用，积压超过空闲线程数时一次 `notify_all` 唤醒整组停驻线程
- `thread_pool` 每任务统计改为工作线程本地计数，`statistics()` 时归并，热路径不再有 `total_completed` / `idle_thread_size` 全局原子写
- `thread_pool` 工作窃取扫描不再持全局互斥量：上下文地址在池生命周期内稳定，窃取者先校验 `attached` 标记，摘除线程时等待窃取者离场
- `thread_pool` 窃取目标选择改用每个工作线程发布的队列深度，避免扫描时触碰非候选者的缓存行
- `thread_pool` 缓存模式线程伸缩移出提交热路径，改由独立伸缩线程按积压与空闲数带滞回增减，`start(n)` 作为硬下限
- `thread_pool::start()` 以就绪屏障等待全部工作线程注册完成，取代固定 5ms 睡眠
- 随机数引擎的统一取值接口抽取到公共基类 `random_engine`（CRTP），各引擎只需实现原始随机数产生与字宽描述，消除 8 份重复实现
- `random_lcd` 的 64 位取值由两次 31 位输出拼接改为跨字拼接的真实 64 位均匀随机数，`next_float<double>()` 不再被限制在 [0, 0.25)
- `secret` 新增实例化 `operator()` 与 `min()` / `max()`，可作为满足 UniformRandomBitGenerator 概念的引擎传给分布函数
- `logarithm_e()` 改为倒数归约 + 2 的整数次幂阶梯归约 + 反双曲正切级数（14 项定长、系数预计算），消除逐项除法与随数值规模增长的归约循环
- `square_root()` 改为阶梯归一化 + 线性初值牛顿迭代，迭代次数由与数值大小相关的数十次降至 5 次以内
- `tcp_client::connect()` 的域名解析改为受 `connect_timeout_` 总预算约束（A/AAAA 查询共享剩余时间切片并禁用 UDP 重试），避免解析失败时按 dns 客户端完整超时预算阻塞数十秒
- `dns_client` 新增 `timeout()` / `max_udp_retries()` 读取接口，便于调用方保存与恢复查询预算配置
- `basic_string` 单字符追加内联化，绕过 traits 层 SIMD 调度，性能提升 2-4 倍
- `basic_string` 拷贝构造直连 `memory_copy` 与内联终止符写入
- `memory_find` / `memory_set` 添加 AVX2 256-bit 宽寄存器路径，单字符查找与填充构造性能大幅提升
- `memory_copy` / `memory_set` 对小于 16 字节的数据直接内联标量操作，跳过 SIMD 层级判断
- `flat_hashtable` 查询接口补齐 SIMD 组探测
- `flat_hashtable` 迭代器缓存当前元数据组的占用位掩码，组内递增只需一次位运算；配合元数据尾部哨兵字节，越界组加载不再需要边界分支
- `flat_hashtable` 插入路径不再重复计算哈希，并拆分为内联热路径与 `NEFORCE_NOINLINE` 冷扩容路径
- `flat_hashtable` 元数据数组改由重绑定后的容器分配器分配，此前绕过分配器直接使用 `::operator new`，自定义分配器无法观测到这部分内存
- `flat_hashtable::next_power_of_2` 由循环移位改为 `bit_ceil` 位运算
- `hashtable` 桶增长策略由约 1.5 倍对齐到约 2 倍，无预留插入的 rehash 次数由 O(log₁.₅ n) 降至 O(log₂ n)
- `hashtable::copy_from` 的桶数组重建由 `clear` + `reserve` + `insert` 三趟改为单趟 `assign`
- AES-256 GCM 模式 GHASH 采用 PCLMULQDQ 无进位乘法替代逐位乘法
- AES-256 解密预计算 InvMixColumns 逆轮密钥，消除每块每轮的 `aesimc` 重复变换
- AES-256 ECB / CBC 解密 / GCM-CTR 采用 4 块交错加密，隐藏 `aesenc` 指令延迟
- AES-256 各模式消除逐块栈拷贝与逐字节写回，改为原地处理与 SIMD 批量异或
- AES-256 标量回退路径改用恒定时间 S-box（GF(2^8) 指数求逆），消除缓存时序侧信道
- Base64 编解码 SSSE3 路径优化：6-bit 索引分段查表、输出组序修正、尾部走标量循环、解码校验恢复
- 字符串转义 `escape()` 添加 SSE2 扫描路径，无特殊字符片段直接批量追加
- JSON 解析器 `skip_space` / `parse_string` 升级 AVX2 256-bit 宽寄存器扫描路径
- TOML 解析器空白/注释跳过、四种字符串扫描 SIMD 化
- YAML 解析器空白/缩进/注释跳过、双引号/单引号字符串、纯量/键名/块标量行扫描 SIMD 化
- `uuid::to_string()` 以单次预分配与十六进制表查找替代 format 引擎调用
- `dns_client` 添加 UDP 超时自动重试：换新随机查询 ID 重发，可通过 set_max_udp_retries() 配置重试次数，总超时预算为单轮超时 ×（重试次数 + 1）
- `dns_client` 添加 0x20 随机大小写编码与响应校验
- `dns_client` 添加共享 UDP socket 源端口定期轮换，缩小 DNS 欺骗攻击窗口
- `dns_client` 缓存遵循记录自身 TTL
- `dns_client` TCP 截断回退与强制 TCP 模式改为异步状态机，不再阻塞事件循环线程
- `dns_client` 缓存命中回调改为经 io_context 异步投递，与 Asio 完成令牌惯例一致
- `dns_client` 内 `build_query` / `parse_response` 添加可选 0x20 大小写模式参数
- `lock_free_queue` 新增显式生产者/消费者令牌、批量入队/出队、无分配接口、内存统计 get_mem_stats，BSD/Boost 许可证署名
- `ssl_socket` 新增 `prepare_server_ssl()` / `prepare_client_ssl()` / `async_handshake()`，
- `ssl_socket` 的 `init_server_ssl()` / `init_client_ssl()` 重构为基于 prepare + 阻塞握手
- `ssl_acceptor` 新增 `async_accept()` 异步接受 TCP 连接并完成 TLS 握手后交付 `ssl_socket`
- `tcp_client` / `ssl_client` 新增 `async_connect()` / `async_read()` / `async_write()`
- `ssl_client` 异步连接建立后自动执行异步 TLS 握手
- `smtp_socket` 新增 `async_connect()` 异步完成 TCP 连接、可选 TLS 握手、220 问候读取与 EHLO 协商
- `icmp_socket` 新增 `async_ping()`，基于定时器轮询驱动，不依赖平台 fd 事件注册
- `http_client::request_async()` 新增 use_awaitable 重载
- `io_context::run_one(timeout)` 无待处理工作时立即返回 0 而不再空等整个超时，有工作则等待到超时或某个 handler 就绪，被唤醒但暂无可执行 handler 时按剩余预算重试
- `file` 读写缓冲改为按需分配，`open()` 不再预分配两块缓冲，并修正缓冲下限，小文件不再把缓冲压缩到文件大小、空文件不再降到 2KB，追加型文件后续增长也能获得 32KB/64KB 档位
- `file::read()` 增加大请求直通，读缓冲已耗尽且请求大于 4 倍缓冲时直接系统调用读取，跳过逐块缓冲拷贝，与写侧直通策略对称
- `file` 的 `mapper()` / `locker()` / `info()` / `async()` 改为按值返回绑定当前句柄的子对象，移除内部子对象与其生命周期管理
- `filesystem::copy()` Linux 侧改用 `copy_file_range(2)` 内核态复制，不支持时回退 256KB 缓冲读写循环，大文件复制的系统调用次数大幅下降
- Windows 目录遍历改为全程宽字符：`remove_all_in_directory()` / `copy_directory()` 改为宽字符内部递归实现，`path_tree::scan_impl()` 递归保持宽路径
- Linux 目录遍历优先使用 `d_type`，仅 `DT_UNKNOWN` 回退 `lstat`，大目录遍历每条目减少一次 stat 系统调用
- `filesystem::copy()` Windows 侧补充最后访问/修改时间保留，与 Linux 侧 `fchmod` / `futimens` 行为对齐
- `file_async` 在无 io_uring 的 Linux 上阻塞 `pread` / `pwrite` 下沉到工作线程并以 io_context 投递完成，不再占用事件循环线程
- `file_async` io_uring 路径显式解析不再依赖内核对 `UINT64_MAX` 偏移的处理
- `nexusforce_install_runtime_dependencies()` 的依赖搜索范围扩展到目标所链接的每个导入目标自身的目录与 vcpkg 已安装目录，并对解析器未解析出的依赖按文件名再做一次大小写不敏感查找：只靠构建输出目录解析在 app-local deps 未复制全部端口时会漏依赖
- UTF 转换批量化：`character` / `wcharacter` / `u8character` / `u16character` / `u32character` 的 21 处转换循环内聚到 `codepoint` 批量接口，消除逐码点跨 TU 调用与逐字节 `push_back`，改为分块解码 + 按目标类型整段扩容写入
- `codepoint` 新增批量转码接口：`decode_utf8` / `decode_utf16` / `decode_wchar` / `encode_utf32` 各提供 string / wstring / u16string / u32string（C++20 另有 u8string）目标重载，一次调用完成整段缓冲区转换
- UTF 解码新增 ASCII 快速通道：SSE2 16 字节全 ASCII 块一次加宽为 16 个码点，AVX2 下以 32 字节掩码判定并复用同一加宽内核；非 x64 架构回退到字级标量扫描
- UTF-8 解码新增同长度序列 SIMD 内核：16 字节内 8 个双字节序列（SSE2）、4 个四字节序列（SSE2）、12 字节内 4 个三字节序列（SSSE3，覆盖 CJK），超长编码与代理项等边界情况仍交由标量解码器处理以保证替换语义完全一致
- UTF-16 与 UTF-32 源新增 SIMD 批量路径：非代理项 UTF-16 码元块一次加宽为 8 个码点，合法 UTF-32 块一次校验并搬运 4 个码点
- `codepoint::display_width()` 的显示宽度表由两张 17408×64 位位图（约 272 KB 静态数据）改为 14 段排序区间表和二分查找，静态数据量下降约 99%，查询落在少量缓存行内
- `character::to_u16string()` 的预留长度由 `size * 2` 修正为 `size`（UTF-8 到 UTF-16 的码元数不超过输入字节数）
- `format()` 新增无选项快速通道：宽度、对齐、强制符号与备用前缀均未请求时直接移交渲染结果，省去一次完整拷贝与潜在分配，命中日志等高频 `{}` 场景
- `format_impl` 与 `format_named` 的字面量片段改为整段批量追加，替代逐字符 `push_back`，直接走 `basic_string` 的 SIMD 拷贝路径
- 浮点格式化重写为尾数精确整数换算：以 1280 位定点大整数完成尾数 × 10^s 的精确缩放与移位，再以十进制串做半值取偶舍入；定点与科学计数法在任意量级（含次正规数、DBL_MAX、1e±300）均为正确舍入，取代原先逐次乘除 10 的定标循环（最坏约 320 次）与 `fraction × 10^p + 0.5` 的非精确舍入
- `byte_size::to_string()` 移除 format 套 format（先用 `format(":.{}f")` 构造格式串再二次 `format`），改为直接调用 `to_string_fixed()`
- `memory_pool` 线程缓存改为按尺寸类成对存放（每类 `{空闲链头, 数量}`，恰好占一条缓存行），此前链头数组与计数数组相距约 230 字节，一次分配要触碰两条缓存行
- `memory_pool` 尺寸类查表改为 256 项表（覆盖 ≤ 256 字节的常见请求）加无分支位宽公式，取代原先最多三个比较分支；1 KiB 尺寸类的往返延迟由 glibc 的 1.13 倍降为 0.82 倍
- `memory_pool` 批量归还按 span 分组拼接：同一 span 的连续块只做一次空闲链拼接与一次计数更新，临界区长度与批量块数解耦，`512 × 64 B` 批量模式由 1.77 倍降至 1.12 倍
- `memory_pool` 空 span 保留预算调整为 2 MiB（每个尺寸类仍保留至少 1 个），混合尺寸 churn 后的常驻由 4.3 MB 降至 2.5 MB
- `memory_pool` 分配快路径内联到头部，线程缓存命中不再进入库内函数，线程缓存访问次数由两次降为一次
- `memory_pool` 线程缓存访问改用 initial-exec TLS 模型，消除每次分配经过的 `__tls_get_addr` 调用
- `memory_pool` 空 span 保留策略由每个尺寸类保留 1 个改为全池字节预算 + 每类下限 1 个，突发负载不再每轮重建 span、不再逐块踩新页，`128×4 KiB` 批量路径吞吐提升约 20 倍
- `memory_pool` refill 批量按线程缓存字节目标推导，小对象的加锁与链表搬运频率下降
- `memory_pool` 批量释放时同一 64 KiB 槽位只查询一次地址映射表
- `memory_pool` 新增每线程大对象区域缓存，大对象分配释放不再进入全池锁，线程退出时归还
- `memory_pool` 地址映射条目压缩为单个 64 位原子（基址槽位号 / 尺寸类 / 槽位类型 / 归属标识）：释放路径的依赖加载由 4 次降为 2 次，发布与回收由每次 2 次原子写降为 1 次，并且不再存在两次发布交错被读成混合判定的窗口
- `memory_pool` 大对象释放先比对归属再查注册表：命中本池时不再进行 256 槽线性扫描，大对象往返少一次无谓的原子读链
- `memory_pool` 线程区域缓存改为按尺寸分桶的 O(1) 命中，并把 `thread_region_bytes` 的判定由映射字节改为载荷字节：256 KiB 级请求（含区域头部与对齐开销后略超 256 KiB）不再落入全池区域锁
- `memory_pool` 新增 `thread_region_budget_bytes` 选项，给线程保留的大对象区域加映射字节总预算，默认 2 MiB
- `memory_pool` 批量归还改为两阶段：同 span 连续块的分组与地址映射表查询在类锁之外完成，锁内只做拼接；span 的 `mmap` 与 `munmap` 同样移出类锁，页错误与 TLB 失效不再发生在自旋锁内
- `memory_pool` 线程缓存溢出裁剪改为有界拆分，不再遍历半个空闲链；溢出阈值在绑定线程缓存时一次性复制，热路径不再访问中央堆的尺寸类状态
- `memory_pool` 释放路径新增最近交付块快捷判定：本线程最近交付且尚未归还的块无需查询地址映射表即可确定归属与尺寸类
- `memory_pool` 释放路径的最近交付块判定改为只登记指针：命中时从 span 头部读取尺寸类，登记成本降为一次存储
- `memory_pool` 线程缓存的释放热路径恢复为直行推送，缓存裁剪拆为冷路径函数，批量模式不再为裁剪分支付出热路径代价
- `memory_pool` 大对象区域缓存桶由 64 KiB 起改为 32 KiB 起逐桶翻倍，线程槽位由 4 增到 8，相邻尺寸的大对象不再互相挤占缓存槽
- TUI 终端尺寸变化后强制整屏重绘，避免缩放后残留旧内容
- TUI Tab 键先交给焦点组件处理，未被消费时才做焦点导航，使编辑器/补全弹层可以占用 Tab
- TUI 渲染器不再把控制字符写入 cell 网格，按钮标签按显示宽度居中
- `screen::register_hyperlink()` 对相同链接去重，句柄表到达 255 上限时返回 0 而不是继续入队造成错位
- TUI 渲染器的文本绘制统一改为基于 `text_layout` 的字形流实现，词换行、字符换行、显式换行与选区叠加共用同一套分行结果
- TUI 文本输入框支持无边框形态、纵向滚动（`with_scroll_y()`）与选区高亮（`with_selection()`），并回写自身布局矩形（`with_rect_state()`）
- `components::select_list` 新增 `set_visible_rows()`，输入区补全弹层按匹配数量收缩，不再固定占满可视行数

### 🐛 Bug Fixes

- 修复 Windows 下 `pty` 子进程未挂接到伪控制台导致读不到任何输出：`UpdateProcThreadAttribute` 的 `PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE` 需要传 `HPCON` 句柄值本身，原实现传的是变量地址（`&pseudo`），内核据此拿到的句柄无效，子进程虽能执行并返回正确退出码，但输出不进入伪控制台管道；现改为直接传 `pseudo`
- 修复 Windows 下 `sysinfo::cpu_usage_breakdown()` 依赖 PDH 格式化百分比，短采样间隔下各项之和明显偏离 100（实测低至 89）：改为按原始计数（100ns 累计时间）增量计算，任意间隔下 user/system/idle/irq/softirq 之和恒为 100
- 修复 Windows 下 `sysinfo::cpu_usage_per_core()` 在短采样间隔下返回空向量或缺失核心（PDH 通配符格式化值在样本不足时 CStatus 无效）：改用 `NtQuerySystemInformation(SystemProcessorPerformanceInformation)` 的原始计数增量，首次调用返回全零、此后始终返回全部核心
- 修复 NFRS 反射生成器在输出文件被清理后跳过生成：`output_exists` 仅凭构造 `file` 对象就判定存在（构造失败不会抛出），缓存命中后直接返回，导致 clean 构建时 `_nfrs_gen_*.cpp` 缺失、编译报 C1083；现改为检查 `is_opened()`
- 修复 Windows 下 `http_client` 流式响应无法被 `http_stream_handle::abort()` 中断：`shutdown()` 不会唤醒阻塞中的 `recv`，流式读取改为非阻塞套接字 + 就绪轮询（50ms 粒度），空闲超时由读取循环基于 `stream_idle_timeout` 自行约束
- 修复 Windows 下 `process::current_command_line()` / `process::get_command_line()` 恒为空：`NtQueryInformationProcess(ProcessCommandLineInformation)` 需先查询所需缓冲区大小再二次查询，字符串内联在返回缓冲区中
- 修复 Windows 下 `process::snapshot()` 中大量进程的 `threads` / `start_time_ms` 为 0：改用 `NtQuerySystemInformation(SystemProcessInformation)` 一次性填充全部进程的线程数与创建时间，不再依赖对每个进程 `OpenProcess` 成功
- 修复 `sysinfo::disk_status(path)` 在 Windows 上对不存在的路径返回当前盘符数据：查询前先校验路径存在，不存在时返回零值信息
- 修复 `path::is_absolute()` 在 Windows 上不把单前导分隔符（如 `/tmp`）视为绝对路径，与 `lexically_normal` 的判定不一致
- 修复 `process::get_memory_info()` Linux 把 `/proc/<pid>/statm` 首字段（驻留页）当作虚拟内存大小的问题
- 修复 `sysinfo::cpu_usage()` Linux 采样状态无锁导致的数据竞争；Windows 改用 `GetSystemTimes`，中文等本地化系统上 PDH 计数器名解析失败导致恒返回 0 的问题不再存在
- 修复未设置背景的文本在字形下绘制主题背景、空白处保持终端默认背景导致的块状色差
- 修复 `scroll_view` 内 `band` 背景不受裁剪栈约束、滚动时把背景色涂到区域外的问题
- 修复 `path_tree::scan` 在 `files_only` 下丢失嵌套目录文件的问题
- 修复 `vector` 拷贝赋值复用旧元素导致嵌套容器缓冲区共享、二次释放的问题；改为重建存储并保留深拷贝语义
- 修复 `renderer::draw_glyph()` 宽字符续列越界：内容超出屏幕底部时，宽字形第二列会写到帧缓冲之外；现在续列同样做行边界检查
- 修复 `process_watcher::start()` 定时器回调的引用环：回调捕获了持有它自身的 `shared_ptr<function<...>>`，每次 `start()` 泄漏一个回调块；改为每轮安装新闭包的 `schedule_poll()`，并去掉不必要的 `make_shared<function>` 间接层
- 修复 `application` 三处定时器的同类引用环：帧 tick、窗口尺寸检查与 `Ctrl+C` 提示复位都捕获了持有自身的 `shared_ptr<function<...>>`，每次运行泄漏；现改为成员函数每轮安装新闭包 / 一次性闭包
- 修复 `strand` 在 handler 抛出非 `exception` 派生类型时永久保持锁定：内外两处 handler 调用都补上 `catch (...)`，异常不再逃逸并导致后续投递全部排队不执行
- 修复 `compressed_pair` EBCO 特化的 `get_base()` 返回 `compressed_pair&` 而非基类引用：`get_base() == get_base()` 因此命中 `icomparable` 的成员 `operator==`（比较 `.value` 的尺寸/指针）而不是比较分配器，导致 `deque` / `list` / `rb_tree` / `hashtable` / `flat_hashtable` 在等分配器（`standard_allocator`）下误走逐元素复制分支；`std::swap` 于 moved-from（中控器为空）的 `deque` 上移动赋值时 `emplace_back` 解引用空迭代器，clang 下抛出 `0xC0000005`（MSVC 下为未定义行为），`StackTest.Swap` / `QueueTest.Swap` 失败
- 修复 `deque::move_assign_impl` 逐元素回退分支对 moved-from 状态的越界访问：中控器为空时先重建再写入，避免空迭代器上构造元素
- 修复 `process_watcher` 在 Windows 上可能丢失子进程最后一段输出：退出轮询只 drain 一次就上报 `on_exit`，而管道与 ConPTY 的尾部数据常在子进程被回收之后才可读；现在退出后最多再轮询两次，直到连续两轮读不到数据才上报退出码
- 修复 `tool_card::status_icon()` 的悬垂引用：`spinner()` 返回的临时元素在语句结束时释放共享节点，绑定到 `element::text()` 的引用随之失效，运行中卡片的图标会随机渲染成 `.`；现改为取值拷贝
- 修复 `components::tool_card::render()` 无条件回写子组件头部导致的无限刷新：每次渲染都调用 `output_block::set_header()` 会再排一帧，`headless::tick()` 因此永不返回；现在仅在头部签名变化时回写
- 修复宿主先于组件树销毁时的野指针崩溃：`reconciler` 析构改为 `release_tree()` 递归清理，`tool_runner` 在 `cleanup()` 中结束子进程并注销句柄
- 修复 `process_watcher` 的定时器在监视器销毁后仍访问自身：自旋轮询与升级终止两个定时回调携带生存标志，析构后自动失效
- 修复自动高度容器中的虚拟列表高度塌陷为 0：`virtual_list` 现在给出 `item_count × item_height` 的估算固有高度，`vbox` 内嵌虚拟列表不再需要显式 `height = fill` 才可见
- 修复异步日志在 `disable_async()` 中死锁：停止线程池时仍持有 sinks 互斥锁，而正在收尾的 drain 任务正等在同一把锁上，`flush()` / `disable_async()` 因此偶发永久阻塞
- 修复 drain 任务在判定自身"已结束"后仍继续写 sink 的问题：`discard` / `overrun_oldest` 策略下，后一批事件可能先于前一批到达 sink，日志顺序错乱
- 修复 `flush()` 在异步模式下与 drain 任务争抢队列：原实现自行弹出队列并写 sink，与后台批量写入并发，sink 收到的事件会乱序，且 `flush()` 返回时更早的事件可能还没写出；现在通过与 drain 握手确认后才返回
- 修复 `disable_async()` 与 `enqueue_async()` 的竞态丢事件：切回同步模式时已在投递路径上的事件被丢弃，现将模式判定移入队列锁并回退为同步写入
- 修复线程池拒绝 drain 任务时事件永久滞留：不再把 `drain_scheduled_` 保持为 true，改为就地排空队列，阻塞策略下的生产者也不会再被永久挂起
- 修复 `enable_async()` 不重置 `drain_scheduled_` / `flush_requested_`：上一轮异步模式遗留的状态会让新一轮不再派发 drain 任务
- 修复 `disable_async()` 会停止调用方传入的共享线程池：现仅在自身创建的线程池上调用 `stop()`
- 修复 `logger` 自动刷新的 `last_auto_flush_` 数据竞争：多线程同时记日志时该时间戳被并发写，现为原子量并保证每个间隔只触发一次刷新
- 修复 `logger::set_level()` 与日志热路径的数据竞争：`explicit_level_` / `effective_level_` / `parent_` 改为原子量，`children_` 由互斥锁保护
- 修复 `logger_registry::get_logger()` 在锁外创建并插入 `loggers_`：并发创建 Logger 时同时改动哈希表，现整个创建过程在注册中心锁内完成
- 修复 `logger_registry::flush_all()` 持锁执行刷新：sink 中再调用 `get_logger()` 会自死锁，现先快照再在锁外刷新
- 修复 `NEFORCE_LOG_*_EVERY_N` / `NEFORCE_LOG_*_FIRST_N` 的静态计数器数据竞争：改为原子计数
- 修复 `log_sink` 格式化器在 `set_formatter()` 与 `log()` 之间的数据竞争：新增受锁保护的 `format_event()`，内置的 console / file / syslog sink 统一走该入口
- 修复 `locale::format_datetime()` 把无时区的 datetime 当作 UTC 实例、再按系统时区偏移呈现，导致 `datetime::parse()` 得到的墙上时间被平移：现在无时区的 datetime 按自身字段格式化，带偏移的 datetime 才换算到系统时区
- 修复 `locale::format_datetime(dt, date_style, time_style)` 把日期风格与时间风格传反：`udat_open` 的第一个参数是时间风格，`full` 与 `short` 组合时的结果因此相反
- 修复 `locale::time()` 的 `date_fmt` 与 `time_fmt` 互换：实参顺序修正后分别返回日期模式与时间模式
- 修复 `locale::time()` 的星期名缺少星期六：ICU 星期符号按 `UCAL_SUNDAY`(1) 起始索引，原来的 0..6 循环少取一个
- 修复 `concat_view` / `split_view` / `slice_view` 等视图在类作用域无条件构成 `decltype(declval<const V&>().begin())`，导致底层不支持常量遍历时整个类型硬编译失败、`requires Range<const V>` 守卫形同失效的问题：新增 `const_range_traits` 与占位迭代器，此类视图现在仅常量成员函数不可用，类型本身仍可正常实例化与遍历
- 修复 `range_adaptor_closure` 的友元 `operator|` 按值接收基类子对象后再向下转型的形式未定义行为，改为按派生闭包类型取值
- 修复 `counted_iterator` 只比较剩余计数与负计数越界不终止的问题，`counted_view::end()` 现在推进到底层对应位置
- 修复 `repeat_iterator` 相等比较忽略所指向值的问题
- 修复 `subrange_view` / `counted_view` 默认构造后成员未初始化的问题，`subrange_view::size()` 对逆序区间不再返回巨大值
- 修复 `transform_view` 移动赋值 `noexcept` 无条件为真、元素类型移动赋值可能抛异常时直接终止进程的问题
- 修复迭代器 `operator->` 在元素以值返回时取临时对象地址或直接编译失败的问题，现仅在元素以左值引用返回时提供
- 修复 `take` / `take_while` / `drop_while` 迭代器类别无条件声明为前向迭代器的问题，现按底层迭代器能力给出
- 修复 `reverse` 适配器与 `join_view` 缺少 `common_range` 约束、非 common 范围下报深层硬错误的问题
- 修复 `iota(start, bound)` 要求两参数同类型的问题，现允许可转换的不同类型
- 修复 `owning_view` 的移动构造与移动赋值被删除的问题：右值范围进入视图链后无法传递，现恢复为可移动
- 修复 `views::concat` 的两处缺陷：一是要求两侧范围均为 common 范围，`take` / `transform` 等非 common 视图作为任一操作数时无法编译，现由适配器统一套用 `common()`；二是右值范围作为第二操作数时闭包持有了不可拷贝的 `owning_view`，现以 `copy_constructible<view_type>` 约束给出明确诊断，改由两参数形式或查询链的 `concat()` 承担
- 修复 `join_iterator` 在 const 外层范围上的编译失败：内层范围类型用 `remove_cvref_t` 丢弃了 const，导致常量迭代器无法赋值给非常量迭代器；现仅在元素引用为右值时衰减
- 修复 `views::take` / `views::drop` 传入无符号计数时因列表初始化收窄而无法编译的问题，现按底层迭代器距离类型显式转换
- 修复 `lock_free_queue` 隐式生产者哈希表探测链被截断导致任务静默丢失、调用者永久阻塞：线程退出时该槽位被写回空槽标记，同一哈希表中位于其后的表项便对其它线程不可见，会重复插入第二个生产者表项，而退出路径每层哈希只清理一个表项；新线程命中该表项时直接取得已停用的生产者对象，写入其中的任务被消费者按"生产者已停用"跳过。现按上游语义把退出槽位写为墓碑标记，并允许墓碑槽位在表项重定位与插入时复用，探测链不再被截断
- 修复唤醒策略单靠空闲巡检线程数提示而可能漏唤醒、导致调用者永久阻塞的缺陷，改由存活巡检兜底
- 修复 `string_builder` 的 `concatenate()` 在 C++14 下无法编译：`__concat_append` 的递归终止分支缺失，`Rest` 为空时调用不存在的零参重载
- 修复 `thread_pool` 优先级倒置：工作线程原先先探测全局 FIFO 再探测优先级堆，只要普通队列有积压，高优先级任务就永远排在其后，现按 优先级 → 本地 → 全局 → 窃取 顺序探测
- 修复 `thread_pool` 待处理任务计数下溢：计数在入队之后才自增，而消费者可能先出队并自减，计数回绕为 4294967295 后阈值判断误判为队列已满，任务被静默拒绝，现改为入队前自增
- 修复 `thread_pool` 在待处理计数与队列不一致时无界自旋：工作线程以 `yield()` 永久空转占满 CPU，同时等待该任务的调用者永久阻塞，现改为停驻等待并在入队失败时显式失败任务
- 修复 `thread_pool` 忽略全局队列入队失败结果导致任务静默丢失：现检查 `enqueue` 返回值并回退计数，任务以 `failed` 状态与错误信息显式失败
- 修复任务函数抛出异常会终止工作线程，现工作线程捕获并计入失败计数
- 修复缓存模式工作线程退出时丢弃其本地队列中未执行的任务，现退出前执行完毕
- 修复 `thread_pool` 在 `stop()` 后重启时定时器调度器已被停止、`submit_after` 永久不触发的问题
- 修复 `flat_hashtable` 删除元素后不归还扩容额度，导致反复「填充—删除」时容量无界增长：扩容判据改由 `size_` 与最大负载因子直接派生，不再依赖只在插入时递减的计数
- 修复 `flat_hashtable` 墓碑（DELETED）永不回收导致探测链持续变长：新增墓碑计数，占用槽逼近扩容阈值时等容量原地重建
- 修复 `flat_hashtable` 在已有容量的对象上重新分配存储数组时不回收旧数组导致的内存泄漏
- 修复 `allocator_traits::rebind_alloc<T>` 未取 `alloc_rebind` 的 `type`，返回的是元函数而非重绑定后的分配器类型，任何使用该别名的地方都无法编译
- 修复 `hashtable::bucket_index()` 调用 `bucket_index_key()` 时漏传桶数参数（模板成员未被实例化而长期潜伏）
- 修复浮点定点格式化在二进制指数非负的整数值上小数点位置错误：原先按已放大 10^p 处理而把整数末几位误作小数，导致绝对值大于 2^52 的值（如 1e308、DBL_MAX）整数部分被截断
- 修复 UTF-16 代理对在批量解码中折叠为单个码点时的差分一致性（新增单元测试覆盖）
- 修复 `ssl_socket` 异步读写绕过 TLS 层的缺陷，现按 TLS 激活状态路由至 `ssl_stream`
- 修复 Windows `io_context` 事件注册缺失 FD_CONNECT / FD_CLOSE 使非阻塞 connect 的完成或失败通知不被注册/映射
- 修复 Windows `io_context` WSAEVENT 句柄生命周期竞态：`remove_fd()` 在监视线程 `WSAWaitForMultipleEvents` 等待期间直接 `CloseHandle`
- 修复 `io_context` fd/文件完成回调的 use-after-free：回调执行中调用 `remove_fd()` 或重复注册会销毁正在执行的回调存储
- 修复 `http_client` 忽略 URL 显式端口导致请求连到错误端口的问题，改用 URL 解析端口
- 修复 NFRS 被安装后索引 Release 动态库的方案
- 修复 Clang 下推断 websocket-deflate 整形符号溢出与 GCC 不同的警告
- 修复 SIMD `string_length` / `string_find` 跨 16 字节块偏移未累加导致的字符串比较错误
- 修复 `simd::memory_copy_offset` 尾部循环指针双重偏移导致的内容错乱
- 修复 `madds_i8x16` SSSE3 路径将有符号操作数当作无符号处理的计算错误
- 修复 Base64 SSSE3 路径 4 处缺陷：查表索引越界、输出组序反转、尾部字节丢失、解码移位错误与校验缺失
- 修复 ARM64 等非 x64 架构错误接收 x86 SIMD 编译标志的问题
- 修复 JSON 解析器字符串扫描控制字符检测使用有符号比较，将 UTF-8 多字节字符误判为控制字符导致 SIMD 快路径失效的问题
- 修复 `retry` 使用引用函数作为参数时在 clang -O2 优化下编译器空悬引用对象导致 ABORT 的问题
- 修复 `use_awaitable` 完成令牌在协程恢复时丢失 continuation 导致协程永不恢复的缺陷，awaitable 改为共享状态实现，支持作为协程返回类型
- 修复 `lock_free_queue` 隐式生产者在线程退出回调中访问已析构队列导致崩溃（0xc0000005）的缺陷：隐式生产者析构时无条件注销线程退出监听器
- 修复 `dns_client` UDP 发送失败时 pending 查询条目悬挂的问题
- 修复 `uninitialized_*` 系列与 `temporary_buffer` 的平凡路径分派条件
- 修复 Windows 上 `file::flush()` 无条件调用 `SetEndOfFile` 按当前文件指针截断文件，改为刷写用户缓冲与操作系统缓存
- 修复 `file::read_line()` 在 `\r\n` 恰好横跨读缓冲边界时多产生一个空行的缺陷
- 修复整文件读取在 Windows 上因 32 位 `size()` 截断而对超过 4GiB 的文件返回空内容，`read()` / `read_binary()` 全部改为 64 位分块读取
- 修复 `file::size()` 在 Windows 上对超过 4GiB 的文件返回低位截断值
- 修复 `file` 读写交替时写入落在预读超前位置导致的内容错位，写前自动把系统偏移回退到逻辑位置，读前先刷写待写缓冲
- 修复 `file_async` 仅有一个待处理槽导致同一句柄并发操作互相覆盖完成回调的问题，改为按操作独立登记并保证每个 handler 调用一次
- 修复 `file_async` 取消槽参数被忽略，改为取消请求提交失败时按标志在完成时设置 `operation_aborted`
- 修复 `file_async` io_uring 环满/提交失败时静默丢弃操作导致调用方永久等待的问题，改为设置 `resource_unavailable_try_again`
- 修复 `filesystem::move()` 在 Linux 上先删除目标再 rename 的破坏性窗口，改为使用 rename 原子替换，仅非空目录覆盖时回退删除后重试
- 修复 `filesystem::copy()` 在源与目标为同一文件时先截断源文件导致数据丢失，改为路径相等直接返回成功，Linux 侧另以 dev/ino 判定别名，并拒绝 FIFO/设备等特殊文件
- 修复 `filesystem::copy_directory()` 在目标位于源子树内时无限递归，新增路径包含关系防护
- 修复 `remove_all_in_directory()` / `copy_directory()` 跟随目录符号链接可能删除或复制链接目标内容的问题，改为 Linux 侧符号链接仅 unlink 不再下降，Windows 侧跳过 reparse point 目录
- 修复 `path_tree::scan()` 在 `follow_symlinks=true` 且不限制深度时目录符号链接成环可无限递归/栈溢出，新增基于 卷/设备号 + 文件节点号 的链环路检测，reparse 目录不再被无条件递归
- 修复 `temp_file` 构造时先创建文件再换用另一候选，导致每次构造泄漏一个临时文件的问题
- 修复 Windows 文件模块使用 ANSI API 导致非 ASCII 路径失败的问题，改用 Unicode API
- 修复 valgrind CI 门禁失效问题，`valgrind ... | tee` 的管道退出码取 `tee` 的 0，导致 `--error-exitcode=1` 永远无法让 CI 失败
- 修复 `VirtualThreadTask.DestroyAwaitingTaskWhileSuspendedAsContinuation` 等待器悬垂：`co_await task` 以任务对象自身为等待器，恢复时仍要读取它，而用例只等到 `inner.is_done()` 便退出作用域，被 detach 的等待方帧恢复时读到已析构的等待器；现改为在作用域内等待该帧执行完毕并断言其返回值
- 修复 `http_session` 的并发数据竞争：`session_manager::cleanup_expired_sessions()` 在清理线程上读取会话状态，而请求线程经 `csrf_filter`、`add_session_cookie`、`session_store` 直接读写，现为会话加入内部互斥量并补充加锁入口
- 修复 `plugin_entry.hpp` 声明 `create_plugin` / `destroy_plugin` 导致的 MSVC 编译失败（C2375 / C2733）：入口函数由插件实现并导出，带导出属性的定义与不带导出属性的声明在 MSVC 下被判为重定义，现移除该声明并改为提供 `NEFORCE_PLUGIN_EXPORT` 导出属性宏
- 修复 `unique_ptr` 同类型移动赋值丢失删除器问题，`__unique_ptr_impl::operator=` 只搬运指针而保留目标自身的删除器
- 修复 `pointer_traits` 对智能指针的 `to_address()` 返回悬垂引用，指针特化用 `decltype(auto)` 推导出 `const Ptr&`
- 修复 `plugin_manager::load_plugins()` 完全不可用问题，现改用 `path_tree::scan()` 扫描目录并按裸扩展名过滤
- 修复 `window` 的 Ctrl+Shift+方向键缩放不可达问题，先匹配的 Ctrl 分支未排除 Shift，导致缩放分支恒被移动分支吞掉
- 修复 `hoverable` 与 `collapsible` 首次 `render()` 即空指针解引用崩溃问题，`component_base::add_child()` 现在会在 `active_child()` 为空时认领首个子组件并忽略空子组件
- 修复 `normal_iterator::operator[]` 对容器迭代器不可用问题，`vector_iterator::operator[]` 缺少 `const`
- 修复 `reverse_iterator::operator[]` 无法实例化问题，其 `noexcept` 说明中的 `decltype` 表达式无法成立， 改为按实际返回表达式推导
- 修复 `graph()` 纵轴缩放错误问题，`static_cast<int>(ratio) * (height - 1)` 先截断再相乘使所有小于 1.0 的比例都落到第 0 行、图形被压平，改为先缩放后取整
- 修复滚动指示器滑块尺寸与位置错误问题，`static_cast<int>(ratio) * height` 先截断再相乘，使滑块恒为 1 格，改为先缩放后取整
- 修复 `string_length` / `string_find` 的块扫描未向下对齐导致的跨页 SIGSEGV问题，非对齐的 16 字节读取在地址落在页尾 15 字节内且后继页未映射时会越界访问。新增 `SimdGuardPageTest` 以 `mmap` + `mprotect(PROT_NONE)` 复现该场景
- 修复 `linear_gradient::add_stop()` 追加位置靠后的色标导致中间色标不可达问题，改为位置升序插入，并统一 `sample()` 的位置语义
- 修复 Sanitizer 构建无法启动问，`string_length` / `string_find` / `string_compare` 的 16 字节块扫描会读取终止符之后的填充字节，ASan 逐条插桩内存访问导致初始化中止，改为 `NEFORCE_SANITIZED_SCAN` 在这些函数中改走标量路径
- 修复 `timer_scheduler::stop()` 的丢唤醒：唤醒可能恰好落在调度线程"谓词读到 false、但尚未挂入等待队列，现将状态改动与唤醒一并放入 `mutex_`
- 修复 `virtual_thread_task` 完成通知的丢唤醒：`final_suspend` 原先在 `task_shared_state::mtx_` 之外改写 `completed_`，落在窗口内的通知会送达零个等待者。现将完成标记、唤醒与 continuation 交接一并放入同一把锁
- 修复 `virtual_thread_task::await_suspend()` 的双原子 TOCTOU：`completed_` 与 `continuation_` 原先是
  两个独立原子，写入顺序之间无法建立 release/acquire 链。存在 "调用者先登记 continuation、随后读到未完成，而被等待任务此刻已完成并已取走 continuation" 的交错，
  双方都以为对方负责恢复，调用者永久挂起。现改为在 `mtx_` 内一次完成"读完成标记 + 登记 continuation"，与 `final_suspend` 的"读 continuation + 置完成标记"互斥，二者恰好有一方接手
- 修复 `co_await` 的等待方在挂起期间被销毁导致的 use-after-free：`scheduled_` 原先只在
  `yield` / `sleep` 等待器中置位，`co_await task` 的登记路径漏置，于是被等待任务仍持有该帧句柄时，
  等待方任务的析构就可能释放它，随后对方恢复一个已释放的帧。现将该标记更名为语义准确的 `detached_`，
  并在 `await_suspend()` 登记 continuation 时置位，析构与移动赋值的判据仍为"未完成且未交付"才回收帧
- 修复 `virtual_thread::start()` 的闭包生命周期陷阱，现在以 `static_assert` 拒绝 "右值 + 非空闭包" 的协程可调用对象
- 修复 `co_await virtual_thread_task` 的等待器悬垂：等待器原先是任务对象自身，等待方恢复时会去读被等待的任务对象，而库既不持有其所有权也不约束其生命周期，被等待任务对象提前销毁即读到垃圾。现在改为 `operator co_await()` 返回独立的等待器，等待器自身持有共享状态引用，等待方与被等待任务对象的生命周期解耦
- 修复等待方协程帧的 detach 标记写入未知内存：`mark_continuation_scheduled()` 把任意 continuation 句柄都当作 `virtual_thread_task<void>::promise_type` 访问其 `shared_state_`，现改为模板化的 `await_suspend(coroutine_handle<Promise>)`，编译期通过 `is_virtual_thread_promise` 判定等待方是否为任务协程
- 修复 `virtual_thread::sleep()` 每次等待创建一个分离线程：现改为调度器内的定时器最小堆，`shutdown()` 会把未到期定时器执行完，避免已交付调度器的协程帧既不恢复也不释放
- 修复 TSan 下 SIMD 块扫描被误报为 use-after-free 的问题：`NEFORCE_SANITIZED_SCAN` 原先只覆盖 ASan/MSan，现新增 `NEFORCE_HAS_THREAD_SANITIZER` 并一并纳入
- 修复线程 hook 的静态析构顺序问题：hook 表与其互斥量改为不做静态析构，线程对象在静态析构阶段被销毁时不再操作已析构的互斥量
- 修复 `wyhash` 短输入分支的越界读取：`len <= 16` 分支本应做 32 位读，写成 64 位读后长度在 [4, 16] 的输入最多越界 4 字节（16 字节键的第二次读从 `p + 12` 起就越界），且多出的高位来自越界内存，导致哈希不可复现
- 修复 `base64_encode_12bytes` 的越界读取：调用方只保证 12 字节可读，函数却直接读取 16 字节；改为 `_mm_loadl_epi64` + 4 字节尾读拼装
- 修复 `dns_client` 的未对齐读取：6 处把字节指针直接重解释为 `uint16_t*`，偏移为奇数时即为未对齐加载，改为逐字节拼装大端字段
- 修复 HPACK 整数解码的移位未定义行为：续字节在 32 位整型上移位，`m` 超过 31 后 UB，改为 64 位累加并在超出 `uint32_t` 范围时饱和返回
- 修复 `io_context::add_fd()` 在 Linux 上固定使用 `EPOLL_CTL_ADD` 并忽略 `epoll_ctl` 返回值的问题，现按描述符是否已在 `fd_map_` 中选择 `EPOLL_CTL_MOD` / `EPOLL_CTL_ADD`，并在失败时 `terminate()` 中止而非静默继续
- 修复服务端 ALPN 协商从未生效：`set_alpn_protos()` 只调用 `SSL_CTX_set_alpn_protos()`，现设置列表时一并安装选择回调
- 修复 `io_context` 中 `fd_map_` 的数据竞争
- 修复 `ssl_stream` 缺少析构函数导致的资源泄漏
- 修复 `regex` 的两处 pcre2 泄漏
- 修复 `zlib_compressor::stream_compressor` / `stream_decompressor` 的 zlib 状态泄漏
- 修复 MySQL 客户端线程局部状态泄漏：libmysqlclient 在某个线程首次调用 C API 时分配线程局部数据，现于 `mysql_connect` 的各入口注册一个函数内 `thread_local` 守护对象，在线程结束时调用
- 修复 `tui::state<T>` 在 `strand` 注入前构造导致的空指针解引用，现移除该参数与成员
- 修复 `hexadecimal` 解析 `-0x8000000000000000` 时转成 `int64_t` 再取负，属未定义行为；现在直接返回 `numeric_traits<int64_t>::min()`
- 修复 Linux 上 CPU 最大频率缺少 `CPUID` 兜底
- 修复安装包 `NEXUSFORCE_AI_API_INDEX` 指向错误前缀：该变量原先在 `find_dependency` 之后取值，而依赖包的配置文件会用自身前缀覆盖 `PACKAGE_PREFIX_DIR`，于是变量指向依赖包的安装位置而非 NexusForce 的安装位置
- 修复 `scripts/install_nexusforce.py` 在 Windows 控制台上因中文提示触发 `UnicodeEncodeError` 而中断安装：脚本启动时把标准输出与标准错误切换到 UTF-8
- 修复型号字符串中 `GHz` 频率的截断：`2.40GHz` / `3.70GHz` 因十进制不可精确表示又被直接截断，改为四舍五入
- 修复 valgrind 工作流把单元测试失败误报为内存泄漏：`--error-exitcode=1` 在 valgrind 未发现错误时会透传被测程序的退出码，任一用例失败即表现为"内存泄漏检查失败"，现改用 `99` 作为泄漏专用退出码并分别报错
- 修复 Windows 上安装包缺失运行时依赖，导致安装前缀的 `NFRS.exe` 与下游消费者的可执行文件以 `0xc0000135`（STATUS_DLL_NOT_FOUND）启动失败：Windows 无 RPATH，安装期解析 NexusForce.dll 的依赖闭包（ICU、PCRE2、OpenSSL、zlib、lz4、hiredis、sqlcipher、libmysql、LIBPQ 等）并随安装包一并复制到 bin 目录
- 修复内存池自旋锁在多线程下丢唤醒导致挂死，其唤醒被自身的等待计数门控，已经抬起竞争标记、尚未进入等待器的线程会错过唤醒而永久停驻，改为等待期望值固定为竞争标记 + 标记清零时无条件唤醒的两阶段协议
- 修复 `memory_pool` 在线程缓存关闭时仍向线程缓存填充整批块的缺陷，此前这些块既不归还也不复用
- 修复 `memory_pool` 尺寸类下标缺少边界校验的缺陷：`size_to_class()` 返回的大对象哨兵值在小对象路径被当作数组下标使用，现降级走大对象路径
- 修复 `memory_pool` 超大请求（超过 2^46 字节）在区域尺寸计算中溢出的缺陷，现直接返回失败而不是映射一小块内存
- 修复 TUI 进度条 `gauge()` / `gauge_direction()` 只有全空与全满两种状态：原先把归一化进度先截断成 `0`/`1` 再乘宽度，现值 `0.5` 可以正确填充一半
- 修复 TUI 输入解析的多处缺陷：Alt+字符（`ESC` 前缀）会使解析器卡死并最终抛出多余 Escape、带修饰键的 CSI 序列整体作废、畸形鼠标序列可永久吞掉后续输入、`ESC[200~` 粘贴被拆成逐字符
- 修复 `screen::to_string()` 忽略 `cursor()`，导致 `set_cursor()` 形同虚设、终端光标始终隐藏
- 修复 `canvas` 元素造成布局索引错位：`find_element_at()` / `hit_test_at()` / `clear_element_area()` 未消费 `canvas` 的布局矩形，其前后元素的点击命中与清除区域都会偏移
- 修复 `reconciler::flush()` 在 `render()` 抛异常后永久停止渲染
- 修复焦点链不随组件增删更新导致 Tab 导航跳过新组件，以及聚焦组件被 `remove_child()` 移除后 `focused_` 悬空
- 修复 `animator` 以空指针构造时在初始化列表解引用空指针
- 修复 `vscroll_indicator()` 在内容可完全显示时返回一行 `height` 个空格，占据 `height` 列而非一列空白
- 修复 TUI 词换行渲染在每个词边界多写一个空格，导致换行后的文本出现双空格
- 修复 TUI 文本输入框的光标位置在词换行/自动换行下偏移，现按渲染器实际排版结果定位
- 修复 TUI 超长单行输入时光标消失，光标绘制不再越过输入框内边界
- 修复 TUI 渲染器把控制字符原样写入 cell 网格，会破坏整帧的字符对齐：现值一律渲染为空白，并按一列测量宽度
- 修复 `strand` 的 handler 抛异常会让串行化锁永久保持在置位状态，其后所有 `post`/`dispatch` 只入队永不执行：`dispatch()` 现在先复位锁、推进队列，再把异常抛回调用者；`drain_one()` 的投递回调吞掉异常后继续排空

### 📚 Documentation

- 添加 `THIRD_PARTY_NOTICES.md` 收录移植代码与链接依赖的版权声明与许可证

## [1.0.0] - 2026-08-03

### 🚀 New Features

- 添加 UTF-8 码点迭代器 `utf8_iterator` / `utf8_range` / `utf8_view`
- format 引擎支持位置参数 `{0}` `{1}` 和顺序格式选项 `{:d}` `{:x}`
- 添加命名参数格式化函数 `format_named()`
- regex 支持拷贝构造和拷贝赋值
- 添加 SIMD 检测宏
- 实现反射驱动的 JSON 序列化器 `json_serializer`（序列化/反序列化/递归嵌套/容器遍历）
- 实现反射驱动的二进制序列化器 `binary_serializer`（大端格式、类型表、属性注解控制）
- 添加 GoogleBenchmark 配置
- `sql_builder` 达到 ANSI SQL-92 Entry/Intermediate 级完整覆盖，添加方言感知占位符、建表、分页操作
- 添加 `sql_mapper<T>` 反射驱动 ORM SQL 生成器，自动生成 DDL（CREATE/DROP TABLE）和 DML（INSERT/UPDATE/DELETE/SELECT）语句，支持方言感知占位符
- 添加 `repository<T, Connect>` 泛型 CRUD 仓库模板，封装 `sql_mapper` 提供高层数据访问接口
- `property_attr` 添加 DB 注解标志：`PROP_PRIMARY_KEY` / `PROP_AUTO_INC` / `PROP_UNIQUE` / `PROP_INDEX` / `PROP_FOREIGN_KEY`
- 添加 ORM 示例 `orm_example`，展示 `sql_mapper` SQL 生成与 `repository` CRUD 操作
- 添加 HTTP/2 (h2) TLS+ALPN 协商支持，`ssl_stream::get_alpn_negotiated()` 接口
- 添加 HTTP/2 连接管理 `http2_connection`（h2c 升级 + h2 ALPN 双模式）
- 添加 HTTP/2 协议帧处理 `http2_protocol`
- 添加 gRPC 服务端示例（Greeter + Health Check）
- 添加反向代理示例 `reverse_proxy`（连接池 + 轮询负载均衡）
- 添加健康检查端点示例 `health_check`（限流 + 安全头）
- 添加异步过滤器框架 `async_filter`，支持异步 pre/post 过滤链
- 添加 HTTP 缓存 `http_cache`、CSRF 防护 `csrf_filter`、安全头 `http_security`
- 添加 HTTP 压缩传输 `http_compress`、范围请求 `http_range`
- 添加多部分解析器 `multipart_parser`、分块传输读取器 `chunked_reader`
- 添加字节游标 `byte_cursor` 用于无拷贝帧解析
- 添加基数树路由器 `radix_router`
- 添加负载均衡器 `load_balancer`
- 添加缓冲区链 `buffer_chain`
- 添加异步事件循环 `event_loop`
- 添加 `http_client_request` 的 `scheme` 字段，支持基于 scheme 的 SSL 自动检测
- 添加稀疏容器 `sparse_vector`，基于排序扁平数组的关联容器基类，支持二分查找 O(log n) 定位与 O(1) 缓存友好迭代
- 添加 `sparse_set`，基于 `sparse_vector` 的唯一键集合容器
- 添加 `sparse_map`，基于 `sparse_vector` 的唯一键映射容器，支持 `operator[]` / `at()` 访问
- 添加 `sparse_multiset`，基于 `sparse_vector` 的可重复键集合容器
- 添加 `sparse_multimap`，基于 `sparse_vector` 的可重复键映射容器
- 添加 `tcp_socket::async_connect()` / `async_read()` / `async_write()`，通过 `io_context` 驱动的异步 TCP 操作，支持取消槽
- 添加 `udp_socket::async_receive_from()` / `async_send_to()`，支持异步 UDP 数据报收发与发送方地址回调
- 添加 `file_async::async_read()` / `async_write()`，通过 `io_context` 驱动的异步文件 I/O，支持指定偏移量与取消槽
- 添加 `async_read()` / `async_write()` 自由函数组合器，基于 `shared_from_this` 自动处理部分读写重试
- 添加 `thread_pool_executor`，将 `thread_pool::submit_task()` 适配为标准 executor 接口
- 添加 `simd_util` 通用 SIMD 工具模块，封装 SSE2/AVX2/NEON 指令集差异，提供 `fill_byte` / `load_unaligned` / `match_bytes` / `to_bitmask` 统一接口，非 SIMD 环境自动回退标量实现
- 添加 `flat_hashtable` 开放寻址平坦哈希表，采用 SwissTable 风格元数据控制块 + H2 预过滤 + SIMD 批量探测，延迟分配策略
- 添加 `flat_unordered_map` / `flat_unordered_set` / `flat_unordered_multimap` / `flat_unordered_multiset` 平坦无序关联容器
- 添加 ChaCha20-Poly1305 AEAD 认证加密算法（RFC 8439），支持关联数据（AAD）认证和常量时间标签验证
- 添加终端 UI 框架 `tui`，提供声明式、响应式的现代终端用户界面开发能力
- 添加 TUI 核心引擎：`application`（应用入口，Builder 模式）、`reconciler`（声明式渲染引擎，Cell 级终端帧差分）、`screen`（终端帧缓冲，增量 ANSI 转义序列生成）、`input_driver`（跨平台终端输入驱动，ANSI 转义序列 / SGR 鼠标 / UTF-8 多字节解析）
- 添加 TUI DOM 层：`element`（虚拟元素树，14+ 节点类型）、Flexbox 布局引擎（direction / wrap / justify / align / gap / flex_grow / flex_shrink）、Gridbox 网格布局、`style` 样式系统与 `theme` 主题系统、`state<T>` 响应式状态管理（自动脏标记 + strand 调度）、`ref<T>` 持有或借用适配器
- 添加 TUI DOM 辅助元素：`gauge` 进度条、`graph` 折线/柱状图、`paragraph` 自动换行文本、`spinner` 加载动画、`scroll_indicator` 滚动条、`linear_gradient` 多色渐变、`table` 声明式表格构建器
- 添加 TUI 组件系统：`component_base` / `component<P>` 基类（焦点管理 / 上下文注入 / 子组件树）、`container` 容器（垂直 / 水平 / 层叠）、`menu` 菜单列表、`dropdown` 下拉选择、`radiobox` 单选组、`toggle` 切换开关、`slider` 滑块、`text_input` 文本输入（UTF-8 感知光标 + 闪烁）、`scroll_view` 滚动视图、`window` 浮动窗口、`modal` 模态覆盖层、`collapsible` 折叠面板、`hoverable` 鼠标悬停检测、`resizable_split` 可拖拽分割面板、`renderer` 渲染辅助（含 `catch_event` / `maybe` 条件渲染）
- 添加 TUI 动画系统：`animator` 属性动画器 + `easing` 缓动函数（linear / quadratic / cubic / sine / elastic / bounce）
- 添加 `charset` 字符集工具类，支持集合运算与 ASCII 预定义字符集
- 添加 `string_builder` 字符串构建器，延迟拼接策略实现单次分配输出，提供 `concatenate()` 自由函数
- `basic_string` / `basic_string_view` 添加 `charset` 重载

### 🔧 Improvements

- `tcp_socket` 继承 `async_stream` 抽象接口，统一异步读写协议
- `tcp_client` / `tcp_server` / `tcp_acceptor` 适配 `io_context` + `cancellation_slot` 异步模式
- `http_client` / `http_server` / `http2_connection` / `websocket` / `reverse_proxy` 适配 `io_context` 事件驱动模型
- 异常构造使用 `error_code` 替代原始 `int` 错误码，统一错误信息传递
- `timer` 添加任务取消标志检查，取消后的任务节点跳过执行
- 移除独立 `event_loop`，事件循环功能并入 `io_context`
- 异常体系移除 `static_type`，改用 `type()` 虚函数
- `char_traits_find` 窄字符子串搜索采用 Boyer-Moore-Horspool 算法，平均 O(n) 复杂度
- `standard_allocator` 添加 `max_size()` 成员函数
- `regex_iterator` 改为惰性求值 forward_iterator，避免全量缓存 O(n) 内存占用
- `format` 引擎内部重构为 tuple 随机访问架构
- `basic_string` / `basic_string_view` 添加 `@note` 注释注明 NUL 字符截断风险
- `sql_builder` 扩展 API，覆盖 DML/DDL/视图/索引/CTE/窗口函数/集合操作
- `database_pool` 新增 `active_count()`（活跃连接数查询）/ `warm_up()`（连接池预热）/ `get_tb_connect_for()`（自定义超时获取表连接）API
- `http_client` 重构为 scheme 感知模式：`scheme == "https"` 时自动创建 SSL 上下文
- `ssl_client::post_connect()` 无 SSL 上下文时跳过 TLS 握手，支持纯 TCP 模式
- `http_server` HTTPS 构造函数自动设置 ALPN 协议列表 (`h2`, `http/1.1`)
- `tcp_socket::receive()` 在非阻塞模式下 EAGAIN/EWOULDBLOCK 返回 0（不再抛异常）
- `websocket_server` 支持事件驱动模式 (`event_loop`) 与多线程模式切换
- `authentication_filter` 添加 `add_included_path()` 白名单模式
- `meta_any` 采用 SBO + 函数指针分发架构，支持属性注解、枚举反射、多态克隆工厂、容器类型标识
- NFRS 生成代码使用完全限定名 `neforce::reflect::` 避免命名空间污染
- 分离 `reflect_macros.hpp`（空标记宏，供扫描器识别）与 `reflect.hpp`（构建器 API），消除同一宏两种定义导致的重复定义警告
- 添加 `NEFORCE_REFLECT_ENUM` / `NEFORCE_REFLECT_ENUM_VAL` 宏用于枚举注册
- 添加 `NEFORCE_REFLECT_RESOLVE_BASES()` 统一延迟基类解析
- 添加反射扫描器 NFRS（NeForce Reflection Scanner），MOC 类预编译代码生成器，扫描 `NEFORCE_REFLECT_*` 标记并生成 `_neforce_reflect_gen.cpp`
- NFRS 支持增量扫描（基于文件修改时间缓存，文件未变更跳过重新生成）
- NFRS 支持枚举扫描注册（`NEFORCE_REFLECT_ENUM` / `NEFORCE_REFLECT_ENUM_VAL`）
- NFRS 支持信号扫描注册（`NEFORCE_REFLECT_SIGNAL` 宏）
- NFRS 支持复合属性注解（`PROP_OPTIONAL | PROP_TRANSIENT` 管道组合）
- 添加 `signal_base` 类型擦除基类，为 `signal<T...>` 提供虚函数接口（`disconnect_all`、`block`、`unblock`、`slot_count`、`emit_dynamic`、`connect_dynamic`）
- 添加 `meta_function::invoke()` 运行时调用支持
- 添加 `registry::connect_signal_to_slot()` 运行时动态信号槽连接
- 添加 `meta_type::add_property()` 动态属性注册
- 添加 `meta_property::set_notify_signal()` / `notify_signal()` 属性变更通知信号
- 添加 `meta_any::emplace<T>()` 原地构造，支持不可拷贝/不可移动类型

### 🐛 Bug Fixes

- 修复 `__string_bitmap` 对 `char16_t`/`char32_t` 宽字符截断高位导致 false positive
- 修复 `utf8_iterator::operator++` 无效 UTF-8 字节跳过数量错误（以 `decode_utf8` 实际消耗代替 `utf8_length()`）
- 修复 `utf8_iterator::operator==` 空 range 比较失败导致 range-for 死循环
- 修复 `regex_token_iterator` 悬挂 `string_view` 改为持有 `string` 拷贝
- 修复 SSO 启用时 `sql_builder` 的 `values(initializer_list<string>)` 在 LONG 模式字符串边界导致堆损坏的问题
- 修复 Linux 平台的 CMake install 配置
- 修复 `http_server::send_response()` 对 TLS 连接使用 `::writev()` 裸 fd 绕过 SSL 的问题
- 修复 HTTP/2 h2c 升级握手挂起（缺少 SETTINGS 帧发送与 `flush_writes()`）
- 修复 HTTP/2 h2c 升级后请求体丢失（`handle_upgrade_request` 传递空数据）
- 修复 HTTP/2 h2c POST 请求 `end_stream` 始终为 true 导致请求体为空
- 修复 HTTP/2 响应头大小写未转换为小写（HTTP/2 要求小写头）
- 修复 `http_client` `set_verify_ssl()` 在已连接状态下调用 `set_verify_peer()` 导致崩溃
- 修复 `reverse_proxy::forward()` 未设置 `creq.host`/`creq.port` 导致无响应
- 修复反向代理 `http_client` 连接慢（`ssl_client::post_connect()` 对纯 HTTP 也尝试 TLS 握手）
- 修复 WebSocket 事件驱动模式无回显（`queue_frame()` 在 ET epoll 下等待 EPOLLOUT 永远不触发）
- 修复 `tcp_socket::receive()` 非阻塞模式下 EAGAIN 抛异常导致 SIGABRT
- 修复 WebSocket 掩码键字节序反转导致小端系统上文本乱码（`try_read_be32` + `reinterpret_cast`）
- 修复 WebSocket 第二条消息解压乱码（解压器未在消息间重置上下文）
- 修复 zlib 流式压缩/解压无限循环（`!data.empty()` 条件永不变化）
- 修复 zlib `Z_FINISH` 在 raw deflate 下不保证追加空存储块导致浏览器 WebSocket permessage-deflate 压缩消息被拒绝 (PROTOCOL_ERROR 1002)
- 修复 MSVC `/OPT:ICF` 将不同模板实例化的 `type_id_for<T>()` 函数合并，导致所有未注册类型的 type_id 碰撞为同一值。
- 修复 `registry::instance()` 在库边界上被复制为多份单例，导致库内与测试代码使用不同的注册表实例
- 修复 `binary_serializer::deserialize()` 中 `be_to_host(read_beXX())` 双重字节序转换导致整数损坏
- 修复 `dynamic_library::load_by_name()` 在 Linux 上对含版本后缀的名称（如 `libpthread.so.0`）错误追加 `.so` 后缀的问题
- 修复 `share_memory::map()` 在只读访问模式下因 `__atomic_compare_exchange_n` 在 x86 上失败时仍执行写入（`lock cmpxchg`）导致 SIGSEGV 的问题
- 修复 `semaphore` 条件判断反转：`update > 0` 改为 `update <= 0`，修正 release 后未正确唤醒等待线程的问题
- 修复 `thread_pool` 在 cached 模式下因 `thread_pool_id_generator` 的 inline 函数中 `static atomic` 在 DLL/EXE 边界产生双实例，导致线程 ID 冲突引发 `lazy_thread::start()` 空函数崩溃的问题
- 修复 `thread_pool` worker 退出路径中 `threads_map_.empty()` 检查和 `exit_cond_.notify_all()` 未在 `worker_contexts_mtx_` 保护下执行，导致 `stop()` 可能丢失 `exit_cond_` 通知而永久阻塞
- 修复 `database_pool::stop()` 中 `cv_.notify_all()` 未持锁调用，在 replenish 线程的 `pred()` 检查与 `wait()` 之间产生竞态窗口，可能导致通知丢失而使 `replenish_thread_.join()` 永久阻塞
- 修复 `io_context` Windows WSA 事件映射中将 `FD_CLOSE` 错误纳入 `epoll_in`，导致 UDP ICMP 错误触发虚假可读通知
- 修复 `io_context::~io_context()` 在 `run_one()` 惰性启动 monitor 线程后未 join，导致 `~thread()` 对 joinable 线程调用 `terminate`
- 修复 `ssl_stream::connect()` / `accept()` 中 `ERR_get_error` 双重消费错误队列，导致 `handle_ssl_error` 获取不到真实错误
- 修复 `ssl_context` 在 Windows 上无法加载系统 CA 证书，导致 TLS 客户端证书验证失败（错误码 167772294 / `0xA000086`）

### 🔧 Improvements

- `dns_client` 深度重构为 per-operation 异步状态对象架构，`pending_entry` 基于 `weak_ptr` 管理生命周期
- `dns_client` 添加 completion-token 异步 API：`async_query()` 六重载（回调/取消槽/模板令牌/`use_future`/`detached`/`use_awaitable`）
- `dns_client` 添加 `cancellation_slot` 支持，可取消飞行中的 DNS 查询
- 添加 `async_result<use_future_t, void(error_code, dns_query_result)>` 和 `future_handler<error_code, dns_query_result>` 特化
- `drain_events()` 使用 CAS 替代无条件 store 调度 drain 任务，消除高并发下的低效重提交洪

## [1.0.0-beta] - 2026-05-18

### 🚀 New Features

- 项目从 MSTL 重命名为 NexusForce
- 添加 GTest 全量单元测试于集成测试
- 导出 cmake 配置函数
- 添加代码格式化与检测配置
- 添加 lz4 压缩操作
- 添加线程屏障 barrier / latch
- 添加协程 coroutine / generator 支持
- 添加危险指针 hazard_ptr
- 添加系统信号量 semaphore
- 添加基于协程的虚拟线程 virtual_thread
- 添加布隆过滤器 bloom_filter
- 添加 lru_cache / ttl_cache
- 添加断点调用 breakpoint
- 添加错误码系统 errc / error_category / error_code / error_condition
- 添加 YAML 1.2 的 builder 与 parser
- 添加路径树 path_tree
- 添加端序操作 endian
- 添加 shared_ptr / weak_ptr 特化的 atomic 无锁操作
- 添加 int128_t 与 uint128_t 操作
- 添加反射系统与反射注册表 registry
- 添加 Unicode 码点操作类 codepoint
- 添加 PCRE2[with JIT] 正则类 regex
- 添加错误流输出能力 eprint
- 添加本地化配置类 locale
- 添加系统管道操作类 pipe
- 添加 Windows 注册表类 registry
- 添加共享内存类 share_memory
- 添加系统信息获取类 sysinfo
- 添加范围计时类 click
- 添加 scope 操作
- 添加 UUID v4 / v7 生成器
- 添加 ICMP / SMTP socket 操作
- 添加 ARP / MAC / IP / ports 操作
- 添加 Websockets 通信操作
- 添加 cacert 证书测试

### 🔧 Improvements

- 完善配置项并通过 CodeQL / clang-format / clang-tidy / valgrind 进行自动化分析
- 完善 README 特性项
- 使用外部配置 cmake 选项
- 使用 vcpkg 包管理
- 大幅优化 network 结构设计
- 优化文档结构
- 优化 FUTEX / atomic 设计结构
- 优化 call_once ，采用 FUTEX 线程提示机制
- 优化 path / file 设计，分离职责到子工具类
- 使用 iiterator 优化迭代器实现
- 优化 unique_ptr 的转换功能
- 优化随机数生成器的结构设计
- 优化 UTF 转换操作实现
- 优化 formatter 实现
- 优化 expected 结构
- 使用 none 统一工具类的空表示
- 优化 WinSock 初始化方式
- 优化数据库连接池实现
- 优化测试结构
- 使用匿名命名空间优化编译单元内部实现
- 修复 tcp_socket 的链接问题
- 重构 process 操作为 RAII 设计
- 去除 builtin_allocator 内置行为的分配器
- 去除 device 操作

### 📚 Documentation

- 除 db 与 network 外的大部分 API 文档
- 优化 README 结构

### 🐛 Bug Fixes

- 修复 make_shared 内存泄漏问题
- 修复线程池 cached 模式下的临界区操作异常问题
- 修复 datetime 对UTC时间处理的异常问题
- 修复 zlib 压缩解压缩的句柄释放方式
- 修复 futex 在 Linux 的 private 阻塞问题
- 修复 timer 的异步内存访问问题
- 修复 bitmap 的内存未初始化问题
- 修复 AES256 的 gf128_multiply 算法
- 修复 function 的类型擦除导致字面量被识别为右值的问题
- 修复 make_shared 在联合分配时的内存泄漏问题
- 修复随机数生成器的生成范围异常问题
- 修复 regex_token_iterator 的异常迭代问题
- 修复 system_signal_manager 在多平台行为不一致的问题
- 修复 sql_builder 的行为与 ANSI 标准不符合的地方

## [0.4.0] - 2025-12-26

### 🚀 New Features

- 添加 zlib 压缩操作
- 添加固定大小的位操作类 bitset
- 添加 xor / base64 / md5 / sha1 / sha256 / aes256 加密算法
- 添加跨线程传递异常的 exception_ptr
- 添加 terminate 操作
- 添加 scope_guard 操作
- 添加 ENV / INI / JSON / TOML 的 value / builder / parser 操作结构
- 添加 path / file_watcher / temp_file
- 添加 CRTP 静态多态接口，零内存开销地统一接口实现
- 添加 ranges 操作
- 添加位操作系列函数
- 添加内存视图 memory_view
- 添加弱智能指针 weak_ptr
- 添加数值信息类 numeric_limits
- 添加数学比率类 ratio
- 添加 UTF 转换系统
- 添加字符串格式化函数 format 和以 vsprints 为例的缓冲区格式化系列函数
- 添加进程参数解析类 cmdline
- 添加控制台操作类 sys_console 与唯一单例 console
- 添加环境变量操作类 environment
- 添加进程控制类 process
- 添加信号控制类 signal_manager
- 添加堆栈回溯类 stacktrace
- 添加持续时间 duration 系统与时钟系统
- 添加 RGBA 颜色操作类 color
- 添加基础类型包装系统 packages
- 添加标准 SQL 语句流式构建类 sql_builder
- 添加日志系统 log_event / log_formatter / log_sink / logger
- 添加 SSL/TLS 操作
- 添加 URL 操作
- 添加插件系统 plugin
- 添加测试资源文件

### 🔧 Improvements

- 通过 vcpkg 内存泄露分析
- 大幅优化项目结构，执行职责分离设计
- 大幅优化 database 结构设计
- 大幅优化 HTTP 结构设计，以 router / filter / server 结构代替 servlet
- 添加 future / promise / packaged_task 异步编程结构
- 实现 FUTEX / atomic / condition_variable / mutex / thread / stop_token 异步编程工具
- 优化 exception 实现结构
- 优化哈希函数实现结构
- 健全类型萃取结构 type_traits

### 📚 Documentation

- 添加英文 README

### 🐛 Bug Fixes

- 修复 deque 的内存泄漏问题

## [0.3.0] - 2025-08-28

### 🚀 New Features

- 全面支持 Linux
- 添加 db interface 并新增支持 MySQL / SQLite3 / Redis 的数据库连接池
- 添加存储任意类型的类型擦除类 any
- 添加可扩展大小的位操作类 bitmap
- 添加 date / time / datetime / timestamp 日期操作系统
- 添加莱昂纳多堆算法 leonardo_heap
- 添加文件操作类 file
- 添加 invoke / apply
- 添加十六进制操作类 hexadecimal
- 添加 optional
- 添加 json_parser / json_builder 结构
- 添加 print 打印函数以代替 detailof 打印函数
- 添加 LCD / Mersenne Twister / hardware Noise 随机算法
- 添加字符串转数据类型系列函数
- 添加基于原子操作的无锁队列 lock_free_queue
- 添加定时任务执行器 timer
- 添加 socket 包装
- 添加 HTTP session / cookies / filter 与 servlet 服务器
- 添加 DNS 客户端

### 🔧 Improvements

- 优化 function 实现结构
- 优化仿函数的实现结构
- 优化各容器的将亡值操作
- 健全数学库实现
- 健全 uninitialized 函数实现
- 健全 standard_allocator 实现
- 健全 basic_stringstream 实现
- 健全类型萃取结构 type_traits
- 健全线程池 thread_pool
- 优化项目结构

### 📚 Documentation

- 添加 README 编译指南

## [0.2.0] - 2025-03-08

### 🚀 New Features

- 添加 char_traits / basic_string_view / basic_stringstream
- 类型擦除的函数包装类 function 初步实现
- detailof 容器信息打印函数
- 适配 C++ 14 / 17 标准
- 更多的基本内存操作
- DEBUG 调试宏
- 添加并行算法
- 支持 C++17 类型推导
- 添加内存的就地构造与销毁操作
- 支持反向迭代器 reverse_iterator
- 添加常用数学常量及超越函数
- 添加标准内存分配器
- 添加独占指针 unique_ptr 与共享指针 shared_ptr
- 添加 MySQL 连接池
- 添加更多哈希与排序算法
- 添加基于 boost-stacktrack 的内存分配追踪器 trace_allocator
- 添加标准类型萃取系统 type_traits
- 添加空基类压缩对 compressed_pair
- 添加 variant

### 🔧 Improvements

- 将 string 改为支持任意字符类型的 basic_string
- 健全条件编译宏与编译器attributes
- 健全基本类型别名，适配32位系统
- 健全 pair / tuple 结构的实现
- 健全 concepts 结构
- 使用编译器attributes、constexpr与noexcept优化代码实现
- 使用 SFINAE 健全标准算法的实现
- 使用 DEBUG 调试宏健全容器的内存操作
- 使用 static_assert 限制容器的模板参数类型
- 标准化所有容器与配接器的包装类名与实现结构
- 删除基于 buddy system 的内存池
- 删除 object 结构
- 删除 depositary
- 删除仿函数配接器

### 📚 Documentation

- 添加 README 模块介绍

## [0.1.0] - 2024-12-17

### 🚀 New Features

- array / vector / list / deque / rbtree / hashtable 容器及其配接器 queue / stack 初步实现
- pair / tuple / depositary 工具初步实现
- 堆分配的 string 初步实现
- 异常系统 exception 初步实现
- 基本内存操作函数初步实现
- 基于 buddy system 的内存池的初步实现
- 线程池初步实现
- concepts 结构初步实现
- 通用可读类型名检测 check_type 实现
- object 结构实现
- 标准算法库 algo / algobase / numeric / heap 初步实现
- 仿函数系统初步实现
- 迭代器标签与萃取系统初步实现
- 使用 cmake 构建
