#include <NeForce/core/memory/shared_ptr.hpp>
#include <NeForce/core/system/console.hpp>
#include <NeForce/network/tcp/tcp_server.hpp>
#include <NeForce/logging/logger.hpp>
#include <NeForce/logging/file_sink.hpp>
#include <NeForce/core/numeric/random.hpp>
#include <NeForce/core/async/thread_pool.hpp>
#include <NeForce/core/exception/terminate.hpp>
#include <NeForce/core/system/process.hpp>
#include "example1_protocol.hpp"

struct task_result {
    neforce::string client_id;
    neforce::string result;
    neforce::steady_clock::time_point timestamp;
};

class worker_manager {
private:
    struct worker_info {
        neforce::shared_ptr<neforce::tcp_socket> socket;
        neforce::string id;
        bool has_result = false;
        task_result result;
    };

    neforce::vector<worker_info> workers_;
    mutable neforce::mutex mtx_;
    neforce::atomic<int> connected_count_{0};

public:
    void register_worker(const neforce::shared_ptr<neforce::tcp_socket>& sock, const neforce::string& id) {
        neforce::lock<neforce::mutex> lk(mtx_);
        workers_.push_back({sock, id, false, {}});
        ++connected_count_;
        NEFORCE_LOGF_INFO("Worker registered: {} fd={}", id, sock->native_handle());
    }

    void remove_worker(const neforce::shared_ptr<neforce::tcp_socket>& sock) {
        neforce::lock<neforce::mutex> lk(mtx_);
        for (auto it = workers_.begin(); it != workers_.end(); ++it) {
            if (it->socket == sock) {
                NEFORCE_LOGF_INFO("Worker removed: {}", it->id);
                workers_.erase(it);
                --connected_count_;
                return;
            }
        }
    }

    bool send_task_to(const neforce::string& task_data, const neforce::string& id) {
        neforce::shared_ptr<neforce::tcp_socket> target_sock;
        {
            neforce::lock<neforce::mutex> lk(mtx_);
            for (auto& w: workers_) {
                if (w.id == id) {
                    w.has_result = false;
                    target_sock = w.socket;
                    break;
                }
            }
        }

        if (!target_sock) {
            NEFORCE_LOGF_ERROR("Target worker not found: {}", id);
            return false;
        }

        const auto packet = message::serialize(task_data, message_type::SUBMIT_TASK_REQUEST);
        try {
            if (!message_transport::send_message(*target_sock, packet)) {
                NEFORCE_LOGF_ERROR("Send failed to worker: {}", id);
                return false;
            }
            NEFORCE_LOGF_INFO("Task sent to worker: {}", id);
            return true;
        } catch (const neforce::exception& e) {
            NEFORCE_LOGF_ERROR("Exception sending to worker {}: {}", id, e.what());
            return false;
        }
    }

    bool send_task_to_rnd(const neforce::string& task_data) {
        neforce::string chosen_id;
        {
            neforce::lock<neforce::mutex> lk(mtx_);
            if (workers_.empty()) {
                NEFORCE_LOG_WARN("No workers available");
                return false;
            }
            thread_local neforce::random_mt tl_mt{};
            chosen_id = workers_[tl_mt.next_int(workers_.size())].id;
        }

        return send_task_to(task_data, chosen_id);
    }

    void record_result(const neforce::tcp_socket& sock, const neforce::string& result) {
        neforce::lock<neforce::mutex> lk(mtx_);
        for (auto& w: workers_) {
            if (w.socket->native_handle() == sock.native_handle()) {
                w.result = {w.id, result, neforce::steady_clock::now()};
                w.has_result = true;
                NEFORCE_LOGF_INFO("Result received from: {}", w.id);
                return;
            }
        }
    }

    int get_connected_count() const noexcept { return connected_count_; }
};

class task_server {
private:
    neforce::tcp_server server_;
    worker_manager manager_;
    neforce::thread_pool pool_;
    neforce::thread console_thread_;
    neforce::atomic<bool> running_{false};

private:
    static bool recv_msg(neforce::tcp_socket& sock, message& msg) {
        try {
            return message_transport::receive_message(sock, msg);
        } catch (const neforce::exception& e) {
            NEFORCE_LOGF_ERROR("recv error: {}", e.what());
            return false;
        }
    }

    static bool send_msg(neforce::tcp_socket& sock, const neforce::string& data, message_type type) {
        try {
            return message_transport::send_message(sock, data, type);
        } catch (const neforce::exception& e) {
            NEFORCE_LOGF_ERROR("send error: {}", e.what());
            return false;
        }
    }

