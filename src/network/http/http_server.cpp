#include <NeForce/core/encrypt/base64.hpp>
#include <NeForce/core/encrypt/sha1.hpp>
#include <NeForce/core/numeric/random.hpp>
#include <NeForce/core/utility/hexadecimal.hpp>
#include <NeForce/core/utility/scope.hpp>
#include <NeForce/network/http/http_server.hpp>
#include <NeForce/network/http/http2_connection.hpp>
#include <NeForce/network/ssl/ssl_socket.hpp>
#include <NeForce/network/tcp/tcp_client.hpp>
#include <NeForce/network/util/url.hpp>
#ifdef NEFORCE_PLATFORM_LINUX
#    include <poll.h>
#    include <sys/uio.h>
#endif
#include <cerrno>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

namespace {
    constexpr int max_forward_count = 5;

    bool parse_header_value_ci(const string& data, const string_view lower_key, size_t header_end, string& value_out) {
        const string data_lower = string(data.view(0, header_end)).lowercase();
        const size_t pos = data_lower.find(string(lower_key));
        if (pos == string::npos) {
            return false;
        }
        const size_t val_end = data.find("\r\n", pos);
        if (val_end == string::npos || val_end > header_end) {
            return false;
        }
        value_out = data.substr(pos + lower_key.length(), val_end - pos - lower_key.length()).trim();
        return true;
    }

    string compute_websocket_accept(const string_view key) {
        constexpr string_view websocket_guid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
        const string combined = string(key) + websocket_guid;
        const string sha1_result = sha1(combined);
        return base64_encode(cbyte_view{reinterpret_cast<const byte_t*>(sha1_result.data()), sha1_result.size()});
    }

    string decode_chunked_body(const string_view chunked_data, unordered_map<string, string>* trailers_out = nullptr) {
        string decoded;
        size_t pos = 0;

        while (pos < chunked_data.size()) {
            size_t line_end = chunked_data.find("\r\n", pos);
            if (line_end == string::npos) {
                NEFORCE_THROW_EXCEPTION(http_exception("Malformed chunked body: missing chunk size line"));
            }
            string_view size_line = chunked_data.view(pos, line_end - pos).trim();
            size_t semi = size_line.find(';');
            if (semi != string_view::npos) {
                size_line = size_line.head(semi);
            }
            uint64_t chunk_size = 0;
            try {
                chunk_size = hexadecimal::parse(size_line).value();
            } catch (...) {
                NEFORCE_THROW_EXCEPTION(http_exception("Malformed chunked body: invalid chunk size"));
            }
            pos = line_end + 2;

            if (chunk_size == 0) {
                if (trailers_out != nullptr) {
                    while (pos < chunked_data.size()) {
                        size_t trailer_end = chunked_data.find("\r\n", pos);
                        if (trailer_end == string::npos || trailer_end == pos) {
                            if (trailer_end == pos) {
                                pos = trailer_end + 2;
                            }
                            break;
                        }
                        string_view trailer_line = chunked_data.view(pos, trailer_end - pos);
                        size_t colon = trailer_line.find(':');
                        if (colon != string_view::npos) {
                            string key(trailer_line.view(0, colon).trim());
                            string val(trailer_line.view(colon + 1).trim());
                            (*trailers_out)[move(key)] = move(val);
                        }
                        pos = trailer_end + 2;
                    }
                } else {
                    while (pos < chunked_data.size()) {
                        size_t trailer_end = chunked_data.find("\r\n", pos);
                        if (trailer_end == string::npos || trailer_end == pos) {
                            if (trailer_end == pos) {
                                pos = trailer_end + 2;
                            }
                            break;
                        }
                        pos = trailer_end + 2;
                    }
                }
                break;
            }

            if (pos + chunk_size + 2 > chunked_data.size()) {
                NEFORCE_THROW_EXCEPTION(http_exception("Malformed chunked body: chunk exceeds data"));
            }
            decoded.append(chunked_data.view(pos, chunk_size));
            pos += chunk_size + 2;
        }
        return decoded;
    }

