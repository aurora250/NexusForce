function(configure_compiler_options target)
    if(MSVC)
        target_compile_options(${target} PRIVATE
                $<$<COMPILE_LANGUAGE:CXX>:
                /utf-8
                $<$<CONFIG:Debug>:/WX /Od /Zi /MDd>
                $<$<CONFIG:Release>:/O2 /Zc:__cplusplus /MD>
                >
        )
        target_compile_options(${target} PRIVATE
                $<$<COMPILE_LANGUAGE:ASM_MASM>:
                /W3
                >
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

function(find_and_link_library target lib_name)
    set(options REQUIRED)
    set(oneValueArgs CONDITION)
    set(multiValueArgs COMPONENTS DEFINITIONS)
    cmake_parse_arguments(ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(DEFINED ARGS_CONDITION AND NOT ${ARGS_CONDITION})
        message(STATUS "${lib_name} support disabled by condition")
        return()
    endif()

    find_package(${lib_name} ${ARGS_COMPONENTS} QUIET)

    if(${lib_name}_FOUND)
        foreach(component IN LISTS ARGS_COMPONENTS)
            if(component STREQUAL "")
                target_link_libraries(${target} PUBLIC ${lib_name}::${lib_name})
            else()
                target_link_libraries(${target} PUBLIC ${lib_name}::${component})
            endif()
        endforeach()

        foreach(definition IN LISTS ARGS_DEFINITIONS)
            target_compile_definitions(${target} PUBLIC ${definition})
        endforeach()

        message(STATUS "${lib_name} found and linked successfully")
    else()
        if(ARGS_REQUIRED)
            message(FATAL_ERROR "${lib_name} is required but not found")
        else()
            message(WARNING "${lib_name} not found - related functionality will be disabled")
        endif()
    endif()
endfunction()

function(enable_database_support target)
    get_target_property(existing_defs ${target} COMPILE_DEFINITIONS)
    if(existing_defs)
        list(FIND existing_defs "MSTL_SUPPORT_DB__" db_support_index)
        if(db_support_index EQUAL -1)
            target_compile_definitions(${target} PUBLIC MSTL_SUPPORT_DB__)
        endif()
    else()
        target_compile_definitions(${target} PUBLIC MSTL_SUPPORT_DB__)
    endif()
endfunction()

function(find_and_link_openssl target)
    if(NOT MSTL_ENABLE_OPENSSL)
        message(STATUS "OpenSSL support disabled by option")
        return()
    endif()

    if(MSTL_ARCH STREQUAL "x64")
        find_package(OpenSSL QUIET COMPONENTS Crypto SSL)

        if(OpenSSL_FOUND)
            target_link_libraries(${target} PUBLIC OpenSSL::SSL OpenSSL::Crypto)
            target_compile_definitions(${target} PUBLIC MSTL_SUPPORT_OPENSSL__)
            message(STATUS "OpenSSL linked successfully")

            if(WIN32)
                find_file(OPENSSL_DLL_SSL
                        NAMES libssl-3-x64.dll libssl-3.dll libssl-1_1-x64.dll
                        PATHS
                        "${OpenSSL_ROOT_DIR}/bin"
                        "${OpenSSL_ROOT_DIR}"
                        "$ENV{OPENSSL_ROOT_DIR}/bin"
                        NO_DEFAULT_PATH
                )
                find_file(OPENSSL_DLL_CRYPTO
                        NAMES libcrypto-3-x64.dll libcrypto-3.dll libcrypto-1_1-x64.dll
                        PATHS
                        "${OpenSSL_ROOT_DIR}/bin"
                        "${OpenSSL_ROOT_DIR}"
                        "$ENV{OPENSSL_ROOT_DIR}/bin"
                        NO_DEFAULT_PATH
                )

                if(OPENSSL_DLL_SSL AND EXISTS ${OPENSSL_DLL_SSL})
                    add_custom_command(TARGET ${target} POST_BUILD
                            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                            ${OPENSSL_DLL_SSL}
                            $<TARGET_FILE_DIR:${target}>
                            COMMENT "Copying OpenSSL SSL DLL to output directory"
                    )
                endif()

                if(OPENSSL_DLL_CRYPTO AND EXISTS ${OPENSSL_DLL_CRYPTO})
                    add_custom_command(TARGET ${target} POST_BUILD
                            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                            ${OPENSSL_DLL_CRYPTO}
                            $<TARGET_FILE_DIR:${target}>
                            COMMENT "Copying OpenSSL Crypto DLL to output directory"
                    )
                endif()
            endif()
        else()
            message(WARNING "OpenSSL not found - OpenSSL components will be unavailable")
            message(STATUS "  Hint: On Linux, install openssl development package")
            message(STATUS "        On Windows, set OpenSSL_ROOT_DIR to your OpenSSL installation")
        endif()
    else()
        message(STATUS "OpenSSL support disabled: only x64 architecture supported")
    endif()
endfunction()

function(find_and_link_zlib target)
    if(NOT MSTL_ENABLE_ZLIB)
        message(STATUS "zlib support disabled by option")
        return()
    endif()

    if(MSTL_ARCH STREQUAL "x64")
    find_package(ZLIB QUIET)

    if(ZLIB_FOUND)
        target_link_libraries(${target} PUBLIC ZLIB::ZLIB)
        target_compile_definitions(${target} PUBLIC MSTL_SUPPORT_ZLIB__)
        message(STATUS "zlib linked successfully")

        if(WIN32)
            get_target_property(zlib_location ZLIB::ZLIB LOCATION)
            if(zlib_location AND EXISTS ${zlib_location})
                get_filename_component(zlib_dir ${zlib_location} DIRECTORY)
                find_file(ZLIB_DLL
                        NAMES zlib1.dll zlib.dll
                        PATHS
                        ${zlib_dir}
                        "${zlib_dir}/../bin"
                        "${zlib_dir}/../dll"
                        NO_DEFAULT_PATH
                )

                if(ZLIB_DLL AND EXISTS ${ZLIB_DLL})
                    add_custom_command(TARGET ${target} POST_BUILD
                            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                            ${ZLIB_DLL}
                            $<TARGET_FILE_DIR:${target}>
                            COMMENT "Copying zlib DLL to output directory"
                    )
                endif()
            endif()
        endif()
    else()
        message(STATUS "CMake find_package for zlib failed, trying manual search...")

        if(WIN32)
            set(ZLIB_ROOT_DIR "D:/zlib" CACHE PATH "zlib root directory")

            find_path(ZLIB_INCLUDE_DIR NAMES zlib.h
                    HINTS "${ZLIB_ROOT_DIR}/include"
                    PATHS
                    "$ENV{ZLIB_ROOT}/include"
                    "C:/zlib/include"
                    NO_DEFAULT_PATH
            )

            find_library(ZLIB_LIBRARY NAMES zlib zlibstatic zlibstaticd
                    HINTS "${ZLIB_ROOT_DIR}/lib"
                    PATHS
                    "$ENV{ZLIB_ROOT}/lib"
                    "C:/zlib/lib"
                    NO_DEFAULT_PATH
            )

            if(ZLIB_INCLUDE_DIR AND ZLIB_LIBRARY)
                target_include_directories(${target} PUBLIC ${ZLIB_INCLUDE_DIR})
                target_link_libraries(${target} PUBLIC ${ZLIB_LIBRARY})
                target_compile_definitions(${target} PUBLIC MSTL_SUPPORT_ZLIB__)
                message(STATUS "zlib linked manually successfully")

                find_file(ZLIB_DLL
                        NAMES zlib1.dll zlib.dll
                        HINTS "${ZLIB_ROOT_DIR}/bin"
                        "${ZLIB_ROOT_DIR}"
                        NO_DEFAULT_PATH
                )

                if(ZLIB_DLL AND EXISTS ${ZLIB_DLL})
                    add_custom_command(TARGET ${target} POST_BUILD
                            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                            ${ZLIB_DLL}
                            $<TARGET_FILE_DIR:${target}>
                            COMMENT "Copying zlib DLL to output directory"
                    )
                endif()
            else()
                message(WARNING "zlib not found - zlib components will be unavailable")
            endif()
        else()
            find_path(ZLIB_INCLUDE_DIR NAMES zlib.h
                    PATHS /usr/include /usr/local/include
            )

            find_library(ZLIB_LIBRARY NAMES z
                    PATHS /usr/lib /usr/lib64 /usr/local/lib
            )

            if(ZLIB_INCLUDE_DIR AND ZLIB_LIBRARY)
                target_include_directories(${target} PUBLIC ${ZLIB_INCLUDE_DIR})
                target_link_libraries(${target} PUBLIC ${ZLIB_LIBRARY})
                target_compile_definitions(${target} PUBLIC MSTL_SUPPORT_ZLIB__)
                message(STATUS "zlib linked manually successfully")
            else()
                message(WARNING "zlib not found - zlib components will be unavailable")
                message(STATUS "  Hint: On Ubuntu/Debian: sudo apt-get install zlib1g-dev")
                message(STATUS "        On CentOS/RHEL: sudo yum install zlib-devel")
            endif()
        endif()
    endif()
    endif()
endfunction()


# 链接文件
set(CPP_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/core/time/datetime.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/time/clocks.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/system/device.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/system/console.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/system/cmdline.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/system/pipe.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/system/process.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/system/stacktrace.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/system/signal.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/system/sysinfo.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/system/environment.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/string/string_util.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/string/vsprintf.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/numeric/random.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/utility/any.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/functional/hash.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/file.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/path.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/file_watcher.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/temp_file.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/json/json_builder.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/json/json_parser.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/json/json_value.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/ini/ini_builder.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/ini/ini_parser.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/ini/ini_value.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/env/env_builder.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/env/env_parser.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/env/env_value.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/toml/toml_builder.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/toml/toml_parser.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/toml/toml_value.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/yaml/yaml_builder.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/yaml/yaml_parser.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/yaml/yaml_value.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/encrypt/encrypt.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/async/at_thread_exit.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/async/condition_variable.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/async/hazard_ptr.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/async/futex.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/async/mutex.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/async/thread.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/async/thread_pool.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/exception/breakpoint.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/exception/exception.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/exception/exception_ptr.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/logging/logger.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/logging/log_sink.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/logging/file_sink.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/database/db_config.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/database/postgresql/postgresql_result.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/database/postgresql/postgresql_prepared_statement.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/database/postgresql/postgresql_connect.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/database/mysql/mysql_result.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/database/mysql/mysql_connect.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/database/mysql/mysql_prepared_result.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/database/mysql/mysql_prepared_statement.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/database/sqlite/sqlite_result.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/database/sqlite/sqlite_connect.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/database/sqlite/sqlite_prepared_statement.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/database/sqlite/sqlite_prepared_result.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/database/redis/redis_result.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/database/redis/redis_connect.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/database/sql_builder.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/database/database_pool.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/ssl_context.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/ssl_socket.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/url.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/tcp_server.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/tcp_client.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/tcp_socket.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/http_constants.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/http_client.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/http_router.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/http_server.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/http_message.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/http_filter.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/session.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/dns/dns_client.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/compress/zlib_compress.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/plugin/dynamic_library.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/plugin/plugin_manager.cpp
)

# 编译为动态库
add_library(MSTL SHARED ${CPP_SOURCES})
add_library(MSTL::MSTL ALIAS MSTL)


target_compile_definitions(MSTL
        PUBLIC
        $<$<CONFIG:Debug>:MSTL_STATE_DEBUG__>
)

if(MSTL_USING_SSO)
    target_compile_definitions(MSTL PUBLIC MSTL_USING_SSO__)
endif()


# 编译设置
if (WIN32)
    set_target_properties(MSTL PROPERTIES
            CXX_STANDARD ${CMAKE_CXX_STANDARD}
            CXX_STANDARD_REQUIRED ON
            CXX_EXTENSIONS OFF
            LINKER_LANGUAGE CXX
            VERSION ${MSTL_VERSION}
            SOVERSION ${MSTL_VERSION_MAJOR}

            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
            ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"

            ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib/Debug
            LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib/Debug
            RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug

            ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib/Release
            LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib/Release
            RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release

            ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/lib/RelWithDebInfo
            LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/lib/RelWithDebInfo
            RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/bin/RelWithDebInfo

            ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/lib/MinSizeRel
            LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/lib/MinSizeRel
            RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/bin/MinSizeRel

            $<$<PLATFORM_ID:Windows>:WINDOWS_EXPORT_ALL_SYMBOLS ON>
    )
elseif(UNIX AND NOT APPLE)
    set_target_properties(MSTL PROPERTIES
            CXX_STANDARD ${CMAKE_CXX_STANDARD}
            CXX_STANDARD_REQUIRED ON
            CXX_EXTENSIONS OFF
            LINKER_LANGUAGE CXX
            VERSION ${MSTL_VERSION}
            SOVERSION ${MSTL_VERSION_MAJOR}

            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
            ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    )
endif()

# 链接头文件
target_include_directories(MSTL
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# 定义宏
if(MSVC)
    target_compile_definitions(MSTL PRIVATE MSTL_DLLEXPORTS)
endif()

# 编译选项
configure_compiler_options(MSTL)

# 链接系统库
if(WIN32)
    target_link_libraries(MSTL PUBLIC
            ws2_32 wsock32 SetupAPI cfgmgr32 pdh
            DbgHelp synchronization Shell32 Psapi
    )
elseif(UNIX AND NOT APPLE)
    target_link_libraries(MSTL PUBLIC
            atomic
    )
endif()


#PostgreSQL
if(MSTL_ENABLE_POSTGRESQL AND MSTL_ARCH STREQUAL "x64")
    set(PostgreSQL_FOUND FALSE)

    if(WIN32)
        set(POSTGRESQL_ROOT_DIR "D:/PostgreSQL/18" CACHE PATH "PostgreSQL root directory")

        find_path(POSTGRESQL_INCLUDE_DIR
                NAMES libpq-fe.h
                HINTS "${POSTGRESQL_ROOT_DIR}/include"
                PATHS
                "C:/Program Files/PostgreSQL/*/include"
                "$ENV{POSTGRESQL_ROOT}/include"
                NO_DEFAULT_PATH
        )

        find_library(POSTGRESQL_LIBRARY
                NAMES libpq libpq-static pq
                HINTS "${POSTGRESQL_ROOT_DIR}/lib"
                PATHS
                "C:/Program Files/PostgreSQL/*/lib"
                "$ENV{POSTGRESQL_ROOT}/lib"
                NO_DEFAULT_PATH
        )

        if(MSVC)
            find_file(POSTGRESQL_DLL
                    NAMES libpq.dll
                    HINTS "${POSTGRESQL_ROOT_DIR}/bin"
                    PATHS
                    "C:/Program Files/PostgreSQL/*/bin"
                    "$ENV{POSTGRESQL_ROOT}/bin"
                    NO_DEFAULT_PATH
            )
        endif()

    else()
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            pkg_check_modules(PC_POSTGRESQL QUIET libpq)
        endif()

        find_path(POSTGRESQL_INCLUDE_DIR
                NAMES libpq-fe.h
                HINTS ${PC_POSTGRESQL_INCLUDE_DIRS}
                PATHS
                /usr/include
                /usr/include/postgresql
                /usr/local/include
                /usr/local/include/postgresql
                /opt/local/include/postgresql
                PATH_SUFFIXES postgresql
        )

        find_library(POSTGRESQL_LIBRARY
                NAMES pq
                HINTS ${PC_POSTGRESQL_LIBRARY_DIRS}
                PATHS
                /usr/lib
                /usr/lib64
                /usr/lib/x86_64-linux-gnu
                /usr/local/lib
                /usr/local/lib64
                PATH_SUFFIXES postgresql
        )
    endif()

    if(POSTGRESQL_INCLUDE_DIR AND POSTGRESQL_LIBRARY)
        set(PostgreSQL_FOUND TRUE)
        target_include_directories(MSTL PUBLIC ${POSTGRESQL_INCLUDE_DIR})
        target_link_libraries(MSTL PUBLIC ${POSTGRESQL_LIBRARY})

        if(WIN32 AND POSTGRESQL_DLL AND EXISTS ${POSTGRESQL_DLL})
            add_custom_command(TARGET MSTL POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${POSTGRESQL_DLL}
                    $<TARGET_FILE_DIR:MSTL>
                    COMMENT "Copying libpq.dll to output directory"
            )
        endif()

        target_compile_definitions(MSTL PUBLIC MSTL_SUPPORT_POSTGRESQL__)
        enable_database_support(MSTL)
        message(STATUS "PostgreSQL linked successfully")
    else()
        message(WARNING "PostgreSQL not found - PostgreSQL components will be unavailable")
        if(WIN32)
            message(STATUS "  Hint: Set POSTGRESQL_ROOT_DIR to your PostgreSQL installation directory")
        endif()
    endif()
else()
    set(disable_reasons "")
    if(NOT MSTL_ARCH STREQUAL "x64")
        list(APPEND disable_reasons "only x64 supported")
    endif()
    if(NOT MSTL_ENABLE_POSTGRESQL)
        list(APPEND disable_reasons "disabled by option")
    endif()
    string(JOIN ", " reason_str ${disable_reasons})
    message(STATUS "PostgreSQL support disabled: ${reason_str}")
endif()


# MySQL
if(MSTL_ENABLE_MYSQL AND MSTL_ARCH STREQUAL "x64")
    set(MySQL_FOUND FALSE)

    if(WIN32)
        set(MYSQL_ROOT_DIR "D:/MySQL/MySQL Server 8.0" CACHE PATH "MySQL root directory")
        find_path(MySQL_INCLUDE_DIR NAMES mysql.h
                HINTS "${MYSQL_ROOT_DIR}/include"
                PATHS ENV MYSQL_DIR PATH_SUFFIXES include
                NO_DEFAULT_PATH)
        find_library(MySQL_LIBRARY NAMES libmysql mysqlclient
                HINTS "${MYSQL_ROOT_DIR}/lib"
                PATHS ENV MYSQL_DIR PATH_SUFFIXES lib
                NO_DEFAULT_PATH)
    else()
        find_path(MySQL_INCLUDE_DIR NAMES mysql.h mysql/mysql.h
                PATHS /usr/include/mysql /usr/local/include/mysql /opt/mysql/include)
        find_library(MySQL_LIBRARY NAMES mysqlclient
                PATHS /usr/lib /usr/local/lib /opt/mysql/lib
                PATH_SUFFIXES x86_64-linux-gnu)
    endif()

    if(MySQL_INCLUDE_DIR AND MySQL_LIBRARY)
        set(MySQL_FOUND TRUE)
        target_include_directories(MSTL PUBLIC ${MySQL_INCLUDE_DIR})
        target_link_libraries(MSTL PUBLIC ${MySQL_LIBRARY})
        target_compile_definitions(MSTL PUBLIC MSTL_SUPPORT_MYSQL__)
        enable_database_support(MSTL)
        message(STATUS "MySQL linked successfully")
    else()
        message(WARNING "MySQL not found - MySQL components will be unavailable")
    endif()
else()
    message(STATUS "MySQL support disabled")
endif()


# SQLite3
if(MSTL_ENABLE_SQLITE3 AND MSTL_ARCH STREQUAL "x64")
    if(WIN32)
        set(SQLITE3_ROOT_DIR "D:/sqlite3" CACHE PATH "SQLite3 root directory")
        find_path(SQLITE3_INCLUDE_DIR NAMES sqlite3.h
                HINTS "${SQLITE3_ROOT_DIR}/include"
                NO_DEFAULT_PATH)
        find_library(SQLITE3_LIBRARY NAMES sqlite3
                HINTS "${SQLITE3_ROOT_DIR}/lib"
                NO_DEFAULT_PATH)

        if(SQLITE3_INCLUDE_DIR AND SQLITE3_LIBRARY)
            target_include_directories(MSTL PUBLIC ${SQLITE3_INCLUDE_DIR})
            target_link_libraries(MSTL PUBLIC ${SQLITE3_LIBRARY})
            target_compile_definitions(MSTL PUBLIC MSTL_SUPPORT_SQLITE3__)
            enable_database_support(MSTL)
            message(STATUS "SQLite3 linked successfully")
        else()
            message(WARNING "SQLite3 not found - SQLite3 components will be unavailable")
        endif()
    else()
        find_package(SQLite3 QUIET)
        if(SQLite3_FOUND)
            target_link_libraries(MSTL PUBLIC SQLite::SQLite3)
            target_compile_definitions(MSTL PUBLIC MSTL_SUPPORT_SQLITE3__)
            enable_database_support(MSTL)
            message(STATUS "SQLite3 linked successfully")
        else()
            message(WARNING "SQLite3 not found - SQLite3 components will be unavailable")
        endif()
    endif()
else()
    message(STATUS "SQLite3 support disabled")
endif()


# Redis
if(MSTL_ENABLE_REDIS AND MSTL_ARCH STREQUAL "x64")
    set(HIREDIS_FOUND FALSE)

    if(WIN32)
        set(HIREDIS_ROOT_DIR "D:/vcpkg/packages/hiredis_x64-windows" CACHE PATH "Hiredis root directory")
        find_path(HIREDIS_INCLUDE_DIR NAMES hiredis.h
                HINTS "${HIREDIS_ROOT_DIR}/include/hiredis"
                NO_DEFAULT_PATH)
        find_library(HIREDIS_LIBRARY NAMES hiredis
                HINTS "${HIREDIS_ROOT_DIR}/lib"
                NO_DEFAULT_PATH)
        find_file(HIREDIS_DLL NAMES hiredis.dll
                HINTS "${HIREDIS_ROOT_DIR}/bin"
                "$ENV{HIREDIS_ROOT}/bin"
                NO_DEFAULT_PATH)
    else()
        find_path(HIREDIS_INCLUDE_DIR NAMES hiredis.h
                PATHS /usr/include/hiredis /usr/local/include/hiredis)
        find_library(HIREDIS_LIBRARY NAMES hiredis
                PATHS /usr/lib /usr/local/lib
                PATH_SUFFIXES x86_64-linux-gnu)
    endif()

    if(HIREDIS_INCLUDE_DIR AND HIREDIS_LIBRARY)
        target_include_directories(MSTL PUBLIC ${HIREDIS_INCLUDE_DIR})
        target_link_libraries(MSTL PUBLIC ${HIREDIS_LIBRARY})
        if(WIN32 AND HIREDIS_DLL AND EXISTS ${HIREDIS_DLL})
            add_custom_command(TARGET MSTL POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${HIREDIS_DLL}
                    $<TARGET_FILE_DIR:MSTL>
                    COMMENT "Copying hiredis.dll to output directory"
            )
        endif()
        target_compile_definitions(MSTL PUBLIC MSTL_SUPPORT_REDIS__)
        enable_database_support(MSTL)
        message(STATUS "Redis (hiredis) linked successfully")
    else()
        message(WARNING "Redis (hiredis) not found - Redis components will be unavailable")
    endif()
else()
    message(STATUS "Redis support disabled")
endif()

find_and_link_zlib(MSTL)

find_and_link_openssl(MSTL)


# 安装库文件
if(WIN32)
    install(TARGETS MSTL
            EXPORT MSTLTargets

            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}

            LIBRARY
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/$<CONFIG>
            CONFIGURATIONS Debug Release RelWithDebInfo MinSizeRel

            ARCHIVE
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/$<CONFIG>
            CONFIGURATIONS Debug Release RelWithDebInfo MinSizeRel

            RUNTIME
            DESTINATION ${CMAKE_INSTALL_BINDIR}/$<CONFIG>
            CONFIGURATIONS Debug Release RelWithDebInfo MinSizeRel

            INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
elseif(UNIX AND NOT APPLE)
    install(TARGETS MSTL
            EXPORT MSTLTargets

            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}

            INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
endif()

if(UNIX AND NOT APPLE)
    install(TARGETS MSTL
            LIBRARY
                DESTINATION ${CMAKE_INSTALL_LIBDIR}
                COMPONENT Runtime
            ARCHIVE
                DESTINATION ${CMAKE_INSTALL_LIBDIR}
                COMPONENT Development
    )
endif()
