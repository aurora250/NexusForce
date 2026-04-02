#ifndef NEFORCE_NEWORK_SOCKET_SMTP_SOCKET_HPP__
#define NEFORCE_NEWORK_SOCKET_SMTP_SOCKET_HPP__
#include "NeForce/core/container/map.hpp"
#include "NeForce/core/container/vector.hpp"
#include "NeForce/network/dns/dns_client.hpp"
#include "NeForce/network/socket/ip_socket.hpp"
#include "NeForce/network/ssl/ssl_stream.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @struct smtp_exception
 * @brief SMTP操作异常
 */
struct NEFORCE_API smtp_exception final : network_exception {
    explicit smtp_exception(const char* info = "SMTP Operation Failed.", const char* type = static_type,
                            const int code = 0) noexcept :
    network_exception(info, type, code) {}

    explicit smtp_exception(const exception& e) :
    network_exception(e) {}

    ~smtp_exception() override = default;

    static constexpr auto static_type = "smtp_exception";
};


struct NEFORCE_API smtp_message {
    string from;
    vector<string> to;
    vector<string> cc;
    vector<string> bcc;
    string subject;
    string body;
    bool is_html = false;
    map<string, string> extra_headers;
};


class NEFORCE_API smtp_socket final : public ip_socket {
public:
    enum class auth_method { none, plain, login };

    enum class tls_mode { none, implicit, starttls };

    struct response {
        int code;
        string message;

        NEFORCE_NODISCARD bool is_success() const noexcept { return code >= 200 && code < 400; }
    };

    struct starttls_result {
        bool upgraded = false;
        string cipher_name;
        string tls_version;
        bool peer_verified = false;
    };

private:
    string server_domain_;
    bool connected_ = false;

    tls_mode tls_mode_ = tls_mode::none;
    bool tls_active_ = false;

    ssl_stream ssl_;

    ssize_t raw_send(const char* data, size_t len);
    ssize_t raw_recv(char* buf, size_t len);

    bool read_line(string& out);
    response read_response();
    response send_command(const string& cmd);
    void expect_code(int expected, const string& cmd);

    vector<string> do_ehlo(const string& domain);

    void do_post_connect(const string& domain, tls_mode mode, const ssl_context* ctx, const string& sni_hostname);

    void do_tls_handshake(const ssl_context& ctx, const string& sni_hostname);

    void open_and_connect(const ip_address& addr);

public:
    smtp_socket() = default;

    explicit smtp_socket(native_handle_type fd) noexcept :
    ip_socket(fd) {}

    /**
     * @brief 连接SMTP服务器
     * @param addr         服务器地址
     * @param domain       本机EHLO域名
     * @param mode         TLS模式
     * @param ctx          SSL上下文（mode != none时必须提供）
     * @param sni_hostname SNI主机名，为空则不设置
     */
    void connect(const ip_address& addr, const string& domain = "localhost", tls_mode mode = tls_mode::none,
                 const ssl_context* ctx = nullptr, const string& sni_hostname = "");

    /**
     * @brief 通过域名连接SMTP服务器
     * @param hostname     域名，如 "smtp.qq.com"
     * @param port         端口
     * @param domain       EHLO域名
     * @param mode         TLS模式
     * @param dns          DNS客户端，为nullptr时使用默认配置
     * @param ctx          SSL上下文
     * @param sni_hostname SNI主机名，为空则使用hostname
     */
    void connect(const string& hostname, ports port, const string& domain = "localhost", tls_mode mode = tls_mode::none,
                 dns_client* dns = nullptr, const ssl_context* ctx = nullptr, const string& sni_hostname = "");

    /**
     * @brief 在已有明文连接上执行STARTTLS升级
     * @param ctx SSL上下文
     * @param sni_hostname SNI主机名
     * @return STARTTLS协商结果
     */
    starttls_result starttls(const ssl_context& ctx, const string& sni_hostname = "");

    /**
     * @brief 断开连接，发送QUIT命令
     */
    void disconnect();

    /**
     * @brief PLAIN认证
     */
    void authenticate(const string& username, const string& password, auth_method method = auth_method::plain);

    /**
     * @brief 发送邮件
     */
    void send(const smtp_message& msg);

    void noop();

    /**
     * @brief 是否已连接
     */
    NEFORCE_NODISCARD bool is_connected() const noexcept { return connected_ && is_open(); }

    NEFORCE_NODISCARD bool is_tls_active() const noexcept { return tls_active_; }

    /**
     * @brief 验证服务器证书
     */
    NEFORCE_NODISCARD bool verify_peer() const { return ssl_.verify_peer(); }

    /**
     * @brief 获取当前TLS密码套件名
     */
    NEFORCE_NODISCARD string cipher_name() const { return ssl_.get_cipher_name(); }

    /**
     * @brief 获取当前TLS版本
     */
    NEFORCE_NODISCARD string tls_version() const { return ssl_.get_version(); }
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NEWORK_SOCKET_SMTP_SOCKET_HPP__
