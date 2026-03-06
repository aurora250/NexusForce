#include "test.h"


void handle_session_api(
    http_request &request, http_response &response, http_server &server) {
    session *sess = server.session(request);
    string action = request.parameter("action");

    if (action == "create") {
        sess = server.session(request, true);
        response.status = HTTP_STATUS::S2_OK;
        response.status_message = "OK";
        response.set_content_type(HTTP_CONTENT::JSON_APP);
        response.body = R"({"sessionId":")" + sess->id + R"("})";
    } else if (action == "invalidate" && sess) {
        sess->invalidated = true;
        sess->data.clear();
        response.status = HTTP_STATUS::S2_OK;
        response.status_message = "OK";
        response.set_content_type(HTTP_CONTENT::JSON_APP);
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
                .value(sess->data).end_object()
                .build();

            response.status = HTTP_STATUS::S2_OK;
            response.status_message = "OK";
            response.set_content_type(HTTP_CONTENT::JSON_APP);
            response.body = json->to_string();
        } else {
            response.status = HTTP_STATUS::S4_BAD_REQUEST;
            response.status_message = "Bad Request";
            response.set_content_type(HTTP_CONTENT::JSON_APP);
            response.body = R"({"error":"No active session found"})";
        }
    } else {
        response.status = HTTP_STATUS::S4_BAD_REQUEST;
        response.status_message = "Bad Request";
        response.set_content_type(HTTP_CONTENT::JSON_APP);
        response.body = R"({"error":"Invalid session action"})";
    }
}

void handle_session_attribute(http_request &request, http_response &response, http_server &server) {
    string attrName, attrValue;
    string content_type = request.header(HTTP_KEY::Content_Type);

    if (content_type.find(HTTP_CONTENT::JSON_APP.content()) == 0) {
        try {
            auto root = json_parser(request.body).parse();
            if (root && root->is_object()) {
                const json_object *obj = root->as_object();

                const json_value *attrNameVal = obj->get_member("attrName");
                if (attrNameVal && attrNameVal->is_string()) {
                    attrName = attrNameVal->as_string()->get_value();
                }

                const json_value *attrValueVal = obj->get_member("attrValue");
                if (attrValueVal && attrValueVal->is_string()) {
                    attrValue = attrValueVal->as_string()->get_value();
                }
            }
        } catch (const exception &e) {
            println("JSON parse error:", e.what());
        }
    } else {
        attrName = request.parameter("attrName");
        attrValue = request.parameter("attrValue");
    }

    session *sess = server.session(request, true);

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

        response.status = HTTP_STATUS::S2_OK;
        response.status_message = "OK";
        response.set_content_type(HTTP_CONTENT::JSON_APP);
        response.body = json->to_string();
    } else {
        response.status = HTTP_STATUS::S4_BAD_REQUEST;
        response.status_message = "Bad Request";
        response.set_content_type(HTTP_CONTENT::JSON_APP);
        response.body = R"({"error":"Missing attribute name"})";
    }
}

