# NexusForceReflectScanner.cmake
# 提供 nexusforce_reflect_scan() 函数，供下游用户便捷集成 NexusRsvp 代码生成。
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
#   1. 运行 NexusRsvp 扫描 HEADERS 目录下的 .hpp/.h 文件
#   2. 生成 _nexusrsvp_gen_<target>.cpp 反射注册文件
#   3. 将生成文件添加到目标的源文件列表中
#

set(NEXUSFORCE_HOST_NEXUSRSVP "" CACHE FILEPATH
    "Host NexusRsvp executable used for reflection generation when cross-compiling without an emulator")

function(nexusforce_reflect_scan)
    set(options "")
    set(oneValueArgs TARGET HEADERS OUTPUT)
    set(multiValueArgs EXCLUDES DEPENDS)
    cmake_parse_arguments(NEXUSRSVP "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT NEXUSRSVP_TARGET)
        message(FATAL_ERROR "nexusforce_reflect_scan: TARGET is required")
    endif()
    if(NOT NEXUSRSVP_HEADERS)
        message(FATAL_ERROR "nexusforce_reflect_scan: HEADERS is required")
    endif()

    if(TARGET NexusForce::NexusRsvp)
        set(__nexusrsvp_target NexusForce::NexusRsvp)
    elseif(TARGET NexusRsvp)
        set(__nexusrsvp_target NexusRsvp)
    else()
        message(FATAL_ERROR "nexusforce_reflect_scan: NexusRsvp target not found. "
                            "Ensure find_package(NexusForce) has been called.")
    endif()

    if(NOT NEXUSRSVP_OUTPUT)
        set(NEXUSRSVP_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/_nexusrsvp_gen_${NEXUSRSVP_TARGET}.cpp")
    endif()

    file(GLOB_RECURSE __nexusrsvp_header_deps
        "${NEXUSRSVP_HEADERS}/*.hpp"
        "${NEXUSRSVP_HEADERS}/*.h"
    )

    if(CMAKE_CROSSCOMPILING AND NOT CMAKE_CROSSCOMPILING_EMULATOR)
        if(NOT NEXUSFORCE_HOST_NEXUSRSVP AND DEFINED ENV{NEXUSFORCE_HOST_NEXUSRSVP})
            set(NEXUSFORCE_HOST_NEXUSRSVP "$ENV{NEXUSFORCE_HOST_NEXUSRSVP}" CACHE FILEPATH
                "Host NexusRsvp executable used for reflection generation when cross-compiling without an emulator"
                FORCE)
        endif()

        if(NEXUSFORCE_HOST_NEXUSRSVP)
            if(NOT EXISTS "${NEXUSFORCE_HOST_NEXUSRSVP}")
                message(FATAL_ERROR "nexusforce_reflect_scan: NEXUSFORCE_HOST_NEXUSRSVP points at a missing file: "
                                    "${NEXUSFORCE_HOST_NEXUSRSVP}")
            endif()
        else()
            file(GLOB __nexusrsvp_host_candidates
                "${CMAKE_SOURCE_DIR}/cmake-build-*/bin/*/NexusRsvp"
                "${CMAKE_SOURCE_DIR}/cmake-build-*/bin/*/NexusRsvp.exe"
                "${CMAKE_SOURCE_DIR}/build*/bin/*/NexusRsvp"
                "${CMAKE_SOURCE_DIR}/build*/bin/*/NexusRsvp.exe"
                "${CMAKE_SOURCE_DIR}/out/build/*/bin/*/NexusRsvp"
                "${CMAKE_SOURCE_DIR}/out/build/*/bin/*/NexusRsvp.exe"
            )
            foreach(__nexusrsvp_host_candidate IN LISTS __nexusrsvp_host_candidates)
                # The candidate must actually be executable on this host;
                # a binary of the target architecture is rejected by the loader and reports an error instead of an exit code.
                execute_process(COMMAND "${__nexusrsvp_host_candidate}" RESULT_VARIABLE __nexusrsvp_host_probe
                                OUTPUT_QUIET ERROR_QUIET)
                if(__nexusrsvp_host_probe MATCHES "^[0-9]+$")
                    set(NEXUSFORCE_HOST_NEXUSRSVP "${__nexusrsvp_host_candidate}" CACHE FILEPATH
                        "Host NexusRsvp executable used for reflection generation when cross-compiling without an emulator"
                        FORCE)
                    message(STATUS "nexusforce_reflect_scan: using host NexusRsvp ${NEXUSFORCE_HOST_NEXUSRSVP}")
                    break()
                endif()
            endforeach()
        endif()

        if(NOT NEXUSFORCE_HOST_NEXUSRSVP)
            message(FATAL_ERROR "nexusforce_reflect_scan: cross-compiling without an emulator needs a host NexusRsvp, and none "
                                "was found in any native build tree of ${CMAKE_SOURCE_DIR}. Build one once "
                                "(cmake --build <native build dir> --target NexusRsvp) or pass "
                                "-DNEXUSFORCE_HOST_NEXUSRSVP=<host NexusRsvp executable>.")
        endif()
        set(__nexusrsvp_dependency "${NEXUSFORCE_HOST_NEXUSRSVP}")
        set(__nexusrsvp_command
            "${NEXUSFORCE_HOST_NEXUSRSVP}"
            "${NEXUSRSVP_HEADERS}"
            -o "${NEXUSRSVP_OUTPUT}"
        )
    elseif(CMAKE_CROSSCOMPILING_EMULATOR)
        set(__nexusrsvp_dependency "$<TARGET_FILE:${__nexusrsvp_target}>")
        set(__nexusrsvp_command
            ${CMAKE_CROSSCOMPILING_EMULATOR}
            "$<TARGET_FILE:${__nexusrsvp_target}>"
            "${NEXUSRSVP_HEADERS}"
            -o "${NEXUSRSVP_OUTPUT}"
        )
    else()
        set(__nexusrsvp_dependency "$<TARGET_FILE:${__nexusrsvp_target}>")
        set(__nexusrsvp_command
            "$<TARGET_FILE:${__nexusrsvp_target}>"
            "${NEXUSRSVP_HEADERS}"
            -o "${NEXUSRSVP_OUTPUT}"
        )
    endif()
    foreach(__ex IN LISTS NEXUSRSVP_EXCLUDES)
        list(APPEND __nexusrsvp_command -e "${__ex}")
    endforeach()

    add_custom_command(
        OUTPUT "${NEXUSRSVP_OUTPUT}"
        COMMAND ${__nexusrsvp_command}
        DEPENDS
            ${__nexusrsvp_dependency}
            ${__nexusrsvp_header_deps}
            ${NEXUSRSVP_DEPENDS}
        COMMENT "Running NexusRsvp to generate reflection registration for ${NEXUSRSVP_TARGET}..."
        VERBATIM
    )

    target_sources(${NEXUSRSVP_TARGET} PRIVATE "${NEXUSRSVP_OUTPUT}")
endfunction()
