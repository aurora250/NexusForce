cmake_minimum_required(VERSION 3.21)

if(POLICY CMP0218)
    cmake_policy(SET CMP0218 NEW)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

file(READ "config.json" JSON_CONTENT)


if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE PATH "vcpkg toolchain")
elseif(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND NOT CMAKE_HOST_APPLE)
    if(WIN32)
        string(JSON CONFIG_VCPKG_PATH GET ${JSON_CONTENT} vcpkg windows)
    else()
        string(JSON CONFIG_VCPKG_PATH GET ${JSON_CONTENT} vcpkg linux)
    endif()
    set(CMAKE_TOOLCHAIN_FILE "${CONFIG_VCPKG_PATH}/scripts/buildsystems/vcpkg.cmake" CACHE PATH "vcpkg toolchain")
endif()

if(WIN32)
    string(JSON CONFIG_DOT_EXECUTABLE GET ${JSON_CONTENT} graphviz windows)
else()
    string(JSON CONFIG_DOT_EXECUTABLE GET ${JSON_CONTENT} graphviz linux)
endif()
string(JSON CONFIG_CPP_STANDARD      GET ${JSON_CONTENT} standard)
string(JSON CONFIG_ENABLE_POSTGRESQL GET ${JSON_CONTENT} dependency postgresql switch)
string(JSON CONFIG_ENABLE_MYSQL      GET ${JSON_CONTENT} dependency mysql      switch)
string(JSON CONFIG_ENABLE_SQLITE3    GET ${JSON_CONTENT} dependency sqlite3    switch)
string(JSON CONFIG_ENABLE_HIREDIS    GET ${JSON_CONTENT} dependency hiredis    switch)
string(JSON CONFIG_ENABLE_ZLIB       GET ${JSON_CONTENT} dependency zlib       switch)
string(JSON CONFIG_ENABLE_LZ4        GET ${JSON_CONTENT} dependency lz4        switch)
string(JSON CONFIG_ENABLE_ZSTD       GET ${JSON_CONTENT} dependency zstd       switch)
string(JSON CONFIG_ENABLE_ICU        GET ${JSON_CONTENT} dependency icu        switch)

# The locale component can be dropped entirely, which is what small embedded builds want:
# the key is optional so that an older config.json keeps the component enabled.
string(JSON CONFIG_ENABLE_LOCALE ERROR_VARIABLE _config_locale_error GET ${JSON_CONTENT} feature locale)
if(_config_locale_error)
    set(CONFIG_ENABLE_LOCALE ON)
endif()

string(JSON CONFIG_USING_SSO         GET ${JSON_CONTENT} feature sso)
string(JSON CONFIG_USING_INTEL_TSX   GET ${JSON_CONTENT} feature intel-tsx)
string(JSON CONFIG_USING_SQLCIPHER   GET ${JSON_CONTENT} feature sqlcipher)
string(JSON CONFIG_USING_IO_URING    GET ${JSON_CONTENT} feature io_uring)
string(JSON CONFIG_USING_MEMORY_POOL GET ${JSON_CONTENT} feature memory-pool)

# The global allocator override is a process wide decision and stays off unless config.json asks for it.
string(JSON CONFIG_MEMORY_POOL_OVERRIDE ERROR_VARIABLE _config_pool_override_error
        GET ${JSON_CONTENT} feature memory-pool-global-override)
if(_config_pool_override_error)
    set(CONFIG_MEMORY_POOL_OVERRIDE OFF)
endif()

# Missing keys default to ON: keeps AES-NI/PCLMUL enabled on Linux/GCC by
# default, aligned with the MSVC-always-available assumption.
string(JSON CONFIG_USING_AES_NI ERROR_VARIABLE _config_aes_ni_error GET ${JSON_CONTENT} feature aes-ni)
if(_config_aes_ni_error)
    set(CONFIG_USING_AES_NI ON)
