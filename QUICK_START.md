# 快速开始

本章节通过 `examples` 目录中的实际示例，展示如何使用 NexusForce 库快速构建常见功能。每个示例均可独立编译运行。

---

## 网络模块

### 1. TCP Echo（tcp_echo_server / tcp_echo_client）

演示 TCP 客户端/服务器的基本用法：创建服务器、设置客户端处理器、连接、收发、超时、异常处理。

**启动服务器：**
```bash
./build/bin/NexusForceTcpEchoServerExample
# TCP Echo Server started on port 8080
# Connect with: telnet localhost 8080
```

**运行客户端：**
```bash
./build/bin/NexusForceTcpEchoClientExample
# Connecting to 127.0.0.1:8080...
# Connected!
# Sent 37 bytes: Hello from NexusForce TCP Client!
# Received 37 bytes: Hello from NexusForce TCP Client!
```

**手动测试：**
```bash
echo "hello world" | nc localhost 8080
```

---

### 2. UDP Echo（udp_echo）

演示 UDP socket 的无连接模式（send_to / receive_from）和已连接模式（connect + send / receive）。

```bash
./build/bin/NexusForceUdpEchoExample
# --- Connectionless Mode ---
# [Server] Received 30 bytes from 127.0.0.1:xxxxx: Hello UDP from NexusForce!
# [Server] Echoed 30 bytes back
# [Client] Sent 30 bytes to 127.0.0.1:9999
# [Client] Received 30 bytes from 127.0.0.1:9999: Hello UDP from NexusForce!
#
# --- Connected Mode ---
# [Client] Sent 26 bytes
# [Client] Received 26 bytes: Hello via connected UDP!
```

---

### 3. HTTP Server（http_server）

演示完整 HTTP 服务器的核心功能：路由注册（GET/POST/PUT/DELETE）、路径参数、正则路由、静态页面、JSON API、表单处理、查询参数、405/404 处理器。

```bash
./build/bin/NexusForceHttpServerExample
# HTTP Server started on http://localhost:8080
```

**测试端点：**
```bash
# 首页
curl http://localhost:8080/

# JSON API
curl http://localhost:8080/api/hello
# {"message":"Hello from NexusForce!","status":"ok"}

# 路径参数
curl http://localhost:8080/api/users/42
# {"user_id":"42","name":"User 42"}

# POST Echo
curl -X POST http://localhost:8080/api/echo -d 'hello world'
# {"echo":"hello world","content_type":"application/x-www-form-urlencoded"}

# 查询参数
curl "http://localhost:8080/api/greet?name=NexusForce"
# {"greeting":"Hello, NexusForce"}

# 查看请求头
curl http://localhost:8080/api/headers

# 正则路由
curl http://localhost:8080/api/v1/anything/here
# {"version":"v1","path":"anything/here"}

# 表单页面（浏览器访问）
open http://localhost:8080/form
```

---

### 4. HTTPS Server（https_server）

演示 HTTPS 服务器：加载 SSL/TLS 证书、加密 HTTP 服务。

**生成自签名证书（一次性）：**
```bash
openssl req -x509 -newkey rsa:2048 -keyout server.key -out server.crt \
  -days 365 -nodes -subj '/CN=localhost'
```

**启动服务器：**
```bash
./build/bin/NexusForceHttpsServerExample
# Certificate loaded successfully
# HTTPS Server started on https://localhost:8443
```

**测试：**
```bash
curl -k https://localhost:8443/api/hello
# {"message":"Hello from NexusForce HTTPS!","status":"ok","tls":true}

curl -k https://localhost:8443/api/users/42
# {"user_id":"42","name":"User 42","tls":true}

curl -k https://localhost:8443/api/info
# {"protocol":"HTTPS (HTTP over TLS)","user_agent":"curl/...","client_ip":"127.0.0.1"}
```

---

### 5. HTTP Client（http_client）

演示 HTTP 客户端：GET/POST 请求、自定义请求头、JSON/表单提交、重定向、HTTPS、超时配置。

```bash
./build/bin/NexusForceHttpClientExample
# === GET Request ===
# Status: 200
# Body: {"args":{"hello":"world"},...}
#
# === Custom Headers ===
# Status: 200
#
# === POST JSON ===
# Status: 200
#
# === POST Form ===
# Status: 200
#
# === Redirect ===
# Status: 200
#
# === HTTPS Request ===
# Status: 200
#
# === Timeout Config ===
# Connect timeout: 10000ms
# ...
```

---

### 6. WebSocket Server（websocket_server）

演示 WebSocket 协议：路由注册、消息收发（文本/二进制）、心跳检测（Ping/Pong）、广播、关闭处理。

```bash
./build/bin/NexusForceWebSocketServerExample
# WebSocket Server started on http://localhost:8080
# Open your browser and navigate to the address above.
# Or use wscat: wscat -c ws://localhost:8080/chat
```

**使用 wscat 测试：**
```bash
wscat -c ws://localhost:8080/chat
# Connected (press CTRL+C to quit)
# < Welcome to NexusForce WebSocket Chat!
# > hello
# < Echo: hello
# > ping
# < Echo: ping
```

**浏览器测试：**
打开 `http://localhost:8080/`，页面内置了完整的 WebSocket 聊天 Demo，可以直接收发消息。

---

### 7. SSL Echo（ssl_echo_server / ssl_echo_client）

演示 SSL/TLS 套接字的直接使用（非 HTTP）：证书加载、TLS 握手、加密数据收发、客户端证书验证。

