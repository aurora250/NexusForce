#
# Cross-compilation toolchain for LoongArch64 using Clang
#
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR loongarch64)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_C_COMPILER_TARGET loongarch64-linux-gnu)
set(CMAKE_CXX_COMPILER_TARGET loongarch64-linux-gnu)

set(CMAKE_CROSSCOMPILING_EMULATOR "qemu-loongarch64-static;-L;/usr/loongarch64-linux-gnu")

set(CMAKE_EXE_LINKER_FLAGS_INIT "-Wl,-rpath-link,/usr/loongarch64-linux-gnu/lib:/usr/loongarch64-linux-gnu/lib64")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-Wl,-rpath-link,/usr/loongarch64-linux-gnu/lib:/usr/loongarch64-linux-gnu/lib64")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
