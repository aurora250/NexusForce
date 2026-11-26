include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/NexusForceCompilerOptions.cmake)

# 链接文件
set(CPP_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/compress/lz4_compress.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/compress/zlib_compress.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/compress/zstd_compress.cpp

        ${CMAKE_CURRENT_SOURCE_DIR}/core/async/async_compose.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/async/async_stream.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/async/condition_variable.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/async/futex.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/async/heap_timer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/async/io_context.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/async/lazy_thread.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/async/mutex.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/async/named_mutex.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/async/notification.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/async/process_watcher.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/async/semaphore.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/async/this_thread.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/async/thread.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/async/thread_exit_notifier.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/async/thread_exit_register.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/async/thread_pool.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/async/timer_scheduler.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/async/virtual_thread.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/encoding/base32.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/encoding/percent.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/encoding/base64.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/kdf/bcrypt.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/kdf/hkdf.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/encrypt/aes256.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/encrypt/chacha20_poly1305.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/exception/crash_dump.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/exception/crash_dump_elf.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/exception/debug.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/exception/error_category.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/exception/error_code.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/exception/exception.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/exception/exception_ptr.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/exception/terminate.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/env/env_builder.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/env/env_parser.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/env/env_value.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/file.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/file_async.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/file_diff.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/file_info.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/file_locker.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/file_mapper.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/file_watcher.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/filesystem.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/ini/ini_builder.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/ini/ini_parser.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/ini/ini_value.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/json/json_builder.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/json/json_parser.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/json/json_value.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/md/markdown_builder.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/md/markdown_parser.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/md/markdown_value.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/mermaid/mermaid.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/path.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/path_tree.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/temp_file.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/rotating_file.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/toml/toml_builder.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/toml/toml_parser.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/toml/toml_value.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/yaml/yaml_builder.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/yaml/yaml_parser.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/file/yaml/yaml_value.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/hash/blake2.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/hash/blake3.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/hash/city_hash.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/hash/md5.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/hash/murmur.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/hash/sha1.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/hash/sha256.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/hash/sha512.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/hash/wyhash.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/hash/xxhash.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/memory/memory_pool.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/numeric/random/engine.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/numeric/random/lcg.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/numeric/random/mt.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/numeric/random/pcg.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/numeric/random/secret.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/reflect/registry.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/serialize/binary_serializer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/serialize/json_serializer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/string/codepoint.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/string/regex.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/string/regex_builtin.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/string/string.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/string/to_numerics.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/string/to_string.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/string/vsprintf.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/string/width.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/system/cmdline.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/system/console.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/system/daemon.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/system/dynamic_library.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/system/environment.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/system/hardware.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/system/io_queue.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/system/pipe.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/system/process.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/system/pty.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/system/registry_key.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/system/share_memory.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/system/signal.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/system/stacktrace.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/system/sysinfo.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/async/waitable_event.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/time/clocks.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/time/datetime.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/typeinfo/check_type.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/utility/any.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/utility/hexadecimal.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/utility/uuid.cpp

        ${CMAKE_CURRENT_SOURCE_DIR}/db/database_pool.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/db/db_config.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/db/mysql/mysql_connect.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/db/mysql/mysql_prepared_result.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/db/mysql/mysql_prepared_statement.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/db/mysql/mysql_result.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/db/pgsql/pgsql_connect.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/db/pgsql/pgsql_prepared_statement.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/db/pgsql/pgsql_result.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/db/redis/redis_connect.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/db/redis/redis_result.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/db/sql_builder.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/db/sqlite/sqlite_connect.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/db/sqlite/sqlite_prepared_statement.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/db/sqlite/sqlite_result.cpp

        ${CMAKE_CURRENT_SOURCE_DIR}/logging/file_sink.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/logging/log_sink.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/logging/logger.cpp

        ${CMAKE_CURRENT_SOURCE_DIR}/network/dns/dns_client.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/ftp/ftp_client.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/ftp/ftp_protocol.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/ftp/ftp_server.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/chunked_reader.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/csrf_filter.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/grpc.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/health_check.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/http2_connection.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/http2_protocol.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/http_cache.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/http_client.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/http_client_message.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/http_compress.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/http_constants.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/http_filter.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/http_range.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/http_router.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/http_security.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/http_server.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/http_server_message.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/http_session.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/load_balancer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/multipart_parser.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/rate_limiter.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/reverse_proxy.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/session_store_redis.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/websocket.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/http/websocket_deflate.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/icmp_socket.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/ip_socket.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/smtp_socket.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/socket_base.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/ssl/sni_manager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/ssl/ssl_acceptor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/ssl/ssl_context.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/ssl/ssl_session_cache.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/ssl/ssl_exception.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/ssl/ssl_socket.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/ssl/ssl_stream.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/ssl/x509_certificate.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/tcp/tcp_acceptor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/tcp/tcp_client.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/tcp/tcp_server.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/tcp/tcp_socket.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/udp_socket.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/util/arp.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/util/ip_address.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/util/mac_address.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/util/network_exception.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/util/ports.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/network/util/url.cpp

        ${CMAKE_CURRENT_SOURCE_DIR}/plugin/plugin_manager.cpp

        ${CMAKE_CURRENT_SOURCE_DIR}/tui/application.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/chart/canvas_chart.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/chart/chart.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/component/animation.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/component/approve_prompt.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/component/collapsible.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/component/component.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/component/composer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/component/containers.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/component/dropdown.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/component/hoverable.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/component/log_view.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/component/menu.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/component/modal.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/component/motion.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/component/output_block.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/component/radiobox.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/component/renderer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/component/resizable_split.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/component/scroll_view.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/component/select_list.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/component/slider.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/component/status_indicator.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/component/streaming_text.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/component/text_area.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/component/text_input.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/component/toggle.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/component/tool_card.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/component/tool_runner.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/component/virtual_list.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/component/window.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/dom/canvas.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/dom/code_block.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/dom/diff.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/dom/element.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/dom/gauge.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/dom/graph.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/dom/highlight.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/dom/latex.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/dom/layout.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/dom/markdown.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/dom/paragraph.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/dom/scroll_indicator.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/dom/spinner.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/dom/style.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/dom/table.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/dom/text_layout.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/dom/theme.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/dom/theme_palettes.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/event_dispatcher.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/focus_manager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/headless.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/image.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/input.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/input_parser.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/keymap.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/reconciler.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/recorder.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/renderer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/screen.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/selection.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/terminal_protocol.cpp
)