    void parse_parameters(http_request& request) {
        if (!request.query.empty()) {
            url::parse_query(request.query.view(), request.parameters);
        }

        if (request.method.is_post() && !request.body.empty()) {
            const auto content_type = request.content_type();
            if (http_content::is_form_app(content_type)) {
                url::parse_query(request.body.view(), request.form_data);
            }
        }
    }

    string generate_session_id() {
        string str;
        str.reserve(32);
        for (int i = 0; i < 32; ++i) {
            str += format("{:x}", secret::next_int<uint32_t>(16));
        }
        return move(str);
    }
} // namespace


http_server::session_manager::session_manager() :
cleanup_running_(true) {
    cleanup_thread_.start(&session_manager::cleanup_expired_sessions, this);
}

http_server::session_manager::~session_manager() {
    cleanup_running_ = false;
    cv_.notify_one();
    if (cleanup_thread_.joinable()) {
        cleanup_thread_.join();
    }
}

http_session* http_server::session_manager::get_session(const string& session_id, const bool create) {
    lock<mutex> lock(mutex_);

    const auto session_iter = sessions_.find(session_id);
    if (session_iter != sessions_.end()) {
        if (session_iter->second.is_valid()) {
            session_iter->second.touch();
            return &session_iter->second;
        }
        sessions_.erase(session_iter);
    }

    if (create) {
        if (sessions_.size() >= max_sessions_) {
            auto oldest = sessions_.begin();
            for (auto it = sessions_.begin(); it != sessions_.end(); ++it) {
                if (it->second.last_access < oldest->second.last_access) {
                    oldest = it;
                }
            }
            sessions_.erase(oldest);
        }

        string new_id = session_id.empty() ? generate_session_id() : session_id;

        while (sessions_.find(new_id) != sessions_.end()) {
            new_id = generate_session_id();
        }

        http_session tmp;
        tmp.id = new_id;
        const auto pir = sessions_.emplace(new_id, move(tmp));
        return &pir.first->second;
    }

    return nullptr;
}

void http_server::session_manager::remove_session(const string& session_id) noexcept {
    lock<mutex> lock(mutex_);
    sessions_.erase(session_id);
}

void http_server::session_manager::cleanup_expired_sessions() {
    while (cleanup_running_) {
        {
            lock<mutex> lock(mutex_);
            for (auto it = sessions_.begin(); it != sessions_.end();) {
                if (!it->second.is_valid() || it->second.expired()) {
                    it = sessions_.erase(it);
                } else {
                    ++it;
                }
            }
        }
        unique_lock<mutex> lk(mutex_);
        cv_.wait_for(lk, cleanup_interval_, [&] { return !cleanup_running_.load(); });
    }
}

bool http_server::session_manager::session_exists(const string& session_id) const noexcept {
    lock<mutex> lk(mutex_);
    const auto it = sessions_.find(session_id);
    return it != sessions_.end() && it->second.is_valid();
}

size_t http_server::session_manager::session_count() const noexcept {
    lock<mutex> lk(mutex_);
    return sessions_.size();
}

void http_server::session_manager::set_cleanup_interval(const seconds interval) noexcept {
    cleanup_interval_ = interval;
}

void http_server::session_manager::set_max_sessions(const size_t max) noexcept { max_sessions_ = max; }

