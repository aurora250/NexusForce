// WebSocket / HttpClient / gRPC protocol benchmarks.
//
// WebSocket uses a raw TCP client with a manual HTTP Upgrade and masked text
// frames; the server echoes each frame. HttpClient uses a persistent
// http_client per worker against an in-process HTTP server. gRPC uses unary
// gRPC frames over HTTP/1.1 through http_client.
//
// Run: NexusForceHttpProtocolBenchmark --benchmark_min_time=1s
#include <NeForce/core/async/io_context.hpp>
#include <NeForce/core/time/clocks.hpp>
#include <NeForce/network/http/grpc.hpp>
#include <NeForce/network/http/http2_connection.hpp>
#include <NeForce/network/http/http2_protocol.hpp>
#include <NeForce/network/http/http_client.hpp>
#include <NeForce/network/http/http_server.hpp>
#include <NeForce/network/http/websocket.hpp>
#include <NeForce/network/tcp/tcp_client.hpp>
#include <NeForce/network/tcp/tcp_server.hpp>
#include <NeForce/network/tcp/tcp_socket.hpp>
#include <benchmark/benchmark.h>
using namespace neforce;
using namespace neforce::http;

namespace {
    struct worker_result {
        int64_t requests{0};
        int64_t failures{0};
        vector<uint32_t> latency_us;
    };

    uint32_t elapsed_us(const steady_clock::time_point& begin, const steady_clock::time_point& end) {
        return static_cast<uint32_t>(duration<double, micro>(end - begin).count());
    }

    void publish_result(benchmark::State& state, vector<worker_result>& results, const double total_elapsed,
                        const int64_t payload_size) {
        int64_t requests = 0;
        int64_t failures = 0;
        vector<uint32_t> latencies;
        for (auto& result: results) {
            requests += result.requests;
            failures += result.failures;
            latencies.insert(latencies.end(), result.latency_us.begin(), result.latency_us.end());
        }
        sort(latencies.begin(), latencies.end());
        const auto percentile = [&latencies](const double p) {
            size_t index = static_cast<size_t>(p * static_cast<double>(latencies.size()));
            return latencies[min(index, latencies.size() - 1)];
        };
        if (!latencies.empty()) {
            double sum = 0.0;
            for (const auto sample: latencies) {
                sum += static_cast<double>(sample);
            }
            state.counters["mean_us"] = benchmark::Counter(sum / static_cast<double>(latencies.size()));
            state.counters["p50_us"] = benchmark::Counter(static_cast<double>(percentile(0.50)));
            state.counters["p90_us"] = benchmark::Counter(static_cast<double>(percentile(0.90)));
            state.counters["p99_us"] = benchmark::Counter(static_cast<double>(percentile(0.99)));
            state.counters["max_us"] = benchmark::Counter(static_cast<double>(latencies.back()));
        }
        state.counters["QPS"] = benchmark::Counter(static_cast<double>(requests) / total_elapsed);
        state.counters["failures"] = benchmark::Counter(static_cast<double>(failures));
        state.counters["payload_bytes"] = benchmark::Counter(static_cast<double>(payload_size));
        state.SetItemsProcessed(requests);
        state.SetBytesProcessed(requests * payload_size);
    }

    NEFORCE_NODISCARD bool receive_exact(tcp_socket& socket, char* data, size_t size) {
        while (size > 0) {
            const ssize_t received = socket.receive(memory_view<char>(data, size));
            if (received <= 0) {
                return false;
            }
            data += received;
            size -= static_cast<size_t>(received);
        }
        return true;
    }

    NEFORCE_NODISCARD bool receive_exact(tcp_client_base& client, char* data, size_t size) {
        while (size > 0) {
            const ssize_t received = client.receive(data, size);
            if (received <= 0) {
                return false;
            }
            data += received;
            size -= static_cast<size_t>(received);
        }
        return true;
    }

    uint32_t payload_count(const int64_t payload_size) {
        if (payload_size >= 32 * 1024) {
            return 300;
        }
        if (payload_size >= 2048) {
            return 1000;
        }
        return 4000;
    }

