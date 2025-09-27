# MSTL V1.3.0

[![Build Status](https://travis-ci.org/aurora250/MSTL.svg?branch=master)](https://travis-ci.org/aurora250/MSTL)
[![License](https://img.shields.io/badge/License-MIT%20License-blue.svg)](https://opensource.org/licenses/MIT)

> Read this in other languages: [Chinese](README.md)

This project aims to establish a comprehensive STL library (excluding concurrency libraries) that is highly readable and suitable for C++ beginners to learn and use, while providing various functional interfaces for educational purposes.
This project minimizes the use of standard libraries except for concurrency components and attempts to implement simplified versions from scratch.
We welcome issues and contributions to help improve this project. If there are deficiencies, please feel free to provide corrections.

Suggested learning approach for beginners: Read and use files in the order described in the file introduction section below, consulting classmates or AI when encountering difficulties.

If you are compiling on Windows, please ensure your system's code page is set to UTF-8 or use MSTL's built-in set_utf8_console function for configuration.

## What can you learn by reading and using MSTL?

- Using constexpr and if constexpr to reduce runtime overhead;
- Using concept and requires to strengthen code robustness;
- Strengthening noexcept guarantees;
- Using template meta-programming techniques such as variadic templates, recursive expansion, and template specialization to implement type traits and write functional containers;
- Functional programming design and type erasure design;
- Implementing SFINAE (Substitution Failure Is Not An Error) through enable_if;
- Implementing EBCO (Empty Base Class Optimization) through compressed_pair;
- Using compiler built-in attributes to optimize code behavior;
- Distinguishing type deduction and decay rules of decltype, auto, and template;
- Coordinated use of memory allocation and placement construction;
- Data manipulation methods of complex containers such as deque, red-black trees, and hash tables;
- Conversion rules between different character encoding types;
- Using Windows and Linux native interfaces to implement utility classes like datetime and file, understanding the similar data interfaces and processing methods between the two operating systems;
- Implementing most standard algorithms (including concurrent algorithms) and all commonly used standard containers, with extensions of some impractical algorithms for educational purposes;
- Implementation methods of more than ten general-purpose sorting algorithms;
- Usage of standard library concurrency interfaces (atomic/condition_variable/thread/mutex/future/packaged_task, etc.);
- Modern-style wrapper and usage of MySQL database C-style interfaces;
- Designing thread polling pool operation patterns;
- Socket wrapper for modern-style servlet for port listening and web operations;
  ......

## Supporting Environments

WINDOWS LINUX

X64

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

You can toggle dependencies in CMakeLists.txt in the project root
And directly modify dependency paths in src\CMakeLists.txt

- Windows

```bash
# Clone latest release version
git clone --depth 1 https://github.com/aurora250/MSTL.git
cd MSTL

# Create build directory
mkdir build && cd build

# Configure build options, you can also edit them in CMakeLists.txt
cmake .. -G "Visual Studio 17 2022" -A x64 \
  -DMSTL_ENABLE_QT6=OFF \
  -DMSTL_BUILD_TESTS=ON \
  -DMYSQL_ROOT_DIR="C:/Program Files/MySQL/MySQL Server 8.0"

# Build
cmake --build . --config Release

# Optional: Install to system directory
cmake --install . --config Release
```

- Linux

```bash
# Clone latest release version
git clone --depth 1 https://github.com/aurora250/MSTL.git
cd MSTL

# Create build directory
mkdir build && cd build

# Configure build options, you can also edit them in CMakeLists.txt
cmake .. -DCMAKE_BUILD_TYPE=Release \
  -DMSTL_ENABLE_QT6=OFF \
  -DMSTL_BUILD_TESTS=ON

# Build
make -j$(nproc)

# Optional: Install to system directory
sudo make install
```

## File Introduction

![File Structure](dependencies_structure.png)

The following files are introduced according to the hierarchical level of the file structure.

- [basiclib.hpp](include/MSTL/core/basiclib.hpp)

Implements multi-compilation environment adaptation using macros
for operating system platform, hosting platform, bus width, and C++ version,
and defines memory operation and C-style string operation functions.

- [type_traits.hpp](include/MSTL/core/type_traits.hpp)

Uses template meta-programming techniques to deduce type information at compile time
and provides hash functions for basic numeric types and iterator extractors.

- [errorlib.hpp](include/MSTL/core/exception.hpp)

Defines error types and quick invocation macros.
All error types in this project are contained in this file.
You can use BUILD_ERROR series macros to quickly build error types compatible with this project.

- [functor.hpp](include/MSTL/core/functor.hpp)

Defines functors and functor adapters (deprecated after C++11).

- [concepts.hpp](include/MSTL/core/concepts.hpp)

Defines commonly used constraints and iterator type judgment trait constants.

- [mathlib.hpp](include/MSTL/core/mathlib.hpp)

Defines commonly used constexpr mathematical functions and constants.

- [numeric.hpp](include/MSTL/core/numeric.hpp)

Defines mathematical algorithms.

- [utility.hpp](include/MSTL/core/utility.hpp)

Defines compressed_pair, pair and their hash functions, type erasure functions,
and C-style string to numeric type conversion functions.

- [heap.hpp](include/MSTL/core/heap.hpp)

Defines ordinary heap algorithms.

- [iterator.hpp](include/MSTL/core/iterator.hpp)

Defines iterator utility functions and iterator adapters.

- [tuple.hpp](include/MSTL/core/tuple.hpp)

Defines tuple class and its auxiliary functions, providing hash functions for tuple.

- [algobase.hpp](include/MSTL/core/algobase.hpp)

Defines comparison, copy, and move algorithms.

- [optional.hpp](include/MSTL/core/optional.hpp)

Defines optional class that can host a type and set empty value nullopt.

- [memory.hpp](include/MSTL/core/memory.hpp)

Defines memory operation functions, temporary buffer classes, allocator classes, and smart pointer classes.

- [array.hpp](include/MSTL/core/array.hpp)

Defines array class that can determine values at compile time and operate arrays in a safer, more modern way.

- [variant.hpp](include/MSTL/core/variant.hpp)

Defines variant class that can host multiple types simultaneously on the same block of memory.

- [string_view.hpp](include/MSTL/core/string_view.hpp)

Defines char_traits class, auxiliary extraction functions, and basic_string_view class with constexpr properties.

- [functional.hpp](include/MSTL/core/functional.hpp)

Defines function class that hosts function pointers and function-like types.

- [list.hpp](include/MSTL/core/list.hpp)

Defines doubly linked list class.

- [deque.hpp](include/MSTL/core/deque.hpp)

Defines double-ended queue class that can maintain map and buffer to allow data insertion at both front
and back using double buffering mechanism.

- [bitmap.hpp](include/MSTL/core/bitmap.hpp)

Defines bitmap class, but does not use it as bool specialization for vector.

- [vector.hpp](include/MSTL/core/vector.hpp)

Defines vector class.

- [algo.hpp](include/MSTL/core/algo.hpp)

Defines judgment, set, search, merge, move, transform, bind, permutation and other algorithms.

- [rb_tree.hpp](include/MSTL/core/rb_tree.hpp)

Defines red-black tree class rb_tree as proxy class for ordered containers.

- [basic_string.hpp](include/MSTL/core/basic_string.hpp)

Defines basic_string class.

- [queue.hpp](include/MSTL/core/queue.hpp)

Defines queue as adapter of deque, and priority_queue based on ordinary heap algorithms.

- [stack.hpp](include/MSTL/core/stack.hpp)

Defines stack as adapter of deque.

- [hashtable.hpp](include/MSTL/core/hashtable.hpp)

Defines hashtable class as proxy class for unordered containers.

- [leonardo_heap.hpp](include/MSTL/ext/leonardo_heap.hpp)

Defines leonardo heap algorithms.

- [sort.hpp](include/MSTL/ext/sort.hpp)

Defines multiple sorting algorithms including
bubble, cocktail, selection, shell, counting, bucket, index, merge, partial, quick, introspective, tim, monkey sorts.

- [sort.hpp](include/MSTL/ext/sort.hpp)

Define bubble, cocktail, select, shell, count, bucket, index, merge, partial, quick, introspective, tim, monkey sort algorithms.

- [algorithm.hpp](include/MSTL/core/algorithm.hpp)

Includes basic algorithms and mathematical algorithms, defines concurrent algorithms for convenient user inclusion.

- [map.hpp](include/MSTL/core/map.hpp)

Defines ordered dictionary classes map and multimap.

- [set.hpp](include/MSTL/core/set.hpp)

Defines ordered set classes set and multiset.

- [string.hpp](include/MSTL/core/string.hpp)

Defines string classes for multiple character types and provides their hash functions, conversion functions
from other character types to UTF-8 encoded string types,
and conversion functions from basic data types to string types.

- [unordered_map.hpp](include/MSTL/core/unordered_map.hpp)

Defines unordered dictionary classes unordered_map and unordered_multimap.

- [unordered_set.hpp](include/MSTL/core/unordered_set.hpp)

Defines unordered set classes unordered_set and unordered_multiset.

- [timer.hpp](include/MSTL/ext/timer.hpp)

Defines timer class that can manually poll to implement timing operations.

- [datetime.hpp](include/MSTL/core/datetime.hpp)

Defines time, date, datetime, and UNIX timestamp classes, providing convenient operation utility functions.

- [stringstream.hpp](include/MSTL/core/stringstream.hpp)

Defines stream-like string classes basic_istringstream, basic_ostringstream, and basic_stringstream.
They are not based on standard IO streams but are merely string classes that behave like streams.

- [trace_memory.hpp](include/MSTL/ext/trace_memory.hpp)

Defines boost-based stack tracing allocator trace_allocator.

- [random.hpp](include/MSTL/core/random.hpp)

Defines pseudo-random number generator classes random_lcd, random_mt,
and hardware noise-based true random number generator class secret.

- [file.hpp](include/MSTL/core/file.hpp)

Defines file operation class based on OS native interfaces,
using 8KB buffer to accommodate high-volume small data read/write operations.

- [check_type.hpp](include/MSTL/core/check_type.hpp)

Defines type information analysis class to make type information cleaner.

- [thread_pool.hpp](include/MSTL/ext/thread_pool.hpp)

Defines pooling thread pool type.

- [print.hpp](include/MSTL/core/print.hpp)

Defines type information output functions to quickly obtain
well-formatted type content or content containing type information.
Use printer to quickly extend custom output.

- [database_pool.hpp](include/MSTL/ext/database_pool.hpp)

Defines polymorphic database connections and the database connection pool
that support MySQL, SQLite3, and Redis connections.

- [servlet.hpp](include/MSTL/web/servlet.hpp)

Defines servlet class, providing port listening, filter configuration,
cookie setting, session attribute operations, and other functionalities.

## License

This project is based on the [MIT License](LICENSE) 。