http_request http_server::parse_request(tcp_socket* client_socket, session_manager& manager,
                                        const http_cookie_name& name, const byte_size max_header_size,
                                        const byte_size max_body_size, const size_t max_header_count,
                                        const milliseconds body_read_timeout) {

    const int sock_fd = static_cast<int>(client_socket->native_handle());
    timeval original_timeout = {};
    bool timeout_modified = false;

    if (body_read_timeout.count() > 0) {
        socklen_t optlen = sizeof(original_timeout);
        if (::getsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, &original_timeout, &optlen) == 0) {
            timeout_modified = true;
        }
        const timeval tv{body_read_timeout.count() / 1000, (body_read_timeout.count() % 1000) * 1000};
        ::setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    }

    scope_exit restore_timeout([&] {
        if (timeout_modified) {
            ::setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, &original_timeout, sizeof(original_timeout));
        }
    });

    string request_data;
    char buffer[8192];

    while (true) {
        const ssize_t bytes_read = client_socket->receive(memory_view<char>(buffer));
        if (bytes_read <= 0) {
            NEFORCE_THROW_EXCEPTION(http_exception("Connection closed while reading request"));
        }
        request_data.append(buffer, bytes_read);

        const size_t header_end = request_data.find("\r\n\r\n");
        if (header_end != string::npos) {
            bool expect_continue = false;
            {
                string expect_val;
                if (parse_header_value_ci(request_data, "expect:", header_end, expect_val)) {
                    expect_continue = expect_val.lowercase() == "100-continue";
                }
            }

            size_t content_length = 0;

            string cl_value;
            if (parse_header_value_ci(request_data, "content-length:", header_end, cl_value)) {
                try {
                    content_length = uinteger64::parse(cl_value.view()).value();
                } catch (...) {
                    NEFORCE_THROW_EXCEPTION(http_exception("Invalid Content-Length"));
                }
            }

            bool is_chunked = false;
            string te_value;
            if (parse_header_value_ci(request_data, "transfer-encoding:", header_end, te_value)) {
                is_chunked = te_value.lowercase().contains("chunked");
            }

            const size_t body_start = header_end + 4;

            if (expect_continue && (content_length > 0 || is_chunked) && content_length <= max_body_size.bytes()) {
                static constexpr string_view continue_resp = "HTTP/1.1 100 Continue\r\n\r\n";
                client_socket->send_all(memory_view<const char>(continue_resp.data(), continue_resp.size()));
            }

            if (is_chunked) {
                if (request_data.size() - body_start > max_body_size.bytes()) {
                    NEFORCE_THROW_EXCEPTION(http_exception("Request body too large"));
                }
                while (request_data.find("\r\n0\r\n\r\n", body_start) == string::npos &&
                       request_data.view(body_start) != "0\r\n\r\n") {
                    if (request_data.size() > max_body_size.bytes()) {
                        NEFORCE_THROW_EXCEPTION(http_exception("Request body too large"));
                    }
                    const ssize_t n = client_socket->receive(memory_view<char>(buffer));
                    if (n <= 0) {
                        NEFORCE_THROW_EXCEPTION(http_exception("Connection closed while reading chunked body"));
                    }
                    request_data.append(buffer, n);
                }
                string decoded = decode_chunked_body(request_data.view(body_start));
                request_data.resize(body_start);
                request_data += decoded;
            } else {
                if (content_length > max_body_size.bytes()) {
                    NEFORCE_THROW_EXCEPTION(http_exception("Request body too large"));
                }

                const size_t body_received = request_data.size() - body_start;

                if (body_received < content_length) {
                    size_t remaining = content_length - body_received;
                    request_data.reserve(body_start + content_length);

                    while (remaining > 0) {
                        const size_t to_read = (remaining < sizeof(buffer)) ? remaining : sizeof(buffer);
                        const ssize_t n =
                                client_socket->receive(memory_view<char>(static_cast<char*>(buffer), to_read));

                        if (n <= 0) {
                            NEFORCE_THROW_EXCEPTION(http_exception("Connection closed while reading body"));
                        }

                        request_data.append(static_cast<char*>(buffer), n);
                        remaining -= n;
                    }
                }
            }
            break;
        }

        if (request_data.size() > max_header_size.bytes()) {
            NEFORCE_THROW_EXCEPTION(http_exception("Request header too large"));
        }
    }

    http_request request = http_request::parse(request_data.view());

    if (max_header_count > 0 && request.headers.size() > max_header_count) {
        NEFORCE_THROW_EXCEPTION(http_exception("Too many request headers"));
    }

    parse_parameters(request);

    const string& session_id = request.cookie(name.cookie_name());
    if (!session_id.empty() && manager.session_exists(session_id)) {
        request.session = manager.get_session(session_id, false);
    }

    return request;
}

