#
# Cross-compilation toolchain for macOS (x86_64) on a non-macOS host.
#
# The toolchain drives an osxcross installation, which provides the macOS SDK
# and the cctools linker, with a host clang that is new enough for the SDK's
# libc++.  Only the target libraries and SDK are taken from the cross tree;
# every build tool (clang, cmake, ninja) still runs on the host.
#
# Prerequisites:
#   - osxcross built with a macOS SDK, e.g.
#       /home/user/osxcross/target/SDK/MacOSX26.5.sdk
#   - clang >= 19 on the host (the SDK's libc++ uses builtins added in clang 19)
#   - target OpenSSL, PCRE2, zlib and GTest installed into a prefix, built once
#       with: NEXUSFORCE_MACOS_ARCH=x86_64 scripts/build_macos_deps.sh
#
# Usage (the dependencies default to <repo>/build-macos-deps-x86_64):
#   NEXUSFORCE_MACOS_ARCH=x86_64 scripts/build_macos_deps.sh
#   cmake -B build-macos-x86_64 -G Ninja \
#       -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/x86_64-macos-clang.cmake \
#       -DCMAKE_BUILD_TYPE=Release
#   cmake --build build-macos-x86_64 --target NexusForceUnitTests
#   scripts/run_darling_tests.sh build-macos-x86_64
#
# osxcross is discovered through OSXCROSS_TARGET_DIR, ~/osxcross/target,
# ~/Workspace/osxcross/target, /opt/osxcross/target or
# -DNEXUSFORCE_MACOS_OSXCROSS_ROOT=<osxcross>/target.
#
set(CMAKE_SYSTEM_NAME Darwin)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
# The project has no C++20 module sources; module scanning also needs a host
# clang-scan-deps that is not resolvable for this cross target.
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set(CMAKE_OSX_ARCHITECTURES x86_64)