endif()
string(JSON CONFIG_USING_PCLMUL ERROR_VARIABLE _config_pclmul_error GET ${JSON_CONTENT} feature pclmul)
if(_config_pclmul_error)
    set(CONFIG_USING_PCLMUL ON)
endif()
string(JSON CONFIG_USING_VAES ERROR_VARIABLE _config_vaes_error GET ${JSON_CONTENT} feature vaes)
if(_config_vaes_error)
    set(CONFIG_USING_VAES OFF)
endif()

string(JSON CONFIG_BUILD_TESTS       GET ${JSON_CONTENT} build tests)
string(JSON CONFIG_BUILD_EXAMPLES    GET ${JSON_CONTENT} build examples)
string(JSON CONFIG_BUILD_BENCHMARKS  GET ${JSON_CONTENT} build benchmarks)
string(JSON CONFIG_BUILD_DOCS        GET ${JSON_CONTENT} build docs)



set(NEXUSFORCE_VERSION_MAJOR 2)
set(NEXUSFORCE_VERSION_MINOR 0)
set(NEXUSFORCE_VERSION_PATCH 0)
set(NEXUSFORCE_VERSION "${NEXUSFORCE_VERSION_MAJOR}.${NEXUSFORCE_VERSION_MINOR}.${NEXUSFORCE_VERSION_PATCH}")


# Force MSVC English locale so CMake can parse /showIncludes output for dependency tracking.
# Without this, non-English MSVC outputs localized "注意: 包含文件:" lines that
# CMake's regex cannot match, causing them to leak to the terminal as noise.
#
# ENV{VSLANG} covers compiler detection during the configure step.
# CMAKE_*_COMPILER_LAUNCHER covers every compiler invocation during the build step
# and MUST be set before project() so CMake initializes its internal cache entry.
if(WIN32)
    set(ENV{VSLANG} "1033")
    set(VSLANG_LAUNCHER "${CMAKE_COMMAND};-E;env;VSLANG=1033")
    set(CMAKE_C_COMPILER_LAUNCHER "${VSLANG_LAUNCHER}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${VSLANG_LAUNCHER}")
endif()


