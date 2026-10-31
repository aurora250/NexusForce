#include <NeForce/network/http/reverse_proxy.hpp>
#include <NeForce/core/time/clocks.hpp>
#include <NeForce/core/container/deque.hpp>
#include <NeForce/core/utility/hexadecimal.hpp>
#include <NeForce/network/util/ip_address.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <WS2tcpip.h>
#else
#    include <netdb.h>
#endif
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

namespace {
    constexpr size_t async_backend_buffer_size = 16UL * 1024UL;

    NEFORCE_NODISCARD bool ascii_icontains(const string_view haystack, const string_view needle) noexcept {
        if (needle.size() > haystack.size()) {
            return false;
        }
        for (size_t i = 0; i + needle.size() <= haystack.size(); ++i) {
            bool equal = true;
            for (size_t j = 0; j < needle.size(); ++j) {
                char a = haystack[i + j];
                char b = needle[j];
                if (a >= 'A' && a <= 'Z') {
                    a = static_cast<char>(a + 32);
                }
                if (b >= 'A' && b <= 'Z') {
                    b = static_cast<char>(b + 32);
                }
                if (a != b) {
                    equal = false;
                    break;
                }
            }
            if (equal) {
                return true;
            }
        }
        return false;
    }

    void remove_header_ci(flat_unordered_map<string, string>& headers, const string_view name) {
        for (auto it = headers.begin(); it != headers.end();) {
            if (it->first.compare_ignore_case(name) == 0) {
                it = headers.erase(it);
            } else {
                ++it;
            }
        }
    }

    void filter_raw_headers_impl(string& raw, const vector<string>& skipped_headers) {
        if (skipped_headers.empty()) {
            return;
        }
        const size_t header_end = raw.find("\r\n\r\n");
        if (header_end == string::npos) {
            return;
        }
        size_t pos = raw.find("\r\n");
        if (pos == string::npos || pos + 2 > header_end) {
            return;
        }
        pos += 2;

        string filtered;
        filtered.reserve(raw.size());
        filtered.append(raw.data(), pos);
        bool removed = false;
        while (pos < header_end) {
            const size_t line_end = raw.find("\r\n", pos);
            if (line_end == string::npos || line_end > header_end) {
                return;
            }
            const string_view line(raw.data() + pos, line_end - pos);
            const size_t colon = line.find(':');
            const string_view name = colon == string_view::npos ? line : line.head(colon);
            bool skip = false;
            for (const auto& skipped: skipped_headers) {
                if (name.compare_ignore_case(skipped.view()) == 0) {
                    skip = true;
                    removed = true;
                    break;
                }
            }
            if (!skip) {
                filtered.append(line.data(), line.size());
                filtered.append("\r\n");
            }
            pos = line_end + 2;
        }
        if (!removed) {
            return;
        }
        filtered.append("\r\n");
        filtered.append(raw.data() + header_end + 4, raw.size() - header_end - 4);
        raw = move(filtered);
    }

    struct async_backend_response {
        http_response response; ///< parsed response
        string raw;             ///< raw header + body
        bool keep_alive{false};
    };

    NEFORCE_NODISCARD bool decode_chunked_body(const string_view chunked, string& decoded, size_t& consumed) {
        decoded.clear();
        size_t pos = 0;
        while (pos < chunked.size()) {
            const size_t line_end = chunked.find("\r\n", pos);
            if (line_end == string::npos) {
                return false;
            }
            string_view size_line = chunked.view(pos, line_end - pos).trim();
            const size_t semicolon = size_line.find(';');
            if (semicolon != string_view::npos) {
                size_line = size_line.head(semicolon);
            }
            uint64_t chunk_size = 0;
            try {
                chunk_size = hexadecimal::parse(size_line).value();
            } catch (...) {
                return false;
            }
            pos = line_end + 2;

            if (chunk_size == 0) {
                if (pos + 2 <= chunked.size() && chunked.view(pos, 2) == "\r\n") {
                    consumed = pos + 2;
                    return true;
                }
                const size_t trailer_end = chunked.find("\r\n\r\n", pos);
                if (trailer_end == string::npos) {
                    return false;
                }
                consumed = trailer_end + 4;
                return true;
            }

            if (chunk_size > chunked.size() - pos || pos + chunk_size + 2 > chunked.size()) {
                return false;
            }
            decoded.append(chunked.view(pos, static_cast<size_t>(chunk_size)));
            pos += static_cast<size_t>(chunk_size);
            if (chunked.view(pos, 2) != "\r\n") {
                return false;
            }
            pos += 2;
        }
        return false;
    }

    NEFORCE_NODISCARD bool parse_upstream_head(const string_view head, http_response& response, size_t& head_size,
                                               uint64_t& content_length, bool& chunked, bool& keep_alive) {
        const size_t header_end = head.find("\r\n\r\n");
        if (header_end == string::npos) {
            return false;
        }
        const size_t status_end = head.find("\r\n");
        if (status_end == string::npos) {
            return false;
        }
        const string_view status_line = head.head(status_end);
        const size_t sp1 = status_line.find(' ');
        if (sp1 == string::npos) {
            return false;
        }
        const size_t sp2 = status_line.find(' ', sp1 + 1);
        if (sp2 == string::npos) {
            return false;
        }

        response.version = string(status_line.head(sp1));
        uint16_t code = 0;
        try {
            code = uinteger16::parse(status_line.view(sp1 + 1, sp2 - sp1 - 1)).value();
        } catch (...) {
            return false;
        }
        response.status = http_status_from_code(code);
        response.status_message = string(status_line.tail(sp2 + 1));
        response.headers.clear();

        content_length = numeric_traits<uint64_t>::max();
        chunked = false;
        string connection;
        size_t pos = status_end + 2;
        while (pos < header_end) {
            const size_t line_end = head.find("\r\n", pos);
            if (line_end == string::npos || line_end > header_end) {
                return false;
            }
            const string_view line = head.view(pos, line_end - pos);
            const size_t colon = line.find(':');
            if (colon != string_view::npos) {
                const string key(line.head(colon).trim());
                const string value(line.tail(colon + 1).trim());
                if (key.compare_ignore_case("content-length") == 0) {
                    try {
                        content_length = uinteger64::parse(value.view()).value();
                    } catch (...) {
                        return false;
                    }
                } else if (key.compare_ignore_case("transfer-encoding") == 0) {
                    if (ascii_icontains(value.view(), "chunked")) {
                        chunked = true;
                    }
                } else if (key.compare_ignore_case("connection") == 0) {
                    connection = value;
                }
                auto it = response.headers.find(key);
                if (it == response.headers.end()) {
                    response.headers[key] = value;
                }
            }
            pos = line_end + 2;
        }

        const string connection_lower = connection.lowercase();
        if (response.version.starts_with("HTTP/1.0")) {
            keep_alive = connection_lower.contains("keep-alive");
        } else {
            keep_alive = !connection_lower.contains("close");
        }

        remove_header_ci(response.headers, "connection");
        remove_header_ci(response.headers, "keep-alive");
        remove_header_ci(response.headers, "proxy-connection");
        remove_header_ci(response.headers, "transfer-encoding");
        remove_header_ci(response.headers, "te");
        remove_header_ci(response.headers, "trailer");
        remove_header_ci(response.headers, "upgrade");
        if (chunked) {
            remove_header_ci(response.headers, "content-length");
        }

        head_size = header_end + 4;
        return true;
    }

