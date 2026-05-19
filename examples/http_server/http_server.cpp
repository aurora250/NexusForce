#include <NeForce/core/exception/terminate.hpp>
#include <NeForce/core/file/file.hpp>
#include <NeForce/core/file/json/json_builder.hpp>
#include <NeForce/core/file/json/json_parser.hpp>
#include <NeForce/core/system/console.hpp>
#include <NeForce/core/system/signal.hpp>
#include <NeForce/network/http/http_server.hpp>
using namespace neforce::literals;

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


void handle_session_api(neforce::http::http_request& request, neforce::http::http_response& response,
                        neforce::http::http_server& server) {
    neforce::http::http_session* sess = server.get_session(request);
    neforce::string action = request.parameter("action");

    if (action == "create") {
        sess = server.get_session(request, true);
        response.status = neforce::http::http_status::S2_OK;
        response.status_message = "OK";
        response.set_content_type(neforce::http::http_content::JSON_APP());
        response.body = R"({"sessionId":")" + sess->id + R"("})";
    } else if (action == "invalidate" && sess) {
        sess->invalidated = true;
        sess->data.clear();
        response.status = neforce::http::http_status::S2_OK;
        response.status_message = "OK";
        response.set_content_type(neforce::http::http_content::JSON_APP());
        response.body = R"({"message":"Session invalidated"})";
    } else if (action == "info") {
        if (sess) {
            auto json = neforce::json_builder()
                                .begin_object()
                                .key("sessionId")
                                .value(sess->id)
                                .key("createTime")
                                .value(sess->create_time.to_RFC3339())
                                .key("lastAccess")
                                .value(sess->last_access.to_RFC3339())
                                .key("attributes")
                                .value(sess->data)
                                .end_object()
                                .build();

            response.status = neforce::http::http_status::S2_OK;
            response.status_message = "OK";
            response.set_content_type(neforce::http::http_content::JSON_APP());
            response.body = json->to_string();
        } else {
            response.status = neforce::http::http_status::S4_BAD_REQUEST;
            response.status_message = "Bad Request";
            response.set_content_type(neforce::http::http_content::JSON_APP());
            response.body = R"({"error":"No active session found"})";
        }
    } else {
        response.status = neforce::http::http_status::S4_BAD_REQUEST;
        response.status_message = "Bad Request";
        response.set_content_type(neforce::http::http_content::JSON_APP());
        response.body = R"({"error":"Invalid session action"})";
    }
}

void handle_session_attribute(neforce::http::http_request& request, neforce::http::http_response& response,
                              neforce::http::http_server& server) {
    neforce::string attrName, attrValue;
    neforce::string content_type = request.header(neforce::http::http_key::Content_Type());

    if (content_type.find(neforce::http::http_content::JSON_APP().content()) == 0) {
        try {
            auto root = neforce::json_parser(request.body).parse();
            if (root && root->is_object()) {
                const neforce::json_object* obj = root->as_object();

                const neforce::json_value* attrNameVal = obj->get_member("attrName");
                if (attrNameVal && attrNameVal->is_string()) {
                    attrName = attrNameVal->as_string()->get_value();
                }

                const neforce::json_value* attrValueVal = obj->get_member("attrValue");
                if (attrValueVal && attrValueVal->is_string()) {
                    attrValue = attrValueVal->as_string()->get_value();
                }
            }
        } catch (const neforce::exception& e) {
            neforce::println("JSON parse error:", e.what());
        }
    } else {
        attrName = request.parameter("attrName");
        attrValue = request.parameter("attrValue");
    }

    neforce::http::http_session* sess = server.get_session(request, true);

    if (!attrName.empty()) {
        (*sess)[attrName] = attrValue;

        auto json = neforce::json_builder()
                            .begin_object()
                            .key("attrName")
                            .value(attrName)
                            .key("attrValue")
                            .value(attrValue)
                            .end_object()
                            .build();

        response.status = neforce::http::http_status::S2_OK;
        response.status_message = "OK";
        response.set_content_type(neforce::http::http_content::JSON_APP());
        response.body = json->to_string();
    } else {
        response.status = neforce::http::http_status::S4_BAD_REQUEST;
        response.status_message = "Bad Request";
        response.set_content_type(neforce::http::http_content::JSON_APP());
        response.body = R"({"error":"Missing attribute name"})";
    }
}

