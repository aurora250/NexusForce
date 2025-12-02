cmake_minimum_required(VERSION 3.17)
project(MSTL LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

if(NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES
            "Debug;Release;RelWithDebInfo;MinSizeRel")
    set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING
            "Configurations for MSTL" FORCE)
endif()

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

set(CMAKE_CXX_STANDARD 20)

if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ standard used by MSTL")
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_CXX_STANDARD LESS 14)
    message(FATAL_ERROR "MSTL can only be compiled with C++14 or higher standard")
endif()


set(CMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY OFF)
set(CMAKE_FIND_PACKAGE_NO_SYSTEM_PACKAGE_REGISTRY ON)
set(CMAKE_FIND_USE_PACKAGE_REGISTRY OFF)

if(DEFINED ENV{CONDA_PREFIX})
    message(STATUS "Conda environment detected, adjusting library search paths")
    set(CMAKE_IGNORE_PATH "$ENV{CONDA_PREFIX}/lib" "$ENV{CONDA_PREFIX}/include")
endif()


if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(MSTL_ARCH "x64")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(MSTL_ARCH "x86")
else()
    message(FATAL_ERROR "MSTL can only be compiled in X64 or X86 system")
endif()
message(STATUS "Current Architecture: ${MSTL_ARCH}")
message(STATUS "C++ Standard Version: ${CMAKE_CXX_STANDARD}")


option(MSTL_ENABLE_BOOST "Enable Boost library support" ON)
option(MSTL_ENABLE_POSTGRESQL "Enable PostgreSQL database support" ON)
option(MSTL_ENABLE_MYSQL "Enable MySQL database support" ON)
option(MSTL_ENABLE_SQLITE3 "Enable SQLite3 database support" ON)
option(MSTL_ENABLE_REDIS "Enable Redis database support" ON)
option(MSTL_ENABLE_QT6 "Enable Qt6 toolchain support" OFF)
option(MSTL_ENABLE_CUDA "Enable CUDA GPU acceleration" OFF)
option(MSTL_BUILD_TESTS "Build unit tests (requires GTest)" ON)
option(MSTL_BUILD_EXAMPLES "Build example programs" OFF)


message(STATUS "Project Configuration:")
message(STATUS "  MSTL_ENABLE_BOOST:        ${MSTL_ENABLE_BOOST}")
message(STATUS "  MSTL_ENABLE_POSTGRESQL:   ${MSTL_ENABLE_POSTGRESQL}")
message(STATUS "  MSTL_ENABLE_MYSQL:        ${MSTL_ENABLE_MYSQL}")
message(STATUS "  MSTL_ENABLE_SQLITE3:      ${MSTL_ENABLE_SQLITE3}")
message(STATUS "  MSTL_ENABLE_REDIS:        ${MSTL_ENABLE_REDIS}")
message(STATUS "  MSTL_ENABLE_QT6:          ${MSTL_ENABLE_QT6}")
message(STATUS "  MSTL_ENABLE_CUDA:         ${MSTL_ENABLE_CUDA}")

message(STATUS "  MSTL_BUILD_TESTS:         ${MSTL_BUILD_TESTS}")
message(STATUS "  MSTL_BUILD_EXAMPLES:      ${MSTL_BUILD_EXAMPLES}")


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


install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/MSTL/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/MSTL
        FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h" PATTERN "*.asm"
)

set(INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/MSTL)

write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/MSTLConfigVersion.cmake
        VERSION 1.4.0
        COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/MSTLConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/MSTLConfig.cmake
        INSTALL_DESTINATION ${INSTALL_CONFIGDIR}
        PATH_VARS
            CMAKE_INSTALL_INCLUDEDIR
            CMAKE_INSTALL_LIBDIR
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
