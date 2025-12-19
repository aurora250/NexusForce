#include <MSTL/network/tcp/tcp_server.hpp>
#include <MSTL/core/system/console.hpp>
MSTL_BEGIN_NAMESPACE__

void tcp_server::start_workers(const int thread_count) {
    for (int i = 0; i < thread_count; ++i) {
        worker_threads_.emplace_back(&tcp_server::accept_conns, this);
    }
}

void tcp_server::accept_conns() const {
    while (running_) {
        tcp_socket client_socket = server_socket_.accept();
        if (!client_socket.is_valid()) {
            if (running_) {
                println("accept failed");
            }
            continue;
        }
        
        try {
            if (client_handler_) {
                client_handler_(_MSTL move(client_socket));
            } else {
                client_socket.close();
            }
        } catch (const exception& e) {
            println("Error handling client: ", e);
        }
    }
}

tcp_server::tcp_server(const uint16_t port, const int backlog) 
    : port_(port), backlog_(backlog) {
    _MSTL memory_zero(&server_addr_, sizeof(server_addr_));
}

bool tcp_server::start(const SOCKET_DOMAIN domain, const SOCKET_TYPE type,
    const SOCKET_PROTOCOL protocol, const uint16_t thread_count) {
    if (running_) return true;

#ifdef MSTL_PLATFORM_WINDOWS__
    if (::WSAStartup(MAKEWORD(2, 2), &wsa_data_) != 0) {
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

    server_addr_.sin_family = AF_INET;
    server_addr_.sin_addr.s_addr = INADDR_ANY;
    server_addr_.sin_port = ::htons(port_);

    if (!server_socket_.bind(server_addr_)) {
        printcln(color::red(), "bind failed");
        return false;
    }

    if (!server_socket_.listen(backlog_)) {
        printcln(color::red(), "listen failed");
        return false;
    }

    running_ = true;
    start_workers(static_cast<int32_t>(thread_count));
    println("TCP server started on port ", port_);
    return true;
}

void tcp_server::stop() noexcept {
    if (!running_) return;

    running_ = false;
    server_socket_.close();
    
#ifdef MSTL_PLATFORM_WINDOWS__
    ::WSACleanup();
#endif

    for (auto& t : worker_threads_) {
        if (t.joinable()) t.join();
    }
    worker_threads_.clear();
    
    println("TCP server stopped");
}

MSTL_END_NAMESPACE__