void handle_cookie_api(
    http_request& request, http_response& response) {
    if (request.method.is_post()) {
        HTTP_COOKIE_NAME name;
        string value, max_age_str;
        string content_type = request.header(HTTP_KEY::Content_Type);

        if (content_type.find(HTTP_CONTENT::JSON_APP.content()) != string::npos) {
            try {
                auto root = json_parser(request.body).parse();
                if (root && root->is_object()) {
                    const json_object* obj = root->as_object();

                    const json_value* nameVal = obj->get_member("name");
                    const json_value* valueVal = obj->get_member("value");
                    const json_value* maxAgeVal = obj->get_member("maxAge");

                    if (nameVal && nameVal->is_string()) {
                        name = HTTP_COOKIE_NAME(nameVal->as_string()->get_value());
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
            name = HTTP_COOKIE_NAME(request.parameter("name"));
            value = request.parameter("value");
            max_age_str = request.parameter("maxAge");
        }

        if (!name.cookie_name().empty()) {
            cookie ck;
            ck.name = name;
            ck.value = value;
            if (!max_age_str.empty()) {
                ck.max_age = integer32::parse(max_age_str.view());
            }
            response.cookies.emplace_back(move(ck));

            auto json = json_builder()
                .begin_object()
                .key("name").value(name.cookie_name())
                .key("value").value(value)
                .end_object().build();

            response.status = HTTP_STATUS::S2_OK;
            response.status_message = "OK";
            response.set_content_type(HTTP_CONTENT::JSON_APP);
            response.body = json->to_string();
        } else {
            response.status = HTTP_STATUS::S4_BAD_REQUEST;
            response.status_message = "Bad Request";
            response.set_content_type(HTTP_CONTENT::JSON_APP);
            response.body = R"({"error":"Missing cookie name"})";
        }
    }
    else if (request.method.is_delete()) {
        HTTP_COOKIE_NAME name(request.parameter("name"));
        if (!name.cookie_name().empty()) {
            cookie ck;
            ck.name = name;
            ck.max_age = 0;
            ck.expires = datetime::epoch();
            response.cookies.emplace_back(move(ck));

            response.status = HTTP_STATUS::S2_OK;
            response.status_message = "OK";
            response.set_content_type(HTTP_CONTENT::JSON_APP);
            response.body = R"({"name":")" + name.to_string() + R"("})";
        } else {
            response.status = HTTP_STATUS::S4_BAD_REQUEST;
            response.status_message = "Bad Request";
            response.set_content_type(HTTP_CONTENT::JSON_APP);
            response.body = R"({"error":"Missing cookie name"})";
        }
    }
}

void test_https_server() {
#ifdef NEFORCE_SUPPORT_OPENSSL
    try {
        https_server server(8443, 128);
        server.load_certificate("D:/OpenSSL/server.crt", "D:/OpenSSL/server.key");

        http_router& r = server.router();

        r.get("/", [](http_request& req, http_response& res) {
            printcln(color::cyan(), "HTTPS Request from:", req.header("User-Agent"));

            res.status = HTTP_STATUS::S2_OK;
            res.status_message = "OK";
            res.set_content_type(HTTP_CONTENT::HTML_TEXT);

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

            res.body = html;
        });

        r.get("/api/info", [](http_request& req, http_response& res) {
            res.status = HTTP_STATUS::S2_OK;
            res.status_message = "OK";
            res.set_content_type(HTTP_CONTENT::JSON_APP);

            json_builder response;
            response.begin_object()
                .key("https").value(req.header(HTTP_KEY::X_Forwarded_Proto) == "https")
                .key("method").value(req.method.to_string())
                .key("path").value(req.path)
                .key("user_agent").value(req.header("User-Agent"))
                .end_object();

            res.body = response.build()->to_string();
        });

        r.post("/api/echo", [](http_request& req, http_response& res) {
            res.status = HTTP_STATUS::S2_OK;
            res.status_message = "OK";
            res.set_content_type(HTTP_CONTENT::JSON_APP);

            json_builder response;
            response.begin_object()
                .key("https").value(req.header(HTTP_KEY::X_Forwarded_Proto) == "https")
                .key("body").value(req.body)
                .key("content_type").value(req.header(HTTP_KEY::Content_Type))
                .end_object();

            res.body = response.build()->to_string();
        });

        r.set_not_found_handler([](http_request&, http_response& res) {
            res.status = HTTP_STATUS::S4_NOT_FOUNT;
            res.body = "HTTPS 404 - Not Found";
        });

        if (server.start()) {
            printcln(color::green(), "HTTPS Server started on port 8443");
            printcln(color::yellow(), "Note: Using self-signed certificate");
            printcln(color::yellow(), "Press Ctrl+C to stop");

            signal_guard guard;

            signal_manager::instance().register_handler(
                SIGNAL_EVENT::INTERRUPT,
                signal_handler
            );

            while (true) {
                this_thread::sleep_for(seconds(1));
            }
        }
    } catch (const exception& e) {
        printcln(color::red(), "HTTPS Server error: " + string(e.what()));
    }
#endif
}

void test_http_server() {
    try {
        http_server server(8080, 128);

        http_router& r = server.router();
        r.use(new logging_filter());
        r.use(new cors_filter("http://127.0.0.1:5500"));
        r.use(new static_file_filter(res_root().str()));

        r.post("/old-link", [](http_request&, http_response& res) {
            res.redirect_url = "/new-link";
        });
        r.post("/forward-me", [](http_request&, http_response& res) {
            res.forward_path = "/forward-target";
        });
        r.post("/forward-target", [](http_request&, http_response& res) {
            res.status = HTTP_STATUS::S2_OK;
            res.status_message = "OK";
            res.body = "Forward Successfully";
        });

        r.get_post("/api/session",
            [&server](http_request& req, http_response& res) {
                handle_session_api(req, res, server);
            }
        );
        r.get_post("/api/session-attribute",
            [&server](http_request& req, http_response& res) {
                handle_session_attribute(req, res, server);
            }
        );
        r.post_delete("/api/cookie",
            [](http_request& req, http_response& res) {
                handle_cookie_api(req, res);
            }
        );

        r.get("/api/logger-test", [](http_request&, http_response& res) {
            res.status = HTTP_STATUS::S2_OK;
            res.status_message = "OK";
            res.body = "Logging filter test successful";
        });
        r.get("/api/data", [](http_request&, http_response& res) {
            res.status = HTTP_STATUS::S2_OK;
            res.status_message = "OK";
            res.set_content_type(HTTP_CONTENT::JSON_APP);
            res.body = R"({"status":"success"})";
        });

        r.get("/", [](http_request&, http_response& res) {
            res.status = HTTP_STATUS::S2_OK;
            res.status_message = "OK";
            res.set_content_type(HTTP_CONTENT::HTML_TEXT);
            res.body = file::read(res_root() / "index.html");
        });

        r.get("/detail", [](http_request&, http_response& res) {
            res.status = HTTP_STATUS::S2_OK;
            res.status_message = "OK";
            res.set_content_type(HTTP_CONTENT::HTML_TEXT);
            res.body = file::read(res_root() / "detail.html");
        });

        r.get("/new-link", [](http_request&, http_response& res) {
            res.status = HTTP_STATUS::S2_OK;
            res.status_message = "OK";
            res.set_content_type(HTTP_CONTENT::HTML_TEXT);
            res.body = file::read(res_root() / "index.html");
        });

        r.get("/test", [](http_request&, http_response& res) {
            res.status = HTTP_STATUS::S2_OK;
            res.status_message = "OK";
            res.set_content_type(HTTP_CONTENT::HTML_TEXT);
            res.body = file::read(res_root() / "test.html");
        });

        r.set_not_found_handler([](http_request&, http_response &res) {
            res.status = HTTP_STATUS::S4_NOT_FOUNT;
            res.status_message = "Not Found";
            res.set_content_type(HTTP_CONTENT::HTML_TEXT);
            try {
                res.body = file::read(res_root() / "404err.html");
            } catch (...) {
                res.body = "<h1>404 - Page Not Found</h1>";
            }
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

            session->set_close_handler([](WEBSOCKET_STATUS status, const string& reason) {
                println("Connection closed:", reason);
            });

            if (session->is_open()) {
                session->send("Welcome to chat room!");
            }
        });

        if (server.start()) {
            signal_manager::instance().start_monitoring();

            signal_manager::instance().register_handler(
                SIGNAL_EVENT::INTERRUPT,
                [&server](SIGNAL_EVENT event, void* context) -> bool {
                    if (event == SIGNAL_EVENT::INTERRUPT) {
                        println("Interrupting...");
                        server.stop();
                        signal_manager::instance().stop_monitoring();
                        std::exit(0);
                    }
                    return false;
                }
            );

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
        config.follow_redirects = true;
        config.max_redirects = 5;
        http_client client(config);

        http_client_request req;
        req.host = "www.example.com";
        req.port = 80;
        req.method = HTTP_METHOD::GET;
        req.path = "/";
        req.headers["User-Agent"] = "NeForce HTTP Client/1.0";
        req.headers["Accept"] = "*/*";
        auto response = client.request(move(req));

        println("HTTP Version: HTTP/", response.http_version_major, ".", response.http_version_minor);
        println("Status Message: ", response.status_message);
        println("Effective URL: ", response.effective_url);
        println("Total Time: ", response.total_time.count(), "ms");
        println("Headers:");
        for (const auto& elem : response.headers) {
            const auto& key = elem.first;
            const auto& values = elem.second;
            for (const auto& val : values) {
                println("  ", key, ": ", val);
            }
        }

        if (!response.body.empty()) {
            println();
            println("Body (first 200 chars):");
            println(response.body.substr(0, 200), "...");
        }

        const auto& cookies = response.cookies;
        if (!cookies.empty()) {
            println();
            println("Cookies received:");
            for (const auto& c : cookies) {
                println("  ", c.name.cookie_name(), "=", c.value);
            }
        }
    } catch (const exception& e) {
        printcln(color::red(), "HTTP Client error: " + string(e.what()));
    }
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
        custom_config.port = 5353;
        custom_config.timeout = milliseconds(3000);
        dns_client custom_client(custom_config);

        auto ips = cloudflare_client.resolve_a("example.com");
        println("IPv4 addresses for example.com:");
        for (const auto& ip : ips) {
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
        for (const auto& addr : ipv6_addrs) {
            println("  ", addr);
        }

        {
            scoped_click sc(c);
            ipv6_addrs = client.resolve_aaaa("www.google.com");
        }
        println("Second query (cached): ", c.during().count(), "ns");

        auto mx_records = client.resolve_mx("gmail.com");
        println("MX records for gmail.com:");
        for (const auto& mx : mx_records) {
            println("  ", mx);
        }

        vector<string> domains = {
            "google.com",
            "facebook.com",
            "twitter.com",
            "github.com",
            "stackoverflow.com"
        };
        auto results = client.batch_query(domains, DNS_RECORD::A);

        for (size_t i = 0; i < domains.size(); ++i) {
            println(domains[i], ":");

            if (results[i].is_success()) {
                for (const auto& answer : results[i].answers) {
                    println("  ", answer.data);
                }
                println("  Query time:", results[i].query_time.count(), "ms");
            } else {
                println("  Query failed\n");
            }
        }

        auto txt_records = client.resolve_txt("google.com");
        println("TXT records for google.com:");
        for (const auto& txt : txt_records) {
            println("  ", txt);
        }
    } catch (const exception& e) {
        printcln(color::red(), "DNS test error: " + string(e.what()));
    }
}
