#include <NeForce/core/utility/packages.hpp>
#include <NeForce/core/memory/byte_cursor.hpp>
#include <NeForce/network/http/http2_connection.hpp>
#include <algorithm>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

namespace {
    constexpr size_t WINDOW_UPDATE_THRESHOLD = 65535;
    constexpr size_t MAX_READ_BUFFER_SIZE = 65536;    // 64 KB
    constexpr size_t MAX_WRITE_BUFFER_SIZE = 1048576; // 1 MB

    vector<hpack_header_field> response_headers_to_hpack(int status_code,
                                                         const unordered_map<string, string>& headers) {
        vector<hpack_header_field> result;
        result.push_back({":status", to_string(status_code)});

        for (const auto& h: headers) {
            const string lower = h.first.lowercase();
            if (lower == "connection" || lower == "transfer-encoding" || lower == "keep-alive" ||
                lower.starts_with(":")) {
                continue;
            }
            result.push_back({move(lower), h.second});
        }
        return result;
    }
} // namespace


http2_connection::http2_connection(unique_ptr<tcp_socket> socket, io_context& ctx) :
socket_(move(socket)),
ctx_(&ctx),
encoder_(local_settings_.header_table_size()),
decoder_(remote_settings_.header_table_size()),
flow_control_(local_settings_.initial_window_size()) {
    local_stream_window_ = local_settings_.initial_window_size();
}

http2_connection::~http2_connection() {
    try {
        close_connection();
        // NOLINTNEXTLINE(bugprone-empty-catch)
    } catch (...) {
        // ignore
    }
}

void http2_connection::start() {
    const int fd = static_cast<int>(socket_->native_handle());
    auto weak_self = weak_from_this();
    ctx_->add_fd(fd, epoll_in, [weak_self](int, uint32_t, error_code) {
        auto self = weak_self.lock();
        if (!self || self->closed_) {
            return;
        }
        self->on_readable(0, 0, error_code{});
    });

    http2_settings_frame sf;
    sf.entries.push_back({http2_settings_id::MAX_CONCURRENT_STREAMS, local_settings_.max_concurrent_streams()});
    sf.entries.push_back({http2_settings_id::INITIAL_WINDOW_SIZE, local_settings_.initial_window_size()});
    sf.entries.push_back({http2_settings_id::MAX_FRAME_SIZE, local_settings_.max_frame_size()});
    sf.entries.push_back({http2_settings_id::HEADER_TABLE_SIZE, local_settings_.header_table_size()});
    write_frame(framer_.encode_settings_frame(sf));
    flush_writes();
}

void http2_connection::on_readable(int /*fd*/, uint32_t /*events*/, error_code /*ec*/) {
    lock<recursive_mutex> lk(stream_mutex_);
    if (closed_) {
        return;
    }

    byte_t buf[16384];
    const ssize_t n = socket_->receive(memory_view<char>(reinterpret_cast<char*>(buf), sizeof(buf)));
    if (n <= 0) {
        close_connection(http2_error::INTERNAL_ERROR);
        return;
    }

    if (read_buffer_.size() + static_cast<size_t>(n) > MAX_READ_BUFFER_SIZE) {
        close_connection(http2_error::ENHANCE_YOUR_CALM);
        return;
    }

    read_buffer_.insert(read_buffer_.end(), buf, buf + n);

    if (!preface_received_) {
        if (read_buffer_.size() >= 24) {
            const string_view received(reinterpret_cast<const char*>(read_buffer_.data()), 24);
            if (received != HTTP2_CLIENT_PREFACE) {
                close_connection(http2_error::PROTOCOL_ERROR);
                return;
            }
            preface_received_ = true;
            read_buffer_.erase(read_buffer_.begin(), read_buffer_.begin() + 24);

            // Send server's initial SETTINGS frame (RFC 7540 §3.5)
            http2_settings_frame server_settings;
            server_settings.ack = false;
            server_settings.entries = {
                    {http2_settings_id::MAX_CONCURRENT_STREAMS, 100},
                    {http2_settings_id::INITIAL_WINDOW_SIZE, 65535},
            };
            write_frame(http2_framer::encode_settings_frame(server_settings));
        } else {
            return;
        }
    }

    framer_.decode_frames(read_buffer_.data(), read_buffer_.size(),
                          [this](http2_frame_type type, uint8_t flags, uint32_t stream_id, const byte_t* payload,
                                 size_t len) { handle_frame(type, flags, stream_id, payload, len); });

    // Frame decoder internally buffers incomplete frame data; read_buffer_ can be safely cleared
    read_buffer_.clear();

    // Flush any pending write data after each read
    flush_writes();
}