http_session* http_server::get_or_create_session(http_request& request, const bool create, session_manager& manager,
                                                 const http_cookie_name& name) {

    http_session* sess = request.session;
    if (sess != nullptr) {
        return sess;
    }

    const string& session_id = request.cookie(name.cookie_name());
    if (!session_id.empty()) {
        sess = manager.get_session(session_id, false);
    }

    if (sess == nullptr && create) {
        sess = manager.get_session("", true);
    }
    request.session = sess;
    return sess;
}

void send_response(tcp_socket* client_socket, const http_response& response) {
    const string header = response.build_header_string();

#ifdef NEFORCE_PLATFORM_LINUX
    if (!client_socket->is_ssl() && response.redirect_url.empty() && !response.body.empty()) {
        ::iovec iov[2];
        iov[0].iov_base = const_cast<char*>(header.data());
        iov[0].iov_len = header.size();
        iov[1].iov_base = const_cast<char*>(response.body.data());
        iov[1].iov_len = response.body.size();

        while (iov[0].iov_len > 0) {
            const ssize_t n = ::writev(client_socket->native_handle(), iov, 2);
            if (n <= 0) {
                NEFORCE_THROW_EXCEPTION(socket_exception("send_response writev failed"));
            }
            auto written = static_cast<size_t>(n);
            if (written >= iov[0].iov_len) {
                written -= iov[0].iov_len;
                iov[0].iov_len = 0;
                iov[1].iov_base = static_cast<char*>(iov[1].iov_base) + written;
                iov[1].iov_len -= written;
            } else {
                iov[0].iov_base = static_cast<char*>(iov[0].iov_base) + written;
                iov[0].iov_len -= written;
            }
        }
        return;
    }
#endif

    const string data = header + response.body;
    client_socket->send_all(memory_view<const char>(data.data(), data.size()));
}

void add_session_cookie(const http_request& request, http_response& response, http_session* session,
                        const http_cookie_name& name) {

    if (session == nullptr || !session->is_new) {
        return;
    }

    http_cookie session_cookie;
    session_cookie.name = name;
    session_cookie.value = session->id;
    session_cookie.http_only = true;

    const bool is_https = request.header(http_key::X_Forwarded_Proto()) == "https";
    session_cookie.secure = is_https;
    session_cookie.same_site = is_https ? http_key::Strict() : http_key::Lax();

    response.cookies.emplace_back(move(session_cookie));
    session->is_new = false;
}

void send_error_response(tcp_socket* client_socket, const http_status status, const string& message) {
    try {
        http_response error_response;
        error_response.status = status;
        error_response.status_message = http_status_message(status);
        error_response.set_content_type(http_content::HTML_TEXT());
        error_response.body = "<!DOCTYPE html>"
                              "<html><head><title>Error</title></head>"
                              "<body><h1>" +
                              error_response.status_message +
                              "</h1>"
                              "<p>" +
                              message +
                              "</p>"
                              "</body></html>";
        send_response(client_socket, error_response);
        // NOLINTNEXTLINE(bugprone-empty-catch)
    } catch (...) {
        // ignore
    }
}

