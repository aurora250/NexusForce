#include <MSTL/network/tcp_client.hpp>
#include <MSTL/core/async/thread.hpp>
#include <MSTL/core/system/console.hpp>
#include <MSTL/core/numeric/random.hpp>
#include "message_protocol.hpp"
using namespace MSTL;

string generate_worker_id() {
    const int rnd = random_mt::next_int(1000, 9999);
    return "WORKER_" + to_string(rnd);
}

class processing_task {
public:
    static string process_task(const string& task_data) {
        this_thread::sleep_for(milliseconds(100 + rand() % 500));

        if (task_data.find("reverse") == 0) {
            string data = task_data.substr(8);
            data.reverse();
            return "REVERSED_" + data;
        } else if (task_data.find("uppercase") == 0) {
            string data = task_data.substr(10);
            data.uppercase();
            return "UPPERCASE_" + data;
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
    mutex socket_mtx_;

private:
    void heartbeat_loop() {
        while (running_) {
            this_thread::sleep_for(seconds(3));
            if (client_.is_connected()) {
                lock_guard<mutex> lock(socket_mtx_);
                try {
                    if (!message_protocol::send_message(client_.socket(), "HEARTBEAT")) {
                        printcln(color::yellow(), "Heartbeat send failed");
                        running_.store(false);
                        break;
                    }
                } catch (const exception& e) {
                    printcln(color::red(), "Heartbeat send failed:", e.what());
                    running_.store(false);
                    break;
                }
            }
        }
    }

    void receive_loop() {
        while (running_) {
            string msg;
            bool success = false;
            {
                lock_guard<mutex> lock(socket_mtx_);
                try {
                    success = message_protocol::receive_message(client_.socket(), msg, 10000);
                } catch (const exception& e) {
                    printcln(color::red(), "Receive error:", e.what());
                    success = false;
                }
            }

            if (!success) {
                printcln(color::red(), "Connection lost or timeout");
                running_.store(false);
                break;
            } else {
                println("Client receive message:", msg);
            }

            if (msg == "ALIVE") {
                // Heartbeat response, do nothing
                println("Receive ALIVE");
                continue;
            } else if (msg == "RESULT_ACK") {
                println("Receive RESULT_ACK");
                continue;
            } else if (msg.find("TASK:") == 0) {
                string task_data = msg.substr(5);
                printcln(color::cyan(), "Received task:", task_data);

                string result = processing_task::process_task(task_data);
                string result_msg = "RESULT:" + result;
                {
                    lock_guard<mutex> lock(socket_mtx_);
                    try {
                        if (message_protocol::send_message(client_.socket(), result_msg)) {
                            printcln(color::green(), "Result sent:", result);
                        } else {
                            printcln(color::red(), "Failed to send result");
                            running_.store(false);
                        }
                    } catch (const exception& e) {
                        printcln(color::red(), "Failed to send result:", e.what());
                        running_.store(false);
                    }
                }
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
        {
            lock_guard<mutex> lock(socket_mtx_);
            if (!message_protocol::send_message(client_.socket(), worker_id_)) {
                printcln(color::red(), "Failed to send worker ID");
                return false;
            }
        }

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

    void interactive_mode() const {
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

    while (client.is_connected()) {
        this_thread::sleep_for(seconds(1));
    }
    client.close();

    return 0;
}