# CHANGELOG

## [1.0.0] - 2026-03-29

### 🚀 New Features
- 项目从 MSTL 重命名为 NexusForce
- 导出 cmake 配置函数
- 添加 lz4 压缩操作
- 添加线程屏障 barrier / latch
- 添加协程 coroutine / generator 支持
- 添加危险指针 hazard_ptr
- 添加信号量 semaphore
- 添加基于协程的虚拟线程 virtual_thread
- 添加布隆过滤器 bloom_filter
- 添加 lru_cache / ttl_cache
- 添加断点调用 breakpoint
- 添加端序操作 endian
- 添加 shared_ptr / weak_ptr 特化的 atomic 无锁操作
- 添加反射系统与反射注册表 registry
- 添加 Unicode 码点操作类 codepoint
- 添加 PCRE2[with JIT] 正则类 regex
- 添加系统管道操作类 pipe
- 添加共享内存类 share_memory
- 添加系统信息获取类 sysinfo
- 添加范围计时类 click
- 添加 scope 操作
- 添加 UUID v4 / v7 生成器
- 添加 ICMP / SMTP socket 操作
- 添加 ARP / MAC / IP / ports 操作
- 添加 Websockets 通信操作

### 🔧 Improvements
- 使用外部配置 cmake 选项
- 使用 vcpkg 包管理
- 大幅优化 network 结构设计
- 优化 FUTEX 设计结构
- 优化 call_once 设计
- 优化 path / file 设计，分离职责到子工具类
- 优化随机数生成器的结构设计
- 优化 UTF 转换操作实现
- 优化 formatter 实现
- 使用 none 统一工具类的空表示
- 优化 WinSock 初始化方式
- 优化数据库连接池实现
- 使用匿名命名空间优化编译单元内部实现

### 📚 Documentation
- 除 db 与 network 外的大部分 API 文档
- 优化 README 结构

### 🐛 Bug Fixes
- 修复 make_shared 内存泄漏问题
- 修复线程池 cached 模式下的临界区操作异常问题

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
- 添加持续时间 duraion 系统与时钟系统
- 添加 RGBA 颜色操作类 color
- 添加基础类型包装系统 packages
- 添加标准 SQL 语句流式构建类 sql_builder
- 添加日志系统 log_event / log_formatter / log_sink / logger
- 添加 SSL/TLS 操作
- 添加 URL 操作
- 添加插件系统 plugin
- 添加测试资源文件

### 🔧 Improvements
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
- 添加 print 打印函数
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
- 使用 cmake 代替 sin 进行跨平台构建，优化项目结构

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
- 添加常用数学常量及函数体系
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

## [0.1.0] - 2024-21-17

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
- sin 构建