    NEFORCE_NODISCARD optional<ip_address> resolve_backend_endpoint(const string& host, const ports port) {
        if (const auto numeric = ip_address::parse(host, port)) {
            return numeric;
        }

        ::addrinfo hints{};
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        ::addrinfo* result = nullptr;
        const string service = to_string(port.value());
        if (::getaddrinfo(host.data(), service.data(), &hints, &result) != 0 || result == nullptr) {
            return none;
        }

        optional<ip_address> endpoint;
        optional<ip_address> ipv6_endpoint;
        for (const auto* ai = result; ai != nullptr; ai = ai->ai_next) {
            if (ai->ai_family == AF_INET && ai->ai_addrlen >= sizeof(::sockaddr_in)) {
                ::sockaddr_in addr{};
                memory_copy(&addr, ai->ai_addr, sizeof(addr));
                endpoint = ip_address(addr);
                break;
            }
            if (ai->ai_family == AF_INET6 && ai->ai_addrlen >= sizeof(::sockaddr_in6) && !ipv6_endpoint) {
                ::sockaddr_in6 addr{};
                memory_copy(&addr, ai->ai_addr, sizeof(addr));
                ipv6_endpoint = ip_address(addr);
            }
        }
        ::freeaddrinfo(result);
        return endpoint ? endpoint : ipv6_endpoint;
    }

    class async_backend_connection final : public enable_shared_from_this<async_backend_connection> {
    public:
        struct options {
            size_t max_connections{0};
            size_t max_queue{0};
            size_t max_idle{32};
            size_t max_response_size{64UL * 1024UL * 1024UL};
            bool verify_ssl{true};
            milliseconds connect_timeout{5000};
            milliseconds receive_timeout{30000};
        };

        async_backend_connection(io_context& ctx, proxy_backend backend) :
        ctx_(&ctx),
        backend_(move(backend)) {
            read_buffer_.resize(async_backend_buffer_size);
        }

        ~async_backend_connection() { close(); }

        async_backend_connection(const async_backend_connection&) = delete;
        async_backend_connection& operator=(const async_backend_connection&) = delete;

        NEFORCE_NODISCARD bool is_connected() const noexcept { return client_ && client_->is_connected(); }

        void detach_fd() noexcept {
            if (!client_ || !client_->is_connected()) {
                return;
            }
            try {
                ctx_->remove_fd(client_->socket().native_handle());
                // NOLINTNEXTLINE(bugprone-empty-catch)
            } catch (const exception&) {
            }
        }

        void close() noexcept {
            if (timer_id_ != 0) {
                ctx_->cancel_timer(timer_id_);
                timer_id_ = 0;
            }
            if (cancel_source_) {
                const auto source = move(cancel_source_);
                source->request_stop();
            }
            detach_fd();
            if (client_) {
                client_->disconnect();
                client_.reset();
            }
        }

        void connect(const ip_address& endpoint, const options& opts, const shared_ptr<ssl_context>& client_ssl_context,
                     function<void(error_code)> handler) {
            opts_ = opts;
            try {
                if (backend_.scheme == "https") {
                    auto ssl = make_unique<ssl_client>(*ctx_);
                    ssl->set_ssl_context(client_ssl_context->share());
                    ssl->set_sni_hostname(backend_.host);
                    ssl->set_verify_peer(opts.verify_ssl);
                    client_ = move(ssl);
                } else {
                    client_ = make_unique<tcp_client>(*ctx_);
                }
                client_->set_connect_timeout(opts.connect_timeout);
                client_->set_recv_timeout(opts.receive_timeout);
            } catch (const exception&) {
                handler(error_code(static_cast<int>(errc::invalid_argument), error_category::system()));
                return;
            }

            auto self = shared_from_this();
            client_->async_connect(*ctx_, endpoint, [self, handler = move(handler)](error_code ec) mutable {
                if (ec) {
                    self->close();
                    handler(ec);
                    return;
                }
                try {
                    self->client_->socket().set_nonblocking(true);
                    // NOLINTNEXTLINE(bugprone-empty-catch)
                } catch (const exception&) {
                }
                handler(error_code{});
            });
        }

        void start_request(string data, const bool head,
                           function<void(error_code, shared_ptr<async_backend_response>)> handler) {
            request_ = move(data);
            write_offset_ = 0;
            buffer_.clear();
            callback_ = move(handler);
            head_request_ = head;
            completed_ = false;
            head_parsed_ = false;
            body_start_ = 0;
            content_length_ = 0;
            chunked_ = false;
            body_expected_ = true;
            read_until_close_ = false;
            keep_alive_ = false;
            response_.reset();
            timed_out_ = false;
            cancel_source_ = make_unique<stop_source>();
            cancel_slot_ = cancellation_slot(cancel_source_->get_token());

            if (!is_connected()) {
                finish(error_code(static_cast<int>(errc::not_connected), error_category::system()), nullptr);
                return;
            }

            if (opts_.receive_timeout > milliseconds(0)) {
                timer_id_ = ctx_->schedule_timer(static_cast<uint64_t>(opts_.receive_timeout.count()),
                                                 [self = shared_from_this()] { self->on_timeout(); });
            }

            write_next();
        }

