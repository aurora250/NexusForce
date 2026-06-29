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

| 维度        | NexusForce                         | Boost                              |
|-----------|------------------------------------|------------------------------------|
| 对 std 的态度 | **自研替代** —— 自行实现容器/智能指针/线程/原子等，再扩展 | **扩展/补充** —— 在 std 之上构建，补充标准库没有的功能 |
| 组件关系      | 组件间深度集成（反射→ORM、反射→序列化、反射→信号槽）      | 各组件相对独立，无跨库集成约定                    |
| 设计目标      | 统一风格、教学可读、生产可用                     | 极致的泛型编程与编译期能力                      |
| 代码体积      | 核心约 9 万行                           | 数千万行（仅头文件）                         |
| 学习门槛      | 中等（接近标准库的使用方式）                     | 高（大量模板元编程技巧）                       |
| 编译速度      | 较快（单个翻译单元）                         | 慢（深度模板实例化）                         |

**选择 NexusForce 的场景**：你需要一个风格统一、开箱即用的全栈 C++ 工具库，且希望源码可读性强、可作为学习参考。

**选择 Boost 的场景**：你需要特定功能（如 Boost.Graph、Boost.Spirit、Boost.Asio）且项目已有 Boost 依赖。

---

### Q: NexusForce 和 Qt 有什么区别？反射系统是否类似？

两者的反射系统确实有可比性，但整体定位不同：

| 维度       | NexusForce                                     | Qt                                 |
|----------|------------------------------------------------|------------------------------------|
| 范围       | **库**（你的代码调用它）                                 | **框架**（它的 main 函数调用你的代码）           |
| 反射机制     | NFRS 预编译扫描器 + 运行时注册表                           | MOC（Meta-Object Compiler）          |
| 代码生成触发   | CMake 构建阶段，增量扫描（文件修改时间缓存）                      | qmake/CMake AUTOMOC                |
| 反射应用     | ORM（sql_mapper + repository）、JSON/二进制序列化、动态信号槽 | 信号槽、QML 绑定、属性动画                    |
| ORM      | **反射驱动 auto-DDL/DML**                          | Qt SQL 模块（手动 SQL 或 QSqlTableModel） |
| HTTP 服务器 | HTTP/1.1 + HTTP/2 + WebSocket + 反向代理           | 需额外组件（Qt HTTP Server 较新）           |
| GUI      | 无（纯库）                                          | Qt Widgets / Qt Quick              |
| 许可证      | MIT                                            | LGPL/GPL/商用                        |

**关键差异**：NexusForce 的反射系统**不依赖任何预处理工具链之外的构建系统耦合**，仅需 CMake 集成即可工作。
Qt 的 MOC 需要完整的 Qt 构建体系。NexusForce 的反射直接驱动 ORM 和序列化系统。

**选择 NexusForce 的场景**：后端服务、网络中间件、数据库应用、CLI 工具——任何不需要 GUI 的场景。

**选择 Qt 的场景**：需要跨平台 GUI 的应用开发。

---

### Q: NexusForce 和 POCO 有什么区别？

POCO（POrtable COmponents）是另一套 C++ 工具库，在定位上比 Boost 更接近 NexusForce：

| 维度       | NexusForce                                    | POCO                          |
|----------|-----------------------------------------------|-------------------------------|
| 标准库态度    | 自研替代标准库组件                                     | 在 std 之上构建                    |
| 容器/内存/线程 | **全部自研**                                      | 使用标准库                         |
| HTTP 能力  | HTTP/1.1 + HTTP/2 + WebSocket + Radix Tree 路由 | HTTP/1.1（有路由但无 Radix Tree）    |
| ORM      | 反射驱动 auto-DDL/DML + repository                | 有 Data 模块（不如 NexusForce 深度集成） |
| 反射系统     | 完整（宏标记 + 代码生成 + 运行时注册表）                       | 无                             |
| 工程质量     | 0 漏洞 / 0 静态分析警告 / 0 内存泄漏                      | 良好                            |
| 更新活跃度    | 活跃（2026 年持续更新）                                | 较慢                            |

---

### Q: NexusForce 和 folly 有什么区别？

folly 是 Meta（Facebook）开源的 C++ 组件库，在性能追求上有相似之处：

