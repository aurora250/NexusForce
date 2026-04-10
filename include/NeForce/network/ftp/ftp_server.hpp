#ifndef NEFORCE_NETWORK_FTP_SERVER_HPP__
#define NEFORCE_NETWORK_FTP_SERVER_HPP__
#include "NeForce/network/ftp/ftp_protocol.hpp"
#include "NeForce/network/tcp/tcp_acceptor.hpp"
NEFORCE_BEGIN_NAMESPACE__

class NEFORCE_API ftp_session final : public ftp_protocol {
public:
    explicit ftp_session(tcp_socket&& sock) : ftp_protocol(sock.release()) {}

    ftp_session(const ftp_session&) = delete;
    ftp_session& operator=(const ftp_session&) = delete;

    ftp_session(ftp_session&&) = default;
    ftp_session& operator=(ftp_session&&) = default;

    ~ftp_session() override = default;

    pair<string, string> read_command();

    void reply(int code, const string& msg) { send_response(code, msg); }

    void accept_tls(ssl_context& ctx);
};

class NEFORCE_API ftp_server {
private:
    tcp_acceptor acceptor_;

public:
    ftp_server() = default;

    ftp_server(const ftp_server&) = delete;
    ftp_server& operator=(const ftp_server&) = delete;

    ftp_server(ftp_server&&) = default;
    ftp_server& operator=(ftp_server&&) = default;

    ~ftp_server() = default;

    void listen(const ip_address& endpoint, int backlog = SOMAXCONN) { acceptor_.open(endpoint, backlog); }

    NEFORCE_NODISCARD ftp_session accept() { return ftp_session(acceptor_.accept()); }

    NEFORCE_NODISCARD optional<ftp_session> accept_nonblock() {
        auto sock = acceptor_.accept_nonblock();
        if (sock) return optional<ftp_session>{ftp_session{move(*sock)}};
        return {};
    }

    NEFORCE_NODISCARD bool is_listening() const noexcept { return acceptor_.is_open(); }

    NEFORCE_NODISCARD optional<ip_address> local_endpoint() const { return acceptor_.local_endpoint(); }

    void close() noexcept { acceptor_.close(); }
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_FTP_SERVER_HPP__
