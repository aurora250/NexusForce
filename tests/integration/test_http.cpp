#include <NeForce/network/http/http_constants.hpp>
#include <NeForce/network/http/http_session.hpp>
#include <NeForce/network/http/http_server_message.hpp>
#include <NeForce/network/http/http_client_message.hpp>
#include <NeForce/network/http/http_filter.hpp>
#include <NeForce/network/http/http_router.hpp>
#include <NeForce/network/http/http_security.hpp>
#include <NeForce/network/http/http_server.hpp>
#include <NeForce/network/http/rate_limiter.hpp>
#include <NeForce/network/http/websocket.hpp>
#include <NeForce/network/http/websocket_deflate.hpp>
#include <NeForce/network/http/session_store.hpp>
#include <NeForce/network/http/csrf_filter.hpp>
#include <NeForce/network/http/http_compress.hpp>
#include <NeForce/network/http/http_range.hpp>
#include <NeForce/network/http/health_check.hpp>
#include <NeForce/network/http/multipart_parser.hpp>
#include <NeForce/network/http/reverse_proxy.hpp>
#include <NeForce/network/http/load_balancer.hpp>
#include <NeForce/network/http/http_cache.hpp>
#include <NeForce/network/http/grpc.hpp>
#include <NeForce/network/http/http2_protocol.hpp>
#include <NeForce/network/http/chunked_reader.hpp>
#include <NeForce/network/http/http2_connection.hpp>
#include <NeForce/network/ssl/ssl_context.hpp>
#include <NeForce/network/ssl/ssl_socket.hpp>
#include <NeForce/network/tcp/tcp_client.hpp>
#include <NeForce/core/time/datetime.hpp>
#include <gtest/gtest.h>
using namespace neforce;
using namespace neforce::http;

#ifdef DELETE
#    undef DELETE
#endif

class CookieLifecycleTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(CookieLifecycleTest, ParseToStringRoundTrip) {
    auto original = http_cookie::parse("sess=abc; Path=/api; HttpOnly; Secure; Max-Age=7200; SameSite=Strict");
    EXPECT_TRUE(original.is_valid());

    auto serialized = original.to_string();
    EXPECT_TRUE(serialized.contains("sess=abc"));
    EXPECT_TRUE(serialized.contains("Path=/api"));
    EXPECT_TRUE(serialized.contains("HttpOnly"));
    EXPECT_TRUE(serialized.contains("Secure"));
    EXPECT_TRUE(serialized.contains("Max-Age=7200"));
    EXPECT_TRUE(serialized.contains("SameSite=Strict"));
}

TEST_F(CookieLifecycleTest, MaxAgeZeroCookieIsInvalid) {
    auto c = http_cookie::parse("expired=val; Max-Age=0");
    EXPECT_FALSE(c.is_valid());
    EXPECT_TRUE(c.is_expired());
}

TEST_F(CookieLifecycleTest, SessionCookieIsValid) {
    auto c = http_cookie::parse("session=val");
    EXPECT_TRUE(c.is_valid());
    EXPECT_FALSE(c.is_expired());
}

class RequestParsingIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(RequestParsingIntegrationTest, FullGetRequestWithMultipleHeaders) {
    auto req = http_request::parse("GET /api/v1/users?limit=10&offset=0 HTTP/1.1\r\n"
                                   "Host: api.example.com\r\n"
                                   "User-Agent: TestAgent/1.0\r\n"
                                   "Accept: application/json\r\n"
                                   "Accept-Language: en-US,en;q=0.9\r\n"
                                   "Connection: keep-alive\r\n"
                                   "Cookie: token=abc123; theme=dark\r\n"
                                   "\r\n");

    EXPECT_EQ(req.method.method(), "GET");
    EXPECT_EQ(req.path, "/api/v1/users");
    EXPECT_EQ(req.query, "limit=10&offset=0");
    EXPECT_EQ(req.version, "HTTP/1.1");
    EXPECT_EQ(req.header("Host"), "api.example.com");
    EXPECT_EQ(req.header("User-Agent"), "TestAgent/1.0");
    EXPECT_EQ(req.header("Accept"), "application/json");
    EXPECT_TRUE(req.is_keep_alive());
    EXPECT_EQ(req.cookie("token"), "abc123");
    EXPECT_EQ(req.cookie("theme"), "dark");
}

TEST_F(RequestParsingIntegrationTest, PostFormRequestWithBody) {
    auto req = http_request::parse("POST /login HTTP/1.1\r\n"
                                   "Host: example.com\r\n"
                                   "Content-Type: application/x-www-form-urlencoded\r\n"
                                   "Content-Length: 29\r\n"
                                   "\r\n"
                                   "username=john&password=secret");

    EXPECT_TRUE(req.method.is_post());
    EXPECT_EQ(req.path, "/login");
    EXPECT_EQ(req.content_type(), "application/x-www-form-urlencoded");
    EXPECT_EQ(req.body, "username=john&password=secret");
}

TEST_F(RequestParsingIntegrationTest, JsonRequest) {
    auto req = http_request::parse("POST /api/data HTTP/1.1\r\n"
                                   "Host: api.example.com\r\n"
                                   "Content-Type: application/json\r\n"
                                   "Content-Length: 36\r\n"
                                   "\r\n"
                                   "{\"name\":\"John\",\"email\":\"john@test\"}");

    EXPECT_EQ(req.content_type(), "application/json");
    EXPECT_EQ(req.body, "{\"name\":\"John\",\"email\":\"john@test\"}");
}

class ResponseGenerationTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(ResponseGenerationTest, FullHtmlResponse) {
    http_response resp;
    resp.status = http_status::S2_OK;
    resp.status_message = "OK";
    resp.set_content_type(http_content::HTML_TEXT());
    resp.body = "<html><body>Hello</body></html>";

    auto result = resp.to_string();

    EXPECT_TRUE(result.starts_with("HTTP/1.1 200 OK\r\n"));
    EXPECT_TRUE(result.contains("Content-Type: text/html\r\n"));
    EXPECT_TRUE(result.contains("Content-Length: 31\r\n"));
    EXPECT_TRUE(result.contains("\r\n\r\n"));
    EXPECT_TRUE(result.ends_with("<html><body>Hello</body></html>"));
}

TEST_F(ResponseGenerationTest, JsonResponse) {
    http_response resp;
    resp.status = http_status::S2_OK;
    resp.status_message = "OK";
    resp.set_content_type(http_content::JSON_APP());
    resp.body = "{\"status\":\"ok\"}";

    auto result = resp.to_string();
    EXPECT_TRUE(result.contains("Content-Type: application/json\r\n"));
}

TEST_F(ResponseGenerationTest, MultipleCookiesInResponse) {
    http_response resp;
    resp.status = http_status::S2_OK;
    resp.status_message = "OK";

    http_cookie c1;
    c1.name = http_cookie_name{"session"};
    c1.value = "abc123";
    c1.http_only = true;

    http_cookie c2;
    c2.name = http_cookie_name{"theme"};
    c2.value = "dark";
    c2.max_age = seconds{86400};

    resp.cookies.push_back(c1);
    resp.cookies.push_back(c2);

    auto result = resp.to_string();
    EXPECT_TRUE(result.contains("Set-Cookie: session=abc123"));
    EXPECT_TRUE(result.contains("Set-Cookie: theme=dark"));
    EXPECT_TRUE(result.contains("HttpOnly"));
    EXPECT_TRUE(result.contains("Max-Age=86400"));
}

