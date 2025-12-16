#include <MSTL/core/utility/hexadecimal.hpp>
#include <MSTL/core/string/string_util.hpp>
#include <MSTL/core/time/datetime.hpp>
#include <MSTL/network/http/http_client.hpp>
#ifdef MSTL_PLATFORM_LINUX__
#include <arpa/inet.h>
#endif
MSTL_BEGIN_NAMESPACE__

bool http_client::try_connect(const string& host, const uint16_t port, const string& ip, const bool ipv6) {
    socket s(
        ipv6 ? SOCKET_DOMAIN::IPV6 : SOCKET_DOMAIN::IPV4,
        SOCKET_TYPE::STREAM,
        SOCKET_PROTOCOL::TCP);

    if (!s.is_valid()) return false;
    if (!s.set_send_timeout(connect_timeout_) ||
        !s.set_receive_timeout(read_timeout_)) {
        return false;
    }

    ::sockaddr_storage addr{};
    ::socklen_t addr_len;
    if (ipv6) {
        auto *a6 = reinterpret_cast<::sockaddr_in6 *>(&addr);
        a6->sin6_family = AF_INET6;
        ::inet_pton(AF_INET6, ip.c_str(), &a6->sin6_addr);
        a6->sin6_port = ::htons(port);
        addr_len = sizeof(::sockaddr_in6);
    } else {
        auto *a4 = reinterpret_cast<::sockaddr_in *>(&addr);
        a4->sin_family = AF_INET;
        ::inet_pton(AF_INET, ip.c_str(), &a4->sin_addr);
        a4->sin_port = ::htons(port);
        addr_len = sizeof(::sockaddr_in);
    }

    if (!s.connect(reinterpret_cast<::sockaddr *>(&addr), addr_len)) {
        return false;
    }

#ifdef MSTL_SUPPORT_OPENSSL__
    if (use_ssl_) {
        init_ssl_context();
        ssl_sock_ = ssl_socket(ssl_ctx_.context(), s);
        if (!ssl_sock_.accept()) {
            return false;
        }
        use_ssl_ = true;
    } else {
        sock_ = _MSTL move(s);
        use_ssl_ = false;
    }
#else
    if (use_ssl) {
        return false;
    }
    sock_ = _MSTL move(s);
    use_ssl_ = false;
#endif

    connected_ = true;
    connected_host_ = host;
    connected_port_ = port;
    return true;
}

bool http_client::connect_domain(const string& host, const uint16_t port) {
    if (connected_ && connected_host_ == host && connected_port_ == port) {
        return true;
    }
    close_connection();

    vector<string> ips = dns_.resolve_a(host);
    vector<string> ips6;
    if (ips.empty()) ips6 = dns_.resolve_aaaa(host);

    for (auto &ip : ips) if (try_connect(host, port, ip, false)) return true;
    for (auto &ip : ips6) if (try_connect(host, port, ip, true)) return true;

    return false;
}

void http_client::close_connection() noexcept {
    if (connected_) {
#ifdef MSTL_SUPPORT_OPENSSL__
        if (use_ssl_) {
            ssl_sock_.close();
        } else {
            sock_.close();
        }
#else
        sock_.close();
#endif
        connected_ = false;
        connected_host_.clear();
        connected_port_ = 0;
    }
}

string http_client::build_request_str(const http_client_request& req, const url& req_url) const {
    string req_str = req.method.method() + " " + req.path + " " + req.version + HTTP_CRLF;
    req_str += "Host: " + req.host;

    if (req.port != 80 && req.port != 443) {
        req_str += ":" + _MSTL to_string(req.port);
    }
    req_str += HTTP_CRLF;

    string cookie_header = build_cookie_header(req_url);

    if (!cookie_header.empty()) {
        req_str += "Cookie: " + _MSTL move(cookie_header) + HTTP_CRLF;
    }

    bool has_content_type = false;
    for (const auto &kv : req.headers) {
        string key = kv.first;
        string value = kv.second;

        if (key == "Content-Type") has_content_type = true;
        if (key == "Host" || key == "Cookie") continue;

        req_str += key + ": " + value + HTTP_CRLF;
    }

    if (!req.body.empty()) {
        if (!has_content_type) {
            req_str += "Content-Type: application/x-www-form-urlencoded"_s + HTTP_CRLF;
        }
        req_str += "Content-Length: " + _MSTL to_string(req.body.size()) + HTTP_CRLF;
    }
    req_str += "Connection: close"_s + HTTP_CRLF2;
    if (!req.body.empty()) req_str += req.body;
    return req_str;
}

bool http_client::send_request(const string& request_str) const {
    const ssize_t total = static_cast<ssize_t>(request_str.size());
    ssize_t sent = 0;

    while (sent < total) {
        ssize_t n;
#ifdef MSTL_SUPPORT_OPENSSL__
        if (use_ssl_) {
            n = ssl_sock_.write(request_str.data() + sent, total - sent);
        } else {
            n = sock_.send(request_str.data() + sent, total - sent);
        }
#else
        n = sock_.send(request_str.data() + sent, total - sent);
#endif
        if (n <= 0) return false;
        sent += n;
    }
    return true;
}

