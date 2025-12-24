#ifndef MSTL_NETWORK_HTTP_HTTP_CLIENT_HPP__
#define MSTL_NETWORK_HTTP_HTTP_CLIENT_HPP__
#include "../dns/dns_client.hpp"
#include "../tcp_client.hpp"
#include "../url.hpp"
#include "http_client_message.hpp"
MSTL_BEGIN_NAMESPACE__

class MSTL_API http_client {
private:
    tcp_client client_;
    unordered_map<string, cookie> cookie_jar_; // key: cookie_name@domain_path
    uint16_t max_redirect_ = 5;

    string build_request_str(const http_client_request& req, const url& req_url) const;
    bool send_request(string_view request_str);
    bool read_response(string& out_data) const;
    static bool parse_response(string_view resp_str, http_client_response& resp);

    static cookie parse_set_cookie(string_view str, string default_domain, string default_path) ;
    void update_cookies(const vector<cookie>& resp_cookies, const url& request_url);
    string build_cookie_header(const url& request_url) const;

    static bool parse_chunked_body(string_view chunked, string& decoded);

public:
    explicit http_client() = default;
    explicit http_client(dns_client dns) : client_(_MSTL move(dns)) {}
#ifdef MSTL_SUPPORT_OPENSSL__
    explicit http_client(ssl_context ctx) : client_(_MSTL move(ctx)) {}
#endif
    ~http_client() = default;

    void set_max_redirect(const uint16_t max) { max_redirect_ = max; }
#ifdef MSTL_SUPPORT_OPENSSL__
    void set_ssl_context(ssl_context ctx) { client_.set_ssl_context(_MSTL move(ctx)); }
#endif

    MSTL_NODISCARD http_client_response request(http_client_request req);

    void close() noexcept { client_.close(); }
};

MSTL_END_NAMESPACE__
#endif // MSTL_NETWORK_HTTP_HTTP_CLIENT_HPP__
