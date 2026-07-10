/**
 * @example smtp_mail.cpp
 * @brief SMTP邮件发送示例
 *
 * 演示 smtp_socket 的核心功能：
 * - 连接到SMTP服务器
 * - TLS加密（STARTTLS / 隐式TLS）
 * - 用户认证（PLAIN / LOGIN）
 * - 发送纯文本/HTML邮件
 * - 抄送/密送支持
 * - 自定义邮件头
 */

#include <NeForce/core/file/file.hpp>
#include <NeForce/core/system/console.hpp>
#include <NeForce/network/smtp_socket.hpp>
#include <NeForce/network/ssl/ssl_context.hpp>

using namespace neforce;

int main() {
    println("=== SMTP Client Example ===\n");

    // 读取QQ邮箱授权码
    path auth_path = path::current_executable_path() / "../../../../tests/resource/authcode";
    file auth_file(auth_path.lexically_normal());
    println(auth_file.file_path());
    string auth_code = auth_file.read();
    printfln("Authorization code loaded ({} chars)", auth_code.size());

    // QQ邮箱SMTP配置
    const string smtp_host = "smtp.qq.com";
    const ports smtp_port(587u);
    const string smtp_user = "1737900250@qq.com";
    const string smtp_from = "1737900250@qq.com";
    const string smtp_to = "1737900250@qq.com"; // 发给自己

    try {
        smtp_socket smtp;
        io_context ctx;
        dns_client resolver{dns_client::config{}, ctx, true};

        // 连接到SMTP服务器（使用STARTTLS）
        printfln("Connecting to {}:{}...", smtp_host, static_cast<uint16_t>(smtp_port));
        smtp.connect(smtp_host, smtp_port, "nexusforce.local", smtp_socket::tls_mode::starttls, &resolver);

        // STARTTLS 加密
        println("Upgrading to TLS...");
        ssl_context ssl(ssl_method::TLS_CLIENT);
        smtp.starttls(ssl, smtp_host);

        // 认证
        println("Authenticating...");
        smtp.authenticate(smtp_user, auth_code, smtp_socket::auth_method::login);

        // 构建邮件
        smtp_message msg;
        msg.from = smtp_from;
        msg.to = {smtp_to};
        msg.subject = "Hello from NexusForce SMTP Client";
        msg.body = "<h1>Hello!</h1>"
                   "<p>This email was sent using <b>NexusForce</b> SMTP client.</p>"
                   "<p>Features demonstrated:</p>"
                   "<ul>"
                   "<li>STARTTLS encryption</li>"
                   "<li>LOGIN authentication</li>"
                   "<li>HTML email format</li>"
                   "</ul>";
        msg.is_html = true;

        // 发送
        println("Sending email...");
        smtp.send(msg);
        println("Email sent successfully!");

        smtp.disconnect();
    } catch (const ssl_exception& e) {
        eprintfln("SSL error: {} ({})", e.what(), e.code());
        return 1;
    } catch (const smtp_exception& e) {
        eprintfln("SMTP error: {} ({})", e.what(), e.code().value());
        return 1;
    } catch (const exception& e) {
        eprintfln("error: {}", e.what());
        return 1;
    }

    return 0;
}