    void handle_worker(neforce::shared_ptr<neforce::tcp_socket> client) {
        if (!client->set_receive_timeout(neforce::milliseconds(30000))) {
            NEFORCE_LOG_ERROR("Failed to set receive timeout");
            return;
        }

        message reg_msg;
        if (!recv_msg(*client, reg_msg)) {
            NEFORCE_LOG_ERROR("Failed to receive registration message");
            return;
        }

        if (reg_msg.header.type != message_type::REGISTER_WORKER_REQUEST) {
            NEFORCE_LOGF_ERROR("Expected REGISTER_WORKER_REQUEST, got type=0x{:02X}",
                               static_cast<uint8_t>(reg_msg.header.type));
            return;
        }
        if (reg_msg.body.empty()) {
            NEFORCE_LOG_ERROR("Empty worker ID");
            return;
        }

        const neforce::string worker_id{reg_msg.body.data(), reg_msg.body.size()};

        if (!send_msg(*client, "OK", message_type::REGISTER_WORKER_RESPONSE)) {
            NEFORCE_LOG_ERROR("Failed to send register response");
            return;
        }

        manager_.register_worker(client, worker_id);

        while (running_) {
            message msg;
            if (!recv_msg(*client, msg)) {
                NEFORCE_LOGF_INFO("Worker {} disconnected or timed out", worker_id);
                break;
            }

            switch (msg.header.type) {
                case message_type::SUBMIT_TASK_RESPONSE: {
                    NEFORCE_LOGF_INFO("Worker {} acknowledged task", worker_id);
                    break;
                }
                case message_type::TASK_COMPLETE_REQUEST: {
                    neforce::string result{msg.body.data(), msg.body.size()};
                    manager_.record_result(*client, result);
                    if (!send_msg(*client, "RESULT_ACK", message_type::TASK_COMPLETE_RESPONSE)) {
                        NEFORCE_LOG_ERROR("Failed to send RESULT_ACK");
                    }
                    break;
                }
                case message_type::HEARTBEAT_REQUEST: {
                    if (!send_msg(*client, "ALIVE", message_type::HEARTBEAT_RESPONSE)) {
                        NEFORCE_LOG_ERROR("Failed to send heartbeat response");
                        goto cleanup;
                    }
                    NEFORCE_LOGF_INFO("Heartbeat from {}", worker_id);
                    break;
                }
                default: {
                    NEFORCE_LOGF_WARN("Unknown message type 0x{:02X} from {}", static_cast<uint8_t>(msg.header.type),
                                      worker_id);
                    break;
                }
            }
        }

    cleanup:
        manager_.remove_worker(client);
    }

    void console_loop() {
        while (running_) {
            neforce::print("\nServer> ");
            neforce::string cmd = neforce::console.readln();

            if (cmd == "quit") {
                neforce::exit(0);
            } else if (cmd == "status") {
                neforce::println("Connected workers:", manager_.get_connected_count());
            } else if (cmd.find("send ") == 0) {
                neforce::string task_data = cmd.tail(5);
                if (!send(task_data)) {
                    break;
                }
            } else if (cmd == "help") {
                neforce::println("Available commands:");
                neforce::println("  status          - Show connected workers");
                neforce::println("  send <data>     - Send task to worker");
                neforce::println("  quit            - Stop server");
                neforce::println("  help            - Show this help");
            } else if (!cmd.empty()) {
                neforce::println("Unknown command. Type 'help' for available commands.");
            }
        }
    }

public:
    explicit task_server(const neforce::ports port) :
    server_(port) {
        server_.set_client_handler([this](neforce::tcp_socket client) {
            handle_worker(neforce::make_shared<neforce::tcp_socket>(neforce::move(client)));
        });
    }

    ~task_server() {
        stop();
        pool_.stop();
    }

    bool running() const noexcept { return running_; }

    bool start() {
        if (running_) {
            return true;
        }

        pool_.start(5);
        if (!server_.start()) {
            pool_.stop();
            return false;
        }

        running_.store(true);
        console_thread_.start([this] { console_loop(); });

        neforce::printcln(neforce::color::green(), "Task server started on port " + neforce::to_string(server_.port()) +
                                                           "  |  type 'help' for commands");
        neforce::console.flush();
        return true;
    }

    void stop() {
        if (!running_.exchange(false)) {
            return;
        }

        server_.stop();

        if (console_thread_.joinable()) {
            console_thread_.join();
        }

        neforce::printcln(neforce::color::yellow(), "Task server stopped");
        NEFORCE_LOG_WARN("Task server stopped");
    }

    bool send(const neforce::string& task_data) {
        if (manager_.get_connected_count() == 0) {
            neforce::println("No workers connected");
            return false;
        }
        pool_.submit_task([this, task_data] { manager_.send_task_to_rnd(task_data); });
        return true;
    }
};

int main() {
    auto& logger = neforce::logger::instance();
    logger.add_context("app", "Server");
    logger.set_level(neforce::log_level::TRACE);
    const auto sink = neforce::make_shared<neforce::file_sink>(neforce::path("logging.s"));
    sink->set_formatter(neforce::make_unique<neforce::log_formatter>("[{time}][{level}][{context.app}] {message}"));
    logger.add_sink(sink);
    logger.enable_async(true);

    task_server server(neforce::ports{8080});
    if (!server.start()) {
        neforce::printcln(neforce::color::red(), "Failed to start server");
        return 1;
    }

    while (server.running()) {
        neforce::this_thread::sleep_for(neforce::seconds(1));
    }
}
