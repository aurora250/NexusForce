#include <NeForce/core/utility/packages.hpp>
#include <NeForce/network/ftp/ftp_client.hpp>
#ifdef NEFORCE_PLATFORM_LINUX
#    include <arpa/inet.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

namespace {
    vector<char> read_data_channel(tcp_socket& sock) {
        vector<char> result;
        char buf[8192];
        while (true) {
            const ssize_t n = ::recv(sock.native_handle(), buf, sizeof(buf), 0);
            if (n <= 0) {
                break;
            }
            result.insert(result.end(), buf, buf + n);
        }
        return result;
    }

    vector<char> read_data_channel_tls(ssl_stream& stream) {
        vector<char> result;
        char buf[8192];
        while (true) {
            const ssize_t n = stream.read(buf, sizeof(buf));
            if (n <= 0) {
                break;
            }
            result.insert(result.end(), buf, buf + n);
        }
        return result;
    }

    void write_data_channel(tcp_socket& sock, const char* data, const size_t len) {
        size_t total = 0;
        while (total < len) {
            const ssize_t n = ::send(sock.native_handle(), data + total, static_cast<int>(len - total), 0);
            if (n <= 0) {
                NEFORCE_THROW_EXCEPTION(ftp_exception("Data channel write failed"));
            }
            total += static_cast<size_t>(n);
        }
    }

    void write_data_channel_tls(ssl_stream& stream, const char* data, const size_t len) {
        size_t total = 0;
        while (total < len) {
            const ssize_t n = stream.write(data + total, len - total);
            if (n <= 0) {
                NEFORCE_THROW_EXCEPTION(ftp_exception("TLS data channel write failed"));
            }
            total += static_cast<size_t>(n);
        }
    }
} // namespace


ftp_client::response ftp_client::send_command(const string& cmd) {
    const string full = cmd + "\r\n";
    size_t total = 0;
    while (total < full.size()) {
        const ssize_t n = ctrl_send(full.data() + total, full.size() - total);
        if (n <= 0) {
            NEFORCE_THROW_EXCEPTION(ftp_exception("FTP control channel send failed"));
        }
        total += static_cast<size_t>(n);
    }
    return read_response();
}

void ftp_client::expect_code(const int expected, const string& cmd) {
    const auto resp = send_command(cmd);
    if (resp.code != expected) {
        const string err = "FTP command failed [" + cmd + "] expected=" + to_string(expected) +
                           " got=" + to_string(resp.code) + " " + resp.message;
        NEFORCE_THROW_EXCEPTION(ftp_exception(err.data()));
    }
}

void ftp_client::expect_codes(const std::initializer_list<int> codes, const string& cmd) {
    const auto resp = send_command(cmd);
    for (const int c: codes) {
        if (resp.code == c) {
            return;
        }
    }
    const string err = "FTP command failed [" + cmd + "] got=" + to_string(resp.code) + " " + resp.message;
    NEFORCE_THROW_EXCEPTION(ftp_exception(err.data()));
}

void ftp_client::do_ctrl_tls_handshake() {
    if (ssl_ctx_ == nullptr) {
        NEFORCE_THROW_EXCEPTION(value_exception("ssl_context required for FTPS"));
    }

    ctrl_ssl_.reset(*ssl_ctx_);
    ctrl_ssl_.set_fd(fd_);
    if (!sni_host_.empty()) {
        ctrl_ssl_.set_sni_hostname(sni_host_);
    }
    ctrl_ssl_.connect();

    tls_active_ = true;
}

