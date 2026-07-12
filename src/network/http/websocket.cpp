#include <NeForce/core/numeric/random.hpp>
#include <NeForce/core/memory/byte_cursor.hpp>
#include <NeForce/network/http/websocket.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

namespace {
    constexpr size_t max_write_queue_size = 1024;
    constexpr size_t max_ctrl_queue_size = 256;
    constexpr seconds heartbeat_interval_sec{30};
    constexpr seconds heartbeat_timeout_sec{10};

    int64_t now_ms() noexcept { return time_cast<milliseconds>(steady_clock::now().since_epoch()).count(); }

    byte_vector build_frame(websocket_opcode opcode, const string& payload, bool masked, bool rsv1 = false) {
        byte_vector frame;
        frame.reserve(14 + payload.size());
        auto first_byte = static_cast<byte_t>(0x80 | (static_cast<uint8_t>(opcode) & 0x0F));
        if (rsv1) {
            first_byte |= 0x40;
        }
        frame.push_back(first_byte);

        const size_t len = payload.size();
        const byte_t second = masked ? 0x80 : 0x00;

        if (len < 126) {
            frame.push_back(static_cast<byte_t>(second | static_cast<byte_t>(len)));
        } else if (len <= 0xFFFF) {
            frame.push_back(static_cast<byte_t>(second | 126));
            const uint16_t net_len = endian::host_to_network(static_cast<uint16_t>(len));
            frame.insert(frame.end(), reinterpret_cast<const byte_t*>(&net_len),
                         reinterpret_cast<const byte_t*>(&net_len) + 2);
        } else {
            frame.push_back(static_cast<byte_t>(second | 127));
            const uint64_t net_len = endian::host_to_network(static_cast<uint64_t>(len));
            frame.insert(frame.end(), reinterpret_cast<const byte_t*>(&net_len),
                         reinterpret_cast<const byte_t*>(&net_len) + 8);
        }

        uint32_t masking_key = 0;
        if (masked) {
            thread_local random_mt tl_mt;
            masking_key = tl_mt.next_int<uint32_t>();
            frame.insert(frame.end(), reinterpret_cast<byte_t*>(&masking_key),
                         reinterpret_cast<byte_t*>(&masking_key) + 4);
        }

        if (len > 0) {
            const size_t base = frame.size();
            frame.insert(frame.end(), payload.begin(), payload.end());
            if (masked) {
                const auto* key = reinterpret_cast<const byte_t*>(&masking_key);
                for (size_t i = 0; i < len; ++i) {
                    frame[base + i] ^= key[i % 4];
                }
            }
        }

        return frame;
    }

    string make_close_payload(const websocket_status status, const string& reason) {
        const auto code = static_cast<uint16_t>(status);
        string payload;
        payload.reserve(2 + reason.size());
        payload.push_back(static_cast<char>((code >> 8) & 0xFF));
        payload.push_back(static_cast<char>(code & 0xFF));
        payload += reason;
        return payload;
    }

    bool receive_exact(tcp_socket& socket, void* buf, const size_t n) {
        auto* ptr = static_cast<char*>(buf);
        size_t remaining = n;
        while (remaining > 0) {
            const ssize_t got = socket.receive(memory_view<char>(ptr, remaining));
            if (got <= 0) {
                return false;
            }
            ptr += got;
            remaining -= static_cast<size_t>(got);
        }
        return true;
    }
} // namespace


bool websocket_server::handle_upgrade(const http_request& request, unique_ptr<tcp_socket> sock) {
    const auto it = route_handlers_.find(request.path);
    if (it == route_handlers_.end()) {
        return false;
    }

    const auto session = make_shared<websocket_session>(move(sock), this);
    if (ctx_ != nullptr) {
        session->set_io_context(*ctx_);
    }

#ifdef NEFORCE_SUPPORT_ZLIB
    const string_view extensions = request.header("Sec-WebSocket-Extensions");
    if (!extensions.empty()) {
        const auto deflate_cfg = websocket_deflate_config::negotiate(extensions);
        if (deflate_cfg.active) {
            session->set_deflate_config(deflate_cfg);
        }
    }
#endif

    {
        lock<mutex> lk(sessions_mutex_);
        sessions_.push_back(session);
    }
    it->second(session);
    session->start();
    return true;
}

