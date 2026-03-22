#ifndef NEFORCE_NETWORK_TCP_TCP_CLIENT_HPP__
#define NEFORCE_NETWORK_TCP_TCP_CLIENT_HPP__
#include "NeForce/network/dns/dns_client.hpp"
#ifdef NEFORCE_SUPPORT_OPENSSL
#include "NeForce/network/socket/ssl_socket.hpp"
#endif
NEFORCE_BEGIN_NAMESPACE__

template <typename SocketT>
class basic_tcp_client {
    static_assert(is_base_of_v<tcp_socket, SocketT>, "SocketT must derive from tcp_socket");

public:
    using socket_type = SocketT;
    using duration = milliseconds;
    using exception_handler_t = function<void(const exception&)>;
    using connect_callback_t = function<void(const string&, uint16_t)>;
    using disconnect_callback_t = function<void()>;

private:
    dns_client dns_;
    optional<socket_type> socket_;

    string connected_host_;
    uint16_t connected_port_ = 0;
    int reconnect_attempts_ = 3;
    atomic<int> current_reconnect_attempt_{0};

    duration connect_timeout_{5000};
    duration send_timeout_{5000};
    duration recv_timeout_{5000};
    duration reconnect_delay_{1000};

    bool auto_reconnect_ = false;
    bool prefer_ipv6_ = false;
    atomic<bool> is_reconnecting_{false};

protected:
    connect_callback_t connect_callback_;
    disconnect_callback_t disconnect_callback_;
    exception_handler_t exception_handler_;

private:
    bool try_connect_to_ip(const string& ip, const uint16_t port) {
        const bool is_ipv6_conn = ip.find(':') != string::npos;

        try {
            socket_type sock;
            auto* sockp = static_cast<tcp_socket*>(&sock);

            sockp->open(is_ipv6_conn ? AF_INET6 : AF_INET);

            if (!sockp->set_send_timeout(send_timeout_) ||
                !sockp->set_receive_timeout(recv_timeout_)) {
                sockp->close();
                return false;
            }

            auto endpoint = ip_address::parse(ip, port);
            if (!endpoint) {
                sockp->close();
                return false;
            }

            if (!sock.connect(*endpoint, connect_timeout_)) {
                sockp->close();
                return false;
            }

            socket_ = _NEFORCE move(sock);

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

protected:
    virtual bool post_connect() { return true; }
    virtual void pre_disconnect() {}

    bool reconnect_if_needed() {
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

    NEFORCE_ALWAYS_INLINE void handle_exception(const exception& e) const {
        if (exception_handler_) {
            exception_handler_(e);
        }
    }

public:
    basic_tcp_client() = default;

    explicit basic_tcp_client(dns_client dns)
    : dns_(_NEFORCE move(dns)) {}

    virtual ~basic_tcp_client() {
        disconnect();
    }

    basic_tcp_client(const basic_tcp_client&) = delete;
    basic_tcp_client& operator =(const basic_tcp_client&) = delete;

    basic_tcp_client(basic_tcp_client&&) noexcept = default;
    basic_tcp_client& operator =(basic_tcp_client&&) noexcept = default;

    void set_connect_timeout(const duration timeout) {
        if (timeout <= duration(0)) {
            NEFORCE_THROW_EXCEPTION(value_exception("Connect timeout must be positive"));
        }
        connect_timeout_ = timeout;
    }

    NEFORCE_NODISCARD duration connect_timeout() const noexcept {
        return connect_timeout_;
    }

    void set_send_timeout(const duration timeout) {
        if (timeout <= duration(0)) {
            NEFORCE_THROW_EXCEPTION(value_exception("Connect timeout must be positive"));
        }
        send_timeout_ = timeout;
    }

    NEFORCE_NODISCARD duration send_timeout() const noexcept {
        return send_timeout_;
    }

    void set_recv_timeout(const duration timeout) {
        if (timeout <= duration(0)) {
            NEFORCE_THROW_EXCEPTION(value_exception("Connect timeout must be positive"));
        }
        recv_timeout_ = timeout;
    }

    NEFORCE_NODISCARD duration recv_timeout() const noexcept {
        return recv_timeout_;
    }

    void set_auto_reconnect(const bool enable, const int max_attempts = 3) {
        if (max_attempts <= 0) {
            NEFORCE_THROW_EXCEPTION(value_exception("Reconnect attempts must be positive"));
        }
        auto_reconnect_ = enable;
        reconnect_attempts_ = max_attempts;
    }

    NEFORCE_NODISCARD bool is_auto_reconnect() const noexcept {
        return auto_reconnect_;
    }

    NEFORCE_NODISCARD int reconnect_attempts() const noexcept {
        return reconnect_attempts_;
    }

    NEFORCE_NODISCARD int current_reconnect_attempt() const noexcept {
        return current_reconnect_attempt_;
    }

    void set_reconnect_delay(const duration delay) {
        if (delay < duration(0)) {
            NEFORCE_THROW_EXCEPTION(value_exception("Reconnect delay cannot be negative"));
        }
        reconnect_delay_ = delay;
    }

    NEFORCE_NODISCARD duration reconnect_delay() const noexcept {
        return reconnect_delay_;
    }

    void set_prefer_ipv6(const bool prefer) noexcept {
        prefer_ipv6_ = prefer;
    }

    NEFORCE_NODISCARD bool prefer_ipv6() const noexcept {
        return prefer_ipv6_;
    }

    void set_dns_server(dns_client::config cfg) {
        dns_.set_config(move(cfg));
    }

    void set_exception_handler(exception_handler_t handler) {
        exception_handler_ = move(handler);
    }

    void set_connect_callback(connect_callback_t callback) {
        connect_callback_ = move(callback);
    }

    void set_disconnect_callback(disconnect_callback_t callback) {
        disconnect_callback_ = move(callback);
    }

    virtual bool connect(const string& host, uint16_t port) {
        if (host.empty()) {
            handle_exception(value_exception("Host cannot be empty"));
            return false;
        }

        if (port == 0) {
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
                vector<string> ipv4s;
                vector<string> ipv6s;

                try {
                    ipv4s = dns_.resolve_a(host.view());
                } catch (...) {
                    // ignore
                }

                try {
                    ipv6s = dns_.resolve_aaaa(host.view());
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

        for (const auto& ip : ips) {
            if (try_connect_to_ip(ip, port)) {
                connected_host_ = host;
                connected_port_ = port;

                if (connect_callback_) {
                    connect_callback_(host, _NEFORCE move(port));
                }

                return true;
            }
        }

        handle_exception(network_exception(("Failed to connect to any resolved address for host: " + host).data()));
        return false;
    }

    void disconnect() noexcept {
        if (!socket_.has_value()) {
            return;
        }

        try {
            pre_disconnect();

            if (disconnect_callback_) {
                disconnect_callback_();
            }

            socket_->close();
        } catch (...) {
            // ignore
        }

        socket_.reset();
        connected_host_.clear();
        connected_port_ = 0;
    }

    ssize_t send(const void* data, const size_t length) {
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
            const ssize_t sent = socket_->send(memory_view<const char>(static_cast<const char*>(data), length));
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

    ssize_t send(const string_view data) {
        return send(data.data(), data.size());
    }

    bool send_all(const void* data, const size_t length) {
        if (data == nullptr && length > 0) {
            handle_exception(value_exception("Invalid data pointer"));
            return false;
        }

        if (length == 0) {
            return true;
        }

        size_t total_sent = 0;
        const auto ptr = static_cast<const char*>(data);

        while (total_sent < length) {
            const ssize_t sent = send(ptr + total_sent, length - total_sent);
            if (sent <= 0) {
                return false;
            }
            total_sent += sent;
        }

        return true;
    }

    bool send_all(const string_view data) {
        return send_all(data.data(), data.size());
    }

    ssize_t receive(void* buffer, const size_t length) {
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
            const ssize_t received = socket_->receive(memory_view<char>(static_cast<char*>(buffer), length));
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

    vector<char> receive_all(const size_t max_size = 0) {
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
                const ssize_t n = socket_->receive(memory_view<char>(buffer));

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

    bool receive_exact(memory_view<const char> buffer) {
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
                const ssize_t n = socket_->receive(buffer.view(total));

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

    optional<string> receive_line(const size_t max_length = 8192) {
        if (!is_connected()) {
            if (!reconnect_if_needed()) {
                return none;
            }
        }

        try {
            string line;
            line.reserve(128);
            char ch;

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

    NEFORCE_NODISCARD bool is_connected() const noexcept {
        return socket_.has_value() && socket_->is_open();
    }

    NEFORCE_NODISCARD bool is_reconnecting() const noexcept {
        return is_reconnecting_;
    }

    NEFORCE_NODISCARD const string& connected_host() const noexcept {
        return connected_host_;
    }

    NEFORCE_NODISCARD uint16_t connected_port() const noexcept {
        return connected_port_;
    }

    NEFORCE_NODISCARD socket_type& socket() {
        if (!socket_.has_value()) {
            NEFORCE_THROW_EXCEPTION(value_exception("Socket is not connected"));
        }
        return *socket_;
    }

    NEFORCE_NODISCARD const socket_type& socket() const {
        if (!socket_.has_value()) {
            NEFORCE_THROW_EXCEPTION(value_exception("Socket is not connected"));
        }
        return *socket_;
    }

    NEFORCE_NODISCARD dns_client& get_dns_client() noexcept {
        return dns_;
    }

    NEFORCE_NODISCARD const dns_client& get_dns_client() const noexcept {
        return dns_;
    }
};


using tcp_client = basic_tcp_client<tcp_socket>;


#ifdef NEFORCE_SUPPORT_OPENSSL

class ssl_client final : public basic_tcp_client<ssl_socket> {
private:
    optional<ssl_context> ssl_ctx_;
    string sni_hostname_;
    bool verify_peer_ = true;
    bool ssl_initialized_ = false;

protected:
    bool post_connect() override {
        if (!ssl_ctx_) {
            ssl_ctx_ = ssl_context(ssl_method::TLS_CLIENT);
            ssl_ctx_->set_verify_mode(verify_peer_ ? SSL_VERIFY_PEER : SSL_VERIFY_NONE);
        }

        if (!ssl_ctx_->is_valid()) {
            handle_exception(ssl_exception("SSL context is invalid"));
            return false;
        }

        try {
            const string& hostname = sni_hostname_.empty() ? connected_host() : sni_hostname_;
            socket().init_client_ssl(*ssl_ctx_, hostname);

            if (verify_peer_) {
                if (!socket().ssl().verify_peer()) {
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

    void pre_disconnect() override {
        ssl_initialized_ = false;
    }

public:
    ssl_client() = default;

    explicit ssl_client(ssl_context ctx)
    : ssl_ctx_(move(ctx)) {
        if (ssl_ctx_) {
            ssl_ctx_->set_options(
                SSL_OP_NO_SSLv2 |
                SSL_OP_NO_SSLv3 |
                SSL_OP_NO_TLSv1 |
                SSL_OP_NO_TLSv1_1
            );
        }
    }

    void set_ssl_context(ssl_context ctx) {
        if (is_connected()) {
            NEFORCE_THROW_EXCEPTION(ssl_exception("Cannot set SSL context while connected"));
        }
        if (!ctx.is_valid()) {
            NEFORCE_THROW_EXCEPTION(ssl_exception("Invalid SSL context"));
        }

        ctx.set_options(
            SSL_OP_NO_SSLv2 |
            SSL_OP_NO_SSLv3 |
            SSL_OP_NO_TLSv1 |
            SSL_OP_NO_TLSv1_1
        );

        ssl_ctx_ = move(ctx);
    }

    void set_verify_peer(const bool verify) {
        if (is_connected()) {
            NEFORCE_THROW_EXCEPTION(ssl_exception("Cannot change verification mode while connected"));
        }
        verify_peer_ = verify;
        if (ssl_ctx_ && ssl_ctx_->is_valid()) {
            ssl_ctx_->set_verify_mode(verify ? SSL_VERIFY_PEER : SSL_VERIFY_NONE);
        }
    }

    NEFORCE_NODISCARD bool get_verify_peer() const noexcept {
        return verify_peer_;
    }

    void set_sni_hostname(string hostname) {
        if (is_connected()) {
            NEFORCE_THROW_EXCEPTION(ssl_exception("Cannot set SNI hostname while connected"));
        }
        sni_hostname_ = _NEFORCE move(hostname);
    }

    NEFORCE_NODISCARD const string& sni_hostname() const noexcept {
        return sni_hostname_;
    }

    bool load_ca_file(const string& ca_file) {
        if (!ssl_ctx_) {
            ssl_ctx_ = ssl_context(ssl_method::TLS_CLIENT);
        }
        return ssl_ctx_->load_verify_locations(ca_file, "");
    }

    bool load_ca_path(const string& ca_path) {
        if (!ssl_ctx_) {
            ssl_ctx_ = ssl_context(ssl_method::TLS_CLIENT);
        }
        return ssl_ctx_->load_verify_locations("", ca_path);
    }

    NEFORCE_NODISCARD string_view peer_certificate_info() const {
        if (!is_connected() || !ssl_initialized_) {
            return "";
        }
        return socket().peer_certificate_info().view();
    }

    NEFORCE_NODISCARD string_view cipher_name() const {
        if (!is_connected() || !ssl_initialized_) {
            return "";
        }
        return socket().ssl().get_cipher_name().view();
    }

    NEFORCE_NODISCARD string_view protocol_version() const {
        if (!is_connected() || !ssl_initialized_) {
            return "";
        }
        return socket().ssl().get_version().view();
    }

    NEFORCE_NODISCARD bool has_ssl_context() const noexcept {
        return ssl_ctx_.has_value() && ssl_ctx_->is_valid();
    }

    NEFORCE_NODISCARD bool is_ssl_initialized() const noexcept {
        return ssl_initialized_;
    }
};

#endif // NEFORCE_SUPPORT_OPENSSL


NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_TCP_TCP_CLIENT_HPP__
