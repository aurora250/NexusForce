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

---

## 数据库模块

### 1. SQL Builder（sql_builder）

演示 sql_builder 编程式 SQL 构建：SELECT/INSERT/UPDATE/DELETE、聚合函数、分页、子查询、多表 JOIN、复杂 WHERE 条件。

```bash
./build/bin/NexusForceSqlBuilderExample
# SELECT: SELECT id, name, email, COUNT(id) AS total FROM users AS u ...
# INSERT: INSERT INTO users (name, email, age) VALUES (?, ?, ?)
# UPDATE: UPDATE users SET name = 'NewName', age = 25 WHERE id = '1'
# DELETE: DELETE FROM users WHERE age < '18'
# 聚合:   SELECT SUM(amount) AS total_amount, AVG(amount) AS avg_amount, ...
# 分页:   SELECT * FROM users ORDER BY id ASC LIMIT 10 OFFSET 20
# 子查询: SELECT (SELECT MAX(amount) FROM orders WHERE ...) AS max_order FROM users
# 多JOIN: SELECT * FROM t1 INNER JOIN t2 ON ... LEFT JOIN t3 ON ... RIGHT JOIN t4 ON ...
# 条件:   SELECT * FROM users WHERE status = 'active' AND age BETWEEN 18 AND 65 AND ...
```

---

### 2. DB Config（db_config）

演示 db_config 的多后端连接配置：SQLite、MySQL、PostgreSQL、Redis，以及配置的复制与赋值。

```bash
./build/bin/NexusForceDbConfigExample
# SQLite: database=example.db
# MySQL: host=192.168.1.100, database=mydb, user=admin, charset=utf8mb4
# PgSQL: host=192.168.1.100, database=mydb, user=postgres
# Redis: host=192.168.1.100, database=0
# 复制: database=example.db
# 赋值: database=example.db
```

---

### 3. CRUD + 事务（db_crud）

演示基于 idb_tb_connect 接口的完整数据库操作：建表、插入、查询、更新、删除、表存在性检查、事务 Begin/Commit/Rollback。

```bash
./build/bin/NexusForceDbCrudExample
# 已连接到 SQLite 内存数据库
# users 表已创建
# 已插入 3 行数据
#
# === 查询所有用户 ===
# 列数: 5, 列名: id, name, age, email, salary
#   Row: id=1, name=Alice, age=30, email=alice@test.com, salary=75000.5
#   Row: id=2, name=Bob, age=25, email=bob@test.com, salary=62000.0
#   Row: id=3, name=Charlie, age=35, email=charlie@test.com, salary=88000.0
#
# === 更新 Alice 的薪资 ===
# Alice 更新后: age=31, salary=80000.0
#
# === 删除 Charlie ===
# 删除后剩余行数: 2
#
# === table_exists ===
# users 表存在: true
# nonexistent 表存在: false
#
# === 事务：Commit ===
# Commit 后查到: name=TxUser
#
# === 事务：Rollback ===
# Rollback 后查到: false (预期 false)
```

---

### 4. Transaction Guard（transaction_guard）

演示 RAII 事务管理模式：构造时自动 BEGIN、commit() 显式提交、析构时自动 ROLLBACK、make_transaction 工厂函数。

```bash
./build/bin/NexusForceTransactionGuardExample
# === 转账成功（Commit） ===
# 事务已提交
#   Alice: balance=800.0
#   Bob: balance=700.0
#
# === 转账失败（自动回滚） ===
# 未调用 commit，离开作用域时将自动 rollback
#   Alice: balance=800.0 (应为转账前的值)
#   Bob: balance=700.0 (应为转账前的值)
#
# === make_transaction 工厂函数 ===
# 使用工厂函数创建事务并提交
```

---

### 5. Prepared Statement（prepared_statement）

演示预处理语句的参数化查询：参数绑定（字符串/整数/浮点）、执行型与查询型语句、参数数量获取。