void http2_connection::on_writable(int /*fd*/, uint32_t /*events*/, error_code /*ec*/) { flush_writes(); }

void http2_connection::handle_frame(http2_frame_type type, uint8_t flags, uint32_t stream_id, const byte_t* payload,
                                    size_t len) {
    // Check the flow limits after GOAWAY
    if (last_stream_id_ > 0 && stream_id > last_stream_id_) {
        return;
    }

    switch (type) {
        case http2_frame_type::DATA:
            handle_data_frame(flags, stream_id, payload, len);
            break;
        case http2_frame_type::HEADERS:
            handle_headers_frame(flags, stream_id, payload, len);
            break;
        case http2_frame_type::SETTINGS:
            handle_settings_frame(flags, payload, len);
            break;
        case http2_frame_type::PING:
            handle_ping_frame(flags, payload, len);
            break;
        case http2_frame_type::GOAWAY:
            handle_goaway_frame(payload, len);
            break;
        case http2_frame_type::RST_STREAM:
            handle_rst_stream_frame(stream_id, payload);
            break;
        case http2_frame_type::WINDOW_UPDATE:
            handle_window_update_frame(stream_id, payload);
            break;
        case http2_frame_type::PRIORITY:
            handle_priority_frame(stream_id, payload, len);
            break;
        case http2_frame_type::PUSH_PROMISE:
            handle_push_promise_frame(stream_id, payload, len);
            break;
        case http2_frame_type::CONTINUATION:
            handle_continuation_frame(flags, stream_id, payload, len);
            break;
    }
}

void http2_connection::handle_data_frame(uint8_t flags, uint32_t stream_id, const byte_t* payload, size_t len) {
    auto it = pending_.find(stream_id);
    if (it == pending_.end()) {
        send_rst_stream(stream_id, http2_error::STREAM_CLOSED);
        return;
    }

    flow_control_.consume(stream_id, static_cast<uint32_t>(len));
    connection_consumed_ += static_cast<uint32_t>(len);

    // Send WINDOW_UPDATE (Connection Level + Stream Level)
    if (connection_consumed_ >= WINDOW_UPDATE_THRESHOLD) {
        send_window_update(0, connection_consumed_);
        connection_consumed_ = 0;
    }

    auto stream_it = stream_consumed_.find(stream_id);
    uint32_t& stream_consumed = stream_consumed_[stream_id];
    stream_consumed += static_cast<uint32_t>(len);
    if (stream_consumed >= WINDOW_UPDATE_THRESHOLD) {
        send_window_update(stream_id, stream_consumed);
        stream_consumed = 0;
    }

    const bool end_stream = (flags & HTTP2_FLAG_END_STREAM) != 0;
    it->second.data.insert(it->second.data.end(), payload, payload + len);
    it->second.end_stream = end_stream;

    if (end_stream) {
        auto stream_it2 = streams_.find(stream_id);
        if (stream_it2 != streams_.end()) {
            stream_it2->second->on_receive_data(true);
            if (stream_it2->second->is_closed()) {
                streams_.erase(stream_it2);
                stream_priorities_.erase(stream_id);
            }
        }

        if (router_ != nullptr) {
            route_stream(stream_id, it->second.headers, it->second.data.data(), it->second.data.size(), true);
        } else if (stream_handler_) {
            stream_handler_(stream_id, it->second.headers, it->second.data.data(), it->second.data.size(), true);
        }
        pending_.erase(it);
        stream_consumed_.erase(stream_id);
    }
}

