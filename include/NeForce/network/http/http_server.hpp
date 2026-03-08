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
    static constexpr size_t max_header_size = 16 * 1024;
    static constexpr size_t max_body_size = 100 * 1024 * 1024;  // 100MB
    static constexpr int max_forward_count = 5;

protected:
    struct NEFORCE_API session_manager {
        unordered_map<string, session> sessions_;
        mutable mutex mutex_;
        atomic<bool> cleanup_running_;
        thread cleanup_thread_;
        random_mt rand_;
        seconds cleanup_interval_{300};
        size_t max_sessions_{10000};

        string generate_session_id();

        session_manager();
        ~session_manager();

        session* get_session(const string& session_id, bool create = true);
        void remove_session(const string& session_id) noexcept;

        void cleanup_expired_sessions();
        bool session_exists(const string& session_id) const noexcept;

        size_t session_count() const noexcept;
        void set_cleanup_interval(seconds interval) noexcept;
        void set_max_sessions(size_t max) noexcept;
    };

    static string compute_websocket_accept(string_view key);

    static void parse_cookies(string_view cookie_header, http_request& request);
    static void parse_parameters(http_request& request);
    static string build_response_str(const http_response& response);

    static http_request parse_request(
        tcp_socket* client_socket,
        session_manager& manager,
        const HTTP_COOKIE_NAME& name,
        size_t max_header_size = max_header_size,
        size_t max_body_size = max_body_size);

    static void add_session_cookie(
        const http_request& request,
        http_response& response,
        session* session,
        const HTTP_COOKIE_NAME& name);

    static session* get_or_create_session(
        http_request& request,
        bool create,
        session_manager& manager,
        const HTTP_COOKIE_NAME& name);

    static void send_response(tcp_socket* client_socket, const http_response& response);
    static void send_error_response(tcp_socket* client_socket, HTTP_STATUS status, const string& message);
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

public:
    size_t max_header_size_{max_header_size};
    size_t max_body_size_{max_body_size};
    bool enable_websocket_{true};

private:
    void handle_client(socket_type client_socket)  {
        try {
            http_request request = this->parse_request(
                static_cast<tcp_socket*>(&client_socket),
                session_manager_,
                cookie_name_,
                max_header_size_,
                max_body_size_);

            if constexpr (is_same_v<SocketType, ssl_socket>) {
                request.set_header(HTTP_KEY::X_Forwarded_Proto, "https");
            }

            if (enable_websocket_ && this->try_websocket_upgrade(client_socket, request)) {
                return;
            }

            session* sess = this->get_or_create_session(request, true, session_manager_, cookie_name_);
            this->handle_request_with_forward(client_socket, request, sess);
        } catch (const http_exception& e) {
            this->send_error_response(
                static_cast<tcp_socket*>(&client_socket),
                HTTP_STATUS::S4_BAD_REQUEST,
                e.what());
        } catch (const exception& e) {
            this->send_error_response(
                static_cast<tcp_socket*>(&client_socket),
                HTTP_STATUS::S5_INTERNAL_ERROR,
                e.what());
        } catch (...) {
            this->send_error_response(
                static_cast<tcp_socket*>(&client_socket),
                HTTP_STATUS::S5_INTERNAL_ERROR,
                "Unknown internal error");
        }
    }

    bool try_websocket_upgrade(socket_type& client_socket, http_request& request) {
        const string_view upgrade = request.header("Upgrade");
        const string_view connection = request.header("Connection");

        if (upgrade != "websocket" || connection.find("Upgrade") == string::npos) {
            return false;
        }

        const string_view key = request.header("Sec-WebSocket-Key");
        if (key.empty()) {
            return false;
        }

        string accept = compute_websocket_accept(key);

        http_response upgrade_response;
        upgrade_response.status = HTTP_STATUS::S1_SWITCH_PROTOCOL;
        upgrade_response.status_message = "Switching Protocols";
        upgrade_response.set_header("Upgrade", "websocket");
        upgrade_response.set_header("Connection", "Upgrade");
        upgrade_response.set_header("Sec-WebSocket-Accept", move(accept));

        send_response(static_cast<tcp_socket*>(&client_socket), upgrade_response);

        return ws_server_.handle_upgrade(request, _NEFORCE move(client_socket));
    }

    void handle_request_with_forward(socket_type& client_socket, http_request& request, session* sess) {
        int forward_count = 0;

        while (forward_count < max_forward_count) {
            http_response response = router_.handle_request(request);

            if (sess) {
                add_session_cookie(request, response, sess, cookie_name_);
            }

            if (!response.forward_path.empty()) {
                request.path = move(response.forward_path);
                request.parameters.clear();
                parse_parameters(request);
                forward_count++;
                continue;
            }

            send_response(static_cast<tcp_socket*>(&client_socket), response);
            break;
        }

        if (forward_count >= max_forward_count) {
            this->send_error_response(
                static_cast<tcp_socket*>(&client_socket),
                HTTP_STATUS::S5_INTERNAL_ERROR,
                "Too many forwards");
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

    NEFORCE_NODISCARD http_router& router() noexcept { return router_; }
    NEFORCE_NODISCARD const http_router& router() const noexcept { return router_; }

    NEFORCE_NODISCARD websocket_server<socket_type>& websocket() noexcept { return ws_server_; }
    NEFORCE_NODISCARD const websocket_server<socket_type>& websocket() const noexcept { return ws_server_; }

    void set_cookie_name(HTTP_COOKIE_NAME name) noexcept {
        cookie_name_ = move(name);
    }

    NEFORCE_NODISCARD const HTTP_COOKIE_NAME& cookie_name() const noexcept {
        return cookie_name_;
    }

    void set_session_cleanup_interval(const seconds interval) noexcept {
        session_manager_.set_cleanup_interval(interval);
    }

    void set_max_sessions(const size_t max) noexcept {
        session_manager_.set_max_sessions(max);
    }

    NEFORCE_NODISCARD uint16_t port() const noexcept {
        return server_.port();
    }

    NEFORCE_NODISCARD bool is_running() const noexcept {
        return server_.is_running();
    }

    NEFORCE_NODISCARD session* get_session(http_request& request, bool create = false) {
        return get_or_create_session(request, create, session_manager_, cookie_name_);
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
