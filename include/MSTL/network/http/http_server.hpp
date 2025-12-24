#ifndef MSTL_NETWORK_HTTP_SERVER_HPP__
#define MSTL_NETWORK_HTTP_SERVER_HPP__
#include "../tcp_server.hpp"
#include "../ssl_socket.hpp"
#include "http_router.hpp"
MSTL_BEGIN_NAMESPACE__

class MSTL_API http_server {
private:
    using handle_sock_t = tcp_server::handle_sock_t;

    tcp_server server_;
    http_router router_;

    _INNER session_manager session_manager_;
    HTTP_COOKIE_NAME cookie_name_{HTTP_COOKIE_NAME::JSESSIONID};

private:
    void handle_client(handle_sock_t client_socket);

    static void parse_cookies(string_view cookie_header, http_request& request);
    static void parse_parameters(http_request& request);
    static void parse_url_encoded(string_view data, unordered_map<string, string>& params);
    static string url_decode(string_view str);

    http_request parse_request(const handle_sock_t& client_socket);
    static string build_response_str(const http_response& response);
    static void send_response(const handle_sock_t& client_socket, const http_response& response);

    void add_session_cookie(const http_request& request, http_response& response, _MSTL session* session) const;

public:
    explicit http_server(uint16_t port, int backlog = 128);
    ~http_server() = default;

#ifdef MSTL_SUPPORT_OPENSSL__
    bool load_certificate(const string& cert_file, const string& key_file) {
        return server_.load_certificate(cert_file, key_file);
    }
#endif

    MSTL_NODISCARD http_router& router() noexcept { return router_; }
    MSTL_NODISCARD const http_router& router() const noexcept { return router_; }

    MSTL_NODISCARD _MSTL session* session(http_request& request, bool create = false);

    void set_cookie_name(HTTP_COOKIE_NAME name) noexcept {
        cookie_name_ = _MSTL move(name);
    }
    MSTL_NODISCARD const HTTP_COOKIE_NAME& cookie_name() const noexcept {
        return cookie_name_;
    }

    MSTL_NODISCARD uint16_t port() const noexcept { return server_.port(); }
    MSTL_NODISCARD bool is_running() const noexcept { return server_.is_running(); }

    bool start(SOCKET_DOMAIN domain = SOCKET_DOMAIN::IPV4,
               SOCKET_TYPE type = SOCKET_TYPE::STREAM,
               SOCKET_PROTOCOL protocol = SOCKET_PROTOCOL::AUTO,
               uint16_t thread_count = 5) {
        return server_.start(domain, type, protocol, thread_count);
    }

    void stop() noexcept { server_.stop(); }
};

MSTL_END_NAMESPACE__
#endif // MSTL_NETWORK_HTTP_SERVER_HPP__
