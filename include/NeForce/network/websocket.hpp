#ifndef NEFORCE_NETWORK_WEBSOCKET_HPP__
#define NEFORCE_NETWORK_WEBSOCKET_HPP__
#include "NeForce/core/async/thread.hpp"
#include "NeForce/core/async/condition_variable.hpp"
#include "NeForce/core/container/queue.hpp"
#include "NeForce/core/container/unordered_map.hpp"
#include "NeForce/core/functional/function.hpp"
#include "NeForce/core/memory/endian.hpp"
#include "NeForce/core/memory/shared_ptr.hpp"
#include "NeForce/network/http/http_server_message.hpp"
#include "NeForce/network/socket/ssl_socket.hpp"
NEFORCE_BEGIN_NAMESPACE__

enum class websocket_opcode : uint8_t {
    CONTINUATION = 0x0,
    TEXT = 0x1,
    BINARY = 0x2,
    CLOSE = 0x8,
    PING = 0x9,
    PONG = 0xA
};

#pragma pack(push, 1)
struct websocket_frame_header {
    byte_t fin : 1;
    byte_t rsv1 : 1;
    byte_t rsv2 : 1;
    byte_t rsv3 : 1;
    byte_t opcode : 4;
    byte_t masked : 1;
    byte_t payload_len : 7;
};
#pragma pack(pop)


template <typename SocketType>
class websocket_session;


template <typename SocketType>
class websocket_server {
public:
    using socket_type = SocketType;
    using session_handler = function<void(shared_ptr<websocket_session<socket_type>>)>;

private:
    unordered_map<string, session_handler> route_handlers_;
    vector<shared_ptr<websocket_session<socket_type>>> sessions_;
    mutex sessions_mutex_;

public:
    void route(const string& path, session_handler handler) {
        route_handlers_[path] = _NEFORCE move(handler);
    }

    bool handle_upgrade(const http_request& request, socket_type sock) {
        const auto it = route_handlers_.find(request.path);
        if (it == route_handlers_.end()) {
            return false;
        }

        auto session = _NEFORCE make_shared<websocket_session<socket_type>>(_NEFORCE move(sock));
        session->start();

        {
            lock<mutex> lock(sessions_mutex_);
            sessions_.push_back(session);
        }

        it->second(_NEFORCE move(session));
        return true;
    }

    void remove_session(shared_ptr<websocket_session<socket_type>> session) {
        lock<mutex> lock(sessions_mutex_);
        auto it = find(sessions_.begin(), sessions_.end(), session);
        if (it != sessions_.end()) {
            sessions_.erase(it);
        }
    }
};


class NEFORCE_API websocket_session_base {
protected:
    static byte_vector build_frame(websocket_opcode opcode, const string& payload, bool masked);
};


