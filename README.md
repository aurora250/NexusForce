# MSTL V1.4.0

[![Build Status](https://travis-ci.org/aurora250/MSTL.svg?branch=master)](https://travis-ci.org/aurora250/MSTL)
[![License](https://img.shields.io/badge/License-MIT%20License-blue.svg)](https://opensource.org/licenses/MIT)

> 通过其他语言阅读: [English](README.EN.md)

本项目旨在建立功能健全、风格统一、可读性强、社区共建、跨平台兼容的
现代C++开发库MSTL(Modern Standard Template Library)。
通过清晰的架构设计、规范的代码实现、丰富的设计模式应用，
为项目开发提供实用的工具集，同时也为C++学习者提供理解底层原理的实践载体。
有劳各位多多issue，使本项目趋于健全。如有不足，还望斧正。

本库使用IO设备时默认您的操作系统字符集为UTF-8，如不是，请尝试设置，否则可能在IO时乱码。

## 支持环境

WINDOWS LINUX

X64 X86

MSVC GCC CLANG

C++ 14 17 20

## 编译指南

### 前置依赖

- CMake 3.17+
- 支持C++14及以上的编译器
- 可选依赖：
  - PostGreSQL
  - MySQL
  - SQLite3
  - hiredis
  - zlib
  - OpenSSL

### 编译步骤

您可以在项目根目录的CMakeLists.txt中开关依赖项并在src\CMakeLists.txt中直接更改您本地的依赖路径以进行个性化编译

- Windows

```bash
# 克隆最新发布版
git clone --depth 1 https://github.com/aurora250/MSTL.git
cd MSTL

# 创建构建目录
mkdir build && cd build

# 编译选项配置，您也可以在CMakeLists.txt内直接更改
cmake .. -G "Visual Studio 17 2022" -A x64 \
  -DMSTL_ENABLE_MYSQL=OFF \
  -DMSTL_BUILD_TESTS=ON \
  -DMYSQL_ROOT_DIR="C:/Program Files/MySQL/MySQL Server 8.0"

# 编译
cmake --build . --config Release

# 安装到系统目录
cmake --install . --config Release
```

- Linux

```bash
# 克隆最新发布版
git clone --depth 1 https://github.com/aurora250/MSTL.git
cd MSTL

# 创建构建目录
mkdir build && cd build

# 编译选项配置，您也可以在CMakeLists.txt内直接更改
cmake .. -DCMAKE_BUILD_TYPE=Release \
  -DMSTL_ENABLE_MYSQL=OFF \
  -DMSTL_BUILD_TESTS=ON

# 编译
make -j$(nproc)

# 安装到系统目录
sudo make install
```

## 文档

文档请参见 [MSTL文档 - 专题首页](https://aurora250.github.io/MSTL/topics.html)

## 开源协议

本项目基于 [MIT 开源协议](LICENSE) 。

## 待实现功能

- basic_string/vector SSO
- Doxygen注释及文档
- 统一assertion提示
- 接入vcpkg/conan
- 支持yaml、xml配置
- 支持macOS、嵌入式Linux
