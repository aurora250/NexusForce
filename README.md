# NexusForce V1.0.0-rc

[![vcpkg](https://img.shields.io/badge/vcpkg-Enabled-0A7FAA?style=flat-square&logo=vcpkg&logoColor=white)](https://vcpkg.io)
[![CMake](https://img.shields.io/badge/CMake-3.19+-064C8B?style=flat-square&logo=cmake&logoColor=white)](https://cmake.org)
[![C++](https://img.shields.io/badge/C++-14/17/20-00599C?style=flat-square&logo=cplusplus&logoColor=white)](https://isocpp.org)
[![CodeQL](https://github.com/aurora250/NexusForce/workflows/CodeQL%20Analysis/badge.svg)](https://github.com/aurora250/NexusForce/actions/workflows/codeql.yml)
[![Clang Format](https://img.shields.io/badge/Clang--Format-19.0-blue?style=flat-square&logo=llvm&logoColor=white)](https://clang.llvm.org/docs/ClangFormat.html)
[![Clang Tidy](https://img.shields.io/badge/Clang--Tidy-Passed-brightgreen?style=flat-square&logo=llvm&logoColor=white)](https://clang.llvm.org/extra/clang-tidy/)
[![C++ Core Guidelines](https://img.shields.io/badge/C%2B%2B%20Core%20Guidelines-Checked-00599C?style=flat-square&logo=cplusplus&logoColor=white)](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)
[![Valgrind](https://img.shields.io/badge/Valgrind-Passed-2E8B57?style=flat-square&logo=valgrind&logoColor=white)](https://valgrind.org)
[![License](https://img.shields.io/badge/License-MIT-F9A825?style=flat-square)](https://opensource.org/licenses/MIT)
[![Docs](https://img.shields.io/badge/Docs-Website-1565C0?style=flat-square&logo=readthedocs&logoColor=white)](https://nexusforce.org.cn)
[![PRs Welcome](https://img.shields.io/badge/PRs-Welcome-4CAF50?style=flat-square&logo=git&logoColor=white)](https://github.com/aurora250/NexusForce/pulls)

> 🌐 **语言** / **Language**  
> 通过其他语言阅读: [English](README.EN.md)

---

## 📑 目录

- [项目简介](#-项目简介)
- [支持环境](#️-支持环境)
- [工程质量](#-工程质量)
- [标准合规](#-标准合规)
- [特性](#-特性)
- [编译指南](#-编译指南)
- [快速开始](#-快速开始)
- [常见问题](#-常见问题)
- [文档](#-文档)
- [协议](#️-协议)
- [更新日志](#-更新日志)
- [贡献者](#-贡献者)
- [TODO](#-todo)

---

## 📖 项目简介

本项目旨在建立**功能健全、风格统一、可读性强、社区共建、跨平台兼容**的现代 C++ 开发库。通过清晰的架构设计、规范的代码实现、丰富的设计模式应用，为项目开发提供实用的工具集，同时也为 C++ 学习者提供理解底层原理的实践载体，建立从学习到生产的连接点。

💡 有劳各位多多 [提交 Issue](https://github.com/aurora250/NexusForce/issues)，使本项目趋于健全。如有不足，还望斧正。

---

## 🖥️ 支持环境

| 项目      | 详情                                                                           |
|-----------|--------------------------------------------------------------------------------|
| 平台      | 🪟 WINDOWS / 🐧 LINUX                                                          |
| 指令集    | X86                                                                            |
| 位宽      | 64位                                                                           |
| 编译器    | MSVC (Windows) / LLVM-Clang (Windows, Linux) / ClangCL (Windows) / GCC (Linux) |
| C++ 标准  | 14 / 17 / 20                                                                   |

> ℹ️ **兼容性说明**  
> 本库欢迎开发者进行更多编译器与操作系统的兼容性开发，欢迎您进行贡献。

---

## ✨ 工程质量

NexusForce 严格遵循现代 C++ 工程最佳实践，通过多层次自动化检查确保代码健壮性与可读性。

| 指标                           | 状态           | 说明                                                                                                               |
|--------------------------------|----------------|--------------------------------------------------------------------------------------------------------------------|
| 📊 **代码规模**                | 总计 16万+ 行  | 核心库源码与头文件 9万+ 行，测试代码 6万+ 行                                                                       |
| 🔒 **CodeQL 安全分析**         | **0 漏洞**     | `security-and-quality` 全规则集，零安全告警                                                                        |
| 🔍 **Clang-Tidy 静态检查**     | **零警告**     | 全量规则集（`bugprone` / `cppcoreguidelines` / `hicpp` / `modernize` / `performance` / `readability`），警告即错误 |
| 🎨 **Clang-Format 代码风格**   | **强制统一**   | 120 列、4 空格、K&R 变体大括号、强制大括号插入等                                                                   |
| 💧 **动态内存检查**            | **0 泄漏**     | Valgrind 全量测试，无内存泄漏与越界访问                                                                            |

> 📋 **关于规则豁免**：[`.clang-tidy`](.clang-tidy) 包含约 60 项显式豁免，[`.clang-format`](.clang-format) 包含多项风格定制。每一项均针对底层系统编程的固有需求，遵循"默认严格，按需放开"原则。

---

## 📡 标准合规

NexusForce 的核心组件实现严格遵循相关国际标准与行业规范，确保行为可预测、互操作性强且安全可靠。以下为关键组件的标准映射：

### 🌐 网络协议与互联网标准

| 组件                  | 遵循标准                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 | 说明                                                                                                                                                                            |
|-----------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| **HTTP 与 WebSocket** | [RFC 9110](https://www.rfc-editor.org/rfc/rfc9110.html) / [RFC 9112](https://www.rfc-editor.org/rfc/rfc9112.html) (HTTP/1.1), [RFC 7540](https://www.rfc-editor.org/rfc/rfc7540.html) / [RFC 7541](https://www.rfc-editor.org/rfc/rfc7541.html) (HTTP/2, HPACK), [RFC 6265](https://www.rfc-editor.org/rfc/rfc6265.html) (Cookie), [RFC 6455](https://www.rfc-editor.org/rfc/rfc6455.html) / [RFC 7692](https://www.rfc-editor.org/rfc/rfc7692.html) (WebSocket, permessage-deflate), [RFC 6066](https://www.rfc-editor.org/rfc/rfc6066.html) (SNI), [W3C Fetch CORS](https://fetch.spec.whatwg.org/#http-cors-protocol) | HTTP/1.1 语义与路由、HTTP/2 帧层与 HPACK 头部压缩、Range 请求、响应压缩、CONNECT 隧道、Cookie 与 CSRF、CORS 跨域策略、WebSocket 升级/帧协议/permessage-deflate 压缩、SNI 多证书 |
| **DNS 客户端**        | [RFC 1034](https://www.rfc-editor.org/rfc/rfc1034.html), [RFC 1035](https://www.rfc-editor.org/rfc/rfc1035.html), [RFC 2181](https://www.rfc-editor.org/rfc/rfc2181.html), [RFC 6891](https://www.rfc-editor.org/rfc/rfc6891.html), [RFC 3596](https://www.rfc-editor.org/rfc/rfc3596.html), [RFC 2782](https://www.rfc-editor.org/rfc/rfc2782.html)                                                                                                                                                                                                                                                                     | DNS 协议客户端，A/AAAA/MX/SRV/PTR 记录查询、UDP/TCP 传输自动切换与 TTL 缓存管理                                                                                                 |
| **ICMP 协议**         | [RFC 792](https://www.rfc-editor.org/rfc/rfc792.html) (STD 5), [RFC 1122](https://www.rfc-editor.org/rfc/rfc1122.html), [RFC 4884](https://www.rfc-editor.org/rfc/rfc4884.html), [IANA ICMP 参数注册表](https://www.iana.org/assignments/icmp-parameters/icmp-parameters.xhtml)                                                                                                                                                                                                                                                                                                                                          | Ping (Echo Request/Reply) 与 Traceroute (Time Exceeded) 网络诊断，含 RFC 1071 校验和算法                                                                                        |
| **SMTP 协议**         | [RFC 5321](https://www.rfc-editor.org/rfc/rfc5321.html) (STD 10), [RFC 5322](https://www.rfc-editor.org/rfc/rfc5322.html), [RFC 3207](https://www.rfc-editor.org/rfc/rfc3207.html) (STARTTLS), [RFC 8314](https://www.rfc-editor.org/rfc/rfc8314.html) (隐式 TLS), [RFC 4954](https://www.rfc-editor.org/rfc/rfc4954.html) (AUTH), [RFC 2045–2047](https://www.rfc-editor.org/rfc/rfc2045.html) (MIME)                                                                                                                                                                                                                   | 邮件传输与消息格式，支持 PLAIN/LOGIN 认证、STARTTLS/隐式 TLS 加密及 MIME 多部分消息                                                                                             |
| **MAC 地址**          | [IEEE 802-2014](https://standards.ieee.org/ieee/802/3714/), [IEEE 802.3-2022](https://standards.ieee.org/ieee/802.3/10422/), [RFC 7042](https://www.rfc-editor.org/rfc/rfc7042.html)                                                                                                                                                                                                                                                                                                                                                                                                                                     | 48 位 EUI-48 地址解析与格式化，支持单播/多播/本地管理地址识别与标准十六进制表示                                                                                                 |
| **URL 解析与编码**    | [RFC 3986](https://www.rfc-editor.org/rfc/rfc3986), [RFC 3987](https://www.rfc-editor.org/rfc/rfc3987), [WHATWG URL](https://url.spec.whatwg.org/)                                                                                                                                                                                                                                                                                                                                                                                                                                                                       | URI 通用语法、百分号编码及国际化资源标识符                                                                                                                                      |
| **网络端口定义**      | [IANA 端口号注册表](https://www.iana.org/assignments/service-names-port-numbers/), [RFC 6335](https://www.rfc-editor.org/rfc/rfc6335)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    | 知名端口分配                                                                                                                                                                    |
| **UUID 生成**         | [RFC 4122](https://www.rfc-editor.org/rfc/rfc4122), [RFC 9562](https://www.rfc-editor.org/rfc/rfc9562)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   | UUID v4（随机）与 v7（时间有序）生成规范                                                                                                                                        |
| **字节大小单位**      | [IEC 80000-13:2008](https://www.iso.org/standard/31898.html), [IEEE 1541-2021](https://standards.ieee.org/ieee/1541/10790/), [BIPM SI Brochure (9th Ed.)](https://www.bipm.org/en/publications/si-brochure)                                                                                                                                                                                                                                                                                                                                                                                                              | 二进制前缀 (KiB/MiB/GiB) 与十进制前缀 (kB/MB/GB)                                                                                                                                |

### 📁 配置文件格式

| 组件                | 遵循标准                                                                                                                                                                                                                 | 说明                                                                                          |
|---------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|-----------------------------------------------------------------------------------------------|
| **JSON RFC 8259**   | [RFC 8259](https://www.rfc-editor.org/rfc/rfc8259), [ECMA-404:2017](https://ecma-international.org/publications-and-standards/standards/ecma-404/)                                                                       | JSON 六种值类型、UTF-8 编码、IEEE 754-2019 双精度数字与字符串转义序列                         |
| **TOML 1.0.0**      | [TOML v1.0.0](https://toml.io/en/v1.0.0)                                                                                                                                                                                 | 包含日期时间格式遵循 [RFC 3339](https://www.rfc-editor.org/rfc/rfc3339) / ISO 8601            |
| **YAML 1.2**        | [YAML 1.2.2](https://yaml.org/spec/1.2.2/), [RFC 8259](https://www.rfc-editor.org/rfc/rfc8259.html), [RFC 3339](https://www.rfc-editor.org/rfc/rfc3339.html), [IEEE 754-2019](https://standards.ieee.org/ieee/754/6210/) | YAML 1.2 是 JSON 严格超集，支持八种核心值类型、五种字符串标量样式、锚点别名与标签系统         |

### 🔐 密码学与安全算法

| 组件                       | 遵循标准                                                                                                                                   | 说明                                                                                                                                                                |
|----------------------------|--------------------------------------------------------------------------------------------------------------------------------------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| **AES-256 加密**           | [NIST FIPS 197](https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.197-upd1.pdf), [ISO/IEC 18033-3](https://www.iso.org/standard/54531.html) | 高级加密标准，支持 ECB/CBC/GCM 模式（[NIST SP 800-38A](https://csrc.nist.gov/pubs/sp/800/38/a/final) / [SP 800-38D](https://csrc.nist.gov/pubs/sp/800/38/d/final)） |
| **ChaCha20-Poly1305 AEAD** | [IETF RFC 8439](https://www.rfc-editor.org/rfc/rfc8439.html)                                                                               | ChaCha20 流密码 + Poly1305 认证器，AEAD 认证加密，支持关联数据（AAD）                                                                                               |
| **SHA-256 哈希**           | [NIST FIPS 180-4](https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.180-4.pdf), [RFC 6234](https://www.rfc-editor.org/rfc/rfc6234)          | 安全哈希算法（SHA-2 家族），256 位输出                                                                                                                              |
| **SHA-1 哈希**             | [NIST FIPS 180-4](https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.180-4.pdf) (历史兼容)                                                   | ⚠️ 已标注安全警告（[SHAttered](https://shattered.io/) 碰撞攻击）                                                                                                    |
| **MD5 哈希**               | [RFC 1321](https://www.rfc-editor.org/rfc/rfc1321) (历史兼容)                                                                              | ⚠️ 已标注安全警告，仅用于非安全校验场景                                                                                                                             |
| **Base64 编码**            | [RFC 4648 §4](https://www.rfc-editor.org/rfc/rfc4648.html#section-4), [RFC 4648 §5](https://www.rfc-editor.org/rfc/rfc4648.html#section-5) | 标准与 URL 安全 Base64 编解码，严格填充规则与非法字符检测                                                                                                           |

### 🔤 字符编码与国际化

| 组件                          | 遵循标准                                                                                                                                                                                                                         | 说明                                               |
|-------------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|----------------------------------------------------|
| **UTF-8 / UTF-16 / UTF-32**   | [Unicode 15.1.0](https://unicode.org/versions/Unicode15.1.0/), [ISO/IEC 10646](https://www.iso.org/standard/76835.html), [RFC 3629](https://www.rfc-editor.org/rfc/rfc3629) / [RFC 2781](https://www.rfc-editor.org/rfc/rfc2781) | Unicode 码点操作、规范化与编码转换，含无效序列检测 |
| **Unicode 码点处理**          | [Unicode 15.1.0](https://unicode.org/versions/Unicode15.1.0/) §2.4, §2.13                                                                                                                                                        | 代理对处理、BOM 检测与替换字符 (U+FFFD) 规则       |

### 📐 数据结构与算法

| 组件                      | 遵循标准 / 学术文献                                                                                                                                                                                      | 说明                                                                                          |
|---------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|-----------------------------------------------------------------------------------------------|
| **堆算法**                | [ISO/IEC 14882:2020](https://www.iso.org/standard/79358.html) §25.8.6                                                                                                                                    | 复杂度保证与 Floyd 堆调整优化 ([Algorithm 245](https://dl.acm.org/doi/10.1145/512274.512284)) |
| **红黑树**                | [Guibas & Sedgewick (1978)](https://doi.org/10.1109/SFCS.1978.3)                                                                                                                                         | 自平衡二叉搜索树经典实现，O(log n) 复杂度保证                                                 |
| **莱昂纳多堆 / 平滑排序** | [Dijkstra (1981) EWD796a](https://www.cs.utexas.edu/~EWD/transcriptions/EWD07xx/EWD796a.html)                                                                                                            | 自适应排序算法，最优时间复杂度 O(n)                                                           |
| **内省排序**              | [Musser (1997)](https://doi.org/10.1002/(SICI)1097-024X(199708)27:8<983::AID-SPE117>3.0.CO;2-%23)                                                                                                        | 混合快速/堆/插入排序，C++ 标准库 sort 默认算法                                                |
| **非加密哈希**            | [FNV-1a 草案](https://datatracker.ietf.org/doc/html/draft-eastlake-fnv-17), [MurmurHash3](https://github.com/aappleby/smhasher/wiki/MurmurHash3)                                                         | 高性能哈希表与布隆过滤器专用                                                                  |
| **布隆过滤器**            | [Bloom (1970)](https://doi.org/10.1145/362686.362692), [Broder & Mitzenmacher (2004)](https://doi.org/10.1080/15427951.2004.10129096), [Kirsch & Mitzenmacher (2006)](https://doi.org/10.1002/rsa.20208) | 双哈希优化的概率性集合成员查询结构，O(k) 插入/查询，支持最优参数 (m, k) 推导与误报率估算      |

### ⚙️ 系统、并发与命令行

| 组件               | 遵循标准                                                                                                                                                                          | 说明                                                       |
|--------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|------------------------------------------------------------|
| **原子内存序**     | [ISO/IEC 14882:2020](https://www.iso.org/standard/79358.html) §31.4                                                                                                               | C++ 内存模型，含硬件屏障等价与 Intel TSX HLE 支持          |
| **命令行解析**     | [POSIX.1-2017 (IEEE 1003.1)](https://pubs.opengroup.org/onlinepubs/9699919799/) 第12章, [GNU getopt_long](https://man7.org/linux/man-pages/man3/getopt.3.html)                    | 支持短选项组合、长选项、`--` 分隔符与可选值                |
| **日期与时间**     | [ISO 8601-1:2019](https://www.iso.org/standard/70907.html), [RFC 3339](https://www.rfc-editor.org/rfc/rfc3339), [POSIX 时间戳](https://pubs.opengroup.org/onlinepubs/9699919799/) | 公历计算、儒略日转换与 Unix 纪元处理                       |
| **SQL 语句构建器** | [ISO/IEC 9075](https://www.iso.org/standard/16663.html) (SQL-92 及后续版本)                                                                                                       | 生成符合 ANSI SQL 的 SELECT/INSERT/UPDATE/DELETE 语句      |

### 🎨 图形、色彩与数学

| 组件               | 遵循标准                                                                                                                          | 说明                                              |
|--------------------|-----------------------------------------------------------------------------------------------------------------------------------|---------------------------------------------------|
| **RGB 颜色模型**   | [W3C CSS Color Level 4](https://www.w3.org/TR/css-color-4/), [Compositing Level 1](https://www.w3.org/TR/compositing-1/)          | 直通 Alpha 合成、十六进制格式与 ANSI 256 色调色板 |
| **灰度转换**       | [ITU-R BT.709](https://www.itu.int/rec/R-REC-BT.709/) / [IEC 61966-2-1 (sRGB)](https://webstore.iec.ch/publication/6169)          | 人眼感知加权（0.299R + 0.587G + 0.114B）          |
| **数学函数与常量** | [IEEE 754-2019](https://standards.ieee.org/ieee/754/6210/), [ISO/IEC 10967 (LIA)](https://www.iso.org/standard/24417.html)        | 三角函数归约、牛顿迭代法与机器精度容差            |
| **随机数生成**     | [ISO/IEC 18031:2011](https://www.iso.org/standard/54945.html), [NIST SP 800-90A](https://csrc.nist.gov/pubs/sp/800/90/a/r1/final) | 梅森旋转 (MT19937) 与操作系统熵源真随机数         |

> 📖 **文档完整性**：所有涉及上述标准的类与函数，均在其 API 注释（Doxygen 格式）中标注了具体的标准章节号与官方链接，开发者可随时溯源验证。

---

## 🚀 特性

### 🔄 并发与异步 (Async)
- **`thread_pool`** - 基于 NUMA 感知任务窃取与多策略定制的高性能线程池，性能测试详见 [线程池性能测试记录](benchmark/async/PERFORMANCE.md)
- **`io_context`** - 异步 I/O 执行上下文，统一事件循环、定时器与取消模型
- **`cancellation_slot`** - 支持 `async_connect` / `async_read` / `async_write` 可中断异步 I/O
- **`timer_scheduler`/`basic_timer`** - 基于红黑树的定时任务调度，支持任务取消标志
- **`async_stream`** - 异步流抽象接口，统一读写协议
- **`async_read()` / `async_write()`** - 自由函数组合器，基于 `shared_from_this` 自动处理部分读写重试
- **`thread_pool_executor`** - 将 `thread_pool::submit_task()` 适配为标准 executor 接口
- **`generator`/`task`** - 协程原语和任务生成器
- **`virtual_thread`** - C#风格的轻量级协程
- **`connection`/`signal`/`signal_base`** - 观察者模式的信号槽，`signal_base` 提供类型擦除基类支持反射驱动的动态连接
- **`call_once`** - 基于FUTEX的多线程单次调用实现
- **停止令牌** - 可取消的异步操作 `stop_token`/`stop_source`/`stop_callback`
- **同步原语** - 互斥锁 `mutex`、读写锁 `shared_mutex`、信号量 `semaphore`/`atomic_semaphore`、线程屏障 `barrier`与闩锁 `latch`
- **原子操作** - 原子类型 `atomic`、原子FUTEX `atomic_futex`、全局原子操作函数体系
- **多策略线程** - 通用线程 `thread`、携带停止令牌的作用域线程 `scope_thread`、手动开启的惰性线程 `lazy_thread`
- **基本异步模型** - `async` 及其配套的 `future`/`promise`/`packaged_task` 结构
- **危险指针** - 无锁数据结构的内存管理 `hazard_ptr`/`hazard_pointer_domain`

### 📦 容器 (Container)
- **标准容器** - `array`/`vector`/`list`/`deque`/`map`/`set`/`unordered_map`/`unordered_set`/`sparse_map`/`sparse_set`/`flat_unordered_map`/`flat_unordered_set` 等
- **`rb_tree`** - 自平衡二叉搜索树实现
- **`hashtable`** - 链地址法哈希表
- **`flat_hashtable`** - SwissTable 开放寻址平坦哈希表，H2 预过滤 + SIMD 批量探测
- **`bloom_filter`** - 概率性数据结构
- **`lru_cache`/`ttl_cache`** - 基于最近最少使用/过期时间的缓存策略
- **`buffer_chain`** - 零拷贝链式缓冲区，支持 writev 聚合输出
- **`sparse_vector`** - 基于排序扁平数组的关联容器，O(log n) 二分查找，O(1) 缓存友好迭代
- **`bitmap`/`bitset`** - 高效位操作容器

### 🔐 加密与安全 (Encrypt)
- **`AES256`** - 高级加密标准实现
- **`ChaCha20Poly1305`** - ChaCha20-Poly1305 AEAD 认证加密，支持 AAD
- **`SHA1`/`SHA256`** - 安全哈希算法
- **`MD5`** - 消息摘要算法
- **`base64`** - 二进制数据编码

### 📁 文件系统 (File)
- **路径/文件操作** - 路径/文件系统操作 `path`/`path_tree`/`file`/`file_async`/`file_diff`/`file_locker`/`file_mapper`
- **`file_async`** - 基于 `io_context` 的异步文件 I/O，支持指定偏移量与取消槽
- **`file_watcher`** - 实时文件系统变更监控
- **配置文件解析** - JSON/TOML/YAML/INI/ENV 值系统、格式解析与流式构建
- **`temp_file`** - 安全的临时文件管理

### 🌐 网络库 (Network)
- **HTTP/1.1 服务器** - 完整协议实现：`http_server` / `http_router` / `http_filter` 中间件链
- **HTTP/2 支持** - 帧层与 HPACK 头部压缩（RFC 7540/7541），9 种帧类型，流状态机，流量控制，TLS+ALPN 协商
- **Radix Tree 路由** - 基于压缩前缀树的 O(k) 路由匹配，支持静态路径、:param 参数、* 通配符、正则回退
- **HTTP 高级特性** - Range 请求（206 单/多范围）、gzip/deflate 响应压缩、Chunked 分块传输、CONNECT 隧道
- **`multipart_parser` / `chunked_reader`** - 多部分表单解析与分块传输读取
- **会话管理** - 可插拔 session_store（内存/Redis），CSRF Double-Submit Cookie 防护，Session Fixation 防护
- **安全中间件** - `csrf_filter` CSRF 防护、`http_security` 安全响应头、`http_cache` HTTP 缓存策略
- **WebSocket** - RFC 6455/7692 全双工通信 `websocket_session` / `websocket_server`，事件驱动零线程模式
- **WebSocket 压缩** - permessage-deflate（RFC 7692），窗口比特位协商与上下文接管控制
- **HTTP 客户端** - `http_client` scheme 感知模式（https 自动 SSL），请求/响应处理
- **TCP/UDP 套接字** - 高性能网络通信 `tcp_socket` / `udp_socket`，支持 `async_connect()` / `async_read()` / `async_write()` 异步操作
- **SSL/TLS** - 加密网络传输 `ssl_context` / `ssl_stream`，SNI 多证书管理 `sni_manager`，ALPN 协议协商
- **`dns_client`** - 域名解析，per-operation 异步状态对象架构，completion-token 异步 API（回调/取消槽/future/awaitable）
- **`io_context`** - 统一事件循环（Linux epoll 边缘触发 + min-heap 定时器），异步 I/O 回调驱动，取消模型
- **`async_filter`** - 异步 pre/post 过滤链框架
- **`byte_cursor`** - 无拷贝字节级协议解析游标，带边界检查与位级缓冲
- **`load_balancer`** - 连接池 + 轮询负载均衡
- **FTP** - FTP 服务器与客户端
- **ICMP/SMTP** - ICMP 和 SMTP 协议操作
- **`arp`/`mac_address`/`ip_address`/`ports`/`url`** - 网络编程工具

### 🗄️ 数据库 (DB)
- **`database_pool`** - 数据库连接复用与管理，支持 `warm_up()` 预热 / `active_count()` 活跃查询 / `get_tb_connect_for()` 自定义超时
- **`sql_builder`** - 标准 SQL 语句流式构建，支持**方言感知占位符**（Generic `?` / PgSQL `$N` / Oracle `:N`）和**方言感知 DDL**（AUTO_INCREMENT / SERIAL / IDENTITY 自动适配）
- **`sql_mapper<T>`** - 反射驱动 ORM SQL 生成器，自动生成 DDL/DML 语句，支持方言感知
- **`repository<T, Connect>`** - 泛型 CRUD 仓库模板（find_all / find_by_pk / find_where / find_page / insert / update / remove / count）
- **多数据库支持**:
  - MySQL 客户端
  - PostgreSQL 客户端
  - SQLite 客户端
  - Redis 客户端
- **预处理语句** - 防止 SQL 注入
- **结果集封装** - 统一结果访问接口
- **DB 属性注解** - `PROP_PRIMARY_KEY` / `PROP_AUTO_INC` / `PROP_UNIQUE` / `PROP_INDEX` / `PROP_FOREIGN_KEY`，反射系统与 ORM 共享元数据

### 📝 日志系统 (Logging)
- **`log_sink`** - 可扩展的日志输出目标
- **`file_sink`** - 日志文件管理与轮转
- **`log_formatter`** - 自定义日志格式
- **`logger`** - 线程池驱动的异步日志，支持 `block`/`discard`/`overrun_oldest` 三种溢出策略，有界环形队列、自动 drain 调度

### 🔤 字符串处理 (String)
- **PCRE2 正则表达式** - 支持 JIT 的高效正则匹配 `regex`/`match_result`/`regex_iterator`/`regex_token_iterator`，支持拷贝
- **Unicode 支持** - UTF 转换系统、码点操作类 `codepoint`、UTF-8 码点迭代器 `utf8_iterator` / `utf8_view`
- **`formatter`/`format`** - 类型安全的格式化输出，支持位置参数 `{0}` `{1}` 和命名参数 `format_named()`
- **`string_view`** - 大量使用字符串视图优化操作
- **数值转换** - 可扩展的字符串与数值互转体系
- **Boyer-Moore-Horspool** - 窄字符子串搜索加速

### ⚙️ 系统接口 (System)
- **`process`** - 进程创建与控制
- **`pipe`** - 管道创建与管理
- **`dynamic_library`** - 运行时库加载
- **`console`** - 高集成度的终端交互
- **`cmdline`** - 进程参数分析与操作
- **`stacktrace`** - 异常调试
- **`share_memory`** - 跨进程共享内存
- **`locale`** - 本地化设置与解析
- **`sysinfo`** - 硬件与 OS 信息
- **`environment`** - 环境变量操作
- **`system_signal_manager`** - 系统信号控制
- **`system_event`** - 系统事件管理

### ⏰ 时间处理 (Time)
- **`steady_clock` / `system_clock`** - 多种时钟源高精度时钟
- **`duration`** - 时间跨度计算
- **`date` / `time` / `datetime` / `timestamp`** - 日历操作
- **`scoped_click`** - 代码块执行时间测量

### 🛠️ 工具库 (Utility)
- **字面类型包装类** - 字面类型的高级封装
- **`optional`** - 可选值处理
- **`variant`** - 类型安全联合体
- **`expected`** - 错误状态处理
- **`any`** - 类型擦除容器
- **`pair`/`tuple`** - 编译期键值对/元组
- **`color`** - RGB 颜色操作
- **`scope_exit` / `scope_fail` / `scope_success`** - 作用域守卫
- **数值/字符信息** - 数值/字符类型信息获取
- **`ratio`** - 编译期比率计算
- **`uuid`** - UUID v4/v7 生成器
- **`compressed_pair`** - EBCO内存优化

### 🔍 反射系统 (Reflection)
- **`NFRS`** - 预编译代码生成器，扫描 `NEFORCE_REFLECT_*` 标记自动生成类型注册代码，支持增量扫描
- **`registry`** - 全局类型反射注册表，支持名称/type_id 双重查找、线程安全注册、动态信号槽连接 `connect_signal_to_slot()`
- **`meta_type`** - 运行时类型元数据：基类列表、属性/函数映射、构造/克隆工厂、枚举/容器信息、信号名称列表、动态属性注册 `add_property()`
- **`meta_property`** - 属性反射描述符，支持注解标志、变更通知信号
- **`meta_function`** - 成员函数/静态函数反射描述符，支持重载、参数提示与运行时 `invoke()` 调用
- **`meta_enum`** - 枚举反射：名称↔值双向查找、条目遍历
- **`type_builder`** - 链式API 类型构建器：基类注册、属性/函数/信号注册、构造/克隆/容器配置
- **JSON 序列化器** - 反射驱动的对象↔JSON 序列化与反序列化，递归处理嵌套类型与容器
- **二进制序列化器** - 大端格式（Magic "NEBF" + 类型表 + 数据段），属性注解控制序列化行为
- **类型识别** - 基于 type_name 特化 + 编译器函数签名的 hash-based type_id，非侵入式类型注册
- **反射宏** - `NEFORCE_REFLECT_OBJ` / `PROP` / `FUNC` / `SIGNAL` / `ENUM` / `ENUM_VAL` 类体内标记，供 NFRS 扫描器识别

### 🖥️ 终端UI框架 (TUI)
- **`application`** - 应用入口，Builder 模式配置（主题/FPS/标题），驱动事件循环与动画计时器
- **`reconciler`** - 声明式渲染引擎，Cell 级终端帧差分与增量 ANSI 输出，焦点链遍历，鼠标命中测试与滚动条交互
- **`screen`** - 终端帧缓冲，逐 Cell 差分生成最小 ANSI 转义序列
- **`input_driver`** - 跨平台终端输入驱动，解析 ANSI/鼠标/UTF-8 序列
- **`element`** - 不可变虚拟元素树，14+ 节点类型，装饰器链式组合，完整 Flexbox + Gridbox 网格布局
- **`style` / `theme`** - 样式系统 + 语义化主题，预置暗色主题
- **`state<T>`** - 响应式状态管理，写操作自动脏标记 + strand 合并调度重渲染
- **`component_base` / `component<P>`** - 组件基类，焦点管理、上下文注入（`provide_context`/`context`，沿父链查找）、响应式状态工厂
- **`animator` / `easing`** - 属性动画器 + 缓动函数

### 🧬 类型与特性 (TypeInfo)
- **类型萃取** - 完备的编译期类型判断
- **概念约束** - 概念集支持
- **`check_type`** - 运行时可读的类型名
- **CRTP 静态多态** - 全局统一的零开销接口

### 💾 内存管理 (Memory)
- **`shared_ptr`、`unique_ptr`、`weak_ptr`** - 智能指针结构
- **`atomic<shared_ptr/weak_ptr>`** - 原子智能指针无锁操作
- **`memory_view`** - 安全内存访问
- **位/端序/字节流操作** - 内存状态修改
- **构造/析构工具** - 对象生命周期管理
- **`trace_allocator` ** - 调试用内存监控
- **`standard_allocator`** - 基于编译器特性的策略特化分配器

### 📦 压缩 (Compress)
- **lz4 压缩** - `lz4_compressor` 高速数据压缩/解压
- **zlib 压缩** - `zlib_compressor` 多策略通用数据压缩/解压

### 🔌 插件系统 (Plugin)
- **动态插件管理** - `plugin_manager` 运行时加载卸载插件
- **插件接口** - 标准化插件开发

### ❗ 异常处理 (Exception)
- **异常指针** - `exception_ptr` 跨线程异常传递
- **终止处理** - 多状态程序终止方式与对应回调
- **异常/错误码系统** - 自定义的异常与错误码体系
- **断点处理** - 调试断点触发与处理

### 📐 算法库 (Algorithm)
- **标准算法** - 基于迭代器系统的范围迭代算法
- **并行算法** - 并行执行策略
- **数值算法** - 数值计算与累加
- **堆算法** - 堆操作与优先级队列
- **范围操作** - ranges 支持
- **哈希算法** - 多种哈希函数实现

### 📊 数学库 (Math)
- **数学常量** - 常用数学常数
- **数学函数** - 超越函数与数值计算
- **随机数生成** - LC、梅森旋转、硬件噪声算法 `random_lcd` / `random_mt` / `secret`
- **128位数学计算** - 128位有符号/无符号数值操作 `int128_t` / `uint128_t`

---

## 🔧 编译指南

### 📋 前置依赖

| 类型               | 依赖                                                                | 版本要求 |
|--------------------|---------------------------------------------------------------------|----------|
| 🔨 构建工具        | [CMake](https://cmake.org/)                                         | 3.19+    |
| 📦 包管理器        | [vcpkg](https://github.com/microsoft/vcpkg)                         | Latest   |
| 🎨 代码格式化      | [clang-format](https://clang.llvm.org/docs/ClangFormat.html)        | 19+      |
| 🔍 静态分析        | [clang-tidy](https://clang.llvm.org/extra/clang-tidy/)              | 19+      |
| ⚠️ 必选依赖(vcpkg) | [pcre2](https://www.pcre.org/)                                      | 10.47+   |
|                    | [icu](https://icu.unicode.org/)                                     | 78.2+    |
|                    | [OpenSSL](https://www.openssl.org/)                                 | 3.6.1+   |
| 📦 可选依赖(vcpkg) | [libpq](https://www.postgresql.org/)                                | 16.9+    |
|                    | [libmysql](https://www.mysql.com/)                                  | 8.0.40+  |
|                    | [sqlite3](https://sqlite.org/index.html)                            | 3.51.2+  |
|                    | [hiredis](https://redis.ac.cn/docs/latest/develop/clients/hiredis/) | 1.3.0+   |
|                    | [lz4](https://lz4.org/)                                             | 1.10.0+  |
|                    | [zlib](https://www.zlib.net/)                                       | 1.3.1+   |
|                    | [GTest](https://google.github.io/googletest/)                       | 1.17.0+  |
|                    | [benchmark](https://github.com/google/benchmark/)                   | 1.9.5+   |

### 🏗️ 编译步骤

编译前确保您已经正确安装并配置了 CMake、vcpkg、clang-format、clang-tidy

> 💡 您可以在项目根目录的 `config.json` (编译项配置) 与 `vcpkg.json` (包管理配置) 中更改配置项以进行个性化编译
>
> **本项目在 linux 系统中额外依赖 liburing-dev，构建前确保您已经安装**
>
> 实测 linux 系统中，vcpkg 构建 libmysql 需要 libtirpc-dev 库，libpq 需要 bison、flex、autoconf 库，且这些库不会由系统包管理器默认安装。
> 如果您需要对应依赖，您可以通过系统包管理器提前安装以免 cmake 构建失败

#### 🪟 Windows

```bash
# 克隆最新发布版
git clone --depth 1 https://github.com/aurora250/NexusForce.git
cd NexusForce

# 创建构建目录
mkdir build && cd build

# 编译选项配置
cmake .. -G "Visual Studio 17 2022" -A x64

# 编译
cmake --build . --config Release

# 安装到系统目录
cmake --install . --config Release

# 或使用脚本快速安装，更多参数参见install_nexusforce.py注释
python ./scripts/install_nexusforce.py --release
```

#### 🐧 Linux

```bash
# 克隆最新发布版
git clone --depth 1 https://github.com/aurora250/NexusForce.git
cd NexusForce

# 创建构建目录
mkdir build && cd build

# 编译选项配置
cmake ..

# 编译
make -j$(nproc)

# 安装到系统目录
sudo make install

# 或使用脚本快速安装，更多参数参见install_nexusforce.py注释
python ./scripts/install_nexusforce.py --release
```

### 🔗 在你的项目中使用 NexusForce

安装完成后，在你的 CMake 项目中通过 `find_package` 引入 NexusForce，并使用提供的配置函数完成编译选项、反射扫描和运行时部署。

#### 基础链接

```cmake
cmake_minimum_required(VERSION 3.19)
project(my_app)

find_package(NexusForce REQUIRED)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE NexusForce::NexusForce)
nexusforce_compiler_options(my_app)
```

#### 反射代码生成（NFRS）

如果你的代码中使用了 `NEFORCE_REFLECT_OBJ` 等反射标记宏，通过 `nexusforce_reflect_scan()` 一键集成 NFRS 预编译扫描：

```cmake
# 扫描 src/ 目录下的头文件，自动生成反射注册代码并注入到目标
nexusforce_reflect_scan(
    TARGET  my_app
    HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/src
)
```

函数会在构建时运行 NFRS 扫描指定目录中的 `.hpp` / `.h` 文件，生成 `_nfrs_gen_<target>.cpp` 并自动添加到目标源文件列表。

参数说明：

| 参数        | 必填 | 说明                                                                    |
|-------------|------|-------------------------------------------------------------------------|
| `TARGET`    | 是   | 要注入生成代码的目标                                                    |
| `HEADERS`   | 是   | 要扫描的头文件目录                                                      |
| `OUTPUT`    | 否   | 生成文件路径，默认 `${CMAKE_CURRENT_BINARY_DIR}/_nfrs_gen_<target>.cpp` |
| `EXCLUDES`  | 否   | 排除的路径片段（传递给 NFRS 的 `-e` 选项）                              |
| `DEPENDS`   | 否   | 额外的 CMake 级依赖文件                                                 |

#### 运行时 DLL 部署

在 Windows 上，链接共享库后需要将 DLL 部署到可执行文件所在目录才能直接运行。使用 `nexusforce_deploy_runtime()` 自动完成：

```cmake
nexusforce_deploy_runtime(my_app)
```

该函数在 Windows 上通过 `POST_BUILD` 将 `NexusForce.dll` 复制到目标输出目录。Linux 上此函数为空操作（由 RPATH 机制处理）。

---

---

## 🚀 快速开始

在开始之前，请确保已完成编译指南中的安装步骤。

具体参见 [使用 NexusForce 快速构建常见功能！](QUICK_START.md)

---

## ❓ 常见问题

关于 NexusForce 的定位、与其他框架（Boost / Qt / POCO / folly）的差异对比、设计理念、功能选型建议等，
请参见 [Q&A](Q&A.md)。

---

## 📚 文档

完整 API 文档请访问 [NexusForce 文档网站](https://nexusforce.org.cn)

---

## ⚖️ 协议

本项目基于 [MIT 开源协议](LICENSE) 发布。

---

## 📝 更新日志

详细更新记录请参见 [CHANGELOG](CHANGELOG.md)

---

## 👥 贡献者

感谢所有为本项目做出贡献的开发者！查看 [CONTRIBUTORS](CONTRIBUTORS.md) 获取完整名单。

---

## 📌 TODO

核心 ABI 已固定

TODO 条例参见代码内的 TODO 注释