void websocket_server::remove_session(const session_ptr& session) {
    lock<mutex> lk(sessions_mutex_);
    const auto it = find(sessions_.begin(), sessions_.end(), session);
    if (it != sessions_.end()) {
        sessions_.erase(it);
    }
}

websocket_server::~websocket_server() {
    try {
        stop();
        // NOLINTNEXTLINE(bugprone-empty-catch)
    } catch (...) {
        // ignore
    }
}

void websocket_server::stop() {
    vector<session_ptr> sessions_copy;
    {
        lock<mutex> lk(sessions_mutex_);
        sessions_copy.swap(sessions_);
    }
    for (const auto& session: sessions_copy) {
        session->stop();
    }
}

void websocket_server::broadcast(const string& data, const websocket_opcode opcode) {
    lock<mutex> lk(sessions_mutex_);
    for (const auto& s: sessions_) {
        if (s->is_open()) {
            s->send(data, opcode);
        }
    }
}

bool websocket_session::queue_frame(byte_vector frame, const bool is_control) {
    {
        lock<mutex> lk(write_mutex_);
        if (is_control) {
            if (ctrl_queue_.size() >= max_ctrl_queue_size) {
                return false;
            }
            ctrl_queue_.push(move(frame));
        } else {
            if (write_queue_.size() >= max_write_queue_size) {
                return false;
            }
            write_queue_.push(move(frame));
        }
    }
    if (event_driven_ && ctx_ != nullptr) {
        flush_event_writes();
    } else {
        write_cv_.notify_one();
    }
    return true;
}

void websocket_session::write_loop() {
    try {
        while (running_) {
            byte_vector frame;
            {
                unique_lock<mutex> lk(write_mutex_);
                write_cv_.wait(lk, [this] { return !running_ || !ctrl_queue_.empty() || !write_queue_.empty(); });
                if (!running_ && ctrl_queue_.empty() && write_queue_.empty()) {
                    break;
                }

                if (!ctrl_queue_.empty()) {
                    frame = ctrl_queue_.front();
                    ctrl_queue_.pop();
                } else if (!write_queue_.empty()) {
                    frame = write_queue_.front();
                    write_queue_.pop();
                } else {
                    continue;
                }
            }

            socket_->send_all(memory_view<const char>(reinterpret_cast<const char*>(frame.data()), frame.size()));
        }
    } catch (const exception& e) {
        if (on_error_) {
            try {
                on_error_(e);
                // NOLINTNEXTLINE(bugprone-empty-catch)
            } catch (...) {
                // ignore
            }
        }
        return;
    }

    try {
        lock<mutex> lk(write_mutex_);
        while (!ctrl_queue_.empty()) {
            auto& f = ctrl_queue_.front();
            socket_->send_all(memory_view<const char>(reinterpret_cast<const char*>(f.data()), f.size()));
            ctrl_queue_.pop();
        }
        // NOLINTNEXTLINE(bugprone-empty-catch)
    } catch (...) {
        // ignore
    }
}

void websocket_session::read_loop() {
    try {
        while (running_) {
            if (!read_frame()) {
                break;
            }
        }
        do_stop(websocket_status::ABNORMAL_CLOSURE, "Connection lost");
    } catch (const exception& e) {
        if (on_error_) {
            try {
                on_error_(e);
                // NOLINTNEXTLINE(bugprone-empty-catch)
            } catch (...) {
                // ignore
            }
        }
    }
}

