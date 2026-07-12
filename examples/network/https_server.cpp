/**
 * @example https_server.cpp
 * @brief HTTPS 服务器综合示例
 *
 * 演示 HTTPS 核心功能：
 * - 加载 SSL/TLS 证书
 * - Filter 链（CORS + 压缩 + 日志 + 静态文件）
 * - Session 会话管理（访问计数器）
 * - HTTP/2 ALPN 自动协商（服务端自动注册 h2 + http/1.1）
 * - 路由注册（GET/POST，路径参数，JSON 响应）
 * - 405/404 处理器
 *
 * 运行前需生成自签名证书：
 *   openssl req -x509 -newkey rsa:2048 -keyout server.key -out server.crt -days 365 -nodes -subj '/CN=localhost'
 *
 * 测试：
 *   HTTP/1.1:  curl -k https://localhost:8443/api/hello
 *   HTTP/2:    curl --http2 -k https://localhost:8443/api/info
 */

#include <NeForce/core/file/json/json_builder.hpp>
#include <NeForce/core/system/console.hpp>
#include <NeForce/network/http/http_compress.hpp>
#include <NeForce/network/http/http_filter.hpp>
#include <NeForce/network/http/http_server.hpp>
#include <NeForce/network/ssl/ssl_context.hpp>

using namespace neforce;
using namespace neforce::http;

namespace {
#ifdef NEFORCE_PLATFORM_WINDOWS
    const char* SERVER_CERT = "D:/OpenSSL/server.crt";
    const char* SERVER_KEY = "D:/OpenSSL/server.key";
#else
    const char* SERVER_CERT = "/home/huenqi/server.crt";
    const char* SERVER_KEY = "/home/huenqi/server.key";
#endif
} // namespace