    private:
        void on_timeout() {
            if (completed_) {
                return;
            }
            timer_id_ = 0;
            timed_out_ = true;
            if (cancel_source_) {
                const auto source = move(cancel_source_);
                source->request_stop();
            }
            if (!completed_) {
                finish(make_error_code(errc::timed_out), nullptr);
            }
        }

        void fail(const error_code ec) { finish(ec, nullptr); }

        void finish(error_code ec, shared_ptr<async_backend_response> response) {
            if (completed_) {
                return;
            }
            completed_ = true;
            if (timer_id_ != 0) {
                ctx_->cancel_timer(timer_id_);
                timer_id_ = 0;
            }
            const auto cb = move(callback_);
            callback_ = nullptr;
            if (cb) {
                cb(ec, move(response));
            }
        }

        void write_next() {
            if (completed_) {
                return;
            }
            if (write_offset_ >= request_.size()) {
                read_next();
                return;
            }
            auto self = shared_from_this();
            client_->async_write(
                    *ctx_, memory_view<const char>(request_.data() + write_offset_, request_.size() - write_offset_),
                    cancel_slot_, [self](error_code ec, size_t bytes) {
                        if (self->completed_) {
                            return;
                        }
                        if (ec || bytes == 0) {
                            if (self->timed_out_) {
                                self->fail(make_error_code(errc::timed_out));
                                return;
                            }
                            self->fail(ec ? ec
                                          : error_code(static_cast<int>(errc::connection_reset),
                                                       error_category::system()));
                            return;
                        }
                        self->write_offset_ += bytes;
                        if (self->write_offset_ < self->request_.size()) {
                            self->write_next();
                        } else {
                            self->read_next();
                        }
                    });
        }

        void read_next() {
            if (completed_) {
                return;
            }
            auto self = shared_from_this();
            client_->async_read(
                    *ctx_, memory_view<char>(read_buffer_.data(), read_buffer_.size()), cancel_slot_,
                    [self](error_code ec, size_t bytes) {
                        if (self->completed_) {
                            return;
                        }
                        if (ec) {
                            if (self->timed_out_) {
                                self->fail(make_error_code(errc::timed_out));
                                return;
                            }
                            if (self->read_until_close_ && self->head_parsed_) {
                                self->complete_until_close();
                            } else {
                                self->fail(ec);
                            }
                            return;
                        }
                        if (bytes == 0) {
                            if (self->read_until_close_ && self->head_parsed_) {
                                self->complete_until_close();
                            } else {
                                self->fail(
                                        error_code(static_cast<int>(errc::connection_reset), error_category::system()));
                            }
                            return;
                        }
                        self->buffer_.append(self->read_buffer_.data(), bytes);
                        if (self->buffer_.size() > self->opts_.max_response_size) {
                            self->fail(error_code(static_cast<int>(errc::value_too_large), error_category::system()));
                            return;
                        }
                        self->process();
                    });
        }

        void process() {
            if (completed_) {
                return;
            }

            if (!head_parsed_) {
                for (;;) {
                    const size_t header_end = buffer_.find("\r\n\r\n");
                    if (header_end == string::npos) {
                        if (buffer_.size() > 64UL * 1024UL) {
                            fail(error_code(static_cast<int>(errc::value_too_large), error_category::system()));
                            return;
                        }
                        read_next();
                        return;
                    }

                    uint64_t content_length = 0;
                    bool chunked = false;
                    bool keep_alive = false;
                    auto parsed = make_shared<http_response>();
                    size_t head_size = 0;
                    if (!parse_upstream_head(buffer_.view(0, header_end + 4), *parsed, head_size, content_length,
                                             chunked, keep_alive)) {
                        fail(error_code(static_cast<int>(errc::invalid_argument), error_category::system()));
                        return;
                    }

                    const auto code = static_cast<uint16_t>(parsed->status);
                    if (code >= 100 && code < 200 && code != 101) {
                        buffer_.erase(0, head_size);
                        continue;
                    }

                    head_parsed_ = true;
                    body_start_ = head_size;
                    content_length_ = content_length;
                    chunked_ = chunked;
                    keep_alive_ = keep_alive;
                    response_ = move(parsed);

                    if (head_request_ || code == 204 || code == 304 || (code >= 100 && code < 200)) {
                        body_expected_ = false;
                    }
                    if (code == 101) {
                        keep_alive_ = false;
                    }
                    break;
                }
            }

            if (!body_expected_) {
                complete_body(0);
                return;
            }

            if (chunked_) {
                string decoded;
                size_t consumed = 0;
                if (decode_chunked_body(buffer_.view(body_start_), decoded, consumed)) {
                    response_->body = move(decoded);
                    complete_body(consumed);
                    return;
                }
                if (buffer_.size() > opts_.max_response_size) {
                    fail(error_code(static_cast<int>(errc::value_too_large), error_category::system()));
                    return;
                }
                read_next();
                return;
            }

            if (content_length_ != numeric_traits<uint64_t>::max()) {
                const size_t available = buffer_.size() - body_start_;
                if (available >= content_length_) {
                    response_->body.assign(buffer_.data() + body_start_, static_cast<size_t>(content_length_));
                    complete_body(static_cast<size_t>(content_length_));
                    return;
                }
                read_next();
                return;
            }

            read_until_close_ = true;
            read_next();
        }

        void complete_until_close() {
            keep_alive_ = false;
            const size_t available = buffer_.size() - body_start_;
            if (available > 0) {
                response_->body.assign(buffer_.data() + body_start_, available);
            }
            complete_body(available);
        }

        void complete_body(const size_t body_size) {
            auto result = make_shared<async_backend_response>();
            result->keep_alive = keep_alive_;
            result->response = move(*response_);
            result->raw = buffer_.head(body_start_ + body_size);
            finish(error_code{}, move(result));
        }

        io_context* ctx_;
        proxy_backend backend_;
        unique_ptr<tcp_client_base> client_;
        options opts_;
        string request_;
        size_t write_offset_{0};
        vector<char> read_buffer_;
        string buffer_;
        size_t body_start_{0};
        uint64_t content_length_{0};
        size_t timer_id_{0};
        function<void(error_code, shared_ptr<async_backend_response>)> callback_;
        shared_ptr<http_response> response_;
        bool head_request_{false};
        bool head_parsed_{false};
        bool body_expected_{true};
        bool chunked_{false};
        bool read_until_close_{false};
        bool keep_alive_{false};
        atomic<bool> completed_{false};
        bool timed_out_{false};
        unique_ptr<stop_source> cancel_source_;
        cancellation_slot cancel_slot_;
    };

