#include <MSTL/network/http/servlet.hpp>
#include <MSTL/core/system/console.hpp>
MSTL_BEGIN_NAMESPACE__

void servlet::start_workers(const int thread_count) {
    for (int i = 0; i < thread_count; ++i) {
        worker_threads_.emplace_back(&servlet::accept_conns, this);
    }
}

void servlet::accept_conns() {
    while (running_) {
        socket client_socket = server_socket_.accept();
        if (!client_socket.is_valid()) {
            if (running_) {
                println("accept failed");
            }
            continue;
        }

        try {
            handle_client(client_socket);
        } catch (const exception& e) {
            println(e);
        }
        client_socket.close();
    }
}

void servlet::handle_client(const socket& client_socket) {
    http_request request = parse_request(client_socket);
    int forward_count = 0;
    do {
        constexpr int MAX_FORWARD = 5;
        http_response response = handle_request(request);
        if (!response.forward().empty() && forward_count < MAX_FORWARD) {
            request.set_path(response.forward());
            forward_count++;
            continue;
        }
        send_response(client_socket, response);
        return;
    } while (true);
}

void servlet::parse_cookies(const string& cookie_header, http_request &request) {
    if (cookie_header.empty()) return;

    vector<string> cookie_pairs;
    size_t start = 0;
    size_t end = cookie_header.find(';');

    while (end != string::npos) {
        cookie_pairs.push_back(cookie_header.substr(start, end - start).trim());
        start = end + 1;
        end = cookie_header.find(';', start);
    }
    cookie_pairs.push_back(cookie_header.substr(start).trim());

    for (const auto &pair: cookie_pairs) {
        const size_t eq_pos = pair.find('=');
        if (eq_pos != string::npos) {
            const string name = pair.substr(0, eq_pos).trim();
            const string value = pair.substr(eq_pos + 1).trim();
            request.set_cookie(name, value);
        }
    }
}

void servlet::parse_parameters(http_request& request) {
    if (!request.query().empty()) {
        parse_url_encoded(request.query().view(), request.parameters());
    }
    if (request.method().is_post() && !request.body().empty()) {
        if (HTTP_CONTENT::is_form_app(request.content_type().view())) {
            parse_url_encoded(request.body().view(), request.parameters());
        }
    }
}

void servlet::parse_url_encoded(const string_view data, unordered_map<string, string> &params) {
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
            string name = url_decode(pair.substr(0, eq_pos));
            params[name] = url_decode(pair.substr(eq_pos + 1));
        }
    }
}