| 维度     | NexusForce                                      | folly                            |
|--------|-------------------------------------------------|----------------------------------|
| 定位     | 通用全栈库                                           | 性能优化库（服务于 Facebook 基础设施）         |
| 标准库替代  | 完整替代（string/vector/map/smart ptr 全部自研）          | 补充/优化（FBString、F14 哈希表等优化版）      |
| 并发     | FUTEX、Hazard Ptr、Lock-Free Queue、Virtual Thread | Hazard Ptr、原子数据结构、协程（via wangle） |
| HTTP   | **内置完整 HTTP/1.1 + HTTP/2 + WebSocket**          | Proxygen（独立项目，较重）                |
| 反射/ORM | 完整                                              | 无                                |
| 跨平台    | Windows + Linux                                 | 以 Linux 为主（部分功能不支持 Windows）      |

**选择 NexusForce 的场景**：需要 Windows 支持、或需要完整的 HTTP/ORM/反射栈。

**选择 folly 的场景**：极致的 Linux 服务器端性能优化（且可以承受 Facebook 风格代码的复杂度）。

---

## 设计理念

### Q: 为什么要重新实现标准库组件，而不是基于 std 扩展？

这是 NexusForce 最根本的设计决策，原因有：

1. **跨平台行为一致性**：不同编译器/版本的行为不一致。自研实现保证 Windows MSVC / Linux GCC / Clang 的行为完全一致。

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

2. **反射系统集成**：HTTP 路由可以直接通过反射获取类型信息，实现自动的请求体反序列化。

3. **零依赖原则**：引入 Beast 会引入 Boost 整个依赖链（编译时间、二进制体积）。

4. **Event Loop 深度定制**：基于 Linux epoll 边缘触发 + min-heap 定时器，可根据自身需求做针对性优化。

5. **统一的内存管理**：使用 NexusForce 自己的 `shared_ptr`、`buffer_chain`（零拷贝链式缓冲区）、`byte_cursor`（无拷贝帧解析），与框架内存管理策略完全一致。

---

## 功能相关

### Q: ORM 的性能如何？和直接用 SQL 有多大差距？

NexusForce 的 ORM（`sql_mapper` + `repository`）采用**代码生成**而非运行时反射解释——`sql_mapper<T>` 在编译期生成 SQL 语句，运行时仅有参数绑定开销：

- DDL 生成（CREATE TABLE）发生在初始化阶段，不影响请求路径
- DML 生成在编译期完成模板实例化，运行时仅做字符串拼接（方言占位符替换）
- `repository` 的 CRUD 方法基于预处理语句，防止 SQL 注入的同时保证查询性能
- 与手写 SQL 的性能差异主要来自：类型安全的 getter 调用（约 1-2 个虚函数调用级别），在实际数据库 IO 场景下可忽略不计

### Q: 支持哪些数据库？如何添加新的数据库后端？

当前支持：
- **SQLite**（`sqlite_connect`）
- **MySQL**（`mysql_connect`）
- **PostgreSQL**（`pgsql_connect`）
- **Redis**（`redis_connect`）

添加新数据库后端的步骤：
1. 实现 `idb_connect` 接口（连接管理）
2. 实现 `idb_result` 接口（结果集封装）
3. 实现 `idb_prepared_statement` 接口（预处理语句）

---

### Q: NexusForce 适合用在哪些类型的项目中？

| 场景               | 推荐度   | 说明                                           |
|------------------|-------|----------------------------------------------|
| HTTP REST API 服务 | ⭐⭐⭐⭐⭐ | HTTP/1.1 + HTTP/2，Radix Tree 路由，中间件链，SSL/TLS |
| WebSocket 实时通信   | ⭐⭐⭐⭐⭐ | 完整 RFC 6455/7692 支持，permessage-deflate 压缩    |
| 数据库应用 / 数据持久化    | ⭐⭐⭐⭐⭐ | 反射驱动 ORM，连接池，预处理语句，方言感知 SQL                  |
| 网络中间件 / 代理       | ⭐⭐⭐⭐⭐ | 反向代理，负载均衡，速率限制，健康检查                          |
| CLI 工具 / 系统工具    | ⭐⭐⭐⭐  | cmdline 解析，console 交互，进程管理，管道，信号             |
| 配置管理 / 文件处理      | ⭐⭐⭐⭐  | 5 种配置格式，文件监控，异步 IO，路径操作                      |
| 加密/安全相关          | ⭐⭐⭐⭐  | AES-256/SHA-256 自研实现，SSL/TLS 集成，CSRF/CORS    |
| 嵌入式系统            | ⭐⭐⭐   | 必须依赖 OpenSSL/PCRE2，对资源受限环境需评估                |
| GUI 应用           | ⭐     | 无 GUI 组件，纯库定位                                |

---

### Q: NexusForce 的并发模型是怎样的？

NexusForce 提供多层次的并发支持：

1. **线程池**（`thread_pool`）：基于任务窃取（work stealing）的多策略线程池，适合 CPU 密集型并行任务。