    struct async_pool_bucket {
        proxy_backend backend;
        async_backend_connection::options options;
        vector<shared_ptr<async_backend_connection>> idle;
        deque<function<void(shared_ptr<async_backend_connection>, error_code)>> waiters;
        size_t active{0};
        size_t waiting{0};
    };
} // namespace

struct reverse_proxy_filter::reverse_proxy_async_state
: enable_shared_from_this<reverse_proxy_filter::reverse_proxy_async_state> {
    explicit reverse_proxy_async_state(io_context& ctx) :
    ctx_(&ctx) {}

    ~reverse_proxy_async_state() noexcept {
        try {
            close_all();
            // NOLINTNEXTLINE(bugprone-empty-catch)
        } catch (...) {
        }
    }

    void acquire(const proxy_backend& backend, const async_backend_connection::options& options,
                 function<void(shared_ptr<async_backend_connection>, error_code)> callback) {
        const string key = backend.scheme + "://" + backend.host + ":" + to_string(backend.port.value());
        shared_ptr<async_backend_connection> reuse;
        bool create = false;
        bool reject = false;
        {
            lock<mutex> lk(mutex_);
            if (stopping_) {
                reject = true;
            } else {
                auto& bucket = buckets_[key];
                bucket.backend = backend;
                bucket.options = options;
                if (!bucket.idle.empty()) {
                    reuse = move(bucket.idle.back());
                    bucket.idle.pop_back();
                } else if (options.max_connections == 0 || bucket.active < options.max_connections) {
                    ++bucket.active;
                    create = true;
                } else if (options.max_queue > 0 && bucket.waiting >= options.max_queue) {
                    ++queue_rejections_;
                    reject = true;
                } else {
                    ++bucket.waiting;
                    bucket.waiters.push_back(move(callback));
                    pool_waits_.fetch_add(1, memory_order_relaxed);
                    return;
                }
            }
        }

        if (reject) {
            callback(nullptr,
                     error_code(static_cast<int>(errc::resource_unavailable_try_again), error_category::system()));
            return;
        }
        if (reuse) {
            if (reuse->is_connected()) {
                callback(move(reuse), error_code{});
                return;
            }
            reuse->close();
            create = true;
        }
        if (create) {
            start_connect(key, backend, options, move(callback));
        }
    }

    void release(const string& key, shared_ptr<async_backend_connection> connection, const bool reusable) {
        function<void(shared_ptr<async_backend_connection>, error_code)> waiter;
        bool close_connection = false;
        bool start_new = false;
        proxy_backend backend;
        async_backend_connection::options options;
        {
            lock<mutex> lk(mutex_);
            const auto it = buckets_.find(key);
            if (it == buckets_.end()) {
                close_connection = true;
            } else {
                auto& bucket = it->second;
                backend = bucket.backend;
                options = bucket.options;
                if (stopping_) {
                    close_connection = true;
                    if (bucket.active > 0) {
                        --bucket.active;
                    }
                } else if (reusable && connection && connection->is_connected()) {
                    if (!bucket.waiters.empty()) {
                        waiter = move(bucket.waiters.front());
                        bucket.waiters.pop_front();
                        if (bucket.waiting > 0) {
                            --bucket.waiting;
                        }
                    } else if (bucket.idle.size() < bucket.options.max_idle) {
                        bucket.idle.push_back(move(connection));
                    } else {
                        close_connection = true;
                        if (bucket.active > 0) {
                            --bucket.active;
                        }
                    }
                } else {
                    close_connection = true;
                    if (bucket.active > 0) {
                        --bucket.active;
                    }
                    if (!bucket.waiters.empty()) {
                        waiter = move(bucket.waiters.front());
                        bucket.waiters.pop_front();
                        if (bucket.waiting > 0) {
                            --bucket.waiting;
                        }
                        ++bucket.active;
                        start_new = true;
                    }
                }
            }
        }

        if (close_connection && connection) {
            connection->close();
        }
        if (waiter) {
            if (!start_new && connection) {
                waiter(move(connection), error_code{});
            } else {
                start_connect(key, backend, options, move(waiter));
            }
        }
    }

    void close_idle() {
        flat_unordered_map<string, vector<shared_ptr<async_backend_connection>>> idle;
        {
            lock<mutex> lk(mutex_);
            for (auto& entry: buckets_) {
                if (!entry.second.idle.empty()) {
                    idle[entry.first] = move(entry.second.idle);
                    entry.second.idle.clear();
                    if (entry.second.active >= idle[entry.first].size()) {
                        entry.second.active -= idle[entry.first].size();
                    } else {
                        entry.second.active = 0;
                    }
                }
            }
        }
        for (auto& entry: idle) {
            for (auto& connection: entry.second) {
                if (connection) {
                    connection->close();
                }
            }
        }
    }

    void close_all() {
        flat_unordered_map<string, vector<shared_ptr<async_backend_connection>>> idle;
        {
            lock<mutex> lk(mutex_);
            stopping_ = true;
            for (auto& entry: buckets_) {
                idle[entry.first] = move(entry.second.idle);
                entry.second.idle.clear();
            }
        }
        for (auto& entry: idle) {
            for (auto& connection: entry.second) {
                if (connection) {
                    connection->close();
                }
            }
        }
    }

    NEFORCE_NODISCARD shared_ptr<ssl_context> ssl_client_context() {
        lock<mutex> lk(mutex_);
        if (!ssl_context_) {
            ssl_context_ = make_shared<ssl_context>(ssl_method::tls_client);
        }
        return ssl_context_;
    }

    NEFORCE_NODISCARD optional<ip_address> resolve(const proxy_backend& backend) {
        const string key = backend.host + ":" + to_string(backend.port.value());
        {
            lock<mutex> lk(mutex_);
            const auto it = resolved_.find(key);
            if (it != resolved_.end()) {
                return it->second;
            }
        }
        auto endpoint = resolve_backend_endpoint(backend.host, backend.port);
        if (endpoint) {
            lock<mutex> lk(mutex_);
            resolved_[key] = *endpoint;
        }
        return endpoint;
    }

    void start_connect(const string& key, const proxy_backend& backend,
                       const async_backend_connection::options& options,
                       function<void(shared_ptr<async_backend_connection>, error_code)> callback) {
        const auto endpoint = resolve(backend);
        if (!endpoint) {
            on_connect_failure(key, backend, options, move(callback),
                               error_code(static_cast<int>(errc::host_unreachable), error_category::system()));
            return;
        }

        auto connection = make_shared<async_backend_connection>(*ctx_, backend);
        shared_ptr<ssl_context> ssl_ctx;
        if (backend.scheme == "https") {
            try {
                ssl_ctx = ssl_client_context();
            } catch (const exception&) {
                on_connect_failure(key, backend, options, move(callback),
                                   error_code(static_cast<int>(errc::invalid_argument), error_category::system()));
                return;
            }
        }

        auto self = shared_from_this();
        connection->connect(
                *endpoint, options, ssl_ctx,
                // NOLINTNEXTLINE(bugprone-exception-escape)
                [self, key, backend, options, connection, callback = move(callback)](error_code ec) mutable {
                    if (ec) {
                        self->on_connect_failure(key, backend, options, move(callback), ec);
                        return;
                    }
                    self->connections_.fetch_add(1, memory_order_relaxed);
                    callback(move(connection), error_code{});
                });
    }

    void on_connect_failure(const string& key, const proxy_backend& backend,
                            const async_backend_connection::options& options,
                            function<void(shared_ptr<async_backend_connection>, error_code)> callback,
                            const error_code ec) {
        function<void(shared_ptr<async_backend_connection>, error_code)> waiter;
        {
            lock<mutex> lk(mutex_);
            auto& bucket = buckets_[key];
            if (!bucket.waiters.empty()) {
                waiter = move(bucket.waiters.front());
                bucket.waiters.pop_front();
                if (bucket.waiting > 0) {
                    --bucket.waiting;
                }
            } else if (bucket.active > 0) {
                --bucket.active;
            }
        }
        if (waiter) {
            start_connect(key, backend, options, move(waiter));
        }
        callback(nullptr, ec);
    }

    io_context* ctx_;
    mutex mutex_;
    flat_unordered_map<string, async_pool_bucket> buckets_;
    flat_unordered_map<string, ip_address> resolved_;
    shared_ptr<ssl_context> ssl_context_;
    atomic<uint64_t> requests_{0};
    atomic<uint64_t> connections_{0};
    atomic<uint64_t> errors_{0};
    atomic<uint64_t> pool_waits_{0};
    atomic<uint64_t> queue_rejections_{0};
    bool stopping_{false};
};


