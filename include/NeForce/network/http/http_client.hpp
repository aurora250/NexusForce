#ifndef NEFORCE_NETWORK_HTTP_HTTP_CLIENT_HPP__
#define NEFORCE_NETWORK_HTTP_HTTP_CLIENT_HPP__
#include "NeForce/core/async/mutex.hpp"
#include "NeForce/network/http/http_client_message.hpp"
#include "NeForce/network/tcp_client.hpp"
#include "NeForce/network/url.hpp"
NEFORCE_BEGIN_NAMESPACE__

class NEFORCE_API http_client {
public:
    struct config {
        milliseconds connect_timeout{5000};
        milliseconds send_timeout{5000};
        milliseconds receive_timeout{5000};
        uint16_t max_redirects = 5;
        bool follow_redirects = true;
        bool keep_alive = false;
        size_t max_response_size = 10 * 1024 * 1024; // 10MB
        unordered_map<string, string> default_headers;
    };

#ifdef NEFORCE_SUPPORT_OPENSSL
    using client_type = ssl_client;
#else
    using client_type = tcp_client;
#endif

private:
    client_type client_;
    config config_;
    unordered_map<string, cookie> cookie_jar_;
    unordered_map<string, string> persistent_headers_;

    string build_request_str(const http_client_request& req, const url& req_url) const;
    bool send_request(string_view request_str);
    optional<http_client_response> read_response();

    static bool parse_response(string_view resp_str, http_client_response& resp);
    static cookie parse_set_cookie(string_view str, string default_domain, string default_path);
    void update_cookies(const vector<cookie>& resp_cookies, const url& request_url);
    string build_cookie_header(const url& request_url) const;

    static bool parse_chunked_body(string_view chunked, string& decoded);
    static string url_encode(string_view str);
    static string url_decode(string_view str);

    http_client_response do_request(http_client_request req, int redirect_count = 0);

public:
    explicit http_client(config config = {});

#ifdef NEFORCE_SUPPORT_OPENSSL
    explicit http_client(ssl_context ctx, config config = {});
#endif

    ~http_client() = default;

    void set_config(config cfg) {
        config_ = move(cfg);
    }

    const config& get_config() const noexcept {
        return config_;
    }

    void set_default_header(const string& key, string value) {
        persistent_headers_[key] = move(value);
    }

    void remove_default_header(const string& key) {
        persistent_headers_.erase(key);
    }

    void set_max_redirects(uint16_t max) {
        config_.max_redirects = max;
    }

    void set_follow_redirects(bool follow) {
        config_.follow_redirects = follow;
    }

    void set_timeout(milliseconds timeout) {
        config_.connect_timeout = timeout;
        config_.send_timeout = timeout;
        config_.receive_timeout = timeout;
    }

#ifdef NEFORCE_SUPPORT_OPENSSL
    void set_ssl_context(ssl_context ctx);
#endif

    void clear_cookies() {
        cookie_jar_.clear();
    }

    void set_cookie(const cookie& c, const string& domain, const string& path = "/");

    http_client_response get(const string& url, const unordered_map<string, string>& headers = {});

    http_client_response post(const string& url, const string& body = "",
                              const string& content_type = "application/x-www-form-urlencoded",
                              const unordered_map<string, string>& headers = {});

    http_client_response put(const string& url, const string& body = "",
                             const string& content_type = "application/x-www-form-urlencoded",
                             const unordered_map<string, string>& headers = {});

    http_client_response del(const string& url, const unordered_map<string, string>& headers = {});

    http_client_response head(const string& url, const unordered_map<string, string>& headers = {});

    http_client_response options(const string& url, const unordered_map<string, string>& headers = {});

    http_client_response request(http_client_request req);

    future<http_client_response> request_async(http_client_request req);

    void close();
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_HTTP_HTTP_CLIENT_HPP__
