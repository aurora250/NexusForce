# NexusForce Q&A

## 项目定位

### Q: NexusForce 是什么？它解决了什么问题？

NexusForce 是一个**功能健全、风格统一、跨平台兼容**的现代 C++ 开发库。
它的核心定位是"标准库超集"——自行实现 C++ 标准库的核心组件（容器、智能指针、线程、原子操作等），
并在此基础上扩展了 HTTP 服务器、数据库 ORM、反射系统、配置解析、加密算法等生产级模块。

它解决的核心问题：
1. **依赖碎片化**：C++ 生态中，网络库、数据库库、配置解析库、日志库通常来自不同项目，API 风格、内存管理策略、错误处理方式各不相同。NexusForce 提供一套统一风格的"一站式"方案。
2. **标准库不可控**：不同平台/版本的 C++ 标准库实现存在差异（如 `std::string` 的 SSO 阈值、`std::regex` 的性能问题）。NexusForce 自行实现标准库组件，保证跨平台行为一致。
3. **学习与生产的断层**：NexusForce 的源码可直接用于学习标准库底层原理（红黑树、哈希表、FUTEX、内存模型），同时也是生产可用的工具库。

---

## 与其他框架的对比

### Q: NexusForce 和 Boost 有什么区别？是不是"又一个 Boost"？

**不是。** NexusForce 和 Boost 在哲学上截然不同：

| 维度          | NexusForce                                                | Boost                                                  |
|---------------|-----------------------------------------------------------|--------------------------------------------------------|
| 对 std 的态度 | **自研替代** —— 自行实现容器/智能指针/线程/原子等，再扩展 | **扩展/补充** —— 在 std 之上构建，补充标准库没有的功能 |
| 组件关系      | 组件间深度集成                                            | 各组件相对独立，无跨库集成约定                         |
| 设计目标      | 统一风格、生产可用、教学可读                              | 极致的泛型编程与编译期能力                             |
| 代码体积      | 核心约 28 万行（头文件 + 实现）                           | 数千万行（仅头文件）                                   |
| 学习门槛      | 中等（接近标准库的使用方式）                              | 高（大量模板元编程技巧）                               |
| 编译速度      | 较快（单个翻译单元）                                      | 慢（深度模板实例化）                                   |

**选择 NexusForce 的场景**：你需要一个风格统一、开箱即用的全栈 C++ 工具库，且希望源码可读性强、可作为学习参考。

**选择 Boost 的场景**：你需要特定功能（如 Boost.Graph、Boost.Spirit、Boost.Asio）且项目已有 Boost 依赖。

---

### Q: NexusForce 和 Qt 有什么区别？反射系统是否类似？

两者的反射系统确实有可比性，但整体定位不同：

| 维度          | NexusForce                                                     | Qt                                        |
|---------------|----------------------------------------------------------------|-------------------------------------------|
| 范围          | **库**（你的代码调用它）                                       | **框架**（它的 main 函数调用你的代码）    |
| 反射机制      | NFRS 预编译扫描器 + 运行时注册表                               | MOC（Meta-Object Compiler）               |
| 代码生成触发  | CMake 构建阶段，增量扫描（文件修改时间缓存）                   | qmake/CMake AUTOMOC                       |
| 反射应用      | ORM（sql_mapper + repository）、JSON/二进制序列化、动态信号槽  | 信号槽、QML 绑定、属性动画                |
| ORM           | **反射驱动 auto-DDL/DML**                                      | Qt SQL 模块（手动 SQL 或 QSqlTableModel） |
| HTTP 服务器   | HTTP/1.1 + HTTP/2 + WebSocket + 反向代理                       | 需额外组件（Qt HTTP Server 较新）         |
| GUI           | 无（纯库）                                                     | Qt Widgets / Qt Quick                     |
| 许可证        | MIT                                                            | LGPL/GPL/商用                             |

**关键差异**：NexusForce 的反射系统**不依赖任何预处理工具链之外的构建系统耦合**，仅需 CMake 集成即可工作。
Qt 的 MOC 需要完整的 Qt 构建体系。NexusForce 的反射直接驱动 ORM 和序列化系统。

**选择 NexusForce 的场景**：后端服务、网络中间件、数据库应用、CLI 工具——任何不需要 GUI 的场景。