bool reverse_proxy_filter::should_proxy(const string& path) const {
    if (path_prefix_.empty()) {
        return true;
    }
    return path.starts_with(path_prefix_);
}

proxy_backend reverse_proxy_filter::select_backend(const http_request& request) {
    lock<mutex> lk(mutex_);

    if (selector_) {
        return selector_(request);
    }

    if (backends_.empty()) {
        return {};
    }

    const size_t idx = rr_counter_++ % backends_.size();
    return backends_[idx];
}

const string& reverse_proxy_filter::backend_key(const proxy_backend& backend) const {
    thread_local const reverse_proxy_filter* cached_filter = nullptr;
    thread_local string cached_scheme;
    thread_local string cached_host;
    thread_local ports cached_port;
    thread_local string cached_key;
    if (cached_filter != this || cached_scheme != backend.scheme || cached_host != backend.host ||
        cached_port != backend.port) {
        cached_filter = this;
        cached_scheme = backend.scheme;
        cached_host = backend.host;
        cached_port = backend.port;
        cached_key = backend.scheme + "://" + backend.host + ":" + to_string(backend.port.value());
    }
    return cached_key;
}

http_client::config reverse_proxy_filter::backend_client_config() const {
    http_client::config cfg;
    cfg.connect_timeout = connect_timeout;
    cfg.send_timeout = send_timeout;
    cfg.receive_timeout = receive_timeout;
    cfg.follow_redirects = follow_redirects;
    cfg.keep_alive = true;
    cfg.verify_ssl = verify_backend_ssl;
    return cfg;
}


namespace {
    struct thread_local_backend_client {
        const reverse_proxy_filter* filter{nullptr};
        string key;
        unique_ptr<http_client> client;
    };

    thread_local_backend_client& backend_thread_cache() {
        thread_local thread_local_backend_client cache;
        return cache;
    }
} // namespace


unique_ptr<http_client> reverse_proxy_filter::acquire_backend_client(const proxy_backend& backend) {
    const string& key = backend_key(backend);
    auto& local = backend_thread_cache();
    if (enable_thread_local_clients && max_connections_per_backend == 0) {
        if (local.filter == this && local.key == key) {
            if (local.client && local.client->is_connected()) {
                return move(local.client);
            }
            if (local.client) {
                local.client->close();
                local.client.reset();
            }
            auto client = make_unique<http_client>(*ctx_, backend_client_config());
            backend_connection_count_.fetch_add(1, memory_order_relaxed);
            return client;
        }
        if (local.client) {
            local.client->close();
            local.client.reset();
        }
        local.filter = this;
        local.key = key;
        auto client = make_unique<http_client>(*ctx_, backend_client_config());
        backend_connection_count_.fetch_add(1, memory_order_relaxed);
        return client;
    }

    for (;;) {
        unique_ptr<http_client> client;
        {
            unique_lock<mutex> lk(pool_mutex_);
            auto& state = backend_states_[key];

            if (max_connections_per_backend > 0 && state.active >= max_connections_per_backend) {
                if (max_queue_per_backend > 0 && state.waiting >= max_queue_per_backend) {
                    queue_rejection_count_.fetch_add(1, memory_order_relaxed);
                    return nullptr;
                }
                ++state.waiting;
                const auto wait_begin = steady_clock::now();
                pool_cv_.wait(lk, [&] { return state.active < max_connections_per_backend; });
                --state.waiting;
                pool_wait_count_.fetch_add(1, memory_order_relaxed);
                const auto wait_us =
                        static_cast<uint32_t>(duration<double, micro>(steady_clock::now() - wait_begin).count());
                pool_wait_us_.push_back(wait_us);
            }

            auto& idle = idle_clients_[key];
            if (!idle.empty()) {
                client = move(idle.back());
                idle.pop_back();
            }
            ++state.active;
        }

        if (!client) {
            break;
        }
        if (client->is_connected()) {
            return client;
        }
        client->close();
        {
            lock<mutex> lk(pool_mutex_);
            auto& state = backend_states_[key];
            if (state.active > 0) {
                --state.active;
            }
        }
        pool_cv_.notify_one();
    }

    try {
        auto client = make_unique<http_client>(*ctx_, backend_client_config());
        backend_connection_count_.fetch_add(1, memory_order_relaxed);
        return client;
    } catch (...) {
        lock<mutex> lk(pool_mutex_);
        auto& state = backend_states_[key];
        if (state.active > 0) {
            --state.active;
        }
        pool_cv_.notify_one();
        throw;
    }
}

