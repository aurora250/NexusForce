#include <NeForce/core/async/async.hpp>
#include <NeForce/core/file/file.hpp>
#include <NeForce/core/utility/hexadecimal.hpp>
#include <NeForce/network/http/http_client.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    string url_encode(const string_view str) {
        string result;
        result.reserve(str.size() * 3);

        for (const auto c : str) {
            if (is_alpha_or_digit(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                result += c;
            } else if (c == ' ') {
                result += '+';
            } else {
                result += '%';
                result += "0123456789ABCDEF"[c >> 4];
                result += "0123456789ABCDEF"[c & 0x0F];
            }
        }
        return result;
    }

    string url_decode(const string_view str) {
        string result;
        result.reserve(str.size());

        for (size_t i = 0; i < str.size(); ++i) {
            if (str[i] == '%' && i + 2 < str.size()) {
                const int high = hexadecimal::digit_value(str[i + 1]);
                const int low = hexadecimal::digit_value(str[i + 2]);
                if (high >= 0 && low >= 0) {
                    result += static_cast<char>((high << 4) | low);
                    i += 2;
                } else {
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

    bool parse_chunked_body(const string_view chunked, string& decoded) {
        decoded.clear();
        size_t pos = 0;
        const size_t size = chunked.size();

        while (pos < size) {
            const auto line_end = chunked.find("\r\n", pos);
            if (line_end == string::npos) {
                return false;
            }

            auto size_str = chunked.substr(pos, line_end - pos).trim();
            const size_t semicolon = size_str.find(';');
            if (semicolon != string::npos) {
                size_str = size_str.substr(0, semicolon);
            }

            size_t chunk_size = 0;
            try {
                chunk_size = hexadecimal::parse(size_str);
            } catch (...) {
                return false;
            }

            pos = line_end + 2;

            if (chunk_size == 0) {
                break;
            }
            if (pos + chunk_size + 2 > size) {
                return false;
            }

            decoded.append(chunked.substr(pos, chunk_size));
            pos += chunk_size + 2;
        }
        return true;
    }


    cookie parse_set_cookie(const string_view str, string default_domain, string default_path) {
        vector<string_view> tokens;
        size_t start = 0, end;

        while ((end = str.find(';', start)) != string::npos) {
            tokens.push_back(str.substr(start, end - start).trim());
            start = end + 1;
        }
        tokens.push_back(str.substr(start).trim());

        cookie c{};
        if (tokens.empty()) return c;

        const size_t eq_pos = tokens[0].find('=');
        if (eq_pos == string::npos) {
            return c;
        }

        c.name = tokens[0].substr(0, eq_pos);
        c.value = tokens[0].substr(eq_pos + 1);
        c.domain = move(default_domain);
        c.path = default_path.empty() ? "/" : move(default_path);

        for (size_t i = 1; i < tokens.size(); ++i) {
            auto& attr = tokens[i];
            auto lower_attr = string(attr);
            lower_attr.lowercase();

            if (lower_attr.starts_with("domain=")) {
                c.domain = attr.substr(7);
            } else if (lower_attr.starts_with("path=")) {
                c.path = attr.substr(5);
            } else if (lower_attr == "secure") {
                c.secure = true;
            } else if (lower_attr == "httponly") {
                c.http_only = true;
            } else if (lower_attr.starts_with("max-age=")) {
                try {
                    c.max_age = integer32::parse(attr.substr(8));
                } catch (...) {
                    // ignore
                }
            } else if (lower_attr.starts_with("samesite=")) {
                c.same_site = attr.substr(9);
            } else if (lower_attr.starts_with("expires=")) {
                try {
                    c.expires = datetime::parse_GMT(attr.substr(8));
                } catch (...) {
                    // ignore
                }
            }
        }

        return c;
    }

    bool parse_response(const string_view resp_str, http_client_response& resp) {
        const size_t line_end = resp_str.find("\r\n");
        if (line_end == string::npos) {
            return false;
        }

        const string_view status_line = resp_str.substr(0, line_end);

        const size_t sp1 = status_line.find(' ');
        if (sp1 == string::npos) {
            return false;
        }

        // Parse HTTP version
        const string_view version_str = status_line.substr(0, sp1);
        if (version_str.starts_with("HTTP/")) {
            const string_view ver = version_str.substr(5);
            const size_t dot = ver.find('.');
            if (dot != string::npos) {
                try {
                    resp.http_version_major = static_cast<uint16_t>(uinteger16::parse(ver.substr(0, dot)));
                    resp.http_version_minor = static_cast<uint16_t>(uinteger16::parse(ver.substr(dot + 1)));
                } catch (...) {
                    // ignore
                }
            }
        }

        // Parse status code
        const size_t sp2 = status_line.find(' ', sp1 + 1);
        if (sp2 == string::npos) {
            return false;
        }

        try {
            uint16_t code = uinteger16::parse(status_line.substr(sp1 + 1, sp2 - sp1 - 1));
            resp.status = static_cast<HTTP_STATUS>(code);
        } catch (...) {
            // ignore
        }
        resp.status_message = status_line.substr(sp2 + 1);

        // Parse headers
        const size_t header_start = line_end + 2;
        const size_t header_end = resp_str.find("\r\n\r\n", header_start);
        if (header_end == string::npos) {
            return false;
        }

        const string_view headers_block = resp_str.substr(header_start, header_end - header_start);
        size_t hpos = 0;

        while (hpos < headers_block.size()) {
            const auto hline_end = headers_block.find("\r\n", hpos);
            string_view line = headers_block.substr(
                hpos, hline_end == string::npos ?
                      headers_block.size() - hpos : hline_end - hpos);
            if (line.empty()) break;

            const size_t colon = line.find(':');

            if (colon != string::npos) {
                const auto key = line.substr(0, colon).trim();
                const auto value = line.substr(colon + 1).trim();
                resp.headers[key].push_back(value);

                string key_lower = key;
                key_lower.lowercase();

                if (key_lower == "set-cookie") {
                    cookie c = parse_set_cookie(value, resp.header("host"), "/");
                    resp.cookies.emplace_back(move(c));
                } else if (key_lower == "transfer-encoding") {
                    resp.chunked = value.find("chunked") != string::npos;
                } else if (key_lower == "content-length") {
                    try {
                        resp.content_length = uinteger64::parse(value);
                    } catch (...) {
                        // ignore
                    }
                }
            }

            hpos = hline_end == string::npos ? headers_block.size() : hline_end + 2;
        }

        // Parse body
        const string_view body_part = resp_str.substr(header_end + 4);

        if (resp.chunked) {
            string decoded;
            if (parse_chunked_body(body_part, decoded)) {
                resp.body += move(decoded);
            } else {
                return false;
            }
        } else if (resp.content_length > 0) {
            if (body_part.size() < resp.content_length) return false;
            resp.body += body_part.substr(0, resp.content_length);
        } else {
            resp.body += body_part;
        }

        return true;
    }

    string build_query_string(const unordered_map<string, string>& params) {
        if (params.empty()) {
            return "";
        }

        string result;
        bool first = true;

        for (const auto& pair : params) {
            const auto& key = pair.first;
            const auto& value = pair.second;
            if (!first) {
                result += "&";
            }
            result += url_encode(key.view()) + "=" + url_encode(value.view());
            first = false;
        }

        return result;
    }
}


string http_client_request::build_full_path() const {
    string full_path = path;

    if (!query_params.empty()) {
        full_path += "?";
        bool first = true;
        for (const auto& pair : query_params) {
            const auto& key = pair.first;
            const auto& value = pair.second;
            if (!first) {
                full_path += "&";
            }
            full_path += url_encode(key.view()) + "=" + url_encode(value.view());
            first = false;
        }
    }

    return full_path;
}

string http_client::build_request_str(const http_client_request& req, const url& req_url) const {
    string req_str;
    req_str.reserve(1024);

    string full_path = req.path;
    if (!req.query_params.empty()) {
        full_path += "?" + build_query_string(req.query_params);
    }

    req_str += req.method.method() + " " + full_path + " " + req.version + "\r\n";

    req_str += "Host: " + req.host;
    if ((req_url.scheme == "http" && req.port != 80) ||
        (req_url.scheme == "https" && req.port != 443)) {
        req_str += ":" + to_string(req.port);
    }
    req_str += "\r\n";

    // Cookie header
    string cookie_header = build_cookie_header(req_url);
    if (!cookie_header.empty()) {
        req_str += "Cookie: " + move(cookie_header) + "\r\n";
    }

    // Persistent headers
    for (const auto& kv : persistent_headers_) {
        const string& key = kv.first;
        const string& value = kv.second;
        if (req.headers.find(key) == req.headers.end() && key != "Host" && key != "Cookie") {
            req_str += key + ": " + value + "\r\n";
        }
    }

    // Request headers
    bool has_content_type = false;
    for (const auto& kv : req.headers) {
        const string& key = kv.first;
        const string& value = kv.second;
        string key_lower = key;
        key_lower.lowercase();

        if (key_lower == "content-type") has_content_type = true;
        if (key_lower != "host" && key_lower != "cookie") {
            req_str += key + ": " + value + "\r\n";
        }
    }

    // Body headers
    if (!req.body.empty()) {
        if (!has_content_type) {
            req_str += "Content-Type: application/x-www-form-urlencoded\r\n";
        }
        req_str += "Content-Length: " + to_string(req.body.size()) + "\r\n";
    }

    // Connection header
    if (!config_.keep_alive) {
        req_str += "Connection: close\r\n";
    } else {
        req_str += "Connection: keep-alive\r\n";
    }

    req_str += "\r\n";

    if (!req.body.empty()) {
        req_str += req.body;
    }

    return req_str;
}

bool http_client::send_request(const string_view request_str, time_point& send_start) {
    send_start = steady_clock::now();

    try {
        return client_.send_all(request_str);
    } catch (const exception& e) {
        if (error_callback_) {
            error_callback_(e);
        }
        return false;
    }
}

optional<http_client_response> http_client::read_response(time_point& receive_start) {
    receive_start = steady_clock::now();

    http_client_response response;
    string response_data;
    vector<char> buffer(config_.buffer_size);

    size_t total_received = 0;
    bool headers_complete = false;
    size_t header_end_pos = 0;

    try {
        while (response_data.size() < config_.max_response_size) {
            const ssize_t n = client_.receive(buffer.data(), buffer.size());

            if (n <= 0) {
                break;
            }

            response_data.append(buffer.data(), n);
            total_received += n;

            if (!headers_complete) {
                header_end_pos = response_data.find("\r\n\r\n");
                if (header_end_pos != string::npos) {
                    headers_complete = true;

                    http_client_response temp;
                    if (parse_response(response_data.view(), temp)) {
                        if (temp.content_length > 0) {
                            const size_t body_start = header_end_pos + 4;
                            size_t body_received = response_data.size() - body_start;

                            if (body_received >= temp.content_length) {
                                break;
                            }

                            if (progress_callback_) {
                                progress_callback_(move(body_received), move(temp.content_length));
                            }
                        } else if (!temp.chunked) {
                            // No content-length and not chunked
                        }
                    }
                }
            } else if (response.content_length > 0) {
                const size_t body_start = header_end_pos + 4;
                size_t body_received = response_data.size() - body_start;

                if (progress_callback_) {
                    progress_callback_(move(body_received), move(response.content_length));
                }

                if (body_received >= response.content_length) {
                    break;
                }
            }
        }

        if (!parse_response(response_data.view(), response)) {
            return none;
        }
        return response;
    }
    catch (const exception& e) {
        if (error_callback_) {
            error_callback_(e);
        }
        return none;
    }
}

bool http_client::ensure_connected(const string& host, const uint16_t port) {
    const bool is_https = (port == 443);

    if (client_.is_connected()) {
        if (client_.connected_host() == host && client_.connected_port() == port) {
            return true;
        }
        client_.disconnect();
    }

    try {
#ifdef NEFORCE_SUPPORT_OPENSSL
        // Set SNI hostname for HTTPS connections
        if (is_https) {
            client_.set_sni_hostname(host);
        }
#endif
        return client_.connect(host, port);
    } catch (const exception& e) {
        if (error_callback_) {
            error_callback_(e);
        }
        return false;
    }
}

http_client_response http_client::do_request(http_client_request request, int redirect_count) {
    http_client_response response;
    const auto start_time = steady_clock::now();

    url req_url;
    req_url.scheme = url::default_scheme(request.port);
    req_url.host = request.host;
    req_url.port = request.port;
    req_url.path = request.path;

    const auto connect_start = steady_clock::now();
    if (!ensure_connected(request.host, request.port)) {
        response.status = HTTP_STATUS::S5_INTERNAL_ERROR;
        response.status_message = "Connection failed";
        response.effective_url = req_url.to_string();
        return response;
    }
    response.connect_time = time_cast<milliseconds>(steady_clock::now() - connect_start);

    const string request_str = build_request_str(request, req_url);
    steady_clock::time_point send_start;
    if (!send_request(request_str.view(), send_start)) {
        response.status = HTTP_STATUS::S5_INTERNAL_ERROR;
        response.status_message = "Send failed";
        response.effective_url = req_url.to_string();
        return response;
    }
    response.send_time = time_cast<milliseconds>(steady_clock::now() - send_start);

    steady_clock::time_point receive_start;
    auto resp_opt = read_response(receive_start);
    if (!resp_opt) {
        response.status = HTTP_STATUS::S5_INTERNAL_ERROR;
        response.status_message = "Receive/Parse failed";
        response.effective_url = req_url.to_string();
        return response;
    }

    response = move(*resp_opt);
    response.receive_time = time_cast<milliseconds>(steady_clock::now() - receive_start);
    response.total_time = time_cast<milliseconds>(steady_clock::now() - start_time);
    response.effective_url = req_url.to_string();
    response.redirect_count = redirect_count;

    update_cookies(response.cookies, req_url);

    if (config_.follow_redirects && response.is_redirect() && redirect_count < config_.max_redirects) {
        string_view location = response.header("Location");
        if (!location.empty()) {
            url new_url;
            if (location.starts_with("http://") || location.starts_with("https://")) {
                new_url = url::parse(location);
            } else if (location.starts_with("/")) {
                new_url = move(req_url);
                new_url.path = location;
            } else {
                new_url = move(req_url);
                size_t last_slash = new_url.path.find_last_of('/');
                if (last_slash != string::npos) {
                    new_url.path = new_url.path.substr(0, last_slash + 1) + location;
                } else {
                    new_url.path = "/"_s + location;
                }
            }

            http_client_request new_req;
            new_req.host = new_url.host;
            new_req.port = url::default_port(new_url.scheme.view());
            new_req.path = new_url.path.empty() ? "/" : new_url.path;
            new_req.version = request.version;
            new_req.headers = request.headers;

            const auto status_code = static_cast<uint16_t>(response.status);
            if (status_code == 303) {
                new_req.method = HTTP_METHOD::GET;
                new_req.body.clear();
            } else if (status_code == 301 || status_code == 302) {
                if (request.method.is_post()) {
                    new_req.method = HTTP_METHOD::GET;
                    new_req.body.clear();
                } else {
                    new_req.method = request.method;
                    new_req.body = request.body;
                }
            } else {
                new_req.method = request.method;
                new_req.body = request.body;
            }

            return do_request(move(new_req), redirect_count + 1);
        }
    }

    return response;
}

void http_client::update_cookies(const vector<cookie>& resp_cookies, const url& request_url) {
    lock<mutex> lk(mutex_);

    for (const auto &c : resp_cookies) {
        string domain = c.domain.empty() ? request_url.host : c.domain;
        string path = c.path;
        string key = c.name.cookie_name() + "@" + domain + path;

        if (c.max_age == 0 || (c.is_valid() && c.expires.is_valid() && c.is_expired())) {
            cookie_jar_.erase(key);
        } else {
            cookie_jar_[key] = c;
        }
    }
}

string http_client::build_cookie_header(const url& request_url) const {
    string cookie_header;

    for (const auto& pair : cookie_jar_) {
        const auto& c = pair.second;
        if (c.max_age == 0) {
            continue;
        }
        if (c.is_valid() && c.is_expired()) {
            continue;
        }

        bool domain_match = false;
        if (c.domain.empty() || request_url.host == c.domain) {
            domain_match = true;
        } else if (c.domain.starts_with(".")) {
            if (request_url.host.ends_with(c.domain.view(1))) {
                domain_match = true;
            }
        }

        if (!domain_match) {
            continue;
        }
        if (!request_url.path.starts_with(c.path.view())) {
            continue;
        }
        if (c.secure && request_url.scheme != "https") {
            continue;
        }

        if (!cookie_header.empty()) {
            cookie_header += "; ";
        }
        cookie_header += c.name.cookie_name() + "=" + c.value;
    }
    return cookie_header;
}

void http_client::set_cookie(const cookie& c, const string& domain, const string& path) {
    lock<mutex> lk(mutex_);
    const string key = c.name.cookie_name() + "@" + domain + path;
    cookie_jar_[key] = c;
}

http_client::http_client(config config)
: config_(move(config)) {
    persistent_headers_["User-Agent"] = config_.user_agent;
    persistent_headers_["Accept"] = "*/*";

    client_.set_connect_timeout(config_.connect_timeout);
    client_.set_send_timeout(config_.send_timeout);
    client_.set_recv_timeout(config_.receive_timeout);
}

#ifdef NEFORCE_SUPPORT_OPENSSL
http_client::http_client(ssl_context ctx, config config)
: client_(_NEFORCE move(ctx)), config_(_NEFORCE move(config)) {
    persistent_headers_["User-Agent"] = config_.user_agent;
    persistent_headers_["Accept"] = "*/*";

    client_.set_connect_timeout(config_.connect_timeout);
    client_.set_send_timeout(config_.send_timeout);
    client_.set_recv_timeout(config_.receive_timeout);
    client_.set_verify_peer(config_.verify_ssl);
}

void http_client::set_ssl_context(ssl_context ctx) {
    client_.set_ssl_context(move(ctx));
}

void http_client::set_verify_ssl(const bool verify) {
    config_.verify_ssl = verify;
    client_.set_verify_peer(verify);
}
#endif

http_client_response http_client::get(const string& url, const unordered_map<string, string>& headers) {
    _NEFORCE url parsed_url{url::parse(url.view())};

    http_client_request req;
    req.host = parsed_url.host;
    req.port = _NEFORCE url::default_port(parsed_url.scheme.view());
    req.method = HTTP_METHOD::GET;
    req.path = parsed_url.path.empty() ? "/" : parsed_url.path;

    if (!parsed_url.query.empty()) {
        req.path += "?" + parsed_url.query;
    }
    req.headers = headers;

    return request(move(req));
}

http_client_response http_client::post(
    const string& url,
    const string& body,
    const string& content_type,
    const unordered_map<string, string>& headers) {

    _NEFORCE url parsed_url(url::parse(url.view()));

    http_client_request req;
    req.host = parsed_url.host;
    req.port = _NEFORCE url::default_port(parsed_url.scheme.view());
    req.method = HTTP_METHOD::POST;
    req.path = parsed_url.path.empty() ? "/" : parsed_url.path;

    if (!parsed_url.query.empty()) {
        req.path += "?" + parsed_url.query;
    }

    req.headers = headers;
    req.headers["Content-Type"] = content_type;
    req.body = body;

    return request(move(req));
}

http_client_response http_client::post_json(
    const string& url_str,
    const string& json_body,
    const unordered_map<string, string>& headers) {

    return post(url_str, json_body, "application/json", headers);
}

http_client_response http_client::post_form(
    const string& url_str,
    const unordered_map<string, string>& form_data,
    const unordered_map<string, string>& headers) {

    const string body = build_query_string(form_data);
    return post(url_str, body, "application/x-www-form-urlencoded", headers);
}

http_client_response http_client::put(
    const string& url,
    const string& body,
    const string& content_type,
    const unordered_map<string, string>& headers) {

    _NEFORCE url parsed_url(url::parse(url.view()));

    http_client_request req;
    req.host = parsed_url.host;
    req.port = _NEFORCE url::default_port(parsed_url.scheme.view());
    req.method = HTTP_METHOD::PUT;
    req.path = parsed_url.path.empty() ? "/" : parsed_url.path;

    if (!parsed_url.query.empty()) {
        req.path += "?" + parsed_url.query;
    }

    req.headers = headers;
    req.headers["Content-Type"] = content_type;
    req.body = body;

    return request(move(req));
}

http_client_response http_client::del(const string& url, const unordered_map<string, string>& headers) {
    _NEFORCE url parsed_url(url::parse(url.view()));

    http_client_request req;
    req.host = parsed_url.host;
    req.port = _NEFORCE url::default_port(parsed_url.scheme.view());
    req.method = HTTP_METHOD::DELETE;
    req.path = parsed_url.path.empty() ? "/" : parsed_url.path;

    if (!parsed_url.query.empty()) {
        req.path += "?" + parsed_url.query;
    }

    req.headers = headers;

    return request(move(req));
}

http_client_response http_client::head(const string& url, const unordered_map<string, string>& headers) {
    _NEFORCE url parsed_url(url::parse(url.view()));

    http_client_request req;
    req.host = parsed_url.host;
    req.port = _NEFORCE url::default_port(parsed_url.scheme.view());
    req.method = HTTP_METHOD::HEAD;
    req.path = parsed_url.path.empty() ? "/" : parsed_url.path;

    if (!parsed_url.query.empty()) {
        req.path += "?" + parsed_url.query;
    }

    req.headers = headers;

    return request(move(req));
}

http_client_response http_client::options(const string& url, const unordered_map<string, string>& headers) {
    _NEFORCE url parsed_url(url::parse(url.view()));

    http_client_request req;
    req.host = parsed_url.host;
    req.port = _NEFORCE url::default_port(parsed_url.scheme.view());
    req.method = HTTP_METHOD::OPTIONS;
    req.path = parsed_url.path.empty() ? "/" : parsed_url.path;

    if (!parsed_url.query.empty()) {
        req.path += "?" + parsed_url.query;
    }

    req.headers = headers;

    return request(move(req));
}

http_client_response http_client::patch(
    const string& url,
    const string& body,
    const string& content_type,
    const unordered_map<string, string>& headers) {

    _NEFORCE url parsed_url(url::parse(url.view()));

    http_client_request req;
    req.host = parsed_url.host;
    req.port = _NEFORCE url::default_port(parsed_url.scheme.view());
    req.method = HTTP_METHOD::PATCH;
    req.path = parsed_url.path.empty() ? "/" : parsed_url.path;

    if (!parsed_url.query.empty()) {
        req.path += "?" + parsed_url.query;
    }

    req.headers = headers;
    req.headers["Content-Type"] = content_type;
    req.body = body;

    return request(_NEFORCE move(req));
}

bool http_client::download_file(const string& url, path output) {
    try {
        const auto response = get(url);

        if (!response.is_success()) {
            return false;
        }

        file f(move(output));

        if (!f.is_opened()) {
            return false;
        }
        if (f.write(response.body) != response.body.size()) {
            return false;
        }
        return true;
    } catch (const exception& e) {
        if (error_callback_) {
            error_callback_(e);
        }
        return false;
    }
}

http_client_response http_client::request(http_client_request req) {
    return do_request(move(req), 0);
}

future<http_client_response> http_client::request_async(http_client_request req) {
    return _NEFORCE async(launch::async, [this, req = move(req)]() mutable {
        return request(move(req));
    });
}

void http_client::close() {
    client_.disconnect();
}

NEFORCE_END_NAMESPACE__
