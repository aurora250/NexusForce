function(nexusforce_compiler_options target)
    set(SIMD_FLAGS "")

    set(USE_MSVC_STYLE OFF)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        set(USE_MSVC_STYLE ON)
    endif()

    if(NEXUSFORCE_ARCH STREQUAL "x64")
        if(USE_MSVC_STYLE)
            if(NEXUSFORCE_SIMD_LEVEL STREQUAL "AVX512")
                set(SIMD_FLAGS "/arch:AVX512")
            elseif(NEXUSFORCE_SIMD_LEVEL STREQUAL "AVX2")
                set(SIMD_FLAGS "/arch:AVX2")
            elseif(NEXUSFORCE_SIMD_LEVEL STREQUAL "AVX")
                set(SIMD_FLAGS "/arch:AVX")
            elseif(NEXUSFORCE_SIMD_LEVEL STREQUAL "SSE2")
                set(SIMD_FLAGS "")
            else()
                set(SIMD_FLAGS "")
            endif()
        else()
            if(NEXUSFORCE_SIMD_LEVEL STREQUAL "AVX512")
                set(SIMD_FLAGS "-mavx512f -mavx512cd -mavx512bw -mavx512dq")
            elseif(NEXUSFORCE_SIMD_LEVEL STREQUAL "AVX2")
                set(SIMD_FLAGS "-mavx2")
            elseif(NEXUSFORCE_SIMD_LEVEL STREQUAL "AVX")
                set(SIMD_FLAGS "-mavx")
            elseif(NEXUSFORCE_SIMD_LEVEL STREQUAL "SSE2")
                set(SIMD_FLAGS "-msse2")
            else()
                set(SIMD_FLAGS "-march=native")
            endif()
        endif()
    endif()

    if(USE_MSVC_STYLE)
        target_compile_options(${target} PRIVATE
                $<$<COMPILE_LANGUAGE:CXX>:
                /utf-8 /wd4819
                ${SIMD_FLAGS}
                $<$<CONFIG:Debug>:/WX /Od /Zi /MDd /bigobj>
                $<$<CONFIG:Release>:/O2 /Zc:__cplusplus /MD>
                >
        )
        target_compile_options(${target} PRIVATE
                $<$<COMPILE_LANGUAGE:ASM_MASM>:/W3>
        )

        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
        add_compile_options(
                "$<$<CONFIG:Debug>:/MDd>"
                "$<$<NOT:$<CONFIG:Debug>>:/MD>"
        )

        message(STATUS "Configured MSVC compiler options for ${target} (${CMAKE_BUILD_TYPE})")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE
                -finput-charset=UTF-8 -fexec-charset=UTF-8
                -Wno-unused-result -Wno-psabi
                ${SIMD_FLAGS}
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

        if(WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            target_compile_definitions(${target} PRIVATE
                    $<$<CONFIG:Debug>:_DEBUG>
                    _MT
                    _DLL
            )
        endif()

        message(STATUS "Configured GNU/Clang compiler options for ${target} (${CMAKE_BUILD_TYPE})")
    else()
        message(FATAL_ERROR "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}")
    endif()
endfunction()
