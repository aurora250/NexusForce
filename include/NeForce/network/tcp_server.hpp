#ifndef NEFORCE_NETWORK_TCP_TCP_SERVER_HPP__
#define NEFORCE_NETWORK_TCP_TCP_SERVER_HPP__
#include "NeForce/core/async/thread_pool.hpp"
#include "NeForce/network/socket/tcp_acceptor.hpp"
#ifdef NEFORCE_SUPPORT_OPENSSL
#include "NeForce/network/socket/ssl_socket.hpp"
#endif
NEFORCE_BEGIN_NAMESPACE__

template <typename SocketT>
class basic_tcp_server {
public:
    using socket_type = SocketT;
    using client_handler_t = function<void(socket_type)>;
    using exception_handler_t = function<void(const exception&)>;

private:
    tcp_acceptor acceptor_;
    uint16_t port_;
    atomic<bool> running_{false};
    vector<thread> worker_threads_;
    thread_pool client_pool_;

protected:
    client_handler_t client_handler_;
    exception_handler_t exception_handler_;

protected:
    void accept_loop() {
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

                client_pool_.submit_task([this, sock = _NEFORCE move(*client)]() mutable {
                    try {
                        this->handle_client(socket_type(_NEFORCE move(sock)));
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
    explicit basic_tcp_server(
        const uint16_t port,
        const size_t worker_count = thread_pool::max_threshhold)
    : port_(port) {
        if (worker_count == 0) {
            throw_exception(value_exception("Worker count must be greater than 0"));
        }
        client_pool_.set_thread_threshhold(worker_count);
        client_pool_.start();
    }

    virtual ~basic_tcp_server() {
        stop();
    }

    basic_tcp_server(const basic_tcp_server&) = delete;
    basic_tcp_server& operator =(const basic_tcp_server&) = delete;

    basic_tcp_server(basic_tcp_server&&) noexcept = default;
    basic_tcp_server& operator =(basic_tcp_server&&) noexcept = default;
    
    void set_client_handler(client_handler_t handler) {
        client_handler_ = _NEFORCE move(handler);
    }

    void set_exception_handler(exception_handler_t handler) {
        exception_handler_ = _NEFORCE move(handler);
    }

    virtual bool load_certificate(const string&, const string&) {
        return false;
    }

    virtual bool start(const int backlog = SOMAXCONN) noexcept {
        if (running_) {
            return true;
        }
        if (backlog <= 0) {
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

        for (auto& t : worker_threads_) {
            if (t.joinable()) t.join();
        }
        worker_threads_.clear();

        client_pool_.stop();
    }

    bool is_running() const noexcept {
        return running_;
    }

    uint16_t port() const noexcept {
        return port_;
    }
};


using tcp_server = basic_tcp_server<tcp_socket>;


#ifdef NEFORCE_SUPPORT_OPENSSL

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
    explicit ssl_server(
        const uint16_t port,
        const size_t worker_count = thread_pool::max_threshhold)
    : basic_tcp_server<ssl_socket>(port, worker_count),
      ssl_ctx_(ssl_method::TLS_SERVER) {}

    bool load_certificate(const string& cert_file, const string& key_file) override {
        if (cert_file.empty() || key_file.empty()) {
            return false;
        }
        return ssl_ctx_.load_certificate(cert_file, key_file);
    }

    void set_ssl_context(ssl_context ctx) {
        if (!ctx.is_valid()) {
            throw_exception(ssl_exception("Invalid SSL context"));
        }
        ssl_ctx_ = _NEFORCE move(ctx);
    }

    ssl_context& get_ssl_context() noexcept {
        return ssl_ctx_;
    }

    const ssl_context& get_ssl_context() const noexcept {
        return ssl_ctx_;
    }

    bool start(const int backlog = SOMAXCONN) noexcept override {
        if (!ssl_ctx_.is_valid()) {
            return false;
        }
        return basic_tcp_server::start(backlog);
    }
};

#endif // NEFORCE_SUPPORT_OPENSSL


NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_TCP_TCP_SERVER_HPP__
