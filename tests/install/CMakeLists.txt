# Third-party consumer of an installed NexusForce package: it is configured against a prefix
# produced by scripts/install_nexusforce.py (never against the build tree) and exercises the
# documented consumption surface end to end. CI copies this directory out of the repository, keeps
# the package prefix outside it as well, and passes NEXUSFORCE_EXPECTED_PREFIX so that resolving
# NexusForce from anywhere but the installed package fails the build:
#
#   find_package(NexusForce)              the exported targets and variables
#   NexusForce::NexusForce                include directories, ABI definitions, dependencies
#   nexusforce_compiler_options()         the project's own compiler options
#   nexusforce_sanitizer_options()        sanitizer flags (a no-op unless the consumer enables them)
#   nexusforce_deploy_runtime()           runtime deployment next to the executable (Windows)
#   nexusforce_reflect_scan()             NFRS code generation, which also proves NexusForce::NFRS
#                                         is installed and runnable in every configuration
#
# main.cpp then verifies at runtime that the generated reflection code was compiled, linked and
# registered. CI runs this from .github/workflows/install.yml.
cmake_minimum_required(VERSION 3.20)

project(NexusForceInstallConsumer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(NexusForce REQUIRED)

if(NEXUSFORCE_EXPECTED_PREFIX)
    file(TO_CMAKE_PATH "${NEXUSFORCE_EXPECTED_PREFIX}" _expected_prefix)
    file(TO_CMAKE_PATH "${NexusForce_DIR}" _found_dir)
    string(REGEX REPLACE "/+$" "" _expected_prefix "${_expected_prefix}")
    if(WIN32)
        string(TOLOWER "${_expected_prefix}" _expected_prefix)
        string(TOLOWER "${_found_dir}" _found_dir)
    endif()

    if(NOT _found_dir MATCHES "^${_expected_prefix}/")
        message(FATAL_ERROR
                "NexusForce was found at ${NexusForce_DIR}, which is outside the expected prefix "
                "${NEXUSFORCE_EXPECTED_PREFIX}: the consumer would be linking something other than "
                "the installed package")
    endif()
    message(STATUS "using the installed package from ${NexusForce_DIR}")
endif()

foreach(_required_target IN ITEMS NexusForce::NexusForce NexusForce::NFRS)
    if(NOT TARGET ${_required_target})
        message(FATAL_ERROR "installed package does not provide the target ${_required_target}")
    endif()
endforeach()

if(NOT NEXUSFORCE_VERSION)
    message(FATAL_ERROR "installed package does not report NEXUSFORCE_VERSION")
endif()

if(NOT NEXUSFORCE_INCLUDE_DIRS OR NOT NEXUSFORCE_LIBRARIES)
    message(FATAL_ERROR "installed package does not report its legacy include/library variables")
endif()

if(NOT EXISTS "${NEXUSFORCE_AI_API_INDEX}")
    message(FATAL_ERROR "NEXUSFORCE_AI_API_INDEX does not point at an installed file: ${NEXUSFORCE_AI_API_INDEX}")
endif()

# The allocator mode is part of the ABI: library containers are allocated by the process wide memory pool
# inside the shared library, so a consumer compiled without the matching definition would free pooled blocks
# through the C runtime heap. The package has to forward the definition, not merely report the option.
if(NOT DEFINED NEXUSFORCE_USING_MEMORY_POOL)
    message(FATAL_ERROR "installed package does not report NEXUSFORCE_USING_MEMORY_POOL")
endif()

get_target_property(_nexusforce_interface_definitions NexusForce::NexusForce INTERFACE_COMPILE_DEFINITIONS)
if(NEXUSFORCE_USING_MEMORY_POOL AND NOT _nexusforce_interface_definitions MATCHES "NEFORCE_USING_MEMORY_POOL")
    message(FATAL_ERROR
            "the installed package reports the memory pool as enabled but does not forward "
            "NEFORCE_USING_MEMORY_POOL to consumers: consumer containers would use the C runtime heap "
            "while the library uses the pool")
endif()

add_executable(install_consumer main.cpp)

target_link_libraries(install_consumer PRIVATE NexusForce::NexusForce)
target_compile_definitions(install_consumer PRIVATE NEXUSFORCE_EXPECTED_VERSION="${NEXUSFORCE_VERSION}")

nexusforce_compiler_options(install_consumer)
nexusforce_sanitizer_options(install_consumer)
nexusforce_deploy_runtime(install_consumer)
nexusforce_reflect_scan(
        TARGET install_consumer
        HEADERS ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(install_consumer PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/bin/RelWithDebInfo
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/bin/MinSizeRel
)

message(STATUS "install consumer: NexusForce ${NEXUSFORCE_VERSION}, configuration ${CMAKE_BUILD_TYPE}")