bool websocket_session::read_frame() {
    try {
        websocket_frame_header hdr{};
        if (!receive_exact(*socket_, &hdr, 2)) {
            return false;
        }

        const auto opcode = static_cast<websocket_opcode>(hdr.opcode);
        const bool is_ctrl = (hdr.opcode >= 0x8);

        if (hdr.rsv2 || hdr.rsv3) {
            send_close_frame(websocket_status::PROTOCOL_ERROR, "Reserved bits set");
            return false;
        }
        if (hdr.rsv1) {
            if (!deflate_config_.active || is_ctrl) {
                send_close_frame(websocket_status::PROTOCOL_ERROR, "RSV1 set without negotiation");
                return false;
            }
        }

        if (is_ctrl && !hdr.fin) {
            send_close_frame(websocket_status::PROTOCOL_ERROR, "Fragmented control frame");
            return false;
        }

        uint64_t payload_len = hdr.payload_len;
        if (payload_len == 126) {
            uint16_t ext{};
            if (!receive_exact(*socket_, &ext, 2)) {
                return false;
            }
            payload_len = endian::network_to_host<uint16_t>(ext);
        } else if (payload_len == 127) {
            uint64_t ext{};
            if (!receive_exact(*socket_, &ext, 8)) {
                return false;
            }
            payload_len = endian::network_to_host<uint64_t>(ext);
        }

        if (is_ctrl && payload_len > 125) {
            send_close_frame(websocket_status::PROTOCOL_ERROR, "Control frame payload too large");
            return false;
        }

        constexpr uint64_t max_payload_length = 64ULL * 1024ULL * 1024ULL;
        if (payload_len > max_payload_length) {
            send_close_frame(websocket_status::MESSAGE_TOO_BIG, "Payload exceeds limit");
            return false;
        }

        uint32_t masking_key = 0;
        if (hdr.masked) {
            if (!receive_exact(*socket_, &masking_key, 4)) {
                return false;
            }
        }

        string payload;
        if (payload_len > 0) {
            payload.resize(static_cast<size_t>(payload_len));
            if (!receive_exact(*socket_, payload.data(), static_cast<size_t>(payload_len))) {
                return false;
            }
        }

        if (hdr.masked && payload_len > 0) {
            const auto* key_bytes = reinterpret_cast<const byte_t*>(&masking_key);
            auto* payload_bytes = reinterpret_cast<byte_t*>(payload.data());
            for (size_t i = 0; i < static_cast<size_t>(payload_len); ++i) {
                payload_bytes[i] ^= key_bytes[i % 4];
            }
        }

        return dispatch(hdr, opcode, payload);

    } catch (const exception& e) {
        if (on_error_) {
            try {
                on_error_(e);
                // NOLINTNEXTLINE(bugprone-empty-catch)
            } catch (...) {
                // ignore
            }
        }
        return false;
    }
}

bool websocket_session::dispatch(const websocket_frame_header& hdr, const websocket_opcode opcode, string payload) {
    switch (opcode) {
        case websocket_opcode::TEXT:
        case websocket_opcode::BINARY: {
            if (in_fragment_) {
                send_close_frame(websocket_status::PROTOCOL_ERROR, "New data frame before fragment complete");
                return false;
            }
            if (hdr.rsv1 && deflate_decompressor_) {
                if (!hdr.fin) {
                    fragment_opcode_ = opcode;
                    deflate_fragment_buffer_.insert(deflate_fragment_buffer_.end(), payload.begin(), payload.end());
                    in_fragment_ = true;
                } else {
                    try {
                        const string decompressed = deflate_decompressor_->process(payload.view(), true);
                        deliver_message(decompressed, opcode);
                    } catch (...) {
                        deliver_message(payload, opcode);
                    }
                    if (deflate_config_.client_no_context_takeover) {
                        deflate_decompressor_->reset_context();
                    }
                }
            } else {
                if (!hdr.fin) {
                    fragment_opcode_ = opcode;
                    fragment_buffer_ = move(payload);
                    in_fragment_ = true;
                } else {
                    deliver_message(payload, opcode);
                }
            }
            return true;
        }
        case websocket_opcode::CONTINUATION: {
            if (!in_fragment_) {
                send_close_frame(websocket_status::PROTOCOL_ERROR, "Unexpected continuation frame");
                return false;
            }
            if (hdr.rsv1 && deflate_decompressor_ && !deflate_fragment_buffer_.empty()) {
                deflate_fragment_buffer_.insert(deflate_fragment_buffer_.end(), payload.begin(), payload.end());
                if (hdr.fin) {
                    try {
                        const string compressed(reinterpret_cast<const char*>(deflate_fragment_buffer_.data()),
                                                deflate_fragment_buffer_.size());
                        const string decompressed = deflate_decompressor_->process(compressed.view(), true);
                        deliver_message(decompressed, fragment_opcode_);
                    } catch (...) {
                        fragment_buffer_.clear();
                    }
                    deflate_fragment_buffer_.clear();
                    if (deflate_config_.client_no_context_takeover) {
                        deflate_decompressor_->reset_context();
                    }
                    in_fragment_ = false;
                }
            } else {
                fragment_buffer_ += move(payload);
                if (hdr.fin) {
                    deliver_message(fragment_buffer_, fragment_opcode_);
                    fragment_buffer_.clear();
                    in_fragment_ = false;
                }
            }
            return true;
        }
        case websocket_opcode::PING: {
            queue_frame(build_frame(websocket_opcode::PONG, payload, false), true);
            return true;
        }
        case websocket_opcode::PONG: {
            last_pong_ms_ = now_ms();
            ping_pending_ = false;
            return true;
        }
        case websocket_opcode::CLOSE: {
            handle_close_frame(move(payload));
            return false;
        }
        default: {
            send_close_frame(websocket_status::PROTOCOL_ERROR, "Unknown opcode");
            return false;
        }
    }
}

