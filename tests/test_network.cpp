#include "test.h"

using namespace neforce::http;


void handle_session_api(http_request& request, http_response& response, http_server& server) {
    http_session* sess = server.get_session(request);
    string action = request.parameter("action");

    if (action == "create") {
        sess = server.get_session(request, true);
        response.status = http_status::S2_OK;
        response.status_message = "OK";
        response.set_content_type(http_content::JSON_APP());
        response.body = R"({"sessionId":")" + sess->id + R"("})";
    } else if (action == "invalidate" && sess) {
        sess->invalidated = true;
        sess->data.clear();
        response.status = http_status::S2_OK;
        response.status_message = "OK";
        response.set_content_type(http_content::JSON_APP());
        response.body = R"({"message":"Session invalidated"})";
    } else if (action == "info") {
        if (sess) {
            auto json = json_builder()
                                .begin_object()
                                .key("sessionId")
                                .value(sess->id)
                                .key("createTime")
                                .value(sess->create_time.to_string_ISO_UTC())
                                .key("lastAccess")
                                .value(sess->last_access.to_string_ISO_UTC())
                                .key("attributes")
                                .value(sess->data)
                                .end_object()
                                .build();

            response.status = http_status::S2_OK;
            response.status_message = "OK";
            response.set_content_type(http_content::JSON_APP());
            response.body = json->to_string();
        } else {
            response.status = http_status::S4_BAD_REQUEST;
            response.status_message = "Bad Request";
            response.set_content_type(http_content::JSON_APP());
            response.body = R"({"error":"No active session found"})";
        }
    } else {
        response.status = http_status::S4_BAD_REQUEST;
        response.status_message = "Bad Request";
        response.set_content_type(http_content::JSON_APP());
        response.body = R"({"error":"Invalid session action"})";
    }
}

void handle_session_attribute(http_request& request, http_response& response, http_server& server) {
    string attrName, attrValue;
    string content_type = request.header(http_key::Content_Type());

    if (content_type.find(http_content::JSON_APP().content()) == 0) {
        try {
            auto root = json_parser(request.body).parse();
            if (root && root->is_object()) {
                const json_object* obj = root->as_object();

                const json_value* attrNameVal = obj->get_member("attrName");
                if (attrNameVal && attrNameVal->is_string()) {
                    attrName = attrNameVal->as_string()->get_value();
                }

                const json_value* attrValueVal = obj->get_member("attrValue");
                if (attrValueVal && attrValueVal->is_string()) {
                    attrValue = attrValueVal->as_string()->get_value();
                }
            }
        } catch (const exception& e) {
            println("JSON parse error:", e.what());
        }
    } else {
        attrName = request.parameter("attrName");
        attrValue = request.parameter("attrValue");
    }

    http_session* sess = server.get_session(request, true);

    if (!attrName.empty()) {
        (*sess)[attrName] = attrValue;

        auto json = json_builder()
                            .begin_object()
                            .key("attrName")
                            .value(attrName)
                            .key("attrValue")
                            .value(attrValue)
                            .end_object()
                            .build();

        response.status = http_status::S2_OK;
        response.status_message = "OK";
        response.set_content_type(http_content::JSON_APP());
        response.body = json->to_string();
    } else {
        response.status = http_status::S4_BAD_REQUEST;
        response.status_message = "Bad Request";
        response.set_content_type(http_content::JSON_APP());
        response.body = R"({"error":"Missing attribute name"})";
    }
}

