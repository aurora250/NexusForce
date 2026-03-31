#ifndef NEFORCE_NETWORK_WEBSOCKET_HPP__
#define NEFORCE_NETWORK_WEBSOCKET_HPP__
#include "NeForce/core/async/atomic.hpp"
#include "NeForce/core/async/condition_variable.hpp"
#include "NeForce/core/async/thread.hpp"
#include "NeForce/core/container/queue.hpp"
#include "NeForce/core/container/unordered_map.hpp"
#include "NeForce/core/functional/function.hpp"
#include "NeForce/core/memory/endian.hpp"
#include "NeForce/core/memory/shared_ptr.hpp"
#include "NeForce/network/http/http_server_message.hpp"
#include "NeForce/network/socket/ssl_socket.hpp"
NEFORCE_BEGIN_NAMESPACE__

enum class websocket_status : uint16_t {
    NORMAL_CLOSURE = 1000,
    GOING_AWAY = 1001,
    PROTOCOL_ERROR = 1002,
    UNSUPPORTED_DATA = 1003,
    RESERVED = 1004,
    NO_STATUS_RCVD = 1005,
    ABNORMAL_CLOSURE = 1006,
    INVALID_FRAME_PAYLOAD_DATA = 1007,
    POLICY_VIOLATION = 1008,
    MESSAGE_TOO_BIG = 1009,
    MANDATORY_EXT = 1010,
    INTERNAL_ERROR = 1011,
    SERVICE_RESTART = 1012,
    TRY_AGAIN_LATER = 1013,
    BAD_GATEWAY = 1014,
    TLS_HANDSHAKE = 1015
};

enum class websocket_opcode : uint8_t {
    CONTINUATION = 0x0,
    TEXT         = 0x1,
    BINARY       = 0x2,
    CLOSE        = 0x8,
    PING         = 0x9,
    PONG         = 0xA
};

#pragma pack(push, 1)
struct websocket_frame_header {
    byte_t fin    : 1;
    byte_t rsv1   : 1;
    byte_t rsv2   : 1;
    byte_t rsv3   : 1;
    byte_t opcode : 4;
    byte_t masked      : 1;
    byte_t payload_len : 7;
};
#pragma pack(pop)


template <typename SocketType>
class websocket_session;


class NEFORCE_API websocket_session_base {
protected:
    template <typename SocketType>
    static bool receive_exact(SocketType& socket, void* buf, size_t n) {
        auto* ptr = static_cast<char*>(buf);
        size_t remaining = n;
        while (remaining > 0) {
            const ssize_t got = socket.receive(memory_view<char>(ptr, remaining));
            if (got <= 0) return false;
            ptr += got;
            remaining -= static_cast<size_t>(got);
        }
        return true;
    }

    static byte_vector build_frame(websocket_opcode opcode, const string& payload, bool masked);

    static string make_close_payload(websocket_status status, const string& reason);
};


template <typename SocketType>
class websocket_server {
public:
    using socket_type       = SocketType;
    using session_type      = websocket_session<SocketType>;
    using session_ptr       = shared_ptr<session_type>;
    using session_handler   = function<void(session_ptr)>;

private:
    unordered_map<string, session_handler> route_handlers_;
    vector<session_ptr> sessions_;
    mutable mutex sessions_mutex_;

public:
    void route(const string& path, session_handler handler) {
        route_handlers_[path] = _NEFORCE move(handler);
    }

    bool handle_upgrade(const http_request& request, socket_type sock) {
        const auto it = route_handlers_.find(request.path);
        if (it == route_handlers_.end()) return false;

        auto session = make_shared<session_type>(_NEFORCE move(sock), this);
        {
            lock<mutex> lk(sessions_mutex_);
            sessions_.push_back(session);
        }
        session->start();
        it->second(_NEFORCE move(session));
        return true;
    }