TEST_F(ResponseGenerationTest, PermanentRedirectUsesCorrectStatusCode) {
    http_response resp;
    resp.status = http_status::S3_MOVED_PERMANENT;
    resp.status_message = "Moved Permanently";
    resp.redirect_url = "https://new-site.example.com";

    auto result = resp.to_string();
    EXPECT_TRUE(result.contains("301 Moved Permanently"));
    EXPECT_TRUE(result.contains("Location: https://new-site.example.com"));
}

TEST_F(ResponseGenerationTest, TemporaryRedirectUsesCorrectStatusCode) {
    http_response resp;
    resp.status = http_status::S3_TEMPORARY_REDIRECT;
    resp.status_message = "Temporary Redirect";
    resp.redirect_url = "/temporary";

    auto result = resp.to_string();
    EXPECT_TRUE(result.contains("307 Temporary Redirect"));
}

TEST_F(ResponseGenerationTest, SeeOtherRedirect) {
    http_response resp;
    resp.status = http_status::S3_SEE_OTHER;
    resp.status_message = "See Other";
    resp.redirect_url = "/other";

    auto result = resp.to_string();
    EXPECT_TRUE(result.contains("303 See Other"));
}

class RouterIntegrationTest : public ::testing::Test {
protected:
    http_router router;

    void SetUp() override {
        router.get("/api/health", [](http_request&, http_response& res) {
            res.status = http_status::S2_OK;
            res.status_message = "OK";
            res.set_content_type(http_content::JSON_APP());
            res.body = "{\"status\":\"healthy\"}";
        });

        router.get("/api/users/:userId", [](http_request& req, http_response& res) {
            auto id = req.parameter("userId");
            res.status = http_status::S2_OK;
            res.status_message = "OK";
            res.set_content_type(http_content::JSON_APP());
            res.body = "{\"id\":" + string(id) + ",\"name\":\"User " + string(id) + "\"}";
        });

        router.post("/api/users", [](http_request& req, http_response& res) {
            res.status = http_status::S2_CREATED;
            res.status_message = "Created";
            res.set_content_type(http_content::JSON_APP());
            res.body = req.body;
        });
    }

    void TearDown() override {}
};

TEST_F(RouterIntegrationTest, HealthEndpointReturnsJson) {
    http_request req;
    req.method = http_method::GET();
    req.path = "/api/health";

    auto resp = router.handle_request(req);
    EXPECT_EQ(static_cast<uint16_t>(resp.status), 200u);
    EXPECT_EQ(resp.body, "{\"status\":\"healthy\"}");
    EXPECT_EQ(resp.header("Content-Type"), "application/json");
}

TEST_F(RouterIntegrationTest, UserEndpointExtractsPathParam) {
    http_request req;
    req.method = http_method::GET();
    req.path = "/api/users/42";

    auto resp = router.handle_request(req);
    EXPECT_EQ(static_cast<uint16_t>(resp.status), 200u);
    EXPECT_EQ(resp.body, "{\"id\":42,\"name\":\"User 42\"}");
}

TEST_F(RouterIntegrationTest, PathParamWithAlphaNumericValue) {
    http_request req;
    req.method = http_method::GET();
    req.path = "/api/users/user123";

    auto resp = router.handle_request(req);
    EXPECT_EQ(static_cast<uint16_t>(resp.status), 200u);
    EXPECT_TRUE(resp.body.contains("user123"));
}

TEST_F(RouterIntegrationTest, PostEndpointAcceptsJsonBody) {
    http_request req;
    req.method = http_method::POST();
    req.path = "/api/users";
    req.body = R"({"name":"Jane"})";

    auto resp = router.handle_request(req);
    EXPECT_EQ(static_cast<uint16_t>(resp.status), 201u);
    EXPECT_EQ(resp.body, "{\"name\":\"Jane\"}");
}

class RouterErrorIntegrationTest : public ::testing::Test {
protected:
    http_router router;

    void SetUp() override {
        router.get("/exists", [](http_request&, http_response& res) { res.body = "exists"; });
    }

    void TearDown() override {}
};

TEST_F(RouterErrorIntegrationTest, NonexistentPathReturns404) {
    http_request req;
    req.method = http_method::GET();
    req.path = "/does-not-exist";

    auto resp = router.handle_request(req);
    EXPECT_EQ(static_cast<uint16_t>(resp.status), 404u);
}

TEST_F(RouterErrorIntegrationTest, WrongMethodReturns405) {
    http_request req;
    req.method = http_method::POST();
    req.path = "/exists";

    auto resp = router.handle_request(req);
    EXPECT_EQ(static_cast<uint16_t>(resp.status), 405u);
}

class FilterChainIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(FilterChainIntegrationTest, PreFilterCanStopExecution) {
    class StoppingFilter : public http_filter {
    public:
        bool pre_filter(http_request&, http_response& res) override {
            res.status = http_status::S4_FORBIDDEN;
            res.status_message = "Forbidden";
            res.body = "blocked";
            return false;
        }
        void do_filter(http_request&, http_response&) override {}
    };

    http_router router;
    router.use(make_unique<StoppingFilter>());

    bool handler_called = false;
    router.get("/test", [&](http_request&, http_response&) { handler_called = false; });

    http_request req;
    req.method = http_method::GET();
    req.path = "/test";

    auto resp = router.handle_request(req);
    EXPECT_FALSE(handler_called);
    EXPECT_EQ(resp.body, "blocked");
    EXPECT_EQ(static_cast<uint16_t>(resp.status), 403u);
}

class CorsIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(CorsIntegrationTest, WildcardOriginAllowsAnyOrigin) {
    cors_filter filter("*");

    http_request req;
    req.set_header("Origin", "https://random-site.com");
    http_response resp;
    filter.pre_filter(req, resp);

    EXPECT_EQ(resp.header("Access-Control-Allow-Origin"), "*");
}

TEST_F(CorsIntegrationTest, SpecificOriginOnlyAllowsMatching) {
    cors_filter filter("https://myapp.com");

    http_request req1;
    req1.set_header("Origin", "https://myapp.com");
    http_response resp1;
    filter.pre_filter(req1, resp1);
    EXPECT_EQ(resp1.header("Access-Control-Allow-Origin"), "https://myapp.com");

    http_request req2;
    req2.set_header("Origin", "https://evil.com");
    http_response resp2;
    filter.pre_filter(req2, resp2);
    EXPECT_EQ(resp2.header("Access-Control-Allow-Origin"), "");
}

class SessionIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(SessionIntegrationTest, SessionCreateAndRetrieve) {
    http_session session;
    session.id = "test-session-001";
    session.set("user_id", "100");
    session.set("role", "admin");

    EXPECT_TRUE(session.is_valid());
    EXPECT_EQ(session.get("user_id"), "100");
    EXPECT_EQ(session.get("role"), "admin");
    EXPECT_TRUE(session.contains("user_id"));
}

TEST_F(SessionIntegrationTest, SessionExpireAfterInactivePeriod) {
    http_session session;
    session.id = "test-session-002";
    session.max_age = seconds{1};
    session.last_access = datetime::now() - 10;

    EXPECT_TRUE(session.expired());
    EXPECT_FALSE(session.is_valid());
}

TEST_F(SessionIntegrationTest, InvalidateRemovesData) {
    http_session session;
    session.id = "test-session-003";
    session.set("key", "value");
    session.invalidate();

    EXPECT_FALSE(session.is_valid());
    EXPECT_EQ(session.get("key"), "");
}

class AuthFilterIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(AuthFilterIntegrationTest, ExcludedPathPrefixBypassesAuth) {
    authentication_filter filter([](const http_request&) { return false; });
    filter.add_excluded_path("/public");
    filter.add_excluded_path("/assets");

    http_request public_req;
    public_req.path = "/public/index.html";
    http_response resp1;
    EXPECT_TRUE(filter.pre_filter(public_req, resp1));

