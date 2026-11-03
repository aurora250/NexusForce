#
# Native FreeBSD toolchain using the ports GCC together with the base libc++.
#
# GCC on FreeBSD defaults to libstdc++; the project's prebuilt dependencies
# (GTest, ICU, ...) are compiled with clang and libc++, so GCC is pointed at the
# base libc++ headers and runtime instead.  This keeps the ABI identical to the
# clang builds and lets the GCC job reuse the clang-built vcpkg tree.
#
# Usage (on FreeBSD):
#   pkg install gcc13 cmake ninja
#   cmake -B build-gcc -G Ninja \
#       -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/freebsd-gcc.cmake
#
set(FREEBSD_GCC_EXECUTABLE "gcc13" CACHE STRING "FreeBSD GCC executable")
set(FREEBSD_GXX_EXECUTABLE "g++13" CACHE STRING "FreeBSD G++ executable")

set(CMAKE_C_COMPILER "${FREEBSD_GCC_EXECUTABLE}" CACHE FILEPATH "" FORCE)
set(CMAKE_CXX_COMPILER "${FREEBSD_GXX_EXECUTABLE}" CACHE FILEPATH "" FORCE)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

# Base libc++ headers/runtime shipped with FreeBSD, not GCC's libstdc++.
set(CMAKE_CXX_FLAGS_INIT "-nostdinc++ -isystem /usr/include/c++/v1")
# GNU libgcc_s carries versioned symbols (GCC_4.5.0) that the base libgcc_s
# does not provide on FreeBSD/aarch64, so point the runtime loader at GCC's.
# The rpath is part of the standard libraries so it lands on every link line.
set(FREEBSD_GCC_RUNTIME_DIR "/usr/local/lib/gcc13" CACHE PATH "GCC runtime directory")
set(CMAKE_CXX_STANDARD_LIBRARIES
    "-lc++ -lcxxrt -Wl,--as-needed -Wl,-rpath,${FREEBSD_GCC_RUNTIME_DIR}")
set(CMAKE_C_STANDARD_LIBRARIES "")
