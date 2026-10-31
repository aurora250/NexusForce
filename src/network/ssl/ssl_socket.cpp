#include <NeForce/network/ssl/ssl_socket.hpp>
#include <openssl/ssl.h>
#include <openssl/x509.h>
NEFORCE_BEGIN_NAMESPACE__

bool ssl_socket::close() noexcept {
    if (ssl_.has_value()) {
        ssl_->close();
        ssl_.reset();
    }
    return tcp_socket::close();
}

void ssl_socket::prepare_server_ssl(const ssl_context& ctx) {
    if (!is_open()) {
        NEFORCE_THROW_EXCEPTION(value_exception("Socket is not open"));
    }
    if (!ctx.is_valid()) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("Invalid SSL context"));
    }

    ssl_.emplace(ctx);
    ssl_->set_fd(native_handle());
    ::SSL_set_accept_state(static_cast<::SSL*>(ssl_->native_handle()));
}

void ssl_socket::prepare_client_ssl(const ssl_context& ctx, const string& hostname) {
    if (!is_open()) {
        NEFORCE_THROW_EXCEPTION(value_exception("Socket is not open"));
    }
    if (!ctx.is_valid()) {
        NEFORCE_THROW_EXCEPTION(ssl_exception("Invalid SSL context"));
    }

    ssl_.emplace(ctx);
    ssl_->set_fd(native_handle());
    if (!hostname.empty()) {
        ssl_->set_sni_hostname(hostname);
    }
    ::SSL_set_connect_state(static_cast<::SSL*>(ssl_->native_handle()));
}

void ssl_socket::init_server_ssl(const ssl_context& ctx) {
    try {
        prepare_server_ssl(ctx);
        ssl_->accept();
    } catch (...) {
        ssl_.reset();
        throw;
    }
}

void ssl_socket::init_client_ssl(const ssl_context& ctx, const string& hostname, void* session) {
    try {
        prepare_client_ssl(ctx, hostname);
        ssl_->import_session(session);
        ssl_->connect();
    } catch (...) {
        ssl_.reset();
        throw;
    }
}

void ssl_socket::async_handshake(io_context& ctx, function<void(error_code)> handler) {
    if (!ssl_ || !ssl_->is_valid()) {
        handler(error_code(static_cast<int>(errc::invalid_argument), error_category::system()));
        return;
    }
    ssl_->async_handshake(ctx, move(handler));
}

void ssl_socket::async_handshake(io_context& ctx, cancellation_slot& slot, function<void(error_code)> handler) {
    if (!ssl_ || !ssl_->is_valid()) {
        handler(error_code(static_cast<int>(errc::invalid_argument), error_category::system()));
        return;
    }
    ssl_->async_handshake(ctx, slot, move(handler));
}

string ssl_socket::peer_certificate_info() const {
    if (!ssl_ || !ssl_->is_valid()) {
        return "";
    }

    const auto cert = ssl_->peer_certificate();
    if (!cert.is_valid()) {
        return "";
    }

    string result;

    const string subject = cert.subject_name();
    const string issuer = cert.issuer_name();

    if (!subject.empty()) {
        result = "Subject: " + move(subject) + "\n";
    }
    if (!issuer.empty()) {
        result += "Issuer: " + move(issuer);
    }

    return move(result);
}

string ssl_socket::get_alpn_negotiated() const {
    if (!ssl_ || !ssl_->is_valid()) {
        return "";
    }
    return ssl_->alpn_negotiated();
}

void ssl_socket::drain_session_tickets() {
    if (!ssl_ || !ssl_->is_valid()) {
        return;
    }
    char discard = 0;
    const bool was_nonblocking = is_nonblocking();
    set_nonblocking(true);
    static_cast<void>(ssl_->read(&discard, 1));
    set_nonblocking(was_nonblocking);
}

ssize_t ssl_socket::send(memory_view<const char> data, const int flags) {
    if (ssl_ && ssl_->is_valid()) {
        if (data.empty()) {
            return 0;
        }
        return ssl_->write(data.data(), data.size());
    }
    return tcp_socket::send(data, flags);
}

ssize_t ssl_socket::receive(memory_view<char> buffer, const int flags) {
    if (ssl_ && ssl_->is_valid()) {
        if (buffer.empty()) {
            return 0;
        }
        return ssl_->read(buffer.data(), buffer.size());
    }
    return tcp_socket::receive(buffer, flags);
}

void ssl_socket::async_read(io_context& ctx, memory_view<char> buffer, function<void(error_code, size_t)> handler) {
    if (ssl_ && ssl_->is_valid()) {
        ssl_->async_read(ctx, buffer, move(handler));
        return;
    }
    tcp_socket::async_read(ctx, buffer, move(handler));
}

void ssl_socket::async_read(io_context& ctx, memory_view<char> buffer, cancellation_slot& slot,
                            function<void(error_code, size_t)> handler) {
    if (ssl_ && ssl_->is_valid()) {
        ssl_->async_read(ctx, buffer, slot, move(handler));
        return;
    }
    tcp_socket::async_read(ctx, buffer, slot, move(handler));
}

void ssl_socket::async_write(io_context& ctx, memory_view<const char> buffer,
                             function<void(error_code, size_t)> handler) {
    if (ssl_ && ssl_->is_valid()) {
        ssl_->async_write(ctx, buffer, move(handler));
        return;
    }
    tcp_socket::async_write(ctx, buffer, move(handler));
}

void ssl_socket::async_write(io_context& ctx, memory_view<const char> buffer, cancellation_slot& slot,
                             function<void(error_code, size_t)> handler) {
    if (ssl_ && ssl_->is_valid()) {
        ssl_->async_write(ctx, buffer, slot, move(handler));
        return;
    }
    tcp_socket::async_write(ctx, buffer, slot, move(handler));
}

NEFORCE_END_NAMESPACE__
