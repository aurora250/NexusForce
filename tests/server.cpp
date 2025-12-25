#include <MSTL/network/tcp_server.hpp>
#include <MSTL/core/system/console.hpp>
#include <MSTL/core/async/condition_variable.hpp>
#include <MSTL/core/container/map.hpp>

using namespace MSTL;

struct task_result {
    string client_id;
    string result;
    steady_clock::time_point timestamp;
};

class worker_manager {
private:
    struct worker_info {
        tcp_socket::socket_t socket;
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
    void register_worker(tcp_socket::socket_t client, const string& id) {
        lock_guard<mutex> lock(mtx_);
        workers_.push_back({client, id, true, false});
        ++connected_workers_;
        println("Worker registered:", id, "socket:", client);
    }

    void remove_worker(tcp_socket::socket_t client) {
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

        for (auto& worker : workers_) {
            worker.has_result = false;
            worker.ready = false;
        }

        for (size_t i = 0; i < workers_.size(); ++i) {
            auto& worker = workers_[i];
            string task_msg = "TASK:" + task_data;
            const auto sent = tcp_socket(worker.socket).send(task_msg.data(), task_msg.size());
            if (sent <= 0) {
                println("Failed to send task to worker:", worker.id);
                disconnected.push_back(i);
            } else {
                println("Task sent to worker:", worker.id);
                worker.ready = true;
            }
        }

        for (auto it = disconnected.rbegin(); it != disconnected.rend(); ++it) {
            workers_.erase(workers_.begin() + *it);
            --connected_workers_;
        }
    }

    void record_result(tcp_socket::socket_t client, const string& result) {
        lock_guard<mutex> lock(mtx_);
        for (auto& worker : workers_) {
            if (worker.socket == client) {
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

        if (cv_.wait_for(lock, seconds(timeout_seconds),
            [this]() { return results_received_ >= connected_workers_; })) {

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

    void print_results_analysis(const vector<task_result>& results) const {
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
private:
    tcp_server server_;
    worker_manager manager_;
    thread console_thread_;
    atomic_bool running_{false};

private:
    void handle_worker(tcp_server::handle_sock_t client) {
        char buffer[1024];

        ssize_t n = client.receive(buffer, sizeof(buffer) - 1);
        if (n <= 0) {
            return;
        }
        buffer[n] = '\0';
        string worker_id(buffer, n);

        manager_.register_worker(client.socket().sockfd(), worker_id);

        while (true) {
            n = client.receive(buffer, sizeof(buffer) - 1);
            if (n <= 0) {
                break;
            }
            buffer[n] = '\0';
            string msg(buffer, n);

            if (msg.find("RESULT:") == 0) {
                string result = msg.substr(7);
                manager_.record_result(client.socket().sockfd(), result);
            } else if (msg == "HEARTBEAT") {
                constexpr string_view A = "ALIVE";
                auto sent = client.send(A.data(), 5);
            }
        }

        manager_.remove_worker(client.socket().sockfd());
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
                if (manager_.get_connected_count() == 0) {
                    println("No workers connected");
                    continue;
                }

                println("Sending task to all workers...");
                manager_.send_task_to_all(task_data);

                println("Waiting for results...");
                auto results = manager_.wait_for_all_results();

                manager_.print_results_analysis(results);
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
    explicit task_server(uint16_t port) : server_(port) {
        server_.set_client_handler([this](tcp_server::handle_sock_t client) {
            handle_worker(move(client));
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

        console_thread_ = thread([this]() {
            console_loop();
        });

        printcln(color::green(), "Task server started on port:", server_.port());
        println("Type 'help' for available commands");
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
};

int main() {
    task_server server(8080);
    if (!server.start()) {
        printcln(color::red(), "Failed to start server");
        return 1;
    }
    while (true) {
        this_thread::sleep_for(seconds(1));
    }
}