#include <MSTL/core/system/console.hpp>
#include <MSTL/network/tcp_server.hpp>
MSTL_BEGIN_NAMESPACE__

void tcp_server::start_workers(const int thread_count) {
    for (int i = 0; i < thread_count; ++i) {
        worker_threads_.emplace_back(&tcp_server::accept_conns, this);
    }
}

void tcp_server::accept_conns() const {
    while (running_) {
        tcp_socket tcpsock = server_socket_.accept();
        if (!tcpsock.is_valid()) {
            if (running_) {
                println("accept failed");
            }
            continue;
        }

#ifdef MSTL_SUPPORT_OPENSSL__
        handle_sock_t client_sock(move(tcpsock));
        if (ssl_ctx_.is_valid()) {
            client_sock.set_context(ssl_ctx_);
            if (!client_sock.accept()) {
                continue;
            }
        }
#else
        handle_sock_t client_sock = move(tcpsock);
#endif
        try {
            client_handler_(move(client_sock));
        } catch (const exception& e) {
            println(e);
        }
    }
}

tcp_server::tcp_server(const uint16_t port, const int backlog) 
: port_(port), backlog_(backlog) {}

#ifdef MSTL_SUPPORT_OPENSSL__
bool tcp_server::load_certificate(const string& cert_file, const string& key_file) {
    if (!cert_file.empty() && !key_file.empty()) {
        return ssl_ctx_.load_certificate(cert_file, key_file);
    }
    return false;
}
#endif

bool tcp_server::start(const SOCKET_DOMAIN domain, const SOCKET_TYPE type,
    const SOCKET_PROTOCOL protocol, const uint16_t thread_count) {
    if (running_) return true;

#ifdef MSTL_PLATFORM_WINDOWS__
    if (!winsock_initialized()) {
        printcln(color::red(), "WSAStartup failed");
        return false;
    }
#endif

    server_socket_ = _MSTL move(tcp_socket(domain, type, protocol));
    if (!server_socket_.is_valid()) {
        printcln(color::red(), "socket creation failed");
        return false;
    }

    if (!server_socket_.reuse_addr()) {
        printcln(color::red(), "setsockopt failed");
        return false;
    }
    if (!server_socket_.bind(port_)) {
        printcln(color::red(), "bind failed");
        return false;
    }
    if (!server_socket_.listen(backlog_)) {
        printcln(color::red(), "listen failed");
        return false;
    }

    running_ = true;
    start_workers(static_cast<int32_t>(thread_count));
    return true;
}

void tcp_server::stop() noexcept {
    if (!running_) return;

    running_ = false;
    server_socket_.close();

    for (auto& t : worker_threads_) {
        if (t.joinable()) t.join();
    }
    worker_threads_.clear();
}

MSTL_END_NAMESPACE__
