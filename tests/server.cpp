#include <MSTL/core/async/condition_variable.hpp>
#include <MSTL/core/memory/shared_ptr.hpp>
#include <MSTL/core/system/console.hpp>
#include <MSTL/network/tcp_server.hpp>
#include <MSTL/logging/logger.hpp>
#include <MSTL/logging/file_sink.hpp>
#include <MSTL/core/numeric/random.hpp>
#include <MSTL/core/async/thread_pool.hpp>
#include <MSTL/core/system/process.hpp>
#include "message_protocol.hpp"

using namespace MSTL;

struct task_result {
    string client_id;
    string result;
    steady_clock::time_point timestamp;
};

class worker_manager {
    using handle_sock_t = tcp_server::handle_sock_t;

private:
    struct worker_info {
        shared_ptr<handle_sock_t> socket;
        string id;
        bool ready = false;
        bool has_result = false;
        task_result result;
    };

    vector<worker_info> workers_;
    mutable mutex mtx_;
    condition_variable cv_;
    atomic_int connected_workers_{0};
    atomic_int results_received_{0};

public:
    void register_worker(const shared_ptr<handle_sock_t>& client, const string& id) {
        lock_guard<mutex> lock(mtx_);
        workers_.push_back({client, id, true, false});
        ++connected_workers_;
        MSTL_LOGF_INFO("Worker registered: {} socket: {}", id, client->sockfd());
    }

    void remove_worker(const shared_ptr<handle_sock_t>& client) {
        lock_guard<mutex> lock(mtx_);
        for (auto it = workers_.begin(); it != workers_.end(); ++it) {
            if (it->socket == client) {
                MSTL_LOGF_INFO("Worker removed: {}", it->id);
                workers_.erase(it);
                --connected_workers_;
                cv_.notify_all();
                return;
            }
        }
    }

    void send_task_to(const string& task_data, const string& id) {
        lock_guard<mutex> lock(mtx_);
        vector<size_t> disconnected;
        results_received_.store(0);
        vector<char> packet = message::serialize(
            task_data, MESSAGE_TYPE::SUBMIT_TASK_REQUEST);

        worker_info* tar = nullptr;
        ssize_t index = -1;
        for (size_t i = 0; i < workers_.size(); ++i) {
            auto& worker = workers_[i];
            if (worker.id == id) {
                tar = &worker;
                index = i;
                break;
            }
        }

        if (tar) {
            tar->has_result = false;
            tar->ready = false;
        } else {
            MSTL_LOGF_ERROR("Target worker not exist: {}", id);
            return;
        }

        auto& worker = *tar;
        try {
            if (!message_transport::send_message(*worker.socket, packet)) {
                MSTL_LOG_ERROR("worker send failed");
            }
            MSTL_LOGF_INFO("Task sent to worker: {}", worker.id);
            worker.ready = true;
        } catch (const exception& e) {
            MSTL_LOGF_ERROR("Failed to send task to worker: {} error: {}", worker.id, e.what());
            MSTL_LOGF_INFO("Removing disconnected worker: {}", workers_[index].id);
            workers_.erase(workers_.begin() + index);
        }
    }

    void send_task_to_rnd(const string& task_data) {
        send_task_to(task_data, workers_[random_mt::next_int(0, workers_.size())].id);
    }

    void record_result(const handle_sock_t& client, const string& result) {
        lock_guard<mutex> lock(mtx_);
        for (auto& worker : workers_) {
            if (*worker.socket == client) {
                worker.result = {worker.id, result, steady_clock::now()};
                worker.has_result = true;
                ++results_received_;
                MSTL_LOGF_INFO("Result received from: {}", worker.id);
                cv_.notify_one();
                return;
            }
        }
    }

    int get_connected_count() const {
        lock_guard<mutex> lock(mtx_);
        return connected_workers_;
    }
};

class task_server {
    using handle_sock_t = tcp_server::handle_sock_t;

private:
    tcp_server server_;
    worker_manager manager_;
    thread_pool pool_;
    thread console_thread_;
    atomic_bool running_{false};

private:
    static bool receive_from_client(const handle_sock_t& client, message& msg) {
        try {
            return message_transport::receive_message(client, msg);
        } catch (const exception& e) {
            MSTL_LOGF_ERROR("receive error:", e.what());
        }
        return false;
    }