```bash
./build/bin/NexusForcePreparedStatementExample
# 参数数量: 3
# 插入成功: Alice
# 插入成功: Bob
# 插入成功: Charlie
#
# === 年龄 > 20 的用户 ===
#   name=Alice, age=30, email=alice@test.com
#   name=Bob, age=25, email=bob@test.com
#   name=Charlie, age=35, email=charlie@test.com
```

---

### 6. Batch Insert（batch_insert）

演示 batch_insert 批量数据写入：单次调用插入多行、自动参数化、空数据边界处理。

```bash
./build/bin/NexusForceBatchInsertExample
# === 批量插入 ===
# 批量插入了 5 行
#
# === 查询结果 ===
#   Laptop | 5999.00 | Electronics
#   Mouse | 299.00 | Electronics
#   Keyboard | 899.00 | Electronics
#   Monitor | 2999.00 | Electronics
#   Desk | 1599.00 | Furniture
#
# === 边界情况 ===
# 空数据返回: 0 (预期 0)
```

---

### 7. Connection Pool（database_pool）

演示 database_pool 连接池管理：池配置、获取/归还连接、RAII 自动归还、状态查询、优雅停止。

```bash
./build/bin/NexusForceConnectionPoolExample
# 连接池已创建
#   运行状态: true
#   总连接数: 2
#   空闲连接数: 2
#
# === 获取连接执行操作 ===
# 查询结果: pool_example
# 连接已归还到池中
#
# === 同时获取多个连接 ===
# 同时持有 2 个连接
#
# 连接池已停止
#   运行状态: false
#   总连接数: 0
```

---

### 8. Result Metadata（result_metadata）

演示结果集元数据与类型安全访问：列名、列类型、类型安全 getter（get_int32/get_float64）、NULL 检测。

```bash
./build/bin/NexusForceResultMetadataExample
# 列数: 5
# 列元数据:
#   列[0]: name=id, type=INTEGER, ...
#   列[1]: name=name, type=TEXT, ...
#   ...
#
# === 类型安全访问 ===
#   get(1)        → Alice
#   get_int32(2)  → 30
#   get_float64(3) → 75000.0
#   get(4)        → Engineering
```

---

### 9. Redis（redis_example）

演示 Redis 键值操作：SET/GET/EXISTS/DEL、SETEX 过期、Hash（HSET/HGETALL）、List（LPUSH/LRANGE）、Set（SADD/SMEMBERS）、事务（MULTI/EXEC）。

```bash
./build/bin/NexusForceRedisExample
# === SET/GET ===
# GET greeting → Hello from NexusForce!
# SETEX temp_key (600s TTL)
#
# === EXISTS / DEL ===
# EXISTS del_test: true
# EXISTS del_test (after DEL): false
#
# === Hash 操作 ===
# HGETALL user:1:
#   name → Alice
#   age → 30
#   email → alice@test.com
#
# === List 操作 ===
# LRANGE tasks 0 -1:
#   [0] task1
#   [1] task2
#   [2] task3
#
# === Set 操作 ===
# SMEMBERS tags:
#   - cpp
#   - database
#   - network
#
# === Redis 事务 ===
# GET tx_key → tx_value
```

**构建选项：** 需要启用示例构建 `-DNEXUSFORCE_BUILD_EXAMPLES=ON`，各数据库后端通过 `-DNEXUSFORCE_SUPPORT_SQLITE3=ON`、`-DNEXUSFORCE_SUPPORT_HIREDIS=ON` 等选项控制。

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
| SQL Builder | `examples/db/sql_builder_example.cpp` |
| DB Config | `examples/db/db_config_example.cpp` |
| CRUD + 事务 | `examples/db/db_crud_example.cpp` |
| Transaction Guard | `examples/db/transaction_guard_example.cpp` |
| Prepared Statement | `examples/db/prepared_statement_example.cpp` |
| Batch Insert | `examples/db/batch_insert_example.cpp` |
| Connection Pool | `examples/db/connection_pool_example.cpp` |
| Result Metadata | `examples/db/result_metadata_example.cpp` |
| Redis | `examples/db/redis_example.cpp` |
