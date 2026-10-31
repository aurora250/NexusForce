#include <NeForce/network/socket_base.hpp>
#include <NeForce/network/ssl/ssl_exception.hpp>
#include <NeForce/network/ssl/ssl_stream.hpp>
#include <openssl/err.h>
#include <openssl/ssl.h>
NEFORCE_BEGIN_NAMESPACE__

void ssl_stream::handle_ssl_error(const int ret, const char* operation) {
    if (ssl_ == nullptr) {
        last_error_ = string(operation) + " failed: SSL object is null";
        return;
    }

    const int err = ::SSL_get_error(static_cast<::SSL*>(ssl_), ret);

    switch (err) {
        case SSL_ERROR_NONE: {
            last_error_.clear();
            return;
        }
        case SSL_ERROR_WANT_READ: {
            last_error_ = string(operation) + " needs more data to read";
            return;
        }
        case SSL_ERROR_WANT_WRITE: {
            last_error_ = string(operation) + " needs to write data";
            return;
        }
        case SSL_ERROR_ZERO_RETURN: {
            last_error_ = string(operation) + " connection closed";
            NEFORCE_THROW_EXCEPTION(ssl_exception("SSL connection closed cleanly"));
        }
        case SSL_ERROR_SYSCALL: {
            const unsigned long ssl_err = ::ERR_get_error();
            if (ssl_err == 0) {
                if (ret == 0) {
                    last_error_ = string(operation) + " unexpected EOF";
                    NEFORCE_THROW_EXCEPTION(ssl_exception("Unexpected EOF in SSL operation"));
                }
                last_error_ = string(operation) + " system call error";
                NEFORCE_THROW_EXCEPTION(ssl_exception("System call error in SSL operation"));
            }
            last_error_ = string(operation) + " syscall error: " + ssl_category().message(static_cast<int>(ssl_err));
            NEFORCE_THROW_EXCEPTION(ssl_exception(static_cast<int>(ssl_err)));
        }
        case SSL_ERROR_SSL: {
            const unsigned long ssl_err = ::ERR_get_error();
            last_error_ =
                    string(operation) + " SSL protocol error: " + ssl_category().message(static_cast<int>(ssl_err));
            NEFORCE_THROW_EXCEPTION(ssl_exception(static_cast<int>(ssl_err)));
        }
        default: {
            last_error_ = string(operation) + " unknown error";
            NEFORCE_THROW_EXCEPTION(ssl_exception(err));
        }
    }
}

ssl_stream::ssl_stream(ssl_stream&& other) noexcept :
ssl_(other.ssl_),
last_error_(move(other.last_error_)) {
    other.ssl_ = nullptr;
}

ssl_stream& ssl_stream::operator=(ssl_stream&& other) noexcept {
    if (addressof(other) == this) {
        return *this;
    }
    free_ssl();
    ssl_ = other.ssl_;
    other.ssl_ = nullptr;
    last_error_ = move(other.last_error_);
    return *this;
}

ssl_stream::~ssl_stream() { free_ssl(); }

void ssl_stream::free_ssl() noexcept {
    auto* ssl = static_cast<::SSL*>(ssl_);
    if (ssl != nullptr) {
        ::SSL_free(ssl);
    }
    ssl_ = nullptr;
}

void ssl_stream::reset(const ssl_context& ctx) {
    if (!ctx.is_valid()) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("Invalid SSL context"));
    }

    free_ssl();
    ssl_ = ::SSL_new(static_cast<::SSL_CTX*>(ctx.native_handle()));
    if (ssl_ == nullptr) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("SSL_new failed"));
    }

    last_error_.clear();
}

void ssl_stream::set_fd(const native_handle_type fd) {
    if (ssl_ == nullptr) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("SSL object not initialized"));
    }

    if (fd == socket_base::invalid_handle) {
        NEFORCE_THROW_EXCEPTION(value_exception("Invalid file descriptor"));
    }

    if (::SSL_set_fd(static_cast<::SSL*>(ssl_), static_cast<int>(fd)) != 1) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("SSL_set_fd failed"));
    }
}

void ssl_stream::accept() {
    if (ssl_ == nullptr) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("SSL object not initialized"));
    }

    while (true) {
        const int ret = ::SSL_accept(static_cast<::SSL*>(ssl_));
        if (ret == 1) {
            return;
        }
        const int err = ::SSL_get_error(static_cast<::SSL*>(ssl_), ret);
        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
            continue;
        }
        if (err == SSL_ERROR_SYSCALL && ::ERR_peek_error() == 0) {
            last_error_ = "SSL_accept system call error";
            NEFORCE_THROW_EXCEPTION(ssl_exception("SSL accept failed: connection error"));
        }
        handle_ssl_error(ret, "SSL_accept");
    }
}

