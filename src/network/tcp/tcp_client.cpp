#include <NeForce/network/tcp/tcp_client.hpp>
NEFORCE_BEGIN_NAMESPACE__

bool tcp_client_base::try_connect_to_ip(const string& ip, ports port) {
    const bool is_ipv6_conn = ip.contains(':');

    try {
        auto sock = create_socket();
        if (!sock) {
            return false;
        }

        sock->open(is_ipv6_conn ? ip_address::family::INET6 : ip_address::family::INET4);

        if (!sock->set_send_timeout(send_timeout_) || !sock->set_receive_timeout(recv_timeout_)) {
            sock->close();
            return false;
        }

        auto endpoint = ip_address::parse(ip, port);
        if (!endpoint) {
            sock->close();
            return false;
        }

        if (!sock->connect(*endpoint, connect_timeout_)) {
            sock->close();
            return false;
        }

        socket_ = move(sock);

        if (!post_connect()) {
            socket_.reset();
            return false;
        }

        return true;
    } catch (const exception& e) {
        handle_exception(e);
        return false;
    }
}

bool tcp_client_base::reconnect_if_needed() {
    if (!auto_reconnect_ || connected_host_.empty()) {
        return false;
    }
    if (is_reconnecting_.exchange(true)) {
        return false;
    }

    bool success = false;
    current_reconnect_attempt_ = 0;

    for (int i = 0; i < reconnect_attempts_; ++i) {
        current_reconnect_attempt_ = i + 1;

        try {
            if (connect(connected_host_, connected_port_)) {
                success = true;
                break;
            }
        } catch (const exception& e) {
            if (exception_handler_) {
                exception_handler_(e);
            }
        }

        if (i < reconnect_attempts_ - 1) {
            this_thread::sleep_for(reconnect_delay_ * (i + 1));
        }
    }

    is_reconnecting_ = false;
    current_reconnect_attempt_ = 0;
    return success;
}

void tcp_client_base::set_connect_timeout(const milliseconds timeout) {
    if (timeout <= milliseconds(0)) {
        NEFORCE_THROW_EXCEPTION(value_exception("Connect timeout must be positive"));
    }
    connect_timeout_ = timeout;
}

void tcp_client_base::set_send_timeout(const milliseconds timeout) {
    if (timeout <= milliseconds(0)) {
        NEFORCE_THROW_EXCEPTION(value_exception("Send timeout must be positive"));
    }
    send_timeout_ = timeout;
}

void tcp_client_base::set_recv_timeout(const milliseconds timeout) {
    if (timeout <= milliseconds(0)) {
        NEFORCE_THROW_EXCEPTION(value_exception("Receive timeout must be positive"));
    }
    recv_timeout_ = timeout;
}

void tcp_client_base::set_auto_reconnect(const bool enable, const int max_attempts) {
    if (max_attempts <= 0) {
        NEFORCE_THROW_EXCEPTION(value_exception("Reconnect attempts must be positive"));
    }
    auto_reconnect_ = enable;
    reconnect_attempts_ = max_attempts;
}

void tcp_client_base::set_reconnect_delay(const milliseconds delay) {
    if (delay < milliseconds(0)) {
        NEFORCE_THROW_EXCEPTION(value_exception("Reconnect delay cannot be negative"));
    }
    reconnect_delay_ = delay;
}

bool tcp_client_base::connect(const string& host, ports port) {
    if (host.empty()) {
        handle_exception(value_exception("Host cannot be empty"));
        return false;
    }
    if (!port) {
        handle_exception(value_exception("Port cannot be zero"));
        return false;
    }

    disconnect();

    vector<string> ips;
    const auto addr = ip_address::parse(host, port);

    if (addr.has_value()) {
        if (addr->is_ipv4() || addr->is_ipv6()) {
            ips.push_back(host);
        }
    } else {
        try {
            vector<string> ipv4s, ipv6s;
            try {
                ipv4s = dns_->resolve_a(host.view());
                // NOLINTNEXTLINE(bugprone-empty-catch)
            } catch (...) {
                // ignore
            }
            try {
                ipv6s = dns_->resolve_aaaa(host.view());
                // NOLINTNEXTLINE(bugprone-empty-catch)
            } catch (...) {
                // ignore
            }

            if (prefer_ipv6_) {
                ips.insert(ips.end(), ipv6s.begin(), ipv6s.end());
                ips.insert(ips.end(), ipv4s.begin(), ipv4s.end());
            } else {
                ips.insert(ips.end(), ipv4s.begin(), ipv4s.end());
                ips.insert(ips.end(), ipv6s.begin(), ipv6s.end());
            }
        } catch (const exception& e) {
            handle_exception(e);
            return false;
        }
    }

    if (ips.empty()) {
        handle_exception(network_exception(("Failed to resolve host: " + host).data()));
        return false;
    }

    for (const auto& ip: ips) {
        if (try_connect_to_ip(ip, port)) {
            connected_host_ = host;
            connected_port_ = port;
            if (connect_callback_) {
                connect_callback_(host, ports(port));
            }
            return true;
        }
    }

    handle_exception(network_exception(("Failed to connect to any resolved address for host: " + host).data()));
    return false;
}