    // ---- WebSocket ----

    NEFORCE_NODISCARD bool ws_connect(tcp_socket& socket, const ports port) {
        socket.open(ip_family::inet4);
        socket.set_tcp_nodelay(true);
        const auto endpoint = ip_address::parse("127.0.0.1", port);
        if (!endpoint || !socket.connect(*endpoint, milliseconds(5000))) {
            return false;
        }
        socket.set_receive_timeout(milliseconds(5000));

        const string request = "GET /ws HTTP/1.1\r\n"
                               "Host: 127.0.0.1\r\n"
                               "Upgrade: websocket\r\n"
                               "Connection: Upgrade\r\n"
                               "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
                               "Sec-WebSocket-Version: 13\r\n\r\n";
        socket.send_all(memory_view<const char>(request.data(), request.size()));

        string response;
        char buffer[2048];
        while (response.find("\r\n\r\n") == string::npos) {
            const ssize_t received = socket.receive(memory_view<char>(buffer, sizeof(buffer)));
            if (received <= 0) {
                return false;
            }
            response.append(buffer, static_cast<size_t>(received));
            if (response.size() > 16384) {
                return false;
            }
        }
        return response.find(" 101 ") != string::npos;
    }

    NEFORCE_NODISCARD string ws_client_frame(const string& payload, const uint32_t sequence) {
        string frame;
        frame.reserve(payload.size() + 14);
        frame.push_back(static_cast<char>(0x81));
        const auto size = payload.size();
        if (size < 126) {
            frame.push_back(static_cast<char>(0x80 | size));
        } else if (size <= 0xFFFF) {
            frame.push_back(static_cast<char>(0x80 | 126));
            frame.push_back(static_cast<char>((size >> 8) & 0xFF));
            frame.push_back(static_cast<char>(size & 0xFF));
        } else {
            frame.push_back(static_cast<char>(0x80 | 127));
            for (int shift = 56; shift >= 0; shift -= 8) {
                frame.push_back(static_cast<char>((static_cast<uint64_t>(size) >> shift) & 0xFF));
            }
        }
        char mask[4] = {static_cast<char>(sequence & 0xFF), static_cast<char>((sequence >> 8) & 0xFF),
                        static_cast<char>((sequence >> 16) & 0xFF), static_cast<char>((sequence >> 24) & 0xFF)};
        frame.append(mask, 4);
        for (size_t i = 0; i < size; ++i) {
            frame.push_back(static_cast<char>(payload[i] ^ mask[i & 3]));
        }
        return frame;
    }

    void ws_send_close(tcp_socket& socket) {
        const char frame[] = {static_cast<char>(0x88),        static_cast<char>(0x82),       0x01, 0x00, 0x00, 0x00,
                              static_cast<char>(0x03 ^ 0x01), static_cast<char>(0xE8 ^ 0x00)};
        try {
            socket.send_all(memory_view<const char>(frame, sizeof(frame)));
            // NOLINTNEXTLINE(bugprone-empty-catch)
        } catch (const exception&) {
        }
    }

    NEFORCE_NODISCARD bool ws_read_frame(tcp_socket& socket, string& payload) {
        byte_t header[2];
        if (!receive_exact(socket, reinterpret_cast<char*>(header), sizeof(header))) {
            return false;
        }
        uint64_t length = header[1] & 0x7F;
        if (length == 126) {
            byte_t extended[2];
            if (!receive_exact(socket, reinterpret_cast<char*>(extended), 2)) {
                return false;
            }
            length = (static_cast<uint64_t>(extended[0]) << 8) | extended[1];
        } else if (length == 127) {
            byte_t extended[8];
            if (!receive_exact(socket, reinterpret_cast<char*>(extended), 8)) {
                return false;
            }
            length = 0;
            for (const auto byte: extended) {
                length = (length << 8) | byte;
            }
        }
        payload.resize(static_cast<size_t>(length));
        if (length > 0 && !receive_exact(socket, payload.data(), payload.size())) {
            return false;
        }
        return (header[0] & 0x0F) == static_cast<uint8_t>(websocket_opcode::text);
    }

