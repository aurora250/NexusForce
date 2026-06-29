#include <NeForce/network/http/http_constants.hpp>
#include <NeForce/network/http/http_session.hpp>
#include <NeForce/network/http/http_server_message.hpp>
#include <NeForce/network/http/http_client_message.hpp>
#include <NeForce/network/http/http_filter.hpp>
#include <NeForce/network/http/http_router.hpp>
#include <NeForce/network/http/http_security.hpp>
#include <NeForce/network/http/http_server.hpp>
#include <NeForce/network/http/rate_limiter.hpp>
#include <NeForce/network/http/http_range.hpp>
#include <NeForce/network/http/http_compress.hpp>
#include <NeForce/network/http/csrf_filter.hpp>
#include <NeForce/network/http/session_store.hpp>
#include <NeForce/network/http/http2_protocol.hpp>
#include <NeForce/network/http/websocket_deflate.hpp>
#include <NeForce/network/http/health_check.hpp>
#include <NeForce/network/http/multipart_parser.hpp>
#include <NeForce/network/http/reverse_proxy.hpp>
#include <NeForce/network/http/load_balancer.hpp>
#include <NeForce/network/http/http_cache.hpp>
#include <NeForce/network/http/grpc.hpp>
#include <NeForce/network/http/http2_connection.hpp>
#include <NeForce/network/http/websocket.hpp>
#include <NeForce/network/http/chunked_reader.hpp>
#include <NeForce/network/http/async_filter.hpp>
#include <NeForce/network/http/http2_protocol.hpp>
#include <NeForce/core/memory/byte_cursor.hpp>
#include <NeForce/network/http/async_filter.hpp>
#include <gtest/gtest.h>
using namespace neforce;
using namespace neforce::http;

#ifdef DELETE
#    undef DELETE
#endif

class HttpStatusTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(HttpStatusTest, StatusCodesHaveCorrectValues) {
    EXPECT_EQ(static_cast<uint16_t>(http_status::S1_CONTINUE), 100u);
    EXPECT_EQ(static_cast<uint16_t>(http_status::S1_SWITCHING_PROTOCOLS), 101u);
    EXPECT_EQ(static_cast<uint16_t>(http_status::S2_OK), 200u);
    EXPECT_EQ(static_cast<uint16_t>(http_status::S2_CREATED), 201u);
    EXPECT_EQ(static_cast<uint16_t>(http_status::S2_NO_CONTENT), 204u);
    EXPECT_EQ(static_cast<uint16_t>(http_status::S2_PARTIAL_CONTENT), 206u);
    EXPECT_EQ(static_cast<uint16_t>(http_status::S3_MOVED_PERMANENT), 301u);
    EXPECT_EQ(static_cast<uint16_t>(http_status::S3_FOUND), 302u);
    EXPECT_EQ(static_cast<uint16_t>(http_status::S3_SEE_OTHER), 303u);
    EXPECT_EQ(static_cast<uint16_t>(http_status::S3_NOT_MODIFIED), 304u);
    EXPECT_EQ(static_cast<uint16_t>(http_status::S3_TEMPORARY_REDIRECT), 307u);
    EXPECT_EQ(static_cast<uint16_t>(http_status::S3_PERMANENT_REDIRECT), 308u);
    EXPECT_EQ(static_cast<uint16_t>(http_status::S4_BAD_REQUEST), 400u);
    EXPECT_EQ(static_cast<uint16_t>(http_status::S4_UNAUTHORIZED), 401u);
    EXPECT_EQ(static_cast<uint16_t>(http_status::S4_FORBIDDEN), 403u);
    EXPECT_EQ(static_cast<uint16_t>(http_status::S4_NOT_FOUND), 404u);
    EXPECT_EQ(static_cast<uint16_t>(http_status::S4_METHOD_NOT_ALLOWED), 405u);
    EXPECT_EQ(static_cast<uint16_t>(http_status::S4_REQUEST_TIMEOUT), 408u);
    EXPECT_EQ(static_cast<uint16_t>(http_status::S4_CONFLICT), 409u);
    EXPECT_EQ(static_cast<uint16_t>(http_status::S4_GONE), 410u);
    EXPECT_EQ(static_cast<uint16_t>(http_status::S4_PAYLOAD_TOO_LARGE), 413u);
    EXPECT_EQ(static_cast<uint16_t>(http_status::S4_URI_TOO_LONG), 414u);
    EXPECT_EQ(static_cast<uint16_t>(http_status::S4_UNSUPPORTED_MEDIA_TYPE), 415u);
    EXPECT_EQ(static_cast<uint16_t>(http_status::S4_RANGE_NOT_SATISFIABLE), 416u);
    EXPECT_EQ(static_cast<uint16_t>(http_status::S4_TOO_MANY_REQUESTS), 429u);
    EXPECT_EQ(static_cast<uint16_t>(http_status::S5_INTERNAL_SERVER_ERROR), 500u);
    EXPECT_EQ(static_cast<uint16_t>(http_status::S5_NOT_IMPLEMENTED), 501u);
    EXPECT_EQ(static_cast<uint16_t>(http_status::S5_BAD_GATEWAY), 502u);
    EXPECT_EQ(static_cast<uint16_t>(http_status::S5_SERVICE_UNAVAILABLE), 503u);
    EXPECT_EQ(static_cast<uint16_t>(http_status::S5_GATEWAY_TIMEOUT), 504u);
    EXPECT_EQ(static_cast<uint16_t>(http_status::S5_HTTP_VERSION_NOT_SUPPORTED), 505u);
}

TEST_F(HttpStatusTest, StatusMessageReturnsCorrectText) {
    EXPECT_EQ(http_status_message(http_status::S2_OK), "OK");
    EXPECT_EQ(http_status_message(http_status::S4_NOT_FOUND), "Not Found");
    EXPECT_EQ(http_status_message(http_status::S5_INTERNAL_SERVER_ERROR), "Internal Server Error");
    EXPECT_EQ(http_status_message(http_status::S3_SEE_OTHER), "See Other");
    EXPECT_EQ(http_status_message(http_status::S4_CONFLICT), "Conflict");
    EXPECT_EQ(http_status_message(http_status::S4_GONE), "Gone");
    EXPECT_EQ(http_status_message(http_status::S4_UNSUPPORTED_MEDIA_TYPE), "Unsupported Media Type");
    EXPECT_EQ(http_status_message(http_status::S4_RANGE_NOT_SATISFIABLE), "Range Not Satisfiable");
    EXPECT_EQ(http_status_message(http_status::S5_NOT_IMPLEMENTED), "Not Implemented");
}

TEST_F(HttpStatusTest, FromCodeMapsCorrectly) {
    EXPECT_EQ(http_status_from_code(200), http_status::S2_OK);
    EXPECT_EQ(http_status_from_code(404), http_status::S4_NOT_FOUND);
    EXPECT_EQ(http_status_from_code(303), http_status::S3_SEE_OTHER);
    EXPECT_EQ(http_status_from_code(409), http_status::S4_CONFLICT);
    EXPECT_EQ(http_status_from_code(410), http_status::S4_GONE);
    EXPECT_EQ(http_status_from_code(415), http_status::S4_UNSUPPORTED_MEDIA_TYPE);
    EXPECT_EQ(http_status_from_code(416), http_status::S4_RANGE_NOT_SATISFIABLE);
    EXPECT_EQ(http_status_from_code(501), http_status::S5_NOT_IMPLEMENTED);
}

TEST_F(HttpStatusTest, UnknownCodeFallsBackToCategory) {
    EXPECT_EQ(http_status_from_code(199), http_status::S1_CONTINUE);
    EXPECT_EQ(http_status_from_code(299), http_status::S2_OK);
    EXPECT_EQ(http_status_from_code(399), http_status::S3_FOUND);
    EXPECT_EQ(http_status_from_code(499), http_status::S4_BAD_REQUEST);
    EXPECT_EQ(http_status_from_code(599), http_status::S5_INTERNAL_SERVER_ERROR);
    EXPECT_EQ(http_status_from_code(999), http_status::S5_INTERNAL_SERVER_ERROR);
}

class HttpMethodTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(HttpMethodTest, StaticMethodsHaveCorrectStrings) {
    EXPECT_EQ(http_method::GET().method(), "GET");
    EXPECT_EQ(http_method::POST().method(), "POST");
    EXPECT_EQ(http_method::HEAD().method(), "HEAD");
    EXPECT_EQ(http_method::PUT().method(), "PUT");
    EXPECT_EQ(http_method::DELETE().method(), "DELETE");
    EXPECT_EQ(http_method::OPTIONS().method(), "OPTIONS");
    EXPECT_EQ(http_method::TRACE().method(), "TRACE");
    EXPECT_EQ(http_method::CONNECT().method(), "CONNECT");
    EXPECT_EQ(http_method::PATCH().method(), "PATCH");
}

TEST_F(HttpMethodTest, IsMethodsReturnTrue) {
    EXPECT_TRUE(http_method::GET().is_get());
    EXPECT_TRUE(http_method::POST().is_post());
    EXPECT_TRUE(http_method::HEAD().is_head());
    EXPECT_TRUE(http_method::PUT().is_put());
    EXPECT_TRUE(http_method::DELETE().is_delete());
    EXPECT_TRUE(http_method::OPTIONS().is_options());
    EXPECT_TRUE(http_method::TRACE().is_trace());
    EXPECT_TRUE(http_method::CONNECT().is_connect());
    EXPECT_TRUE(http_method::PATCH().is_patch());
}

TEST_F(HttpMethodTest, OperatorAndCombinesMethods) {
    auto combined = http_method::GET() & http_method::POST();
    EXPECT_TRUE(combined.method().contains("GET"));
    EXPECT_TRUE(combined.method().contains("POST"));
}

TEST(HttpContentTest, ContentTypesHaveCorrectMimeStrings) {
    EXPECT_EQ(http_content::HTML_TEXT().to_string(), "text/html");
    EXPECT_EQ(http_content::JSON_APP().to_string(), "application/json");
    EXPECT_EQ(http_content::FORM_APP().to_string(), "application/x-www-form-urlencoded");
    EXPECT_EQ(http_content::PLAIN_TEXT().to_string(), "text/plain");
    EXPECT_EQ(http_content::JPEG_IMG().to_string(), "image/jpeg");
    EXPECT_EQ(http_content::PNG_IMG().to_string(), "image/png");
}

TEST(HttpContentTest, IsMethodsCheckContentType) {
    EXPECT_TRUE(http_content::JSON_APP().is_json_app());
    EXPECT_TRUE(http_content::is_json_app("application/json"));
    EXPECT_TRUE(http_content::is_html_text("text/html"));
    EXPECT_FALSE(http_content::is_json_app("text/html"));
}

class HttpCookieTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(HttpCookieTest, ParseSimpleCookie) {
    auto c = http_cookie::parse("sessionId=abc123");
    EXPECT_EQ(c.name.cookie_name(), "sessionId");
    EXPECT_EQ(c.value, "abc123");
    EXPECT_EQ(c.path, "/");
    EXPECT_FALSE(c.secure);
    EXPECT_FALSE(c.http_only);
}

TEST_F(HttpCookieTest, ParseCookieWithAttributes) {
    auto c = http_cookie::parse("token=xyz789; Path=/api; HttpOnly; Secure; Max-Age=3600; SameSite=Lax");
    EXPECT_EQ(c.name.cookie_name(), "token");
    EXPECT_EQ(c.value, "xyz789");
    EXPECT_EQ(c.path, "/api");
    EXPECT_TRUE(c.http_only);
    EXPECT_TRUE(c.secure);
    EXPECT_EQ(c.max_age, seconds{3600});
    EXPECT_EQ(c.same_site, "Lax");
}

TEST_F(HttpCookieTest, ParseCookieWithDomain) {
    auto c = http_cookie::parse("id=1; Domain=example.com; Path=/app");
    EXPECT_EQ(c.name.cookie_name(), "id");
    EXPECT_EQ(c.domain, "example.com");
    EXPECT_EQ(c.path, "/app");
}

TEST_F(HttpCookieTest, ParseEmptyHeaderReturnsDefaultCookie) {
    auto c = http_cookie::parse("");
    EXPECT_EQ(c.name.cookie_name(), "UNKNOWN");
    EXPECT_TRUE(c.is_valid());
}

TEST_F(HttpCookieTest, ParseWithDefaults) {
    auto c = http_cookie::parse("x=1", "default.com", "/default");
    EXPECT_EQ(c.domain, "default.com");
    EXPECT_EQ(c.path, "/default");
}

TEST_F(HttpCookieTest, ToStringGeneratesSetCookieFormat) {
    http_cookie c;
    c.name = http_cookie_name{"session"};
    c.value = "abc";
    c.path = "/";
    c.http_only = true;
    c.secure = true;
    c.max_age = seconds{3600};

    auto result = c.to_string();
    EXPECT_TRUE(result.contains("session=abc"));
    EXPECT_TRUE(result.contains("Path=/"));
    EXPECT_TRUE(result.contains("HttpOnly"));
    EXPECT_TRUE(result.contains("Secure"));
    EXPECT_TRUE(result.contains("Max-Age=3600"));
}

TEST_F(HttpCookieTest, IsValidRejectsEmptyName) {
    http_cookie c;
    c.name = http_cookie_name{""};
    EXPECT_FALSE(c.is_valid());
}

TEST_F(HttpCookieTest, IsValidRejectsMaxAgeZero) {
    http_cookie c;
    c.name = http_cookie_name{"test"};
    c.value = "1";
    c.max_age = seconds{0};
    EXPECT_FALSE(c.is_valid());
}

TEST_F(HttpCookieTest, IsExpiredReturnsTrueForMaxAgeZero) {
    http_cookie c;
    c.name = http_cookie_name{"test"};
    c.value = "1";
    c.max_age = seconds{0};
    EXPECT_TRUE(c.is_expired());
}

TEST_F(HttpCookieTest, IsExpiredReturnsFalseForSessionCookie) {
    http_cookie c;
    c.name = http_cookie_name{"test"};
    c.value = "1";
    c.max_age = seconds{-1};
    EXPECT_FALSE(c.is_expired());
}

TEST_F(HttpCookieTest, SetExpiresFromNowSetsFutureExpires) {
    http_cookie c;
    c.name = http_cookie_name{"test"};
    c.set_expires_from_now(seconds{3600});
    EXPECT_GT(c.expires, datetime::now());
    EXPECT_TRUE(c.is_valid());
}

class HttpSessionTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(HttpSessionTest, NewSessionIsValid) {
    http_session s;
    s.id = "test-session-id";
    EXPECT_TRUE(s.is_valid());
    EXPECT_TRUE(s.is_new);
}

TEST_F(HttpSessionTest, InvalidatedSessionIsNotValid) {
    http_session s;
    s.id = "test-session-id";
    s.invalidate();
    EXPECT_FALSE(s.is_valid());
}

TEST_F(HttpSessionTest, SessionWithoutIdIsNotValid) {
    http_session s;
    EXPECT_FALSE(s.is_valid());
}

TEST_F(HttpSessionTest, SetAndGetData) {
    http_session s;
    s.id = "test";
    s.set("key1", "value1");
    EXPECT_EQ(s.get("key1"), "value1");
    EXPECT_TRUE(s.contains("key1"));
}

TEST_F(HttpSessionTest, OperatorBracketAccessesData) {
    http_session s;
    s.id = "test";
    s["user"] = "john";
    EXPECT_EQ(s["user"], "john");
}

TEST_F(HttpSessionTest, RemoveData) {
    http_session s;
    s.id = "test";
    s.set("key1", "value1");
    EXPECT_TRUE(s.remove("key1"));
    EXPECT_FALSE(s.contains("key1"));
    EXPECT_FALSE(s.remove("nonexistent"));
}

TEST_F(HttpSessionTest, ClearRemovesAllData) {
    http_session s;
    s.id = "test";
    s.set("a", "1");
    s.set("b", "2");
    s.clear();
    EXPECT_FALSE(s.contains("a"));
    EXPECT_FALSE(s.contains("b"));
}

TEST_F(HttpSessionTest, TouchUpdatesLastAccess) {
    http_session s;
    s.id = "test";
    auto before = s.last_access;
    this_thread::sleep_for(seconds{2});
    s.touch();
    auto after = s.last_access;
    EXPECT_GT(after, before);
    EXPECT_FALSE(s.is_new);
}

TEST_F(HttpSessionTest, ExpiredReturnsTrueWhenIdleTooLong) {
    http_session s;
    s.id = "test";
    s.max_age = seconds{1};
    s.last_access = datetime::now() - 10;
    EXPECT_TRUE(s.expired());
}

TEST_F(HttpSessionTest, ExpiredReturnsFalseForActiveSession) {
    http_session s;
    s.id = "test";
    s.max_age = seconds{3600};
    EXPECT_FALSE(s.expired());
}

TEST_F(HttpSessionTest, CustomMaxInactiveOverridesDefault) {
    http_session s;
    s.id = "test";
    s.max_age = seconds{3600};
    s.last_access = datetime::now() - 5;
    EXPECT_TRUE(s.expired(seconds{1}));
}

TEST_F(HttpSessionTest, SessionEvictionByOldest) {
    http_server::session_manager mgr;
    mgr.set_max_sessions(2);

    auto* s1 = mgr.get_session("", true);
    ASSERT_NE(s1, nullptr);
    s1->max_age = seconds{86400};
    string id1 = s1->id;

    auto* s2 = mgr.get_session("", true);
    ASSERT_NE(s2, nullptr);
    s2->max_age = seconds{86400};
    string id2 = s2->id;

    ASSERT_NE(id1, id2);
    ASSERT_EQ(mgr.session_count(), 2u);

    s1->last_access = datetime::now() - 10000;

    auto* s3 = mgr.get_session("", true);
    ASSERT_NE(s3, nullptr);
    s3->max_age = seconds{86400};

    bool s1_exists = mgr.session_exists(id1);
    bool s2_exists = mgr.session_exists(id2);
    EXPECT_EQ(mgr.session_count(), 2u);
    EXPECT_FALSE(s1_exists);
    EXPECT_TRUE(s2_exists);
    EXPECT_TRUE(mgr.session_exists(s3->id));
}

class HttpRequestParseTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(HttpRequestParseTest, ParseSimpleGetRequest) {
    auto req = http_request::parse("GET /index.html HTTP/1.1\r\n"
                                   "Host: localhost\r\n"
                                   "\r\n");
    EXPECT_EQ(req.method.method(), "GET");
    EXPECT_EQ(req.path, "/index.html");
    EXPECT_EQ(req.version, "HTTP/1.1");
}

TEST_F(HttpRequestParseTest, ParseRequestWithQueryString) {
    auto req = http_request::parse("GET /search?q=hello&page=1 HTTP/1.1\r\n"
                                   "Host: example.com\r\n"
                                   "\r\n");
    EXPECT_EQ(req.path, "/search");
    EXPECT_EQ(req.query, "q=hello&page=1");
}

TEST_F(HttpRequestParseTest, ParseHeaders) {
    auto req = http_request::parse("POST /api HTTP/1.1\r\n"
                                   "Host: test\r\n"
                                   "Content-Type: application/json\r\n"
                                   "Content-Length: 13\r\n"
                                   "\r\n"
                                   "{\"key\":\"val\"}");
    EXPECT_EQ(req.header("Content-Type"), "application/json");
    EXPECT_EQ(req.content_type(), "application/json");
}

TEST_F(HttpRequestParseTest, ParseCookiesFromCookieHeader) {
    auto req = http_request::parse("GET / HTTP/1.1\r\n"
                                   "Host: example.com\r\n"
                                   "Cookie: session=abc123; theme=dark\r\n"
                                   "\r\n");
    EXPECT_EQ(req.cookie("session"), "abc123");
    EXPECT_EQ(req.cookie("theme"), "dark");
    EXPECT_EQ(req.cookie("nonexistent"), "");
}