tcp_socket ftp_client::open_data_channel() {
    const bool use_ipv6 = address_family() == ip_address::family::INET6;

    if (passive_mode_ == passive_mode::passive) {
        if (use_ipv6) {
            const auto resp = send_command("EPSV");
            if (resp.code != 229) {
                NEFORCE_THROW_EXCEPTION(ftp_exception("EPSV command failed"));
            }
            const auto msg = resp.message.view();
            const size_t lp = msg.rfind('(');
            const size_t rp = msg.rfind(')');
            if (lp == string::npos || rp == string::npos || rp <= lp) {
                NEFORCE_THROW_EXCEPTION(ftp_exception("Failed to parse EPSV response"));
            }
            const auto port_part = msg.substr(lp + 1, rp - lp - 1);
            size_t p = port_part.rfind('|');
            if (p == string::npos || p < 1) {
                NEFORCE_THROW_EXCEPTION(ftp_exception("Invalid EPSV response format"));
            }
            const auto port_str = port_part.view(port_part.rfind('|', p - 1) + 1, p - port_part.rfind('|', p - 1) - 1);
            const auto data_port = static_cast<uint16_t>(integer32::parse(port_str).value());

            const auto ctrl_local = local_endpoint();
            if (!ctrl_local) {
                NEFORCE_THROW_EXCEPTION(ftp_exception("Failed to get control connection local address"));
            }
            string host_str = ctrl_local->to_string();
            const size_t port_col = host_str.rfind(':');
            if (port_col != string::npos) {
                host_str = host_str.view(0, port_col);
            }
            if (host_str.starts_with("[")) {
                host_str = host_str.view(1, host_str.size() - 2);
            }

            auto data_addr = ip_address::parse(host_str, ports(data_port));
            if (!data_addr) {
                NEFORCE_THROW_EXCEPTION(ftp_exception("Invalid data channel address from EPSV"));
            }

            tcp_socket data_sock;
            data_sock.open(use_ipv6 ? ip_address::family::INET6 : ip_address::family::INET4);
            static_cast<ip_socket&>(data_sock).connect(*data_addr);
            return data_sock;
        }

        const auto resp = send_command("PASV");
        if (resp.code != 227) {
            NEFORCE_THROW_EXCEPTION(ftp_exception("PASV command failed"));
        }

        const auto msg = resp.message.view();
        const size_t lp = msg.rfind('(');
        const size_t rp = msg.rfind(')');
        if (lp == string::npos || rp == string::npos || rp <= lp) {
            NEFORCE_THROW_EXCEPTION(ftp_exception("Failed to parse PASV response"));
        }

        const auto nums_str = msg.substr(lp + 1, rp - lp - 1);
        int nums[6] = {};
        size_t pos = 0;
        int idx = 0;
        while (idx < 6 && pos < nums_str.size()) {
            const size_t comma = nums_str.find(',', pos);
            const auto token = (comma == string::npos) ? nums_str.tail(pos) : nums_str.substr(pos, comma - pos);
            nums[idx++] = integer32::parse(token).value();
            if (comma == string::npos) {
                break;
            }
            pos = comma + 1;
        }
        if (idx != 6) {
            NEFORCE_THROW_EXCEPTION(ftp_exception("Invalid PASV address format"));
        }

        const string data_ip =
                to_string(nums[0]) + "." + to_string(nums[1]) + "." + to_string(nums[2]) + "." + to_string(nums[3]);
        const ports data_port{static_cast<uint16_t>(nums[4] * 256 + nums[5])};

        auto data_addr = ip_address::parse(data_ip, data_port);
        if (!data_addr) {
            NEFORCE_THROW_EXCEPTION(ftp_exception("Invalid data channel address from PASV"));
        }

        tcp_socket data_sock;
        data_sock.open();
        static_cast<ip_socket&>(data_sock).connect(*data_addr);
        return data_sock;
    }

    tcp_socket listen_sock;
    listen_sock.open(use_ipv6 ? ip_address::family::INET6 : ip_address::family::INET4);
    listen_sock.bind(ip_address::any(ports(0U), use_ipv6 ? ip_address::family::INET6 : ip_address::family::INET4));
    listen_sock.listen(1);

    const auto bound = listen_sock.local_endpoint();
    if (!bound) {
        NEFORCE_THROW_EXCEPTION(ftp_exception("Failed to get bound address for active mode"));
    }

    const auto ctrl_local = local_endpoint();
    if (!ctrl_local) {
        NEFORCE_THROW_EXCEPTION(ftp_exception("Failed to get control connection local address"));
    }

    if (use_ipv6) {
        // RFC 2428 EPRT command: |2|<ipv6-address>|<tcp-port>|
        string ctrl_str = ctrl_local->to_string();
        const size_t port_col = ctrl_str.rfind(':');
        string ctrl_host = ctrl_str.view(0, port_col);
        if (ctrl_host.starts_with("[")) {
            ctrl_host = ctrl_host.view(1, ctrl_host.size() - 2);
        }
        const string eprt_cmd = "EPRT |2|" + ctrl_host + "|" + to_string(static_cast<uint16_t>(bound->port())) + "|";
        expect_code(200, eprt_cmd);
    } else {
        string local_ip;
        {
            char buf[INET_ADDRSTRLEN] = {};
            const auto* sa4 = reinterpret_cast<const ::sockaddr_in*>(ctrl_local->data());
            ::inet_ntop(AF_INET, &sa4->sin_addr, buf, sizeof(buf));
            local_ip = buf;
        }

        const uint16_t bp = static_cast<uint16_t>(bound->port());
        string ip_comma = local_ip;
        for (char& c: ip_comma) {
            if (c == '.') {
                c = ',';
            }
        }

        const string port_cmd = "PORT " + ip_comma + "," + to_string(bp >> 8) + "," + to_string(bp & 0xFF);
        expect_code(200, port_cmd);
    }

    {
        const int lfd = static_cast<int>(listen_sock.native_handle());
        ::fd_set fds;
        FD_ZERO(&fds);
        FD_SET(lfd, &fds);
        ::timeval tv{kActiveAcceptTimeoutSec, 0};
        const int sel = ::select(lfd + 1, &fds, nullptr, nullptr, &tv);
        if (sel < 0) {
            NEFORCE_THROW_EXCEPTION(socket_exception("select failed"));
        }
        if (sel == 0) {
            NEFORCE_THROW_EXCEPTION(socket_exception("accept timed out"));
        }
    }

    ::sockaddr_storage peer{};
    ::socklen_t plen = sizeof(peer);
    const auto client_fd = ::accept(listen_sock.native_handle(), reinterpret_cast<::sockaddr*>(&peer), &plen);
    if (client_fd == invalid_handle) {
        NEFORCE_THROW_EXCEPTION(socket_exception("PORT: accept failed"));
    }

    return tcp_socket(client_fd);
}

