#include <NeForce/core/async/future.hpp>
#include <NeForce/core/async/this_thread.hpp>
#include <NeForce/core/file/file.hpp>
#include <NeForce/core/utility/hexadecimal.hpp>
#include <NeForce/network/http/http_client.hpp>
#ifdef NEFORCE_PLATFORM_POSIX
#    include <poll.h>
#endif
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
                size_str = size_str.head(semicolon);
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

    bool parse_response_head(const string_view resp_str, http_client_response& resp, const string& request_host,
                             const string& request_path, size_t& body_offset) {
        const size_t line_end = resp_str.find("\r\n");
        if (line_end == string::npos) {
            return false;
        }

        const string_view status_line = resp_str.head(line_end);

        const size_t sp1 = status_line.find(' ');
        if (sp1 == string::npos) {
            return false;
        }

        // Parse HTTP version
        const string_view version_str = status_line.head(sp1);
        if (version_str.starts_with("HTTP/")) {
            const string_view ver = version_str.substr(5);
            const size_t dot = ver.find('.');
            if (dot != string::npos) {
                try {
                    resp.http_version_major = uinteger16::parse(ver.head(dot)).value();
                    resp.http_version_minor = uinteger16::parse(ver.tail(dot + 1)).value();
                    // NOLINTNEXTLINE(bugprone-empty-catch)
                } catch (const exception& e) {
                    NEFORCE_REPORT_EXCEPTION(e);
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
        } catch (const exception& e) {
            NEFORCE_REPORT_EXCEPTION(e);
        }
        resp.status_message = status_line.tail(sp2 + 1);

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
                const auto key = line.head(colon).trim();
                const auto value = line.tail(colon + 1).trim();
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
                    } catch (const exception& e) {
                        NEFORCE_REPORT_EXCEPTION(e);
                    }
                }
            }

            hpos = hline_end == string::npos ? headers_block.size() : hline_end + 2;
        }

        body_offset = header_end + 4;
        return true;
    }

    bool parse_response(const string_view resp_str, http_client_response& resp, const string& request_host,
                        const string& request_path) {
        size_t body_offset = 0;
        if (!parse_response_head(resp_str, resp, request_host, request_path, body_offset)) {
            return false;
        }

        // Parse body
        const string_view body_part = resp_str.substr(body_offset);

        if (resp.chunked) {
            string decoded;
            if (parse_chunked_body(body_part, decoded)) {
                resp.body += move(decoded);
            } else {
                return false;
            }
        } else if (resp.content_length > 0) {
            const size_t to_copy = min<uint64_t>(body_part.size(), resp.content_length);
            resp.body += body_part.head(to_copy);
            if (to_copy < resp.content_length) {
                return false;
            }
        } else {
            resp.body += body_part;
        }

        return true;
    }

    class chunk_decoder {
    public:
        void push(const string_view raw) { buffer_.append(raw); }

        // Decodes everything currently buffered into out. Returns false on a malformed stream.
        bool decode(string& out) {
            while (true) {
                if (state_ == state::size_line) {
                    const size_t eol = buffer_.find("\r\n", offset_);
                    if (eol == string::npos) {
                        break;
                    }

                    string_view size_str = buffer_.view(offset_, eol - offset_);
                    const size_t semicolon = size_str.find(';');
                    if (semicolon != string::npos) {
                        size_str = size_str.head(semicolon);
                    }

                    size_t chunk_size = 0;
                    try {
                        chunk_size = hexadecimal::parse(size_str.trim()).value();
                    } catch (...) {
                        failed_ = true;
                        return false;
                    }

                    offset_ = eol + 2;
                    if (chunk_size == 0) {
                        state_ = state::trailer;
                    } else {
                        remaining_ = chunk_size;
                        state_ = state::data;
                    }
                } else if (state_ == state::data) {
                    const size_t available = buffer_.size() - offset_;
                    const size_t taken = min(available, remaining_);
                    if (taken > 0) {
                        out.append(buffer_.view(offset_, taken));
                        offset_ += taken;
                        remaining_ -= taken;
                    }
                    if (remaining_ > 0) {
                        break;
                    }
                    state_ = state::data_crlf;
                } else if (state_ == state::data_crlf) {
                    if (buffer_.size() - offset_ < 2) {
                        break;
                    }
                    offset_ += 2;
                    state_ = state::size_line;
                } else {
                    const size_t eol = buffer_.find("\r\n", offset_);
                    if (eol == string::npos) {
                        break;
                    }
                    const bool blank = (eol == offset_);
                    offset_ = eol + 2;
                    if (blank) {
                        done_ = true;
                        break;
                    }
                }
            }

            compact();
            return true;
        }

        NEFORCE_NODISCARD bool is_done() const noexcept { return done_; }

    private:
        enum class state : uint8_t {
            size_line, ///< 等待 chunk-size 行
            data,      ///< 正在读取 chunk 数据
            data_crlf, ///< 等待 chunk 数据后的 CRLF
            trailer,   ///< 正在读取 trailer 段
        };

        void compact() {
            if (offset_ < 8192) {
                return;
            }
            string rest;
            rest.append(buffer_.data() + offset_, buffer_.size() - offset_);
            buffer_ = move(rest);
            offset_ = 0;
        }

        string buffer_;
        size_t offset_{0};
        size_t remaining_{0};
        state state_{state::size_line};
        bool done_{false};
        bool failed_{false};
    };

    bool is_retryable_status(const http_status status) noexcept {
        switch (status) {
            case http_status::S4_REQUEST_TIMEOUT:
            case http_status::S4_TOO_MANY_REQUESTS:
            case http_status::S5_INTERNAL_SERVER_ERROR:
            case http_status::S5_BAD_GATEWAY:
            case http_status::S5_SERVICE_UNAVAILABLE:
            case http_status::S5_GATEWAY_TIMEOUT:
                return true;
            default:
                return false;
        }
    }

    // Only the delta-seconds form of Retry-After is honoured; the HTTP-date form is rare for the
    // rate-limited APIs this targets and would need a full date parser.
    milliseconds parse_retry_after(const http_client_response& response) {
        const string_view value = response.header("Retry-After").trim();
        if (value.empty()) {
            return milliseconds(0);
        }
        try {
            const auto secs = uinteger32::parse(value).value();
            return milliseconds(static_cast<int64_t>(secs) * 1000);
        } catch (...) {
            return milliseconds(0);
        }
    }

    // Resolves the Location header of a redirect into a follow-up request.
    // The scheme has to be carried over:
    // an http -> https redirect would otherwise be replayed as plain HTTP.
    optional<http_client_request> build_redirect_request(const http_client_request& request,
                                                         const http_client_response& response, url base_url) {
        const string_view location = response.header("Location");
        if (location.empty()) {
            return none;
        }

        url new_url;
        if (location.starts_with("http://") || location.starts_with("https://")) {
            new_url = url::parse(location);
        } else if (location.starts_with("/")) {
            new_url = move(base_url);
            new_url.path = location;
        } else {
            new_url = move(base_url);
            const size_t last_slash = new_url.path.find_last_of('/');
            if (last_slash != string::npos) {
                new_url.path = new_url.path.head(last_slash + 1) + location;
            } else {
                new_url.path = "/"_s + location;
            }
        }

        http_client_request follow_up;
        follow_up.host = new_url.host;
        follow_up.port = new_url.port;
        follow_up.scheme = new_url.scheme;
        follow_up.path = new_url.path.empty() ? "/" : new_url.path;
        follow_up.version = request.version;
        follow_up.headers = request.headers;

        const auto status_code = static_cast<uint16_t>(response.status);
        if (status_code == 303) {
            follow_up.method = http_method::GET();
        } else if ((status_code == 301 || status_code == 302) && request.method.is_post()) {
            follow_up.method = http_method::GET();
        } else {
            follow_up.method = request.method;
            follow_up.body = request.body;
        }
        return follow_up;
    }

    class stream_timeout_guard {
    public:
        stream_timeout_guard(tcp_client_base& client, const milliseconds timeout) {
            if (timeout <= milliseconds(0) || !client.is_connected()) {
                return;
            }
            client_ = &client;
            try {
                saved_ = client_->socket().get_receive_timeout();
                client_->socket().set_receive_timeout(timeout);
            } catch (const exception& e) {
                NEFORCE_REPORT_EXCEPTION(e);
                saved_.reset();
            }
        }

        ~stream_timeout_guard() {
            if (client_ == nullptr || !saved_.has_value() || !client_->is_connected()) {
                return;
            }
            client_->socket().set_receive_timeout(*saved_);
        }

        stream_timeout_guard(const stream_timeout_guard&) = delete;
        stream_timeout_guard& operator=(const stream_timeout_guard&) = delete;

    private:
        tcp_client_base* client_{nullptr};
        optional<milliseconds> saved_;
    };

    bool wait_socket_readable(const socket_base& socket, const int timeout_ms) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
        ::fd_set read_set;
        FD_ZERO(&read_set);
        FD_SET(static_cast<::SOCKET>(socket.native_handle()), &read_set);
        ::timeval wait_time{};
        wait_time.tv_sec = timeout_ms / 1000;
        wait_time.tv_usec = static_cast<long>((timeout_ms % 1000) * 1000);
        return ::select(0, &read_set, nullptr, nullptr, &wait_time) > 0;
