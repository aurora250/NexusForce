# ---- TCP Echo Server ----
add_nexusforce_example(NexusForceTcpEchoServerExample network
    tcp_echo_server.cpp
)

# ---- TCP Echo Client ----
add_nexusforce_example(NexusForceTcpEchoClientExample network
    tcp_echo_client.cpp
)

# ---- HTTP Server ----
add_nexusforce_example(NexusForceHttpServerExample network
    http_server.cpp
)

# ---- HTTP Client ----
add_nexusforce_example(NexusForceHttpClientExample network
    http_client.cpp
)

# ---- SSL Echo Server ----
add_nexusforce_example(NexusForceSslEchoServerExample network
    ssl_echo_server.cpp
)

# ---- SSL Echo Client ----
add_nexusforce_example(NexusForceSslEchoClientExample network
    ssl_echo_client.cpp
)

# ---- DNS Resolver ----
add_nexusforce_example(NexusForceDnsResolverExample network
    dns_resolver.cpp
)

# ---- ICMP Ping ----
add_nexusforce_example(NexusForcePingExample network
    ping.cpp
)

# ---- SMTP Mail ----
add_nexusforce_example(NexusForceSmtpMailExample network
    smtp_mail.cpp
)

# ---- FTP Client ----
add_nexusforce_example(NexusForceFtpClientExample network
    ftp_client.cpp
)

# ---- UDP Echo ----
add_nexusforce_example(NexusForceUdpEchoExample network
    udp_echo.cpp
)

# ---- WebSocket Server ----
add_nexusforce_example(NexusForceWebSocketServerExample network
    websocket_server.cpp
)

# ---- HTTPS Server ----
add_nexusforce_example(NexusForceHttpsServerExample network
    https_server.cpp
)

# ---- HTTP/2 Server (h2c + h2 ALPN) ----
add_nexusforce_example(NexusForceHttp2ServerExample network
    http2_server.cpp
)

# ---- gRPC Server ----
add_nexusforce_example(NexusForceGrpcServerExample network
    grpc_server.cpp
)

# ---- Reverse Proxy + Load Balancer ----
add_nexusforce_example(NexusForceReverseProxyExample network
    reverse_proxy.cpp
)

# ---- Health Check + Rate Limiting + Logging ----
add_nexusforce_example(NexusForceHealthCheckExample network
    health_check.cpp
)

# ---- Vue.js Frontend SPA Example ----
add_nexusforce_example(NexusForceVueFrontendExample network
    vue_frontend.cpp
)
