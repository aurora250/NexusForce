# =============================================================================
# NexusForce Network Module Examples
# =============================================================================

# ---- TCP Echo Server ----
add_executable(NexusForceTcpEchoServerExample
    tcp_echo_server.cpp
)
if(WIN32)
    set_target_properties(NexusForceTcpEchoServerExample PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
elseif(UNIX AND NOT APPLE)
    set_target_properties(NexusForceTcpEchoServerExample PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endif()
target_link_libraries(NexusForceTcpEchoServerExample PRIVATE NexusForce::NexusForce)
nexusforce_compiler_options(NexusForceTcpEchoServerExample)

# ---- TCP Echo Client ----
add_executable(NexusForceTcpEchoClientExample
    tcp_echo_client.cpp
)
if(WIN32)
    set_target_properties(NexusForceTcpEchoClientExample PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
elseif(UNIX AND NOT APPLE)
    set_target_properties(NexusForceTcpEchoClientExample PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endif()
target_link_libraries(NexusForceTcpEchoClientExample PRIVATE NexusForce::NexusForce)
nexusforce_compiler_options(NexusForceTcpEchoClientExample)

# ---- HTTP Server ----
add_executable(NexusForceHttpServerExample
    http_server.cpp
)
if(WIN32)
    set_target_properties(NexusForceHttpServerExample PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
elseif(UNIX AND NOT APPLE)
    set_target_properties(NexusForceHttpServerExample PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endif()
target_link_libraries(NexusForceHttpServerExample PRIVATE NexusForce::NexusForce)
nexusforce_compiler_options(NexusForceHttpServerExample)

# ---- HTTP Client ----
add_executable(NexusForceHttpClientExample
    http_client.cpp
)
if(WIN32)
    set_target_properties(NexusForceHttpClientExample PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
elseif(UNIX AND NOT APPLE)
    set_target_properties(NexusForceHttpClientExample PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endif()
target_link_libraries(NexusForceHttpClientExample PRIVATE NexusForce::NexusForce)
nexusforce_compiler_options(NexusForceHttpClientExample)

# ---- SSL Echo Server ----
add_executable(NexusForceSslEchoServerExample
    ssl_echo_server.cpp
)
if(WIN32)
    set_target_properties(NexusForceSslEchoServerExample PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
elseif(UNIX AND NOT APPLE)
    set_target_properties(NexusForceSslEchoServerExample PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endif()
target_link_libraries(NexusForceSslEchoServerExample PRIVATE NexusForce::NexusForce)
nexusforce_compiler_options(NexusForceSslEchoServerExample)

# ---- SSL Echo Client ----
add_executable(NexusForceSslEchoClientExample
    ssl_echo_client.cpp
)
if(WIN32)
    set_target_properties(NexusForceSslEchoClientExample PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
elseif(UNIX AND NOT APPLE)
    set_target_properties(NexusForceSslEchoClientExample PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endif()
target_link_libraries(NexusForceSslEchoClientExample PRIVATE NexusForce::NexusForce)
nexusforce_compiler_options(NexusForceSslEchoClientExample)

# ---- DNS Resolver ----
add_executable(NexusForceDnsResolverExample
    dns_resolver.cpp
)
if(WIN32)
    set_target_properties(NexusForceDnsResolverExample PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
elseif(UNIX AND NOT APPLE)
    set_target_properties(NexusForceDnsResolverExample PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endif()
target_link_libraries(NexusForceDnsResolverExample PRIVATE NexusForce::NexusForce)
nexusforce_compiler_options(NexusForceDnsResolverExample)

# ---- ICMP Ping ----
add_executable(NexusForcePingExample
    ping.cpp
)
if(WIN32)
    set_target_properties(NexusForcePingExample PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
elseif(UNIX AND NOT APPLE)
    set_target_properties(NexusForcePingExample PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endif()
target_link_libraries(NexusForcePingExample PRIVATE NexusForce::NexusForce)
nexusforce_compiler_options(NexusForcePingExample)

# ---- SMTP Mail ----
add_executable(NexusForceSmtpMailExample
    smtp_mail.cpp
)
if(WIN32)
    set_target_properties(NexusForceSmtpMailExample PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
elseif(UNIX AND NOT APPLE)
    set_target_properties(NexusForceSmtpMailExample PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endif()
target_link_libraries(NexusForceSmtpMailExample PRIVATE NexusForce::NexusForce)
nexusforce_compiler_options(NexusForceSmtpMailExample)

# ---- FTP Client ----
add_executable(NexusForceFtpClientExample
    ftp_client.cpp
)
if(WIN32)
    set_target_properties(NexusForceFtpClientExample PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
elseif(UNIX AND NOT APPLE)
    set_target_properties(NexusForceFtpClientExample PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endif()
target_link_libraries(NexusForceFtpClientExample PRIVATE NexusForce::NexusForce)
nexusforce_compiler_options(NexusForceFtpClientExample)

# ---- UDP Echo ----
add_executable(NexusForceUdpEchoExample
    udp_echo.cpp
)
if(WIN32)
    set_target_properties(NexusForceUdpEchoExample PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
elseif(UNIX AND NOT APPLE)
    set_target_properties(NexusForceUdpEchoExample PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endif()
target_link_libraries(NexusForceUdpEchoExample PRIVATE NexusForce::NexusForce)
nexusforce_compiler_options(NexusForceUdpEchoExample)

# ---- WebSocket Server ----
add_executable(NexusForceWebSocketServerExample
    websocket_server.cpp
)
if(WIN32)
    set_target_properties(NexusForceWebSocketServerExample PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
elseif(UNIX AND NOT APPLE)
    set_target_properties(NexusForceWebSocketServerExample PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endif()
target_link_libraries(NexusForceWebSocketServerExample PRIVATE NexusForce::NexusForce)
nexusforce_compiler_options(NexusForceWebSocketServerExample)

# ---- HTTPS Server ----
add_executable(NexusForceHttpsServerExample
    https_server.cpp
)
if(WIN32)
    set_target_properties(NexusForceHttpsServerExample PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
elseif(UNIX AND NOT APPLE)
    set_target_properties(NexusForceHttpsServerExample PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endif()
target_link_libraries(NexusForceHttpsServerExample PRIVATE NexusForce::NexusForce)
nexusforce_compiler_options(NexusForceHttpsServerExample)