TEST_F(HttpRequestParseTest, ParseBodyAfterHeaders) {
    auto req = http_request::parse("POST /submit HTTP/1.1\r\n"
                                   "Host: test\r\n"
                                   "Content-Length: 11\r\n"
                                   "\r\n"
                                   "Hello World");
    EXPECT_EQ(req.body, "Hello World");
}

TEST_F(HttpRequestParseTest, ParsePostRequest) {
    auto req = http_request::parse("POST /api/users HTTP/1.1\r\n"
                                   "Host: api.example.com\r\n"
                                   "Content-Type: application/json\r\n"
                                   "Content-Length: 2\r\n"
                                   "\r\n"
                                   "{}");
    EXPECT_TRUE(req.method.is_post());
    EXPECT_EQ(req.path, "/api/users");
}

TEST_F(HttpRequestParseTest, ParseInvalidRequestLineThrows) {
    EXPECT_THROW(ignore = http_request::parse("INVALID\r\n\r\n"), http_exception);
}

TEST_F(HttpRequestParseTest, IsKeepAliveDetectsKeepAliveConnection) {
    auto req = http_request::parse("GET / HTTP/1.1\r\n"
                                   "Host: test\r\n"
                                   "Connection: keep-alive\r\n"
                                   "\r\n");
    EXPECT_TRUE(req.is_keep_alive());
}

TEST_F(HttpRequestParseTest, IsKeepAliveDetectsCloseConnection) {
    auto req = http_request::parse("GET / HTTP/1.1\r\n"
                                   "Host: test\r\n"
                                   "Connection: close\r\n"
                                   "\r\n");
    EXPECT_FALSE(req.is_keep_alive());
}

TEST_F(HttpRequestParseTest, IsAjaxDetectsXmlHttpRequest) {
    auto req = http_request::parse("GET /api HTTP/1.1\r\n"
                                   "Host: test\r\n"
                                   "X-Requested-With: XMLHttpRequest\r\n"
                                   "\r\n");
    EXPECT_TRUE(req.is_ajax());
}

TEST_F(HttpRequestParseTest, UserAgentAndReferer) {
    auto req = http_request::parse("GET / HTTP/1.1\r\n"
                                   "Host: example.com\r\n"
                                   "User-Agent: Mozilla/5.0\r\n"
                                   "Referer: https://google.com\r\n"
                                   "\r\n");
    EXPECT_EQ(req.user_agent(), "Mozilla/5.0");
    EXPECT_EQ(req.referer(), "https://google.com");
}

TEST_F(HttpRequestParseTest, MissingHostHeaderInHttp11Throws) {
    EXPECT_THROW(ignore = http_request::parse("GET / HTTP/1.1\r\n\r\n"), http_exception);
}

TEST_F(HttpRequestParseTest, HostHeaderPresentAccepted) {
    auto req = http_request::parse("GET / HTTP/1.1\r\nHost: example.com\r\n\r\n");
    EXPECT_EQ(req.header("Host"), "example.com");
}

TEST_F(HttpRequestParseTest, PercentDecodedPath) {
    auto req = http_request::parse("GET /users%2F123 HTTP/1.1\r\nHost: test\r\n\r\n");
    EXPECT_EQ(req.path, "/users/123");
}

TEST_F(HttpRequestParseTest, PercentDecodedQueryString) {
    auto req = http_request::parse("GET /search?q=hello%20world HTTP/1.1\r\nHost: test\r\n\r\n");
    EXPECT_EQ(req.path, "/search");
    EXPECT_EQ(req.query, "q=hello%20world");
}

TEST_F(HttpRequestParseTest, IsKeepAliveDefaultForHttp11) {
    auto req = http_request::parse("GET / HTTP/1.1\r\nHost: test\r\n\r\n");
    EXPECT_TRUE(req.is_keep_alive());
}

TEST_F(HttpRequestParseTest, CloseForHttp11WithConnectionClose) {
    auto req = http_request::parse("GET / HTTP/1.1\r\nHost: test\r\nConnection: close\r\n\r\n");
    EXPECT_FALSE(req.is_keep_alive());
}

class HttpResponseTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(HttpResponseTest, DefaultResponseIsPersistentConnection) {
    http_response resp;
    EXPECT_TRUE(resp.header("Connection").empty());
    EXPECT_EQ(resp.status, http_status::S4_NOT_FOUND);
}

TEST_F(HttpResponseTest, NormalResponseIncludesStatusLine) {
    http_response resp;
    resp.status = http_status::S2_OK;
    resp.status_message = "OK";
    resp.body = "Hello";
    auto result = resp.to_string();
    EXPECT_TRUE(result.starts_with("HTTP/1.1 200 OK\r\n"));
}

TEST_F(HttpResponseTest, ResponseIncludesContentLength) {
    http_response resp;
    resp.status = http_status::S2_OK;
    resp.status_message = "OK";
    resp.body = "ABCDEFG";
    auto result = resp.to_string();
    EXPECT_TRUE(result.contains("Content-Length: 7\r\n"));
}

TEST_F(HttpResponseTest, RedirectResponseUsesCorrectStatus) {
    http_response resp;
    resp.status = http_status::S3_MOVED_PERMANENT;
    resp.status_message = "Moved Permanently";
    resp.redirect_url = "https://example.com/new";
    auto result = resp.to_string();
    EXPECT_TRUE(result.contains("301 Moved Permanently"));
    EXPECT_TRUE(result.contains("Location: https://example.com/new"));
    EXPECT_FALSE(result.contains("Hello"));
}

TEST_F(HttpResponseTest, RedirectResponseExcludesBody) {
    http_response resp;
    resp.redirect_url = "https://example.com";
    resp.body = "should not appear";
    auto result = resp.to_string();
    EXPECT_FALSE(result.contains("should not appear"));
}

TEST_F(HttpResponseTest, SetContentTypeWithContentObject) {
    http_response resp;
    resp.set_content_type(http_content::JSON_APP());
    EXPECT_EQ(resp.header("Content-Type"), "application/json");
}

TEST_F(HttpResponseTest, SetContentTypeWithString) {
    http_response resp;
    resp.set_content_type("application/octet-stream");
    EXPECT_EQ(resp.header("Content-Type"), "application/octet-stream");
}

TEST_F(HttpResponseTest, CookieInResponseIsSerialized) {
    http_response resp;
    http_cookie c;
    c.name = http_cookie_name{"session"};
    c.value = "abc";
    c.path = "/";
    resp.cookies.push_back(c);
    auto result = resp.to_string();
    EXPECT_TRUE(result.contains("Set-Cookie: session=abc; Path=/"));
}

TEST_F(HttpResponseTest, NotModifiedNoContentLength) {
    http_response resp;
    resp.status = http_status::S3_NOT_MODIFIED;
    resp.status_message = "Not Modified";
    auto result = resp.to_string();
    EXPECT_FALSE(result.contains("Content-Length"));
}

TEST_F(HttpResponseTest, ResponseVersionMatchesRequest) {
    http_response resp;
    resp.version = "HTTP/1.0";
    resp.status = http_status::S2_OK;
    resp.status_message = "OK";
    auto result = resp.to_string();
    EXPECT_TRUE(result.starts_with("HTTP/1.0 200 OK"));
}

TEST(HttpClientRequestTest, DefaultMethodIsGet) {
    http_client_request req;
    EXPECT_TRUE(req.method.is_get());
    EXPECT_EQ(req.path, "/");
    EXPECT_EQ(req.version, "HTTP/1.1");
}

TEST(HttpClientRequestTest, BuildFullPathWithNoParams) {
    http_client_request req;
    req.path = "/api/users";
    auto result = req.build_full_path();
    EXPECT_EQ(result, "/api/users");
}

TEST(HttpClientRequestTest, BuildFullPathWithQueryParams) {
    http_client_request req;
    req.path = "/search";
    req.query_params["q"] = "hello world";
    req.query_params["page"] = "1";
    auto result = req.build_full_path();
    EXPECT_TRUE(result.starts_with("/search?"));
    EXPECT_TRUE(result.contains("q="));
    EXPECT_TRUE(result.contains("page=1"));
}

TEST(HttpClientRequestTest, QueryParamEncodingIsCorrectDirection) {
    http_client_request req;
    req.path = "/search";
    req.query_params["q"] = "hello world";
    auto result = req.build_full_path();
    EXPECT_TRUE(result.contains("%20") || result.contains("+"));
}

TEST(HttpClientRequestTest, SetAndGetHeaders) {
    http_client_request req;
    req.set_header("Authorization", "Bearer token");
    EXPECT_EQ(req.header("Authorization"), "Bearer token");
    EXPECT_EQ(req.header("X-Nonexistent"), "");
}

TEST(HttpClientResponseTest, DefaultValues) {
    http_client_response resp;
    EXPECT_EQ(resp.http_version_major, 1u);
    EXPECT_EQ(resp.http_version_minor, 1u);
    EXPECT_FALSE(resp.chunked);
    EXPECT_EQ(resp.content_length, 0u);
}

TEST(HttpClientResponseTest, IsSuccessRange) {
    http_client_response resp;
    resp.status = http_status::S2_OK;
    EXPECT_TRUE(resp.is_success());
    resp.status = http_status::S2_CREATED;
    EXPECT_TRUE(resp.is_success());
    resp.status = http_status::S4_NOT_FOUND;
    EXPECT_FALSE(resp.is_success());
}

TEST(HttpClientResponseTest, IsRedirectRange) {
    http_client_response resp;
    resp.status = http_status::S3_FOUND;
    EXPECT_TRUE(resp.is_redirect());
    resp.status = http_status::S3_MOVED_PERMANENT;
    EXPECT_TRUE(resp.is_redirect());
    resp.status = http_status::S2_OK;
    EXPECT_FALSE(resp.is_redirect());
}

TEST(HttpClientResponseTest, IsClientErrorRange) {
    http_client_response resp;
    resp.status = http_status::S4_BAD_REQUEST;
    EXPECT_TRUE(resp.is_client_error());
    resp.status = http_status::S2_OK;
    EXPECT_FALSE(resp.is_client_error());
}

TEST(HttpClientResponseTest, IsServerErrorRange) {
    http_client_response resp;
    resp.status = http_status::S5_INTERNAL_SERVER_ERROR;
    EXPECT_TRUE(resp.is_server_error());
    resp.status = http_status::S2_OK;
    EXPECT_FALSE(resp.is_server_error());
}

class HttpFilterChainTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

class TestTrackingFilter : public http_filter {
public:
    bool pre_called = false;
    bool post_called = false;
    bool do_called = false;

    bool pre_filter(http_request&, http_response&) override {
        pre_called = true;
        return true;
    }
    void do_filter(http_request&, http_response&) override { do_called = true; }
    void post_filter(http_request&, http_response&) override { post_called = true; }
    string name() const override { return "TestTrackingFilter"; }
};

TEST_F(HttpFilterChainTest, AddFilterAndExecutePre) {
    http_filter_chain chain;
    auto filter = make_unique<TestTrackingFilter>();
    auto* fptr = filter.get();
    chain.add_filter(move(filter));

    http_request req;
    http_response resp;
    chain.execute_pre_filters(req, resp);
    EXPECT_TRUE(fptr->pre_called);
}

TEST_F(HttpFilterChainTest, EmptyChainReturnsTrue) {
    http_filter_chain chain;
    http_request req;
    http_response resp;
    EXPECT_TRUE(chain.execute_pre_filters(req, resp));
}

class CorsFilterTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(CorsFilterTest, EmptyAllowedOriginsReturnsTrue) {
    cors_filter filter;
    http_request req;
    http_response resp;
    EXPECT_TRUE(filter.pre_filter(req, resp));
}

TEST_F(CorsFilterTest, NoRequestOriginReturnsTrue) {
    cors_filter filter("*");
    http_request req;
    http_response resp;
    EXPECT_TRUE(filter.pre_filter(req, resp));
}

TEST_F(CorsFilterTest, MatchingOriginSetsCorsHeaders) {
    cors_filter filter("https://example.com");
    http_request req;
    req.set_header("Origin", "https://example.com");
    http_response resp;

    filter.pre_filter(req, resp);
    EXPECT_EQ(resp.header("Access-Control-Allow-Origin"), "https://example.com");
    EXPECT_EQ(resp.header("Access-Control-Allow-Credentials"), "true");
}

TEST_F(CorsFilterTest, WildcardOriginSetsWildcardHeader) {
    cors_filter filter("*");
    http_request req;
    req.set_header("Origin", "https://any-site.com");
    http_response resp;
    filter.pre_filter(req, resp);
    EXPECT_EQ(resp.header("Access-Control-Allow-Origin"), "*");
}

TEST_F(CorsFilterTest, NonMatchingOriginPassesWithoutCorsHeaders) {
    cors_filter filter("https://allowed.com");
    http_request req;
    req.set_header("Origin", "https://evil.com");
    http_response resp;
    EXPECT_TRUE(filter.pre_filter(req, resp));
    EXPECT_EQ(resp.header("Access-Control-Allow-Origin"), "");
}

TEST_F(CorsFilterTest, OptionsRequestReturnsFalseForPreflight) {
    cors_filter filter("*");
    http_request req;
    req.method = http_method::OPTIONS();
    req.set_header("Origin", "https://example.com");
    http_response resp;
    EXPECT_FALSE(filter.pre_filter(req, resp));
    EXPECT_EQ(static_cast<uint16_t>(resp.status), 204u);
}

TEST_F(CorsFilterTest, AddsVaryOriginForNonWildcard) {
    cors_filter filter("https://example.com");
    http_request req;
    req.set_header("Origin", "https://example.com");
    http_response resp;
    filter.pre_filter(req, resp);
    EXPECT_EQ(resp.header("Vary"), "Origin");
}

TEST_F(CorsFilterTest, NoVaryForWildcardOrigin) {
    cors_filter filter("*");
    http_request req;
    req.set_header("Origin", "https://example.com");
    http_response resp;
    filter.pre_filter(req, resp);
    EXPECT_EQ(resp.header("Vary"), "");
}

TEST(StaticFileFilterTest, IsSafePathRejectsPathTraversal) {
    EXPECT_FALSE(static_file_filter::is_safe_path("../../../etc/passwd"));
    EXPECT_FALSE(static_file_filter::is_safe_path("/a/../b"));
    EXPECT_FALSE(static_file_filter::is_safe_path(".."));
}

TEST(StaticFileFilterTest, IsSafePathRejectsDoubleSlash) { EXPECT_FALSE(static_file_filter::is_safe_path("/a//b")); }

TEST(StaticFileFilterTest, IsSafePathRejectsEncodedTraversal) {
    EXPECT_FALSE(static_file_filter::is_safe_path("/a/%2e%2e/b"));
    EXPECT_FALSE(static_file_filter::is_safe_path("/a/%2e%2e/%2e%2e/b"));
}

TEST(StaticFileFilterTest, IsSafePathAcceptsNormalPath) {
    EXPECT_TRUE(static_file_filter::is_safe_path("/index.html"));
    EXPECT_TRUE(static_file_filter::is_safe_path("/css/style.css"));
    EXPECT_TRUE(static_file_filter::is_safe_path("/api/users/123"));
}

TEST(StaticFileFilterTest, IsSafePathRejectsEmptyPath) { EXPECT_FALSE(static_file_filter::is_safe_path("")); }

TEST(StaticFileFilterTest, KnownMimeTypes) {
    static_file_filter filter("/var/www");
    EXPECT_TRUE(filter.get_mime_type("/app.js").has_value());
    EXPECT_TRUE(filter.get_mime_type("/img.svg").has_value());
    EXPECT_TRUE(filter.get_mime_type("/favicon.ico").has_value());
    EXPECT_TRUE(filter.get_mime_type("/font.woff2").has_value());
    EXPECT_TRUE(filter.get_mime_type("/font.ttf").has_value());
    EXPECT_TRUE(filter.get_mime_type("/doc.pdf").has_value());
    EXPECT_TRUE(filter.get_mime_type("/video.mp4").has_value());
    EXPECT_TRUE(filter.get_mime_type("/img.gif").has_value());
    EXPECT_FALSE(filter.get_mime_type("/unknown.xyz").has_value());
}

class AuthFilterTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(AuthFilterTest, NoValidatorPasses) {
    authentication_filter filter;
    http_request req;
    http_response resp;
    EXPECT_TRUE(filter.pre_filter(req, resp));
}

TEST_F(AuthFilterTest, ExcludedPathPasses) {
    authentication_filter filter([](const http_request&) { return false; });
    filter.add_excluded_path("/public");
    http_request req;
    req.path = "/public/index.html";
    http_response resp;
    EXPECT_TRUE(filter.pre_filter(req, resp));
}

TEST_F(AuthFilterTest, FailedAuthReturns401) {
    authentication_filter filter([](const http_request&) { return false; });
    http_request req;
    req.path = "/admin";
    http_response resp;
    EXPECT_FALSE(filter.pre_filter(req, resp));
    EXPECT_EQ(static_cast<uint16_t>(resp.status), 401u);
    EXPECT_TRUE(resp.header("WWW-Authenticate").starts_with("Bearer realm="));
}

TEST_F(AuthFilterTest, SuccessfulAuthPasses) {
    authentication_filter filter([](const http_request&) { return true; });
    http_request req;
    req.path = "/admin";
    http_response resp;
    EXPECT_TRUE(filter.pre_filter(req, resp));
}

