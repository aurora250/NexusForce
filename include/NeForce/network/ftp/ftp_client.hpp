#ifndef NEFORCE_NETWORK_FTP_FTP_CLIENT_HPP__
#define NEFORCE_NETWORK_FTP_FTP_CLIENT_HPP__
#include "NeForce/network/dns/dns_client.hpp"
#include "NeForce/network/ftp/ftp_protocol.hpp"
#include "NeForce/network/tcp/tcp_socket.hpp"
NEFORCE_BEGIN_NAMESPACE__

class NEFORCE_API ftp_client final : public ftp_protocol {
public:
    struct entry {
        string name;
        string raw;
        uint64_t size = 0;
        bool is_directory = false;
    };

private:
    bool connected_ = false;
    tls_mode tls_mode_ = tls_mode::none;
    string sni_host_;

    transfer_mode transfer_mode_ = transfer_mode::binary;
    passive_mode passive_mode_ = passive_mode::passive;
    string server_host_;
    ports server_port_{ports::FTP};

    static constexpr int kActiveAcceptTimeoutSec = 30;

    response send_command(const string& cmd);

    void expect_code(int expected, const string& cmd);
    void expect_codes(std::initializer_list<int> codes, const string& cmd);

    tcp_socket open_data_channel();
    ssl_stream wrap_data_channel(tcp_socket sock);

    void do_ctrl_tls_handshake();
    void do_post_connect();

    vector<char> download_impl(const string& remote_path, tcp_socket& data_sock, uint64_t offset);

    void upload_impl(const string& remote_path, tcp_socket& data_sock, const char* data, size_t len, uint64_t offset);

    void open_and_connect(const ip_address& addr);

    static entry parse_list_entry(const string& line);

public:
    ftp_client() = default;

    ftp_client(ftp_client&& other) = default;
    ftp_client& operator=(ftp_client&& other) = default;

    ftp_client(const ftp_client&) = delete;
    ftp_client& operator=(const ftp_client&) = delete;

    ~ftp_client() override;

    void connect(const ip_address& addr, tls_mode mode = tls_mode::none, ssl_context* ctx = nullptr,
                 const string& sni_hostname = "");

    void connect(const string& hostname, ports port = ports::FTP, tls_mode mode = tls_mode::none,
                 dns_client* dns = nullptr, ssl_context* ctx = nullptr, const string& sni = "");

    tls_info upgrade_tls(ssl_context& ctx, const string& sni_hostname = "");

    void disconnect();

    void login(const string& username, const string& password);
    void login_anonymous();

    void set_transfer_mode(transfer_mode mode);
    void set_passive_mode(passive_mode mode) noexcept { passive_mode_ = mode; }
    void set_data_protection(bool protect);

    NEFORCE_NODISCARD string pwd();
    void cwd(const string& path);
    void cdup();
    void mkdir(const string& path);
    void rmdir(const string& path);
    NEFORCE_NODISCARD vector<entry> list(const string& path = "");
    NEFORCE_NODISCARD vector<string> nlst(const string& path = "");

    NEFORCE_NODISCARD uint64_t file_size(const string& remote_path);
    void rename(const string& from, const string& to);
    void remove(const string& remote_path);

    NEFORCE_NODISCARD vector<char> download(const string& remote_path);
    NEFORCE_NODISCARD vector<char> download_resume(const string& remote_path, uint64_t offset);

    void upload(const string& remote_path, const char* data, size_t len);
    void upload_resume(const string& remote_path, const char* data, size_t len, uint64_t offset);

    void noop();

    NEFORCE_NODISCARD bool is_connected() const noexcept { return connected_ && is_open(); }

    NEFORCE_NODISCARD bool is_tls_active() const noexcept { return tls_active_; }

    NEFORCE_NODISCARD tls_info get_tls_info() const noexcept;
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_FTP_FTP_CLIENT_HPP__
