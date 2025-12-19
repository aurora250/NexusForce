#ifndef MSTL_NETWORK_HTTP_HTTP_CLIENT_HPP__
#define MSTL_NETWORK_HTTP_HTTP_CLIENT_HPP__
#include "../url.hpp"
#include "../ssl_socket.hpp"
#include "../ssl_context.hpp"
#include "../dns/dns_client.hpp"
#include "http_client_message.hpp"
MSTL_BEGIN_NAMESPACE__

class MSTL_API http_client {
private:
    dns_client dns_{};
    tcp_socket sock_{};
#ifdef MSTL_SUPPORT_OPENSSL__
    ssl_context ssl_ctx_{};
    ssl_socket ssl_sock_{};
#endif
    bool connected_ = false;
    bool use_ssl_ = false;
    string connected_host_;
    uint16_t connected_port_ = 0;

    unordered_map<string, cookie> cookie_jar_; // key: cookie_name@domain_path

    int max_redirect_ = 5;
    milliseconds connect_timeout_{5000};
    milliseconds read_timeout_{5000};

#ifdef MSTL_SUPPORT_OPENSSL__
    bool verify_peer_ = true;
#endif

    bool try_connect(const string& host, uint16_t port, const string& ip, bool ipv6);
    bool connect_domain(const string& host, uint16_t port);
    void close_connection() noexcept;

    string build_request_str(const http_client_request& req, const url& req_url) const;
    bool send_request(const string& request_str) const;
    bool read_response(string& out_data) const;
    static bool parse_response(string_view resp_str, http_client_response& resp);

    static cookie parse_set_cookie(string_view str, const string& default_domain, const string& default_path) ;
    void update_cookies(const vector<cookie>& resp_cookies, const url& request_url);
    string build_cookie_header(const url& request_url) const;

    static bool parse_chunked_body(string_view chunked, string& decoded);

#ifdef MSTL_SUPPORT_OPENSSL__
    void init_ssl_context();
#endif

public:
    explicit http_client() = default;
    explicit http_client(dns_client dns) : dns_(_MSTL move(dns)) {}

#ifdef MSTL_SUPPORT_OPENSSL__
    explicit http_client(ssl_context ctx) : ssl_ctx_(_MSTL move(ctx)) {}
#endif

    ~http_client() { close(); }

    void set_max_redirect(const int max) { max_redirect_ = max; }
#ifdef MSTL_SUPPORT_OPENSSL__
    void set_verify_peer(const bool verify) { verify_peer_ = verify; }
    void set_ssl_context(ssl_context ctx) { ssl_ctx_ = _MSTL move(ctx); }
#endif

    MSTL_NODISCARD http_client_response request(http_client_request req);

    void close() noexcept { close_connection(); }
};

MSTL_END_NAMESPACE__
#endif // MSTL_NETWORK_HTTP_HTTP_CLIENT_HPP__