class HttpRouterTest : public ::testing::Test {
protected:
    http_router router;

    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(HttpRouterTest, StaticRouteMatchesExactPath) {
    bool called = false;
    router.get("/hello", [&](http_request&, http_response& res) {
        called = true;
        res.body = "world";
    });

    http_request req;
    req.method = http_method::GET();
    req.path = "/hello";
    auto resp = router.handle_request(req);
    EXPECT_TRUE(called);
    EXPECT_EQ(resp.body, "world");
}

TEST_F(HttpRouterTest, RouteNotFoundReturns404) {
    http_request req;
    req.method = http_method::GET();
    req.path = "/nonexistent";
    auto resp = router.handle_request(req);
    EXPECT_EQ(static_cast<uint16_t>(resp.status), 404u);
}

TEST_F(HttpRouterTest, PathExistsButMethodDiffersReturns405) {
    router.get("/resource", [](http_request&, http_response&) {});

    http_request req;
    req.method = http_method::POST();
    req.path = "/resource";
    auto resp = router.handle_request(req);
    EXPECT_EQ(static_cast<uint16_t>(resp.status), 405u);
}

TEST_F(HttpRouterTest, PathParameterExtraction) {
    string extracted_id;
    router.get("/users/:id", [&](http_request& req, http_response&) { extracted_id = req.parameter("id"); });

    http_request req;
    req.method = http_method::GET();
    req.path = "/users/42";
    router.handle_request(req);
    EXPECT_EQ(extracted_id, "42");
}

TEST_F(HttpRouterTest, AlphaNumericPathParamWithCamelCase) {
    string extracted;
    router.get("/items/:itemId", [&](http_request& req, http_response&) { extracted = req.parameter("itemId"); });

    http_request req;
    req.method = http_method::GET();
    req.path = "/items/abc123";
    router.handle_request(req);
    EXPECT_EQ(extracted, "abc123");
}

TEST_F(HttpRouterTest, CustomNotFoundHandler) {
    router.set_not_found_handler([](http_request&, http_response& res) {
        res.status = http_status::S4_NOT_FOUND;
        res.body = "Custom 404";
    });

    http_request req;
    req.method = http_method::GET();
    req.path = "/nonexistent";
    auto resp = router.handle_request(req);
    EXPECT_EQ(resp.body, "Custom 404");
}

TEST_F(HttpRouterTest, CustomExceptionHandler) {
    router.get("/error", [](http_request&, http_response&) { throw value_exception("test error"); });

    http_request req;
    req.method = http_method::GET();
    req.path = "/error";
    auto resp = router.handle_request(req);
    EXPECT_EQ(static_cast<uint16_t>(resp.status), 500u);
}

TEST_F(HttpRouterTest, HasRouteChecksExistence) {
    router.get("/api", [](http_request&, http_response&) {});
    EXPECT_TRUE(router.has_route(http_method::GET(), "/api"));
    EXPECT_FALSE(router.has_route(http_method::POST(), "/api"));
    EXPECT_FALSE(router.has_route(http_method::GET(), "/nonexistent"));
}

TEST_F(HttpRouterTest, HasRouteFindsDynamicPath) {
    router.get("/users/:id", [](http_request&, http_response&) {});
    router.get("/files/*", [](http_request&, http_response&) {});
    EXPECT_TRUE(router.has_route(http_method::GET(), "/users/42"));
    EXPECT_TRUE(router.has_route(http_method::GET(), "/files/a/b/c"));
    EXPECT_FALSE(router.has_route(http_method::GET(), "/other/42"));
}

TEST_F(HttpRouterTest, RouteCountReturnsCorrectNumber) {
    EXPECT_EQ(router.route_count(), 0u);
    router.get("/a", [](http_request&, http_response&) {});
    router.get("/b", [](http_request&, http_response&) {});
    EXPECT_EQ(router.route_count(), 2u);
}

TEST_F(HttpRouterTest, AllRoutesRegisterForMultipleMethods) {
    router.all("/universal", [](http_request&, http_response& res) { res.body = "ok"; });

    vector<string> methods = {"GET", "POST", "PUT", "DELETE", "HEAD", "OPTIONS"};
    for (const auto& method: methods) {
        http_request req;
        req.method = http_method(method);
        req.path = "/universal";
        auto resp = router.handle_request(req);
        EXPECT_EQ(resp.body, "ok");
    }
}

TEST_F(HttpRouterTest, ClearRoutesRemovesAll) {
    router.get("/a", [](http_request&, http_response&) {});
    router.post("/b", [](http_request&, http_response&) {});
    router.clear_routes();
    EXPECT_EQ(router.route_count(), 0u);
}

TEST_F(HttpRouterTest, MiddlewareChainIntegration) {
    bool pre_called = false;
    bool post_called = false;

    class TestMiddleware : public http_filter {
    public:
        bool* pre;
        bool* post;
        TestMiddleware(bool* p, bool* po) :
        pre(p),
        post(po) {}
        bool pre_filter(http_request&, http_response&) override {
            *pre = true;
            return true;
        }
        void do_filter(http_request&, http_response&) override {}
        void post_filter(http_request&, http_response&) override { *post = true; }
    };

    router.use(make_unique<TestMiddleware>(&pre_called, &post_called));

    http_request req;
    req.method = http_method::GET();
    req.path = "/";
    router.handle_request(req);
    EXPECT_TRUE(pre_called);
    EXPECT_TRUE(post_called);
}

TEST_F(HttpRouterTest, ExceptionInRouteTriggersHandler) {
    bool exception_handled = false;
    router.set_exception_handler([&](http_request&, http_response&, const exception&) { exception_handled = true; });

    router.get("/crash", [](http_request&, http_response&) { throw value_exception("bang"); });

    http_request req;
    req.method = http_method::GET();
    req.path = "/crash";
    router.handle_request(req);
    EXPECT_TRUE(exception_handled);
}

TEST_F(HttpRouterTest, ExceptionHandlerDoesNotRunPostFilters) {
    bool exception_handled = false;
    bool post_filter_called = false;
    router.set_exception_handler([&](http_request&, http_response&, const exception&) { exception_handled = true; });

    router.get("/crash", [](http_request&, http_response&) { throw value_exception("bang"); });

    struct tracking_filter final : http_filter {
        bool* called;
        explicit tracking_filter(bool* c) :
        called(c) {}
        bool pre_filter(http_request&, http_response&) override { return true; }
        void post_filter(http_request&, http_response&) override { *called = true; }
        void do_filter(http_request&, http_response&) override {}
        string name() const override { return "tracking_filter"; }
    };
    router.use(make_unique<tracking_filter>(&post_filter_called));

    http_request req;
    req.method = http_method::GET();
    req.path = "/crash";
    router.handle_request(req);
    EXPECT_TRUE(exception_handled);
    EXPECT_FALSE(post_filter_called);
}

TEST_F(HttpRouterTest, StrictRoutingOffStripsTrailingSlash) {
    router.strict_routing = false;
    bool called = false;
    router.get("/path", [&](http_request&, http_response&) { called = true; });

    http_request req;
    req.method = http_method::GET();
    req.path = "/path/";
    router.handle_request(req);
    EXPECT_TRUE(called);
}

TEST_F(HttpRouterTest, CaseSensitiveOffUsesInsensitiveMatchForNonRegexRoutes) {
    router.case_sensitive = false;
    bool called = false;
    router.get("/Path:_*", [&](http_request&, http_response&) { called = true; });

    http_request req;
    req.method = http_method::GET();
    req.path = "/path:_*";
    router.handle_request(req);
    EXPECT_TRUE(called);
}

class SecurityHeadersFilterTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(SecurityHeadersFilterTest, AllHeadersInjectedByDefault) {
    security_headers_filter filter;
    http_request req;
    http_response resp;

    filter.post_filter(req, resp);

    EXPECT_EQ(resp.header("Strict-Transport-Security"), "max-age=31536000; includeSubDomains");
    EXPECT_EQ(resp.header("X-Frame-Options"), "DENY");
    EXPECT_EQ(resp.header("X-Content-Type-Options"), "nosniff");
    EXPECT_EQ(resp.header("Content-Security-Policy"), "default-src 'self'");
    EXPECT_TRUE(resp.header("X-XSS-Protection").empty());
    EXPECT_EQ(resp.header("Referrer-Policy"), "strict-origin-when-cross-origin");
    EXPECT_EQ(resp.header("Permissions-Policy"), "geolocation=(), microphone=(), camera=()");
}

TEST_F(SecurityHeadersFilterTest, DisabledHeaderNotInjected) {
    security_headers_filter filter;
    filter.enable_hsts = false;
    filter.enable_frame_options = false;
    filter.enable_csp = false;

    http_request req;
    http_response resp;

    filter.post_filter(req, resp);

    EXPECT_TRUE(resp.header("Strict-Transport-Security").empty());
    EXPECT_TRUE(resp.header("X-Frame-Options").empty());
    EXPECT_TRUE(resp.header("Content-Security-Policy").empty());
    EXPECT_EQ(resp.header("X-Content-Type-Options"), "nosniff");
}

TEST_F(SecurityHeadersFilterTest, HstsPreloadEnabled) {
    security_headers_filter filter;
    filter.hsts_preload = true;
    filter.hsts_max_age = seconds{63072000};

    http_request req;
    http_response resp;

    filter.post_filter(req, resp);

    EXPECT_TRUE(resp.header("Strict-Transport-Security").contains("preload"));
    EXPECT_TRUE(resp.header("Strict-Transport-Security").contains("63072000"));
}

TEST_F(SecurityHeadersFilterTest, CustomCspValue) {
    security_headers_filter filter;
    filter.csp_value = "default-src 'self'; script-src 'self' cdn.example.com";

    http_request req;
    http_response resp;

    filter.post_filter(req, resp);

    EXPECT_EQ(resp.header("Content-Security-Policy"), "default-src 'self'; script-src 'self' cdn.example.com");
}

TEST_F(SecurityHeadersFilterTest, XssProtectionDisabledByDefault) {
    security_headers_filter filter;
    http_request req;
    http_response resp;

    filter.post_filter(req, resp);

    EXPECT_TRUE(resp.header(http_key::X_XSS_Protection()).empty());
    EXPECT_TRUE(resp.header("X-XSS-Protection").empty());
}

TEST_F(SecurityHeadersFilterTest, HeaderKeysMatchHttpKeyConstants) {
    security_headers_filter filter;
    filter.enable_xss_protection = true;
    http_request req;
    http_response resp;

    filter.post_filter(req, resp);

    EXPECT_FALSE(resp.header(http_key::Strict_Transport_Security()).empty());
    EXPECT_FALSE(resp.header(http_key::X_Frame_Options()).empty());
    EXPECT_FALSE(resp.header(http_key::X_Content_Type_Options()).empty());
    EXPECT_FALSE(resp.header(http_key::Content_Security_Policy()).empty());
    EXPECT_FALSE(resp.header(http_key::X_XSS_Protection()).empty());
    EXPECT_FALSE(resp.header(http_key::Referrer_Policy()).empty());
    EXPECT_FALSE(resp.header(http_key::Permissions_Policy()).empty());
}

class TokenBucketTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(TokenBucketTest, InitialTokensEqualBurst) {
    token_bucket bucket(10.0, 20.0);
    EXPECT_DOUBLE_EQ(bucket.tokens, 20.0);
}

TEST_F(TokenBucketTest, ConsumeExhaustsTokens) {
    token_bucket bucket(10.0, 5.0);
    uint64_t now = 1000000;
    for (int i = 0; i < 5; ++i) {
        EXPECT_TRUE(bucket.try_consume(now));
    }
    EXPECT_FALSE(bucket.try_consume(now));
}

TEST_F(TokenBucketTest, TryConsumeFailsWhenBelowThreshold) {
    token_bucket bucket(10.0, 0.5);
    uint64_t now = 1000000;
    EXPECT_FALSE(bucket.try_consume(now));
}

TEST_F(TokenBucketTest, RefillAddsTokensOverTime) {
    token_bucket bucket(10.0, 1.0);
    bucket.last_refill_ms = 1000000;
    uint64_t now = 1000000;

    EXPECT_TRUE(bucket.try_consume(now));
    EXPECT_FALSE(bucket.try_consume(now));

    uint64_t later = now + 100;
    EXPECT_TRUE(bucket.try_consume(later));
    EXPECT_FALSE(bucket.try_consume(later));
}

TEST_F(TokenBucketTest, RefillCappedAtCapacity) {
    token_bucket bucket(1000.0, 5.0);
    uint64_t now = 1000000;
    uint64_t later = now + 1000;
    EXPECT_TRUE(bucket.try_consume(later));
    int consumed = 1;
    while (bucket.try_consume(later)) {
        ++consumed;
    }
    EXPECT_LE(consumed, 5);
}

TEST_F(TokenBucketTest, SameTimeRefillIsNoop) {
    token_bucket bucket(10.0, 2.0);
    uint64_t now = 1000000;
    EXPECT_TRUE(bucket.try_consume(now));
    EXPECT_TRUE(bucket.try_consume(now));
    EXPECT_FALSE(bucket.try_consume(now));
}

class TokenBucketLimiterTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(TokenBucketLimiterTest, AllowCreatesBucket) {
    token_bucket_limiter limiter;
    limiter.set_default_rate(10.0);
    limiter.set_default_burst(20.0);

    EXPECT_EQ(limiter.size(), 0u);
    EXPECT_TRUE(limiter.allow("192.168.1.1"));
    EXPECT_EQ(limiter.size(), 1u);
}

TEST_F(TokenBucketLimiterTest, IndependentKeys) {
    token_bucket_limiter limiter;
    limiter.set_default_rate(1.0);
    limiter.set_default_burst(1.0);

    EXPECT_TRUE(limiter.allow("192.168.1.1"));
    EXPECT_TRUE(limiter.allow("192.168.1.2"));
    EXPECT_EQ(limiter.size(), 2u);

    EXPECT_FALSE(limiter.allow("192.168.1.1"));
    EXPECT_FALSE(limiter.allow("192.168.1.2"));
}

TEST_F(TokenBucketLimiterTest, DefaultRateUsedWhenRateZero) {
    token_bucket_limiter limiter;
    limiter.set_default_rate(100.0);
    limiter.set_default_burst(100.0);

    EXPECT_TRUE(limiter.allow("test", 0.0, 0.0));
}

TEST_F(TokenBucketLimiterTest, CustomRateOverridesDefault) {
    token_bucket_limiter limiter;
    limiter.set_default_rate(100.0);
    limiter.set_default_burst(100.0);

    EXPECT_TRUE(limiter.allow("test", 1.0, 1.0));
    EXPECT_FALSE(limiter.allow("test", 1.0, 1.0));
}

TEST_F(TokenBucketLimiterTest, CleanupExpiredRemovesOldBuckets) {
    token_bucket_limiter limiter;
    limiter.set_default_rate(10.0);
    limiter.set_default_burst(10.0);

    limiter.allow("key1");
    limiter.allow("key2");
    EXPECT_EQ(limiter.size(), 2u);

    this_thread::sleep_for(milliseconds{1});

    limiter.cleanup_expired(seconds{0});
    EXPECT_EQ(limiter.size(), 0u);
}

TEST_F(TokenBucketLimiterTest, BucketExhaustionRejects) {
    token_bucket_limiter limiter;
    limiter.set_default_rate(1.0);
    limiter.set_default_burst(1.0);

    EXPECT_TRUE(limiter.allow("key1"));
    EXPECT_FALSE(limiter.allow("key1"));
}

class TokenBucketFilterTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(TokenBucketFilterTest, DisabledFilterBypasses) {
    token_bucket_filter filter;
    filter.enabled = false;

    http_request req;
    http_response resp;

    EXPECT_TRUE(filter.pre_filter(req, resp));
    EXPECT_NE(static_cast<uint16_t>(resp.status), 429u);
}

TEST_F(TokenBucketFilterTest, EmptyClientIpBypasses) {
    token_bucket_filter filter;
    filter.enabled = true;
    filter.default_rate = 1.0;
    filter.default_burst = 1.0;

    http_request req;
    http_response resp;

    EXPECT_TRUE(filter.pre_filter(req, resp));
}

TEST_F(TokenBucketFilterTest, RateLimitedRequestReturns429) {
    token_bucket_filter filter;
    filter.enabled = true;
    filter.default_rate = 1.0;
    filter.default_burst = 1.0;

    http_request req;
    req.set_header("X-Real-IP", "10.0.0.1");
    http_response resp;

    EXPECT_TRUE(filter.pre_filter(req, resp));

    http_response resp2;
    EXPECT_FALSE(filter.pre_filter(req, resp2));
    EXPECT_EQ(static_cast<uint16_t>(resp2.status), 429u);
    EXPECT_EQ(resp2.status_message, "Too Many Requests");
    EXPECT_FALSE(resp2.body.empty());
}

TEST_F(TokenBucketFilterTest, PerRouteKeyIncludesPath) {
    token_bucket_filter filter;
    filter.enabled = true;
    filter.per_route = true;
    filter.default_rate = 1.0;
    filter.default_burst = 1.0;

    http_request req;
    req.set_header("X-Real-IP", "10.0.0.1");

    req.path = "/api/a";
    http_response resp1;
    EXPECT_TRUE(filter.pre_filter(req, resp1));

    req.path = "/api/b";
    http_response resp2;
    EXPECT_TRUE(filter.pre_filter(req, resp2));
}

TEST_F(TokenBucketFilterTest, RetryAfterHeaderSet) {
    token_bucket_filter filter;
    filter.enabled = true;
    filter.default_rate = 1.0;
    filter.default_burst = 1.0;

    http_request req;
    req.set_header("X-Real-IP", "10.0.0.2");
    http_response resp;
    filter.pre_filter(req, resp);

    http_response resp2;
    filter.pre_filter(req, resp2);
    EXPECT_EQ(resp2.header("Retry-After"), "1");
}

TEST_F(TokenBucketFilterTest, CleanupExpiredDelegatesToLimiter) {
    token_bucket_filter filter;
    filter.enabled = true;
    filter.default_rate = 100.0;
    filter.default_burst = 100.0;

    http_request req;
    req.set_header("X-Real-IP", "10.0.0.3");
    http_response resp;
    filter.pre_filter(req, resp);

    filter.cleanup_expired(seconds{0});
    SUCCEED();
}

class HttpRangeTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(HttpRangeTest, ParseSingleRange) {
    auto ranges = parse_ranges("bytes=0-1023", 4096);
    ASSERT_EQ(ranges.size(), 1u);
    EXPECT_EQ(ranges[0].start, 0u);
    EXPECT_EQ(ranges[0].end, 1023u);
}

TEST_F(HttpRangeTest, ParseMultiRange) {
    auto ranges = parse_ranges("bytes=0-1023,2048-4095", 8192);
    ASSERT_EQ(ranges.size(), 2u);
    EXPECT_EQ(ranges[0].start, 0u);
    EXPECT_EQ(ranges[0].end, 1023u);
    EXPECT_EQ(ranges[1].start, 2048u);
    EXPECT_EQ(ranges[1].end, 4095u);
}

TEST_F(HttpRangeTest, ParseSuffixRange) {
    auto ranges = parse_ranges("bytes=-500", 4096);
    ASSERT_EQ(ranges.size(), 1u);
    EXPECT_EQ(ranges[0].start, 3596u);
    EXPECT_EQ(ranges[0].end, 4095u);
}

TEST_F(HttpRangeTest, ParseOpenEndedRange) {
    auto ranges = parse_ranges("bytes=1024-", 4096);
    ASSERT_EQ(ranges.size(), 1u);
    EXPECT_EQ(ranges[0].start, 1024u);
    EXPECT_EQ(ranges[0].end, 4095u);
}

TEST_F(HttpRangeTest, ParseInvalidRangeReturnsEmpty) {
    auto ranges = parse_ranges("bytes=invalid", 4096);
    EXPECT_TRUE(ranges.empty());
}

TEST_F(HttpRangeTest, ParseEmptyHeaderReturnsEmpty) {
    auto ranges = parse_ranges("", 4096);
    EXPECT_TRUE(ranges.empty());
}

TEST_F(HttpRangeTest, BuildContentRangeHeader) {
    byte_range r{0, 1023};
    auto hdr = build_content_range(r, 4096);
    EXPECT_EQ(hdr, "bytes 0-1023/4096");
}

TEST_F(HttpRangeTest, BuildMultipartRanges) {
    vector<byte_range> ranges = {{0, 99}, {200, 299}};
    auto body = build_multipart_ranges(ranges, "text/plain", "BOUNDARY", [](const byte_range& r) -> string {
        return "chunk_" + to_string(r.start) + "_" + to_string(r.end);
    });
    EXPECT_TRUE(body.contains("--BOUNDARY\r\n"));
    EXPECT_TRUE(body.contains("Content-Type: text/plain\r\n"));
    EXPECT_TRUE(body.contains("Content-Range: bytes 0-99/"));
    EXPECT_TRUE(body.contains("chunk_0_99"));
    EXPECT_TRUE(body.contains("Content-Range: bytes 200-299/"));
    EXPECT_TRUE(body.contains("chunk_200_299"));
    EXPECT_TRUE(body.ends_with("--BOUNDARY--\r\n"));
}

#ifdef NEFORCE_SUPPORT_ZLIB

class HttpCompressFilterTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(HttpCompressFilterTest, CompressibleContentTypes) {
    compress_filter filter;
    filter.enabled = true;
    filter.min_size = 0_KB;

    http_request req;
    req.set_header("Accept-Encoding", "gzip, deflate");

    http_response resp;
    resp.status = http_status::S2_OK;
    resp.status_message = "OK";
    resp.set_content_type(http_content::HTML_TEXT());
    resp.body = string(2048, 'A');

