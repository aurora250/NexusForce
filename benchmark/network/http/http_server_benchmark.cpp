// http_server QPS / latency benchmark.
//
// Each worker keeps a raw HTTP/1.1 connection open and measures a full send +
// response round trip. The /bench handler never touches request.session, so
// lazy sessions are not allocated in the keep-alive cases. "NewConnection"
// opens a TCP connection for every request; TLS and HTTP/2 cases use the local
// test certificate, and the proxy case runs a front-end reverse_proxy_filter
// in front of a loopback back-end.
//
// Run: NexusForceHttpServerBenchmark --benchmark_min_time=1s
#include <NeForce/core/async/io_context.hpp>
#include <NeForce/core/time/clocks.hpp>
#include <NeForce/core/system/environment.hpp>
#include <NeForce/network/http/http_server.hpp>
#include <NeForce/network/http/http2_protocol.hpp>
#include <NeForce/network/http/reverse_proxy.hpp>
#include <NeForce/network/ssl/ssl_context.hpp>
#include <NeForce/network/tcp/tcp_client.hpp>
#include <benchmark/benchmark.h>
using namespace neforce;
using namespace neforce::http;

namespace {
    using client_factory = function<unique_ptr<tcp_client_base>(io_context&)>;

    NEFORCE_NODISCARD client_factory make_plain_client_factory() {
        return [](io_context& ctx) -> unique_ptr<tcp_client_base> { return make_unique<tcp_client>(ctx); };
    }

    NEFORCE_NODISCARD client_factory make_tls_client_factory(const bool tls12 = false,
                                                             shared_ptr<ssl_session_cache>* cache_out = nullptr) {
        auto session_cache = make_shared<ssl_session_cache>();
        if (cache_out != nullptr) {
            *cache_out = session_cache;
        }
        auto shared_ssl = make_shared<ssl_context>(ssl_method::tls_client);
        shared_ssl->enable_session_reuse(true);
        if (tls12) {
            shared_ssl->enable_tls12_short_connection_acceleration();
        }
        return [session_cache, shared_ssl](io_context& ctx) -> unique_ptr<tcp_client_base> {
            auto client = make_unique<ssl_client>(ctx, shared_ssl->share());
            client->set_verify_peer(false);
            client->set_session_cache(session_cache);
            return client;
        };
    }
    struct worker_result {
        int64_t requests{0};
        int64_t failures{0};
        int64_t reused_sessions{0};
        vector<uint32_t> latency_us;
    };

    class response_reader {
    public:
        explicit response_reader(tcp_client_base& client) :
        client_(client) {}

        NEFORCE_NODISCARD bool read(size_t& body_size) {
            for (;;) {
                const size_t header_end = buffer_.find("\r\n\r\n", scan_offset_);
                if (header_end != string::npos) {
                    const size_t content_length = parse_content_length(scan_offset_, header_end);
                    const size_t response_end = header_end + 4 + content_length;
                    while (buffer_.size() < response_end) {
                        if (!receive_more()) {
                            return false;
                        }
                    }
                    body_size = content_length;
                    parse_session_cookie(scan_offset_, header_end);
                    scan_offset_ = response_end;
                    if (scan_offset_ >= 64 * 1024) {
                        buffer_.erase(0, scan_offset_);
                        scan_offset_ = 0;
                    }
                    return true;
                }
                if (!receive_more()) {
                    return false;
                }
            }
        }

        NEFORCE_NODISCARD const string& session_cookie() const noexcept { return session_cookie_; }

    private:
        void parse_session_cookie(const size_t begin, const size_t end) {
            if (!session_cookie_.empty()) {
                return;
            }
            const string_view head = buffer_.view(begin, end - begin);
            const size_t key = head.find("Set-Cookie:");
            if (key == string_view::npos) {
                return;
            }
            size_t pos = key + 11;
            while (pos < head.size() && (head[pos] == ' ' || head[pos] == '\t')) {
                ++pos;
            }
            const size_t value_end = head.find(';', pos);
            if (value_end == string_view::npos) {
                return;
            }
            session_cookie_ = head.view(pos, value_end - pos).trim();
        }

        NEFORCE_NODISCARD size_t parse_content_length(const size_t begin, const size_t end) const {
            const string_view head = buffer_.view(begin, end - begin);
            const size_t key = head.find("Content-Length:");
            if (key == string_view::npos) {
                return 0;
            }
            size_t pos = key + 15;
            while (pos < head.size() && (head[pos] == ' ' || head[pos] == '\t')) {
                ++pos;
            }
            size_t value = 0;
            while (pos < head.size() && head[pos] >= '0' && head[pos] <= '9') {
                value = value * 10 + static_cast<size_t>(head[pos] - '0');
                ++pos;
            }
            return value;
        }

        NEFORCE_NODISCARD bool receive_more() {
            char chunk[16 * 1024];
            const ssize_t received = client_.receive(chunk, sizeof(chunk));
            if (received <= 0) {
                return false;
            }
            buffer_.append(chunk, static_cast<size_t>(received));
            return true;
        }

        tcp_client_base& client_;
        string buffer_;
        size_t scan_offset_{0};
        string session_cookie_;
    };

    NEFORCE_NODISCARD string make_request(const bool keep_alive, const string_view cookie = {}) {
        string request = string("GET /bench HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: ") +
                         (keep_alive ? "keep-alive" : "close") + "\r\n";
        if (!cookie.empty()) {
            request += "Cookie: " + string(cookie) + "\r\n";
        }
        request += "\r\n";
        return request;
    }

    NEFORCE_NODISCARD uint32_t elapsed_us(const steady_clock::time_point& begin, const steady_clock::time_point& end) {
        return static_cast<uint32_t>(duration<double, micro>(end - begin).count());
    }

