/**
 * @example ssl_echo_client.cpp
 * @brief SSL/TLS Echo客户端示例
 *
 * 演示 ssl_socket 的客户端用法：
 * - 创建SSL/TLS客户端连接
 * - 证书验证（可选跳过）
 * - 加密数据收发
 * - 获取对等方证书信息
 */

#include <NeForce/core/system/console.hpp>
#include <NeForce/network/ssl/ssl_context.hpp>
#include <NeForce/network/ssl/ssl_socket.hpp>
#include <NeForce/network/util/ip_address.hpp>

using namespace neforce;

int main() {
    // 创建SSL上下文（客户端模式）
    ssl_context ctx(ssl_method::TLS_CLIENT);

    // 自签名证书测试：跳过服务器证书验证
    ctx.set_verify_mode(SSL_VERIFY_NONE);

    // 创建SSL Socket
    ssl_socket client;
    client.open(AF_INET);

    // 连接到服务器
    const string host = "127.0.0.1";
    const ports port(8443u);
    auto server_addr = ip_address::parse(host, port);
    if (!server_addr) {
        println("Failed to parse address");
        return 1;
    }

    printfln("Connecting to {}:{}...", host, static_cast<uint16_t>(port));
    try {
        if (!client.connect(*server_addr, milliseconds(5000))) {
            println("Connection failed (timeout or refused)");
            return 1;
        }
    } catch (const exception& e) {
        printfln("Connection failed: {}", e.what());
        println("Make sure SslEchoServerExample is running first.");
        return 1;
    }
    println("TCP connected, starting TLS handshake...");

    // 初始化SSL客户端
    try {
        client.init_client_ssl(ctx, host);
        println("TLS handshake successful");

        // 获取服务器证书信息
        auto cert_info = client.peer_certificate_info();
        if (!cert_info.empty()) {
            printfln("Server certificate:\n{}", cert_info);
        }
    } catch (const exception& e) {
        printfln("TLS handshake failed: {}", e.what());
        return 1;
    }

    // 发送消息
    const string msg = "Hello from NexusForce SSL Client!";
    client.send_all({msg.data(), msg.size()});
    printfln("Sent: {}", msg);

    // 接收响应
    char buffer[4096];
    ssize_t received = client.receive({buffer, sizeof(buffer)});
    if (received > 0) {
        printfln("Received: {}", string_view(buffer, static_cast<size_t>(received)));
    }

    client.close();
    println("Connection closed");
    return 0;
}