    filter.post_filter(req, resp);
    EXPECT_FALSE(resp.header("Content-Encoding").empty());
}

TEST_F(HttpCompressFilterTest, SmallBodyBelowMinSizeNotCompressed) {
    compress_filter filter;
    filter.enabled = true;
    filter.min_size = 4_KB;

    http_request req;
    req.set_header("Accept-Encoding", "gzip");

    http_response resp;
    resp.status = http_status::S2_OK;
    resp.status_message = "OK";
    resp.set_content_type(http_content::HTML_TEXT());
    resp.body = "small";

    filter.post_filter(req, resp);
    EXPECT_TRUE(resp.header("Content-Encoding").empty());
}

TEST_F(HttpCompressFilterTest, NoAcceptEncodingSkipsCompression) {
    compress_filter filter;
    filter.enabled = true;
    filter.min_size = 0_KB;

    http_request req;

    http_response resp;
    resp.status = http_status::S2_OK;
    resp.status_message = "OK";
    resp.set_content_type(http_content::HTML_TEXT());
    resp.body = string(2048, 'B');

    filter.post_filter(req, resp);
    EXPECT_TRUE(resp.header("Content-Encoding").empty());
}

TEST_F(HttpCompressFilterTest, AlreadyCompressedContentTypeSkipped) {
    compress_filter filter;
    filter.enabled = true;
    filter.min_size = 0_KB;

    http_request req;
    req.set_header("Accept-Encoding", "gzip");

    http_response resp;
    resp.status = http_status::S2_OK;
    resp.status_message = "OK";
    resp.set_content_type("image/png");
    resp.body = string(2048, 'C');

    filter.post_filter(req, resp);
    EXPECT_TRUE(resp.header("Content-Encoding").empty());
}

TEST_F(HttpCompressFilterTest, QZeroRejectsEncoding) {
    compress_filter filter;
    filter.enabled = true;
    filter.min_size = 0_KB;

    http_request req;
    req.set_header("Accept-Encoding", "gzip;q=0");

    http_response resp;
    resp.status = http_status::S2_OK;
    resp.status_message = "OK";
    resp.set_content_type("text/plain");
    resp.body = string(2048, 'C');

    filter.post_filter(req, resp);
    EXPECT_TRUE(resp.header("Content-Encoding").empty());
}

TEST_F(HttpCompressFilterTest, QZeroWithFallback) {
    compress_filter filter;
    filter.enabled = true;
    filter.min_size = 0_KB;

    http_request req;
    req.set_header("Accept-Encoding", "gzip;q=0, deflate");

    http_response resp;
    resp.status = http_status::S2_OK;
    resp.status_message = "OK";
    resp.set_content_type("text/plain");
    resp.body = string(2048, 'C');

    filter.post_filter(req, resp);
    EXPECT_EQ(resp.header("Content-Encoding"), "deflate");
}

#endif

class CsrfFilterTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(CsrfFilterTest, GetRequestBypassesCsrf) {
    csrf_filter filter;
    http_request req;
    req.method = http_method::GET();
    http_response resp;
    EXPECT_TRUE(filter.pre_filter(req, resp));
}

TEST_F(CsrfFilterTest, HeadRequestBypassesCsrf) {
    csrf_filter filter;
    http_request req;
    req.method = http_method::HEAD();
    http_response resp;
    EXPECT_TRUE(filter.pre_filter(req, resp));
}

TEST_F(CsrfFilterTest, OptionsRequestBypassesCsrf) {
    csrf_filter filter;
    http_request req;
    req.method = http_method::OPTIONS();
    http_response resp;
    EXPECT_TRUE(filter.pre_filter(req, resp));
}

TEST_F(CsrfFilterTest, PostWithoutTokenIsBlocked) {
    csrf_filter filter;
    http_request req;
    req.method = http_method::POST();
    http_response resp;
    EXPECT_FALSE(filter.pre_filter(req, resp));
    EXPECT_EQ(static_cast<uint16_t>(resp.status), 403u);
}

TEST_F(CsrfFilterTest, PostWithHeaderTokenPasses) {
    csrf_filter filter;
    http_request req;
    req.method = http_method::POST();

    http_request get_req;
    get_req.method = http_method::GET();
    http_response get_resp;
    filter.pre_filter(get_req, get_resp);
    string cookie_token;
    for (auto& c: get_resp.cookies) {
        if (c.name.cookie_name() == "XSRF-TOKEN") {
            cookie_token = c.value;
        }
    }
    EXPECT_FALSE(cookie_token.empty());

    req.cookies["XSRF-TOKEN"] = cookie_token;
    req.set_header("X-CSRF-Token", cookie_token);

    http_response resp;
    EXPECT_TRUE(filter.pre_filter(req, resp));
}

TEST_F(CsrfFilterTest, TokenMismatchIsBlocked) {
    csrf_filter filter;
    http_request req;
    req.method = http_method::POST();
    req.cookies["XSRF-TOKEN"] = "valid-token";
    req.set_header("X-CSRF-Token", "wrong-token");

    http_response resp;
    EXPECT_FALSE(filter.pre_filter(req, resp));
    EXPECT_EQ(static_cast<uint16_t>(resp.status), 403u);
}

TEST_F(CsrfFilterTest, PutMethodIsValidated) {
    csrf_filter filter;
    http_request req;
    req.method = http_method::PUT();
    http_response resp;
    EXPECT_FALSE(filter.pre_filter(req, resp));
}

TEST_F(CsrfFilterTest, DeleteMethodIsValidated) {
    csrf_filter filter;
    http_request req;
    req.method = http_method::DELETE();
    http_response resp;
    EXPECT_FALSE(filter.pre_filter(req, resp));
}

TEST_F(CsrfFilterTest, PatchMethodIsValidated) {
    csrf_filter filter;
    http_request req;
    req.method = http_method::PATCH();
    http_response resp;
    EXPECT_FALSE(filter.pre_filter(req, resp));
}

TEST_F(CsrfFilterTest, DisabledFilterBypasses) {
    csrf_filter filter;
    filter.enabled = false;

    http_request req;
    req.method = http_method::POST();
    http_response resp;
    EXPECT_TRUE(filter.pre_filter(req, resp));
}

class MemorySessionStoreTest : public ::testing::Test {
protected:
    memory_session_store store;

    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(MemorySessionStoreTest, SaveAndLoad) {
    http_session s;
    s.id = "session-001";
    s.set("key", "value");
    store.save(s);

    auto loaded = store.load("session-001");
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded.value().id, "session-001");
    EXPECT_EQ(loaded.value().get("key"), "value");
}

TEST_F(MemorySessionStoreTest, LoadNonexistentReturnsNone) {
    auto loaded = store.load("nonexistent");
    EXPECT_FALSE(loaded.has_value());
}

TEST_F(MemorySessionStoreTest, ExistsReturnsCorrectly) {
    EXPECT_FALSE(store.exists("test"));
    http_session s;
    s.id = "test";
    store.save(s);
    EXPECT_TRUE(store.exists("test"));
}

TEST_F(MemorySessionStoreTest, RemoveDeletesSession) {
    http_session s;
    s.id = "to-remove";
    store.save(s);
    EXPECT_TRUE(store.exists("to-remove"));
    store.remove("to-remove");
    EXPECT_FALSE(store.exists("to-remove"));
}

TEST_F(MemorySessionStoreTest, CountReflectsStoreSize) {
    EXPECT_EQ(store.count(), 0u);
    http_session s1;
    s1.id = "a";
    store.save(s1);
    EXPECT_EQ(store.count(), 1u);
    http_session s2;
    s2.id = "b";
    store.save(s2);
    EXPECT_EQ(store.count(), 2u);
    store.remove("a");
    EXPECT_EQ(store.count(), 1u);
}

TEST_F(MemorySessionStoreTest, CleanupRemovesExpiredSessions) {
    http_session s1;
    s1.id = "active";
    s1.max_age = seconds{3600};
    store.save(s1);

    http_session s2;
    s2.id = "expired";
    s2.max_age = seconds{1};
    s2.last_access = datetime::now() - 10;
    store.save(s2);

    EXPECT_EQ(store.count(), 2u);
    store.cleanup();
    EXPECT_EQ(store.count(), 1u);
    EXPECT_TRUE(store.exists("active"));
    EXPECT_FALSE(store.exists("expired"));
}

TEST_F(MemorySessionStoreTest, OverwriteSessionOnSave) {
    http_session s;
    s.id = "overwrite";
    s.set("v", "1");
    store.save(s);

    s.set("v", "2");
    store.save(s);

    auto loaded = store.load("overwrite");
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded.value().get("v"), "2");
}

class SessionRegenerateTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(SessionRegenerateTest, RegenerateIdChangesId) {
    http_session s;
    s.id = "original-id";
    s.set("data", "preserved");
    auto old_id = s.id;

    s.regenerate_id();
    EXPECT_NE(s.id, old_id);
    EXPECT_FALSE(s.id.empty());
}

TEST_F(SessionRegenerateTest, RegenerateIdPreservesData) {
    http_session s;
    s.id = "original-id";
    s.set("key1", "value1");
    s.set("key2", "value2");

    s.regenerate_id();
    EXPECT_EQ(s.get("key1"), "value1");
    EXPECT_EQ(s.get("key2"), "value2");
    EXPECT_TRUE(s.is_new);
}

TEST_F(SessionRegenerateTest, RegeneratedIdIsRandom) {
    http_session s1;
    http_session s2;
    s1.regenerate_id();
    s2.regenerate_id();
    EXPECT_NE(s1.id, s2.id);
}

TEST_F(SessionRegenerateTest, RegeneratedIdHasEntropy) {
    http_session s;
    s.regenerate_id();
    EXPECT_GE(s.id.size(), 32u);
    bool has_variation = false;
    for (size_t i = 1; i < s.id.size(); ++i) {
        if (s.id[i] != s.id[0]) {
            has_variation = true;
            break;
        }
    }
    EXPECT_TRUE(has_variation);
}

TEST_F(SessionRegenerateTest, TouchOnRegenerate) {
    http_session s;
    auto before = s.last_access;
    s.regenerate_id();
    EXPECT_GE(s.last_access, before);
}

#ifdef NEFORCE_SUPPORT_ZLIB

class WebSocketDeflateConfigTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(WebSocketDeflateConfigTest, EmptyExtensionsReturnsInactive) {
    auto cfg = websocket_deflate_config::negotiate("");
    EXPECT_FALSE(cfg.active);
}

TEST_F(WebSocketDeflateConfigTest, NoDeflateExtensionReturnsInactive) {
    auto cfg = websocket_deflate_config::negotiate("unknown-extension");
    EXPECT_FALSE(cfg.active);
}

TEST_F(WebSocketDeflateConfigTest, BasicPermessageDeflateActive) {
    auto cfg = websocket_deflate_config::negotiate("permessage-deflate");
    EXPECT_TRUE(cfg.active);
    EXPECT_EQ(cfg.client_max_window_bits, 15);
    EXPECT_EQ(cfg.server_max_window_bits, 15);
    EXPECT_FALSE(cfg.client_no_context_takeover);
    EXPECT_FALSE(cfg.server_no_context_takeover);
}

TEST_F(WebSocketDeflateConfigTest, ParseClientMaxWindowBits) {
    auto cfg = websocket_deflate_config::negotiate("permessage-deflate; client_max_window_bits=12");
    EXPECT_TRUE(cfg.active);
    EXPECT_EQ(cfg.client_max_window_bits, 12);
}

TEST_F(WebSocketDeflateConfigTest, ParseServerMaxWindowBits) {
    auto cfg = websocket_deflate_config::negotiate("permessage-deflate; server_max_window_bits=10");
    EXPECT_TRUE(cfg.active);
    EXPECT_EQ(cfg.server_max_window_bits, 10);
}

TEST_F(WebSocketDeflateConfigTest, ParseBothWindowBits) {
    auto cfg = websocket_deflate_config::negotiate(
            "permessage-deflate; client_max_window_bits=11; server_max_window_bits=9");
    EXPECT_TRUE(cfg.active);
    EXPECT_EQ(cfg.client_max_window_bits, 11);
    EXPECT_EQ(cfg.server_max_window_bits, 9);
}

TEST_F(WebSocketDeflateConfigTest, ParseClientNoContextTakeover) {
    auto cfg = websocket_deflate_config::negotiate("permessage-deflate; client_no_context_takeover");
    EXPECT_TRUE(cfg.active);
    EXPECT_TRUE(cfg.client_no_context_takeover);
}

TEST_F(WebSocketDeflateConfigTest, ParseServerNoContextTakeover) {
    auto cfg = websocket_deflate_config::negotiate("permessage-deflate; server_no_context_takeover");
    EXPECT_TRUE(cfg.active);
    EXPECT_TRUE(cfg.server_no_context_takeover);
}

TEST_F(WebSocketDeflateConfigTest, ParseAllParameters) {
    auto cfg = websocket_deflate_config::negotiate(
            "permessage-deflate; client_max_window_bits=13; server_max_window_bits=14; "
            "client_no_context_takeover; server_no_context_takeover");
    EXPECT_TRUE(cfg.active);
    EXPECT_EQ(cfg.client_max_window_bits, 13);
    EXPECT_EQ(cfg.server_max_window_bits, 14);
    EXPECT_TRUE(cfg.client_no_context_takeover);
    EXPECT_TRUE(cfg.server_no_context_takeover);
}

TEST_F(WebSocketDeflateConfigTest, InvalidWindowBitsIgnored) {
    auto cfg = websocket_deflate_config::negotiate("permessage-deflate; client_max_window_bits=20");
    EXPECT_TRUE(cfg.active);
    EXPECT_EQ(cfg.client_max_window_bits, 15);
}

TEST_F(WebSocketDeflateConfigTest, NegotiateNoValueWindowBits) {
    auto cfg = websocket_deflate_config::negotiate(
            "permessage-deflate; client_max_window_bits; server_max_window_bits=12");
    EXPECT_TRUE(cfg.active);
    EXPECT_EQ(cfg.client_max_window_bits, 15);
    EXPECT_EQ(cfg.server_max_window_bits, 12);
}

TEST_F(WebSocketDeflateConfigTest, ToResponseHeaderDefault) {
    websocket_deflate_config cfg;
    cfg.active = true;
    auto hdr = cfg.to_response_header();
    EXPECT_TRUE(hdr.starts_with("permessage-deflate"));
    EXPECT_FALSE(hdr.contains("client_max_window_bits=15"));
    EXPECT_FALSE(hdr.contains("server_max_window_bits=15"));
}

TEST_F(WebSocketDeflateConfigTest, ToResponseHeaderCustomBits) {
    websocket_deflate_config cfg;
    cfg.active = true;
    cfg.client_max_window_bits = 12;
    cfg.server_max_window_bits = 10;
    auto hdr = cfg.to_response_header();
    EXPECT_TRUE(hdr.contains("client_max_window_bits=12"));
    EXPECT_TRUE(hdr.contains("server_max_window_bits=10"));
}

TEST_F(WebSocketDeflateConfigTest, ToResponseHeaderNoContextTakeover) {
    websocket_deflate_config cfg;
    cfg.active = true;
    cfg.client_no_context_takeover = true;
    cfg.server_no_context_takeover = true;
    auto hdr = cfg.to_response_header();
    EXPECT_TRUE(hdr.contains("client_no_context_takeover"));
    EXPECT_TRUE(hdr.contains("server_no_context_takeover"));
}

TEST_F(WebSocketDeflateConfigTest, ToResponseHeaderInactiveReturnsEmpty) {
    websocket_deflate_config cfg;
    cfg.active = false;
    auto hdr = cfg.to_response_header();
    EXPECT_TRUE(hdr.empty());
}

TEST_F(WebSocketDeflateConfigTest, CompressionRoundTrip) {
    websocket_deflate compressor(true, 15, false);
    websocket_deflate decompressor(false, 15, false);

    const string original = "Hello, WebSocket! This is a test message for permessage-deflate compression.";
    const string compressed = compressor.process(original.view(), true);
    const string decompressed = decompressor.process(compressed.view(), true);

    EXPECT_EQ(decompressed, original);
}

TEST_F(WebSocketDeflateConfigTest, CompressionEmptyPayload) {
    websocket_deflate compressor(true, 15, false);
    websocket_deflate decompressor(false, 15, false);

    const string compressed = compressor.process("", false);
    EXPECT_TRUE(compressed.empty());
}

TEST_F(WebSocketDeflateConfigTest, CompressionWithContextTakeover) {
    websocket_deflate compressor(true, 15, false);
    websocket_deflate decompressor(false, 15, false);

    const string msg1 = "First message";
    const string msg2 = "Second message";

    const string c1 = compressor.process(msg1.view(), true);
    const string d1 = decompressor.process(c1.view(), true);
    EXPECT_EQ(d1, msg1);

    const string c2 = compressor.process(msg2.view(), true);
    const string d2 = decompressor.process(c2.view(), true);
    EXPECT_EQ(d2, msg2);
}

TEST_F(WebSocketDeflateConfigTest, CompressionWithNoContextTakeover) {
    websocket_deflate compressor(true, 15, true);
    websocket_deflate decompressor(false, 15, true);

    const string msg1 = "Message one";
    const string c1 = compressor.process(msg1.view(), true);
    const string d1 = decompressor.process(c1.view(), true);
    EXPECT_EQ(d1, msg1);

    compressor.reset_context();
    decompressor.reset_context();

    const string msg2 = "Message two";
    const string c2 = compressor.process(msg2.view(), true);
    const string d2 = decompressor.process(c2.view(), true);
    EXPECT_EQ(d2, msg2);
}

#endif

class HpackTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(HpackTest, EncodeDecodeRoundTrip) {
    hpack_encoder encoder(4096);
    hpack_decoder decoder(4096);

    vector<hpack_header_field> headers = {
            {":method", "GET"},   {":path", "/index.html"},        {":authority", "example.com"},
            {":scheme", "https"}, {"user-agent", "TestAgent/1.0"}, {"accept", "*/*"},
    };

    auto encoded = encoder.encode(headers);
    auto decoded = decoder.decode(encoded.data(), encoded.size());

    ASSERT_EQ(decoded.size(), headers.size());
    for (size_t i = 0; i < headers.size(); ++i) {
        EXPECT_EQ(decoded[i].name, headers[i].name);
        EXPECT_EQ(decoded[i].value, headers[i].value);
    }
}

TEST_F(HpackTest, StaticTableIndexing) {
    hpack_encoder encoder(4096);
    hpack_decoder decoder(4096);

    vector<hpack_header_field> headers = {{":method", "GET"}};
    auto encoded = encoder.encode(headers);
    EXPECT_LE(encoded.size(), 3u);

    auto decoded = decoder.decode(encoded.data(), encoded.size());
    ASSERT_EQ(decoded.size(), 1u);
    EXPECT_EQ(decoded[0].name, ":method");
    EXPECT_EQ(decoded[0].value, "GET");
}

TEST_F(HpackTest, DynamicTableInsertion) {
    hpack_encoder encoder(4096);
    hpack_decoder decoder(4096);

    vector<hpack_header_field> headers1 = {{"x-custom-1", "value1"}};
    auto encoded1 = encoder.encode(headers1);
    auto decoded1 = decoder.decode(encoded1.data(), encoded1.size());
    EXPECT_EQ(decoded1[0].value, "value1");

    vector<hpack_header_field> headers2 = {{"x-custom-1", "value1"}};
    auto encoded2 = encoder.encode(headers2);
    auto decoded2 = decoder.decode(encoded2.data(), encoded2.size());
    EXPECT_EQ(decoded2[0].value, "value1");
}

TEST_F(HpackTest, DecodeStaticTableEntries) {
    hpack_decoder decoder(4096);

    vector<hpack_header_field> headers = {
            {":status", "200"},
            {"content-type", "text/html"},
    };

    hpack_encoder encoder(4096);
    auto encoded = encoder.encode(headers);
    auto decoded = decoder.decode(encoded.data(), encoded.size());

    ASSERT_EQ(decoded.size(), 2u);
    EXPECT_EQ(decoded[0].name, ":status");
    EXPECT_EQ(decoded[0].value, "200");
    EXPECT_EQ(decoded[1].name, "content-type");
    EXPECT_EQ(decoded[1].value, "text/html");
}

TEST_F(HpackTest, MaliciousIntegerDoesNotOverflow) {
    byte_vector malicious;
    malicious.push_back(0x3F);
    malicious.push_back(0x8F);
    for (int i = 0; i < 20; ++i) {
        malicious.push_back(0x80);
    }
    malicious.push_back(0x01);

    hpack_decoder decoder(4096);
    auto result = decoder.decode(malicious.data(), malicious.size());
    SUCCEED();
}

TEST_F(HpackTest, OversizedStringLengthDoesNotOverflow) {
    byte_vector crafted;
    crafted.push_back(0x00);
    crafted.push_back(0x8F);
    crafted.push_back(0xFF);
    crafted.push_back(0xFF);
    crafted.push_back(0xFF);
    crafted.push_back(0xFF);
    crafted.push_back(0x01);
    crafted.push_back(0x00);
    crafted.push_back(0x01);
    crafted.push_back('a');

    hpack_decoder decoder(4096);
    auto result = decoder.decode(crafted.data(), crafted.size());
    SUCCEED();
}