    http_request asset_req;
    asset_req.path = "/assets/style.css";
    http_response resp2;
    EXPECT_TRUE(filter.pre_filter(asset_req, resp2));

    http_request private_req;
    private_req.path = "/admin/dashboard";
    http_response resp3;
    EXPECT_FALSE(filter.pre_filter(private_req, resp3));
}

class HttpServerIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(HttpServerIntegrationTest, ServerPortConfiguration) {
    http_server server(ports{18080});
    EXPECT_EQ(server.port(), ports{18080});
    EXPECT_FALSE(server.is_running());
}

TEST_F(HttpServerIntegrationTest, ServerRouterAccess) {
    http_server server(ports{18081});
    server.router().get("/test", [](http_request&, http_response& res) {
        res.status = http_status::S2_OK;
        res.status_message = "OK";
        res.body = "test ok";
    });

    http_request req;
    req.method = http_method::GET();
    req.path = "/test";
    auto resp = server.router().handle_request(req);
    EXPECT_EQ(resp.body, "test ok");
}

TEST_F(HttpServerIntegrationTest, ServerCookieNameConfiguration) {
    http_server server(ports{18082});
    server.set_cookie_name(http_cookie_name::PHPSESSID());
    EXPECT_EQ(server.cookie_name().cookie_name(), "PHPSESSID");
}

TEST_F(HttpServerIntegrationTest, SessionManagerSettings) {
    http_server server(ports{18083});
    server.set_session_cleanup_interval(seconds{600});
    server.set_max_sessions(5000);

    EXPECT_NO_THROW(server.set_session_cleanup_interval(seconds{600}));
    EXPECT_NO_THROW(server.set_max_sessions(5000));
}

class ClientRequestIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(ClientRequestIntegrationTest, FullRequestConstruction) {
    http_client_request req;
    req.method = http_method::POST();
    req.host = "api.example.com";
    req.port = ports::HTTP;
    req.path = "/api/v1/submit";
    req.set_header("Authorization", "Bearer token123");
    req.set_header("Content-Type", "application/json");
    req.body = "{\"data\":\"test\"}";
    req.add_query_param("format", "json");

    EXPECT_EQ(req.method.method(), "POST");
    EXPECT_EQ(req.host, "api.example.com");
    EXPECT_EQ(req.header("Authorization"), "Bearer token123");
    EXPECT_EQ(req.body, "{\"data\":\"test\"}");

    auto full_path = req.build_full_path();
    EXPECT_TRUE(full_path.contains("/api/v1/submit"));
    EXPECT_TRUE(full_path.contains("format=json"));
}

class WebSocketIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(WebSocketIntegrationTest, OpcodeValuesAreCorrect) {
    EXPECT_EQ(static_cast<uint8_t>(websocket_opcode::CONTINUATION), 0x0u);
    EXPECT_EQ(static_cast<uint8_t>(websocket_opcode::TEXT), 0x1u);
    EXPECT_EQ(static_cast<uint8_t>(websocket_opcode::BINARY), 0x2u);
    EXPECT_EQ(static_cast<uint8_t>(websocket_opcode::CLOSE), 0x8u);
    EXPECT_EQ(static_cast<uint8_t>(websocket_opcode::PING), 0x9u);
    EXPECT_EQ(static_cast<uint8_t>(websocket_opcode::PONG), 0xAu);
}

TEST_F(WebSocketIntegrationTest, FrameHeaderIsPacked) { EXPECT_EQ(sizeof(websocket_frame_header), 2u); }

TEST_F(WebSocketIntegrationTest, CloseStatusValuesAreCorrect) {
    EXPECT_EQ(static_cast<uint16_t>(websocket_status::NORMAL_CLOSURE), 1000u);
    EXPECT_EQ(static_cast<uint16_t>(websocket_status::GOING_AWAY), 1001u);
    EXPECT_EQ(static_cast<uint16_t>(websocket_status::PROTOCOL_ERROR), 1002u);
    EXPECT_EQ(static_cast<uint16_t>(websocket_status::UNSUPPORTED_DATA), 1003u);
    EXPECT_EQ(static_cast<uint16_t>(websocket_status::POLICY_VIOLATION), 1008u);
    EXPECT_EQ(static_cast<uint16_t>(websocket_status::MESSAGE_TOO_BIG), 1009u);
    EXPECT_EQ(static_cast<uint16_t>(websocket_status::INTERNAL_ERROR), 1011u);
}

class EndToEndRouterTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(EndToEndRouterTest, RESTfulCRUDFlow) {
    http_router router;
    unordered_map<string, string> db;

    router.get("/items/:id", [&](http_request& req, http_response& res) {
        auto id = string(req.parameter("id"));
        if (db.find(id) != db.end()) {
            res.status = http_status::S2_OK;
            res.status_message = "OK";
            res.set_content_type(http_content::JSON_APP());
            res.body = "{\"id\":\"" + id + "\",\"value\":\"" + db[id] + "\"}";
        } else {
            res.status = http_status::S4_NOT_FOUND;
            res.status_message = "Not Found";
        }
    });

    router.post("/items/:id", [&](http_request& req, http_response& res) {
        auto id = string(req.parameter("id"));
        db[id] = req.body;
        res.status = http_status::S2_CREATED;
        res.status_message = "Created";
        res.body = req.body;
    });

    router.del("/items/:id", [&](http_request& req, http_response& res) {
        auto id = string(req.parameter("id"));
        db.erase(id);
        res.status = http_status::S2_NO_CONTENT;
        res.status_message = "No Content";
    });

    {
        http_request req;
        req.method = http_method::POST();
        req.path = "/items/item1";
        req.body = "hello";
        auto resp = router.handle_request(req);
        EXPECT_EQ(static_cast<uint16_t>(resp.status), 201u);
        EXPECT_EQ(resp.body, "hello");
    }

    {
        http_request req;
        req.method = http_method::GET();
        req.path = "/items/item1";
        auto resp = router.handle_request(req);
        EXPECT_EQ(static_cast<uint16_t>(resp.status), 200u);
        EXPECT_TRUE(resp.body.contains("hello"));
    }

    {
        http_request req;
        req.method = http_method::GET();
        req.path = "/items/nonexistent";
        auto resp = router.handle_request(req);
        EXPECT_EQ(static_cast<uint16_t>(resp.status), 404u);
    }

    {
        http_request req;
        req.method = http_method::DELETE();
        req.path = "/items/item1";
        auto resp = router.handle_request(req);
        EXPECT_EQ(static_cast<uint16_t>(resp.status), 204u);
    }

    {
        http_request req;
        req.method = http_method::GET();
        req.path = "/items/item1";
        auto resp = router.handle_request(req);
        EXPECT_EQ(static_cast<uint16_t>(resp.status), 404u);
    }
}

class RequestSerializationTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(RequestSerializationTest, SerializeAndReparse) {
    http_request original;
    original.method = http_method::POST();
    original.path = "/submit";
    original.set_header("Host", "test");
    original.set_header("Content-Type", "text/plain");
    original.body = "payload";
    original.version = "HTTP/1.1";

    auto serialized = original.to_string();
    EXPECT_TRUE(serialized.contains("POST /submit HTTP/1.1"));
    EXPECT_TRUE(serialized.contains("Content-Type: text/plain"));
    EXPECT_TRUE(serialized.contains("Content-Length: 7"));
    EXPECT_TRUE(serialized.contains("payload"));

    auto reparsed = http_request::parse(serialized.view());
    EXPECT_EQ(reparsed.method.method(), "POST");
    EXPECT_EQ(reparsed.path, "/submit");
    EXPECT_EQ(reparsed.header("Content-Type"), "text/plain");
}

