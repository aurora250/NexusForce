#include <NeForce/core/encrypt/base64.hpp>
#include <NeForce/core/system/process.hpp>
#include <NeForce/core/time/clocks.hpp>
#include <NeForce/core/utility/packages.hpp>
#include <NeForce/network/smtp_socket.hpp>
#include <openssl/ssl.h>
#include <ctime>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    string build_message(const smtp_message& msg) {
        string date_header;
        {
            const auto now = system_clock::now();
            const auto secs = system_clock::to_seconds(now);
            const auto t = static_cast<::time_t>(secs.count());
            ::tm gmt{};
#ifdef NEFORCE_PLATFORM_WINDOWS
            ::gmtime_s(&gmt, &t);
#else
            ::gmtime_r(&t, &gmt);
#endif
            char date_buf[64];
            ::strftime(date_buf, sizeof(date_buf), "%a, %d %b %Y %H:%M:%S +0000", &gmt);
            date_header = string("Date: ") + date_buf + "\r\n";
        }

        string message_id;
        {
            const auto now = system_clock::now();
            const auto secs = system_clock::to_seconds(now);
            const uint16_t pid = process::current_id();
            message_id = "Message-ID: <" + to_string(secs.count()) + "." + to_string(pid) + "@nexusforce.local>\r\n";
        }

        string result;
        result += date_header;
        result += message_id;
        result += "From: " + msg.from + "\r\n";
        result += "To: ";
        for (size_t i = 0; i < msg.to.size(); ++i) {
            if (i > 0) {
                result += ", ";
            }
            result += msg.to[i];
        }
        result += "\r\n";

        if (!msg.cc.empty()) {
            result += "Cc: ";
            for (size_t i = 0; i < msg.cc.size(); ++i) {
                if (i > 0) {
                    result += ", ";
                }
                result += msg.cc[i];
            }
            result += "\r\n";
        }

        result += "Subject: " + msg.subject + "\r\n";

        if (msg.is_html) {
            result += "Content-Type: text/html; charset=utf-8\r\n";
        } else {
            result += "Content-Type: text/plain; charset=utf-8\r\n";
        }

        result += "MIME-Version: 1.0\r\n";

        for (const auto& header: msg.extra_headers) {
            auto key = header.first;
            auto value = header.second;
            if (key.lowercase() == "date" || key.lowercase() == "message-id") {
                continue;
            }
            result += key + ": " + value + "\r\n";
        }

        result += "\r\n";
        result += msg.body;
        result += "\r\n";

        return result;
    }
} // namespace


ssize_t smtp_socket::raw_send(const char* data, const size_t len) {
    if (tls_active_) {
        return ssl_.write(data, len);
    }
    return ::send(static_cast<int>(fd_), data, static_cast<int>(len), 0);
}

ssize_t smtp_socket::raw_recv(char* buf, const size_t len) {
    if (tls_active_) {
        return ssl_.read(buf, len);
    }
    return ::recv(static_cast<int>(fd_), buf, static_cast<int>(len), 0);
}

bool smtp_socket::read_line(string& out) {
    out.clear();
    char ch = '\0';
    while (true) {
        const ssize_t n = raw_recv(&ch, 1);
        if (n <= 0) {
            return false;
        }
        if (ch == '\r') {
            continue;
        }
        if (ch == '\n') {
            return true;
        }
        out += ch;
    }
}

smtp_socket::response smtp_socket::read_response() {
    response resp{};
    string line;

    while (true) {
        if (!read_line(line)) {
            NEFORCE_THROW_EXCEPTION(smtp_exception("Connection closed while reading SMTP response"));
        }
        if (line.size() < 3) {
            NEFORCE_THROW_EXCEPTION(smtp_exception("Malformed SMTP response"));
        }

        int code = 0;
        for (int i = 0; i < 3; ++i) {
            if (line[i] < '0' || line[i] > '9') {
                NEFORCE_THROW_EXCEPTION(smtp_exception("Invalid SMTP response code"));
            }
            code = code * 10 + (line[i] - '0');
        }

        const bool multi = (line.size() > 3 && line[3] == '-');
        if (!resp.message.empty()) {
            resp.message += '\n';
        }
        if (line.size() > 4) {
            resp.message += line.tail(4);
        }
        resp.code = code;

        if (!multi) {
            break;
        }
    }
    return resp;
}