2. **事件循环**（`event_loop`）：Linux epoll 边缘触发 + min-heap 定时器，适合 IO 密集型场景（HTTP 服务器、WebSocket）。

3. **协程**（coroutine `generator` / `task`）：C++20 协程原语。

4. **虚拟线程**（`virtual_thread`）：C# 风格的轻量级协程。

5. **传统线程**：`thread`、`scope_thread`（携带停止令牌）、`lazy_thread`（手动开启）。

选择建议：
- CPU 密集型 → `thread_pool`
- IO 密集型（网络服务） → `event_loop`
- 大量并发任务 → `virtual_thread` 或协程
- 简单并发 → `thread` / `async` / `future`

---

### Q: NexusForce 的代码质量和安全性如何？

NexusForce 追求 C++ 项目的工程质量极致：

| 检查类型                | 工具                                      | 结果                |
|---------------------|-----------------------------------------|-------------------|
| 安全漏洞                | CodeQL `security-and-quality` 全规则集      | **0 漏洞**          |
| 静态分析                | Clang-Tidy 全量规则集                        | **零警告**           |
| 动态内存检查              | Valgrind 全量测试                           | **0 泄漏，0 越界访问**   |
| 代码风格                | Clang-Format 19.0（120 列、4 空格、K&R 变体大括号） | **强制统一**          |
| C++ Core Guidelines | Clang-Tidy `cppcoreguidelines` 规则       | **已检查通过**         |
| 测试规模                | GTest 单元测试 + 集成测试                       | **6 万+ 行测试代码**    |

约 60 项 Clang-Tidy 显式豁免，均针对底层系统编程的固有需求（如 `reinterpret_cast` 在内存映射读写中的合理使用），遵循"默认严格，按需放开"原则。

---

## 实际使用

### Q: 如何开始使用 NexusForce？

最小化步骤：

```bash
# 1. 克隆
git clone --depth 1 https://github.com/aurora250/NexusForce.git
cd NexusForce

# 2. 构建
mkdir build && cd build
cmake ..     # Linux
# 或
cmake .. -G "Visual Studio 17 2022" -A x64  # Windows

cmake --build . --config Release

# 3. 安装
cmake --install . --config Release
```

在你的 CMake 项目中：

```cmake
find_package(NexusForce REQUIRED)
target_link_libraries(your_target PRIVATE NeForce::NexusForce)
```

详见 [QUICK_START.md](QUICK_START.md) 和 [examples](examples) 目录。

---

### Q: 兼容哪些编译器和平台？

| 平台                | 编译器                       | 状态      |
|-------------------|---------------------------|---------|
| Windows 10+ (x64) | MSVC (Visual Studio 2022) | ✅ 完全支持  |
| Windows 10+ (x64) | LLVM-Clang / ClangCL      | ✅ 完全支持  |
| Linux (x64)       | GCC                       | ✅ 完全支持  |
| Linux (x64)       | LLVM-Clang                | ✅ 完全支持  |
| MinGW             | -                         | ❌ 不兼容   |
| ARM / ARM64       | -                         | ❌ 当前不支持 |

C++ 标准：兼容 C++14 / 17 / 20。

---

### Q: NexusForce 的 ABI 稳定性承诺是什么？

V1.0.0-rc 起，**核心 ABI 已固定**。这意味着在后续的 1.x 版本中：

- 公有 API（类布局、函数签名）保持二进制兼容
- 补丁版本（1.0.x）不会引入 ABI 变更
- 大版本（2.0.0）之前不会移除已标记为公有 API 的符号

---

### Q: 项目依赖哪些第三方库？

| 依赖       | 用途             | 是否必选    |
|----------|----------------|---------|
| GTest    | 单元测试（仅测试编译）    | 构建测试时必选 |
| PCRE2    | 正则表达式          | **必选**  |
| OpenSSL  | SSL/TLS 加密     | **必选**  |
| libpq    | PostgreSQL 客户端 | 可选      |
| libmysql | MySQL 客户端      | 可选      |
| sqlite3  | SQLite 嵌入式数据库  | 可选      |
| hiredis  | Redis 客户端      | 可选      |
| lz4      | LZ4 高速压缩       | 可选      |
| zlib     | Zlib 通用压缩      | 可选      |

仅 3 个必选依赖（GTest 仅测试时需要），可选依赖通过 CMake 选项控制（如 `-DNEXUSFORCE_SUPPORT_SQLITE3=ON`）。

---

### Q: 可以只使用部分组件吗？

可以。NexusForce 的模块之间具有清晰的依赖边界：

