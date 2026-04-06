#ifndef NEFORCE_NETWORK_HTTP_SERVER_MESSAGE_HPP__
#define NEFORCE_NETWORK_HTTP_SERVER_MESSAGE_HPP__
#include "NeForce/network/http/http_session.hpp"
NEFORCE_BEGIN_NAMESPACE__

struct http_request {
    HTTP_METHOD method = HTTP_METHOD::GET();
    string path = "/";
    string version = "HTTP/1.1";
    string query{};
    string body{};

    unordered_map<string, string> headers;
    unordered_map<string, string> cookies;
    unordered_map<string, string> parameters;
    unordered_map<string, string> form_data;

    http_session* session = nullptr;

    NEFORCE_NODISCARD string_view parameter(const string& name) const noexcept {
        const auto it = parameters.find(name);
        return it != parameters.end() ? it->second.view() : "";
    }

    void set_parameter(const string& name, string value) { parameters[name] = move(value); }

    NEFORCE_NODISCARD bool has_parameter(const string& name) const noexcept {
        return parameters.find(name) != parameters.end();
    }

    NEFORCE_NODISCARD string_view cookie(const string& name) const noexcept {
        const auto it = cookies.find(name);
        return it != cookies.end() ? it->second.view() : "";
    }

    void set_cookie(const string& name, string value) { cookies[name] = move(value); }

    NEFORCE_NODISCARD bool has_cookie(const string& name) const noexcept { return cookies.find(name) != cookies.end(); }

    NEFORCE_NODISCARD string_view header(const string& name) const noexcept {
        const auto it = headers.find(name);
        return it != headers.end() ? it->second.view() : "";
    }

    void set_header(const string& name, string value) { headers[name] = move(value); }

    NEFORCE_NODISCARD bool has_header(const string& name) const noexcept { return headers.find(name) != headers.end(); }

    NEFORCE_NODISCARD bool has_session() const noexcept { return session != nullptr && session->is_valid(); }

    NEFORCE_NODISCARD string_view content_type() const noexcept { return header(HTTP_KEY::Content_Type()); }

    NEFORCE_NODISCARD bool is_keep_alive() const noexcept {
        const auto conn = header(HTTP_KEY::Connection());
        return conn == "keep-alive" || conn == "Keep-Alive";
    }

    NEFORCE_NODISCARD string_view client_ip() const noexcept {
        const auto xff = header("X-Forwarded-For");
        if (!xff.empty()) {
            const auto comma = xff.find(',');
            return comma != string::npos ? xff.view(0, comma) : xff;
        }
        const auto xri = header("X-Real-IP");
        if (!xri.empty()) {
            return xri;
        }
        return "";
    }

    NEFORCE_NODISCARD string_view user_agent() const noexcept { return header("User-Agent"); }

    NEFORCE_NODISCARD string_view referer() const noexcept { return header("Referer"); }

    NEFORCE_NODISCARD bool is_ajax() const noexcept {
        const auto xhr = header("X-Requested-With");
        return xhr == "XMLHttpRequest";
    }

    void clear() {
        method = HTTP_METHOD::GET();
        path = "/";
        version = "HTTP/1.1";
        query.clear();
        body.clear();
        headers.clear();
        cookies.clear();
        parameters.clear();
        session = nullptr;
    }
};


struct NEFORCE_API http_response {
    string version{"HTTP/1.1"};
    HTTP_STATUS status = HTTP_STATUS::S4_NOT_FOUNT;
    string status_message{};
    unordered_map<string, string> headers;
    vector<http_cookie> cookies;
    string body{};
    string redirect_url{};
    string forward_path{};

    http_response() {
        headers[HTTP_KEY::Content_Type()] = HTTP_CONTENT::PLAIN_TEXT().to_string() + "; charset=utf-8";
        headers[HTTP_KEY::Connection()] = "close";
    }

    NEFORCE_NODISCARD string_view header(const string& name) const noexcept {
        const auto it = headers.find(name);
        return it != headers.end() ? it->second.view() : "";
    }

    void set_header(const string& name, string value) { headers[name] = move(value); }

    NEFORCE_NODISCARD bool has_header(const string& name) const noexcept { return headers.find(name) != headers.end(); }

    void set_content_type(HTTP_CONTENT value) { headers[HTTP_KEY::Content_Type()] = move(value).content(); }

    void set_content_type(string value) { headers[HTTP_KEY::Content_Type()] = move(value); }
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_HTTP_SERVER_MESSAGE_HPP__
