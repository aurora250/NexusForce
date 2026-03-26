#include <NeForce/network/http/http_server.hpp>
#include <NeForce/core/utility/hexadecimal.hpp>
#include <NeForce/core/encrypt/base64.hpp>
#include <NeForce/core/encrypt/sha1.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    string url_decode(const string_view str) {
        string result;
        result.reserve(str.length());

        for (size_t i = 0; i < str.length(); ++i) {
            if (str[i] == '%' && i + 2 < str.length()) {
                try {
                    const auto hex_str = str.view(i + 1, 2);
                    result += static_cast<char>(hexadecimal::parse(hex_str).value());
                    i += 2;
                } catch (...) {
                    result += str[i];
                }
            } else if (str[i] == '+') {
                result += ' ';
            } else {
                result += str[i];
            }
        }
        return result;
    }

    void parse_url_encoded(const string_view data, unordered_map<string, string>& params)  {
        if (data.empty()) return;

        size_t start = 0;
        while (start < data.length()) {
            const size_t end = data.find('&', start);
            const size_t pair_end = (end == string::npos) ? data.length() : end;

            const auto pair = data.view(start, pair_end - start);
            const size_t eq_pos = pair.find('=');

            if (eq_pos != string::npos) {
                const string name = url_decode(pair.view(0, eq_pos));
                string value = url_decode(pair.view(eq_pos + 1));
                params[name] = _NEFORCE move(value);
            }

            if (end == string::npos) {
                break;
            }
            start = end + 1;
        }
    }

    string get_status_message(const HTTP_STATUS status) {
        switch (status) {
            case HTTP_STATUS::S4_BAD_REQUEST: return "Bad Request";
            case HTTP_STATUS::S4_UNAUTHORIZED: return "Unauthorized";
            case HTTP_STATUS::S4_FORBIDDEN: return "Forbidden";
            case HTTP_STATUS::S4_NOT_FOUNT: return "Not Found";
            case HTTP_STATUS::S4_METHOD_NOT_ALLOWED: return "Method Not Allowed";
            case HTTP_STATUS::S5_INTERNAL_ERROR: return "Internal Server Error";
            case HTTP_STATUS::S5_SERVICE_UNAVAILABLE: return "Service Unavailable";
            default: return "Error";
        }
    }
}


string http_server_base::session_manager::generate_session_id()  {
    string str;
    str.reserve(32);
    for (int i = 0; i < 32; ++i) {
        str += format("{x}", rand_.next_int(0, 15));
    }
    return move(str);
}

http_server_base::session_manager::session_manager() {
    cleanup_running_ = true;
    cleanup_thread_ = thread(&session_manager::cleanup_expired_sessions, this);
}

http_server_base::session_manager::~session_manager() {
    cleanup_running_ = false;
    if (cleanup_thread_.joinable()) {
        cleanup_thread_.join();
    }
}

http_session* http_server_base::session_manager::get_session(const string& session_id, const bool create) {
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

void http_server_base::session_manager::remove_session(const string& session_id) noexcept {
    lock<mutex> lock(mutex_);
    sessions_.erase(session_id);
}

void http_server_base::session_manager::cleanup_expired_sessions()  {
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

        this_thread::sleep_for(cleanup_interval_);
    }
}

bool http_server_base::session_manager::session_exists(const string& session_id) const noexcept {
    lock<mutex> lk(mutex_);
    const auto it = sessions_.find(session_id);
    return it != sessions_.end() && it->second.is_valid();
}

size_t http_server_base::session_manager::session_count() const noexcept {
    lock<mutex> lk(mutex_);
    return sessions_.size();
}

void http_server_base::session_manager::set_cleanup_interval(const seconds interval) noexcept {
    cleanup_interval_ = interval;
}

void http_server_base::session_manager::set_max_sessions(const size_t max) noexcept {
    max_sessions_ = max;
}

