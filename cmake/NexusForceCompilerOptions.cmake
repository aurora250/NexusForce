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
                include(CheckCXXCompilerFlag)
                check_cxx_compiler_flag("/arch:AVX2" _nexusforce_has_msvc_avx2)
                check_cxx_compiler_flag("/arch:AVX" _nexusforce_has_msvc_avx)
                set(_msvc_level "${NEXUSFORCE_SIMD_LEVEL}")
                if(_msvc_level STREQUAL "AVX512")
                    check_cxx_compiler_flag("/arch:AVX512" _nexusforce_has_msvc_avx512)
                    if(NOT _nexusforce_has_msvc_avx512)
                        message(STATUS "NexusForce: MSVC does not support /arch:AVX512, fallback to /arch:AVX2")
                        set(_msvc_level "AVX2")
                    endif()
                endif()
                if(_msvc_level STREQUAL "AVX2" AND NOT _nexusforce_has_msvc_avx2)
                    message(STATUS "NexusForce: MSVC does not support /arch:AVX2, fallback to /arch:AVX")
                    set(_msvc_level "AVX")
                endif()
                if(_msvc_level STREQUAL "AVX" AND NOT _nexusforce_has_msvc_avx)
                    message(STATUS "NexusForce: MSVC does not support /arch:AVX, SIMD falls back to SSE2")
                    set(_msvc_level "NONE")
                endif()
                if(_msvc_level STREQUAL "AVX512")
                    list(APPEND _flags /arch:AVX512)
                elseif(_msvc_level STREQUAL "AVX2")
                    list(APPEND _flags /arch:AVX2)
                elseif(_msvc_level STREQUAL "AVX")
                    list(APPEND _flags /arch:AVX)
                endif()
            else()
                include(CheckCXXCompilerFlag)
                check_cxx_compiler_flag("-msse2" _nexusforce_has_sse2)
                check_cxx_compiler_flag("-mavx" _nexusforce_has_avx)
                check_cxx_compiler_flag("-mavx2" _nexusforce_has_avx2)
                check_cxx_compiler_flag("-mavx512f" _nexusforce_has_avx512f)
                check_cxx_compiler_flag("-mavx512cd" _nexusforce_has_avx512cd)
                check_cxx_compiler_flag("-mavx512bw" _nexusforce_has_avx512bw)
                check_cxx_compiler_flag("-mavx512dq" _nexusforce_has_avx512dq)
                set(_nexusforce_has_avx512 OFF)
                if(_nexusforce_has_avx512f AND _nexusforce_has_avx512cd AND _nexusforce_has_avx512bw AND
                   _nexusforce_has_avx512dq)
                    set(_nexusforce_has_avx512 ON)
                endif()

                set(_x64_level "${NEXUSFORCE_SIMD_LEVEL}")
                if(_x64_level STREQUAL "AUTO")
                    list(APPEND _flags -march=native)
                elseif(_x64_level STREQUAL "AVX512" AND NOT _nexusforce_has_avx512)
                    message(STATUS "NexusForce: compiler does not support AVX-512, fallback to AVX2")
                    set(_x64_level "AVX2")
                endif()
                if(_x64_level STREQUAL "AVX2" AND NOT _nexusforce_has_avx2)
                    message(STATUS "NexusForce: compiler does not support -mavx2, fallback to AVX")
                    set(_x64_level "AVX")
                endif()
                if(_x64_level STREQUAL "AVX" AND NOT _nexusforce_has_avx)
                    message(STATUS "NexusForce: compiler does not support -mavx, fallback to SSE2")
                    set(_x64_level "SSE2")
                endif()
                if(_x64_level STREQUAL "SSE2" AND NOT _nexusforce_has_sse2)
                    message(STATUS "NexusForce: compiler does not support -msse2, SIMD falls back to scalar")
                    set(_x64_level "NONE")
                endif()

                if(_x64_level STREQUAL "AVX512")
                    list(APPEND _flags -mavx512f -mavx512cd -mavx512bw -mavx512dq)
                elseif(_x64_level STREQUAL "AVX2")
                    list(APPEND _flags -mavx2)
                elseif(_x64_level STREQUAL "AVX")
                    list(APPEND _flags -mavx)
                elseif(_x64_level STREQUAL "SSE2")
                    list(APPEND _flags -msse2)
                endif()
            endif()
        elseif(arch STREQUAL "aarch64")
            include(CheckCXXCompilerFlag)
            if(NEXUSFORCE_SIMD_LEVEL STREQUAL "SVE")
                check_cxx_compiler_flag("-march=armv8.2-a+sve" _nexusforce_has_sve)
                if(_nexusforce_has_sve)
                    # sve requires an armv8.2-a base arch
                    set(_march_base "armv8.2-a")
                    set(_march_exts "sve")
                else()
                    message(STATUS "NexusForce: compiler does not support SVE, fallback to NEON")
                endif()
            elseif(NEXUSFORCE_SIMD_LEVEL STREQUAL "SVE2")
                check_cxx_compiler_flag("-march=armv8.2-a+sve2" _nexusforce_has_sve2)
                if(_nexusforce_has_sve2)
                    set(_march_base "armv8.2-a")
                    set(_march_exts "sve2")
                else()
                    check_cxx_compiler_flag("-march=armv8.2-a+sve" _nexusforce_has_sve2)
                    if(_nexusforce_has_sve2)
                        message(STATUS "NexusForce: compiler does not support SVE2, fallback to SVE")
                        set(_march_base "armv8.2-a")
                        set(_march_exts "sve")
                    else()
                        message(STATUS "NexusForce: compiler does not support SVE/SVE2, fallback to NEON")
                    endif()
                endif()
            endif()
            # BASELINE/AUTO: NEON is mandatory on AArch64; AUTO stays empty to
            # remain safe for cross-compilation.
        elseif(arch STREQUAL "riscv64")
            if(NEXUSFORCE_SIMD_LEVEL STREQUAL "RVV" AND NOT _use_msvc_style)
                # keep the toolchain's zihintpause extension (the last -march wins)
                include(CheckCXXCompilerFlag)
                check_cxx_compiler_flag("-march=rv64gcv_zihintpause" _nexusforce_has_rvv)
                if(_nexusforce_has_rvv)
                    list(APPEND _flags -march=rv64gcv_zihintpause)
                else()
                    message(STATUS "NexusForce: compiler does not support RVV, SIMD falls back to scalar")
                endif()
            endif()
        elseif(arch STREQUAL "loongarch64")
            if(NOT _use_msvc_style)
                include(CheckCXXCompilerFlag)
                if(NEXUSFORCE_SIMD_LEVEL STREQUAL "LASX")
                    check_cxx_compiler_flag("-mlasx" _nexusforce_has_mlasx)
                    if(_nexusforce_has_mlasx)
                        list(APPEND _flags -mlasx)
                    else()
                        message(STATUS "NexusForce: current LoongArch toolchain not support -mlasx, rollback to LSX or scalar")
                        check_cxx_compiler_flag("-mlsx" _nexusforce_has_mlsx)
                        if(_nexusforce_has_mlsx)
                            list(APPEND _flags -mlsx)
                        endif()
                    endif()
                elseif(NEXUSFORCE_SIMD_LEVEL STREQUAL "LSX")
                    check_cxx_compiler_flag("-mlsx" _nexusforce_has_mlsx)
                    if(_nexusforce_has_mlsx)
                        list(APPEND _flags -mlsx)
                    else()
                        message(STATUS "NexusForce: current LoongArch toolchain not support -mlsx, rollback to scalar")
                    endif()
                endif()
            endif()
        endif()
    endif()

    if(DEFINED NEXUSFORCE_VECTOR_BYTES AND NOT NEXUSFORCE_VECTOR_BYTES STREQUAL "0" AND NOT _use_msvc_style)
        include(CheckCXXCompilerFlag)
        if(arch STREQUAL "aarch64")
            check_cxx_compiler_flag("-msve-vector-bits=${NEXUSFORCE_VECTOR_BYTES}" _nexusforce_has_sve_bits)
            if(_nexusforce_has_sve_bits)
                list(APPEND _flags "-msve-vector-bits=${NEXUSFORCE_VECTOR_BYTES}")
            endif()
        elseif(arch STREQUAL "riscv64")
            check_cxx_compiler_flag("-mrvv-vector-bits=${NEXUSFORCE_VECTOR_BYTES}" _nexusforce_has_rvv_bits)
            if(_nexusforce_has_rvv_bits)
                list(APPEND _flags "-mrvv-vector-bits=${NEXUSFORCE_VECTOR_BYTES}")
            endif()
        endif()
    endif()

    # AES / PCLMUL feature flags, orthogonal to the SIMD level.
    # MSVC-style needs no flags: the intrinsics are always available and the
    # header fallback gate is NEFORCE_USING_AES_NI / NEFORCE_USING_PCLMUL.
    # Apple x64 CPUs (including macos-*-intel CI runners) do not guarantee
    # VAES, so keep the instruction set off there and fall back to the 128-bit
    # AES-NI/PCLMUL kernel.
    set(_vaes_enabled "${NEXUSFORCE_USING_VAES}")
    if(APPLE AND arch STREQUAL "x64")
        set(_vaes_enabled OFF)
    endif()
    if(NOT _use_msvc_style)
        if(arch STREQUAL "x64")
            if(NEXUSFORCE_USING_AES_NI)
                list(APPEND _flags -maes)
            endif()
            if(NEXUSFORCE_USING_PCLMUL)
                list(APPEND _flags -mpclmul)
            endif()
            if(_vaes_enabled)
                list(APPEND _flags -mvaes -mvpclmulqdq)
            endif()
        elseif(arch STREQUAL "aarch64")
            if(NEXUSFORCE_USING_AES_ARM)
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

        if(NOT WIN32)
            target_compile_options(${target} PRIVATE -fvisibility=hidden)
        endif()

        if(CMAKE_SYSTEM_NAME STREQUAL "Linux" OR CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
            get_target_property(_nexusforce_link_type ${target} TYPE)
            if(_nexusforce_link_type STREQUAL "SHARED_LIBRARY" OR _nexusforce_link_type STREQUAL "MODULE_LIBRARY")
                target_link_options(${target} PRIVATE "LINKER:--exclude-libs,ALL")
            endif()
        endif()

        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            target_compile_options(${target} PRIVATE
                    # -Watomic-alignment to atomic_base in Clang X86:
                    # misaligned atomic operation may incur significant performance penalty;
                    # the expected alignment (8 bytes) exceeds the actual alignment (4 bytes)
                    -Wno-atomic-alignment
            )
        endif()

        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND
                CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 13 AND
                CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
            target_compile_options(${target} PRIVATE -Wno-stringop-overflow)
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

    if(COMMAND nexusforce_sanitizer_options)
        nexusforce_sanitizer_options(${target})
    endif()
endfunction()