void handle_cookie_api(neforce::http::http_request& request, neforce::http::http_response& response) {
    if (request.method.is_post()) {
        neforce::http::http_cookie_name name;
        neforce::string value, max_age_str;
        neforce::string content_type = request.header(neforce::http::http_key::Content_Type());

        if (content_type.find(neforce::http::http_content::JSON_APP().content()) != neforce::string::npos) {
            try {
                auto root = neforce::json_parser(request.body).parse();
                if (root && root->is_object()) {
                    const neforce::json_object* obj = root->as_object();

                    const neforce::json_value* nameVal = obj->get_member("name");
                    const neforce::json_value* valueVal = obj->get_member("value");
                    const neforce::json_value* maxAgeVal = obj->get_member("maxAge");

                    if (nameVal && nameVal->is_string()) {
                        name = neforce::http::http_cookie_name(nameVal->as_string()->get_value());
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
            } catch (const neforce::exception& e) {
                neforce::println("JSON parse error:", e.what());
            }
        } else {
            name = neforce::http::http_cookie_name(request.parameter("name"));
            value = request.parameter("value");
            max_age_str = request.parameter("maxAge");
        }

        if (!name.cookie_name().empty()) {
            neforce::http::http_cookie ck;
            ck.name = name;
            ck.value = value;
            if (!max_age_str.empty()) {
                ck.max_age = neforce::seconds{neforce::integer32::parse(max_age_str.view()).value()};
            }
            response.cookies.emplace_back(move(ck));

            auto json = neforce::json_builder()
                                .begin_object()
                                .key("name")
                                .value(name.cookie_name())
                                .key("value")
                                .value(value)
                                .end_object()
                                .build();

            response.status = neforce::http::http_status::S2_OK;
            response.status_message = "OK";
            response.set_content_type(neforce::http::http_content::JSON_APP());
            response.body = json->to_string();
        } else {
            response.status = neforce::http::http_status::S4_BAD_REQUEST;
            response.status_message = "Bad Request";
            response.set_content_type(neforce::http::http_content::JSON_APP());
            response.body = R"({"error":"Missing cookie name"})";
        }
    } else if (request.method.is_delete()) {
        neforce::http::http_cookie_name name(request.parameter("name"));
        if (!name.cookie_name().empty()) {
            neforce::http::http_cookie ck;
            ck.name = name;
            ck.max_age = 0_s;
            ck.expires = neforce::datetime::epoch();
            response.cookies.emplace_back(move(ck));

            response.status = neforce::http::http_status::S2_OK;
            response.status_message = "OK";
            response.set_content_type(neforce::http::http_content::JSON_APP());
            response.body = R"({"name":")" + name.to_string() + R"("})";
        } else {
            response.status = neforce::http::http_status::S4_BAD_REQUEST;
            response.status_message = "Bad Request";
            response.set_content_type(neforce::http::http_content::JSON_APP());
            response.body = R"({"error":"Missing cookie name"})";
        }
    }
}

void start_server() {
    try {
        neforce::http::http_server server(neforce::ports(8080), 128);
        //         server.load_certificate(
        // #ifdef NEFORCE_PLATFORM_LINUX
        //                 "/home/huenqi/server.crt", "/home/huenqi/server.key"
        // #else
        //                 "D:/OpenSSL/server.crt", "D:/OpenSSL/server.key"
        // #endif
        //         );

        neforce::http::http_router& router = server.router();
        router.use(neforce::make_unique<neforce::http::logging_filter>());
        router.use(neforce::make_unique<neforce::http::cors_filter>("http://127.0.0.1:5500"));
        router.use(make_unique<neforce::http::static_file_filter>(res_root().str()));

        router.post("/old-link", [](neforce::http::http_request&, neforce::http::http_response& response) {
            response.redirect_url = "/new-link";
        });
        router.post("/forward-me", [](neforce::http::http_request&, neforce::http::http_response& response) {
            response.forward_path = "/forward-target";
        });
        router.post("/forward-target", [](neforce::http::http_request&, neforce::http::http_response& response) {
            response.status = neforce::http::http_status::S2_OK;
            response.status_message = "OK";
            response.body = "Forward Successfully";
        });

        router.get_post("/api/session",
                        [&server](neforce::http::http_request& request, neforce::http::http_response& response) {
                            handle_session_api(request, response, server);
                        });
        router.get_post("/api/session-attribute",
                        [&server](neforce::http::http_request& request, neforce::http::http_response& response) {
                            handle_session_attribute(request, response, server);
                        });
        router.post_delete("/api/cookie",
                           [](neforce::http::http_request& request, neforce::http::http_response& response) {
                               handle_cookie_api(request, response);
                           });

        router.get("/api/logger-test", [](neforce::http::http_request&, neforce::http::http_response& response) {
            response.status = neforce::http::http_status::S2_OK;
            response.status_message = "OK";
            response.body = "Logging filter test successful";
        });
        router.get("/api/data/*", [](neforce::http::http_request&, neforce::http::http_response& response) {
            response.status = neforce::http::http_status::S2_OK;
            response.status_message = "OK";
            response.set_content_type(neforce::http::http_content::JSON_APP());
            response.body = R"({"status":"success"})";
        });

        router.get("/", [](neforce::http::http_request&, neforce::http::http_response& response) {
            static neforce::file index{res_root() / "index.html"};
            response.status = neforce::http::http_status::S2_OK;
            response.status_message = "OK";
            response.set_content_type(neforce::http::http_content::HTML_TEXT());
            response.body = index.read();
        });

        router.get("/detail", [](neforce::http::http_request&, neforce::http::http_response& response) {
            static neforce::file detail{res_root() / "detail.html"};
            response.status = neforce::http::http_status::S2_OK;
            response.status_message = "OK";
            response.set_content_type(neforce::http::http_content::HTML_TEXT());
            response.body = detail.read();
        });

        router.get("/new-link", [](neforce::http::http_request&, neforce::http::http_response& response) {
            static neforce::file index{res_root() / "index.html"};
            response.status = neforce::http::http_status::S2_OK;
            response.status_message = "OK";
            response.set_content_type(neforce::http::http_content::HTML_TEXT());
            response.body = index.read();
        });

        router.get("/test", [](neforce::http::http_request&, neforce::http::http_response& response) {
            static neforce::file test{res_root() / "test.html"};
            response.status = neforce::http::http_status::S2_OK;
            response.status_message = "OK";
            response.set_content_type(neforce::http::http_content::HTML_TEXT());
            response.body = test.read();
        });

        router.set_not_found_handler([](neforce::http::http_request&, neforce::http::http_response& response) {
            static neforce::file err{res_root() / "404err.html"};
            response.status = neforce::http::http_status::S4_NOT_FOUNT;
            response.status_message = "Not Found";
            response.set_content_type(neforce::http::http_content::HTML_TEXT());
            response.body = err.read();
        });

        auto& ws = server.websocket();

        ws.route("/chat", [](neforce::shared_ptr<neforce::http::websocket_session> session) {
            neforce::println("New WebSocket connection established");

            neforce::weak_ptr<neforce::http::websocket_session> weak_session{session};

            session->set_message_handler(
                    [weak_session](const neforce::string& message, neforce::http::websocket_opcode opcode) {
                        neforce::println("Received: ", message);

                        if (auto session = weak_session.lock()) {
                            if (session->is_open()) {
                                session->send("Server received: " + message);
                            }
                        }
                    });

            session->set_close_handler([](neforce::http::websocket_status status, const neforce::string& reason) {
                neforce::println("Connection closed:", reason);
            });

            if (session->is_open()) {
                session->send("Welcome to chat room!");
            }
        });

        if (server.start()) {
            neforce::system_signal_manager::instance().start_monitoring();

            neforce::system_signal_manager::instance().register_handler(
                    neforce::system_signal_manager::event::INTERRUPT,
                    [&server](neforce::system_signal_manager::event event, void* context) -> bool {
                        if (event == neforce::system_signal_manager::event::INTERRUPT) {
                            neforce::println("Interrupting...");
                            server.stop();
                            neforce::immediate_exit(0);
                        }
                        return false;
                    });

            neforce::printcln(neforce::color::green(), "Press Ctrl+C to stop the server.");
            while (server.is_running()) {
                neforce::this_thread::sleep_for(1_s);
            }
            return;
        }
        neforce::printcln(neforce::color::red(), "Failed to start server!");
    } catch (const neforce::exception& e) {
        neforce::printcln(neforce::color::red(), "HTTP Server error: " + neforce::string(e.what()));
    }
}

int main() { start_server(); }