string http_server_base::compute_websocket_accept(const string_view key) {
    const string combined = string(key) + websocket_guid;
    const string sha1_result = sha1(combined);
    return base64_encode(cbyte_view{
        reinterpret_cast<const byte_t*>(sha1_result.data()),
        sha1_result.size()
    });
}

void http_server_base::parse_cookies(const string_view cookie_header, http_request& request) {
    if (cookie_header.empty()) return;

    size_t start = 0;
    while (start < cookie_header.length()) {
        const size_t end = cookie_header.find(';', start);
        const size_t pair_end = (end == string::npos) ? cookie_header.length() : end;

        const auto pair = cookie_header.view(start, pair_end - start).trim();
        const size_t eq_pos = pair.find('=');

        if (eq_pos != string::npos) {
            request.set_cookie(pair.substr(0, eq_pos).trim(), pair.substr(eq_pos + 1).trim());
        }

        if (end == string::npos) {
            break;
        }
        start = end + 1;
    }
}

void http_server_base::parse_parameters(http_request& request) {
    if (!request.query.empty()) {
        parse_url_encoded(request.query.view(), request.parameters);
    }

    if (request.method.is_post() && !request.body.empty()) {
        const auto content_type = request.content_type();
        if (HTTP_CONTENT::is_form_app(content_type)) {
            parse_url_encoded(request.body.view(), request.form_data);
        }
    }
}

http_request http_server_base::parse_request(
    tcp_socket* client_socket,
    session_manager& manager,
    const HTTP_COOKIE_NAME& name,
    const size_t max_header_size,
    const size_t max_body_size) {

    http_request request;
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
            const size_t cl_pos = request_data.find("Content-Length:");
            if (cl_pos != string::npos && cl_pos < header_end) {
                const size_t cl_end = request_data.find("\r\n", cl_pos);
                if (cl_end != string::npos) {
                    const auto cl_str = request_data.view(cl_pos + 15, cl_end - cl_pos - 15).trim();
                    try {
                        content_length = uinteger64::parse(cl_str);
                    } catch (...) {
                        NEFORCE_THROW_EXCEPTION(http_exception("Invalid Content-Length"));
                    }
                }
            }

            if (content_length > max_body_size) {
                NEFORCE_THROW_EXCEPTION(http_exception("Request body too large"));
            }

            const size_t body_start = header_end + 4;
            const size_t body_received = request_data.size() - body_start;

            if (body_received < content_length) {
                size_t remaining = content_length - body_received;
                request_data.reserve(body_start + content_length);

                while (remaining > 0) {
                    const size_t to_read = (remaining < sizeof(buffer)) ? remaining : sizeof(buffer);
                    const ssize_t n = client_socket->receive(memory_view<char>(buffer, to_read));

                    if (n <= 0) {
                        NEFORCE_THROW_EXCEPTION(http_exception("Connection closed while reading body"));
                    }

                    request_data.append(buffer, n);
                    remaining -= n;
                }
            }
            break;
        }

        if (request_data.size() > max_header_size) {
            NEFORCE_THROW_EXCEPTION(http_exception("Request header too large"));
        }
    }

    // Parse request line
    size_t pos = 0;
    string_view line;

    if (getline(request_data.view(), pos, line)) {
        line = line.trim();
        const size_t pos1 = line.find(' ');

        if (pos1 == string::npos) {
            NEFORCE_THROW_EXCEPTION(http_exception("Invalid request line"));
        }

        request.method = line.view(0, pos1);
        const size_t pos2 = line.find(' ', pos1 + 1);

        if (pos2 == string::npos) {
            NEFORCE_THROW_EXCEPTION(http_exception("Invalid request line"));
        }

        request.path = line.view(pos1 + 1, pos2 - pos1 - 1);
        request.version = line.view(pos2 + 1);
    }

    // Parse query string
    const size_t query_pos = request.path.find('?');
    if (query_pos != string::npos) {
        request.query = request.path.substr(query_pos + 1);
        request.path = request.path.substr(0, query_pos);
    }

    // Parse headers
    while (getline(request_data.view(), pos, line)) {
        line = line.trim();
        if (line.empty()) break;

        const size_t colon_pos = line.find(':');
        if (colon_pos != string::npos) {
            const string key = line.view(0, colon_pos).trim();
            string value = line.view(colon_pos + 1).trim();
            request.set_header(key, move(value));
        }
    }

    // Extract body
    const size_t body_start = request_data.find("\r\n\r\n");
    if (body_start != string::npos && body_start + 4 < request_data.size()) {
        request.body = request_data.substr(body_start + 4);
    }

    // Parse cookies
    const auto cookie_str = request.header("Cookie");
    if (!cookie_str.empty()) {
        parse_cookies(cookie_str, request);
    }

    parse_parameters(request);

    const string& session_id = request.cookie(name.cookie_name());
    if (!session_id.empty() && manager.session_exists(session_id)) {
        request.session = manager.get_session(session_id, false);
    }

    return request;
}

