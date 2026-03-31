#ifndef NEFORCE_NETWORK_FTP_SERVER_HPP__
#define NEFORCE_NETWORK_FTP_SERVER_HPP__
#include "NeForce/network/socket/tcp_acceptor.hpp"
NEFORCE_BEGIN_NAMESPACE__

class NEFORCE_API ftp_server {
private:
    tcp_acceptor acceptor_;

public:
    ftp_server() = default;

    ftp_server(const ftp_server&) = delete;
    ftp_server& operator =(const ftp_server&) = delete;

    ftp_server(ftp_server&&) = default;
    ftp_server& operator =(ftp_server&&) = default;

    ~ftp_server() = default;

    void listen(const ip_address& endpoint, int backlog = SOMAXCONN) {
        acceptor_.open(endpoint, backlog);
    }

    NEFORCE_NODISCARD tcp_socket accept() {
        return acceptor_.accept();
    }

    NEFORCE_NODISCARD optional<tcp_socket> accept_nonblock() {
        return acceptor_.accept_nonblock();
    }

    NEFORCE_NODISCARD bool is_listening() const noexcept {
        return acceptor_.is_open();
    }

    NEFORCE_NODISCARD optional<ip_address> local_endpoint() const {
        return acceptor_.local_endpoint();
    }

    void close() noexcept {
        acceptor_.close();
    }
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_FTP_SERVER_HPP__
