#include <MSTL/network/tcp_server.hpp>
#include <MSTL/core/system/console.hpp>

using namespace MSTL;


class client_manager {
private:
    vector<tcp_socket::socket_t> clients_;
    mutable mutex mtx_;

public:
    void add_client(tcp_socket::socket_t client) {
        lock_guard<mutex> lock(mtx_);
        clients_.push_back(client);
        println("New client updated:", client);
    }

    void remove_client(tcp_socket::socket_t client) {
        lock_guard<mutex> lock(mtx_);
        for (auto it = clients_.begin(); it != clients_.end(); ++it) {
            if (*it == client) {
                clients_.erase(it);
                println("client removed:", client);
                return;
            }
        }
    }

    void broadcast(const string& msg) {
        lock_guard<mutex> lock(mtx_);
        vector<size_t> disconnected;
        for (size_t i = 0; i < clients_.size(); ++i) {
            auto client = clients_[i];
            const auto sent = tcp_socket(client).send(msg.data(), msg.size());
            if (sent <= 0) {
                println("client disconnected:", client);
                disconnected.push_back(i);
            }
        }

        for (const size_t it : disconnected) {
            clients_.erase(clients_.begin() + it);
        }
    }

    size_t client_count() const {
        lock_guard<mutex> lock(mtx_);
        return clients_.size();
    }
};


class broadcast_server {
private:
    tcp_server server_;
    client_manager manager_;
    thread broadcast_thread_;
    atomic_bool running_{false};

private:
    void broadcast_loop() {
        int counter = 0;
        while (running_) {
            this_thread::sleep_for(seconds(5));
            if (manager_.client_count() > 0) {
                string msg = "Server broadcast #" + to_string(++counter);
                manager_.broadcast(msg);
            }
        }
    }

    void handle_client(tcp_server::handle_sock_t client) {
        manager_.add_client(client.socket().sockfd());
        try {
            receive_message(client);
        } catch (const exception& e) {
            println("client received exception:", e.what());
        }
    }

    void receive_message(const tcp_server::handle_sock_t& client) {
        char buffer[1024];
        while (true) {
            const ssize_t n = client.receive(buffer, sizeof(buffer) - 1);
            if (n <= 0) {
                break;
            }
            buffer[n] = '\0';
            string msg(buffer, n);
            println("client received:", msg);
            string response = "server received:" + msg;
            ssize_t ss = client.send(response.data(), response.size());

            if (manager_.client_count()) {
                manager_.broadcast(msg);
            }
        }
    }

public:
    explicit broadcast_server(const uint16_t port) : server_(port) {
        server_.set_client_handler([this](tcp_server::handle_sock_t client) {
            handle_client(move(client));
        });
    }

    ~broadcast_server() {
        stop();
    }

    bool start() {
        if (running_) return true;
        if (!server_.start()) {
            return false;
        }
        running_.store(true);

        broadcast_thread_ = thread([this]() {
            broadcast_loop();
        });
        printcln(color::green(), "broadcast server started on port:", server_.port());
        return true;
    }

    void stop() {
        if (!running_) return;
        running_.store(false);
        if (broadcast_thread_.joinable()) {
            broadcast_thread_.join();
        }
        server_.stop();
        printcln(color::yellow(), "broadcast server stopped on port:", server_.port());
    }
};


int main() {
    broadcast_server server(8080);
    if (!server.start()) {
        printcln(color::red(), "failed to start server");
        return 1;
    }
    console.pause("press ctrl^c to interrupt this process...\n");
    server.stop();
    return 0;
}
