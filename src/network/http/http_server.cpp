#include <NeForce/core/encrypt/base64.hpp>
#include <NeForce/core/encrypt/sha1.hpp>
#include <NeForce/core/utility/hexadecimal.hpp>
#include <NeForce/network/http/http_server.hpp>
#include <NeForce/network/util/url.hpp>
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

    string decode_chunked_body(const string_view chunked_data) {
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

} // namespace


string http_server::session_manager::generate_session_id() {
    string str;
    str.reserve(32);
    for (int i = 0; i < 32; ++i) {
        str += format("{:x}", rand_.next_int(0, 15));
    }
    return move(str);
}

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
                                        const byte_size max_body_size) {

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
    string res_str = response.to_string();
    // NOLINTNEXTLINE(clang-analyzer-cplusplus.Move)
    client_socket->send_all(memory_view<const char>(res_str.data(), res_str.size()));
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
    try {
        http_request request = parse_request(client_socket.get(), session_manager_, cookie_name_,
                                             max_server_header_size, max_server_body_size);

        if (client_socket->is_ssl()) {
            request.set_header(http_key::X_Forwarded_Proto(), "https");
        }

        if (enable_websocket && try_websocket_upgrade(client_socket, request)) {
            return;
        }

        http_session* sess = get_or_create_session(request, true, session_manager_, cookie_name_);
        // NOLINTNEXTLINE(clang-analyzer-cplusplus.Move)
        handle_request_with_forward(*client_socket, request, sess);
    } catch (const http_exception& e) {
        send_error_response(client_socket.get(), http_status::S4_BAD_REQUEST, e.what());
    } catch (const exception& e) {
        send_error_response(client_socket.get(), http_status::S5_INTERNAL_SERVER_ERROR, e.what());
    } catch (...) {
        send_error_response(client_socket.get(), http_status::S5_INTERNAL_SERVER_ERROR, "Unknown internal error");
    }
}

bool http_server::try_websocket_upgrade(unique_ptr<tcp_socket>& client_socket, http_request& request) {
    string upgrade = request.header("Upgrade");
    string connection = request.header("Connection");

    if (upgrade.lowercase() != "websocket" || !connection.lowercase().contains("upgrade")) {
        return false;
    }

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

    send_response(client_socket.get(), upgrade_response);

    return ws_server_.handle_upgrade(request, move(client_socket));
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
}

http_server::http_server(ports port, ssl_context ctx, size_t worker_count) :
server_(make_unique<ssl_server>(port, worker_count)) {
    auto* ssl_srv = dynamic_cast<ssl_server*>(server_.get());
    if (ssl_srv != nullptr) {
        ssl_srv->set_ssl_context(move(ctx));
    }
    server_->set_client_handler([this](unique_ptr<tcp_socket> sock) { this->handle_client(move(sock)); });
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
