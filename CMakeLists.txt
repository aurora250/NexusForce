cmake_minimum_required(VERSION 3.17)

set(MSTL_VERSION_MAJOR 1)
set(MSTL_VERSION_MINOR 4)
set(MSTL_VERSION_PATCH 0)
set(MSTL_VERSION "${MSTL_VERSION_MAJOR}.${MSTL_VERSION_MINOR}.${MSTL_VERSION_PATCH}")

project(MSTL
        VERSION ${MSTL_VERSION}
        LANGUAGES CXX
)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

if(CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES
            "Debug;Release;RelWithDebInfo;MinSizeRel"
            CACHE STRING "Configurations for MSTL" FORCE
    )
endif()

set(CMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY OFF)
set(CMAKE_FIND_PACKAGE_NO_SYSTEM_PACKAGE_REGISTRY ON)
set(CMAKE_FIND_USE_PACKAGE_REGISTRY OFF)


set(CMAKE_CXX_STANDARD 20)

if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ standard used by MSTL")
elseif(CMAKE_CXX_STANDARD LESS 14)
    message(FATAL_ERROR "MSTL can only be compiled with C++14 or higher standard")
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(MSTL_ARCH "x64")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(MSTL_ARCH "x86")
else()
    message(FATAL_ERROR "MSTL can only be compiled in X64 or X86 system")
endif()

message(STATUS "Current Architecture: ${MSTL_ARCH}")
message(STATUS "C++ Standard Version: ${CMAKE_CXX_STANDARD}")


option(MSTL_ENABLE_POSTGRESQL "Enable PostgreSQL database support" ON)
option(MSTL_ENABLE_MYSQL      "Enable MySQL database support"      ON)
option(MSTL_ENABLE_SQLITE3    "Enable SQLite3 database support"    ON)
option(MSTL_ENABLE_REDIS      "Enable Redis database support"      ON)
option(MSTL_ENABLE_ZLIB       "Enable zlib compression support"    ON)
option(MSTL_ENABLE_OPENSSL    "Enable OpenSSL support"             ON)
option(MSTL_ENABLE_CUDA       "Enable CUDA GPU acceleration"       OFF)

option(MSTL_BUILD_TESTS       "Build unit tests"                   ON)
option(MSTL_BUILD_EXAMPLES    "Build example programs"             OFF)
option(MSTL_BUILD_DOCS        "Build documentation with Doxygen"   ON)


message(STATUS "Project Configuration:")
message(STATUS "  MSTL_ENABLE_POSTGRESQL:   ${MSTL_ENABLE_POSTGRESQL}")
message(STATUS "  MSTL_ENABLE_MYSQL:        ${MSTL_ENABLE_MYSQL}")
message(STATUS "  MSTL_ENABLE_SQLITE3:      ${MSTL_ENABLE_SQLITE3}")
message(STATUS "  MSTL_ENABLE_REDIS:        ${MSTL_ENABLE_REDIS}")
message(STATUS "  MSTL_ENABLE_ZLIB:         ${MSTL_ENABLE_ZLIB}")
message(STATUS "  MSTL_ENABLE_OPENSSL:      ${MSTL_ENABLE_OPENSSL}")
message(STATUS "  MSTL_ENABLE_CUDA:         ${MSTL_ENABLE_CUDA}")

message(STATUS "  MSTL_BUILD_TESTS:         ${MSTL_BUILD_TESTS}")
message(STATUS "  MSTL_BUILD_EXAMPLES:      ${MSTL_BUILD_EXAMPLES}")
message(STATUS "  MSTL_BUILD_DOCS:          ${MSTL_BUILD_DOCS}")


add_subdirectory(src)

if(MSTL_BUILD_TESTS)
    add_subdirectory(tests)
else()
    message(STATUS "Skipping tests build (MSTL_BUILD_TESTS=OFF)")
endif()

if(MSTL_BUILD_EXAMPLES)
    add_subdirectory(examples)
else()
    message(STATUS "Skipping examples build (MSTL_BUILD_EXAMPLES=OFF)")
endif()

if(MSTL_BUILD_DOCS)
    add_subdirectory(docs)
else()
    message(STATUS "Skipping documentation build (MSTL_BUILD_DOCS=OFF)")
endif()


install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/MSTL/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/MSTL
        FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h" PATTERN "*.asm"
)

set(INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/MSTL)

write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/MSTLConfigVersion.cmake
        VERSION ${MSTL_VERSION}
        COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/MSTLConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/MSTLConfig.cmake
        INSTALL_DESTINATION ${INSTALL_CONFIGDIR}
        PATH_VARS
            CMAKE_INSTALL_INCLUDEDIR
            CMAKE_INSTALL_LIBDIR
        NO_SET_AND_CHECK_MACRO
        NO_CHECK_REQUIRED_COMPONENTS_MACRO
)


install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/MSTLConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/MSTLConfigVersion.cmake
        DESTINATION ${INSTALL_CONFIGDIR}
)

install(EXPORT MSTLTargets
        FILE MSTLTargets.cmake
        NAMESPACE MSTL::
        DESTINATION ${INSTALL_CONFIGDIR}
)
