/**
 * @example https_server.cpp
 * @brief HTTPS服务器示例
 *
 * 演示 http_server 的HTTPS功能：
 * - 加载SSL/TLS证书
 * - 创建HTTPS服务器
 * - 路由注册（同HTTP）
 * - 对比HTTP vs HTTPS
 *
 * 运行前需生成自签名证书：
 *   openssl req -x509 -newkey rsa:2048 -keyout server.key -out server.crt -days 365 -nodes -subj '/CN=localhost'
 *
 * 测试：
 *   curl -k https://localhost:8443/api/hello
 */

#include <NeForce/core/file/json/json_builder.hpp>
#include <NeForce/core/system/console.hpp>
#include <NeForce/network/http/http_server.hpp>
#include <NeForce/network/ssl/ssl_context.hpp>

using namespace neforce;
using namespace neforce::http;

int main() {
    // ========== 加载SSL证书 ==========
    ssl_context ctx(ssl_method::TLS_SERVER);

    if (!ctx.load_certificate("server.crt", "server.key")) {
        println("Failed to load certificate. Generate with:");
        println("  openssl req -x509 -newkey rsa:2048 -keyout server.key -out server.crt -days 365 -nodes -subj "
                "'/CN=localhost'");
        return 1;
    }
    println("Certificate loaded successfully");

    // ========== 创建HTTPS服务器 ==========
    // 使用 ssl_context 构造即启用TLS
    http_server server(ports(8443u), move(ctx));

    auto& router = server.router();

    // 首页
    router.get("/", [](http_request& req, http_response& res) {
        res.body = "<h1>NexusForce HTTPS Server</h1>"
                   "<p>You are connected over a secure TLS connection.</p>"
                   "<ul>"
                   "<li>GET /api/hello</li>"
                   "<li>GET /api/users/:id</li>"
                   "<li>GET /api/info</li>"
                   "</ul>";
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
        string_view user_id = req.parameter("id");

        json_builder jb;
        jb.begin_object();
        jb.key("user_id").value(user_id);
        jb.key("name").value("User " + string(user_id));
        jb.key("tls").value(true);
        jb.end_object();
        res.body = jb.build()->to_string();
        res.set_content_type(http_content::JSON_APP());
    });

    // 连接信息
    router.get("/api/info", [](http_request& req, http_response& res) {
        json_builder jb;
        jb.begin_object();
        jb.key("protocol").value("HTTPS (HTTP over TLS)");
        jb.key("user_agent").value(req.user_agent());
        jb.key("client_ip").value(req.client_ip());
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

    // ========== 启动服务器 ==========
    if (server.start()) {
        printfln("HTTPS Server started on https://localhost:{}", static_cast<uint16_t>(server.port()));
        println("\nTry these endpoints:");
        printfln("  curl -k https://localhost:{}/", static_cast<uint16_t>(server.port()));
        printfln("  curl -k https://localhost:{}/api/hello", static_cast<uint16_t>(server.port()));
        printfln("  curl -k https://localhost:{}/api/users/42", static_cast<uint16_t>(server.port()));
        printfln("  curl -k https://localhost:{}/api/info", static_cast<uint16_t>(server.port()));
        console.pause("\nPress any key to stop...");
        server.stop();
        println("Server stopped");
    } else {
        println("Failed to start HTTPS server");
        return 1;
    }

    return 0;
}