ssl_stream ftp_client::wrap_data_channel(tcp_socket sock) {
    if (ssl_ctx_ == nullptr) {
        NEFORCE_THROW_EXCEPTION(value_exception("ssl_context required for data channel TLS"));
    }

    ssl_stream data_ssl;
    data_ssl.reset(*ssl_ctx_);
    data_ssl.set_fd(sock.release());

    if (!sni_host_.empty()) {
        data_ssl.set_sni_hostname(sni_host_);
    }
    data_ssl.connect();

    return data_ssl;
}

void ftp_client::do_post_connect() {
    const auto greeting = read_response();
    if (greeting.code != 220) {
        close();
        NEFORCE_THROW_EXCEPTION(ftp_exception("FTP server did not send 220 greeting"));
    }
    connected_ = true;
}

vector<char> ftp_client::download_impl(const string& remote_path, tcp_socket& data_sock, const uint64_t offset) {
    if (offset > 0) {
        expect_code(350, "REST " + to_string(offset));
    }

    expect_codes({125, 150}, "RETR " + remote_path);

    vector<char> data;
    if (tls_active_ && data_tls_) {
        auto data_ssl = wrap_data_channel(move(data_sock));
        data = read_data_channel_tls(data_ssl);
    } else {
        data = read_data_channel(data_sock);
        data_sock.close();
    }

    ignore = read_response();
    return data;
}

