# MSTL V1.4.0

[![Build Status](https://travis-ci.org/aurora250/MSTL.svg?branch=master)](https://travis-ci.org/aurora250/MSTL)
[![License](https://img.shields.io/badge/License-MIT%20License-blue.svg)](https://opensource.org/licenses/MIT)

> Read this in other languages: [中文 (Chinese)](README.md)

This project aims to establish a feature-complete, stylistically unified, highly readable, community-driven, 
and cross-platform compatible Modern C++ development library - MSTL (Modern Standard Template Library). 
Through clear architectural design, standardized code implementation, and rich applications of design patterns,
it provides a practical toolkit for project development while also serving as a practical learning resource for
C++ beginners to understand underlying principles. We welcome issues to help improve this project. 
If there are any deficiencies, please feel free to provide feedback.

This library assumes your operating system uses UTF-8 code page when working with IO devices. 
If not, please try to configure it; otherwise, garbled characters may occur during IO operations.

## What can you learn by reading and using MSTL?

- Functional programming design and type erasure design;
- Type traits implementation using template metaprogramming;
- Using `concept` and `requires` to constrain template parameter behavior;
- Strengthening `noexcept` guarantees;
- Implementing SFINAE (Substitution Failure Is Not An Error) via `enable_if`;
- Implementing EBCO (Empty Base Class Optimization) via `compressed_pair`;
- Coordinating memory allocation with in-place construction;
- Conversion rules between UTF-8, UTF-16, and UTF-32;
- Implementing static polymorphism using CRTP (Curiously Recurring Template Pattern);
- Implementing operating system operation classes using Windows/Linux APIs, analyzing and understanding the similar yet distinct data processing methods across different operating systems;
- Implementation of complex containers like deques, red-black trees, and hash tables;
- Modern wrappers and usage for PostgreSQL, MySQL, and Redis interfaces;
- Implementation of scheduling tools like timers, thread pools, and database connection pools;
- Implementation of modern-style network development tools for TCP/UDP/DNS/HTTP/HTTPS;
  ......

## Supporting Environments

WINDOWS LINUX

X64 X86

MSVC GCC CLANG

C++ 14 17 20

## Build Guide

### Prerequisites

- CMake 3.17+
- Compiler supporting C++14 or higher
- Optional dependencies:
  - PostgreSQL
  - MySQL
  - SQLite3
  - hiredis
  - zlib
  - OpenSSL
  - CUDA Toolkit

Note: MSTL has discontinued CUDA support, which is disabled by default.

### Build Steps

You can toggle dependencies in the project root's `CMakeLists.txt` and directly modify your local dependency paths
in `src/CMakeLists.txt` for customized builds.

- Windows

```bash
# Clone the latest release
git clone --depth 1 https://github.com/aurora250/MSTL.git
cd MSTL

# Create build directory
mkdir build && cd build

# Configure build options (can also modify directly in CMakeLists.txt)
cmake .. -G "Visual Studio 17 2022" -A x64 \
  -DMSTL_ENABLE_MYSQL=OFF \
  -DMSTL_BUILD_TESTS=ON \
  -DMYSQL_ROOT_DIR="C:/Program Files/MySQL/MySQL Server 8.0"

# Build
cmake --build . --config Release

# Install to system directory
cmake --install . --config Release
```

- Linux

```bash
# Clone the latest release
git clone --depth 1 https://github.com/aurora250/MSTL.git
cd MSTL

# Create build directory
mkdir build && cd build

# Configure build options (can also modify directly in CMakeLists.txt)
cmake .. -DCMAKE_BUILD_TYPE=Release \
  -DMSTL_ENABLE_MYSQL=OFF \
  -DMSTL_BUILD_TESTS=ON

# Build
make -j$(nproc)

# Install to system directory
sudo make install
```

## Include Structure

```bash
├───include
│   └───MSTL
│       │   MSTL.hpp
│       │
│       ├───compress
│       │       zlib_compress.hpp
│       │
│       ├───core
│       │   ├───algorithm
│       │   │       algorithm.hpp
│       │   │       bound.hpp
│       │   │       compare.hpp
│       │   │       erase.hpp
│       │   │       ext_sort.hpp
│       │   │       heap.hpp
│       │   │       iterator.hpp
│       │   │       leonardo_heap.hpp
│       │   │       merge.hpp
│       │   │       numeric.hpp
│       │   │       parallel.hpp
│       │   │       partition.hpp
│       │   │       permutation.hpp
│       │   │       search.hpp
│       │   │       set.hpp
│       │   │       shift.hpp
│       │   │       shuffle.hpp
│       │   │       sort.hpp
│       │   │       type_erase.hpp
│       │   │
│       │   ├───async
│       │   │       async.hpp
│       │   │       atomic.hpp
│       │   │       atomic_base.hpp
│       │   │       atomic_futex.hpp
│       │   │       atomic_futex_base.hpp
│       │   │       atomic_timed_wait.hpp
│       │   │       atomic_wait.hpp
│       │   │       at_thread_exit.hpp
│       │   │       call_once.hpp
│       │   │       condition_variable.hpp
│       │   │       future.hpp
│       │   │       future_base.hpp
│       │   │       jthread.hpp
│       │   │       lock_free_queue.hpp
│       │   │       mutex.hpp
│       │   │       packaged_task.hpp
│       │   │       promise.hpp
│       │   │       semaphore.hpp
│       │   │       shared_mutex.hpp
│       │   │       stop_token.hpp
│       │   │       thread.hpp
│       │   │       thread_pool.hpp
│       │   │       timer.hpp
│       │   │
│       │   ├───config
│       │   │       c++config.hpp
│       │   │       undef_cmacro.hpp
│       │   │
│       │   ├───container
│       │   │       array.hpp
│       │   │       bitmap.hpp
│       │   │       bitset.hpp
│       │   │       deque.hpp
│       │   │       hashtable.hpp
│       │   │       list.hpp
│       │   │       map.hpp
│       │   │       multimap.hpp
│       │   │       multiset.hpp
│       │   │       priority_queue.hpp
│       │   │       queue.hpp
│       │   │       rb_tree.hpp
│       │   │       set.hpp
│       │   │       stack.hpp
│       │   │       unordered_map.hpp
│       │   │       unordered_multimap.hpp
│       │   │       unordered_multiset.hpp
│       │   │       unordered_set.hpp
│       │   │       vector.hpp
│       │   │
│       │   ├───encrypt
│       │   │       aes256.hpp
│       │   │       base64.hpp
│       │   │       encrypt.hpp
│       │   │       md5.hpp
│       │   │       sha1.hpp
│       │   │       sha256.hpp
│       │   │       xor.hpp
│       │   │
│       │   ├───exception
│       │   │       assertion.hpp
│       │   │       exception.hpp
│       │   │       exception_ptr.hpp
│       │   │       scope_guard.hpp
│       │   │       terminate.hpp
│       │   │
│       │   ├───file
│       │   │   │   file.hpp
│       │   │   │   file_constants.hpp
│       │   │   │   file_watcher.hpp
│       │   │   │   path.hpp
│       │   │   │   temp_file.hpp
│       │   │   │
│       │   │   ├───env
│       │   │   │       env_builder.hpp
│       │   │   │       env_parser.hpp
│       │   │   │       env_value.hpp
│       │   │   │
│       │   │   ├───ini
│       │   │   │       ini_builder.hpp
│       │   │   │       ini_parser.hpp
│       │   │   │       ini_value.hpp
│       │   │   │
│       │   │   ├───json
│       │   │   │       json_builder.hpp
│       │   │   │       json_parser.hpp
│       │   │   │       json_value.hpp
│       │   │   │
│       │   │   ├───toml
│       │   │   │       toml_builder.hpp
│       │   │   │       toml_parser.hpp
│       │   │   │       toml_value.hpp
│       │   │   │
│       │   │   └───yaml
│       │   │           yaml_builder.hpp
│       │   │           yaml_parser.hpp
│       │   │           yaml_value.hpp
│       │   │
│       │   ├───functional
│       │   │       apply.hpp
│       │   │       call_wrapper.hpp
│       │   │       function.hpp
│       │   │       functor.hpp
│       │   │       functor_adapter.hpp
│       │   │       hash.hpp
│       │   │       invoke.hpp
│       │   │
│       │   ├───interface
│       │   │       icharacter.hpp
│       │   │       icollector.hpp
│       │   │       icommon.hpp
│       │   │       inumeric.hpp
│       │   │       iobject.hpp
│       │   │       ipackage.hpp
│       │   │       istringify.hpp
│       │   │
│       │   ├───iterator
│       │   │       file_line_iterator.hpp
│       │   │       insert_iterator.hpp
│       │   │       iterator_traits.hpp
│       │   │       normal_iterator.hpp
│       │   │       path_iterator.hpp
│       │   │       ranges.hpp
│       │   │       reverse_iterator.hpp
│       │   │
│       │   ├───memory
│       │   │       aligned_buffer.hpp
│       │   │       allocated_ptr.hpp
│       │   │       allocator_traits.hpp
│       │   │       bit.hpp
│       │   │       builtin_allocator.hpp
│       │   │       construct.hpp
│       │   │       memory.hpp
│       │   │       memory_view.hpp
│       │   │       shared_ptr.hpp
│       │   │       standard_allocator.hpp
│       │   │       temporary_buffer.hpp
│       │   │       trace_memory.hpp
│       │   │       uninitialized.hpp
│       │   │       unique_ptr.hpp
│       │   │       weak_ptr.hpp
│       │   │
│       │   ├───numeric
│       │   │       math.hpp
│       │   │       numeric_limits.hpp
│       │   │       numeric_types.hpp
│       │   │       random.hpp
│       │   │       ratio.hpp
│       │   │       static_numeric.hpp
│       │   │
│       │   ├───serialize
│       │   │       concepts.hpp
│       │   │       serialize.hpp
│       │   │       serialize_traits.hpp
│       │   │
│       │   ├───string
│       │   │       basic_string.hpp
│       │   │       basic_string_view.hpp
│       │   │       character.hpp
│       │   │       char_traits.hpp
│       │   │       char_types.hpp
│       │   │       cstring.hpp
│       │   │       format.hpp
│       │   │       string.hpp
│       │   │       string_util.hpp
│       │   │       string_view.hpp
│       │   │       to_numerics.hpp
│       │   │       to_string.hpp
│       │   │       vsprintf.hpp
│       │   │
│       │   ├───system
│       │   │   │   cmdline.hpp
│       │   │   │   console.hpp
│       │   │   │   environment.hpp
│       │   │   │   process.hpp
│       │   │   │   signal.hpp
│       │   │   │   stacktrace.hpp
│       │   │   │
│       │   │   └───device
│       │   │           device.hpp
│       │   │           device_constants.hpp
│       │   │           serial_port.hpp
│       │   │           storage_device.hpp
│       │   │
│       │   ├───time
│       │   │       clocks.hpp
│       │   │       datetime.hpp
│       │   │       duration.hpp
│       │   │       time_point.hpp
│       │   │
│       │   ├───typeinfo
│       │   │       check_type.hpp
│       │   │       concepts.hpp
│       │   │       pointer_traits.hpp
│       │   │       tags.hpp
│       │   │       types.hpp
│       │   │       type_traits.hpp
│       │   │
│       │   └───utility
│       │           any.hpp
│       │           color.hpp
│       │           compressed_pair.hpp
│       │           expected.hpp
│       │           hexadecimal.hpp
│       │           integer_sequence.hpp
│       │           optional.hpp
│       │           packages.hpp
│       │           pair.hpp
│       │           tuple.hpp
│       │           variant.hpp
│       │
│       ├───database
│       │   │   database_pool.hpp
│       │   │   db_config.hpp
│       │   │   db_interface.hpp
│       │   │   sql_builder.hpp
│       │   │
│       │   ├───mysql
│       │   │       mysql_config.hpp
│       │   │       mysql_connect.hpp
│       │   │       mysql_prepared_result.hpp
│       │   │       mysql_prepared_statement.hpp
│       │   │       mysql_result.hpp
│       │   │
│       │   ├───postgresql
│       │   │       postgresql_config.hpp
│       │   │       postgresql_connect.hpp
│       │   │       postgresql_prepared_result.hpp
│       │   │       postgresql_prepared_statement.hpp
│       │   │       postgresql_result.hpp
│       │   │
│       │   ├───redis
│       │   │       redis_config.hpp
│       │   │       redis_connect.hpp
│       │   │       redis_result.hpp
│       │   │
│       │   └───sqlite
│       │           sqlite_config.hpp
│       │           sqlite_connect.hpp
│       │           sqlite_prepared_result.hpp
│       │           sqlite_prepared_statement.hpp
│       │           sqlite_result.hpp
│       │
│       ├───logging
│       │       file_sink.hpp
│       │       logger.hpp
│       │       log_event.hpp
│       │       log_formatter.hpp
│       │       log_sink.hpp
│       │
│       ├───network
│       │   │   ssl_context.hpp
│       │   │   ssl_socket.hpp
│       │   │   tcp_client.hpp
│       │   │   tcp_server.hpp
│       │   │   tcp_socket.hpp
│       │   │   url.hpp
│       │   │
│       │   ├───dns
│       │   │       dns_client.hpp
│       │   │       dns_constants.hpp
│       │   │       dns_message.hpp
│       │   │
│       │   └───http
│       │           http_client.hpp
│       │           http_client_message.hpp
│       │           http_constants.hpp
│       │           http_filter.hpp
│       │           http_router.hpp
│       │           http_server.hpp
│       │           http_server_message.hpp
│       │           session.hpp
│       │
│       └───plugin
│               dynamic_library.hpp
│               iplugin.hpp
│               plugin_entry.hpp
│               plugin_manager.hpp
```

## License

This project is licensed under the [MIT License](LICENSE).

## To-Do Features

- SSO optimization
- Unified standard documentation
- Unified container-related assert messages
- vcpkg integration
- Support for yaml and xml configurations
- Support for macOS and embedded Linux
