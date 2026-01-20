#ifndef MSTL_NETWORK_HTTP_SERVER_MESSAGE_HPP__
#define MSTL_NETWORK_HTTP_SERVER_MESSAGE_HPP__
#include "session.hpp"
MSTL_BEGIN_NAMESPACE__

struct MSTL_API http_request {
private:
    HTTP_METHOD method_ = HTTP_METHOD::GET;
    string path_ = "/";
    string version_ = "HTTP/1.1";
    unordered_map<string, string> headers_;
    unordered_map<string, string> cookies_;
    unordered_map<string, string> parameters_; // query + body parameters
    string query_{};
    string body_{};
    _MSTL session* session_ = nullptr;

    static const string EMPTY_MARK;

public:
    http_request() = default;
    http_request(const http_request&) = delete;
    http_request& operator =(const http_request&) = delete;
    http_request(http_request&& req) noexcept = default;
    http_request& operator =(http_request&& req) noexcept = default;
    ~http_request() = default;

    void set_version(string version) noexcept { this->version_ = _MSTL move(version); }
    MSTL_NODISCARD const string& version() const noexcept { return version_; }

    MSTL_NODISCARD const unordered_map<string, string>& parameters() const noexcept { return parameters_; }
    MSTL_NODISCARD unordered_map<string, string>& parameters() noexcept { return parameters_; }

    MSTL_NODISCARD const unordered_map<string, string>& headers() const noexcept { return headers_; }
    MSTL_NODISCARD unordered_map<string, string>& headers() noexcept { return headers_; }

    void set_parameter(const string& name, string value) {
        parameters_[name] = _MSTL move(value);
    }
    MSTL_NODISCARD const string& parameter(const string& name) const noexcept {
        const auto it = parameters_.find(name);
        return it != parameters_.end() ? it->second : EMPTY_MARK;
    }

    void set_cookie(const string& name, string value) {
        cookies_[name] = _MSTL move(value);
    }
    MSTL_NODISCARD const string& cookie(const string& name) const noexcept {
        const auto it = cookies_.find(name);
        return it != cookies_.end() ? it->second : EMPTY_MARK;
    }

    void set_session(_MSTL session* session) noexcept { this->session_ = session; }
    MSTL_NODISCARD const _MSTL session* session() const noexcept { return session_; }
    MSTL_NODISCARD _MSTL session* session() noexcept { return session_; }

    void set_header(const string& name, string value) {
        headers_[name] = _MSTL move(value);
    }
    MSTL_NODISCARD const string& header(const string& name) const noexcept {
        const auto it = headers_.find(name);
        return it != headers_.end() ? it->second : EMPTY_MARK;
    }

    void set_content_type(string value) { cookies_["Content-Type"] = _MSTL move(value); }
    MSTL_NODISCARD const string& content_type() const noexcept { return header("Content-Type"); }
    MSTL_NODISCARD const string& header_cookie() const noexcept { return header("Cookie"); }

    void set_method(HTTP_METHOD method) noexcept { this->method_ = _MSTL move(method); }
    MSTL_NODISCARD const HTTP_METHOD& method() const noexcept { return method_; }

    void set_path(string path) noexcept { this->path_ = _MSTL move(path); }
    MSTL_NODISCARD const string& path() const noexcept { return path_; }

    void set_query(string query) noexcept { this->query_ = _MSTL move(query); }
    MSTL_NODISCARD const string& query() const noexcept { return query_; }

    void set_body(string body) noexcept { this->body_ = _MSTL move(body); }
    MSTL_NODISCARD const string& body() const noexcept { return body_; }

    MSTL_NODISCARD const string& forward_protocol() const noexcept {
        return header("X-Forwarded-Proto");
    }
    void set_forward_protocol(string proto) {
        set_header("X-Forwarded-Proto", _MSTL move(proto));
    }

    MSTL_NODISCARD bool is_https() const noexcept { return forward_protocol() == "https"; }
    MSTL_NODISCARD bool is_http() const noexcept { return forward_protocol() == "http"; }
    void set_https() { set_forward_protocol("https"); }
    void set_http() { set_forward_protocol("http"); }
};


struct MSTL_API http_response {
private:
    string version_ = "HTTP/1.1";
    HTTP_STATUS status_ = HTTP_STATUS::S4_NOT_FOUNT;
    string status_msg_ = "";
    unordered_map<string, string> headers_;
    vector<cookie> cookies_;
    string body_{};
    string redirect_url_{};
    string forward_path_{};

    static const string EMPTY_MARK;

public:
    http_response() {
        set_content_type(HTTP_CONTENT::HTML_TEXT);
        set_content_encode("utf-8");
        set_header("Connection", "close");
    }