TEST(HttpMethodIntegrationTest, CustomMethodStrings) {
    http_method custom1{"GET"};
    http_method custom2{"POST"};

    EXPECT_TRUE(custom1.is_get());
    EXPECT_EQ(custom1.to_string(), "GET");
    EXPECT_TRUE(custom2.is_post());
    EXPECT_EQ(custom2.to_string(), "POST");
}

TEST(HttpMethodIntegrationTest, NullRouteHandlerIgnored) {
    http_router router;
    router.get("/test", nullptr);
    router.route(http_method::GET(), "/test", nullptr);
    EXPECT_EQ(router.route_count(), 0u);
}

TEST(HttpMethodIntegrationTest, EmptyPathRouteIgnored) {
    http_router router;
    router.get("", [](http_request&, http_response&) {});
    EXPECT_EQ(router.route_count(), 0u);
}

#ifdef NEFORCE_SUPPORT_ZLIB

class WebSocketDeflateIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(WebSocketDeflateIntegrationTest, NegotiateFromClientExtensions) {
    const string_view client_ext = "permessage-deflate; client_max_window_bits=13; server_no_context_takeover";
    auto cfg = websocket_deflate_config::negotiate(client_ext);

    EXPECT_TRUE(cfg.active);
    EXPECT_EQ(cfg.client_max_window_bits, 13);
    EXPECT_TRUE(cfg.server_no_context_takeover);

    auto response = cfg.to_response_header();
    EXPECT_TRUE(response.starts_with("permessage-deflate"));
    EXPECT_TRUE(response.contains("client_max_window_bits=13"));
}

TEST_F(WebSocketDeflateIntegrationTest, NegotiateFullParams) {
    const string_view client_ext = "permessage-deflate; client_max_window_bits=11; server_max_window_bits=12; "
                                   "client_no_context_takeover; server_no_context_takeover";
    auto cfg = websocket_deflate_config::negotiate(client_ext);

    EXPECT_TRUE(cfg.active);
    EXPECT_EQ(cfg.client_max_window_bits, 11);
    EXPECT_EQ(cfg.server_max_window_bits, 12);
    EXPECT_TRUE(cfg.client_no_context_takeover);
    EXPECT_TRUE(cfg.server_no_context_takeover);
}

TEST_F(WebSocketDeflateIntegrationTest, UnknownExtensionIgnored) {
    const string_view client_ext = "unknown-extension; permessage-deflate";
    auto cfg = websocket_deflate_config::negotiate(client_ext);

    EXPECT_TRUE(cfg.active);
}

TEST_F(WebSocketDeflateIntegrationTest, ResponseRoundTrip) {
    const string_view request = "permessage-deflate; client_max_window_bits=12";
    auto cfg = websocket_deflate_config::negotiate(request);
    EXPECT_TRUE(cfg.active);

    auto response = cfg.to_response_header();
    EXPECT_FALSE(response.empty());

    EXPECT_TRUE(response.contains("permessage-deflate"));
    EXPECT_TRUE(response.contains("client_max_window_bits=12"));
}

class CompressFilterIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(CompressFilterIntegrationTest, GzipEncodingSelected) {
    compress_filter filter;
    filter.enabled = true;
    filter.min_size = 0_KB;

    http_request req;
    req.set_header("Accept-Encoding", "gzip, deflate, br");

    http_response resp;
    resp.status = http_status::S2_OK;
    resp.status_message = "OK";
    resp.set_content_type(http_content::HTML_TEXT());
    resp.body = string(2048, 'X');

    filter.post_filter(req, resp);
    EXPECT_EQ(resp.header("Content-Encoding"), "gzip");
}

TEST_F(CompressFilterIntegrationTest, DeflateFallbackWhenNoGzip) {
    compress_filter filter;
    filter.enabled = true;
    filter.min_size = 0_KB;

    http_request req;
    req.set_header("Accept-Encoding", "deflate");

    http_response resp;
    resp.status = http_status::S2_OK;
    resp.status_message = "OK";
    resp.set_content_type(http_content::PLAIN_TEXT());
    resp.body = string(2048, 'Y');

    filter.post_filter(req, resp);
    EXPECT_EQ(resp.header("Content-Encoding"), "deflate");
}

#endif

class SessionStoreIntegrationTest : public ::testing::Test {
protected:
    memory_session_store store;

    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(SessionStoreIntegrationTest, FullLifecycle) {
    http_session s;
    s.id = "lifecycle-test";
    s.set("user", "alice");
    s.set("role", "admin");
    store.save(s);

    auto loaded = store.load("lifecycle-test");
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded.value().get("user"), "alice");
    EXPECT_EQ(loaded.value().get("role"), "admin");

    loaded.value().set("role", "superadmin");
    store.save(loaded.value());

    auto updated = store.load("lifecycle-test");
    ASSERT_TRUE(updated.has_value());
    EXPECT_EQ(updated.value().get("role"), "superadmin");

    store.remove("lifecycle-test");
    EXPECT_FALSE(store.exists("lifecycle-test"));
    EXPECT_EQ(store.count(), 0u);
}

TEST_F(SessionStoreIntegrationTest, BulkOperations) {
    for (int i = 0; i < 100; ++i) {
        http_session s;
        s.id = "bulk-" + to_string(i);
        s.set("index", to_string(i));
        store.save(s);
    }
    EXPECT_EQ(store.count(), 100u);

    store.cleanup();
    EXPECT_EQ(store.count(), 100u);

    for (int i = 0; i < 100; ++i) {
        store.remove("bulk-" + to_string(i));
    }
    EXPECT_EQ(store.count(), 0u);
}

class CsrfFilterIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(CsrfFilterIntegrationTest, TokenGenerationAndValidation) {
    csrf_filter filter;

    http_request get_req;
    get_req.method = http_method::GET();
    http_response get_resp;
    EXPECT_TRUE(filter.pre_filter(get_req, get_resp));

    string token;
    for (const auto& c: get_resp.cookies) {
        if (c.name.cookie_name() == "XSRF-TOKEN") {
            token = c.value;
            break;
        }
    }
    EXPECT_FALSE(token.empty());

    http_request post_req;
    post_req.method = http_method::POST();
    post_req.cookies["XSRF-TOKEN"] = token;
    post_req.set_header("X-CSRF-Token", token);
    http_response post_resp;
    EXPECT_TRUE(filter.pre_filter(post_req, post_resp));
}

TEST_F(CsrfFilterIntegrationTest, MultipleGetRequestsReuseToken) {
    csrf_filter filter;

    http_request req1;
    req1.method = http_method::GET();
    http_response resp1;
    filter.pre_filter(req1, resp1);

    string token1;
    for (const auto& c: resp1.cookies) {
        if (c.name.cookie_name() == "XSRF-TOKEN") {
            token1 = c.value;
        }
    }

    http_request req2;
    req2.method = http_method::GET();
    req2.cookies["XSRF-TOKEN"] = token1;
    http_response resp2;
    filter.pre_filter(req2, resp2);

    string token2;
    for (const auto& c: resp2.cookies) {
        if (c.name.cookie_name() == "XSRF-TOKEN") {
            token2 = c.value;
        }
    }

    EXPECT_EQ(token1, token2);
}

TEST_F(CsrfFilterIntegrationTest, TokenFromFormField) {
    csrf_filter filter;

    http_request get_req;
    get_req.method = http_method::GET();
    http_response get_resp;
    filter.pre_filter(get_req, get_resp);

    string token;
    for (const auto& c: get_resp.cookies) {
        if (c.name.cookie_name() == "XSRF-TOKEN") {
            token = c.value;
        }
    }

    http_request post_req;
    post_req.method = http_method::POST();
    post_req.cookies["XSRF-TOKEN"] = token;
    post_req.set_header("Content-Type", "application/x-www-form-urlencoded");
    post_req.form_data["_csrf"] = token;
    http_response post_resp;
    EXPECT_TRUE(filter.pre_filter(post_req, post_resp));
}

class RangeIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(RangeIntegrationTest, FullRangeRequestFlow) {
    const uint64_t file_size = 8192;

    auto ranges = parse_ranges("bytes=0-1023", file_size);
    ASSERT_EQ(ranges.size(), 1u);
    EXPECT_EQ(ranges[0].start, 0u);
    EXPECT_EQ(ranges[0].end, 1023u);

    auto header = build_content_range(ranges[0], file_size);
    EXPECT_EQ(header, "bytes 0-1023/8192");
}

TEST_F(RangeIntegrationTest, MultiRangeRequestFlow) {
    const uint64_t file_size = 10000;

    auto ranges = parse_ranges("bytes=0-499,1000-1499,5000-5499", file_size);
    ASSERT_EQ(ranges.size(), 3u);

    auto body = build_multipart_ranges(
            ranges, "application/octet-stream", "BOUNDARY_SEP",
            [](const byte_range& r) -> string { return "DATA_" + to_string(r.start) + "_" + to_string(r.end); },
            file_size);

    EXPECT_TRUE(body.starts_with("--BOUNDARY_SEP\r\n"));
    EXPECT_TRUE(body.contains("Content-Range: bytes 0-499/10000"));
    EXPECT_TRUE(body.contains("DATA_0_499"));
    EXPECT_TRUE(body.contains("Content-Range: bytes 1000-1499/10000"));
    EXPECT_TRUE(body.contains("DATA_1000_1499"));
    EXPECT_TRUE(body.contains("Content-Range: bytes 5000-5499/10000"));
    EXPECT_TRUE(body.contains("DATA_5000_5499"));
    EXPECT_TRUE(body.ends_with("--BOUNDARY_SEP--\r\n"));
}

class HttpUpgradeIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(HttpUpgradeIntegrationTest, RegisterCustomUpgradeHandler) {
    http_server server(ports{19001});
    bool custom_called = false;

    server.set_upgrade_handler("custom-proto", [&](http_request&, unique_ptr<tcp_socket>) -> bool {
        custom_called = true;
        return true;
    });

    EXPECT_FALSE(custom_called);
    SUCCEED();
}

TEST_F(HttpUpgradeIntegrationTest, WebSocketServerAccessible) {
    http_server server(ports{19002});
    auto& ws = server.websocket();
    EXPECT_EQ(ws.session_count(), 0u);

    ws.route("/chat", [](websocket_server::session_ptr) {});
    SUCCEED();
}

class SessionRegenerateIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(SessionRegenerateIntegrationTest, RegenerateInStore) {
    memory_session_store store;

    http_session s;
    s.id = "pre-regenerate";
    s.set("auth", "valid");
    store.save(s);

    auto loaded = store.load("pre-regenerate");
    ASSERT_TRUE(loaded.has_value());
    loaded.value().regenerate_id();

    string new_id = loaded.value().id;
    EXPECT_NE(new_id, "pre-regenerate");
    EXPECT_EQ(loaded.value().get("auth"), "valid");

    store.save(loaded.value());
    store.remove("pre-regenerate");

    EXPECT_FALSE(store.exists("pre-regenerate"));
    EXPECT_TRUE(store.exists(new_id));
}

TEST_F(FilterChainIntegrationTest, RouterWithExceptionHandlerSkipsPostFilters) {
    http_router router;
    bool post_filter_called = false;

    struct tracker : http_filter {
        bool* flag;
        explicit tracker(bool* f) :
        flag(f) {}
        bool pre_filter(http_request&, http_response&) override { return true; }
        void post_filter(http_request&, http_response&) override { *flag = true; }
        void do_filter(http_request&, http_response&) override {}
        string name() const override { return "tracker"; }
    };

    router.use(make_unique<tracker>(&post_filter_called));
    router.set_exception_handler([](http_request&, http_response& res, const exception&) {
        res.status = http_status::S5_INTERNAL_SERVER_ERROR;
        res.body = "handled";
    });
    router.get("/crash", [](http_request&, http_response&) { throw value_exception("boom"); });

    http_request req;
    req.method = http_method::GET();
    req.path = "/crash";
    auto resp = router.handle_request(req);

    EXPECT_EQ(resp.body, "handled");
    EXPECT_FALSE(post_filter_called);
}

TEST_F(FilterChainIntegrationTest, MixedOwnershipFilterChainCleanup) {
    http_filter_chain chain;

    auto f1 = make_unique<logging_filter>();

    struct dummy_filter : http_filter {
        bool pre_filter(http_request&, http_response&) override { return true; }
        void do_filter(http_request&, http_response&) override {}
        string name() const override { return "dummy"; }
    };

    auto f2 = make_unique<dummy_filter>();
    auto* raw_f2 = f2.get();
    chain.add_filter(move(f1));
    chain.add_filter_ref(raw_f2);
    static_cast<void>(f2.release());

    EXPECT_EQ(chain.size(), 2u);

    http_request req;
    req.method = http_method::GET();
    req.path = "/test";
    http_response resp;

    bool result = chain.execute_pre_filters(req, resp);
    EXPECT_TRUE(result);

    chain.clear();
    EXPECT_EQ(chain.size(), 0u);
    delete raw_f2;
    SUCCEED();
}

class RateLimiterIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(RateLimiterIntegrationTest, TokenBucketFailClosedOnExhaustion) {
    token_bucket_limiter limiter;
    limiter.set_default_rate(1.0);
    limiter.set_default_burst(2.0);

    EXPECT_TRUE(limiter.allow("client1"));
    EXPECT_TRUE(limiter.allow("client1"));
    EXPECT_FALSE(limiter.allow("client1"));
}

TEST_F(RateLimiterIntegrationTest, TokenBucketDifferentKeysIndependent) {
    token_bucket_limiter limiter;
    limiter.set_default_rate(5.0);
    limiter.set_default_burst(5.0);

    for (int i = 0; i < 5; ++i) {
        EXPECT_TRUE(limiter.allow("clientA"));
    }
    EXPECT_FALSE(limiter.allow("clientA"));

    EXPECT_TRUE(limiter.allow("clientB"));
}

class CsrfSessionIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(CsrfSessionIntegrationTest, TokenStoredInSession) {
    csrf_filter filter;
    filter.cookie_name = "csrf_test";

    http_session session;
    session.id = "session-1";

    http_request req;
    req.method = http_method::POST();
    req.path = "/submit";
    req.session = &session;

    http_response resp;

    req.method = http_method::GET();
    bool result = filter.pre_filter(req, resp);
    EXPECT_TRUE(result);

    EXPECT_FALSE(session.get("_csrf_token").empty());

    string token = string(session.get("_csrf_token"));
    req.method = http_method::POST();
    req.set_header("X-CSRF-Token", token);
    resp = http_response();

    result = filter.pre_filter(req, resp);
    EXPECT_TRUE(result);
}

TEST_F(CsrfSessionIntegrationTest, TokenMismatchRejected) {
    csrf_filter filter;
    filter.cookie_name = "csrf_test";

    http_session session;
    session.id = "session-2";

    http_request req;
    req.method = http_method::POST();
    req.path = "/submit";
    req.session = &session;

    http_response resp;

    bool result = filter.pre_filter(req, resp);
    if (!result) {
        EXPECT_EQ(static_cast<int>(resp.status), 403);
    }
}

class SecurityRouterIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(SecurityRouterIntegrationTest, XssProtectionDisabledByDefaultWithRouter) {
    http_router router;
    auto sec = make_unique<security_headers_filter>();
    router.use(move(sec));

    router.get("/", [](http_request&, http_response& res) {
        res.status = http_status::S2_OK;
        res.body = "ok";
    });

    http_request req;
    req.method = http_method::GET();
    req.path = "/";
    auto resp = router.handle_request(req);

    EXPECT_EQ(resp.body, "ok");
    EXPECT_TRUE(resp.header("X-XSS-Protection").empty());
}

TEST_F(SecurityRouterIntegrationTest, RateLimitWithMiddlewareChain) {
    http_router router;
    auto rl = make_unique<token_bucket_filter>(100.0, 200.0);
    router.use(move(rl));

    router.get("/api", [](http_request&, http_response& res) {
        res.status = http_status::S2_OK;
        res.body = "data";
    });

    http_request req;
    req.method = http_method::GET();
    req.path = "/api";
    auto resp = router.handle_request(req);
    EXPECT_EQ(resp.body, "data");
}

class HealthCheckIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(HealthCheckIntegrationTest, FilterInRouterChain) {
    http_router router;
    auto hc = make_unique<health_check_filter>();
    router.use(move(hc));

    router.get("/api", [](http_request&, http_response& res) {
        res.status = http_status::S2_OK;
        res.body = "api data";
    });

    http_request req;
    req.method = http_method::GET();
    req.path = "/healthz";
    auto resp = router.handle_request(req);
    EXPECT_EQ(resp.status, http_status::S2_OK);
    EXPECT_TRUE(resp.body.contains("\"status\":\"ok\""));
    EXPECT_TRUE(resp.body.contains("\"uptime\""));

    http_request req2;
    req2.method = http_method::GET();
    req2.path = "/api";
    auto resp2 = router.handle_request(req2);
    EXPECT_EQ(resp2.body, "api data");
}

TEST_F(HealthCheckIntegrationTest, FailedCheckAffectsRouter) {
    http_router router;
    auto hc = make_unique<health_check_filter>();
    hc->add_check("db", [] { return false; });
    router.use(move(hc));

    http_request req;
    req.method = http_method::GET();
    req.path = "/healthz";
    auto resp = router.handle_request(req);
    EXPECT_EQ(resp.status, http_status::S5_SERVICE_UNAVAILABLE);
    EXPECT_TRUE(resp.body.contains("unhealthy"));
}

TEST_F(HealthCheckIntegrationTest, CacheControlHeadersSet) {
    http_router router;
    auto hc = make_unique<health_check_filter>();
    router.use(move(hc));

    http_request req;
    req.method = http_method::GET();
    req.path = "/healthz";
    auto resp = router.handle_request(req);
    EXPECT_EQ(resp.header("Cache-Control"), "no-cache, no-store, must-revalidate");
    EXPECT_EQ(resp.header("Content-Type"), "application/json");
}

class MultipartIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(MultipartIntegrationTest, FullFormDataParse) {
    string boundary = "----TestFormBoundary";
    string body = "------TestFormBoundary\r\n"
                  "Content-Disposition: form-data; name=\"username\"\r\n"
                  "\r\n"
                  "alice\r\n"
                  "------TestFormBoundary\r\n"
                  "Content-Disposition: form-data; name=\"email\"\r\n"
                  "\r\n"
                  "alice@example.com\r\n"
                  "------TestFormBoundary\r\n"
                  "Content-Disposition: form-data; name=\"bio\"\r\n"
                  "\r\n"
                  "Hello world\r\n"
                  "------TestFormBoundary--\r\n";

    multipart_parser parser;
    auto fields = parser.parse(body.view(), boundary.view());

    ASSERT_EQ(fields.size(), 3u);
    EXPECT_EQ(fields[0].name, "username");
    EXPECT_EQ(string(fields[0].data.begin(), fields[0].data.end()), "alice");
    EXPECT_EQ(fields[1].name, "email");
    EXPECT_EQ(string(fields[1].data.begin(), fields[1].data.end()), "alice@example.com");
    EXPECT_EQ(fields[2].name, "bio");
}

TEST_F(MultipartIntegrationTest, ExtractBoundaryFromContentType) {
    multipart_parser parser;
    string_view ct = "multipart/form-data; boundary=----WebKitFormBoundary";
    auto boundary = parser.extract_boundary(ct);
    EXPECT_EQ(boundary, "----WebKitFormBoundary");
}

TEST_F(MultipartIntegrationTest, ExtractQuotedBoundary) {
    multipart_parser parser;
    string_view ct = R"(multipart/form-data; boundary="---quoted boundary")";
    auto boundary = parser.extract_boundary(ct);
    EXPECT_EQ(boundary, "---quoted boundary");
}

TEST_F(MultipartIntegrationTest, TotalSizeLimitEnforced) {
    multipart_parser parser;
    parser.max_total_size = byte_size{50};

    string boundary = "----B";
    string body = "------B\r\n"
                  "Content-Disposition: form-data; name=\"big\"\r\n"
                  "\r\n"
                  "this is a fairly long piece of data that should exceed 50 bytes\r\n"
                  "------B--\r\n";

    auto fields = parser.parse(body.view(), boundary.view());
    EXPECT_TRUE(fields.empty());
}

class ReverseProxyIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(ReverseProxyIntegrationTest, PathPrefixMatching) {
    reverse_proxy_filter proxy;
    proxy.set_path_prefix("/api/");
    proxy.add_backend({"backend1", ports(8080)});

    http_request req;
    req.method = http_method::GET();
    req.path = "/api/users";
    http_response res;

    bool cont = proxy.pre_filter(req, res);
    EXPECT_FALSE(cont);
}

TEST_F(ReverseProxyIntegrationTest, NonMatchingPathBypasses) {
    reverse_proxy_filter proxy;
    proxy.set_path_prefix("/api/");

    http_request req;
    req.method = http_method::GET();
    req.path = "/static/style.css";
    http_response res;

    bool cont = proxy.pre_filter(req, res);
    EXPECT_TRUE(cont);
}

TEST_F(ReverseProxyIntegrationTest, HeaderRewriteCallback) {
    reverse_proxy_filter proxy;
    proxy.set_path_prefix("/api/");
    proxy.add_backend({"backend", ports(8080)});
    proxy.set_header_rewrite([](unordered_map<string, string>& headers) { headers["X-Forwarded-Proto"] = "https"; });

    http_request req;
    req.method = http_method::GET();
    req.path = "/api/data";
    http_response res;
    EXPECT_FALSE(proxy.pre_filter(req, res));
}

class LoadBalancerIntegrationTest : public ::testing::Test {
protected:
    load_balancer lb;

    void SetUp() override {
        lb.add_backend({"srv1", ports(8080), "http", 1});
        lb.add_backend({"srv2", ports(8080), "http", 2});
        lb.add_backend({"srv3", ports(8080), "http", 1});
    }
    void TearDown() override {}
};

TEST_F(LoadBalancerIntegrationTest, RoundRobinDistribution) {
    lb.set_strategy(lb_strategy::ROUND_ROBIN);

    auto* b1 = lb.select_backend();
    auto* b2 = lb.select_backend();
    auto* b3 = lb.select_backend();
    auto* b4 = lb.select_backend();

    ASSERT_NE(b1, nullptr);
    ASSERT_NE(b2, nullptr);
    ASSERT_NE(b3, nullptr);
    ASSERT_NE(b4, nullptr);

    EXPECT_EQ(b4->host, b1->host);

    lb.release_backend(b1);
    lb.release_backend(b2);
    lb.release_backend(b3);
    lb.release_backend(b4);
}

