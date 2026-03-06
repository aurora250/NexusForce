#ifndef NEFORCE_NETWORK_HTTP_HTTP_CLIENT_MESSAGE_HPP__
#define NEFORCE_NETWORK_HTTP_HTTP_CLIENT_MESSAGE_HPP__
#include "NeForce/core/container/unordered_map.hpp"
#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/time/duration.hpp"
#include "NeForce/network/http/session.hpp"
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
    HTTP_STATUS status;
    string status_message;
    unordered_map<string, vector<string>> headers;
    string body;
    vector<cookie> cookies;

    NEFORCE_NODISCARD string_view header(const string& key) const noexcept {
        const auto it = headers.find(key);
        if (it == headers.end() || it->second.empty()) return "";
        return it->second[0].view();
    }
};

struct http_client_request {
    HTTP_METHOD method = HTTP_METHOD::GET;
    string host;
    uint16_t port = 80;
    string path = "/";
    string version = "HTTP/1.1";
    unordered_map<string, string> headers;
    string body;

    NEFORCE_NODISCARD string_view header(const string& key) const noexcept {
        const auto it = headers.find(key);
        if (it == headers.end() || it->second.empty()) return "";
        return it->second.view();
    }
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_HTTP_HTTP_CLIENT_MESSAGE_HPP__
