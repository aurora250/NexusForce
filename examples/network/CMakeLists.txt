# ---- TCP Echo Server ----
add_nexusforce_example(NexusForceTcpEchoServerExample
    tcp_echo_server.cpp
)

# ---- TCP Echo Client ----
add_nexusforce_example(NexusForceTcpEchoClientExample
    tcp_echo_client.cpp
)

# ---- HTTP Server ----
add_nexusforce_example(NexusForceHttpServerExample
    http_server.cpp
)

# ---- HTTP Client ----
add_nexusforce_example(NexusForceHttpClientExample
    http_client.cpp
)

# ---- SSL Echo Server ----
add_nexusforce_example(NexusForceSslEchoServerExample
    ssl_echo_server.cpp
)

# ---- SSL Echo Client ----
add_nexusforce_example(NexusForceSslEchoClientExample
    ssl_echo_client.cpp
)

# ---- DNS Resolver ----
add_nexusforce_example(NexusForceDnsResolverExample
    dns_resolver.cpp
)

# ---- ICMP Ping ----
add_nexusforce_example(NexusForcePingExample
    ping.cpp
)

# ---- SMTP Mail ----
add_nexusforce_example(NexusForceSmtpMailExample
    smtp_mail.cpp
)

# ---- FTP Client ----
add_nexusforce_example(NexusForceFtpClientExample
    ftp_client.cpp
)

# ---- UDP Echo ----
add_nexusforce_example(NexusForceUdpEchoExample
    udp_echo.cpp
)

# ---- WebSocket Server ----
add_nexusforce_example(NexusForceWebSocketServerExample
    websocket_server.cpp
)

# ---- HTTPS Server ----
add_nexusforce_example(NexusForceHttpsServerExample
    https_server.cpp
)

# ---- HTTP/2 Server (h2c + h2 ALPN) ----
add_nexusforce_example(NexusForceHttp2ServerExample
    http2_server.cpp
)

# ---- gRPC Server ----
add_nexusforce_example(NexusForceGrpcServerExample
    grpc_server.cpp
)

# ---- Reverse Proxy + Load Balancer ----
add_nexusforce_example(NexusForceReverseProxyExample
    reverse_proxy.cpp
)

# ---- Health Check + Rate Limiting + Logging ----
add_nexusforce_example(NexusForceHealthCheckExample
    health_check.cpp
)

# ---- Vue.js Frontend SPA Example ----
add_nexusforce_example(NexusForceVueFrontendExample
    vue_frontend.cpp
)