smtp_socket::response smtp_socket::send_command(const string& cmd) {
    const string full = cmd + "\r\n";
    size_t total = 0;
    while (total < full.size()) {
        const ssize_t n = raw_send(full.data() + total, full.size() - total);
        if (n <= 0) {
            NEFORCE_THROW_EXCEPTION(smtp_exception("SMTP send command failed"));
        }
        total += static_cast<size_t>(n);
    }
    return read_response();
}

void smtp_socket::expect_code(const int expected, const string& cmd) {
    const auto resp = send_command(cmd);
    if (resp.code != expected) {
        const string err = "SMTP command failed: " + cmd.head(cmd.find(' ')) + " expected " + to_string(expected) +
                           " got " + to_string(resp.code) + ": " + resp.message;
        NEFORCE_THROW_EXCEPTION(smtp_exception(err));
    }
}

vector<string> smtp_socket::do_ehlo(const string& domain) {
    vector<string> caps;
    const string cmd = "EHLO " + domain + "\r\n";
    size_t total = 0;
    while (total < cmd.size()) {
        const ssize_t n = raw_send(cmd.data() + total, cmd.size() - total);
        if (n <= 0) {
            NEFORCE_THROW_EXCEPTION(smtp_exception("EHLO send failed"));
        }
        total += static_cast<size_t>(n);
    }

    string line;
    while (true) {
        if (!read_line(line)) {
            NEFORCE_THROW_EXCEPTION(smtp_exception("Connection closed during EHLO"));
        }
        if (line.size() < 3) {
            NEFORCE_THROW_EXCEPTION(smtp_exception("Malformed EHLO response"));
        }

        int code = 0;
        for (int i = 0; i < 3; ++i) {
            if (line[i] < '0' || line[i] > '9') {
                NEFORCE_THROW_EXCEPTION(smtp_exception("Malformed EHLO response code"));
            }
            code = code * 10 + (line[i] - '0');
        }

        if (code != 250) {
            const auto helo = send_command("HELO " + domain);
            if (helo.code != 250) {
                NEFORCE_THROW_EXCEPTION(smtp_exception("SMTP HELO/EHLO failed"));
            }
            return caps;
        }

        if (line.size() > 4) {
            caps.push_back(line.tail(4));
        }

        if (line.size() <= 3 || line[3] != '-') {
            break;
        }
    }
    return caps;
}

void smtp_socket::do_post_connect(const string& domain, const tls_mode mode, const ssl_context* ctx,
                                  const string& sni_hostname) {
    tls_mode_ = mode;
    tls_active_ = false;

    if (mode == tls_mode::implicit) {
        if (ctx == nullptr) {
            close();
            NEFORCE_THROW_EXCEPTION(value_exception("ssl_context required for implicit TLS"));
        }
        do_tls_handshake(*ctx, sni_hostname);
    }

    const auto greeting = read_response();
    if (greeting.code != 220) {
        close();
        NEFORCE_THROW_EXCEPTION(smtp_exception("SMTP server did not send 220 greeting"));
    }

    server_domain_ = domain;
    ignore = do_ehlo(domain);
    connected_ = true;
}

void smtp_socket::do_tls_handshake(const ssl_context& ctx, const string& sni_hostname) {
    ssl_.reset(ctx);
    ssl_.set_fd(fd_);

    if (!sni_hostname.empty()) {
        ssl_.set_sni_hostname(sni_hostname);
    }

    ssl_.connect();
    tls_active_ = true;
}

