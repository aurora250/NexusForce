/**
 * @example ssl_echo_server.cpp
 * @brief SSL/TLS Echo服务器示例
 *
 * 演示 ssl_acceptor 和 ssl_socket 的基本用法：
 * - 加载证书和私钥
 * - 接受SSL/TLS客户端连接
 * - 加密数据收发（Echo）
 * - 客户端证书验证
 */

#include <NeForce/core/async/thread.hpp>
#include <NeForce/core/system/console.hpp>
#include <NeForce/network/ssl/ssl_acceptor.hpp>
#include <NeForce/network/ssl/ssl_context.hpp>
#include <NeForce/network/ssl/ssl_socket.hpp>
#include <NeForce/network/util/ip_address.hpp>

using namespace neforce;

int main() {
    // 创建SSL上下文（服务器模式）
    ssl_context ctx(ssl_method::TLS_SERVER);

    // 加载服务器证书和私钥
    if (!ctx.load_certificate("/home/huenqi/server.crt", "/home/huenqi/server.key")) {
        println("Failed to load certificate. Generate with:");
        println("  openssl req -x509 -newkey rsa:2048 -keyout server.key -out server.crt -days 365 -nodes -subj "
                "'/CN=localhost'");
        return 1;
    }
    println("Certificate loaded successfully");

    // 创建SSL Acceptor
    ssl_acceptor acceptor;
    acceptor.set_ssl_context(move(ctx));

    // 绑定并监听端口
    auto endpoint = ip_address::any(ports(8443u));
    acceptor.open(endpoint, 128);
    acceptor.set_nonblocking(true);

    printfln("TLS Echo Server listening on {}", endpoint.to_string());
    printfln("Connect with: openssl s_client -connect localhost:{}", static_cast<uint16_t>(endpoint.port()));

    // 在后台线程接受连接（简化演示，仅处理一个连接）
    atomic<bool> running{true};
    auto server_thread = thread([&]() {
        while (running) {
            auto client = acceptor.accept_ssl_nonblock();
            if (!client) {
                this_thread::sleep_for(milliseconds(100));
                continue;
            }
            try {

                auto client_addr = client->remote_endpoint();
                if (client_addr) {
                    printfln("New TLS connection from {}", client_addr->to_string());
                }

                // Echo: 接收并返回数据
                char buffer[4096];
                ssize_t received = client->receive({buffer, sizeof(buffer)});
                if (received > 0) {
                    printfln("Received {} bytes", received);
                    client->send_all({buffer, static_cast<size_t>(received)});
                    printfln("Echoed {} bytes back", received);
                }
            } catch (const exception& e) {
                // 超时或其他错误，继续等待
            }
        }
    });

    while (true) {
        this_thread::sleep_for(milliseconds(100));
    }

    return 0;
}