bool http_client::read_response(string& out_data) const {
    constexpr size_t buf_size = 8192;
    char buffer[buf_size];
    out_data.clear();

    while (true) {
        ssize_t n;
#ifdef MSTL_SUPPORT_OPENSSL__
        if (use_ssl_) {
            n = ssl_sock_.read(buffer, buf_size);
        } else {
            n = sock_.receive(buffer, buf_size);
        }
#else
        n = sock_.receive(buffer, buf_size);
#endif

        if (n == 0) break;
        if (n < 0) return false;
        out_data.append(buffer, n);
    }
    return true;
}

bool http_client::parse_chunked_body(const string_view chunked, string& decoded) {
    decoded.clear();
    size_t pos = 0;
    const size_t size = chunked.size();

    while (pos < size) {
        const auto line_end = chunked.find(HTTP_CRLF, pos);
        if (line_end == string::npos) return false;
        auto size_str = chunked.substr(pos, line_end - pos);
        auto chunk_size = _MSTL hexadecimal::parse(size_str.trim()).value();
        if (!chunk_size) return false;

        pos = line_end + 2;
        if (pos + chunk_size > size) return false;

        decoded.append(chunked.substr(pos, chunk_size));
        pos += chunk_size + 2;
    }
    return false;
}

#ifdef MSTL_SUPPORT_OPENSSL__
void http_client::init_ssl_context() {
    if (!ssl_ctx_.context()) {
        ssl_ctx_ = ssl_context();
    }
}
#endif