void http2_connection::handle_headers_frame(uint8_t flags, uint32_t stream_id, const byte_t* payload, size_t len) {
    const bool end_stream = (flags & HTTP2_FLAG_END_STREAM) != 0;
    const bool end_headers = (flags & HTTP2_FLAG_END_HEADERS) != 0;

    if (streams_.find(stream_id) == streams_.end()) {
        if (streams_.size() >= local_settings_.max_concurrent_streams()) {
            send_rst_stream(stream_id, http2_error::REFUSED_STREAM);
            return;
        }
        auto stream = make_unique<http2_stream>(stream_id);
        stream->on_receive_headers(end_stream);
        streams_[stream_id] = move(stream);
    }

    auto& ps = pending_[stream_id];
    ps.header_block_fragment.insert(ps.header_block_fragment.end(), payload, payload + len);
    ps.end_stream = end_stream;

    // If is waiting CONTINUATION
    if (!end_headers) {
        ps.waiting_continuation = true;
        return;
    }

    ps.headers = decoder_.decode(ps.header_block_fragment.data(), ps.header_block_fragment.size());
    ps.header_block_fragment.clear();
    ps.waiting_continuation = false;

    if (end_stream) {
        auto stream_it = streams_.find(stream_id);
        if (stream_it != streams_.end() && stream_it->second->is_closed()) {
            streams_.erase(stream_it);
            stream_priorities_.erase(stream_id);
        }

        if (router_ != nullptr) {
            route_stream(stream_id, ps.headers, nullptr, 0, true);
        } else if (stream_handler_) {
            stream_handler_(stream_id, ps.headers, nullptr, 0, true);
        }
        pending_.erase(stream_id);
    }
}

void http2_connection::handle_continuation_frame(uint8_t flags, uint32_t stream_id, const byte_t* payload, size_t len) {
    const bool end_headers = (flags & HTTP2_FLAG_END_HEADERS) != 0;

    auto it = pending_.find(stream_id);
    if (it == pending_.end() || !it->second.waiting_continuation) {
        close_connection(http2_error::PROTOCOL_ERROR);
        return;
    }

    it->second.header_block_fragment.insert(it->second.header_block_fragment.end(), payload, payload + len);

    if (end_headers) {
        it->second.headers =
                decoder_.decode(it->second.header_block_fragment.data(), it->second.header_block_fragment.size());
        it->second.header_block_fragment.clear();
        it->second.waiting_continuation = false;

        if (it->second.end_stream) {
            if (router_ != nullptr) {
                route_stream(stream_id, it->second.headers, nullptr, 0, true);
            } else if (stream_handler_) {
                stream_handler_(stream_id, it->second.headers, nullptr, 0, true);
            }
            pending_.erase(it);
        }
    }
}

void http2_connection::handle_push_promise_frame(uint32_t /*stream_id*/, const byte_t* payload, size_t len) {
    // Server Push只应由服务器发送，客户端接收到PUSH_PROMISE是正常的
    byte_cursor cur(payload, len);

    // PUSH_PROMISE payload: Pad Length? + R(1) + Promised Stream ID(31) + Header Block Fragment + Padding
    auto psid_opt = cur.try_read_be32();
    if (!psid_opt) {
        return;
    }
    uint32_t promised_stream_id = *psid_opt & 0x7FFFFFFF;

    // Save header block fragment to pending_
    auto& ps = pending_[promised_stream_id];
    ps.header_block_fragment.insert(ps.header_block_fragment.end(), cur.data(), cur.data() + cur.remaining());
}

