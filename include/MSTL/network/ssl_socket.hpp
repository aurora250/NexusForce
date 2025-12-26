#ifndef MSTL_NETWORK_SSL_SOCKET_HPP__
#define MSTL_NETWORK_SSL_SOCKET_HPP__
#include "tcp_socket.hpp"
#ifdef MSTL_SUPPORT_OPENSSL__
#include "MSTL/core/string/string.hpp"
#include "ssl_context.hpp"
#include <openssl/ssl.h>
MSTL_BEGIN_NAMESPACE__

class MSTL_API ssl_socket {
private:
    ::SSL* ssl_ = nullptr;
    tcp_socket socket_{};

    string last_error_{};
    
public:
    ssl_socket() noexcept = default;
    ~ssl_socket() { close(); }

    explicit ssl_socket(tcp_socket sock);
    explicit ssl_socket(const ssl_context& ctx, tcp_socket sock);

    ssl_socket(const ssl_socket&) = delete;
    ssl_socket& operator =(const ssl_socket&) = delete;
    ssl_socket(ssl_socket&& other) noexcept = default;
    ssl_socket& operator =(ssl_socket&& other) noexcept = default;
    
    MSTL_NODISCARD bool accept();
    void close() noexcept;
    
    MSTL_NODISCARD bool ssl_valid() const noexcept {
        return ssl_ != nullptr;
    }
    MSTL_NODISCARD bool tcp_valid() const noexcept {
        return socket_.is_valid();
    }
    MSTL_NODISCARD bool is_valid() const noexcept {
        return ssl_ != nullptr || socket_.is_valid();
    }

    bool set_context(const ssl_context& ctx);

    ssize_t receive(void* buffer, size_t size) const;
    ssize_t send(const void* buffer, size_t size) const;
    
    MSTL_NODISCARD const tcp_socket& socket() const noexcept { return socket_; }
    MSTL_NODISCARD tcp_socket& socket() noexcept { return socket_; }
    MSTL_NODISCARD tcp_socket::socket_t sockfd() const noexcept { return socket_.sockfd(); }

    MSTL_NODISCARD const string& last_error() const noexcept { return last_error_; }

    MSTL_NODISCARD bool operator ==(const ssl_socket& other) const noexcept {
        return socket_ == other.socket_ && ssl_ == other.ssl_;
    }
    MSTL_NODISCARD bool operator !=(const ssl_socket& other) const noexcept {
        return socket_ != other.socket_ || ssl_ != other.ssl_;
    }
};

MSTL_END_NAMESPACE__
#endif
#endif // MSTL_NETWORK_SSL_SOCKET_HPP__
