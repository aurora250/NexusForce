/**
 * @example dns_resolver.cpp
 * @brief DNS解析器示例
 *
 * 演示 dns_client 的核心功能：
 * - 基本DNS查询（A/AAAA/MX/TXT/SOA记录）
 * - 反向DNS查询（PTR）
 * - 批量查询
 * - 自定义DNS服务器
 * - 缓存控制
 * - DNSSEC支持
 */

#include <NeForce/core/system/console.hpp>
#include <NeForce/network/dns/dns_client.hpp>

using namespace neforce;

int main() {
    io_context io_ctx;

    dns_client client(
            dns_client::config{.server = "114.114.114.114", .port = ports::DNS, .timeout = milliseconds(5000)}, io_ctx);
    println("DNS Server: 114.114.114.114");

    // ========== A记录查询（IPv4） ==========
    println("\n=== A Record (IPv4) ===");
    try {
        auto ips = client.resolve_a("www.google.com");
        for (const auto& ip: ips) {
            printfln("  IPv4: {}", ip);
        }
    } catch (const exception& e) {
        printfln("  Error: {}", e.what());
    }

    // ========== AAAA记录查询（IPv6） ==========
    println("\n=== AAAA Record (IPv6) ===");
    try {
        auto ips = client.resolve_aaaa("www.google.com");
        for (const auto& ip: ips) {
            printfln("  IPv6: {}", ip);
        }
    } catch (const exception& e) {
        printfln("  Error: {}", e.what());
    }

    // ========== MX记录查询 ==========
    println("\n=== MX Record ===");
    try {
        auto records = client.resolve_mx("gmail.com");
        for (const auto& r: records) {
            printfln("  MX: {}", r);
        }
    } catch (const exception& e) {
        printfln("  Error: {}", e.what());
    }

    // ========== TXT记录查询 ==========
    println("\n=== TXT Record ===");
    try {
        auto records = client.resolve_txt("google.com");
        for (const auto& r: records) {
            printfln("  TXT: {}", r);
        }
    } catch (const exception& e) {
        printfln("  Error: {}", e.what());
    }

    // ========== SOA记录查询 ==========
    println("\n=== SOA Record ===");
    try {
        auto soa = client.resolve_soa("google.com");
        if (soa) {
            printfln("  MName: {}", soa->mname);
            printfln("  RName: {}", soa->rname);
            printfln("  Serial: {}", soa->serial);
        }
    } catch (const exception& e) {
        printfln("  Error: {}", e.what());
    }

    // ========== 通用查询 ==========
    println("\n=== General Query ===");
    try {
        auto result = client.query("example.com", dns_record::A);
        printfln("  Status: {}", result.is_success() ? "success" : "failed");
        printfln("  Answer count: {}", result.answers.size());
        for (const auto& ans: result.answers) {
            printfln("  Answer: {} → {}", ans.name, ans.data);
        }
    } catch (const exception& e) {
        printfln("  Error: {}", e.what());
    }

    // ========== 反向DNS查询 ==========
    println("\n=== Reverse DNS (PTR) ===");
    try {
        auto hostname = client.reverse_query("114.114.114.114");
        if (!hostname.empty()) {
            printfln("  114.114.114.114 → {}", hostname);
        }
    } catch (const exception& e) {
        printfln("  Error: {}", e.what());
    }

    // ========== 批量查询 ==========
    println("\n=== Batch Query ===");
    try {
        vector<string> domains{"baidu.com", "taobao.com", "jd.com"};
        auto results = client.batch_query(domains, dns_record::A);
        for (size_t i = 0; i < results.size(); ++i) {
            printfln("  {} → {} answers", domains.begin()[i], results[i].answers.size());
        }
    } catch (const exception& e) {
        printfln("  Error: {}", e.what());
    }

    // ========== 缓存管理 ==========
    println("\n=== Cache Control ===");
    {
        client.set_cache_ttl(seconds(600));
        println("  Cache TTL set to 600s");

        client.clear_cache();
        println("  Cache cleared");
    }

    // ========== DNSSEC ==========
    println("\n=== DNSSEC ===");
    {
        client.set_dnssec_ok(true);
        client.set_edns_udp_payload(4096);
        println("  DNSSEC enabled, EDNS0 payload: 4096 bytes");
    }

    println("\nAll DNS examples completed");
    return 0;
}