void http2_connection::handle_priority_frame(uint32_t stream_id, const byte_t* payload, size_t len) {
    byte_cursor cur(payload, len);
    cur.skip(4); // stream dependency (4 bytes)
    auto weight = cur.try_read_byte();
    if (weight) {
        stream_priorities_[stream_id] = *weight;
    }
}

void http2_connection::handle_settings_frame(uint8_t flags, const byte_t* payload, size_t len) {
    if ((flags & HTTP2_FLAG_ACK) != 0) {
        return;
    }

    byte_cursor cur(payload, len);
    http2_settings_frame sf;
    while (cur.remaining() >= 6) {
        auto id_opt = cur.try_read_be16();
        auto val_opt = cur.try_read_be32();
        if (!id_opt || !val_opt) {
            break;
        }
        http2_settings_entry entry;
        entry.id = static_cast<http2_settings_id>(*id_opt);
        entry.value = *val_opt;
        sf.entries.push_back(entry);
    }

    remote_settings_.apply_remote_settings(sf);

    encoder_.set_max_table_size(remote_settings_.header_table_size());
    decoder_.set_max_table_size(remote_settings_.header_table_size());
    flow_control_.set_initial_window(remote_settings_.initial_window_size());

    http2_settings_frame ack;
    ack.ack = true;
    write_frame(framer_.encode_settings_frame(ack));
}

void http2_connection::handle_ping_frame(uint8_t flags, const byte_t* payload, size_t /*len*/) {
    if ((flags & HTTP2_FLAG_ACK) != 0) {
        return;
    }

    byte_cursor cur(payload, 8);
    auto data_opt = cur.try_read_be64();
    if (!data_opt) {
        return;
    }

    http2_ping_frame ping;
    ping.ack = true;
    ping.opaque_data = *data_opt;
    write_frame(framer_.encode_ping_frame(ping));
}

void http2_connection::handle_goaway_frame(const byte_t* payload, size_t /*len*/) {
    byte_cursor cur(payload, 8);
    auto sid_opt = cur.try_read_be32();
    if (!sid_opt) {
        return;
    }
    last_stream_id_ = *sid_opt & 0x7FFFFFFF;
    close_connection();
}

void http2_connection::handle_rst_stream_frame(uint32_t stream_id, const byte_t* /*payload*/) {
    auto stream_it = streams_.find(stream_id);
    if (stream_it != streams_.end()) {
        stream_it->second->on_receive_rst_stream();
        streams_.erase(stream_it);
        stream_priorities_.erase(stream_id);
    }
    pending_.erase(stream_id);
    stream_consumed_.erase(stream_id);
}

void http2_connection::handle_window_update_frame(uint32_t stream_id, const byte_t* payload) {
    byte_cursor cur(payload, 4);
    auto inc_opt = cur.try_read_be32();
    if (!inc_opt) {
        return;
    }
    uint32_t increment = *inc_opt & 0x7FFFFFFF;

    // RFC 7540 §6.9: WINDOW_UPDATE with increment of 0 is a PROTOCOL_ERROR
    if (increment == 0) {
        close_connection(http2_error::PROTOCOL_ERROR);
        return;
    }

    // RFC 7540 §6.9: window must not exceed 2^31-1
    uint32_t current = (stream_id == 0) ? flow_control_.connection_window() : flow_control_.window(stream_id);
    if (static_cast<uint64_t>(current) + increment > 0x7FFFFFFF) {
        send_goaway(http2_error::FLOW_CONTROL_ERROR);
        return;
    }

    flow_control_.add_window(stream_id, increment);
}

void http2_connection::send_window_update(uint32_t stream_id, uint32_t increment) {
    increment &= 0x7FFFFFFF;
    if (increment == 0) {
        return;
    }
    http2_window_update_frame wuf;
    wuf.stream_id = stream_id;
    wuf.window_size_increment = increment;
    write_frame(framer_.encode_window_update_frame(wuf));
}