project(NexusForce
        VERSION ${NEXUSFORCE_VERSION}
        LANGUAGES CXX
)
# An empty CMAKE_BUILD_TYPE (for example a CLion profile whose Build type is "Default") defines
# neither NDEBUG nor CONFIG=Debug. Debug-only death tests are then compiled in while
# NEFORCE_DEBUG_VERIFY is compiled out, and they fail with "failed to die". Default to Release for
# single-config generators; multi-config generators select a configuration explicitly.
if((NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "None") AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    message(STATUS "CMAKE_BUILD_TYPE not set; defaulting to Release")
endif()



include(GNUInstallDirs)
include(CMakePackageConfigHelpers)


set(VCPKG_MANIFEST_MODE ON)
set(VCPKG_MANIFEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(VCPKG_INSTALLED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed")
set(VCPKG_MANIFEST_INSTALL ON)

if(DEFINED CMAKE_TOOLCHAIN_FILE AND EXISTS ${CMAKE_TOOLCHAIN_FILE})
    message(STATUS "Using toolchain: ${CMAKE_TOOLCHAIN_FILE}")
    message(STATUS "")
else()
    message(WARNING "vcpkg toolchain not detected. Please set CMAKE_TOOLCHAIN_FILE to use vcpkg-managed dependencies.")
    message(STATUS "Example: -DCMAKE_TOOLCHAIN_FILE=[vcpkg-root]/scripts/buildsystems/vcpkg.cmake")
    message(STATUS "")
endif()

# CMake keeps the detected system/processor in the build directory. Reusing a build directory with
# a different cross toolchain leaves CMAKE_SYSTEM_PROCESSOR at the old architecture, after which
# architecture-specific flags such as -mavx2 are passed to an aarch64 compiler. Detect this instead
# of reporting the compiler's "unsupported option" errors.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND DEFINED VCPKG_TARGET_TRIPLET)
    if(VCPKG_TARGET_TRIPLET MATCHES "^arm64" AND NOT CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|ARM64)$")
        message(FATAL_ERROR "Stale build directory: VCPKG_TARGET_TRIPLET=${VCPKG_TARGET_TRIPLET} but "
                            "CMAKE_SYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR}. Delete ${CMAKE_BINARY_DIR} "
                            "and configure again with the aarch64 toolchain.")
    elseif(VCPKG_TARGET_TRIPLET MATCHES "^riscv64" AND NOT CMAKE_SYSTEM_PROCESSOR MATCHES "^(riscv64|RISCV64)$")
        message(FATAL_ERROR "Stale build directory: VCPKG_TARGET_TRIPLET=${VCPKG_TARGET_TRIPLET} but "
                            "CMAKE_SYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR}. Delete ${CMAKE_BINARY_DIR} "
                            "and configure again with the riscv64 toolchain.")
    elseif(VCPKG_TARGET_TRIPLET MATCHES "^loongarch64" AND NOT CMAKE_SYSTEM_PROCESSOR MATCHES "^(loongarch64|LOONGARCH64)$")
        message(FATAL_ERROR "Stale build directory: VCPKG_TARGET_TRIPLET=${VCPKG_TARGET_TRIPLET} but "
                            "CMAKE_SYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR}. Delete ${CMAKE_BINARY_DIR} "
                            "and configure again with the loongarch64 toolchain.")
    endif()
endif()

if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|ARM64)$")
    set(NEXUSFORCE_ARCH "aarch64")
    if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
        set(NEXUSFORCE_VCPKG_TRIPLET "arm64-freebsd" CACHE STRING "vcpkg triplet for arm64")
    elseif(APPLE)
        set(NEXUSFORCE_VCPKG_TRIPLET "arm64-osx" CACHE STRING "vcpkg triplet for arm64")
    else()
        set(NEXUSFORCE_VCPKG_TRIPLET "arm64-linux" CACHE STRING "vcpkg triplet for arm64")
    endif()
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(riscv64|RISCV64)$")
    set(NEXUSFORCE_ARCH "riscv64")
    set(NEXUSFORCE_VCPKG_TRIPLET "riscv64-linux" CACHE STRING "vcpkg triplet for riscv64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(loongarch64|LOONGARCH64)$")
    set(NEXUSFORCE_ARCH "loongarch64")
    set(NEXUSFORCE_VCPKG_TRIPLET "loongarch64-linux" CACHE STRING "vcpkg triplet for loongarch64")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(NEXUSFORCE_ARCH "x64")
    if(WIN32)
        set(NEXUSFORCE_VCPKG_TRIPLET "x64-windows" CACHE STRING "vcpkg triplet for x64")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
        set(NEXUSFORCE_VCPKG_TRIPLET "x64-freebsd" CACHE STRING "vcpkg triplet for x64")
    elseif(APPLE)
        set(NEXUSFORCE_VCPKG_TRIPLET "x64-osx" CACHE STRING "vcpkg triplet for x64")
    else()
        set(NEXUSFORCE_VCPKG_TRIPLET "x64-linux" CACHE STRING "vcpkg triplet for x64")
    endif()
else()
    message(FATAL_ERROR "NexusForce: unsupported target architecture")
endif()

include(cmake/NexusForceSanitizer.cmake)
include(cmake/NexusForceCompilerOptions.cmake)

string(JSON CONFIG_SIMD_TYPE ERROR_VARIABLE _config_simd_type_error TYPE ${JSON_CONTENT} simd)
if(_config_simd_type_error)
    message(FATAL_ERROR "config.json: missing \"simd\" entry")
elseif(CONFIG_SIMD_TYPE STREQUAL "STRING")
    string(JSON CONFIG_SIMD_LEVEL GET ${JSON_CONTENT} simd)
elseif(CONFIG_SIMD_TYPE STREQUAL "OBJECT")
    string(JSON CONFIG_SIMD_LEVEL ERROR_VARIABLE _config_simd_arch_error GET ${JSON_CONTENT} simd ${NEXUSFORCE_ARCH})
    if(_config_simd_arch_error)
        message(FATAL_ERROR "config.json: \"simd\" has no entry for architecture \"${NEXUSFORCE_ARCH}\"")
    endif()
else()
    message(FATAL_ERROR "config.json: \"simd\" must be a string or a per-architecture object")
endif()

set(VCPKG_TARGET_TRIPLET ${NEXUSFORCE_VCPKG_TRIPLET} CACHE STRING "vcpkg triplet")

set(CMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY OFF)
set(CMAKE_FIND_PACKAGE_NO_SYSTEM_PACKAGE_REGISTRY ON)
set(CMAKE_FIND_USE_PACKAGE_REGISTRY OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


set(NEXUSFORCE_VALID_STANDARDS 20 17 14)


if(CONFIG_CPP_STANDARD IN_LIST NEXUSFORCE_VALID_STANDARDS)
    set(CMAKE_CXX_STANDARD ${CONFIG_CPP_STANDARD})
endif()

if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20 CACHE STRING "C++ standard 20 used by NexusForce in default")
elseif(CMAKE_CXX_STANDARD LESS 14)
    message(FATAL_ERROR "NexusForce can only be compiled with C++ standard 14 or higher standard")
endif()

if(CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES
            "Debug;Release;RelWithDebInfo;MinSizeRel"
            CACHE STRING "Configurations for NexusForce" FORCE
    )
endif()


option(NEXUSFORCE_ENABLE_POSTGRESQL "Enable PostgreSQL database support" ${CONFIG_ENABLE_POSTGRESQL})
option(NEXUSFORCE_ENABLE_MYSQL      "Enable MySQL database support"      ${CONFIG_ENABLE_MYSQL})
option(NEXUSFORCE_ENABLE_SQLITE3    "Enable SQLite3 database support"    ${CONFIG_ENABLE_SQLITE3})
option(NEXUSFORCE_ENABLE_REDIS      "Enable Redis database support"      ${CONFIG_ENABLE_HIREDIS})
option(NEXUSFORCE_ENABLE_ZLIB       "Enable zlib compression support"    ${CONFIG_ENABLE_ZLIB})
option(NEXUSFORCE_ENABLE_LZ4        "Enable lz4 compression support"     ${CONFIG_ENABLE_LZ4})
option(NEXUSFORCE_ENABLE_ZSTD       "Enable zstd compression support"    ${CONFIG_ENABLE_ZSTD})
option(NEXUSFORCE_ENABLE_ICU        "Enable ICU4C locale support"        ${CONFIG_ENABLE_ICU})
option(NEXUSFORCE_ENABLE_LOCALE     "Build the locale component"         ${CONFIG_ENABLE_LOCALE})

if(NOT NEXUSFORCE_ENABLE_LOCALE AND NEXUSFORCE_ENABLE_ICU)
    set(NEXUSFORCE_ENABLE_ICU OFF)
endif()

option(NEXUSFORCE_USING_SSO         "Using basic_string SSO"                   ${CONFIG_USING_SSO})
option(NEXUSFORCE_USING_INTEL_TSX   "Using Intel TSX"                          ${CONFIG_USING_INTEL_TSX})
option(NEXUSFORCE_USING_SQLCIPHER   "Using SQLCipher"                          ${CONFIG_USING_SQLCIPHER})
option(NEXUSFORCE_USING_IO_URING    "Using io uring"                           ${CONFIG_USING_IO_URING})

# io_uring is a Linux host facility and is not available in cross sysroots. Linking the host
# liburing.so into a foreign-architecture library fails with "file in wrong format".
if(CMAKE_CROSSCOMPILING AND NEXUSFORCE_USING_IO_URING)
    set(NEXUSFORCE_USING_IO_URING OFF CACHE BOOL "Using io uring" FORCE)
    message(STATUS "io_uring disabled for cross-compilation")
endif()

option(NEXUSFORCE_USING_AES_NI      "Using AES-NI / AES crypto"                ${CONFIG_USING_AES_NI})
option(NEXUSFORCE_USING_PCLMUL      "Using PCLMULQDQ / PMULL"                  ${CONFIG_USING_PCLMUL})
option(NEXUSFORCE_USING_VAES        "Using VAES / VPCLMULQDQ"                  ${CONFIG_USING_VAES})

# Apple x64 CPUs (including macos-*-intel CI runners) do not guarantee VAES.
if(APPLE AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64)$" AND NEXUSFORCE_USING_VAES)
    set(NEXUSFORCE_USING_VAES OFF CACHE BOOL "Using VAES / VPCLMULQDQ" FORCE)
    message(STATUS "VAES disabled on macOS x64 (Apple x64 CPUs do not guarantee VAES)")
endif()

option(NEXUSFORCE_BUILD_TESTS       "Build unit tests"                   ${CONFIG_BUILD_TESTS})
option(NEXUSFORCE_BUILD_EXAMPLES    "Build example programs"             ${CONFIG_BUILD_EXAMPLES})
option(NEXUSFORCE_BUILD_BENCHMARKS  "Build benchmark programs"           ${CONFIG_BUILD_BENCHMARKS})
option(NEXUSFORCE_BUILD_DOCS        "Build documentation with Doxygen"   ${CONFIG_BUILD_DOCS})


option(NEXUSFORCE_USING_MEMORY_POOL
        "Using memory pool as the allocator of library containers"            ${CONFIG_USING_MEMORY_POOL})
option(NEXUSFORCE_MEMORY_POOL_GLOBAL_OVERRIDE
        "Replace the process wide operator new / delete with the memory pool" ${CONFIG_MEMORY_POOL_OVERRIDE})

option(NEXUSFORCE_OVERRIDE_TEST_WITH_SHARED_GTEST
        "Build the allocator override test against a shared GTest (rejected by the Windows debug heap)" OFF)


# NEXUSFORCE_SIMD_LEVEL is cached and architecture-specific (AVX2 for x64, BASELINE/SVE for
# aarch64). Reusing a build directory after an architecture switch must not carry the old level
# over; drop it once the detected architecture actually changes.
if(DEFINED NEXUSFORCE_LAST_ARCH AND NOT NEXUSFORCE_LAST_ARCH STREQUAL NEXUSFORCE_ARCH)
    unset(NEXUSFORCE_SIMD_LEVEL CACHE)
endif()
set(NEXUSFORCE_LAST_ARCH "${NEXUSFORCE_ARCH}" CACHE INTERNAL
    "architecture used when the cached SIMD level was last validated")

if(DEFINED CONFIG_SIMD_LEVEL)
    set(NEXUSFORCE_SIMD_LEVEL_DEFAULT "${CONFIG_SIMD_LEVEL}")
else()
    set(NEXUSFORCE_SIMD_LEVEL_DEFAULT "AUTO")
endif()

# FORCE keeps config.json authoritative; a command-line
# -DNEXUSFORCE_SIMD_LEVEL=... is therefore not honored.
set(NEXUSFORCE_SIMD_LEVEL "${NEXUSFORCE_SIMD_LEVEL_DEFAULT}" CACHE STRING "SIMD level for ${NEXUSFORCE_ARCH}" FORCE)
_nexusforce_arch_levels(_simd_levels ${NEXUSFORCE_ARCH})
set_property(CACHE NEXUSFORCE_SIMD_LEVEL PROPERTY STRINGS ${_simd_levels})
if(NOT NEXUSFORCE_SIMD_LEVEL IN_LIST _simd_levels)
    list(JOIN _simd_levels ", " _simd_levels_joined)
    message(FATAL_ERROR
            "NEXUSFORCE_SIMD_LEVEL='${NEXUSFORCE_SIMD_LEVEL}' is not valid for ${NEXUSFORCE_ARCH}. "
            "Valid levels: ${_simd_levels_joined}")
endif()


option(NEXUSFORCE_FORMAT            "Enable clang-format code formatting"   ON)
option(NEXUSFORCE_CLANG_TIDY        "Enable clang-tidy static analysis"     ON)

option(NEXUSFORCE_ENABLE_ASAN       "Build with AddressSanitizer"           OFF)
option(NEXUSFORCE_ENABLE_UBSAN      "Build with UndefinedBehaviorSanitizer" OFF)
option(NEXUSFORCE_ENABLE_TSAN       "Build with ThreadSanitizer"            OFF)

_nexusforce_validate_sanitizers()
_nexusforce_sanitizer_summary(NEXUSFORCE_SANITIZER_SUMMARY)

# AddressSanitizer and ThreadSanitizer install their own operator new / delete,
# and a second replacement in the same process makes both of them unreliable.
# The pool itself stays available, only the allocator substitution is dropped,
# so the sanitizer workflows keep working as documented.
if((NEXUSFORCE_USING_MEMORY_POOL OR NEXUSFORCE_MEMORY_POOL_GLOBAL_OVERRIDE) AND
   (NEXUSFORCE_ENABLE_ASAN OR NEXUSFORCE_ENABLE_TSAN))
    message(WARNING
            "NEXUSFORCE_ENABLE_ASAN / NEXUSFORCE_ENABLE_TSAN own the allocation path; disabling "
            "NEXUSFORCE_USING_MEMORY_POOL and NEXUSFORCE_MEMORY_POOL_GLOBAL_OVERRIDE for this build "
            "(memory_pool itself stays available).")
    set(NEXUSFORCE_USING_MEMORY_POOL OFF)
    set(NEXUSFORCE_MEMORY_POOL_GLOBAL_OVERRIDE OFF)
endif()


message(STATUS "Project Configuration:")
message(STATUS "")
message(STATUS "Current Architecture: ${NEXUSFORCE_ARCH}")
message(STATUS "C++ Standard Version: ${CMAKE_CXX_STANDARD}")
message(STATUS "vcpkg triplet: ${VCPKG_TARGET_TRIPLET}")
message(STATUS "")
message(STATUS "  NEXUSFORCE_ENABLE_POSTGRESQL:   ${NEXUSFORCE_ENABLE_POSTGRESQL}")
message(STATUS "  NEXUSFORCE_ENABLE_MYSQL:        ${NEXUSFORCE_ENABLE_MYSQL}")
message(STATUS "  NEXUSFORCE_ENABLE_SQLITE3:      ${NEXUSFORCE_ENABLE_SQLITE3}")
message(STATUS "  NEXUSFORCE_ENABLE_REDIS:        ${NEXUSFORCE_ENABLE_REDIS}")
message(STATUS "  NEXUSFORCE_ENABLE_ZLIB:         ${NEXUSFORCE_ENABLE_ZLIB}")
message(STATUS "  NEXUSFORCE_ENABLE_LZ4:          ${NEXUSFORCE_ENABLE_LZ4}")
message(STATUS "  NEXUSFORCE_ENABLE_ZSTD:         ${NEXUSFORCE_ENABLE_ZSTD}")
message(STATUS "  NEXUSFORCE_ENABLE_LOCALE:       ${NEXUSFORCE_ENABLE_LOCALE}")
message(STATUS "  NEXUSFORCE_ENABLE_ICU:          ${NEXUSFORCE_ENABLE_ICU}")
message(STATUS "  NEXUSFORCE_ENABLE_OPENSSL:      ON (NECESSARY)")
message(STATUS "  NEXUSFORCE_ENABLE_PCRE2:        ON (NECESSARY)")
message(STATUS "")
message(STATUS "  NEXUSFORCE_USING_SSO:           ${NEXUSFORCE_USING_SSO}")
message(STATUS "  NEXUSFORCE_USING_INTEL_TSX:     ${NEXUSFORCE_USING_INTEL_TSX}")
message(STATUS "  NEXUSFORCE_USING_SQLCIPHER:     ${NEXUSFORCE_USING_SQLCIPHER}")
message(STATUS "  NEXUSFORCE_USING_IO_URING:      ${NEXUSFORCE_USING_IO_URING}")
message(STATUS "  NEXUSFORCE_USING_AES_NI:        ${NEXUSFORCE_USING_AES_NI}")
message(STATUS "  NEXUSFORCE_USING_PCLMUL:        ${NEXUSFORCE_USING_PCLMUL}")
message(STATUS "  NEXUSFORCE_USING_VAES:          ${NEXUSFORCE_USING_VAES}")
message(STATUS "")
message(STATUS "  NEXUSFORCE_USING_MEMORY_POOL:           ${NEXUSFORCE_USING_MEMORY_POOL}")
message(STATUS "  NEXUSFORCE_MEMORY_POOL_GLOBAL_OVERRIDE: ${NEXUSFORCE_MEMORY_POOL_GLOBAL_OVERRIDE}")
message(STATUS "")
message(STATUS "  NEXUSFORCE_BUILD_TESTS:         ${NEXUSFORCE_BUILD_TESTS}")
message(STATUS "  NEXUSFORCE_BUILD_EXAMPLES:      ${NEXUSFORCE_BUILD_EXAMPLES}")
message(STATUS "  NEXUSFORCE_BUILD_BENCHMARKS:    ${NEXUSFORCE_BUILD_BENCHMARKS}")
message(STATUS "  NEXUSFORCE_BUILD_DOCS:          ${NEXUSFORCE_BUILD_DOCS}")
message(STATUS "")
message(STATUS "  NEXUSFORCE_SIMD_LEVEL:          ${NEXUSFORCE_SIMD_LEVEL}")
message(STATUS "")
message(STATUS "  NEXUSFORCE_ENABLE_ASAN:         ${NEXUSFORCE_ENABLE_ASAN}")
message(STATUS "  NEXUSFORCE_ENABLE_UBSAN:        ${NEXUSFORCE_ENABLE_UBSAN}")
message(STATUS "  NEXUSFORCE_ENABLE_TSAN:         ${NEXUSFORCE_ENABLE_TSAN}")
message(STATUS "  Sanitizers active:             ${NEXUSFORCE_SANITIZER_SUMMARY}")
message(STATUS "")


# Unified output directories. $<CONFIG> ensures per-config subdirectories
# (bin/Debug/, bin/Release/, etc.) on both multi-config generators (MSVC,
# Xcode) and single-config generators (Ninja, Makefiles).
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY  "${CMAKE_BINARY_DIR}/lib/$<CONFIG>")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY  "${CMAKE_BINARY_DIR}/lib/$<CONFIG>")


