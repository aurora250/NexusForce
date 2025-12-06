#ifndef MSTL_NETWORK_HTTP_HTTP_CLIENT_HPP__
#define MSTL_NETWORK_HTTP_HTTP_CLIENT_HPP__
#include "../url.hpp"
#include "../socket.hpp"
#include "../dns/dns_client.hpp"
#include "http_client_message.hpp"
MSTL_BEGIN_NAMESPACE__

class MSTL_API http_client {
private:
    dns_client dns_{};
    socket sock_{};
    bool connected_ = false;
    string connected_host_;
    uint16_t connected_port_ = 0;

    unordered_map<string, cookie> cookie_jar_; // key: cookie_name@domain_path

    int max_redirect_ = 5;
    _MSTL_CHRONO milliseconds connect_timeout_{5000};
    _MSTL_CHRONO milliseconds read_timeout_{5000};

    bool try_connect(const string& host, uint16_t port, const string& ip, bool ipv6);
    bool connect_domain(const string& host, uint16_t port);
    void close_connection() noexcept;

    string build_request_str(const http_client_request& req, const url& req_url) const;

    bool read_response(string& out_data) const;
    bool parse_response(string_view resp_str, http_client_response& resp) const;

    cookie parse_set_cookie(string_view str, const string& default_domain, const string& default_path) const;

    void update_cookies(const vector<cookie>& resp_cookies, const url& request_url);

    bool parse_chunked_body(string_view chunked, string& decoded) const;
    string build_cookie_header(const url& ) const;

public:
    explicit http_client() = default;
    explicit http_client(dns_client dns) : dns_(_MSTL move(dns)) {}

    ~http_client() { close(); }

    void set_max_redirect(const int max) { max_redirect_ = max; }

    http_client_response request(http_client_request req);

    void close() noexcept { close_connection(); }
};

MSTL_END_NAMESPACE__
#endif // MSTL_NETWORK_HTTP_HTTP_CLIENT_HPP__
