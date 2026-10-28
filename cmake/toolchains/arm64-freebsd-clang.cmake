#
# Cross-compilation toolchain for FreeBSD (aarch64) on a non-FreeBSD host.
#
# Prerequisites:
#   - clang / clang++ with lld
#   - a FreeBSD aarch64 sysroot extracted from base.txz:
#       sudo tar -xJpf base.txz -C /opt/freebsd/sysroot-arm64
#   - target dependencies (OpenSSL, PCRE2, ...) available for FreeBSD, either
#     from the sysroot or from the vcpkg triplet arm64-freebsd
#
# Usage A - dependencies already built for FreeBSD (no vcpkg):
#   cmake -B build-freebsd-arm64 -G Ninja \
#       -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm64-freebsd-clang.cmake \
#       -DVCPKG_MANIFEST_INSTALL=OFF
#
# Usage B - let vcpkg cross-build the dependencies:
#   cmake -B build-freebsd-arm64 -G Ninja \
#       -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
#       -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=cmake/toolchains/arm64-freebsd-clang.cmake \
#       -DVCPKG_OVERLAY_TRIPLETS=cmake/triplets \
#       -DVCPKG_TARGET_TRIPLET=arm64-freebsd
#
# The overlay triplet is required for Usage B: vcpkg forwards
# VCPKG_CHAINLOAD_TOOLCHAIN_FILE to port builds from the triplet, not from the
# CMake cache; without it the ports are compiled with the host compiler.
#
# Tests and examples cannot run on a non-FreeBSD host, so disable them.
#
set(CMAKE_SYSTEM_NAME FreeBSD)
# The project has no C++20 module sources; module scanning also needs a
# host clang-scan-deps that is not resolvable for this cross target.
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(FREEBSD_SYSROOT "/opt/freebsd/sysroot-arm64" CACHE PATH "FreeBSD aarch64 sysroot created from base.txz")
set(FREEBSD_TARGET_VERSION "14.4" CACHE STRING "FreeBSD release used for the clang target triple")

if(NOT EXISTS "${FREEBSD_SYSROOT}/usr/include/stdio.h")
    message(FATAL_ERROR "NexusForce: FreeBSD sysroot not found at '${FREEBSD_SYSROOT}'. "
                        "Extract base.txz there or pass -DFREEBSD_SYSROOT=<path>.")
endif()

# Respect a compiler selected by the caller (CLion or -DCMAKE_C(XX)_COMPILER);
# otherwise pick the newest clang available.
if(NOT DEFINED CMAKE_C_COMPILER)
    find_program(FREEBSD_C_COMPILER NAMES clang clang-20 clang-19 clang-18 REQUIRED)
    set(CMAKE_C_COMPILER "${FREEBSD_C_COMPILER}")
endif()
if(NOT DEFINED CMAKE_CXX_COMPILER)
    find_program(FREEBSD_CXX_COMPILER NAMES clang++ clang++-20 clang++-19 clang++-18 REQUIRED)
    set(CMAKE_CXX_COMPILER "${FREEBSD_CXX_COMPILER}")
endif()
set(FREEBSD_TARGET_TRIPLE "aarch64-unknown-freebsd${FREEBSD_TARGET_VERSION}")
set(CMAKE_C_COMPILER_TARGET "${FREEBSD_TARGET_TRIPLE}")
set(CMAKE_CXX_COMPILER_TARGET "${FREEBSD_TARGET_TRIPLE}")

set(CMAKE_SYSROOT "${FREEBSD_SYSROOT}")
# vcpkg manages CMAKE_FIND_ROOT_PATH itself and appends its installed tree to it;
# forcing ONLY modes here hides the vcpkg-installed configs, which breaks
# find_package() inside ports such as boost.
if(NOT CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg\\.cmake$")
    set(CMAKE_FIND_ROOT_PATH "${FREEBSD_SYSROOT}")
endif()

# vcpkg builds autotools/make ports (OpenSSL, ...) from CMAKE_<LANG>_FLAGS, not from
# CMAKE_<LANG>_COMPILER_TARGET / CMAKE_SYSROOT, so carry both here as well.
# -fPIC: the project links its static vcpkg dependencies into libNexusForce.so.
set(CMAKE_C_FLAGS_INIT "--target=${FREEBSD_TARGET_TRIPLE} --sysroot=${FREEBSD_SYSROOT} -fPIC")
set(CMAKE_CXX_FLAGS_INIT "--target=${FREEBSD_TARGET_TRIPLE} --sysroot=${FREEBSD_SYSROOT} -fPIC")

set(CMAKE_EXE_LINKER_FLAGS_INIT "-fuse-ld=lld")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-fuse-ld=lld")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "-fuse-ld=lld")

# Programs always come from the host: the sysroot ships FreeBSD executables
# (llvm-ar, ld.lld, ...) that a Linux host cannot run.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
if(NOT CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg\\.cmake$")
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
endif()