- `core/container`、`core/memory`、`core/utility`、`core/string` 等核心模块可独立使用
- `core/reflect` 需依赖核心模块
- `db` 模块需依赖 `core/reflect` 和 `core/string`
- `network` 模块需依赖 `core/async`（event_loop）和 `core/string`
- 编译时可通过 CMake 选项裁剪不必要的数据库后端

---

## 技术深度 FAQ

### Q: `hazard_ptr`（危险指针）是如何工作的？

Hazard Pointer 是一种无锁编程中的安全内存回收（SMR）机制。核心思想：

1. **每个线程持有一个"危险指针"**（`hazard_pointer_record`），当线程要访问某个共享对象时，先将其地址写入自己的 hazard_ptr（memory_order_release），再读取对象。此时即便其他线程删除了该对象，回收器发现 hazard_ptr 仍指向它，就会延迟释放。

```
读取线程:                           回收线程:
hazard_ptr.store(ptr, release)      retire(ptr)
// 屏障                          扫描所有线程的 hazard_ptr
obj = *ptr                         if ptr 不在任何 hazard_ptr 中:
hazard_ptr.store(null, release)        delete ptr
                                   else:
                                       加入待回收列表（稍后重试）
```

2. **`hazard_pointer_domain`** 管理全局的 hazard pointer 记录链表，`hazard_pointer_obj_base` 是所有可通过 hazard pointer 安全回收的对象的基类。

3. 这解决了两个核心问题：
   - **ABA 问题**：CAS 操作前对象被释放又被重新分配，指针值相同但内容已变。hazard pointer 保证被保护的指针在保护期间不会被释放。
   - **use-after-free**：线程读取节点时，节点被另一线程删除。hazard pointer 延迟释放直到没有线程持有该指针。

---

### Q: FUTEX 是什么？NexusForce 如何做跨平台 FUTEX 封装？

**FUTEX**（Fast Userspace muTEX）是 Linux 内核提供的轻量级同步机制。核心洞察：大多数锁竞争不会发生，无需每次都陷入内核。

**工作流程**：
```
加锁（fast path，用户态）:
  atomic CAS: 0 → 1  成功 → 直接获取锁，无系统调用

等锁（slow path，内核态）:
  atomic CAS 失败 → futex(FUTEX_WAIT) → 内核挂起线程

解锁:
  atomic store: 1 → 0 → futex(FUTEX_WAKE) → 唤醒等待者
```

**跨平台实现**：

| 操作   | Linux                           | Windows                                        |
|------|---------------------------------|------------------------------------------------|
| 等待   | `futex(FUTEX_WAIT)`             | `WaitOnAddress()`                              |
| 唤醒   | `futex(FUTEX_WAKE)`             | `WakeByAddressSingle()` / `WakeByAddressAll()` |
| 定时等待 | `futex(FUTEX_WAIT_BITSET)` + 超时 | `WaitOnAddress()` + 超时                         |

NexusForce 的 `futex` 枚举映射了 Linux FUTEX 的全部 13 种操作码（`WAIT`/`WAKE`/`REQUEUE`/`CMP_REQUEUE`/`WAKE_OP`/`LOCK_PI`/`UNLOCK_PI` 等），加上 `PRIVATE_FLAG` 用于进程内优化。

**`atomic_futex`** 在此基础上实现了一个完整的用户态互斥锁模板（`WaiterBit` 默认为 `0x80000000`）：
- `wait_until(operand, equal)` —— 等待直到值不等于（或等于）operand
- `load_and_test_until` —— 循环中设置 WaiterBit → futex_wait → 重新检查，防范假唤醒
- 同时支持 `system_clock` 和 `steady_clock` 两种超时（后者不受系统时间调整影响）

---

### Q: `lock_free_queue`（无锁队列）是如何实现线程安全的？

NexusForce 的无锁队列采用**多生产者-多消费者**（MPMC）设计，核心思路是**环形缓冲区 + 原子 CAS 序号抢占**：

1. **环形缓冲区**：底层是一个固定大小的 slot 数组，每个 slot 带有一个原子 turn 序号。生产者/消费者通过原子索引（`tail_`/`head_`）竞争写入/读取位置。

2. **隐式生产者**：每个线程在首次 `try_push` 时自动注册为"隐式生产者"（`thread_exit_listener`），线程退出时通过 `thread_local` 对象析构自动清理，防止死线程占用 slot。

3. **核心技巧**：
   - `circular_less_than(a, b)` —— 通过有符号差值比较处理 uint64_t 回绕
   - `ceil_to_pow_2(x)` —— 保证容量为 2 的幂，使取模变为位运算
   - `align_for<T>` —— 确保每个 slot 对齐到 `alignment_of<T>`，避免 false sharing