**生成证书：**
```bash
openssl req -x509 -newkey rsa:2048 -keyout server.key -out server.crt \
  -days 365 -nodes -subj '/CN=localhost'
```

**启动服务器：**
```bash
./build/bin/NexusForceSslEchoServerExample
# Certificate loaded successfully
# TLS Echo Server listening on 0.0.0.0:8443
```

**运行客户端：**
```bash
./build/bin/NexusForceSslEchoClientExample
# Connecting to 127.0.0.1:8443...
# TCP connected, starting TLS handshake...
# TLS handshake successful
# Server certificate:
#   Subject: CN=localhost
# Sent: Hello from NexusForce SSL Client!
# Received: Hello from NexusForce SSL Client!
# Connection closed
```

**手动测试：**
```bash
echo "hello tls" | openssl s_client -connect localhost:8443 -quiet 2>/dev/null
```

---

### 8. DNS Resolver（dns_resolver）

演示 DNS 客户端：A/AAAA/MX/TXT/SOA 记录查询、反向解析（PTR）、批量查询、缓存控制、DNSSEC。

> **中国大陆用户注意：** 示例已使用 `114.114.114.114` 替代被阻断的 `8.8.8.8`，查询域名也替换为国内可访问域名。

```bash
./build/bin/NexusForceDnsResolverExample
# DNS Server: 114.114.114.114
#
# === A Record (IPv4) ===
#   IPv4: 142.250.80.4
#
# === AAAA Record (IPv6) ===
#   IPv6: 2404:6800:4005:801::2004
#
# === MX Record ===
#   MX: 30 alt3.gmr-smtp-in.l.google.com. (priority=30)
#
# === TXT Record ===
#   TXT: v=spf1 include:_spf.google.com ~all
#
# === SOA Record ===
#   MName: ns1.google.com
#   RName: dns-admin.google.com
#   Serial: ...
#
# === Reverse DNS (PTR) ===
#   114.114.114.114 → public1.114dns.com
#
# === Batch Query ===
#   baidu.com → X answers
#   taobao.com → X answers
#   jd.com → X answers
```

---

### 9. ICMP Ping（ping）

演示 ICMP 协议：Ping（Echo Request/Reply）、Traceroute（路由追踪）、RTT 测量。

> **需要 root 权限：** Linux 下原始 ICMP 套接字需要 `CAP_NET_RAW` 能力。

```bash
sudo ./build/bin/NexusForcePingExample
# === Ping 8.8.8.8 ===
#
# 64 bytes from 8.8.8.8: icmp_seq=1 ttl=118 time=15.3ms
# 64 bytes from 8.8.8.8: icmp_seq=2 ttl=118 time=14.8ms
# 64 bytes from 8.8.8.8: icmp_seq=3 ttl=118 time=15.1ms
# 64 bytes from 8.8.8.8: icmp_seq=4 ttl=118 time=14.9ms
#
# --- 8.8.8.8 ping statistics ---
# 4 packets transmitted, 4 received, 0% loss
#
# === Traceroute to 8.8.8.8 ===
# 1  192.168.1.1  1.2ms  1.1ms  1.3ms
# 2  10.0.0.1  5.4ms  5.2ms  5.6ms
# ...
```

---

### 10. SMTP Mail（smtp_mail）

演示 SMTP 邮件发送：STARTTLS 加密、LOGIN 认证、HTML 邮件格式。

> 示例使用 QQ 邮箱 SMTP（smtp.qq.com:587），需要将授权码放入 `tests/resource/authcode` 文件。

```bash
# 准备授权码文件
echo "你的QQ邮箱授权码" > tests/resource/authcode

./build/bin/NexusForceSmtpMailExample
# === SMTP Client Example ===
# /path/to/tests/resource/authcode
# Authorization code loaded (16 chars)
# Connecting to smtp.qq.com:587...
# Upgrading to TLS...
# Authenticating...
# Sending email...
# Email sent successfully!
```

---

### 11. FTP Client（ftp_client）

演示 FTP 客户端：匿名登录、目录列表、文件浏览。通过环境变量可配置目标服务器。

```bash
# 使用默认服务器（ftp.gnu.org，匿名登录）
./build/bin/NexusForceFtpClientExample
# === FTP Client Example ===
# FTP_HOST not set, using default: ftp.gnu.org
# Connecting to ftp.gnu.org:21...
# Connected!
# Logging in as anonymous...
# Login successful!
# Current directory: /
#
# === Directory Listing ===
# d           4096  pub
# -          12345  README
# ...
```

**自定义服务器：**
```bash
FTP_HOST=ftp.example.com FTP_USER=myuser FTP_PASS=mypass ./build/bin/NexusForceFtpClientExample
```

---

## 文件路径说明

| 示例 | 源文件 |
|------|--------|
| TCP Echo Server/Client | `examples/network/tcp_echo_server.cpp` / `tcp_echo_client.cpp` |
| UDP Echo | `examples/network/udp_echo.cpp` |
| HTTP Server | `examples/network/http_server.cpp` |
| HTTPS Server | `examples/network/https_server.cpp` |
| HTTP Client | `examples/network/http_client.cpp` |
| WebSocket Server | `examples/network/websocket_server.cpp` |
| SSL Echo Server/Client | `examples/network/ssl_echo_server.cpp` / `ssl_echo_client.cpp` |
| DNS Resolver | `examples/network/dns_resolver.cpp` |
| ICMP Ping | `examples/network/ping.cpp` |
| SMTP Mail | `examples/network/smtp_mail.cpp` |
| FTP Client | `examples/network/ftp_client.cpp` |