bool ssl_stream::connect() {
    if (ssl_ == nullptr) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("SSL not initialized"));
    }

    auto* ssl = static_cast<::SSL*>(ssl_);
    ::SSL_set_connect_state(ssl);

    while (true) {
        const int ret = ::SSL_connect(ssl);
        if (ret == 1) {
            return true;
        }
        const int err = ::SSL_get_error(ssl, ret);
        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
            continue;
        }
        if (err == SSL_ERROR_SYSCALL && ::ERR_peek_error() == 0) {
            last_error_ = "SSL_connect system call error";
            NEFORCE_THROW_EXCEPTION(ssl_exception("SSL connect failed: connection error"));
        }
        handle_ssl_error(ret, "SSL_connect");
    }
}

void ssl_stream::close() noexcept {
    auto* ssl = static_cast<::SSL*>(ssl_);
    if (ssl != nullptr) {
        ::SSL_shutdown(ssl);
    }
    free_ssl();
    last_error_.clear();
}

void* ssl_stream::export_session() const noexcept {
    if (ssl_ == nullptr) {
        return nullptr;
    }
    return ::SSL_get1_session(static_cast<::SSL*>(ssl_));
}

bool ssl_stream::session_reused() const noexcept {
    return ssl_ != nullptr && ::SSL_session_reused(static_cast<::SSL*>(ssl_)) == 1;
}

bool ssl_stream::session_resumable() const noexcept {
    if (ssl_ == nullptr) {
        return false;
    }
    ::SSL_SESSION* session = ::SSL_get_session(static_cast<::SSL*>(ssl_));
    return session != nullptr && ::SSL_SESSION_is_resumable(session) == 1;
}

void ssl_stream::import_session(void* session) noexcept {
    if (ssl_ != nullptr && session != nullptr) {
        ::SSL_set_session(static_cast<::SSL*>(ssl_), static_cast<::SSL_SESSION*>(session));
    }
}

void* ssl_stream::duplicate_session(void* session) noexcept {
    if (session == nullptr) {
        return nullptr;
    }
    auto* ssl_session = static_cast<::SSL_SESSION*>(session);
    if (::SSL_SESSION_up_ref(ssl_session) != 1) {
        return nullptr;
    }
    return ssl_session;
}

void ssl_stream::free_session(void* session) noexcept {
    if (session != nullptr) {
        ::SSL_SESSION_free(static_cast<::SSL_SESSION*>(session));
    }
}

ssize_t ssl_stream::read(void* buffer, const size_t size) {
    if (ssl_ == nullptr) {
        last_error_ = "SSL object not initialized";
        return -1;
    }
    if (buffer == nullptr && size > 0) {
        last_error_ = "Invalid buffer";
        return -1;
    }
    if (size == 0) {
        return 0;
    }

    if (size > numeric_traits<int>::max()) {
        last_error_ = "Size exceeds maximum allowed";
        return -1;
    }

    const int ret = ::SSL_read(static_cast<::SSL*>(ssl_), buffer, static_cast<int>(size));
    if (ret <= 0) {
        const int err = ::SSL_get_error(static_cast<::SSL*>(ssl_), ret);
        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
            last_error_ = "SSL_read would block";
            return 0;
        }
        if (err == SSL_ERROR_ZERO_RETURN) {
            last_error_ = "SSL connection closed cleanly";
            return 0;
        }
        const unsigned long ssl_err = ::ERR_get_error();
        if (err == SSL_ERROR_SYSCALL && ssl_err == 0) {
            last_error_ = "SSL_read system call error";
            return -1;
        }
        if (ssl_err != 0) {
            last_error_ = "SSL_read error: " + ssl_category().message(static_cast<int>(ssl_err));
        } else {
            last_error_ = "SSL_read error";
        }
        return -1;
    }

    last_error_.clear();
    return ret;
}

ssize_t ssl_stream::write(const void* buffer, const size_t size) {
    if (ssl_ == nullptr) {
        last_error_ = "SSL object not initialized";
        return -1;
    }
    if (buffer == nullptr && size > 0) {
        last_error_ = "Invalid buffer";
        return -1;
    }
    if (size == 0) {
        return 0;
    }

    if (size > numeric_traits<int>::max()) {
        last_error_ = "Size exceeds maximum allowed";
        return -1;
    }

    const int ret = ::SSL_write(static_cast<::SSL*>(ssl_), buffer, static_cast<int>(size));
    if (ret <= 0) {
        const int err = ::SSL_get_error(static_cast<::SSL*>(ssl_), ret);
        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
            last_error_ = "SSL_write would block";
            return 0;
        }
        const unsigned long ssl_err = ::ERR_get_error();
        if (err == SSL_ERROR_SYSCALL && ssl_err == 0) {
            last_error_ = "SSL_write system call error";
            return -1;
        }
        if (ssl_err != 0) {
            last_error_ = "SSL_write error: " + ssl_category().message(static_cast<int>(ssl_err));
        } else {
            last_error_ = "SSL_write error";
        }
        return -1;
    }

    last_error_.clear();
    return ret;
}

