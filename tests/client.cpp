#include <MSTL/network/tcp_client.hpp>
#include <MSTL/core/system/console.hpp>
#include <MSTL/core/async/thread_pool.hpp>
#include <MSTL/core/system/process.hpp>
#include <MSTL/core/string/string_util.hpp>
#include "message_protocol.hpp"
using namespace MSTL;

string generate_worker_id() {
    const int rnd = random_mt::next_int(1000, 9999);
    return "WORKER_" + to_string(rnd);
}


class worker_client {
private:
    tcp_client client_;
    string worker_id_;
    atomic_bool running_{false};
    thread receive_thread_;
    thread heartbeat_thread_;
    thread task_worker_thread_;
    mutex socket_mtx_;
    thread_pool pool_;

private:
    void heartbeat_loop() {
        while (running_) {
            this_thread::sleep_for(seconds(3));
            if (client_.is_connected()) {
                lock_guard<mutex> lock(socket_mtx_);
                try {
                    if (!message_transport::send_message(
                        client_.socket(), "HEARTBEAT MSG", MESSAGE_TYPE::HEARTBEAT_REQUEST)) {
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

    void task_cmp(string msg) {
        {
            lock_guard<mutex> lock(socket_mtx_);
            try {
                if (message_transport::send_message(client_.socket(), msg, MESSAGE_TYPE::TASK_COMPLETE_REQUEST)) {
                    printcln(color::green(), "Result sent:", to_string(msg));
                    } else {
                        printcln(color::red(), "Failed to send result");
                        running_.store(false);
                    }
            } catch (const exception& e) {
                printcln(color::red(), "Failed to send result:", e.what());
                running_.store(false);
            }
        }
    }

    bool process_data(const string& data) {
        auto vec = split(data, " ");
        if (vec.size() < 2) {
            printcln(color::red(), "Data formation invalid");
            return false;
        }
        string type = vec.front();
        type.uppercase();
        if (type == "PROCESS") {
            pool_.submit_task([vec, this] {
                println(vec);
                auto info = process::create_process(vec[1], {vec.begin() + 1, vec.end()}, true);
                int ret = process::wait_for_process(info);
                task_cmp("PROCESS RET:" + to_string(ret) + " : " + to_string(info.stdout_output));
            });
        } else if (type == "STRING") {
            if (vec.size() > 2) {
                printcln(color::red(), "Data formation invalid");
                return false;
            }
            task_cmp("PROCESSED_" + vec[1] + "_DONE");
        }
        return true;
    }

    void receive_loop() {
        while (running_) {
            message msg;
            bool success = false;
            {
                lock_guard<mutex> lock(socket_mtx_);
                success = message_transport::receive_message(
                    client_.socket(), msg, milliseconds(1000));
            }

            if (!success) {
                printcln(color::yellow(), "Connection lost or timeout");
                this_thread::sleep_for(milliseconds(500));
            } else {
                println("Client receive message:", msg.header);
            }

            if (msg.header.type == MESSAGE_TYPE::HEARTBEAT_RESPONSE) {
                // Heartbeat response, do nothing
                println("Receive ALIVE");
                continue;
            } else if (msg.header.type == MESSAGE_TYPE::TASK_COMPLETE_RESPONSE) {
                println("Receive RESULT_ACK");
                continue;
            } else if (msg.header.type == MESSAGE_TYPE::SUBMIT_TASK_REQUEST) {
                string task_data{msg.body.begin(), msg.body.end()};
                printcln(color::cyan(), "Received task:", task_data);

                if (!process_data(task_data)) {
                    continue;
                }
                {
                    lock_guard<mutex> lock(socket_mtx_);
                    try {
                        if (message_transport::send_message(
                            client_.socket(), "Task submitted", MESSAGE_TYPE::SUBMIT_TASK_RESPONSE)) {
                            printcln(color::green(), "Task submitted");
                        } else {
                            printcln(color::red(), "Failed to send submitted");
                            running_.store(false);
                        }
                    } catch (const exception& e) {
                        printcln(color::red(), "Failed to submitted task:", e.what());
                        running_.store(false);
                    }
                }
            } else {
                printcln(color::yellow(), "Unknown message:", msg.header);
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
            if (!message_transport::send_message(
                client_.socket(), worker_id_, MESSAGE_TYPE::REGISTER_WORKER_REQUEST)) {
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

        pool_.start();

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

        pool_.stop();
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