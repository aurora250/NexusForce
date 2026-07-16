include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/NexusForceCompilerOptions.cmake)

# 链接文件
set(CPP_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/compress/lz4_compress.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/compress/zlib_compress.cpp

        ${CMAKE_CURRENT_SOURCE_DIR}/core/async/async_compose.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/async/async_stream.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/async/condition_variable.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/async/futex.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/async/io_context.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/async/lazy_thread.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/async/mutex.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/async/named_mutex.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/async/semaphore.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/async/this_thread.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/async/thread.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/async/thread_exit_notifier.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/async/thread_exit_register.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/async/thread_pool.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/encrypt/aes256.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/encrypt/chacha20_poly1305.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/encrypt/base64.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/encrypt/md5.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/encrypt/sha1.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/encrypt/sha256.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/encrypt/xor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/exception/breakpoint.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/exception/error_category.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/exception/exception.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/exception/exception_ptr.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/exception/terminate.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/file_async.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/file_diff.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/file_info.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/file_locker.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/file_mapper.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/file.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/file_watcher.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/filesystem.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/path.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/path_tree.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/temp_file.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/env/env_builder.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/env/env_parser.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/env/env_value.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/ini/ini_builder.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/ini/ini_parser.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/ini/ini_value.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/json/json_builder.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/json/json_parser.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/json/json_value.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/toml/toml_builder.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/toml/toml_parser.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/toml/toml_value.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/yaml/yaml_builder.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/yaml/yaml_parser.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/yaml/yaml_value.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/functional/hash.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/numeric/int128.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/numeric/random.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/serialize/binary_serializer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/serialize/json_serializer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/reflect/registry.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/string/codepoint.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/string/regex.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/string/string.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/string/vsprintf.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/system/cmdline.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/system/console.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/system/dynamic_library.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/system/environment.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/system/locale.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/system/daemon.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/system/pipe.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/system/process.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/system/registry_key.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/system/share_memory.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/system/signal.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/system/stacktrace.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/system/sysinfo.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/system/system_event.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/time/clocks.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/time/datetime.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/typeinfo/check_type.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/utility/any.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/utility/hexadecimal.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/utility/uuid.cpp

        ${CMAKE_CURRENT_SOURCE_DIR}/db/database_pool.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/db/db_config.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/db/sql_builder.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/db/mysql/mysql_connect.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/db/mysql/mysql_prepared_result.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/db/mysql/mysql_prepared_statement.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/db/mysql/mysql_result.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/db/pgsql/pgsql_connect.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/db/pgsql/pgsql_prepared_statement.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/db/pgsql/pgsql_result.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/db/redis/redis_connect.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/db/redis/redis_result.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/db/sqlite/sqlite_connect.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/db/sqlite/sqlite_prepared_statement.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/db/sqlite/sqlite_result.cpp

        ${CMAKE_CURRENT_SOURCE_DIR}/logging/file_sink.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/logging/log_sink.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/logging/logger.cpp

        ${CMAKE_CURRENT_SOURCE_DIR}/network/icmp_socket.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/ip_socket.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/smtp_socket.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/socket_base.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/udp_socket.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/dns/dns_client.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/ftp/ftp_client.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/ftp/ftp_protocol.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/ftp/ftp_server.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/http_client.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/http_client_message.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/http2_connection.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/http2_protocol.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/http_compress.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/http_constants.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/http_filter.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/http_range.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/http_router.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/http_security.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/rate_limiter.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/session_store_redis.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/http_server.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/http_server_message.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/http_session.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/chunked_reader.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/csrf_filter.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/grpc.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/health_check.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/http_cache.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/load_balancer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/multipart_parser.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/reverse_proxy.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/websocket.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/websocket_deflate.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/ssl/ssl_acceptor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/ssl/sni_manager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/ssl/ssl_context.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/ssl/ssl_socket.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/ssl/ssl_stream.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/tcp/tcp_acceptor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/tcp/tcp_client.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/tcp/tcp_server.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/tcp/tcp_socket.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/util/arp.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/util/ip_address.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/util/mac_address.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/util/ports.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/util/url.cpp

        ${CMAKE_CURRENT_SOURCE_DIR}/plugin/plugin_manager.cpp
)

