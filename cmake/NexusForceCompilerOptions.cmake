# Internal: detect the target architecture from CMake variables.
# Installed-package consumers do not define NEXUSFORCE_ARCH, so the helper
# must be self-contained.
function(_nexusforce_detect_arch out_var)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|ARM64)$")
        set(${out_var} "aarch64" PARENT_SCOPE)
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(riscv64|RISCV64)$")
        set(${out_var} "riscv64" PARENT_SCOPE)
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(loongarch64|LOONGARCH64)$")
        set(${out_var} "loongarch64" PARENT_SCOPE)
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(${out_var} "x64" PARENT_SCOPE)
    else()
        message(FATAL_ERROR "NexusForce: unsupported target architecture")
    endif()
endfunction()

# Internal: valid SIMD level names for an architecture.
# Single source of truth shared by the cache STRINGS property, early
# validation and flag selection.
function(_nexusforce_arch_levels out_var arch)
    if(arch STREQUAL "x64")
        set(_levels AUTO BASELINE SSE2 AVX AVX2 AVX512)
    elseif(arch STREQUAL "aarch64")
        set(_levels AUTO BASELINE SVE SVE2)
    elseif(arch STREQUAL "riscv64")
        set(_levels AUTO BASELINE RVV)
    elseif(arch STREQUAL "loongarch64")
        set(_levels AUTO BASELINE LSX LASX)
    else()
        message(FATAL_ERROR "NexusForce: unsupported architecture ${arch}")
    endif()
    set(${out_var} ${_levels} PARENT_SCOPE)
endfunction()

# Internal: compute SIMD and AES/PCLMUL feature flags for the given
# architecture. An undefined NEXUSFORCE_SIMD_LEVEL (installed-package
# consumers) yields no SIMD flags at all.
function(_nexusforce_select_simd_flags out_var arch)
    set(_flags "")

    # pending -march spec for aarch64, finalized after the crypto merge
    set(_march_base "")
    set(_march_exts "")

    set(_use_msvc_style OFF)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        set(_use_msvc_style ON)
    endif()

    if(DEFINED NEXUSFORCE_SIMD_LEVEL)
        _nexusforce_arch_levels(_levels ${arch})
        if(NOT NEXUSFORCE_SIMD_LEVEL IN_LIST _levels)
            list(JOIN _levels ", " _levels_joined)
            message(FATAL_ERROR
                    "NexusForce: NEXUSFORCE_SIMD_LEVEL='${NEXUSFORCE_SIMD_LEVEL}' is not valid for ${arch}. "
                    "Valid levels: ${_levels_joined}")
        endif()

        if(arch STREQUAL "x64")
            if(_use_msvc_style)
                if(NEXUSFORCE_SIMD_LEVEL STREQUAL "AVX512")
                    list(APPEND _flags /arch:AVX512)
                elseif(NEXUSFORCE_SIMD_LEVEL STREQUAL "AVX2")
                    list(APPEND _flags /arch:AVX2)
                elseif(NEXUSFORCE_SIMD_LEVEL STREQUAL "AVX")
                    list(APPEND _flags /arch:AVX)
                endif()
            else()
                if(NEXUSFORCE_SIMD_LEVEL STREQUAL "AVX512")
                    list(APPEND _flags -mavx512f -mavx512cd -mavx512bw -mavx512dq)
                elseif(NEXUSFORCE_SIMD_LEVEL STREQUAL "AVX2")
                    list(APPEND _flags -mavx2)
                elseif(NEXUSFORCE_SIMD_LEVEL STREQUAL "AVX")
                    list(APPEND _flags -mavx)
                elseif(NEXUSFORCE_SIMD_LEVEL STREQUAL "SSE2")
                    list(APPEND _flags -msse2)
                elseif(NEXUSFORCE_SIMD_LEVEL STREQUAL "AUTO")
                    list(APPEND _flags -march=native)
                endif()
            endif()
        elseif(arch STREQUAL "aarch64")
            if(NEXUSFORCE_SIMD_LEVEL STREQUAL "SVE")
                # sve requires an armv8.2-a base arch
                set(_march_base "armv8.2-a")
                set(_march_exts "sve")
            elseif(NEXUSFORCE_SIMD_LEVEL STREQUAL "SVE2")
                set(_march_base "armv8.2-a")
                set(_march_exts "sve2")
            endif()
            # BASELINE/AUTO: NEON is mandatory on AArch64; AUTO stays empty to
            # remain safe for cross-compilation.
        elseif(arch STREQUAL "riscv64")
            if(NEXUSFORCE_SIMD_LEVEL STREQUAL "RVV" AND NOT _use_msvc_style)
                # keep the toolchain's zihintpause extension (the last -march wins)
                list(APPEND _flags -march=rv64gcv_zihintpause)
            endif()
        elseif(arch STREQUAL "loongarch64")
            if(NOT _use_msvc_style)
                if(NEXUSFORCE_SIMD_LEVEL STREQUAL "LASX")
                    list(APPEND _flags -mlasx)
                elseif(NEXUSFORCE_SIMD_LEVEL STREQUAL "LSX")
                    list(APPEND _flags -mlsx)
                endif()
            endif()
        endif()
    endif()

    # AES / PCLMUL feature flags, orthogonal to the SIMD level.
    # MSVC-style needs no flags: the intrinsics are always available and the
    # header fallback gate is NEFORCE_USING_AES_NI / NEFORCE_USING_PCLMUL.
    if(NOT _use_msvc_style)
        if(arch STREQUAL "x64")
            if(NEXUSFORCE_USING_AES_NI)
                list(APPEND _flags -maes)
            endif()
            if(NEXUSFORCE_USING_PCLMUL)
                list(APPEND _flags -mpclmul)
            endif()
        elseif(arch STREQUAL "aarch64")
            if(NEXUSFORCE_USING_AES_NI OR NEXUSFORCE_USING_PCLMUL)
                if(_march_base STREQUAL "")
                    set(_march_base "armv8-a")
                endif()
                list(APPEND _march_exts crypto)
            endif()
            if(NOT _march_base STREQUAL "")
                if(_march_exts)
                    list(JOIN _march_exts "+" _march_joined)
                    list(APPEND _flags "-march=${_march_base}+${_march_joined}")
                else()
                    list(APPEND _flags "-march=${_march_base}")
                endif()
            endif()
        endif()
        # riscv64/loongarch64: no AES/PCLMUL flag for now
    endif()

    set(${out_var} ${_flags} PARENT_SCOPE)
endfunction()

function(nexusforce_compiler_options target)
    if(DEFINED NEXUSFORCE_ARCH)
        set(_arch "${NEXUSFORCE_ARCH}")
    else()
        _nexusforce_detect_arch(_arch)
    endif()

    _nexusforce_select_simd_flags(SIMD_FLAGS ${_arch})

    set(USE_MSVC_STYLE OFF)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        set(USE_MSVC_STYLE ON)
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
