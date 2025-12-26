#include <MSTL/core/async/condition_variable.hpp>
#include <MSTL/core/container/map.hpp>
#include <MSTL/core/memory/shared_ptr.hpp>
#include <MSTL/core/system/console.hpp>
#include <MSTL/network/tcp_server.hpp>
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
        println("Worker registered:", id, "socket:", client->sockfd());
    }

    void remove_worker(const shared_ptr<handle_sock_t>& client) {
        lock_guard<mutex> lock(mtx_);
        for (auto it = workers_.begin(); it != workers_.end(); ++it) {
            if (it->socket == client) {
                println("Worker removed:", it->id);
                workers_.erase(it);
                --connected_workers_;
                cv_.notify_all();
                return;
            }
        }
    }

    void send_task_to_all(const string& task_data) {
        lock_guard<mutex> lock(mtx_);
        vector<size_t> disconnected;
        results_received_.store(0);

        string task_msg = "TASK:" + task_data;
        vector<char> packet = message_protocol::serialize(task_msg);

        for (auto& worker : workers_) {
            worker.has_result = false;
            worker.ready = false;
        }

        for (size_t i = 0; i < workers_.size(); ++i) {
            auto& worker = workers_[i];
            try {
                size_t total_sent = 0;
                while (total_sent < packet.size()) {
                    ssize_t sent = worker.socket->send(packet.data() + total_sent, packet.size() - total_sent);
                    if (sent <= 0) {
                        printcln(color::red(), "send failed");
                        throw_exception(exception("send failed"));
                    }
                    total_sent += sent;
                }
                println("Task sent to worker:", worker.id);
                worker.ready = true;
            } catch (const exception& e) {
                println("Failed to send task to worker:", worker.id, "error:", e.what());
                disconnected.push_back(i);
            }
        }

        for (auto it = disconnected.rbegin(); it != disconnected.rend(); ++it) {
            if (*it < workers_.size()) {
                println("Removing disconnected worker:", workers_[*it].id);
                workers_.erase(workers_.begin() + *it);
                --connected_workers_;
            }
        }
    }

    void record_result(const handle_sock_t& client, const string& result) {
        lock_guard<mutex> lock(mtx_);
        for (auto& worker : workers_) {
            if (*(worker.socket) == client) {
                worker.result = {worker.id, result, steady_clock::now()};
                worker.has_result = true;
                ++results_received_;
                println("Result received from:", worker.id);
                cv_.notify_one();
                return;
            }
        }
    }

    vector<task_result> wait_for_all_results(int timeout_seconds = 30) {
        vector<task_result> results;
        unique_lock<mutex> lock(mtx_);

        if (cv_.wait_for(lock, seconds(timeout_seconds), [this]() {
                return results_received_ >= connected_workers_;
            })) {
            for (const auto& worker : workers_) {
                if (worker.has_result) {
                    results.push_back(worker.result);
                }
            }
            return results;
        } else {
            println("Timeout waiting for results");
            return results;
        }
    }

    int get_connected_count() const {
        lock_guard<mutex> lock(mtx_);
        return connected_workers_;
    }

    static void print_results_analysis(const vector<task_result>& results) {
        if (results.empty()) {
            println("No results received");
            return;
        }

        println("\n=== Results Analysis ===");
        println("Total results received:", results.size());

        auto min_time = results[0].timestamp;
        auto max_time = results[0].timestamp;

        for (const auto& res : results) {
            println("Worker:", res.client_id, "-> Result:", res.result);

            if (res.timestamp < min_time) min_time = res.timestamp;
            if (res.timestamp > max_time) max_time = res.timestamp;
        }

        auto duration_ms = duration_cast<milliseconds>(max_time - min_time).count();
        println("\nProcessing time span:", duration_ms, "ms");

        map<string, int> result_counts;
        for (const auto& res : results) {
            result_counts[res.result]++;
        }

        println("\nResult distribution:");
        for (const auto& [result, count] : result_counts) {
            println(result, ":", count, "workers");
        }
    }
};

class task_server {
    using handle_sock_t = tcp_server::handle_sock_t;

private:
    tcp_server server_;
    worker_manager manager_;
    thread console_thread_;
    atomic_bool running_{false};

private:
    static bool receive_from_client(handle_sock_t& client, string& msg) {
        try {
            return message_protocol::receive_message(client, msg);
        } catch (const exception& e) {
            printcln(color::red(), "receive error:", e.what());
        }
        return false;
    }

    static bool send_to_client(handle_sock_t& client, const string& msg) {
        try {
            return message_protocol::send_message(client, msg);
        } catch (const exception& e) {
            printcln(color::red(), "send error:", e.what());
        }
        return false;
    }

    void handle_worker(shared_ptr<handle_sock_t> client) {
        string worker_id;
        if (!receive_from_client(*client, worker_id)) {
            printcln(color::red(), "failed to receive worker ID");
            return;
        }

        manager_.register_worker(client, worker_id);

        while (true) {
            string msg;
            if (!receive_from_client(*client, msg)) {
                break;
            }

            if (msg.find("RESULT:") == 0) {
                string result = msg.substr(7);
                manager_.record_result(*client, result);
                if (send_to_client(*client, "RESULT_ACK")) {
                    println("Sent RESULT_ACK to worker");
                } else {
                    println("Failed to send RESULT_ACK to worker");
                }
            } else if (msg == "HEARTBEAT") {
                if (!send_to_client(*client, "ALIVE")) {
                    break;
                }
                println("Received HEARTBEAT");
            }
        }

        manager_.remove_worker(client);
    }

    void console_loop() {
        while (running_) {
            print("\nServer> ");
            string cmd = console.readln();

            if (cmd == "quit") {
                running_.store(false);
                break;
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
                println("  send <data>     - Send task to all workers");
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
        stop();
    }

    bool start() {
        if (running_) return true;
        if (!server_.start()) {
            return false;
        }
        running_.store(true);

        // console_thread_ = thread([this]() {
        //     console_loop();
        // });

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
    }

    bool send(const string& msg) {
        if (manager_.get_connected_count() == 0) {
            println("No workers connected");
            return false;
        }

        println("Sending task to all workers...");
        manager_.send_task_to_all(msg);

        println("Waiting for results...");
        const auto results = manager_.wait_for_all_results();

        worker_manager::print_results_analysis(results);
        return true;
    }
};

int main() {
    task_server server(8080);
    if (!server.start()) {
        printcln(color::red(), "Failed to start server");
        return 1;
    }

    int count = 0;
    while (true) {
        this_thread::sleep_for(seconds(10));
        server.send("TEST" + to_string(++count));
    }
}