#include <NeForce/network/tcp/tcp_client.hpp>
#include <NeForce/core/system/console.hpp>
#include <NeForce/core/async/thread_pool.hpp>
#include <NeForce/core/container/queue.hpp>
#include <NeForce/core/system/process.hpp>
#include <NeForce/core/string/string_util.hpp>
#include <NeForce/core/numeric/random.hpp>
#include "example1_protocol.hpp"

neforce::string generate_worker_id() {
    thread_local neforce::random_mt tl_mt{};
    return "WORKER_" + neforce::to_string(tl_mt.next_int(1000, 9999));
}


class worker_client {
private:
    neforce::tcp_client client_;
    neforce::unique_ptr<neforce::tcp_socket> socket_;
    neforce::string worker_id_;
    neforce::atomic<bool> running_{false};

    neforce::thread receive_thread_;
    neforce::thread send_thread_;
    neforce::thread heartbeat_thread_;

    neforce::queue<neforce::vector<char>> send_queue_;
    neforce::mutex send_queue_mtx_;
    neforce::condition_variable send_queue_cv_;

    neforce::thread_pool task_pool_;

    neforce::tcp_socket& sock() { return *socket_; }

    void enqueue_packet(neforce::vector<char> packet) {
        {
            neforce::lock<neforce::mutex> lk(send_queue_mtx_);
            send_queue_.push(neforce::move(packet));
        }
        send_queue_cv_.notify_one();
    }

    void enqueue_message(const neforce::string& data, message_type type) {
        enqueue_packet(message::serialize(data, type));
    }

    void send_loop() {
        while (running_) {
            neforce::vector<char> packet;

            {
                neforce::unique_lock<neforce::mutex> lk(send_queue_mtx_);
                send_queue_cv_.wait(lk, [this] { return !send_queue_.empty() || !running_; });

                if (!running_ && send_queue_.empty()) {
                    break;
                }

                packet = neforce::move(send_queue_.front());
                send_queue_.pop();
            }

            try {
                if (!socket_ || !socket_->is_open()) {
                    neforce::printcln(neforce::color::red(), "[send_loop] Socket not available, drop packet");
                    running_.store(false);
                    break;
                }

                if (!message_transport::send_message(sock(), packet)) {
                    neforce::printcln(neforce::color::red(), "[send_loop] Send failed");
                    running_.store(false);
                    break;
                }
            } catch (const neforce::exception& e) {
                neforce::printcln(neforce::color::red(), "[send_loop] neforce::exception:", e.what());
                running_.store(false);
                break;
            }
        }

        send_queue_cv_.notify_all();
        neforce::println("[send_loop] Exited");
    }

    void heartbeat_loop() {
        while (running_) {
            neforce::this_thread::sleep_for(neforce::seconds(3));
            if (!running_) {
                break;
            }

            try {
                if (!socket_ || !socket_->is_open()) {
                    neforce::printcln(neforce::color::yellow(), "[heartbeat] Socket not available");
                    running_.store(false);
                    break;
                }
                enqueue_message("HEARTBEAT", message_type::HEARTBEAT_REQUEST);
                neforce::println("[heartbeat] Heartbeat enqueued");
            } catch (...) {
                neforce::printcln(neforce::color::red(), "[heartbeat] Exception, stopping");
                running_.store(false);
                break;
            }
        }
        neforce::println("[heartbeat] Exited");
    }

    void task_cmp(neforce::string result) {
        try {
            enqueue_message(result, message_type::TASK_COMPLETE_REQUEST);
            neforce::printcln(neforce::color::green(), "[task_cmp] Result enqueued:", result);
        } catch (...) {
            neforce::printcln(neforce::color::red(), "[task_cmp] Exception while enqueuing");
        }
    }

    bool process_data(const neforce::string& data) {
        auto vec = neforce::split(data, " ");
        if (vec.empty()) {
            neforce::printcln(neforce::color::red(), "[process_data] Empty data");
            return false;
        }

        neforce::string type = vec.front();
        type.uppercase();

        if (type == "PROCESS") {
            if (vec.size() < 2) {
                neforce::printcln(neforce::color::red(), "[process_data] PROCESS requires program name");
                return false;
            }

            task_pool_.submit_task([this, vec] {
                neforce::println("[process_data] Launching:", vec[1]);
                try {
                    auto info = neforce::process::create(vec[1], {vec.begin() + 1, vec.end()}, true);
                    int ret = neforce::process::wait_for(info);
                    task_cmp("PROCESS RET:" + neforce::to_string(ret) + " : " + neforce::to_string(info.stdout_output));
                } catch (const neforce::exception& e) {
                    task_cmp("PROCESS ERROR:" + neforce::string(e.what()));
                }
            });

        } else if (type == "STRING") {
            if (vec.size() < 2) {
                neforce::printcln(neforce::color::red(), "[process_data] STRING requires argument");
                return false;
            }
            neforce::string payload;
            for (size_t i = 1; i < vec.size(); ++i) {
                if (i > 1) {
                    payload += " ";
                }
                payload += vec[i];
            }
            task_cmp("PROCESSED_" + payload + "_DONE");

        } else {
            neforce::printcln(neforce::color::red(), "[process_data] Unknown task type:", type);
            return false;
        }

        return true;
    }