    void run_keep_alive_worker(const ports port, const client_factory& factory, const string& base_request,
                               const int64_t request_count, const size_t expected_body_size, worker_result& result) {
        io_context ctx;
        result.latency_us.reserve(static_cast<size_t>(request_count));

        int64_t completed = 0;
        int64_t reconnects = 0;
        // A server may close an idle or keep-alive-limited connection; reconnect
        // and resume rather than treating the peer close as a failed request.
        const int64_t max_reconnects = request_count / 100 + 16;

        while (completed < request_count) {
            auto client = factory(ctx);
            if (!client->connect("127.0.0.1", port)) {
                result.failures = request_count - completed;
                break;
            }
            client->socket().set_tcp_nodelay(true);
            client->socket().set_receive_timeout(milliseconds(10000));
            if (auto* ssl_sock = dynamic_cast<ssl_socket*>(&client->socket());
                ssl_sock != nullptr && ssl_sock->session_reused()) {
                ++result.reused_sessions;
            }

            response_reader reader(*client);
            string request = base_request;
            bool connection_lost = false;
            for (; completed < request_count; ++completed) {
                try {
                    const auto begin = steady_clock::now();
                    if (!client->send_all(request.view())) {
                        connection_lost = true;
                        break;
                    }
                    size_t body_size = 0;
                    if (!reader.read(body_size) || body_size != expected_body_size) {
                        connection_lost = true;
                        break;
                    }
                    const auto end = steady_clock::now();
                    result.latency_us.push_back(elapsed_us(begin, end));
                    ++result.requests;
                    if (!reader.session_cookie().empty()) {
                        request = make_request(true, reader.session_cookie().view());
                    }
                } catch (...) {
                    connection_lost = true;
                    break;
                }
            }

            client->disconnect();
            if (!connection_lost) {
                break;
            }
            if (++reconnects > max_reconnects) {
                result.failures = request_count - completed;
                break;
            }
        }
    }

    void run_pipeline_worker(const ports port, const client_factory& factory, const string& request,
                             const int64_t request_count, const size_t expected_body_size, worker_result& result) {
        io_context ctx;
        auto client = factory(ctx);
        if (!client->connect("127.0.0.1", port)) {
            result.failures = request_count;
            return;
        }
        client->socket().set_tcp_nodelay(true);
        client->socket().set_nonblocking(false);
        client->socket().set_receive_timeout(milliseconds(30000));

        string pipeline;
        pipeline.reserve(request.size() * static_cast<size_t>(request_count));
        for (int64_t i = 0; i < request_count; ++i) {
            pipeline += request;
        }

        const auto begin = steady_clock::now();
        if (!client->send_all(pipeline.view())) {
            result.failures = request_count;
            client->disconnect();
            return;
        }

        response_reader reader(*client);
        auto previous = begin;
        for (int64_t i = 0; i < request_count; ++i) {
            size_t body_size = 0;
            if (!reader.read(body_size) || body_size != expected_body_size) {
                ++result.failures;
                break;
            }
            const auto now = steady_clock::now();
            result.latency_us.push_back(elapsed_us(previous, now));
            previous = now;
            ++result.requests;
        }
        client->disconnect();
    }

    void run_new_connection_worker(const ports port, const client_factory& factory, const string& request,
                                   const int64_t request_count, const size_t expected_body_size,
                                   worker_result& result) {
        io_context ctx;
        result.latency_us.reserve(static_cast<size_t>(request_count));

        for (int64_t i = 0; i < request_count; ++i) {
            auto client = factory(ctx);
            const auto begin = steady_clock::now();
            if (!client->connect("127.0.0.1", port)) {
                ++result.failures;
                break;
            }
            client->socket().set_tcp_nodelay(true);
            client->socket().set_receive_timeout(milliseconds(10000));
            if (auto* ssl_sock = dynamic_cast<ssl_socket*>(&client->socket());
                ssl_sock != nullptr && ssl_sock->session_reused()) {
                ++result.reused_sessions;
            }

            if (!client->send_all(request.view())) {
                ++result.failures;
                break;
            }
            response_reader reader(*client);
            size_t body_size = 0;
            if (!reader.read(body_size) || body_size != expected_body_size) {
                ++result.failures;
                break;
            }
            const auto end = steady_clock::now();
            result.latency_us.push_back(elapsed_us(begin, end));
            ++result.requests;
            client->disconnect();
        }
    }

    struct h2_frame {
        uint8_t type{0};
        uint8_t flags{0};
        uint32_t stream_id{0};
        vector<byte_t> payload;
    };

    NEFORCE_NODISCARD bool send_h2_frame(tcp_client_base& client, const byte_vector& frame) {
        return client.send_all(string_view(reinterpret_cast<const char*>(frame.data()), frame.size()));
    }

    NEFORCE_NODISCARD bool receive_h2_frame(tcp_client_base& client, h2_frame& frame) {
        byte_t header[9];
        if (!client.receive_exact(memory_view<char>(reinterpret_cast<char*>(header), sizeof(header)))) {
            return false;
        }
        const uint32_t length = (static_cast<uint32_t>(header[0]) << 16) | (static_cast<uint32_t>(header[1]) << 8) |
                                static_cast<uint32_t>(header[2]);
        frame.type = header[3];
        frame.flags = header[4];
        frame.stream_id = ((static_cast<uint32_t>(header[5]) & 0x7F) << 24) | (static_cast<uint32_t>(header[6]) << 16) |
                          (static_cast<uint32_t>(header[7]) << 8) | static_cast<uint32_t>(header[8]);
        frame.payload.resize(length);
        return !(length > 0 &&
                 !client.receive_exact(memory_view<char>(reinterpret_cast<char*>(frame.payload.data()), length)));
    }

    NEFORCE_NODISCARD byte_vector h2_encode_request(hpack_encoder& encoder, const uint32_t stream_id) {
        vector<hpack_header_field> headers;
        headers.push_back({":method", "GET"});
        headers.push_back({":scheme", "https"});
        headers.push_back({":authority", "127.0.0.1"});
        headers.push_back({":path", "/bench"});

        http2_headers_frame request_frame;
        request_frame.stream_id = stream_id;
        request_frame.header_block = encoder.encode(headers);
        request_frame.end_headers = true;
        request_frame.end_stream = true;
        return http2_framer::encode_headers_frame(request_frame);
    }