    void run_ws_worker(const ports port, const size_t payload_size, const uint32_t count, const uint32_t pipeline_depth,
                       worker_result& result) {
        tcp_socket socket;
        if (!ws_connect(socket, port)) {
            result.failures = count;
            return;
        }
        const string payload(payload_size, 'w');
        const uint32_t depth = max<uint32_t>(1, pipeline_depth);
        uint32_t sequence = 1;
        uint32_t sent = 0;
        string response;
        while (sent < count) {
            const uint32_t batch = min<uint32_t>(depth, count - sent);
            string frames;
            frames.reserve(static_cast<size_t>(batch) * (payload.size() + 14));
            for (uint32_t i = 0; i < batch; ++i) {
                frames += ws_client_frame(payload, sequence++);
            }
            const auto begin = steady_clock::now();
            socket.send_all(memory_view<const char>(frames.data(), frames.size()));
            bool ok = true;
            for (uint32_t i = 0; i < batch; ++i) {
                if (!ws_read_frame(socket, response) || response.size() != payload.size()) {
                    ok = false;
                    break;
                }
            }
            if (!ok) {
                ++result.failures;
                break;
            }
            const auto end = steady_clock::now();
            const auto elapsed = max<uint32_t>(1, elapsed_us(begin, end) / batch);
            for (uint32_t i = 0; i < batch; ++i) {
                result.latency_us.push_back(elapsed);
            }
            result.requests += batch;
            sent += batch;
        }
        ws_send_close(socket);
        socket.close();
    }