# 编译为动态库
add_library(NexusForce SHARED ${CPP_SOURCES})
add_library(NexusForce::NexusForce ALIAS NexusForce)

target_compile_definitions(NexusForce
        PUBLIC
        $<$<CONFIG:Debug>:NEFORCE_STATE_DEBUG>
)

if(NEXUSFORCE_USING_SSO)
    target_compile_definitions(NexusForce PUBLIC NEFORCE_USING_SSO)
endif()

if(NEXUSFORCE_USING_INTEL_TSX)
    target_compile_definitions(NexusForce PUBLIC NEFORCE_USING_INTEL_TSX)
endif()

if(NEXUSFORCE_USING_SQLCIPHER)
    target_compile_definitions(NexusForce PUBLIC NEFORCE_USING_SQLCIPHER)
endif()


# 编译设置
if (WIN32)
    set_target_properties(NexusForce PROPERTIES
            CXX_STANDARD ${CMAKE_CXX_STANDARD}
            CXX_STANDARD_REQUIRED ON
            CXX_EXTENSIONS OFF
            LINKER_LANGUAGE CXX
            VERSION ${NEXUSFORCE_VERSION}
            SOVERSION ${NEXUSFORCE_VERSION_MAJOR}
    )
    if(MSVC)
        set_target_properties(NexusForce PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
    elseif(WIN32)
        target_link_options(NexusForce PRIVATE -Wl,-export-all-symbols)
    endif()
elseif(UNIX AND NOT APPLE)
    set_target_properties(NexusForce PROPERTIES
            CXX_STANDARD ${CMAKE_CXX_STANDARD}
            CXX_STANDARD_REQUIRED ON
            CXX_EXTENSIONS OFF
            LINKER_LANGUAGE CXX
            VERSION ${NEXUSFORCE_VERSION}
            SOVERSION ${NEXUSFORCE_VERSION_MAJOR}
    )
endif()

# 链接头文件
target_include_directories(NexusForce
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# 定义宏
if(WIN32)
    target_compile_definitions(NexusForce PRIVATE NEFORCE_DLLEXPORTS)
endif()

# 编译选项
nexusforce_compiler_options(NexusForce)

# 链接系统库
if(WIN32)
    target_link_libraries(NexusForce PUBLIC
            bcrypt cfgmgr32 Crypt32 DbgHelp iphlpapi Psapi pdh
            SetupAPI Shell32 synchronization Wbemuuid ws2_32 wsock32
    )
elseif(UNIX AND NOT APPLE)
    target_link_libraries(NexusForce PUBLIC
            atomic pthread uring
    )
endif()


if(NEXUSFORCE_ENABLE_POSTGRESQL)
    find_package(PostgreSQL QUIET)

    if(TARGET PostgreSQL::PostgreSQL)
        target_link_libraries(NexusForce PUBLIC PostgreSQL::PostgreSQL)
        target_compile_definitions(NexusForce PUBLIC NEFORCE_SUPPORT_POSTGRESQL NEFORCE_SUPPORT_DB)
        message(STATUS "PostgreSQL linked successfully")
    else()
        message(WARNING "PostgreSQL not found - related functionality will be disabled")
    endif()
else()
    message(STATUS "PostgreSQL support disabled")
endif()

if(NEXUSFORCE_ENABLE_MYSQL)
    find_package(unofficial-libmysql CONFIG QUIET)
    if(TARGET unofficial::libmysql::libmysql)
        target_link_libraries(NexusForce PUBLIC unofficial::libmysql::libmysql)
        target_compile_definitions(NexusForce PUBLIC NEFORCE_SUPPORT_MYSQL NEFORCE_SUPPORT_DB)
        message(STATUS "MySQL linked successfully")
    else()
        message(WARNING "MySQL not found - related functionality will be disabled")
    endif()
else()
    message(STATUS "MySQL support disabled")
endif()

if(NEXUSFORCE_ENABLE_SQLITE3)
    if(WIN32 AND NEXUSFORCE_USING_SQLCIPHER)
        find_package(sqlcipher CONFIG QUIET)
        if(TARGET sqlcipher::sqlcipher)
            target_link_libraries(NexusForce PUBLIC sqlcipher::sqlcipher)
            target_compile_definitions(NexusForce PUBLIC
                NEFORCE_SUPPORT_SQLITE3 NEFORCE_SUPPORT_SQLCIPHER NEFORCE_SUPPORT_DB
                SQLITE_HAS_CODEC SQLITE_TEMP_STORE=2)
            set(NEXUSFORCE_SQLCIPHER_LINKED TRUE PARENT_SCOPE)
            message(STATUS "SQLCipher linked successfully")
        else()
            message(WARNING "SQLCipher not found - falling back to SQLite3 without encryption")
            find_package(unofficial-sqlite3 CONFIG QUIET)
            if(TARGET unofficial::sqlite3::sqlite3)
                target_link_libraries(NexusForce PUBLIC unofficial::sqlite3::sqlite3)
                target_compile_definitions(NexusForce PUBLIC NEFORCE_SUPPORT_SQLITE3 NEFORCE_SUPPORT_DB)
                message(STATUS "SQLite3 linked successfully")
            else()
                message(WARNING "SQLite3 not found - related functionality will be disabled")
            endif()
        endif()
    else()
        find_package(unofficial-sqlite3 CONFIG QUIET)
        if(TARGET unofficial::sqlite3::sqlite3)
            target_link_libraries(NexusForce PUBLIC unofficial::sqlite3::sqlite3)
            target_compile_definitions(NexusForce PUBLIC NEFORCE_SUPPORT_SQLITE3 NEFORCE_SUPPORT_DB)
            message(STATUS "SQLite3 linked successfully")
        else()
            message(WARNING "SQLite3 not found - related functionality will be disabled")
        endif()
    endif()
else()
    message(STATUS "SQLite3 support disabled")
endif()

if(NEXUSFORCE_ENABLE_REDIS)
    find_package(hiredis CONFIG QUIET)
    if(TARGET hiredis::hiredis)
        target_link_libraries(NexusForce PUBLIC hiredis::hiredis)
        target_compile_definitions(NexusForce PUBLIC NEFORCE_SUPPORT_HIREDIS NEFORCE_SUPPORT_DB)
        message(STATUS "hiredis linked successfully")
    else()
        message(WARNING "hiredis not found - related functionality will be disabled")
    endif()
else()
    message(STATUS "Redis support disabled")
endif()

if(NEXUSFORCE_ENABLE_ZLIB)
    find_package(ZLIB QUIET)
    if(TARGET ZLIB::ZLIB)
        target_link_libraries(NexusForce PUBLIC ZLIB::ZLIB)
        target_compile_definitions(NexusForce PUBLIC NEFORCE_SUPPORT_ZLIB)
        message(STATUS "ZLIB linked successfully")
    endif()
else()
    message(STATUS "zlib support disabled")
endif()

if(NEXUSFORCE_ENABLE_LZ4)
    find_package(lz4 QUIET)
    if(TARGET lz4::lz4)
        target_link_libraries(NexusForce PUBLIC lz4::lz4)
        target_compile_definitions(NexusForce PUBLIC NEFORCE_SUPPORT_LZ4)
        message(STATUS "lz4 linked successfully")
    endif()
else()
    message(STATUS "lz4 support disabled")
endif()


find_package(OpenSSL REQUIRED)
target_link_libraries(NexusForce PUBLIC OpenSSL::SSL OpenSSL::Crypto)
target_compile_definitions(NexusForce PUBLIC NEFORCE_SUPPORT_OPENSSL)
message(STATUS "OpenSSL linked successfully")

find_package(pcre2 CONFIG REQUIRED COMPONENTS 8BIT)
target_link_libraries(NexusForce PUBLIC PCRE2::8BIT)
target_compile_definitions(NexusForce PUBLIC
        NEFORCE_SUPPORT_PCRE2
        PCRE2_CODE_UNIT_WIDTH=8
)
message(STATUS "PCRE2 with JIT linked successfully")

find_package(ICU REQUIRED COMPONENTS uc i18n)
target_link_libraries(NexusForce PUBLIC ICU::uc ICU::i18n)
target_compile_definitions(NexusForce PUBLIC NEFORCE_SUPPORT_ICU)
message(STATUS "ICU linked successfully")

message(STATUS "")


# 安装库文件
install(TARGETS NexusForce
        EXPORT NexusForceTargets

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
