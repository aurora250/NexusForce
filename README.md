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

## 编译指南

### 前置依赖

- CMake 3.19+
- 可选依赖：
  - PostGreSQL
  - MySQL
  - SQLite3
  - hiredis
  - zlib
  - OpenSSL
  - pcre2

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

## 版本

## TODO

- 支持 macOS
- 支持 ARM / RISC-V / LOONG ARCH
