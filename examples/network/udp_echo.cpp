/**
 * @example udp_echo.cpp
 * @brief UDP Echo客户端/服务器示例
 *
 * 演示 udp_socket 的核心功能：
 * - UDP socket创建和绑定
 * - 无连接数据报发送（send_to）
 * - 数据报接收并获取发送方地址（receive_from）
 * - 已连接UDP模式（connect + send/receive）
 * - 简单的Echo协议实现
 */

#include <NeForce/core/async/thread.hpp>
#include <NeForce/core/system/console.hpp>
#include <NeForce/network/udp_socket.hpp>
#include <NeForce/network/util/ip_address.hpp>

using namespace neforce;

static void udp_echo_once(ports port) {
    udp_socket server;
    server.open();
    server.bind(ip_address::any(port));

    char buffer[1024];
    auto [received, client_addr] = server.receive_from({buffer, sizeof(buffer)});

    if (received > 0) {
        string_view msg(buffer, static_cast<size_t>(received));
        printfln("[Server] Received {} bytes from {}: {}", received, client_addr.to_string(), msg);
        ssize_t sent = server.send_to({buffer, static_cast<size_t>(received)}, client_addr);
        printfln("[Server] Echoed {} bytes back", sent);
    }

    server.close();
}

int main() {
    const ports port(9999u);

    // ========== 无连接模式 (send_to / receive_from) ==========
    println("--- Connectionless Mode ---");
    {
        thread server_thread(udp_echo_once, port);
        this_thread::sleep_for(milliseconds(100));

        udp_socket client;
        client.open();
        auto server_addr = ip_address::parse("127.0.0.1", port);
        if (!server_addr) {
            println("Failed to parse address");
            return 1;
        }

        const string msg = "Hello UDP from NexusForce!";
        ssize_t sent = client.send_to({msg.data(), msg.size()}, *server_addr);
        printfln("[Client] Sent {} bytes to 127.0.0.1:{}", sent, static_cast<uint16_t>(port));

        char buffer[1024];
        auto [received, from_addr] = client.receive_from({buffer, sizeof(buffer)});
        if (received > 0) {
            printfln("[Client] Received {} bytes from {}: {}", received, from_addr.to_string(),
                     string_view(buffer, static_cast<size_t>(received)));
        }

        client.close();
        server_thread.join();
    }

    // ========== 已连接模式 (connect + send/receive) ==========
    println("\n--- Connected Mode ---");
    {
        thread server_thread(udp_echo_once, port);
        this_thread::sleep_for(milliseconds(100));

        udp_socket connected_client;
        connected_client.open();
        auto server_addr = ip_address::parse("127.0.0.1", port);
        connected_client.connect(*server_addr);

        const string msg = "Hello via connected UDP!";
        ssize_t sent = connected_client.send({msg.data(), msg.size()});
        printfln("[Client] Sent {} bytes", sent);

        char buffer[1024];
        ssize_t received = connected_client.receive({buffer, sizeof(buffer)});
        if (received > 0) {
            printfln("[Client] Received {} bytes: {}", received, string_view(buffer, static_cast<size_t>(received)));
        }

        connected_client.close();
        server_thread.join();
    }

    println("\nAll UDP examples completed");
    return 0;
}
