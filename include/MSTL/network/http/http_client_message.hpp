#ifndef MSTL_NETWORK_HTTP_HTTP_CLIENT_MESSAGE_HPP__
#define MSTL_NETWORK_HTTP_HTTP_CLIENT_MESSAGE_HPP__
#include "MSTL/core/container/unordered_map.hpp"
#include "MSTL/core/container/vector.hpp"
#include "http_constants.hpp"
#include "session.hpp"
MSTL_BEGIN_NAMESPACE__

struct http_client_response {
private:
    string version_;
    HTTP_STATUS status_;
    string status_msg_;
    unordered_map<string, vector<string>> headers_;
    string body_;
    vector<cookie> cookies_;

public:
    http_client_response() = default;
    http_client_response(const http_client_response &) = delete;
    http_client_response &operator=(const http_client_response &) = delete;
    http_client_response(http_client_response &&) noexcept = default;
    http_client_response &operator=(http_client_response &&) noexcept = default;

    void set_version(string v) noexcept { version_ = _MSTL move(v); }
    void set_status(const HTTP_STATUS s) noexcept { status_ = s; }
    void set_status_msg(string msg) noexcept { status_msg_ = _MSTL move(msg); }

    void add_header(const string& key, string value) {
        headers_[key].push_back(_MSTL move(value));
    }
    MSTL_NODISCARD const string& header(const string &key) const noexcept {
        static const string empty;
        const auto it = headers_.find(key);
        if (it == headers_.end() || it->second.empty()) return empty;
        return it->second[0];
    }
    MSTL_NODISCARD const vector<string>& headers(const string &key) const noexcept {
        static vector<string> empty;
        const auto it = headers_.find(key);
        return it != headers_.end() ? it->second : empty;
    }
    MSTL_NODISCARD const unordered_map<string, vector<string>>&
    all_headers() const noexcept {
        return headers_;
    }

    void append_body(string b) { body_ += _MSTL move(b); }
    MSTL_NODISCARD const string& body() const noexcept { return body_; }

    void append_cookie(cookie c) { cookies_.push_back(_MSTL move(c)); }
    MSTL_NODISCARD const vector<cookie>& cookies() const noexcept { return cookies_; }

    MSTL_NODISCARD const string& version() const noexcept { return version_; }
    MSTL_NODISCARD HTTP_STATUS status() const noexcept { return status_; }
    MSTL_NODISCARD const string& status_msg() const noexcept { return status_msg_; }
};

struct http_client_request {
    HTTP_METHOD method = HTTP_METHOD::GET;
    string host;
    uint16_t port = 80;
    string path = "/";
    string version = "HTTP/1.1";
    unordered_map<string, string> headers;
    string body;

    explicit http_client_request(string h, const uint16_t p = 80)
    : host(_MSTL move(h)), port(p) {}

    void set_header(const string& key, string value) { headers[key] = _MSTL move(value); }
    void set_body(string b) { body = _MSTL move(b); }
    void set_method(HTTP_METHOD m) { method = _MSTL move(m); }
};

MSTL_END_NAMESPACE__
#endif // MSTL_NETWORK_HTTP_HTTP_CLIENT_MESSAGE_HPP__
