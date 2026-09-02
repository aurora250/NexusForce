#ifndef NEFORCE_NETWORK_SMTP_SOCKET_HPP__
#define NEFORCE_NETWORK_SMTP_SOCKET_HPP__

/**
 * @file smtp_socket.hpp
 * @brief SMTP协议Socket实现
 *
 * 此文件提供了SMTP（简单邮件传输协议）客户端的完整实现。
 * 支持明文、STARTTLS和隐式TLS连接，支持多种认证方式。
 */

#include "NeForce/core/container/map.hpp"
#include "NeForce/core/container/vector.hpp"
#include "NeForce/network/dns/dns_client.hpp"
#include "NeForce/network/ssl/ssl_stream.hpp"
#include "NeForce/network/ip_socket.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup Exceptions 异常类集
 * @{
 */

/**
 * @struct smtp_exception
 * @brief SMTP操作异常类
 *
 * SMTP协议操作失败时抛出的异常。
 */
NEFORCE_ERROR_BUILD_NETWORK_CLASS(smtp_exception, "SMTP Operation Failed.")

/** @} */ // Exceptions

/**
 * @defgroup SMTP SMTP
 * @brief SMTP协议实现
 *
 * 支持明文、STARTTLS和隐式TLS连接与多种认证方式。
 *
 * @section standards 遵循的国际标准
 * 本实现严格遵循以下电子邮件传输与安全相关标准规范：
 *
 * **SMTP 核心协议规范：**
 * - **IETF STD 10 / RFC 5321**：简单邮件传输协议（SMTP）
 *   https://www.rfc-editor.org/rfc/rfc5321.html
 * - **IETF RFC 5322**：Internet 消息格式（邮件头部与正文格式）
 *   https://www.rfc-editor.org/rfc/rfc5322.html
 *
 * **SMTP 扩展标准：**
 * - **IETF RFC 1869**：SMTP 服务扩展（ESMTP，EHLO 命令）
 *   https://www.rfc-editor.org/rfc/rfc1869.html
 * - **IETF RFC 1870**：SMTP 消息大小声明扩展（SIZE）
 *   https://www.rfc-editor.org/rfc/rfc1870.html
 * - **IETF RFC 3461**：SMTP 投递状态通知扩展（DSN）
 *   https://www.rfc-editor.org/rfc/rfc3461.html
 *
 * **SMTP 安全标准：**
 * - **IETF RFC 3207**：SMTP 服务的 TLS 扩展（STARTTLS）
 *   https://www.rfc-editor.org/rfc/rfc3207.html
 * - **IETF RFC 8314**：邮件协议的 TLS 使用建议（隐式 TLS）
 *   https://www.rfc-editor.org/rfc/rfc8314.html
 *
 * **SMTP 认证标准：**
 * - **IETF RFC 4954**：SMTP 服务认证（AUTH 扩展，PLAIN/LOGIN）
 *   https://www.rfc-editor.org/rfc/rfc4954.html
 * - **IETF RFC 4616**：PLAIN SASL 机制
 *   https://www.rfc-editor.org/rfc/rfc4616.html
 *
 * **邮件格式与 MIME 标准：**
 * - **IETF RFC 2045**：MIME 第一部分：Internet 消息体格式
 *   https://www.rfc-editor.org/rfc/rfc2045.html
 * - **IETF RFC 2046**：MIME 第二部分：媒体类型
 *   https://www.rfc-editor.org/rfc/rfc2046.html
 * - **IETF RFC 2047**：MIME 第三部分：非 ASCII 文本头扩展
 *   https://www.rfc-editor.org/rfc/rfc2047.html
 *
 * @section smtp_session SMTP 会话流程
 * 根据 RFC 5321 §3.1，SMTP 会话的基本流程：
 *
 * | 步骤 | 客户端命令     | 服务器响应码 | 说明                           |
 * |------|----------------|--------------|--------------------------------|
 * | 1    | -              | 220          | 服务器就绪                     |
 * | 2    | EHLO domain    | 250          | 扩展问候（ESMTP）              |
 * | 3    | STARTTLS       | 220          | 升级到 TLS（可选）             |
 * | 4    | AUTH LOGIN     | 334/235      | 认证（可选）                   |
 * | 5    | MAIL FROM:     | 250          | 设置发件人                     |
 * | 6    | RCPT TO:       | 250          | 设置收件人（可多次）           |
 * | 7    | DATA           | 354          | 开始发送邮件正文               |
 * | 8    | .              | 250          | 邮件正文结束                   |
 * | 9    | QUIT           | 221          | 断开连接                       |
 *
 * @section smtp_response_codes SMTP 响应码分类
 * 根据 RFC 5321 §4.2，SMTP 响应码按百位数字分类：
 *
 * | 类别 | 响应码范围 | 含义               | 典型响应码                   |
 * |------|------------|--------------------|------------------------------|
 * | 2xx  | 200 – 299  | 命令成功           | 220 (就绪), 250 (OK), 235 (认证成功) |
 * | 3xx  | 300 – 399  | 命令待处理         | 334 (等待认证凭据), 354 (开始数据) |
 * | 4xx  | 400 – 499  | 临时失败（可重试） | 450 (邮箱不可用)             |
 * | 5xx  | 500 – 599  | 永久失败           | 550 (邮箱不存在), 554 (事务失败) |
 *
 * @section tls_modes TLS 连接模式
 * 根据 RFC 3207 和 RFC 8314，SMTP 支持三种 TLS 模式：
 *
 * | 模式       | TLS 时机         | 端口    | 安全性                           |
 * |------------|------------------|---------|----------------------------------|
 * | none       | 不加密           | 25      | 最低（明文传输）                 |
 * | starttls   | 先明文，后升级   | 587     | 中等（存在降级攻击风险）         |
 * | implicit   | 连接即 TLS       | 465     | 最高（始终加密）                 |
 *
 * @section auth_methods SMTP 认证方式
 * 根据 RFC 4954，本实现支持以下认证方式：
 *
 * | 方式  | RFC 引用 | 说明                                           |
 * |-------|----------|------------------------------------------------|
 * | none  | -        | 无认证                                         |
 * | plain | RFC 4616 | 用户名和密码 Base64 编码传输（需 TLS）         |
 * | login | RFC 4954 | 用户名和密码分两步 Base64 编码传输（需 TLS）   |
 *
 * @section email_format 邮件格式说明
 * 根据 RFC 5322 和 RFC 2045，邮件格式规范：
 *
 * **邮件头字段**：
 * | 字段         | RFC 引用  | 说明                   | 是否必需 |
 * |--------------|-----------|------------------------|----------|
 * | From         | §3.4      | 发件人地址             | 是       |
 * | To           | §3.4      | 收件人地址             | 是       |
 * | Cc           | §3.4      | 抄送地址               | 否       |
 * | Bcc          | §3.4      | 密送地址（不显示）     | 否       |
 * | Subject      | §3.6.5    | 邮件主题               | 否       |
 * | Date         | §3.6.1    | 发送日期               | 否（自动生成） |
 * | Content-Type | RFC 2045  | 内容类型（text/plain 或 text/html） | 是 |
 * | MIME-Version | RFC 2045  | MIME 版本（固定 1.0）  | 是       |
 *
 * **正文编码规则**：
 * - 以 `\r\n.\r\n` 作为邮件正文结束标记
 * - 正文中以 `.` 开头的行需要额外转义一个 `.`
 * - Bcc 收件人不在邮件头中显示，但同样通过 RCPT TO 发送
 *
 * @section usage_examples 使用示例
 * 基本 SMTP 连接与邮件发送：
 * ```cpp
 * smtp_socket smtp;
 *
 * // 连接到 SMTP 服务器（IP 地址）
 * auto addr = ip_address::parse("192.168.1.100", ports::smtp);
 * smtp.connect(*addr, "example.com", smtp_socket::tls_mode::none);
 *
 * // 构建邮件
 * smtp_message msg;
 * msg.from = "sender@example.com";
 * msg.to = {"recipient@example.com"};
 * msg.subject = "Test Email";
 * msg.body = "Hello, this is a test email!";
 *
 * // 发送邮件
 * smtp.send(msg);
 * smtp.disconnect();
 * ```
 *
 * 使用 STARTTLS 和认证：
 * ```cpp
 * ssl_context ctx(ssl_method::TLS_CLIENT);
 * ctx.load_verify_locations("ca-bundle.crt");
 *
 * smtp_socket smtp;
 * smtp.connect("smtp.qq.com", 587, "example.com",
 *              smtp_socket::tls_mode::starttls, nullptr, &ctx);
 *
 * smtp.authenticate("username@qq.com", "password",
 *                   smtp_socket::auth_method::login);
 *
 * smtp_message msg;
 * msg.from = "sender@qq.com";
 * msg.to = {"recipient@example.com"};
 * msg.subject = "Encrypted Email";
 * msg.body = "<h1>Hello</h1><p>This is HTML email.</p>";
 * msg.is_html = true;
 *
 * smtp.send(msg);
 * smtp.disconnect();
 * ```
 *
 * @note SMTP 协议使用 TCP 端口 25（明文）、587（STARTTLS）或 465（隐式 TLS）。
 *       现代邮件服务推荐使用 STARTTLS（端口 587）或隐式 TLS（端口 465）。
 *
 * @warning 明文 SMTP 传输的所有数据都是可见的，
 *          仅应在受控网络环境中使用。生产环境强烈建议使用 STARTTLS 或隐式 TLS。
 *          PLAIN/LOGIN 认证方式在无 TLS 保护时等同于明文密码传输。
 *
 * @see https://www.rfc-editor.org/rfc/rfc5321.html
 * @see https://www.rfc-editor.org/rfc/rfc3207.html
 * @see https://www.rfc-editor.org/rfc/rfc4954.html
 * @see https://en.wikipedia.org/wiki/Simple_Mail_Transfer_Protocol
 * @{
 */

/**
 * @struct smtp_message
 * @brief SMTP邮件消息结构
 *
 * 表示一封完整的邮件，包含发件人、收件人、主题、正文等信息。
 */
struct NEFORCE_API smtp_message {
    string from;                       ///< 发件人地址
    vector<string> to;                 ///< 收件人地址列表
    vector<string> cc;                 ///< 抄送地址列表
    vector<string> bcc;                ///< 密送地址列表
    string subject;                    ///< 邮件主题
    string body;                       ///< 邮件正文
    bool is_html = false;              ///< 是否为HTML格式
    map<string, string> extra_headers; ///< 额外邮件头
};

/**
 * @class smtp_socket
 * @brief SMTP Socket类
 *
 * 实现SMTP客户端协议，支持完整的邮件发送流程。
 * 支持多种连接方式和认证机制。
 *
 * 主要功能：
 * - SMTP服务器连接（支持IP和域名）
 * - TLS/SSL加密（隐式TLS、STARTTLS）
 * - 认证（PLAIN、LOGIN）
 * - 邮件发送（支持收件人、抄送、密送、HTML邮件）
 * - EHLO/HELO协议协商
 * - DNS解析
 */
class NEFORCE_API smtp_socket final : public ip_socket {
public:
    /**
     * @enum auth_method
     * @brief SMTP认证方式
     */
    enum class auth_method {
        none,  ///< 无认证
        plain, ///< PLAIN认证
        login  ///< LOGIN认证
    };

    /**
     * @enum tls_mode
     * @brief TLS连接模式
     */
    enum class tls_mode {
        none,     ///< 明文连接
        implicit, ///< 隐式TLS（直接TLS）
        starttls  ///< STARTTLS（从明文升级到TLS）
    };

    /**
     * @struct response
     * @brief SMTP服务器响应
     */
    struct response {
        int code;       ///< 响应码
        string message; ///< 响应消息文本

        /**
         * @brief 检查响应是否成功
         * @return 2xx响应返回true
         */
        NEFORCE_NODISCARD bool is_success() const noexcept { return code >= 200 && code < 400; }
    };

    /**
     * @struct starttls_result
     * @brief STARTTLS升级结果
     */
    struct starttls_result {
        bool upgraded = false;      ///< 是否成功升级
        string cipher_name;         ///< 当前密码套件名称
        string tls_version;         ///< TLS协议版本
        bool peer_verified = false; ///< 对等方证书是否已验证
    };

private:
    string server_domain_;   ///< 服务器域名
    bool connected_ = false; ///< 是否已连接

    tls_mode tls_mode_ = tls_mode::none; ///< TLS模式
    bool tls_active_ = false;            ///< TLS是否已激活

    ssl_stream ssl_;           ///< SSL流对象
    io_context* ctx_{nullptr}; ///< 异步 I/O 执行上下文

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
    /**
     * @brief 默认构造函数
     */
    smtp_socket() = default;

    /**
     * @brief 从原生句柄构造
     * @param fd 原生socket句柄
     */
    explicit smtp_socket(native_handle_type fd) noexcept :
    ip_socket(fd) {}

    /**
     * @brief 连接SMTP服务器（IP）
     * @param addr 服务器IP地址
     * @param domain 本机EHLO域名
     * @param mode TLS模式
     * @param ctx SSL上下文（mode != none时必须提供）
     * @param sni_hostname SNI主机名（为空则不设置）
     */
    void connect(const ip_address& addr, const string& domain = "localhost", tls_mode mode = tls_mode::none,
                 const ssl_context* ctx = nullptr, const string& sni_hostname = "");

    /**
     * @brief 连接SMTP服务器（域名）
     * @param hostname 服务器域名
     * @param port 端口号
     * @param domain EHLO域名
     * @param mode TLS模式
     * @param dns DNS客户端（为nullptr时使用默认配置）
     * @param ctx SSL上下文
     * @param sni_hostname SNI主机名（为空则使用hostname）
     */
    void connect(const string& hostname, ports port, const string& domain = "localhost", tls_mode mode = tls_mode::none,
                 dns_client* dns = nullptr, const ssl_context* ctx = nullptr, const string& sni_hostname = "");

    /**
     * @brief 执行STARTTLS升级
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
     * @brief SMTP认证
     * @param username 用户名
     * @param password 密码
     * @param method 认证方式
     */
    void authenticate(const string& username, const string& password, auth_method method = auth_method::plain);

    /**
     * @brief 发送邮件
     * @param msg 邮件消息
     */
    void send(const smtp_message& msg);

    /**
     * @brief 发送NOOP命令
     */
    void noop();

    /**
     * @brief 是否已连接
     * @return 已连接返回true
     */
    NEFORCE_NODISCARD bool is_connected() const noexcept { return connected_ && is_open(); }

    /**
     * @brief TLS是否已激活
     * @return 已激活返回true
     */
    NEFORCE_NODISCARD bool is_tls_active() const noexcept { return tls_active_; }

    /**
     * @brief 验证服务器证书
     * @return 验证通过返回true
     */
    NEFORCE_NODISCARD bool verify_peer() const { return ssl_.verify_peer(); }

    /**
     * @brief 获取当前TLS密码套件名
     * @return 密码套件名称
     */
    NEFORCE_NODISCARD string cipher_name() const { return ssl_.get_cipher_name(); }

    /**
     * @brief 获取当前TLS版本
     * @return TLS版本字符串
     */
    NEFORCE_NODISCARD string tls_version() const { return ssl_.get_version(); }
};

/** @} */ // SMTP

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_SMTP_SOCKET_HPP__
