#ifndef NEFORCE_NETWORK_HTTP_SERVER_HPP__
#define NEFORCE_NETWORK_HTTP_SERVER_HPP__
#include "NeForce/core/numeric/random.hpp"
#include "NeForce/network/http/http_router.hpp"
#include "NeForce/network/tcp_server.hpp"
#include "NeForce/network/websocket.hpp"
NEFORCE_BEGIN_NAMESPACE__

struct NEFORCE_API http_server_base {
public:
    static constexpr string_view websocket_guid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

protected:
    struct NEFORCE_API session_manager {
        unordered_map<string, _NEFORCE session> sessions_;
        mutable mutex mutex_;
        atomic<bool> cleanup_running_;
        thread cleanup_thread_;
        random_mt rand_;

        string generate_session_id();

        session_manager();

        ~session_manager();

        _NEFORCE session* get_session(const string& session_id, bool create = true);

        void remove_session(const string& session_id) noexcept;

        void cleanup_expired_sessions();

        bool session_exists(const string& session_id) const noexcept;
    };

    static string compute_websocket_accept(string_view key);

    static void parse_cookies(string_view cookie_header, http_request& request);

    static void parse_parameters(http_request& request);

    static string build_response_str(const http_response& response);

    static void add_session_cookie(
        const http_request& request, http_response& response,
        _NEFORCE session* session, const HTTP_COOKIE_NAME& name);

    static http_request parse_request(
        tcp_socket* client_socket,
        session_manager& manager,
        const HTTP_COOKIE_NAME& name);

    static _NEFORCE session* session(
        http_request& request,
        bool create,
        session_manager& manager,
        const HTTP_COOKIE_NAME& name);

    static void send_response(tcp_socket* client_socket, const http_response& response);
};


template <typename SocketType>
class basic_http_server final : public http_server_base {
    static_assert(is_base_of_v<tcp_socket, SocketType>, "SocketType must be a tcp_socket");

public:
    using socket_type = SocketType;
    using server_type = basic_tcp_server<socket_type>;

private:
    server_type server_;
    http_router router_;
    websocket_server<socket_type> ws_server_;

    session_manager session_manager_;
    HTTP_COOKIE_NAME cookie_name_{HTTP_COOKIE_NAME::JSESSIONID};

private:
    void handle_client(socket_type client_socket)  {
        try {
            http_request request = http_server_base::parse_request(
                static_cast<tcp_socket*>(&client_socket), session_manager_, cookie_name_);
            if (client_socket.is_ssl()) {
                request.set_header(HTTP_KEY::X_Forwarded_Proto, "https");
            }

            const string_view upgrade = request.header("Upgrade");
            const string_view connection = request.header("Connection");
            if (upgrade == "websocket" && connection.find("Upgrade") != string::npos) {
                const string_view key = request.header("Sec-WebSocket-Key");
                if (!key.empty()) {
                    string accept = compute_websocket_accept(key);

                    http_response upgrade_response;
                    upgrade_response.status = HTTP_STATUS::S1_SWITCH_PROTOCOL;
                    upgrade_response.set_header("Upgrade", "websocket");
                    upgrade_response.set_header("Connection", "Upgrade");
                    upgrade_response.set_header("Sec-WebSocket-Accept", accept);
                    this->send_response(static_cast<tcp_socket*>(&client_socket), upgrade_response);

                    if (ws_server_.handle_upgrade(request, _NEFORCE move(client_socket))) {
                        return;
                    }
                }
            }

            _NEFORCE session* sess = http_server_base::session(request, true, session_manager_, cookie_name_);
            int forward_count = 0;
            constexpr int MAX_FORWARD = 5;

            do {
                http_response response = router_.handle_request(request);
                if (sess) {
                    add_session_cookie(request, response, sess, cookie_name_);
                }

                if (!response.forward_path.empty() && forward_count < MAX_FORWARD) {
                    request.path = response.forward_path;
                    forward_count++;
                    continue;
                }

                this->send_response(static_cast<tcp_socket*>(&client_socket), response);
                break;
            } while (true);
        } catch (const exception& e) {
            http_response error_response;
            error_response.status = HTTP_STATUS::S5_INTERNAL_ERROR;
            error_response.status_message = "Internal Server Error";
            error_response.body = e.what();
            this->send_response(static_cast<tcp_socket*>(&client_socket), error_response);
        }
    }

public:
    explicit basic_http_server(uint16_t port, int backlog = 128)
    : server_(port, backlog) {
        server_.set_client_handler([this](socket_type sock) {
            this->handle_client(_NEFORCE move(sock));
        });
    }

    ~basic_http_server() = default;

    basic_http_server(const basic_http_server&) = delete;
    basic_http_server& operator=(const basic_http_server&) = delete;

    basic_http_server(basic_http_server&&) noexcept = default;
    basic_http_server& operator =(basic_http_server&&) noexcept = default;

#ifdef NEFORCE_SUPPORT_OPENSSL
    bool load_certificate(const string& cert_file, const string& key_file) {
        return server_.load_certificate(cert_file, key_file);
    }
#endif

    http_router& router() noexcept { return router_; }
    const http_router& router() const noexcept { return router_; }

    websocket_server<socket_type>& websocket() noexcept { return ws_server_; }

    void set_cookie_name(HTTP_COOKIE_NAME name) noexcept {
        cookie_name_ = move(name);
    }
    const HTTP_COOKIE_NAME& cookie_name() const noexcept {
        return cookie_name_;
    }

    uint16_t port() const noexcept { return server_.port(); }
    bool is_running() const noexcept { return server_.is_running(); }

    _NEFORCE session* session(http_request& request, bool create = false) {
        return http_server_base::session(request, create, session_manager_, cookie_name_);
    }

    bool start(int backlog = SOMAXCONN) {
        return server_.start(backlog);
    }

    void stop() noexcept {
        server_.stop();
    }
};

template class basic_http_server<tcp_socket>;
#ifdef NEFORCE_SUPPORT_OPENSSL
template class basic_http_server<ssl_socket>;
#endif

using http_server = basic_http_server<tcp_socket>;
#ifdef NEFORCE_SUPPORT_OPENSSL
using https_server = basic_http_server<ssl_socket>;
#endif

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_HTTP_SERVER_HPP__