string servlet::url_decode(const string_view str) {
    string result;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '%' && i + 2 < str.length()) {
            try {
                result += static_cast<char>(
                    hexadecimal::parse(str.substr(i + 1, 2)).to_int64()
                    );
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

http_request servlet::parse_request(const socket& client_socket) {
    http_request req;
    char buffer[4096];
    string request_data;

    ssize_t total_read = 0;
    while (true) {
        ssize_t bytes_read = client_socket.receive(buffer, sizeof(buffer) - 1);
        if (bytes_read <= 0) break;

        buffer[bytes_read] = '\0';
        request_data.append(buffer, bytes_read);
        total_read += bytes_read;

        const size_t header_end = request_data.find("\r\n\r\n");
        if (header_end != string::npos) {
            size_t content_length = 0;
            const size_t content_start = header_end + 4;

            const size_t cl_pos = request_data.find("Content-Length:");
            if (cl_pos != string::npos) {
                const size_t cl_end = request_data.find("\r\n", cl_pos);
                if (cl_end != string::npos) {
                    const string cl_str = request_data.substr(
                        cl_pos + 15, cl_end - cl_pos - 15
                        ).trim();
                    content_length = _MSTL uinteger32::parse(cl_str.c_str());
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

    string line;
    size_t pos = 0;
    if (_MSTL getline(request_data, pos, line)) {
        line.trim();
        const size_t pos1 = line.find(' ');
        if (pos1 != string::npos) {
            req.set_method(HTTP_METHOD{line.substr(0, pos1)});
            const size_t pos2 = line.find(' ', pos1 + 1);
            if (pos2 != string::npos) {
                req.set_path(line.substr(pos1 + 1, pos2 - pos1 - 1));
                req.set_version(line.substr(pos2 + 1));
            }
        }
    }

    // url query
    const size_t query_pos = req.path().find('?');
    if (query_pos != string::npos) {
        req.set_query(req.path().substr(query_pos+1));
        req.set_path(req.path().substr(0, query_pos));
    }

    // request header
    while (_MSTL getline(request_data, pos, line)) {
        line.trim();
        if (line.empty()) break;

        const size_t colon_pos = line.find(':');
        if (colon_pos != string::npos) {
            string key = line.substr(0, colon_pos).trim();
            string value = line.substr(colon_pos+1).trim();
            req.set_header(key, _MSTL move(value));
        }
    }

    // request body
    const size_t body_start = request_data.find("\r\n\r\n");
    if (body_start != string::npos && body_start + 4 < request_data.size()) {
        req.set_body(request_data.substr(body_start + 4));
    }

    // Parse cookies
    const auto& cookie_str = req.cookie();
    if (!cookie_str.empty()) {
        parse_cookies(cookie_str, req);
    }

    // Parse parameters
    parse_parameters(req);

    // Handle session
    const auto& session_id = req.cookie(cookie_name_.cookie_name());
    if (!session_id.empty() && session_manager_.session_exists(session_id)) {
        req.set_session(session_manager_.get_session(session_id, false));
    }

    return req;
}

string servlet::build_response_str(const http_response& response) {
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

void servlet::send_response(const socket& client_socket, const http_response& response) {
    string res_str = build_response_str(response);
    const size_t total = res_str.size();
    size_t sent = 0;

    while (sent < total) {
        const ssize_t bytes_sent = client_socket.send(res_str.data() + sent, total - sent);
        if (bytes_sent <= 0) {
            println("send failed");
            break;
        }
        sent += bytes_sent;
    }
}

session* servlet::get_session(http_request& request, const bool create) {
    session* session = request.get_session();
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

void servlet::add_session_cookie(const http_request& request, http_response& response, session* session) const {
    if (session && session->is_new()) {
        cookie session_cookie(cookie_name_, session->id());
        session_cookie.set_path("/");
        session_cookie.set_http_only(true);

        const bool is_https = request.is_https();
        session_cookie.set_http_only(is_https);
        session_cookie.set_same_site(is_https ? "Strict" : "Lax");

        response.add_cookie(session_cookie);
        session->set_is_new(false);
    }
}

http_response servlet::handle_request(http_request& request) {
    http_response response;

    if (!filter_chain_.execute_pre_filters(request, response)) {
        return response;
    }

    session* session = get_session(request, true);
    if (session) {
        add_session_cookie(request, response, session);
    }
    filter_chain_.execute_filters(request, response);

    println(
        "[", request.method(), "]", request.path(), "Query:", request.query(),
        "Body size:", request.body().size(), *request.get_session()
    );

    const auto& method = request.method();
    if (method.is_get()) {
        do_get(request, response);
    } else if (method.is_post()) {
        do_post(request, response);
    } else if (method.is_put()) {
        do_put(request, response);
    } else if (method.is_delete()) {
        do_delete(request, response);
    } else if (method.is_head()) {
        do_head(request, response);
    } else if (method.is_options()) {
        do_options(request, response);
    } else if (method.is_trace()) {
        do_trace(request, response);
    } else if (method.is_connect()) {
        do_connect(request, response);
    } else {
        response.set_status(HTTP_STATUS::S4_METHOD_NOT_ALLOWED);
        response.set_status_msg("Method Not Allowed");
        response.set_body("Method Not Allowed");
    }

    filter_chain_.execute_post_filters(request, response);
    return response;
}

void servlet::do_head(http_request& request, http_response& response) {
    do_get(request, response);
    response.set_body(""); // HEAD should not return body
}

void servlet::do_options(http_request& request, http_response& response) {
    response.set_status(HTTP_STATUS::S2_NO_CONTENT);
    response.set_status_msg("No Content");
    response.set_allow_method(HTTP_METHOD::DEFAULT);
    response.set_allow_headers("Content-Type, Cookie, Accept, X-Requested-With");
    response.set_max_age(86400);
}

void servlet::do_trace(http_request& request, http_response& response) {
    response.set_ok();
    response.set_status_msg("OK");
    response.set_content_type(HTTP_CONTENT::HTML_MSG);

    string result;
    result += request.method().to_string() + " " + request.path();
    if (!request.query().empty()) result += "?" + request.query();
    result += " " + request.version() + "\r";
    const auto& headers = request.headers();
    for (auto iter = headers.begin(); iter != headers.end(); ++iter) {
        result += iter->first + ": " + iter->second + "\r";
    }
    result += "\r" + request.body();
    response.set_body(result);
}

void servlet::do_connect(http_request& request, http_response& response) {
    response.set_status(HTTP_STATUS::S4_METHOD_NOT_ALLOWED);
    response.set_status_msg("Method Not Allowed");
    response.set_body("CONNECT method not supported");
}

servlet::~servlet() {
    this->stop();
    filter_chain_.clean_filter();
}

bool servlet::start(const SOCKET_DOMAIN domain, const SOCKET_TYPE type,
                    const SOCKET_PROTOCOL protocol, const uint32_t thread_count
                    ) {
    if (running_) return true;
    if (!init()) return false;

#ifdef MSTL_PLATFORM_WINDOWS__
    if (::WSAStartup(MAKEWORD(2, 2), &wsa_data_) != 0) {
        println("WSAStartup failed");
        return false;
    }
#endif

    server_socket_ = _MSTL move(socket(domain, type, protocol));
    if (!server_socket_.is_valid()) {
        println("socket creation failed");
        return false;
    }

    if (server_socket_.reuse_addr()) {
        println("setsockopt failed");
        return false;
    }

    server_addr_.sin_family = AF_INET;
    server_addr_.sin_addr.s_addr = INADDR_ANY;
    server_addr_.sin_port = ::htons(port_);

    if (server_socket_.bind(server_addr_)) {
        println("bind failed");
        return false;
        }

    if (server_socket_.listen(backlog_)) {
        println("listen failed");
        return false;
    }

    running_ = true;
    start_workers(static_cast<int32_t>(thread_count));
    return true;
}

void servlet::stop() noexcept {
    if (!running_) return;

    running_ = false;
    server_socket_.close();
#ifdef MSTL_PLATFORM_WINDOWS__
    ::WSACleanup();
#endif

    for (auto& t : worker_threads_) {
        if (t.joinable()) t.join();
    }
    worker_threads_.clear();
    destroy();
}

MSTL_END_NAMESPACE__
