#include <NeForce/core/async/async.hpp>
#include <NeForce/core/utility/hexadecimal.hpp>
#include <NeForce/network/http/http_client.hpp>
NEFORCE_BEGIN_NAMESPACE__

string http_client::build_request_str(const http_client_request& req, const url& req_url) const {
    string req_str = req.method.method() + " " + req.path + " " + req.version + "\r\n";
    req_str += "Host: " + req.host;
    if ((req_url.scheme == "http" && req.port != 80) || (req_url.scheme == "https" && req.port != 443)) {
        req_str += ":" + to_string(req.port);
    }
    req_str += "\r\n";

    string cookie_header = build_cookie_header(req_url);
    if (!cookie_header.empty()) {
        req_str += "Cookie: " + move(cookie_header) + "\r\n";
    }

    for (const auto& kv : persistent_headers_) {
        const string& key = kv.first;
        const string& value = kv.second;
        if (req.headers.find(key) == req.headers.end() && key != "Host" && key != "Cookie") {
            req_str += key + ": " + value + "\r\n";
        }
    }

    bool has_content_type = false;
    for (const auto& kv : req.headers) {
        string key = kv.first;
        string value = kv.second;

        string key_lower = key;
        key_lower.lowercase();
        if (key_lower == "content-type") has_content_type = true;
        if (key_lower != "host" && key_lower != "cookie") {
            req_str += key + ": " + value + "\r\n";
        }
    }

    if (!req.body.empty()) {
        if (!has_content_type) {
            req_str += "Content-Type: application/x-www-form-urlencoded\r\n";
        }
        req_str += "Content-Length: " + to_string(req.body.size()) + "\r\n";
    }

    if (!config_.keep_alive) {
        req_str += "Connection: close\r\n";
    }

    req_str += "\r\n";

    if (!req.body.empty()) {
        req_str += req.body;
    }

    return req_str;
}

bool http_client::send_request(const string_view request_str) {
    try {
        const ssize_t sent = client_.send(request_str);
        return sent == static_cast<ssize_t>(request_str.size());
    } catch (...) {
        return false;
    }
}

optional<http_client_response> http_client::read_response() {
    http_client_response response;
    string response_data;
    char buffer[8192];

    auto start_time = steady_clock::now();

    try {
        while (response_data.size() < config_.max_response_size) {
            ssize_t n = client_.receive(buffer, sizeof(buffer));
            if (n < 0) {
                // 超时或错误
                break;
            }
            if (n == 0) {
                // 连接关闭
                break;
            }

            response_data.append(buffer, n);

            if (response_data.find("\r\n\r\n") != string::npos) {
                http_client_response temp;
                if (parse_response(response_data.view(), temp)) {
                    if (temp.content_length > 0) {
                        size_t header_end = response_data.find("\r\n\r\n") + 4;
                        size_t body_received = response_data.size() - header_end;
                        if (body_received >= temp.content_length) {
                            break;
                        }
                    } else if (!temp.chunked) {}
                }
            }
        }

        if (!parse_response(response_data.view(), response)) {
            return none;
        }

        response.total_time = time_cast<milliseconds>(steady_clock::now() - start_time);

        return response;

    } catch (...) {
        return none;
    }
}

