# NexusForceReflectScanner.cmake
# 提供 nexusforce_reflect_scan() 函数，供下游用户便捷集成 NFRS 代码生成。
#
# 用法:
#   find_package(NexusForce REQUIRED)
#   add_executable(my_app main.cpp)
#   nexusforce_reflect_scan(
#       TARGET my_app
#       HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/include
#   )
#
# 该函数会:
#   1. 运行 NFRS 扫描 HEADERS 目录下的 .hpp/.h 文件
#   2. 生成 _nfrs_gen_<target>.cpp 反射注册文件
#   3. 将生成文件添加到目标的源文件列表中
#

set(NEXUSFORCE_HOST_NFRS "" CACHE FILEPATH
    "Host NFRS executable used for reflection generation when cross-compiling without an emulator")

function(nexusforce_reflect_scan)
    set(options "")
    set(oneValueArgs TARGET HEADERS OUTPUT)
    set(multiValueArgs EXCLUDES DEPENDS)
    cmake_parse_arguments(NFRS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT NFRS_TARGET)
        message(FATAL_ERROR "nexusforce_reflect_scan: TARGET is required")
    endif()
    if(NOT NFRS_HEADERS)
        message(FATAL_ERROR "nexusforce_reflect_scan: HEADERS is required")
    endif()

    if(TARGET NexusForce::NFRS)
        set(__nfrs_target NexusForce::NFRS)
    elseif(TARGET NFRS)
        set(__nfrs_target NFRS)
    else()
        message(FATAL_ERROR "nexusforce_reflect_scan: NFRS target not found. "
                            "Ensure find_package(NexusForce) has been called.")
    endif()

    if(NOT NFRS_OUTPUT)
        set(NFRS_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/_nfrs_gen_${NFRS_TARGET}.cpp")
    endif()

    file(GLOB_RECURSE __nfrs_header_deps
        "${NFRS_HEADERS}/*.hpp"
        "${NFRS_HEADERS}/*.h"
    )

    if(CMAKE_CROSSCOMPILING AND NOT CMAKE_CROSSCOMPILING_EMULATOR)
        if(NOT NEXUSFORCE_HOST_NFRS)
            # Reuse an NFRS from any native build tree of this source tree; the scanner only
            # reads the headers, so a host binary built for any architecture works.
            file(GLOB __nfrs_host_candidates
                "${CMAKE_SOURCE_DIR}/cmake-build-*/bin/*/NFRS"
                "${CMAKE_SOURCE_DIR}/build*/bin/*/NFRS"
            )
            foreach(__nfrs_host_candidate IN LISTS __nfrs_host_candidates)
                # The candidate must actually be executable on this host; a FreeBSD or
                # otherwise incompatible binary reports an exec error instead of an exit code.
                execute_process(COMMAND "${__nfrs_host_candidate}" RESULT_VARIABLE __nfrs_host_probe
                                OUTPUT_QUIET ERROR_QUIET)
                if(__nfrs_host_probe MATCHES "^[0-9]+$")
                    set(NEXUSFORCE_HOST_NFRS "${__nfrs_host_candidate}" CACHE FILEPATH
                        "Host NFRS executable used for reflection generation when cross-compiling without an emulator"
                        FORCE)
                    message(STATUS "nexusforce_reflect_scan: using host NFRS ${NEXUSFORCE_HOST_NFRS}")
                    break()
                endif()
            endforeach()
        endif()
        if(NOT NEXUSFORCE_HOST_NFRS)
            message(FATAL_ERROR "nexusforce_reflect_scan: cross-compiling without an emulator needs a host NFRS. "
                                "Configure with -DNEXUSFORCE_HOST_NFRS=<host NFRS executable> "
                                "(for example build it once with a native configuration and target NFRS).")
        endif()
        set(__nfrs_dependency "${NEXUSFORCE_HOST_NFRS}")
        set(__nfrs_command
            "${NEXUSFORCE_HOST_NFRS}"
            "${NFRS_HEADERS}"
            -o "${NFRS_OUTPUT}"
        )
    elseif(CMAKE_CROSSCOMPILING_EMULATOR)
        set(__nfrs_dependency "$<TARGET_FILE:${__nfrs_target}>")
        set(__nfrs_command
            ${CMAKE_CROSSCOMPILING_EMULATOR}
            "$<TARGET_FILE:${__nfrs_target}>"
            "${NFRS_HEADERS}"
            -o "${NFRS_OUTPUT}"
        )
    else()
        set(__nfrs_dependency "$<TARGET_FILE:${__nfrs_target}>")
        set(__nfrs_command
            "$<TARGET_FILE:${__nfrs_target}>"
            "${NFRS_HEADERS}"
            -o "${NFRS_OUTPUT}"
        )
    endif()
    foreach(__ex IN LISTS NFRS_EXCLUDES)
        list(APPEND __nfrs_command -e "${__ex}")
    endforeach()

    add_custom_command(
        OUTPUT "${NFRS_OUTPUT}"
        COMMAND ${__nfrs_command}
        DEPENDS
            ${__nfrs_dependency}
            ${__nfrs_header_deps}
            ${NFRS_DEPENDS}
        COMMENT "Running NFRS to generate reflection registration for ${NFRS_TARGET}..."
        VERBATIM
    )

    target_sources(${NFRS_TARGET} PRIVATE "${NFRS_OUTPUT}")
endfunction()