    void remove_session(const session_ptr& session) {
        lock<mutex> lk(sessions_mutex_);
        auto it = find(sessions_.begin(), sessions_.end(), session);
        if (it != sessions_.end()) sessions_.erase(it);
    }

    void broadcast(const string& data, websocket_opcode opcode = websocket_opcode::TEXT) {
        lock<mutex> lk(sessions_mutex_);
        for (auto& s : sessions_) {
            if (s->is_open()) s->send(data, opcode);
        }
    }

    size_t session_count() const noexcept {
        lock<mutex> lk(sessions_mutex_);
        return sessions_.size();
    }
};


template <typename SocketType>
class websocket_session :
    public websocket_session_base,
    public enable_shared_from_this<websocket_session<SocketType>> {

public:
    using message_handler = function<void(const string&, websocket_opcode)>;
    using close_handler   = function<void(websocket_status, const string&)>;
    using error_handler   = function<void(const exception&)>;

    static constexpr size_t MAX_WRITE_QUEUE_SIZE = 1024;
    static constexpr uint64_t MAX_PAYLOAD_SIZE   = 64ULL * 1024 * 1024;
    static constexpr int HEARTBEAT_INTERVAL_SEC  = 30;
    static constexpr int HEARTBEAT_TIMEOUT_SEC   = 10;

private:
    SocketType socket_;
    websocket_server<SocketType>* server_;

    atomic<bool> running_{false};
    atomic_flag closed_once_;

    thread read_thread_;
    thread write_thread_;
    thread heartbeat_thread_;

    mutex write_mutex_;
    condition_variable write_cv_;
    queue<byte_vector> write_queue_;
    queue<byte_vector> ctrl_queue_;

    string fragment_buffer_;
    websocket_opcode fragment_opcode_ = websocket_opcode::TEXT;
    bool in_fragment_ = false;

    atomic<bool> ping_pending_{false};
    atomic<int64_t> last_pong_ms_{0};

    message_handler on_message_;
    close_handler on_close_;
    error_handler on_error_;

    static int64_t now_ms() noexcept {
        return time_cast<milliseconds>(steady_clock::now().since_epoch()).count();
    }

    void notify_error(const exception& e) noexcept {
        if (on_error_) {
            try { on_error_(e); } catch (...) {}
        }
    }

    bool queue_frame(byte_vector frame, bool is_control = false) {
        {
            lock<mutex> lk(write_mutex_);
            if (is_control) {
                ctrl_queue_.push(_NEFORCE move(frame));
            } else {
                if (write_queue_.size() >= MAX_WRITE_QUEUE_SIZE) {
                    return false;
                }
                write_queue_.push(_NEFORCE move(frame));
            }
        }
        write_cv_.notify_one();
        return true;
    }

    void write_loop() noexcept {
        while (running_) {
            byte_vector frame;
            {
                unique_lock<mutex> lk(write_mutex_);
                write_cv_.wait(lk, [this] {
                    return !running_ || !ctrl_queue_.empty() || !write_queue_.empty();
                });
                if (!running_ && ctrl_queue_.empty() && write_queue_.empty()) break;

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

            try {
                socket_.send_all(memory_view<const char>(
                    reinterpret_cast<const char*>(frame.data()),
                    frame.size()));
            } catch (const exception& e) {
                notify_error(e);
                break;
            }
        }

        try {
            lock<mutex> lk(write_mutex_);
            while (!ctrl_queue_.empty()) {
                auto& f = ctrl_queue_.front();
                socket_.send_all(memory_view<const char>(
                    reinterpret_cast<const char*>(f.data()), f.size()));
                ctrl_queue_.pop();
            }
        } catch (...) {}
    }

    void read_loop() noexcept {
        while (running_) {
            if (!read_frame()) break;
        }
        do_stop(websocket_status::ABNORMAL_CLOSURE, "Connection lost");
    }

    bool read_frame() noexcept {
        try {
            websocket_frame_header hdr{};
            if (!receive_exact(socket_, &hdr, 2)) return false;

            if (hdr.rsv1 || hdr.rsv2 || hdr.rsv3) {
                send_close_frame(websocket_status::PROTOCOL_ERROR, "Reserved bits set");
                return false;
            }

            const auto opcode = static_cast<websocket_opcode>(hdr.opcode);
            const bool is_ctrl = (hdr.opcode >= 0x8);

            if (is_ctrl && !hdr.fin) {
                send_close_frame(websocket_status::PROTOCOL_ERROR, "Fragmented control frame");
                return false;
            }

            uint64_t payload_len = hdr.payload_len;
            if (payload_len == 126) {
                uint16_t ext{};
                if (!receive_exact(socket_, &ext, 2)) return false;
                payload_len = endian::network_to_host<uint16_t>(ext);
            } else if (payload_len == 127) {
                uint64_t ext{};
                if (!receive_exact(socket_, &ext, 8)) return false;
                payload_len = endian::network_to_host<uint64_t>(ext);
            }

            if (is_ctrl && payload_len > 125) {
                send_close_frame(websocket_status::PROTOCOL_ERROR, "Control frame payload too large");
                return false;
            }

            if (payload_len > MAX_PAYLOAD_SIZE) {
                send_close_frame(websocket_status::MESSAGE_TOO_BIG, "Payload exceeds limit");
                return false;
            }

            uint32_t masking_key = 0;
            if (hdr.masked) {
                if (!receive_exact(socket_, &masking_key, 4)) return false;
            }

            string payload;
            if (payload_len > 0) {
                payload.resize(static_cast<size_t>(payload_len));
                if (!receive_exact(socket_, payload.data(), static_cast<size_t>(payload_len))) {
                    return false;
                }
            }

            if (hdr.masked && payload_len > 0) {
                const auto* key_bytes = reinterpret_cast<const char*>(&masking_key);
                for (size_t i = 0; i < static_cast<size_t>(payload_len); ++i) {
                    payload[i] ^= key_bytes[i % 4];
                }
            }

            return dispatch(hdr, opcode, payload);

        } catch (const exception& e) {
            notify_error(e);
            return false;
        }
    }

    bool dispatch(const websocket_frame_header& hdr, websocket_opcode opcode, string payload) {
        switch (opcode) {
            case websocket_opcode::TEXT:
            case websocket_opcode::BINARY: {
                if (in_fragment_) {
                    send_close_frame(websocket_status::PROTOCOL_ERROR,
                                     "New data frame before fragment complete");
                    return false;
                }
                if (!hdr.fin) {
                    fragment_opcode_  = opcode;
                    fragment_buffer_  = payload;
                    in_fragment_      = true;
                } else {
                    deliver_message(payload, opcode);
                }
                return true;
            }
            case websocket_opcode::CONTINUATION: {
                if (!in_fragment_) {
                    send_close_frame(websocket_status::PROTOCOL_ERROR,
                                     "Unexpected continuation frame");
                    return false;
                }
                fragment_buffer_ += payload;
                if (hdr.fin) {
                    deliver_message(fragment_buffer_, fragment_opcode_);
                    fragment_buffer_.clear();
                    in_fragment_ = false;
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
                handle_close_frame(payload);
                return false;
            }
            default: {
                send_close_frame(websocket_status::PROTOCOL_ERROR, "Unknown opcode");
                return false;
            }
        }
    }

    void deliver_message(const string& data, websocket_opcode opcode) {
        if (on_message_) {
            try { on_message_(data, move(opcode)); }
            catch (const exception& e) { notify_error(e); }
        }
    }

    void send_close_frame(websocket_status status, const string& reason) {
        auto frame = build_frame(websocket_opcode::CLOSE, make_close_payload(status, reason), false);
        queue_frame(_NEFORCE move(frame), true);
    }

    void handle_close_frame(string payload) {
        auto status = websocket_status::NORMAL_CLOSURE;
        string reason;

        if (payload.size() >= 2) {
            const uint16_t code =
                (static_cast<uint8_t>(payload[0]) << 8) |
                 static_cast<uint8_t>(payload[1]);
            status = static_cast<websocket_status>(code);
            if (payload.size() > 2) reason = payload.substr(2);
        }

        send_close_frame(status, reason);

        do_stop(status, reason);
    }

    void heartbeat_loop() noexcept {
        last_pong_ms_ = now_ms();

        while (running_) {
            for (int i = 0; i < HEARTBEAT_INTERVAL_SEC * 10 && running_; ++i) {
                this_thread::sleep_for(milliseconds(100));
            }
            if (!running_) break;

            const int64_t elapsed_ms = now_ms() - last_pong_ms_.load();
            constexpr int64_t timeout_ms = static_cast<int64_t>(HEARTBEAT_TIMEOUT_SEC) * 1000;

            if (ping_pending_.load() && elapsed_ms > timeout_ms) {
                do_stop(websocket_status::ABNORMAL_CLOSURE, "Heartbeat timeout");
                return;
            }

            if (!ping_pending_.load()) {
                ping_pending_ = true;
                queue_frame(build_frame(websocket_opcode::PING, "", false), true);
            }
        }
    }

    void do_stop(websocket_status status, const string& reason) noexcept {
        if (closed_once_.test_and_set()) return;

        running_ = false;
        write_cv_.notify_all();
        socket_.close();

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

        if (on_close_) {
            try {
                on_close_(move(status), reason);
            } catch (...) {}
        }

        if (server_) {
            server_->remove_session(this->shared_from_this());
        }
    }

public:
    explicit websocket_session(SocketType sock, websocket_server<SocketType>* server = nullptr)
    : socket_(_NEFORCE move(sock)), server_(server) {}

    ~websocket_session() {
        do_stop(websocket_status::NORMAL_CLOSURE, "Session destroyed");
    }

    websocket_session(const websocket_session&) = delete;
    websocket_session& operator=(const websocket_session&) = delete;

    void start() {
        if (running_.exchange(true)) return;
        last_pong_ms_ = now_ms();
        read_thread_ = thread(&websocket_session::read_loop, this);
        write_thread_ = thread(&websocket_session::write_loop, this);
        heartbeat_thread_ = thread(&websocket_session::heartbeat_loop, this);
    }

    void close(websocket_status status = websocket_status::NORMAL_CLOSURE, const string& reason = "") {
        if (!running_) return;
        send_close_frame(status, reason);

        const auto deadline = steady_clock::now() + seconds(2);
        while (steady_clock::now() < deadline) {
            {
                lock<mutex> lk(write_mutex_);
                if (ctrl_queue_.empty() && write_queue_.empty()) break;
            }
            this_thread::sleep_for(milliseconds(10));
        }
        do_stop(status, reason);
    }

    void stop() {
        do_stop(websocket_status::NORMAL_CLOSURE, "Stopped");
    }

    bool send(const string& data, websocket_opcode opcode = websocket_opcode::TEXT) {
        if (!running_) return false;
        return queue_frame(build_frame(opcode, data, false));
    }

    bool send_binary(const string& data) {
        return send(data, websocket_opcode::BINARY);
    }

    bool is_open() const noexcept {
        return running_ && socket_.is_open();
    }

    void set_message_handler(message_handler handler) {
        on_message_ = _NEFORCE move(handler);
    }

    void set_close_handler(close_handler handler) {
        on_close_ = _NEFORCE move(handler);
    }

    void set_error_handler(error_handler handler) {
        on_error_ = _NEFORCE move(handler);
    }

    SocketType& socket() noexcept { return socket_; }
    const SocketType& socket() const noexcept { return socket_; }
};

template class websocket_session<tcp_socket>;
template class websocket_session<ssl_socket>;

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_WEBSOCKET_HPP__
