#
# Cross-compilation toolchain for RISCV64 on Linux X64 host
#
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR riscv64)

set(CMAKE_C_COMPILER riscv64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER riscv64-linux-gnu-g++)

# Enable Zihintpause extension for pause instruction
set(CMAKE_C_FLAGS_INIT "-march=rv64gc_zihintpause")
set(CMAKE_CXX_FLAGS_INIT "-march=rv64gc_zihintpause")

set(CMAKE_CROSSCOMPILING_EMULATOR "qemu-riscv64-static;-L;/usr/riscv64-linux-gnu")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