void ftp_client::upload_impl(const string& remote_path, tcp_socket& data_sock, const char* data, const size_t len,
                             const uint64_t offset) {
    if (offset > 0) {
        expect_code(350, "REST " + to_string(offset));
    }

    expect_codes({125, 150}, "STOR " + remote_path);

    if (tls_active_ && data_tls_) {
        auto data_ssl = wrap_data_channel(move(data_sock));
        write_data_channel_tls(data_ssl, data, len);
    } else {
        write_data_channel(data_sock, data, len);
        data_sock.close();
    }

    ignore = read_response();
}

void ftp_client::open_and_connect(const ip_address& addr) {
    open_ip(addr.address_family(), type::STREAM, protocol::TCP);
    set_receive_timeout(15000_ms);
    if (::connect(fd_, addr.data(), addr.size()) != 0) {
        close();
        NEFORCE_THROW_EXCEPTION(socket_exception("Failed to connect to FTP server"));
    }
    clear_buffer();
}

ftp_client::entry ftp_client::parse_list_entry(const string& line) {
    entry e;
    e.raw = line;

    if (line.empty()) {
        return e;
    }

    if (line[0] >= '0' && line[0] <= '9') {
        size_t pos = 0;
        for (int field = 0; field < 2; ++field) {
            while (pos < line.size() && line[pos] == ' ') {
                ++pos;
            }
            while (pos < line.size() && line[pos] != ' ') {
                ++pos;
            }
        }

        while (pos < line.size() && line[pos] == ' ') {
            ++pos;
        }
        if (line.substr(pos, 5) == "<DIR>") {
            e.is_directory = true;
            pos += 5;
        } else {
            e.is_directory = false;
            size_t size_end = pos;
            while (size_end < line.size() && line[size_end] != ' ') {
                ++size_end;
            }
            e.size = static_cast<uint64_t>(to_int64(line.substr(pos, size_end - pos).view()));
            pos = size_end;
        }

        while (pos < line.size() && line[pos] == ' ') {
            ++pos;
        }
        e.name = line.substr(pos);
        return e;
    }

    e.is_directory = (line[0] == 'd');

    size_t pos = 0;
    int fields = 0;
    while (fields < 8 && pos < line.size()) {
        while (pos < line.size() && line[pos] == ' ') {
            ++pos;
        }

        const size_t start = pos;
        while (pos < line.size() && line[pos] != ' ') {
            ++pos;
        }

        if (fields == 4 && pos > start) {
            e.size = static_cast<uint64_t>(to_int64(line.substr(start, pos - start).view()));
        }
        ++fields;
    }

    while (pos < line.size() && line[pos] == ' ') {
        ++pos;
    }
    e.name = line.substr(pos);

    return e;
}

ftp_client::~ftp_client() {
    if (!connected_) {
        return;
    }
    try {
        disconnect();
        // NOLINTNEXTLINE(bugprone-empty-catch)
    } catch (...) {
        // ignore
    }
}

void ftp_client::connect(const ip_address& addr, const tls_mode mode, ssl_context* ctx, const string& sni_hostname) {
    if (!addr.is_valid()) {
        NEFORCE_THROW_EXCEPTION(value_exception("Invalid FTP server address"));
    }

    connected_ = false;
    tls_active_ = false;
    tls_mode_ = mode;
    ssl_ctx_ = ctx;
    sni_host_ = sni_hostname;
    data_tls_ = false;

    open_and_connect(addr);

    if (mode == tls_mode::implicit_) {
        do_ctrl_tls_handshake();
    }

    do_post_connect();

    if (mode == tls_mode::explicit_) {
        expect_code(234, "AUTH TLS");
        do_ctrl_tls_handshake();
    }
}