void http_server::handle_client(unique_ptr<tcp_socket> client_socket) {
    static constexpr size_t max_keep_alive_requests = 100;

    string client_ip;
    if (max_connections_per_ip > 0) {
        const auto endpoint = client_socket->remote_endpoint();
        if (endpoint.has_value()) {
            client_ip = endpoint.value().to_string();
            lock<mutex> lk(conn_mutex_);
            if (conn_per_ip_[client_ip] >= max_connections_per_ip) {
                return;
            }
            ++conn_per_ip_[client_ip];
        }
    }

    auto ip_cleanup = scope_exit([&]() noexcept {
        if (client_ip.empty()) {
            return;
        }
        lock<mutex> lk(conn_mutex_);
        auto it = conn_per_ip_.find(client_ip);
        if (it != conn_per_ip_.end() && --it->second == 0) {
            conn_per_ip_.erase(it);
        }
    });

    // ALPN h2 detection: If the TLS handshake is agreed to h2, the HTTP/2 connection is directly initiated
    if (client_socket->is_ssl()) {
        auto* ssl_sock = dynamic_cast<ssl_socket*>(client_socket.get());
        if (ssl_sock != nullptr) {
            string alpn = ssl_sock->get_alpn_negotiated();
            if (alpn == "h2") {
                auto loop = make_shared<event_loop>();
                auto conn = make_shared<http2_connection>(move(client_socket), loop);
                conn->set_router(&router_);
                conn->start();
                thread t([loop]() { loop->run(); });
                {
                    lock<mutex> lk(h2c_mutex_);
                    h2c_loops_.push_back(loop);
                    h2c_threads_.push_back(move(t));
                }
                return;
            }
        }
    }

    bool keep_alive = true;
    for (size_t req_count = 0; req_count < max_keep_alive_requests && keep_alive; ++req_count) {
        try {
            http_request request =
                    parse_request(client_socket.get(), session_manager_, cookie_name_, max_server_header_size,
                                  max_server_body_size, max_header_count, body_read_timeout);

            if (client_socket->is_ssl()) {
                request.set_header(http_key::X_Forwarded_Proto(), "https");
            }

            if (enable_connect && request.method.is_connect()) {
                handle_connect(move(client_socket), request);
                return;
            }

            if (try_upgrade(client_socket, request)) {
                return;
            }

            http_session* sess = get_or_create_session(request, true, session_manager_, cookie_name_);
            // NOLINTNEXTLINE(clang-analyzer-cplusplus.Move)
            handle_request_with_forward(*client_socket, request, sess);

            keep_alive = request.is_keep_alive();
        } catch (const http_exception& e) {
            send_error_response(client_socket.get(), http_status::S4_BAD_REQUEST, e.what());
            keep_alive = false;
        } catch (const exception& e) {
            send_error_response(client_socket.get(), http_status::S5_INTERNAL_SERVER_ERROR, e.what());
            keep_alive = false;
        } catch (...) {
            send_error_response(client_socket.get(), http_status::S5_INTERNAL_SERVER_ERROR, "Unknown internal error");
            keep_alive = false;
        }
    }
}