void http2_connection::send_push_promise(uint32_t stream_id, uint32_t promised_stream_id,
                                         const vector<hpack_header_field>& request_headers) {
    if (closed_) {
        return;
    }
    if (!remote_settings_.enable_push()) {
        return;
    }
    if ((promised_stream_id & 1) != 0 || promised_stream_id == 0) {
        return;
    }

    byte_vector header_block = encoder_.encode(request_headers);

    auto stream = make_unique<http2_stream>(promised_stream_id);
    stream->on_send_headers(false);
    streams_[promised_stream_id] = move(stream);

    http2_push_promise_frame ppf;
    ppf.stream_id = stream_id;
    ppf.promised_stream_id = promised_stream_id;
    ppf.header_block = header_block;
    ppf.end_headers = true;
    write_frame(framer_.encode_push_promise_frame(ppf));
    last_stream_id_ = max(promised_stream_id, last_stream_id_);
}

void http2_connection::send_response(uint32_t stream_id, const vector<hpack_header_field>& headers, const string& body,
                                     bool end_stream) {
    byte_vector header_block = encoder_.encode(headers);
    const uint32_t max_frame = local_settings_.max_frame_size();

    // Split header block into HEADERS + CONTINUATION if it exceeds max frame size
    if (header_block.size() > max_frame) {
        size_t offset = 0;
        bool first = true;

        while (offset < header_block.size()) {
            size_t chunk_size = header_block.size() - offset;
            chunk_size = min<size_t>(chunk_size, max_frame);
            bool is_last = (offset + chunk_size >= header_block.size());

            if (first) {
                http2_headers_frame hf;
                hf.stream_id = stream_id;
                hf.header_block.assign(header_block.begin() + static_cast<ptrdiff_t>(offset),
                                       header_block.begin() + static_cast<ptrdiff_t>(offset + chunk_size));
                hf.end_headers = is_last;
                hf.end_stream = is_last && end_stream && body.empty();
                write_frame(framer_.encode_headers_frame(hf));
                first = false;
            } else {
                http2_continuation_frame cf;
                cf.stream_id = stream_id;
                cf.header_block.assign(header_block.begin() + static_cast<ptrdiff_t>(offset),
                                       header_block.begin() + static_cast<ptrdiff_t>(offset + chunk_size));
                cf.end_headers = is_last;
                write_frame(framer_.encode_continuation_frame(cf));
            }

            offset += chunk_size;
        }
    } else {
        http2_headers_frame hf;
        hf.stream_id = stream_id;
        hf.header_block = header_block;
        hf.end_headers = true;
        hf.end_stream = end_stream && body.empty();

        write_frame(framer_.encode_headers_frame(hf));
    }

    if (!body.empty()) {
        const auto* data_ptr = reinterpret_cast<const byte_t*>(body.data());
        size_t remaining = body.size();

        while (remaining > 0) {
            uint32_t available = flow_control_.window(stream_id);
            available = min(available, flow_control_.connection_window());
            if (available == 0) {
                available = 1;
            }

            size_t chunk = (remaining < available) ? remaining : static_cast<size_t>(available);
            chunk = (chunk > local_settings_.max_frame_size()) ? local_settings_.max_frame_size() : chunk;

            http2_data_frame df;
            df.stream_id = stream_id;
            df.end_stream = (chunk >= remaining) && end_stream;
            df.data.assign(data_ptr, data_ptr + chunk);

            flow_control_.consume(stream_id, static_cast<uint32_t>(chunk));
            write_frame(framer_.encode_data_frame(df));

            data_ptr += chunk;
            remaining -= chunk;
        }
    }

    if (end_stream) {
        lock<recursive_mutex> lk(stream_mutex_);
        auto stream_it = streams_.find(stream_id);
        if (stream_it != streams_.end()) {
            stream_it->second->on_send_data(true);
            if (stream_it->second->is_closed()) {
                streams_.erase(stream_it);
                stream_priorities_.erase(stream_id);
            }
        }
    }
    flush_writes();
}