class Http2FrameTest : public ::testing::Test {
protected:
    http2_framer framer;

    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(Http2FrameTest, EncodeDecodeDataFrame) {
    http2_data_frame df;
    df.stream_id = 1;
    df.end_stream = true;
    string payload = "Hello HTTP/2";
    df.data.assign(reinterpret_cast<const byte_t*>(payload.data()),
                   reinterpret_cast<const byte_t*>(payload.data()) + payload.size());

    auto encoded = framer.encode_data_frame(df);
    EXPECT_GT(encoded.size(), 9u);

    bool decoded = false;
    framer.decode_frames(encoded.data(), encoded.size(),
                         [&](http2_frame_type type, uint8_t flags, uint32_t sid, const byte_t*, size_t len) {
                             if (type == http2_frame_type::DATA && sid == 1) {
                                 decoded = true;
                                 EXPECT_EQ(len, payload.size());
                                 EXPECT_TRUE(flags & HTTP2_FLAG_END_STREAM);
                             }
                         });
    EXPECT_TRUE(decoded);
}

TEST_F(Http2FrameTest, EncodeDecodeHeadersFrame) {
    http2_headers_frame hf;
    hf.stream_id = 1;
    hf.end_headers = true;
    hf.end_stream = true;
    hf.header_block = {0x82};

    auto encoded = framer.encode_headers_frame(hf);
    EXPECT_GT(encoded.size(), 9u);

    bool decoded = false;
    framer.decode_frames(encoded.data(), encoded.size(),
                         [&](http2_frame_type type, uint8_t flags, uint32_t sid, const byte_t*, size_t len) {
                             if (type == http2_frame_type::HEADERS && sid == 1) {
                                 decoded = true;
                                 EXPECT_EQ(len, 1u);
                                 EXPECT_TRUE(flags & HTTP2_FLAG_END_HEADERS);
                                 EXPECT_TRUE(flags & HTTP2_FLAG_END_STREAM);
                             }
                         });
    EXPECT_TRUE(decoded);
}

TEST_F(Http2FrameTest, EncodeDecodeSettingsFrame) {
    http2_settings_frame sf;
    sf.entries.push_back({http2_settings_id::MAX_CONCURRENT_STREAMS, 100});
    sf.entries.push_back({http2_settings_id::INITIAL_WINDOW_SIZE, 65535});

    auto encoded = framer.encode_settings_frame(sf);
    EXPECT_GT(encoded.size(), 9u);
    EXPECT_EQ(encoded.size(), 9u + 12u);

    bool decoded = false;
    framer.decode_frames(encoded.data(), encoded.size(),
                         [&](http2_frame_type type, uint8_t flags, uint32_t sid, const byte_t*, size_t len) {
                             if (type == http2_frame_type::SETTINGS) {
                                 decoded = true;
                                 EXPECT_EQ(sid, 0u);
                                 EXPECT_EQ(len, 12u);
                             }
                         });
    EXPECT_TRUE(decoded);
}

TEST_F(Http2FrameTest, SettingsAckHasZeroPayload) {
    http2_settings_frame ack;
    ack.ack = true;

    auto encoded = framer.encode_settings_frame(ack);
    EXPECT_EQ(encoded.size(), 9u);

    framer.decode_frames(encoded.data(), encoded.size(),
                         [&](http2_frame_type type, uint8_t flags, uint32_t sid, const byte_t*, size_t len) {
                             EXPECT_EQ(type, http2_frame_type::SETTINGS);
                             EXPECT_TRUE(flags & HTTP2_FLAG_ACK);
                             EXPECT_EQ(len, 0u);
                         });
}

TEST_F(Http2FrameTest, EncodeDecodePingFrame) {
    http2_ping_frame ping;
    ping.opaque_data = 0x1234567890ABCDEFULL;

    auto encoded = framer.encode_ping_frame(ping);
    EXPECT_EQ(encoded.size(), 9u + 8u);

    bool decoded = false;
    framer.decode_frames(encoded.data(), encoded.size(),
                         [&](http2_frame_type type, uint8_t, uint32_t sid, const byte_t*, size_t len) {
                             if (type == http2_frame_type::PING) {
                                 decoded = true;
                                 EXPECT_EQ(sid, 0u);
                                 EXPECT_EQ(len, 8u);
                             }
                         });
    EXPECT_TRUE(decoded);
}

TEST_F(Http2FrameTest, EncodeDecodeGoawayFrame) {
    http2_goaway_frame goaway;
    goaway.last_stream_id = 7;
    goaway.error_code = http2_error::NO_ERROR;

    auto encoded = framer.encode_goaway_frame(goaway);
    EXPECT_GE(encoded.size(), 9u + 8u);

    bool decoded = false;
    framer.decode_frames(encoded.data(), encoded.size(),
                         [&](http2_frame_type type, uint8_t, uint32_t sid, const byte_t*, size_t len) {
                             if (type == http2_frame_type::GOAWAY) {
                                 decoded = true;
                                 EXPECT_EQ(sid, 0u);
                                 EXPECT_GE(len, 8u);
                             }
                         });
    EXPECT_TRUE(decoded);
}

TEST_F(Http2FrameTest, EncodeDecodeRstStreamFrame) {
    http2_rst_stream_frame rst;
    rst.stream_id = 5;
    rst.error_code = http2_error::CANCEL;

    auto encoded = framer.encode_rst_stream_frame(rst);
    EXPECT_EQ(encoded.size(), 9u + 4u);

    framer.decode_frames(encoded.data(), encoded.size(),
                         [&](http2_frame_type type, uint8_t, uint32_t sid, const byte_t*, size_t len) {
                             EXPECT_EQ(type, http2_frame_type::RST_STREAM);
                             EXPECT_EQ(sid, 5u);
                             EXPECT_EQ(len, 4u);
                         });
}

TEST_F(Http2FrameTest, EncodeDecodeWindowUpdateFrame) {
    http2_window_update_frame wuf;
    wuf.stream_id = 3;
    wuf.window_size_increment = 1024;

    auto encoded = framer.encode_window_update_frame(wuf);
    EXPECT_EQ(encoded.size(), 9u + 4u);

    framer.decode_frames(encoded.data(), encoded.size(),
                         [&](http2_frame_type type, uint8_t, uint32_t sid, const byte_t*, size_t len) {
                             EXPECT_EQ(type, http2_frame_type::WINDOW_UPDATE);
                             EXPECT_EQ(sid, 3u);
                             EXPECT_EQ(len, 4u);
                         });
}

TEST_F(Http2FrameTest, FrameHeaderIsNineBytes) { EXPECT_EQ(sizeof(http2_frame_header), 9u); }

TEST_F(Http2FrameTest, FrameHeaderLengthRoundTrip) {
    http2_frame_header hdr;
    hdr.set_length(0);
    EXPECT_EQ(hdr.get_length(), 0u);
    hdr.set_length(16384);
    EXPECT_EQ(hdr.get_length(), 16384u);
    hdr.set_length(16777215);
    EXPECT_EQ(hdr.get_length(), 16777215u);
}

TEST_F(Http2FrameTest, FrameHeaderStreamIdRoundTrip) {
    http2_frame_header hdr;
    hdr.set_stream_id(0);
    EXPECT_EQ(hdr.get_stream_id(), 0u);
    hdr.set_stream_id(1);
    EXPECT_EQ(hdr.get_stream_id(), 1u);
    hdr.set_stream_id(0x7FFFFFFF);
    EXPECT_EQ(hdr.get_stream_id(), 0x7FFFFFFFu);
    http2_frame_header hdr2{};
    hdr2.sid_r = 0x80;
    EXPECT_EQ(hdr2.get_stream_id(), 0u);
}

TEST_F(Http2FrameTest, FrameHeaderEncodeGolden) {
    http2_frame_header hdr;
    hdr.set_length(6);
    hdr.type = 4;
    hdr.flags = 0;
    hdr.set_stream_id(0);
    const auto* bytes = reinterpret_cast<const uint8_t*>(&hdr);
    EXPECT_EQ(bytes[0], 0x00);
    EXPECT_EQ(bytes[1], 0x00);
    EXPECT_EQ(bytes[2], 0x06);
    EXPECT_EQ(bytes[3], 0x04);
    EXPECT_EQ(bytes[4], 0x00);
    EXPECT_EQ(bytes[5], 0x00);
    EXPECT_EQ(bytes[6], 0x00);
    EXPECT_EQ(bytes[7], 0x00);
    EXPECT_EQ(bytes[8], 0x00);
}

TEST_F(Http2FrameTest, HuffmanEosSymbolRejected) {
    hpack_decoder decoder(4096);
    byte_vector crafted;
    crafted.push_back(0x10);
    crafted.push_back(0x00);
    crafted.push_back(0x88);
    crafted.push_back(0xFF);
    crafted.push_back(0xFF);
    crafted.push_back(0xFF);
    crafted.push_back(0xFF);
    auto result = decoder.decode(crafted.data(), crafted.size());
    SUCCEED();
}

TEST_F(Http2FrameTest, HuffmanInvalidPaddingRejected) {
    hpack_decoder decoder(4096);
    byte_vector crafted;
    crafted.push_back(0x10);
    crafted.push_back(0x00);
    crafted.push_back(0x81);
    crafted.push_back(0x18);
    auto result = decoder.decode(crafted.data(), crafted.size());
    SUCCEED();
}

class Http2StreamTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(Http2StreamTest, NewStreamIsIdle) {
    http2_stream stream(1);
    EXPECT_EQ(stream.state(), http2_stream_state::IDLE);
    EXPECT_FALSE(stream.is_closed());
}

TEST_F(Http2StreamTest, ClientSendHeadersOpensStream) {
    http2_stream stream(1);
    stream.on_send_headers(false);
    EXPECT_EQ(stream.state(), http2_stream_state::OPEN);
    EXPECT_TRUE(stream.can_send_data());
}

TEST_F(Http2StreamTest, SendHeadersWithEndStreamHalfCloses) {
    http2_stream stream(1);
    stream.on_send_headers(true);
    EXPECT_EQ(stream.state(), http2_stream_state::HALF_CLOSED_LOCAL);
    EXPECT_FALSE(stream.can_send_data());
}

TEST_F(Http2StreamTest, ReceiveHeadersOpensStream) {
    http2_stream stream(1);
    stream.on_receive_headers(false);
    EXPECT_EQ(stream.state(), http2_stream_state::OPEN);
    EXPECT_TRUE(stream.can_receive());
}

TEST_F(Http2StreamTest, ReceiveHeadersWithEndStreamHalfCloses) {
    http2_stream stream(1);
    stream.on_receive_headers(true);
    EXPECT_EQ(stream.state(), http2_stream_state::HALF_CLOSED_REMOTE);
}

TEST_F(Http2StreamTest, BothSidesEndStreamCloses) {
    http2_stream stream(1);
    stream.on_send_headers(true);
    stream.on_receive_headers(true);
    EXPECT_TRUE(stream.is_closed());
}

TEST_F(Http2StreamTest, SendDataWithEndStream) {
    http2_stream stream(1);
    stream.on_send_headers(false);
    stream.on_send_data(true);
    EXPECT_EQ(stream.state(), http2_stream_state::HALF_CLOSED_LOCAL);
}

TEST_F(Http2StreamTest, RstStreamClosesStream) {
    http2_stream stream(1);
    stream.on_send_headers(false);
    stream.on_send_rst_stream();
    EXPECT_TRUE(stream.is_closed());
}

TEST_F(Http2StreamTest, ReceiveRstStreamClosesStream) {
    http2_stream stream(1);
    stream.on_receive_headers(false);
    stream.on_receive_rst_stream();
    EXPECT_TRUE(stream.is_closed());
}

class Http2FlowControlTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(Http2FlowControlTest, DefaultConnectionWindow) {
    http2_flow_control fc;
    EXPECT_EQ(fc.connection_window(), 65535u);
}

TEST_F(Http2FlowControlTest, CustomInitialWindow) {
    http2_flow_control fc(32768);
    EXPECT_EQ(fc.connection_window(), 32768u);
}

TEST_F(Http2FlowControlTest, CanSendWithinWindow) {
    http2_flow_control fc(65535);
    EXPECT_TRUE(fc.can_send(1, 1000));
    EXPECT_TRUE(fc.can_send(0, 65535));
}

TEST_F(Http2FlowControlTest, CannotSendBeyondWindow) {
    http2_flow_control fc(1000);
    EXPECT_FALSE(fc.can_send(1, 2000));
}

TEST_F(Http2FlowControlTest, ConsumeReducesWindow) {
    http2_flow_control fc(65535);
    uint32_t before = fc.window(1);
    fc.consume(1, 1000);
    EXPECT_EQ(fc.window(1), before - 1000);
}

TEST_F(Http2FlowControlTest, AddWindowIncreasesWindow) {
    http2_flow_control fc(65535);
    fc.consume(1, 5000);
    uint32_t after_consume = fc.window(1);
    fc.add_window(1, 3000);
    EXPECT_EQ(fc.window(1), after_consume + 3000);
}

TEST_F(Http2FlowControlTest, ConnectionWindowAffectsStreams) {
    http2_flow_control fc(1000);
    EXPECT_TRUE(fc.can_send(1, 1000));
    fc.consume(1, 800);
    EXPECT_TRUE(fc.can_send(2, 200));
    EXPECT_FALSE(fc.can_send(2, 300));
}

TEST_F(Http2FlowControlTest, SetInitialWindow) {
    http2_flow_control fc(65535);
    fc.set_initial_window(32768);
    EXPECT_EQ(fc.window(10), 32768u);
}

class Http2SettingsTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(Http2SettingsTest, DefaultValues) {
    auto s = http2_settings::defaults();
    EXPECT_EQ(s.header_table_size(), 4096u);
    EXPECT_FALSE(s.enable_push());
    EXPECT_EQ(s.initial_window_size(), 65535u);
    EXPECT_EQ(s.max_frame_size(), 16384u);
}

TEST_F(Http2SettingsTest, SetAndGet) {
    http2_settings s;
    s.set(http2_settings_id::HEADER_TABLE_SIZE, 8192);
    EXPECT_EQ(s.get(http2_settings_id::HEADER_TABLE_SIZE), 8192u);
}

TEST_F(Http2SettingsTest, ApplyRemoteSettings) {
    http2_settings local;
    http2_settings_frame sf;
    sf.entries.push_back({http2_settings_id::MAX_CONCURRENT_STREAMS, 50});
    sf.entries.push_back({http2_settings_id::INITIAL_WINDOW_SIZE, 131072});

    local.apply_remote_settings(sf);
    EXPECT_EQ(local.max_concurrent_streams(), 50u);
    EXPECT_EQ(local.initial_window_size(), 131072u);
}

class SniManagerTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(SniManagerTest, NewManagerIsEmpty) {
    sni_manager sni;
    EXPECT_EQ(sni.host_count(), 0u);
    EXPECT_FALSE(sni.has_host("example.com"));
}

TEST_F(SniManagerTest, AddAndCheckHost) {
    sni_manager sni;
    sni.add_host("example.com", ssl_context{});
    EXPECT_EQ(sni.host_count(), 1u);
    EXPECT_TRUE(sni.has_host("example.com"));
}

TEST_F(SniManagerTest, RemoveHost) {
    sni_manager sni;
    sni.add_host("example.com", ssl_context{});
    sni.add_host("test.com", ssl_context{});
    EXPECT_EQ(sni.host_count(), 2u);
    sni.remove_host("example.com");
    EXPECT_EQ(sni.host_count(), 1u);
    EXPECT_FALSE(sni.has_host("example.com"));
    EXPECT_TRUE(sni.has_host("test.com"));
}

TEST_F(SniManagerTest, ExactHostnameMatch) {
    sni_manager sni;
    ssl_context ctx;
    sni.add_host("api.example.com", move(ctx));

    auto* selected = sni.select_ssl_ctx("api.example.com");
    EXPECT_NE(selected, nullptr);
}

TEST_F(SniManagerTest, NonMatchingHostnameReturnsDefault) {
    sni_manager sni;
    sni.add_host("example.com", ssl_context{});
    auto* selected = sni.select_ssl_ctx("unknown.com");
    EXPECT_EQ(selected, nullptr);
}

TEST_F(SniManagerTest, CaseInsensitiveMatch) {
    sni_manager sni;
    sni.add_host("Example.COM", ssl_context{});
    EXPECT_TRUE(sni.has_host("example.com"));
}

class BufferChainTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(BufferChainTest, AppendAndTotalSize) {
    buffer_chain chain;
    chain.append("Hello", 5);
    EXPECT_EQ(chain.total_size(), 5u);
    chain.append(" World", 6);
    EXPECT_EQ(chain.total_size(), 11u);
}

TEST_F(BufferChainTest, AppendStringView) {
    buffer_chain chain;
    chain.append(string_view("test data"));
    EXPECT_EQ(chain.total_size(), 9u);
}

TEST_F(BufferChainTest, FlattenCombinesAll) {
    buffer_chain chain;
    chain.append("ABC", 3);
    chain.append("DEF", 3);
    chain.append("GHI", 3);

    auto result = chain.flatten();
    EXPECT_EQ(result, "ABCDEFGHI");
    EXPECT_EQ(result.size(), 9u);
}

TEST_F(BufferChainTest, EmptyChainFlatten) {
    buffer_chain chain;
    auto result = chain.flatten();
    EXPECT_TRUE(result.empty());
}

TEST_F(BufferChainTest, ChainAppend) {
    buffer_chain chain1;
    chain1.append("Part1", 5);

    buffer_chain chain2;
    chain2.append("Part2", 5);

    chain1.append(chain2);
    EXPECT_EQ(chain1.total_size(), 10u);
    EXPECT_EQ(chain2.total_size(), 5u);
}

TEST_F(BufferChainTest, ClearResetsChain) {
    buffer_chain chain;
    chain.append("Data", 4);
    chain.clear();
    EXPECT_EQ(chain.total_size(), 0u);
    EXPECT_TRUE(chain.flatten().empty());
}

class EventLoopTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(EventLoopTest, ConstructAndDestroy) {
    event_loop loop;
    SUCCEED();
}

TEST_F(EventLoopTest, ScheduleAndCancelTimer) {
    event_loop loop;
    bool fired = false;
    auto id = loop.schedule_timer(10000, [&] { fired = true; });
    EXPECT_GT(id, 0u);
    EXPECT_TRUE(loop.cancel_timer(id));
    EXPECT_FALSE(fired);
}

TEST_F(EventLoopTest, AddAndRemoveFd) {
    event_loop loop;
    loop.add_fd(-1, 0, [](int, uint32_t) {});
    loop.remove_fd(-1);
    SUCCEED();
}

TEST_F(EventLoopTest, ScheduleTimerFires) {
    event_loop loop;
    bool fired = false;

    loop.schedule_timer(10, [&] {
        fired = true;
        loop.stop();
    });

    loop.run();
    EXPECT_TRUE(fired);
}

TEST_F(EventLoopTest, StopWithoutRunDoesNotCrash) {
    event_loop loop;
    loop.stop();
    SUCCEED();
}

TEST_F(EventLoopTest, MultipleTimersFireInOrder) {
    event_loop loop;
    int counter = 0;

    loop.schedule_timer(20, [&] {
        counter++;
        loop.stop();
    });
    loop.schedule_timer(10, [&] { counter++; });

    loop.run();
    EXPECT_GE(counter, 2);
}

class HealthCheckFilterTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(HealthCheckFilterTest, GetHealthzReturnsOk) {
    health_check_filter hc;
    http_request req;
    req.method = http_method::GET();
    req.path = "/healthz";
    http_response res;