void handle_cookie_api(http_request& request, http_response& response) {
    if (request.method.is_post()) {
        http_cookie_name name;
        string value, max_age_str;
        string content_type = request.header(http_key::Content_Type());

        if (content_type.find(http_content::JSON_APP().content()) != string::npos) {
            try {
                auto root = json_parser(request.body).parse();
                if (root && root->is_object()) {
                    const json_object* obj = root->as_object();

                    const json_value* nameVal = obj->get_member("name");
                    const json_value* valueVal = obj->get_member("value");
                    const json_value* maxAgeVal = obj->get_member("maxAge");

                    if (nameVal && nameVal->is_string()) {
                        name = http_cookie_name(nameVal->as_string()->get_value());
                    }
                    if (valueVal && valueVal->is_string()) {
                        value = valueVal->as_string()->get_value();
                    }
                    if (maxAgeVal) {
                        if (maxAgeVal->is_string()) {
                            max_age_str = maxAgeVal->as_string()->get_value();
                        } else if (maxAgeVal->is_number()) {
                            max_age_str = _NEFORCE to_string(maxAgeVal->as_number()->get_value());
                        }
                    }
                }
            } catch (const exception& e) {
                println("JSON parse error:", e.what());
            }
        } else {
            name = http_cookie_name(request.parameter("name"));
            value = request.parameter("value");
            max_age_str = request.parameter("maxAge");
        }

        if (!name.cookie_name().empty()) {
            http_cookie ck;
            ck.name = name;
            ck.value = value;
            if (!max_age_str.empty()) {
                ck.max_age = integer32::parse(max_age_str.view()).value();
            }
            response.cookies.emplace_back(move(ck));

            auto json = json_builder()
                                .begin_object()
                                .key("name")
                                .value(name.cookie_name())
                                .key("value")
                                .value(value)
                                .end_object()
                                .build();

            response.status = http_status::S2_OK;
            response.status_message = "OK";
            response.set_content_type(http_content::JSON_APP());
            response.body = json->to_string();
        } else {
            response.status = http_status::S4_BAD_REQUEST;
            response.status_message = "Bad Request";
            response.set_content_type(http_content::JSON_APP());
            response.body = R"({"error":"Missing cookie name"})";
        }
    } else if (request.method.is_delete()) {
        http_cookie_name name(request.parameter("name"));
        if (!name.cookie_name().empty()) {
            http_cookie ck;
            ck.name = name;
            ck.max_age = 0;
            ck.expires = datetime::epoch();
            response.cookies.emplace_back(move(ck));

            response.status = http_status::S2_OK;
            response.status_message = "OK";
            response.set_content_type(http_content::JSON_APP());
            response.body = R"({"name":")" + name.to_string() + R"("})";
        } else {
            response.status = http_status::S4_BAD_REQUEST;
            response.status_message = "Bad Request";
            response.set_content_type(http_content::JSON_APP());
            response.body = R"({"error":"Missing cookie name"})";
        }
    }
}

void test_https_server() {
    try {
        https_server server(8443, 128);
        server.load_certificate(
#ifdef NEFORCE_PLATFORM_LINUX
                "/home/huenqi/server.crt", "/home/huenqi/server.key"
#else
                "D:/OpenSSL/server.crt", "D:/OpenSSL/server.key"
#endif
        );

        http_router& r = server.router();

        r.get("/", [](http_request& request, http_response& response) {
            printcln(color::cyan(), "HTTPS Request from:", request.header("User-Agent"));

            response.status = http_status::S2_OK;
            response.status_message = "OK";
            response.set_content_type(http_content::HTML_TEXT());

            const string html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>HTTPS Test</title>
</head>
<body>
    <h1>HTTPS Connection Successful!</h1>
    <p>This is served over HTTPS.</p>
    <p>SSL/TLS is working correctly.</p>
    <div id="status"></div>

    <script>
        document.getElementById('status').textContent =
            'Protocol: ' + window.location.protocol;
    </script>
</body>
</html>
            )";

            response.body = html;
        });

        r.get("/api/info", [](http_request& request, http_response& response) {
            response.status = http_status::S2_OK;
            response.status_message = "OK";
            response.set_content_type(http_content::JSON_APP());

            json_builder json;
            json.begin_object()
                    .key("https")
                    .value(request.header(http_key::X_Forwarded_Proto()) == "https")
                    .key("method")
                    .value(request.method.to_string())
                    .key("path")
                    .value(request.path)
                    .key("user_agent")
                    .value(request.header("User-Agent"))
                    .end_object();

            response.body = json.build()->to_string();
        });

        r.post("/api/echo", [](http_request& request, http_response& response) {
            response.status = http_status::S2_OK;
            response.status_message = "OK";
            response.set_content_type(http_content::JSON_APP());

            json_builder json;
            json.begin_object()
                    .key("https")
                    .value(request.header(http_key::X_Forwarded_Proto()) == "https")
                    .key("body")
                    .value(request.body)
                    .key("content_type")
                    .value(request.header(http_key::Content_Type()))
                    .end_object();

            response.body = json.build()->to_string();
        });

        r.set_not_found_handler([](http_request&, http_response& response) {
            response.status = http_status::S4_NOT_FOUNT;
            response.body = "HTTPS 404 - Not Found";
        });

        if (server.start()) {
            printcln(color::green(), "HTTPS Server started on port 8443");
            printcln(color::yellow(), "Note: Using self-signed certificate");
            printcln(color::yellow(), "Press Ctrl+C to stop");

            signal_guard guard;

            signal_manager::instance().register_handler(signal_event::INTERRUPT,
                                                        [&server](signal_event event, void* context) -> bool {
                                                            if (event == signal_event::INTERRUPT) {
                                                                println("Interrupting...");
                                                                server.stop();
                                                                immediate_exit(0);
                                                            }
                                                            return false;
                                                        });

            while (server.is_running()) {
                this_thread::sleep_for(seconds(1));
            }
        }
    } catch (const exception& e) {
        printcln(color::red(), "HTTPS Server error: " + string(e.what()));
    }
}

