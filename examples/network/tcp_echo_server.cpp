/**
 * @example tcp_echo_server.cpp
 * @brief TCP Echo服务器示例
 *
 * 演示 tcp_server 的基本用法：
 * - 创建服务器并设置客户端处理器
 * - 接收客户端消息并原样返回（Echo）
 * - 设置异常处理器
 * - 启动和停止服务器
 */

#include <NeForce/core/system/console.hpp>
#include <NeForce/network/tcp/tcp_server.hpp>
#include <NeForce/network/tcp/tcp_socket.hpp>

using namespace neforce;

int main() {
    // 创建TCP服务器，监听端口8080，使用4个工作线程
    tcp_server server(ports(8080u), 4);

    // 设置客户端处理器：接收数据并原样返回
    server.set_client_handler([](unique_ptr<tcp_socket> client) {
        try {
            char buffer[4096];
            const ssize_t received = client->receive({buffer, sizeof(buffer)});
            if (received > 0) {
                client->send_all({buffer, static_cast<size_t>(received)});
            }
            client->close();
        } catch (const exception& e) {
            eprintfln("Client error: {}", e.what());
        }
    });

    // 设置异常处理器
    server.set_exception_handler([](const exception& e) { printfln("Server exception: {}", e.what()); });

    // 启动服务器
    if (server.start()) {
        printfln("TCP Echo Server started on port {}", static_cast<uint16_t>(server.port()));
        printfln("Connect with: telnet localhost {}", static_cast<uint16_t>(server.port()));
        console.pause("Press any key to stop...");
        server.stop();
        println("Server stopped");
    } else {
        println("Failed to start server");
        return 1;
    }

    return 0;
}