int main() {
    // =========================================================================
    // 加载 SSL 证书
    // =========================================================================
    ssl_context ctx(ssl_method::TLS_SERVER);

    if (!ctx.load_certificate(SERVER_CERT, SERVER_KEY)) {
        println("Failed to load certificate (server.crt / server.key). Generate with:");
        println("  openssl req -x509 -newkey rsa:2048 -keyout server.key -out server.crt \\");
        println("    -days 365 -nodes -subj '/CN=localhost'");
        return 1;
    }
    println("Certificate loaded successfully");

    // =========================================================================
    // 创建 HTTPS 服务器
    //
    // 注意: HTTPS 构造函数会自动注册 ALPN {"h2", "http/1.1"}，
    //       支持 HTTP/2 over TLS（h2）自动协商。
    //       不需要额外配置即可同时服务 HTTP/1.1 和 HTTP/2 客户端。
    // =========================================================================
    io_context ioc;
    http_server server(ports(8443u), ioc, move(ctx), 2);

    auto& router = server.router();

    // =========================================================================
    // 过滤器链
    // =========================================================================

    // 1. CORS 跨域支持
    router.use(make_unique<cors_filter>("*"));

#ifdef NEFORCE_SUPPORT_ZLIB
    // 2. HTTP 响应压缩 — gzip/deflate
    auto compress = make_unique<compress_filter>();
    compress->min_size = 1_KB;
    router.use(move(compress));
#endif

    // 3. 请求/响应日志
    auto logging = make_unique<logging_filter>();
    logging->log_headers = true;
    router.use(move(logging));

    // 4. 静态文件服务（支持 Range 请求）
    auto static_files = make_unique<static_file_filter>("./public");
    static_files->set_enable_range(true);
    router.use(move(static_files));

    // =========================================================================
    // 路由
    // =========================================================================

    // 首页 — Session 访问计数器
    router.get("/", [](http_request& req, http_response& res) {
        int visit_count = 0;
        if (req.has_session()) {
            auto* sess = req.session;
            const auto visits_str = sess->get("visits");
            visit_count = visits_str.empty() ? 0 : to_int32(visits_str);
            visit_count++;
            sess->set("visits", to_string(visit_count));
        }

        res.body = "<h1>NexusForce HTTPS Server</h1>"
                   "<p>Secure connection over TLS. Your visits: " +
                   to_string(visit_count) +
                   "</p>"
                   "<ul>"
                   "<li>GET <a href='/api/hello'>/api/hello</a></li>"
                   "<li>GET <a href='/api/users/42'>/api/users/:id</a></li>"
                   "<li>GET <a href='/api/info'>/api/info</a> (ALPN info)</li>"
                   "<li>GET <a href='/api/session'>/api/session</a> (session data)</li>"
                   "<li>POST /api/echo</li>"
                   "<li>GET <a href='/public/'>/public/*</a> (static files)</li>"
                   "</ul>"
                   "<p><small>Supports HTTP/1.1 and HTTP/2 (h2 via ALPN)</small></p>";
        res.set_content_type(http_content::HTML_TEXT());
    });

    // JSON API
    router.get("/api/hello", [](http_request& req, http_response& res) {
        json_builder jb;
        jb.begin_object();
        jb.key("message").value("Hello from NexusForce HTTPS!");
        jb.key("status").value("ok");
        jb.key("tls").value(true);
        jb.end_object();
        res.body = jb.build()->to_string();
        res.set_content_type(http_content::JSON_APP());
    });

    // 路径参数
    router.get("/api/users/:id", [](http_request& req, http_response& res) {
        json_builder jb;
        jb.begin_object();
        jb.key("user_id").value(req.parameter("id"));
        jb.key("name").value("User " + string(req.parameter("id")));
        jb.key("tls").value(true);
        jb.end_object();
        res.body = jb.build()->to_string();
        res.set_content_type(http_content::JSON_APP());
    });

    // 连接信息（含 ALPN 说明）
    router.get("/api/info", [](http_request& req, http_response& res) {
        json_builder jb;
        jb.begin_object();
        jb.key("protocol").value("HTTPS (HTTP over TLS)");
        jb.key("http2_support").value("h2 via ALPN negotiation");
        jb.key("alpn_protocols").value("h2, http/1.1");
        jb.key("user_agent").value(req.user_agent());
        jb.key("client_ip").value(req.client_ip());
        jb.end_object();
        res.body = jb.build()->to_string();
        res.set_content_type(http_content::JSON_APP());
    });

    // Session 数据查看
    router.get("/api/session", [](http_request& req, http_response& res) {
        json_builder jb;
        jb.begin_object();

        if (req.has_session()) {
            auto* sess = req.session;
            jb.key("session_id").value(sess->id);
            jb.key("is_new").value(sess->is_new);
            jb.key("max_age").value(static_cast<double>(sess->max_age.count()));
            jb.key("data").value(sess->data);
        } else {
            jb.key("error").value("No session available");
        }

        jb.end_object();
        res.body = jb.build()->to_string();
        res.set_content_type(http_content::JSON_APP());
    });

    // POST Echo
    router.post("/api/echo", [](http_request& req, http_response& res) {
        json_builder jb;
        jb.begin_object();
        jb.key("echo").value(req.body);
        jb.key("content_type").value(req.content_type());
        jb.key("size").value(static_cast<double>(req.body.size()));
        jb.end_object();
        res.body = jb.build()->to_string();
        res.set_content_type(http_content::JSON_APP());
    });

    // =========================================================================
    // 错误处理器
    // =========================================================================

    router.set_method_not_allowed_handler([](http_request& req, http_response& res) {
        res.status = http_status::S4_METHOD_NOT_ALLOWED;
        json_builder jb;
        jb.begin_object();
        jb.key("error").value("Method Not Allowed");
        jb.key("method").value(req.method.to_string());
        jb.key("path").value(req.path);
        jb.end_object();
        res.body = jb.build()->to_string();
        res.set_content_type(http_content::JSON_APP());
    });

    router.set_not_found_handler([](http_request& req, http_response& res) {
        res.status = http_status::S4_NOT_FOUND;
        json_builder jb;
        jb.begin_object();
        jb.key("error").value("Not Found");
        jb.key("path").value(req.path);
        jb.end_object();
        res.body = jb.build()->to_string();
        res.set_content_type(http_content::JSON_APP());
    });

    // =========================================================================
    // 启动
    // =========================================================================
    if (server.start()) {
        printfln("HTTPS Server started on https://localhost:{}", static_cast<uint16_t>(server.port()));
        println("  Supports: HTTP/1.1 and HTTP/2 (h2 via ALPN)");
        println("\n  Test with HTTP/1.1:");
        printfln("    curl -k https://localhost:{}/api/hello", static_cast<uint16_t>(server.port()));
        println("\n  Test with HTTP/2:");
        printfln("    curl --http2 -k https://localhost:{}/api/info", static_cast<uint16_t>(server.port()));
        println("\n  Session management is enabled — all requests get a session cookie.");
        println("  Request headers are logged to console.");

        while (true) {
            this_thread::sleep_for(10_ms);
        }

        server.stop();
        println("Server stopped");
    } else {
        println("Failed to start HTTPS server");
        return 1;
    }

    return 0;
}
