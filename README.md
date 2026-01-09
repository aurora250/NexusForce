# MSTL V1.4.0

[![Build Status](https://travis-ci.org/aurora250/MSTL.svg?branch=master)](https://travis-ci.org/aurora250/MSTL)
[![License](https://img.shields.io/badge/License-MIT%20License-blue.svg)](https://opensource.org/licenses/MIT)

> 通过其他语言阅读: [English](README.EN.md)

本项目旨在建立功能健全、风格统一、可读性强、社区共建、跨平台兼容的
现代C++开发库MSTL(Modern Standard Template Library)。
通过清晰的架构设计、规范的代码实现、丰富的设计模式应用，
为项目开发提供实用的工具集，同时也为C++学习者提供理解底层原理的实践载体。
有劳各位多多issue，使本项目趋于健全。如有不足，还望斧正。

本库使用IO设备时默认您的操作系统为代码页为UTF-8，如不是，请尝试设置，否则可能在IO时乱码。

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
  - CUDA Toolkit

请注意：MSTL已停止对CUDA的支持，它被默认关闭依赖

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

## Include 结构

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

## 开源协议

本项目基于 [MIT 开源协议](LICENSE) 。

## 待实现功能

- SSO优化
- 统一的标准注释
- 统一容器相关assert提示
- 接入vcpkg
- 支持yaml、xml配置
- 支持macOS、嵌入式Linux
