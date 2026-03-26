#include <NeForce/core/utility/packages.hpp>
#include <NeForce/network/socket/ftp_socket.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    vector<char> read_data_channel(tcp_socket& sock) {
        vector<char> result;
        char buf[8192];
        while (true) {
            const ssize_t n = ::recv(sock.native_handle(), buf, sizeof(buf), 0);
            if (n <= 0) break;
            result.insert(result.end(), buf, buf + n);
        }
        return result;
    }

    vector<char> read_data_channel_tls(ssl_stream& stream) {
        vector<char> result;
        char buf[8192];
        while (true) {
            const ssize_t n = stream.read(buf, sizeof(buf));
            if (n <= 0) break;
            result.insert(result.end(), buf, buf + n);
        }
        return result;
    }

    void write_data_channel(tcp_socket& sock, const char* data, size_t len) {
        size_t total = 0;
        while (total < len) {
            const ssize_t n = ::send(
                static_cast<int>(sock.native_handle()),
                data + total,
                static_cast<int>(len - total), 0);
            if (n <= 0) {
                NEFORCE_THROW_EXCEPTION(ftp_exception("Data channel write failed"));
            }
            total += static_cast<size_t>(n);
        }
    }

    void write_data_channel_tls(ssl_stream& stream, const char* data, size_t len) {
        size_t total = 0;
        while (total < len) {
            const ssize_t n = stream.write(data + total, len - total);
            if (n <= 0) {
                NEFORCE_THROW_EXCEPTION(ftp_exception("TLS data channel write failed"));
            }
            total += static_cast<size_t>(n);
        }
    }
}


ssize_t ftp_socket::ctrl_send(const char* data, const size_t len) {
    if (tls_active_) {
        return ctrl_ssl_.write(data, len);
    }
    return ::send(static_cast<int>(fd_), data, static_cast<int>(len), 0);
}

ssize_t ftp_socket::ctrl_recv(char* buf, const size_t len) {
    if (tls_active_) {
        return ctrl_ssl_.read(buf, len);
    }
    return ::recv(static_cast<int>(fd_), buf, static_cast<int>(len), 0);
}

bool ftp_socket::ctrl_read_line(string& out) {
    out.clear();
    char ch;
    while (true) {
        const ssize_t n = ctrl_recv(&ch, 1);
        if (n <= 0) return false;
        if (ch == '\r') continue;
        if (ch == '\n') return true;
        out += ch;
    }
}

ftp_socket::response ftp_socket::read_response() {
    response resp{0, {}};
    string line;

    while (true) {
        if (!ctrl_read_line(line)) {
            NEFORCE_THROW_EXCEPTION(ftp_exception("Connection closed while reading FTP response"));
        }
        if (line.size() < 3) {
            NEFORCE_THROW_EXCEPTION(ftp_exception("Malformed FTP response"));
        }

        int code = 0;
        for (int i = 0; i < 3; ++i) {
            if (line[i] < '0' || line[i] > '9') {
                NEFORCE_THROW_EXCEPTION(ftp_exception("Invalid FTP response code"));
            }
            code = code * 10 + (line[i] - '0');
        }

        if (line.size() > 3 && line[3] == '-') {
            if (!resp.message.empty()) resp.message += '\n';
            resp.message += line.substr(4);
            resp.code = code;
            continue;
        }

        resp.code = code;
        if (line.size() > 4) {
            if (!resp.message.empty()) resp.message += '\n';
            resp.message += line.substr(4);
        }
        break;
    }

    return resp;
}