#else
        ::pollfd descriptor{};
        descriptor.fd = static_cast<int>(socket.native_handle());
        descriptor.events = POLLIN;
        return ::poll(&descriptor, 1, timeout_ms) > 0;
#endif
    }

    // Streams can be aborted from another thread. On Windows a blocking recv is not woken by shutdown(),
    // so plain transports are switched to non-blocking and driven by a short readiness wait instead.
    class stream_nonblocking_guard {
    public:
        stream_nonblocking_guard(tcp_client_base& client, const bool enable) {
            if (!enable || !client.is_connected()) {
                return;
            }
            try {
                client_ = &client;
                saved_ = client_->socket().is_nonblocking();
                client_->socket().set_nonblocking(true);
            } catch (const exception& e) {
                NEFORCE_REPORT_EXCEPTION(e);
                client_ = nullptr;
            }
        }

        ~stream_nonblocking_guard() {
            if (client_ == nullptr) {
                return;
            }
            try {
                if (client_->is_connected()) {
                    client_->socket().set_nonblocking(saved_);
                }
            } catch (const exception& e) {
                NEFORCE_REPORT_EXCEPTION(e);
            }
        }

        stream_nonblocking_guard(const stream_nonblocking_guard&) = delete;
        stream_nonblocking_guard& operator=(const stream_nonblocking_guard&) = delete;

    private:
        tcp_client_base* client_{nullptr};
        bool saved_{false};
    };
} // namespace


