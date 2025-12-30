#ifndef MSTL_NETWORK_TCP_TCP_SERVER_HPP__
#define MSTL_NETWORK_TCP_TCP_SERVER_HPP__
#include "MSTL/core/async/atomic.hpp"
#include "MSTL/core/async/thread.hpp"
#include "MSTL/core/container/vector.hpp"
#include "MSTL/core/functional/function.hpp"
#include "ssl_socket.hpp"
MSTL_BEGIN_NAMESPACE__

class MSTL_API tcp_server {
public:
#ifdef MSTL_SUPPORT_OPENSSL__
    using handle_sock_t = ssl_socket;
#else
    using handle_sock_t = tcp_socket;
#endif

    using client_handler_t = function<void(handle_sock_t)>;

private:
    tcp_socket server_socket_{};
#ifdef MSTL_SUPPORT_OPENSSL__
    ssl_context ssl_ctx_{};
#endif

    uint16_t port_;
    int backlog_;
    _MSTL atomic_bool running_{false};
    vector<_MSTL thread> worker_threads_{};

    client_handler_t client_handler_{};

private:
    void start_workers(int thread_count);
    void accept_conns();

public:
    explicit tcp_server(uint16_t port, int backlog = 128);
    ~tcp_server() { stop(); }

#ifdef MSTL_SUPPORT_OPENSSL__
    bool load_certificate(const string& cert_file, const string& key_file);
#endif

    void set_client_handler(client_handler_t handler) noexcept {
        client_handler_ = _MSTL move(handler);
    }
    MSTL_NODISCARD const client_handler_t& client_handler() const noexcept {
        return client_handler_;
    }

    MSTL_NODISCARD uint16_t port() const noexcept {
        return port_;
    }
    MSTL_NODISCARD bool is_running() const noexcept {
        return running_;
    }

    bool start(SOCKET_DOMAIN domain = SOCKET_DOMAIN::IPV4,
               SOCKET_TYPE type = SOCKET_TYPE::STREAM,
               SOCKET_PROTOCOL protocol = SOCKET_PROTOCOL::AUTO,
               uint16_t thread_count = 5);

    void stop() noexcept;
};

MSTL_END_NAMESPACE__
#endif // MSTL_NETWORK_TCP_TCP_SERVER_HPP__
