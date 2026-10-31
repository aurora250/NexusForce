#include <NeForce/core/algorithm/remove.hpp>
#include <NeForce/core/encrypt/base64.hpp>
#include <NeForce/core/encrypt/sha1.hpp>
#include <NeForce/core/numeric/random.hpp>
#include <NeForce/core/utility/hexadecimal.hpp>
#include <NeForce/core/utility/scope.hpp>
#include <NeForce/core/container/deque.hpp>
#include <NeForce/network/http/http_server.hpp>
#include <NeForce/network/http/http2_connection.hpp>
#include <NeForce/network/ssl/ssl_socket.hpp>
#include <NeForce/network/tcp/tcp_client.hpp>
#include <NeForce/network/util/url.hpp>
#ifdef NEFORCE_PLATFORM_POSIX
#    include <poll.h>
#    include <sys/uio.h>
#    include <cerrno>
#endif
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

namespace {
    constexpr int max_forward_count = 5;

    NEFORCE_NODISCARD bool ascii_iequal(const string_view lhs, const string_view rhs) noexcept {
        if (lhs.size() != rhs.size()) {
            return false;
        }
        for (size_t i = 0; i < lhs.size(); ++i) {
            const char a = lhs[i] >= 'A' && lhs[i] <= 'Z' ? static_cast<char>(lhs[i] + 32) : lhs[i];
            const char b = rhs[i] >= 'A' && rhs[i] <= 'Z' ? static_cast<char>(rhs[i] + 32) : rhs[i];
            if (a != b) {
                return false;
            }
        }
        return true;
    }

    NEFORCE_NODISCARD bool ascii_icontains(string_view haystack, string_view needle) noexcept;

    NEFORCE_NODISCARD bool has_complete_request(const string& pending, const size_t max_header_size) {
        const size_t header_end = pending.find("\r\n\r\n");
        if (header_end == string::npos) {
            return false;
        }
        if (header_end + 4 > max_header_size) {
            return true;
        }
        try {
            const http_request header = http_request::parse(pending.view(0, header_end + 4));
            const string_view cl = header.header_ci("Content-Length");
            size_t content_length = 0;
            if (!cl.empty()) {
                content_length = uinteger64::parse(cl).value();
            }
            const string_view te = header.header_ci("Transfer-Encoding");
            if (!te.empty() && ascii_icontains(te, "chunked")) {
                return pending.find("\r\n0\r\n\r\n", header_end + 4) != string::npos ||
                       pending.view(header_end + 4).starts_with("0\r\n\r\n");
            }
            return pending.size() >= header_end + 4 + content_length;
        } catch (...) {
            return true;
        }
    }

    NEFORCE_NODISCARD bool ascii_icontains(const string_view haystack, const string_view needle) noexcept {
        if (needle.size() > haystack.size()) {
            return false;
        }
        for (size_t i = 0; i + needle.size() <= haystack.size(); ++i) {
            if (ascii_iequal(haystack.view(i, needle.size()), needle)) {
                return true;
            }
        }
        return false;
    }

    NEFORCE_NORETURN void throw_status_error(const http_status status, const char* message) {
        NEFORCE_THROW_EXCEPTION(http_exception(message, status));
    }

    string compute_websocket_accept(const string_view key) {
        constexpr string_view websocket_guid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
        const string combined = string(key) + websocket_guid;
        const string sha1_result = sha1(combined);
        return base64_encode(cbyte_view{reinterpret_cast<const byte_t*>(sha1_result.data()), sha1_result.size()});
    }