    NEFORCE_NODISCARD bool h2_send_request(tcp_client_base& client, hpack_encoder& encoder, const uint32_t stream_id) {
        return send_h2_frame(client, h2_encode_request(encoder, stream_id));
    }

    NEFORCE_NODISCARD bool h2_request(tcp_client_base& client, hpack_encoder& encoder, const uint32_t stream_id,
                                      const size_t expected_body_size, size_t& received_body) {
        if (!h2_send_request(client, encoder, stream_id)) {
            return false;
        }

        received_body = 0;
        for (;;) {
            h2_frame frame;
            if (!receive_h2_frame(client, frame)) {
                return false;
            }

            if (frame.type == static_cast<uint8_t>(http2_frame_type::settings) && (frame.flags & HTTP2_FLAG_ACK) == 0) {
                http2_settings_frame ack;
                ack.ack = true;
                if (!send_h2_frame(client, http2_framer::encode_settings_frame(ack))) {
                    return false;
                }
            }

            if (frame.type == static_cast<uint8_t>(http2_frame_type::rst_stream) && frame.stream_id == stream_id) {
                return false;
            }
            if (frame.type == static_cast<uint8_t>(http2_frame_type::goaway)) {
                return false;
            }

            if (frame.stream_id != stream_id) {
                continue;
            }
            if (frame.type == static_cast<uint8_t>(http2_frame_type::data)) {
                received_body += frame.payload.size();
            }
            if ((frame.flags & HTTP2_FLAG_END_STREAM) != 0) {
                return received_body == expected_body_size;
            }
        }
    }

    NEFORCE_NODISCARD bool h2_multiplex_batch(tcp_client_base& client, hpack_encoder& encoder, const int64_t batch_size,
                                              const size_t expected_body_size, uint32_t& next_stream_id,
                                              worker_result& result) {
        const auto begin = steady_clock::now();
        vector<uint32_t> stream_ids;
        vector<size_t> received_bodies;
        stream_ids.reserve(static_cast<size_t>(batch_size));
        received_bodies.resize(static_cast<size_t>(batch_size), 0);

        byte_vector request_batch;
        request_batch.reserve(static_cast<size_t>(batch_size) * 64);
        for (int64_t i = 0; i < batch_size; ++i) {
            const uint32_t stream_id = next_stream_id;
            next_stream_id += 2;
            const byte_vector frame = h2_encode_request(encoder, stream_id);
            request_batch.insert(request_batch.end(), frame.begin(), frame.end());
            stream_ids.push_back(stream_id);
        }
        if (!send_h2_frame(client, request_batch)) {
            return false;
        }

        int64_t completed = 0;
        size_t connection_received = 0;
        while (completed < batch_size) {
            h2_frame frame;
            if (!receive_h2_frame(client, frame)) {
                return false;
            }

            if (frame.type == static_cast<uint8_t>(http2_frame_type::settings) && (frame.flags & HTTP2_FLAG_ACK) == 0) {
                http2_settings_frame ack;
                ack.ack = true;
                if (!send_h2_frame(client, http2_framer::encode_settings_frame(ack))) {
                    return false;
                }
            }
            if (frame.type == static_cast<uint8_t>(http2_frame_type::goaway)) {
                return false;
            }

            size_t stream_index = stream_ids.size();
            for (size_t i = 0; i < stream_ids.size(); ++i) {
                if (stream_ids[i] == frame.stream_id) {
                    stream_index = i;
                    break;
                }
            }
            if (stream_index == stream_ids.size()) {
                if (frame.type == static_cast<uint8_t>(http2_frame_type::data) && frame.stream_id == 0) {
                    http2_window_update_frame window;
                    window.stream_id = 0;
                    window.window_size_increment = static_cast<uint32_t>(frame.payload.size());
                    if (!send_h2_frame(client, http2_framer::encode_window_update_frame(window))) {
                        return false;
                    }
                }
                continue;
            }

            if (frame.type == static_cast<uint8_t>(http2_frame_type::rst_stream)) {
                return false;
            }
            if (frame.type == static_cast<uint8_t>(http2_frame_type::data)) {
                received_bodies[stream_index] += frame.payload.size();
                connection_received += frame.payload.size();
            }
            if ((frame.flags & HTTP2_FLAG_END_STREAM) != 0) {
                if (received_bodies[stream_index] != expected_body_size) {
                    return false;
                }
                const auto end = steady_clock::now();
                result.latency_us.push_back(elapsed_us(begin, end));
                ++result.requests;
                ++completed;
            }
        }

        if (connection_received > 0) {
            http2_window_update_frame window;
            window.stream_id = 0;
            window.window_size_increment = static_cast<uint32_t>(connection_received);
            if (!send_h2_frame(client, http2_framer::encode_window_update_frame(window))) {
                return false;
            }
        }
        return true;
    }

