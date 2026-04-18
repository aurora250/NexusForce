#include <NeForce/network/tcp/tcp_server.hpp>
NEFORCE_BEGIN_NAMESPACE__

void tcp_server_base::accept_loop() {
    if (!acceptor_->set_nonblocking(true)) {
        if (exception_handler_) {
            exception_handler_(socket_exception("Failed to set acceptor to non-blocking mode"));
        }
        running_ = false;
        return;
    }

    while (running_) {
        try {
            auto client_opt = accept_one();
            if (!client_opt) {
                if (!running_) {
                    break;
                }
                this_thread::sleep_for(milliseconds(10));
                continue;
            }

            client_pool_.submit_task([this, sock = move(*client_opt)]() mutable {
                try {
                    handle_client(move(sock));
                } catch (const exception& e) {
                    if (running_ && exception_handler_) {
                        exception_handler_(e);
                    }
                }
            });
        } catch (const exception& e) {
            if (running_ && exception_handler_) {
                exception_handler_(e);
            }
        }
    }
}

tcp_server_base::tcp_server_base(const ports port, const size_t worker_count) :
port_(port) {
    if (worker_count == 0) {
        NEFORCE_THROW_EXCEPTION(value_exception("Worker count must be greater than 0"));
    }
    client_pool_.set_thread_threshhold(worker_count);
    client_pool_.start();
}

bool tcp_server_base::set_client_handler(client_handler_t handler) {
    if (running_) {
        return false;
    }
    client_handler_ = _NEFORCE move(handler);
    return true;
}

bool tcp_server_base::set_exception_handler(exception_handler_t handler) {
    if (running_) {
        return false;
    }
    exception_handler_ = move(handler);
    return true;
}

bool tcp_server_base::start(const int backlog) noexcept {
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
        create_acceptor(endpoint, backlog);

        running_ = true;
        worker_threads_.emplace_back(&tcp_server_base::accept_loop, this);
        return true;
    } catch (...) {
        running_ = false;
        acceptor_.reset();
        return false;
    }
}

void tcp_server_base::stop() {
    if (!running_) {
        return;
    }

    running_ = false;
    if (acceptor_) {
        acceptor_->close();
    }

    for (auto& t: worker_threads_) {
        if (t.joinable()) {
            t.join();
        }
    }
    worker_threads_.clear();

    client_pool_.stop();
}

void tcp_server::create_acceptor(const ip_address& endpoint, int backlog) {
    auto acc = make_unique<tcp_acceptor>();
    acc->open(endpoint, backlog);
    acceptor_ = move(acc);
}

optional<tcp_socket> tcp_server::accept_one() {
    auto* acc = static_cast<tcp_acceptor*>(acceptor_.get());
    return acc->accept_nonblock();
}

ssl_server::ssl_server(const ports port, const size_t worker_count) :
tcp_server_base(port, worker_count),
ssl_ctx_(ssl_method::TLS_SERVER) {}

bool ssl_server::load_certificate(const string& cert_file, const string& key_file) {
    if (is_running()) {
        return false;
    }
    if (cert_file.empty() || key_file.empty()) {
        return false;
    }
    return ssl_ctx_.load_certificate(cert_file, key_file);
}

void ssl_server::set_ssl_context(ssl_context ctx) {
    if (is_running()) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("Cannot set SSL context while server is running"));
    }
    if (!ctx.is_valid()) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("Invalid SSL context"));
    }
    ssl_ctx_ = _NEFORCE move(ctx);
}

bool ssl_server::start(const int backlog) noexcept {
    if (!ssl_ctx_.is_valid()) {
        return false;
    }
    return tcp_server_base::start(backlog);
}

void ssl_server::create_acceptor(const ip_address& endpoint, int backlog) {
    if (!ssl_ctx_.is_valid()) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("SSL context is invalid"));
    }
    auto acc = make_unique<ssl_acceptor>();
    acc->set_ssl_context(move(ssl_ctx_));
    acc->open(endpoint, backlog);
    acceptor_ = move(acc);
}

optional<tcp_socket> ssl_server::accept_one() {
    auto* acc = static_cast<ssl_acceptor*>(acceptor_.get());
    try {
        return acc->accept_ssl();
    } catch (...) {
        return none;
    }
}

NEFORCE_END_NAMESPACE__