void smtp_socket::open_and_connect(const ip_address& addr) {
    open_ip(addr.address_family(), type::stream, protocol::tcp);
    if (::connect(fd_, addr.data(), addr.size()) != 0) {
        close();
        NEFORCE_THROW_EXCEPTION(socket_exception("Failed to connect to SMTP server"));
    }
}

void smtp_socket::connect(const ip_address& addr, const string& domain, const tls_mode mode, const ssl_context* ctx,
                          const string& sni_hostname) {
    if (!addr.is_valid()) {
        NEFORCE_THROW_EXCEPTION(value_exception("Invalid SMTP server address"));
    }
    open_and_connect(addr);
    do_post_connect(domain, mode, ctx, sni_hostname);
}

void smtp_socket::connect(const string& hostname, const ports port, const string& domain, const tls_mode mode,
                          dns_client* dns, const ssl_context* ctx, const string& sni_hostname) {
    if (hostname.empty()) {
        NEFORCE_THROW_EXCEPTION(value_exception("SMTP hostname cannot be empty"));
    }

    auto direct = ip_address::parse(hostname, port);
    if (direct) {
        connect(*direct, domain, mode, ctx, sni_hostname.empty() ? hostname : sni_hostname);
        return;
    }

    optional<dns_client> local_dns;
    dns_client* resolver = dns;

    if (resolver == nullptr) {
        local_dns.emplace();
        resolver = &(*local_dns);
    }

    const auto ips = resolver->resolve_a(hostname.view());
    if (ips.empty()) {
        NEFORCE_THROW_EXCEPTION(smtp_exception(("DNS resolution failed for host: " + hostname).data()));
    }

    for (const auto& ip_str: ips) {
        auto addr = ip_address::parse(ip_str, port);
        if (!addr) {
            continue;
        }

        try {
            open_and_connect(*addr);
        } catch (...) {
            continue;
        }

        const string& effective_sni = sni_hostname.empty() ? hostname : sni_hostname;
        do_post_connect(domain, mode, ctx, effective_sni);
        return;
    }

    NEFORCE_THROW_EXCEPTION(socket_exception("Failed to connect to any resolved address for SMTP host"));
}

void smtp_socket::disconnect() {
    if (is_open()) {
        constexpr string_view quit = "QUIT\r\n";
        ignore = raw_send(quit.data(), quit.size());
        read_response();
    }
    connected_ = false;
    tls_active_ = false;
    close();
}

smtp_socket::starttls_result smtp_socket::starttls(const ssl_context& ctx, const string& sni_hostname) {
    if (!is_connected()) {
        NEFORCE_THROW_EXCEPTION(smtp_exception("Not connected to SMTP server"));
    }
    if (tls_active_) {
        NEFORCE_THROW_EXCEPTION(smtp_exception("TLS is already active"));
    }

    const auto resp = send_command("STARTTLS");
    if (resp.code != 220) {
        NEFORCE_THROW_EXCEPTION(smtp_exception("Server does not support STARTTLS or rejected it"));
    }

    do_tls_handshake(ctx, sni_hostname);
    ignore = do_ehlo(server_domain_);

    starttls_result result;
    result.upgraded = true;
    result.cipher_name = ssl_.cipher_name();
    result.tls_version = ssl_.version();
    result.peer_verified = ssl_.verify_peer();
    return result;
}

void smtp_socket::authenticate(const string& username, const string& password, const auth_method method) {

    if (!is_connected()) {
        NEFORCE_THROW_EXCEPTION(smtp_exception("Not connected to SMTP server"));
    }

    switch (method) {
        case auth_method::plain: {
            string credentials;
            credentials += '\0';
            credentials += username;
            credentials += '\0';
            credentials += password;
            const string encoded = base64_encode(credentials);
            expect_code(235, "AUTH PLAIN " + encoded);
            break;
        }
        case auth_method::login: {
            const auto resp = send_command("AUTH LOGIN");
            if (resp.code != 334) {
                NEFORCE_THROW_EXCEPTION(smtp_exception("AUTH LOGIN failed at handshake"));
            }
            expect_code(334, base64_encode(username));
            expect_code(235, base64_encode(password));
            break;
        }
        case auth_method::none:
        default: {
            break;
        }
    }
}