void tcp_client_base::disconnect() noexcept {
    if (!socket_) {
        return;
    }

    try {
        pre_disconnect();
        if (disconnect_callback_) {
            disconnect_callback_();
        }
        socket_->close();
        // NOLINTNEXTLINE(bugprone-empty-catch)
    } catch (...) {
        // ignore
    }

    socket_.reset();
    connected_host_.clear();
    connected_port_ = ports::UNDEF;
}

ssize_t tcp_client_base::send(const void* data, size_t length) {
    if (data == nullptr && length > 0) {
        handle_exception(value_exception("Invalid data pointer"));
        return -1;
    }
    if (length == 0) {
        return 0;
    }

    if (!is_connected()) {
        if (!reconnect_if_needed()) {
            return -1;
        }
    }

    try {
        ssize_t sent = socket_->send(memory_view<const char>(static_cast<const char*>(data), length));
        if (sent < 0 && auto_reconnect_) {
            disconnect();
        }
        return sent;
    } catch (const exception& e) {
        handle_exception(e);
        if (auto_reconnect_) {
            disconnect();
        }
        return -1;
    }
}

bool tcp_client_base::send_all(const void* data, size_t length) {
    if (data == nullptr && length > 0) {
        handle_exception(value_exception("Invalid data pointer"));
        return false;
    }
    if (length == 0) {
        return true;
    }

    size_t total_sent = 0;
    const auto* ptr = static_cast<const char*>(data);
    while (total_sent < length) {
        const ssize_t sent = send(ptr + total_sent, length - total_sent);
        if (sent <= 0) {
            return false;
        }
        total_sent += sent;
    }
    return true;
}

ssize_t tcp_client_base::receive(void* buffer, size_t length) {
    if (buffer == nullptr && length > 0) {
        handle_exception(value_exception("Invalid buffer pointer"));
        return -1;
    }
    if (length == 0) {
        return 0;
    }

    if (!is_connected()) {
        if (!reconnect_if_needed()) {
            return -1;
        }
    }

    try {
        ssize_t received = socket_->receive(memory_view<char>(static_cast<char*>(buffer), length));
        if (received < 0 && auto_reconnect_) {
            disconnect();
        }
        return received;
    } catch (const exception& e) {
        handle_exception(e);
        if (auto_reconnect_) {
            disconnect();
        }
        return -1;
    }
}

vector<char> tcp_client_base::receive_all(size_t max_size) {
    if (!is_connected()) {
        if (!reconnect_if_needed()) {
            return {};
        }
    }

    try {
        vector<char> result;
        if (max_size > 0) {
            result.reserve(max_size);
        }

        char buffer[8192];
        while (true) {
            ssize_t n = socket_->receive(memory_view<char>(buffer));
            if (n < 0) {
                if (auto_reconnect_) {
                    disconnect();
                }
                break;
            }
            if (n == 0) {
                break;
            }

            result.insert(result.end(), buffer, buffer + n);
            if (max_size > 0 && result.size() >= max_size) {
                result.resize(max_size);
                break;
            }
        }
        return result;
    } catch (const exception& e) {
        handle_exception(e);
        if (auto_reconnect_) {
            disconnect();
        }
        return {};
    }
}

bool tcp_client_base::receive_exact(memory_view<char> buffer) {
    if (buffer.empty()) {
        return true;
    }
    if (!is_connected()) {
        if (!reconnect_if_needed()) {
            return false;
        }
    }

    size_t total = 0;
    while (total < buffer.size()) {
        try {
            ssize_t n = socket_->receive(buffer.view(total));
            if (n <= 0) {
                if (auto_reconnect_) {
                    disconnect();
                }
                return false;
            }
            total += n;
        } catch (const exception& e) {
            handle_exception(e);
            if (auto_reconnect_) {
                disconnect();
            }
            return false;
        }
    }
    return true;
}