    void receive_loop() {
        while (running_) {
            try {
                if (!socket_ || !socket_->is_open()) {
                    neforce::printcln(neforce::color::red(), "[receive_loop] Socket not available");
                    running_.store(false);
                    break;
                }

                message msg;
                const bool ok = message_transport::receive_message(sock(), msg, neforce::milliseconds(1000));

                if (!ok) {
                    if (!socket_->is_open()) {
                        neforce::printcln(neforce::color::red(), "[receive_loop] Disconnected");
                        running_.store(false);
                        break;
                    }
                    continue;
                }

                neforce::println("[receive_loop] Received:", msg.header);

                switch (msg.header.type) {

                    case message_type::HEARTBEAT_RESPONSE:
                        neforce::println("[receive_loop] ALIVE received");
                        break;

                    case message_type::TASK_COMPLETE_RESPONSE:
                        neforce::println("[receive_loop] RESULT_ACK received");
                        break;

                    case message_type::REGISTER_WORKER_RESPONSE:
                        neforce::println("[receive_loop] REGISTER_ACK received");
                        break;

                    case message_type::SUBMIT_TASK_REQUEST: {
                        neforce::string task_data{msg.body.begin(), msg.body.end()};
                        neforce::printcln(neforce::color::cyan(), "[receive_loop] Task received:", task_data);
                        enqueue_message("TASK_ACCEPTED", message_type::SUBMIT_TASK_RESPONSE);
                        process_data(task_data);
                        break;
                    }
                    default: {
                        neforce::printcln(neforce::color::yellow(), "[receive_loop] Unknown type:", msg.header);
                        break;
                    }
                }

            } catch (const neforce::exception& e) {
                neforce::printcln(neforce::color::red(), "[receive_loop] neforce::exception:", e.what());
                running_.store(false);
                break;
            } catch (const std::exception& e) {
                neforce::printcln(neforce::color::red(), "[receive_loop] std::exception:", e.what());
                running_.store(false);
                break;
            } catch (...) {
                neforce::printcln(neforce::color::red(), "[receive_loop] Unknown exception");
                running_.store(false);
                break;
            }
        }

        send_queue_cv_.notify_all();
        neforce::println("[receive_loop] Exited");
    }

public:
    bool connect(const neforce::string& host, neforce::ports port) {
        if (!client_.connect(host, port)) {
            neforce::printcln(neforce::color::red(), "[connect] Failed to connect to", host);
            return false;
        }

        worker_id_ = generate_worker_id();
        neforce::printcln(neforce::color::green(), "[connect] Connected as ", worker_id_);

        try {
            auto reg_packet = message::serialize(worker_id_, message_type::REGISTER_WORKER_REQUEST);
            if (!message_transport::send_message(client_.socket(), reg_packet)) {
                neforce::printcln(neforce::color::red(), "[connect] Failed to send registration");
                client_.disconnect();
                return false;
            }

            message ack;
            if (!message_transport::receive_message(client_.socket(), ack, neforce::milliseconds(5000))) {
                neforce::printcln(neforce::color::red(), "[connect] No registration ACK");
                client_.disconnect();
                return false;
            }

            if (ack.header.type != message_type::REGISTER_WORKER_RESPONSE) {
                neforce::printcln(neforce::color::red(), "[connect] Bad registration response type");
                client_.disconnect();
                return false;
            }

            neforce::println("[connect] Registration confirmed");

            client_.socket().set_receive_timeout(neforce::milliseconds(0));
            client_.socket().set_send_timeout(neforce::milliseconds(5000));

        } catch (const neforce::exception& e) {
            neforce::printcln(neforce::color::red(), "[connect] Exception:", e.what());
            client_.disconnect();
            return false;
        }

        task_pool_.start();
        running_.store(true);

        send_thread_.start([this] { send_loop(); });
        receive_thread_.start([this] { receive_loop(); });
        heartbeat_thread_.start([this] { heartbeat_loop(); });

        neforce::printcln(neforce::color::green(), worker_id_, " ready, all threads started");
        return true;
    }

    void interactive_mode() const {
        neforce::println(worker_id_, " ready. Commands: status, quit, help");

        while (running_) {
            neforce::print("> ");
            neforce::string line = neforce::console.readln();

            if (line == "quit") {
                break;
            } else if (line == "status") {
                neforce::println("Worker ID :", worker_id_);
                neforce::println("Connected :", client_.is_connected() ? "Yes" : "No");
                neforce::println("Running   :", running_.load() ? "Yes" : "No");
            } else if (line == "help") {
                neforce::println("  status  - Show connection status");
                neforce::println("  quit    - Disconnect and exit");
                neforce::println("  help    - This help");
            } else if (!line.empty()) {
                neforce::println("Unknown command. Type 'help'.");
            }
        }
    }

    void close() {
        if (!running_.exchange(false)) {
            return;
        }

        try {
            if (client_.is_connected()) {
                client_.socket().close();
            }
        } catch (...) {
        }

        send_queue_cv_.notify_all();

        if (send_thread_.joinable()) {
            send_thread_.join();
        }
        if (receive_thread_.joinable()) {
            receive_thread_.join();
        }
        if (heartbeat_thread_.joinable()) {
            heartbeat_thread_.join();
        }

        task_pool_.stop();
        client_.disconnect();

        neforce::printcln(neforce::color::yellow(), worker_id_, " disconnected");
    }

    bool is_running() const noexcept { return running_; }
};

int main() {
    worker_client client;

    if (!client.connect("127.0.0.1", neforce::ports{8080})) {
        neforce::printcln(neforce::color::red(), "Failed to connect to server");
        return 1;
    }

    while (client.is_running()) {
        neforce::this_thread::sleep_for(neforce::seconds(1));
    }
    client.close();

    return 0;
}