void smtp_socket::send(const smtp_message& msg) {
    if (!is_connected()) {
        NEFORCE_THROW_EXCEPTION(smtp_exception("Not connected to SMTP server"));
    }
    if (msg.from.empty()) {
        NEFORCE_THROW_EXCEPTION(value_exception("SMTP message 'from' field is empty"));
    }
    if (msg.to.empty()) {
        NEFORCE_THROW_EXCEPTION(value_exception("SMTP message has no recipients"));
    }

    expect_code(250, "MAIL FROM:<" + msg.from + ">");

    for (const auto& addr: msg.to) {
        expect_code(250, "RCPT TO:<" + addr + ">");
    }
    for (const auto& addr: msg.cc) {
        expect_code(250, "RCPT TO:<" + addr + ">");
    }
    for (const auto& addr: msg.bcc) {
        expect_code(250, "RCPT TO:<" + addr + ">");
    }

    const auto data_resp = send_command("DATA");
    if (data_resp.code != 354) {
        NEFORCE_THROW_EXCEPTION(smtp_exception("SMTP DATA command failed"));
    }

    string content = build_message(msg);
    string escaped;
    escaped.reserve(content.size() + 16);
    for (size_t i = 0; i < content.size(); ++i) {
        if ((i == 0 || content[i - 1] == '\n') && content[i] == '.') {
            escaped += '.';
        }
        escaped += content[i];
    }
    escaped += "\r\n.\r\n";

    size_t total = 0;
    while (total < escaped.size()) {
        const ssize_t n = raw_send(escaped.data() + total, escaped.size() - total);
        if (n <= 0) {
            NEFORCE_THROW_EXCEPTION(smtp_exception("Failed to send mail body"));
        }
        total += static_cast<size_t>(n);
    }

    const auto end_resp = read_response();
    if (end_resp.code != 250) {
        NEFORCE_THROW_EXCEPTION(smtp_exception("SMTP message delivery failed"));
    }
}

void smtp_socket::noop() { expect_code(250, "NOOP"); }

struct smtp_socket::async_connect_op : enable_shared_from_this<async_connect_op> {
    smtp_socket* sock{nullptr};
    io_context* ctx{nullptr};
    ip_address endpoint;
    string domain;
    smtp_socket::tls_mode mode{smtp_socket::tls_mode::none};
    const ssl_context* ssl_ctx{nullptr};
    string sni_hostname;
    function<void(error_code)> handler;
    cancellation_slot* cancel_slot{nullptr};
    bool finished{false};

    char byte_{0};
    string line_;
    smtp_socket::response resp_{};
    string send_buf_;
    size_t send_off_{0};

    void start() {
        if (cancel_slot != nullptr && cancel_slot->is_cancelled()) {
            fail(make_operation_aborted());
            return;
        }
        if (!endpoint.is_valid()) {
            fail(error_code(static_cast<int>(errc::invalid_argument), error_category::system()));
            return;
        }

        try {
            sock->open_ip(endpoint.address_family(), socket_type::stream, socket_protocol::tcp);
        } catch (const system_exception& e) {
            fail(e.code());
            return;
        } catch (const exception&) {
            fail(error_code(static_cast<int>(errc::invalid_argument), error_category::system()));
            return;
        }

        sock->tls_mode_ = mode;
        sock->tls_active_ = false;
        sock->set_nonblocking(true);
        do_connect();
    }