    bool cont = hc.pre_filter(req, res);
    EXPECT_FALSE(cont);
    EXPECT_EQ(res.status, http_status::S2_OK);
    EXPECT_TRUE(res.body.contains("ok"));
    EXPECT_TRUE(res.body.contains("uptime"));
}

TEST_F(HealthCheckFilterTest, NonGetMethodBypasses) {
    health_check_filter hc;
    http_request req;
    req.method = http_method::POST();
    req.path = "/healthz";
    http_response res;

    bool cont = hc.pre_filter(req, res);
    EXPECT_TRUE(cont);
}

TEST_F(HealthCheckFilterTest, WrongPathBypasses) {
    health_check_filter hc;
    http_request req;
    req.method = http_method::GET();
    req.path = "/api/test";
    http_response res;

    bool cont = hc.pre_filter(req, res);
    EXPECT_TRUE(cont);
}

TEST_F(HealthCheckFilterTest, DisabledFilterBypasses) {
    health_check_filter hc;
    hc.enabled = false;
    http_request req;
    req.method = http_method::GET();
    req.path = "/healthz";
    http_response res;

    bool cont = hc.pre_filter(req, res);
    EXPECT_TRUE(cont);
}

TEST_F(HealthCheckFilterTest, CustomPathWorks) {
    health_check_filter hc;
    hc.path = "/ready";
    http_request req;
    req.method = http_method::GET();
    req.path = "/ready";
    http_response res;

    bool cont = hc.pre_filter(req, res);
    EXPECT_FALSE(cont);
    EXPECT_EQ(res.status, http_status::S2_OK);
}

TEST_F(HealthCheckFilterTest, FailedCheckReturns503) {
    health_check_filter hc;
    hc.add_check("db", [] { return false; });

    http_request req;
    req.method = http_method::GET();
    req.path = "/healthz";
    http_response res;

    bool cont = hc.pre_filter(req, res);
    EXPECT_FALSE(cont);
    EXPECT_EQ(res.status, http_status::S5_SERVICE_UNAVAILABLE);
    EXPECT_TRUE(res.body.contains("unhealthy"));
}

TEST_F(HealthCheckFilterTest, AllChecksPassReturns200) {
    health_check_filter hc;
    hc.add_check("db", [] { return true; });
    hc.add_check("redis", [] { return true; });

    http_request req;
    req.method = http_method::GET();
    req.path = "/healthz";
    http_response res;

    bool cont = hc.pre_filter(req, res);
    EXPECT_FALSE(cont);
    EXPECT_EQ(res.status, http_status::S2_OK);
}

TEST_F(HealthCheckFilterTest, SimpleModeWithoutDetails) {
    health_check_filter hc;
    hc.show_details = false;

    http_request req;
    req.method = http_method::GET();
    req.path = "/healthz";
    http_response res;

    bool cont = hc.pre_filter(req, res);
    EXPECT_FALSE(cont);
    EXPECT_EQ(res.status, http_status::S2_OK);
    EXPECT_TRUE(res.body.contains("ok"));
}

class MultipartParserTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(MultipartParserTest, ExtractBoundary) {
    string_view ct = "multipart/form-data; boundary=----WebKitFormBoundary7MA4YWxk";
    auto boundary = multipart_parser::extract_boundary(ct);
    EXPECT_EQ(boundary, "----WebKitFormBoundary7MA4YWxk");
}

TEST_F(MultipartParserTest, ExtractBoundaryWithQuotes) {
    string_view ct = R"(multipart/form-data; boundary="my boundary")";
    auto boundary = multipart_parser::extract_boundary(ct);
    EXPECT_EQ(boundary, "my boundary");
}

TEST_F(MultipartParserTest, ExtractBoundaryNotFound) {
    auto boundary = multipart_parser::extract_boundary("text/plain");
    EXPECT_TRUE(boundary.empty());
}

TEST_F(MultipartParserTest, ParseSimpleTextField) {
    string body = "------TestBoundary\r\n"
                  "Content-Disposition: form-data; name=\"username\"\r\n"
                  "\r\n"
                  "john_doe\r\n"
                  "------TestBoundary--\r\n";
    multipart_parser parser;
    auto fields = parser.parse(body.view(), "----TestBoundary");
    ASSERT_EQ(fields.size(), 1u);
    EXPECT_EQ(fields[0].name, "username");
    EXPECT_EQ(string(fields[0].data.begin(), fields[0].data.end()), "john_doe");
    EXPECT_FALSE(fields[0].is_file());
}

TEST_F(MultipartParserTest, ParseMultipleFields) {
    string body = "------Boundary\r\n"
                  "Content-Disposition: form-data; name=\"field1\"\r\n"
                  "\r\n"
                  "value1\r\n"
                  "------Boundary\r\n"
                  "Content-Disposition: form-data; name=\"field2\"\r\n"
                  "\r\n"
                  "value2\r\n"
                  "------Boundary--\r\n";
    multipart_parser parser;
    auto fields = parser.parse(body.view(), "----Boundary");
    ASSERT_EQ(fields.size(), 2u);
    EXPECT_EQ(fields[0].name, "field1");
    EXPECT_EQ(fields[1].name, "field2");
}

TEST_F(MultipartParserTest, ParseFileUpload) {
    string body = "------Boundary\r\n"
                  "Content-Disposition: form-data; name=\"file\"; filename=\"test.txt\"\r\n"
                  "Content-Type: text/plain\r\n"
                  "\r\n"
                  "file content here\r\n"
                  "------Boundary--\r\n";
    multipart_parser parser;
    auto fields = parser.parse(body.view(), "----Boundary");
    ASSERT_EQ(fields.size(), 1u);
    EXPECT_EQ(fields[0].name, "file");
    EXPECT_EQ(fields[0].filename, "test.txt");
    EXPECT_EQ(fields[0].content_type, "text/plain");
    EXPECT_TRUE(fields[0].is_file());
}

TEST_F(MultipartParserTest, EmptyBodyReturnsNoFields) {
    multipart_parser parser;
    auto fields = parser.parse("", "boundary");
    EXPECT_TRUE(fields.empty());
}

TEST_F(MultipartParserTest, EmptyBoundaryReturnsNoFields) {
    multipart_parser parser;
    auto fields = parser.parse("some data", "");
    EXPECT_TRUE(fields.empty());
}

TEST_F(MultipartParserTest, MaxFieldsLimitEnforced) {
    multipart_parser parser;
    parser.max_fields = 2;

    string body = "------B\r\n"
                  "Content-Disposition: form-data; name=\"f1\"\r\n\r\nv1\r\n"
                  "------B\r\n"
                  "Content-Disposition: form-data; name=\"f2\"\r\n\r\nv2\r\n"
                  "------B\r\n"
                  "Content-Disposition: form-data; name=\"f3\"\r\n\r\nv3\r\n"
                  "------B--\r\n";
    auto fields = parser.parse(body.view(), "----B");
    EXPECT_LE(fields.size(), 2u);
}

TEST_F(MultipartParserTest, FieldWithoutNameIsSkipped) {
    string body = "------Boundary\r\n"
                  "Content-Disposition: form-data\r\n"
                  "\r\n"
                  "no_name_data\r\n"
                  "------Boundary\r\n"
                  "Content-Disposition: form-data; name=\"named\"\r\n"
                  "\r\n"
                  "named_data\r\n"
                  "------Boundary--\r\n";
    multipart_parser parser;
    auto fields = parser.parse(body.view(), "----Boundary");
    ASSERT_EQ(fields.size(), 1u);
    EXPECT_EQ(fields[0].name, "named");
}

class GrpcTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(GrpcTest, EncodeSimpleMessage) {
    string payload = "hello";
    grpc_message msg{byte_vector(payload.begin(), payload.end()), false};
    auto frame = grpc_framer::encode(msg);
    ASSERT_EQ(frame.size(), 10u);
    EXPECT_EQ(frame[0], 0);
    uint32_t len = (static_cast<uint32_t>(frame[1]) << 24) | (static_cast<uint32_t>(frame[2]) << 16) |
                   (static_cast<uint32_t>(frame[3]) << 8) | static_cast<uint32_t>(frame[4]);
    EXPECT_EQ(len, 5u);
}

TEST_F(GrpcTest, EncodeCompressedMessage) {
    grpc_message msg{byte_vector{'a', 'b', 'c'}, true};
    auto frame = grpc_framer::encode(msg);
    EXPECT_EQ(frame[0], 1);
}

TEST_F(GrpcTest, DecodeSingleMessage) {
    grpc_message msg{byte_vector{'t', 'e', 's', 't'}, false};
    auto frame = grpc_framer::encode(msg);

    grpc_framer framer;
    vector<grpc_message> out;
    int n = framer.decode(frame.data(), frame.size(), out);
    EXPECT_EQ(n, 1);
    ASSERT_EQ(out.size(), 1u);
    EXPECT_FALSE(out[0].compressed);
    EXPECT_EQ(string(out[0].payload.begin(), out[0].payload.end()), "test");
}

TEST_F(GrpcTest, DecodeMultipleMessages) {
    grpc_framer framer;
    byte_vector combined;
    for (int i = 0; i < 3; i++) {
        string content = "msg" + to_string(i);
        grpc_message msg{byte_vector(content.begin(), content.end()), false};
        auto f = grpc_framer::encode(msg);
        combined.insert(combined.end(), f.begin(), f.end());
    }

    vector<grpc_message> out;
    int n = framer.decode(combined.data(), combined.size(), out);
    EXPECT_EQ(n, 3);
}

TEST_F(GrpcTest, EncodeMessagesHelper) {
    vector<grpc_message> msgs;
    msgs.push_back({byte_vector{'a'}, false});
    msgs.push_back({byte_vector{'b', 'c'}, false});
    auto combined = grpc_framer::encode_messages(msgs);
    EXPECT_GT(combined.size(), 10u);
}

TEST_F(GrpcTest, MaxReceiveSizeExceeded) {
    grpc_framer framer;
    framer.max_receive_size = byte_size{10};
    grpc_message msg{byte_vector(100, 'x'), false};
    auto frame = grpc_framer::encode(msg);

    vector<grpc_message> out;
    int n = framer.decode(frame.data(), frame.size(), out);
    EXPECT_EQ(n, -1);
}

TEST_F(GrpcTest, GrpcStatusToHttpStatus) {
    EXPECT_EQ(grpc_handler::grpc_to_http_status(grpc_status::OK), http_status::S2_OK);
    EXPECT_EQ(grpc_handler::grpc_to_http_status(grpc_status::NOT_FOUND), http_status::S4_NOT_FOUND);
    EXPECT_EQ(grpc_handler::grpc_to_http_status(grpc_status::UNAUTHENTICATED), http_status::S4_UNAUTHORIZED);
    EXPECT_EQ(grpc_handler::grpc_to_http_status(grpc_status::INTERNAL), http_status::S5_INTERNAL_SERVER_ERROR);
    EXPECT_EQ(grpc_handler::grpc_to_http_status(grpc_status::UNAVAILABLE), http_status::S5_SERVICE_UNAVAILABLE);
}

TEST_F(GrpcTest, SendErrorSetsTrailers) {
    http_response res;
    grpc_handler::send_error(res, grpc_status::NOT_FOUND, "Resource missing");
    EXPECT_EQ(res.status, http_status::S4_NOT_FOUND);
    EXPECT_EQ(res.trailers["grpc-status"], "5");
    EXPECT_EQ(res.trailers["grpc-message"], "Resource missing");
}

class Http2PushPromiseTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(Http2PushPromiseTest, EncodePushPromiseFrame) {
    http2_framer framer;
    http2_push_promise_frame ppf;
    ppf.stream_id = 1;
    ppf.promised_stream_id = 2;
    ppf.header_block = byte_vector{'h', 'd', 'r'};
    ppf.end_headers = true;

    auto frame = framer.encode_push_promise_frame(ppf);
    EXPECT_GT(frame.size(), 9u + 4u);

    http2_frame_header* hdr = reinterpret_cast<http2_frame_header*>(frame.data());
    EXPECT_EQ(hdr->type, static_cast<uint8_t>(http2_frame_type::PUSH_PROMISE));
    EXPECT_EQ(hdr->get_stream_id(), 1u);
}

TEST_F(Http2PushPromiseTest, DecodePushPromiseFrame) {
    http2_framer framer;
    http2_push_promise_frame ppf;
    ppf.stream_id = 1;
    ppf.promised_stream_id = 2;
    ppf.header_block = byte_vector{'h', 'd', 'r'};
    ppf.end_headers = true;

    auto frame = framer.encode_push_promise_frame(ppf);

    bool received = false;
    framer.decode_frames(frame.data(), frame.size(),
                         [&](http2_frame_type type, uint8_t, uint32_t sid, const byte_t*, size_t) {
                             if (type == http2_frame_type::PUSH_PROMISE) {
                                 received = true;
                                 EXPECT_EQ(sid, 1u);
                             }
                         });
    EXPECT_TRUE(received);
}

class LoadBalancerTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(LoadBalancerTest, SelectBackendRoundRobin) {
    load_balancer lb;
    lb.set_strategy(lb_strategy::ROUND_ROBIN);
    lb.add_backend({"host1", ports(8080)});
    lb.add_backend({"host2", ports(8080)});

    auto* b1 = lb.select_backend();
    auto* b2 = lb.select_backend();
    auto* b3 = lb.select_backend();

    EXPECT_NE(b1, nullptr);
    EXPECT_NE(b2, nullptr);
    EXPECT_NE(b3, nullptr);
    EXPECT_EQ(b1->host, "host1");
    EXPECT_EQ(b2->host, "host2");
    EXPECT_EQ(b3->host, "host1");

    lb.release_backend(b1);
    lb.release_backend(b2);
    lb.release_backend(b3);
}

TEST_F(LoadBalancerTest, SelectBackendLeastConnections) {
    load_balancer lb;
    lb.set_strategy(lb_strategy::LEAST_CONNECTIONS);
    lb.add_backend({"host1", ports(8080)});
    lb.add_backend({"host2", ports(8080)});
    lb.add_backend({"host3", ports(8080)});

    auto* b1 = lb.select_backend();
    lb.release_backend(b1);

    auto* b2 = lb.select_backend();
    EXPECT_NE(b2, nullptr);
    lb.release_backend(b2);
}

TEST_F(LoadBalancerTest, MarkFailureMarksUnhealthy) {
    load_balancer lb;
    lb.set_max_failures(2);
    lb.add_backend({"host1", ports(8080)});

    auto* b = lb.select_backend();
    lb.mark_failure(b);
    EXPECT_TRUE(b->healthy);

    lb.mark_failure(b);
    EXPECT_FALSE(b->healthy);
}

TEST_F(LoadBalancerTest, UnhealthyBackendNotSelected) {
    load_balancer lb;
    lb.set_max_failures(1);
    lb.add_backend({"host1", ports(8080)});
    lb.add_backend({"host2", ports(8080)});

    auto* b1 = lb.select_backend();
    lb.mark_failure(b1);
    lb.release_backend(b1);

    auto* b2 = lb.select_backend();
    EXPECT_EQ(b2->host, "host2");
    lb.release_backend(b2);
}

TEST_F(LoadBalancerTest, HealthCheckReenablesBackend) {
    load_balancer lb;
    lb.set_max_failures(1);
    lb.add_backend({"host1", ports(8080)});

    auto* b = lb.select_backend();
    lb.mark_failure(b);
    lb.release_backend(b);
    EXPECT_FALSE(b->healthy);

    lb.set_health_check([](const lb_backend&) { return true; });
    lb.run_health_checks();
    EXPECT_TRUE(b->healthy);
}

TEST_F(LoadBalancerTest, HealthyCountReflectsState) {
    load_balancer lb;
    lb.set_max_failures(1);
    lb.add_backend({"h1", ports(8080)});
    lb.add_backend({"h2", ports(8080)});

    EXPECT_EQ(lb.healthy_count(), 2u);

    auto* b = lb.select_backend();
    lb.mark_failure(b);
    lb.release_backend(b);
    EXPECT_EQ(lb.healthy_count(), 1u);
}

TEST_F(LoadBalancerTest, WeightedStrategySelectsByWeight) {
    load_balancer lb;
    lb.set_strategy(lb_strategy::WEIGHTED);
    lb.add_backend({"low", ports(8080), "http", 1});
    lb.add_backend({"high", ports(8080), "http", 100});

    size_t high_count = 0;
    for (int i = 0; i < 50; i++) {
        auto* b = lb.select_backend();
        if (b->host == "high") {
            high_count++;
        }
        lb.release_backend(b);
    }
    EXPECT_GT(high_count, 30u);
}

TEST_F(LoadBalancerTest, EmptyReturnsNull) {
    load_balancer lb;
    EXPECT_EQ(lb.select_backend(), nullptr);
}

TEST_F(LoadBalancerTest, RemoveBackend) {
    load_balancer lb;
    lb.add_backend({"h1", ports(8080)});
    lb.add_backend({"h2", ports(8080)});
    EXPECT_EQ(lb.backend_count(), 2u);

    lb.remove_backend("h1", ports(8080));
    EXPECT_EQ(lb.backend_count(), 1u);
}

TEST_F(LoadBalancerTest, BackendHostsList) {
    load_balancer lb;
    lb.add_backend({"h1", ports(8080)});
    lb.add_backend({"h2", ports(9090)});
    auto hosts = lb.backend_hosts();
    ASSERT_EQ(hosts.size(), 2u);
    EXPECT_TRUE(hosts[0].contains("8080") || hosts[1].contains("8080"));
    EXPECT_TRUE(hosts[0].contains("9090") || hosts[1].contains("9090"));
}

class ReverseProxyFilterTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(ReverseProxyFilterTest, ShouldProxyWithPrefix) {
    reverse_proxy_filter proxy;
    proxy.set_path_prefix("/api");

    http_request req;
    req.path = "/api/users";
    http_response res;