set(NEXUSFORCE_MACOS_OSXCROSS_ROOT "" CACHE PATH "osxcross target directory (contains bin/ and SDK/)")
set(NEXUSFORCE_MACOS_SDK "" CACHE PATH "macOS SDK used for the target")
get_filename_component(_nexusforce_repo_root "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
set(NEXUSFORCE_MACOS_DEPS "${_nexusforce_repo_root}/build-macos-deps-x86_64" CACHE PATH
        "prefix holding the target OpenSSL, PCRE2, zlib and GTest")
set(NEXUSFORCE_MACOS_DEPLOYMENT_TARGET "11.0" CACHE STRING "minimum macOS version of the produced binaries")
set(NEXUSFORCE_MACOS_TARGET_TRIPLE "x86_64-apple-darwin25.5" CACHE STRING "clang target triple")

# Locate osxcross without forcing every caller to repeat the path.  The
# environment variable set by osxcross's own osxcross-conf is honoured first.
if(NOT NEXUSFORCE_MACOS_OSXCROSS_ROOT)
    foreach(_candidate "$ENV{OSXCROSS_TARGET_DIR}"
            "$ENV{HOME}/osxcross/target"
            "$ENV{HOME}/Workspace/osxcross/target"
            "/opt/osxcross/target"
            "/usr/local/osxcross/target")
        if(_candidate AND EXISTS "${_candidate}/SDK")
            set(NEXUSFORCE_MACOS_OSXCROSS_ROOT "${_candidate}" CACHE PATH
                    "osxcross target directory (contains bin/ and SDK/)" FORCE)
            break()
        endif()
    endforeach()
endif()
if(NOT NEXUSFORCE_MACOS_OSXCROSS_ROOT OR NOT EXISTS "${NEXUSFORCE_MACOS_OSXCROSS_ROOT}/SDK")
    message(FATAL_ERROR
            "NexusForce: osxcross not found. Pass -DNEXUSFORCE_MACOS_OSXCROSS_ROOT=<osxcross>/target.")
endif()

if(NOT NEXUSFORCE_MACOS_SDK)
    file(GLOB _macos_sdks LIST_DIRECTORIES true
            "${NEXUSFORCE_MACOS_OSXCROSS_ROOT}/SDK/MacOSX*.sdk")
    if(NOT _macos_sdks)
        message(FATAL_ERROR
                "NexusForce: no macOS SDK found under '${NEXUSFORCE_MACOS_OSXCROSS_ROOT}/SDK'.")
    endif()
    list(SORT _macos_sdks COMPARE NATURAL ORDER DESCENDING)
    list(GET _macos_sdks 0 NEXUSFORCE_MACOS_SDK)
    set(NEXUSFORCE_MACOS_SDK "${NEXUSFORCE_MACOS_SDK}" CACHE PATH "macOS SDK used for the target" FORCE)
endif()
if(NOT EXISTS "${NEXUSFORCE_MACOS_SDK}/usr/include/stdio.h")
    message(FATAL_ERROR "NexusForce: invalid macOS SDK '${NEXUSFORCE_MACOS_SDK}'.")
endif()

# The SDK ships a libc++ that requires clang >= 19; prefer the newest
# versioned clang so an older default 'clang' on PATH does not get picked.
if(NOT DEFINED CMAKE_C_COMPILER)
    find_program(NEXUSFORCE_MACOS_C_COMPILER NAMES clang-20 clang-19 clang REQUIRED)
    set(CMAKE_C_COMPILER "${NEXUSFORCE_MACOS_C_COMPILER}")
endif()
if(NOT DEFINED CMAKE_CXX_COMPILER)
    find_program(NEXUSFORCE_MACOS_CXX_COMPILER NAMES clang++-20 clang++-19 clang++ REQUIRED)
    set(CMAKE_CXX_COMPILER "${NEXUSFORCE_MACOS_CXX_COMPILER}")
endif()

set(CMAKE_C_COMPILER_TARGET "${NEXUSFORCE_MACOS_TARGET_TRIPLE}")
set(CMAKE_CXX_COMPILER_TARGET "${NEXUSFORCE_MACOS_TARGET_TRIPLE}")

# CMake queries the compiler while configuring; keep every probe on the target.
set(CMAKE_SYSROOT "${NEXUSFORCE_MACOS_SDK}")
set(CMAKE_OSX_SYSROOT "${NEXUSFORCE_MACOS_SDK}")

# clang >= 19 has no -fuse-ld=lld on Linux (the Slackware/Ubuntu driver only
# knows ld/bfd); cctools' ld64 is selected by path instead.
if(NOT DEFINED NEXUSFORCE_MACOS_LD)
    file(GLOB _macos_lds "${NEXUSFORCE_MACOS_OSXCROSS_ROOT}/bin/x86_64-apple-darwin*-ld")
    if(NOT _macos_lds)
        message(FATAL_ERROR
                "NexusForce: no x86_64 cctools linker found under '${NEXUSFORCE_MACOS_OSXCROSS_ROOT}/bin'.")
    endif()
    list(GET _macos_lds 0 _macos_ld)
    set(NEXUSFORCE_MACOS_LD "${_macos_ld}" CACHE FILEPATH "cctools ld64 used for linking")
endif()

set(_nexusforce_macos_flags
        "--target=${NEXUSFORCE_MACOS_TARGET_TRIPLE} -isysroot ${NEXUSFORCE_MACOS_SDK} -mmacosx-version-min=${NEXUSFORCE_MACOS_DEPLOYMENT_TARGET}")
set(CMAKE_C_FLAGS_INIT "${_nexusforce_macos_flags}")
set(CMAKE_CXX_FLAGS_INIT "${_nexusforce_macos_flags}")

set(_nexusforce_macos_link_flags "--ld-path=${NEXUSFORCE_MACOS_LD} -mmacosx-version-min=${NEXUSFORCE_MACOS_DEPLOYMENT_TARGET}")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${_nexusforce_macos_link_flags}")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "${_nexusforce_macos_link_flags}")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "${_nexusforce_macos_link_flags}")

# Target binutils; CMake's defaults would be the host's GNU tools.  osxcross
# names them after the full triple (<arch>-apple-darwin<version>-ar), so the
# tools are globbed instead of guessed.
function(_nexusforce_macos_find_tool out_var tool)
    file(GLOB _candidates "${NEXUSFORCE_MACOS_OSXCROSS_ROOT}/bin/x86_64-apple-darwin*-${tool}")
    if(NOT _candidates)
        message(FATAL_ERROR
                "NexusForce: osxcross tool '${tool}' not found under '${NEXUSFORCE_MACOS_OSXCROSS_ROOT}/bin'.")
    endif()
    list(GET _candidates 0 _candidate)
    set(${out_var} "${_candidate}" PARENT_SCOPE)
endfunction()

_nexusforce_macos_find_tool(NEXUSFORCE_MACOS_AR ar)
_nexusforce_macos_find_tool(NEXUSFORCE_MACOS_RANLIB ranlib)
set(CMAKE_AR "${NEXUSFORCE_MACOS_AR}" CACHE FILEPATH "target archiver" FORCE)
set(CMAKE_RANLIB "${NEXUSFORCE_MACOS_RANLIB}" CACHE FILEPATH "target ranlib" FORCE)

foreach(_tool install_name_tool otool lipo dsymutil strip nm)
    file(GLOB _candidates "${NEXUSFORCE_MACOS_OSXCROSS_ROOT}/bin/x86_64-apple-darwin*-${_tool}")
    if(_candidates)
        list(GET _candidates 0 _candidate)
        string(TOUPPER "${_tool}" _tool_upper)
        set(CMAKE_${_tool_upper} "${_candidate}")
    endif()
endforeach()

# Target dependencies.  Debian's multiarch layout is deliberately not searched:
# only configs/libraries from this prefix and the SDK may satisfy find_package.
if(NEXUSFORCE_MACOS_DEPS)
    list(APPEND CMAKE_PREFIX_PATH "${NEXUSFORCE_MACOS_DEPS}")
    set(OPENSSL_ROOT_DIR "${NEXUSFORCE_MACOS_DEPS}" CACHE PATH "target OpenSSL prefix")
endif()

set(CMAKE_FIND_ROOT_PATH "${NEXUSFORCE_MACOS_DEPS}" "${NEXUSFORCE_MACOS_SDK}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