    string decode_chunked_body(const string_view chunked_data,
                               flat_unordered_map<string, string>* trailers_out = nullptr) {
        string decoded;
        size_t pos = 0;

        while (pos < chunked_data.size()) {
            const size_t line_end = chunked_data.find("\r\n", pos);
            if (line_end == string::npos) {
                NEFORCE_THROW_EXCEPTION(http_exception("Malformed chunked body: missing chunk size line"));
            }
            string_view size_line = chunked_data.view(pos, line_end - pos).trim();
            const size_t semi = size_line.find(';');
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
                        const size_t trailer_end = chunked_data.find("\r\n", pos);
                        if (trailer_end == string::npos || trailer_end == pos) {
                            if (trailer_end == pos) {
                                pos = trailer_end + 2;
                            }
                            break;
                        }
                        string_view trailer_line = chunked_data.view(pos, trailer_end - pos);
                        const size_t colon = trailer_line.find(':');
                        if (colon != string_view::npos) {
                            string key(trailer_line.view(0, colon).trim());
                            string val(trailer_line.view(colon + 1).trim());
                            (*trailers_out)[move(key)] = move(val);
                        }
                        pos = trailer_end + 2;
                    }
                } else {
                    while (pos < chunked_data.size()) {
                        const size_t trailer_end = chunked_data.find("\r\n", pos);
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


    void send_response(tcp_socket* client_socket, const http_response& response, const bool include_body = true) {
        if (!response.raw_response.empty()) {
            client_socket->send_all(
                    memory_view<const char>(response.raw_response.data(), response.raw_response.size()));
            return;
        }

        const bool write_body = include_body && response.redirect_url.empty() && response.allows_body();

        if (client_socket->is_ssl()) {
            // SSL_write needs one contiguous buffer, so serialize header and body once.
            const string tls_data = include_body ? response.to_string() : response.build_header_string();
            client_socket->send_all(memory_view<const char>(tls_data.data(), tls_data.size()));
            return;
        }

        const string header = response.build_header_string();

#ifdef NEFORCE_PLATFORM_LINUX
        if (!client_socket->is_ssl() && write_body && !response.body_view().empty()) {
            ::iovec iov[2];
            iov[0].iov_base = const_cast<char*>(header.data());
            iov[0].iov_len = header.size();
            iov[1].iov_base = const_cast<char*>(response.body_view().data());
            iov[1].iov_len = response.body_view().size();

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

        string data = header;
        if (write_body) {
            data += response.body_view();
        }
        client_socket->send_all(memory_view<const char>(data.data(), data.size()));
    }

    void add_session_cookie(const http_request& request, http_response& response, http_session* session,
                            const bool created, const http_cookie_name& name) {

        if (session == nullptr || (!created && !session->is_new_session())) {
            return;
        }

        http_cookie session_cookie;
        session_cookie.name = name;
        session_cookie.value = session->session_id();
        session_cookie.http_only = true;

        const bool is_https = request.header(http_key::X_Forwarded_Proto()) == "https";
        session_cookie.secure = is_https;
        session_cookie.same_site = is_https ? http_key::Strict() : http_key::Lax();

        response.cookies.emplace_back(move(session_cookie));
        session->touch();
    }

    void send_error_response(tcp_socket* client_socket, const http_status status, const string& message) {
        try {
            http_response error_response;
            error_response.status = status;
            error_response.status_message = http_status_message(status);
            error_response.set_header(http_key::Connection(), "close");
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
        } catch (const exception& e) {
            NEFORCE_REPORT_EXCEPTION(e);
        }
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
    {
        shared_lock<shared_mutex> lock(mutex_);
        const auto session_iter = sessions_.find(session_id);
        if (session_iter != sessions_.end() && session_iter->second.is_valid()) {
            session_iter->second.touch();
            return &session_iter->second;
        }
    }

    if (!create) {
        return nullptr;
    }

    unique_lock<shared_mutex> lock(mutex_);
    const auto session_iter = sessions_.find(session_id);
    if (session_iter != sessions_.end()) {
        if (session_iter->second.is_valid()) {
            session_iter->second.touch();
            return &session_iter->second;
        }
        sessions_.erase(session_iter);
    }

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

void http_server::session_manager::remove_session(const string& session_id) noexcept {
    unique_lock<shared_mutex> lock(mutex_);
    sessions_.erase(session_id);
}

void http_server::session_manager::cleanup_expired_sessions() {
    while (cleanup_running_) {
        {
            unique_lock<shared_mutex> lock(mutex_);
            for (auto it = sessions_.begin(); it != sessions_.end();) {
                if (!it->second.is_valid() || it->second.expired()) {
                    it = sessions_.erase(it);
                } else {
                    ++it;
                }
            }
        }
        unique_lock<mutex> lk(cleanup_mutex_);
        cv_.wait_for(lk, cleanup_interval_, [&] { return !cleanup_running_.load(); });
    }
}

bool http_server::session_manager::session_exists(const string& session_id) const noexcept {
    shared_lock<shared_mutex> lk(mutex_);
    const auto it = sessions_.find(session_id);
    return it != sessions_.end() && it->second.is_valid();
}

size_t http_server::session_manager::session_count() const noexcept {
    shared_lock<shared_mutex> lk(mutex_);
    return sessions_.size();
}

void http_server::session_manager::set_cleanup_interval(const seconds interval) noexcept {
    cleanup_interval_ = interval;
}

void http_server::session_manager::set_max_sessions(const size_t max) noexcept { max_sessions_ = max; }

http_request http_server::parse_request(tcp_socket* client_socket, session_manager& manager,
                                        const http_cookie_name& name, const byte_size max_header_size,
                                        const byte_size max_body_size, const size_t max_header_count,
                                        const milliseconds body_read_timeout, string& pending) {

    const auto original_timeout = client_socket->get_receive_timeout();

    if (body_read_timeout.count() > 0) {
        client_socket->set_receive_timeout(body_read_timeout);
    }

    scope_exit restore_timeout([&] {
        if (original_timeout.has_value()) {
            client_socket->set_receive_timeout(original_timeout.value());
        }
    });

    char buffer[8192];

    for (;;) {
        const size_t header_end = pending.find("\r\n\r\n");
        if (header_end == string::npos) {
            if (pending.size() > max_header_size.bytes()) {
                throw_status_error(http_status::header_fields_too_large, "Request header too large");
            }
            const ssize_t bytes_read = client_socket->receive(memory_view<char>(buffer));
            if (bytes_read <= 0) {
                NEFORCE_THROW_EXCEPTION(http_exception("Connection closed while reading request"));
            }
            pending.append(buffer, bytes_read);
            continue;
        }

        if (header_end + 4 > max_header_size.bytes()) {
            throw_status_error(http_status::header_fields_too_large, "Request header too large");
        }

        // Parse the request line and headers once from the receive buffer, then
        // consume exactly one request so pipelined requests stay buffered.
        http_request request = http_request::parse(pending.view(0, header_end + 4));

        if (request.version != "HTTP/1.1" && request.version != "HTTP/1.0") {
            throw_status_error(http_status::http_version_not_supported, "HTTP version not supported");
        }

        if (max_header_count > 0 && request.headers.size() > max_header_count) {
            throw_status_error(http_status::header_fields_too_large, "Too many request headers");
        }

        bool expect_continue = false;
        const string_view expect_value = request.header_ci("Expect");
        if (!expect_value.empty()) {
            if (!ascii_iequal(expect_value, "100-continue")) {
                throw_status_error(http_status::expectation_failed, "Unsupported Expect header");
            }
            expect_continue = true;
        }

        size_t content_length = 0;
        const string_view content_length_value = request.header_ci("Content-Length");
        if (!content_length_value.empty()) {
            try {
                content_length = uinteger64::parse(content_length_value).value();
            } catch (...) {
                throw_status_error(http_status::bad_request, "Invalid Content-Length");
            }
        }

        const string_view transfer_encoding = request.header_ci("Transfer-Encoding");
        const bool is_chunked = !transfer_encoding.empty() && ascii_icontains(transfer_encoding, "chunked");
        const size_t body_start = header_end + 4;
        size_t consumed = 0;

        if (expect_continue && (content_length > 0 || is_chunked) && content_length <= max_body_size.bytes() &&
            request.version == "HTTP/1.1") {
            http_response continue_response;
            continue_response.version = "HTTP/1.1";
            continue_response.status = http_status::continue_;
            continue_response.headers.clear();
            const string continue_head = continue_response.build_header_string();
            client_socket->send_all(memory_view<const char>(continue_head.data(), continue_head.size()));
        }

        if (is_chunked) {
            if (pending.size() - body_start > max_body_size.bytes()) {
                throw_status_error(http_status::payload_too_large, "Request body too large");
            }
            while (pending.find("\r\n0\r\n\r\n", body_start) == string::npos &&
                   pending.view(body_start) != "0\r\n\r\n") {
                if (pending.size() > max_body_size.bytes()) {
                    throw_status_error(http_status::payload_too_large, "Request body too large");
                }
                const ssize_t n = client_socket->receive(memory_view<char>(buffer));
                if (n <= 0) {
                    NEFORCE_THROW_EXCEPTION(http_exception("Connection closed while reading chunked body"));
                }
                pending.append(buffer, n);
            }
            const size_t terminator = pending.find("\r\n0\r\n\r\n", body_start);
            if (terminator != string::npos) {
                consumed = terminator + 7;
            } else if (pending.view(body_start).starts_with("0\r\n\r\n")) {
                consumed = body_start + 5;
            } else {
                consumed = pending.size();
            }
            request.body = decode_chunked_body(pending.view(body_start));
        } else {
            if (content_length > max_body_size.bytes()) {
                throw_status_error(http_status::payload_too_large, "Request body too large");
            }

            const size_t body_received = pending.size() - body_start;
            if (body_received < content_length) {
                size_t remaining = content_length - body_received;
                pending.reserve(body_start + content_length);

                while (remaining > 0) {
                    const size_t to_read = (remaining < sizeof(buffer)) ? remaining : sizeof(buffer);
                    const ssize_t n = client_socket->receive(memory_view<char>(static_cast<char*>(buffer), to_read));

                    if (n <= 0) {
                        NEFORCE_THROW_EXCEPTION(http_exception("Connection closed while reading body"));
                    }

                    pending.append(static_cast<char*>(buffer), n);
                    remaining -= n;
                }
            }
            if (content_length > 0) {
                request.body.assign(pending.view(body_start, content_length));
            }
            consumed = body_start + content_length;
        }

        pending.erase(0, consumed);
        parse_parameters(request);

        const string& session_id = request.cookie(name.cookie_name());
        if (!session_id.empty()) {
            request.session = manager.get_session(session_id, false);
        }

        if (request.session.materialized() == nullptr) {
            string lazy_session_id(session_id);
            request.set_session_resolver(
                    [&manager, id = move(lazy_session_id)]() { return manager.get_session(id, true); });
        }

        return request;
    }
}


namespace {
    void erase_header_ci(flat_unordered_map<string, string>& headers, const string_view name) {
        for (auto it = headers.begin(); it != headers.end();) {
            if (it->first.compare_ignore_case(name) == 0) {
                it = headers.erase(it);
            } else {
                ++it;
            }
        }
    }
} // namespace

class async_http1_session final : public enable_shared_from_this<async_http1_session> {
public:
    async_http1_session(http_server& server, unique_ptr<tcp_socket> socket, io_context& ctx) :
    server_(server),
    socket_(move(socket)),
    ctx_(&ctx) {
        read_buffer_.resize(16UL * 1024UL);
    }

    void start() {
        auto self = shared_from_this();
        post_event([self] { self->schedule_read(); });
    }

private:
    void post_event(function<void()> event) {
        bool dispatch = false;
        {
            lock<mutex> lk(event_mutex_);
            events_.push_back(move(event));
            if (!dispatching_) {
                dispatching_ = true;
                dispatch = true;
            }
        }
        if (!dispatch) {
            return;
        }

        for (;;) {
            function<void()> next;
            {
                lock<mutex> lk(event_mutex_);
                if (events_.empty()) {
                    dispatching_ = false;
                    return;
                }
                next = move(events_.front());
                events_.pop_front();
            }
            try {
                next();
                // NOLINTNEXTLINE(bugprone-empty-catch)
            } catch (const exception& e) {
                NEFORCE_REPORT_EXCEPTION(e);
            } catch (...) {
                NEFORCE_REPORT_EXCEPTION(value_exception("unknown exception in async HTTP/1 event"));
            }
        }
    }

    NEFORCE_NODISCARD bool request_ready() const {
        return has_complete_request(pending_, server_.max_server_header_size.bytes());
    }

    void schedule_read() {
        if (in_dispatch_) {
            need_read_ = true;
            return;
        }
        in_dispatch_ = true;
        do {
            need_read_ = false;
            read_once();
        } while (need_read_);
        in_dispatch_ = false;
    }

    void read_once() {
        if (!keep_alive_ || socket_ == nullptr) {
            close();
            return;
        }
        if (server_.max_keep_alive_requests > 0 && processed_ >= server_.max_keep_alive_requests) {
            close();
            return;
        }
        if (request_ready()) {
            process_next();
            return;
        }
        if (pending_.size() > server_.max_server_header_size.bytes() + server_.max_server_body_size.bytes()) {
            send_error(http_status::payload_too_large, "Request too large");
            return;
        }

        auto self = shared_from_this();
        socket_->async_read(*ctx_, memory_view<char>(read_buffer_.data(), read_buffer_.size()),
                            [self](error_code ec, size_t bytes) {
                                self->post_event([self, ec, bytes] {
                                    if (ec || bytes == 0) {
                                        self->close();
                                        return;
                                    }
                                    self->pending_.append(self->read_buffer_.data(), bytes);
                                    self->schedule_read();
                                });
                            });
    }

    void process_next() {
        http_request request;
        try {
            request = http_server::parse_request(socket_.get(), server_.session_manager_, server_.cookie_name_,
                                                 server_.max_server_header_size, server_.max_server_body_size,
                                                 server_.max_header_count, server_.body_read_timeout, pending_);
        } catch (const http_exception& e) {
            const auto code =
                    e.code().value() == 0 ? http_status::bad_request : static_cast<http_status>(e.code().value());
            send_error(code, e.what());
            return;
        } catch (const exception& e) {
            send_error(http_status::internal_server_error, e.what());
            return;
        } catch (...) {
            send_error(http_status::internal_server_error, "Unknown internal error");
            return;
        }

        if (socket_ != nullptr && socket_->is_ssl()) {
            request.set_header(http_key::X_Forwarded_Proto(), "https");
        }

        if (request.method.is_connect()) {
            send_error(http_status::not_implemented, "CONNECT is not supported in async HTTP/1 mode");
            return;
        }
        if (!request.header("Upgrade").empty()) {
            send_error(http_status::not_implemented, "Protocol upgrades are not supported in async HTTP/1 mode");
            return;
        }

        const bool keep = request.is_keep_alive();
        const bool head = request.method.is_head();
        const string version = request.version;

        auto self = shared_from_this();
        server_.handle_request_with_forward_async(
                move(request),
                // NOLINTNEXTLINE(bugprone-exception-escape)
                [self, keep, head, version](http_request& /*request*/, http_response response) mutable {
                    // NOLINTNEXTLINE(bugprone-exception-escape)
                    self->post_event([self, keep, head, version, response = move(response)]() mutable {
                        const bool alive = keep && !response.close_after_send;
                        self->send_response(move(response), !head, !alive, version);
                    });
                });
    }

    void send_error(const http_status status, const string& message) {
        try {
            http_response response;
            response.status = status;
            response.status_message = http_status_message(status);
            response.set_content_type(http_content::HTML_TEXT());
            response.body = "<!DOCTYPE html><html><head><title>Error</title></head><body><h1>" +
                            response.status_message + "</h1><p>" + message + "</p></body></html>";
            keep_alive_ = false;
            send_response(move(response), true, true, "HTTP/1.1");
            // NOLINTNEXTLINE(bugprone-empty-catch)
        } catch (const exception& e) {
            NEFORCE_REPORT_EXCEPTION(e);
            close();
        }
    }

    void send_response(http_response response, const bool include_body, const bool close_after, const string& version) {
        if (socket_ == nullptr) {
            return;
        }

        if (response.raw_response.empty()) {
            response.version = version;
            erase_header_ci(response.headers, "connection");
            if (close_after) {
                response.set_header(http_key::Connection(), "close");
            } else if (version.starts_with("HTTP/1.0")) {
                response.set_header(http_key::Connection(), "keep-alive");
            }
            write_data_ = include_body ? response.to_string() : response.build_header_string();
        } else {
            write_data_ = move(response.raw_response);
        }
        write_offset_ = 0;
        write_close_ = close_after;
        write_next();
    }

    void write_next() {
        if (socket_ == nullptr) {
            return;
        }
        if (write_offset_ >= write_data_.size()) {
            on_write_complete();
            return;
        }

        auto self = shared_from_this();
        socket_->async_write(
                *ctx_, memory_view<const char>(write_data_.data() + write_offset_, write_data_.size() - write_offset_),
                [self](error_code ec, size_t bytes) {
                    self->post_event([self, ec, bytes] {
                        if (ec || bytes == 0) {
                            self->close();
                            return;
                        }
                        self->write_offset_ += bytes;
                        if (self->write_offset_ < self->write_data_.size()) {
                            self->write_next();
                        } else {
                            self->on_write_complete();
                        }
                    });
                });
    }

    void on_write_complete() {
        if (write_close_) {
            close();
            return;
        }
        ++processed_;
        if (server_.max_keep_alive_requests > 0 && processed_ >= server_.max_keep_alive_requests) {
            close();
            return;
        }
        schedule_read();
    }

    void close() {
        keep_alive_ = false;
        if (socket_) {
            try {
                ctx_->remove_fd(socket_->native_handle());
                // NOLINTNEXTLINE(bugprone-empty-catch)
            } catch (const exception&) {
            }
            socket_.reset();
        }
        pending_.clear();
    }

    http_server& server_;
    unique_ptr<tcp_socket> socket_;
    io_context* ctx_;
    vector<char> read_buffer_;
    string pending_;
    string write_data_;
    size_t write_offset_{0};
    size_t processed_{0};
    bool write_close_{false};
    bool keep_alive_{true};
    bool in_dispatch_{false};
    bool need_read_{false};
    mutex event_mutex_;
    deque<function<void()>> events_;
    bool dispatching_{false};
};

void http_server::handle_client(unique_ptr<tcp_socket> client_socket) {
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
        const auto it = conn_per_ip_.find(client_ip);
        if (it != conn_per_ip_.end() && --it->second == 0) {
            conn_per_ip_.erase(it);
        }
    });

    // ALPN h2 detection: If the TLS handshake is agreed to h2, the HTTP/2 connection is directly initiated
    if (client_socket->is_ssl()) {
        const auto* ssl_sock = dynamic_cast<ssl_socket*>(client_socket.get());
        if (ssl_sock != nullptr) {
            const string alpn = ssl_sock->get_alpn_negotiated();
            if (alpn == "h2") {
                auto conn = make_shared<http2_connection>(move(client_socket), *ctx_);
                conn->set_router(&router_);
                conn->set_max_concurrent_streams(h2_max_concurrent_streams);
                const weak_ptr<http2_connection> weak_conn = conn;
                conn->set_close_handler([this, weak_conn](uint32_t) {
                    if (const auto closing = weak_conn.lock()) {
                        lock<mutex> lk(h2c_mutex_);
                        h2_connections_.erase(_NEFORCE remove(h2_connections_.begin(), h2_connections_.end(), closing),
                                              h2_connections_.end());
                    }
                });
                conn->start();
                {
                    lock<mutex> lk(h2c_mutex_);
                    h2_connections_.push_back(move(conn));
                }
                return;
            }
        }
    }

    if (enable_async_http1 && max_connections_per_ip == 0 && !client_socket->is_ssl()) {
        handle_client_async(unique_ptr<tcp_socket>(client_socket.release()));
        return;
    }

    bool keep_alive = true;
    string pending;
    size_t processed = 0;

    while (keep_alive && (max_keep_alive_requests == 0 || processed < max_keep_alive_requests)) {
        try {
            http_request request =
                    parse_request(client_socket.get(), session_manager_, cookie_name_, max_server_header_size,
                                  max_server_body_size, max_header_count, body_read_timeout, pending);

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

            // NOLINTNEXTLINE(clang-analyzer-cplusplus.Move)
            http_response response = handle_request_with_forward(request);
            const bool include_body = !request.method.is_head();
            keep_alive = request.is_keep_alive();
            ++processed;

            if (!pending.empty() && keep_alive && response.raw_response.empty() &&
                (max_keep_alive_requests == 0 || processed < max_keep_alive_requests) &&
                has_complete_request(pending, max_server_header_size.bytes())) {
                string batch = include_body ? response.to_string() : response.build_header_string();
                while (keep_alive && !pending.empty() &&
                       (max_keep_alive_requests == 0 || processed < max_keep_alive_requests) &&
                       has_complete_request(pending, max_server_header_size.bytes())) {
                    http_request next =
                            parse_request(client_socket.get(), session_manager_, cookie_name_, max_server_header_size,
                                          max_server_body_size, max_header_count, body_read_timeout, pending);
                    if (client_socket->is_ssl()) {
                        next.set_header(http_key::X_Forwarded_Proto(), "https");
                    }
                    http_response next_response = handle_request_with_forward(next);
                    const bool next_include_body = !next.method.is_head();
                    batch += next_include_body ? next_response.to_string() : next_response.build_header_string();
                    keep_alive = next.is_keep_alive();
                    ++processed;
                }
                client_socket->send_all(memory_view<const char>(batch.data(), batch.size()));
                continue;
            }

            // NOLINTNEXTLINE(clang-analyzer-cplusplus.Move)
            send_response(client_socket.get(), response, include_body);
            if (response.close_after_send) {
                keep_alive = false;
            }
        } catch (const http_exception& e) {
            if (e.code().value() == 0) {
                send_error_response(client_socket.get(), http_status::bad_request, e.what());
            } else {
                send_error_response(client_socket.get(), static_cast<http_status>(e.code().value()), e.what());
            }
            keep_alive = false;
        } catch (const exception& e) {
            send_error_response(client_socket.get(), http_status::internal_server_error, e.what());
            keep_alive = false;
        } catch (...) {
            send_error_response(client_socket.get(), http_status::internal_server_error, "Unknown internal error");
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
        const auto it = upgrade_handlers_.find(proto_lower);
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
                send_error_response(client_socket.get(), http_status::bad_request, "Missing Sec-WebSocket-Key");
                return true;
            }

            string accept = compute_websocket_accept(key);

            http_response upgrade_response;
            upgrade_response.status = http_status::switching_protocols;
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
    not_impl.status = http_status::not_implemented;
    not_impl.status_message = "Not Implemented";
    not_impl.set_content_type(http_content::PLAIN_TEXT());
    not_impl.body = "Unsupported upgrade protocol";
    send_response(client_socket.get(), not_impl);
    return true;
}

void http_server::handle_connect(const unique_ptr<tcp_socket>& client_socket, http_request& request) {
    const string_view path = request.path.view();
    const size_t colon = path.find(':');
    if (colon == string_view::npos) {
        send_error_response(client_socket.get(), http_status::bad_request, "Invalid CONNECT target");
        return;
    }

    const string host(path.view(0, colon));
    const string_view port_str(path.view(colon + 1));
    uint16_t port_num = 0;
    try {
        port_num = static_cast<uint16_t>(uinteger64::parse(port_str).value());
    } catch (...) {
        send_error_response(client_socket.get(), http_status::bad_request, "Invalid port in CONNECT target");
        return;
    }

    tcp_client tunnel(*ctx_);
    tunnel.set_connect_timeout(milliseconds(10000));
    if (!tunnel.connect(host, ports{port_num})) {
        send_error_response(client_socket.get(), http_status::bad_gateway, "Failed to connect to tunnel target");
        return;
    }

    http_response established;
    established.version = request.version;
    established.status = http_status::ok;
    established.status_message = "Connection Established";
    established.headers.clear();
    established.suppress_content_length = true;
    send_response(client_socket.get(), established);

    const auto client_fd = client_socket->native_handle();
    const auto tunnel_fd = tunnel.socket().native_handle();

    client_socket->set_nonblocking(true);
    tunnel.socket().set_nonblocking(true);

    char buffer[8192];
    bool client_open = true;
    bool tunnel_open = true;

    while (client_open && tunnel_open) {
#ifdef NEFORCE_PLATFORM_WINDOWS
        ::WSAPOLLFD fds[2];
#else
        ::pollfd fds[2];
#endif
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

#ifdef NEFORCE_PLATFORM_WINDOWS
        const int ret = ::WSAPoll(fds, nfds, -1);
        if (ret < 0) {
            if (::WSAGetLastError() == WSAEINTR) {
                continue;
            }
            break;
        }
#else
        const int ret = ::poll(fds, nfds, -1);
        if (ret < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }
#endif

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


struct http_server::async_forward_state : enable_shared_from_this<http_server::async_forward_state> {
    http_server* server{nullptr};
    shared_ptr<http_request> request;
    int forward_count{0};
    function<void(http_request&, http_response)> callback;

    void run() {
        auto self = shared_from_this();
        server->router_.handle_request_async_with_request(
                *request, [self](http_request& /*dispatched*/, http_response response) mutable {
                    if (!response.forward_path.empty()) {
                        if (self->forward_count >= max_forward_count) {
                            http_response error_response;
                            error_response.status = http_status::internal_server_error;
                            error_response.status_message = "Internal Server Error";
                            error_response.set_content_type(http_content::PLAIN_TEXT());
                            error_response.body = "Too many forwards";
                            self->callback(*self->request, move(error_response));
                            return;
                        }
                        ++self->forward_count;
                        self->request->path = move(response.forward_path);
                        self->request->parameters.clear();
                        parse_parameters(*self->request);
                        self->run();
                        return;
                    }

                    add_session_cookie(*self->request, response, self->request->session.materialized(),
                                       self->request->session.created(), self->server->cookie_name_);
                    self->callback(*self->request, move(response));
                });
    }
};

void http_server::handle_client_async(unique_ptr<tcp_socket> client_socket) {
    client_socket->set_nonblocking(true);
    auto session = make_shared<async_http1_session>(*this, move(client_socket), *ctx_);
    session->start();
}

void http_server::handle_request_with_forward_async(http_request request,
                                                    function<void(http_request&, http_response)> cb) {
    auto state = make_shared<async_forward_state>();
    state->server = this;
    state->request = make_shared<http_request>(move(request));
    state->callback = move(cb);
    state->run();
}

http_response http_server::handle_request_with_forward(http_request& request) {
    int forward_count = 0;

    while (forward_count < max_forward_count) {
        http_response response = router_.handle_request(request);

        if (!response.forward_path.empty()) {
            request.path = move(response.forward_path);
            request.parameters.clear();
            parse_parameters(request);
            forward_count++;
            continue;
        }

        add_session_cookie(request, response, request.session.materialized(), request.session.created(), cookie_name_);
        return response;
    }

    http_response error_response;
    error_response.status = http_status::internal_server_error;
    error_response.status_message = "Internal Server Error";
    error_response.set_content_type(http_content::PLAIN_TEXT());
    error_response.body = "Too many forwards";
    return error_response;
}

http_server::http_server(ports port, io_context& ioc, size_t worker_count) :
ctx_(&ioc),
server_(make_unique<tcp_server>(port, ioc, worker_count)) {
    server_->set_client_handler([this](unique_ptr<tcp_socket> sock) { this->handle_client(move(sock)); });

    set_upgrade_handler("h2c", [this](http_request& request, tcp_socket* sock) -> bool {
        const string_view settings_b64 = request.header("HTTP2-Settings");
        if (settings_b64.empty()) {
            return false;
        }

        {
            lock<mutex> lk(h2c_mutex_);
            if (h2_connections_.size() >= max_h2c_upgrades) {
                return false;
            }
        }

        http_response upgrade_resp;
        upgrade_resp.version = request.version;
        upgrade_resp.status = http_status::switching_protocols;
        upgrade_resp.status_message = "Switching Protocols";
        upgrade_resp.set_header("Connection", "Upgrade");
        upgrade_resp.set_header("Upgrade", "h2c");
        send_response(sock, upgrade_resp);

        auto conn = make_shared<http2_connection>(unique_ptr<tcp_socket>(sock), *ctx_);
        conn->set_router(&router_);
        conn->set_max_concurrent_streams(h2_max_concurrent_streams);
        const weak_ptr<http2_connection> weak_conn = conn;
        conn->set_close_handler([this, weak_conn](uint32_t) {
            if (const auto closing = weak_conn.lock()) {
                lock<mutex> lk(h2c_mutex_);
                h2_connections_.erase(_NEFORCE remove(h2_connections_.begin(), h2_connections_.end(), closing),
                                      h2_connections_.end());
            }
        });
        conn->start();

        // RFC 7540 §3.2: after h2c upgraded，raw HTTP/1.1 request will be regarded as the HEADERS frame of stream 1
        http_response h2_resp = router_.handle_request(request);
        vector<hpack_header_field> resp_headers;
        resp_headers.push_back({":status", to_string(static_cast<uint16_t>(h2_resp.status))});
        for (const auto& h: h2_resp.headers) {
            resp_headers.push_back({h.first.lowercase(), h.second});
        }
        conn->send_response(1, resp_headers, h2_resp.body_view(), true);

        // Keep connection alive to prevent premature destruction of I/O callbacks on io_context pool threads
        {
            lock<mutex> lk(h2c_mutex_);
            h2_connections_.push_back(move(conn));
        }

        return true;
    });
}

http_server::http_server(ports port, io_context& ioc, ssl_context ctx, size_t worker_count) :
ctx_(&ioc),
server_(make_unique<ssl_server>(port, ioc, worker_count)) {
    auto* ssl_srv = dynamic_cast<ssl_server*>(server_.get());
    if (ssl_srv != nullptr) {
        ctx.set_alpn_protos({"h2", "http/1.1"});
        ssl_srv->set_ssl_context(move(ctx));
    }
    server_->set_client_handler([this](unique_ptr<tcp_socket> sock) { this->handle_client(move(sock)); });
}

http_server::~http_server() {
    try {
        server_->stop();
        vector<shared_ptr<http2_connection>> connections;
        {
            lock<mutex> lk(h2c_mutex_);
            connections.swap(h2_connections_);
        }
        connections.clear();
    } catch (const exception& e) {
        NEFORCE_REPORT_EXCEPTION(e);
    }
}

bool http_server::load_certificate(const string& cert_file, const string& key_file) {
    auto* ssl_srv = dynamic_cast<ssl_server*>(server_.get());
    if (ssl_srv == nullptr) {
        return false;
    }
    return ssl_srv->load_certificate(cert_file, key_file);
}

http_session* http_server::get_session(http_request& request, const bool create) {
    if (request.session.materialized() == nullptr && create) {
        const string session_id(request.cookie(cookie_name_.cookie_name()));
        request.session = session_manager_.get_session(session_id, true);
        request.session.mark_created();
    }
    return request.session.materialized();
}

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
