#include <MSTL/network/ssl_socket.hpp>
#ifdef MSTL_SUPPORT_OPENSSL__
#include <MSTL/core/system/console.hpp>
#include <openssl/err.h>
MSTL_BEGIN_NAMESPACE__

ssl_socket::ssl_socket(SSL_CTX* ctx, const tcp_socket& sock) {
    ssl_ = ::SSL_new(ctx);
    if (!ssl_) {
        printcln(color::red(), "SSL_new failed: ", ::ERR_error_string(::ERR_get_error(), nullptr));
        return;
    }
    ::SSL_set_fd(ssl_, sock.sockfd());
}

bool ssl_socket::accept() const {
    const int ret = ::SSL_accept(ssl_);
    if (ret <= 0) {
        printcln(color::red(), "SSL_accept failed: ", ::ERR_error_string(::ERR_get_error(), nullptr));
        return false;
    }
    return true;
}

void ssl_socket::close() const noexcept {
    if (ssl_) {
        ::SSL_shutdown(ssl_);
        ::SSL_free(ssl_);
    }
}

ssize_t ssl_socket::read(char* buffer, const size_t size) const {
    return ::SSL_read(ssl_, buffer, size);
}

ssize_t ssl_socket::write(const char* buffer, const size_t size) const {
    return ::SSL_write(ssl_, buffer, size);
}

MSTL_END_NAMESPACE__
#endif
