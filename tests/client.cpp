#include <MSTL/network/tcp_client.hpp>
#include <MSTL/core/async/thread.hpp>
#include <MSTL/core/system/console.hpp>
#include <MSTL/core/numeric/random.hpp>
using namespace MSTL;

string generate_worker_id() {
    int rnd = random_mt::next_int(1000, 9999);
    return "WORKER_" + to_string(rnd);
}

class processing_task {
public:
    static string process_task(const string& task_data) {
        this_thread::sleep_for(milliseconds(100 + rand() % 500));

        if (task_data.find("reverse") == 0) {
            string data = task_data.substr(8);
            return "REVERSED_" + string(data.rbegin(), data.rend());
        } else if (task_data.find("uppercase") == 0) {
            string data = task_data.substr(10);
            string result;
            for (char c : data) {
                result += toupper(c);
            }
            return "UPPERCASE_" + result;
        } else if (task_data.find("count") == 0) {
            string data = task_data.substr(6);
            return "LENGTH_" + to_string(data.length());
        } else {
            return "PROCESSED_" + task_data + "_DONE";
        }
    }
};

class worker_client {
private:
    tcp_client client_;
    string worker_id_;
    atomic_bool running_{false};
    thread receive_thread_;
    thread heartbeat_thread_;

private:
    void heartbeat_loop() {
        while (running_) {
            this_thread::sleep_for(seconds(3));
            if (client_.is_connected()) {
                constexpr string_view HB = "HEARTBEAT";
                client_.send(HB.data(), 8);
            }
        }
    }

    void receive_loop() {
        char buffer[1024];

        while (running_) {
            if (!client_.is_connected()) {
                break;
            }

            ssize_t n = client_.receive(buffer, sizeof(buffer) - 1);
            if (n <= 0) {
                printcln(color::red(), "Connection lost");
                running_.store(false);
                break;
            }

            buffer[n] = '\0';
            string msg(buffer, n);

            if (msg == "ALIVE") {
                // Heartbeat response, do nothing
                continue;
            }

            if (msg.find("TASK:") == 0) {
                string task_data = msg.substr(5);
                printcln(color::cyan(), "Received task:", task_data);

                // Process the task
                string result = processing_task::process_task(task_data);

                // Send result back
                string result_msg = "RESULT:" + result;
                client_.send(result_msg.data(), result_msg.size());
                printcln(color::green(), "Sent result:", result);
            } else {
                printcln(color::yellow(), "Unknown message:", msg);
            }
        }
    }

public:
    bool connect(const string& host, uint16_t port) {
        if (!client_.connect(host, port)) {
            return false;
        }

        worker_id_ = generate_worker_id();

        // Register with server
        client_.send(worker_id_.data(), worker_id_.size());

        running_.store(true);

        receive_thread_ = thread([this]() {
            receive_loop();
        });

        heartbeat_thread_ = thread([this]() {
            heartbeat_loop();
        });

        printcln(color::green(), "Worker", worker_id_, "connected to server");
        return true;
    }

    void interactive_mode() {
        println("Worker", worker_id_, "ready. Type 'quit' to exit.");

        while (running_) {
            print("> ");
            string line = console.readln();

            if (line == "quit") {
                break;
            } else if (line == "status") {
                println("Worker ID:", worker_id_);
                println("Connected:", client_.is_connected() ? "Yes" : "No");
            } else if (line == "help") {
                println("Commands: quit, status, help");
            }
        }
    }

    void close() {
        running_.store(false);

        if (receive_thread_.joinable()) {
            receive_thread_.join();
        }

        if (heartbeat_thread_.joinable()) {
            heartbeat_thread_.join();
        }

        client_.close();
        printcln(color::yellow(), "Worker disconnected");
    }

    bool is_connected() const {
        return client_.is_connected();
    }
};

int main() {
    worker_client client;

    if (!client.connect("127.0.0.1", 8080)) {
        printcln(color::red(), "Failed to connect to server");
        return 1;
    }

    client.interactive_mode();
    client.close();

    return 0;
}