set(NEXUSFORCE_SQLCIPHER_LINKED FALSE)
set(NEXUSFORCE_URING_LINKED FALSE)
set(NEXUSFORCE_ICU_LINKED FALSE)


include(cmake/NexusForceReflectScanner.cmake)
include(cmake/NexusForceRuntime.cmake)
include(cmake/NexusForceInstallRuntime.cmake)
include(cmake/NexusForceClangFormat.cmake)
include(cmake/NexusForceClangTidy.cmake)
include(cmake/NexusForceClean.cmake)


# Reusing one build directory for two vcpkg triplets leaves package discovery caches pointing into
# the previous triplet's vcpkg_installed tree. CMake trusts cached absolute library paths and then
# creates imported targets whose locations do not apply to the active configuration. Drop only
# cache entries whose value points into a different triplet directory.
if(DEFINED VCPKG_TARGET_TRIPLET)
    if(DEFINED NEXUSFORCE_LAST_VCPKG_TARGET_TRIPLET)
        set(_nf_last_vcpkg_triplet "${NEXUSFORCE_LAST_VCPKG_TARGET_TRIPLET}")
    else()
        set(_nf_last_vcpkg_triplet "")
    endif()

    if(NOT _nf_last_vcpkg_triplet STREQUAL VCPKG_TARGET_TRIPLET)
        get_cmake_property(_nf_cache_vars CACHE_VARIABLES)
        foreach(_nf_var IN LISTS _nf_cache_vars)
            if(DEFINED ${_nf_var} AND "${${_nf_var}}" MATCHES "/vcpkg_installed/([^/]+)/")
                if(NOT CMAKE_MATCH_1 STREQUAL VCPKG_TARGET_TRIPLET)
                    unset(${_nf_var} CACHE)
                endif()
            endif()
        endforeach()
        set(NEXUSFORCE_LAST_VCPKG_TARGET_TRIPLET "${VCPKG_TARGET_TRIPLET}" CACHE INTERNAL
            "vcpkg triplet used when package discovery caches were last normalized")
        unset(_nf_cache_vars)
        unset(_nf_var)
    endif()
    unset(_nf_last_vcpkg_triplet)