cookie http_client::parse_set_cookie(
    const string_view str,
    const string& default_domain,
    const string& default_path) {
    vector<string_view> tokens;
    size_t start = 0, end;
    while ((end = str.find(';', start)) != string::npos) {
        tokens.push_back(str.substr(start, end - start).trim());
        start = end + 1;
    }
    tokens.push_back(str.substr(start).trim());

    cookie c;
    if (tokens.empty()) return c;

    const size_t eq_pos = tokens[0].find('=');
    if (eq_pos == string::npos) {
        return c;
    }
    c.set_name(HTTP_COOKIE_NAME(string{tokens[0].substr(0, eq_pos)}));
    c.set_value(string{tokens[0].substr(eq_pos + 1)});
    c.set_domain(default_domain);
    c.set_path(default_path.empty() ? "/" : default_path);

    for (size_t i = 1; i < tokens.size(); ++i) {
        auto &attr = tokens[i];
        auto lower_attr = string(attr);
        lower_attr.lowercase();

        if (lower_attr.starts_with("domain=")) {
            c.set_domain(string(attr.substr(7)));
        } else if (lower_attr.starts_with("path=")) {
            c.set_path(string(attr.substr(5)));
        } else if (lower_attr == "secure") {
            c.set_secure(true);
        } else if (lower_attr == "httponly") {
            c.set_http_only(true);
        } else if (lower_attr.starts_with("max-age=")) {
            try {
                c.set_max_age(integer32::parse(attr.substr(8)));
            } catch (...) {
                // ignore
            }
        } else if (lower_attr.starts_with("samesite=")) {
            c.set_same_site(string(attr.substr(9)));
        } else if (lower_attr.starts_with("expires=")) {
            try {
                c.set_expires(datetime::parse_GMT(attr.substr(8)));
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
        string key = c.name().cookie_name() + "@" + (c.domain().empty() ? request_url.host : c.domain()) + c.path();
        cookie_jar_[key] = c;
    }
}

string http_client::build_cookie_header(const url& request_url) const {
    string cookie_header;
    for (const auto &kv : cookie_jar_) {
        const auto &c = kv.second;
        // 只发送有效、未过期、作用域匹配的cookie （简化实现，不做完整子域判定）
        if (c.max_age() == 0) continue;

        if (request_url.host == c.domain() || c.domain().empty()) {
            if (!cookie_header.empty()) cookie_header += "; ";
            cookie_header += c.name().cookie_name() + "=" + c.value();
        }
    }
    return cookie_header;
}

bool http_client::parse_response(const string_view resp_str, http_client_response& resp) {
    const size_t pos = resp_str.find(HTTP_CRLF);
    if (pos == string::npos) return false;

    const string_view status_line = resp_str.substr(0, pos);
    const size_t sp1 = status_line.find(' ');
    if (sp1 == string::npos) return false;
    const size_t sp2 = status_line.find(' ', sp1 + 1);
    if (sp2 == string::npos) return false;

    resp.set_version(status_line.substr(0, sp1));
    uint16_t code = to_uint16(status_line.substr(sp1 + 1, sp2 - sp1 - 1));
    resp.set_status(static_cast<HTTP_STATUS>(code));
    resp.set_status_msg(status_line.substr(sp2 + 1));

    const size_t header_start = pos + 2;
    const size_t header_end = resp_str.find(HTTP_CRLF2, header_start);
    if (header_end == string::npos) return false;

    const string_view headers_block = resp_str.substr(header_start, header_end - header_start);
    size_t line_start = 0;
    while (line_start < headers_block.size()) {
        const auto line_end = headers_block.find(HTTP_CRLF, line_start);
        string_view line = headers_block.substr(line_start, line_end == string::npos ?
                headers_block.size() - line_start : line_end - line_start);
        line_start = (line_end == string::npos) ? headers_block.size() : (line_end + 2);
        if (line.empty()) break;
        const size_t colon = line.find(':');
        if (colon == string::npos) continue;

        string key = line.substr(0, colon).trim();
        string val = line.substr(colon + 1).trim();
        resp.add_header(key, val);
        key.lowercase();
        if (key == "set-cookie") {
            cookie c = parse_set_cookie(val.view(), resp.header("host"), "/");
            resp.append_cookie(_MSTL move(c));
        }
    }

    const string_view body_part = resp_str.substr(header_end + 4);
    const string& encoding = resp.header("Transfer-Encoding");
    const string& length = resp.header("Content-Length");

    bool is_chunked = false;
    if (!encoding.empty()) {
        string encoding_lower = encoding;
        encoding_lower.lowercase();
        vector<string_view> encodings = _MSTL split(encoding_lower.view(), ","_sv);
        for (const auto& enc : encodings) {
            if (enc.trim() == "chunked") {
                is_chunked = true;
                break;
            }
        }
    }

    if (is_chunked) {
        string decoded;
        if (parse_chunked_body(body_part, decoded)) {
            resp.append_body(_MSTL move(decoded));
        } else {
            return false;
        }
    } else if (!length.empty()) {
        uint64_t cl;
        try {
            cl = uinteger64::parse(length.view()).value();
        } catch (...) {
            return false;
        }
        if (body_part.size() < cl) return false;
        resp.append_body(body_part.substr(0, cl));
    } else {
        resp.append_body(body_part);
    }
    return true;
}

http_client_response http_client::request(http_client_request req) {
    int redirect_count = 0;

    url req_url;
    req_url.scheme = (req.port == 443) ? "https" : "http";
    req_url.host = req.host;
    req_url.port = req.port;
    req_url.path = req.path;

    while (redirect_count <= max_redirect_) {
        if (!connect_domain(req.host, req.port)) {
            http_client_response resp;
            resp.set_status(HTTP_STATUS::S5_INTERNAL_ERROR);
            resp.set_status_msg("Connect failed");
            return resp;
        }

        string req_str = build_request_str(req, req_url);
        if (!send_request(req_str)) {
            close_connection();
            http_client_response resp;
            resp.set_status(HTTP_STATUS::S5_INTERNAL_ERROR);
            resp.set_status_msg("Send failed");
            return resp;
        }

        string resp_str;
        if (!read_response(resp_str)) {
            close_connection();
            http_client_response resp;
            resp.set_status(HTTP_STATUS::S5_INTERNAL_ERROR);
            resp.set_status_msg("Receive failed");
            return resp;
        }

        http_client_response resp;
        if (!parse_response(resp_str.view(), resp)) {
            close_connection();
            resp.set_status(HTTP_STATUS::S5_INTERNAL_ERROR);
            resp.set_status_msg("Parse failed");
            return resp;
        }

        update_cookies(resp.cookies(), req_url);

        if ((resp.status() == HTTP_STATUS::S3_FOUND ||
             resp.status() == HTTP_STATUS::S3_MOVED_PERMANENT ||
             resp.status() == HTTP_STATUS::S3_TEMPORARY_REDIRECT ||
             resp.status() == HTTP_STATUS::S3_PERMANENT_REDIRECT) &&
             !resp.header("Location").empty()) {

            string location = resp.header("Location");

            if (location.starts_with("http://") || location.starts_with("https://")) {
                url new_url(location);
                req.host = new_url.host;
                req.port = new_url.port ? new_url.port : (new_url.scheme == "https" ? 443 : 80);
                req.path = new_url.path.empty() ? "/" : new_url.path;
                req_url = new_url;
            } else if (location.starts_with("/")) {
                req.path = location;
                req_url.path = location;
            } else {
                size_t last_slash = req.path.find_last_of('/');
                if (last_slash != string::npos) {
                    req.path = req.path.substr(0, last_slash + 1) + location;
                } else {
                    req.path = "/" + location;
                }
                req_url.path = req.path;
            }

            redirect_count++;
            close_connection();
            continue;
        }

        close_connection();
        return resp;
    }

    http_client_response resp;
    resp.set_status(HTTP_STATUS::S5_INTERNAL_ERROR);
    resp.set_status_msg("Too many redirects");
    return resp;
}

MSTL_END_NAMESPACE__
