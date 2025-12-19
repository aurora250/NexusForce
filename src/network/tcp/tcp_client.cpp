#include <MSTL/network/tcp/tcp_client.hpp>
#include <MSTL/core/system/console.hpp>
#ifdef MSTL_PLATFORM_LINUX__
#include <arpa/inet.h>
#endif
MSTL_BEGIN_NAMESPACE__

bool tcp_client::try_connect(const string& host, const uint16_t port, const string& ip, const bool ipv6) {
    tcp_socket s(
        ipv6 ? SOCKET_DOMAIN::IPV6 : SOCKET_DOMAIN::IPV4,
        SOCKET_TYPE::STREAM,
        SOCKET_PROTOCOL::TCP);

    if (!s.is_valid()) {
        printcln(color::red(), "Socket creation failed");
        return false;
    }
    if (!s.set_send_timeout(send_timeout_) || !s.set_receive_timeout(read_timeout_)) {
        printcln(color::yellow(), "Warning: Failed to set socket timeouts");
    }

    ::sockaddr_storage addr{};
    ::socklen_t addr_len;
    
    if (ipv6) {
        auto *a6 = reinterpret_cast<::sockaddr_in6 *>(&addr);
        a6->sin6_family = AF_INET6;
        ::inet_pton(AF_INET6, ip.c_str(), &a6->sin6_addr);
        a6->sin6_port = ::htons(port);
        addr_len = sizeof(::sockaddr_in6);
    } else {
        auto *a4 = reinterpret_cast<::sockaddr_in *>(&addr);
        a4->sin_family = AF_INET;
        ::inet_pton(AF_INET, ip.c_str(), &a4->sin_addr);
        a4->sin_port = ::htons(port);
        addr_len = sizeof(::sockaddr_in);
    }

    if (!s.connect(reinterpret_cast<::sockaddr *>(&addr), addr_len)) {
        printcln(color::red(), "Connection to ", ip, ":", port, " failed");
        return false;
    }

    sock_ = _MSTL move(s);
    connected_ = true;
    connected_host_ = host;
    connected_port_ = port;
    
    println("Connected to ", host, ":", port, " via IP: ", ip);
    return true;
}

bool tcp_client::connect_domain(const string& host, const uint16_t port) {
    if (connected_ && connected_host_ == host && connected_port_ == port) {
        return true;
    }
    close_connection();

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

    printcln(color::red(), "Failed to connect to ", host, ":", port);
    return false;
}

void tcp_client::close_connection() noexcept {
    if (connected_) {
        sock_.close();
        connected_ = false;
        connected_host_.clear();
        connected_port_ = 0;
    }
}

bool tcp_client::connect(const string& host, const uint16_t port) {
    return connect_domain(host, port);
}

ssize_t tcp_client::send(const void* data, const size_t length) {
    if (!connected_) {
        printcln(color::red(), "Not connected");
        return -1;
    }
    
    ssize_t sent = 0;
    const auto buffer = static_cast<const char*>(data);
    
    while (sent < static_cast<ssize_t>(length)) {
        const ssize_t n = sock_.send(buffer + sent, length - sent);
        if (n <= 0) {
            printcln(color::red(), "Send failed");
            close_connection();
            return n;
        }
        sent += n;
    }
    return sent;
}

ssize_t tcp_client::send(const string& data) {
    return send(data.data(), data.size());
}

ssize_t tcp_client::receive(void* buffer, const size_t length) const {
    if (!connected_) {
        printcln(color::red(), "Not connected");
        return -1;
    }
    return sock_.receive(buffer, length);
}

bool tcp_client::receive_all(string& out_data, const size_t expected_length) {
    if (!connected_) {
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
            const ssize_t n = sock_.receive(buffer, to_receive);
            
            if (n <= 0) {
                if (n == 0) {
                    println("Connection closed by peer");
                } else {
                    printcln(color::red(), "Receive failed");
                }
                close_connection();
                return false;
            }
            
            out_data.append(buffer, n);
            total_received += n;
        }
    } else {
        while (true) {
            const ssize_t n = sock_.receive(buffer, BUFFER_SIZE);
            if (n <= 0) {
                if (n == 0) {
                    break;
                } else {
                    printcln(color::red(), "Receive failed");
                    close_connection();
                    return false;
                }
            }
            
            out_data.append(buffer, n);
        }
    }
    
    return true;
}

tcp_client& tcp_client::operator <<(const string& data) {
    send(data);
    return *this;
}

tcp_client& tcp_client::operator >>(string& data) {
    string temp;
    receive_all(temp);
    data = _MSTL move(temp);
    return *this;
}

void tcp_client::receive_with_callback(function<void(const string&)> callback, const size_t buffer_size) {
    if (!connected_ || !callback) {
        return;
    }
    
    vector<char> buffer(buffer_size);
    
    while (connected_) {
        const ssize_t n = sock_.receive(buffer.data(), buffer.size());
        
        if (n > 0) {
            callback(string(buffer.data(), n));
        } else if (n == 0) {
            println("Connection closed by peer");
            close_connection();
            break;
        } else {
            printcln(color::red(), "Receive failed");
            close_connection();
            break;
        }
    }
}

MSTL_END_NAMESPACE__
