# CHANGELOG

## [1.0.0-rc] - 2026-06-09

### 🚀 New Features
- 添加 GoogleBenchmark 配置
- `sql_builder` 达到 ANSI SQL-92 Entry/Intermediate 级完整覆盖
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

### 🔧 Improvements
- `sql_builder` 扩展 API，覆盖 DML/DDL/视图/索引/CTE/窗口函数/集合操作
- `http_client` 重构为 scheme 感知模式：`scheme == "https"` 时自动创建 SSL 上下文
- `ssl_client::post_connect()` 无 SSL 上下文时跳过 TLS 握手，支持纯 TCP 模式
- `http_server` HTTPS 构造函数自动设置 ALPN 协议列表 (`h2`, `http/1.1`)
- `tcp_socket::receive()` 在非阻塞模式下 EAGAIN/EWOULDBLOCK 返回 0（不再抛异常）
- `websocket_server` 支持事件驱动模式 (`event_loop`) 与多线程模式切换
- `authentication_filter` 添加 `add_included_path()` 白名单模式
- 更新所有网络示例代码（http_server, https_server, http2_server, websocket_server 等）

### 🐛 Bug Fixes
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
- 修复浏览器 WebSocket permessage-deflate 压缩消息被拒绝 (PROTOCOL_ERROR 1002)：
  zlib `Z_FINISH` 在 raw deflate 下不保证追加空存储块，导致压缩输出不符合 RFC 7692 格式。
  修复方案：`websocket_deflate::process()` 中检测并手动追加空存储块（BFINAL=1, BTYPE=00），
  再剥离 4 字节 LEN+NLEN 尾部，确保输出始终以 `0x01` 结尾

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