# 按模块拆分源文件
set(NEXUSFORCE_MODULES core logging db compress network plugin tui)
set(NEXUSFORCE_SHARED_OBJECT_TARGETS "")
set(NEXUSFORCE_STATIC_OBJECT_TARGETS "")
set(NEXUSFORCE_SHARED_OBJECTS "")
set(NEXUSFORCE_STATIC_OBJECTS "")
set(NEXUSFORCE_COMPONENT_TARGETS "")
set(_nf_partitioned_count 0)

foreach(_module IN LISTS NEXUSFORCE_MODULES)
    set(_module_sources "")
    foreach(_source IN LISTS CPP_SOURCES)
        if(_source MATCHES "/${_module}/")
            list(APPEND _module_sources "${_source}")
        endif()
    endforeach()
    if(NOT _module_sources)
        message(FATAL_ERROR "NexusForce: no sources found for module '${_module}'")
    endif()
    list(LENGTH _module_sources _module_count)
    math(EXPR _nf_partitioned_count "${_nf_partitioned_count} + ${_module_count}")

    string(SUBSTRING "${_module}" 0 1 _first)
    string(TOUPPER "${_first}" _first)
    string(SUBSTRING "${_module}" 1 -1 _rest)
    set(_name "${_first}${_rest}")

    add_library(NexusForce${_name}SharedObjects OBJECT ${_module_sources})
    add_library(NexusForce${_name}StaticObjects OBJECT ${_module_sources})
    set_target_properties(NexusForce${_name}SharedObjects PROPERTIES FOLDER "src/shared")
    set_target_properties(NexusForce${_name}StaticObjects PROPERTIES FOLDER "src/static")
    list(APPEND NEXUSFORCE_SHARED_OBJECT_TARGETS NexusForce${_name}SharedObjects)
    list(APPEND NEXUSFORCE_STATIC_OBJECT_TARGETS NexusForce${_name}StaticObjects)
    list(APPEND NEXUSFORCE_SHARED_OBJECTS $<TARGET_OBJECTS:NexusForce${_name}SharedObjects>)
    list(APPEND NEXUSFORCE_STATIC_OBJECTS $<TARGET_OBJECTS:NexusForce${_name}StaticObjects>)

    add_library(NexusForce${_name} STATIC $<TARGET_OBJECTS:NexusForce${_name}StaticObjects>)
    if(_module STREQUAL "network")
        set(_component net)
    else()
        set(_component "${_module}")
    endif()
    add_library(NexusForce::${_component} ALIAS NexusForce${_name})
    set_target_properties(NexusForce${_name} PROPERTIES EXPORT_NAME ${_component})
    list(APPEND NEXUSFORCE_COMPONENT_TARGETS NexusForce${_name})
