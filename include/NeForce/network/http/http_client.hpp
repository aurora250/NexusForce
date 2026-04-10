#ifndef NEFORCE_NETWORK_HTTP_HTTP_CLIENT_HPP__
#define NEFORCE_NETWORK_HTTP_HTTP_CLIENT_HPP__
#include "NeForce/core/async/mutex.hpp"
#include "NeForce/network/http/http_client_message.hpp"
#include "NeForce/network/tcp/tcp_client.hpp"
#include "NeForce/network/util/url.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

class NEFORCE_API http_client {
public:
    struct config {
        milliseconds connect_timeout{5000};
        milliseconds send_timeout{5000};
        milliseconds receive_timeout{5000};
        uint16_t max_redirects = 5;
        bool follow_redirects = true;
        bool keep_alive = false;
        bool verify_ssl = true;
        size_t max_response_size = 10 * 1024 * 1024; // 10MB
        size_t buffer_size = 8192;
        unordered_map<string, string> default_headers;
        string user_agent{"NexusForce HTTP Client/1.0"_sv};
        string proxy_host;
        ports proxy_port;
    };

    using progress_callback_t = function<void(size_t, size_t)>;
    using error_callback_t = function<void(const exception&)>;
    using time_point = steady_clock::time_point;

    using client_type = ssl_client;

private:
    client_type client_;
    config config_;
    unordered_map<string, http_cookie> cookie_jar_;
    unordered_map<string, string> persistent_headers_;
    mutable mutex mutex_;

    progress_callback_t progress_callback_;
    error_callback_t error_callback_;

private:
    string build_request_str(const http_client_request& req, const url& req_url) const;
    bool send_request(string_view request_str, time_point& send_start);

    optional<http_client_response> read_response(time_point& receive_start, const string& request_host,
                                                 const string& request_path);

    void update_cookies(const vector<http_cookie>& resp_cookies, const url& request_url);
    string build_cookie_header(const url& request_url) const;

    http_client_response do_request(http_client_request request, int redirect_count = 0);
    bool ensure_connected(const string& host, ports port);

public:
    http_client() :
    http_client(config()) {}

    explicit http_client(config config);
    explicit http_client(ssl_context ctx, config config);

    ~http_client() = default;

    http_client(const http_client&) = delete;
    http_client& operator=(const http_client&) = delete;

    http_client(http_client&&) noexcept = default;
    http_client& operator=(http_client&&) noexcept = default;

    void set_config(config cfg) {
        lock<mutex> lk(mutex_);
        config_ = move(cfg);
    }

    const config& get_config() const noexcept { return config_; }

    const client_type& get_client() const noexcept { return client_; }
    client_type& get_client() noexcept { return client_; }

    void set_default_header(const string& key, string value) {
        lock<mutex> lk(mutex_);
        persistent_headers_[key] = move(value);
    }

    void remove_default_header(const string& key) {
        lock<mutex> lk(mutex_);
        persistent_headers_.erase(key);
    }

    void set_max_redirects(uint16_t max) { config_.max_redirects = max; }

    void set_follow_redirects(bool follow) { config_.follow_redirects = follow; }

    void set_timeout(milliseconds timeout) {
        config_.connect_timeout = timeout;
        config_.send_timeout = timeout;
        config_.receive_timeout = timeout;
    }

    void set_proxy(string host, const ports port) {
        config_.proxy_host = move(host);
        config_.proxy_port = port;
    }

    void clear_proxy() {
        config_.proxy_host.clear();
        config_.proxy_port = ports::undef;
    }

    void set_progress_callback(progress_callback_t callback) { progress_callback_ = move(callback); }

    void set_error_callback(error_callback_t callback) { error_callback_ = move(callback); }

    void set_ssl_context(ssl_context ctx);
    void set_verify_ssl(bool verify);

    void clear_cookies() {
        lock<mutex> lk(mutex_);
        cookie_jar_.clear();
    }

    void set_cookie(const http_cookie& c, const string& domain, const string& path = "/");

    NEFORCE_NODISCARD unordered_map<string, http_cookie> get_cookies() const {
        lock<mutex> lk(mutex_);
        return cookie_jar_;
    }

    http_client_response get(const string& url, const unordered_map<string, string>& headers = {});

    http_client_response post(const string& url, const string& body = "",
                              const string& content_type = "application/x-www-form-urlencoded",
                              const unordered_map<string, string>& headers = {});

    http_client_response post_json(const string& url_str, const string& json_body,
                                   const unordered_map<string, string>& headers);

    http_client_response post_form(const string& url_str, const unordered_map<string, string>& form_data,
                                   const unordered_map<string, string>& headers);

    http_client_response put(const string& url, const string& body = "",
                             const string& content_type = "application/x-www-form-urlencoded",
                             const unordered_map<string, string>& headers = {});

    http_client_response del(const string& url, const unordered_map<string, string>& headers = {});

    http_client_response head(const string& url, const unordered_map<string, string>& headers = {});

    http_client_response options(const string& url, const unordered_map<string, string>& headers = {});

    http_client_response patch(const string& url, const string& body = "",
                               const string& content_type = "application/x-www-form-urlencoded",
                               const unordered_map<string, string>& headers = {});

    http_client_response request(http_client_request req);

    bool download_file(const string& url, path output, bool is_binary = true);

    future<http_client_response> request_async(http_client_request req);

    void close();

    NEFORCE_NODISCARD bool is_connected() const noexcept { return client_.is_connected(); }
};

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_HTTP_HTTP_CLIENT_HPP__
