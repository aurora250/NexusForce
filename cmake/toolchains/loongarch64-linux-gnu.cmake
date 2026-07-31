#
# Cross-compilation toolchain for LoongArch64 on Linux X64 host
#
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR loongarch64)

set(CMAKE_C_COMPILER loongarch64-linux-gnu-gcc-14)
set(CMAKE_CXX_COMPILER loongarch64-linux-gnu-g++-14)

set(CMAKE_EXE_LINKER_FLAGS_INIT "-Wl,-rpath-link,/usr/loongarch64-linux-gnu/lib:/usr/loongarch64-linux-gnu/lib64")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-Wl,-rpath-link,/usr/loongarch64-linux-gnu/lib:/usr/loongarch64-linux-gnu/lib64")

set(CMAKE_CROSSCOMPILING_EMULATOR "qemu-loongarch64-static;-L;/usr/loongarch64-linux-gnu")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
