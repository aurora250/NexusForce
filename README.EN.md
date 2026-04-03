# NexusForce V1.0.0

[![vcpkg](https://img.shields.io/badge/vcpkg-Enabled-0A7FAA?style=flat-square&logo=vcpkg&logoColor=white)](https://vcpkg.io)
[![CMake](https://img.shields.io/badge/CMake-3.19+-064C8B?style=flat-square&logo=cmake&logoColor=white)](https://cmake.org)
[![C++](https://img.shields.io/badge/C++-14/17/20-00599C?style=flat-square&logo=cplusplus&logoColor=white)](https://isocpp.org)
[![Valgrind](https://img.shields.io/badge/Valgrind-Tested-2E8B57?style=flat-square&logo=valgrind&logoColor=white)](https://valgrind.org)
[![Memory Leak](https://img.shields.io/badge/Memory%20Leak-None-00C853?style=flat-square)](valgrind)
[![CodeQL](https://img.shields.io/badge/CodeQL-Analyzed-1E6F9F?style=flat-square&logo=github&logoColor=white)](https://codeql.github.com)
[![License](https://img.shields.io/badge/License-MIT-F9A825?style=flat-square)](https://opensource.org/licenses/MIT)
[![Docs](https://img.shields.io/badge/Docs-Website-1565C0?style=flat-square&logo=readthedocs&logoColor=white)](https://nexusforce.org.cn)
[![PRs Welcome](https://img.shields.io/badge/PRs-Welcome-4CAF50?style=flat-square&logo=git&logoColor=white)](https://github.com/aurora250/NexusForce/pulls)

> Read this in other languages: [中文 (Chinese)](README.md)

This project aims to establish a feature-complete, stylistically unified, highly readable, community-driven, and cross-platform compatible modern C++ development library. Through clear architectural design, standardized code implementation, and rich applications of design patterns, it provides a practical toolkit for project development while also serving as a practical learning resource for C++ learners to understand underlying principles, bridging the gap from learning to production.

Please feel free to submit issues to help improve this project. If there are any deficiencies, please don't hesitate to provide feedback.

This library assumes your operating system uses the UTF-8 character set when working with I/O devices. If not, please try to configure it; otherwise, garbled characters may occur.

## Supported Environments

| Platform  | Instruction Set | Bit Width | Compiler     | C++ Standard |
|-----------|-----------------|-----------|--------------|--------------|
| WINDOWS   | X86             | 32-bit    | MSVC         | 14           |
| LINUX     |                 | 64-bit    | MinGW        | 17           |
|           |                 |           | GCC          | 20           |
|           |                 |           | Clang        |              |

## Features

### Concurrency & Async

- **Thread Pool** - Multi-strategy thread pool based on work stealing
- **Coroutine Support** - Coroutine primitives and generators
- **Virtual Threads** - Lightweight virtual threads based on coroutines
- **Lock-Free Queue** - Thread-safe lock-free queue implementation
- **Synchronization Primitives** - Mutex, shared mutex, semaphore, barrier, and latch
- **Atomic Operations** - Atomic types, FUTEX, timed waiting
- **Future/Promise** - Asynchronous programming model
- **Hazard Pointer** - Memory management for lock-free data structures
- **Stop Token** - Cancellable asynchronous operations

### Containers

- **Standard Containers** - vector, list, deque, map, set, etc.
- **Red-Black Tree** - Self-balancing binary search tree implementation
- **Hash Table** - Open addressing hash table
- **Bloom Filter** - Probabilistic data structure
- **LRU/TTL Cache** - Cache policies based on Least Recently Used / Time-To-Live
- **Bitmap/Bitset** - Efficient bit manipulation container
- **Leonardo Heap** - Leonardo heap algorithm implementation

### Encryption & Security

- **AES256** - Advanced Encryption Standard implementation
- **SHA1/SHA256** - Secure Hash Algorithm
- **MD5** - Message Digest Algorithm
- **Base64** - Binary data encoding
- **XOR** - Simple XOR encryption

### File System

- **Path/File Operations** - Path and file system operations
- **File Watcher** - Real-time file system change monitoring
- **Config File Parsing** - JSON/TOML/INI/ENV format parsing and streaming builder
- **Temporary File** - Secure temporary file management
- **System Pipe** - Pipe operation class
- **Shared Memory** - Cross-process shared memory

### Networking

- **WebSocket** - Full-duplex communication protocol
- **TCP/UDP Socket** - High-performance network communication
- **SSL/TLS** - Encrypted network transport
- **HTTP Client/Server** - HTTP protocol implementation, including routers and filters
- **DNS Client** - Domain name resolution
- **URL Parser** - URL handling
- **ICMP/SMTP** - ICMP and SMTP protocol operations
- **ARP/MAC/IP/Ports** - Low-level network operations

### Database

- **Database Connection Pool** - Connection reuse and management
- **SQL Builder** - Fluent builder for standard SQL statements
- **Multi-Database Support**:
  - MySQL Client
  - PostgreSQL Client
  - SQLite Client
  - Redis Client
- **Prepared Statements** - Prevention of SQL injection
- **Result Set Wrapper** - Unified result access interface

### Logging

- **Multi-Level Logging** - Support for different log levels
- **Log Output** - Log file management and rotation
- **Log Formatting** - Customizable log format
- **Multi-Sink** - Extensible log output targets
- **Loggers** - Flexible and configurable loggers

### String Processing

- **PCRE2 Regular Expressions** - Efficient regular expression matching with JIT support
- **Unicode Support** - UTF conversion system, codepoint operation classes
- **String Formatting** - Type-safe formatted output
- **String View** - Extensive use of string views for optimized operations
- **Numeric Conversion** - Conversion between strings and numeric values

### System Interface

- **Process Management** - Process creation and control
- **Pipe Operations** - Pipe creation and management
- **Dynamic Library Loading** - Runtime library loading
- **Console Operations** - Terminal interaction
- **Process Argument Parsing** - Analysis and manipulation of process arguments
- **Stack Trace** - Exception debugging
- **System Information** - Hardware and OS information
- **Environment Variables** - Environment variable manipulation
- **Signal Management** - Signal control

### Time Utilities

- **High-Resolution Clock** - Multiple clock sources
- **Time Point / Duration** - Time calculations
- **Date Time** - Calendar operations
- **Scope Timer** - Execution time measurement for code blocks

### Utility Library

- **Optional** - Optional value handling
- **Variant** - Type-safe union
- **Expected** - Error handling
- **Any** - Type-erased container
- **Tuple** - Compile-time tuple
- **Color** - RGB color operations
- **Scope Operations** - Scope guards
- **Numeric Limits** - Numerical limit information
- **Math Ratio** - Compile-time ratio calculations
- **UUID** - UUID v4/v7 generator
- **Endian Operations** - Endianness conversion
- **Breakpoint Call** - Debug breakpoint trigger

### Reflection System

- **Reflection Registry** - Type reflection and metadata management
- **Type Information** - Runtime type querying

### Type Traits & Concepts

- **Type Traits** - Compile-time type judgments
- **Concept Constraints** - C++20 concept support
- **Type Checking** - Runtime type information
- **CRTP Static Polymorphism** - Zero-overhead interface unification

### Memory Management

- **Smart Pointers** - shared_ptr, unique_ptr, weak_ptr
- **Atomic Smart Pointers** - Lock-free operations for atomic\<shared_ptr/weak_ptr\>
- **Memory View** - Safe memory access
- **Construction/Destruction Tools** - Object lifecycle management
- **Memory Tracing** - Memory monitoring for debugging
- **Standard Allocators** - Strategy-specialized allocators based on compiler features
- **Empty Base Class Optimization** - compressed_pair utility

### Compression

- **lz4 Compression** - High-speed data compression/decompression
- **zlib Compression** - General-purpose data compression/decompression

### Plugin System

- **Dynamic Plugin Management** - Runtime plugin loading and unloading
- **Plugin Interface** - Standardized plugin development

### Exception Handling

- **Exception Pointer** - Cross-thread exception propagation
- **Termination Handling** - Program termination management
- **Exception System** - Standard exception hierarchy

### Algorithm Library

- **Standard Algorithms** - sort, find, transform, etc.
- **Parallel Algorithms** - Parallel execution policies
- **Numeric Algorithms** - Numerical computation and accumulation
- **Heap Algorithms** - Heap operations and priority queues
- **Range Operations** - Ranges library support
- **Hash Algorithms** - Multiple hash function implementations
- **Bit Operations** - Series of bit manipulation functions

### Math Library

- **Mathematical Constants** - Common mathematical constants
- **Mathematical Functions** - Transcendental functions and numerical computation
- **Random Number Generation** - LC, Mersenne Twister, hardware noise algorithms

## Build Guide

### Prerequisites

- [CMake](https://cmake.org/) 3.19+
- [vcpkg](https://github.com/microsoft/vcpkg)
- [clang-format](https://clang.llvm.org/docs/ClangFormat.html) 19+
- Mandatory Dependencies:
  - pcre2
  - OpenSSL
- Optional Dependencies:
  - PostgreSQL
  - MySQL
  - SQLite3
  - hiredis
  - lz4
  - zlib

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

## Changelog

For the changelog, please see [CHANGELOG](CHANGELOG.md)

## TODO

- Google Benchmark tests
- Hotspot code optimization
- Support for macOS
- Support for ARM / RISC-V / LOONGARCH architectures
