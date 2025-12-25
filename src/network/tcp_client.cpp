#include <MSTL/core/system/console.hpp>
#include <MSTL/network/tcp_client.hpp>
#ifdef MSTL_PLATFORM_LINUX__
#include <arpa/inet.h>
#endif
MSTL_BEGIN_NAMESPACE__

bool tcp_client::try_connect(
    const string& host, const uint16_t port,
    const string& ip, const bool ipv6) {
#ifdef MSTL_PLATFORM_WINDOWS__
    winsock_initialized();
#endif
    tcp_socket s(
        ipv6 ? SOCKET_DOMAIN::IPV6 : SOCKET_DOMAIN::IPV4,
        SOCKET_TYPE::STREAM,
        SOCKET_PROTOCOL::TCP);

    if (!s.set_send_timeout(send_timeout_) ||
        !s.set_receive_timeout(receive_timeout_)) {
        return false;
    }

    if (ipv6) {
        if (!s.connect_ipv6(ip.c_str(), port)) {
            return false;
        }
    } else {
        if (!s.connect_ipv4(ip.c_str(), port)) {
            return false;
        }
    }

    socket_ = handle_sock_t(move(s));
#ifdef MSTL_SUPPORT_OPENSSL__
    if (ssl_ctx_.context()) {
        socket_.set_context(ssl_ctx_);
        if (!socket_.accept()) {
            return false;
        }
    }
#endif

    connected_host_ = host;
    connected_port_ = port;
    return true;
}

tcp_client::tcp_client(dns_client dns)
: dns_(_MSTL move(dns)) {}

#ifdef MSTL_SUPPORT_OPENSSL__
tcp_client::tcp_client(ssl_context ctx)
: ssl_ctx_(_MSTL move(ctx)) {}
#endif

tcp_client::~tcp_client() {
    close();
}

bool tcp_client::connect(const string& host, const uint16_t port) {
    if (is_connected()) {
        return true;
    }
    close();

    if (tcp_socket::is_ipv4(host.data())) {
        if (try_connect(host, port, host, false)) {
            return true;
        }
    } else if (tcp_socket::is_ipv6(host.data())) {
        if (try_connect(host, port, host, true)) {
            return true;
        }
    } else {
        vector<string> ips = dns_.resolve_a(host);
        for (const auto &ip : ips) {
            if (try_connect(host, port, ip, false)) {
                return true;
            }
        }
        vector<string> ips6 = dns_.resolve_aaaa(host);
        for (const auto &ip : ips6) {
            if (try_connect(host, port, ip, true)) {
                return true;
            }
        }
    }
    return false;
}

void tcp_client::close() noexcept {
    if (is_connected()) {
        socket_.close();
        connected_host_.clear();
        connected_port_ = 0;
    }
}

ssize_t tcp_client::send(const void* data, const size_t length) {
    if (!is_connected()) {
        printcln(color::red(), "Not connected");
        return -1;
    }
    
    ssize_t sent = 0;
    const auto buffer = static_cast<const char*>(data);
    
    while (sent < static_cast<ssize_t>(length)) {
        const ssize_t n = socket_.send(buffer + sent, length - sent);
        if (n <= 0) {
            printcln(color::red(), "Send failed");
            close();
            return n;
        }
        sent += n;
    }
    return sent;
}

ssize_t tcp_client::send(const string_view data) {
    return send(data.data(), data.size());
}

ssize_t tcp_client::receive(void* buffer, const size_t length) const {
    if (!is_connected()) {
        printcln(color::red(), "Not connected");
        return -1;
    }
    return socket_.receive(buffer, length);
}

bool tcp_client::receive_all(string& out_data, const size_t expected_length) {
    if (!is_connected()) {
        printcln(color::red(), "Not connected");
        return false;
    }
    
    constexpr size_t BUFFER_SIZE = 8192;
    char buffer[BUFFER_SIZE];
    out_data.clear();
    
    if (expected_length > 0) {
        out_data.reserve(expected_length);
        size_t total_received = 0;
        
        while (total_received < expected_length) {
            const size_t to_receive = _MSTL min(BUFFER_SIZE, expected_length - total_received);
            const ssize_t n = socket_.receive(buffer, to_receive);
            
            if (n <= 0) {
                if (n == 0) {
                    println("Connection closed by peer");
                } else {
                    printcln(color::red(), "Receive failed");
                }
                close();
                return false;
            }
            
            out_data.append(buffer, n);
            total_received += n;
        }
    } else {
        while (true) {
            const ssize_t n = socket_.receive(buffer, BUFFER_SIZE);
            if (n <= 0) {
                if (n == 0) {
                    break;
                } else {
                    printcln(color::red(), "Receive failed");
                    close();
                    return false;
                }
            }
            
            out_data.append(buffer, n);
        }
    }
    
    return true;
}

void tcp_client::receive_with_callback(function<void(const string&)> callback, const size_t buffer_size) {
    if (!is_connected() || !callback) {
        return;
    }
    
    vector<char> buffer(buffer_size);
    
    while (is_connected()) {
        const ssize_t n = socket_.receive(buffer.data(), buffer.size());
        
        if (n > 0) {
            callback(string(buffer.data(), n));
        } else if (n == 0) {
            println("Connection closed by peer");
            close();
            break;
        } else {
            printcln(color::red(), "Receive failed");
            close();
            break;
        }
    }
}

MSTL_END_NAMESPACE__
