#ifndef NEFORCE_NETWORK_HTTP_HTTP_CLIENT_MESSAGE_HPP__
#define NEFORCE_NETWORK_HTTP_HTTP_CLIENT_MESSAGE_HPP__
#include "NeForce/core/container/unordered_map.hpp"
#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/time/duration.hpp"
#include "NeForce/network/http/http_session.hpp"
#include "NeForce/network/ports.hpp"
NEFORCE_BEGIN_NAMESPACE__

struct http_client_response {
public:
    uint16_t http_version_major = 1;
    uint16_t http_version_minor = 1;
    bool chunked = false;
    uint64_t content_length = 0;
    string effective_url;
    int redirect_count = 0;
    milliseconds total_time{0};
    milliseconds connect_time{0};
    milliseconds send_time{0};
    milliseconds receive_time{0};
    HTTP_STATUS status = HTTP_STATUS::S2_OK;
    string status_message;
    unordered_map<string, vector<string>> headers;
    string body;
    vector<http_cookie> cookies;

    NEFORCE_NODISCARD string_view header(const string& key) const noexcept {
        const auto it = headers.find(key);
        if (it == headers.end() || it->second.empty()) {
            return "";
        }
        return it->second[0].view();
    }

    NEFORCE_NODISCARD const vector<string>& headers_all(const string& key) const {
        static const vector<string> empty;
        const auto it = headers.find(key);
        return it != headers.end() ? it->second : empty;
    }

    NEFORCE_NODISCARD bool has_header(const string& key) const noexcept { return headers.find(key) != headers.end(); }

    NEFORCE_NODISCARD bool is_success() const noexcept {
        const auto code = static_cast<uint16_t>(status);
        return code >= 200 && code < 300;
    }

    NEFORCE_NODISCARD bool is_redirect() const noexcept {
        const auto code = static_cast<uint16_t>(status);
        return code >= 300 && code < 400;
    }

    NEFORCE_NODISCARD bool is_client_error() const noexcept {
        const auto code = static_cast<uint16_t>(status);
        return code >= 400 && code < 500;
    }

    NEFORCE_NODISCARD bool is_server_error() const noexcept {
        const auto code = static_cast<uint16_t>(status);
        return code >= 500 && code < 600;
    }

    NEFORCE_NODISCARD string_view content_type() const noexcept { return header("Content-Type"); }
};


struct http_client_request {
    HTTP_METHOD method{HTTP_METHOD::GET};
    string host;
    ports port;
    string path = "/";
    string version = "HTTP/1.1";
    unordered_map<string, string> headers;
    unordered_map<string, string> query_params;
    string body;

    NEFORCE_NODISCARD string_view header(const string& key) const noexcept {
        const auto it = headers.find(key);
        if (it == headers.end() || it->second.empty()) {
            return "";
        }
        return it->second.view();
    }

    void set_header(const string& key, string value) { headers[key] = _NEFORCE move(value); }

    void add_query_param(const string& key, string value) { query_params[key] = _NEFORCE move(value); }

    NEFORCE_NODISCARD string build_full_path() const;
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_HTTP_HTTP_CLIENT_MESSAGE_HPP__
