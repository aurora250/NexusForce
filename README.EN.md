# NexusForce V1.0.0

[![Build Status](https://travis-ci.org/aurora250/NexusForce.svg?branch=master)](https://travis-ci.org/aurora250/NexusForce)
[![License](https://img.shields.io/badge/License-MIT%20License-blue.svg)](https://opensource.org/licenses/MIT)

> Read this in other languages: [中文 (Chinese)](README.md)

This project aims to establish a feature-complete, stylistically unified, 
highly readable, community-driven, and cross-platform compatible modern C++ development library. 
Through clear architectural design, standardized code implementation, 
and rich applications of design patterns, it provides a practical toolkit for 
project development while also serving as a practical learning resource for C++ beginners 
to understand underlying principles, bridging the gap from learning to production.

Please feel free to submit issues to help improve this project.
If there are any deficiencies, please don't hesitate to provide feedback.

This library assumes your operating system uses UTF-8 character set 
when working with I/O devices. If not, please try to configure it; 
otherwise, garbled characters may occur.

## Supported Environments

| Platform  | Instruction Set | Bit Width | Compiler | C++ Standard |
|-----------|-----------------|-----------|----------|--------------|
| WINDOWS   | X86             | 32-bit    | MSVC     | 14           |
| LINUX     |                 | 64-bit    | MinGW    | 17           |
|           |                 |           | GCC      | 20           |
|           |                 |           | CLANG    |              |

## Build Guide

### Prerequisites

- CMake 3.19+
- Optional dependencies:
  - PostgreSQL
  - MySQL
  - SQLite3
  - hiredis
  - zlib
  - OpenSSL
  - pcre2

### Build Steps

You can modify the configuration items in `config.json` in the project root directory for personalized builds.

- Windows

```bash
# Clone the latest release
git clone --depth 1 https://github.com/aurora250/NexusForce.git
cd NexusForce

# Create build directory
mkdir build && cd build

# Configure build options
cmake .. -G "Visual Studio 17 2022" -A x64

# Build
cmake --build . --config Release

# Install to system directory
cmake --install . --config Release
```

- Linux

```bash
# Clone the latest release
git clone --depth 1 https://github.com/aurora250/NexusForce.git
cd NexusForce

# Create build directory
mkdir build && cd build

# Configure build options
cmake ..

# Build
make -j$(nproc)

# Install to system directory
sudo make install
```

## Documentation

For documentation, please visit [NexusForce](https://nexusforce.org.cn)

## License

This project is licensed under the [MIT License](LICENSE)

## CHANGELOG

For CHANGELOG, please visit [CHANGELOG](CHANGELOG.md)

## TODO

- Support for macOS
- Support for ARM / RISC-V / LOONGARCH architectures
