# MSTL V1.3.1

[![Build Status](https://travis-ci.org/aurora250/MSTL.svg?branch=master)](https://travis-ci.org/aurora250/MSTL)
[![License](https://img.shields.io/badge/License-MIT%20License-blue.svg)](https://opensource.org/licenses/MIT)

> Read this in other languages: [Chinese](README.md)

This project aims to establish a readable and relatively complete STL library (excluding concurrency libraries) for C++ beginners to learn and use, while providing various functional interfaces.
It minimizes the use of standard libraries except for concurrency components and attempts to implement simplified versions from scratch.
We welcome issues to help improve this project. If there are any deficiencies, please feel free to correct them.

Suggested learning approach for beginners: Read and use the files in the order described in the file introduction section below. When in doubt, consult classmates or AI.

This library assumes your operating system uses UTF-8 code page when working with IO devices. If not, please try to configure it; otherwise, garbled characters may occur during IO operations.


## What can you learn by reading and using MSTL?

- Using `constexpr` and `if constexpr` to reduce runtime overhead;
- Using `concept` and `requires` to enhance code robustness;
- Strengthening `noexcept` guarantees;
- Using template metaprogramming techniques such as variadic templates, recursive expansion, and template specialization to implement type traits and write functional containers;
- Functional programming design and type erasure design;
- Using compiler-built-in attributes to optimize code behavior;
- Distinguishing type deduction and decay rules among `decltype`, `auto`, and templates;
- Implementing SFINAE (Substitution Failure Is Not An Error) via `enable_if`;
- Implementing EBCO (Empty Base Class Optimization) via `compressed_pair`;
- Coordinating memory allocation with in-place construction;
- Conversion rules between character encodings like UTF-8, UTF-16, and UTF-32;
- Using `format` for fast string formatting;
- Using CRTP (Curiously Recurring Template Pattern) for static polymorphism;
- Implementing utility classes like `datetime` and `file` using Windows and Linux native interfaces, understanding the similar yet distinct data interfaces and processing methods between the two OSes;
- Data manipulation methods for complex containers such as deques, red-black trees, and hash tables;
- Implementing most standard algorithms (including concurrent ones) and all commonly used standard containers, with extensions of some non-practical algorithms for educational purposes;
- Implementation methods for over ten general sorting functions;
- Using standard library concurrency interfaces (`atomic`/`conditional_variable`/`thread`/`mutex`/`future`/`packaged_task`, etc.);
- Modern wrappers and usage for MySQL and Redis interfaces;
- Designing thread pools with polling patterns;
- Wrapping sockets into modern-style servlets for web operations;
  ......

## Supporting Environments

WINDOWS LINUX

X64 X86

MSVC GCC CLANG

C++ 14 17 20

## Build Guide

### Prerequisites

- CMake 3.17+
- Compiler supporting C++14 or higher (GCC 7+, Clang 5+, MSVC 2017+)
- Optional dependencies:
  - Boost
  - MySQL
  - SQLite3
  - hiredis
  - Qt6
  - CUDA Toolkit (MSVC only)

Note: This project has discontinued CUDA support, which is disabled by default.

### Build Steps

You can toggle dependencies in the root `CMakeLists.txt` and directly modify local dependency paths in `src/CMakeLists.txt`.

- Windows

```bash
# Clone the latest release
git clone --depth 1 https://github.com/aurora250/MSTL.git
cd MSTL

# Create build directory
mkdir build && cd build

# Configure build options (can also modify in CMakeLists.txt)
cmake .. -G "Visual Studio 17 2022" -A x64 \
  -DMSTL_ENABLE_QT6=OFF \
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

# Configure build options (can also modify in CMakeLists.txt)
cmake .. -DCMAKE_BUILD_TYPE=Release \
  -DMSTL_ENABLE_QT6=OFF \
  -DMSTL_BUILD_TESTS=ON

# Build
make -j$(nproc)

# Install to system directory
sudo make install
```

## File Introduction

![File Structure](dependencies_structure.png)

The following files are introduced in the order of the hierarchical structure shown above.

- [environment](include/MSTL/core/environment.hpp)

Defines macros for operating system platform, hosting platform, bus width, and C++ version, implementing multi-compilation environment adaptation.

- [vsprintf](include/MSTL/core/vsprintf.hpp)

Defines a series of functions to output variable argument lists to formatted strings.

- [type_traits](include/MSTL/core/type_traits.hpp)

Defines type trait constants, using template metaprogramming to deduce type information at compile time.

- [exception](include/MSTL/core/exception.hpp)

Defines error types and quick-invocation macros. All error types in this project are defined in this file.

- [random](include/MSTL/core/random.hpp)

Defines pseudo-random number generators (`random_lcd`, `random_mt`) and a hardware noise-based true random number generator (`secret`).

- [socket](include/MSTL/web/socket.hpp)

Defines the network socket class `socket`.

- [functor](include/MSTL/core/functor.hpp)

Defines functors and functor adapters (deprecated in C++11).

- [iterator_traits](include/MSTL/core/iterator_traits.hpp)

Defines the iterator extractor `iterator_traits` and convenient type aliases.

- [interface](include/MSTL/core/interface.hpp)

Defines a series of basic CRTP base classes and globally generated functions based on them.

- [hash](include/MSTL/core/hash.hpp)

Defines hash functions for basic types and utility hash functions like FNV.

- [numeric_limits](include/MSTL/core/numeric_limits.hpp)

Defines the numeric type information class `numeric_limits`, providing mathematical details of numeric types at compile time.

- [mutex](include/MSTL/core/mutex.hpp)

Defines the mutex class `mutex` and the scoped locking class `lock_guard`.

- [concepts](include/MSTL/core/concepts.hpp)

Defines common constraints and iterator type judgment trait constants.

- [utility](include/MSTL/core/utility.hpp)

Defines `compressed_pair`, `pair` and their hash functions, type erasure functions, and functions to convert C-style strings to numeric types.

- [tuple](include/MSTL/core/tuple.hpp)

Defines the tuple class `tuple` and its auxiliary functions.

- [mathlib](include/MSTL/core/mathlib.hpp)

Defines common `constexpr` mathematical constants and functions.

- [ratio](include/MSTL/core/ratio.hpp)

Defines the ratio class `ratio`.

- [numeric](include/MSTL/core/numeric.hpp)

Defines mathematical algorithms.

- [heap](include/MSTL/core/heap.hpp)

Defines ordinary heap algorithms.

- [iterator](include/MSTL/core/iterator.hpp)

Defines iterator utility functions and iterator adapters.

- [algobase](include/MSTL/core/algobase.hpp)

Defines comparison, copy, and move algorithms.

- [any](include/MSTL/core/any.hpp)

Defines the `any` class, which can store any type.

- [cstring](include/MSTL/core/cstring.hpp)

Defines memory operation functions and C-style string operation functions.

- [memory](include/MSTL/core/memory.hpp)

Defines memory operation functions, allocator classes, and smart pointer classes.

- [functional](include/MSTL/core/functional.hpp)

Defines the `function` class that hosts function pointers and function-like types.

- [algo](include/MSTL/core/algo.hpp)

Defines algorithms for judgment, set operations, searching, merging, moving, transforming, binding, and permutations.

- [thread](include/MSTL/core/thread.hpp)

Defines the thread class `thread`.

- [sort](include/MSTL/ext/sort.hpp)

Defines multiple sorting algorithms: bubble, cocktail, selection, shell, counting, bucket, index, merge,
partial, quick, introspective, tim, and monkey sort.

- [algorithm](include/MSTL/core/algorithm.hpp)

Includes basic algorithms and mathematical algorithms, 
and defines concurrent algorithms for convenient inclusion by users.

- [char_traits](include/MSTL/core/char_traits.hpp)

Defines the string traits class `basic_char_traits` and auxiliary extraction functions.

- [basic_string_view](include/MSTL/core/basic_string_view.hpp)

Defines the base class `basic_string_view` for string views.

- [string_view](include/MSTL/core/string_view.hpp)

Defines the string view class `string_view`.

- [basic_string](include/MSTL/core/basic_string.hpp)

Defines the base string class `basic_string`.

- [string](include/MSTL/core/string.hpp)

Defines the string class `string`, providing conversion functions between different character encodings.

- [format](include/MSTL/core/format.hpp)

Defines the string formatting helper class `formatter` and the formatting function `format`.

- [encrypt](include/MSTL/core/encrypt.hpp)

Defines character encryption types and functions: `XOR`, `base64`, `MD5`, `SHA1`, `SHA256`, and `AES256`.

- [check_type](include/MSTL/core/check_type.hpp)

Defines the type information analysis function `check_type` to standardize type information across compilers.

- [serialize](include/MSTL/core/serialize.hpp)

Defines a series of CRTP base classes for serialization.

- [datetime](include/MSTL/core/datetime.hpp)

Defines time classes (`time`, `date`, `datetime`) and UNIX timestamp class (`timestamp`), providing convenient utility functions.

- [hexadecimal](include/MSTL/core/hexadecimal.hpp)

Defines the hexadecimal class `hexadecimal` and its formatting helper class.

- [color](include/MSTL/core/color.hpp)

Defines the color class `color`.

- [console](include/MSTL/core/console.hpp)

Defines the IO base class `io_base` and the IO console class `console`.

- [variant](include/MSTL/core/variant.hpp)

Defines the `variant` class, which can host multiple types in the same memory block.

- [optional](include/MSTL/core/optional.hpp)

Defines the `optional` class, which can host a type and optionally set a null value `nullopt`.

- [array](include/MSTL/core/array.hpp)

Defines the array class `array`, which allows compile-time value determination and safer, more modern array operations.

- [bitmap](include/MSTL/core/bitmap.hpp)

Defines the bitmap class `bitmap`, which does not exist as a `vector<bool>` specialization.

- [vector](include/MSTL/core/vector.hpp)

Defines the vector class `vector`.

- [list](include/MSTL/core/list.hpp)

Defines the doubly linked list class `list`.

- [deque](include/MSTL/core/deque.hpp)

Defines the deque class `deque`, which supports O(1) insertion at both front and back.

- [rb_tree](include/MSTL/core/rb_tree.hpp)

Defines the red-black tree class `rb_tree`, used as a proxy class for ordered containers.

- [hashtable](include/MSTL/core/hashtable.hpp)

Defines the hash table class `hashtable`, used as a proxy class for unordered containers.

- [unordered_map](include/MSTL/core/unordered_map.hpp)

Defines the unordered dictionary classes `unordered_map` and `unordered_multimap`.

- [unordered_set](include/MSTL/core/unordered_set.hpp)

Defines the unordered set classes `unordered_set` and `unordered_multiset`.

- [leonardo_heap](include/MSTL/ext/leonardo_heap.hpp)

Defines the Leonardo heap algorithm `leonardo_heap`.

- [queue](include/MSTL/core/queue.hpp)

Defines the queue class `queue` and the priority queue class `priority_queue` based on ordinary heap algorithms.

- [stack](include/MSTL/core/stack.hpp)

Defines the stack class `stack`.

- [map](include/MSTL/core/map.hpp)

Defines the ordered dictionary classes `map` and `multimap`.

- [set](include/MSTL/core/set.hpp)

Defines the ordered set classes `set` and `multiset`.

- [file](include/MSTL/core/file.hpp)

Defines the file class `file`, which uses an 8KB buffer to handle high-volume small data read/write operations.

- [json](include/MSTL/core/json.hpp)

Defines the JSON parser class `json_parser` and JSON builder class `json_builder`.

- [session](include/MSTL/web/session.hpp)

Defines the cookie class, session class `session`, and HTTP constants.

- [servlet](include/MSTL/web/servlet.hpp)

Defines the microservice class `servlet`, providing port listening, filter configuration, cookie setting, session attribute operations, and other functionalities.

- [trace_memory](include/MSTL/ext/trace_memory.hpp)

Defines the Boost-based stack-tracing allocator `trace_allocator`.

- [database_pool](include/MSTL/ext/database_pool.hpp)

Defines polymorphic database connections (supporting MySQL, Sqlite3, Redis) and the database connection pool `database_pool`.

- [thread_pool](include/MSTL/ext/thread_pool.hpp)

Defines the polling thread pool class `thread_pool`.

- [timer](include/MSTL/ext/timer.hpp)

Defines the timer class `timer`.


## License

This project is licensed under the [MIT License](LICENSE).
