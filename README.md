# NexusForce V1.0.0

[![vcpkg](https://img.shields.io/badge/vcpkg-Enabled-0A7FAA?style=flat-square&logo=vcpkg&logoColor=white)](https://vcpkg.io)
[![CMake](https://img.shields.io/badge/CMake-3.19+-064C8B?style=flat-square&logo=cmake&logoColor=white)](https://cmake.org)
[![C++](https://img.shields.io/badge/C++-14/17/20-00599C?style=flat-square&logo=cplusplus&logoColor=white)](https://isocpp.org)
[![CodeQL](https://github.com/aurora250/NexusForce/workflows/CodeQL%20Analysis/badge.svg)](https://github.com/aurora250/NexusForce/actions/workflows/codeql.yml)
[![Quality Gate](https://img.shields.io/badge/Quality%20Gate-Passed-success?style=flat-square&logo=sonarqube&logoColor=white)]()
[![Clang Format](https://img.shields.io/badge/Clang--Format-19.0-blue?style=flat-square&logo=llvm&logoColor=white)](https://clang.llvm.org/docs/ClangFormat.html)
[![Clang Tidy](https://img.shields.io/badge/Clang--Tidy-Passed-brightgreen?style=flat-square&logo=llvm&logoColor=white)](https://clang.llvm.org/extra/clang-tidy/)
[![C++ Core Guidelines](https://img.shields.io/badge/C%2B%2B%20Core%20Guidelines-Checked-00599C?style=flat-square&logo=cplusplus&logoColor=white)](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)
[![Valgrind](https://img.shields.io/badge/Valgrind-Tested-2E8B57?style=flat-square&logo=valgrind&logoColor=white)](https://valgrind.org)
[![Memory Leak](https://img.shields.io/badge/Memory%20Leak-None-00C853?style=flat-square)](valgrind)
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
- [文档](#-文档)
- [协议](#️-协议)
- [更新日志](#-更新日志)
- [贡献者](#-贡献者)
- [TODO](#-todo)

---

## 📖 项目简介

本项目旨在建立**功能健全、风格统一、可读性强、社区共建、跨平台兼容**的现代 C++ 开发库。通过清晰的架构设计、规范的代码实现、丰富的设计模式应用，为项目开发提供实用的工具集，同时也为 C++ 学习者提供理解底层原理的实践载体，建立从学习到生产的连接点。

💡 有劳各位多多 [提交 Issue](https://github.com/aurora250/NexusForce/issues)，使本项目趋于健全。如有不足，还望斧正。

> ℹ️ **字符编码说明**  
> 本库假定运行环境的默认系统代码页为 **UTF-8**。若在非 UTF-8 环境下使用 IO 相关功能，请确保已正确配置 UTF-8 区域设置，以避免潜在的字符行为异常。

---

## 🖥️ 支持环境

| 项目     | 详情                                                                             |
|--------|--------------------------------------------------------------------------------|
| 平台     | 🪟 WINDOWS / 🐧 LINUX                                                          |
| 指令集    | X86                                                                            |
| 位宽     | 64位                                                                            |
| 编译器    | MSVC (Windows) / LLVM-Clang (Windows, Linux) / ClangCL (Windows) / GCC (Linux) |
| C++ 标准 | 14 / 17 / 20                                                                   |

> ℹ️ **兼容性说明**  
> 本库仍保留对32位环境一定的兼容性，但已不再维护；
> 本库保留了对ARM、RISC-V、LOONGARCH的兼容性分支；
> 本库欢迎开发者进行更多编译器与操作系统的兼容性开发。
> 
> 欢迎您进行贡献。

---

## ✨ 工程质量

NexusForce 严格遵循现代 C++ 工程最佳实践，通过多层次自动化检查确保代码健壮性与可读性。

| 指标 | 状态 | 说明                                                                                                    |
|------|------|-------------------------------------------------------------------------------------------------------|
| 📊 **代码规模** | 12万+ 行 | 核心库源码与头文件                                                                                             |
| 🔒 **CodeQL 安全分析** | **0 漏洞** | `security-and-quality` 全规则集，零安全告警                                                                     |
| 🔍 **Clang-Tidy 静态检查** | **零警告** | 全量规则集（`bugprone` / `cppcoreguidelines` / `hicpp` / `modernize` / `performance` / `readability`），警告即错误 |
| 🎨 **Clang-Format 代码风格** | **强制统一** | 120 列、4 空格、K&R 变体大括号、强制大括号插入等                                                                         |
| 💧 **动态内存检查** | **0 泄漏** | Valgrind 全量测试，无内存泄漏与越界访问                                                                              |

> 📋 **关于规则豁免**：[`.clang-tidy`](.clang-tidy) 包含约 60 项显式豁免，[`.clang-format`](.clang-format) 包含多项风格定制。每一项均针对底层系统编程的固有需求，遵循"默认严格，按需放开"原则。

---

## 📡 标准合规

NexusForce 的核心组件实现严格遵循相关国际标准与行业规范，确保行为可预测、互操作性强且安全可靠。以下为关键组件的标准映射：

### 🌐 网络协议与互联网标准

| 组件 | 遵循标准 | 说明 |
|------|----------|------|
| **URL 解析与编码** | [RFC 3986](https://www.rfc-editor.org/rfc/rfc3986), [RFC 3987](https://www.rfc-editor.org/rfc/rfc3987), [WHATWG URL](https://url.spec.whatwg.org/) | URI 通用语法、百分号编码及国际化资源标识符 |
| **网络端口定义** | [IANA 端口号注册表](https://www.iana.org/assignments/service-names-port-numbers/), [RFC 6335](https://www.rfc-editor.org/rfc/rfc6335) | HTTP/HTTPS/FTP/SSH/DNS 等知名端口分配 |
| **Base64 编码** | [RFC 4648](https://www.rfc-editor.org/rfc/rfc4648) | 标准 Base64 与 URL 安全 Base64 字符表 |
| **JSON 数据格式** | [RFC 8259](https://www.rfc-editor.org/rfc/rfc8259), [ECMA-404](https://ecma-international.org/publications-and-standards/standards/ecma-404/) | JSON 语法、数据类型与 UTF-8 编码要求 |
| **TOML 配置格式** | [TOML v1.0.0](https://toml.io/en/v1.0.0) | 包含日期时间格式遵循 [RFC 3339](https://www.rfc-editor.org/rfc/rfc3339) / ISO 8601 |
| **UUID 生成** | [RFC 4122](https://www.rfc-editor.org/rfc/rfc4122), [RFC 9562](https://www.rfc-editor.org/rfc/rfc9562) | UUID v4（随机）与 v7（时间有序）生成规范 |
| **HTTP 与 WebSocket** | [RFC 9110](https://www.rfc-editor.org/rfc/rfc9110.html) / [RFC 9112](https://www.rfc-editor.org/rfc/rfc9112.html) (HTTP/1.1), [RFC 6265](https://www.rfc-editor.org/rfc/rfc6265.html) (Cookie), [RFC 6455](https://www.rfc-editor.org/rfc/rfc6455.html) (WebSocket), [W3C Fetch CORS](https://fetch.spec.whatwg.org/#http-cors-protocol) | 完整 HTTP 语义与路由、Cookie 管理、CORS 跨域策略、WebSocket 升级与帧协议 |

### 🔐 密码学与安全算法

| 组件 | 遵循标准 | 说明 |
|------|----------|------|
| **AES-256 加密** | [NIST FIPS 197](https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.197-upd1.pdf), [ISO/IEC 18033-3](https://www.iso.org/standard/54531.html) | 高级加密标准，支持 ECB/CBC/GCM 模式（[NIST SP 800-38A](https://csrc.nist.gov/pubs/sp/800/38/a/final) / [SP 800-38D](https://csrc.nist.gov/pubs/sp/800/38/d/final)） |
| **SHA-256 哈希** | [NIST FIPS 180-4](https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.180-4.pdf), [RFC 6234](https://www.rfc-editor.org/rfc/rfc6234) | 安全哈希算法（SHA-2 家族），256 位输出 |
| **SHA-1 哈希** | [NIST FIPS 180-4](https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.180-4.pdf) (历史兼容) | ⚠️ 已标注安全警告（[SHAttered](https://shattered.io/) 碰撞攻击） |
| **MD5 哈希** | [RFC 1321](https://www.rfc-editor.org/rfc/rfc1321) (历史兼容) | ⚠️ 已标注安全警告，仅用于非安全校验场景 |
| **PKCS#7 填充** | [RFC 5652](https://www.rfc-editor.org/rfc/rfc5652), [RFC 8018](https://www.rfc-editor.org/rfc/rfc8018) | 加密消息语法与基于密码的加密填充方案 |

### 🔤 字符编码与国际化

| 组件 | 遵循标准 | 说明 |
|------|----------|------|
| **UTF-8 / UTF-16 / UTF-32** | [Unicode 15.1.0](https://unicode.org/versions/Unicode15.1.0/), [ISO/IEC 10646](https://www.iso.org/standard/76835.html), [RFC 3629](https://www.rfc-editor.org/rfc/rfc3629) / [RFC 2781](https://www.rfc-editor.org/rfc/rfc2781) | Unicode 码点操作、规范化与编码转换，含无效序列检测 |
| **Unicode 码点处理** | [Unicode 15.1.0](https://unicode.org/versions/Unicode15.1.0/) §2.4, §2.13 | 代理对处理、BOM 检测与替换字符 (U+FFFD) 规则 |

### 📐 数据结构与算法

| 组件 | 遵循标准 / 学术文献 | 说明 |
|------|---------------------|------|
| **堆算法** | [ISO/IEC 14882:2020](https://www.iso.org/standard/79358.html) §25.8.6 | 复杂度保证与 Floyd 堆调整优化 ([Algorithm 245](https://dl.acm.org/doi/10.1145/512274.512284)) |
| **红黑树** | [Guibas & Sedgewick (1978)](https://doi.org/10.1109/SFCS.1978.3) | 自平衡二叉搜索树经典实现，O(log n) 复杂度保证 |
| **莱昂纳多堆 / 平滑排序** | [Dijkstra (1981) EWD796a](https://www.cs.utexas.edu/~EWD/transcriptions/EWD07xx/EWD796a.html) | 自适应排序算法，最优时间复杂度 O(n) |
| **内省排序** | [Musser (1997)](https://doi.org/10.1002/(SICI)1097-024X(199708)27:8<983::AID-SPE117>3.0.CO;2-%23) | 混合快速/堆/插入排序，C++ 标准库 sort 默认算法 |
| **非加密哈希** | [FNV-1a 草案](https://datatracker.ietf.org/doc/html/draft-eastlake-fnv-17), [MurmurHash3](https://github.com/aappleby/smhasher/wiki/MurmurHash3) | 高性能哈希表与布隆过滤器专用 |
| **字节大小单位** | [IEC 80000-13:2008](https://www.iso.org/standard/31898.html), [IEEE 1541-2021](https://standards.ieee.org/ieee/1541/10790/), [BIPM SI Brochure (9th Ed.)](https://www.bipm.org/en/publications/si-brochure) | 二进制前缀 (KiB/MiB/GiB) 与十进制前缀 (kB/MB/GB) |

### ⚙️ 系统、并发与命令行

| 组件 | 遵循标准 | 说明 |
|------|----------|------|
| **原子内存序** | [ISO/IEC 14882:2020](https://www.iso.org/standard/79358.html) §31.4 | C++ 内存模型，含 x86/ARM 硬件屏障等价与 Intel TSX HLE 支持 |
| **命令行解析** | [POSIX.1-2017 (IEEE 1003.1)](https://pubs.opengroup.org/onlinepubs/9699919799/) 第12章, [GNU getopt_long](https://man7.org/linux/man-pages/man3/getopt.3.html) | 支持短选项组合、长选项、`--` 分隔符与可选值 |
| **日期与时间** | [ISO 8601-1:2019](https://www.iso.org/standard/70907.html), [RFC 3339](https://www.rfc-editor.org/rfc/rfc3339), [POSIX 时间戳](https://pubs.opengroup.org/onlinepubs/9699919799/) | 公历计算、儒略日转换与 Unix 纪元处理 |
| **SQL 语句构建器** | [ISO/IEC 9075](https://www.iso.org/standard/16663.html) (SQL-92 及后续版本) | 生成符合 ANSI SQL 的 SELECT/INSERT/UPDATE/DELETE 语句 |

### 🎨 图形、色彩与数学

| 组件 | 遵循标准 | 说明 |
|------|----------|------|
| **RGB 颜色模型** | [W3C CSS Color Level 4](https://www.w3.org/TR/css-color-4/), [Compositing Level 1](https://www.w3.org/TR/compositing-1/) | 直通 Alpha 合成、十六进制格式与 ANSI 256 色调色板 |
| **灰度转换** | [ITU-R BT.709](https://www.itu.int/rec/R-REC-BT.709/) / [IEC 61966-2-1 (sRGB)](https://webstore.iec.ch/publication/6169) | 人眼感知加权（0.299R + 0.587G + 0.114B） |
| **数学函数与常量** | [IEEE 754-2019](https://standards.ieee.org/ieee/754/6210/), [ISO/IEC 10967 (LIA)](https://www.iso.org/standard/24417.html) | 三角函数归约、牛顿迭代法与机器精度容差 |
| **随机数生成** | [ISO/IEC 18031:2011](https://www.iso.org/standard/54945.html), [NIST SP 800-90A](https://csrc.nist.gov/pubs/sp/800/90/a/r1/final) | 梅森旋转 (MT19937) 与操作系统熵源真随机数 |

> 📖 **文档完整性**：所有涉及上述标准的类与函数，均在其 API 注释（Doxygen 格式）中标注了具体的标准章节号与官方链接，开发者可随时溯源验证。

---

## 🚀 特性

### 🔄 并发与异步 (Async)
- **线程池** - 基于任务窃取的多策略线程池
- **协程支持** - 协程原语和生成器
- **虚拟线程** - 基于协程的轻量级虚拟线程
- **无锁队列** - 线程安全的无锁队列实现
- **同步原语** - 互斥锁、读写锁、信号量、线程屏障与闩锁
- **原子操作** - 原子类型、FUTEX、定时等待
- **Future/Promise** - 异步编程模型
- **危险指针** - 无锁数据结构的内存管理
- **停止令牌** - 可取消的异步操作

### 📦 容器 (Container)
- **标准容器** - vector、list、deque、map、set 等
- **红黑树** - 自平衡二叉搜索树实现
- **哈希表** - 开放寻址哈希表
- **布隆过滤器** - 概率性数据结构
- **LRU/TTL 缓存** - 基于最近最少使用/过期时间的缓存策略
- **位图/位集** - 高效位操作容器
- **莱昂纳多堆** - 莱昂纳多堆算法实现

### 🔐 加密与安全 (Encrypt)
- **AES256** - 高级加密标准实现
- **SHA1/SHA256** - 安全哈希算法
- **MD5** - 消息摘要算法
- **Base64** - 二进制数据编码
- **XOR** - 简单异或加密

### 📁 文件系统 (File)
- **路径/文件操作** - 路径/文件系统操作
- **文件监控** - 实时文件系统变更监控
- **配置文件解析** - JSON/TOML/INI/ENV 格式解析与流式构建
- **临时文件** - 安全的临时文件管理
- **系统管道** - 管道操作类
- **共享内存** - 跨进程共享内存

### 🌐 网络库 (Network)
- **WebSocket** - 全双工通信协议
- **TCP/UDP 套接字** - 高性能网络通信
- **SSL/TLS** - 加密网络传输
- **HTTP 客户端/服务器** - HTTP 协议实现，包含路由器、过滤器
- **DNS 客户端** - 域名解析
- **URL 解析** - URL 处理
- **ICMP/SMTP** - ICMP 和 SMTP 协议操作
- **ARP/MAC/IP/Ports** - 底层网络操作

### 🗄️ 数据库 (DB)
- **数据库连接池** - 连接复用与管理
- **SQL 构建器** - 标准 SQL 语句流式构建
- **多数据库支持**:
  - MySQL 客户端
  - PostgreSQL 客户端
  - SQLite 客户端
  - Redis 客户端
- **预处理语句** - 防止 SQL 注入
- **结果集封装** - 统一结果访问接口

### 📝 日志系统 (Logging)
- **多级别日志** - 支持不同日志级别
- **日志输出** - 日志文件管理与轮转
- **日志格式化** - 自定义日志格式
- **多接收器** - 可扩展的日志输出目标
- **日志器** - 灵活可配置的日志器

### 🔤 字符串处理 (String)
- **PCRE2 正则表达式** - 支持 JIT 的高效正则匹配
- **Unicode 支持** - UTF 转换系统、码点操作类
- **字符串格式化** - 类型安全的格式化输出
- **字符串视图** - 大量使用字符串视图优化操作
- **数值转换** - 字符串与数值互转

### ⚙️ 系统接口 (System)
- **进程管理** - 进程创建与控制
- **管道操作** - 管道创建与管理
- **动态库加载** - 运行时库加载
- **控制台操作** - 终端交互
- **进程参数解析** - 进程参数分析与操作
- **堆栈跟踪** - 异常调试
- **系统信息** - 硬件与 OS 信息
- **环境变量** - 环境变量操作
- **信号管理** - 信号控制

### ⏰ 时间处理 (Time)
- **高精度时钟** - 多种时钟源
- **时间点/时长** - 时间计算
- **日期时间** - 日历操作
- **范围计时** - 代码块执行时间测量

### 🛠️ 工具库 (Utility)
- **Optional** - 可选值处理
- **Variant** - 类型安全联合体
- **Expected** - 错误处理
- **Any** - 类型擦除容器
- **Tuple** - 编译期元组
- **Color** - RGB 颜色操作
- **Scope 操作** - 作用域守卫
- **数值信息** - 数值极限信息
- **数学比率** - 编译期比率计算
- **UUID** - UUID v4/v7 生成器
- **端序操作** - 大小端转换
- **断点调用** - 调试断点触发

### 🔍 反射系统 (Reflection)
- **反射注册表** - 类型反射与元信息管理
- **类型信息** - 运行时类型查询

### 🧬 类型与特性 (TypeInfo)
- **类型萃取** - 编译期类型判断
- **概念约束** - C++20 概念支持
- **类型检查** - 运行时类型信息
- **CRTP 静态多态** - 零开销接口统一

### 💾 内存管理 (Memory)
- **智能指针** - shared_ptr、unique_ptr、weak_ptr
- **原子智能指针** - `atomic<shared_ptr/weak_ptr>` 无锁操作
- **内存视图** - 安全内存访问
- **构造/析构工具** - 对象生命周期管理
- **内存跟踪** - 调试用内存监控
- **标准分配器** - 基于编译器特性的策略特化分配器
- **空基类压缩** - `compressed_pair` 优化

### 📦 压缩 (Compress)
- **lz4 压缩** - 高速数据压缩/解压
- **zlib 压缩** - 通用数据压缩/解压

### 🔌 插件系统 (Plugin)
- **动态插件管理** - 运行时加载卸载插件
- **插件接口** - 标准化插件开发

### ❗ 异常处理 (Exception)
- **异常指针** - 跨线程异常传递
- **终止处理** - 程序终止管理
- **异常系统** - 标准异常体系

### 📐 算法库 (Algorithm)
- **标准算法** - sort、find、transform 等
- **并行算法** - 并行执行策略
- **数值算法** - 数值计算与累加
- **堆算法** - 堆操作与优先级队列
- **范围操作** - ranges 库支持
- **哈希算法** - 多种哈希函数实现
- **位操作** - 位操作函数系列

### 📊 数学库 (Math)
- **数学常量** - 常用数学常数
- **数学函数** - 超越函数与数值计算
- **随机数生成** - LC、梅森旋转、硬件噪声算法

---

## 🔧 编译指南

### 📋 前置依赖

| 类型 | 依赖 | 版本要求   |
|------|------|--------|
| 🔨 构建工具 | [CMake](https://cmake.org/) | 3.19+  |
| 📦 包管理器 | [vcpkg](https://github.com/microsoft/vcpkg) | Latest |
| 🎨 代码格式化 | [clang-format](https://clang.llvm.org/docs/ClangFormat.html) | 19+    |
| 🔍 静态分析 | [clang-tidy](https://clang.llvm.org/extra/clang-tidy/) | 19+    |
| ⚠️ 必选依赖 | [GTest](https://google.github.io/googletest/) | 1.17.0+ |
| | [pcre2](https://www.pcre.org/) | 10.47+ |
| | [OpenSSL](https://www.openssl.org/) | 3.6.1+ |
| 📦 可选依赖 | [libpq](https://www.postgresql.org/) | 16.9+  |
| | [libmysql](https://www.mysql.com/) | 8.0.40+ |
| | [sqlite3](https://sqlite.org/index.html) | 3.51.2+ |
| | [hiredis](https://redis.ac.cn/docs/latest/develop/clients/hiredis/) | 1.3.0+ |
| | [lz4](https://lz4.org/) | 1.10.0+ |
| | [zlib](https://www.zlib.net/) | 1.3.1+ |

### 🏗️ 编译步骤

> 💡 您可以在项目根目录的 `config.json` 中更改对外配置项以进行个性化编译。

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
```

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

- [ ] 📊 Google Benchmark 性能基准测试
- [ ] ⚡ 热点代码优化
- [ ] 🍎 支持 macOS 平台
- [ ] 🖥️ 支持 ARM / RISC-V / LOONG ARCH 架构

更多 TODO 条例参见代码内的 TODO 注释
