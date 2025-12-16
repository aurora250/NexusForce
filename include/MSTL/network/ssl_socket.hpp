#ifndef MSTL_NETWORK_SSL_SOCKET_HPP__
#define MSTL_NETWORK_SSL_SOCKET_HPP__
#include "socket.hpp"
#ifdef MSTL_SUPPORT_OPENSSL__
#include <openssl/ssl.h>
MSTL_BEGIN_NAMESPACE__

class MSTL_API ssl_socket {
public:
    ssl_socket() noexcept = default;
    ssl_socket(::SSL_CTX* ctx, const socket& sock);
    ~ssl_socket() { close(); }

    MSTL_NODISCARD bool accept() const;
    void close() const noexcept;
    MSTL_NODISCARD bool is_valid() const noexcept { return ssl_ == nullptr; }

    ssize_t read(char* buffer, size_t size) const;
    ssize_t write(const char* buffer, size_t size) const;

private:
    ::SSL* ssl_ = nullptr;
};

MSTL_END_NAMESPACE__
#endif
#endif // MSTL_NETWORK_SSL_SOCKET_HPP__