bool http_server::try_upgrade(unique_ptr<tcp_socket>& client_socket, http_request& request) {
    const string upgrade = request.header("Upgrade");
    const string connection = request.header("Connection");

    if (upgrade.empty() || !connection.lowercase().contains("upgrade")) {
        return false;
    }

    {
        const string proto_lower = upgrade.lowercase();
        auto it = upgrade_handlers_.find(string(proto_lower));
        if (it != upgrade_handlers_.end() && it->second) {
            if (it->second(request, client_socket.get())) {
                static_cast<void>(client_socket.release());
                return true;
            }
            return false;
        }
    }

    if (enable_websocket) {
        const string upgrade_lower = upgrade.lowercase();
        if (upgrade_lower == "websocket") {
            const string_view key = request.header("Sec-WebSocket-Key");
            if (key.empty()) {
                return false;
            }

            string accept = compute_websocket_accept(key);

            http_response upgrade_response;
            upgrade_response.status = http_status::S1_SWITCHING_PROTOCOLS;
            upgrade_response.status_message = "Switching Protocols";
            upgrade_response.set_header("Upgrade", "websocket");
            upgrade_response.set_header("Connection", "Upgrade");
            upgrade_response.set_header("Sec-WebSocket-Accept", move(accept));

            const string_view ws_extensions = request.header("Sec-WebSocket-Extensions");
#ifdef NEFORCE_SUPPORT_ZLIB
            if (!ws_extensions.empty()) {
                const auto deflate_cfg = websocket_deflate_config::negotiate(ws_extensions);
                if (deflate_cfg.active) {
                    upgrade_response.set_header("Sec-WebSocket-Extensions", deflate_cfg.to_response_header());
                }
            }
#endif

            send_response(client_socket.get(), upgrade_response);

            return ws_server_.handle_upgrade(request, move(client_socket));
        }
    }

    http_response not_impl;
    not_impl.status = http_status::S5_NOT_IMPLEMENTED;
    not_impl.status_message = "Not Implemented";
    not_impl.set_content_type(http_content::PLAIN_TEXT());
    not_impl.body = "Unsupported upgrade protocol";
    send_response(client_socket.get(), not_impl);
    return true;
}

void http_server::handle_connect(const unique_ptr<tcp_socket>& client_socket, http_request& request) {
    const string_view path = request.path.view();
    size_t colon = path.find(':');
    if (colon == string_view::npos) {
        send_error_response(client_socket.get(), http_status::S4_BAD_REQUEST, "Invalid CONNECT target");
        return;
    }

    const string host(path.view(0, colon));
    const string_view port_str(path.view(colon + 1));
    uint16_t port_num = 0;
    try {
        port_num = static_cast<uint16_t>(uinteger64::parse(port_str).value());
    } catch (...) {
        send_error_response(client_socket.get(), http_status::S4_BAD_REQUEST, "Invalid port in CONNECT target");
        return;
    }

    tcp_client tunnel;
    tunnel.set_connect_timeout(milliseconds(10000));
    if (!tunnel.connect(host, ports{port_num})) {
        send_error_response(client_socket.get(), http_status::S5_BAD_GATEWAY, "Failed to connect to tunnel target");
        return;
    }

    http_response established;
    established.version = request.version;
    established.status = http_status::S2_OK;
    established.status_message = "Connection Established";
    send_response(client_socket.get(), established);

    const int client_fd = static_cast<int>(client_socket->native_handle());
    const int tunnel_fd = static_cast<int>(tunnel.socket().native_handle());

    client_socket->set_nonblocking(true);
    tunnel.socket().set_nonblocking(true);

    char buffer[8192];
    bool client_open = true;
    bool tunnel_open = true;

    while (client_open && tunnel_open) {
        pollfd fds[2];
        int nfds = 0;

        if (client_open) {
            fds[nfds].fd = client_fd;
            fds[nfds].events = POLLIN;
            fds[nfds].revents = 0;
            nfds++;
        }
        if (tunnel_open) {
            fds[nfds].fd = tunnel_fd;
            fds[nfds].events = POLLIN;
            fds[nfds].revents = 0;
            nfds++;
        }

        const int ret = ::poll(fds, nfds, -1);
        if (ret < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }

        for (int i = 0; i < nfds; ++i) {
            if ((fds[i].revents & POLLIN) == 0) {
                continue;
            }

            const bool from_client = (fds[i].fd == client_fd);
            tcp_socket& src = from_client ? *client_socket : tunnel.socket();
            tcp_socket& dst = from_client ? tunnel.socket() : *client_socket;
            bool& src_open = from_client ? client_open : tunnel_open;

            const ssize_t n = src.receive(memory_view<char>(buffer));
            if (n <= 0) {
                src_open = false;
                continue;
            }
            dst.send_all(memory_view<const char>(buffer, n));
        }
    }
}