    void run_h2_worker(const ports port, const int64_t request_count, const size_t expected_body_size,
                       const int64_t concurrent_streams, worker_result& result) {
        io_context ctx;
        ssl_context ssl(ssl_method::tls_client);
        ssl.set_alpn_protos({"h2"});
        ssl_client client(ctx, _NEFORCE move(ssl));
        client.set_verify_peer(false);
        if (!client.connect("127.0.0.1", port)) {
            result.failures = request_count;
            return;
        }
        client.socket().set_tcp_nodelay(true);
        client.socket().set_nonblocking(false);
        client.socket().set_receive_timeout(milliseconds(10000));

        if (!client.send_all(HTTP2_CLIENT_PREFACE)) {
            result.failures = request_count;
            return;
        }
        http2_settings_frame settings;
        settings.entries.push_back({http2_settings_id::initial_window_size, 1U << 20});
        settings.entries.push_back({http2_settings_id::max_frame_size, 16384});
        settings.entries.push_back({http2_settings_id::max_concurrent_streams,
                                    static_cast<uint32_t>(max<int64_t>(1, concurrent_streams))});
        if (!send_h2_frame(client, http2_framer::encode_settings_frame(settings))) {
            result.failures = request_count;
            return;
        }

        // Complete the SETTINGS handshake before opening the first stream.
        h2_frame server_settings_frame;
        if (receive_h2_frame(client, server_settings_frame) &&
            server_settings_frame.type == static_cast<uint8_t>(http2_frame_type::settings)) {
            http2_settings_frame ack;
            ack.ack = true;
            if (!send_h2_frame(client, http2_framer::encode_settings_frame(ack))) {
                result.failures = request_count;
                return;
            }
        }

        hpack_encoder encoder;
        result.latency_us.reserve(static_cast<size_t>(request_count));
        uint32_t stream_id = 1;

        if (concurrent_streams > 1) {
            http2_window_update_frame connection_window;
            connection_window.stream_id = 0;
            connection_window.window_size_increment = 1U << 20;
            if (!send_h2_frame(client, http2_framer::encode_window_update_frame(connection_window))) {
                result.failures = request_count;
                client.disconnect();
                return;
            }

            int64_t remaining = request_count;
            while (remaining > 0) {
                const int64_t batch = min<int64_t>(remaining, concurrent_streams);
                if (!h2_multiplex_batch(client, encoder, batch, expected_body_size, stream_id, result)) {
                    ++result.failures;
                    break;
                }
                remaining -= batch;
            }
        } else {
            for (int64_t i = 0; i < request_count; ++i) {
                const auto begin = steady_clock::now();
                size_t received_body = 0;
                const bool ok = h2_request(client, encoder, stream_id, expected_body_size, received_body);
                const auto end = steady_clock::now();
                if (!ok) {
                    ++result.failures;
                    break;
                }
                result.latency_us.push_back(elapsed_us(begin, end));
                ++result.requests;
                stream_id += 2;
            }
        }
        client.disconnect();
    }

    NEFORCE_NODISCARD int64_t batch_size_for(const int64_t payload_size) {
        if (payload_size >= 32L * 1024L) {
            return 300;
        }
        if (payload_size >= 4L * 1024L) {
            return 1000;
        }
        return 3000;
    }

    void publish_latency(benchmark::State& state, const vector<uint32_t>& samples) {
        if (samples.empty()) {
            return;
        }

        double sum = 0.0;
        for (const auto sample: samples) {
            sum += static_cast<double>(sample);
        }
        const auto percentile = [&samples](const double p) {
            size_t index = static_cast<size_t>(p * static_cast<double>(samples.size()));
            if (index >= samples.size()) {
                index = samples.size() - 1;
            }
            return samples[index];
        };

        state.counters["mean_us"] = benchmark::Counter(sum / static_cast<double>(samples.size()));
        state.counters["p50_us"] = benchmark::Counter(static_cast<double>(percentile(0.50)));
        state.counters["p90_us"] = benchmark::Counter(static_cast<double>(percentile(0.90)));
        state.counters["p99_us"] = benchmark::Counter(static_cast<double>(percentile(0.99)));
        state.counters["max_us"] = benchmark::Counter(static_cast<double>(samples.back()));
    }

    NEFORCE_NODISCARD string benchmark_cert_path() {
        string path = environment::get("NEXUSFORCE_BENCH_CERT");
        return !path.empty() ? move(path) : "/home/huenqi/server.crt";
    }

    NEFORCE_NODISCARD string benchmark_key_path() {
        string path = environment::get("NEXUSFORCE_BENCH_KEY");
        return !path.empty() ? move(path) : "/home/huenqi/server.key";
    }