**选择 Qt 的场景**：需要跨平台 GUI 的应用开发。

---

### Q: NexusForce 和 POCO 有什么区别？

POCO（POrtable COmponents）是另一套 C++ 工具库，在定位上比 Boost 更接近 NexusForce：

| 维度           | NexusForce                                        | POCO                              |
|----------------|---------------------------------------------------|-----------------------------------|
| 标准库态度     | 自研替代标准库组件                                | 在 std 之上构建                   |
| 容器/内存/线程 | **全部自研**                                      | 使用标准库                        |
| HTTP 能力      | HTTP/1.1 + HTTP/2 + WebSocket + Radix Tree 路由   | HTTP/1.1（有路由但无 Radix Tree） |
| ORM            | 反射驱动 auto-DDL/DML + repository                | 有 Data 模块                      |
| 反射系统       | 完整（宏标记 + 代码生成 + 运行时注册表）          | 无                                |

---

### Q: NexusForce 和 folly 有什么区别？

folly 是 Meta（Facebook）开源的 C++ 组件库，在性能追求上有相似之处：

| 维度       | NexusForce                                          | folly                                        |
|------------|-----------------------------------------------------|----------------------------------------------|
| 定位       | 通用全栈库                                          | 性能优化库（服务于 Facebook 基础设施）       |
| 标准库替代 | 完整替代（string/vector/map/smart ptr 全部自研）    | 补充/优化（FBString、F14 哈希表等优化版）    |
| 并发       | FUTEX、Hazard Ptr、Lock-Free Queue、Virtual Thread  | Hazard Ptr、原子数据结构、协程（via wangle） |
| HTTP       | **内置完整 HTTP/1.1 + HTTP/2 + WebSocket**          | Proxygen（独立项目，较重）                   |
| 反射/ORM   | 完整                                                | 无                                           |
| 跨平台     | Windows / Linux / FreeBSD（64 位）                  | 以 Linux 为主（部分功能不支持 Windows）      |

**选择 NexusForce 的场景**：需要 Windows 支持、或需要完整的 HTTP/ORM/反射栈。

**选择 folly 的场景**：极致的 Linux 服务器端性能优化（且可以承受 Facebook 风格代码的复杂度）。

---

## 设计理念

### Q: 为什么要重新实现标准库组件，而不是基于 std 扩展？

这是 NexusForce 最根本的设计决策，原因有：

1. **跨平台行为一致性**：不同编译器/版本的行为不一致。自研实现保证 Windows MSVC、Linux GCC/Clang、FreeBSD Clang/GCC 的行为完全一致。

2. **深度优化空间**：可以对特定场景做针对性优化。

3. **组件间深度集成**：自研的 `basic_string`、`vector`、`map` 可以直接暴露内部结构给反射系统、序列化器，无需通过外部适配层。

4. **教学价值**：源码即是标准库底层原理的"活教材"——红黑树、哈希表、FUTEX、内存序、类型擦除等均以可读性优先的风格实现。

5. **ABI 稳定性**：不依赖编译器的标准库 ABI，可以长期保持 ABI 兼容。

---

### Q: NexusForce 的字符串为什么不直接用 std::string？

NexusForce 的 `neforce::string` 基于 `basic_string<char>` 自研实现，比标准库版本提供：

- **完整的 Unicode 集成**：与 `codepoint`、`utf8_iterator`、`utf8_view` 无缝配合
- **Boyer-Moore-Horspool 子串搜索加速**
- **与格式化引擎深度集成**（`format` / `format_named` / `vsprintf`）
- **统一的 SSO 行为**
- **与反射/序列化系统的零开销互操作**

同时提供 `string_view` 避免不必要的拷贝。

---

### Q: NFRS 反射系统和 C++26 静态反射（P2996）是什么关系？

C++26 有望引入静态反射（`^` operator），但 NexusForce 现在就需要反射能力来驱动 ORM 和序列化：

- NFRS 宏（`NEFORCE_REFLECT_OBJ` / `PROP` / `FUNC` 等）是**编译期标记**，语义接近 C++26 的反射提案
- 当 C++26 静态反射成熟后，NFRS 可以作为**过渡方案**，逐步将底层实现迁移到标准反射，同时保持宏 API 不变
- 在 C++14/17/20 上，NFRS 是唯一能提供运行时反射的方案