bool http_client::parse_chunked_body(const string_view chunked, string& decoded) {
    decoded.clear();
    size_t pos = 0;
    const size_t size = chunked.size();

    while (pos < size) {
        const auto line_end = chunked.find("\r\n", pos);
        if (line_end == string::npos) return false;

        auto size_str = chunked.substr(pos, line_end - pos).trim();
        size_t semicolon = size_str.find(';');
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
            pos += 2;
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

string http_client::url_encode(string_view str) {
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

string http_client::url_decode(string_view str) {
    string result;
    result.reserve(str.size());

    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '%' && i + 2 < str.size()) {
            int high = hexadecimal::digit_value(str[i + 1]);
            int low = hexadecimal::digit_value(str[i + 2]);
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

http_client_response http_client::do_request(http_client_request request, int redirect_count) {
    http_client_response response;

    url req_url;
    req_url.scheme = (request.port == 443) ? "https" : "http";
    req_url.host = request.host;
    req_url.port = request.port;
    req_url.path = request.path;

    bool is_ssl = (req_url.scheme == "https");

    try {
        if (!client_.is_connected()) {
            if (!client_.connect(request.host, request.port)) {
                response.status = HTTP_STATUS::S5_INTERNAL_ERROR;
                response.status_message = "Connection failed";
                response.effective_url = req_url.to_string();
                return response;
            }
        }
    } catch (const exception& e) {
        response.status = HTTP_STATUS::S5_INTERNAL_ERROR;
        response.status_message = "Connection error: "_s + e.what();
        response.effective_url = req_url.to_string();
        return response;
    }

    string request_str = build_request_str(request, req_url);
    if (!send_request(request_str.view())) {
        response.status = HTTP_STATUS::S5_INTERNAL_ERROR;
        response.status_message = "Send failed";
        response.effective_url = req_url.to_string();
        return response;
    }

    auto resp_opt = read_response();
    if (!resp_opt) {
        response.status = HTTP_STATUS::S5_INTERNAL_ERROR;
        response.status_message = "Receive/Parse failed";
        response.effective_url = req_url.to_string();
        return response;
    }

    response = move(*resp_opt);
    response.effective_url = req_url.to_string();
    response.redirect_count = redirect_count;

    update_cookies(response.cookies, req_url);
    const auto status_num = static_cast<uint16_t>(response.status);
    const bool is_redirect =
        status_num == 301 || status_num == 302 || status_num == 303 ||
        status_num == 307 || status_num == 308;

    if (config_.follow_redirects && is_redirect && redirect_count < config_.max_redirects) {
        string_view location = response.header("Location");
        if (!location.empty()) {
            url new_url;
            if (location.starts_with("http://") || location.starts_with("https://")) {
                new_url = url(location);
            } else if (location.starts_with("/")) {
                new_url = req_url;
                new_url.path = location;
            } else {
                new_url = req_url;
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
            new_req.method = request.method;
            new_req.path = new_url.path.empty() ? "/" : new_url.path;
            new_req.version = request.version;
            new_req.headers = request.headers;
            new_req.body = request.body;

            return do_request(move(new_req), redirect_count + 1);
        }
    }

    return response;
}

cookie http_client::parse_set_cookie(const string_view str, string default_domain, string default_path) {
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

void http_client::update_cookies(const vector<cookie>& resp_cookies, const url& request_url) {
    for (const auto &c : resp_cookies) {
        // 简化：以 name@domain+path 作为key，支持多域名路径cookie隔离
        string domain = c.domain.empty() ? request_url.host : c.domain;
        string path = c.path;
        string key = c.name.cookie_name() + "@" + domain + path;
        cookie_jar_[key] = c;
    }
}

string http_client::build_cookie_header(const url& request_url) const {
    string cookie_header;
    for (const auto& kv : cookie_jar_) {
        const auto& c = kv.second;
        // 只发送有效、未过期、作用域匹配的cookie （简化实现，不做完整子域判定）
        if (c.max_age == 0) continue;

        if (request_url.host == c.domain || c.domain.empty()) {
            if (!cookie_header.empty()) cookie_header += "; ";
            cookie_header += c.name.cookie_name() + "=" + c.value;
        }
    }
    return cookie_header;
}

bool http_client::parse_response(const string_view resp_str, http_client_response& resp) {
    const size_t line_end = resp_str.find("\r\n");
    if (line_end == string::npos) return false;

    const string_view status_line = resp_str.substr(0, line_end);

    const size_t sp1 = status_line.find(' ');
    if (sp1 == string::npos) return false;

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

    const size_t sp2 = status_line.find(' ', sp1 + 1);
    if (sp2 == string::npos) return false;

    uint16_t code = to_uint16(status_line.substr(sp1 + 1, sp2 - sp1 - 1));
    resp.status = static_cast<HTTP_STATUS>(code);
    resp.status_message = status_line.substr(sp2 + 1);

    const size_t header_start = line_end + 2;
    const size_t header_end = resp_str.find("\r\n\r\n", header_start);
    if (header_end == string::npos) return false;

    const string_view headers_block = resp_str.substr(header_start, header_end - header_start);
    size_t hpos = 0;

    while (hpos < headers_block.size()) {
        const auto hline_end = headers_block.find("\r\n", hpos);
        string_view line = headers_block.substr(hpos,
            (hline_end == string::npos) ? headers_block.size() - hpos : hline_end - hpos);

        if (line.empty()) break;

        const size_t colon = line.find(':');
        if (colon != string::npos) {
            string key = line.substr(0, colon).trim();
            string val = line.substr(colon + 1).trim();
            resp.headers[key].push_back(val);

            string key_lower = key;
            key_lower.lowercase();
            if (key_lower == "set-cookie") {
                cookie c = parse_set_cookie(val.view(), resp.header("host"), "/");
                resp.cookies.emplace_back(move(c));
            } else if (key_lower == "transfer-encoding") {
                resp.chunked = val.find("chunked") != string::npos;
            } else if (key_lower == "content-length") {
                try {
                    resp.content_length = uinteger64::parse(val.view());
                } catch (...) {
                    // ignore
                }
            }
        }

        hpos = (hline_end == string::npos) ? headers_block.size() : hline_end + 2;
    }

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

http_client::http_client(config config)
: config_(move(config)) {
    persistent_headers_["User-Agent"] = "NeForce HTTP Client/1.0";
    persistent_headers_["Accept"] = "*/*";
}

#ifdef NEFORCE_SUPPORT_OPENSSL
http_client::http_client(ssl_context ctx, config config)
: client_(move(ctx)), config_(move(config)) {
    persistent_headers_["User-Agent"] = "NeForce HTTP Client/1.0";
    persistent_headers_["Accept"] = "*/*";
}
#endif

void http_client::set_ssl_context(ssl_context ctx) {
    client_.set_ssl_context(move(ctx));
}

void http_client::set_cookie(const cookie& c, const string& domain, const string& path) {
    string key = c.name.cookie_name() + "@" + domain + path;
    cookie_jar_[key] = c;
}

http_client_response http_client::get(const string& url_str, const unordered_map<string, string>& headers) {
    url parsed_url(url_str.view());
    http_client_request req;
    req.host = parsed_url.host;
    req.port = url::default_port(parsed_url.scheme.view());

    req.method = HTTP_METHOD::GET;
    req.path = parsed_url.path.empty() ? "/" : parsed_url.path;
    if (!parsed_url.query.empty()) {
        req.path += "?" + parsed_url.query;
    }
    req.headers = headers;

    return request(move(req));
}

http_client_response http_client::post(
    const string& url_str,
    const string& body,
    const string& content_type,
    const unordered_map<string, string>& headers) {

    url parsed_url(url_str.view());
    http_client_request req;
    req.host = parsed_url.host;
    req.port = url::default_port(parsed_url.scheme.view());

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

http_client_response http_client::put(
    const string& url_str,
    const string& body,
    const string& content_type,
    const unordered_map<string, string>& headers) {

    url parsed_url(url_str.view());
    http_client_request req;
    req.host = parsed_url.host;
    req.port = url::default_port(parsed_url.scheme.view());

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

http_client_response http_client::del(const string& url_str, const unordered_map<string, string>& headers) {
    url parsed_url(url_str.view());
    http_client_request req;
    req.host = parsed_url.host;
    req.port = url::default_port(parsed_url.scheme.view());

    req.method = HTTP_METHOD::DELETE;
    req.path = parsed_url.path.empty() ? "/" : parsed_url.path;
    if (!parsed_url.query.empty()) {
        req.path += "?" + parsed_url.query;
    }
    req.headers = headers;

    return request(move(req));
}

http_client_response http_client::head(const string& url_str, const unordered_map<string, string>& headers) {
    url parsed_url(url_str.view());
    http_client_request req;
    req.host = parsed_url.host;
    req.port = url::default_port(parsed_url.scheme.view());

    req.method = HTTP_METHOD::HEAD;
    req.path = parsed_url.path.empty() ? "/" : parsed_url.path;
    if (!parsed_url.query.empty()) {
        req.path += "?" + parsed_url.query;
    }
    req.headers = headers;

    return request(move(req));
}

http_client_response http_client::options(const string& url_str, const unordered_map<string, string>& headers) {
    url parsed_url(url_str.view());
    http_client_request req;
    req.host = parsed_url.host;
    req.port = url::default_port(parsed_url.scheme.view());

    req.method = HTTP_METHOD::OPTIONS;
    req.path = parsed_url.path.empty() ? "/" : parsed_url.path;
    if (!parsed_url.query.empty()) {
        req.path += "?" + parsed_url.query;
    }
    req.headers = headers;

    return request(move(req));
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