void websocket_session::deliver_message(const string& data, websocket_opcode opcode) {
    if (on_message_) {
        try {
            on_message_(data, move(opcode));
        } catch (const exception& e) {
            if (on_error_) {
                try {
                    on_error_(e);
                    // NOLINTNEXTLINE(bugprone-empty-catch)
                } catch (...) {
                    // ignore
                }
            }
        }
    }
}

void websocket_session::send_close_frame(const websocket_status status, const string& reason) {
    auto frame = build_frame(websocket_opcode::CLOSE, make_close_payload(status, reason), false);
    queue_frame(move(frame), true);
}

void websocket_session::handle_close_frame(string payload) {
    auto status = websocket_status::NORMAL_CLOSURE;
    string reason;

    if (payload.size() >= 2) {
        const uint16_t code = (static_cast<uint8_t>(payload[0]) << 8) | static_cast<uint8_t>(payload[1]);
        status = static_cast<websocket_status>(code);
        if (payload.size() > 2) {
            reason = payload.tail(2);
        }
    }

    send_close_frame(status, reason);
    do_stop(status, reason);
}

void websocket_session::heartbeat_loop() {
    try {
        last_pong_ms_ = now_ms();

        while (running_) {
            for (int i = 0; i < heartbeat_interval_sec.count() * 10 && running_; ++i) {
                this_thread::sleep_for(milliseconds(100));
            }
            if (!running_) {
                break;
            }

            const int64_t elapsed_ms = now_ms() - last_pong_ms_.load();
            constexpr milliseconds timeout_ms = heartbeat_timeout_sec.to_milli();

            if (ping_pending_.load() && elapsed_ms > timeout_ms.count()) {
                do_stop(websocket_status::ABNORMAL_CLOSURE, "Heartbeat timeout");
                return;
            }

            if (!ping_pending_.load()) {
                ping_pending_ = true;
                queue_frame(build_frame(websocket_opcode::PING, "", false), true);
            }
        }
    } catch (const exception& e) {
        if (on_error_) {
            try {
                on_error_(e);
                // NOLINTNEXTLINE(bugprone-empty-catch)
            } catch (...) {
                // ignore
            }
        }
    }
}

void websocket_session::do_stop(websocket_status status, const string& reason, bool notify_server) {
    if (closed_once_.test_and_set()) {
        return;
    }

    running_ = false;

    if (event_driven_ && ctx_ != nullptr) {
        if (heartbeat_timer_id_ != 0) {
            ctx_->cancel_timer(heartbeat_timer_id_);
            heartbeat_timer_id_ = 0;
        }
        const int fd = static_cast<int>(socket_->native_handle());
        ctx_->remove_fd(fd);
    } else {
        write_cv_.notify_all();
    }

    socket_->close();

    if (!event_driven_) {
        auto join_if_not_self = [](thread& t) {
            if (t.joinable() && t.get_id() != this_thread::id()) {
                t.join();
            } else if (t.joinable()) {
                t.detach();
            }
        };

        join_if_not_self(read_thread_);
        join_if_not_self(write_thread_);
        join_if_not_self(heartbeat_thread_);
    }

    if (on_close_) {
        try {
            on_close_(move(status), reason);
            // NOLINTNEXTLINE(bugprone-empty-catch)
        } catch (...) {
            // ignore
        }
    }

    if (notify_server && server_ != nullptr) {
        server_->remove_session(shared_from_this());
    }
}

websocket_session::websocket_session(unique_ptr<tcp_socket> sock, websocket_server* server) :
socket_(move(sock)),
server_(server) {}

websocket_session::~websocket_session() {
    running_ = false;
    write_cv_.notify_all();
    socket_->close();

    auto join_if_not_self = [](thread& t) {
        if (t.joinable() && t.get_id() != this_thread::id()) {
            t.join();
        } else if (t.joinable()) {
            t.detach();
        }
    };
    join_if_not_self(read_thread_);
    join_if_not_self(write_thread_);
    join_if_not_self(heartbeat_thread_);
}