---

### Q: HTTP 服务器为什么不用 Beast（Boost）或 libuv？

NexusForce 选择自研 HTTP 协议栈的原因：

1. **HTTP/2 的完整实现**：NexusForce 自研了完整的 HTTP/2 帧层与 HPACK 头部压缩（RFC 7540/7541）。

2. **协议栈自主可控**：HTTP/1.1、HTTP/2、WebSocket、gRPC 与反向代理共用同一套路由、中间件链和 `io_context`，Beast 只覆盖 HTTP/1.1/WebSocket，接入后仍需为 HTTP/2、gRPC 另找实现并统一两套回调模型。

3. **零依赖原则**：引入 Beast 会引入 Boost 整个依赖链（编译时间、二进制体积）。

4. **`io_context` 深度定制**：Linux epoll 边缘触发、FreeBSD poll、Windows IOCP 三套后端，定时器由 `heap_timer` 承担，可根据自身需求做针对性优化。

5. **统一的内存管理**：使用 NexusForce 自己的 `shared_ptr`、`buffer_chain`（零拷贝链式缓冲区）、`byte_cursor`（无拷贝帧解析），与框架内存管理策略完全一致。

---

## 功能相关

### Q: ORM 的性能如何？和直接用 SQL 有多大差距？

NexusForce 的 ORM（`sql_mapper` + `repository`）**不是编译期代码生成**：`sql_mapper<T>` 在运行时读取 reflect registry 中的 `meta_type`（属性列表、主键/自增/transient 注解、表名），用 `sql_builder` 拼出 DDL/DML；`repository` 再把 SQL 交给 `idb_connect::update()` / `query()` 执行。

- DDL（`create_table_sql()`）在需要时生成一次，可由调用方缓存，不落在请求路径上
- DML 每次调用都会遍历 `all_properties()` 并构造 SQL 字符串，开销是反射属性遍历（`meta_property::get()` 返回类型擦除的 `meta_any`）+ 字符串拼接，而不是"1-2 个虚函数调用"
- 需要反复执行同一语句时，用 `idb_connect::prepare_statement()` 返回的 `idb_prepared_statement`（MySQL/PostgreSQL/SQLite 均有实现）把解析与参数绑定从执行路径上拿掉
- 与手写 SQL 的差距主要就是上述字符串拼接与类型擦除转换；在真实数据库 IO 面前通常可以忽略

### Q: 支持哪些数据库？如何添加新的数据库后端？

当前支持：
- **SQLite**（`sqlite_connect`）
- **MySQL**（`mysql_connect`）
- **PostgreSQL**（`pgsql_connect`）
- **Redis**（`redis_connect`）

添加新数据库后端的步骤：
1. 实现 `idb_tb_connect` 或 `idb_kv_connect`（连接管理；SQL 型通常复用 `sql_connect_base`）
2. 实现 `idb_tb_result` / `idb_kv_result`（结果集封装）
3. 实现 `idb_prepared_statement`（预处理语句；不支持时返回 `nullptr` 或空实现）
4. 提供 `idb_factory` 子类，在 `create_connect()` / `create_result()` 中构造上述对象

---

### Q: NexusForce 适合用在哪些类型的项目中？

| 场景                    | 推荐度     | 说明                                                  |
|-------------------------|------------|-------------------------------------------------------|
| HTTP REST API 服务      | ⭐⭐⭐⭐⭐ | HTTP/1.1 + HTTP/2，Radix Tree 路由，中间件链，SSL/TLS |
| WebSocket 实时通信      | ⭐⭐⭐⭐⭐ | 完整 RFC 6455/7692 支持，permessage-deflate 压缩      |
| 数据库应用 / 数据持久化 | ⭐⭐⭐⭐⭐ | 反射驱动 ORM，连接池，预处理语句，方言感知 SQL        |
| 网络中间件 / 代理       | ⭐⭐⭐⭐⭐ | 反向代理，负载均衡，速率限制，健康检查                |
| CLI 工具 / 系统工具     | ⭐⭐⭐⭐   | cmdline 解析，console 交互，进程管理，管道，信号      |
| 配置管理 / 文件处理     | ⭐⭐⭐⭐   | 5 种配置格式，文件监控，异步 IO，路径操作             |
| 加密/安全相关           | ⭐⭐⭐⭐   | AES-256/SHA-256 自研实现，SSL/TLS 集成，CSRF/CORS     |
| 嵌入式系统              | ⭐⭐⭐     | 必须依赖 OpenSSL/PCRE2，对资源受限环境需评估          |
| GUI 应用                | ⭐         | 无 GUI 组件，纯库定位                                 |