void http2_connection::send_rst_stream(uint32_t stream_id, http2_error error) {
    lock<recursive_mutex> lk(stream_mutex_);
    http2_rst_stream_frame rst;
    rst.stream_id = stream_id;
    rst.error_code = error;
    write_frame(framer_.encode_rst_stream_frame(rst));
    streams_.erase(stream_id);
    stream_priorities_.erase(stream_id);
    pending_.erase(stream_id);
    stream_consumed_.erase(stream_id);
}

void http2_connection::send_goaway(http2_error error) {
    http2_goaway_frame goaway;
    goaway.last_stream_id = last_stream_id_;
    goaway.error_code = error;
    write_frame(framer_.encode_goaway_frame(goaway));
    close_connection(error);
}

void http2_connection::write_frame(const byte_vector& frame) {
    if (frame.empty()) {
        return;
    }
    unique_lock<mutex> lk(write_mutex_);
    if (write_buffer_.size() + frame.size() > MAX_WRITE_BUFFER_SIZE) {
        lk.unlock_quiet();
        close_connection(http2_error::ENHANCE_YOUR_CALM);
        return;
    }
    write_buffer_.insert(write_buffer_.end(), frame.begin(), frame.end());
}

void http2_connection::flush_writes() {
    lock<mutex> lk(write_mutex_);
    if (write_buffer_.empty()) {
        return;
    }
    socket_->send_all(
            memory_view<const char>(reinterpret_cast<const char*>(write_buffer_.data()), write_buffer_.size()));
    write_buffer_.clear();
}

void http2_connection::stop() {
    if (closed_) {
        return;
    }
    closed_ = true;
    const int fd = static_cast<int>(socket_->native_handle());
    ctx_->remove_fd(fd);
}

void http2_connection::close_connection(http2_error error) {
    if (closed_) {
        return;
    }
    closed_ = true;

    flush_writes();

    const int fd = static_cast<int>(socket_->native_handle());
    ctx_->remove_fd(fd);

    if (close_handler_) {
        close_handler_(static_cast<uint32_t>(error));
    }
}

void http2_connection::handle_upgrade_request(uint32_t stream_id, const vector<hpack_header_field>& headers,
                                              const byte_t* data, size_t data_len, bool end_stream) {
    route_stream(stream_id, headers, data, data_len, end_stream);
}

void http2_connection::route_stream(uint32_t stream_id, const vector<hpack_header_field>& headers, const byte_t* data,
                                    size_t data_len, bool end_stream) {
    if (router_ == nullptr) {
        return;
    }

    http_request req;

    for (const auto& h: headers) {
        if (h.name == ":method") {
            req.method = http_method(h.value);
        } else if (h.name == ":path") {
            const size_t qpos = h.value.find('?');
            if (qpos != string::npos) {
                req.path = h.value.substr(0, qpos);
                req.query = h.value.substr(qpos + 1);
            } else {
                req.path = h.value;
            }
        } else if (h.name == ":authority") {
            req.set_header("Host", h.value);
        } else if (h.name == ":scheme") {
            req.set_header("X-Forwarded-Proto", h.value);
        } else if (!h.name.starts_with(":")) {
            req.set_header(h.name, h.value);
        }
    }

    if (data != nullptr && data_len > 0) {
        req.body.assign(reinterpret_cast<const char*>(data), data_len);
    }

    auto weak_self = weak_from_this();
    router_->handle_request_async(move(req), [weak_self, stream_id, end_stream](http_response res) {
        auto self = weak_self.lock();
        if (!self || self->closed_) {
            return;
        }

        if (res.status >= http_status::S1_SWITCHING_PROTOCOLS) {
            auto hpack_headers =
                    response_headers_to_hpack(static_cast<int>(static_cast<uint16_t>(res.status)), res.headers);
            self->send_response(stream_id, hpack_headers, res.body, end_stream);
        }
    });
}

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
