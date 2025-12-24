#include <MSTL/network/http/http_server.hpp>
#include <MSTL/core/system/console.hpp>
MSTL_BEGIN_NAMESPACE__

void http_server::handle_client(handle_sock_t client_socket) {
    http_request request = parse_request(client_socket);
    if (client_socket.ssl_valid()) {
        request.set_https();
    }
    _MSTL session* sess = session(request, true);
    int forward_count = 0;

    do {
        constexpr int MAX_FORWARD = 5;

        http_response response = router_.handle_request(request);
        if (sess) {
            add_session_cookie(request, response, sess);
        }

        if (!response.forward().empty() && forward_count < MAX_FORWARD) {
            request.set_path(response.forward());
            forward_count++;
            continue;
        }

        send_response(client_socket, response);
        return;
    } while (true);
}

void http_server::parse_cookies(const string_view cookie_header, http_request& request) {
    if (cookie_header.empty()) return;

    vector<string_view> cookie_pairs;
    size_t start = 0;
    size_t end = cookie_header.find(';');

    while (end != string::npos) {
        cookie_pairs.push_back(cookie_header.view(start, end - start).trim());
        start = end + 1;
        end = cookie_header.find(';', start);
    }
    cookie_pairs.push_back(cookie_header.substr(start).trim());

    for (const auto& pair: cookie_pairs) {
        const size_t eq_pos = pair.find('=');
        if (eq_pos != string::npos) {
            request.set_cookie(pair.substr(0, eq_pos).trim(), pair.substr(eq_pos + 1).trim());
        }
    }
}

void http_server::parse_parameters(http_request& request) {
    if (!request.query().empty()) {
        parse_url_encoded(request.query().view(), request.parameters());
    }
    if (request.method().is_post() && !request.body().empty()) {
        if (HTTP_CONTENT::is_form_app(request.content_type().view())) {
            parse_url_encoded(request.body().view(), request.parameters());
        }
    }
}

void http_server::parse_url_encoded(const string_view data, unordered_map<string, string>& params) {
    if (data.empty()) return;

    vector<string_view> pairs;
    size_t start = 0;
    size_t end = data.find('&');

    while (end != string::npos) {
        pairs.push_back(data.substr(start, end - start));
        start = end + 1;
        end = data.find('&', start);
    }
    pairs.push_back(data.substr(start));

    for (const auto &pair: pairs) {
        const size_t eq_pos = pair.find('=');
        if (eq_pos != string_view::npos) {
            const string name = url_decode(pair.substr(0, eq_pos));
            params[name] = url_decode(pair.substr(eq_pos + 1));
        }
    }
}