void websocket_session::start() {
    if (running_.exchange(true)) {
        return;
    }
    last_pong_ms_ = now_ms();

    if (event_driven_ && ctx_ != nullptr) {
        start_event_driven();
    } else {
        socket_->set_nonblocking(false);
        read_thread_.start(&websocket_session::read_loop, this);
        write_thread_.start(&websocket_session::write_loop, this);
        heartbeat_thread_.start(&websocket_session::heartbeat_loop, this);
    }
}

void websocket_session::close(const websocket_status status, const string& reason) {
    if (!running_) {
        return;
    }
    send_close_frame(status, reason);

    const auto deadline = steady_clock::now() + seconds(2);
    while (steady_clock::now() < deadline) {
        {
            lock<mutex> lk(write_mutex_);
            if (ctrl_queue_.empty() && write_queue_.empty()) {
                break;
            }
        }
        this_thread::sleep_for(milliseconds(10));
    }
    do_stop(status, reason);
}

void websocket_session::stop() { do_stop(websocket_status::NORMAL_CLOSURE, "Stopped"); }

bool websocket_session::send(const string& data, const websocket_opcode opcode) {
    if (!running_) {
        return false;
    }

#ifdef NEFORCE_SUPPORT_ZLIB
    if (deflate_compressor_ && (opcode == websocket_opcode::TEXT || opcode == websocket_opcode::BINARY)) {
        try {
            const string compressed = deflate_compressor_->process(data.view(), true);
            if (!compressed.empty()) {
                const bool ok = queue_frame(build_frame(opcode, compressed, false, true));
                if (deflate_config_.server_no_context_takeover) {
                    deflate_compressor_->reset_context();
                }
                return ok;
            }
            // NOLINTNEXTLINE(bugprone-empty-catch)
        } catch (...) {
            // ignore
        }
    }
#endif

    return queue_frame(build_frame(opcode, data, false));
}

void websocket_session::set_deflate_config(const websocket_deflate_config& cfg) {
    deflate_config_ = cfg;
#ifdef NEFORCE_SUPPORT_ZLIB
    if (cfg.active) {
        deflate_compressor_ =
                make_unique<websocket_deflate>(true, cfg.server_max_window_bits, cfg.server_no_context_takeover);
        deflate_decompressor_ =
                make_unique<websocket_deflate>(false, cfg.client_max_window_bits, cfg.client_no_context_takeover);
    }
#endif
}

void websocket_session::start_event_driven() {
    socket_->set_nonblocking(true);
    const int fd = static_cast<int>(socket_->native_handle());

    auto self = shared_from_this();

    ctx_->add_fd(fd, epoll_in, [self](int f, uint32_t ev, error_code) {
        if ((ev & epoll_in) != 0U) {
            self->on_readable(f, ev, error_code{});
        }
        if ((ev & epoll_out) != 0U) {
            self->on_writable(f, ev, error_code{});
        }
    });

    heartbeat_timer_id_ = ctx_->schedule_timer(static_cast<uint64_t>(heartbeat_interval_sec.count() * 1000),
                                               [self]() { self->on_heartbeat_timer(); });
}

void websocket_session::on_readable(int /*fd*/, uint32_t /*events*/, error_code /*ec*/) {
    if (!running_) {
        return;
    }

    byte_t buf[8192];
    while (true) {
        const ssize_t n = socket_->receive(memory_view<char>(reinterpret_cast<char*>(buf), sizeof(buf)));
        if (n > 0) {
            read_buffer_.insert(read_buffer_.end(), buf, buf + n);
        } else {
            break;
        }
    }

    try_parse_frames();
}

