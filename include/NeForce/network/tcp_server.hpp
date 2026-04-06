#ifndef NEFORCE_NETWORK_TCP_TCP_SERVER_HPP__
#define NEFORCE_NETWORK_TCP_TCP_SERVER_HPP__
#include "NeForce/core/async/thread_pool.hpp"
#include "NeForce/network/socket/ssl_socket.hpp"
#include "NeForce/network/socket/tcp_acceptor.hpp"
NEFORCE_BEGIN_NAMESPACE__

template <typename SocketT>
class basic_tcp_server {
    static_assert(is_base_of_v<tcp_socket, SocketT>, "SocketT must derive from tcp_socket");

public:
    using socket_type = SocketT;
    using client_handler_t = function<void(socket_type)>;
    using exception_handler_t = function<void(const exception&)>;

private:
    tcp_acceptor acceptor_;
    ports port_;
    atomic<bool> running_{false};
    vector<thread> worker_threads_;
    thread_pool client_pool_;

protected:
    client_handler_t client_handler_;
    exception_handler_t exception_handler_;

protected:
    void accept_loop() {
        if (!acceptor_.set_nonblocking(true)) {
            if (exception_handler_) {
                exception_handler_(socket_exception("Failed to set acceptor to non-blocking mode"));
            }
            running_ = false;
            return;
        }

        while (running_) {
            try {
                auto client = acceptor_.accept_nonblock();
                if (!client) {
                    if (!running_) {
                        break;
                    }
                    this_thread::sleep_for(milliseconds(10));
                    continue;
                }

                if (!client->is_open()) {
                    continue;
                }

                client_pool_.submit_task([this, sock = move(*client)]() mutable {
                    try {
                        this->handle_client(socket_type(move(sock)));
                    } catch (const exception& e) {
                        if (running_ && exception_handler_) {
                            exception_handler_(e);
                        }
                    }
                });
            } catch (const exception& e) {
                if (running_) {
                    exception_handler_(e);
                }
            }
        }
    }

    virtual void handle_client(socket_type client) {
        if (client_handler_) {
            client_handler_(_NEFORCE move(client));
        }
    }

public:
    explicit basic_tcp_server(const uint16_t port, const size_t worker_count = thread_pool::max_thread_threshhold()) :
    port_(port) {
        if (worker_count == 0) {
            NEFORCE_THROW_EXCEPTION(value_exception("Worker count must be greater than 0"));
        }
        client_pool_.set_thread_threshhold(worker_count);
        client_pool_.start();
    }

    virtual ~basic_tcp_server() { stop(); }

    basic_tcp_server(const basic_tcp_server&) = delete;
    basic_tcp_server& operator=(const basic_tcp_server&) = delete;

    basic_tcp_server(basic_tcp_server&&) noexcept = default;
    basic_tcp_server& operator=(basic_tcp_server&&) noexcept = default;

    bool set_client_handler(client_handler_t handler) {
        if (running_) {
            return false;
        }
        client_handler_ = _NEFORCE move(handler);
        return true;
    }

    bool set_exception_handler(exception_handler_t handler) {
        if (running_) {
            return false;
        }
        exception_handler_ = move(handler);
        return true;
    }

    virtual bool load_certificate(const string&, const string&) { return false; }

    virtual bool start(const int backlog = SOMAXCONN) noexcept {
        if (running_) {
            return true;
        }
        if (backlog <= 0) {
            return false;
        }
        if (!client_handler_) {
            return false;
        }

        try {
            const auto endpoint = ip_address::any(port_);
            acceptor_.open(endpoint, backlog);

            running_ = true;
            worker_threads_.emplace_back(&basic_tcp_server::accept_loop, this);

            return true;
        } catch (...) {
            running_ = false;
            acceptor_.close();
            return false;
        }
    }

    void stop() {
        if (!running_) {
            return;
        }

        running_ = false;
        acceptor_.close();

        for (auto& t: worker_threads_) {
            if (t.joinable()) {
                t.join();
            }
        }
        worker_threads_.clear();

        client_pool_.stop();
    }

    NEFORCE_NODISCARD bool is_running() const noexcept { return running_; }

    NEFORCE_NODISCARD ports port() const noexcept { return port_; }
};


using tcp_server = basic_tcp_server<tcp_socket>;


class ssl_server final : public basic_tcp_server<ssl_socket> {
private:
    ssl_context ssl_ctx_;

protected:
    void handle_client(ssl_socket client) override {
        if (!client.is_open()) {
            return;
        }

        try {
            client.init_server_ssl(ssl_ctx_);

            if (client_handler_) {
                client_handler_(_NEFORCE move(client));
            }
        } catch (const exception& e) {
            if (is_running() && exception_handler_) {
                exception_handler_(e);
            }
            client.close();
        }
    }

public:
    explicit ssl_server(const uint16_t port, const size_t worker_count = thread_pool::max_thread_threshhold()) :
    basic_tcp_server<ssl_socket>(port, worker_count),
    ssl_ctx_(ssl_method::TLS_SERVER) {}

    bool load_certificate(const string& cert_file, const string& key_file) override {
        if (is_running()) {
            return false;
        }
        if (cert_file.empty() || key_file.empty()) {
            return false;
        }
        return ssl_ctx_.load_certificate(cert_file, key_file);
    }

    void set_ssl_context(ssl_context ctx) {
        if (is_running()) {
            NEFORCE_THROW_EXCEPTION(ssl_exception("Cannot set SSL context while server is running"));
        }
        if (!ctx.is_valid()) {
            NEFORCE_THROW_EXCEPTION(ssl_exception("Invalid SSL context"));
        }
        ssl_ctx_ = _NEFORCE move(ctx);
    }

    NEFORCE_NODISCARD ssl_context& get_ssl_context() noexcept { return ssl_ctx_; }

    NEFORCE_NODISCARD const ssl_context& get_ssl_context() const noexcept { return ssl_ctx_; }

    bool start(const int backlog = SOMAXCONN) noexcept override {
        if (!ssl_ctx_.is_valid()) {
            return false;
        }
        return basic_tcp_server::start(backlog);
    }
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_TCP_TCP_SERVER_HPP__
