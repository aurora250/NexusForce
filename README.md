# NexusForce V1.0.0

[![Build Status](https://travis-ci.org/aurora250/NexusForce.svg?branch=master)](https://travis-ci.org/aurora250/NexusForce)
[![License](https://img.shields.io/badge/License-MIT%20License-blue.svg)](https://opensource.org/licenses/MIT)

> 通过其他语言阅读: [English](README.EN.md)

本项目旨在建立功能健全、风格统一、可读性强、社区共建、跨平台兼容的现代C++开发库。
通过清晰的架构设计、规范的代码实现、丰富的设计模式应用， 为项目开发提供实用的工具集，
同时也为C++学习者提供理解底层原理的实践载体，建立从学习到生产的连接点。

有劳各位多多issue，使本项目趋于健全。如有不足，还望斧正。

本库使用IO设备时默认您的操作系统字符集为UTF-8，如不是，请尝试设置，否则可能乱码。

## 支持环境

| 平台        | 指令集 | 位宽  | 编译器     | C++标准 |
|-----------|-----|-----|---------|-------|
| WINDOWS   | X86 | 32位 | MSVC    | 14    |
| LINUX     |     | 64位 | MinGW   | 17    |
|           |     |     | GCC     | 20    |
|           |     |     | Clang   |       |

## 特性

### 并发与异步 (Async)

- **线程池** - 基于任务窃取的多策略线程池
- **协程支持** - 协程原语和生成器
- **无锁队列** - 线程安全的无锁队列实现
- **同步原语** - 互斥锁、读写锁、信号量、屏障、门闩等
- **原子操作** - 原子类型、FUTEX、定时等待
- **Future/Promise** - 异步编程模型
- **危险指针** - 无锁数据结构的内存管理
- **停止令牌** - 可取消的异步操作

### 容器 (Container)

- **标准容器** - vector、list、deque、map、set等
- **红黑树** - 自平衡二叉搜索树实现
- **哈希表** - 开放寻址哈希表
- **布隆过滤器** - 概率性数据结构
- **位图/位集** - 高效位操作容器

### 加密与安全 (Encrypt)

- **AES256** - 高级加密标准实现
- **SHA1/SHA256** - 安全哈希算法
- **MD5** - 消息摘要算法
- **Base64** - 二进制数据编码
- **XOR加密** - 简单异或加密

### 文件系统 (File)

- **路径/文件操作** - 路径/文件系统操作
- **文件监控** - 实时文件系统变更监控
- **配置文件解析** - JSON/TOML/INI/ENV格式解析与流式构建
- **临时文件** - 安全的临时文件管理

### 网络库 (Network)

- **WebSocket支持** - 全双工通信协议
- **TCP/UDP套接字** - 高性能网络通信
- **SSL/TLS支持** - 加密网络传输
- **HTTP客户端/服务器** - HTTP协议实现，包含路由器、过滤器
- **DNS客户端** - 域名解析
- **URL解析** - URL处理

### 数据库 (DB)

- **数据库连接池** - 连接复用与管理
- **SQL构建器** - 标准SQL语句流式构建
- **多数据库支持**:
  - MySQL客户端
  - PostgreSQL客户端
  - SQLite客户端
  - Redis客户端
- **预处理语句** - 防止SQL注入
- **结果集封装** - 统一结果访问接口

### 日志系统 (Logging)

- **多级别日志** - 支持不同日志级别
- **文件输出** - 日志文件管理与轮转
- **日志格式化** - 自定义日志格式
- **多接收器** - 可扩展的日志输出目标

### 字符串处理 (String)

- **PCRE2正则表达式** - 高效强大的正则匹配
- **字符串格式化** - 类型安全的格式化输出
- **字符串视图** - 大量使用字符串视图优化操作
- **数值转换** - 字符串与数值互转

### 系统接口 (System)

- **进程管理** - 进程创建与控制
- **管道操作** - 管道创建与管理
- **动态库加载** - 运行时库加载
- **控制台操作** - 终端交互
- **进程参数解析** - 进程参数分析与操作
- **堆栈跟踪** - 异常调试
- **系统信息** - 硬件与OS信息

### 时间处理 (Time)

- **高精度时钟** - 多种时钟源
- **时间点/时长** - 时间计算
- **日期时间** - 日历操作

### 工具库 (Utility)

- **Optional** - 可选值处理
- **Variant** - 类型安全联合体
- **Expected** - 错误处理
- **Any** - 类型擦除容器
- **Tuple** - 编译期元组
- **Color** - RGB颜色操作

### 类型与特性 (TypeInfo)

- **类型萃取** - 编译期类型判断
- **概念约束** - C++20概念支持
- **类型检查** - 运行时类型信息

### 内存管理 (Memory)

- **智能指针** - shared_ptr、unique_ptr、weak_ptr
- **内存视图** - 安全内存访问
- **构造/析构工具** - 对象生命周期管理
- **内存跟踪** - 调试用内存监控

### 压缩 (Compress)

- **lz4/zlib压缩** - 数据压缩/解压

### 插件系统 (Plugin)

- **动态插件管理** - 运行时加载卸载插件
- **插件接口** - 标准化插件开发

### 异常处理 (Exception)

- **断点调试** - 条件断点触发
- **终止处理** - 程序终止管理
- **异常指针** - 跨线程异常传递

## 编译指南

### 前置依赖

- CMake 3.19+
- 必选依赖:
  - pcre2
  - mimalloc
- 可选依赖：
  - PostgreSQL
  - MySQL
  - SQLite3
  - hiredis
  - lz4
  - zlib
  - OpenSSL

### 编译步骤

您可以在项目根目录的config.json中更改对外配置项以进行个性化编译

- Windows

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

- Linux

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

## 文档

文档请参见 [NexusForce](https://nexusforce.org.cn)

## 协议

本项目基于 [MIT 开源协议](LICENSE)

## 更新日志

更新日志参见 [CHANGELOG](CHANGELOG.md)

## TODO

- 接入 vcpkg
- 支持 macOS
- 支持 ARM / RISC-V / LOONG ARCH
