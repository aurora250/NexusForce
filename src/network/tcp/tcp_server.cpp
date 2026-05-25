#include <NeForce/network/tcp/tcp_server.hpp>
#ifdef NEFORCE_PLATFORM_LINUX
#    include <poll.h>
#    include <cerrno>
#endif
NEFORCE_BEGIN_NAMESPACE__

#ifdef NEFORCE_PLATFORM_WINDOWS
namespace {
    // NOLINTNEXTLINE(cppcoreguidelines-special-member-functions,hicpp-special-member-functions)
    struct wsa_event_guard {
        ::WSAEVENT event;

        explicit wsa_event_guard(::WSAEVENT e) :
        event(e) {}

        ~wsa_event_guard() {
            if (event != WSA_INVALID_EVENT) {
                ::WSACloseEvent(event);
            }
        }

        wsa_event_guard(const wsa_event_guard&) = delete;
        wsa_event_guard& operator=(const wsa_event_guard&) = delete;
    };
} // namespace
#endif


void tcp_server_base::notify_stop() noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (wake_event_ != WSA_INVALID_EVENT) {
        ::WSASetEvent(wake_event_);
    }
#else
    if (wake_pipe_.is_valid()) {
        constexpr char byte = 1;
        (void) wake_pipe_.write(&byte, 1);
    }
#endif
}

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

    const auto acceptor_fd = acceptor_->native_handle();

#ifdef NEFORCE_PLATFORM_WINDOWS

    wsa_event_guard accept_guard(::WSACreateEvent());
    if (accept_guard.event == WSA_INVALID_EVENT) {
        running_ = false;
        return;
    }
    if (::WSAEventSelect(acceptor_fd, accept_guard.event, FD_ACCEPT) == SOCKET_ERROR) {
        running_ = false;
        return;
    }

    ::HANDLE events[2] = {accept_guard.event, wake_event_};

    while (running_) {
        ::DWORD ret = ::WaitForMultipleObjects(2, events, FALSE, INFINITE);

        if (ret == WAIT_OBJECT_0 + 1) {
            break;
        }
        if (ret == WAIT_FAILED) {
            // error
            break;
        }
        if (ret != WAIT_OBJECT_0) {
            continue;
        }

        ::WSAResetEvent(accept_guard.event);

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

                client_pool_.submit_task([handler = move(handler), sock = move(*client_opt), this]() mutable {
                    try {
                        handler(move(sock));
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
                shared_lock lock(handler_mutex_);
                if (exception_handler_) {
                    exception_handler_(e);
                }
                break;
            }
        }
    }

#else
    const auto wake_read_fd = wake_pipe_.native_read_handle();

    pollfd pfds[2];
    pfds[0].fd = acceptor_fd;
    pfds[0].events = POLLIN;
    pfds[1].fd = wake_read_fd;
    pfds[1].events = POLLIN;

    while (running_) {
        pfds[0].revents = 0;
        pfds[1].revents = 0;

        const int ret = ::poll(pfds, 2, -1);

        if (ret < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }
        if ((pfds[1].revents & POLLIN) != 0) {
            break;
        }

        if ((pfds[0].revents & POLLIN) == 0) {
            continue;
        }

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

                client_pool_.submit_task([this, handler = move(handler), sock = move(*client_opt)]() mutable {
                    try {
                        if (handler) {
                            handler(move(sock));
                        }
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
    }
#endif
}

tcp_server_base::tcp_server_base(const ports port, const size_t worker_count) :
port_(port),
worker_count_(worker_count) {
    if (worker_count == 0) {
        NEFORCE_THROW_EXCEPTION(value_exception("Worker count must be greater than 0"));
    }
}

bool tcp_server_base::set_client_handler(client_handler_t handler) {
    unique_lock<shared_mutex> lock(handler_mutex_);
    if (running_) {
        return false;
    }
    client_handler_ = _NEFORCE move(handler);
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

#ifdef NEFORCE_PLATFORM_WINDOWS
    if (wake_event_ == WSA_INVALID_EVENT) {
        wake_event_ = ::WSACreateEvent();
        if (wake_event_ == WSA_INVALID_EVENT) {
            return false;
        }
    } else {
        ::WSAResetEvent(wake_event_);
    }
#else
    try {
        wake_pipe_ = pipe(false);
    } catch (...) {
        return false;
    }
#endif

    if (!client_pool_.running()) {
        client_pool_.start(worker_count_);
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
        worker_threads_.emplace_back(&tcp_server_base::accept_loop, this);
        return true;
    } catch (...) {
        running_ = false;
        acceptor_.reset();
#ifdef NEFORCE_PLATFORM_WINDOWS
        ::WSACloseEvent(wake_event_);
        wake_event_ = WSA_INVALID_EVENT;
#else
        wake_pipe_.close();
#endif
        return false;
    }
}

void tcp_server_base::stop() {
    bool expected = true;
    if (!running_.compare_exchange_strong(expected, false)) {
        return;
    }

    notify_stop();

    for (auto& t: worker_threads_) {
        if (t.joinable()) {
            t.join();
        }
    }
    worker_threads_.clear();

    {
        lock<mutex> lock(acceptor_mutex_);
        if (acceptor_) {
            acceptor_->close();
            acceptor_.reset();
        }
    }

    client_pool_.stop();
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (wake_event_ != WSA_INVALID_EVENT) {
        ::WSACloseEvent(wake_event_);
        wake_event_ = WSA_INVALID_EVENT;
    }
#else
    wake_pipe_.close();
#endif
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
    if (!ctx.is_valid() || !ctx.has_certificate()) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("Invalid SSL context"));
    }
    ssl_ctx_ = _NEFORCE move(ctx);
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