---

### Q: NexusForce 的并发模型是怎样的？

NexusForce 提供多层次的并发支持：

1. **线程池**（`thread_pool`）：基于任务窃取（work stealing）的多策略线程池，适合 CPU 密集型并行任务。

2. **事件循环**（`io_context`）：跨平台事件循环（Linux epoll / FreeBSD poll / Windows IOCP）+ `heap_timer` 最小堆定时器，适合 IO 密集型场景（HTTP 服务器、WebSocket）。

3. **协程**（coroutine `generator` / `task`）：C++20 协程原语。

4. **虚拟线程**（`virtual_thread`）：C# 风格的轻量级协程。

5. **传统线程**：`thread`、`scope_thread`（携带停止令牌）、`lazy_thread`（手动开启）。

选择建议：
- CPU 密集型 → `thread_pool`
- IO 密集型（网络服务） → `io_context`
- 大量并发任务 → `virtual_thread` 或协程
- 简单并发 → `thread` / `async` / `future`

---

## 技术深度 FAQ

> 从实现深度与复用价值出发，挑了 10 个组件；每题只回答关键机制，不铺功能清单。

### Q: `memory_pool` 如何让通用分配接近无锁？

- 三层结构：线程本地缓存 → 36 个尺寸类的中央 span 堆 → 操作系统虚拟内存；不超过 16 KiB 的对象走小对象路径，块内不带任何元数据。
- 尺寸类与归属靠进程级 64 KiB 槽位地址映射表反查，线程缓存按批次搬运，绝大多数分配/释放不触碰中央锁。
- 大对象直接映射虚拟内存，映射参数记录在区域头部；链接 `NexusForceMemoryPoolOverrideStatic` 可让全进程替换 `operator new/delete`。

---

### Q: `flat_hashtable` 的 SwissTable 设计为什么快？

- 每个 slot 只占 1 字节元数据：`EMPTY=0x80`、`DELETED=0xFE`、有效标签 0–127 为 7-bit H2；控制块与数据数组分离。
- 查找先用 H2 过滤，标签命中才访问数据数组比较完整键；SSE2 一次装入 16 字节元数据，单指令得到 16 个候选 slot。
- 线性探测靠 2 的幂容量做位与，删除留 tombstone；同一实现支撑 `flat_unordered_map` / `flat_unordered_set`。

---

### Q: `thread_pool` 的工作窃取如何控制窃取成本？

- 每个 worker 一条 256 槽环形本地队列：新任务优先入本地队列，溢出才进全局无锁队列；优先级大于 0 的任务走独立的 `priority_queue`。
- 窃取策略可选 `half` / `fixed_batch` / `single` / `adaptive`（默认自适应批量），且同一时刻最多一半 worker 参与窃取，避免互相踩踏。
- NUMA 感知：worker 按 NUMA 节点轮转分配，跨节点仅在目标队列比本地候选深 4 个以上时才窃取；另有 CPU 亲和开关与带自旋守时窗口的定时调度器。

---

### Q: `io_context` 怎样把 I/O、定时器、跨线程投递统一到一个循环？

- 后端三选一：Linux epoll（默认边缘触发）、FreeBSD poll、Windows IOCP（辅以 WSA 事件线程）；等待时长取 `heap_timer` 堆顶并截断到 200 ms。
- 跨线程 `post()` 进入无锁队列，由循环批量 drain；`dispatch()` 在当前线程直接执行；`work` 守卫阻止循环提前退出。
- 定时器是惰性删除的最小堆：`cancel_timer()` 只打取消标记，到期取出时跳过，避免 O(n) 重排。
- `strand` 用 CAS 锁 + 无锁队列把回调串行化，协程、定时器与 socket 回调共用同一套调度。