    void do_connect() {
        const int rc = ::connect(sock->native_handle(), endpoint.data(), endpoint.size());
        if (rc == 0) {
            on_connected();
            return;
        }

        const int err = network_exception::last_error().value();
#ifdef NEFORCE_PLATFORM_WINDOWS
        if (err != WSAEWOULDBLOCK && err != WSAEALREADY && err != WSAEINPROGRESS) {
#else
        if (err != EINPROGRESS && err != EALREADY) {
#endif
            fail(error_code(err, error_category::system()));
            return;
        }

        auto self = shared_from_this();
        if (cancel_slot != nullptr) {
            cancel_slot->assign([self]() mutable { self->fail(make_operation_aborted()); });
        }
        ctx->add_fd(sock->native_handle(), epoll_event::out,
                    [self](int, epoll_event, error_code ec) mutable { self->on_connect_ready(ec); });
    }

    void on_connect_ready(error_code ec) {
        if (ec) {
            fail(ec);
            return;
        }

        int optval = 0;
        ::socklen_t optlen = sizeof(optval);
        if (sock->get_option(SOL_SOCKET, SO_ERROR, &optval, &optlen) && optval == 0) {
            on_connected();
        } else {
            fail(error_code(optval != 0 ? optval : network_exception::last_error().value(), error_category::system()));
        }
    }

    void on_connected() {
        if (mode == smtp_socket::tls_mode::implicit) {
            do_tls();
        } else {
            do_greeting();
        }
    }

    void do_tls() {
        if (ssl_ctx == nullptr) {
            fail(error_code(static_cast<int>(errc::invalid_argument), error_category::system()));
            return;
        }

        try {
            sock->ssl_.reset(*ssl_ctx);
            sock->ssl_.set_fd(sock->native_handle());
            if (!sni_hostname.empty()) {
                sock->ssl_.set_sni_hostname(sni_hostname);
            }
            ::SSL_set_connect_state(static_cast<::SSL*>(sock->ssl_.native_handle()));
        } catch (const system_exception& e) {
            fail(e.code());
            return;
        } catch (const exception&) {
            fail(error_code(static_cast<int>(errc::invalid_argument), error_category::system()));
            return;
        }

        auto self = shared_from_this();
        if (cancel_slot != nullptr) {
            sock->ssl_.async_handshake(*ctx, *cancel_slot, [self](error_code e) mutable { self->on_tls_done(e); });
        } else {
            sock->ssl_.async_handshake(*ctx, [self](error_code e) mutable { self->on_tls_done(e); });
        }
    }

    void on_tls_done(error_code ec) {
        if (ec) {
            fail(ec);
            return;
        }
        sock->tls_active_ = true;
        do_greeting();
    }

    void do_greeting() {
        auto self = shared_from_this();
        async_read_response([self](error_code ec, const smtp_socket::response& r) mutable {
            if (ec) {
                self->fail(ec);
                return;
            }
            if (r.code != 220) {
                self->fail(error_code(static_cast<int>(errc::protocol_error), error_category::system()));
                return;
            }
            self->do_ehlo();
        });
    }

    void do_ehlo() {
        auto self = shared_from_this();
        async_send_all("EHLO " + domain + "\r\n", [self](error_code ec) mutable {
            if (ec) {
                self->fail(ec);
                return;
            }
            self->async_read_response([self](error_code ec2, const smtp_socket::response& r) mutable {
                if (ec2) {
                    self->fail(ec2);
                    return;
                }
                if (r.code == 250) {
                    self->succeed();
                } else {
                    self->do_helo();
                }
            });
        });
    }

    void do_helo() {
        auto self = shared_from_this();
        async_send_all("HELO " + domain + "\r\n", [self](error_code ec) mutable {
            if (ec) {
                self->fail(ec);
                return;
            }
            self->async_read_response([self](error_code ec2, const smtp_socket::response& r) mutable {
                if (ec2) {
                    self->fail(ec2);
                    return;
                }
                if (r.code == 250) {
                    self->succeed();
                } else {
                    self->fail(error_code(static_cast<int>(errc::protocol_error), error_category::system()));
                }
            });
        });
    }

    void async_read_response(function<void(error_code, const smtp_socket::response&)> cb) {
        resp_ = smtp_socket::response{};
        read_response_line(move(cb));
    }

    void read_response_line(function<void(error_code, const smtp_socket::response&)> cb) {
        auto self = shared_from_this();
        async_read_line([self, cb = move(cb)](error_code ec, const string& line) mutable {
            if (ec) {
                cb(ec, smtp_socket::response{});
                return;
            }
            if (line.size() < 3) {
                cb(error_code(static_cast<int>(errc::protocol_error), error_category::system()),
                   smtp_socket::response{});
                return;
            }

            int code = 0;
            for (int i = 0; i < 3; ++i) {
                if (line[i] < '0' || line[i] > '9') {
                    cb(error_code(static_cast<int>(errc::protocol_error), error_category::system()),
                       smtp_socket::response{});
                    return;
                }
                code = code * 10 + (line[i] - '0');
            }

            const bool multi = (line.size() > 3 && line[3] == '-');
            if (!self->resp_.message.empty()) {
                self->resp_.message += '\n';
            }
            if (line.size() > 4) {
                self->resp_.message += line.substr(4);
            }
            self->resp_.code = code;

            if (!multi) {
                cb(error_code{}, self->resp_);
                return;
            }
            self->read_response_line(move(cb));
        });
    }

    void async_read_line(function<void(error_code, const string&)> cb) {
        line_.clear();
        read_line_byte(move(cb));
    }

    void read_line_byte(function<void(error_code, const string&)> cb) {
        auto self = shared_from_this();
        read_one_byte([self, cb = move(cb)](error_code ec, bool ok, char c) mutable {
            if (ec) {
                cb(ec, string{});
                return;
            }
            if (!ok) {
                cb(error_code(static_cast<int>(errc::connection_reset), error_category::system()), string{});
                return;
            }
            if (c == '\n') {
                cb(error_code{}, self->line_);
                return;
            }
            if (c != '\r') {
                self->line_ += c;
            }
            self->read_line_byte(move(cb));
        });
    }

    void read_one_byte(function<void(error_code, bool ok, char c)> cb) {
        if (sock->tls_active_) {
            auto self = shared_from_this();
            sock->ssl_.async_read(
                    *ctx, memory_view<char>(&byte_, 1),
                    [self, cb = move(cb)](error_code ec, size_t n) mutable { cb(ec, n == 1, self->byte_); });
            return;
        }

        const auto fd = sock->native_handle();
        const ssize_t n = ::recv(fd, &byte_, 1, 0);
        if (n == 1) {
            cb(error_code{}, true, byte_);
            return;
        }
        if (n == 0) {
            cb(error_code(static_cast<int>(errc::connection_reset), error_category::system()), false, 0);
            return;
        }

        const int err = network_exception::last_error().value();
        if (!socket_exception::is_would_block(err)) {
            cb(error_code(err, error_category::system()), false, 0);
            return;
        }

        auto self = shared_from_this();
        ctx->add_fd(fd, epoll_event::in, [self, cb = move(cb)](int, epoll_event, error_code ec) mutable {
            if (ec) {
                cb(ec, false, 0);
                return;
            }
            const ssize_t m = ::recv(self->sock->native_handle(), &self->byte_, 1, 0);
            if (m == 1) {
                cb(error_code{}, true, self->byte_);
            } else if (m == 0) {
                cb(error_code(static_cast<int>(errc::connection_reset), error_category::system()), false, 0);
            } else {
                cb(error_code(static_cast<int>(network_exception::last_error().value()), error_category::system()),
                   false, 0);
            }
        });
    }

    void async_send_all(const string& data, function<void(error_code)> cb) {
        send_buf_ = data;
        send_off_ = 0;
        send_next(move(cb));
    }

    void send_next(function<void(error_code)> cb) {
        if (send_off_ >= send_buf_.size()) {
            cb(error_code{});
            return;
        }

#ifdef NEFORCE_PLATFORM_LINUX
        constexpr int send_flags = MSG_NOSIGNAL;
#else
        constexpr int send_flags = 0;
#endif

        if (sock->tls_active_) {
            auto self = shared_from_this();
            sock->ssl_.async_write(
                    *ctx, memory_view<const char>(send_buf_.data() + send_off_, send_buf_.size() - send_off_),
                    [self, cb = move(cb)](error_code ec, size_t n) mutable {
                        if (ec) {
                            cb(ec);
                            return;
                        }
                        if (n == 0) {
                            cb(error_code(static_cast<int>(errc::connection_reset), error_category::system()));
                            return;
                        }
                        self->send_off_ += n;
                        self->send_next(move(cb));
                    });
            return;
        }

        const auto fd = sock->native_handle();
        const ssize_t n =
                ::send(fd, send_buf_.data() + send_off_, static_cast<int>(send_buf_.size() - send_off_), send_flags);
        if (n > 0) {
            send_off_ += static_cast<size_t>(n);
            send_next(move(cb));
            return;
        }
        if (n == 0) {
            cb(error_code(static_cast<int>(errc::connection_reset), error_category::system()));
            return;
        }

        const int err = network_exception::last_error().value();
        if (!socket_exception::is_would_block(err)) {
            cb(error_code(err, error_category::system()));
            return;
        }

        auto self = shared_from_this();
        ctx->add_fd(fd, epoll_event::out, [self, cb = move(cb)](int, epoll_event, error_code ec) mutable {
            if (ec) {
                cb(ec);
                return;
            }
            const ssize_t m = ::send(self->sock->native_handle(), self->send_buf_.data() + self->send_off_,
                                     static_cast<int>(self->send_buf_.size() - self->send_off_), send_flags);
            if (m > 0) {
                self->send_off_ += static_cast<size_t>(m);
                self->send_next(move(cb));
            } else if (m == 0) {
                cb(error_code(static_cast<int>(errc::connection_reset), error_category::system()));
            } else {
                cb(error_code(static_cast<int>(network_exception::last_error().value()), error_category::system()));
            }
        });
    }

    void succeed() {
        if (finished) {
            return;
        }
        finished = true;
        sock->server_domain_ = domain;
        sock->connected_ = true;
        handler(error_code{});
    }

    void fail(error_code ec) {
        if (finished) {
            return;
        }
        finished = true;
        ctx->remove_fd(sock->native_handle());
        sock->close();
        handler(ec);
    }
};

void smtp_socket::async_connect(io_context& ioc, const ip_address& addr, const string& domain, const tls_mode mode,
                                const ssl_context* ssl_ctx, const string& sni_hostname,
                                function<void(error_code)> handler) {
    auto op = make_shared<async_connect_op>();
    op->sock = this;
    op->ctx = &ioc;
    op->endpoint = addr;
    op->domain = domain;
    op->mode = mode;
    op->ssl_ctx = ssl_ctx;
    op->sni_hostname = sni_hostname;
    op->handler = move(handler);
    op->start();
}

void smtp_socket::async_connect(io_context& ioc, const ip_address& addr, const string& domain, const tls_mode mode,
                                const ssl_context* ssl_ctx, const string& sni_hostname, cancellation_slot& slot,
                                function<void(error_code)> handler) {
    auto op = make_shared<async_connect_op>();
    op->sock = this;
    op->ctx = &ioc;
    op->endpoint = addr;
    op->domain = domain;
    op->mode = mode;
    op->ssl_ctx = ssl_ctx;
    op->sni_hostname = sni_hostname;
    op->handler = move(handler);
    op->cancel_slot = &slot;
    op->start();
}

NEFORCE_END_NAMESPACE__
