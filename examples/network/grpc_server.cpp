/**
 * @example grpc_server.cpp
 * @brief gRPC Unary 服务示例
 *
 * 演示 gRPC 协议核心功能：
 * - gRPC 帧编解码（grpc_framer）
 * - Unary RPC 处理（grpc_handler::process_unary）
 * - gRPC 状态码映射（grpc_to_http_status）
 * - HTTP/1.1 承载 gRPC 流量
 * - 客户端测试请求（手写 gRPC 帧）
 *
 * 使用方式：
 *   启动服务器:  ./NexusForceGrpcServerExample
 *   gRPC 测试:   printf '\x00\x00\x00\x00\x05Hello' | curl -s -X POST \
 *                  http://localhost:8080/helloworld.Greeter/SayHello \
 *                  -H 'Content-Type: application/grpc' --data-binary @- --output - | xxd
 *                （gRPC 帧: [无压缩][长度=5][payload="Hello"]）
 */

#include <NeForce/core/file/json/json_builder.hpp>
#include <NeForce/core/system/console.hpp>
#include <NeForce/network/http/grpc.hpp>
#include <NeForce/network/http/http_server.hpp>
#include <NeForce/network/tcp/tcp_client.hpp>
using namespace neforce;
using namespace neforce::http;

int main() {
    io_context context;

    http_server server(ports(8080u), context);

    auto& router = server.router();

    // =========================================================================
    // Greeter 服务 — SayHello RPC
    // =========================================================================

    router.post("/helloworld.Greeter/SayHello", [](http_request& req, http_response& res) {
        // 验证 Content-Type
        if (!req.content_type().view(0).starts_with("application/grpc")) {
            res.status = http_status::S4_UNSUPPORTED_MEDIA_TYPE;
            res.body = "Content-Type must be application/grpc";
            return;
        }

        // 解码 gRPC 请求帧
        grpc_framer framer;
        vector<grpc_message> messages;
        const int decoded = framer.decode(reinterpret_cast<const byte_t*>(req.body.data()), req.body.size(), messages);

        if (decoded < 0) {
            grpc_handler::send_error(res, grpc_status::INVALID_ARGUMENT, "Frame too large");
            return;
        }
        if (messages.empty()) {
            grpc_handler::send_error(res, grpc_status::INVALID_ARGUMENT, "No message received");
            return;
        }

        // 提取请求名称
        const string name(reinterpret_cast<const char*>(messages[0].payload.data()), messages[0].payload.size());

        // 构造响应
        string greeting = "Hello, " + name + "! (from NexusForce gRPC)";
        grpc_message response_msg;
        response_msg.payload.assign(greeting.begin(), greeting.end());

        // 编码 gRPC 响应帧
        const auto encoded = grpc_framer::encode(response_msg);
        res.body = string(reinterpret_cast<const char*>(encoded.data()), encoded.size());
        res.set_content_type("application/grpc");

        // 设置 gRPC trailers（状态码 0 = OK）
        res.trailers["grpc-status"] = "0";
        res.trailers["grpc-message"] = "";
    });

    // =========================================================================
    // 健康检查 RPC（演示错误状态码）
    // =========================================================================

    router.post("/grpc.health.v1.Health/Check", [](http_request& req, http_response& res) {
        grpc_framer framer;
        vector<grpc_message> messages;
        const int decoded = framer.decode(reinterpret_cast<const byte_t*>(req.body.data()), req.body.size(), messages);

        if (decoded <= 0 || messages.empty()) {
            grpc_handler::send_error(res, grpc_status::INVALID_ARGUMENT, "Invalid request");
            return;
        }

        const string service(reinterpret_cast<const char*>(messages[0].payload.data()), messages[0].payload.size());

        // 简单的健康检查逻辑
        grpc_message response_msg;
        string status_json;

        if (service.empty() || service == "helloworld.Greeter") {
            // 服务可用
            status_json = R"({"status":"SERVING"})";
        } else {
            // 未找到服务 → NOT_FOUND
            grpc_handler::send_error(res, grpc_status::NOT_FOUND, "Service not found");
            return;
        }

        response_msg.payload.assign(status_json.begin(), status_json.end());
        const auto encoded = grpc_framer::encode(response_msg);
        res.body = string(reinterpret_cast<const char*>(encoded.data()), encoded.size());
        res.set_content_type("application/grpc");
        res.trailers["grpc-status"] = "0";
        res.trailers["grpc-message"] = "";
    });

    // =========================================================================
    // gRPC 客户端测试端点（HTTP JSON，方便 curl 测试）
    // =========================================================================

    router.get("/test/sayhello", [](http_request& req, http_response& res) {
        string_view name = req.query.view();
        if (name.starts_with("name=")) {
            name = name.view(5);
        } else {
            name = "World";
        }

        // 使用 grpc_framer 手动构造展示 gRPC 请求帧格式
        grpc_message request_msg;
        request_msg.payload.assign(name.data(), name.data() + name.size());

        const auto frame = grpc_framer::encode(request_msg);

        json_builder jb;
        jb.begin_object();
        jb.key("name").value(string(name));
        jb.key("frame_size").value(static_cast<double>(frame.size()));
        jb.key("curl_example")
                .value("printf '\\x00\\x00\\x00\\x00\\x05" + string(name) +
                       "' | curl -s -X POST http://localhost:8080/helloworld.Greeter/SayHello "
                       "-H 'Content-Type: application/grpc' --data-binary @- --output - | xxd");
        jb.end_object();
        res.body = jb.build()->to_string();
        res.set_content_type(http_content::JSON_APP());
    });

    // =========================================================================
    // 启动
    // =========================================================================

    if (server.start()) {
        printfln("gRPC Server started on http://localhost:{}", static_cast<uint16_t>(server.port()));
        println();
        println("=== Test the gRPC Greeter service ===");
        println();
        println("1. Browser test (generates curl command):");
        printfln("   http://localhost:{}/test/sayhello?name=NexusForce", static_cast<uint16_t>(server.port()));
        println();
        println("2. Direct gRPC frame (piped binary):");
        println("   printf '\\x00\\x00\\x00\\x00\\x05Hello' | curl -s -X POST \\");
        println("     http://localhost:8080/helloworld.Greeter/SayHello \\");
        println("     -H 'Content-Type: application/grpc' --data-binary @- --output - | xxd");
        println();
        println("3. Health check:");
        println("   printf '\\x00\\x00\\x00\\x00\\x12helloworld.Greeter' | curl -s -X POST \\");
        println("     http://localhost:8080/grpc.health.v1.Health/Check \\");
        println("     -H 'Content-Type: application/grpc' --data-binary @- --output - | xxd");
        println();

        while (true) {
            this_thread::sleep_for(10_ms);
        }

        server.stop();
        println("Server stopped");
    } else {
        println("Failed to start gRPC server");
        return 1;
    }

    return 0;
}