void websocket_session::try_parse_frames() {
    while (running_ && read_buffer_.size() >= 2) {
        byte_cursor cur(read_buffer_.data(), read_buffer_.size());

        auto b0_opt = cur.try_read_byte();
        auto b1_opt = cur.try_read_byte();
        if (!b0_opt || !b1_opt) {
            return;
        }
        const uint8_t b0 = *b0_opt;
        const uint8_t b1 = *b1_opt;

        websocket_frame_header hdr;
        hdr.opcode = b0 & 0x0F;
        hdr.rsv3 = (b0 >> 4) & 0x01;
        hdr.rsv2 = (b0 >> 5) & 0x01;
        hdr.rsv1 = (b0 >> 6) & 0x01;
        hdr.fin = b0 >> 7;
        hdr.payload_len = b1 & 0x7F;
        hdr.masked = b1 >> 7;

        const auto opcode = static_cast<websocket_opcode>(hdr.opcode);
        const bool is_ctrl_ev = (hdr.opcode >= 0x8);

        if (hdr.rsv2 || hdr.rsv3) {
            send_close_frame(websocket_status::PROTOCOL_ERROR, "Reserved bits set");
            break;
        }
        if (hdr.rsv1 && (!deflate_config_.active || is_ctrl_ev)) {
            send_close_frame(websocket_status::PROTOCOL_ERROR, "RSV1 set without negotiation");
            break;
        }

        uint64_t payload_len = hdr.payload_len;
        if (payload_len == 126) {
            auto ext_opt = cur.try_read_be16();
            if (!ext_opt) {
                return;
            }
            payload_len = *ext_opt;
        } else if (payload_len == 127) {
            auto ext_opt = cur.try_read_be64();
            if (!ext_opt) {
                return;
            }
            payload_len = *ext_opt;
        }

        // 64 MB payload cap — aligned with read_frame()
        constexpr uint64_t max_payload_length = 64ULL * 1024ULL * 1024ULL;
        if (payload_len > max_payload_length) {
            send_close_frame(websocket_status::MESSAGE_TOO_BIG, "Payload exceeds limit");
            break;
        }

        byte_t mask_key[4]{};
        if (hdr.masked) {
            for (byte_t& i: mask_key) {
                auto b = cur.try_read_byte();
                if (!b) {
                    return;
                }
                i = *b;
            }
        }

        auto payload_opt = cur.try_read_bytes(static_cast<size_t>(payload_len));
        if (!payload_opt) {
            return;
        }

        string payload;
        if (payload_len > 0) {
            payload.assign(reinterpret_cast<const char*>(payload_opt->data()), static_cast<size_t>(payload_len));
        }

        if (hdr.masked && payload_len > 0) {
            for (size_t i = 0; i < static_cast<size_t>(payload_len); ++i) {
                payload[i] = static_cast<char>(static_cast<byte_t>(payload[i]) ^ mask_key[i % 4]);
            }
        }

        read_buffer_.erase(read_buffer_.begin(), read_buffer_.begin() + static_cast<ssize_t>(cur.consumed_bytes()));

        if (!dispatch(hdr, opcode, payload)) {
            break;
        }
    }
}

void websocket_session::on_writable(int /*fd*/, uint32_t /*events*/, error_code /*ec*/) { flush_event_writes(); }

void websocket_session::flush_event_writes() {
    lock<mutex> lk(write_mutex_);

    while (!ctrl_queue_.empty()) {
        auto& frame = ctrl_queue_.front();
        try {
            socket_->send_all(memory_view<const char>(reinterpret_cast<const char*>(frame.data()), frame.size()));
        } catch (...) {
            return;
        }
        ctrl_queue_.pop();
    }

    while (!write_queue_.empty()) {
        auto& frame = write_queue_.front();
        try {
            socket_->send_all(memory_view<const char>(reinterpret_cast<const char*>(frame.data()), frame.size()));
        } catch (...) {
            return;
        }
        write_queue_.pop();
    }

    if (ctrl_queue_.empty() && write_queue_.empty() && running_) {
        const int fd = static_cast<int>(socket_->native_handle());
        ctx_->mod_fd(fd, epoll_in);
    }
}

void websocket_session::on_heartbeat_timer() {
    if (!running_) {
        return;
    }

    const int64_t elapsed_ms = now_ms() - last_pong_ms_.load();
    constexpr milliseconds timeout_ms = heartbeat_timeout_sec.to_milli();

    if (ping_pending_.load() && elapsed_ms > timeout_ms.count()) {
        do_stop(websocket_status::ABNORMAL_CLOSURE, "Heartbeat timeout");
        return;
    }

    if (!ping_pending_.load()) {
        ping_pending_ = true;
        queue_frame(build_frame(websocket_opcode::PING, "", false), true);
    }

    auto self = shared_from_this();
    heartbeat_timer_id_ = ctx_->schedule_timer(static_cast<uint64_t>(heartbeat_interval_sec.count() * 1000),
                                               [self]() { self->on_heartbeat_timer(); });
}

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