string http_server::url_decode(const string_view str) {
    string result;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '%' && i + 2 < str.length()) {
            try {
                result += static_cast<char>(hexadecimal::parse(str.substr(i + 1, 2)).value());
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

http_request http_server::parse_request(const handle_sock_t& client_socket) {
    http_request request;
    char buffer[4096];
    string request_data;

    ssize_t total_read = 0;
    while (true) {
        ssize_t bytes_read = client_socket.receive(buffer, sizeof(buffer) - 1);
        if (bytes_read <= 0) break;

        buffer[bytes_read] = '\0';
        request_data.append(buffer, bytes_read);
        total_read += bytes_read;

        const size_t header_end = request_data.find(HTTP_CRLF2);
        if (header_end != string::npos) {
            size_t content_length = 0;
            const size_t content_start = header_end + 4;

            const size_t cl_pos = request_data.find("Content-Length:");
            if (cl_pos != string::npos) {
                const size_t cl_end = request_data.find(HTTP_CRLF, cl_pos);
                if (cl_end != string::npos) {
                    const string_view cl_str = request_data.view(
                        cl_pos + 15, cl_end - cl_pos - 15
                        ).trim();
                    content_length = _MSTL uinteger32::parse(cl_str);
                }
            }

            const auto body_read = static_cast<ssize_t>(total_read - content_start);
            if (body_read < static_cast<ssize_t>(content_length)) {
                ssize_t remaining = static_cast<ssize_t>(content_length - body_read);
                while (remaining > 0) {
                    const auto size = _MSTL min(static_cast<ssize_t>(sizeof(buffer)), remaining);
                    bytes_read = client_socket.receive(buffer, size);
                    if (bytes_read <= 0) break;
                    request_data.append(buffer, bytes_read);
                    remaining -= bytes_read;
                }
            }
            break;
        }
        if (total_read > 1024 * 16) break;
    }

    string_view line;
    size_t pos = 0;
    if (_MSTL getline(request_data.view(), pos, line)) {
        line = line.trim();
        const size_t pos1 = line.find(' ');
        if (pos1 != string::npos) {
            request.set_method(HTTP_METHOD{string{line.substr(0, pos1)}});
            const size_t pos2 = line.find(' ', pos1 + 1);
            if (pos2 != string::npos) {
                request.set_path(string{line.substr(pos1 + 1, pos2 - pos1 - 1)});
                request.set_version(string{line.substr(pos2 + 1)});
            }
        }
    }

    // url query
    const size_t query_pos = request.path().find('?');
    if (query_pos != string::npos) {
        request.set_query(request.path().substr(query_pos+1));
        request.set_path(request.path().substr(0, query_pos));
    }

    // request header
    while (_MSTL getline(request_data.view(), pos, line)) {
        line = line.trim();
        if (line.empty()) break;

        const size_t colon_pos = line.find(':');
        if (colon_pos != string::npos) {
            const string key{line.substr(0, colon_pos).trim()};
            const string value{line.substr(colon_pos+1).trim()};
            request.set_header(key, _MSTL move(value));
        }
    }

    // request body
    const size_t body_start = request_data.find(HTTP_CRLF2);
    if (body_start != string::npos && body_start + 4 < request_data.size()) {
        request.set_body(request_data.substr(body_start + 4));
    }

    // Parse cookies
    const auto& cookie_str = request.header_cookie();
    if (!cookie_str.empty()) {
        parse_cookies(cookie_str.view(), request);
    }

    // Parse parameters
    parse_parameters(request);

    // Handle session
    const auto& session_id = request.cookie(cookie_name_.cookie_name());
    if (!session_id.empty() && session_manager_.session_exists(session_id)) {
        request.set_session(session_manager_.get_session(session_id, false));
    }

    return request;
}

string http_server::build_response_str(const http_response& response) {
    string result;
    if (!response.redirect().empty()) {
        result += response.version() + " 302 Found\r\n";
        result += "Location: " + response.redirect() + "\r\n";
    } else {
        result += response.version() + " " +
            _MSTL to_string(static_cast<uint16_t>(response.status())) + " "
            + response.status_msg() + "\r\n";
    }

    for (const auto& cookie : response.cookies()) {
        result += "Set-Cookie: " + cookie.to_string() + "\r\n";
    }
    if (response.redirect().empty() &&
        response.content_length().empty()) {
        result += "Content-Length: " + _MSTL to_string(response.body().size()) + "\r\n";
        }
    const auto& header = response.headers();
    for (auto iter = header.begin(); iter != header.end(); ++iter) {
        result += iter->first + ": " + iter->second + "\r\n";
    }
    result += "\r\n";
    if (response.redirect().empty()) {
        result += response.body();
    }

    return result;
}

void http_server::send_response(const handle_sock_t& client_socket, const http_response& response) {
    string res_str = build_response_str(response);
    const size_t total = res_str.size();
    size_t sent = 0;

    while (sent < total) {
        const ssize_t bytes_sent = client_socket.send(res_str.data() + sent, total - sent);
        if (bytes_sent <= 0) {
            printcln(color::red(), "send failed");
            break;
        }
        sent += bytes_sent;
    }
}

http_server::http_server(const uint16_t port, const int backlog)
: server_(port, backlog) {
    server_.set_client_handler([this](handle_sock_t sock) {
        http_server::handle_client(_MSTL move(sock));
    });
}

_MSTL session* http_server::session(http_request& request, const bool create) {
    _MSTL session* session = request.session();
    if (session) return session;

    const auto& session_id = request.cookie(cookie_name_.cookie_name());
    if (!session_id.empty()) {
        session = session_manager_.get_session(session_id, false);
    }

    if (!session && create) {
        session = session_manager_.create_session();
    }
    request.set_session(session);
    return session;
}

void http_server::add_session_cookie(const http_request& request, http_response& response, _MSTL session* session) const {
    if (session && session->is_new()) {
        cookie session_cookie(cookie_name_, session->id());
        session_cookie.set_path("/");
        session_cookie.set_http_only(true);

        const bool is_https = request.is_https();
        session_cookie.set_http_only(is_https);
        session_cookie.set_same_site(is_https);

        response.add_cookie(session_cookie);
        session->set_is_new(false);
    }
}

MSTL_END_NAMESPACE__