void test_http_server() {
    try {
        http_server server(8080, 128);

        http_router& router = server.router();
        router.use(make_unique<logging_filter>());
        router.use(make_unique<cors_filter>("http://127.0.0.1:5500"));
        router.use(make_unique<static_file_filter>(res_root().str()));

        router.post("/old-link", [](http_request&, http_response& response) { response.redirect_url = "/new-link"; });
        router.post("/forward-me",
                    [](http_request&, http_response& response) { response.forward_path = "/forward-target"; });
        router.post("/forward-target", [](http_request&, http_response& response) {
            response.status = http_status::S2_OK;
            response.status_message = "OK";
            response.body = "Forward Successfully";
        });

        router.get_post("/api/session", [&server](http_request& request, http_response& response) {
            handle_session_api(request, response, server);
        });
        router.get_post("/api/session-attribute", [&server](http_request& request, http_response& response) {
            handle_session_attribute(request, response, server);
        });
        router.post_delete("/api/cookie", [](http_request& request, http_response& response) {
            handle_cookie_api(request, response);
        });

        router.get("/api/logger-test", [](http_request&, http_response& response) {
            response.status = http_status::S2_OK;
            response.status_message = "OK";
            response.body = "Logging filter test successful";
        });
        router.get("/api/data/*", [](http_request&, http_response& response) {
            response.status = http_status::S2_OK;
            response.status_message = "OK";
            response.set_content_type(http_content::JSON_APP());
            response.body = R"({"status":"success"})";
        });

        router.get("/", [](http_request&, http_response& response) {
            static file index{res_root() / "index.html"};
            response.status = http_status::S2_OK;
            response.status_message = "OK";
            response.set_content_type(http_content::HTML_TEXT());
            response.body = index.read();
        });

        router.get("/detail", [](http_request&, http_response& response) {
            static file detail{res_root() / "detail.html"};
            response.status = http_status::S2_OK;
            response.status_message = "OK";
            response.set_content_type(http_content::HTML_TEXT());
            response.body = detail.read();
        });

        router.get("/new-link", [](http_request&, http_response& response) {
            static file index{res_root() / "index.html"};
            response.status = http_status::S2_OK;
            response.status_message = "OK";
            response.set_content_type(http_content::HTML_TEXT());
            response.body = index.read();
        });

        router.get("/test", [](http_request&, http_response& response) {
            static file test{res_root() / "index.html"};
            response.status = http_status::S2_OK;
            response.status_message = "OK";
            response.set_content_type(http_content::HTML_TEXT());
            response.body = test.read();
        });

        router.set_not_found_handler([](http_request&, http_response& response) {
            static file err{res_root() / "404err.html"};
            response.status = http_status::S4_NOT_FOUNT;
            response.status_message = "Not Found";
            response.set_content_type(http_content::HTML_TEXT());
            response.body = err.read();
        });

        auto& ws = server.websocket();

        ws.route("/chat", [](shared_ptr<websocket_session<tcp_socket>> session) {
            println("New WebSocket connection established");

            weak_ptr<websocket_session<tcp_socket>> weak_session = session;

            session->set_message_handler([weak_session](const string& message, websocket_opcode opcode) {
                println("Received: ", message);

                if (auto session = weak_session.lock()) {
                    if (session->is_open()) {
                        session->send("Server received: " + message);
                    }
                }
            });

            session->set_close_handler(
                    [](websocket_status status, const string& reason) { println("Connection closed:", reason); });

            if (session->is_open()) {
                session->send("Welcome to chat room!");
            }
        });

        if (server.start()) {
            signal_manager::instance().start_monitoring();

            signal_manager::instance().register_handler(signal_event::INTERRUPT,
                                                        [&server](signal_event event, void* context) -> bool {
                                                            if (event == signal_event::INTERRUPT) {
                                                                println("Interrupting...");
                                                                server.stop();
                                                                immediate_exit(0);
                                                            }
                                                            return false;
                                                        });

            printcln(color::green(), "Press Ctrl+C to stop the server.");
            while (server.is_running()) {
                this_thread::sleep_for(seconds(1));
            }
            return;
        }
        printcln(color::red(), "Failed to start server!");
    } catch (const exception& e) {
        printcln(color::red(), "HTTP Server error: " + string(e.what()));
    }
}

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
    config.max_response_size = numeric_traits<size_t>::max();
    config.verify_ssl = true;
    http_client client(config);

    const path pem = res_root() / "cacert.pem";
    client.get_client().load_ca_file(pem.str());
    const bool res = client.download_file("https://www.python.org/ftp/python/3.12.0/python-3.12.0-amd64.exe",
                                          res_root() / "python.exe");
    println("Download result:", res);
}