void reverse_proxy_filter::release_backend_client(const proxy_backend& backend, unique_ptr<http_client> client) {
    const string& key = backend_key(backend);
    const bool reusable = client && client->is_connected();
    if (client && !reusable) {
        client->close();
    }

    auto& local = backend_thread_cache();
    if (enable_thread_local_clients && max_connections_per_backend == 0) {
        if (local.filter == this && local.key == key && !local.client) {
            if (reusable) {
                local.client = move(client);
            }
            return;
        }
        if (local.client) {
            local.client->close();
            local.client.reset();
        }
        local.filter = this;
        local.key = key;
        if (reusable) {
            local.client = move(client);
        }
        return;
    }

    {
        lock<mutex> lk(pool_mutex_);
        auto& state = backend_states_[key];
        if (state.active > 0) {
            --state.active;
        }
        if (reusable && max_idle_connections > 0) {
            try {
                auto& idle = idle_clients_[key];
                if (idle.size() < max_idle_connections) {
                    idle.push_back(move(client));
                }
            } catch (...) {
                if (client) {
                    client->close();
                }
            }
        }
    }
    pool_cv_.notify_one();
}

http_client_request reverse_proxy_filter::build_backend_request(const proxy_backend& backend,
                                                                const http_request& request) {
    http_client_request creq;
    creq.method = request.method;
    creq.host = backend.host;
    creq.port = backend.port;
    creq.scheme = backend.scheme;
    creq.path = request.path;
    if (!request.query.empty()) {
        creq.path += "?" + request.query;
    }

    creq.headers = request.headers;

    if (!passthrough_host_header) {
        creq.headers["Host"] = backend.host;
    }

    const auto client_ip = request.client_ip();
    if (!client_ip.empty()) {
        const auto existing_fwd = creq.headers.find("X-Forwarded-For");
        if (existing_fwd != creq.headers.end()) {
            creq.headers["X-Forwarded-For"] = existing_fwd->second + ", " + client_ip;
        } else {
            creq.headers["X-Forwarded-For"] = client_ip;
        }
    }
    creq.headers["X-Forwarded-Proto"] = backend.scheme;
    creq.headers["X-Forwarded-Host"] = request.header("Host");

    if (header_rewrite_) {
        lock<mutex> lk(mutex_);
        header_rewrite_(creq.headers);
    }

    creq.body = request.body;
    return creq;
}

http_client_response reverse_proxy_filter::forward(const proxy_backend& backend, const http_request& request) {
    auto client = acquire_backend_client(backend);
    if (!client) {
        http_client_response overloaded;
        overloaded.status = http_status::service_unavailable;
        overloaded.status_message = "Service Unavailable";
        overloaded.body = "Backend connection limit reached";
        overloaded.headers[http_key::Content_Type()] = vector<string>{http_content::PLAIN_TEXT().to_string()};
        return overloaded;
    }

    auto response = client->request_fast(build_backend_request(backend, request));
    release_backend_client(backend, move(client));
    return response;
}

http_client_raw_response reverse_proxy_filter::forward_raw(const proxy_backend& backend, const http_request& request) {
    auto client = acquire_backend_client(backend);
    if (!client) {
        return http_client_raw_response{};
    }

    if (header_rewrite_) {
        auto response = client->request_raw(build_backend_request(backend, request));
        release_backend_client(backend, move(client));
        return response;
    }

    const auto request_host = request.header("Host");
    thread_local string raw_request;
    raw_request.clear();
    raw_request.reserve(512 + request.path.size() + request.body.size());

    raw_request += request.method.method();
    raw_request += " ";
    raw_request += request.path;
    if (!request.query.empty()) {
        raw_request += "?";
        raw_request += request.query;
    }
    raw_request += " HTTP/1.1\r\nHost: ";
    if (passthrough_host_header) {
        raw_request += request_host;
    } else {
        raw_request += backend.host;
        const bool default_port =
                (backend.scheme == "https" && backend.port == 443) || (backend.scheme == "http" && backend.port == 80);
        if (!default_port) {
            raw_request += ":";
            raw_request += to_string(backend.port.value());
        }
    }
    raw_request += "\r\n";

    for (const auto& h: request.headers) {
        if (h.first.compare_ignore_case("host") == 0 || h.first.compare_ignore_case("connection") == 0 ||
            h.first.compare_ignore_case("content-length") == 0 ||
            h.first.compare_ignore_case("transfer-encoding") == 0 ||
            h.first.compare_ignore_case("x-forwarded-for") == 0 ||
            h.first.compare_ignore_case("x-forwarded-proto") == 0 ||
            h.first.compare_ignore_case("x-forwarded-host") == 0) {
            continue;
        }
        raw_request += h.first;
        raw_request += ": ";
        raw_request += h.second;
        raw_request += "\r\n";
    }

    const auto client_ip = request.client_ip();
    const auto existing_fwd = request.header("X-Forwarded-For");
    if (!existing_fwd.empty() || !client_ip.empty()) {
        raw_request += "X-Forwarded-For: ";
        raw_request += existing_fwd;
        if (!existing_fwd.empty() && !client_ip.empty()) {
            raw_request += ", ";
        }
        raw_request += client_ip;
        raw_request += "\r\n";
    }
    raw_request += "X-Forwarded-Proto: ";
    raw_request += backend.scheme;
    raw_request += "\r\nX-Forwarded-Host: ";
    raw_request += request_host;
    raw_request += "\r\n";

    if (!request.body.empty()) {
        raw_request += "Content-Length: ";
        raw_request += to_string(request.body.size());
        raw_request += "\r\n";
    }
    raw_request += "Connection: keep-alive\r\n\r\n";
    if (!request.body.empty()) {
        raw_request += request.body;
    }

    auto response = client->request_raw(backend.scheme, backend.host, backend.port, raw_request.view(),
                                        request.method.is_head());
    release_backend_client(backend, move(client));
    if (!response.data.empty()) {
        filter_raw_response_headers(response.data);
    }
    return response;
}

