# 编译器配置
function(configure_compiler_options target)
    if(MSVC)
        set_property(TARGET ${target} PROPERTY
                MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
        )
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
        message(STATUS "Configured MSVC compiler options for ${target} (${CMAKE_BUILD_TYPE})")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE
                -finput-charset=UTF-8 -fexec-charset=UTF-8
                $<$<CONFIG:Debug>:-Werror -O0 -g -gdwarf-4>
                $<$<CONFIG:Release>:-O2>
        )
        message(STATUS "Configured GNU/Clang compiler options for ${target} (${CMAKE_BUILD_TYPE})")
    else()
        message(FATAL_ERROR "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}")
    endif()
endfunction()

# 库查找
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

# 数据库支持检查
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


if(MSVC)
    enable_language(ASM_MASM)

    set(ASM_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/core/memory/memory.asm
    )

    foreach(asm_file ${ASM_SOURCES})
        set_property(
                SOURCE ${asm_file}
                PROPERTY LANGUAGE ASM_MASM
        )
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            set_property(
                    SOURCE ${asm_file}
                    PROPERTY COMPILE_OPTIONS "/nologo;/W3"
            )
            set_property(
                    SOURCE ${asm_file}
                    PROPERTY COMPILE_DEFINITIONS "_WIN64"
            )
        else()
            set_property(
                    SOURCE ${asm_file}
                    PROPERTY COMPILE_OPTIONS "/nologo;/W3"
            )
        endif()
        get_filename_component(asm_name ${asm_file} NAME_WE)
        set_property(SOURCE ${asm_file} PROPERTY OBJECT_OUTPUT_NAME ${asm_name})
    endforeach()
endif()


# 链接文件
set(CPP_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/core/utilities/any.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/datetime/datetime.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/utilities/file.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/utilities/random.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/functional/hash.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/container/rb_tree.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/memory/hexadecimal.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/json/json_value.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/json/json_parser.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/json/json_builder.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/string/encrypt.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/utilities/vsprintf.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/utilities/console.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/memory/memory.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/container/bitmap.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/async/mutex.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/utilities/device.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/async/thread_pool.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/async/thread.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/config/exception.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/logging/logger.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/logging/log_sink.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/utilities/stacktrace.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/datetime/clocks.cpp
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
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/http_constants.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/session.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/servlet.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/http_message.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/filter.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/dns/dns_client.cpp
)

# 编译为动态库
if(MSVC)
    add_library(MSTL SHARED ${CPP_SOURCES} ${ASM_SOURCES})
    target_sources(MSTL PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/MSTL.def)
else()
    add_library(MSTL SHARED ${CPP_SOURCES})
endif()

add_library(MSTL::MSTL ALIAS MSTL)

target_compile_definitions(MSTL
        PRIVATE
        $<$<CONFIG:Debug>:MSTL_STATE_DEBUG__>
)