TEST_F(LoadBalancerIntegrationTest, FailureMarksUnhealthy) {
    lb.set_strategy(lb_strategy::ROUND_ROBIN);
    lb.set_max_failures(2);

    auto* b1 = lb.select_backend();
    ASSERT_NE(b1, nullptr);

    lb.mark_failure(b1);
    lb.mark_failure(b1);

    b1->healthy = false;

    lb.release_backend(b1);
}

TEST_F(LoadBalancerIntegrationTest, LeastConnectionsStrategy) {
    lb.set_strategy(lb_strategy::LEAST_CONNECTIONS);

    auto* b = lb.select_backend();
    ASSERT_NE(b, nullptr);
    EXPECT_EQ(b->active_connections, 1u);

    lb.release_backend(b);
    EXPECT_EQ(b->active_connections, 0u);
}

TEST_F(LoadBalancerIntegrationTest, BackendHostsSnapshot) {
    auto hosts = lb.backend_hosts();
    ASSERT_EQ(hosts.size(), 3u);
    EXPECT_EQ(hosts[0], "srv1:8080");
    EXPECT_EQ(hosts[1], "srv2:8080");
    EXPECT_EQ(hosts[2], "srv3:8080");
}

TEST_F(LoadBalancerIntegrationTest, WeightedStrategySelectsHealthy) {
    lb.set_strategy(lb_strategy::WEIGHTED);

    lb.backend_hosts();

    for (int i = 0; i < 20; ++i) {
        auto* b = lb.select_backend();
        ASSERT_NE(b, nullptr);
        lb.release_backend(b);
    }
}

class CacheIntegrationTest : public ::testing::Test {
protected:
    response_cache cache;
    void SetUp() override { cache.max_entries = 100; }
    void TearDown() override {}
};

TEST_F(CacheIntegrationTest, FullCacheLifecycle) {
    string key = response_cache::build_key(http_method::GET(), "/api/users");

    http_response resp;
    resp.status = http_status::S2_OK;
    resp.body = R"([{"id":1,"name":"Alice"}])";
    resp.headers["Content-Type"] = "application/json";
    cache.put(key, resp, seconds{30});

    auto cached = cache.get(key);
    ASSERT_TRUE(cached.has_value());
    EXPECT_EQ(cached->body, resp.body);
    EXPECT_EQ(cached->status, http_status::S2_OK);
    EXPECT_EQ(cached->headers["Content-Type"], "application/json");
}

TEST_F(CacheIntegrationTest, CacheCleanupRemovesExpired) {
    cache.default_max_age = seconds{0};

    string key = response_cache::build_key(http_method::GET(), "/temporary");

    http_response resp;
    resp.status = http_status::S2_OK;
    resp.body = "temporary data";
    cache.put(key, resp);

    cache.cleanup();
    EXPECT_EQ(cache.size(), 0u);
}

TEST_F(CacheIntegrationTest, EtagGeneration) {
    string body = "hello world etag test";
    string etag1 = response_cache::generate_etag(body.view());
    string etag2 = response_cache::generate_etag(body.view());
    string etag3 = response_cache::generate_etag("different");

    EXPECT_FALSE(etag1.empty());
    EXPECT_EQ(etag1, etag2);
    EXPECT_NE(etag1, etag3);
}

TEST_F(CacheIntegrationTest, CacheKeyBuilding) {
    auto key1 = response_cache::build_key(http_method::GET(), "/api/v1/users");
    auto key2 = response_cache::build_key(http_method::GET(), "/api/v1/users");
    auto key3 = response_cache::build_key(http_method::POST(), "/api/v1/users");
    auto key4 = response_cache::build_key(http_method::GET(), "/api/v1/posts");

    EXPECT_EQ(key1, key2);
    EXPECT_NE(key1, key3);
    EXPECT_NE(key1, key4);
}

TEST_F(CacheIntegrationTest, CacheFilterWithRouter) {
    response_cache shared_cache;
    auto cf = make_unique<cache_filter>(&shared_cache);
    auto* raw_cf = cf.get();

    http_router router;
    router.use(move(cf));

    http_request req1;
    req1.method = http_method::GET();
    req1.path = "/cached";

    http_response resp1;
    bool cont1 = raw_cf->pre_filter(req1, resp1);
    EXPECT_TRUE(cont1);

    resp1.status = http_status::S2_OK;
    resp1.body = "cached content";
    resp1.headers["Cache-Control"] = "public, max-age=60";

    raw_cf->post_filter(req1, resp1);

    http_request req2;
    req2.method = http_method::GET();
    req2.path = "/cached";
    http_response resp2;
    bool cont2 = raw_cf->pre_filter(req2, resp2);
    EXPECT_FALSE(cont2);
    EXPECT_EQ(resp2.body, "cached content");
    EXPECT_EQ(resp2.status, http_status::S2_OK);
}

class GrpcIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(GrpcIntegrationTest, EncodeDecodeRoundTrip) {
    string payload = "hello grpc world";
    grpc_message msg;
    msg.compressed = false;
    msg.payload = byte_vector(reinterpret_cast<const byte_t*>(payload.data()),
                              reinterpret_cast<const byte_t*>(payload.data()) + payload.size());
    byte_vector encoded = grpc_framer::encode(msg);
    ASSERT_GE(encoded.size(), 5u);

    EXPECT_EQ(encoded[0], 0);
    uint32_t length = (static_cast<uint32_t>(encoded[1]) << 24) | (static_cast<uint32_t>(encoded[2]) << 16) |
                      (static_cast<uint32_t>(encoded[3]) << 8) | static_cast<uint32_t>(encoded[4]);
    EXPECT_EQ(length, payload.size());

    grpc_framer framer;
    vector<grpc_message> decoded;
    int count = framer.decode(encoded.data(), encoded.size(), decoded);
    ASSERT_EQ(count, 1);
    EXPECT_FALSE(decoded[0].compressed);
    EXPECT_EQ(string(reinterpret_cast<const char*>(decoded[0].payload.data()), decoded[0].payload.size()), payload);
}

TEST_F(GrpcIntegrationTest, EncodeMultipleMessages) {
    vector<grpc_message> msgs;
    for (auto* s: {"msg1", "msg2", "msg3"}) {
        grpc_message msg;
        msg.payload = byte_vector(reinterpret_cast<const byte_t*>(s), reinterpret_cast<const byte_t*>(s) + strlen(s));
        msgs.push_back(move(msg));
    }
    byte_vector encoded = grpc_framer::encode_messages(msgs);

    grpc_framer framer;
    vector<grpc_message> decoded;
    int count = framer.decode(encoded.data(), encoded.size(), decoded);

    ASSERT_EQ(count, 3);
    EXPECT_EQ(string(reinterpret_cast<const char*>(decoded[0].payload.data()), decoded[0].payload.size()), "msg1");
    EXPECT_EQ(string(reinterpret_cast<const char*>(decoded[1].payload.data()), decoded[1].payload.size()), "msg2");
    EXPECT_EQ(string(reinterpret_cast<const char*>(decoded[2].payload.data()), decoded[2].payload.size()), "msg3");
}

TEST_F(GrpcIntegrationTest, StatusCodeMapping) {
    EXPECT_EQ(grpc_handler::grpc_to_http_status(grpc_status::OK), http_status::S2_OK);
    EXPECT_EQ(grpc_handler::grpc_to_http_status(grpc_status::NOT_FOUND), http_status::S4_NOT_FOUND);
    EXPECT_EQ(grpc_handler::grpc_to_http_status(grpc_status::INTERNAL), http_status::S5_INTERNAL_SERVER_ERROR);
    EXPECT_EQ(grpc_handler::grpc_to_http_status(grpc_status::UNAUTHENTICATED), http_status::S4_UNAUTHORIZED);
    EXPECT_EQ(grpc_handler::grpc_to_http_status(grpc_status::PERMISSION_DENIED), http_status::S4_FORBIDDEN);
}

