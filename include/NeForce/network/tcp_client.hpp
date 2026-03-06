#ifndef NEFORCE_NETWORK_TCP_TCP_CLIENT_HPP__
#define NEFORCE_NETWORK_TCP_TCP_CLIENT_HPP__
#include "NeForce/network/dns/dns_client.hpp"
#ifdef NEFORCE_SUPPORT_OPENSSL
#include "NeForce/network/socket/ssl_socket.hpp"
#endif
NEFORCE_BEGIN_NAMESPACE__

template <typename SocketT>
class basic_tcp_client {
    static_assert(is_base_of_v<tcp_socket, SocketT>, "tcp_client must derive from tcp_socket");

public:
    using socket_type = SocketT;
    using duration = milliseconds;
    using exception_handler_t = function<void(const exception&)>;

private:
    dns_client dns_;
    optional<socket_type> socket_;

    string connected_host_;
    uint16_t connected_port_ = 0;

    duration connect_timeout_{5000};
    duration send_timeout_{5000};
    duration recv_timeout_{5000};

    bool auto_reconnect_ = false;
    int reconnect_attempts_ = 3;

protected:
    exception_handler_t exception_handler_;

protected:
    virtual bool post_connect() { return true; }

    bool reconnect_if_needed() {
        if (!auto_reconnect_ || connected_host_.empty()) {
            return false;
        }

        for (int i = 0; i < reconnect_attempts_; ++i) {
            if (connect(connected_host_, connected_port_)) {
                return true;
            }
            this_thread::sleep_for(milliseconds(100));
        }

        return false;
    }

public:
    basic_tcp_client() = default;

    explicit basic_tcp_client(dns_client dns)
    : dns_(_NEFORCE move(dns)) {}

    virtual ~basic_tcp_client() {
        disconnect();
    }

    basic_tcp_client(const basic_tcp_client&) = delete;
    basic_tcp_client& operator=(const basic_tcp_client&) = delete;

    basic_tcp_client(basic_tcp_client&&) noexcept = default;
    basic_tcp_client& operator=(basic_tcp_client&&) noexcept = default;

    void set_connect_timeout(const duration timeout) { connect_timeout_ = timeout; }
    void set_send_timeout(const duration timeout) { send_timeout_ = timeout; }
    void set_recv_timeout(const duration timeout) { recv_timeout_ = timeout; }

    void set_auto_reconnect(const bool enable, const int max_attempts = 3) {
        auto_reconnect_ = enable;
        reconnect_attempts_ = max_attempts;
    }

    void set_dns_server(const string& server, const uint16_t port = 53) {
        dns_client::config cfg;
        cfg.server = server;
        cfg.port = port;
        dns_.set_config(cfg);
    }

    void set_exception_handler(exception_handler_t handler) {
        exception_handler_ = _NEFORCE move(handler);
    }

    virtual bool connect(const string& host, const uint16_t port) {
        disconnect();

        vector<string> ips;
        const auto addr = ip_address::parse(host, port);
        if (addr.has_value()) {
            const bool is_ipv4 = addr->is_ipv4();
            const bool is_ipv6 = addr->is_ipv6();

            if (is_ipv4 || is_ipv6) {
                ips.push_back(host);
            }
        } else {
            try {
                auto ipv4s = dns_.resolve_a(host.view());
                auto ipv6s = dns_.resolve_aaaa(host.view());
                ips.insert(ips.end(), ipv4s.begin(), ipv4s.end());
                ips.insert(ips.end(), ipv6s.begin(), ipv6s.end());
            } catch (const exception& e) {
                if (exception_handler_) {
                    exception_handler_(e);
                }
                return false;
            }
        }

        if (ips.empty()) {
            return false;
        }

        for (const auto& ip : ips) {
            const bool is_ipv6_conn = ip.find(':') != string::npos;

            try {
                socket_type sock;
                tcp_socket* sockp = static_cast<tcp_socket*>(&sock);
                sockp->open(is_ipv6_conn ? AF_INET6 : AF_INET);

                sockp->set_send_timeout(send_timeout_);
                sockp->set_receive_timeout(recv_timeout_);

                auto endpoint = ip_address::parse(ip, port);
                if (!endpoint) continue;

                if (!sock.connect(*endpoint, connect_timeout_)) {
                    continue;
                }

                socket_ = _NEFORCE move(sock);
                connected_host_ = host;
                connected_port_ = port;

                if (!post_connect()) {
                    socket_.reset();
                    continue;
                }

                return true;

            } catch (const exception& e) {
                if (exception_handler_) {
                    exception_handler_(e);
                }
                continue;
            }
        }

        return false;
    }

