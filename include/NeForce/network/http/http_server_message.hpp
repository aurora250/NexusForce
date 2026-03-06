#ifndef NEFORCE_NETWORK_HTTP_SERVER_MESSAGE_HPP__
#define NEFORCE_NETWORK_HTTP_SERVER_MESSAGE_HPP__
#include "NeForce/network/http/session.hpp"
NEFORCE_BEGIN_NAMESPACE__

struct http_request {
    HTTP_METHOD method = HTTP_METHOD::GET;
    string path = "/";
    string version = "HTTP/1.1";
    string query{};
    string body{};

    unordered_map<string, string> headers;
    unordered_map<string, string> cookies;
    unordered_map<string, string> parameters;
    _NEFORCE session* session = nullptr;

    NEFORCE_NODISCARD string_view parameter(const string& name) const noexcept {
        const auto it = parameters.find(name);
        return it != parameters.end() ? it->second.view() : "";
    }
    void set_parameter(const string& name, string value) {
        parameters[name] = move(value);
    }

    NEFORCE_NODISCARD string_view cookie(const string& name) const noexcept {
        const auto it = cookies.find(name);
        return it != cookies.end() ? it->second.view() : "";
    }
    void set_cookie(const string& name, string value) {
        cookies[name] = move(value);
    }

    NEFORCE_NODISCARD string_view header(const string& name) const noexcept {
        const auto it = headers.find(name);
        return it != headers.end() ? it->second.view() : "";
    }
    void set_header(const string& name, string value) {
        headers[name] = move(value);
    }
};


struct NEFORCE_API http_response {
    string version = "HTTP/1.1";
    HTTP_STATUS status = HTTP_STATUS::S4_NOT_FOUNT;
    string status_message = "";
    unordered_map<string, string> headers;
    vector<cookie> cookies;
    string body{};
    string redirect_url{};
    string forward_path{};

    http_response() {
        headers[HTTP_KEY::Content_Type] = HTTP_CONTENT::HTML_TEXT.to_string() + "; charset=utf-8";
        headers[HTTP_KEY::Connection] = "close";
    }

    NEFORCE_NODISCARD string_view header(const string& name) const noexcept {
        const auto it = headers.find(name);
        return it != headers.end() ? it->second.view() : "";
    }
    void set_header(const string& name, const string& value) {
        headers[name] = value;
    }

    void set_content_type(HTTP_CONTENT value) {
        headers[HTTP_KEY::Content_Type] = move(value).content();
    }
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_HTTP_SERVER_MESSAGE_HPP__