vector<char> ssl_stream::read_all(const size_t max_size) {
    if (ssl_ == nullptr) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("SSL object not initialized"));
    }
    if (max_size == 0) {
        return {};
    }

    vector<char> buffer;
    buffer.reserve(min(max_size, MEMORY_BIG_ALLOC_THRESHHOLD));

    char temp[MEMORY_BIG_ALLOC_THRESHHOLD];

    while (buffer.size() < max_size) {
        const size_t to_read = min(sizeof(temp), max_size - buffer.size());
        const ssize_t ret = read(static_cast<char*>(temp), to_read);

        if (ret < 0) {
            NEFORCE_THROW_EXCEPTION(ssl_exception("Failed to read data"));
        }
        if (ret == 0) {
            continue;
        }

        buffer.insert(buffer.end(), static_cast<char*>(temp), static_cast<char*>(temp) + ret);

        if (compare_less(ret, to_read)) {
            break;
        }
    }

    return buffer;
}

bool ssl_stream::write_all(const void* data, const size_t size) {
    if (ssl_ == nullptr) {
        last_error_ = "SSL object not initialized";
        return false;
    }
    if (data == nullptr) {
        last_error_ = "Invalid data pointer";
        return false;
    }

    if (size == 0) {
        return true;
    }

    const auto* ptr = static_cast<const char*>(data);
    size_t remaining = size;

    while (remaining > 0) {
        const ssize_t written = write(ptr, remaining);
        if (written < 0) {
            return false;
        }

        if (written == 0) {
            last_error_ = "SSL_write returned 0";
            return false;
        }

        ptr += written;
        remaining -= written;
    }

    return true;
}

int ssl_stream::pending() const {
    if (ssl_ == nullptr) {
        return 0;
    }
    return ::SSL_pending(static_cast<::SSL*>(ssl_));
}

void ssl_stream::set_sni_hostname(const string& hostname) {
    if (ssl_ == nullptr) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("SSL object not initialized"));
    }

    if (hostname.empty()) {
        NEFORCE_THROW_EXCEPTION(value_exception("Hostname cannot be null"));
    }

    if (::SSL_set_tlsext_host_name(static_cast<::SSL*>(ssl_), hostname.data()) != 1) {
        const auto err = ::ERR_get_error();
        string error_msg = "SSL_set_tlsext_host_name failed: ";
        error_msg += ssl_category().message(static_cast<int>(err));
        NEFORCE_THROW_EXCEPTION(ssl_exception(error_msg));
    }

#if OPENSSL_VERSION_NUMBER >= 0x10002000L
    ::X509_VERIFY_PARAM* param = ::SSL_get0_param(static_cast<::SSL*>(ssl_));
    if (param != nullptr) {
        ::X509_VERIFY_PARAM_set1_host(param, hostname.data(), 0);
    }
#endif
}

x509_certificate ssl_stream::peer_certificate() const {
    if (ssl_ == nullptr) {
        return {};
    }
    return x509_certificate(::SSL_get_peer_certificate(static_cast<::SSL*>(ssl_)));
}

bool ssl_stream::verify_peer() const {
    if (ssl_ == nullptr) {
        return false;
    }

    ::X509* cert = ::SSL_get_peer_certificate(static_cast<::SSL*>(ssl_));
    if (cert == nullptr) {
        return false;
    }

    const long verify_result = ::SSL_get_verify_result(static_cast<::SSL*>(ssl_));
    ::X509_free(cert);

    return verify_result == X509_V_OK;
}

string ssl_stream::cipher_name() const {
    if (ssl_ == nullptr) {
        return "";
    }
    return ::SSL_get_cipher_name(static_cast<::SSL*>(ssl_));
}

string ssl_stream::version() const {
    if (ssl_ == nullptr) {
        return "";
    }
    return ::SSL_get_version(static_cast<::SSL*>(ssl_));
}

