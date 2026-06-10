/**
 * @example http_server.cpp
 * @brief HTTP服务器综合示例
 *
 * 演示 http_server 核心功能：
 * - 路由注册（GET/POST/PUT/DELETE，路径参数，正则路由）
 * - 会话管理（Cookie-based session，计数器，登录）
 * - CSRF防护（Double-Submit Cookie模式）
 * - HTTP响应压缩（gzip/deflate）
 * - 静态文件服务 + Range请求支持
 * - 过滤器链（CORS、日志、限流）
 * - JSON响应（使用json_builder）
 * - 表单处理 + 查询参数
 * - 405/404处理器
 */

#include <NeForce/core/file/json/json_builder.hpp>
#include <NeForce/core/system/console.hpp>
#include <NeForce/network/http/csrf_filter.hpp>
#include <NeForce/network/http/http_compress.hpp>
#include <NeForce/network/http/http_server.hpp>
#include <NeForce/network/http/rate_limiter.hpp>

using namespace neforce;
using namespace neforce::http;

int main() {
    http_server server(ports(8080u));

    auto& router = server.router();

    // =========================================================================
    // 过滤器链（中间件）
    // =========================================================================

    // 1. CORS 跨域支持
    auto cors = make_unique<cors_filter>("*");
    router.use(move(cors));

    // 2. 限流 — IP 级别令牌桶（10 req/s，突发 20）
    auto rate_limit = make_unique<token_bucket_filter>(10.0, 20.0);
    router.use(move(rate_limit));

    // 3. CSRF 防护 — 状态变更方法验证token
    auto csrf = make_unique<csrf_filter>();
    csrf->cookie_name = "XSRF-TOKEN";
    csrf->token_max_age = seconds(3600);
    router.use(move(csrf));

#ifdef NEFORCE_SUPPORT_ZLIB
    // 4. HTTP 响应压缩 — 自动选择gzip或deflate
    auto compress = make_unique<compress_filter>();
    compress->min_size = 1_KB;
    router.use(move(compress));
#endif

    // 5. 静态文件服务 — 支持Range请求（断点续传）
    auto static_files = make_unique<static_file_filter>("./public");
    static_files->set_enable_range(true);
    router.use(move(static_files));

    // 6. Bearer Token 认证 — 仅保护 /api/protected
    auto auth = make_unique<authentication_filter>();
    auth->set_auth_validator([](const http_request& request) -> bool {
        const string auth_header = request.header("Authorization");
        if (auth_header.starts_with("Bearer ")) {
            return auth_header.view(7) == "nexusforce-demo-token";
        }
        return false;
    });
    auth->add_included_path("/api/protected");
    router.use(move(auth));

    // =========================================================================
    // 基础路由
    // =========================================================================

    router.get("/", [](http_request& req, http_response& res) {
        // 会话计数器
        int visit_count = 0;
        if (req.has_session()) {
            auto* sess = req.session;
            const auto visits_str = sess->get("visits");
            visit_count = visits_str.empty() ? 0 : to_int32(visits_str);
            visit_count++;
            sess->set("visits", to_string(visit_count));
        }

        res.body = "<h1>NexusForce HTTP Server</h1>"
                   "<p>Your visits: " +
                   to_string(visit_count) +
                   "</p>"
                   "<ul>"
                   "<li>GET <a href='/api/hello'>/api/hello</a></li>"
                   "<li>GET <a href='/api/users/42'>/api/users/:id</a></li>"
                   "<li>POST /api/echo</li>"
                   "<li>GET <a href='/api/session'>/api/session</a></li>"
                   "<li>GET <a href='/api/headers'>/api/headers</a></li>"
                   "<li>GET <a href='/api/greet?name=NexusForce'>/api/greet?name=NexusForce</a></li>"
                   "<li>GET <a href='/api/protected'>/api/protected</a> (Bearer token: "
                   "<code>nexusforce-demo-token</code>)</li>"
                   "<li>GET <a href='/form'>/form</a></li>"
                   "<li>GET <a href='/public/'>/public/*</a> (static files + Range)</li>"
                   "</ul>"
                   "<p><small>Rate limited: 10 req/s per IP</small></p>";
        res.set_content_type(http_content::HTML_TEXT());
    });

    // GET JSON API
    router.get("/api/hello", [](http_request& req, http_response& res) {
        json_builder jb;
        jb.begin_object();
        jb.key("message").value("Hello from NexusForce!");
        jb.key("status").value("ok");
        jb.end_object();
        res.body = jb.build()->to_string();
        res.set_content_type(http_content::JSON_APP());
    });

    // 路径参数：/api/users/42 → id=42
    router.get("/api/users/:id", [](http_request& req, http_response& res) {
        string_view user_id = req.parameter("id");

        json_builder jb;
        jb.begin_object();
        jb.key("user_id").value(user_id);
        jb.key("name").value("User " + string(user_id));
        jb.end_object();
        res.body = jb.build()->to_string();
        res.set_content_type(http_content::JSON_APP());
    });

    // =========================================================================
    // 会话管理
    // =========================================================================

    // 读取/设置会话数据
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

    // 登录（设置session用户信息 + 防御Session Fixation）
    router.post("/api/login", [](http_request& req, http_response& res) {
        if (!req.has_session()) {
            res.status = http_status::S5_INTERNAL_SERVER_ERROR;
            res.body = R"({"error":"Session required"})";
            res.set_content_type(http_content::JSON_APP());
            return;
        }

        auto* sess = req.session;
        auto it = req.form_data.find("username");
        const string username = (it != req.form_data.end()) ? it->second : "anonymous";
        sess->set("user", username);
        sess->regenerate_id(); // 防止Session Fixation

        json_builder jb;
        jb.begin_object();
        jb.key("status").value("ok");
        jb.key("user").value(username);
        jb.key("session_id").value(sess->id);
        jb.end_object();
        res.body = jb.build()->to_string();
        res.set_content_type(http_content::JSON_APP());
    });

    // GET Echo：使用说明
    router.get("/api/echo", [](http_request& req, http_response& res) {
        json_builder jb;
        jb.begin_object();
        jb.key("message").value("This endpoint accepts POST requests");
        jb.key("usage").value("curl -X POST http://localhost:8080/api/echo -d 'hello'");
        jb.end_object();
        res.body = jb.build()->to_string();
        res.set_content_type(http_content::JSON_APP());
    });

    // POST Echo：返回请求体（CSRF保护自动验证）
    router.post("/api/echo", [](http_request& req, http_response& res) {
        json_builder jb;
        jb.begin_object();
        jb.key("echo").value(req.body);
        jb.key("content_type").value(req.content_type());
        jb.end_object();
        res.body = jb.build()->to_string();
        res.set_content_type(http_content::JSON_APP());
    });

    // 查看请求头
    router.get("/api/headers", [](http_request& req, http_response& res) {
        json_builder jb;
        jb.value(req.headers);
        res.body = jb.build()->to_string();
        res.set_content_type(http_content::JSON_APP());
    });

    // 正则路由：匹配 /api/v1/ 下的所有路径
    router.get("/api/v1/(.*)", [](http_request& req, http_response& res) {
        string_view sub_path = req.parameter("1");

        json_builder jb;
        jb.begin_object();
        jb.key("version").value("v1");
        jb.key("path").value(sub_path);
        jb.end_object();
        res.body = jb.build()->to_string();
        res.set_content_type(http_content::JSON_APP());
    });

    // =========================================================================
    // 表单处理（CSRF保护）
    // =========================================================================

    router.get("/form", [](http_request& req, http_response& res) {
        res.body = R"(
<form method="POST" action="/api/echo">
  <input type="hidden" name="_csrf" id="csrf_token">
  <input name="message" placeholder="Type something...">
  <button type="submit">Send</button>
</form>
<script>
// 从Cookie读取CSRF token填入隐藏字段
var csrf = document.cookie.split('; ').find(r => r.startsWith('XSRF-TOKEN='));
if (csrf) document.getElementById('csrf_token').value = csrf.split('=')[1];
</script>)";
        res.set_content_type(http_content::HTML_TEXT());
    });

    // =========================================================================
    // 查询参数
    // =========================================================================

    router.get("/api/greet", [](http_request& req, http_response& res) {
        string name = "World";
        string_view query = req.query.view();
        if (query.starts_with("name=")) {
            name = string(query.view(5));
        } else if (query.contains("&name=")) {
            auto pos = query.find("&name=");
            auto end = query.find("&", pos + 1);
            name = string(query.view(pos + 6, end == string::npos ? query.size() : end));
        }

        json_builder jb;
        jb.begin_object();
        jb.key("greeting").value("Hello, " + name);
        jb.end_object();
        res.body = jb.build()->to_string();
        res.set_content_type(http_content::JSON_APP());
    });

    // =========================================================================
    // 受保护的 API (需 Bearer token)
    // =========================================================================

    router.get("/api/protected", [](http_request& req, http_response& res) {
        json_builder jb;
        jb.begin_object();
        jb.key("access").value("granted");
        jb.key("message").value("You accessed the protected endpoint");
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
    // 启动服务器
    // =========================================================================

    if (server.start()) {
        printfln("HTTP Server started on http://localhost:{}", static_cast<uint16_t>(server.port()));
        println("Features: Session, CSRF, Gzip/Deflate, Range, Static Files");
        println("Try these endpoints:");
        printfln("  GET  http://localhost:{}/                (session counter)", static_cast<uint16_t>(server.port()));
        printfln("  GET  http://localhost:{}/api/hello", static_cast<uint16_t>(server.port()));
        printfln("  GET  http://localhost:{}/api/users/42", static_cast<uint16_t>(server.port()));
        printfln("  POST http://localhost:{}/api/echo", static_cast<uint16_t>(server.port()));
        printfln("  GET  http://localhost:{}/api/session", static_cast<uint16_t>(server.port()));
        printfln("  GET  http://localhost:{}/api/headers", static_cast<uint16_t>(server.port()));
        printfln("  GET  http://localhost:{}/api/greet?name=NexusForce", static_cast<uint16_t>(server.port()));
        printfln("  GET  http://localhost:{}/form", static_cast<uint16_t>(server.port()));
        printfln("  GET  http://localhost:{}/public/ (static files + Range)", static_cast<uint16_t>(server.port()));

        while (true) {
            this_thread::sleep_for(10_ms);
        }

        server.stop();
        println("Server stopped");
    } else {
        println("Failed to start HTTP server");
        return 1;
    }

    return 0;
}