optional<string> tcp_client_base::receive_line(size_t max_length) {
    if (!is_connected()) {
        if (!reconnect_if_needed()) {
            return none;
        }
    }

    try {
        string line;
        line.reserve(128);
        char ch = 0;
        while (line.size() < max_length) {
            const ssize_t n = socket_->receive(memory_view<char>(&ch, 1));
            if (n <= 0) {
                if (auto_reconnect_) {
                    disconnect();
                }
                return none;
            }
            if (ch == '\n') {
                break;
            }
            line += ch;
        }
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        return line;
    } catch (const exception& e) {
        handle_exception(e);
        if (auto_reconnect_) {
            disconnect();
        }
        return none;
    }
}

tcp_socket& tcp_client_base::socket() {
    if (!socket_) {
        NEFORCE_THROW_EXCEPTION(value_exception("Socket is not connected"));
    }
    return *socket_;
}

const tcp_socket& tcp_client_base::socket() const {
    if (!socket_) {
        NEFORCE_THROW_EXCEPTION(value_exception("Socket is not connected"));
    }
    return *socket_;
}

bool ssl_client::post_connect() {
    if (!ssl_ctx_) {
        return true;
    }
    ssl_ctx_->set_verify_mode(verify_peer_ ? SSL_VERIFY_PEER : SSL_VERIFY_NONE);

    if (!ssl_ctx_->is_valid()) {
        handle_exception(ssl_exception("SSL context is invalid"));
        return false;
    }

    try {
        const string& hostname = sni_hostname_.empty() ? connected_host() : sni_hostname_;

        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
        static_cast<ssl_socket*>(socket_.get())->init_client_ssl(*ssl_ctx_, hostname);

        if (verify_peer_) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
            if (!static_cast<ssl_socket*>(socket_.get())->ssl().verify_peer()) {
                handle_exception(ssl_exception("Peer certificate verification failed"));
                return false;
            }
        }

        ssl_initialized_ = true;
        return true;
    } catch (const exception& e) {
        handle_exception(e);
        return false;
    }
}

ssl_client::ssl_client(ssl_context ctx) :
ssl_ctx_(move(ctx)) {
    if (ssl_ctx_) {
        ssl_ctx_->set_options(SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1);
    }
}

void ssl_client::set_ssl_context(ssl_context ctx) {
    if (is_connected()) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("Cannot set SSL context while connected"));
    }
    if (!ctx.is_valid()) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("Invalid SSL context"));
    }
    ctx.set_options(SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1);
    ssl_ctx_ = move(ctx);
}

void ssl_client::set_verify_peer(bool verify) {
    if (is_connected()) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("Cannot change verification mode while connected"));
    }
    verify_peer_ = verify;
    if (ssl_ctx_ && ssl_ctx_->is_valid()) {
        ssl_ctx_->set_verify_mode(verify ? SSL_VERIFY_PEER : SSL_VERIFY_NONE);
    }
}

void ssl_client::set_sni_hostname(string hostname) {
    if (is_connected()) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("Cannot set SNI hostname while connected"));
    }
    sni_hostname_ = move(hostname);
}

bool ssl_client::load_ca_file(const string& ca_file) {
    if (!ssl_ctx_) {
        ssl_ctx_ = ssl_context(ssl_method::TLS_CLIENT);
    }
    return ssl_ctx_->load_verify_locations(ca_file, "");
}

bool ssl_client::load_ca_path(const string& ca_path) {
    if (!ssl_ctx_) {
        ssl_ctx_ = ssl_context(ssl_method::TLS_CLIENT);
    }
    return ssl_ctx_->load_verify_locations("", ca_path);
}

NEFORCE_NODISCARD string ssl_client::peer_certificate_info() const {
    if (!is_connected() || !ssl_initialized_) {
        return "";
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    return static_cast<const ssl_socket*>(socket_.get())->peer_certificate_info();
}

NEFORCE_NODISCARD string ssl_client::cipher_name() const {
    if (!is_connected() || !ssl_initialized_) {
        return "";
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    return static_cast<const ssl_socket*>(socket_.get())->ssl().get_cipher_name();
}

NEFORCE_NODISCARD string ssl_client::protocol_version() const {
    if (!is_connected() || !ssl_initialized_) {
        return "";
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    return static_cast<const ssl_socket*>(socket_.get())->ssl().get_version();
}

ssl_socket& ssl_client::ssl_socket_ref() {
    if (!socket_ || !ssl_initialized_) {
        NEFORCE_THROW_EXCEPTION(value_exception("SSL socket is not available"));
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    return *static_cast<ssl_socket*>(socket_.get());
}

const ssl_socket& ssl_client::ssl_socket_ref() const {
    if (!socket_ || !ssl_initialized_) {
        NEFORCE_THROW_EXCEPTION(value_exception("SSL socket is not available"));
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    return *static_cast<const ssl_socket*>(socket_.get());
}

NEFORCE_END_NAMESPACE__