template <typename SocketType>
class websocket_session :
    public websocket_session_base,
    public enable_shared_from_this<websocket_session<SocketType>> {
public:
    using message_handler = function<void(const string&, websocket_opcode)>;
    using close_handler = function<void(WEBSOCKET_STATUS, const string&)>;

private:
    SocketType socket_;
    websocket_server<SocketType>* server_;
    atomic<bool> running_{false};
    thread read_thread_;
    thread write_thread_;
    mutex write_mutex_;
    condition_variable write_cv_;
    queue<vector<byte_t>> write_queue_;

    message_handler on_message_;
    close_handler on_close_;

    steady_clock::time_point last_pong_time_;
    atomic<bool> ping_pending_{false};

private:
    void start_heartbeat() {
        thread([this]() {
            while (running_) {
                this_thread::sleep_for(seconds(30));

                if (!running_) break;

                auto now = steady_clock::now();
                auto elapsed = time_cast<seconds>(now - last_pong_time_).count();

                if (elapsed > 35) {
                    if (ping_pending_) {
                        close(WEBSOCKET_STATUS::ABNORMAL_CLOSURE, "Heartbeat timeout");
                        break;
                    } else {
                        ping_pending_ = true;
                        send("", websocket_opcode::PING);
                    }
                }
            }
        }).detach();
    }

    void write_loop()  {
        byte_vector current_frame;

        while (running_) {
            {
                smart_lock<mutex> lock(write_mutex_);
                write_cv_.wait(lock, [this] {
                    return !running_ || !write_queue_.empty();
                });

                if (!running_) break;

                if (!write_queue_.empty()) {
                    current_frame = move(write_queue_.front());
                    write_queue_.pop();
                } else {
                    continue;
                }
            }

            try {
                socket_.send_all(memory_view<const char>(
                    reinterpret_cast<const char*>(current_frame.data()),
                    current_frame.size()
                ));
            } catch (const exception& e) {
                // handle
                break;
            }
        }

        if (running_) {
            stop();
        }
    }

    void read_loop() {
        while (running_) {
            if (!read_frame()) {
                break;
            }
        }

        if (running_) {
            stop();
        }
    }

    bool read_frame() {
        try {
            if (!socket_.is_open()) {
                return false;
            }

            websocket_frame_header header{};
            ssize_t n = socket_.receive(memory_view<char>(reinterpret_cast<char*>(&header), 1));
            if (n <= 0) return false;

            if (header.rsv1 || header.rsv2 || header.rsv3) {
                close(WEBSOCKET_STATUS::PROTOCOL_ERROR, "Reserved bits not zero");
                return false;
            }

            uint64_t payload_len = header.payload_len;
            if (payload_len == 126) {
                uint16_t net_len;
                n = socket_.receive(memory_view<char>(reinterpret_cast<char*>(&net_len), 2));
                if (n != 2) return false;
                payload_len = endian::network_to_host<uint16_t>(net_len);
            } else if (payload_len == 127) {
                uint64_t net_len;
                n = socket_.receive(memory_view<char>(reinterpret_cast<char*>(&net_len), 8));
                if (n != 8) return false;
                payload_len = endian::network_to_host<uint64_t>(net_len);
            }

            constexpr uint64_t MAX_MESSAGE_SIZE = 64 * 1024 * 1024; // 64MB
            if (payload_len > MAX_MESSAGE_SIZE) {
                close(WEBSOCKET_STATUS::MESSAGE_TOO_BIG, "Message too large");
                return false;
            }

            uint32_t masking_key = 0;
            if (header.masked) {
                n = socket_.receive(memory_view<char>(reinterpret_cast<char*>(&masking_key), 4));
                if (n != 4) return false;
            }

            string payload;
            if (payload_len > 0) {
                payload.resize(payload_len);
                n = socket_.receive(memory_view<char>(&payload[0], payload_len));
                if (static_cast<size_t>(n) != payload_len) return false;
            }

            if (header.masked) {
                for (size_t i = 0; i < payload_len; ++i) {
                    payload[i] ^= reinterpret_cast<char*>(&masking_key)[i % 4];
                }
            }

            websocket_opcode opcode = static_cast<websocket_opcode>(header.opcode);

            if ((opcode == websocket_opcode::CLOSE ||
                 opcode == websocket_opcode::PING ||
                 opcode == websocket_opcode::PONG) && payload_len > 125) {
                close(WEBSOCKET_STATUS::PROTOCOL_ERROR, "Control frame too large");
                return false;
            }

            if (opcode == websocket_opcode::CLOSE) {
                handle_close_frame(payload);
                return false;
            } else if (opcode == websocket_opcode::PING) {
                queue_frame(build_frame(websocket_opcode::PONG, payload, false));
                return true;
            } else if (opcode == websocket_opcode::PONG) {
                last_pong_time_ = steady_clock::now();
                return true;
            } else if (opcode == websocket_opcode::TEXT ||
                opcode == websocket_opcode::BINARY ||
                opcode == websocket_opcode::CONTINUATION) {
                if (on_message_) {
                    auto self = this->shared_from_this();
                    on_message_(payload, move(opcode));
                }
                return true;
            } else {
                close(WEBSOCKET_STATUS::PROTOCOL_ERROR, "Unknown opcode");
                return false;
            }
        } catch (const exception& e) {
            // handle
            return false;
        }
    }

    void queue_frame(byte_vector frame) {
        {
            lock<mutex> lock(write_mutex_);
            write_queue_.push(move(frame));
        }
        write_cv_.notify_one();
    }

    void handle_close_frame(const string& payload) {
        WEBSOCKET_STATUS status = WEBSOCKET_STATUS::NORMAL_CLOSURE;
        string reason;

        if (payload.size() >= 2) {
            uint16_t code = (static_cast<uint8_t>(payload[0]) << 8) | static_cast<uint8_t>(payload[1]);
            status = static_cast<WEBSOCKET_STATUS>(code);
            if (payload.size() > 2) {
                reason = payload.substr(2);
            }
        }

        if (running_) {
            string close_payload;
            close_payload.push_back((static_cast<uint16_t>(status) >> 8) & 0xFF);
            close_payload.push_back(static_cast<uint16_t>(status) & 0xFF);
            close_payload += reason;

            auto close_frame = build_frame(websocket_opcode::CLOSE, close_payload, false);
            queue_frame(move(close_frame));

            auto start = steady_clock::now();
            while (!write_queue_.empty() && steady_clock::now() - start < seconds(1)) {
                this_thread::sleep_for(milliseconds(10));
            }
        }

        if (on_close_) {
            on_close_(move(status), reason);
        }

        stop();
    }

public:
    explicit websocket_session(SocketType sock, websocket_server<SocketType>* server = nullptr)
    : socket_(_NEFORCE move(sock)), server_(server) {}

    ~websocket_session() {
        stop();
    }

    void start() {
        if (running_) return;
        running_ = true;
        last_pong_time_ = steady_clock::now();
        read_thread_ = thread(&websocket_session::read_loop, this);
        write_thread_ = thread(&websocket_session::write_loop, this);
        start_heartbeat();
    }

    void stop() {
        if (!running_) return;
        running_ = false;

        {
            lock<mutex> lock(write_mutex_);
            write_cv_.notify_all();
        }

        socket_.close();

        if (read_thread_.joinable()) read_thread_.join();
        if (write_thread_.joinable()) write_thread_.join();

        if (on_close_) {
            on_close_(WEBSOCKET_STATUS::NORMAL_CLOSURE, "Session stopped");
        }
    }

    void send(const string& data, websocket_opcode opcode = websocket_opcode::TEXT) {
        if (!running_) return;
        auto frame = build_frame(opcode, data, false);
        queue_frame(move(frame));
    }

    void send_binary(const string& data) {
        send(data, websocket_opcode::BINARY);
    }

    void close(WEBSOCKET_STATUS status = WEBSOCKET_STATUS::NORMAL_CLOSURE, const string& reason = "") {
        if (!running_) return;

        const uint16_t raw_status = static_cast<uint16_t>(status);
        string payload;
        payload.push_back((raw_status >> 8) & 0xFF);
        payload.push_back(raw_status & 0xFF);
        payload += reason;

        auto frame = build_frame(websocket_opcode::CLOSE, payload, false);
        queue_frame(move(frame));

        auto start = steady_clock::now();
        while (!write_queue_.empty() && steady_clock::now() - start < seconds(2)) {
            this_thread::sleep_for(milliseconds(10));
        }

        stop();
    }

    bool is_open() const {
        return running_ && socket_.is_open();
    }

    void set_message_handler(message_handler handler) {
        on_message_ = _NEFORCE move(handler);
    }

    void set_close_handler(close_handler handler) {
        on_close_ = [this, handler](WEBSOCKET_STATUS status, const string& reason) {
            handler(move(status), reason);

            if (server_) {
                server_->remove_session(this->shared_from_this());
            }
        };
    }

    SocketType& socket() {
        return socket_;
    }

    const SocketType& socket() const {
        return socket_;
    }
};

template class websocket_session<tcp_socket>;
template class websocket_session<ssl_socket>;

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_WEBSOCKET_HPP__
