function(nexusforce_compiler_options target)
    if(MSVC)
        target_compile_options(${target} PRIVATE
                $<$<COMPILE_LANGUAGE:CXX>:
                /utf-8 /wd4819
                $<$<CONFIG:Debug>:/WX /Od /Zi /MDd /bigobj>
                $<$<CONFIG:Release>:/O2 /Zc:__cplusplus /MD>
                >
        )
        target_compile_options(${target} PRIVATE
                $<$<COMPILE_LANGUAGE:ASM_MASM>:
                /W3
                >
        )

        string(REPLACE "/showIncludes" "" CMAKE_DEPFILE_FLAGS_C "${CMAKE_DEPFILE_FLAGS_C}")
        string(REPLACE "/showIncludes" "" CMAKE_DEPFILE_FLAGS_CXX "${CMAKE_DEPFILE_FLAGS_CXX}")
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
        add_compile_options(
                "$<$<CONFIG:Debug>:/MDd>"
                "$<$<NOT:$<CONFIG:Debug>>:/MD>"
        )

        message(STATUS "Configured MSVC compiler options for ${target} (${CMAKE_BUILD_TYPE})")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE
                -finput-charset=UTF-8 -fexec-charset=UTF-8
                -Wno-unused-result
                $<$<CONFIG:Debug>:-Werror -O0 -g -gdwarf-4>
                $<$<CONFIG:Release>:-O2>
        )
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            target_compile_options(${target} PRIVATE
                    # -Watomic-alignment to atomic_base in Clang X86:
                    # misaligned atomic operation may incur significant performance penalty;
                    # the expected alignment (8 bytes) exceeds the actual alignment (4 bytes)
                    -Wno-atomic-alignment
            )
        endif()
        message(STATUS "Configured GNU/Clang compiler options for ${target} (${CMAKE_BUILD_TYPE})")
    else()
        message(FATAL_ERROR "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}")
    endif()
endfunction()