    static bool send_to_client(const handle_sock_t& client, const string& msg, MESSAGE_TYPE type) {
        try {
            return message_transport::send_message(client, msg, type);
        } catch (const exception& e) {
            MSTL_LOGF_ERROR("send error:", e.what());
        }
        return false;
    }

    void handle_worker(shared_ptr<handle_sock_t> client) {
        message worker_msg;
        if (!receive_from_client(*client, worker_msg)) {
            MSTL_LOG_ERROR("failed to receive worker ID");
            return;
        }

        manager_.register_worker(client, string{worker_msg.body.data(), worker_msg.body.size()});

        while (true) {
            message msg;
            if (!receive_from_client(*client, msg)) {
                break;
            }

            if (msg.header.type == MESSAGE_TYPE::SUBMIT_TASK_RESPONSE) {
                MSTL_LOG_INFO("Task submitted");
            } else if (msg.header.type == MESSAGE_TYPE::TASK_COMPLETE_REQUEST) {
                manager_.record_result(*client, string{msg.body.data(), msg.body.size()});
                if (send_to_client(*client, "RESULT_ACK", MESSAGE_TYPE::TASK_COMPLETE_RESPONSE)) {
                    MSTL_LOG_INFO("Accept result and sent RESULT_ACK to worker");
                } else {
                    MSTL_LOG_ERROR("Failed to send RESULT_ACK to worker");
                }
            } else if (msg.header.type == MESSAGE_TYPE::HEARTBEAT_REQUEST) {
                if (!send_to_client(*client, "ALIVE", MESSAGE_TYPE::HEARTBEAT_RESPONSE)) {
                    MSTL_LOG_ERROR("Failed to send ALIVE to worker");
                    break;
                }
                MSTL_LOG_INFO("Received HEARTBEAT");
            }
        }

        manager_.remove_worker(client);
    }

    void console_loop() {
        while (running_) {
            print("\nServer> ");
            string cmd = console.readln();

            if (cmd == "quit") {
                std::exit(0);
            } else if (cmd == "status") {
                println("Connected workers:", manager_.get_connected_count());
            } else if (cmd.find("send ") == 0) {
                string task_data = cmd.substr(5);
                if (!send(task_data)) {
                    break;
                }
            } else if (cmd == "help") {
                println("Available commands:");
                println("  status          - Show connected workers");
                println("  send <data>     - Send task to worker");
                println("  quit            - Stop server");
                println("  help            - Show this help");
            } else if (!cmd.empty()) {
                println("Unknown command. Type 'help' for available commands.");
            }
        }
    }

public:
    explicit task_server(const uint16_t port) : server_(port) {
        server_.set_client_handler([this](handle_sock_t client) {
            handle_worker(make_shared<handle_sock_t>(move(client)));
        });
    }

    ~task_server() {
        pool_.stop();
        stop();
    }

    bool running() const noexcept {
        return running_;
    }

    bool start() {
        if (running_) return true;
        if (!(server_.start() && pool_.start(5))) {
            return false;
        }
        running_.store(true);

        console_thread_ = thread([this]() {
            console_loop();
        });

        printcln(color::green(),
            "Task server started on port:" + to_string(server_.port()) +
            "\nType 'help' for available commands");
        console.flush();
        return true;
    }

    void stop() {
        if (!running_) return;
        running_.store(false);

        if (console_thread_.joinable()) {
            console_thread_.join();
        }

        server_.stop();
        printcln(color::yellow(), "Task server stopped");
        MSTL_LOG_WARN("Task server stopped");
    }

    bool send(const string& msg) {
        if (manager_.get_connected_count() == 0) {
            println("No workers connected");
            MSTL_LOG_WARN("No workers connected");
            return false;
        }

        pool_.submit_task([this, msg] {
            manager_.send_task_to_rnd(msg);
        });

        return true;
    }
};

int main() {
    auto& logger = logger::instance();
    logger.add_context("app", "Server");
    const auto sink = make_shared<file_sink>(path("logging.s"));
    sink->set_formatter(make_unique<log_formatter>("[{time}][{level}][{context.app}] {message}"));
    logger.add_sink(sink);
    logger.enable_async(true);

    task_server server(8080);
    if (!server.start()) {
        printcln(color::red(), "Failed to start server");
        return 1;
    }

    while (server.running()) {
        this_thread::sleep_for(seconds(1));
    }
}