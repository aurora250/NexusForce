cmake_minimum_required(VERSION 3.21)


file(READ "config.json" JSON_CONTENT)


if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE PATH "vcpkg toolchain")
else()
    if(WIN32)
        string(JSON CONFIG_VCPKG_PATH GET ${JSON_CONTENT} vcpkg windows)
    else()
        string(JSON CONFIG_VCPKG_PATH GET ${JSON_CONTENT} vcpkg linux)
    endif()
    set(CMAKE_TOOLCHAIN_FILE "${CONFIG_VCPKG_PATH}/scripts/buildsystems/vcpkg.cmake")
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

string(JSON CONFIG_USING_SSO         GET ${JSON_CONTENT} feature sso)
string(JSON CONFIG_USING_INTEL_TSX   GET ${JSON_CONTENT} feature intel-tsx)
string(JSON CONFIG_USING_SQLCIPHER   GET ${JSON_CONTENT} feature sqlcipher)
string(JSON CONFIG_USING_IO_URING    GET ${JSON_CONTENT} feature io_uring)

string(JSON CONFIG_BUILD_TESTS       GET ${JSON_CONTENT} build tests)
string(JSON CONFIG_BUILD_EXAMPLES    GET ${JSON_CONTENT} build examples)
string(JSON CONFIG_BUILD_BENCHMARKS  GET ${JSON_CONTENT} build benchmarks)
string(JSON CONFIG_BUILD_DOCS        GET ${JSON_CONTENT} build docs)



set(NEXUSFORCE_VERSION_MAJOR 1)
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


include(GNUInstallDirs)
include(CMakePackageConfigHelpers)


set(VCPKG_MANIFEST_MODE ON)
set(VCPKG_MANIFEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(VCPKG_INSTALLED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed")
set(VCPKG_MANIFEST_INSTALL ON)

if(DEFINED CMAKE_TOOLCHAIN_FILE AND EXISTS ${CMAKE_TOOLCHAIN_FILE})
    message(STATUS "Using vcpkg toolchain: ${CMAKE_TOOLCHAIN_FILE}")
    message(STATUS "")
else()
    message(WARNING "vcpkg toolchain not detected. Please set CMAKE_TOOLCHAIN_FILE to use vcpkg-managed dependencies.")
    message(STATUS "Example: -DCMAKE_TOOLCHAIN_FILE=[vcpkg-root]/scripts/buildsystems/vcpkg.cmake")
    message(STATUS "")
endif()

if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|ARM64)$")
    set(NEXUSFORCE_ARCH "aarch64")
    set(NEXUSFORCE_VCPKG_TRIPLET "arm64-linux" CACHE STRING "vcpkg triplet for arm64")
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
    else()
        set(NEXUSFORCE_VCPKG_TRIPLET "x64-linux" CACHE STRING "vcpkg triplet for x64")
    endif()
else()
    message(FATAL_ERROR "NexusForce: unsupported target architecture")
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

option(NEXUSFORCE_USING_SSO         "Using basic_string SSO"             ${CONFIG_USING_SSO})
option(NEXUSFORCE_USING_INTEL_TSX   "Using Intel TSX"                    ${CONFIG_USING_INTEL_TSX})
option(NEXUSFORCE_USING_SQLCIPHER   "Using SQLCipher"                    ${CONFIG_USING_SQLCIPHER})
option(NEXUSFORCE_USING_IO_URING    "Using io uring"                     ${CONFIG_USING_IO_URING})

option(NEXUSFORCE_BUILD_TESTS       "Build unit tests"                   ${CONFIG_BUILD_TESTS})
option(NEXUSFORCE_BUILD_EXAMPLES    "Build example programs"             ${CONFIG_BUILD_EXAMPLES})
option(NEXUSFORCE_BUILD_BENCHMARKS  "Build benchmark programs"           ${CONFIG_BUILD_BENCHMARKS})
option(NEXUSFORCE_BUILD_DOCS        "Build documentation with Doxygen"   ${CONFIG_BUILD_DOCS})


option(NEXUSFORCE_FORMAT            "Enable clang-format code formatting"   ON)
option(NEXUSFORCE_CLANG_TIDY        "Enable clang-tidy static analysis"     ON)


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
message(STATUS "  NEXUSFORCE_ENABLE_OPENSSL:      ON (NECESSARY)")
message(STATUS "  NEXUSFORCE_ENABLE_PCRE2:        ON (NECESSARY)")
message(STATUS "  NEXUSFORCE_ENABLE_ICU:          ON (NECESSARY)")
message(STATUS "")
message(STATUS "  NEXUSFORCE_USING_SSO:           ${NEXUSFORCE_USING_SSO}")
message(STATUS "  NEXUSFORCE_USING_INTEL_TSX:     ${NEXUSFORCE_USING_INTEL_TSX}")
message(STATUS "  NEXUSFORCE_USING_SQLCIPHER:     ${NEXUSFORCE_USING_SQLCIPHER}")
message(STATUS "  NEXUSFORCE_USING_IO_URING:      ${NEXUSFORCE_USING_IO_URING}")
message(STATUS "")
message(STATUS "  NEXUSFORCE_BUILD_TESTS:         ${NEXUSFORCE_BUILD_TESTS}")
message(STATUS "  NEXUSFORCE_BUILD_EXAMPLES:      ${NEXUSFORCE_BUILD_EXAMPLES}")
message(STATUS "  NEXUSFORCE_BUILD_BENCHMARKS:    ${NEXUSFORCE_BUILD_BENCHMARKS}")
message(STATUS "  NEXUSFORCE_BUILD_DOCS:          ${NEXUSFORCE_BUILD_DOCS}")

message(STATUS "")


# Unified output directories. $<CONFIG> ensures per-config subdirectories
# (bin/Debug/, bin/Release/, etc.) on both multi-config generators (MSVC,
# Xcode) and single-config generators (Ninja, Makefiles).
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY  "${CMAKE_BINARY_DIR}/lib/$<CONFIG>")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY  "${CMAKE_BINARY_DIR}/lib/$<CONFIG>")


set(NEXUSFORCE_SQLCIPHER_LINKED FALSE)
set(NEXUSFORCE_URING_LINKED FALSE)


include(cmake/NexusForceReflectScanner.cmake)
include(cmake/NexusForceRuntime.cmake)
include(cmake/NexusForceClangFormat.cmake)
include(cmake/NexusForceClangTidy.cmake)
include(cmake/NexusForceClean.cmake)


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

        NEXUSFORCE_USING_SSO
        NEXUSFORCE_USING_INTEL_TSX
        NEXUSFORCE_USING_SQLCIPHER
        NEXUSFORCE_USING_IO_URING

        NEXUSFORCE_SQLCIPHER_LINKED
        NEXUSFORCE_URING_LINKED
)

foreach(_var ${PACKAGE_VERSION_VARS})
    set(${_var} ${${_var}})
endforeach()

configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/NexusForceConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/NexusForceConfig.cmake
        INSTALL_DESTINATION ${INSTALL_CONFIGDIR}
        PATH_VARS CMAKE_INSTALL_INCLUDEDIR CMAKE_INSTALL_LIBDIR
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
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/NexusForceReflectScanner.cmake
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/NexusForceRuntime.cmake
        DESTINATION ${INSTALL_CONFIGDIR}
)

install(EXPORT NexusForceTargets
        FILE NexusForceTargets.cmake
        NAMESPACE NexusForce::
        DESTINATION ${INSTALL_CONFIGDIR}
)
