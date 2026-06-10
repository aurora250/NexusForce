/**
 * @example http2_server.cpp
 * @brief HTTP/2 服务器双模式示例 (h2c + h2 TLS)
 *
 * 演示 HTTP/2 核心功能：
 * - h2c 模式 (port 8080): HTTP/1.1 Upgrade 升级到 HTTP/2
 * - h2 模式 (port 8443): TLS + ALPN 协商自动升级到 HTTP/2
 * - 路由注册（GET/POST，路径参数，JSON 响应）
 * - HTTP/2 Server Push (Link preload header)
 * - 过滤器链（日志 + 压缩）
 *
 * 使用方式：
 *   h2c: curl --http2 http://localhost:8080/api/info
 *   h2:  curl --http2 -k https://localhost:8443/api/info
 */

#include <NeForce/core/file/json/json_builder.hpp>
#include <NeForce/core/file/path.hpp>
#include <NeForce/core/system/console.hpp>
#include <NeForce/core/system/process.hpp>
#include <NeForce/network/http/http_compress.hpp>
#include <NeForce/network/http/http_filter.hpp>
#include <NeForce/network/http/http_server.hpp>
#include <NeForce/network/ssl/ssl_context.hpp>

using namespace neforce;
using namespace neforce::http;

namespace {
    auto set_json(http_response& res) { res.set_content_type(http_content::JSON_APP()); }
} // namespace