void reverse_proxy_filter::filter_raw_response_headers(string& raw) {
    filter_raw_headers_impl(raw, skip_resp_headers_);
}

void reverse_proxy_filter::copy_response(http_client_response&& from, http_response& to) {
    to.status = from.status;
    to.status_message = move(from.status_message);
    to.body = move(from.body);

    for (auto& hdr: from.headers) {
        bool skip = false;
        for (const auto& s: skip_resp_headers_) {
            if (hdr.first.compare_ignore_case(s) == 0) {
                skip = true;
                break;
            }
        }
        if (!skip && !hdr.second.empty()) {
            to.headers[move(hdr.first)] = move(hdr.second[0]);
        }
    }
}

reverse_proxy_stats reverse_proxy_filter::stats() const {
    reverse_proxy_stats result;
    result.requests = request_count_.load(memory_order_relaxed);
    result.backend_connections = backend_connection_count_.load(memory_order_relaxed);
    result.pool_waits = pool_wait_count_.load(memory_order_relaxed);
    result.queue_rejections = queue_rejection_count_.load(memory_order_relaxed);
    result.direct_requests = direct_request_count_.load(memory_order_relaxed);

    shared_ptr<reverse_proxy_async_state> async_state;
    {
        lock<mutex> lk(async_mutex_);
        async_state = async_state_;
    }
    if (async_state) {
        result.async_requests = async_state->requests_.load(memory_order_relaxed);
        result.async_connections = async_state->connections_.load(memory_order_relaxed);
        result.async_errors = async_state->errors_.load(memory_order_relaxed);
        result.requests += result.async_requests;
        result.backend_connections += result.async_connections;
        result.pool_waits += async_state->pool_waits_.load(memory_order_relaxed);
        result.queue_rejections += async_state->queue_rejections_.load(memory_order_relaxed);
    }

    lock<mutex> lk(pool_mutex_);
    if (!pool_wait_us_.empty()) {
        vector<uint32_t> sorted = pool_wait_us_;
        sort(sorted.begin(), sorted.end());
        result.pool_wait_p99_us = sorted[min<size_t>(sorted.size() - 1, sorted.size() * 99 / 100)];
    }
    return result;
}

void reverse_proxy_filter::close_idle_connections() {
    {
        lock<mutex> lk(pool_mutex_);
        for (auto& entry: idle_clients_) {
            for (auto& client: entry.second) {
                if (client) {
                    client->close();
                }
            }
        }
        idle_clients_.clear();
        pool_cv_.notify_all();
    }

    shared_ptr<reverse_proxy_async_state> async_state;
    {
        lock<mutex> lk(async_mutex_);
        async_state = async_state_;
    }
    if (async_state) {
        async_state->close_idle();
    }
}


reverse_proxy_filter::~reverse_proxy_filter() noexcept {
    try {
        close_idle_connections();
        // NOLINTNEXTLINE(bugprone-empty-catch)
    } catch (...) {
    }
}

shared_ptr<reverse_proxy_filter::reverse_proxy_async_state> reverse_proxy_filter::ensure_async_state() {
    lock<mutex> lk(async_mutex_);
    if (!async_state_) {
        async_state_ = make_shared<reverse_proxy_async_state>(*ctx_);
    }
    return async_state_;
}

string reverse_proxy_filter::build_async_request(const proxy_backend& backend, const http_request& request) {
    flat_unordered_map<string, string> headers = request.headers;
    if (!passthrough_host_header) {
        string host = backend.host;
        const bool default_port =
                (backend.scheme == "https" && backend.port == 443) || (backend.scheme == "http" && backend.port == 80);
        if (!default_port) {
            host += ":";
            host += to_string(backend.port.value());
        }
        remove_header_ci(headers, "host");
        headers["Host"] = move(host);
    }

    if (header_rewrite_) {
        lock<mutex> lk(mutex_);
        header_rewrite_(headers);
    }

    string result;
    result.reserve(512 + request.path.size() + request.body.size());
    result += request.method.method();
    result += " ";
    result += request.path.empty() ? "/" : request.path.view();
    if (!request.query.empty()) {
        result += "?";
        result += request.query;
    }
    result += " HTTP/1.1\r\n";

    for (const auto& header: headers) {
        if (header.first.compare_ignore_case("connection") == 0 ||
            header.first.compare_ignore_case("content-length") == 0 ||
            header.first.compare_ignore_case("transfer-encoding") == 0 ||
            header.first.compare_ignore_case("expect") == 0 ||
            header.first.compare_ignore_case("proxy-connection") == 0 ||
            header.first.compare_ignore_case("keep-alive") == 0 || header.first.compare_ignore_case("te") == 0 ||
            header.first.compare_ignore_case("trailer") == 0 || header.first.compare_ignore_case("upgrade") == 0 ||
            header.first.compare_ignore_case("x-forwarded-for") == 0 ||
            header.first.compare_ignore_case("x-forwarded-proto") == 0 ||
            header.first.compare_ignore_case("x-forwarded-host") == 0) {
            continue;
        }
        result += header.first;
        result += ": ";
        result += header.second;
        result += "\r\n";
    }

    const string_view existing_fwd = request.header_ci("X-Forwarded-For");
    const string_view client_ip = request.client_ip();
    if (!existing_fwd.empty() || !client_ip.empty()) {
        result += "X-Forwarded-For: ";
        result += existing_fwd;
        if (!existing_fwd.empty() && !client_ip.empty()) {
            result += ", ";
        }
        result += client_ip;
        result += "\r\n";
    }
    result += "X-Forwarded-Proto: ";
    result += backend.scheme;
    result += "\r\nX-Forwarded-Host: ";
    result += request.header_ci("Host");
    result += "\r\n";

    if (!request.body.empty()) {
        result += "Content-Length: ";
        result += to_string(request.body.size());
        result += "\r\n";
    }
    result += "Connection: keep-alive\r\n\r\n";
    if (!request.body.empty()) {
        result += request.body;
    }
    return result;
}