void ftp_client::connect(const string& hostname, const ports port, const tls_mode mode, dns_client* dns,
                         ssl_context* ctx, const string& sni) {
    if (hostname.empty()) {
        NEFORCE_THROW_EXCEPTION(value_exception("FTP hostname cannot be empty"));
    }

    server_host_ = hostname;
    server_port_ = port;

    auto direct = ip_address::parse(hostname, port);
    if (direct) {
        connect(*direct, mode, ctx, sni.empty() ? hostname : sni);
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
        NEFORCE_THROW_EXCEPTION(ftp_exception(("DNS resolution failed for FTP host: " + hostname).data()));
    }

    for (const auto& ip_str: ips) {
        auto addr = ip_address::parse(ip_str, port);
        if (!addr) {
            continue;
        }

        try {
            connected_ = false;
            tls_active_ = false;
            tls_mode_ = mode;
            ssl_ctx_ = ctx;
            sni_host_ = sni.empty() ? hostname : sni;
            data_tls_ = false;

            open_and_connect(*addr);

            if (mode == tls_mode::implicit_) {
                do_ctrl_tls_handshake();
                clear_buffer();
            }

            do_post_connect();

            if (mode == tls_mode::explicit_) {
                expect_code(234, "AUTH TLS");
                do_ctrl_tls_handshake();
            }

            return;

        } catch (...) {
            close();
            clear_buffer();
            connected_ = false;
        }
    }

    NEFORCE_THROW_EXCEPTION(socket_exception("Failed to connect to any resolved address for FTP host"));
}

ftp_client::tls_info ftp_client::upgrade_tls(ssl_context& ctx, const string& sni_hostname) {
    if (!is_connected()) {
        NEFORCE_THROW_EXCEPTION(ftp_exception("Not connected to FTP server"));
    }
    if (tls_active_) {
        NEFORCE_THROW_EXCEPTION(ftp_exception("TLS already active on control channel"));
    }

    ssl_ctx_ = &ctx;
    sni_host_ = sni_hostname;

    expect_code(234, "AUTH TLS");
    do_ctrl_tls_handshake();
    tls_mode_ = tls_mode::explicit_;

    tls_info info;
    info.active = true;
    info.cipher_name = ctrl_ssl_.get_cipher_name();
    info.tls_version = ctrl_ssl_.get_version();
    info.peer_verified = ctrl_ssl_.verify_peer();
    info.data_channel = data_tls_;
    return info;
}

void ftp_client::disconnect() {
    if (is_open()) {
        ignore = send_command("QUIT");
    }
    connected_ = false;
    tls_active_ = false;
    data_tls_ = false;
    ssl_ctx_ = nullptr;

    if (ctrl_ssl_.is_valid()) {
        ctrl_ssl_.close();
    }

    close();
}

void ftp_client::login(const string& username, const string& password) {
    if (!is_connected()) {
        NEFORCE_THROW_EXCEPTION(ftp_exception("Not connected to FTP server"));
    }

    const auto user_resp = send_command("USER " + username);
    if (user_resp.code == 331) {
        const auto resp = send_command("PASS " + password);
        if (resp.code != 230) {
            NEFORCE_THROW_EXCEPTION(ftp_exception("FTP login failed"));
        }
    } else if (user_resp.code != 230) {
        const string err =
                "FTP USER command failed [" + username + "] got=" + to_string(user_resp.code) + " " + user_resp.message;
        NEFORCE_THROW_EXCEPTION(ftp_exception(err.data()));
    }

    set_transfer_mode(transfer_mode::binary);
}

void ftp_client::login_anonymous() { login("anonymous", "anonymous@"); }

void ftp_client::set_transfer_mode(const transfer_mode mode) {
    transfer_mode_ = mode;
    expect_code(200, mode == transfer_mode::binary ? "TYPE I" : "TYPE A");
}

void ftp_client::set_data_protection(const bool protect) {
    if (!tls_active_) {
        NEFORCE_THROW_EXCEPTION(ftp_exception("TLS not active on control channel"));
    }
    expect_code(200, "PBSZ 0");
    expect_code(200, protect ? "PROT P" : "PROT C");
    data_tls_ = protect;
}

string ftp_client::pwd() {
    const auto resp = send_command("PWD");
    if (resp.code != 257) {
        NEFORCE_THROW_EXCEPTION(ftp_exception("PWD failed"));
    }
    const auto& msg = resp.message;
    const size_t q1 = msg.find('"');
    const size_t q2 = msg.rfind('"');
    if (q1 != string::npos && q2 != q1) {
        return msg.substr(q1 + 1, q2 - q1 - 1);
    }
    return msg;
}