4. **性能特性**：无锁意味着没有 mutex 的上下文切换开销，在高竞争场景下吞吐量远超有锁队列。代价是内存序（`memory_order_acquire`/`release`）需要仔细编排以防止 data race。

---

### Q: `bloom_filter`（布隆过滤器）的数学原理是什么？

布隆过滤器是一个概率性集合成员查询结构，由 m 位的位数组和 k 个哈希函数组成：

**核心保证**：
- **假阴性（漏报）**：不可能——已插入元素一定返回 true
- **假阳性（误报）**：可能——未插入元素可能返回 true

**最优参数推导**（给定预期 n 个元素，目标误报率 p）：
```
m = -n × ln(p) / (ln 2)²   （最优位数组大小）
k = (m / n) × ln 2         （最优哈希函数数量）
```

**双哈希优化**（Kirsch & Mitzenmacher 2006）：仅计算两个独立哈希 h₁(x) 和 h₂(x)，通过线性组合生成 k 个哈希值：
```
g_i(x) = (h₁(x) + i × h₂(x)) mod m,   i = 0..k-1
```
要求 h₂(x) 与 m 互质——通过确保 h₂(x) 为奇数来实现。理论证明这与 k 个独立哈希函数具有相同的渐近性能。

**运行中的估算**（设位数组中 1 的比例为 x）：
```
当前误报率 ≈ x^k
当前元素数 ≈ -(m/k) × ln(1 - x)
```

布隆过滤器被设计用于**缓存穿透防护、LSM-Tree 读放大削减、URL 去重、垃圾邮件过滤**等空间敏感且可容忍少量误报的场景。

---

### Q: `radix_router`（Radix Tree 路由）相比正则路由有什么优势？

NexusForce 的 HTTP 路由器使用**段式压缩前缀树（Segment-based Trie）**，将 URL 路径按 `/` 分割为段，每段作为一个 Trie 节点：

```
节点结构:
  segment          — 当前段名
  children         — 静态段 → 子节点索引 (unordered_map)
  param_index      — :param 子节点索引（如 /users/:id）
  wildcard_index   — * 通配符子节点索引（如 /static/*）
  handler          — 路由处理器
```

**匹配优先级**：静态段 > `:param` > `*` 通配符。未匹配时回退到正则路由。

**对比正则路由**：

| 维度         | Radix Tree           | 正则路由                      |
|------------|----------------------|---------------------------|
| 时间复杂度      | O(k)，k = 路径深度        | O(n × r)，n = 路由数，r = 正则回溯 |
| 内存布局       | 紧凑的 segment→index 映射 | 每个路由一个编译后的正则对象            |
| 1000 条路由匹配 | ~3-5 次哈希查找           | 可能回溯数千次                   |
| 插入开销       | O(k)                 | 正则编译 O(正则长度)              |
| 可预测性       | 确定性的，无回溯             | 依赖正则引擎实现                  |

Radix Tree 路由适合 API 网关和微服务场景（大量精确路由 + 少量参数路由），正则路由适合复杂模式匹配。

---

### Q: `byte_cursor` 解决了协议解析中的什么问题？

`byte_cursor` 是一个**带边界检查的字节级游标**，专为协议解析设计。被 HPACK 解码器、HTTP/2 帧解析器、WebSocket 帧解析器共用。

**设计要点**：

1. **边界安全**：每次读取操作（`try_read_byte`、`try_read_uint16`、`try_read_bytes`）返回 `optional<T>`，数据不足时返回 `none`。消除了手动指针运算中最常见的越界读取 bug。

2. **位级缓冲**：内部维护 `bit_buf_`（64 位）和 `bits_in_buf_`（缓存位数），支持跨字节边界的位读取——这在 HPACK Huffman 解码和 HTTP/2 帧解析中至关重要。例如读取 5 位 Huffman 前缀码时，不必手动管理字节边界。

3. **零拷贝**：游标不拥有数据，仅持有 `const byte_t*` + `size_t` + `pos_`。可以从 `cbyte_view` 直接构造，适合在已有缓冲区上叠加解析。

4. **一次性协议解析器**：当解析失败时，游标状态不变，可以丢弃后重试或被其他解析路径复用。

---

### Q: `buffer_chain` 的零拷贝原理是什么？

`buffer_chain` 是一个**链式缓冲区**，将多个内存块链接在一起而不执行实际拷贝：

```
append("HTTP/1.1 200\r\n")
append(header_block)       ← 指向已有的 header 内存，不拷贝
append("\r\n")
append(body_block)         ← 指向已有的 body 内存，不拷贝
     ↓
segments_: [{data→"HTTP/1.1 200\r\n", size=15},
            {data→header_block, size=256},
            {data→"\r\n", size=2},
            {data→body_block, size=4096}]
total_size_: 4369
```