string ssl_stream::alpn_negotiated() const {
    if (ssl_ == nullptr) {
        return "";
    }
    const byte_t* data = nullptr;
    uint32_t len = 0;
    ::SSL_get0_alpn_selected(static_cast<::SSL*>(ssl_), &data, &len);
    if (data == nullptr || len == 0) {
        return "";
    }
    return {reinterpret_cast<const char*>(data), len};
}

void* ssl_stream::release() noexcept {
    auto* tmp = ssl_;
    ssl_ = nullptr;
    return tmp;
}

namespace {
    struct ssl_handshake_op : enable_shared_from_this<ssl_handshake_op> {
        io_context* ctx;
        ssl_stream* stream;
        function<void(error_code)> handler;
        cancellation_slot* cancel_slot{nullptr};
        void* ssl_ptr;
        int fd_{-1};
        bool registered_{false};
        atomic<bool> done_{false};
        mutex mutex_;

        void finish(error_code ec) {
            if (done_.exchange(true, memory_order_acq_rel)) {
                return;
            }
            if (registered_) {
                ctx->remove_fd(fd_);
                registered_ = false;
            }
            handler(ec);
        }

        void start() {
            if (cancel_slot != nullptr && cancel_slot->is_cancelled()) {
                handler(make_operation_aborted());
                return;
            }
            ssl_ptr = stream->native_handle();
            fd_ = ::SSL_get_fd(static_cast<::SSL*>(ssl_ptr));
            do_handshake();
        }

        void do_handshake() {
            if (done_.load(memory_order_acquire)) {
                return;
            }
            const int ret = ::SSL_do_handshake(static_cast<::SSL*>(ssl_ptr));
            if (ret == 1) {
                finish(error_code{});
                return;
            }

            const int err = ::SSL_get_error(static_cast<::SSL*>(ssl_ptr), ret);
            if (err == SSL_ERROR_WANT_READ) {
                auto self = shared_from_this();
                if (cancel_slot != nullptr) {
                    cancel_slot->assign([self]() mutable { self->finish(make_operation_aborted()); });
                }
                ctx->add_fd(fd_, epoll_event::in,
                            [self](int /*fd*/, epoll_event /*events*/, error_code ec) { self->on_fd_ready(ec); });
                registered_ = true;
            } else if (err == SSL_ERROR_WANT_WRITE) {
                auto self = shared_from_this();
                if (cancel_slot != nullptr) {
                    cancel_slot->assign([self]() mutable { self->finish(make_operation_aborted()); });
                }
                ctx->add_fd(fd_, epoll_event::out,
                            [self](int /*fd*/, epoll_event /*events*/, error_code ec) { self->on_fd_ready(ec); });
                registered_ = true;
            } else {
                finish(error_code(err, ssl_category()));
            }
        }

        void on_fd_ready(error_code ec) {
            lock<mutex> lk(mutex_);
            if (done_.load(memory_order_acquire)) {
                return;
            }
            if (ec) {
                finish(ec);
                return;
            }
            do_handshake();
        }
    };
} // namespace

void ssl_stream::async_handshake(io_context& ctx, function<void(error_code)> handler) {
    const auto op = make_shared<ssl_handshake_op>();
    op->ctx = &ctx;
    op->stream = this;
    op->handler = move(handler);
    op->start();
}

void ssl_stream::async_handshake(io_context& ctx, cancellation_slot& slot, function<void(error_code)> handler) {
    const auto op = make_shared<ssl_handshake_op>();
    op->ctx = &ctx;
    op->stream = this;
    op->handler = move(handler);
    op->cancel_slot = &slot;
    op->start();
}

namespace {
    struct ssl_read_op : enable_shared_from_this<ssl_read_op> {
        io_context* ctx;
        ::SSL* ssl;
        int fd{-1};
        memory_view<char> buffer;
        function<void(error_code, size_t)> handler;
        cancellation_slot* cancel_slot{nullptr};

        void start() {
            if (cancel_slot != nullptr && cancel_slot->is_cancelled()) {
                handler(make_operation_aborted(), 0);
                return;
            }
            fd = ::SSL_get_fd(ssl);
            do_read();
        }