void http_server::handle_request_with_forward(tcp_socket& client_socket, http_request& request, http_session* sess) {
    int forward_count = 0;

    while (forward_count < max_forward_count) {
        http_response response = router_.handle_request(request);

        if (sess != nullptr) {
            add_session_cookie(request, response, sess, cookie_name_);
        }

        if (!response.forward_path.empty()) {
            request.path = move(response.forward_path);
            request.parameters.clear();
            parse_parameters(request);
            forward_count++;
            continue;
        }

        send_response(&client_socket, response);
        break;
    }

    if (forward_count >= max_forward_count) {
        send_error_response(&client_socket, http_status::S5_INTERNAL_SERVER_ERROR, "Too many forwards");
    }
}

http_server::http_server(ports port, size_t worker_count) :
server_(make_unique<tcp_server>(port, worker_count)) {
    server_->set_client_handler([this](unique_ptr<tcp_socket> sock) { this->handle_client(move(sock)); });

    set_upgrade_handler("h2c", [this](http_request& request, tcp_socket* sock) -> bool {
        const string_view settings_b64 = request.header("HTTP2-Settings");
        if (settings_b64.empty()) {
            return false;
        }

        {
            lock<mutex> lk(h2c_mutex_);
            if (h2c_loops_.size() >= max_h2c_upgrades) {
                return false;
            }
        }

        http_response upgrade_resp;
        upgrade_resp.version = request.version;
        upgrade_resp.status = http_status::S1_SWITCHING_PROTOCOLS;
        upgrade_resp.status_message = "Switching Protocols";
        upgrade_resp.set_header("Connection", "Upgrade");
        upgrade_resp.set_header("Upgrade", "h2c");
        send_response(sock, upgrade_resp);

        auto loop = make_shared<event_loop>();
        auto conn = make_shared<http2_connection>(unique_ptr<tcp_socket>(sock), loop);
        conn->set_router(&router_);
        conn->start();
        thread t([loop, conn]() { loop->run(); });
        {
            lock<mutex> lk(h2c_mutex_);
            h2c_loops_.push_back(loop);
            h2c_threads_.push_back(move(t));
        }

        // RFC 7540 §3.2: after h2c upgraded，raw HTTP/1.1 request will be regarded as the HEADERS frame of stream 1
        http_response h2_resp = router_.handle_request(request);
        vector<hpack_header_field> resp_headers;
        resp_headers.push_back({":status", to_string(static_cast<uint16_t>(h2_resp.status))});
        for (const auto& h: h2_resp.headers) {
            resp_headers.push_back({h.first.lowercase(), h.second});
        }
        conn->send_response(1, resp_headers, h2_resp.body, true);

        return true;
    });
}

http_server::http_server(ports port, ssl_context ctx, size_t worker_count) :
server_(make_unique<ssl_server>(port, worker_count)) {
    auto* ssl_srv = dynamic_cast<ssl_server*>(server_.get());
    if (ssl_srv != nullptr) {
        ctx.set_alpn_protos({"h2", "http/1.1"});
        ssl_srv->set_ssl_context(move(ctx));
    }
    server_->set_client_handler([this](unique_ptr<tcp_socket> sock) { this->handle_client(move(sock)); });
}

http_server::~http_server() {
    server_->stop();
    {
        lock<mutex> lk(h2c_mutex_);
        for (auto& loop: h2c_loops_) {
            loop->stop();
        }
    }
    for (auto& t: h2c_threads_) {
        if (t.joinable()) {
            t.join();
        }
    }
}

bool http_server::load_certificate(const string& cert_file, const string& key_file) {
    auto* ssl_srv = dynamic_cast<ssl_server*>(server_.get());
    if (ssl_srv == nullptr) {
        return false;
    }
    return ssl_srv->load_certificate(cert_file, key_file);
}

http_session* http_server::get_session(http_request& request, bool create) {
    return get_or_create_session(request, create, session_manager_, cookie_name_);
}

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