当真正需要输出时，有两个选项：
- **`flatten()`** —— 分配到单一 `string` 中（此时才发生拷贝）
- **`to_iovec()`** —— 构建 POSIX `struct iovec` 数组，直接传给 `writev()` 系统调用，由内核完成分散-聚集 IO（scatter-gather I/O），**全程零拷贝**

这在 HTTP 响应组装中极为关键：响应行 + 多个头部 + 空行 + 正文来自不同内存位置，buffer_chain 避免将它们合并拷贝的 O(n) 开销。

---

### Q: Leonardo 堆和平滑排序（Smoothsort）有什么特别之处？

Leonardo 堆是 Edsger W. Dijkstra 于 1981 年设计的数据结构，专门用于**平滑排序**（Smoothsort）。

**Leonardo 数**（Dijkstra 为排序专门设计的数列）：
```
L(0)=1, L(1)=1, L(n)=L(n-1)+L(n-2)+1
序列: 1, 1, 3, 5, 9, 15, 25, 41, 67, ...
闭式: L(n) = 2×F(n+1) - 1（F 为 Fibonacci 数）
```

**堆结构**：每棵 Leonardo 树的大小的根节点左子树大小为 L(n-1)，右子树大小为 L(n-2)，自然形成"近似 AVL 树"的平衡结构。

**核心优势 — 自适应 O(n) 最优情况**：当输入接近有序时，平滑排序的复杂度趋近于 O(n)。这与其他排序算法不同：快速排序在近乎有序时退化到 O(n²)（非随机 pivot），堆排序始终 O(n log n) 不降级。平滑排序是极少数在近乎有序输入上能做到 O(n) 的比较排序。

NexusForce 实现的 `intro_sort`（内省排序）在递归深度过大时，从快速排序切换到堆排序。而 `smooth_sort` 直接使用 Leonardo 堆提供天然的"自适应"行为。

---

### Q: `compressed_pair` 是如何利用 EBCO 优化内存的？

`compressed_pair<IfEmpty, T>` 的核心技巧是**空基类优化**（Empty Base Class Optimization, EBCO）的模板化应用：

```cpp
// 当 IfEmpty 是空类且非 final 时：
template <typename IfEmpty, typename T, bool Compressed = is_empty_v<IfEmpty> && !is_final_v<IfEmpty>>
struct compressed_pair final : IfEmpty, icommon<...> {
    T value;  // 仅存储 T，IfEmpty 零开销
};
```

**压缩条件**：
- `is_empty_v<IfEmpty>` —— IfEmpty 无成员变量（如无状态比较器、无捕获 lambda）
- `!is_final_v<IfEmpty>` —— 不能是 final 类（final 类不可继承）

**内存效果对比**：
```
struct normal_pair<Empty, int> {   // sizeof = 8 (padding)
    Empty e;  // sizeof=1（C++ 不允许零尺寸对象）
    int v;    // sizeof=4, align=4 → 从 offset 4 开始
};

compressed_pair<Empty, int> {      // sizeof = 4
    int v;    // 空基类 Empty 占 0 字节
};
```

NexusForce 的 `scope_exit` 使用 `compressed_pair<Func, bool>` 同时存储回调函数和活跃标志，无额外内存开销。`memory_view` 使用 `compressed_pair` 存储数据和范围信息。

---

### Q: LRU Cache 的 O(1) 访问是如何实现的？

NexusForce 的 `lru_cache<Key, Value>` 通过两个数据结构配合实现 O(1) 的插入和访问：

```
list<pair<Key, Value>>          — 双向链表（头部=最近使用，尾部=最久未使用）
unordered_map<Key, list_iterator> — 键→链表节点的 O(1) 映射
unordered_map<Key, time_point>   — 键→最后访问时间（过期检测）
```

**操作流程**：
```
put(key, value):
  若 key 已存在 → 更新值并移到链表头部（O(1)）
  若缓存满 → 移除链表尾部节点（最久未使用），同时从 map 中删除（O(1)）
  在链表头部插入新节点，map 中记录迭代器（O(1)）

get(key):
  从 map 查找迭代器（O(1)）
  移动到链表头部（O(1)）
  返回 value
```

**`ttl_cache`** 在此基础上叠加了三个特性：
- **独立过期时间**：每个 entry 携带 `time_point expiry`
- **三种刷新策略**：
  - `never` —— 保持原始过期时间
  - `on_access` —— 每次访问重置过期时间
  - `sliding_window` —— 每次访问延长 TTL
- **后台清理线程**：定时扫描并清除过期项，清理间隔可配置