int main() {
    println("========================================");
    println("  NexusForce HTTP/2 Server Example");
    println("========================================");

    // ---- 1. h2c (cleartext) ----
    println("\n[1/2] Starting h2c server on port 8080...");
    http_server h2c_server(ports(8080u), 2);
    auto& r1 = h2c_server.router();

    // 诊断路由：纯 HTTP/1.1，不经过 h2c 升级
    r1.get("/ping", [](http_request& req, http_response& res) {
        res.body = "pong";
        res.set_content_type(http_content::PLAIN_TEXT());
    });

    r1.use(make_unique<logging_filter>());
#ifdef NEFORCE_SUPPORT_ZLIB
    auto c1 = make_unique<compress_filter>();
    c1->min_size = 1_KB;
    r1.use(move(c1));
#endif

    r1.get("/", [](http_request& req, http_response& res) {
        json_builder jb;
        jb.begin_object();
        jb.key("server").value("NexusForce HTTP/2 (h2c)");
        jb.key("protocol").value("h2c (cleartext upgrade)");
        jb.key("endpoints")
                .begin_array()
                .value("GET  /api/info")
                .value("GET  /api/users/:id")
                .value("POST /api/echo")
                .value("GET  /api/stream")
                .end_array();
        jb.end_object();
        res.body = jb.build()->to_string();
        set_json(res);
    });

    r1.get("/api/info", [](http_request& req, http_response& res) {
        json_builder jb;
        jb.begin_object();
        jb.key("protocol").value("HTTP/2 (h2c)");
        jb.key("method").value(req.method.method());
        jb.key("path").value(req.path);
        jb.key("http_version").value(req.version);
        jb.end_object();
        res.body = jb.build()->to_string();
        set_json(res);
    });

    r1.get("/api/users/:id", [](http_request& req, http_response& res) {
        json_builder jb;
        jb.begin_object();
        jb.key("user_id").value(req.parameter("id"));
        jb.key("source").value("HTTP/2 h2c");
        jb.end_object();
        res.body = jb.build()->to_string();
        set_json(res);
    });

    r1.post("/api/echo", [](http_request& req, http_response& res) {
        json_builder jb;
        jb.begin_object();
        jb.key("echo").value(req.body);
        jb.key("size").value(static_cast<double>(req.body.size()));
        jb.end_object();
        res.body = jb.build()->to_string();
        set_json(res);
    });

    if (!h2c_server.start()) {
        println("[h2c] Failed to start on port 8080");
        return 1;
    }
    printfln("[h2c] HTTP/2 (cleartext) on http://localhost:{}", static_cast<uint16_t>(h2c_server.port()));

    // ---- 2. h2 (TLS + ALPN) ----
    println("\n[2/2] Starting h2 server on port 8443...");
    ssl_context ctx(ssl_method::TLS_SERVER);
    optional<http_server> h2_server;

    // 自动生成自签名证书（如不存在）
    if (!path::exists("server.crt")) {
        println("[h2] Generating self-signed certificate...");
        auto res = process::execute_shell("openssl req -x509 -newkey rsa:2048 -keyout server.key -out server.crt "
                                          "-days 365 -nodes -subj '/CN=localhost' 2>/dev/null");
        printfln("exit code: {}", res.exit_code);
    }
    if (ctx.load_certificate("server.crt", "server.key")) {
        println("[h2] Certificate loaded. ALPN auto-registered: h2, http/1.1");

        h2_server.emplace(ports(8443u), move(ctx), 2);
        auto& r2 = h2_server->router();

        r2.use(make_unique<logging_filter>());

        r2.get("/", [](http_request& req, http_response& res) {
            json_builder jb;
            jb.begin_object();
            jb.key("server").value("NexusForce HTTP/2 (h2)");
            jb.key("protocol").value("h2 (ALPN negotiation)");
            jb.key("tls").value(true);
            jb.key("endpoints")
                    .begin_array()
                    .value("GET  /api/info")
                    .value("GET  /api/users/:id")
                    .value("POST /api/echo")
                    .value("GET  /api/push")
                    .end_array();
            jb.end_object();
            res.body = jb.build()->to_string();
            set_json(res);
        });

        r2.get("/api/info", [](http_request& req, http_response& res) {
            json_builder jb;
            jb.begin_object();
            jb.key("protocol").value("HTTP/2 (h2)");
            jb.key("tls").value(true);
            jb.key("alpn").value("h2");
            jb.key("method").value(req.method.method());
            jb.end_object();
            res.body = jb.build()->to_string();
            set_json(res);
        });

        r2.get("/api/users/:id", [](http_request& req, http_response& res) {
            json_builder jb;
            jb.begin_object();
            jb.key("user_id").value(req.parameter("id"));
            jb.key("source").value("HTTP/2 h2 (TLS)");
            jb.end_object();
            res.body = jb.build()->to_string();
            set_json(res);
        });

        r2.post("/api/echo", [](http_request& req, http_response& res) {
            json_builder jb;
            jb.begin_object();
            jb.key("echo").value(req.body);
            jb.key("size").value(static_cast<double>(req.body.size()));
            jb.end_object();
            res.body = jb.build()->to_string();
            set_json(res);
        });

        r2.get("/api/push", [](http_request& req, http_response& res) {
            res.set_header("Link", "</api/info>; rel=preload; as=fetch");
            json_builder jb;
            jb.begin_object();
            jb.key("message").value("Link preload header set for /api/info");
            jb.key("feature").value("HTTP/2 Server Push");
            jb.end_object();
            res.body = jb.build()->to_string();
            set_json(res);
        });

        if (!h2_server->start()) {
            println("[h2] Failed to start on port 8443");
            h2_server.reset();
        } else {
            printfln("[h2] HTTP/2 (TLS) on https://localhost:{}", static_cast<uint16_t>(h2_server->port()));
        }
    } else {
        println("[h2] Certificate not found (server.crt / server.key) — skipping h2 mode");
        println("[h2] Generate: openssl req -x509 -newkey rsa:2048 -keyout server.key -out server.crt \\");
        println("       -days 365 -nodes -subj '/CN=localhost'");
    }

    // ---- 运行 ----
    println("");
    println("Both servers running. Press Ctrl+C to stop.");
    println("");
    println("  h2c: curl --http2 http://localhost:8080/api/info");
    if (h2_server) {
        println("  h2:  curl --http2 -k https://localhost:8443/api/info");
    }

    while (true) {
        this_thread::sleep_for(10_ms);
    }

    h2c_server.stop();
    if (h2_server) {
        h2_server->stop();
    }
    println("Servers stopped.");
    return 0;
}