void test_dns() {
    try {
        dns_client::config cf_config;
        cf_config.server = "1.1.1.1";
        cf_config.timeout = milliseconds(3000);
        dns_client cloudflare_client(cf_config);

        dns_client::config od_config;
        od_config.server = "208.67.222.222";
        od_config.timeout = milliseconds(3000);
        dns_client opendns_client(od_config);

        dns_client::config custom_config;
        custom_config.server = "192.168.1.1";
        custom_config.port = ports{5353};
        custom_config.timeout = milliseconds(3000);
        dns_client custom_client(custom_config);

        auto ips = cloudflare_client.resolve_a("example.com");
        println("IPv4 addresses for example.com:");
        for (const auto& ip: ips) {
            println("  ", ip);
        }

        dns_client client;
        vector<string> ipv6_addrs;

        click c;
        {
            scoped_click sc(c);
            ipv6_addrs = client.resolve_aaaa("www.google.com");
        }
        println("First query (no cache): ", c.during().count(), "ns");
        println("IPv6 addresses:");
        for (const auto& addr: ipv6_addrs) {
            println("  ", addr);
        }

        {
            scoped_click sc(c);
            ipv6_addrs = client.resolve_aaaa("www.google.com");
        }
        println("Second query (cached): ", c.during().count(), "ns");

        auto mx_records = client.resolve_mx("gmail.com");
        println("MX records for gmail.com:");
        for (const auto& mx: mx_records) {
            println("  ", mx);
        }

        vector<string> domains = {"google.com", "facebook.com", "twitter.com", "github.com", "stackoverflow.com"};
        auto results = client.batch_query(domains, dns_record::A);

        for (size_t i = 0; i < domains.size(); ++i) {
            println(domains[i], ":");

            if (results[i].is_success()) {
                for (const auto& answer: results[i].answers) {
                    println("  ", answer.data);
                }
                println("  Query time:", results[i].query_time.count(), "ms");
            } else {
                println("  Query failed\n");
            }
        }

        auto txt_records = client.resolve_txt("google.com");
        println("TXT records for google.com:");
        for (const auto& txt: txt_records) {
            println("  ", txt);
        }
    } catch (const exception& e) {
        printcln(color::red(), "DNS test error: " + string(e.what()));
    }
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

void test_ftp_client() {
    logger::instance().set_level(log_level::TRACE);
    logger::instance().add_sink(make_shared<console_sink>());

    ftp_client client;
    try {
        client.connect("ftp.debian.org");
        client.login_anonymous();

        auto entries = client.list();
        for (const auto& e: entries) {
            println(e.is_directory ? "d" : "-", e.name);
        }

        client.disconnect();
    } catch (const exception& e) {
        println(e.what());
    }
}
