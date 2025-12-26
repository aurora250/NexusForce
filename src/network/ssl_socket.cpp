#include <MSTL/network/ssl_socket.hpp>
#ifdef MSTL_SUPPORT_OPENSSL__
#include <openssl/err.h>
MSTL_BEGIN_NAMESPACE__

ssl_socket::ssl_socket(tcp_socket sock)
: socket_(move(sock)) {}

ssl_socket::ssl_socket(const ssl_context& ctx, tcp_socket sock)
: socket_(move(sock)) {
    set_context(ctx);
}

bool ssl_socket::accept() {
    if (!ssl_valid()) return false;
    const int ret = ::SSL_accept(ssl_);
    if (ret <= 0) {
        last_error_ = ::ERR_error_string(::ERR_get_error(), nullptr);
        return false;
    }
    return true;
}

void ssl_socket::close() noexcept {
    if (ssl_valid()) {
        ::SSL_shutdown(ssl_);
        ::SSL_free(ssl_);
    }
}

bool ssl_socket::set_context(const ssl_context& ctx) {
    close();
    ssl_ = ::SSL_new(ctx.context());
    if (!ssl_) {
        last_error_ = ::ERR_error_string(::ERR_get_error(), nullptr);
        return false;
    }
    return ::SSL_set_fd(ssl_, socket_.sockfd()) > 0;
}

ssize_t ssl_socket::receive(void* buffer, const size_t size) const {
    if (ssl_valid()) {
        return ::SSL_read(ssl_, buffer, size);
    }
    if (socket_.is_valid()) {
        return socket_.receive(buffer, size);
    }
    return 0;
}

ssize_t ssl_socket::send(const void* buffer, const size_t size) const {
    if (ssl_valid()) {
       return ::SSL_write(ssl_, buffer, size);
    }
    if (socket_.is_valid()) {
        return socket_.send(buffer, size);
    }
    return 0;
}

MSTL_END_NAMESPACE__
#endif
