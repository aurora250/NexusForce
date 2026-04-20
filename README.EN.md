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

> 🌐 **Language**  
> Read this in other languages: [中文 (Chinese)](README.md)

---

## 📑 Table of Contents

- [Project Overview](#-project-overview)
- [Supported Environments](#️-supported-environments)
- [Engineering Quality](#-engineering-quality)
- [Standards Compliance](#-standards-compliance)
- [Features](#-features)
- [Build Guide](#-build-guide)
- [Documentation](#-documentation)
- [License](#️-license)
- [Changelog](#-changelog)
- [Contributors](#-contributors)
- [TODO](#-todo)

---

## 📖 Project Overview

This project aims to establish a **feature-complete, stylistically unified, highly readable, community-driven, and cross-platform compatible** modern C++ development library. Through clear architectural design, standardized code implementation, and rich applications of design patterns, it provides a practical toolkit for project development while also serving as a practical learning resource for C++ learners to understand underlying principles, bridging the gap from learning to production.

💡 Please feel free to [submit Issues](https://github.com/aurora250/NexusForce/issues) to help improve this project. If there are any deficiencies, please don't hesitate to provide feedback.

> ℹ️ **Character Encoding Notice**  
> This library assumes the default system code page of the runtime environment is **UTF-8**. If using I/O related functionalities in a non-UTF-8 environment, please ensure the UTF-8 locale is correctly configured to avoid potential character behavior anomalies.

---

## 🖥️ Supported Environments

| Platform | Instruction Set | Bit Width | Compiler | C++ Standard |
|----------|-----------------|-----------|----------|--------------|
| 🪟 WINDOWS | X86 | 32-bit | MSVC | 14 |
| 🐧 LINUX | | 64-bit | MinGW | 17 |
| | | | GCC | 20 |
| | | | Clang | |

---

## ✨ Engineering Quality

NexusForce strictly adheres to modern C++ engineering best practices, ensuring code robustness and readability through multi-layered automated checks.

| Metric | Status | Description |
|--------|--------|-------------|
| 📊 **Codebase Size** | 120k+ Lines | Core library source and header files |
| 🔒 **CodeQL Security Analysis** | **0 Vulnerabilities** | Full `security-and-quality` suite, zero security alerts |
| 🔍 **Clang-Tidy Static Analysis** | **Zero Warnings** | Full ruleset (`bugprone`/`cppcoreguidelines`/`hicpp`/`modernize`/`performance`/`readability`), warnings as errors |
| 🎨 **Clang-Format Code Style** | **Strictly Enforced** | 120 columns, 4 spaces, K&R variant braces, mandatory brace insertion, etc. |
| 💧 **Dynamic Memory Check** | **0 Leaks** | Valgrind full test suite, zero memory leaks or out-of-bounds access |

> 📋 **Regarding Rule Exemptions**: [`.clang-tidy`](.clang-tidy) contains approximately 60 explicit exemptions, and [`.clang-format`](.clang-format) includes several style customizations. Each exemption addresses inherent requirements of low-level system programming, adhering to the principle of "strict by default, relaxed as needed."

---

## 📡 Standards Compliance

The core components of NexusForce strictly adhere to relevant international standards and industry specifications, ensuring predictable behavior, strong interoperability, and reliable security. The following table maps key components to their respective standards:

### 🌐 Network Protocols & Internet Standards

| Component | Standards Followed | Description |
|-----------|-------------------|-------------|
| **URL Parsing & Encoding** | [RFC 3986](https://www.rfc-editor.org/rfc/rfc3986), [RFC 3987](https://www.rfc-editor.org/rfc/rfc3987), [WHATWG URL](https://url.spec.whatwg.org/) | Generic URI syntax, percent-encoding, and Internationalized Resource Identifiers |
| **Network Port Definitions** | [IANA Service Name and Transport Protocol Port Number Registry](https://www.iana.org/assignments/service-names-port-numbers/), [RFC 6335](https://www.rfc-editor.org/rfc/rfc6335) | Well-known port assignments for HTTP/HTTPS/FTP/SSH/DNS, etc. |
| **Base64 Encoding** | [RFC 4648](https://www.rfc-editor.org/rfc/rfc4648) | Standard Base64 and URL-safe Base64 character alphabets |
| **JSON Data Format** | [RFC 8259](https://www.rfc-editor.org/rfc/rfc8259), [ECMA-404](https://ecma-international.org/publications-and-standards/standards/ecma-404/) | JSON syntax, data types, and UTF-8 encoding requirements |
| **TOML Configuration Format** | [TOML v1.0.0](https://toml.io/en/v1.0.0) | Includes date-time format following [RFC 3339](https://www.rfc-editor.org/rfc/rfc3339) / ISO 8601 |
| **UUID Generation** | [RFC 4122](https://www.rfc-editor.org/rfc/rfc4122), [RFC 9562](https://www.rfc-editor.org/rfc/rfc9562) | UUID v4 (random) and v7 (time-ordered) generation specifications |

### 🔐 Cryptography & Security Algorithms

| Component | Standards Followed | Description |
|-----------|-------------------|-------------|
| **AES-256 Encryption** | [NIST FIPS 197](https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.197-upd1.pdf), [ISO/IEC 18033-3](https://www.iso.org/standard/54531.html) | Advanced Encryption Standard, supports ECB/CBC/GCM modes ([NIST SP 800-38A](https://csrc.nist.gov/pubs/sp/800/38/a/final) / [SP 800-38D](https://csrc.nist.gov/pubs/sp/800/38/d/final)) |
| **SHA-256 Hashing** | [NIST FIPS 180-4](https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.180-4.pdf), [RFC 6234](https://www.rfc-editor.org/rfc/rfc6234) | Secure Hash Algorithm (SHA-2 family), 256-bit output |
| **SHA-1 Hashing** | [NIST FIPS 180-4](https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.180-4.pdf) (historical compatibility) | ⚠️ Marked with security warning ([SHAttered](https://shattered.io/) collision attack) |
| **MD5 Hashing** | [RFC 1321](https://www.rfc-editor.org/rfc/rfc1321) (historical compatibility) | ⚠️ Marked with security warning, for non-cryptographic checksum scenarios only |
| **PKCS#7 Padding** | [RFC 5652](https://www.rfc-editor.org/rfc/rfc5652), [RFC 8018](https://www.rfc-editor.org/rfc/rfc8018) | Cryptographic Message Syntax and password-based encryption padding scheme |

### 🔤 Character Encoding & Internationalization

| Component | Standards Followed | Description |
|-----------|-------------------|-------------|
| **UTF-8 / UTF-16 / UTF-32** | [Unicode 15.1.0](https://unicode.org/versions/Unicode15.1.0/), [ISO/IEC 10646](https://www.iso.org/standard/76835.html), [RFC 3629](https://www.rfc-editor.org/rfc/rfc3629) / [RFC 2781](https://www.rfc-editor.org/rfc/rfc2781) | Unicode codepoint operations, normalization, and encoding conversion with invalid sequence detection |
| **Unicode Codepoint Processing** | [Unicode 15.1.0](https://unicode.org/versions/Unicode15.1.0/) §2.4, §2.13 | Surrogate pair handling, BOM detection, and replacement character (U+FFFD) rules |

### 📐 Data Structures & Algorithms

| Component | Standards Followed / Academic Literature | Description |
|-----------|------------------------------------------|-------------|
| **Heap Algorithms** | [ISO/IEC 14882:2020](https://www.iso.org/standard/79358.html) §25.8.6 | Complexity guarantees and Floyd's heap adjustment optimization ([Algorithm 245](https://dl.acm.org/doi/10.1145/512274.512284)) |
| **Red-Black Tree** | [Guibas & Sedgewick (1978)](https://doi.org/10.1109/SFCS.1978.3) | Classic implementation of self-balancing binary search tree, O(log n) complexity guarantee |
| **Leonardo Heap / Smoothsort** | [Dijkstra (1981) EWD796a](https://www.cs.utexas.edu/~EWD/transcriptions/EWD07xx/EWD796a.html) | Adaptive sorting algorithm, optimal time complexity O(n) |
| **Introsort** | [Musser (1997)](https://doi.org/10.1002/(SICI)1097-024X(199708)27:8<983::AID-SPE117>3.0.CO;2-%23) | Hybrid quick/heap/insertion sort, default algorithm for C++ standard library `sort` |
| **Non-cryptographic Hashing** | [FNV-1a Draft](https://datatracker.ietf.org/doc/html/draft-eastlake-fnv-17), [MurmurHash3](https://github.com/aappleby/smhasher/wiki/MurmurHash3) | High-performance hash tables and Bloom filters |

### ⚙️ System, Concurrency & Command-Line

| Component | Standards Followed | Description |
|-----------|-------------------|-------------|
| **Atomic Memory Order** | [ISO/IEC 14882:2020](https://www.iso.org/standard/79358.html) §31.4 | C++ memory model, includes x86/ARM hardware barrier equivalents and Intel TSX HLE support |
| **Command-Line Parsing** | [POSIX.1-2017 (IEEE 1003.1)](https://pubs.opengroup.org/onlinepubs/9699919799/) Chapter 12, [GNU getopt_long](https://man7.org/linux/man-pages/man3/getopt.3.html) | Supports short option grouping, long options, `--` delimiter, and optional values |
| **Date & Time** | [ISO 8601-1:2019](https://www.iso.org/standard/70907.html), [RFC 3339](https://www.rfc-editor.org/rfc/rfc3339), [POSIX Timestamp](https://pubs.opengroup.org/onlinepubs/9699919799/) | Gregorian calendar calculations, Julian day conversion, and Unix epoch handling |
| **SQL Statement Builder** | [ISO/IEC 9075](https://www.iso.org/standard/16663.html) (SQL-92 and later) | Generates ANSI SQL compliant SELECT/INSERT/UPDATE/DELETE statements |

### 🎨 Graphics, Color & Mathematics

| Component | Standards Followed | Description |
|-----------|-------------------|-------------|
| **RGB Color Model** | [W3C CSS Color Level 4](https://www.w3.org/TR/css-color-4/), [Compositing Level 1](https://www.w3.org/TR/compositing-1/) | Straight alpha compositing, hexadecimal formats, and ANSI 256-color palette |
| **Grayscale Conversion** | [ITU-R BT.709](https://www.itu.int/rec/R-REC-BT.709/) / [IEC 61966-2-1 (sRGB)](https://webstore.iec.ch/publication/6169) | Human perception weighting (0.299R + 0.587G + 0.114B) |
| **Mathematical Functions & Constants** | [IEEE 754-2019](https://standards.ieee.org/ieee/754/6210/), [ISO/IEC 10967 (LIA)](https://www.iso.org/standard/24417.html) | Trigonometric reduction, Newton's method iterations, and machine epsilon tolerances |
| **Random Number Generation** | [ISO/IEC 18031:2011](https://www.iso.org/standard/54945.html), [NIST SP 800-90A](https://csrc.nist.gov/pubs/sp/800/90/a/r1/final) | Mersenne Twister (MT19937) and OS entropy source true random numbers |

> 📖 **Documentation Completeness**: All classes and functions referencing the above standards include specific standard section numbers and official links in their API comments (Doxygen format), enabling developers to trace and verify at any time.

---

## 🚀 Features

### 🔄 Concurrency & Async
- **Thread Pool** - Multi-strategy thread pool based on work stealing
- **Coroutine Support** - Coroutine primitives and generators
- **Virtual Threads** - Lightweight virtual threads based on coroutines
- **Lock-Free Queue** - Thread-safe lock-free queue implementation
- **Synchronization Primitives** - Mutex, shared mutex, semaphore, barrier, and latch
- **Atomic Operations** - Atomic types, FUTEX, timed waiting
- **Future/Promise** - Asynchronous programming model
- **Hazard Pointer** - Memory management for lock-free data structures
- **Stop Token** - Cancellable asynchronous operations

### 📦 Containers
- **Standard Containers** - vector, list, deque, map, set, etc.
- **Red-Black Tree** - Self-balancing binary search tree implementation
- **Hash Table** - Open addressing hash table
- **Bloom Filter** - Probabilistic data structure
- **LRU/TTL Cache** - Cache policies based on Least Recently Used / Time-To-Live
- **Bitmap/Bitset** - Efficient bit manipulation container
- **Leonardo Heap** - Leonardo heap algorithm implementation

### 🔐 Encryption & Security
- **AES256** - Advanced Encryption Standard implementation
- **SHA1/SHA256** - Secure Hash Algorithm
- **MD5** - Message Digest Algorithm
- **Base64** - Binary data encoding
- **XOR** - Simple XOR encryption

### 📁 File System
- **Path/File Operations** - Path and file system operations
- **File Watcher** - Real-time file system change monitoring
- **Config File Parsing** - JSON/TOML/INI/ENV format parsing and streaming builder
- **Temporary File** - Secure temporary file management
- **System Pipe** - Pipe operation class
- **Shared Memory** - Cross-process shared memory

### 🌐 Networking
- **WebSocket** - Full-duplex communication protocol
- **TCP/UDP Socket** - High-performance network communication
- **SSL/TLS** - Encrypted network transport
- **HTTP Client/Server** - HTTP protocol implementation, including routers and filters
- **DNS Client** - Domain name resolution
- **URL Parser** - URL handling
- **ICMP/SMTP** - ICMP and SMTP protocol operations
- **ARP/MAC/IP/Ports** - Low-level network operations

### 🗄️ Database
- **Database Connection Pool** - Connection reuse and management
- **SQL Builder** - Fluent builder for standard SQL statements
- **Multi-Database Support**:
  - MySQL Client
  - PostgreSQL Client
  - SQLite Client
  - Redis Client
- **Prepared Statements** - Prevention of SQL injection
- **Result Set Wrapper** - Unified result access interface

### 📝 Logging
- **Multi-Level Logging** - Support for different log levels
- **Log Output** - Log file management and rotation
- **Log Formatting** - Customizable log format
- **Multi-Sink** - Extensible log output targets
- **Loggers** - Flexible and configurable loggers

### 🔤 String Processing
- **PCRE2 Regular Expressions** - Efficient regular expression matching with JIT support
- **Unicode Support** - UTF conversion system, codepoint operation classes
- **String Formatting** - Type-safe formatted output
- **String View** - Extensive use of string views for optimized operations
- **Numeric Conversion** - Conversion between strings and numeric values

### ⚙️ System Interface
- **Process Management** - Process creation and control
- **Pipe Operations** - Pipe creation and management
- **Dynamic Library Loading** - Runtime library loading
- **Console Operations** - Terminal interaction
- **Process Argument Parsing** - Analysis and manipulation of process arguments
- **Stack Trace** - Exception debugging
- **System Information** - Hardware and OS information
- **Environment Variables** - Environment variable manipulation
- **Signal Management** - Signal control

### ⏰ Time Utilities
- **High-Resolution Clock** - Multiple clock sources
- **Time Point / Duration** - Time calculations
- **Date Time** - Calendar operations
- **Scope Timer** - Execution time measurement for code blocks

### 🛠️ Utility Library
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

### 🔍 Reflection System
- **Reflection Registry** - Type reflection and metadata management
- **Type Information** - Runtime type querying

### 🧬 Type Traits & Concepts
- **Type Traits** - Compile-time type judgments
- **Concept Constraints** - C++20 concept support
- **Type Checking** - Runtime type information
- **CRTP Static Polymorphism** - Zero-overhead interface unification

### 💾 Memory Management
- **Smart Pointers** - shared_ptr, unique_ptr, weak_ptr
- **Atomic Smart Pointers** - Lock-free operations for `atomic<shared_ptr/weak_ptr>`
- **Memory View** - Safe memory access
- **Construction/Destruction Tools** - Object lifecycle management
- **Memory Tracing** - Memory monitoring for debugging
- **Standard Allocators** - Strategy-specialized allocators based on compiler features
- **Empty Base Class Optimization** - `compressed_pair` utility

### 📦 Compression
- **lz4 Compression** - High-speed data compression/decompression
- **zlib Compression** - General-purpose data compression/decompression

### 🔌 Plugin System
- **Dynamic Plugin Management** - Runtime plugin loading and unloading
- **Plugin Interface** - Standardized plugin development

### ❗ Exception Handling
- **Exception Pointer** - Cross-thread exception propagation
- **Termination Handling** - Program termination management
- **Exception System** - Standard exception hierarchy

### 📐 Algorithm Library
- **Standard Algorithms** - sort, find, transform, etc.
- **Parallel Algorithms** - Parallel execution policies
- **Numeric Algorithms** - Numerical computation and accumulation
- **Heap Algorithms** - Heap operations and priority queues
- **Range Operations** - Ranges library support
- **Hash Algorithms** - Multiple hash function implementations
- **Bit Operations** - Series of bit manipulation functions

### 📊 Math Library
- **Mathematical Constants** - Common mathematical constants
- **Mathematical Functions** - Transcendental functions and numerical computation
- **Random Number Generation** - LC, Mersenne Twister, hardware noise algorithms

---

## 🔧 Build Guide

### 📋 Prerequisites

| Type | Dependency | Version Requirement |
|------|------------|---------------------|
| 🔨 Build Tool | [CMake](https://cmake.org/) | 3.19+ |
| 📦 Package Manager | [vcpkg](https://github.com/microsoft/vcpkg) | Latest |
| 🎨 Code Formatter | [clang-format](https://clang.llvm.org/docs/ClangFormat.html) | 19+ |
| 🔍 Static Analyzer | [clang-tidy](https://clang.llvm.org/extra/clang-tidy/) | 19+ |
| ⚠️ Mandatory Dependencies | [GTest](https://google.github.io/googletest/) | 1.17.0#2+ |
| | [pcre2](https://www.pcre.org/) | 10.47+ |
| | [OpenSSL](https://www.openssl.org/) | 3.6.1#2+ |
| 📦 Optional Dependencies | [libpq](https://www.postgresql.org/) | 16.9#3+ |
| | [libmysql](https://www.mysql.com/) | 8.0.40#1+ |
| | [sqlite3](https://sqlite.org/index.html) | 3.51.2+ |
| | [hiredis](https://redis.ac.cn/docs/latest/develop/clients/hiredis/) | 1.3.0+ |
| | [lz4](https://lz4.org/) | 1.10.0+ |
| | [zlib](https://www.zlib.net/) | 1.3.1+ |

### 🏗️ Build Steps

> 💡 You can modify the configuration items in `config.json` in the project root directory for personalized builds.

#### 🪟 Windows

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

#### 🐧 Linux

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

---

## 📚 Documentation

For complete API documentation, please visit the [NexusForce Documentation Website](https://nexusforce.org.cn)

---

## ⚖️ License

This project is licensed under the [MIT License](LICENSE)

---

## 📝 Changelog

For detailed update records, please see [CHANGELOG](CHANGELOG.md)

---

## 👥 Contributors

Thanks to all the developers who have contributed to this project! See [CONTRIBUTORS](CONTRIBUTORS.md) for the full list.

---

## 📌 TODO

- [ ] 📊 Google Benchmark Performance Benchmarking
- [ ] ⚡ Hotspot Code Optimization
- [ ] 🍎 Support for macOS Platform
- [ ] 🖥️ Support for ARM / RISC-V / LOONGARCH Architectures
