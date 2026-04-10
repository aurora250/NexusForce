#include <NeForce/core/async/async.hpp>
#include <NeForce/core/file/file.hpp>
#include <NeForce/core/utility/hexadecimal.hpp>
#include <NeForce/network/http/http_client.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

namespace {
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
                chunk_size = hexadecimal::parse(size_str).value();
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

    bool parse_response(const string_view resp_str, http_client_response& resp, const string& request_host,
                        const string& request_path) {
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
                    resp.http_version_major = uinteger16::parse(ver.substr(0, dot)).value();
                    resp.http_version_minor = uinteger16::parse(ver.substr(dot + 1)).value();
                    // NOLINTNEXTLINE(bugprone-empty-catch)
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
            const uint16_t code = uinteger16::parse(status_line.substr(sp1 + 1, sp2 - sp1 - 1)).value();
            resp.status = http_status_from_code(code);
            if (resp.status_message.empty()) {
                resp.status_message = http_status_message(resp.status);
            }
            // NOLINTNEXTLINE(bugprone-empty-catch)
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
            string_view line = headers_block.substr(hpos, hline_end == string::npos ? headers_block.size() - hpos
                                                                                    : hline_end - hpos);
            if (line.empty()) {
                break;
            }

            const size_t colon = line.find(':');

            if (colon != string::npos) {
                const auto key = line.substr(0, colon).trim();
                const auto value = line.substr(colon + 1).trim();
                resp.headers[key].push_back(value);

                string key_lower = key;
                key_lower.lowercase();

                if (key_lower == "set-cookie") {
                    http_cookie c = http_cookie::parse(value, request_host, request_path);
                    resp.cookies.emplace_back(move(c));
                } else if (key_lower == "transfer-encoding") {
                    resp.chunked = value.contains("chunked");
                } else if (key_lower == "content-length") {
                    try {
                        resp.content_length = uinteger64::parse(value).value();
                        // NOLINTNEXTLINE(bugprone-empty-catch)
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
            const size_t to_copy = min(body_part.size(), resp.content_length);
            resp.body += body_part.substr(0, to_copy);
            if (to_copy < resp.content_length) {
                return false;
            }
        } else {
            resp.body += body_part;
        }

        return true;
    }
} // namespace


string http_client::build_request_str(const http_client_request& req, const url& req_url) const {
    string req_str;
    req_str.reserve(1024);

    const string full_path = req.build_full_path();

    req_str += req.method.method() + " " + full_path + " " + req.version + "\r\n";

    req_str += "Host: " + req.host;
    if ((req_url.scheme == "http" && req.port != 80) || (req_url.scheme == "https" && req.port != 443)) {
        req_str += ":" + to_string(req.port);
    }
    req_str += "\r\n";

    // Cookie header
    string cookie_header = build_cookie_header(req_url);
    if (!cookie_header.empty()) {
        req_str += "Cookie: " + move(cookie_header) + "\r\n";
    }

    // Persistent headers
    for (const auto& kv: persistent_headers_) {
        const string& key = kv.first;
        const string& value = kv.second;
        if (req.headers.find(key) == req.headers.end() && key != "Host" && key != "Cookie") {
            req_str += key + ": " + value + "\r\n";
        }
    }

    // Request headers
    bool has_content_type = false;
    for (const auto& kv: req.headers) {
        const string& key = kv.first;
        const string& value = kv.second;
        string key_lower = key;
        key_lower.lowercase();

        if (key_lower == "content-type") {
            has_content_type = true;
        }
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

optional<http_client_response> http_client::read_response(time_point& receive_start, const string& request_host,
                                                          const string& request_path) {

    receive_start = steady_clock::now();

    string response_data;
    vector<char> buffer(config_.buffer_size);

    try {
        size_t header_end_pos = string::npos;
        while (header_end_pos == string::npos) {
            const ssize_t n = client_.receive(buffer.data(), buffer.size());
            if (n <= 0) {
                break;
            }
            response_data.append(buffer.data(), n);
            header_end_pos = response_data.find("\r\n\r\n");
        }

        if (header_end_pos == string::npos) {
            return none;
        }

        http_client_response meta;
        parse_response(response_data.view(0, header_end_pos + 4), meta, request_host, request_path);

        const size_t body_start = header_end_pos + 4;
        size_t body_received = response_data.size() - body_start;

        if (meta.content_length > 0) {
            if (meta.content_length > config_.max_response_size) {
                return none;
            }

            response_data.reserve(body_start + meta.content_length);

            while (body_received < meta.content_length) {
                const ssize_t n = client_.receive(buffer.data(), buffer.size());
                if (n <= 0) {
                    break;
                }
                response_data.append(buffer.data(), n);
                body_received += n;

                if (progress_callback_) {
                    progress_callback_(move(body_received), move(meta.content_length));
                }
            }

            if (body_received < meta.content_length) {
                return none;
            }
        } else if (meta.chunked) {
            while (true) {
                if (response_data.find("0\r\n\r\n", body_start) != string::npos) {
                    break;
                }
                if (response_data.size() >= config_.max_response_size) {
                    break;
                }

                const ssize_t n = client_.receive(buffer.data(), buffer.size());
                if (n <= 0) {
                    break;
                }
                response_data.append(buffer.data(), n);
            }
        } else {
            while (response_data.size() < config_.max_response_size) {
                const ssize_t n = client_.receive(buffer.data(), buffer.size());
                if (n <= 0) {
                    break;
                }
                response_data.append(buffer.data(), n);
            }
        }

        http_client_response final_response;
        if (!parse_response(response_data.view(), final_response, request_host, request_path)) {
            return none;
        }
        return final_response;

    } catch (const exception& e) {
        if (error_callback_) {
            error_callback_(e);
        }
        return none;
    }
}

bool http_client::ensure_connected(const string& host, const ports port) {
    const bool is_https = port == ports::https;

    if (client_.is_connected()) {
        if (client_.connected_host() == host && client_.connected_port() == port) {
            return true;
        }
        client_.disconnect();
    }

    try {
        // Set SNI hostname for HTTPS connections
        if (is_https) {
            client_.set_sni_hostname(host);
        }
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
    req_url.scheme = request.port.to_string();
    req_url.host = request.host;
    req_url.port = request.port;
    req_url.path = request.path;

    const auto connect_start = steady_clock::now();
    if (!ensure_connected(request.host, request.port)) {
        response.status = http_status::S5_INTERNAL_ERROR;
        response.status_message = "Connection failed";
        response.effective_url = req_url.to_string();
        return response;
    }
    response.connect_time = time_cast<milliseconds>(steady_clock::now() - connect_start);

    const string request_str = build_request_str(request, req_url);
    steady_clock::time_point send_start;
    if (!send_request(request_str.view(), send_start)) {
        response.status = http_status::S5_INTERNAL_ERROR;
        response.status_message = "Send failed";
        response.effective_url = req_url.to_string();
        return response;
    }
    response.send_time = time_cast<milliseconds>(steady_clock::now() - send_start);

    steady_clock::time_point receive_start;
    auto resp_opt = read_response(receive_start, request.host, request.path);
    if (!resp_opt) {
        response.status = http_status::S5_INTERNAL_ERROR;
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

    if (config_.follow_redirects && response.is_redirect() &&
        static_cast<uint16_t>(redirect_count) < config_.max_redirects) {
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
            new_req.port = ports::parse(new_url.scheme.view());
            new_req.path = new_url.path.empty() ? "/" : new_url.path;
            new_req.version = request.version;
            new_req.headers = request.headers;

            const auto status_code = static_cast<uint16_t>(response.status);
            if (status_code == 303) {
                new_req.method = http_method::GET();
                new_req.body.clear();
            } else if (status_code == 301 || status_code == 302) {
                if (request.method.is_post()) {
                    new_req.method = http_method::GET();
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

void http_client::update_cookies(const vector<http_cookie>& resp_cookies, const url& request_url) {
    lock<mutex> lk(mutex_);

    for (const auto& c: resp_cookies) {
        string domain = c.domain.empty() ? request_url.host : c.domain;
        string path = c.path;
        string key = c.name.cookie_name() + "@" + domain + path;

        const bool should_delete =
                (c.max_age == 0) || (c.max_age > 0 && c.is_valid() && c.expires.is_valid() && c.is_expired());

        if (should_delete) {
            cookie_jar_.erase(key);
        } else {
            cookie_jar_[key] = c;
        }
    }
}

string http_client::build_cookie_header(const url& request_url) const {
    string cookie_header;

    for (const auto& pair: cookie_jar_) {
        const auto& c = pair.second;
        if (c.max_age == 0) {
            continue;
        }
        if (c.max_age > 0 && c.is_valid() && c.expires.is_valid() && c.is_expired()) {
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

void http_client::set_cookie(const http_cookie& c, const string& domain, const string& path) {
    lock<mutex> lk(mutex_);
    const string key = c.name.cookie_name() + "@" + domain + path;
    cookie_jar_[key] = c;
}

http_client::http_client(config config) :
config_(move(config)) {
    persistent_headers_["User-Agent"] = config_.user_agent;
    persistent_headers_["Accept"] = "*/*";

    client_.set_connect_timeout(config_.connect_timeout);
    client_.set_send_timeout(config_.send_timeout);
    client_.set_recv_timeout(config_.receive_timeout);
}

http_client::http_client(ssl_context ctx, config config) :
client_(_NEFORCE move(ctx)),
config_(_NEFORCE move(config)) {
    persistent_headers_["User-Agent"] = config_.user_agent;
    persistent_headers_["Accept"] = "*/*";

    client_.set_connect_timeout(config_.connect_timeout);
    client_.set_send_timeout(config_.send_timeout);
    client_.set_recv_timeout(config_.receive_timeout);
    client_.set_verify_peer(config_.verify_ssl);
}

void http_client::set_ssl_context(ssl_context ctx) { client_.set_ssl_context(move(ctx)); }

void http_client::set_verify_ssl(const bool verify) {
    config_.verify_ssl = verify;
    client_.set_verify_peer(verify);
}

http_client_response http_client::get(const string& url, const unordered_map<string, string>& headers) {
    _NEFORCE url parsed_url{url::parse(url.view())};

    http_client_request req;
    req.host = parsed_url.host;
    req.port = ports::parse(parsed_url.scheme.view());
    req.method = http_method::GET();
    req.path = parsed_url.path.empty() ? "/" : parsed_url.path;

    if (!parsed_url.query.empty()) {
        req.path += "?" + parsed_url.query;
    }
    req.headers = headers;

    return request(move(req));
}

http_client_response http_client::post(const string& url, const string& body, const string& content_type,
                                       const unordered_map<string, string>& headers) {

    _NEFORCE url parsed_url(url::parse(url.view()));

    http_client_request req;
    req.host = parsed_url.host;
    req.port = ports::parse(parsed_url.scheme.view());
    req.method = http_method::POST();
    req.path = parsed_url.path.empty() ? "/" : parsed_url.path;

    if (!parsed_url.query.empty()) {
        req.path += "?" + parsed_url.query;
    }

    req.headers = headers;
    req.headers["Content-Type"] = content_type;
    req.body = body;

    return request(move(req));
}

http_client_response http_client::post_json(const string& url_str, const string& json_body,
                                            const unordered_map<string, string>& headers) {

    return post(url_str, json_body, "application/json", headers);
}

http_client_response http_client::post_form(const string& url_str, const unordered_map<string, string>& form_data,
                                            const unordered_map<string, string>& headers) {
    const string body = url::build_query(form_data);
    return post(url_str, body, "application/x-www-form-urlencoded", headers);
}

http_client_response http_client::put(const string& url, const string& body, const string& content_type,
                                      const unordered_map<string, string>& headers) {

    _NEFORCE url parsed_url(url::parse(url.view()));

    http_client_request req;
    req.host = parsed_url.host;
    req.port = ports::parse(parsed_url.scheme.view());
    req.method = http_method::PUT();
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
    req.port = ports::parse(parsed_url.scheme.view());
    req.method = http_method::DELETE();
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
    req.port = ports::parse(parsed_url.scheme.view());
    req.method = http_method::HEAD();
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
    req.port = ports::parse(parsed_url.scheme.view());
    req.method = http_method::OPTIONS();
    req.path = parsed_url.path.empty() ? "/" : parsed_url.path;

    if (!parsed_url.query.empty()) {
        req.path += "?" + parsed_url.query;
    }

    req.headers = headers;

    return request(move(req));
}

http_client_response http_client::patch(const string& url, const string& body, const string& content_type,
                                        const unordered_map<string, string>& headers) {

    _NEFORCE url parsed_url(url::parse(url.view()));

    http_client_request req;
    req.host = parsed_url.host;
    req.port = ports::parse(parsed_url.scheme.view());
    req.method = http_method::PATCH();
    req.path = parsed_url.path.empty() ? "/" : parsed_url.path;

    if (!parsed_url.query.empty()) {
        req.path += "?" + parsed_url.query;
    }

    req.headers = headers;
    req.headers["Content-Type"] = content_type;
    req.body = body;

    return request(_NEFORCE move(req));
}

bool http_client::download_file(const string& url, path output, const bool is_binary) {
    try {
        auto response = get(url);

        if (!response.is_success()) {
            return false;
        }

        file f(move(output), false, file_access::READ_WRITE, file_shared::SHARE_READ, file_creation::CREATE_FORCE);

        if (!f.is_opened()) {
            return false;
        }

        if (is_binary) {
            if (f.write(response.body.data(), response.body.size()) != response.body.size()) {
                return false;
            }
        } else {
            if (f.write(response.body) != response.body.size()) {
                return false;
            }
        }

        return true;
    } catch (const exception& e) {
        if (error_callback_) {
            error_callback_(e);
        }
        return false;
    }
}

http_client_response http_client::request(http_client_request req) { return do_request(move(req), 0); }

future<http_client_response> http_client::request_async(http_client_request req) {
    return _NEFORCE async(launch::async, [this, req = move(req)]() mutable { return request(move(req)); });
}

void http_client::close() { client_.disconnect(); }

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