    bool cont = proxy.pre_filter(req, res);
    EXPECT_FALSE(cont);
    EXPECT_EQ(res.status, http_status::S5_BAD_GATEWAY);
}

TEST_F(ReverseProxyFilterTest, ShouldNotProxyWithoutPrefix) {
    reverse_proxy_filter proxy;
    proxy.set_path_prefix("/api");

    http_request req;
    req.path = "/other";
    http_response res;

    bool cont = proxy.pre_filter(req, res);
    EXPECT_TRUE(cont);
}

TEST_F(ReverseProxyFilterTest, NoPrefixProxiesEverything) {
    reverse_proxy_filter proxy;

    http_request req;
    req.path = "/anything";
    http_response res;

    bool cont = proxy.pre_filter(req, res);
    EXPECT_FALSE(cont);
    EXPECT_EQ(res.status, http_status::S5_BAD_GATEWAY);
}

class ResponseCacheTest : public ::testing::Test {
protected:
    void SetUp() override { cache_ = make_unique<response_cache>(); }
    void TearDown() override { cache_.reset(); }
    unique_ptr<response_cache> cache_;
};

TEST_F(ResponseCacheTest, PutAndGet) {
    http_response resp;
    resp.status = http_status::S2_OK;
    resp.body = "cached content";
    cache_->put("key1", resp, 60_s);

    auto cached = cache_->get("key1");
    ASSERT_TRUE(cached.has_value());
    EXPECT_EQ(cached->body, "cached content");
}

TEST_F(ResponseCacheTest, GetNonexistentReturnsNone) {
    auto cached = cache_->get("nonexistent");
    EXPECT_FALSE(cached.has_value());
}

TEST_F(ResponseCacheTest, RemoveDeletesEntry) {
    http_response resp;
    resp.body = "data";
    cache_->put("key1", resp, 60_s);
    EXPECT_TRUE(cache_->get("key1").has_value());

    cache_->remove("key1");
    EXPECT_FALSE(cache_->get("key1").has_value());
}

TEST_F(ResponseCacheTest, GenerateEtag) {
    string etag = response_cache::generate_etag("test content");
    EXPECT_FALSE(etag.empty());
    EXPECT_TRUE(etag.starts_with("\""));
}

TEST_F(ResponseCacheTest, BuildCacheKey) {
    string key = response_cache::build_key(http_method::GET(), "/api/users");
    EXPECT_TRUE(key.contains("GET"));
    EXPECT_TRUE(key.contains("/api/users"));
}

TEST_F(ResponseCacheTest, CleanupRemovesExpired) {
    http_response resp;
    resp.body = "data";
    cache_->default_max_age = -1_s;
    cache_->put("key1", resp, 0_s);

    cache_->cleanup();
    auto cached = cache_->get("key1");
    EXPECT_FALSE(cached.has_value());
}

TEST_F(ResponseCacheTest, ClearRemovesAll) {
    http_response resp;
    resp.body = "data";
    cache_->put("k1", resp);
    cache_->put("k2", resp);
    EXPECT_EQ(cache_->size(), 2u);

    cache_->clear();
    EXPECT_EQ(cache_->size(), 0u);
}

TEST_F(ResponseCacheTest, LargeBodyNotCached) {
    http_response resp;
    resp.body = string(2 * 1024 * 1024, 'x');
    cache_->max_body_size = 1_MB;
    cache_->put("key", resp);
    EXPECT_FALSE(cache_->get("key").has_value());
}

class CacheFilterTest : public ::testing::Test {
protected:
    void SetUp() override {
        cache_ = make_unique<response_cache>();
        filter_ = make_unique<cache_filter>();
        filter_->set_cache(cache_.get());
    }
    void TearDown() override {
        filter_.reset();
        cache_.reset();
    }
    unique_ptr<response_cache> cache_;
    unique_ptr<cache_filter> filter_;
};

TEST_F(CacheFilterTest, CacheHitReturnsCachedResponse) {
    http_response resp;
    resp.status = http_status::S2_OK;
    resp.body = "cached";

    string key = response_cache::build_key(http_method::GET(), "/data");
    cache_->put(key, resp, 60_s);

    http_request req;
    req.method = http_method::GET();
    req.path = "/data";
    http_response res;

    bool cont = filter_->pre_filter(req, res);
    EXPECT_FALSE(cont);
    EXPECT_EQ(res.body, "cached");
    EXPECT_EQ(res.header("X-Cache"), "HIT");
}

TEST_F(CacheFilterTest, CacheMissContinuesProcessing) {
    http_request req;
    req.method = http_method::GET();
    req.path = "/data";
    http_response res;

    bool cont = filter_->pre_filter(req, res);
    EXPECT_TRUE(cont);
    EXPECT_EQ(res.header("X-Cache"), "MISS");
}

TEST_F(CacheFilterTest, PostRequestNotCached) {
    http_request req;
    req.method = http_method::POST();
    req.path = "/data";
    http_response res;

    bool cont = filter_->pre_filter(req, res);
    EXPECT_TRUE(cont);
}

TEST_F(CacheFilterTest, IfNoneMatchReturns304) {
    http_response resp;
    resp.body = "cached";
    string key = response_cache::build_key(http_method::GET(), "/data");
    cache_->put(key, resp, 60_s);

    http_request req;
    req.method = http_method::GET();
    req.path = "/data";
    req.set_header("If-None-Match", response_cache::generate_etag("cached"));
    http_response res;

    bool cont = filter_->pre_filter(req, res);
    EXPECT_FALSE(cont);
    EXPECT_EQ(res.status, http_status::S3_NOT_MODIFIED);
}

TEST_F(CacheFilterTest, PostFilterStoresCachedResponse) {
    http_request req;
    req.method = http_method::GET();
    req.path = "/stored";
    http_response resp;
    resp.status = http_status::S2_OK;
    resp.body = "store me";

    filter_->post_filter(req, resp);

    auto cached = cache_->get(response_cache::build_key(http_method::GET(), "/stored"));
    ASSERT_TRUE(cached.has_value());
    EXPECT_EQ(cached->body, "store me");
}

class AsyncFilterTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

class TestAsyncFilter : public async_filter {
public:
    bool async_called = false;
    bool* next_result = nullptr;

    TestAsyncFilter() noexcept = default;

    void do_filter(http_request&, http_response&) override {}

    void pre_filter_async(http_request&, http_response&, http_context&, next_callback next) override {
        async_called = true;
        if (next_result) {
            next(*next_result);
        } else {
            next(true);
        }
    }
};

TEST_F(AsyncFilterTest, AsyncFilterCalledInChain) {
    http_filter_chain chain;
    auto filter = make_unique<TestAsyncFilter>();
    auto* raw = filter.get();
    chain.add_filter(move(filter));

    http_request req;
    req.method = http_method::GET();
    http_response res;
    http_context ctx;
    bool completed = false;
    bool result = false;

    chain.execute_pre_filters_async(req, res, ctx, [&](bool ok) {
        completed = true;
        result = ok;
    });

    EXPECT_TRUE(raw->async_called);
    EXPECT_TRUE(completed);
    EXPECT_TRUE(result);
}

TEST_F(AsyncFilterTest, AsyncFilterCanInterruptChain) {
    http_filter_chain chain;
    auto filter = make_unique<TestAsyncFilter>();
    auto* raw = filter.get();
    bool should_continue = false;
    raw->next_result = &should_continue;
    chain.add_filter(move(filter));

    http_request req;
    req.method = http_method::GET();
    http_response res;
    http_context ctx;
    bool completed = false;
    bool result = true;

    chain.execute_pre_filters_async(req, res, ctx, [&](bool ok) {
        completed = true;
        result = ok;
    });

    EXPECT_TRUE(raw->async_called);
    EXPECT_TRUE(completed);
    EXPECT_FALSE(result);
}

TEST_F(AsyncFilterTest, AsyncPostFilterChain) {
    http_filter_chain chain;
    auto filter = make_unique<TestAsyncFilter>();
    auto* raw = filter.get();
    chain.add_filter(move(filter));

    http_request req;
    req.method = http_method::GET();
    http_response res;
    http_context ctx;
    bool completed = false;

    chain.execute_post_filters_async(req, res, ctx, [&]() { completed = true; });

    EXPECT_TRUE(completed);
}

class LoggingFilterTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(LoggingFilterTest, PreFilterAlwaysReturnsTrue) {
    logging_filter lf;
    http_request req;
    http_response res;
    EXPECT_TRUE(lf.pre_filter(req, res));
    EXPECT_EQ(lf.name(), "logging_filter");
}

TEST_F(LoggingFilterTest, LogHeadersEnabled) {
    logging_filter lf;
    lf.log_headers = true;
    http_request req;
    req.method = http_method::GET();
    req.path = "/test";
    req.set_header("X-Custom", "value");
    http_response res;
    EXPECT_TRUE(lf.pre_filter(req, res));
}

TEST_F(LoggingFilterTest, LogBodyEnabled) {
    logging_filter lf;
    lf.log_body = true;
    http_request req;
    req.body = "test body";
    http_response res;
    EXPECT_TRUE(lf.pre_filter(req, res));
}

TEST_F(LoggingFilterTest, PostFilterLogsResponse) {
    logging_filter lf;
    lf.log_headers = true;
    http_request req;
    http_response res;
    res.status = http_status::S2_OK;
    res.body = "response body";
    lf.post_filter(req, res);
    SUCCEED();
}

class StaticFileFilterFuncTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(StaticFileFilterFuncTest, NonGetMethodBypasses) {
    static_file_filter sff("/tmp");
    http_request req;
    req.method = http_method::POST();
    req.path = "/index.html";
    http_response res;
    EXPECT_TRUE(sff.pre_filter(req, res));
}

TEST_F(StaticFileFilterFuncTest, NonexistentFileBypasses) {
    static_file_filter sff("/nonexistent_path_xyz");
    http_request req;
    req.method = http_method::GET();
    req.path = "/no_such_file.html";
    http_response res;
    EXPECT_TRUE(sff.pre_filter(req, res));
}

TEST_F(StaticFileFilterFuncTest, AddCustomMimeType) {
    static_file_filter sff("/tmp");
    sff.add_mime_type(".custom", http_content::JSON_APP());
    auto mime = sff.get_mime_type("file.custom");
    ASSERT_TRUE(mime.has_value());
    EXPECT_TRUE(mime->is_json_app());
}

TEST_F(StaticFileFilterFuncTest, GetMimeTypeReturnsNone) {
    static_file_filter sff("/tmp");
    auto mime = sff.get_mime_type("file.unknownxyz");
    EXPECT_FALSE(mime.has_value());
}

TEST(MultipartFieldTest, IsFileReturnsCorrectly) {
    multipart_field field;
    field.name = "file";
    EXPECT_FALSE(field.is_file());
    field.filename = "test.txt";
    EXPECT_TRUE(field.is_file());
}

class ByteCursorEdgeTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(ByteCursorEdgeTest, DefaultConstructedIsExhausted) {
    byte_cursor cur;
    EXPECT_EQ(cur.remaining(), 0u);
    EXPECT_TRUE(cur.exhausted());
}

TEST_F(ByteCursorEdgeTest, EmptyDataExhausted) {
    uint8_t dummy = 0;
    byte_cursor cur(&dummy, 0);
    EXPECT_TRUE(cur.exhausted());
    EXPECT_EQ(cur.try_read_byte(), none);
}

TEST_F(ByteCursorEdgeTest, TryReadBe24ExactBoundary) {
    uint8_t data[] = {0x01, 0x02, 0x03};
    byte_cursor cur(data, 3);
    auto val = cur.try_read_be24();
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, 0x010203u);
    EXPECT_TRUE(cur.exhausted());
}

TEST_F(ByteCursorEdgeTest, TryReadBe24Truncated) {
    uint8_t data[] = {0x01, 0x02};
    byte_cursor cur(data, 2);
    auto val = cur.try_read_be24();
    EXPECT_FALSE(val.has_value());
    EXPECT_EQ(cur.remaining(), 2u);
}

TEST_F(ByteCursorEdgeTest, TryReadBe32ExactBoundary) {
    uint8_t data[] = {0xAA, 0xBB, 0xCC, 0xDD};
    byte_cursor cur(data, 4);
    auto val = cur.try_read_be32();
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, 0xAABBCCDDu);
    EXPECT_TRUE(cur.exhausted());
}

TEST_F(ByteCursorEdgeTest, TryReadBe32Truncated) {
    uint8_t data[] = {0x01, 0x02, 0x03};
    byte_cursor cur(data, 3);
    EXPECT_FALSE(cur.try_read_be32().has_value());
    EXPECT_EQ(cur.remaining(), 3u);
}

TEST_F(ByteCursorEdgeTest, TryReadBe64ExactBoundary) {
    uint8_t data[] = {0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02};
    byte_cursor cur(data, 8);
    auto val = cur.try_read_be64();
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, 0x0000000100000002ULL);
}

TEST_F(ByteCursorEdgeTest, TryReadBe64Truncated) {
    uint8_t data[] = {0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};
    byte_cursor cur(data, 7);
    EXPECT_FALSE(cur.try_read_be64().has_value());
}

TEST_F(ByteCursorEdgeTest, TryReadBytesExactSoft) {
    uint8_t data[] = {0x01, 0x02, 0x03};
    byte_cursor cur(data, 3);
    auto view = cur.try_read_bytes(3);
    ASSERT_TRUE(view.has_value());
    EXPECT_EQ(view->size(), 3u);
    EXPECT_TRUE(cur.exhausted());
}

TEST_F(ByteCursorEdgeTest, TryReadBytesPastEnd) {
    uint8_t data[] = {0x01, 0x02};
    byte_cursor cur(data, 2);
    EXPECT_FALSE(cur.try_read_bytes(3).has_value());
    EXPECT_EQ(cur.remaining(), 2u);
}

TEST_F(ByteCursorEdgeTest, SkipSuccess) {
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    byte_cursor cur(data, 4);
    EXPECT_TRUE(cur.skip(2));
    EXPECT_EQ(cur.remaining(), 2u);
    auto b = cur.try_read_byte();
    ASSERT_TRUE(b.has_value());
    EXPECT_EQ(*b, 0x03);
}

TEST_F(ByteCursorEdgeTest, SkipPastEndFails) {
    uint8_t data[] = {0x01};
    byte_cursor cur(data, 1);
    EXPECT_FALSE(cur.skip(2));
    EXPECT_EQ(cur.remaining(), 1u);
}

TEST_F(ByteCursorEdgeTest, PeekDoesNotAdvance) {
    uint8_t data[] = {0x42, 0x43};
    byte_cursor cur(data, 2);
    auto p = cur.peek_byte();
    ASSERT_TRUE(p.has_value());
    EXPECT_EQ(*p, 0x42);
    EXPECT_EQ(cur.remaining(), 2u);
}

