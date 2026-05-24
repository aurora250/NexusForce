#include <NeForce/NeForce.hpp>
using namespace neforce;
using namespace neforce::http;

namespace {
    const neforce::path& res_root() {
        static neforce::path res_root
#ifdef NEFORCE_PLATFORM_WINDOWS
                {R"(D:/Workspace/Cpp Workspace/CLine Workspace/NexusForce/tests/resource)"};
#elif defined(NEFORCE_PLATFORM_LINUX)
                {R"(/media/huenqi/Programming/Workspace/NexusForce-Linux/tests/resource)"};
#endif
        return res_root;
    }
} // namespace


void test_http_client() {
    try {
        http_client::config config;
        config.connect_timeout = milliseconds(5000);
        config.receive_timeout = milliseconds(5000);
        config.follow_redirects = true;
        config.max_redirects = 5;
        config.verify_ssl = true;
        http_client client(config);

        const path pem = res_root() / "cacert.pem";
        client.get_client().load_ca_file(pem.str());

        auto response = client.get("https://www.example.com");
        if (response.is_success()) {
            println("Response:", response.body);
        } else {
            println("Request failed:", response.status_message);
        }

        printfln("HTTP Version: HTTP/{}.{}", response.http_version_major, response.http_version_minor);
        println("Status Message: ", response.status_message);
        println("Effective URL: ", response.effective_url);
        printfln("Total Time: {}ms", response.total_time.count());
        println("Headers:");
        for (const auto& elem: response.headers) {
            const auto& key = elem.first;
            const auto& values = elem.second;
            for (const auto& val: values) {
                println("  ", key, ": ", val);
            }
        }

        if (!response.body.empty()) {
            println();
            println("Body (first 200 chars):");
            println(response.body.view(0, 200), "...");
        }

        const auto& cookies = response.cookies;
        if (!cookies.empty()) {
            println();
            println("Cookies received:");
            for (const auto& c: cookies) {
                println("  ", c.name.cookie_name(), "=", c.value);
            }
        }
    } catch (const exception& e) {
        printcln(color::red(), "HTTP Client error: " + string(e.what()));
    }
}

void test_download() {
    http_client::config config;
    config.connect_timeout = seconds(10);
    config.receive_timeout = seconds(15);
    config.follow_redirects = true;
    config.max_redirects = 5;
    config.max_response_size = byte_size{numeric_traits<size_t>::max()};
    config.verify_ssl = true;
    http_client client(config);

    const path pem = res_root() / "cacert.pem";
    client.get_client().load_ca_file(pem.str());
    const bool res = client.download_file("https://www.python.org/ftp/python/3.12.0/python-3.12.0-amd64.exe",
                                          res_root() / "python.exe");
    println("Download result:", res);
}

void test_traceroute() {
    try {
        icmp_socket icmp;
        icmp.open();

        auto dest_opt = ip_address::parse("8.8.8.8");
        if (!dest_opt.has_value()) {
            println("解析目标地址失败");
            return;
        }
        ip_address dest = dest_opt.value();

        println("开始 traceroute 到", dest.to_string());
        println("最多 30 跳，每跳 3 次探测，超时 1 秒\n");

        auto hops = icmp.traceroute(dest, 30, milliseconds(1000), 3);

        println("traceroute 到目标，最大", hops.size(), "跳");
        println(" 跳数           IP地址           RTT1     RTT2     RTT3");
        println("------   -------------------   ------   ------   ------");

        for (size_t i = 0; i < hops.size(); ++i) {
            const auto& hop = hops[i];

            neforce::printf("  {}     ", (i + 1));

            if (hop.address.is_valid()) {
                print(hop.address.to_string());
            } else {
                print("      *      ");
            }

            for (int p = 0; p < 3; ++p) {
                if (hop.rtt[p].count() >= 0) {
                    neforce::printf("    {}ms    ", hop.rtt[p].count());
                } else {
                    print("    *    ");
                }
            }
            println();

            if (hop.reached) {
                println("\n*** 已达到目标地址 ***");
                break;
            }
        }

    } catch (const socket_exception& e) {
        println("套接字错误:", e.what());
        if (e.code() == 1) {
            println("需要管理员/root权限才能创建原始ICMP套接字");
        }
    } catch (const exception& e) {
        println("未知错误:", e.what());
    }
}

void test_ping() {
    try {
        icmp_socket icmp;
        icmp.open();

        auto dest = ip_address::parse("8.8.8.8");
        if (!dest.has_value()) {
            println("无法解析目标地址\n");
            return;
        }

        printfln("正在 Ping {} ...\n", dest->to_string());

        char custom_data[] = "Hello NeForce Ping!";

        int success_count = 0;
        for (int i = 0; i < 4; i++) {
            auto result = icmp.ping(*dest, milliseconds(2000), i, custom_data, sizeof(custom_data));

            if (result.success) {
                success_count++;
                printfln("来自 {} 的回复: 字节={} 时间={}ms TTL={}\n", result.destination.to_string(),
                         result.reply_size, result.rtt.count(), result.reply_ttl);
            } else {
                printfln("请求超时 (序列号 {})\n", i);
            }

            if (i < 3) {
                this_thread::sleep_for(milliseconds(1000));
            }
        }

        int loss_rate = (4 - success_count) * 100 / 4;
        println("\nPing 统计信息:\n");
        printfln("    已发送 = 4, 已接收 = {}, 丢失 = {} ({}% 丢失)\n", success_count, 4 - success_count, loss_rate);

    } catch (const exception& e) {
        printfln("Ping 失败: {}\n", e.what());
    }
}

void test_arp() {
    arp arp_resolver;
    if (!arp_resolver.open()) {
        println("Failed to open ARP resolver (you may need root)");
        return;
    }

    auto ip = ip_address::parse("192.168.1.1");
    if (ip) {
        auto mac = arp_resolver.resolve(*ip, milliseconds(2000));
        if (mac) {
            println(*mac);
        } else {
            println("MAC address not resolved");
        }
    }
}

void test_smtp() {
    ssl_context ctx(ssl_method::TLS_CLIENT);
    ctx.set_default_options();
    const path pem = res_root() / "cacert.pem";
    ctx.load_verify_locations(pem.str());

    dns_client dns;

    smtp_socket smtp;
    smtp.connect("smtp.qq.com", ports{465}, "myhost.local", smtp_socket::tls_mode::implicit, &dns, &ctx, "smtp.qq.com");

    file code{res_root() / "authcode"};
    smtp.authenticate("1737900250@qq.com", code.read(), smtp_socket::auth_method::login);

    smtp_message msg;
    msg.from = "1737900250@qq.com";
    msg.to = {"1737900250@qq.com"};
    msg.subject = "Hello from NeForce";
    msg.body = "This is a test email sent to QQ mailbox.";

    smtp.send(msg);
    smtp.disconnect();
}
