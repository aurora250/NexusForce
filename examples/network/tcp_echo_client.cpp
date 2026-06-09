/**
 * @example tcp_echo_client.cpp
 * @brief TCP Echo客户端示例
 *
 * 演示 tcp_client 的基本用法：
 * - 连接、发送、接收、断开
 * - 超时设置
 * - 异常处理
 */

#include <NeForce/core/system/console.hpp>
#include <NeForce/network/tcp/tcp_client.hpp>

using namespace neforce;

int main() {
    tcp_client client;

    // 配置超时时间
    client.set_connect_timeout(milliseconds(5000));
    client.set_send_timeout(milliseconds(5000));
    client.set_recv_timeout(milliseconds(5000));

    // 设置异常处理器
    client.set_exception_handler([](const exception& e) { printfln("Client error: {}", e.what()); });

    // 连接到服务器
    const string host = "127.0.0.1";
    const ports port(8080u);

    printfln("Connecting to {}:{}...", host, static_cast<uint16_t>(port));
    if (!client.connect(host, port)) {
        println("Failed to connect");
        return 1;
    }
    println("Connected!");

    // 发送消息
    const string msg = "Hello from NexusForce TCP Client!";
    const ssize_t sent = client.send(msg.data(), msg.size());
    printfln("Sent {} bytes: {}", sent, msg);

    // 接收响应
    char buffer[4096];
    const ssize_t received = client.receive(buffer, sizeof(buffer));
    if (received > 0) {
        printfln("Received {} bytes: {}", received, string_view(buffer, static_cast<size_t>(received)));
    }

    client.disconnect();
    println("Disconnected");
    return 0;
}