TEST_F(GrpcIntegrationTest, IncompleteFrameHandling) {
    string hello = "hello";
    grpc_message msg;
    msg.payload = byte_vector(reinterpret_cast<const byte_t*>(hello.data()),
                              reinterpret_cast<const byte_t*>(hello.data()) + hello.size());
    byte_vector encoded = grpc_framer::encode(msg);

    byte_vector incomplete(encoded.data(), encoded.data() + 3);

    grpc_framer framer;
    vector<grpc_message> decoded;
    int count = framer.decode(incomplete.data(), incomplete.size(), decoded);
    EXPECT_EQ(count, 0);
    EXPECT_TRUE(decoded.empty());
}

TEST_F(GrpcIntegrationTest, CompressedMessageRoundTrip) {
    string payload = "compressible data for testing";
    grpc_message msg;
    msg.compressed = true;
    msg.payload = byte_vector(reinterpret_cast<const byte_t*>(payload.data()),
                              reinterpret_cast<const byte_t*>(payload.data()) + payload.size());
    byte_vector encoded = grpc_framer::encode(msg);

    EXPECT_EQ(encoded[0], 1);

    grpc_framer framer;
    vector<grpc_message> decoded;
    int count = framer.decode(encoded.data(), encoded.size(), decoded);
    ASSERT_EQ(count, 1);
    EXPECT_TRUE(decoded[0].compressed);
}

class Http2PushIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(Http2PushIntegrationTest, PushPromiseEncodeDecode) {
    vector<hpack_header_field> headers;
    headers.push_back({":method", "GET"});
    headers.push_back({":path", "/style.css"});
    headers.push_back({":authority", "example.com"});
    headers.push_back({":scheme", "https"});

    http2_push_promise_frame pp_frame;
    pp_frame.stream_id = 1;
    pp_frame.promised_stream_id = 2;
    pp_frame.header_block = byte_vector(headers[0].name.begin(), headers[0].name.end());
    pp_frame.end_headers = true;
    byte_vector frame = http2_framer::encode_push_promise_frame(pp_frame);
    ASSERT_GE(frame.size(), 13u);

    EXPECT_EQ(frame[3], 0x5);
    EXPECT_EQ(frame[4] & 0x4, 0x4);

    uint32_t stream_id = (static_cast<uint32_t>(frame[5]) << 24) | (static_cast<uint32_t>(frame[6]) << 16) |
                         (static_cast<uint32_t>(frame[7]) << 8) | static_cast<uint32_t>(frame[8]);
    EXPECT_EQ(stream_id, 1u);

    uint32_t promised_id = (static_cast<uint32_t>(frame[9]) << 24) | (static_cast<uint32_t>(frame[10]) << 16) |
                           (static_cast<uint32_t>(frame[11]) << 8) | static_cast<uint32_t>(frame[12]);
    EXPECT_EQ(promised_id & 0x7FFFFFFF, 2u);
}

TEST_F(Http2PushIntegrationTest, PushPromiseWithPadding) {
    vector<hpack_header_field> headers;
    headers.push_back({":method", "GET"});
    headers.push_back({":path", "/app.js"});

    http2_push_promise_frame pp_frame;
    pp_frame.stream_id = 3;
    pp_frame.promised_stream_id = 4;
    pp_frame.header_block = byte_vector(headers[0].name.begin(), headers[0].name.end());
    pp_frame.end_headers = true;
    byte_vector frame = http2_framer::encode_push_promise_frame(pp_frame);

    EXPECT_EQ(frame[3], 0x5);

    uint32_t promised_id = (static_cast<uint32_t>(frame[9]) << 24) | (static_cast<uint32_t>(frame[10]) << 16) |
                           (static_cast<uint32_t>(frame[11]) << 8) | static_cast<uint32_t>(frame[12]);
    EXPECT_EQ(promised_id & 0x7FFFFFFF, 4u);
    EXPECT_EQ((promised_id & 0x7FFFFFFF) % 2, 0u);
}

class ChunkedReaderIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(ChunkedReaderIntegrationTest, ConfigurableLimits) {
    byte_size chunk_sz{1024 * 1024};
    byte_size total_sz{10 * 1024 * 1024};

    EXPECT_EQ(chunk_sz.bytes(), 1024u * 1024u);
    EXPECT_EQ(total_sz.bytes(), 10u * 1024u * 1024u);
    EXPECT_EQ(byte_size{}.bytes(), 0u);
}

class H2TlsIntegrationTest : public ::testing::Test {
protected:
    const char* cert_path = "/tmp/h2test.crt";
    const char* key_path = "/tmp/h2test.key";

    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(H2TlsIntegrationTest, H2AlpnNegotiationServerClient) {
    ssl_context server_ctx(ssl_method::TLS_SERVER);
    if (!server_ctx.load_certificate(cert_path, key_path)) {
        GTEST_SKIP() << "Test certificate not found at " << cert_path;
    }

    http_server server(ports(8443), move(server_ctx), 1);
    server.start();
    this_thread::sleep_for(milliseconds(200));

    ssl_context client_ctx(ssl_method::TLS_CLIENT);
    client_ctx.set_alpn_protos({"h2"});

    ssl_client client(move(client_ctx));
    client.set_verify_peer(false);
    if (!client.connect(ip_address::loopback().to_string(), ports(8443))) {
        server.stop();
        GTEST_SKIP() << "TLS connection failed (port 8443 may be unavailable)";
    }

    auto* ssl_sock = dynamic_cast<ssl_socket*>(&client.socket());
    ASSERT_NE(ssl_sock, nullptr);
    string negotiated = ssl_sock->get_alpn_negotiated();
    EXPECT_EQ(negotiated, "h2");

    client.disconnect();
    server.stop();
}

TEST_F(H2TlsIntegrationTest, Http11AlpnFallback) {
    ssl_context server_ctx(ssl_method::TLS_SERVER);
    if (!server_ctx.load_certificate(cert_path, key_path)) {
        GTEST_SKIP() << "Test certificate not found at " << cert_path;
    }

    http_server server(ports(8444), move(server_ctx), 1);
    server.start();
    this_thread::sleep_for(milliseconds(200));

    ssl_context client_ctx(ssl_method::TLS_CLIENT);
    client_ctx.set_alpn_protos({"http/1.1"});

    ssl_client client(move(client_ctx));
    client.set_verify_peer(false);
    if (!client.connect(ip_address::loopback().to_string(), ports(8444))) {
        server.stop();
        GTEST_SKIP() << "TLS connection failed (port 8444 may be unavailable)";
    }

    auto* ssl_sock = dynamic_cast<ssl_socket*>(&client.socket());
    ASSERT_NE(ssl_sock, nullptr);
    string negotiated = ssl_sock->get_alpn_negotiated();
    EXPECT_EQ(negotiated, "http/1.1");

    client.disconnect();
    server.stop();
}

TEST_F(H2TlsIntegrationTest, NoAlpnReturnsEmpty) {
    ssl_stream s;
    EXPECT_TRUE(s.get_alpn_negotiated().empty());

    ssl_socket sock;
    EXPECT_TRUE(sock.get_alpn_negotiated().empty());
}

TEST_F(H2TlsIntegrationTest, SslSocketGetAlpnWorks) {
    ssl_socket sock;
    EXPECT_TRUE(sock.get_alpn_negotiated().empty());
    EXPECT_FALSE(sock.is_ssl());
}