void http_stream_handle::abort() noexcept {
    aborted_.store(true);

    const socket_base::native_handle_type handle = handle_.load();
    if (handle == socket_base::invalid_handle) {
        return;
    }
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::shutdown(static_cast<::SOCKET>(handle), SD_BOTH);
#else
    ::shutdown(handle, SHUT_RDWR);
#endif
}

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
        if (!req.headers.contains(key) && key != "Host" && key != "Cookie") {
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
                                                          const string& request_path, const bool expect_body) {

    receive_start = steady_clock::now();

    string response_data;
    vector<char> buffer(config_.buffer_size.bytes());

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

        // Responses to HEAD and 1xx/204/304 carry no body, regardless of what the headers claim.
        // Reading them would block until the keep-alive connection times out.
        const auto status_code = static_cast<uint16_t>(meta.status);
        if (!expect_body || status_code == 204 || status_code == 304 || (status_code >= 100 && status_code < 200)) {
            return meta;
        }

        const size_t body_start = header_end_pos + 4;
        size_t body_received = response_data.size() - body_start;

        if (meta.content_length > 0) {
            if (meta.content_length > config_.max_response_size.bytes()) {
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
                if (response_data.size() >= config_.max_response_size.bytes()) {
                    break;
                }

                const ssize_t n = client_.receive(buffer.data(), buffer.size());
                if (n <= 0) {
                    break;
                }
                response_data.append(buffer.data(), n);
            }
        } else {
            while (response_data.size() < config_.max_response_size.bytes()) {
                const ssize_t n = client_.receive(buffer.data(), buffer.size());
                if (n <= 0) {
                    break;
                }
                response_data.append(buffer.data(), n);
            }
            if (response_data.size() >= config_.max_response_size.bytes()) {
                return none;
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

bool http_client::ensure_connected(const string& host, const ports port, const bool use_ssl) {
    if (client_.is_connected()) {
        if (client_.connected_host() == host && client_.connected_port() == port) {
            const bool expired = config_.keep_alive_idle_timeout > milliseconds(0) && last_activity_ != time_point{} &&
                                 steady_clock::now() - last_activity_ > config_.keep_alive_idle_timeout;
            if (!expired) {
                return true;
            }
        }
        client_.disconnect();
    }

    try {
        if (use_ssl) {
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

bool http_client::prepare_connection(const http_client_request& request) {
    const bool need_ssl = (request.scheme == "https");
    if (need_ssl && !client_.has_ssl_context()) {
        if (client_.is_connected()) {
            client_.disconnect();
        }
        ssl_context ctx(ssl_method::tls_client);
        client_.set_verify_peer(config_.verify_ssl);
        client_.set_ssl_context(move(ctx));
    }
    return ensure_connected(request.host, request.port, need_ssl);
}

milliseconds http_client::retry_backoff(const uint16_t attempt, const http_client_response& response) const {
    const http_retry_policy& policy = config_.retry;

    if (policy.honor_retry_after) {
        const milliseconds hinted = parse_retry_after(response);
        if (hinted > milliseconds(0)) {
            return policy.max_backoff > milliseconds(0) && hinted > policy.max_backoff ? policy.max_backoff : hinted;
        }
    }

    milliseconds delay = policy.initial_backoff;
    const uint16_t multiplier = policy.backoff_multiplier == 0 ? 1 : policy.backoff_multiplier;
    for (uint16_t i = 1; i < attempt; ++i) {
        delay = delay * multiplier;
        if (policy.max_backoff > milliseconds(0) && delay >= policy.max_backoff) {
            return policy.max_backoff;
        }
    }
    if (policy.max_backoff > milliseconds(0) && delay > policy.max_backoff) {
        return policy.max_backoff;
    }
    return delay;
}

http_client_response http_client::do_request(http_client_request request, const int redirect_count) {
    const uint16_t attempts = config_.retry.max_attempts == 0 ? 1 : config_.retry.max_attempts;

    for (uint16_t attempt = 1;; ++attempt) {
        bool transport_failed = false;
        http_client_response response = do_request_once(request, redirect_count, transport_failed);

        if (attempt >= attempts) {
            return response;
        }

        const bool retryable_status = config_.retry.retry_on_status_code && is_retryable_status(response.status);
        const bool retryable_transport = config_.retry.retry_on_transport_error && transport_failed;
        if (!retryable_status && !retryable_transport) {
            return response;
        }

        if (transport_failed && client_.is_connected()) {
            client_.disconnect();
        }

        this_thread::sleep_for_ms(static_cast<uint32_t>(retry_backoff(attempt, response).count()));
    }
}

http_client_response http_client::do_request_once(http_client_request request, const int redirect_count,
                                                  bool& transport_failed) {
    http_client_response response;
    const auto start_time = steady_clock::now();

    transport_failed = false;

    url req_url;
    req_url.scheme = request.scheme;
    req_url.host = request.host;
    req_url.port = request.port;
    req_url.path = request.path;

    const auto connect_start = steady_clock::now();
    if (!prepare_connection(request)) {
        response.status = http_status::S5_INTERNAL_SERVER_ERROR;
        response.status_message = "Connection failed";
        response.effective_url = req_url.to_string();
        transport_failed = true;
        return response;
    }
    response.connect_time = time_cast<milliseconds>(steady_clock::now() - connect_start);

    const string request_str = build_request_str(request, req_url);
    steady_clock::time_point send_start;
    if (!send_request(request_str.view(), send_start)) {
        response.status = http_status::S5_INTERNAL_SERVER_ERROR;
        response.status_message = "Send failed";
        response.effective_url = req_url.to_string();
        transport_failed = true;
        return response;
    }
    response.send_time = time_cast<milliseconds>(steady_clock::now() - send_start);

    steady_clock::time_point receive_start;
    auto resp_opt = read_response(receive_start, request.host, request.path, !request.method.is_head());
    if (!resp_opt) {
        client_.disconnect();
        response.status = http_status::S5_INTERNAL_SERVER_ERROR;
        response.status_message = "Receive/Parse failed";
        response.effective_url = req_url.to_string();
        transport_failed = true;
        return response;
    }

    response = move(*resp_opt);
    response.receive_time = time_cast<milliseconds>(steady_clock::now() - receive_start);
    response.total_time = time_cast<milliseconds>(steady_clock::now() - start_time);
    response.effective_url = req_url.to_string();
    response.redirect_count = redirect_count;
    last_activity_ = steady_clock::now();

    // If the server sent Connection:
    // close, proactively disconnect so the next request reconnects.
    // Otherwise is_connected() returns true (socket fd still open in CLOSE_WAIT)
    // and subsequent send/receive fail with 500.
    if (string(response.header("Connection")).lowercase().contains("close")) {
        client_.disconnect();
    }

    update_cookies(response.cookies, req_url);

    if (config_.follow_redirects && response.is_redirect() &&
        static_cast<uint16_t>(redirect_count) < config_.max_redirects) {
        optional<http_client_request> follow_up = build_redirect_request(request, response, move(req_url));
        if (follow_up) {
            return do_request(move(*follow_up), redirect_count + 1);
        }
    }

    return response;
}

http_client_response http_client::stream_once(const http_client_request& request, const int redirect_count,
                                              const http_stream_observer& observer, http_stream_handle* handle,
                                              bool& transport_failed) {
    http_client_response response;
    const auto start_time = steady_clock::now();
    transport_failed = false;

    url req_url;
    req_url.scheme = request.scheme;
    req_url.host = request.host;
    req_url.port = request.port;
    req_url.path = request.path;

    const auto connect_start = steady_clock::now();
    if (!prepare_connection(request)) {
        response.status = http_status::S5_INTERNAL_SERVER_ERROR;
        response.status_message = "Connection failed";
        response.effective_url = req_url.to_string();
        transport_failed = true;
        return response;
    }
    response.connect_time = time_cast<milliseconds>(steady_clock::now() - connect_start);

    const string request_str = build_request_str(request, req_url);
    steady_clock::time_point send_start;
    if (!send_request(request_str.view(), send_start)) {
        response.status = http_status::S5_INTERNAL_SERVER_ERROR;
        response.status_message = "Send failed";
        response.effective_url = req_url.to_string();
        transport_failed = true;
        return response;
    }
    response.send_time = time_cast<milliseconds>(steady_clock::now() - send_start);

    if (handle != nullptr && client_.is_connected()) {
        try {
            handle->attach(client_.socket().native_handle());
        } catch (const exception& e) {
            NEFORCE_REPORT_EXCEPTION(e);
        }
    }

    // The unbinder has to outlive every return path below, so it is declared before the read loop.
    struct handle_unbinder {
        http_stream_handle* handle_;

        ~handle_unbinder() {
            if (handle_ != nullptr) {
                handle_->detach();
            }
        }
    } unbinder{handle};

    const stream_timeout_guard timeout_guard(client_, config_.stream_idle_timeout);

    const auto aborted_by_caller = [handle] { return handle != nullptr && handle->is_aborted(); };
    const auto emit = [&observer, handle](const string_view data) {
        if (handle != nullptr && handle->is_aborted()) {
            return false;
        }
        if (!observer.on_data) {
            return true;
        }
        if (observer.on_data(data)) {
            return true;
        }
        if (handle != nullptr) {
            handle->abort();
        }
        return false;
    };

    const bool plain_transport = request.scheme != "https";
    const stream_nonblocking_guard nonblocking_guard(client_, plain_transport);
    const auto idle_limit =
            config_.stream_idle_timeout > milliseconds(0) ? config_.stream_idle_timeout : config_.receive_timeout;
    auto last_data_at = steady_clock::now();

    // Returns the received byte count (>0), 0 on orderly close, -1 when the caller aborted,
    // and -2 on transport failure or idle timeout.
    const auto receive_chunk = [&](char* data, const size_t size) -> ssize_t {
        for (;;) {
            if (aborted_by_caller()) {
                return -1;
            }
            if (steady_clock::now() - last_data_at >= idle_limit) {
                return -2;
            }
            if (!plain_transport) {
                try {
                    return client_.receive(data, size);
                } catch (const exception&) {
                    return aborted_by_caller() ? -1 : -2;
                }
            }
            bool readable = false;
            try {
                readable = wait_socket_readable(client_.socket(), 50);
            } catch (const exception&) {
                return -2;
            }
            if (!readable) {
                continue;
            }
            try {
                return client_.receive(data, size);
            } catch (const exception&) {
                return aborted_by_caller() ? -1 : -2;
            }
        }
    };

    const auto receive_start = steady_clock::now();
    vector<char> buffer(config_.buffer_size.bytes());
    const uint64_t max_size = config_.max_response_size.bytes();

    // Read the header block. Whatever follows it in the same read belongs to the body.
    string head;
    size_t header_end = string::npos;
    uint64_t raw_read = 0;
    while (header_end == string::npos) {
        if (aborted_by_caller()) {
            response.status = http_status::S5_INTERNAL_SERVER_ERROR;
            response.status_message = "Aborted";
            response.effective_url = req_url.to_string();
            client_.disconnect();
            return response;
        }

        const ssize_t n = receive_chunk(buffer.data(), buffer.size());
        if (n < 0) {
            response.status = http_status::S5_INTERNAL_SERVER_ERROR;
            response.status_message = n == -1 ? "Aborted" : "Receive/Parse failed";
            response.effective_url = req_url.to_string();
            transport_failed = n != -1;
            client_.disconnect();
            return response;
        }
        if (n == 0) {
            response.status = http_status::S5_INTERNAL_SERVER_ERROR;
            response.status_message = "Receive/Parse failed";
            response.effective_url = req_url.to_string();
            transport_failed = true;
            client_.disconnect();
            return response;
        }

        last_data_at = steady_clock::now();
        raw_read += static_cast<uint64_t>(n);
        head.append(buffer.data(), static_cast<size_t>(n));
        header_end = head.find("\r\n\r\n");

        if (header_end == string::npos && raw_read > max_size) {
            response.status = http_status::S5_INTERNAL_SERVER_ERROR;
            response.status_message = "Response header too large";
            response.effective_url = req_url.to_string();
            transport_failed = true;
            client_.disconnect();
            return response;
        }
    }

    http_client_response meta;
    size_t body_offset = 0;
    if (!parse_response_head(head.view(), meta, request.host, request.path, body_offset)) {
        response.status = http_status::S5_INTERNAL_SERVER_ERROR;
        response.status_message = "Receive/Parse failed";
        response.effective_url = req_url.to_string();
        transport_failed = true;
        client_.disconnect();
        return response;
    }

    string pending = head.view(body_offset);
    head.clear();

    if (observer.on_headers) {
        observer.on_headers(meta);
    }

    const auto status_code = static_cast<uint16_t>(meta.status);
    bool complete = request.method.is_head() || status_code == 204 || status_code == 304 ||
                    (status_code >= 100 && status_code < 200);
    bool aborted = false;

    if (!complete && meta.chunked) {
        chunk_decoder decoder;
        decoder.push(pending.view());
        pending.clear();
        string decoded;

        while (true) {
            if (!decoder.decode(decoded)) {
                transport_failed = true;
                break;
            }
            if (!decoded.empty()) {
                const bool keep_going = emit(decoded.view());
                decoded.clear();
                if (!keep_going) {
                    aborted = true;
                    break;
                }
            }
            if (decoder.is_done()) {
                complete = true;
                break;
            }
            if (aborted_by_caller()) {
                aborted = true;
                break;
            }
            if (raw_read > max_size) {
                transport_failed = true;
                break;
            }

            const ssize_t n = receive_chunk(buffer.data(), buffer.size());
            if (n < 0) {
                if (n == -1) {
                    aborted = true;
                } else {
                    transport_failed = true;
                }
                break;
            }
            if (n == 0) {
                transport_failed = true;
                break;
            }

            last_data_at = steady_clock::now();
            raw_read += static_cast<uint64_t>(n);
            decoder.push(string_view(buffer.data(), static_cast<size_t>(n)));
        }
    } else if (!complete && meta.content_length > 0) {
        uint64_t remaining = meta.content_length;

        while (true) {
            if (!pending.empty()) {
                const size_t take = min<size_t>(pending.size(), static_cast<size_t>(remaining));
                if (take > 0) {
                    const bool keep_going = emit(pending.view(0, take));
                    remaining -= take;
                    pending.clear();
                    if (!keep_going) {
                        aborted = true;
                        break;
                    }
                } else {
                    pending.clear();
                }
                if (remaining == 0) {
                    complete = true;
                    break;
                }
            }
            if (aborted_by_caller()) {
                aborted = true;
                break;
            }
            if (raw_read > max_size) {
                transport_failed = true;
                break;
            }

            const ssize_t n = receive_chunk(buffer.data(), buffer.size());
            if (n < 0) {
                if (n == -1) {
                    aborted = true;
                } else {
                    transport_failed = true;
                }
                break;
            }
            if (n == 0) {
                transport_failed = true;
                break;
            }

            last_data_at = steady_clock::now();
            raw_read += static_cast<uint64_t>(n);
            pending.assign(buffer.data(), static_cast<size_t>(n));
        }
    } else if (!complete) {
        // No framing information: the body ends when the peer closes the connection.
        while (true) {
            if (!pending.empty()) {
                const bool keep_going = emit(pending.view());
                pending.clear();
                if (!keep_going) {
                    aborted = true;
                    break;
                }
            }
            if (aborted_by_caller()) {
                aborted = true;
                break;
            }
            if (raw_read > max_size) {
                transport_failed = true;
                break;
            }

            const ssize_t n = receive_chunk(buffer.data(), buffer.size());
            if (n < 0) {
                if (n == -1) {
                    aborted = true;
                } else {
                    transport_failed = true;
                }
                break;
            }
            if (n == 0) {
                complete = true;
                break;
            }

            last_data_at = steady_clock::now();
            raw_read += static_cast<uint64_t>(n);
            pending.assign(buffer.data(), static_cast<size_t>(n));
        }
    }

    response = move(meta);
    response.receive_time = time_cast<milliseconds>(steady_clock::now() - receive_start);
    response.total_time = time_cast<milliseconds>(steady_clock::now() - start_time);
    response.effective_url = req_url.to_string();
    response.redirect_count = redirect_count;

    // A stream that was cut short, cancelled, or failed mid-body leaves the connection in an unknown state,
    // so it must never go back into the keep-alive pool.
    if (aborted || transport_failed || !complete ||
        string(response.header("Connection")).lowercase().contains("close")) {
        client_.disconnect();
    } else {
        last_activity_ = steady_clock::now();
    }

    update_cookies(response.cookies, req_url);

    if (!aborted && !transport_failed && raw_read == 0 && config_.follow_redirects && response.is_redirect() &&
        static_cast<uint16_t>(redirect_count) < config_.max_redirects) {
        optional<http_client_request> follow_up = build_redirect_request(request, response, move(req_url));
        if (follow_up) {
            return do_request_stream(*follow_up, observer, handle, redirect_count + 1);
        }
    }

    return response;
}

http_client_response http_client::do_request_stream(const http_client_request& request,
                                                    const http_stream_observer& observer, http_stream_handle* handle,
                                                    const int redirect_count) {
    const uint16_t attempts = config_.retry.max_attempts == 0 ? 1 : config_.retry.max_attempts;

    for (uint16_t attempt = 1;; ++attempt) {
        // Tracks whether any body bytes reached the caller:
        // retrying after the caller has already seen part of a response would duplicate content on screen.
        bool emitted = false;
        http_stream_observer counting = observer;
        counting.on_data = [&observer, &emitted](const string_view data) {
            emitted = true;
            return observer.on_data ? observer.on_data(data) : true;
        };

        bool transport_failed = false;
        http_client_response response = stream_once(request, redirect_count, counting, handle, transport_failed);

        const bool cancelled = handle != nullptr && handle->is_aborted();
        const bool retryable_status = config_.retry.retry_on_status_code && is_retryable_status(response.status);
        const bool retryable_transport = config_.retry.retry_on_transport_error && transport_failed;

        if (attempt >= attempts || emitted || cancelled || (!retryable_status && !retryable_transport)) {
            return response;
        }

        if (client_.is_connected()) {
            client_.disconnect();
        }
        this_thread::sleep_for_ms(static_cast<uint32_t>(retry_backoff(attempt, response).count()));
    }
}

http_client_response http_client::request_stream(const http_client_request& req, const http_stream_observer& observer,
                                                 http_stream_handle* handle) {
    if (!observer.valid()) {
        NEFORCE_THROW_EXCEPTION(value_exception("http_stream_observer::on_data is required"));
    }
    if (handle != nullptr) {
        handle->reset();
    }
    return do_request_stream(req, observer, handle, 0);
}

future<http_client_response> http_client::request_stream_async(http_client_request req,
                                                               const http_stream_observer& observer,
                                                               http_stream_handle* handle) {
    auto prom = make_shared<promise<http_client_response>>();
    auto result = prom->get_future();
    ctx_->post([this, req = move(req), observer, handle, prom = move(prom)]() mutable {
        try {
            prom->set_value(request_stream(req, observer, handle));
        } catch (...) {
            prom->set_exception(current_exception());
        }
    });
    return result;
}

void http_client::update_cookies(const vector<http_cookie>& resp_cookies, const url& request_url) {
    lock<mutex> lk(mutex_);

    for (const auto& c: resp_cookies) {
        string domain = c.domain.empty() ? request_url.host : c.domain;
        string path = c.path;
        string key = c.name.cookie_name() + "@" + domain + path;

        const bool should_delete =
                (c.max_age == 0_s) || (c.max_age > 0_s && c.is_valid() && c.expires.is_valid() && c.is_expired());

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
        if (c.max_age == 0_s) {
            continue;
        }
        if (c.max_age > 0_s && c.is_valid() && c.expires.is_valid() && c.is_expired()) {
            continue;
        }

        bool domain_match = false;
        if (c.domain.empty() || request_url.host == c.domain) {
            domain_match = true;
        } else if (c.domain.starts_with(".")) {
            const string_view domain_without_dot = c.domain.view(1);
            if (request_url.host == domain_without_dot) {
                domain_match = true;
            } else if (request_url.host.size() > domain_without_dot.size() &&
                       request_url.host[request_url.host.size() - domain_without_dot.size() - 1] == '.') {
                domain_match = request_url.host.ends_with(domain_without_dot);
            }
        }

        if (!domain_match) {
            continue;
        }
        if (!request_url.path.starts_with(c.path.view())) {
            continue;
        }
        if (!c.path.ends_with("/") && request_url.path.length() > c.path.length()) {
            if (request_url.path[c.path.length()] != '/') {
                continue;
            }
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

http_client::http_client(io_context& ioc, config cfg) :
client_(ioc),
ctx_(&ioc),
config_(move(cfg)) {
    persistent_headers_["User-Agent"] = config_.user_agent;
    persistent_headers_["Accept"] = "*/*";

    client_.set_connect_timeout(config_.connect_timeout);
    client_.set_send_timeout(config_.send_timeout);
    client_.set_recv_timeout(config_.receive_timeout);
}

http_client::http_client(io_context& ioc, ssl_context ctx, config cfg) :
client_(ioc, move(ctx)),
ctx_(&ioc),
config_(move(cfg)) {
    persistent_headers_["User-Agent"] = config_.user_agent;
    persistent_headers_["Accept"] = "*/*";

    client_.set_connect_timeout(config_.connect_timeout);
    client_.set_send_timeout(config_.send_timeout);
    client_.set_recv_timeout(config_.receive_timeout);
    client_.set_verify_peer(config_.verify_ssl);
}

void http_client::set_ssl_context(ssl_context ctx) { client_.set_ssl_context(move(ctx)); }

void http_client::set_verify_ssl(const bool verify) { config_.verify_ssl = verify; }

http_client_response http_client::get(const string& url, const flat_unordered_map<string, string>& headers) {
    _NEFORCE url parsed_url{url::parse(url.view())};

    http_client_request req;
    req.host = parsed_url.host;
    req.scheme = parsed_url.scheme;
    req.port = parsed_url.port;
    req.method = http_method::GET();
    req.path = parsed_url.path.empty() ? "/" : parsed_url.path;

    if (!parsed_url.query.empty()) {
        req.path += "?" + parsed_url.query;
    }
    req.headers = headers;

    return request(move(req));
}

http_client_response http_client::post(const string& url, const string& body, const string& content_type,
                                       const flat_unordered_map<string, string>& headers) {

    _NEFORCE url parsed_url(url::parse(url.view()));

    http_client_request req;
    req.host = parsed_url.host;
    req.scheme = parsed_url.scheme;
    req.port = parsed_url.port;
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
                                            const flat_unordered_map<string, string>& headers) {

    return post(url_str, json_body, "application/json", headers);
}

http_client_response http_client::post_form(const string& url_str, const flat_unordered_map<string, string>& form_data,
                                            const flat_unordered_map<string, string>& headers) {
    const string body = url::build_query(form_data);
    return post(url_str, body, "application/x-www-form-urlencoded", headers);
}

http_client_response http_client::put(const string& url, const string& body, const string& content_type,
                                      const flat_unordered_map<string, string>& headers) {

    _NEFORCE url parsed_url(url::parse(url.view()));

    http_client_request req;
    req.host = parsed_url.host;
    req.scheme = parsed_url.scheme;
    req.port = parsed_url.port;
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

http_client_response http_client::del(const string& url, const flat_unordered_map<string, string>& headers) {
    _NEFORCE url parsed_url(url::parse(url.view()));

    http_client_request req;
    req.host = parsed_url.host;
    req.scheme = parsed_url.scheme;
    req.port = parsed_url.port;
    req.method = http_method::DELETE();
    req.path = parsed_url.path.empty() ? "/" : parsed_url.path;

    if (!parsed_url.query.empty()) {
        req.path += "?" + parsed_url.query;
    }

    req.headers = headers;

    return request(move(req));
}

http_client_response http_client::head(const string& url, const flat_unordered_map<string, string>& headers) {
    _NEFORCE url parsed_url(url::parse(url.view()));

    http_client_request req;
    req.host = parsed_url.host;
    req.scheme = parsed_url.scheme;
    req.port = parsed_url.port;
    req.method = http_method::HEAD();
    req.path = parsed_url.path.empty() ? "/" : parsed_url.path;

    if (!parsed_url.query.empty()) {
        req.path += "?" + parsed_url.query;
    }

    req.headers = headers;

    return request(move(req));
}

http_client_response http_client::options(const string& url, const flat_unordered_map<string, string>& headers) {
    _NEFORCE url parsed_url(url::parse(url.view()));

    http_client_request req;
    req.host = parsed_url.host;
    req.scheme = parsed_url.scheme;
    req.port = parsed_url.port;
    req.method = http_method::OPTIONS();
    req.path = parsed_url.path.empty() ? "/" : parsed_url.path;

    if (!parsed_url.query.empty()) {
        req.path += "?" + parsed_url.query;
    }

    req.headers = headers;

    return request(move(req));
}

http_client_response http_client::patch(const string& url, const string& body, const string& content_type,
                                        const flat_unordered_map<string, string>& headers) {

    _NEFORCE url parsed_url(url::parse(url.view()));

    http_client_request req;
    req.host = parsed_url.host;
    req.scheme = parsed_url.scheme;
    req.port = parsed_url.port;
    req.method = http_method::PATCH();
    req.path = parsed_url.path.empty() ? "/" : parsed_url.path;

    if (!parsed_url.query.empty()) {
        req.path += "?" + parsed_url.query;
    }

    req.headers = headers;
    req.headers["Content-Type"] = content_type;
    req.body = body;

    return request(move(req));
}

bool http_client::download_file(const string& url, path output, const bool is_binary) {
    try {
        auto response = get(url);

        if (!response.is_success()) {
            return false;
        }

        file f(move(output), false, file_access::read_write, file_shared::share_read, file_creation::create_force);

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
    auto prom = make_shared<promise<http_client_response>>();
    auto result = prom->get_future();
    ctx_->post([this, req = move(req), prom = move(prom)]() mutable {
        try {
            auto resp = request(move(req));
            prom->set_value(move(resp));
        } catch (...) {
            prom->set_exception(current_exception());
        }
    });
    return result;
}

#ifdef NEFORCE_STANDARD_20
awaitable<http_client_response> http_client::request_async(http_client_request req, use_awaitable_t /*unused*/) {
    async_result<use_awaitable_t, void(http_client_response)> result(use_awaitable);
    auto handler = result.get_handler();
    auto state = result.awaitable_;
    ctx_->post([this, req = move(req), handler = move(handler), state = move(state)]() mutable {
        try {
            handler(request(move(req)));
        } catch (...) {
            state->set_exception(current_exception());
        }
    });
    return result.get();
}
#endif

void http_client::close() { client_.disconnect(); }

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
