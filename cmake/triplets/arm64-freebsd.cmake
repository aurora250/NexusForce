#
# Overlay triplet for cross-building arm64 FreeBSD dependencies on a non-FreeBSD host.
#
# Mirrors vcpkg's community triplet community/arm64-freebsd.cmake, but chains in
# the NexusForce FreeBSD toolchain so every port is compiled for FreeBSD instead
# of falling back to the host compiler.
#
# Usage:
#   -DVCPKG_OVERLAY_TRIPLETS=<repo>/cmake/triplets
#   -DVCPKG_TARGET_TRIPLET=arm64-freebsd
#   -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=<repo>/cmake/toolchains/arm64-freebsd-clang.cmake
#
set(VCPKG_TARGET_ARCHITECTURE arm64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME FreeBSD)
# autoconf ports (icu, ...) need an explicit --host, otherwise configure
# treats the build as native and tries to run FreeBSD test binaries.
set(VCPKG_MAKE_BUILD_TRIPLET "--host=aarch64-unknown-freebsd14.4")
set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/../toolchains/arm64-freebsd-clang.cmake")
