#include <NeForce/network/http/http_constants.hpp>
#include <NeForce/network/http/http_session.hpp>
#include <NeForce/network/http/http_server_message.hpp>
#include <NeForce/network/http/http_client_message.hpp>
#include <NeForce/network/http/http_filter.hpp>
#include <NeForce/network/http/http_router.hpp>
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
                                   "Connection: keep-alive\r\n"
                                   "\r\n");
    EXPECT_TRUE(req.is_keep_alive());
}

TEST_F(HttpRequestParseTest, IsKeepAliveDetectsCloseConnection) {
    auto req = http_request::parse("GET / HTTP/1.1\r\n"
                                   "Connection: close\r\n"
                                   "\r\n");
    EXPECT_FALSE(req.is_keep_alive());
}

TEST_F(HttpRequestParseTest, IsAjaxDetectsXmlHttpRequest) {
    auto req = http_request::parse("GET /api HTTP/1.1\r\n"
                                   "X-Requested-With: XMLHttpRequest\r\n"
                                   "\r\n");
    EXPECT_TRUE(req.is_ajax());
}

TEST_F(HttpRequestParseTest, UserAgentAndReferer) {
    auto req = http_request::parse("GET / HTTP/1.1\r\n"
                                   "User-Agent: Mozilla/5.0\r\n"
                                   "Referer: https://google.com\r\n"
                                   "\r\n");
    EXPECT_EQ(req.user_agent(), "Mozilla/5.0");
    EXPECT_EQ(req.referer(), "https://google.com");
}

class HttpResponseTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(HttpResponseTest, DefaultResponseHasCloseConnection) {
    http_response resp;
    EXPECT_EQ(resp.header("Connection"), "close");
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

class RateLimitFilterTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(RateLimitFilterTest, FirstRequestPasses) {
    rate_limit_filter filter(5, seconds{60});
    http_request req;
    req.set_header("X-Forwarded-For", "192.168.1.1");
    http_response resp;
    EXPECT_TRUE(filter.pre_filter(req, resp));
}

TEST_F(RateLimitFilterTest, ExceedingLimitReturns429) {
    rate_limit_filter filter(2, seconds{60});
    http_request req;
    req.set_header("X-Forwarded-For", "10.0.0.1");
    http_response resp;

    EXPECT_TRUE(filter.pre_filter(req, resp));
    EXPECT_TRUE(filter.pre_filter(req, resp));
    EXPECT_FALSE(filter.pre_filter(req, resp));
    EXPECT_EQ(static_cast<uint16_t>(resp.status), 429u);
    EXPECT_EQ(resp.header("Retry-After"), "60");
}

TEST_F(RateLimitFilterTest, EmptyClientIpPasses) {
    rate_limit_filter filter(1, seconds{60});
    http_request req;
    http_response resp;
    EXPECT_TRUE(filter.pre_filter(req, resp));
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
    EXPECT_EQ(resp.header("WWW-Authenticate"), "Bearer");
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
        EXPECT_EQ(resp.body, "ok") << "Method: " << method.data();
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