    http_response(const http_response&) = delete;
    http_response& operator =(const http_response&) = delete;
    http_response(http_response&& res) noexcept = default;
    http_response& operator =(http_response&& res) noexcept = default;

    void set_version(string version) noexcept { this->version_ = _MSTL move(version); }
    MSTL_NODISCARD const string& version() const noexcept { return version_; }


    void add_cookie(cookie cookie) { cookies_.emplace_back(_MSTL move(cookie)); }
    void add_cookie(const HTTP_COOKIE_NAME& name, const string& value) { cookies_.emplace_back(name, value); }

    MSTL_NODISCARD const vector<cookie>& cookies() const noexcept { return cookies_; }
    MSTL_NODISCARD vector<cookie>& cookies() noexcept { return cookies_; }


    void set_header(const string& name, const string& value) {
        headers_[name] = value;
    }
    MSTL_NODISCARD const string& header(const string& name) const noexcept {
        const auto it = headers_.find(name);
        return it != headers_.end() ? it->second : EMPTY_MARK;
    }

    MSTL_NODISCARD const unordered_map<string, string>& headers() const noexcept { return headers_; }
    MSTL_NODISCARD unordered_map<string, string>& headers() noexcept { return headers_; }


    MSTL_NODISCARD const string& content_length() const noexcept {
        return header("Content-Length");
    }

    void set_content_type(string value) {
        headers_["Content-Type"] = _MSTL move(value);
    }
    void set_content_type(HTTP_CONTENT value) {
        headers_["Content-Type"] = _MSTL move(value).content();
    }
    MSTL_NODISCARD const string& content_type() const noexcept {
        return header("Content-Type");
    }

    void set_content_encode(string value) {
        headers_["Content-Type"] += "; charset=" + _MSTL move(value);
    }


    void set_allow_method(string method) {
        headers_["Access-Control-Allow-Methods"] = _MSTL move(method);
    }
    void set_allow_method(HTTP_METHOD method) {
        headers_["Access-Control-Allow-Methods"] = _MSTL move(method).method();
    }
    MSTL_NODISCARD const string& allow_method() const noexcept {
        return header("Access-Control-Allow-Methods");
    }

    void set_max_age(const size_t age) {
        headers_["Access-Control-Max-Age"] = _MSTL to_string(age);
    }
    MSTL_NODISCARD size_t max_age() const noexcept {
        return package_t<size_t>().try_parse(header("Access-Control-Max-Age").view());
    }

    void set_allow_credentials(const bool allow) {
        if (allow) headers_["Access-Control-Allow-Credentials"] = "true";
        else headers_["Access-Control-Allow-Credentials"] = "false";
    }
    MSTL_NODISCARD bool all_credentials() const noexcept {
        return boolean().try_parse(header("Access-Control-Allow-Credentials").view());
    }

    void set_allow_origin(string origin) {
        headers_["Access-Control-Allow-Origin"] = _MSTL move(origin);
    }
    MSTL_NODISCARD const string& allow_origin() const noexcept {
        return header("Access-Control-Allow-Origin");
    }

    void set_allow_headers(string header) {
        headers_["Access-Control-Allow-Headers"] = _MSTL move(header);
    }
    MSTL_NODISCARD const string& allow_headers() const noexcept {
        return header("Access-Control-Allow-Headers");
    }


    void set_status(const HTTP_STATUS status) noexcept { status_ = status; }
    void set_ok() noexcept { status_ = HTTP_STATUS::S2_OK; }
    void set_not_found() noexcept { status_ = HTTP_STATUS::S4_NOT_FOUNT; }
    void set_bad_request() noexcept { status_ = HTTP_STATUS::S4_BAD_REQUEST; }

    MSTL_NODISCARD HTTP_STATUS status() const { return status_; }


    void set_status_msg(string status_msg) { this->status_msg_ = _MSTL move(status_msg); }
    MSTL_NODISCARD const string& status_msg() const { return status_msg_; }

    void set_body(string body) noexcept { this->body_ = _MSTL move(body); }
    MSTL_NODISCARD const string& body() const noexcept { return body_; }

    void set_redirect(string url) noexcept { redirect_url_ = _MSTL move(url); }
    MSTL_NODISCARD const string& redirect() const noexcept { return redirect_url_; }

    void set_forward(string url) noexcept { forward_path_ = _MSTL move(url); }
    MSTL_NODISCARD const string& forward() const noexcept { return forward_path_; }
};

MSTL_END_NAMESPACE__
#endif // MSTL_NETWORK_HTTP_SERVER_MESSAGE_HPP__