endforeach()

list(LENGTH CPP_SOURCES _nf_total_count)
if(NOT _nf_partitioned_count EQUAL _nf_total_count)
    message(FATAL_ERROR "NexusForce: ${_nf_total_count} sources but only ${_nf_partitioned_count} assigned to modules")
endif()

# 模块间链接关系
target_link_libraries(NexusForceLogging PUBLIC NexusForceCore)
target_link_libraries(NexusForceDb PUBLIC NexusForceCore)
target_link_libraries(NexusForceCompress PUBLIC NexusForceCore)
target_link_libraries(NexusForceNetwork PUBLIC NexusForceCore NexusForceLogging NexusForceDb NexusForceCompress)
target_link_libraries(NexusForcePlugin PUBLIC NexusForceCore)
target_link_libraries(NexusForceTui PUBLIC NexusForceCore)

# 单体动态库 / 静态库
add_library(NexusForce SHARED ${NEXUSFORCE_SHARED_OBJECTS})
add_library(NexusForce::NexusForce ALIAS NexusForce)

add_library(NexusForceStatic STATIC ${NEXUSFORCE_STATIC_OBJECTS})
add_library(NexusForce::NexusForceStatic ALIAS NexusForceStatic)

set(NEXUSFORCE_COMPILE_TARGETS ${NEXUSFORCE_SHARED_OBJECT_TARGETS} ${NEXUSFORCE_STATIC_OBJECT_TARGETS})
set(NEXUSFORCE_MONOLITHIC_TARGETS NexusForce NexusForceStatic)
set(NEXUSFORCE_LINK_TARGETS ${NEXUSFORCE_MONOLITHIC_TARGETS} ${NEXUSFORCE_COMPONENT_TARGETS})
set(NEXUSFORCE_LIB_TARGETS ${NEXUSFORCE_COMPILE_TARGETS} ${NEXUSFORCE_LINK_TARGETS})

set(_nf_static_def_targets ${NEXUSFORCE_STATIC_OBJECT_TARGETS} ${NEXUSFORCE_COMPONENT_TARGETS})
foreach(_lib IN LISTS _nf_static_def_targets)
    target_compile_definitions(${_lib} PUBLIC NEFORCE_STATIC)
endforeach()
target_compile_definitions(NexusForceStatic PUBLIC NEFORCE_STATIC)

if(WIN32)
    foreach(_lib IN LISTS NEXUSFORCE_SHARED_OBJECT_TARGETS)
        target_compile_definitions(${_lib} PRIVATE NEFORCE_DLLEXPORTS)
    endforeach()
endif()

