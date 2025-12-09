#ifndef MSTL_NETWORK_HTTP_SERVER_HPP__
#define MSTL_NETWORK_HTTP_SERVER_HPP__
#include "MSTL/core/config/undef_cmacro.hpp"
#include "../socket.hpp"
#include "http_router.hpp"
MSTL_BEGIN_NAMESPACE__

class MSTL_API http_server {
private:
    socket server_socket_{};
    uint16_t port_;
    int backlog_;
    _MSTL atomic_bool running_{false};
#ifdef MSTL_PLATFORM_WINDOWS__
    ::WSADATA wsa_data_{};
#endif
    ::sockaddr_in server_addr_{};
    vector<_MSTL thread> worker_threads_;

    _INNER __session_manager session_manager_;
    HTTP_COOKIE_NAME cookie_name_ = HTTP_COOKIE_NAME::JSESSIONID;

    http_router router_;

private:
    void start_workers(int thread_count);

    void accept_conns();
    void handle_client(const socket& client_socket);

    static void parse_cookies(string_view cookie_header, http_request &request);
    static void parse_parameters(http_request& request);
    static void parse_url_encoded(string_view data, unordered_map<string, string> &params);
    static string url_decode(string_view str);

    http_request parse_request(const socket& client_socket);
    static string build_response_str(const http_response& response);
    static void send_response(const socket& client_socket, const http_response& response);

    void add_session_cookie(const http_request& request, http_response& response, session* session) const;

public:
    explicit http_server(const uint16_t port, const int backlog = 128)
    : port_(port), backlog_(backlog) {
        _MSTL memory_zero(&server_addr_, sizeof(server_addr_));
    }

    ~http_server() { stop(); }

    http_router& get_router() noexcept { return router_; }
    const http_router& get_router() const noexcept { return router_; }

    session* get_session(http_request& request, bool create = false);

    void set_session_cookie_name(HTTP_COOKIE_NAME name) noexcept {
        cookie_name_ = _MSTL move(name);
    }
    const HTTP_COOKIE_NAME& get_session_cookie_name() const noexcept {
        return cookie_name_;
    }


    bool start(SOCKET_DOMAIN domain = SOCKET_DOMAIN::IPV4,
               SOCKET_TYPE type = SOCKET_TYPE::STREAM,
               SOCKET_PROTOCOL protocol = SOCKET_PROTOCOL::AUTO,
               uint32_t thread_count = 5);

    void stop() noexcept;
};


MSTL_END_NAMESPACE__
#endif // MSTL_NETWORK_HTTP_SERVER_HPP__