    void run_server_benchmark(benchmark::State& state, const int64_t client_count, const int64_t payload_size,
                              const int64_t server_threads, const bool keep_alive, const bool tls,
                              const bool pipelined = false, const bool tls12 = false) {
        const auto clients = static_cast<size_t>(max<int64_t>(1, client_count));
        const auto workers = static_cast<size_t>(max<int64_t>(1, server_threads));
        const auto body = make_shared<string>(static_cast<size_t>(max<int64_t>(0, payload_size)), 'x');

        io_context server_ctx;
        unique_ptr<http_server> server;
        if (tls) {
            ssl_context ssl(ssl_method::tls_server);
            ssl.enable_session_reuse(true);
            if (tls12) {
                ssl.enable_tls12_short_connection_acceleration();
            }
            if (!ssl.load_certificate(benchmark_cert_path(), benchmark_key_path())) {
                state.SkipWithError("TLS benchmark certificate not found; set NEXUSFORCE_BENCH_CERT / "
                                    "NEXUSFORCE_BENCH_KEY");
                return;
            }
            server = make_unique<http_server>(ports(0), server_ctx, _NEFORCE move(ssl), workers);
        } else {
            server = make_unique<http_server>(ports(0), server_ctx, workers);
        }
        server->max_keep_alive_requests = 1000000;
        server->router().get("/bench", [body](http_request&, http_response& response) {
            response.status = http_status::ok;
            response.set_body_view(body->view());
        });
        if (!server->start()) {
            state.SkipWithError("http_server failed to bind loopback port");
            return;
        }
        const ports port = server->port();
        shared_ptr<ssl_session_cache> session_cache;
        const client_factory factory =
                tls ? make_tls_client_factory(tls12, &session_cache) : make_plain_client_factory();

        const string request = make_request(keep_alive);
        const int64_t per_thread = keep_alive ? batch_size_for(payload_size) : 100;

        vector<worker_result> results(clients);
        for (auto& result: results) {
            result.latency_us.reserve(static_cast<size_t>(per_thread));
        }

        vector<uint32_t> all_latencies;
        all_latencies.reserve(static_cast<size_t>(per_thread * static_cast<int64_t>(clients) * 4));

        int64_t total_requests = 0;
        int64_t total_failures = 0;
        int64_t reused_sessions = 0;
        double total_elapsed = 0.0;

        for (auto _: state) {
            for (auto& result: results) {
                result.requests = 0;
                result.failures = 0;
                result.reused_sessions = 0;
                result.latency_us.clear();
            }

            const auto begin = steady_clock::now();
            vector<thread> threads;
            threads.reserve(clients);
            for (size_t i = 0; i < clients; ++i) {
                threads.emplace_back([&, i] {
                    try {
                        if (pipelined) {
                            run_pipeline_worker(port, factory, request, per_thread, static_cast<size_t>(payload_size),
                                                results[i]);
                        } else if (keep_alive) {
                            run_keep_alive_worker(port, factory, request, per_thread, static_cast<size_t>(payload_size),
                                                  results[i]);
                        } else {
                            run_new_connection_worker(port, factory, request, per_thread,
                                                      static_cast<size_t>(payload_size), results[i]);
                        }
                    } catch (...) {
                        results[i].failures = per_thread;
                    }
                });
            }
            for (auto& thread: threads) {
                thread.join();
            }
            const auto end = steady_clock::now();
            const double elapsed = duration<double>(end - begin).count();
            total_elapsed += elapsed;
            state.SetIterationTime(elapsed);

            for (const auto& result: results) {
                total_requests += result.requests;
                total_failures += result.failures;
                reused_sessions += result.reused_sessions;
                all_latencies.insert(all_latencies.end(), result.latency_us.begin(), result.latency_us.end());
            }
        }

        server->stop();

        sort(all_latencies.begin(), all_latencies.end());
        publish_latency(state, all_latencies);

        if (total_elapsed > 0.0) {
            state.counters["QPS"] = benchmark::Counter(static_cast<double>(total_requests) / total_elapsed);
        }
        state.counters["failures"] = benchmark::Counter(static_cast<double>(total_failures));
        state.counters["tls_reused"] = benchmark::Counter(static_cast<double>(reused_sessions));
        if (session_cache) {
            const auto cache_stats = session_cache->stats();
            state.counters["tls_cache_hits"] = benchmark::Counter(static_cast<double>(cache_stats.hits));
            state.counters["tls_cache_misses"] = benchmark::Counter(static_cast<double>(cache_stats.misses));
            state.counters["tls_tickets_stored"] = benchmark::Counter(static_cast<double>(cache_stats.stored));
            state.counters["tls_tickets_evicted"] = benchmark::Counter(static_cast<double>(cache_stats.evicted));
        }
        state.counters["clients"] = benchmark::Counter(static_cast<double>(clients));
        state.counters["payload_bytes"] = benchmark::Counter(static_cast<double>(payload_size));
        state.counters["server_threads"] = benchmark::Counter(static_cast<double>(workers));
        state.SetItemsProcessed(total_requests);
        state.SetBytesProcessed(total_requests * payload_size);
        state.SetLabel("clients=" + std::to_string(clients) + " payload=" + std::to_string(payload_size) +
                       "B server_threads=" + std::to_string(workers) +
                       (tls ? (tls12 ? " tls12_reuse_accel" : " tls") : " plain"));
    }
    void run_direct_proxy_benchmark(benchmark::State& state, const int64_t client_count, const int64_t payload_size,
                                    const int64_t server_threads) {
        const auto clients = static_cast<size_t>(max<int64_t>(1, client_count));
        const auto workers = static_cast<size_t>(max<int64_t>(1, server_threads));
        const auto body = make_shared<string>(static_cast<size_t>(max<int64_t>(0, payload_size)), 'x');

        io_context proxy_ctx;
        http_server proxy(ports(0), proxy_ctx, workers);
        proxy.max_keep_alive_requests = 1000000;
        auto proxy_filter = make_unique<reverse_proxy_filter>(proxy_ctx);
        proxy_filter->direct_backend = [body](http_request&, http_response& response) {
            response.status = http_status::ok;
            response.body = *body;
            response.set_header("X-Backend-Debug", "1");
        };
        auto* proxy_filter_ptr = proxy_filter.get();
        proxy.router().use(move(proxy_filter));
        if (!proxy.start()) {
            state.SkipWithError("direct proxy frontend failed to bind");
            return;
        }
        const ports port = proxy.port();
        const string request = make_request(true);
        const int64_t per_thread = batch_size_for(payload_size);
        const client_factory factory = make_plain_client_factory();
        vector<worker_result> results(clients);
        for (auto& result: results) {
            result.latency_us.reserve(static_cast<size_t>(per_thread));
        }
        vector<uint32_t> all_latencies;
        int64_t total_requests = 0;
        int64_t total_failures = 0;
        double total_elapsed = 0.0;
        for (auto _: state) {
            for (auto& result: results) {
                result.requests = 0;
                result.failures = 0;
                result.latency_us.clear();
            }
            const auto begin = steady_clock::now();
            vector<thread> threads;
            threads.reserve(clients);
            for (size_t i = 0; i < clients; ++i) {
                threads.emplace_back([&, i] {
                    try {
                        run_keep_alive_worker(port, factory, request, per_thread, static_cast<size_t>(payload_size),
                                              results[i]);
                    } catch (...) {
                        results[i].failures = per_thread;
                    }
                });
            }
            for (auto& thread: threads) {
                thread.join();
            }
            const auto end = steady_clock::now();
            const double elapsed = duration<double>(end - begin).count();
            total_elapsed += elapsed;
            state.SetIterationTime(elapsed);
            for (const auto& result: results) {
                total_requests += result.requests;
                total_failures += result.failures;
                all_latencies.insert(all_latencies.end(), result.latency_us.begin(), result.latency_us.end());
            }
        }
        const auto stats = proxy_filter_ptr->stats();
        proxy.stop();
        sort(all_latencies.begin(), all_latencies.end());
        publish_latency(state, all_latencies);
        if (total_elapsed > 0.0) {
            state.counters["QPS"] = benchmark::Counter(static_cast<double>(total_requests) / total_elapsed);
        }
        state.counters["failures"] = benchmark::Counter(static_cast<double>(total_failures));
        state.counters["clients"] = benchmark::Counter(static_cast<double>(clients));
        state.counters["payload_bytes"] = benchmark::Counter(static_cast<double>(payload_size));
        state.counters["server_threads"] = benchmark::Counter(static_cast<double>(workers));
        state.counters["direct_requests"] = benchmark::Counter(static_cast<double>(stats.direct_requests));
        state.SetItemsProcessed(total_requests);
        state.SetBytesProcessed(total_requests * payload_size);
        state.SetLabel("in_process_proxy clients=" + std::to_string(clients) +
                       " payload=" + std::to_string(payload_size) + "B server_threads=" + std::to_string(workers));
    }

