#include <NeForce/network/tcp/tcp_server.hpp>
NEFORCE_BEGIN_NAMESPACE__

void tcp_server_base::accept_loop() {
    if (!acceptor_->set_nonblocking(true)) {
        {
            shared_lock<shared_mutex> lock(handler_mutex_);
            if (exception_handler_) {
                exception_handler_(socket_exception("Failed to set acceptor to non-blocking mode"));
            }
        }
        running_ = false;
        return;
    }

    const auto acceptor_fd = static_cast<int>(acceptor_->native_handle());
    auto fired = make_shared<atomic<bool>>(false);
    bool fd_registered = false;

    auto register_fd = [&] {
        if (fd_registered) {
            ctx_.remove_fd(acceptor_fd);
            fd_registered = false;
        }
        fired->store(false, memory_order_release);
        ctx_.add_fd(acceptor_fd, epoll_in,
                    [fired](int, uint32_t, error_code) { fired->store(true, memory_order_release); });
        fd_registered = true;
    };

    // Initial registration
    register_fd();

    while (running_) {
        // Wait for accept event via io_context (or timeout after 200ms)
        ctx_.run_one(200);

        if (!running_) {
            break;
        }

        bool had_accept = fired->exchange(false, memory_order_acq_rel);

        // Accept all pending connections
        while (running_) {
            try {
                auto client_opt = accept_one();
                if (!client_opt) {
                    break;
                }

                client_handler_t handler;
                {
                    shared_lock<shared_mutex> lock(handler_mutex_);
                    handler = client_handler_;
                }

                if (!handler) {
                    continue;
                }

                auto holder = make_shared<unique_ptr<tcp_socket>>(move(*client_opt));
                ctx_.post([this, handler = move(handler), holder = move(holder)]() mutable {
                    try {
                        handler(move(*holder));
                    } catch (const exception& e) {
                        if (!running_) {
                            return;
                        }
                        shared_lock<shared_mutex> lock(handler_mutex_);
                        if (exception_handler_) {
                            exception_handler_(e);
                        }
                    }
                });
            } catch (const exception& e) {
                if (!running_) {
                    break;
                }
                shared_lock<shared_mutex> lock(handler_mutex_);
                if (exception_handler_) {
                    exception_handler_(e);
                }
                break;
            }
        }

        // Edge-triggered epoll requires re-registration after each event batch.
        // Re-register to catch connections that arrived during accept processing.
        if (running_ && !had_accept) {
            // Also flush any io_context handlers that were posted
            ctx_.poll();
        }
        if (running_) {
            register_fd();
        }
    }

    if (fd_registered) {
        ctx_.remove_fd(acceptor_fd);
    }
}

tcp_server_base::tcp_server_base(const ports port, io_context& ctx, const size_t worker_count) :
ctx_(ctx),
port_(port),
worker_count_(worker_count == 0 ? sysinfo::instance().get_CPU_info().logical_processors : worker_count) {
    if (worker_count_ == 0) {
        NEFORCE_THROW_EXCEPTION(value_exception("Worker count must be greater than 0"));
    }
}

bool tcp_server_base::set_client_handler(client_handler_t handler) {
    unique_lock<shared_mutex> lock(handler_mutex_);
    if (running_) {
        return false;
    }
    client_handler_ = move(handler);
    return true;
}

bool tcp_server_base::set_exception_handler(exception_handler_t handler) {
    unique_lock<shared_mutex> lock(handler_mutex_);
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

    {
        shared_lock<shared_mutex> lock(handler_mutex_);
        if (!client_handler_) {
            return false;
        }
    }

    try {
        const auto endpoint = ip_address::any(port_);
        create_acceptor(endpoint, backlog);

        if (port_.value() == 0) {
            if (const auto local = acceptor_->local_endpoint()) {
                port_ = local->port();
            }
        }

        running_ = true;
        worker_threads_.emplace_back([this] {
            try {
                accept_loop();
                // NOLINTNEXTLINE(bugprone-empty-catch)
            } catch (...) {
                // ignore
            }
        });

        ctx_work_ = make_unique<io_context::work>(ctx_);
        ctx_.run_pool(worker_count_);
        return true;
    } catch (...) {
        running_ = false;
        ctx_work_.reset();
        acceptor_.reset();
        return false;
    }
}

void tcp_server_base::stop() {
    bool expected = true;
    if (!running_.compare_exchange_strong(expected, false)) {
        return;
    }

    ctx_work_.reset();
    ctx_.stop();

    for (auto& t: worker_threads_) {
        if (t.joinable()) {
            t.join();
        }
    }
    worker_threads_.clear();

    ctx_.restart();

    {
        lock<mutex> lock(acceptor_mutex_);
        if (acceptor_) {
            acceptor_->close();
            acceptor_.reset();
        }
    }
}

void tcp_server::create_acceptor(const ip_address& endpoint, int backlog) {
    auto acc = make_unique<tcp_acceptor>();
    acc->open(endpoint, backlog);
    acceptor_ = move(acc);
}

optional<unique_ptr<tcp_socket>> tcp_server::accept_one() {
    auto* acc = static_cast<tcp_acceptor*>(acceptor_.get());
    if (acc == nullptr) {
        return none;
    }
    auto client = acc->accept_nonblock();
    if (!client) {
        return none;
    }
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::WSAEventSelect(client->native_handle(), nullptr, 0);
#endif
    client->set_nonblocking(false);
    return make_unique<tcp_socket>(move(*client));
}

ssl_server::ssl_server(const ports port, io_context& ctx, const size_t worker_count) :
tcp_server_base(port, ctx, worker_count),
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
    if (!ctx.is_valid() || !ctx.has_certificate()) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("Invalid SSL context"));
    }
    ssl_ctx_ = move(ctx);
}

bool ssl_server::start(const int backlog) noexcept {
    if (!ssl_ctx_.is_valid() || !ssl_ctx_.has_certificate()) {
        return false;
    }
    return tcp_server_base::start(backlog);
}

void ssl_server::create_acceptor(const ip_address& endpoint, int backlog) {
    if (!ssl_ctx_.is_valid()) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("SSL context is invalid"));
    }
    auto acc = make_unique<ssl_acceptor>();
    acc->set_ssl_context(ssl_ctx_.clone());
    acc->open(endpoint, backlog);
    acceptor_ = move(acc);
}

optional<unique_ptr<tcp_socket>> ssl_server::accept_one() {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    auto* acc = static_cast<ssl_acceptor*>(acceptor_.get());
    if (acc == nullptr) {
        return none;
    }
    try {
        return make_unique<ssl_socket>(acc->accept_ssl());
    } catch (const exception& e) {
        if (running_) {
            shared_lock<shared_mutex> hl(handler_mutex_);
            if (exception_handler_) {
                exception_handler_(e);
            }
        }
        return none;
    }
}

NEFORCE_END_NAMESPACE__
