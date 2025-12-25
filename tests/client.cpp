#include <MSTL/network/tcp_client.hpp>
#include <MSTL/core/async/thread.hpp>
#include <MSTL/core/system/console.hpp>
using namespace MSTL;

class worker_client {
private:
    tcp_client client_;
    atomic_bool running_{false};
    thread worker_thread_;

private:
    void receive() {
        while (running_) {
            string response;
            client_.receive_with_callback([](const string& msg) {
                printcln(color::cyan(), "from server:" + msg);
            }, 1024);
            running_ = false;
            printcln(color::yellow(), "connection to server lost");
        }
    }

public:
    bool connect(const string& host, const uint16_t port) {
        if (!client_.connect(host, port)) {
            return false;
        }
        running_ = true;
        worker_thread_ = thread([this]() {
            receive();
        });
        return true;
    }

    void start() {
        while (running_) {
            print("> ");
            string line = console.readln();
            if (line == "quit") {
                break;
            }
            if (!line.empty()) {
                send(line);
            }
        }
    }

    void close() {
        running_ = false;
        if (worker_thread_.joinable()) {
            worker_thread_.join();
        }
        client_.close();
        printcln(color::yellow(), "connection closed");
    }

    void send(const string& msg) {
        if (!client_.is_connected()) {
            printcln(color::red(), "connection to server lost");
            return;
        }
        const ssize_t sent = client_.send(msg.view());
        if (sent) {
            printcln(color::green(), "sent:", msg);
        } else {
            printcln(color::red(), "failed to sent:", msg);
        }
    }
};


int main() {
    worker_client client;
    if (!client.connect("127.0.0.1", 8080)) {
        printcln(color::red(), "connect to server lost");
        return 1;
    }
    client.start();
    client.close();
    return 0;
}