    void run_reverse_proxy_benchmark(benchmark::State& state, const int64_t client_count, const int64_t payload_size,
                                     const int64_t server_threads, const bool frontend_tls, const bool backend_tls,
                                     const bool async = false) {
        const auto clients = static_cast<size_t>(max<int64_t>(1, client_count));
        const auto workers = static_cast<size_t>(max<int64_t>(1, server_threads));
        const auto body = make_shared<string>(static_cast<size_t>(max<int64_t>(0, payload_size)), 'x');

        io_context backend_ctx;
        unique_ptr<http_server> backend;
        if (backend_tls) {
            ssl_context ssl(ssl_method::tls_server);
            ssl.enable_session_reuse(true);
            if (!ssl.load_certificate(benchmark_cert_path(), benchmark_key_path())) {
                state.SkipWithError("HTTPS proxy certificate not found; set NEXUSFORCE_BENCH_CERT / "
                                    "NEXUSFORCE_BENCH_KEY");
                return;
            }
            backend = make_unique<http_server>(ports(0), backend_ctx, _NEFORCE move(ssl), workers);
        } else {
            backend = make_unique<http_server>(ports(0), backend_ctx, workers);
        }
        backend->max_keep_alive_requests = 1000000;
        backend->enable_async_http1 = async;
        backend->router().get("/bench", [body](http_request&, http_response& response) {
            response.status = http_status::ok;
            response.body = *body;
            response.set_header("X-Backend-Debug", "1");
        });
        if (!backend->start()) {
            state.SkipWithError("reverse proxy backend failed to bind");
            return;
        }

        io_context proxy_ctx;
        unique_ptr<http_server> proxy;
        if (frontend_tls) {
            ssl_context ssl(ssl_method::tls_server);
            ssl.enable_session_reuse(true);
            if (!ssl.load_certificate(benchmark_cert_path(), benchmark_key_path())) {
                backend->stop();
                state.SkipWithError("HTTPS proxy certificate not found; set NEXUSFORCE_BENCH_CERT / "
                                    "NEXUSFORCE_BENCH_KEY");
                return;
            }
            proxy = make_unique<http_server>(ports(0), proxy_ctx, _NEFORCE move(ssl), workers);
        } else {
            proxy = make_unique<http_server>(ports(0), proxy_ctx, workers);
        }
        proxy->max_keep_alive_requests = 1000000;
        proxy->enable_async_http1 = async;
        auto proxy_filter = make_unique<reverse_proxy_filter>(proxy_ctx, "127.0.0.1", backend->port(),
                                                              backend_tls ? "https" : "http");
        proxy_filter->verify_backend_ssl = false;
        proxy_filter->raw_response_passthrough = true;
        proxy_filter->max_idle_connections = max<size_t>(32, workers);
        proxy_filter->add_skip_response_header("X-Backend-Debug");
        auto* proxy_filter_ptr = proxy_filter.get();
        proxy->router().use(move(proxy_filter));
        if (!proxy->start()) {
            backend->stop();
            state.SkipWithError("reverse proxy frontend failed to bind");
            return;
        }

        const ports port = proxy->port();
        const client_factory factory = frontend_tls ? make_tls_client_factory() : make_plain_client_factory();
        const string request = make_request(true);
        const int64_t per_thread = batch_size_for(payload_size);

        vector<worker_result> results(clients);
        for (auto& result: results) {
            result.latency_us.reserve(static_cast<size_t>(per_thread));
        }

        vector<uint32_t> all_latencies;
        int64_t total_requests = 0;
        int64_t total_failures = 0;
        double total_elapsed = 0.0;

        for (auto _: state) {
            for (auto& result: results) {
                result.requests = 0;
                result.failures = 0;
                result.latency_us.clear();
            }

            const auto begin = steady_clock::now();
            vector<thread> threads;
            threads.reserve(clients);
            for (size_t i = 0; i < clients; ++i) {
                threads.emplace_back([&, i] {
                    try {
                        run_keep_alive_worker(port, factory, request, per_thread, static_cast<size_t>(payload_size),
                                              results[i]);
                    } catch (...) {
                        results[i].failures = per_thread;
                    }
                });
            }
            for (auto& thread: threads) {
                thread.join();
            }
            const auto end = steady_clock::now();
            const double elapsed = duration<double>(end - begin).count();
            total_elapsed += elapsed;
            state.SetIterationTime(elapsed);

            for (const auto& result: results) {
                total_requests += result.requests;
                total_failures += result.failures;
                all_latencies.insert(all_latencies.end(), result.latency_us.begin(), result.latency_us.end());
            }
        }

        const auto proxy_stats = proxy_filter_ptr->stats();
        proxy_filter_ptr->close_idle_connections();
        proxy->stop();
        backend->stop();

        sort(all_latencies.begin(), all_latencies.end());
        publish_latency(state, all_latencies);
        if (total_elapsed > 0.0) {
            state.counters["QPS"] = benchmark::Counter(static_cast<double>(total_requests) / total_elapsed);
        }
        state.counters["failures"] = benchmark::Counter(static_cast<double>(total_failures));
        state.counters["clients"] = benchmark::Counter(static_cast<double>(clients));
        state.counters["payload_bytes"] = benchmark::Counter(static_cast<double>(payload_size));
        state.counters["server_threads"] = benchmark::Counter(static_cast<double>(workers));
        state.counters["proxy_requests"] = benchmark::Counter(static_cast<double>(proxy_stats.requests));
        state.counters["proxy_backend_connections"] =
                benchmark::Counter(static_cast<double>(proxy_stats.backend_connections));
        state.counters["proxy_pool_waits"] = benchmark::Counter(static_cast<double>(proxy_stats.pool_waits));
        state.counters["proxy_pool_wait_p99_us"] =
                benchmark::Counter(static_cast<double>(proxy_stats.pool_wait_p99_us));
        state.counters["proxy_queue_rejections"] =
                benchmark::Counter(static_cast<double>(proxy_stats.queue_rejections));
        state.SetItemsProcessed(total_requests);
        state.SetBytesProcessed(total_requests * payload_size);
        std::string proxy_label = async ? "async_reverse_proxy" : "reverse_proxy";
        if (frontend_tls && backend_tls) {
            proxy_label += "_tls";
        } else if (frontend_tls) {
            proxy_label += "_tls_frontend";
        } else if (backend_tls) {
            proxy_label += "_tls_backend";
        }
        proxy_label += " clients=" + std::to_string(clients);
        proxy_label += " payload=" + std::to_string(payload_size);
        proxy_label += "B server_threads=" + std::to_string(workers);
        state.SetLabel(proxy_label);
    }

