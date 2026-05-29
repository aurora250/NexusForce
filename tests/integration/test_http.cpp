#include <NeForce/network/http/http_constants.hpp>
#include <NeForce/network/http/http_session.hpp>
#include <NeForce/network/http/http_server_message.hpp>
#include <NeForce/network/http/http_client_message.hpp>
#include <NeForce/network/http/http_filter.hpp>
#include <NeForce/network/http/http_router.hpp>
#include <NeForce/network/http/http_server.hpp>
#include <NeForce/network/http/websocket.hpp>
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
    unordered_map<string, string> db; // Simple in-memory store

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
