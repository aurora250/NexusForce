/**
 * @example http_client.cpp
 * @brief HTTP客户端示例
 *
 * 演示 http_client 的核心功能：
 * - GET/POST请求
 * - 自定义请求头
 * - 响应解析（状态码、头部、正文）
 * - Cookie管理
 * - HTTPS请求（跳过证书验证）
 * - 重定向处理
 * - 超时配置
 */

#include <NeForce/core/system/console.hpp>
#include <NeForce/network/http/http_client.hpp>

using namespace neforce;
using namespace neforce::http;

int main() {
    http_client client;

    // 配置DNS服务器（中国大陆需使用国内DNS）
    client.get_client().set_dns_server(
            dns_client::config{.server = "114.114.114.114", .port = ports::DNS, .timeout = milliseconds(5000)});

    // ========== GET请求 ==========
    println("=== GET Request ===");
    {
        auto response = client.get("http://httpbin.org/get?hello=world");
        printfln("Status: {}", static_cast<uint16_t>(response.status));
        printfln("Body: {}", response.body.view(0, 200));

        // 遍历响应头
        for (const auto& [key, values]: response.headers) {
            for (const auto& value: values) {
                printfln("  Header: {} = {}", key, value);
            }
        }
    }

    // ========== 自定义请求头 ==========
    println("\n=== Custom Headers ===");
    {
        http_client_request req;
        req.method = http_method::GET();
        req.host = "httpbin.org";
        req.port = ports(80u);
        req.path = "/headers";
        req.set_header("X-Custom-Header", "NexusForce-Example");
        req.set_header("User-Agent", "NexusForce/1.0");

        auto response = client.request(req);
        printfln("Status: {}", static_cast<uint16_t>(response.status));
        printfln("Body: {}", response.body.view(0, 300));
    }

    // ========== POST JSON ==========
    println("\n=== POST JSON ===");
    {
        string json = R"({"name": "NexusForce", "type": "example"})";
        auto response = client.post_json("http://httpbin.org/post", json, {});
        printfln("Status: {}", static_cast<uint16_t>(response.status));
        printfln("Body: {}", response.body.view(0, 300));
    }

    // ========== POST表单 ==========
    println("\n=== POST Form ===");
    {
        auto response =
                client.post_form("http://httpbin.org/post", {{"username", "admin"}, {"password", "secret"}}, {});
        printfln("Status: {}", static_cast<uint16_t>(response.status));
        printfln("Body: {}", response.body.view(0, 300));
    }

    // ========== 重定向处理 ==========
    println("\n=== Redirect ===");
    {
        client.set_follow_redirects(true);
        client.set_max_redirects(5);

        auto response = client.get("http://httpbin.org/redirect/2");
        printfln("Status: {}", static_cast<uint16_t>(response.status));
        printfln("Final body (truncated): {}", response.body.view(0, 100));
    }

    // ========== HTTPS请求 ==========
    println("\n=== HTTPS Request ===");
    {
        // 跳过证书验证（仅用于测试）
        client.set_verify_ssl(false);

        auto response = client.get("https://httpbin.org/json");
        printfln("Status: {}", static_cast<uint16_t>(response.status));
        if (response.is_success()) {
            printfln("Body: {}", response.body.head(200), "...");
        }
    }

    // ========== 超时配置 ==========
    println("\n=== Timeout Config ===");
    {
        client.set_timeout(milliseconds(10000));

        auto cfg = client.get_config();
        printfln("Connect timeout: {}ms", cfg.connect_timeout.count());
        printfln("Send timeout: {}ms", cfg.send_timeout.count());
        printfln("Receive timeout: {}ms", cfg.receive_timeout.count());
    }

    println("\nAll HTTP client examples completed");
    return 0;
}
