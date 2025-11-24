#ifndef MSTL_SERVLET_HPP__
#define MSTL_SERVLET_HPP__
#include "MSTL/core/config/undef_cmacro.hpp"
#include "../socket.hpp"
#include "filter.hpp"
MSTL_BEGIN_NAMESPACE__

MSTL_ERROR_BUILD_FINAL_CLASS(http_exception, link_exception, "Http Actions Failed");


class MSTL_API servlet {
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
    filter_chain filter_chain_;
    HTTP_COOKIE_NAME cookie_name_ = HTTP_COOKIE_NAME::JSESSIONID;

private:
    void start_workers(int thread_count);

    void accept_conns();
    void handle_client(const socket& client_socket);

    static void parse_cookies(const string& cookie_header, http_request &request);
    static void parse_parameters(http_request& request);
    static void parse_url_encoded(string_view data, unordered_map<string, string> &params);

    static string url_decode(string_view str);

protected:
    virtual http_request parse_request(const socket& client_socket);
    virtual string build_response_str(const http_response& response);

    void send_response(const socket& client_socket, const http_response& response);


    void add_filter(filter* filter) { filter_chain_.add_filter(filter); }
    void add_session_cookie(const http_request& request, http_response& response, session* session) const;

    session* get_session(http_request& request, bool create);
    session* get_session(http_request& request) { return get_session(request, false); }


    virtual bool init() { return true; }
    virtual void destroy() noexcept {}

    virtual http_response handle_request(http_request& request);


    virtual void do_get(http_request& request, http_response& response) = 0;
    virtual void do_post(http_request& request, http_response& response) = 0;
    virtual void do_put(http_request& request, http_response& response) { do_post(request, response); }
    virtual void do_delete(http_request& request, http_response& response) { do_post(request, response); }
    virtual void do_head(http_request& request, http_response& response);
    virtual void do_options(http_request& request, http_response& response);
    virtual void do_trace(http_request& request, http_response& response);
    virtual void do_connect(http_request& request, http_response& response);

public:
    explicit servlet(const uint16_t port, const int backlog = 128)
    : port_(port), backlog_(backlog) {
        _MSTL memory_zero(&server_addr_, sizeof(server_addr_));
    }

    virtual ~servlet();


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
#endif // MSTL_SERVLET_HPP__