# 共享库的对象需要 PIC
if(WIN32 OR UNIX)
    set_target_properties(${NEXUSFORCE_SHARED_OBJECT_TARGETS} PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

function(nexusforce_link_all mode)
    foreach(_lib IN LISTS NEXUSFORCE_LINK_TARGETS)
        target_link_libraries(${_lib} ${mode} ${ARGN})
    endforeach()
endfunction()

function(nexusforce_link_module module mode)
    foreach(_lib IN LISTS NEXUSFORCE_MONOLITHIC_TARGETS)
        target_link_libraries(${_lib} ${mode} ${ARGN})
    endforeach()
    target_link_libraries(NexusForce${module} ${mode} ${ARGN})
    foreach(_obj IN ITEMS NexusForce${module}SharedObjects NexusForce${module}StaticObjects)
        target_link_libraries(${_obj} PRIVATE ${ARGN})
    endforeach()
endfunction()

function(nexusforce_link_both mode)
    nexusforce_link_all(${mode} ${ARGN})
endfunction()

function(nexusforce_define_both mode)
    foreach(_lib IN LISTS NEXUSFORCE_LIB_TARGETS)
        target_compile_definitions(${_lib} ${mode} ${ARGN})
    endforeach()
endfunction()

nexusforce_define_both(PUBLIC
        $<$<CONFIG:Debug>:NEFORCE_STATE_DEBUG>
        NEXUSFORCE_VERSION_MAJOR=${NEXUSFORCE_VERSION_MAJOR}
        NEXUSFORCE_VERSION_MINOR=${NEXUSFORCE_VERSION_MINOR}
        NEXUSFORCE_VERSION_PATCH=${NEXUSFORCE_VERSION_PATCH}
        NEXUSFORCE_VERSION="${NEXUSFORCE_VERSION}"
)

if(NEXUSFORCE_USING_INTEL_TSX)
    nexusforce_define_both(PUBLIC NEFORCE_USING_INTEL_TSX)
endif()

if(NEXUSFORCE_USING_AES_NI)
    nexusforce_define_both(PUBLIC NEFORCE_USING_AES_NI)
endif()

if(NEXUSFORCE_USING_PCLMUL)
    nexusforce_define_both(PUBLIC NEFORCE_USING_PCLMUL)
endif()

if(NEXUSFORCE_USING_VAES)
    nexusforce_define_both(PUBLIC NEFORCE_USING_VAES)
endif()

if(NEXUSFORCE_USING_SQLCIPHER)
    nexusforce_define_both(PUBLIC NEFORCE_USING_SQLCIPHER)
endif()


# 编译设置
if(WIN32 OR UNIX)
    set_target_properties(${NEXUSFORCE_LIB_TARGETS} PROPERTIES
            CXX_STANDARD ${CMAKE_CXX_STANDARD}
            CXX_STANDARD_REQUIRED ON
            CXX_EXTENSIONS OFF
    )
    set_target_properties(${NEXUSFORCE_LINK_TARGETS} PROPERTIES LINKER_LANGUAGE CXX)
    set_target_properties(NexusForce PROPERTIES
            VERSION ${NEXUSFORCE_VERSION}
            SOVERSION ${NEXUSFORCE_VERSION_MAJOR}
    )
endif()

set_target_properties(${NEXUSFORCE_LINK_TARGETS} PROPERTIES DEBUG_POSTFIX "d")
set_target_properties(NexusForce PROPERTIES FOLDER "src/shared")
set_target_properties(NexusForceStatic PROPERTIES FOLDER "src/static")
set_target_properties(${NEXUSFORCE_COMPONENT_TARGETS} PROPERTIES FOLDER "src/static/components")

# 链接头文件
foreach(_lib IN LISTS NEXUSFORCE_LIB_TARGETS)
    target_include_directories(${_lib}
            PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
    )
endforeach()

# 编译选项
foreach(_lib IN LISTS NEXUSFORCE_LIB_TARGETS)
    nexusforce_compiler_options(${_lib})
endforeach()

if(MSVC)
    foreach(_lib IN LISTS NEXUSFORCE_LINK_TARGETS)
        target_compile_options(${_lib} INTERFACE /utf-8)
    endforeach()
endif()

# 链接系统库
if(WIN32)
    nexusforce_link_both(PUBLIC
            bcrypt cfgmgr32 Crypt32 DbgHelp iphlpapi Psapi pdh powrprof
            SetupAPI Shell32 synchronization Wbemuuid ws2_32 wsock32
    )
elseif(APPLE)
    # macOS deprecates the unnamed POSIX semaphore API and libc++ turns the
    # deprecation into an error under -Werror.
    foreach(_lib IN LISTS NEXUSFORCE_LIB_TARGETS)
        target_compile_options(${_lib} PRIVATE -Wno-deprecated-declarations)
    endforeach()
    # Installed dylibs must be relocatable; the build tree keeps its absolute
    # install name so examples can run from the build directory.
    set_target_properties(NexusForce PROPERTIES INSTALL_NAME_DIR "@rpath")
    # libSystem already provides pthread, dl and the backtrace symbols; there
    # is no separate libatomic or librt to link.
    nexusforce_link_both(PUBLIC "-framework Security")
elseif(UNIX)
    if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
        # FreeBSD's libc already provides the atomic builtins; there is no libatomic.
        # backtrace() lives in libexecinfo, so consumers of the library need it as well.
        nexusforce_link_both(PUBLIC pthread execinfo devstat)
    else()
        nexusforce_link_both(PUBLIC
                atomic pthread
        )
    endif()
else()
    message(FATAL_ERROR "This system is NOT SUPPORTED!")
endif()


if(NEXUSFORCE_ENABLE_POSTGRESQL)
    find_package(PostgreSQL QUIET)

    if(TARGET PostgreSQL::PostgreSQL)
        nexusforce_link_module(Db PRIVATE PostgreSQL::PostgreSQL)
        nexusforce_define_both(PUBLIC NEFORCE_SUPPORT_POSTGRESQL NEFORCE_SUPPORT_DB)
        set(NEXUSFORCE_POSTGRESQL_LINKED TRUE PARENT_SCOPE)
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
        nexusforce_link_module(Db PRIVATE unofficial::libmysql::libmysql)
        nexusforce_define_both(PUBLIC NEFORCE_SUPPORT_MYSQL NEFORCE_SUPPORT_DB)
        set(NEXUSFORCE_MYSQL_LINKED TRUE PARENT_SCOPE)
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
            nexusforce_link_module(Db PRIVATE sqlcipher::sqlcipher)
            nexusforce_define_both(PUBLIC
                NEFORCE_SUPPORT_SQLITE3 NEFORCE_SUPPORT_SQLCIPHER NEFORCE_SUPPORT_DB
                SQLITE_HAS_CODEC SQLITE_TEMP_STORE=2)
            set(NEXUSFORCE_SQLCIPHER_LINKED TRUE PARENT_SCOPE)
            set(NEXUSFORCE_SQLITE_LINKED TRUE PARENT_SCOPE)
            message(STATUS "SQLCipher linked successfully")
        else()
            message(WARNING "SQLCipher not found - falling back to SQLite3 without encryption")
            find_package(unofficial-sqlite3 CONFIG QUIET)
            if(TARGET unofficial::sqlite3::sqlite3)
                nexusforce_link_module(Db PRIVATE unofficial::sqlite3::sqlite3)
                nexusforce_define_both(PUBLIC NEFORCE_SUPPORT_SQLITE3 NEFORCE_SUPPORT_DB)
                set(NEXUSFORCE_SQLITE_LINKED TRUE PARENT_SCOPE)
                message(STATUS "SQLite3 linked successfully")
            else()
                message(WARNING "SQLite3 not found - related functionality will be disabled")
            endif()
        endif()
    else()
        find_package(unofficial-sqlite3 CONFIG QUIET)
        if(TARGET unofficial::sqlite3::sqlite3)
            nexusforce_link_module(Db PRIVATE unofficial::sqlite3::sqlite3)
            nexusforce_define_both(PUBLIC NEFORCE_SUPPORT_SQLITE3 NEFORCE_SUPPORT_DB)
            set(NEXUSFORCE_SQLITE_LINKED TRUE PARENT_SCOPE)
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
        nexusforce_link_module(Db PRIVATE hiredis::hiredis)
        nexusforce_define_both(PUBLIC NEFORCE_SUPPORT_HIREDIS NEFORCE_SUPPORT_DB)
        set(NEXUSFORCE_REDIS_LINKED TRUE PARENT_SCOPE)
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
        nexusforce_link_module(Compress PRIVATE ZLIB::ZLIB)
        nexusforce_link_module(Tui PRIVATE ZLIB::ZLIB)
        nexusforce_define_both(PUBLIC NEFORCE_SUPPORT_ZLIB)
        set(NEXUSFORCE_ZLIB_LINKED TRUE PARENT_SCOPE)
        message(STATUS "ZLIB linked successfully")
    endif()
else()
    message(STATUS "zlib support disabled")
endif()

if(NEXUSFORCE_ENABLE_LZ4)
    find_package(lz4 CONFIG QUIET)
    if(TARGET lz4::lz4)
        nexusforce_link_module(Compress PRIVATE lz4::lz4)
        nexusforce_define_both(PUBLIC NEFORCE_SUPPORT_LZ4)
        set(NEXUSFORCE_LZ4_LINKED TRUE PARENT_SCOPE)
        message(STATUS "lz4 linked successfully")
    endif()
else()
    message(STATUS "lz4 support disabled")
endif()


find_package(OpenSSL REQUIRED)
nexusforce_link_module(Network PRIVATE OpenSSL::SSL OpenSSL::Crypto)
nexusforce_define_both(PUBLIC NEFORCE_SUPPORT_OPENSSL)
message(STATUS "OpenSSL linked successfully")

if(NEXUSFORCE_ENABLE_ZSTD)
    find_path(NEFORCE_ZSTD_INCLUDE_DIR zstd.h)
    find_library(NEFORCE_ZSTD_LIBRARY zstd)
    if (NEFORCE_ZSTD_INCLUDE_DIR AND NEFORCE_ZSTD_LIBRARY)
        foreach(_lib IN LISTS NEXUSFORCE_LIB_TARGETS)
            target_include_directories(${_lib} PRIVATE ${NEFORCE_ZSTD_INCLUDE_DIR})
        endforeach()
        nexusforce_link_module(Compress PRIVATE ${NEFORCE_ZSTD_LIBRARY})
        nexusforce_define_both(PRIVATE NEFORCE_SUPPORT_ZSTD)
        set(NEXUSFORCE_ZSTD_LINKED TRUE PARENT_SCOPE)
        message(STATUS "zstd linked successfully")
    endif()
else()
    message(STATUS "zstd support disabled")
endif()


if(NEXUSFORCE_ENABLE_LOCALE)
    foreach(_lib IN ITEMS NexusForceCoreSharedObjects NexusForceCoreStaticObjects)
        target_sources(${_lib} PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}/core/locale/locale.cpp
                ${CMAKE_CURRENT_SOURCE_DIR}/core/locale/locale_data.cpp
                ${CMAKE_CURRENT_SOURCE_DIR}/core/locale/locale_fallback.cpp
                ${CMAKE_CURRENT_SOURCE_DIR}/core/locale/locale_tables.cpp
        )
    endforeach()
    nexusforce_define_both(PUBLIC NEFORCE_SUPPORT_LOCALE)

    if(NEXUSFORCE_ENABLE_ICU)
        find_package(ICU QUIET COMPONENTS uc i18n)
        if(TARGET ICU::uc AND TARGET ICU::i18n)
            nexusforce_link_module(Core PRIVATE ICU::uc ICU::i18n)
            nexusforce_define_both(PUBLIC NEFORCE_SUPPORT_ICU)
            set(NEXUSFORCE_ICU_LINKED TRUE PARENT_SCOPE)
            message(STATUS "ICU linked successfully")
        else()
            message(WARNING "ICU not found - the locale component falls back to its built-in backend")
        endif()
    else()
        message(STATUS "ICU support disabled - the locale component uses its built-in backend")
    endif()
