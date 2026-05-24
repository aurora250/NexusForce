/**
 * @example http_server.cpp
 * @brief HTTP服务器示例
 *
 * 演示 http_server 的核心功能：
 * - 路由注册（GET/POST/PUT/DELETE，路径参数，正则路由）
 * - 静态文件服务
 * - 会话管理（Cookie-based session）
 * - 过滤器（中间件）
 * - JSON响应（使用json_builder）
 * - SSL/TLS支持
 */

#include <NeForce/core/file/json/json_builder.hpp>
#include <NeForce/core/system/console.hpp>
#include <NeForce/network/http/http_server.hpp>

using namespace neforce;
using namespace neforce::http;

int main() {
    http_server server(ports(8080u));

    auto& router = server.router();

    // ========== 基础路由 ==========
    router.get("/", [](http_request& req, http_response& res) {
        res.body = "<h1>NexusForce HTTP Server</h1>"
                   "<p>Routes:</p>"
                   "<ul>"
                   "<li>GET /api/hello</li>"
                   "<li>GET /api/users/:id</li>"
                   "<li>POST /api/echo</li>"
                   "<li>GET /api/headers</li>"
                   "</ul>";
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

    // GET Echo：显示使用说明
    router.get("/api/echo", [](http_request& req, http_response& res) {
        json_builder jb;
        jb.begin_object();
        jb.key("message").value("This endpoint accepts POST requests");
        jb.key("usage").value("curl -X POST http://localhost:8080/api/echo -d 'hello'");
        jb.end_object();
        res.body = jb.build()->to_string();
        res.set_content_type(http_content::JSON_APP());
    });

    // POST Echo：返回请求体内容
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
        jb.begin_object();
        for (const auto& [key, value]: req.headers) {
            jb.key(key).value(value);
        }
        jb.end_object();
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

    // ========== 表单处理 ==========
    router.get("/form", [](http_request& req, http_response& res) {
        res.body = R"(
<form method="POST" action="/api/echo">
  <input name="message" placeholder="Type something...">
  <button type="submit">Send</button>
</form>)";
        res.set_content_type(http_content::HTML_TEXT());
    });

    // ========== 查询参数 ==========
    router.get("/api/greet", [](http_request& req, http_response& res) {
        // 从原始query字符串解析name参数
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

    // ========== 405处理器 ==========
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

    // ========== 404处理器 ==========
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

    // 启动服务器
    if (server.start()) {
        printfln("HTTP Server started on http://localhost:{}", static_cast<uint16_t>(server.port()));
        println("Try these endpoints:");
        printfln("  GET  http://localhost:{}/", static_cast<uint16_t>(server.port()));
        printfln("  GET  http://localhost:{}/api/hello", static_cast<uint16_t>(server.port()));
        printfln("  GET  http://localhost:{}/api/users/42", static_cast<uint16_t>(server.port()));
        printfln("  POST http://localhost:{}/api/echo", static_cast<uint16_t>(server.port()));
        printfln("  GET  http://localhost:{}/api/headers", static_cast<uint16_t>(server.port()));
        printfln("  GET  http://localhost:{}/api/greet?name=NexusForce", static_cast<uint16_t>(server.port()));
        printfln("  GET  http://localhost:{}/form", static_cast<uint16_t>(server.port()));
        console.pause("\nPress any key to stop...");
        server.stop();
        println("Server stopped");
    } else {
        println("Failed to start HTTP server");
        return 1;
    }

    return 0;
}