endif()

add_subdirectory(src)
add_subdirectory(tools)
add_subdirectory(tests)
add_subdirectory(examples)
add_subdirectory(benchmark)
add_subdirectory(docs)


install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/NeForce/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/NeForce
        FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h" PATTERN "*.asm"
)

set(INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/NexusForce)

write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/NexusForceConfigVersion.cmake
        VERSION ${NEXUSFORCE_VERSION}
        COMPATIBILITY SameMajorVersion
)


set(PACKAGE_VERSION_VARS
        NEXUSFORCE_VERSION
        NEXUSFORCE_ENABLE_POSTGRESQL
        NEXUSFORCE_ENABLE_MYSQL
        NEXUSFORCE_ENABLE_SQLITE3
        NEXUSFORCE_ENABLE_REDIS
        NEXUSFORCE_ENABLE_ZLIB
        NEXUSFORCE_ENABLE_LZ4
        NEXUSFORCE_ENABLE_ZSTD
        NEXUSFORCE_ENABLE_ICU
        NEXUSFORCE_ENABLE_LOCALE

        NEXUSFORCE_USING_SSO
        NEXUSFORCE_USING_INTEL_TSX
        NEXUSFORCE_USING_SQLCIPHER
        NEXUSFORCE_USING_IO_URING
        NEXUSFORCE_USING_AES_NI
        NEXUSFORCE_USING_PCLMUL
        NEXUSFORCE_USING_VAES

        NEXUSFORCE_SQLCIPHER_LINKED
        NEXUSFORCE_URING_LINKED
        NEXUSFORCE_ICU_LINKED
)

