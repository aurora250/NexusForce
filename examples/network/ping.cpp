/**
 * @example ping.cpp
 * @brief ICMP Ping/Traceroute示例
 *
 * 演示 icmp_socket 的核心功能：
 * - ICMP Echo请求（Ping）
 * - 网络路径追踪（Traceroute）
 * - RTT测量
 * - TTL控制
 *
 * @note Linux需要root权限运行原始ICMP套接字。使用 sudo ./NexusForcePingExample
 */

#include <NeForce/core/system/console.hpp>
#include <NeForce/network/icmp_socket.hpp>
#include <NeForce/network/util/ip_address.hpp>

using namespace neforce;

int main() {
    // 解析目标地址
    const string target = "8.8.8.8";
    auto dest = ip_address::parse(target);
    if (!dest) {
        printfln("Failed to parse address: {}", target);
        return 1;
    }

    icmp_socket sock;
    sock.open();

    // 打开ICMP原始套接字（需要root权限）
    if (!sock.is_open()) {
        println("Failed to open ICMP socket. Try running with sudo.");
        return 1;
    }

    // ========== Ping ==========
    printfln("=== Ping {} ===\n", target);
    {
        int sent = 0;
        int received = 0;

        for (int i = 0; i < 4; ++i) {
            auto result = sock.ping(*dest, milliseconds(2000));

            if (result.success) {
                printfln("{} bytes from {}: icmp_seq={} ttl={} time={}ms", result.reply_size,
                         result.destination.to_string(), i + 1, result.reply_ttl, result.rtt.count());
                ++received;
            } else {
                printfln("Request timeout for icmp_seq={}", i + 1);
            }
            ++sent;
        }

        printfln("\n--- {} ping statistics ---", target);
        printfln("{} packets transmitted, {} received, {}% loss", sent, received, (sent - received) * 100 / sent);
    }

    // ========== Traceroute ==========
    printfln("\n=== Traceroute to {} ===\n", target);

    using neforce::printf;

    {
        auto hops = sock.traceroute(*dest, 30, milliseconds(2000), 3);

        for (size_t i = 0; i < hops.size(); ++i) {
            const auto& hop = hops[i];
            printf("{}  ", i + 1);

            if (!hop.address.is_valid()) {
                println("* * *");
            } else {
                printf("{}  ", hop.address.to_string());

                for (const auto& rtt: hop.rtt) {
                    printf("{:.1f}ms  ", rtt.count());
                }
                printfln("{}", hop.reached ? "(reached)" : "");
            }

            if (hop.reached) {
                break;
            }
        }
    }

    println("\nAll ICMP examples completed");
    return 0;
}