---

### Q: `event_loop` 的 epoll 边缘触发和 min-heap 定时器是如何配合的？

`event_loop` 是 Reactor 模式的核心，混合了 I/O 多路复用和定时调度：

**I/O 多路复用**：
```
epoll_create → epoll_ctl(ADD, fd, EPOLLIN|EPOLLET) → epoll_wait(timeout)
```
- **边缘触发**（EPOLLET）：仅在状态变化时通知一次，要求回调中必须循环 `read()` 直到 `EAGAIN`，避免事件丢失
- timeout 由 `next_timer_deadline()` 动态计算：取 min-heap 堆顶定时器的到期时间差

**定时器管理**（min-heap）：
```
timer_entry { id, deadline_ms, callback }
timer_heap_ = min-heap<timer_entry>  ← 堆顶 = 最早到期
```
- `schedule_timer(delay_ms, callback)` → push 入堆 → 若新定时器更早到期则 `wake()`
- `process_timers()` → 弹出所有 `deadline_ms <= now` 的 entry 并执行回调
- 定时器 ID 单调递增，支持 `cancel_timer(id)` 取消

**跨平台差异**：

| 操作     | Linux          | Windows                    |
|--------|----------------|----------------------------|
| I/O 监控 | `epoll_wait()` | IOCP（I/O Completion Port）  |
| 唤醒     | `eventfd`      | 手动重置事件（manual-reset event） |

---

### Q: `virtual_thread` 和操作系统线程的本质区别是什么？

`virtual_thread<T>` 是 C++20 协程（stackless coroutine）上的抽象层，不是真正的"虚拟线程"（green thread / stackful coroutine）。其核心结构：

```
task_shared_state<T>:
  aligned_buffer<T> result_buffer_  — 返回值存储（SBO 优化）
  exception_ptr exception_          — 异常传播
  atomic<coroutine_handle<>> continuation_ — 等待者 continuation
  atomic<bool> completed_           — 任务完成标志
  mutex + condition_variable        — 阻塞等待 get_result() 的支持
  atomic<unsigned long> ref_count_  — 引用计数（多等待者安全）
```

**与 OS 线程的关键区别**：

| 维度   | `virtual_thread`       | `std::thread`           |
|------|------------------------|-------------------------|
| 底层机制 | C++20 协程（编译器状态机）       | 操作系统内核线程                |
| 栈    | 无独立栈（无栈协程）             | 1MB+ 内核栈                |
| 切换开销 | ~函数调用级别                | ~微秒级（系统调用 + 上下文切换）      |
| 创建数量 | 百万级                    | 数千级                     |
| 抢占   | 协作式（co_await/co_yield） | 抢占式（内核调度）               |
| 阻塞操作 | `get_result()` 阻塞等待线程  | `thread::join()` 阻塞等待线程 |

`virtual_thread` 适合 IO 密集型的"大量等待"场景：网络请求、数据库查询等。CPU 密集型任务应使用 `thread_pool`。

---

### Q: `memory_view` 对比 `std::span` 有什么差异？

`memory_view<T, Extent>` 是 NexusForce 的非拥有内存视图，与 C++20 `std::span` 类似但有一些差异：

```
memory_view<T>               — 动态范围（extent = dynamic_extent）
memory_view<T, N>            — 静态范围（N 在编译期确定）
```

**内部设计**：
- `extent_storage<Extent>` —— 静态范围版本在编译期确定大小，不存储 size 成员（零开销）；动态范围版本存储 `size_t extent_value_`
- 使用 `compressed_pair` 存储指针和 extent_storage，减少 padding

**与 `std::span` 的差异**：

| 维度        | `memory_view`                                        | `std::span`   |
|-----------|------------------------------------------------------|---------------|
| 元素类型      | `T`（支持 `void`/`const void` 原始字节视图 `cbyte_view`）      | `T`（不支持 void） |
| extent 实现 | 特化 `extent_storage<dynamic_extent>`                  | 类似特化          |
| 迭代器       | NexusForce 自研 `normal_iterator` + `reverse_iterator` | 标准迭代器         |
| 集成        | 与 `byte_cursor` / `buffer_chain` / 反射系统互操作           | 标准库生态         |

---

### Q: Sparse 容器（`sparse_set`/`sparse_map`）的权衡是什么？

NexusForce 的 sparse 系列容器基于**有序 vector + 二分查找**：

```
sparse_vector<T>:
  底层: vector<pair<Key, T>>（始终有序）
  查找: binary_search → O(log n)
  插入: binary_search + 元素右移 → O(n)
  迭代: 连续内存遍历 → O(1), cache-friendly
```