        void do_read() {
            const int ret = ::SSL_read(ssl, buffer.data(), static_cast<int>(buffer.size()));
            if (ret > 0) {
                handler(error_code{}, static_cast<size_t>(ret));
                return;
            }
            const int err = ::SSL_get_error(ssl, ret);
            if (err == SSL_ERROR_WANT_READ) {
                auto self = shared_from_this();
                if (cancel_slot != nullptr) {
                    cancel_slot->assign([self]() mutable {
                        self->ctx->remove_fd(self->fd);
                        self->handler(make_operation_aborted(), 0);
                    });
                }
                ctx->add_fd(fd, epoll_event::in,
                            [self](int /*fd*/, epoll_event /*events*/, error_code ec) { self->on_ready(ec); });
            } else if (err == SSL_ERROR_WANT_WRITE) {
                auto self = shared_from_this();
                if (cancel_slot != nullptr) {
                    cancel_slot->assign([self]() mutable {
                        self->ctx->remove_fd(self->fd);
                        self->handler(make_operation_aborted(), 0);
                    });
                }
                ctx->add_fd(fd, epoll_event::out,
                            [self](int /*fd*/, epoll_event /*events*/, error_code ec) { self->on_ready(ec); });
            } else if (err == SSL_ERROR_ZERO_RETURN) {
                handler(error_code{make_error_code(errc::connection_reset)}, 0);
            } else {
                handler(error_code(err, ssl_category()), 0);
            }
        }

        void on_ready(error_code ec) {
            if (ec) {
                handler(ec, 0);
                return;
            }
            do_read();
        }
    };
} // namespace

void ssl_stream::async_read(io_context& ctx, memory_view<char> buffer, function<void(error_code, size_t)> handler) {
    const auto op = make_shared<ssl_read_op>();
    op->ctx = &ctx;
    op->ssl = static_cast<::SSL*>(ssl_);
    op->buffer = buffer;
    op->handler = move(handler);
    op->start();
}

void ssl_stream::async_read(io_context& ctx, memory_view<char> buffer, cancellation_slot& slot,
                            function<void(error_code, size_t)> handler) {
    const auto op = make_shared<ssl_read_op>();
    op->ctx = &ctx;
    op->ssl = static_cast<::SSL*>(ssl_);
    op->buffer = buffer;
    op->handler = move(handler);
    op->cancel_slot = &slot;
    op->start();
}

namespace {
    struct ssl_write_op : enable_shared_from_this<ssl_write_op> {
        io_context* ctx;
        ::SSL* ssl;
        int fd{-1};
        memory_view<const char> buffer;
        function<void(error_code, size_t)> handler;
        cancellation_slot* cancel_slot{nullptr};

        void start() {
            if (cancel_slot != nullptr && cancel_slot->is_cancelled()) {
                handler(make_operation_aborted(), 0);
                return;
            }
            fd = ::SSL_get_fd(ssl);
            do_write();
        }

        void do_write() {
            const int ret = ::SSL_write(ssl, buffer.data(), static_cast<int>(buffer.size()));
            if (ret > 0) {
                handler(error_code{}, static_cast<size_t>(ret));
                return;
            }
            const int err = ::SSL_get_error(ssl, ret);
            if (err == SSL_ERROR_WANT_WRITE) {
                auto self = shared_from_this();
                if (cancel_slot != nullptr) {
                    cancel_slot->assign([self]() mutable {
                        self->ctx->remove_fd(self->fd);
                        self->handler(make_operation_aborted(), 0);
                    });
                }
                ctx->add_fd(fd, epoll_event::out,
                            [self](int /*fd*/, epoll_event /*events*/, error_code ec) { self->on_ready(ec); });
            } else if (err == SSL_ERROR_WANT_READ) {
                auto self = shared_from_this();
                if (cancel_slot != nullptr) {
                    cancel_slot->assign([self]() mutable {
                        self->ctx->remove_fd(self->fd);
                        self->handler(make_operation_aborted(), 0);
                    });
                }
                ctx->add_fd(fd, epoll_event::in,
                            [self](int /*fd*/, epoll_event /*events*/, error_code ec) { self->on_ready(ec); });
            } else {
                handler(error_code(err, ssl_category()), 0);
            }
        }

        void on_ready(error_code ec) {
            if (ec) {
                handler(ec, 0);
                return;
            }
            do_write();
        }
    };
} // namespace

void ssl_stream::async_write(io_context& ctx, memory_view<const char> buffer,
                             function<void(error_code, size_t)> handler) {
    const auto op = make_shared<ssl_write_op>();
    op->ctx = &ctx;
    op->ssl = static_cast<::SSL*>(ssl_);
    op->buffer = buffer;
    op->handler = move(handler);
    op->start();
}

void ssl_stream::async_write(io_context& ctx, memory_view<const char> buffer, cancellation_slot& slot,
                             function<void(error_code, size_t)> handler) {
    const auto op = make_shared<ssl_write_op>();
    op->ctx = &ctx;
    op->ssl = static_cast<::SSL*>(ssl_);
    op->buffer = buffer;
    op->handler = move(handler);
    op->cancel_slot = &slot;
    op->start();
}

NEFORCE_END_NAMESPACE__