void reverse_proxy_filter::pre_filter_async(http_request& request, http_response& response, http_context& /*ctx*/,
                                            async_filter::next_callback next) {
    if (!should_proxy(request.path)) {
        next(true);
        return;
    }

    if (direct_backend || !enable_async_backend) {
        const bool should_continue = pre_filter(request, response);
        next(should_continue);
        return;
    }

    const auto backend = select_backend(request);
    if (backend.scheme == "https") {
        const bool should_continue = pre_filter(request, response);
        next(should_continue);
        return;
    }
    if (backend.host.empty()) {
        response.status = http_status::bad_gateway;
        response.status_message = "Bad Gateway";
        response.body = "No backend available";
        response.set_content_type(http_content::PLAIN_TEXT());
        next(false);
        return;
    }

    string request_data;
    try {
        request_data = build_async_request(backend, request);
    } catch (const exception& e) {
        response.status = http_status::bad_gateway;
        response.status_message = "Bad Gateway";
        response.body = string("Proxy request error: ") + e.what();
        response.set_content_type(http_content::PLAIN_TEXT());
        next(false);
        return;
    }

    vector<string> skipped_headers;
    {
        lock<mutex> lk(mutex_);
        skipped_headers = skip_resp_headers_;
    }

    const bool raw = raw_response_passthrough;
    const bool head = request.method.is_head();
    const string key = backend.scheme + "://" + backend.host + ":" + to_string(backend.port.value());

    async_backend_connection::options options;
    options.max_connections = max_connections_per_backend;
    options.max_queue = max_queue_per_backend;
    options.max_idle = async_max_idle_connections;
    options.max_response_size = async_max_response_size;
    options.verify_ssl = verify_backend_ssl;
    options.connect_timeout = connect_timeout;
    options.receive_timeout = receive_timeout;

    shared_ptr<reverse_proxy_async_state> state;
    try {
        state = ensure_async_state();
    } catch (const exception& e) {
        response.status = http_status::bad_gateway;
        response.status_message = "Bad Gateway";
        response.body = string("Async proxy initialization failed: ") + e.what();
        response.set_content_type(http_content::PLAIN_TEXT());
        next(false);
        return;
    }

    state->requests_.fetch_add(1, memory_order_relaxed);
    state->acquire(backend, options,
                   // NOLINTNEXTLINE(bugprone-exception-escape)
                   [state, key, request_data = move(request_data), raw, head, skipped_headers, &response,
                    next = move(next)](shared_ptr<async_backend_connection> connection, error_code ec) mutable {
                       if (ec || !connection) {
                           state->errors_.fetch_add(1, memory_order_relaxed);
                           response.status = http_status::bad_gateway;
                           response.status_message = "Bad Gateway";
                           response.body = "Backend connection failed";
                           response.set_content_type(http_content::PLAIN_TEXT());
                           next(false);
                           return;
                       }

                       auto connection_holder = connection;
                       connection->start_request(
                               move(request_data), head,
                               // NOLINTNEXTLINE(bugprone-exception-escape)
                               [state, key, &response, raw, skipped_headers, next = move(next), connection_holder](
                                       error_code error, shared_ptr<async_backend_response> backend_response) mutable {
                                   if (error || !backend_response) {
                                       state->errors_.fetch_add(1, memory_order_relaxed);
                                       state->release(key, connection_holder, false);
                                       response.status = http_status::bad_gateway;
                                       response.status_message = "Bad Gateway";
                                       response.body = "Backend request failed";
                                       response.set_content_type(http_content::PLAIN_TEXT());
                                       next(false);
                                       return;
                                   }

                                   if (raw) {
                                       response.close_after_send = !backend_response->keep_alive;
                                       response.raw_response = move(backend_response->raw);
                                       filter_raw_headers_impl(response.raw_response, skipped_headers);
                                   } else {
                                       response.status = backend_response->response.status;
                                       response.status_message = move(backend_response->response.status_message);
                                       if (response.status_message.empty()) {
                                           response.status_message = http_status_message(response.status);
                                       }
                                       response.version = move(backend_response->response.version);
                                       response.headers = move(backend_response->response.headers);
                                       response.body = move(backend_response->response.body);
                                       response.close_after_send = !backend_response->keep_alive;

                                       for (auto it = response.headers.begin(); it != response.headers.end();) {
                                           bool skip = false;
                                           for (const auto& skipped: skipped_headers) {
                                               if (it->first.compare_ignore_case(skipped.view()) == 0) {
                                                   skip = true;
                                                   break;
                                               }
                                           }
                                           if (skip) {
                                               it = response.headers.erase(it);
                                           } else {
                                               ++it;
                                           }
                                       }
                                   }

                                   state->release(key, connection_holder,
                                                  backend_response->keep_alive && connection_holder->is_connected());
                                   next(false);
                               });
                   });
}

bool reverse_proxy_filter::pre_filter(http_request& request, http_response& response) {
    if (!should_proxy(request.path)) {
        return true;
    }

    request_count_.fetch_add(1, memory_order_relaxed);

    if (direct_backend) {
        direct_request_count_.fetch_add(1, memory_order_relaxed);
        try {
            direct_backend(request, response);
        } catch (const exception& e) {
            response.status = http_status::bad_gateway;
            response.status_message = "Bad Gateway";
            response.body = string("Direct backend error: ") + e.what();
            response.set_content_type(http_content::PLAIN_TEXT());
        }
        return false;
    }

    const auto backend = select_backend(request);
    if (backend.host.empty()) {
        response.status = http_status::bad_gateway;
        response.status_message = "Bad Gateway";
        response.body = "No backend available";
        response.set_content_type(http_content::PLAIN_TEXT());
        return false;
    }

    try {
        if (raw_response_passthrough) {
            auto raw = forward_raw(backend, request);
            if (raw.data.empty()) {
                response.status = http_status::bad_gateway;
                response.status_message = "Bad Gateway";
                response.body = "Backend returned an empty response";
                response.set_content_type(http_content::PLAIN_TEXT());
                return false;
            }
            response.close_after_send = !raw.keep_alive;
            response.raw_response = move(raw.data);
            return false;
        }

        auto backend_resp = forward(backend, request);
        copy_response(move(backend_resp), response);
    } catch (const exception& e) {
        response.status = http_status::bad_gateway;
        response.status_message = "Bad Gateway";
        response.body = string("Proxy error: ") + e.what();
        response.set_content_type(http_content::PLAIN_TEXT());
    }

    return false;
}

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
