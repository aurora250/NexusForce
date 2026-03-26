#ifndef NEFORCE_NETWORK_SOCKET_FTP_SOCKET_HPP__
#define NEFORCE_NETWORK_SOCKET_FTP_SOCKET_HPP__
#include "NeForce/network/dns/dns_client.hpp"
#include "NeForce/network/socket/tcp_socket.hpp"
#include "NeForce/network/ssl/ssl_stream.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @struct ftp_exception
 * @extends network_exception
 * @brief FTP操作异常
 */
struct NEFORCE_API ftp_exception final : network_exception {
    explicit ftp_exception(
        const char* info = "FTP Operation Failed.",
        const char* type = static_type,
        const int code = 0) noexcept
    : network_exception(info, type, code) {}

    explicit ftp_exception(const exception& e)
    : network_exception(e) {}

    ~ftp_exception() override = default;

    static constexpr auto static_type = "ftp_exception";
};


class NEFORCE_API ftp_socket final : public socket_base {
public:
    enum class transfer_mode {
        ascii,
        binary
    };

    enum class passive_mode {
        active,
        passive
    };

    enum class tls_mode {
        none,
        implicit_,
        explicit_
    };

    struct entry {
        string name;
        string raw;
        uint64_t size = 0;
        bool is_directory = false;
    };

    struct response {
        int code;
        string message;

        NEFORCE_NODISCARD bool is_success() const noexcept {
            return code >= 200 && code < 400;
        }
        NEFORCE_NODISCARD bool is_positive_preliminary() const noexcept {
            return code >= 100 && code < 200;
        }
    };

    struct tls_info {
        bool active = false;
        string cipher_name;
        string tls_version;
        bool peer_verified = false;
        bool data_channel = false;
    };

private:
    bool connected_ = false;
    tls_mode tls_mode_ = tls_mode::none;
    bool tls_active_ = false;

    ssl_stream ctrl_ssl_;
    ssl_context* ssl_ctx_ = nullptr;
    string sni_host_;
    bool data_tls_ = false;

    transfer_mode transfer_mode_ = transfer_mode::binary;
    passive_mode passive_mode_ = passive_mode::passive;
    string server_host_;
    uint16_t server_port_ = 21;

    static constexpr int kActiveAcceptTimeoutSec = 30;

    ssize_t ctrl_send(const char* data, size_t len);
    ssize_t ctrl_recv(char* buf, size_t len);
    bool ctrl_read_line(string& out);

    response read_response();
    response send_command(const string& cmd);

    void expect_code(int expected, const string& cmd);
    void expect_codes(std::initializer_list<int> codes, const string& cmd);

    tcp_socket open_data_channel();
    ssl_stream wrap_data_channel(tcp_socket&& sock);

    void do_ctrl_tls_handshake();
    void send_pbsz_prot();

    void do_post_connect();

    vector<char> download_impl(const string& remote_path,
                               tcp_socket& data_sock,
                               uint64_t offset);

    void upload_impl(const string& remote_path,
                     tcp_socket& data_sock,
                     const char* data, size_t len,
                     uint64_t offset);

    static entry parse_list_entry(const string& line);

public:
    ftp_socket() = default;

    ftp_socket(ftp_socket&& other) = default;
    ftp_socket& operator =(ftp_socket&& other) = default;

    ftp_socket(const ftp_socket&) = delete;
    ftp_socket& operator =(const ftp_socket&) = delete;

    ~ftp_socket() override;

    void connect(const ip_address& addr, tls_mode mode = tls_mode::none,
                 ssl_context* ctx = nullptr, const string& sni_hostname = "");

    void connect(const string& hostname, uint16_t port = 21,
                 tls_mode mode = tls_mode::none, dns_client* dns = nullptr,
                 ssl_context* ctx = nullptr, const string& sni = "");

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

    NEFORCE_NODISCARD bool is_connected() const noexcept {
        return connected_ && is_open();
    }

    NEFORCE_NODISCARD bool is_tls_active() const noexcept {
        return tls_active_;
    }

    NEFORCE_NODISCARD tls_info get_tls_info() const noexcept;
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_SOCKET_FTP_SOCKET_HPP__