    void run_websocket_benchmark(benchmark::State& state, const int64_t client_count, const int64_t payload_size,
                                 const int64_t server_threads, const int64_t pipeline_depth) {
        const auto clients = static_cast<size_t>(max<int64_t>(1, client_count));
        const auto workers = static_cast<size_t>(max<int64_t>(1, server_threads));
        io_context server_ctx;
        http_server server(ports(0), server_ctx, workers);
        server.max_keep_alive_requests = 0;
        auto& ws = server.websocket();
        ws.set_io_context(server_ctx);
        ws.route("/ws", [](websocket_server::session_ptr session) {
            const weak_ptr<websocket_session> weak_session = session;
            session->set_message_handler([weak_session](const string& message, websocket_opcode opcode) {
                if (const auto current = weak_session.lock()) {
                    current->send(message, opcode);
                }
            });
        });
        if (!server.start()) {
            state.SkipWithError("WebSocket server failed to start");
            return;
        }
        const ports port = server.port();
        const uint32_t count = payload_count(payload_size);
        vector<worker_result> results(clients);

        int64_t total_requests = 0;
        int64_t total_failures = 0;
        double total_elapsed = 0.0;
        vector<uint32_t> all_latencies;
        for (auto _: state) {
            for (auto& result: results) {
                result = worker_result{};
            }
            const auto begin = steady_clock::now();
            vector<thread> threads;
            for (size_t i = 0; i < clients; ++i) {
                threads.emplace_back([&, i] {
                    try {
                        run_ws_worker(port, static_cast<size_t>(payload_size), count,
                                      static_cast<uint32_t>(max<int64_t>(1, pipeline_depth)), results[i]);
                        // NOLINTNEXTLINE(bugprone-empty-catch)
                    } catch (const exception&) {
                        results[i].failures = count;
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
        this_thread::sleep_for(milliseconds(50));
        ws.stop();
        server.stop();
        if (total_elapsed <= 0.0) {
            state.SkipWithError("WebSocket benchmark produced no time");
            return;
        }
        results.clear();
        results.emplace_back();
        results.front().requests = total_requests;
        results.front().failures = total_failures;
        results.front().latency_us = move(all_latencies);
        publish_result(state, results, total_elapsed, payload_size);
        state.counters["clients"] = benchmark::Counter(static_cast<double>(clients));
        state.counters["server_threads"] = benchmark::Counter(static_cast<double>(workers));
    }

    // ---- HttpClient ----

    void run_http_client_worker(const ports port, const size_t payload_size, const uint32_t count,
                                worker_result& result) {
        io_context client_ctx;
        http_client client(client_ctx);
        const string url = "http://127.0.0.1:" + to_string(port.value()) + "/bench";
        for (uint32_t i = 0; i < count; ++i) {
            const auto begin = steady_clock::now();
            const auto response = client.get(url);
            const auto end = steady_clock::now();
            if (response.status != http_status::ok || response.body.size() != payload_size) {
                ++result.failures;
                break;
            }
            result.latency_us.push_back(elapsed_us(begin, end));
            ++result.requests;
        }
        client.close();
    }

    void run_http_client_benchmark(benchmark::State& state, const int64_t client_count, const int64_t payload_size,
                                   const int64_t server_threads) {
        const auto clients = static_cast<size_t>(max<int64_t>(1, client_count));
        const auto workers = static_cast<size_t>(max<int64_t>(1, server_threads));
        const auto body = make_shared<string>(static_cast<size_t>(max<int64_t>(0, payload_size)), 'h');
        io_context server_ctx;
        http_server server(ports(0), server_ctx, workers);
        server.max_keep_alive_requests = 0;
        server.router().get("/bench", [body](http_request&, http_response& response) {
            response.status = http_status::ok;
            response.set_body_view(body->view());
        });
        if (!server.start()) {
            state.SkipWithError("HttpClient benchmark server failed to start");
            return;
        }
        const ports port = server.port();
        const uint32_t count = payload_count(payload_size);
        vector<worker_result> results(clients);

        int64_t total_requests = 0;
        int64_t total_failures = 0;
        double total_elapsed = 0.0;
        vector<uint32_t> all_latencies;
        for (auto _: state) {
            for (auto& result: results) {
                result = worker_result{};
            }
            const auto begin = steady_clock::now();
            vector<thread> threads;
            for (size_t i = 0; i < clients; ++i) {
                threads.emplace_back([&, i] {
                    try {
                        run_http_client_worker(port, static_cast<size_t>(payload_size), count, results[i]);
                        // NOLINTNEXTLINE(bugprone-empty-catch)
                    } catch (const exception&) {
                        results[i].failures = count;
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
        if (total_elapsed <= 0.0) {
            state.SkipWithError("HttpClient benchmark produced no time");
            return;
        }
        results.clear();
        results.emplace_back();
        results.front().requests = total_requests;
        results.front().failures = total_failures;
        results.front().latency_us = move(all_latencies);
        publish_result(state, results, total_elapsed, payload_size);
        state.counters["clients"] = benchmark::Counter(static_cast<double>(clients));
        state.counters["server_threads"] = benchmark::Counter(static_cast<double>(workers));
    }

    // ---- gRPC over HTTP/2 streaming ----

    struct h2_raw_frame {
        uint8_t type{0};
        uint8_t flags{0};
        uint32_t stream_id{0};
        byte_vector payload;
    };

    NEFORCE_NODISCARD bool h2_send_frame(tcp_client& client, const byte_vector& frame) {
        try {
            return client.send_all(string_view(reinterpret_cast<const char*>(frame.data()), frame.size()));
            // NOLINTNEXTLINE(bugprone-empty-catch)
        } catch (const exception&) {
            return false;
        }
    }

    NEFORCE_NODISCARD bool h2_receive_frame(tcp_client& client, h2_raw_frame& frame) {
        byte_t header[9];
        if (!receive_exact(client, reinterpret_cast<char*>(header), sizeof(header))) {
            return false;
        }
        const uint32_t length =
                (static_cast<uint32_t>(header[0]) << 16) | (static_cast<uint32_t>(header[1]) << 8) | header[2];
        frame.type = header[3];
        frame.flags = header[4];
        frame.stream_id = ((static_cast<uint32_t>(header[5]) & 0x7F) << 24) | (static_cast<uint32_t>(header[6]) << 16) |
                          (static_cast<uint32_t>(header[7]) << 8) | header[8];
        frame.payload.resize(length);
        return length == 0 || receive_exact(client, reinterpret_cast<char*>(frame.payload.data()), length);
    }

    NEFORCE_NODISCARD bool h2_handshake(tcp_client& client, hpack_encoder& encoder) {
        static constexpr string_view preface = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";
        try {
            if (!client.send_all(preface)) {
                return false;
            }
            // NOLINTNEXTLINE(bugprone-empty-catch)
        } catch (const exception&) {
            return false;
        }
        http2_settings_frame settings;
        settings.entries.push_back({http2_settings_id::initial_window_size, 1U << 20});
        settings.entries.push_back({http2_settings_id::max_frame_size, 16384});
        settings.entries.push_back({http2_settings_id::max_concurrent_streams, 256});
        if (!h2_send_frame(client, http2_framer::encode_settings_frame(settings))) {
            return false;
        }
        h2_raw_frame frame;
        while (true) {
            if (!h2_receive_frame(client, frame)) {
                return false;
            }
            if (frame.type == static_cast<uint8_t>(http2_frame_type::settings) && (frame.flags & HTTP2_FLAG_ACK) == 0) {
                http2_settings_frame ack;
                ack.ack = true;
                if (!h2_send_frame(client, http2_framer::encode_settings_frame(ack))) {
                    return false;
                }
                return true;
            }
        }
    }

    NEFORCE_NODISCARD bool h2_grpc_request(tcp_client& client, hpack_encoder& encoder, const uint32_t stream_id,
                                           const string& grpc_body) {
        vector<hpack_header_field> headers;
        headers.push_back({":method", "POST"});
        headers.push_back({":scheme", "http"});
        headers.push_back({":path", "/grpc.test.Echo/Stream"});
        headers.push_back({":authority", "127.0.0.1"});
        headers.push_back({"content-type", "application/grpc"});
        headers.push_back({"te", "trailers"});
        headers.push_back({"content-length", to_string(grpc_body.size())});

        http2_headers_frame hf;
        hf.stream_id = stream_id;
        hf.header_block = encoder.encode(headers);
        hf.end_headers = true;
        hf.end_stream = false;
        http2_data_frame df;
        df.stream_id = stream_id;
        df.end_stream = true;
        df.data.assign(reinterpret_cast<const byte_t*>(grpc_body.data()),
                       reinterpret_cast<const byte_t*>(grpc_body.data()) + grpc_body.size());

        byte_vector batch = http2_framer::encode_headers_frame(hf);
        const auto data_frame = http2_framer::encode_data_frame(df);
        batch.insert(batch.end(), data_frame.begin(), data_frame.end());
        return h2_send_frame(client, batch);
    }

    NEFORCE_NODISCARD bool h2_read_grpc_stream(tcp_client& client, const uint32_t stream_id,
                                               const uint32_t expected_messages, uint32_t& messages) {
        messages = 0;
        uint32_t consumed = 0;
        byte_vector response_data;
        for (;;) {
            h2_raw_frame frame;
            if (!h2_receive_frame(client, frame)) {
                return false;
            }
            if (frame.type == static_cast<uint8_t>(http2_frame_type::settings) && (frame.flags & HTTP2_FLAG_ACK) == 0) {
                http2_settings_frame ack;
                ack.ack = true;
                if (!h2_send_frame(client, http2_framer::encode_settings_frame(ack))) {
                    return false;
                }
                continue;
            }
            if (frame.type == static_cast<uint8_t>(http2_frame_type::rst_stream) && frame.stream_id == stream_id) {
                return false;
            }
            if (frame.stream_id == stream_id && frame.type == static_cast<uint8_t>(http2_frame_type::data)) {
                response_data.insert(response_data.end(), frame.payload.begin(), frame.payload.end());
                consumed += static_cast<uint32_t>(frame.payload.size());
                if (consumed >= 16384) {
                    http2_window_update_frame update;
                    update.stream_id = 0;
                    update.window_size_increment = consumed;
                    if (!h2_send_frame(client, http2_framer::encode_window_update_frame(update))) {
                        return false;
                    }
                    consumed = 0;
                }
            }
            if (frame.stream_id == stream_id && frame.type == static_cast<uint8_t>(http2_frame_type::headers) &&
                (frame.flags & HTTP2_FLAG_END_STREAM) != 0) {
                grpc_framer framer;
                vector<grpc_message> decoded;
                framer.decode(response_data.data(), response_data.size(), decoded);
                messages = static_cast<uint32_t>(decoded.size());
                return messages == expected_messages;
            }
        }
    }

    void run_grpc_h2_stream_worker(const ports port, const int64_t rpc_count, const int64_t messages_per_stream,
                                   const size_t payload_size, worker_result& result) {
        io_context ctx;
        tcp_client client(ctx);
        if (!client.connect("127.0.0.1", port)) {
            result.failures = rpc_count;
            return;
        }
        client.socket().set_tcp_nodelay(true);
        client.socket().set_nonblocking(false);
        client.socket().set_receive_timeout(milliseconds(10000));
        hpack_encoder encoder;
        if (!h2_handshake(client, encoder)) {
            result.failures = rpc_count;
            return;
        }

        grpc_message request_message;
        request_message.payload.assign(payload_size, static_cast<byte_t>('s'));
        const auto request_frame = grpc_framer::encode(request_message);
        const string body(reinterpret_cast<const char*>(request_frame.data()), request_frame.size());
        uint32_t stream_id = 1;
        for (int64_t i = 0; i < rpc_count; ++i) {
            const auto begin = steady_clock::now();
            if (!h2_grpc_request(client, encoder, stream_id, body)) {
                ++result.failures;
                break;
            }
            uint32_t messages = 0;
            if (!h2_read_grpc_stream(client, stream_id, static_cast<uint32_t>(messages_per_stream), messages)) {
                ++result.failures;
                break;
            }
            const auto end = steady_clock::now();
            result.latency_us.push_back(elapsed_us(begin, end));
            ++result.requests;
            stream_id += 2;
        }
        client.disconnect();
    }

    void run_grpc_h2_stream_benchmark(benchmark::State& state, const int64_t client_count,
                                      const int64_t messages_per_stream, const int64_t server_threads,
                                      const int64_t payload_size) {
        const auto clients = static_cast<size_t>(max<int64_t>(1, client_count));
        const auto workers = static_cast<size_t>(max<int64_t>(1, server_threads));
        io_context server_ctx;
        tcp_server server(ports(0), server_ctx, workers);
        vector<shared_ptr<http2_connection>> connections;
        mutex connections_mutex;
        const auto messages = static_cast<uint32_t>(max<int64_t>(1, messages_per_stream));
        server.set_client_handler([&](unique_ptr<tcp_socket> socket) {
            socket->set_tcp_nodelay(true);
            auto conn = make_shared<http2_connection>(move(socket), server_ctx);
            conn->set_max_concurrent_streams(256);
            const weak_ptr<http2_connection> weak_conn = conn;
            conn->set_stream_handler([weak_conn, messages](uint32_t stream_id, const vector<hpack_header_field>&,
                                                           const byte_t* data, size_t len, bool end_stream) {
                if (!end_stream) {
                    return;
                }
                if (const auto current = weak_conn.lock()) {
                    grpc_handler::process_server_stream(
                            *current, stream_id, data, len,
                            [messages](const grpc_message& request, function<void(grpc_message)> sink) {
                                for (uint32_t i = 0; i < messages; ++i) {
                                    grpc_message response;
                                    response.payload = request.payload;
                                    sink(move(response));
                                }
                            });
                }
            });
            conn->start();
            lock<mutex> lk(connections_mutex);
            connections.push_back(conn);
        });
        if (!server.start()) {
            state.SkipWithError("gRPC HTTP/2 streaming server failed to start");
            return;
        }
        const ports port = server.port();
        const uint32_t count = static_cast<uint32_t>(max<int64_t>(1, 4000 / max<int64_t>(1, messages_per_stream)));
        vector<worker_result> results(clients);
        int64_t total_requests = 0;
        int64_t total_failures = 0;
        double total_elapsed = 0.0;
        vector<uint32_t> all_latencies;
        for (auto _: state) {
            for (auto& result: results) {
                result = worker_result{};
            }
            const auto begin = steady_clock::now();
            vector<thread> threads;
            for (size_t i = 0; i < clients; ++i) {
                threads.emplace_back([&, i] {
                    try {
                        run_grpc_h2_stream_worker(port, count, messages_per_stream, static_cast<size_t>(payload_size),
                                                  results[i]);
                        // NOLINTNEXTLINE(bugprone-empty-catch)
                    } catch (const exception&) {
                        results[i].failures = count;
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
        connections.clear();
        if (total_elapsed <= 0.0) {
            state.SkipWithError("gRPC HTTP/2 streaming benchmark produced no time");
            return;
        }
        results.clear();
        results.emplace_back();
        results.front().requests = total_requests;
        results.front().failures = total_failures;
        results.front().latency_us = move(all_latencies);
        publish_result(state, results, total_elapsed, payload_size);
        state.counters["clients"] = benchmark::Counter(static_cast<double>(clients));
        state.counters["server_threads"] = benchmark::Counter(static_cast<double>(workers));
        state.counters["messages_per_second"] =
                benchmark::Counter(static_cast<double>(total_requests * messages_per_stream) / total_elapsed);
        state.counters["messages_per_stream"] = benchmark::Counter(static_cast<double>(messages_per_stream));
        state.counters["streaming_rpcs"] = benchmark::Counter(static_cast<double>(total_requests));
    }

    // ---- gRPC ----

    void run_grpc_worker(const ports port, const size_t payload_size, const uint32_t count, worker_result& result) {
        io_context client_ctx;
        http_client client(client_ctx);
        const string url = "http://127.0.0.1:" + to_string(port.value()) + "/grpc.test.Echo/Echo";
        grpc_message message;
        message.payload.assign(payload_size, static_cast<byte_t>('g'));
        const auto frame = grpc_framer::encode(message);
        const string body(reinterpret_cast<const char*>(frame.data()), frame.size());
        for (uint32_t i = 0; i < count; ++i) {
            const auto begin = steady_clock::now();
            const auto response = client.post(url, body, "application/grpc");
            const auto end = steady_clock::now();
            if (response.status != http_status::ok || response.body.size() != body.size()) {
                ++result.failures;
                break;
            }
            result.latency_us.push_back(elapsed_us(begin, end));
            ++result.requests;
        }
        client.close();
    }

    void run_grpc_benchmark(benchmark::State& state, const int64_t client_count, const int64_t payload_size,
                            const int64_t server_threads) {
        const auto clients = static_cast<size_t>(max<int64_t>(1, client_count));
        const auto workers = static_cast<size_t>(max<int64_t>(1, server_threads));
        io_context server_ctx;
        http_server server(ports(0), server_ctx, workers);
        server.max_keep_alive_requests = 0;
        server.router().post("/grpc.test.Echo/Echo", [](http_request& request, http_response& response) {
            grpc_framer framer;
            vector<grpc_message> messages;
            if (framer.decode(reinterpret_cast<const byte_t*>(request.body.data()), request.body.size(), messages) <=
                        0 ||
                messages.empty()) {
                grpc_handler::send_error(response, grpc_status::invalid_argument, "Invalid gRPC frame");
                return;
            }
            grpc_message output;
            output.payload = move(messages.front().payload);
            const auto encoded = grpc_framer::encode(output);
            response.body.assign(reinterpret_cast<const char*>(encoded.data()), encoded.size());
            response.set_content_type("application/grpc");
            response.trailers["grpc-status"] = "0";
            response.trailers["grpc-message"] = "";
        });
        if (!server.start()) {
            state.SkipWithError("gRPC benchmark server failed to start");
            return;
        }
        const ports port = server.port();
        const uint32_t count = payload_count(payload_size);
        vector<worker_result> results(clients);

        int64_t total_requests = 0;
        int64_t total_failures = 0;
        double total_elapsed = 0.0;
        vector<uint32_t> all_latencies;
        const size_t frame_size = payload_size + 5;
        for (auto _: state) {
            for (auto& result: results) {
                result = worker_result{};
            }
            const auto begin = steady_clock::now();
            vector<thread> threads;
            for (size_t i = 0; i < clients; ++i) {
                threads.emplace_back([&, i] {
                    try {
                        run_grpc_worker(port, static_cast<size_t>(payload_size), count, results[i]);
                        // NOLINTNEXTLINE(bugprone-empty-catch)
                    } catch (const exception&) {
                        results[i].failures = count;
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
        if (total_elapsed <= 0.0) {
            state.SkipWithError("gRPC benchmark produced no time");
            return;
        }
        results.clear();
        results.emplace_back();
        results.front().requests = total_requests;
        results.front().failures = total_failures;
        results.front().latency_us = move(all_latencies);
        publish_result(state, results, total_elapsed, static_cast<int64_t>(frame_size));
        state.counters["clients"] = benchmark::Counter(static_cast<double>(clients));
        state.counters["server_threads"] = benchmark::Counter(static_cast<double>(workers));
    }
} // namespace

static void BM_WebSocket_Echo(benchmark::State& state) {
    run_websocket_benchmark(state, state.range(0), state.range(1), state.range(2), 1);
}
BENCHMARK(BM_WebSocket_Echo)
        ->Args({1, 64, 4})
        ->Args({4, 64, 4})
        ->Args({8, 64, 8})
        ->Args({16, 64, 16})
        ->Args({32, 64, 32})
        ->Args({64, 64, 64})
        ->Args({4, 4096, 4})
        ->Args({16, 4096, 16})
        ->UseManualTime()
        ->MinTime(0.7)
        ->Unit(benchmark::kSecond);

static void BM_WebSocket_EchoPipelined(benchmark::State& state) {
    run_websocket_benchmark(state, state.range(0), state.range(1), state.range(2), state.range(3));
}
BENCHMARK(BM_WebSocket_EchoPipelined)
        ->Args({8, 64, 8, 16})
        ->Args({16, 64, 16, 16})
        ->Args({32, 64, 32, 16})
        ->Args({64, 64, 64, 16})
        ->Args({16, 64, 16, 32})
        ->Args({8, 4096, 8, 8})
        ->Args({16, 4096, 16, 8})
        ->Args({8, 4096, 8})
        ->UseManualTime()
        ->MinTime(0.7)
        ->Unit(benchmark::kSecond);

static void BM_HttpClient_Get(benchmark::State& state) {
    run_http_client_benchmark(state, state.range(0), state.range(1), state.range(2));
}
BENCHMARK(BM_HttpClient_Get)
        ->Args({1, 4, 4})
        ->Args({4, 4, 4})
        ->Args({8, 4, 8})
        ->Args({4, 4096, 4})
        ->Args({8, 4096, 8})
        ->Args({4, 65536, 4})
        ->UseManualTime()
        ->MinTime(0.7)
        ->Unit(benchmark::kSecond);

static void BM_Grpc_Unary(benchmark::State& state) {
    run_grpc_benchmark(state, state.range(0), state.range(1), state.range(2));
}
BENCHMARK(BM_Grpc_Unary)
        ->Args({1, 16, 4})
        ->Args({4, 16, 4})
        ->Args({8, 16, 8})
        ->Args({4, 4096, 4})
        ->Args({8, 4096, 8})
        ->UseManualTime()
        ->MinTime(0.7)
        ->Unit(benchmark::kSecond);

static void BM_Grpc_H2_ServerStream(benchmark::State& state) {
    run_grpc_h2_stream_benchmark(state, state.range(0), state.range(1), state.range(2), state.range(3));
}
BENCHMARK(BM_Grpc_H2_ServerStream)
        ->Args({1, 8, 4, 256})
        ->Args({4, 8, 4, 256})
        ->Args({8, 8, 8, 256})
        ->Args({4, 32, 4, 256})
        ->Args({8, 32, 8, 256})
        ->UseManualTime()
        ->MinTime(0.7)
        ->Unit(benchmark::kSecond);

BENCHMARK_MAIN();
