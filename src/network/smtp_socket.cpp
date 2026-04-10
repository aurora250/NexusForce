#include <NeForce/core/encrypt/base64.hpp>
#include <NeForce/core/utility/packages.hpp>
#include <NeForce/network/smtp_socket.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    string build_message(const smtp_message& msg) {
        string result;

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
            const auto& key = header.first;
            const auto& value = header.second;
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
            resp.message += line.substr(4);
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
        const string err = "SMTP command failed: " + cmd.substr(0, cmd.find(' ')) + " expected " + to_string(expected) +
                           " got " + to_string(resp.code) + ": " + resp.message;
        NEFORCE_THROW_EXCEPTION(smtp_exception(err.data()));
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
            caps.push_back(line.substr(4));
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
    open_ip(addr.family(), SOCK_STREAM, IPPROTO_TCP);
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
        const string quit = "QUIT\r\n";
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
    result.cipher_name = ssl_.get_cipher_name();
    result.tls_version = ssl_.get_version();
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

NEFORCE_END_NAMESPACE__
