# CHANGELOG

## [1.0.1] - 2026-08-04

### 🐛 Bug Fixes

- 修复 NFRS 被安装后索引 Release 动态库的方案
- 修复 Clang 下推断 websocket-deflate 整形符号溢出与 GCC 不同的警告

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
