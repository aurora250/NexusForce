# NexusForceRuntime.cmake
# 提供 nexusforce_deploy_runtime() 函数，自动将 NexusForce 运行时库部署到目标构建目录。
#
# 用法:
#   find_package(NexusForce REQUIRED)
#   add_executable(my_app main.cpp)
#   target_link_libraries(my_app PRIVATE NexusForce::NexusForce)
#   nexusforce_deploy_runtime(my_app)
#
# 该函数在 Windows 上将 NexusForce.dll 复制到目标的输出目录，
# 确保构建后可直接运行（无需手动设置 PATH）。
# 在 Linux 上此函数为空操作（由 RPATH 机制处理）。

function(nexusforce_deploy_runtime target)
    if(NOT TARGET ${target})
        message(FATAL_ERROR "nexusforce_deploy_runtime: target '${target}' not found")
    endif()

    if(WIN32)
        if(NOT TARGET NexusForce::NexusForce)
            message(FATAL_ERROR "nexusforce_deploy_runtime: NexusForce::NexusForce target not found. "
                                "Ensure find_package(NexusForce) has been called.")
        endif()

        get_target_property(__nf_type NexusForce::NexusForce TYPE)
        if(__nf_type STREQUAL "SHARED_LIBRARY" OR __nf_type STREQUAL "UNKNOWN")
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "$<TARGET_FILE:NexusForce::NexusForce>"
                    "$<TARGET_FILE_DIR:${target}>"
                COMMENT "Deploying NexusForce.dll for ${target}..."
                VERBATIM
            )
        endif()
    endif()
endfunction()