**与传统关联容器对比**：

| 维度   | `sparse_set`                 | `set`（rb_tree）  | `unordered_set` |
|------|------------------------------|-----------------|-----------------|
| 查找   | O(log n)                     | O(log n)        | O(1) 平均         |
| 插入   | O(n)（移动元素）                   | O(log n)        | O(1) 平均         |
| 迭代   | **O(1)，连续内存，cache-friendly** | O(1)，指针追逐       | O(1)，跳跃遍历       |
| 内存   | **最小**（仅 vector 开销）          | 每个节点 3 个指针 + 颜色 | 桶数组+链表节点        |
| 最佳场景 | 少量插入、大量遍历                    | 频繁插入/删除         |  纯 O(1) 查找      |

Sparse 容器在"构建一次、查询多次、频繁迭代"的场景（配置表、查找表、索引）中是最佳选择。两个插入策略：
- `insert_unique` —— 键必须唯一，重复键返回已有元素迭代器
- `insert_equal` —— 允许重复键（等价于 `sparse_multiset`/`sparse_multimap`）

---

### Q: `int128_t`/`uint128_t` 的运算如何实现？

由于没有硬件原生支持 128 位运算，`int128_t`/`uint128_t` 用软件模拟：

```
struct uint128_t {
    uint64_t lo;  // 低 64 位
    uint64_t hi;  // 高 64 位
};
```

**加法**（带进位传播）：
```
lo = a.lo + b.lo         → 可能溢出，产生 carry
hi = a.hi + b.hi + carry → 进位传播到高 64 位
```

**乘法**（分治）：
```
将 128 位拆为两个 64 位 × 64 位 → 128 位的乘积，再累加
```

**除法**：使用长除法（shift-and-subtract）模拟，是 128 位运算中最昂贵的操作。

实现 `iarithmetic<T>`、`ibinary<T>`、`iobject<T>` 等 CRTP 接口，支持完整的运算符重载（+/-/\*/\/\%/<<\>>/&\|^/\~）和与内置整数类型的隐式构造。

---

### Q: `scope_exit` 的构造函数为什么有两个重载？

`scope_exit<Func>` 的构造函数通过 SFINAE 分派到两个版本：

1. **异常不安全版本**（`is_nothrow_constructible_v<Func, F> == false`）：使用 function-try-block，若 Func 构造抛异常，立即执行 `func()` 进行清理。

2. **异常安全版本**（`is_nothrow_constructible_v<Func, F> == true`）：标记 `noexcept`，无 try-catch 开销。

两个版本均使用 `exact_arg_construct_tag` 和 `_NEFORCE forward<F>(func)`，确保完美转发且不触发隐式转换。激活标志 `bool{true}` 与 Func 一起存储在 `compressed_pair` 中。

三个守卫的语义：

| 守卫              | 触发条件         | 典型用途                   |
|-----------------|--------------|------------------------|
| `scope_exit`    | 作用域退出（正常+异常） | 资源释放（fd close、内存 free） |
| `scope_fail`    | 仅异常退出        | 事务回滚、状态恢复              |
| `scope_success` | 仅正常退出        | 提交操作、日志记录              |

---

### Q: `buffer_chain` 和 `byte_cursor` 在 HTTP 响应路径中如何协作？

以 HTTP/1.1 响应发送为例，两者在"读"和"写"两端各司其职：

**写入端 — 响应组装**（`buffer_chain`）：
```cpp
buffer_chain response;
response.append("HTTP/1.1 200 OK\r\n");      // 状态行
response.append("Content-Type: text/html\r\n"); // 头
response.append("\r\n");                        // 空行
response.append(body.data(), body.size());      // 正文（body 可能在另一个缓冲区）
// response 仅持有指针，尚未拷贝任何数据

// POSIX 零拷贝输出：
struct iovec* iov = response.to_iovec();
writev(fd, iov, iov_count);  // 内核直接 scatter-gather
```

**读取端 — 协议解析**（`byte_cursor`）：
```cpp
byte_cursor cursor(received_data, len);
auto b1 = cursor.try_read_byte();       // 安全读取一个字节
auto u16 = cursor.try_read_uint16();    // 安全读取 2 字节（大端）
auto payload = cursor.try_read_bytes(n); // 安全读取 n 字节
if (!payload) { /* 数据不足，等待更多数据 */ }
```

在 WebSocket 帧解析中，`byte_cursor` 的位级缓冲可直接读取 4 位 opcode、1 位 mask、7 位 payload length 等非字节对齐字段。

---

*本文档持续更新。如有未覆盖的问题，欢迎提交 [Issue](https://github.com/aurora250/NexusForce/issues)。*