ftp_socket::response ftp_socket::send_command(const string& cmd) {
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

void ftp_socket::expect_code(const int expected, const string& cmd) {
    const auto resp = send_command(cmd);
    if (resp.code != expected) {
        const string err = "FTP command failed [" + cmd + "] expected=" +
                           to_string(expected) + " got=" +
                           to_string(resp.code) + " " + resp.message;
        NEFORCE_THROW_EXCEPTION(ftp_exception(err.data()));
    }
}

void ftp_socket::expect_codes(std::initializer_list<int> codes, const string& cmd) {
    const auto resp = send_command(cmd);
    for (const int c : codes) {
        if (resp.code == c) return;
    }
    const string err = "FTP command failed [" + cmd + "] got=" +
                       to_string(resp.code) + " " + resp.message;
    NEFORCE_THROW_EXCEPTION(ftp_exception(err.data()));
}

void ftp_socket::do_ctrl_tls_handshake() {
    if (!ssl_ctx_) {
        NEFORCE_THROW_EXCEPTION(value_exception("ssl_context required for FTPS"));
    }

    ctrl_ssl_.reset(*ssl_ctx_);
    ctrl_ssl_.set_fd(static_cast<ssl_stream::native_handle_type>(fd_));
    if (!sni_host_.empty()) {
        ctrl_ssl_.set_sni_hostname(sni_host_);
    }
    ctrl_ssl_.connect();

    tls_active_ = true;
}

void ftp_socket::send_pbsz_prot() {
    expect_code(200, "PBSZ 0");
    expect_code(200, "PROT P");
    data_tls_ = true;
}

tcp_socket ftp_socket::open_data_channel() {
    if (passive_mode_ == passive_mode::passive) {
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
            const auto token = (comma == string::npos)
                ? nums_str.substr(pos)
                : nums_str.substr(pos, comma - pos);
            nums[idx++] = integer32::parse(token);
            if (comma == string::npos) break;
            pos = comma + 1;
        }
        if (idx != 6) {
            NEFORCE_THROW_EXCEPTION(ftp_exception("Invalid PASV address format"));
        }

        const string data_ip = to_string(nums[0]) + "." + to_string(nums[1]) + "." +
                               to_string(nums[2]) + "." + to_string(nums[3]);
        const uint16_t data_port = static_cast<uint16_t>(nums[4] * 256 + nums[5]);

        auto data_addr = ip_address::parse(data_ip, data_port);
        if (!data_addr) {
            NEFORCE_THROW_EXCEPTION(ftp_exception("Invalid data channel address from PASV"));
        }

        tcp_socket data_sock;
        data_sock.open(AF_INET);
        data_sock.connect(*data_addr);
        return data_sock;

    } else {
        const auto local = local_endpoint();
        if (!local) {
            NEFORCE_THROW_EXCEPTION(ftp_exception("Failed to get local endpoint for PORT"));
        }

        tcp_socket listen_sock;
        listen_sock.open(AF_INET);
        listen_sock.bind(ip_address::any(0, AF_INET));
        listen_sock.listen(1);

        const auto bound = listen_sock.local_endpoint();
        if (!bound) {
            NEFORCE_THROW_EXCEPTION(ftp_exception("PORT: failed to get bound address"));
        }

        string local_ip;
        {
            char buf[INET_ADDRSTRLEN] = {};
            const auto* sa4 = reinterpret_cast<const ::sockaddr_in*>(local->data());
            ::inet_ntop(AF_INET, &sa4->sin_addr, buf, sizeof(buf));
            local_ip = buf;
        }

        const uint16_t bp = bound->port();
        string ip_comma = local_ip;
        for (char& c : ip_comma) if (c == '.') c = ',';

        const string port_cmd = "PORT " + ip_comma + "," +
                                to_string(bp >> 8) + "," + to_string(bp & 0xFF);
        expect_code(200, port_cmd);

        {
            const int lfd = static_cast<int>(listen_sock.native_handle());
            ::fd_set fds;
            FD_ZERO(&fds);
            FD_SET(lfd, &fds);
            constexpr ::timeval tv { kActiveAcceptTimeoutSec, 0 };
            const int sel = ::select(lfd + 1, &fds, nullptr, nullptr, &tv);
            if (sel < 0) {
                NEFORCE_THROW_EXCEPTION(socket_exception("PORT: select failed"));
            }
            if (sel == 0) {
                NEFORCE_THROW_EXCEPTION(socket_exception("PORT: accept timed out"));
            }
        }

        ::sockaddr_storage peer{};
        ::socklen_t plen = sizeof(peer);
        const auto client_fd = ::accept(
            static_cast<int>(listen_sock.native_handle()),
            reinterpret_cast<::sockaddr*>(&peer), &plen);
        if (client_fd == INVALID_SOCKET) {
            NEFORCE_THROW_EXCEPTION(socket_exception("PORT: accept failed"));
        }

        return tcp_socket(client_fd);
    }
}