---

### Q: HTTP/2 栈如何兼顾协议完整性与吞吐？

- HPACK 静态表 61 项 + 按 SETTINGS 上限淘汰的动态表，字符串支持 Huffman 编解码；CONTINUATION 帧会挂起并合并回 HEADERS。
- 连接级与流级双窗口流控：DATA 发送前扣窗口，WINDOW_UPDATE 归还；帧序与流状态由状态机校验。
- 读写都挂在 `io_context`：读事件内批量解析帧，写回在事件内合并，小响应延迟 flush，单次写体 ≥16 KiB 或积压 ≥1 MiB 才强制写出。

---

### Q: `virtual_thread` 的协程调度器与线程池是什么关系？

- `virtual_thread_task<T>` 是 stackless 协程，挂起不占线程栈；调度器单例以 `lock_free_queue<coroutine_handle<>>` 作为就绪队列。
- sleep 走 `heap_timer`，每轮最多批量转入 32 个到期协程；空闲 worker 停在 FUTEX 唤醒字上，`thread_local` 标记避免“自己唤醒自己”的惊群。
- `get_result()` 用 mutex + condition_variable 把异步结果桥接给同步调用者；协程之间用 `co_await` 直接传递 continuation。

---

### Q: NFRS 反射系统如何在没有编译器反射的情况下驱动 ORM？

- 宏（`NEFORCE_REFLECT_OBJ/PROP/FUNC/SIGNAL/ENUM`）在类体内只留标记；NFRS 扫描头文件，按 mtime 缓存增量生成注册 `.cpp` 并注入目标。
- 运行时 `registry` 以 `type_id` 为主键：`type_id` 是 `type_name` 特化字符串（或编译器函数签名）的 FNV-64 哈希；`meta_type` 保存属性/函数/构造/枚举/容器信息，值经 `meta_any` 类型擦除。
- `sql_mapper<T>` 读同一份元数据生成 DDL/DML，`repository` 执行；JSON/二进制序列化与动态信号槽也复用该元数据。

---

### Q: TUI 的差量渲染如何把整屏刷新压到微秒级？

- 组件树 → 元素树 → Flexbox/Gridbox 布局 → cell 网格；每帧只比较前后两块屏幕缓冲。
- 差量按字节扫描：行内用 16 字节 SSE 掩码找首个变化字节，行尾用 AVX2 一次跳过 128 字节相同区段再定位末个变化字节，只对变化区段生成 ANSI。
- `cell_text` 固定 8 字节保存单码点并缓存显示宽度，整屏比较是连续内存扫描；帧调度经 `strand` 合并，`headless` 后端支持离屏快照。

---

### Q: `lock_free_queue` 与 `hazard_ptr` 如何配合实现无锁容器？

- `lock_free_queue` 是 moodycamel 移植：每生产者一条 SPSC block 子队列，块由无锁 free list 复用；显式 token 与按线程 ID 哈希的隐式生产者并存，支持批量入队/出队。
- 消费者按轮转配额切换子队列，`try_dequeue` 选最长非空队；隐式生产者由 `thread_exit_notifier` 在退出时回收。
- 删除节点用 `hazard_ptr`：读侧先 `protect`，写侧 `retire` 到线程本地列表，攒到 100 个再扫描全部 hazard 记录（排序 + 二分），未被保护才析构。

---

### Q: `crash_dump` 如何在信号处理函数里安全写出转储？

- 统一捕获 POSIX 致命信号、未捕获 C++ 异常与 Windows SEH；写入只调用系统调用并使用静态缓冲区，不分配堆。
- Windows 走 `MiniDumpWriteDump`；Linux/FreeBSD x86-64/AArch64 自写 ELF core（PT_NOTE 放 `NT_PRSTATUS`，PT_LOAD 装可读段）。
- 文件名 `<prefix>-<日期>-<时间>-<pid>.<ext>`，同秒冲突追加序号；可通过回调取摘要，写入失败还能回退内核 core。

---

*本文档持续更新。如有未覆盖的问题，欢迎提交 [Issue](https://github.com/aurora250/NexusForce/issues)。*