http_session* http_server_base::get_or_create_session(
    http_request& request,
    const bool create,
    session_manager& manager,
    const HTTP_COOKIE_NAME& name) {

    http_session* sess = request.session;
    if (sess) return sess;

    const string& session_id = request.cookie(name.cookie_name());
    if (!session_id.empty()) {
        sess = manager.get_session(session_id, false);
    }

    if (!sess && create) {
        sess = manager.get_session("", true);
    }
    request.session = sess;
    return sess;
}

string http_server_base::build_response_str(const http_response& response) {
    string result;
    result.reserve(1024 + response.body.size());

    // Status line
    if (!response.redirect_url.empty()) {
        result += response.version + " 302 Found\r\n";
        result += "Location: " + response.redirect_url + "\r\n";
    } else {
        result += response.version + " " +
            _NEFORCE to_string(static_cast<uint16_t>(response.status)) + " " +
            response.status_message + "\r\n";
    }

    // Cookies
    for (const auto& cookie : response.cookies) {
        result += "Set-Cookie: " + cookie.to_string() + "\r\n";
    }

    // Content-Length
    if (response.redirect_url.empty() && !response.has_header(HTTP_KEY::Content_Length)) {
        result += HTTP_KEY::Content_Length + ": " + _NEFORCE to_string(response.body.size()) + "\r\n";
    }

    for (const auto& pair : response.headers) {
        const auto key = pair.first;
        const auto value = pair.second;
        result += move(key) + ": " + move(value) + "\r\n";
    }
    result += "\r\n";

    if (response.redirect_url.empty()) {
        result += response.body;
    }
    return result;
}

void http_server_base::send_response(tcp_socket* client_socket, const http_response& response) {
    string res_str = build_response_str(response);
    client_socket->send_all(memory_view<const char>(res_str.data(), res_str.size()));
}

void http_server_base::add_session_cookie(
    const http_request& request,
    http_response& response,
    http_session* session,
    const HTTP_COOKIE_NAME& name) {

    if (!session || !session->is_new) {
        return;
    }

    http_cookie session_cookie;
    session_cookie.name = name;
    session_cookie.value = session->id;
    session_cookie.http_only = true;

    const bool is_https = request.header(HTTP_KEY::X_Forwarded_Proto) == "https";
    session_cookie.secure = is_https;
    session_cookie.same_site = is_https ? HTTP_KEY::Strict : HTTP_KEY::Lax;

    response.cookies.emplace_back(move(session_cookie));
    session->is_new = false;
}

void http_server_base::send_error_response(tcp_socket* client_socket, const HTTP_STATUS status, const string& message) {
    try {
        http_response error_response;
        error_response.status = status;
        error_response.status_message = get_status_message(status);
        error_response.set_content_type(HTTP_CONTENT::HTML_TEXT);
        error_response.body =
            "<!DOCTYPE html>"
            "<html><head><title>Error</title></head>"
            "<body><h1>" + error_response.status_message + "</h1>"
            "<p>" + message + "</p>"
            "</body></html>";
        send_response(client_socket, error_response);
    } catch (...) {
        // ignore
    }
}

NEFORCE_END_NAMESPACE__