else()
    message(STATUS "locale component disabled - its sources and the ICU probe are skipped")
endif()

message(STATUS "")


# 全局分配器覆盖目标
# 打开 NEXUSFORCE_MEMORY_POOL_GLOBAL_OVERRIDE 只构建提供者，不会让消费者自动继承；
# 需要整进程统一的模块必须显式链接 NexusForceMemoryPoolOverrideStatic。
if(NEXUSFORCE_MEMORY_POOL_GLOBAL_OVERRIDE)
    add_library(NexusForceMemoryPoolOverride OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/core/memory/new.cpp)
    target_link_libraries(NexusForceMemoryPoolOverride PUBLIC NexusForce::NexusForce)
    target_compile_definitions(NexusForceMemoryPoolOverride PRIVATE NEFORCE_USING_MEMORY_POOL_OVERRIDE)
    set_target_properties(NexusForceMemoryPoolOverride PROPERTIES FOLDER "src/static/override")
    nexusforce_compiler_options(NexusForceMemoryPoolOverride)

    add_library(NexusForceMemoryPoolOverrideStatic STATIC $<TARGET_OBJECTS:NexusForceMemoryPoolOverride>)
    target_link_libraries(NexusForceMemoryPoolOverrideStatic PUBLIC NexusForce::NexusForce)
    target_compile_definitions(NexusForceMemoryPoolOverrideStatic INTERFACE NEFORCE_USING_MEMORY_POOL_OVERRIDE)

    set_target_properties(NexusForceMemoryPoolOverrideStatic PROPERTIES
            FOLDER "src/static/override"
            DEBUG_POSTFIX "d")
endif()

# 安装库文件
install(TARGETS NexusForce NexusForceStatic ${NEXUSFORCE_COMPONENT_TARGETS}
        EXPORT NexusForceTargets

        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}

        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

if(NEXUSFORCE_MEMORY_POOL_GLOBAL_OVERRIDE)
    install(TARGETS NexusForceMemoryPoolOverrideStatic
            EXPORT NexusForceTargets
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endif()