void ftp_client::cwd(const string& path) { expect_code(250, "CWD " + path); }

void ftp_client::cdup() { expect_code(250, "CDUP"); }

void ftp_client::mkdir(const string& path) { expect_codes({257}, "MKD " + path); }

void ftp_client::rmdir(const string& path) { expect_code(250, "RMD " + path); }

vector<ftp_client::entry> ftp_client::list(const string& path) {
    auto data_sock = open_data_channel();

    const string cmd = path.empty() ? "LIST" : "LIST " + path;
    expect_codes({125, 150}, cmd);

    vector<char> raw;
    if (tls_active_ && data_tls_) {
        auto data_ssl = wrap_data_channel(move(data_sock));
        raw = read_data_channel_tls(data_ssl);
    } else {
        raw = read_data_channel(data_sock);
        data_sock.close();
    }

    ignore = read_response();

    vector<entry> entries;
    string line;
    for (size_t i = 0; i <= raw.size(); ++i) {
        if (i == raw.size() || raw[i] == '\n') {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            if (!line.empty()) {
                entries.push_back(parse_list_entry(line));
            }
            line.clear();
        } else {
            line += raw[i];
        }
    }
    return entries;
}

vector<string> ftp_client::nlst(const string& path) {
    auto data_sock = open_data_channel();

    const string cmd = path.empty() ? "NLST" : "NLST " + path;
    expect_codes({125, 150}, cmd);

    vector<char> raw;
    if (tls_active_ && data_tls_) {
        auto data_ssl = wrap_data_channel(move(data_sock));
        raw = read_data_channel_tls(data_ssl);
    } else {
        raw = read_data_channel(data_sock);
        data_sock.close();
    }

    ignore = read_response();

    vector<string> names;
    string line;
    for (size_t i = 0; i <= raw.size(); ++i) {
        if (i == raw.size() || raw[i] == '\n') {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            if (!line.empty()) {
                names.push_back(move(line));
            }
            line.clear();
        } else {
            line += raw[i];
        }
    }
    return names;
}

uint64_t ftp_client::file_size(const string& remote_path) {
    const auto resp = send_command("SIZE " + remote_path);
    if (resp.code != 213) {
        NEFORCE_THROW_EXCEPTION(ftp_exception("SIZE command failed"));
    }
    return static_cast<uint64_t>(to_int64(resp.message.view()));
}

void ftp_client::rename(const string& from, const string& to) {
    expect_code(350, "RNFR " + from);
    expect_code(250, "RNTO " + to);
}

void ftp_client::remove(const string& remote_path) { expect_code(250, "DELE " + remote_path); }

vector<char> ftp_client::download(const string& remote_path) {
    auto data_sock = open_data_channel();
    return download_impl(remote_path, data_sock, 0);
}

void ftp_client::upload(const string& remote_path, const char* data, const size_t len) {
    auto data_sock = open_data_channel();
    upload_impl(remote_path, data_sock, data, len, 0);
}

vector<char> ftp_client::download_resume(const string& remote_path, const uint64_t offset) {
    auto data_sock = open_data_channel();
    return download_impl(remote_path, data_sock, offset);
}

void ftp_client::upload_resume(const string& remote_path, const char* data, const size_t len, const uint64_t offset) {
    auto data_sock = open_data_channel();
    upload_impl(remote_path, data_sock, data, len, offset);
}

void ftp_client::noop() { expect_code(200, "NOOP"); }

ftp_client::tls_info ftp_client::get_tls_info() const noexcept {
    tls_info info;
    info.active = tls_active_;
    if (tls_active_) {
        info.cipher_name = ctrl_ssl_.get_cipher_name();
        info.tls_version = ctrl_ssl_.get_version();
        info.peer_verified = ctrl_ssl_.verify_peer();
        info.data_channel = data_tls_;
    }
    return info;
}

NEFORCE_END_NAMESPACE__