    void run_h2_benchmark(benchmark::State& state, const int64_t client_count, const int64_t payload_size,
                          const int64_t server_threads, const int64_t concurrent_streams) {
        const auto clients = static_cast<size_t>(max<int64_t>(1, client_count));
        const auto workers = static_cast<size_t>(max<int64_t>(1, server_threads));
        const auto body = make_shared<string>(static_cast<size_t>(max<int64_t>(0, payload_size)), 'x');

        ssl_context ssl(ssl_method::tls_server);
        ssl.enable_session_reuse(true);
        if (!ssl.load_certificate(benchmark_cert_path(), benchmark_key_path())) {
            state.SkipWithError("HTTP/2 benchmark certificate not found; set NEXUSFORCE_BENCH_CERT / "
                                "NEXUSFORCE_BENCH_KEY");
            return;
        }
        io_context server_ctx;
        http_server server(ports(0), server_ctx, _NEFORCE move(ssl), workers);
        server.h2_max_concurrent_streams = static_cast<uint32_t>(max<int64_t>(1, concurrent_streams));
        server.router().get("/bench", [body](http_request&, http_response& response) {
            response.status = http_status::ok;
            response.set_body_view(body->view());
        });
        if (!server.start()) {
            state.SkipWithError("HTTP/2 server failed to bind");
            return;
        }
        const ports port = server.port();

        constexpr int64_t per_thread = 5000;
        vector<worker_result> results(clients);
        for (auto& result: results) {
            result.latency_us.reserve(static_cast<size_t>(per_thread));
        }

        vector<uint32_t> all_latencies;
        int64_t total_requests = 0;
        int64_t total_failures = 0;
        double total_elapsed = 0.0;

        for (auto _: state) {
            for (auto& result: results) {
                result.requests = 0;
                result.failures = 0;
                result.latency_us.clear();
            }

            const auto begin = steady_clock::now();
            vector<thread> threads;
            threads.reserve(clients);
            for (size_t i = 0; i < clients; ++i) {
                threads.emplace_back([&, i] {
                    try {
                        run_h2_worker(port, per_thread, static_cast<size_t>(payload_size), concurrent_streams,
                                      results[i]);
                    } catch (...) {
                        results[i].failures = per_thread;
                    }
                });
            }
            for (auto& thread: threads) {
                thread.join();
            }
            const auto end = steady_clock::now();
            const double elapsed = duration<double>(end - begin).count();
            total_elapsed += elapsed;
            state.SetIterationTime(elapsed);

            for (const auto& result: results) {
                total_requests += result.requests;
                total_failures += result.failures;
                all_latencies.insert(all_latencies.end(), result.latency_us.begin(), result.latency_us.end());
            }
        }

        server.stop();
        sort(all_latencies.begin(), all_latencies.end());
        publish_latency(state, all_latencies);
        if (total_elapsed > 0.0) {
            state.counters["QPS"] = benchmark::Counter(static_cast<double>(total_requests) / total_elapsed);
        }
        state.counters["failures"] = benchmark::Counter(static_cast<double>(total_failures));
        state.counters["clients"] = benchmark::Counter(static_cast<double>(clients));
        state.counters["payload_bytes"] = benchmark::Counter(static_cast<double>(payload_size));
        state.counters["server_threads"] = benchmark::Counter(static_cast<double>(workers));
        state.SetItemsProcessed(total_requests);
        state.SetBytesProcessed(total_requests * payload_size);
        state.SetLabel("h2_tls clients=" + std::to_string(clients) + " payload=" + std::to_string(payload_size) +
                       "B server_threads=" + std::to_string(workers));
    }

} // namespace

static void BM_HttpServer_H2Tls(benchmark::State& state) {
    run_h2_benchmark(state, state.range(0), state.range(1), state.range(2), 1);
}
BENCHMARK(BM_HttpServer_H2Tls)
        ->Args({1, 4, 8})
        ->Args({4, 4, 4})
        ->Args({8, 4, 128})
        ->UseManualTime()
        ->MinTime(0.7)
        ->Unit(benchmark::kSecond);

static void BM_HttpServer_H2TlsMultiplex(benchmark::State& state) {
    run_h2_benchmark(state, state.range(0), state.range(1), state.range(2), 16);
}
static void BM_HttpServer_H2Tls256Streams(benchmark::State& state) {
    run_h2_benchmark(state, state.range(0), state.range(1), state.range(2), 256);
}
BENCHMARK(BM_HttpServer_H2Tls256Streams)
        ->Args({64, 4, 128})
        ->Args({128, 4, 256})
        ->Args({256, 4, 256})
        ->UseManualTime()
        ->MinTime(0.7)
        ->Unit(benchmark::kSecond);

BENCHMARK(BM_HttpServer_H2TlsMultiplex)
        ->Args({4, 4096, 64})
        ->Args({8, 4096, 64})
        ->Args({16, 4096, 64})
        ->Args({32, 4096, 128})
        ->Args({64, 4096, 128})
        ->Args({128, 4096, 256})
        ->Args({4, 65536, 64})
        ->Args({16, 65536, 64})
        ->UseManualTime()
        ->MinTime(0.7)
        ->Unit(benchmark::kSecond);