# 编译设置
set_target_properties(MSTL PROPERTIES
        CXX_STANDARD ${CMAKE_CXX_STANDARD}
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        LINKER_LANGUAGE CXX
        VERSION 1.4.0
        SOVERSION 1

        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH "$ORIGIN/../lib"
        SKIP_BUILD_RPATH FALSE
        BUILD_RPATH_USE_ORIGIN TRUE

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

# 链接头文件
target_include_directories(MSTL
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# 定义导出宏
target_compile_definitions(MSTL PRIVATE MSTL_DLLEXPORTS)

configure_compiler_options(MSTL)

# Windows链接系统库
if(WIN32)
    target_link_libraries(MSTL PUBLIC ws2_32 wsock32 SetupAPI cfgmgr32 DbgHelp)
endif()


# Boost
if(MSTL_ENABLE_BOOST AND MSTL_ARCH STREQUAL "x64")
    if(POLICY CMP0167)
        cmake_policy(SET CMP0167 OLD)
    endif()

    find_package(Boost QUIET COMPONENTS system filesystem)
    if(Boost_FOUND)
        target_link_libraries(MSTL PUBLIC Boost::system)
        target_compile_definitions(MSTL PUBLIC MSTL_SUPPORT_BOOST__)
        message(STATUS "Boost linked successfully")
    else()
        message(WARNING "Boost not found - Boost components will be unavailable")
    endif()
else()
    message(STATUS "Boost support disabled")
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


# Qt6
if(MSTL_ENABLE_QT6 AND MSTL_ARCH STREQUAL "x64" AND CMAKE_CXX_STANDARD GREATER_EQUAL 17)
    set(QT6_SKIP FALSE)
    if(UNIX AND NOT APPLE)
        find_package(OpenGL QUIET)
        if(NOT OpenGL_FOUND)
            message(WARNING "OpenGL not found. Qt6 requires OpenGL support on Linux")
            set(QT6_SKIP TRUE)
        endif()
    endif()

    if(NOT QT6_SKIP)
        if(NOT DEFINED QT6_ROOT_DIR)
            if(MSVC)
                set(QT6_ROOT_DIR "D:/Qt/6.9.1/msvc2022_64")
            elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang" AND WIN32)
                set(QT6_ROOT_DIR "D:/Qt/6.9.1/mingw_64")
            elseif(UNIX AND NOT APPLE)
                set(POSSIBLE_QT6_PATHS
                        "/usr/lib/x86_64-linux-gnu/qt6"
                        "/usr/lib64/qt6"
                        "/usr/lib/qt6"
                        "/opt/qt6"
                        "/usr/local/qt6")
                foreach(QT_PATH ${POSSIBLE_QT6_PATHS})
                    if(EXISTS "${QT_PATH}/lib/cmake/Qt6")
                        set(QT6_ROOT_DIR "${QT_PATH}")
                        break()
                    endif()
                endforeach()
            elseif(APPLE)
                set(QT6_ROOT_DIR "/opt/homebrew/Cellar/qt@6")
            endif()
            set(QT6_ROOT_DIR "${QT6_ROOT_DIR}" CACHE PATH "Qt6 root directory")
        endif()

        if(EXISTS "${QT6_ROOT_DIR}")
            list(APPEND CMAKE_PREFIX_PATH "${QT6_ROOT_DIR}")
            if(EXISTS "${QT6_ROOT_DIR}/lib/cmake")
                list(APPEND CMAKE_PREFIX_PATH "${QT6_ROOT_DIR}/lib/cmake")
            endif()
        endif()

        find_package(Qt6 COMPONENTS Core Widgets Gui QUIET)
        if(Qt6_FOUND)
            if(MSVC)
                target_compile_options(MSTL PRIVATE
                        $<$<COMPILE_LANGUAGE:CXX>:/Zc:__cplusplus>
                )
            endif()

            set_target_properties(MSTL PROPERTIES
                    AUTOMOC ON
                    AUTOUIC ON
                    AUTORCC ON
            )

            target_link_libraries(MSTL PUBLIC Qt6::Core)
            if(OpenGL_FOUND)
                target_link_libraries(MSTL PUBLIC OpenGL::GL)
            endif()

            target_compile_definitions(MSTL PUBLIC MSTL_SUPPORT_QT6__)
            message(STATUS "Qt6 linked successfully")
        else()
            message(WARNING "Qt6 not found.")
        endif()
    endif()
else()
    set(disable_reasons "")
    if(NOT MSTL_ARCH STREQUAL "x64")
        list(APPEND disable_reasons "only x64 supported")
    endif()
    if(CMAKE_CXX_STANDARD LESS 17)
        list(APPEND disable_reasons "requires C++17+")
    endif()
    if(NOT MSTL_ENABLE_QT6)
        list(APPEND disable_reasons "disabled by option")
    endif()
    string(JOIN ", " reason_str ${disable_reasons})
    message(STATUS "Qt6 support disabled: ${reason_str}")
endif()


# CUDA
if(MSTL_ENABLE_CUDA AND MSTL_ARCH STREQUAL "x64")
    if(MSVC AND DEFINED CMAKE_CUDA_COMPILER)
        enable_language(CUDA)
        find_package(CUDAToolkit QUIET)

        if(CUDAToolkit_FOUND)
            set_target_properties(MSTL PROPERTIES
                    CUDA_SEPARABLE_COMPILATION ON
                    CUDA_RESOLVE_DEVICE_SYMBOLS ON
            )
            target_link_libraries(MSTL PUBLIC CUDA::cudart)
            target_compile_definitions(MSTL PUBLIC MSTL_SUPPORT_CUDA__)
            message(STATUS "CUDA linked successfully")
        else()
            message(WARNING "CUDA Toolkit not found - CUDA components will be unavailable")
        endif()
    else()
        message(STATUS "CUDA support disabled: requires MSVC and CUDA compiler")
    endif()
else()
    message(STATUS "CUDA support disabled")
endif()

# 安装库文件
install(TARGETS MSTL
        EXPORT MSTLTargets # 导出目标

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