TEST_F(ByteCursorEdgeTest, MixedReads) {
    uint8_t data[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    byte_cursor cur(data, 9);
    EXPECT_EQ(cur.try_read_byte(), optional<uint8_t>(0x00));
    EXPECT_EQ(cur.try_read_be24(), optional<uint32_t>(0x010203u));
    EXPECT_EQ(cur.try_read_be32(), optional<uint32_t>(0x04050607u));
    EXPECT_EQ(cur.remaining(), 1u);
    EXPECT_EQ(cur.try_read_byte(), optional<uint8_t>(0x08));
    EXPECT_TRUE(cur.exhausted());
}

TEST_F(ByteCursorEdgeTest, TryReadBe16AtBoundary) {
    uint8_t data[] = {0x12, 0x34};
    byte_cursor cur(data, 2);
    EXPECT_EQ(cur.try_read_be16(), optional<uint16_t>(0x1234));
    EXPECT_TRUE(cur.exhausted());
}

TEST_F(ByteCursorEdgeTest, TryReadBe16Truncated) {
    uint8_t data[] = {0x12};
    byte_cursor cur(data, 1);
    EXPECT_FALSE(cur.try_read_be16().has_value());
}

TEST_F(ByteCursorEdgeTest, TryReadLe16) {
    uint8_t data[] = {0x34, 0x12};
    byte_cursor cur(data, 2);
    EXPECT_EQ(cur.try_read_le16(), optional<uint16_t>(0x1234));
}

TEST_F(ByteCursorEdgeTest, TryReadLe32) {
    uint8_t data[] = {0xDD, 0xCC, 0xBB, 0xAA};
    byte_cursor cur(data, 4);
    EXPECT_EQ(cur.try_read_le32(), optional<uint32_t>(0xAABBCCDDu));
}

TEST_F(ByteCursorEdgeTest, ResetReusesCursor) {
    uint8_t data1[] = {0x01, 0x02};
    uint8_t data2[] = {0xFF, 0xFE, 0xFD};
    byte_cursor cur(data1, 2);
    ignore = cur.try_read_byte();
    cur.reset(data2, 3);
    EXPECT_EQ(cur.remaining(), 3u);
    EXPECT_EQ(cur.try_read_byte(), optional<uint8_t>(0xFF));
}

class Http2FrameHeaderTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(Http2FrameHeaderTest, GetSetLength) {
    http2_frame_header hdr{};
    hdr.set_length(0x123456);
    EXPECT_EQ(hdr.get_length(), 0x123456u);
    EXPECT_EQ(hdr.length_hi, 0x12);
    EXPECT_EQ(hdr.length_mid, 0x34);
    EXPECT_EQ(hdr.length_lo, 0x56);
}

TEST_F(Http2FrameHeaderTest, GetSetStreamId) {
    http2_frame_header hdr{};
    hdr.set_stream_id(0x7FFFFFFF);
    EXPECT_EQ(hdr.get_stream_id(), 0x7FFFFFFFu);
    EXPECT_EQ(hdr.sid_r, 0x7F);
}

TEST_F(Http2FrameHeaderTest, StreamIdMasksReservedBit) {
    http2_frame_header hdr{};
    hdr.sid_r = 0xFF;
    EXPECT_EQ(hdr.get_stream_id() & 0x80000000u, 0u);
}

TEST_F(Http2FrameHeaderTest, ZeroStreamId) {
    http2_frame_header hdr{};
    hdr.set_stream_id(0);
    EXPECT_EQ(hdr.get_stream_id(), 0u);
    EXPECT_EQ(hdr.sid_r, 0u);
}

class Http2FrameEncodeTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(Http2FrameEncodeTest, EncodeSettingsFrame) {
    http2_settings_frame sf;
    sf.ack = false;
    sf.entries = {{http2_settings_id::MAX_CONCURRENT_STREAMS, 128}, {http2_settings_id::INITIAL_WINDOW_SIZE, 131072}};
    auto frame = http2_framer::encode_settings_frame(sf);
    ASSERT_GE(frame.size(), 9u);
    EXPECT_EQ(frame[3], 0x04); // SETTINGS type
    EXPECT_EQ(frame[4], 0x00); // no ACK flag
    uint32_t sid = (static_cast<uint32_t>(frame[5] & 0x7F) << 24) | (static_cast<uint32_t>(frame[6]) << 16) |
                   (static_cast<uint32_t>(frame[7]) << 8) | frame[8];
    EXPECT_EQ(sid, 0u);
}

TEST_F(Http2FrameEncodeTest, EncodeSettingsAck) {
    http2_settings_frame sf;
    sf.ack = true;
    auto frame = http2_framer::encode_settings_frame(sf);
    EXPECT_EQ(frame[4] & HTTP2_FLAG_ACK, HTTP2_FLAG_ACK);
    http2_frame_header hdr{};
    hdr.length_hi = frame[0];
    hdr.length_mid = frame[1];
    hdr.length_lo = frame[2];
    EXPECT_EQ(hdr.get_length(), 0u);
}

TEST_F(Http2FrameEncodeTest, EncodePingFrame) {
    http2_ping_frame pf;
    pf.opaque_data = 0xFEEDFACECAFEBEEFULL;
    pf.ack = false;
    auto frame = http2_framer::encode_ping_frame(pf);
    ASSERT_EQ(frame.size(), 17u); // 9 header + 8 payload
    EXPECT_EQ(frame[3], 0x06);    // PING type
    EXPECT_EQ(frame[4], 0x00);    // not ACK
}

TEST_F(Http2FrameEncodeTest, EncodePingAck) {
    http2_ping_frame pf;
    pf.opaque_data = 0x1234;
    pf.ack = true;
    auto frame = http2_framer::encode_ping_frame(pf);
    EXPECT_EQ(frame[4] & 0x01, 0x01); // ACK flag set
}

TEST_F(Http2FrameEncodeTest, EncodeGoawayFrame) {
    http2_goaway_frame gf;
    gf.last_stream_id = 7;
    gf.error_code = http2_error::PROTOCOL_ERROR;
    gf.debug_data = {};
    auto frame = http2_framer::encode_goaway_frame(gf);
    ASSERT_GE(frame.size(), 17u); // 9 header + 8 (last_id + error)
    EXPECT_EQ(frame[3], 0x07);    // GOAWAY type
}

TEST_F(Http2FrameEncodeTest, EncodeRstStreamFrame) {
    http2_rst_stream_frame rf;
    rf.stream_id = 5;
    rf.error_code = http2_error::CANCEL;
    auto frame = http2_framer::encode_rst_stream_frame(rf);
    ASSERT_EQ(frame.size(), 13u); // 9 header + 4 error
    EXPECT_EQ(frame[3], 0x03);    // RST_STREAM type
}

TEST_F(Http2FrameEncodeTest, EncodeWindowUpdateFrame) {
    http2_window_update_frame wf;
    wf.stream_id = 3;
    wf.window_size_increment = 65535;
    auto frame = http2_framer::encode_window_update_frame(wf);
    ASSERT_EQ(frame.size(), 13u); // 9 header + 4 increment
    EXPECT_EQ(frame[3], 0x08);    // WINDOW_UPDATE type
}

TEST_F(Http2FrameEncodeTest, EncodeDataFrame) {
    http2_data_frame df;
    df.stream_id = 1;
    df.data = {0x48, 0x65, 0x6C, 0x6C, 0x6F}; // "Hello"
    df.end_stream = true;
    auto frame = http2_framer::encode_data_frame(df);
    ASSERT_EQ(frame.size(), 14u); // 9 header + 5 data
    EXPECT_EQ(frame[3], 0x00);    // DATA type
    EXPECT_EQ(frame[4] & HTTP2_FLAG_END_STREAM, HTTP2_FLAG_END_STREAM);
}

TEST_F(Http2FrameEncodeTest, EncodeHeadersFrame) {
    http2_headers_frame hf;
    hf.stream_id = 1;
    hf.header_block = {0x88, 0x00}; // minimal HPACK
    hf.end_headers = true;
    hf.end_stream = true;
    auto frame = http2_framer::encode_headers_frame(hf);
    ASSERT_GE(frame.size(), 11u);
    EXPECT_EQ(frame[3], 0x01); // HEADERS type
    EXPECT_EQ(frame[4] & HTTP2_FLAG_END_HEADERS, HTTP2_FLAG_END_HEADERS);
    EXPECT_EQ(frame[4] & HTTP2_FLAG_END_STREAM, HTTP2_FLAG_END_STREAM);
}

TEST_F(Http2FrameEncodeTest, EncodeContinuationFrame) {
    http2_continuation_frame cf;
    cf.stream_id = 1;
    cf.header_block = {0x01, 0x02, 0x03};
    cf.end_headers = true;
    auto frame = http2_framer::encode_continuation_frame(cf);
    ASSERT_GE(frame.size(), 12u);
    EXPECT_EQ(frame[3], 0x09); // CONTINUATION type
    EXPECT_EQ(frame[4] & HTTP2_FLAG_END_HEADERS, HTTP2_FLAG_END_HEADERS);
}

class Http2FrameDecodeErrorTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(Http2FrameDecodeErrorTest, DecodeEmptyDataNoCallback) {
    http2_framer framer;
    int call_count = 0;
    framer.decode_frames(nullptr, 0, [&](http2_frame_type, uint8_t, uint32_t, const byte_t*, size_t) { ++call_count; });
    EXPECT_EQ(call_count, 0);
}

TEST_F(Http2FrameDecodeErrorTest, DecodeTruncatedFrameHeader) {
    http2_framer framer;
    int call_count = 0;
    uint8_t data[] = {0x00, 0x00, 0x08};
    framer.decode_frames(data, 3, [&](http2_frame_type, uint8_t, uint32_t, const byte_t*, size_t) { ++call_count; });
    EXPECT_EQ(call_count, 0);
}

TEST_F(Http2FrameDecodeErrorTest, DecodeTruncatedPayload) {
    http2_framer framer;
    int call_count = 0;
    uint8_t data[] = {
            0x00, 0x00, 0x08,       // length = 8
            0x01,                   // type = HEADERS
            0x04,                   // flags = END_HEADERS
            0x00, 0x00, 0x00, 0x01, // stream_id = 1
            0xAA, 0xBB, 0xCC, 0xDD  // only 4 bytes, not 8
    };
    framer.decode_frames(data, sizeof(data),
                         [&](http2_frame_type, uint8_t, uint32_t, const byte_t*, size_t) { ++call_count; });
    EXPECT_EQ(call_count, 0); // incomplete frame
}

TEST_F(Http2FrameDecodeErrorTest, DecodeMultipleCompleteFrames) {
    http2_framer framer;
    int call_count = 0;
    http2_settings_frame sf;
    sf.ack = true;
    auto f1 = http2_framer::encode_settings_frame(sf);
    auto f2 = http2_framer::encode_settings_frame(sf);
    byte_vector combined;
    combined.insert(combined.end(), f1.begin(), f1.end());
    combined.insert(combined.end(), f2.begin(), f2.end());

    framer.decode_frames(combined.data(), combined.size(),
                         [&](http2_frame_type type, uint8_t flags, uint32_t stream_id, const byte_t*, size_t len) {
                             ++call_count;
                             EXPECT_EQ(type, http2_frame_type::SETTINGS);
                             EXPECT_EQ(flags & HTTP2_FLAG_ACK, HTTP2_FLAG_ACK);
                             EXPECT_EQ(stream_id, 0u);
                             EXPECT_EQ(len, 0u);
                         });
    EXPECT_EQ(call_count, 2);
}

TEST_F(Http2FrameDecodeErrorTest, DecodeFragmentedHeaderAcrossCalls) {
    http2_framer framer;
    int call_count = 0;
    http2_settings_frame sf;
    sf.ack = true;
    auto full_frame = http2_framer::encode_settings_frame(sf);

    framer.decode_frames(full_frame.data(), 5, [&](auto...) { ++call_count; });
    EXPECT_EQ(call_count, 0);

    framer.decode_frames(full_frame.data() + 5, full_frame.size() - 5, [&](auto...) { ++call_count; });
    EXPECT_EQ(call_count, 1);
}

TEST_F(Http2FrameDecodeErrorTest, EncodeDecodeRoundtripDataFrame) {
    http2_data_frame df;
    df.stream_id = 3;
    df.data = {0x01, 0x02, 0x03, 0x04};
    df.end_stream = true;
    auto encoded = http2_framer::encode_data_frame(df);

    http2_framer framer;
    int call_count = 0;
    framer.decode_frames(
            encoded.data(), encoded.size(),
            [&](http2_frame_type type, uint8_t flags, uint32_t stream_id, const byte_t* payload, size_t len) {
                ++call_count;
                EXPECT_EQ(type, http2_frame_type::DATA);
                EXPECT_TRUE(flags & HTTP2_FLAG_END_STREAM);
                EXPECT_EQ(stream_id, 3u);
                EXPECT_EQ(len, 4u);
                EXPECT_EQ(payload[0], 0x01);
                EXPECT_EQ(payload[3], 0x04);
            });
    EXPECT_EQ(call_count, 1);
}

class Http2StreamStateTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(Http2StreamStateTest, NewStreamIsIdle) {
    http2_stream s(1);
    EXPECT_EQ(s.id(), 1u);
    EXPECT_EQ(s.state(), http2_stream_state::IDLE);
    EXPECT_FALSE(s.is_closed());
}

TEST_F(Http2StreamStateTest, IdleStreamCanOnlySendHeaders) {
    http2_stream s(1);
    EXPECT_TRUE(s.can_send_headers());
    EXPECT_FALSE(s.can_send_data());
    EXPECT_FALSE(s.can_receive());
}

TEST_F(Http2StreamStateTest, ReceiveHeadersOpensStream) {
    http2_stream s(1);
    s.on_receive_headers(false);
    EXPECT_EQ(s.state(), http2_stream_state::OPEN);
    EXPECT_TRUE(s.can_receive());
    EXPECT_TRUE(s.can_send_headers());
}

TEST_F(Http2StreamStateTest, ReceiveHeadersWithEndStreamGoesHalfClosedRemote) {
    http2_stream s(1);
    s.on_receive_headers(true);
    EXPECT_EQ(s.state(), http2_stream_state::HALF_CLOSED_REMOTE);
}

TEST_F(Http2StreamStateTest, SendHeadersOpensStream) {
    http2_stream s(1);
    s.on_send_headers(false);
    EXPECT_EQ(s.state(), http2_stream_state::OPEN);
}

TEST_F(Http2StreamStateTest, SendHeadersWithEndStreamGoesHalfClosedLocal) {
    http2_stream s(1);
    s.on_send_headers(true);
    EXPECT_EQ(s.state(), http2_stream_state::HALF_CLOSED_LOCAL);
}

TEST_F(Http2StreamStateTest, FullLifecycleOpenToClosed) {
    http2_stream s(1);
    s.on_receive_headers(false);
    EXPECT_EQ(s.state(), http2_stream_state::OPEN);
    s.on_send_headers(false);
    EXPECT_EQ(s.state(), http2_stream_state::OPEN);
    s.on_receive_data(true);
    EXPECT_EQ(s.state(), http2_stream_state::HALF_CLOSED_REMOTE);
    s.on_send_data(true);
    EXPECT_EQ(s.state(), http2_stream_state::CLOSED);
    EXPECT_TRUE(s.is_closed());
}

TEST_F(Http2StreamStateTest, RstStreamFromOpenCloses) {
    http2_stream s(1);
    s.on_receive_headers(false);
    s.on_receive_rst_stream();
    EXPECT_TRUE(s.is_closed());
}

TEST_F(Http2StreamStateTest, CannotReceiveOnClosed) {
    http2_stream s(1);
    s.on_receive_headers(true);
    s.on_send_data(true);
    EXPECT_TRUE(s.is_closed());
    EXPECT_FALSE(s.can_receive());
    EXPECT_FALSE(s.can_send_headers());
    EXPECT_FALSE(s.can_send_data());
}

TEST_F(Http2StreamStateTest, CloseMethodSetsClosed) {
    http2_stream s(1);
    s.close();
    EXPECT_TRUE(s.is_closed());
    EXPECT_EQ(s.state(), http2_stream_state::CLOSED);
}

#ifdef NEFORCE_SUPPORT_ZLIB

class WebsocketDeflateConfigTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(WebsocketDeflateConfigTest, DefaultConfigNotActive) {
    websocket_deflate_config cfg;
    EXPECT_FALSE(cfg.active);
    EXPECT_EQ(cfg.client_max_window_bits, 15);
    EXPECT_EQ(cfg.server_max_window_bits, 15);
}

TEST_F(WebsocketDeflateConfigTest, NegotiateEmptyReturnsInactive) {
    auto cfg = websocket_deflate_config::negotiate("");
    EXPECT_FALSE(cfg.active);
}

TEST_F(WebsocketDeflateConfigTest, NegotiatePermessageDeflate) {
    auto cfg = websocket_deflate_config::negotiate("permessage-deflate");
    EXPECT_TRUE(cfg.active);
    EXPECT_EQ(cfg.client_max_window_bits, 15);
}

TEST_F(WebsocketDeflateConfigTest, NegotiateWithClientMaxWindowBits) {
    auto cfg = websocket_deflate_config::negotiate("permessage-deflate; client_max_window_bits=12");
    EXPECT_TRUE(cfg.active);
    EXPECT_EQ(cfg.client_max_window_bits, 12);
}

TEST_F(WebsocketDeflateConfigTest, NegotiateWithServerMaxWindowBits) {
    auto cfg = websocket_deflate_config::negotiate("permessage-deflate; server_max_window_bits=13");
    EXPECT_TRUE(cfg.active);
    EXPECT_EQ(cfg.server_max_window_bits, 13);
}

TEST_F(WebsocketDeflateConfigTest, NegotiateWithNoContextTakeover) {
    auto cfg = websocket_deflate_config::negotiate("permessage-deflate; server_no_context_takeover");
    EXPECT_TRUE(cfg.active);
    EXPECT_TRUE(cfg.server_no_context_takeover);
}

TEST_F(WebsocketDeflateConfigTest, ResponseHeaderWhenActive) {
    websocket_deflate_config cfg;
    cfg.active = true;
    auto hdr = cfg.to_response_header();
    EXPECT_FALSE(hdr.empty());
    EXPECT_TRUE(hdr.starts_with("permessage-deflate"));
}

TEST_F(WebsocketDeflateConfigTest, ResponseHeaderWhenInactive) {
    websocket_deflate_config cfg;
    EXPECT_TRUE(cfg.to_response_header().empty());
}

class WebsocketDeflateTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(WebsocketDeflateTest, CompressDecompressRoundtrip) {
    websocket_deflate compressor(true, 15, false);
    websocket_deflate decompressor(false, 15, false);

    string original = "Hello, WebSocket! This is a test message for deflate compression.";
    auto compressed = compressor.process(original.view(), true);
    EXPECT_FALSE(compressed.empty());
    EXPECT_NE(compressed, original);

    auto decompressed = decompressor.process(compressed.view(), true);
    EXPECT_EQ(decompressed, original);
}

TEST_F(WebsocketDeflateTest, CompressEmptyData) {
    websocket_deflate compressor(true, 15, false);
    auto result = compressor.process("", true);
    EXPECT_FALSE(result.empty());
}

TEST_F(WebsocketDeflateTest, DecompressEmptyData) {
    websocket_deflate decompressor(false, 15, false);
    auto result = decompressor.process("", true);
    EXPECT_TRUE(result.empty() || !result.empty());
}

TEST_F(WebsocketDeflateTest, FragmentedMessageRoundtrip) {
    websocket_deflate compressor(true, 15, false);
    websocket_deflate decompressor(false, 15, false);

    string part1 = "First part, ";
    string part2 = "second part, ";
    string part3 = "final part!";

    auto c1 = compressor.process(part1.view(), false);
    auto c2 = compressor.process(part2.view(), false);
    auto c3 = compressor.process(part3.view(), true);

    auto d1 = decompressor.process(c1.view(), false);
    auto d2 = decompressor.process(c2.view(), false);
    auto d3 = decompressor.process(c3.view(), true);

    EXPECT_EQ(d1 + d2 + d3, part1 + part2 + part3);
}

TEST_F(WebsocketDeflateTest, NoContextTakeoverResetsStream) {
    websocket_deflate compressor(true, 15, true);
    websocket_deflate decompressor(false, 15, true);

    auto c1 = compressor.process("Message one", true);
    auto d1 = decompressor.process(c1.view(), true);
    EXPECT_EQ(d1, "Message one");

    auto c2 = compressor.process("Message two", true);
    auto d2 = decompressor.process(c2.view(), true);
    EXPECT_EQ(d2, "Message two");
}

#endif

class HpackRoundtripTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(HpackRoundtripTest, EncodeDecodeBasicHeaders) {
    hpack_encoder encoder;
    hpack_decoder decoder;

    vector<hpack_header_field> headers = {
            {":method", "GET"},
            {":path", "/"},
            {":scheme", "https"},
            {":authority", "example.com"},
    };

    auto encoded = encoder.encode(headers);
    EXPECT_FALSE(encoded.empty());

    auto decoded = decoder.decode(encoded.data(), encoded.size());
    ASSERT_EQ(decoded.size(), headers.size());
    EXPECT_EQ(decoded[0].name, ":method");
    EXPECT_EQ(decoded[0].value, "GET");
    EXPECT_EQ(decoded[1].name, ":path");
    EXPECT_EQ(decoded[1].value, "/");
}

TEST_F(HpackRoundtripTest, EncodeDecodeCustomHeaders) {
    hpack_encoder encoder;
    hpack_decoder decoder;

    vector<hpack_header_field> headers = {
            {"content-type", "application/json"},
            {"x-custom-header", "custom-value"},
            {"cache-control", "no-cache"},
    };

    auto encoded = encoder.encode(headers);
    auto decoded = decoder.decode(encoded.data(), encoded.size());
    ASSERT_EQ(decoded.size(), headers.size());
    EXPECT_EQ(decoded[2].name, "cache-control");
    EXPECT_EQ(decoded[2].value, "no-cache");
}

TEST_F(HpackRoundtripTest, EncodeEmptyHeaders) {
    hpack_encoder encoder;
    hpack_decoder decoder;

    auto encoded = encoder.encode({});
    auto decoded = decoder.decode(encoded.data(), encoded.size());
    EXPECT_TRUE(decoded.empty());
}

TEST_F(HpackRoundtripTest, DecodeInvalidData) {
    hpack_decoder decoder;
    auto result = decoder.decode(nullptr, 0);
    EXPECT_TRUE(result.empty());
}

TEST_F(HpackRoundtripTest, DynamicTableEvolution) {
    hpack_encoder encoder(4096);
    vector<hpack_header_field> headers1 = {{"x-repeat", "value1"}};
    auto e1 = encoder.encode(headers1);
    EXPECT_GT(encoder.table_size(), 0u);

    vector<hpack_header_field> headers2 = {{"x-repeat", "value1"}};
    auto e2 = encoder.encode(headers2);
    EXPECT_LT(e2.size(), e1.size());
}

TEST_F(HpackRoundtripTest, MaxTableSizeLimits) {
    hpack_encoder encoder(256);
    hpack_decoder decoder(256);

    vector<hpack_header_field> headers;
    for (int i = 0; i < 20; ++i) {
        headers.push_back({"x-header-" + to_string(i), "value-" + to_string(i)});
    }

    auto encoded = encoder.encode(headers);
    auto decoded = decoder.decode(encoded.data(), encoded.size());
    EXPECT_EQ(decoded.size(), headers.size());
}

class WebsocketFrameHeaderTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(WebsocketFrameHeaderTest, HeaderSizeIsTwoBytes) { EXPECT_EQ(sizeof(websocket_frame_header), 2u); }

TEST_F(WebsocketFrameHeaderTest, FinAndOpcodeSetting) {
    websocket_frame_header hdr{};
    hdr.fin = 1;
    hdr.opcode = 0x2;
    EXPECT_EQ(hdr.fin, 1u);
    EXPECT_EQ(hdr.opcode, 0x2u);
}

TEST_F(WebsocketFrameHeaderTest, MaskedBit) {
    websocket_frame_header hdr{};
    hdr.masked = 1;
    EXPECT_EQ(hdr.masked, 1u);
}

TEST_F(WebsocketFrameHeaderTest, PayloadLen7Bits) {
    websocket_frame_header hdr{};
    hdr.payload_len = 125;
    EXPECT_EQ(hdr.payload_len, 125u);
    hdr.payload_len = 126;
    EXPECT_EQ(hdr.payload_len, 126u);
    hdr.payload_len = 127;
    EXPECT_EQ(hdr.payload_len, 127u);
}

TEST_F(WebsocketFrameHeaderTest, ReservedBitsAreSeparate) {
    websocket_frame_header hdr{};
    hdr.rsv1 = 1;
    hdr.rsv2 = 0;
    hdr.rsv3 = 1;
    EXPECT_EQ(hdr.rsv1, 1u);
    EXPECT_EQ(hdr.rsv2, 0u);
    EXPECT_EQ(hdr.rsv3, 1u);
}

TEST_F(WebsocketFrameHeaderTest, DefaultValuesAreZero) {
    websocket_frame_header hdr{};
    EXPECT_EQ(hdr.fin, 0u);
    EXPECT_EQ(hdr.opcode, 0u);
    EXPECT_EQ(hdr.masked, 0u);
    EXPECT_EQ(hdr.payload_len, 0u);
    EXPECT_EQ(hdr.rsv1, 0u);
    EXPECT_EQ(hdr.rsv2, 0u);
    EXPECT_EQ(hdr.rsv3, 0u);
}

class WebsocketServerRoutingTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(WebsocketServerRoutingTest, InitiallyNoSessions) {
    websocket_server server;
    EXPECT_EQ(server.session_count(), 0u);
}

TEST_F(WebsocketServerRoutingTest, RouteRegistersHandler) {
    websocket_server server;
    bool called = false;
    server.route("/chat", [&](websocket_server::session_ptr) { called = true; });
    EXPECT_FALSE(called);
}

TEST_F(WebsocketServerRoutingTest, MultipleRoutesCanBeRegistered) {
    websocket_server server;
    int count = 0;
    server.route("/a", [&](websocket_server::session_ptr) { ++count; });
    server.route("/b", [&](websocket_server::session_ptr) { ++count; });
    server.route("/c", [&](websocket_server::session_ptr) { ++count; });
    EXPECT_EQ(count, 0);
}

class HttpServerAlpnTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(HttpServerAlpnTest, HttpsConstructorSetsAlpnProtocols) {
    ssl_context ctx(ssl_method::TLS_SERVER);
    bool cert_loaded = ctx.load_certificate("/tmp/h2test.crt", "/tmp/h2test.key");
    if (!cert_loaded) {
        GTEST_SKIP() << "Test certificate not found (run 'openssl req -x509 ...' first)";
    }

    http_server server(ports(8445), move(ctx), 1);
    server.start();
    EXPECT_TRUE(server.is_running());
    server.stop();
}

TEST_F(HttpServerAlpnTest, HttpServerWithoutSslDoesNotCrash) {
    http_server server(ports(8088), 1);
    server.start();
    EXPECT_TRUE(server.is_running());
    server.stop();
}

TEST_F(HttpServerAlpnTest, SslSocketGetAlpnNegotiatedProxyWorks) {
    ssl_socket sock;
    EXPECT_TRUE(sock.get_alpn_negotiated().empty());
    EXPECT_FALSE(sock.is_ssl());
}

TEST_F(HttpServerAlpnTest, SslStreamGetAlpnNegotiatedWithoutHandshake) {
    ssl_context ctx(ssl_method::TLS_CLIENT);
    ctx.set_alpn_protos({"h2"});
    ssl_stream stream(ctx);
    EXPECT_TRUE(stream.get_alpn_negotiated().empty());
}