    void disconnect() noexcept {
        socket_.reset();
        connected_host_.clear();
        connected_port_ = 0;
    }

    ssize_t send(const void* data, const size_t length) {
        if (!is_connected()) {
            if (!reconnect_if_needed()) return -1;
        }

        try {
            return socket_->send(memory_view<const char>(static_cast<const char*>(data), length));
        } catch (const exception& e) {
            if (exception_handler_) {
                exception_handler_(e);
            }
            if (auto_reconnect_) {
                disconnect();
            }
            return -1;
        }
    }

    ssize_t send(const string_view data) {
        return send(data.data(), data.size());
    }

    ssize_t receive(void* buffer, const size_t length) {
        if (!is_connected()) {
            if (!reconnect_if_needed()) return -1;
        }

        try {
            return socket_->receive(memory_view<char>(static_cast<char*>(buffer), length));
        } catch (const exception& e) {
            if (exception_handler_) {
                exception_handler_(e);
            }
            if (auto_reconnect_) {
                disconnect();
            }
            return -1;
        }
    }

    vector<char> receive_all(const size_t max_size = 0) {
        if (!is_connected()) {
            if (!reconnect_if_needed()) return {};
        }

        try {
            vector<char> result;
            char buffer[8192];

            while (true) {
                const ssize_t n = socket_->receive(memory_view<char>(buffer));
                if (n < 0) break;
                if (n == 0) break;

                result.insert(result.end(), buffer, buffer + n);

                if (max_size > 0 && result.size() >= max_size) {
                    break;
                }
            }

            return result;

        } catch (const exception& e) {
            if (exception_handler_) {
                exception_handler_(e);
            }
            if (auto_reconnect_) {
                disconnect();
            }
            return {};
        }
    }

    bool receive_exact(memory_view<const char> buffer) {
        if (!is_connected()) {
            if (!reconnect_if_needed()) return false;
        }

        size_t total = 0;
        while (total < buffer.size()) {
            try {
                const ssize_t n = socket_->receive(buffer.view(total));
                if (n <= 0) return false;
                total += n;
            } catch (const exception& e) {
                if (exception_handler_) {
                    exception_handler_(e);
                }
                if (auto_reconnect_) {
                    disconnect();
                }
                return false;
            }
        }

        return true;
    }

    bool is_connected() const noexcept {
        return socket_.has_value() && socket_->is_open();
    }

    const string& connected_host() const noexcept {
        return connected_host_;
    }

    uint16_t connected_port() const noexcept {
        return connected_port_;
    }

    socket_type& socket() {
        return *socket_;
    }

    const socket_type& socket() const {
        return *socket_;
    }
};


using tcp_client = basic_tcp_client<tcp_socket>;


#ifdef NEFORCE_SUPPORT_OPENSSL

class ssl_client final : public basic_tcp_client<ssl_socket> {
private:
    optional<ssl_context> ssl_ctx_;
    bool verify_peer_ = true;

protected:
    bool post_connect() override {
        if (!ssl_ctx_) return false;

        try {
            socket().init_client_ssl(*ssl_ctx_);

            if (verify_peer_ && !socket().ssl().verify_peer()) {
                return false;
            }

            return true;
        } catch (const exception& e) {
            if (exception_handler_) {
                exception_handler_(e);
            }
            return false;
        }
    }

public:
    ssl_client() = default;

    explicit ssl_client(ssl_context ctx)
    : ssl_ctx_(_NEFORCE move(ctx)) {}

    void set_ssl_context(ssl_context ctx) {
        ssl_ctx_ = _NEFORCE move(ctx);
    }

    void set_verify_peer(const bool verify) {
        verify_peer_ = verify;
    }

    string peer_certificate_info() const {
        if (!is_connected()) return "";
        return socket().peer_certificate_info();
    }

    string cipher_name() const {
        if (!is_connected()) return "";
        return socket().ssl().get_cipher_name();
    }

    string protocol_version() const {
        if (!is_connected()) return "";
        return socket().ssl().get_version();
    }
};

#endif // NEFORCE_SUPPORT_OPENSSL


NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_TCP_TCP_CLIENT_HPP__