static void BM_HttpServer_ReverseProxy(benchmark::State& state) {
    run_reverse_proxy_benchmark(state, state.range(0), state.range(1), state.range(2), false, false);
}
BENCHMARK(BM_HttpServer_ReverseProxy)
        ->Args({1, 4, 4})
        ->Args({4, 4, 4})
        ->Args({8, 4, 8})
        ->Args({16, 4, 16})
        ->Args({32, 4, 32})
        ->UseManualTime()
        ->MinTime(0.7)
        ->Unit(benchmark::kSecond);

static void BM_HttpServer_DirectProxy(benchmark::State& state) {
    run_direct_proxy_benchmark(state, state.range(0), state.range(1), state.range(2));
}
BENCHMARK(BM_HttpServer_DirectProxy)
        ->Args({1, 4, 4})
        ->Args({4, 4, 4})
        ->Args({8, 4, 8})
        ->Args({32, 4, 32})
        ->Args({8, 4096, 8})
        ->UseManualTime()
        ->MinTime(0.7)
        ->Unit(benchmark::kSecond);

static void BM_HttpServer_HttpsReverseProxy(benchmark::State& state) {
    run_reverse_proxy_benchmark(state, state.range(0), state.range(1), state.range(2), true, true);
}
BENCHMARK(BM_HttpServer_HttpsReverseProxy)
        ->Args({1, 4, 4})
        ->Args({4, 4, 4})
        ->Args({8, 4, 8})
        ->UseManualTime()
        ->MinTime(0.7)
        ->Unit(benchmark::kSecond);

static void BM_HttpServer_AsyncReverseProxy(benchmark::State& state) {
    run_reverse_proxy_benchmark(state, state.range(0), state.range(1), state.range(2), false, false, true);
}
BENCHMARK(BM_HttpServer_AsyncReverseProxy)
        ->Args({1, 4, 4})
        ->Args({4, 4, 4})
        ->Args({8, 4, 8})
        ->Args({16, 4, 16})
        ->Args({32, 4, 32})
        ->Args({8, 4096, 8})
        ->UseManualTime()
        ->MinTime(0.7)
        ->Unit(benchmark::kSecond);

static void BM_HttpServer_KeepAlive(benchmark::State& state) {
    run_server_benchmark(state, state.range(0), state.range(1), state.range(2), true, false);
}
BENCHMARK(BM_HttpServer_KeepAlive)
        // Client scaling, fixed server worker count.
        ->Args({1, 4, 8})
        ->Args({2, 4, 8})
        ->Args({4, 4, 8})
        ->Args({8, 4, 8})
        // Server worker scaling, fixed client count.
        ->Args({8, 4, 1})
        ->Args({8, 4, 2})
        ->Args({8, 4, 4})
        // Response body size.
        ->Args({4, 4, 4})
        ->Args({4, 64, 4})
        ->Args({4, 1024, 4})
        ->Args({4, 4096, 4})
        ->Args({4, 16384, 4})
        ->Args({4, 65536, 4})
        ->UseManualTime()
        ->MinTime(0.7)
        ->Unit(benchmark::kSecond);

static void BM_HttpServer_TlsKeepAlive(benchmark::State& state) {
    run_server_benchmark(state, state.range(0), state.range(1), state.range(2), true, true);
}
BENCHMARK(BM_HttpServer_TlsKeepAlive)
        ->Args({1, 4, 8})
        ->Args({4, 4, 4})
        ->Args({8, 4, 8})
        ->Args({8, 4, 16})
        ->Args({8, 4, 32})
        ->Args({8, 4, 64})
        ->Args({4, 65536, 4})
        ->UseManualTime()
        ->MinTime(0.7)
        ->Unit(benchmark::kSecond);

static void BM_HttpServer_TlsNewConnection(benchmark::State& state) {
    run_server_benchmark(state, state.range(0), state.range(1), state.range(2), false, true);
}
BENCHMARK(BM_HttpServer_TlsNewConnection)
        ->Args({1, 4, 1})
        ->Args({4, 4, 4})
        ->Args({8, 4, 64})
        ->Args({16, 4, 16})
        ->Args({32, 4, 64})
        ->UseManualTime()
        ->MinTime(0.7)
        ->Unit(benchmark::kSecond);

static void BM_HttpServer_Tls12ReuseAcceleration(benchmark::State& state) {
    run_server_benchmark(state, state.range(0), state.range(1), state.range(2), false, true, false, true);
}
BENCHMARK(BM_HttpServer_Tls12ReuseAcceleration)
        ->Args({1, 4, 1})
        ->Args({4, 4, 4})
        ->Args({8, 4, 32})
        ->Args({16, 4, 16})
        ->Args({32, 4, 64})
        ->UseManualTime()
        ->MinTime(0.7)
        ->Unit(benchmark::kSecond);

static void BM_HttpServer_Pipelining(benchmark::State& state) {
    run_server_benchmark(state, state.range(0), state.range(1), state.range(2), true, false, true);
}
BENCHMARK(BM_HttpServer_Pipelining)
        ->Args({1, 4, 8})
        ->Args({4, 4, 8})
        ->Args({8, 4, 8})
        ->Args({4, 4096, 4})
        ->UseManualTime()
        ->MinTime(0.7)
        ->Unit(benchmark::kSecond);

static void BM_HttpServer_NewConnection(benchmark::State& state) {
    run_server_benchmark(state, state.range(0), state.range(1), state.range(2), false, false);
}
BENCHMARK(BM_HttpServer_NewConnection)
        ->Args({1, 4, 1})
        ->Args({2, 4, 2})
        ->Args({4, 4, 4})
        ->Args({8, 4, 8})
        ->UseManualTime()
        ->MinTime(0.7)
        ->Unit(benchmark::kSecond);

BENCHMARK_MAIN();