foreach(_var ${PACKAGE_VERSION_VARS})
    set(${_var} ${${_var}})
endforeach()

configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/NexusForceConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/NexusForceConfig.cmake
        INSTALL_DESTINATION ${INSTALL_CONFIGDIR}
        PATH_VARS CMAKE_INSTALL_BINDIR CMAKE_INSTALL_INCLUDEDIR CMAKE_INSTALL_LIBDIR CMAKE_INSTALL_DATADIR
        NO_SET_AND_CHECK_MACRO
        NO_CHECK_REQUIRED_COMPONENTS_MACRO
)


install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/NexusForceConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/NexusForceConfigVersion.cmake
        DESTINATION ${INSTALL_CONFIGDIR}
)

install(FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/NexusForceCompilerOptions.cmake
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/NexusForceSanitizer.cmake
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/NexusForceReflectScanner.cmake
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/NexusForceRuntime.cmake
        DESTINATION ${INSTALL_CONFIGDIR}
)

install(EXPORT NexusForceTargets
        FILE NexusForceTargets.cmake
        NAMESPACE NexusForce::
        DESTINATION ${INSTALL_CONFIGDIR}
)

nexusforce_install_runtime_dependencies(NexusForce)

install(FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/docs/ai/api_index.jsonl
        DESTINATION ${CMAKE_INSTALL_DATADIR}/ai
)

install(FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE
        ${CMAKE_CURRENT_SOURCE_DIR}/THIRD_PARTY_NOTICES.md
        DESTINATION ${CMAKE_INSTALL_DATADIR}/NexusForce
)