ssl_stream ftp_socket::wrap_data_channel(tcp_socket&& sock) {
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

void ftp_socket::do_post_connect() {
    const auto greeting = read_response();
    if (greeting.code != 220) {
        close();
        NEFORCE_THROW_EXCEPTION(ftp_exception("FTP server did not send 220 greeting"));
    }
    connected_ = true;
}

vector<char> ftp_socket::download_impl(const string& remote_path,
                                       tcp_socket& data_sock,
                                       const uint64_t offset) {
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

    NEFORCE_IGNORE read_response();
    return data;
}

void ftp_socket::upload_impl(const string& remote_path,
                             tcp_socket& data_sock,
                             const char* data, const size_t len,
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

    NEFORCE_IGNORE read_response();
}

ftp_socket::entry ftp_socket::parse_list_entry(const string& line) {
    entry e;
    e.raw = line;

    if (line.empty()) return e;

    if (line[0] >= '0' && line[0] <= '9') {
        size_t pos = 0;
        for (int field = 0; field < 2; ++field) {
            while (pos < line.size() && line[pos] == ' ') ++pos;
            while (pos < line.size() && line[pos] != ' ') ++pos;
        }

        while (pos < line.size() && line[pos] == ' ') ++pos;
        if (line.substr(pos, 5) == "<DIR>") {
            e.is_directory = true;
            pos += 5;
        } else {
            e.is_directory = false;
            size_t size_end = pos;
            while (size_end < line.size() && line[size_end] != ' ') ++size_end;
            e.size = static_cast<uint64_t>(to_int64(line.substr(pos, size_end - pos).view()));
            pos = size_end;
        }

        while (pos < line.size() && line[pos] == ' ') ++pos;
        e.name = line.substr(pos);
        return e;
    }

    e.is_directory = (line[0] == 'd');

    size_t pos = 0;
    int fields = 0;
    while (fields < 8 && pos < line.size()) {
        while (pos < line.size() && line[pos] == ' ') ++pos;

        const size_t start = pos;
        while (pos < line.size() && line[pos] != ' ') ++pos;

        if (fields == 4 && pos > start) {
            e.size = static_cast<uint64_t>(
                to_int64(line.substr(start, pos - start).view()));
        }
        ++fields;
    }

    while (pos < line.size() && line[pos] == ' ') ++pos;
    e.name = line.substr(pos);

    return e;
}

ftp_socket::~ftp_socket() {
    if (connected_) {
        disconnect();
    }
}

void ftp_socket::connect(const ip_address& addr, const tls_mode mode,
                         ssl_context* ctx, const string& sni_hostname) {
    if (!addr.is_valid()) {
        NEFORCE_THROW_EXCEPTION(value_exception("Invalid FTP server address"));
    }

    close();
    connected_  = false;
    tls_active_ = false;
    tls_mode_   = mode;
    ssl_ctx_  = ctx;
    sni_host_ = sni_hostname;
    data_tls_ = false;

    fd_ = ::socket(addr.family(), SOCK_STREAM, IPPROTO_TCP);
    if (!is_open()) {
        NEFORCE_THROW_EXCEPTION(socket_exception("Failed to create TCP socket for FTP"));
    }

    if (::connect(fd_, addr.data(), addr.size()) != 0) {
        close();
        NEFORCE_THROW_EXCEPTION(socket_exception("Failed to connect to FTP server"));
    }

    if (mode == tls_mode::implicit_) {
        do_ctrl_tls_handshake();
    }

    do_post_connect();

    if (mode == tls_mode::explicit_) {
        expect_code(234, "AUTH TLS");
        do_ctrl_tls_handshake();
    }
}

void ftp_socket::connect(const string& hostname, const uint16_t port, const tls_mode mode,
                         dns_client* dns, ssl_context* ctx, const string& sni) {
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
    if (!resolver) {
        local_dns.emplace();
        resolver = &(*local_dns);
    }

    const auto ips = resolver->resolve_a(hostname.view());
    if (ips.empty()) {
        NEFORCE_THROW_EXCEPTION(ftp_exception(("DNS resolution failed for FTP host: " + hostname).data()));
    }

    for (const auto& ip_str : ips) {
        auto addr = ip_address::parse(ip_str, port);
        if (!addr) continue;

        close();
        fd_ = ::socket(addr->family(), SOCK_STREAM, IPPROTO_TCP);
        if (!is_open()) continue;

        if (::connect(fd_, addr->data(), addr->size()) != 0) {
            close();
            continue;
        }

        connected_  = false;
        tls_active_ = false;
        tls_mode_   = mode;
        ssl_ctx_  = ctx;
        sni_host_ = sni.empty() ? hostname : sni;
        data_tls_ = false;

        if (mode == tls_mode::implicit_) {
            do_ctrl_tls_handshake();
        }

        do_post_connect();

        if (mode == tls_mode::explicit_) {
            expect_code(234, "AUTH TLS");
            do_ctrl_tls_handshake();
        }
        return;
    }

    NEFORCE_THROW_EXCEPTION(socket_exception("Failed to connect to any resolved address for FTP host"));
}

ftp_socket::tls_info ftp_socket::upgrade_tls(ssl_context& ctx, const string& sni_hostname) {
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

void ftp_socket::disconnect() {
    if (is_open()) {
        NEFORCE_IGNORE send_command("QUIT");
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

void ftp_socket::login(const string& username, const string& password) {
    if (!is_connected()) {
        NEFORCE_THROW_EXCEPTION(ftp_exception("Not connected to FTP server"));
    }

    expect_code(331, "USER " + username);

    const auto resp = send_command("PASS " + password);
    if (resp.code != 230) {
        NEFORCE_THROW_EXCEPTION(ftp_exception("FTP login failed"));
    }

    set_transfer_mode(transfer_mode::binary);
}

void ftp_socket::login_anonymous() {
    login("anonymous", "anonymous@");
}

void ftp_socket::set_transfer_mode(const transfer_mode mode) {
    transfer_mode_ = mode;
    expect_code(200, mode == transfer_mode::binary ? "TYPE I" : "TYPE A");
}

void ftp_socket::set_data_protection(const bool protect) {
    if (!tls_active_) {
        NEFORCE_THROW_EXCEPTION(ftp_exception("TLS not active on control channel"));
    }
    expect_code(200, "PBSZ 0");
    expect_code(200, protect ? "PROT P" : "PROT C");
    data_tls_ = protect;
}

string ftp_socket::pwd() {
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

void ftp_socket::cwd(const string& path) {
    expect_code(250, "CWD " + path);
}

void ftp_socket::cdup() {
    expect_code(250, "CDUP");
}

void ftp_socket::mkdir(const string& path) {
    expect_codes({257}, "MKD " + path);
}

void ftp_socket::rmdir(const string& path) {
    expect_code(250, "RMD " + path);
}

vector<ftp_socket::entry> ftp_socket::list(const string& path) {
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

    NEFORCE_IGNORE read_response();

    vector<entry> entries;
    string line;
    for (size_t i = 0; i <= raw.size(); ++i) {
        if (i == raw.size() || raw[i] == '\n') {
            if (!line.empty() && line.back() == '\r') line.pop_back();
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

vector<string> ftp_socket::nlst(const string& path) {
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

    NEFORCE_IGNORE read_response();

    vector<string> names;
    string line;
    for (size_t i = 0; i <= raw.size(); ++i) {
        if (i == raw.size() || raw[i] == '\n') {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (!line.empty()) names.push_back(move(line));
            line.clear();
        } else {
            line += raw[i];
        }
    }
    return names;
}

uint64_t ftp_socket::file_size(const string& remote_path) {
    const auto resp = send_command("SIZE " + remote_path);
    if (resp.code != 213) {
        NEFORCE_THROW_EXCEPTION(ftp_exception("SIZE command failed"));
    }
    return static_cast<uint64_t>(to_int64(resp.message.view()));
}

void ftp_socket::rename(const string& from, const string& to) {
    expect_code(350, "RNFR " + from);
    expect_code(250, "RNTO " + to);
}

void ftp_socket::remove(const string& remote_path) {
    expect_code(250, "DELE " + remote_path);
}

vector<char> ftp_socket::download(const string& remote_path) {
    auto data_sock = open_data_channel();
    return download_impl(remote_path, data_sock, 0);
}

void ftp_socket::upload(const string& remote_path, const char* data, const size_t len) {
    auto data_sock = open_data_channel();
    upload_impl(remote_path, data_sock, data, len, 0);
}

vector<char> ftp_socket::download_resume(const string& remote_path, const uint64_t offset) {
    auto data_sock = open_data_channel();
    return download_impl(remote_path, data_sock, offset);
}

void ftp_socket::upload_resume(const string& remote_path,
                               const char* data, const size_t len,
                               const uint64_t offset) {
    auto data_sock = open_data_channel();
    upload_impl(remote_path, data_sock, data, len, offset);
}

void ftp_socket::noop() {
    expect_code(200, "NOOP");
}

ftp_socket::tls_info ftp_socket::get_tls_info() const noexcept {
    tls_info info;
    info.active = tls_active_;
    if (tls_active_) {
        info.cipher_name   = ctrl_ssl_.get_cipher_name();
        info.tls_version   = ctrl_ssl_.get_version();
        info.peer_verified = ctrl_ssl_.verify_peer();
        info.data_channel  = data_tls_;
    }
    return info;
}

NEFORCE_END_NAMESPACE__
