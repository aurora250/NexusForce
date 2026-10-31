#include <NeForce/core/async/mutex.hpp>
#include <NeForce/network/tcp/tcp_server.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <WinSock2.h>
#else
#    include <poll.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

namespace {
    const auto processors = sysinfo::instance().get_cpu_info().logical_processors;
}


void tcp_server_base::accept_loop() {
    if (!acceptor_->set_nonblocking(false)) {
        running_ = false;
        return;
    }

    const auto acceptor_fd = static_cast<int>(acceptor_->native_handle());

    while (running_) {
#ifdef NEFORCE_PLATFORM_WINDOWS
        ::WSAPOLLFD pfd{};
        pfd.fd = static_cast<::SOCKET>(acceptor_fd);
        pfd.events = POLLIN;
        const int ready = ::WSAPoll(&pfd, 1, 100);
#else
        ::pollfd pfd{};
        pfd.fd = acceptor_fd;
        pfd.events = POLLIN;
        const int ready = ::poll(&pfd, 1, 100);
#endif
        if (ready <= 0) {
            continue;
        }
        if (!running_) {
            break;
        }
        try {
            auto client_opt = accept_one();
            if (!client_opt) {
                continue;
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
                    if (!prepare_client(**holder)) {
                        return;
                    }
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
        }
    }
}

tcp_server_base::tcp_server_base(const ports port, io_context& ctx, const size_t worker_count) :
ctx_(ctx),
port_(port),
worker_count_(worker_count == 0 ? processors : worker_count) {
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
            } catch (const exception& e) {
                NEFORCE_REPORT_EXCEPTION(e);
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
ssl_ctx_(ssl_method::tls_server) {}

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
        auto client = acc->tcp_acceptor::accept();
        if (!client.is_open()) {
            return none;
        }
        return make_unique<ssl_socket>(move(client));
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

bool ssl_server::prepare_client(tcp_socket& client) {
    auto* ssl_client = dynamic_cast<ssl_socket*>(&client);
    if (ssl_client == nullptr) {
        return false;
    }
    try {
        client.set_tcp_nodelay(true);
        ssl_client->init_server_ssl(ssl_ctx_);
        return true;
    } catch (const exception& e) {
        if (running_) {
            shared_lock<shared_mutex> hl(handler_mutex_);
            if (exception_handler_) {
                exception_handler_(e);
            }
        }
        return false;
    }
}

NEFORCE_END_NAMESPACE__
