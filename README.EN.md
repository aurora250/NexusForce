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
- [Quick Start](#-quick-start)
- [FAQ](#-faq)
- [Documentation](#-documentation)
- [License](#️-license)
- [Changelog](#-changelog)
- [Contributors](#-contributors)
- [TODO](#-todo)

---

## 📖 Project Overview

This project aims to establish a **feature-complete, stylistically unified, highly readable, community-driven, and cross-platform compatible** modern C++ development library. Through clear architectural design, standardized code implementation, and rich applications of design patterns, it provides a practical toolkit for project development while also serving as a practical learning resource for C++ learners to understand underlying principles, bridging the gap from learning to production.

💡 Please feel free to [submit Issues](https://github.com/aurora250/NexusForce/issues) to help improve this project. If there are any deficiencies, please don't hesitate to provide feedback.

---

## 🖥️ Supported Environments

| Item            | Details                                                                        |
|-----------------|--------------------------------------------------------------------------------|
| Platform        | 🪟 WINDOWS / 🐧 LINUX                                                          |
| Instruction Set | X86 / ARM / RISC-V (LINUX) / LOONGARCH (LINUX)                                 |
| Bit Width       | 64-bit                                                                         |
| Compiler        | MSVC (Windows) / LLVM-Clang (Windows, Linux) / ClangCL (Windows) / GCC (Linux) |
| C++ Standard    | 14 / 17 / 20                                                                   |

> ℹ️ **Compatibility Note**  
> This library welcomes developers to contribute compatibility with more compilers and operating systems. Your contributions are greatly appreciated.

---

## ✨ Engineering Quality

NexusForce strictly adheres to modern C++ engineering best practices, ensuring code robustness and readability through multi-layered automated checks.

| Metric                            | Status                | Description                                                                                                                 |
|-----------------------------------|-----------------------|-----------------------------------------------------------------------------------------------------------------------------|
| 📊 **Codebase Size**              | 160k+ Lines           | Core library source and headers 90k+ lines, test code 60k+ lines                                                            |
| 🔒 **CodeQL Security Analysis**   | **0 Vulnerabilities** | Full `security-and-quality` suite, zero security alerts                                                                     |
| 🔍 **Clang-Tidy Static Analysis** | **Zero Warnings**     | Full ruleset (`bugprone` / `cppcoreguidelines` / `hicpp` / `modernize` / `performance` / `readability`), warnings as errors |
| 🎨 **Clang-Format Code Style**    | **Strictly Enforced** | 120 columns, 4 spaces, K&R variant braces, mandatory brace insertion, etc.                                                  |
| 💧 **Dynamic Memory Check**       | **0 Leaks**           | Valgrind full test suite, zero memory leaks or out-of-bounds access                                                         |

> 📋 **Regarding Rule Exemptions**: [`.clang-tidy`](.clang-tidy) contains approximately 60 explicit exemptions, and [`.clang-format`](.clang-format) includes several style customizations. Each exemption addresses inherent requirements of low-level system programming, adhering to the principle of "strict by default, relaxed as needed."

---

## 📡 Standards Compliance

The core components of NexusForce strictly adhere to relevant international standards and industry specifications, ensuring predictable behavior, strong interoperability, and reliable security. The following table maps key components to their respective standards:

### 🌐 Network Protocols & Internet Standards

| Component                    | Standards Followed                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       | Description                                                                                                                                                                                                                                                                 |
|------------------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| **HTTP & WebSocket**         | [RFC 9110](https://www.rfc-editor.org/rfc/rfc9110.html) / [RFC 9112](https://www.rfc-editor.org/rfc/rfc9112.html) (HTTP/1.1), [RFC 7540](https://www.rfc-editor.org/rfc/rfc7540.html) / [RFC 7541](https://www.rfc-editor.org/rfc/rfc7541.html) (HTTP/2, HPACK), [RFC 6265](https://www.rfc-editor.org/rfc/rfc6265.html) (Cookie), [RFC 6455](https://www.rfc-editor.org/rfc/rfc6455.html) / [RFC 7692](https://www.rfc-editor.org/rfc/rfc7692.html) (WebSocket, permessage-deflate), [RFC 6066](https://www.rfc-editor.org/rfc/rfc6066.html) (SNI), [W3C Fetch CORS](https://fetch.spec.whatwg.org/#http-cors-protocol) | HTTP/1.1 semantics and routing, HTTP/2 frame layer and HPACK header compression, Range requests, response compression, CONNECT tunneling, Cookie and CSRF, CORS cross-origin policy, WebSocket upgrade/frame protocol/permessage-deflate compression, SNI multi-certificate |
| **DNS Client**               | [RFC 1034](https://www.rfc-editor.org/rfc/rfc1034.html), [RFC 1035](https://www.rfc-editor.org/rfc/rfc1035.html), [RFC 2181](https://www.rfc-editor.org/rfc/rfc2181.html), [RFC 6891](https://www.rfc-editor.org/rfc/rfc6891.html), [RFC 3596](https://www.rfc-editor.org/rfc/rfc3596.html), [RFC 2782](https://www.rfc-editor.org/rfc/rfc2782.html)                                                                                                                                                                                                                                                                     | DNS protocol client, A/AAAA/MX/SRV/PTR record queries, UDP/TCP transport auto-switching and TTL cache management                                                                                                                                                            |
| **ICMP Protocol**            | [RFC 792](https://www.rfc-editor.org/rfc/rfc792.html) (STD 5), [RFC 1122](https://www.rfc-editor.org/rfc/rfc1122.html), [RFC 4884](https://www.rfc-editor.org/rfc/rfc4884.html), [IANA ICMP Parameters Registry](https://www.iana.org/assignments/icmp-parameters/icmp-parameters.xhtml)                                                                                                                                                                                                                                                                                                                                 | Ping (Echo Request/Reply) and Traceroute (Time Exceeded) network diagnostics, including RFC 1071 checksum algorithm                                                                                                                                                         |
| **SMTP Protocol**            | [RFC 5321](https://www.rfc-editor.org/rfc/rfc5321.html) (STD 10), [RFC 5322](https://www.rfc-editor.org/rfc/rfc5322.html), [RFC 3207](https://www.rfc-editor.org/rfc/rfc3207.html) (STARTTLS), [RFC 8314](https://www.rfc-editor.org/rfc/rfc8314.html) (Implicit TLS), [RFC 4954](https://www.rfc-editor.org/rfc/rfc4954.html) (AUTH), [RFC 2045–2047](https://www.rfc-editor.org/rfc/rfc2045.html) (MIME)                                                                                                                                                                                                               | Email transport and message format, supporting PLAIN/LOGIN authentication, STARTTLS/Implicit TLS encryption, and MIME multipart messages                                                                                                                                    |
| **MAC Address**              | [IEEE 802-2014](https://standards.ieee.org/ieee/802/3714/), [IEEE 802.3-2022](https://standards.ieee.org/ieee/802.3/10422/), [RFC 7042](https://www.rfc-editor.org/rfc/rfc7042.html)                                                                                                                                                                                                                                                                                                                                                                                                                                     | 48-bit EUI-48 address parsing and formatting, supporting unicast/multicast/locally administered address identification and standard hexadecimal representation                                                                                                              |
| **URL Parsing & Encoding**   | [RFC 3986](https://www.rfc-editor.org/rfc/rfc3986), [RFC 3987](https://www.rfc-editor.org/rfc/rfc3987), [WHATWG URL](https://url.spec.whatwg.org/)                                                                                                                                                                                                                                                                                                                                                                                                                                                                       | Generic URI syntax, percent-encoding, and Internationalized Resource Identifiers                                                                                                                                                                                            |
| **Network Port Definitions** | [IANA Service Name and Transport Protocol Port Number Registry](https://www.iana.org/assignments/service-names-port-numbers/), [RFC 6335](https://www.rfc-editor.org/rfc/rfc6335)                                                                                                                                                                                                                                                                                                                                                                                                                                        | Well-known port assignments                                                                                                                                                                                                                                                 |
| **UUID Generation**          | [RFC 4122](https://www.rfc-editor.org/rfc/rfc4122), [RFC 9562](https://www.rfc-editor.org/rfc/rfc9562)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   | UUID v4 (random) and v7 (time-ordered) generation specifications                                                                                                                                                                                                            |
| **Byte Size Units**          | [IEC 80000-13:2008](https://www.iso.org/standard/31898.html), [IEEE 1541-2021](https://standards.ieee.org/ieee/1541/10790/), [BIPM SI Brochure (9th Ed.)](https://www.bipm.org/en/publications/si-brochure)                                                                                                                                                                                                                                                                                                                                                                                                              | Binary prefixes (KiB/MiB/GiB) and decimal prefixes (kB/MB/GB)                                                                                                                                                                                                               |

### 📁 Configuration File Formats

| Component         | Standards Followed                                                                                                                                                                                                       | Description                                                                                                                                |
|-------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|--------------------------------------------------------------------------------------------------------------------------------------------|
| **JSON RFC 8259** | [RFC 8259](https://www.rfc-editor.org/rfc/rfc8259), [ECMA-404:2017](https://ecma-international.org/publications-and-standards/standards/ecma-404/)                                                                       | JSON six value types, UTF-8 encoding, IEEE 754-2019 double-precision numbers, and string escape sequences                                  |
| **TOML 1.0.0**    | [TOML v1.0.0](https://toml.io/en/v1.0.0)                                                                                                                                                                                 | Includes date-time format following [RFC 3339](https://www.rfc-editor.org/rfc/rfc3339) / ISO 8601                                          |
| **YAML 1.2**      | [YAML 1.2.2](https://yaml.org/spec/1.2.2/), [RFC 8259](https://www.rfc-editor.org/rfc/rfc8259.html), [RFC 3339](https://www.rfc-editor.org/rfc/rfc3339.html), [IEEE 754-2019](https://standards.ieee.org/ieee/754/6210/) | YAML 1.2 is a strict superset of JSON, supporting eight core value types, five string scalar styles, anchors and aliases, and a tag system |

### 🔐 Cryptography & Security Algorithms

| Component                  | Standards Followed                                                                                                                           | Description                                                                                                                                                                             |
|----------------------------|----------------------------------------------------------------------------------------------------------------------------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| **AES-256 Encryption**     | [NIST FIPS 197](https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.197-upd1.pdf), [ISO/IEC 18033-3](https://www.iso.org/standard/54531.html)   | Advanced Encryption Standard, supports ECB/CBC/GCM modes ([NIST SP 800-38A](https://csrc.nist.gov/pubs/sp/800/38/a/final) / [SP 800-38D](https://csrc.nist.gov/pubs/sp/800/38/d/final)) |
| **ChaCha20-Poly1305 AEAD** | [IETF RFC 8439](https://www.rfc-editor.org/rfc/rfc8439.html)                                                                                 | ChaCha20 stream cipher + Poly1305 authenticator, AEAD with associated data (AAD) support                                                                                                |
| **SHA-256 Hashing**        | [NIST FIPS 180-4](https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.180-4.pdf), [RFC 6234](https://www.rfc-editor.org/rfc/rfc6234)            | Secure Hash Algorithm (SHA-2 family), 256-bit output                                                                                                                                    |
| **SHA-1 Hashing**          | [NIST FIPS 180-4](https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.180-4.pdf) (historical compatibility)                                     | ⚠️ Marked with security warning ([SHAttered](https://shattered.io/) collision attack)                                                                                                   |
| **MD5 Hashing**            | [RFC 1321](https://www.rfc-editor.org/rfc/rfc1321) (historical compatibility)                                                                | ⚠️ Marked with security warning, for non-cryptographic checksum scenarios only                                                                                                          |
| **Base64 Encoding**        | [RFC 4648 §4](https://www.rfc-editor.org/rfc/rfc4648.html#section-4), [RFC 4648 §5](https://www.rfc-editor.org/rfc/rfc4648.html#section-5)   | Standard and URL-safe Base64 encoding/decoding, strict padding rules and illegal character detection                                                                                    |

### 🔤 Character Encoding & Internationalization

| Component                        | Standards Followed                                                                                                                                                                                                               | Description                                                                                          |
|----------------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|------------------------------------------------------------------------------------------------------|
| **UTF-8 / UTF-16 / UTF-32**      | [Unicode 15.1.0](https://unicode.org/versions/Unicode15.1.0/), [ISO/IEC 10646](https://www.iso.org/standard/76835.html), [RFC 3629](https://www.rfc-editor.org/rfc/rfc3629) / [RFC 2781](https://www.rfc-editor.org/rfc/rfc2781) | Unicode codepoint operations, normalization, and encoding conversion with invalid sequence detection |
| **Unicode Codepoint Processing** | [Unicode 15.1.0](https://unicode.org/versions/Unicode15.1.0/) §2.4, §2.13                                                                                                                                                        | Surrogate pair handling, BOM detection, and replacement character (U+FFFD) rules                     |

### 📐 Data Structures & Algorithms

| Component                      | Standards Followed / Academic Literature                                                                                                                                                                 | Description                                                                                                                                                                            |
|--------------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| **Heap Algorithms**            | [ISO/IEC 14882:2020](https://www.iso.org/standard/79358.html) §25.8.6                                                                                                                                    | Complexity guarantees and Floyd's heap adjustment optimization ([Algorithm 245](https://dl.acm.org/doi/10.1145/512274.512284))                                                         |
| **Red-Black Tree**             | [Guibas & Sedgewick (1978)](https://doi.org/10.1109/SFCS.1978.3)                                                                                                                                         | Classic implementation of self-balancing binary search tree, O(log n) complexity guarantee                                                                                             |
| **Leonardo Heap / Smoothsort** | [Dijkstra (1981) EWD796a](https://www.cs.utexas.edu/~EWD/transcriptions/EWD07xx/EWD796a.html)                                                                                                            | Adaptive sorting algorithm, optimal time complexity O(n)                                                                                                                               |
| **Introsort**                  | [Musser (1997)](https://doi.org/10.1002/(SICI)1097-024X(199708)27:8<983::AID-SPE117>3.0.CO;2-%23)                                                                                                        | Hybrid quick/heap/insertion sort, default algorithm for C++ standard library `sort`                                                                                                    |
| **Non-cryptographic Hashing**  | [FNV-1a Draft](https://datatracker.ietf.org/doc/html/draft-eastlake-fnv-17), [MurmurHash3](https://github.com/aappleby/smhasher/wiki/MurmurHash3)                                                        | High-performance hash tables and Bloom filters                                                                                                                                         |
| **Bloom Filter**               | [Bloom (1970)](https://doi.org/10.1145/362686.362692), [Broder & Mitzenmacher (2004)](https://doi.org/10.1080/15427951.2004.10129096), [Kirsch & Mitzenmacher (2006)](https://doi.org/10.1002/rsa.20208) | Probabilistic set membership query structure with double hashing optimization, O(k) insertion/query, supporting optimal parameter (m, k) derivation and false positive rate estimation |

### ⚙️ System, Concurrency & Command-Line

| Component                 | Standards Followed                                                                                                                                                                   | Description                                                                       |
|---------------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|-----------------------------------------------------------------------------------|
| **Atomic Memory Order**   | [ISO/IEC 14882:2020](https://www.iso.org/standard/79358.html) §31.4                                                                                                                  | C++ memory model, includes hardware barrier equivalents and Intel TSX HLE support |
| **Command-Line Parsing**  | [POSIX.1-2017 (IEEE 1003.1)](https://pubs.opengroup.org/onlinepubs/9699919799/) Chapter 12, [GNU getopt_long](https://man7.org/linux/man-pages/man3/getopt.3.html)                   | Supports short option grouping, long options, `--` delimiter, and optional values |
| **Date & Time**           | [ISO 8601-1:2019](https://www.iso.org/standard/70907.html), [RFC 3339](https://www.rfc-editor.org/rfc/rfc3339), [POSIX Timestamp](https://pubs.opengroup.org/onlinepubs/9699919799/) | Gregorian calendar calculations, Julian day conversion, and Unix epoch handling   |
| **SQL Statement Builder** | [ISO/IEC 9075](https://www.iso.org/standard/16663.html) (SQL-92 and later)                                                                                                           | Generates ANSI SQL compliant SELECT/INSERT/UPDATE/DELETE statements               |

### 🎨 Graphics, Color & Mathematics

| Component                              | Standards Followed                                                                                                                | Description                                                                         |
|----------------------------------------|-----------------------------------------------------------------------------------------------------------------------------------|-------------------------------------------------------------------------------------|
| **RGB Color Model**                    | [W3C CSS Color Level 4](https://www.w3.org/TR/css-color-4/), [Compositing Level 1](https://www.w3.org/TR/compositing-1/)          | Straight alpha compositing, hexadecimal formats, and ANSI 256-color palette         |
| **Grayscale Conversion**               | [ITU-R BT.709](https://www.itu.int/rec/R-REC-BT.709/) / [IEC 61966-2-1 (sRGB)](https://webstore.iec.ch/publication/6169)          | Human perception weighting (0.299R + 0.587G + 0.114B)                               |
| **Mathematical Functions & Constants** | [IEEE 754-2019](https://standards.ieee.org/ieee/754/6210/), [ISO/IEC 10967 (LIA)](https://www.iso.org/standard/24417.html)        | Trigonometric reduction, Newton's method iterations, and machine epsilon tolerances |
| **Random Number Generation**           | [ISO/IEC 18031:2011](https://www.iso.org/standard/54945.html), [NIST SP 800-90A](https://csrc.nist.gov/pubs/sp/800/90/a/r1/final) | Mersenne Twister (MT19937) and OS entropy source true random numbers                |

> 📖 **Documentation Completeness**: All classes and functions referencing the above standards include specific standard section numbers and official links in their API comments (Doxygen format), enabling developers to trace and verify at any time.

---

## 🚀 Features

### 🔄 Concurrency & Async
- **`thread_pool`** - Multi-strategy thread pool based on work stealing, For detailed performance testing, see for [thread_pool performance test records](benchmark/async/PERFORMANCE.md)
- **`io_context`** - Async I/O execution context, unified event loop, timer and cancellation model
- **`cancellation_slot`** - Supports `async_connect` / `async_read` / `async_write` interruptible async I/O
- **`timer_scheduler` / `basic_timer`** - Timer task scheduling based on red-black tree
- **`async_stream`** - Async stream abstraction interface, unified read/write protocol
- **`async_read()` / `async_write()`** - Free function combinators, automatic partial read/write retry via `shared_from_this`
- **`thread_pool_executor`** - Adapts `thread_pool::submit_task()` to standard executor interface
- **`generator` / `task`** - Coroutine primitives and task generator
- **`virtual_thread`** - C#-style lightweight coroutines
- **`connection` / `signal` / `signal_base`** - Observer pattern signal-slot, `signal_base` provides type-erased base for reflection-driven dynamic connections
- **`call_once`** - Multi-threaded single-call implementation based on FUTEX
- **Stop Token** - Cancellable asynchronous operations `stop_token` / `stop_source` / `stop_callback`
- **Synchronization Primitives** - Mutex `mutex`, shared mutex `shared_mutex`, semaphore `semaphore` / `atomic_semaphore`, thread barrier `barrier` and latch `latch`
- **Atomic Operations** - Atomic types `atomic`, atomic FUTEX `atomic_futex`, global atomic operation function system
- **Multi-strategy Threads** - General thread `thread`, scoped thread with stop token `scope_thread`, manually started lazy thread `lazy_thread`
- **Basic Async Model** - `async` and its supporting `future` / `promise` / `packaged_task` structures
- **Hazard Pointer** - Memory management for lock-free data structures `hazard_ptr` / `hazard_pointer_domain`

### 📦 Containers
- **Standard Containers** - `array` / `vector` / `list` / `deque` / `map` / `set` / `unordered_map` / `unordered_set` / `flat_unordered_map` / `flat_unordered_set`, etc.
- **`rb_tree`** - Self-balancing binary search tree implementation
- **`hashtable`** - Separate chaining hash table
- **`flat_hashtable`** - SwissTable-style Open addressing flat hash table, H2 pre-filtering + SIMD group probing
- **`bloom_filter`** - Probabilistic data structure
- **`lru_cache` / `ttl_cache`** - Cache policies based on Least Recently Used / Time-To-Live
- **`buffer_chain`** - Zero-copy chained buffer, supports writev aggregated output
- **`sparse_vector`** - Sorted flat-array based associative containers, O(log n) binary search, O(1) cache-friendly iteration
- **`bitmap` / `bitset`** - Efficient bit manipulation containers

### 🔐 Encryption & Security
- **`AES256`** - Advanced Encryption Standard implementation
- **`ChaCha20Poly1305`** - ChaCha20-Poly1305 AEAD authenticated encryption with AAD support
- **`SHA1` / `SHA256`** - Secure Hash Algorithm
- **`MD5`** - Message Digest Algorithm
- **`base64`** - Binary data encoding

### 📁 File System
- **Path/File Operations** - Path and file system operations `path` / `path_tree` / `file` / `file_async` / `file_diff` / `file_locker` / `file_mapper`
- **`file_async`** - `io_context`-driven async file I/O, with offset specification and cancellation slot support
- **`file_watcher`** - Real-time file system change monitoring
- **Config File Parsing** - JSON/TOML/YAML/INI/ENV value system, format parsing and streaming builder
- **`temp_file`** - Secure temporary file management

### 🌐 Networking
- **HTTP/1.1 Server** - Complete protocol: `http_server` / `http_router` / `http_filter` middleware chain
- **HTTP/2 Support** - Frame layer & HPACK header compression (RFC 7540/7541), 9 frame types, stream state machine, flow control, TLS+ALPN negotiation
- **Radix Tree Routing** - O(k) route matching via compressed prefix tree, static paths, :param params, * wildcards, regex fallback
- **HTTP Advanced Features** - Range requests (206 single/multi-range), gzip/deflate response compression, Chunked transfer, CONNECT tunneling
- **`multipart_parser` / `chunked_reader`** - Multipart form parsing and chunked transfer reading
- **Session Management** - Pluggable session_store (memory/Redis), CSRF Double-Submit Cookie protection, Session Fixation protection
- **Security Middleware** - `csrf_filter` CSRF protection, `http_security` security headers, `http_cache` HTTP caching policies
- **WebSocket** - RFC 6455/7692 full-duplex `websocket_session` / `websocket_server`, event-driven zero-thread mode
- **WebSocket Compression** - permessage-deflate (RFC 7692), window bit negotiation and context takeover control
- **HTTP Client** - `http_client` scheme-aware mode (auto SSL for https), request/response handling
- **TCP/UDP Socket** - High-performance `tcp_socket` / `udp_socket`, with `async_connect()` / `async_read()` / `async_write()` async operations
- **SSL/TLS** - Encrypted transport `ssl_context` / `ssl_stream`, SNI multi-certificate management `sni_manager`, ALPN protocol negotiation
- **`dns_client`** - Domain name resolution, per-operation async state object architecture, completion-token async API (callback/cancellation slot/future/awaitable)
- **`io_context`** - Unified event loop (Linux epoll edge-triggered + min-heap timer), async I/O callback-driven, cancellation model
- **`async_filter`** - Async pre/post filter chain framework
- **`byte_cursor`** - Zero-copy byte-level protocol parsing cursor, with bounds checking and bit-level buffering
- **`load_balancer`** - Connection pool + round-robin load balancing
- **FTP** - FTP server and client
- **ICMP/SMTP** - ICMP and SMTP protocol operations
- **`arp` / `mac_address` / `ip_address` / `ports` / `url`** - Network programming utilities

### 🗄️ Database
- **`database_pool`** - Database connection reuse and management, with `warm_up()` / `active_count()` / `get_tb_connect_for()` timeout-aware acquisition
- **`sql_builder`** - Fluent builder for standard SQL statements, with **dialect-aware placeholders** (Generic `?` / PgSQL `$N` / Oracle `:N`) and **dialect-aware DDL** (AUTO_INCREMENT / SERIAL / IDENTITY auto-adaptation)
- **`sql_mapper<T>`** - Reflection-driven ORM SQL generator, auto-generates DDL/DML statements with dialect awareness
- **`repository<T, Connect>`** - Generic CRUD repository template (find_all / find_by_pk / find_where / find_page / insert / update / remove / count)
- **Multi-Database Support**:
  - MySQL Client
  - PostgreSQL Client
  - SQLite Client
  - Redis Client
- **Prepared Statements** - Prevention of SQL injection
- **Result Set Wrapper** - Unified result access interface
- **DB Property Annotations** - `PROP_PRIMARY_KEY` / `PROP_AUTO_INC` / `PROP_UNIQUE` / `PROP_INDEX` / `PROP_FOREIGN_KEY`, shared metadata between reflection system and ORM

### 📝 Logging
- **`log_sink`** - Extensible log output targets
- **`file_sink`** - Log file management and rotation
- **`log_formatter`** - Customizable log format
- **`logger`** - Thread-pool-driven async logger, supports `block`/`discard`/`overrun_oldest` overflow strategies, bounded ring buffer, auto drain scheduling

### 🔤 String Processing
- **PCRE2 Regular Expressions** - Efficient regex matching with JIT support `regex`/`match_result`/`regex_iterator`/`regex_token_iterator`, supports copy
- **Unicode Support** - UTF conversion system, codepoint operation class `codepoint`, UTF-8 codepoint iterator `utf8_iterator` / `utf8_view`
- **`formatter`/`format`** - Type-safe formatted output, supports positional params `{0}` `{1}` and named params `format_named()`
- **`string_view`** - Extensive use of string views for optimized operations
- **Numeric Conversion** - Extensible string-to-number and number-to-string conversion system
- **Boyer-Moore-Horspool** - Narrow character substring search acceleration

### ⚙️ System Interface
- **`process`** - Process creation and control
- **`pipe`** - Pipe creation and management
- **`dynamic_library`** - Runtime library loading
- **`console`** - Highly integrated terminal interaction
- **`cmdline`** - Process argument analysis and manipulation
- **`stacktrace`** - Exception debugging
- **`share_memory`** - Cross-process shared memory
- **`locale`** - Locale configuration and parsing
- **`sysinfo`** - Hardware and OS information
- **`environment`** - Environment variable manipulation
- **`system_signal_manager`** - System signal control
- **`system_event`** - System event management

### ⏰ Time Utilities
- **`steady_clock` / `system_clock`** - High-resolution clocks with multiple clock sources
- **`duration`** - Time span calculation
- **`date` / `time` / `datetime` / `timestamp`** - Calendar operations
- **`scoped_click`** - Execution time measurement for code blocks

### 🛠️ Utility Library
- **Literal Type Wrapper Classes** - Advanced encapsulation of literal types
- **`optional`** - Optional value handling
- **`variant`** - Type-safe union
- **`expected`** - Error state handling
- **`any`** - Type-erased container
- **`pair` / `tuple`** - Compile-time key-value pair / tuple
- **`color`** - RGB color operations
- **`scope_exit` / `scope_fail` / `scope_success`** - Scope guards
- **Numeric/Character Info** - Numeric and character type information retrieval
- **`ratio`** - Compile-time ratio calculation
- **`uuid`** - UUID v4/v7 generator
- **`compressed_pair`** - EBCO memory optimization

### 🔍 Reflection System
- **`NFRS`** - MOC-like pre-compile code generator, scans `NEFORCE_REFLECT_*` markers to auto-generate type registration code, with incremental scanning (file modification time cache)
- **`meta_any`** - SBO-optimized + function-pointer dispatch type-erased container, supports `emplace<T>()` in-place construction (compatible with non-copyable/non-movable types)
- **`registry`** - Global type reflection registry with name/type_id dual lookup, thread-safe registration, and dynamic signal-slot connection `connect_signal_to_slot()`
- **`meta_type`** - Runtime type metadata: base class list, property/function maps, constructor/clone factory, enum/container info, signal name list, dynamic property registration `add_property()`
- **`meta_property`** - Property reflection descriptor with getter/setter, annotation flags (transient/required/readonly/optional/versioned/primary_key/auto_inc/unique/index/foreign_key), and change notification signal `notify_signal()`
- **`meta_function`** - Member/static function reflection descriptor with overload support, parameter hints, and runtime `invoke()` call
- **`meta_enum`** - Enum reflection: name↔value bidirectional lookup, entry iteration
- **`type_builder`** - Fluent API type builder: base class registration, property/function/signal registration, constructor/clone/container configuration
- **`signal_base`** - Type-erased base class for `signal<T...>`, providing `connect_dynamic()` / `emit_dynamic()` for runtime reflection-driven signal-slot connections
- **JSON Serializer** - Reflection-driven object↔JSON serialization/deserialization, recursive nested types and containers
- **Binary Serializer** - Big-endian format (Magic "NEBF" + type table + data segment), attribute-controlled serialization behavior
- **Type Identification** - Hash-based type_id via type_name specialization + compiler function signature, non-intrusive type registration
- **Reflection Macros** - `NEFORCE_REFLECT_OBJ` / `PROP` / `FUNC` / `SIGNAL` / `ENUM` / `ENUM_VAL` in-class markers for NFRS scanner recognition

### 🖥️ Terminal UI Framework (TUI)
- **`application`** - Application entry point, Builder pattern config (theme/FPS/title), drives event loop and animation timer
- **`reconciler`** - Declarative rendering engine, cell-level terminal frame diffing with incremental ANSI output, focus chain traversal (Tab/Shift+Tab), mouse hit-testing and scrollbar interaction
- **`screen`** - Terminal frame buffer, per-cell diffing to produce minimal ANSI escape sequences
- **`input_driver`** - Cross-platform terminal input driver (Linux epoll / Windows background thread), parses ANSI/mouse/UTF-8 sequences
- **`element`** - Immutable virtual element tree, 14+ node kinds (vbox/hbox/zstack/flexbox/gridbox/text/button/text_input/checkbox/scroll_view/canvas etc.), decorator chaining composition
- **Flexbox / Gridbox Layout** - Full Flexbox (direction/wrap/justify/align/gap/flex_grow/flex_shrink) + Gridbox layout engine
- **`style` / `theme`** - Style system (fg/bg/bold/italic/underline/border/padding/margin/align) + semantic theming (primary/secondary/danger etc.), dark theme preset
- **`state<T>`** - Reactive state management, auto dirty-marking on write + strand-coalesced re-render scheduling
- **DOM Helpers** - `gauge` progress bar / `graph` chart / `paragraph` word-wrapped text / `spinner` loading animation / `scroll_indicator` scrollbar / `linear_gradient` multi-stop gradient / `table` declarative table builder
- **`component_base` / `component<P>`** - Component base class, focus management, context injection (`provide_context`/`context`, walks up parent chain), reactive state factory
- **Interactive Components** - `container` directional containers / `menu` menu list / `dropdown` dropdown select / `radiobox` radio group / `toggle` toggle switch / `slider` value slider / `text_input` text input (UTF-8 aware cursor) / `scroll_view` scroll view / `window` floating window / `modal` modal overlay / `collapsible` collapsible panel / `hoverable` hover detection / `resizable_split` draggable split pane / `renderer` render helpers (`catch_event`/`maybe` conditional)
- **`animator` / `easing`** - Property animator + easing functions (linear/quadratic/cubic/sine/elastic/bounce)

### 🧬 Type Traits & Concepts
- **Type Traits** - Comprehensive compile-time type judgments
- **Concept Constraints** - Concept set support
- **`check_type`** - Human-readable type names at runtime
- **CRTP Static Polymorphism** - Globally unified zero-overhead interface

### 💾 Memory Management
- **`shared_ptr`, `unique_ptr`, `weak_ptr`** - Smart pointer structures
- **`atomic<shared_ptr/weak_ptr>`** - Lock-free operations for atomic smart pointers
- **`memory_view`** - Safe memory access
- **Bit/Endian/Byte Stream Operations** - Memory state modification
- **Construction/Destruction Tools** - Object lifecycle management
- **`trace_allocator`** - Memory monitoring for debugging
- **`standard_allocator`** - Strategy-specialized allocators based on compiler features

### 📦 Compression
- **lz4 Compression** - `lz4_compressor` high-speed data compression/decompression
- **zlib Compression** - `zlib_compressor` multi-strategy general-purpose data compression/decompression

### 🔌 Plugin System
- **Dynamic Plugin Management** - `plugin_manager` runtime plugin loading and unloading
- **Plugin Interface** - Standardized plugin development

### ❗ Exception Handling
- **Exception Pointer** - `exception_ptr` cross-thread exception propagation
- **Termination Handling** - Multi-state program termination methods and corresponding callbacks
- **Exception/Error Code System** - Custom exception and error code system
- **Breakpoint Handling** - Debug breakpoint triggering and handling

### 📐 Algorithm Library
- **Standard Algorithms** - Range iteration algorithms based on iterator system
- **Parallel Algorithms** - Parallel execution policies
- **Numeric Algorithms** - Numerical computation and accumulation
- **Heap Algorithms** - Heap operations and priority queues
- **Range Operations** - Ranges library support
- **Hash Algorithms** - Multiple hash function implementations

### 📊 Math Library
- **Mathematical Constants** - Common mathematical constants
- **Mathematical Functions** - Transcendental functions and numerical computation
- **Random Number Generation** - LC, Mersenne Twister, hardware noise algorithms `random_lcd` / `random_mt` / `secret`
- **128-bit Math** - 128-bit signed/unsigned numeric operations `int128_t` / `uint128_t`

---

## 🔧 Build Guide

### 📋 Prerequisites

| Type                              | Dependency                                                          | Version Requirement |
|-----------------------------------|---------------------------------------------------------------------|---------------------|
| 🔨 Build Tool                     | [CMake](https://cmake.org/)                                         | 3.19+               |
| 📦 Package Manager                | [vcpkg](https://github.com/microsoft/vcpkg)                         | Latest              |
| 🎨 Code Formatter                 | [clang-format](https://clang.llvm.org/docs/ClangFormat.html)        | 19+                 |
| 🔍 Static Analyzer                | [clang-tidy](https://clang.llvm.org/extra/clang-tidy/)              | 19+                 |
| ⚠️ Mandatory Dependencies (vcpkg) | [pcre2](https://www.pcre.org/)                                      | 10.47+              |
|                                   | [icu](https://icu.unicode.org/)                                     | 78.2+               |
|                                   | [OpenSSL](https://www.openssl.org/)                                 | 3.6.1+              |
| 📦 Optional Dependencies (vcpkg)  | [libpq](https://www.postgresql.org/)                                | 16.9+               |
|                                   | [libmysql](https://www.mysql.com/)                                  | 8.0.40+             |
|                                   | [sqlite3](https://sqlite.org/index.html)                            | 3.51.2+             |
|                                   | [hiredis](https://redis.ac.cn/docs/latest/develop/clients/hiredis/) | 1.3.0+              |
|                                   | [lz4](https://lz4.org/)                                             | 1.10.0+             |
|                                   | [zlib](https://www.zlib.net/)                                       | 1.3.1+              |
|                                   | [GTest](https://google.github.io/googletest/)                       | 1.17.0+             |
|                                   | [benchmark](https://github.com/google/benchmark/)                   | 1.9.5+              |

### 🏗️ Build Steps

Ensure that CMake, vcpkg, clang-format, and clang-tidy are correctly installed and configured before building.

> 💡 You can modify the configuration items in `config.json` (build options) and `vcpkg.json` (package management configuration) in the project root directory for personalized builds.
>
> **This project additionally relies on liburing-dev on Linux systems; please ensure you have installed it before building**
>
> In actual testing, on Linux, VCPKG requires libtirpc-dev libraries to build libmysql, libpq requires bison, flex, and autoconf libraries, and these libraries are not installed by default by the package manager.
> If you need corresponding dependencies, you can install them in advance using the package manager to prevent cmake build failures

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

## 🚀 Quick Start

Before you begin, ensure you have completed the installation steps in the Build Guide.

Quick start example overview:

| Example             | Recommended Scenario                    | Key Features                      |
|---------------------|-----------------------------------------|-----------------------------------|
| HTTP Server         | REST APIs, Microservices                | Routing, Middleware, SSL          |
| Thread Pool         | CPU-intensive tasks, Batch processing   | Work Stealing, Load Balancing     |
| Config Parsing      | Application configuration management    | Multi-format support, Type Safety |
| Encryption          | Data encryption, Integrity verification | AES-256, SHA-256                  |
| Database Operations | Data persistence                        | Connection Pool, SQL Builder      |
| File Watcher        | Hot reload, Real-time sync              | Cross-platform, Event-driven      |

For details, see [Quickly Build Common Features with NexusForce!](QUICK_START.md)

---

## ❓ FAQ

For project positioning, comparisons with other frameworks (Boost / Qt / POCO / folly), design philosophy, feature selection guidance, etc.,
please see [Q&A](Q&A.md).

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

The core ABI has been stabilized.

For TODO items, see TODO comments within the code.
