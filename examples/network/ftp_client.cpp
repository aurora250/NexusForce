/**
 * @example ftp_client.cpp
 * @brief FTP客户端示例
 *
 * 演示 ftp_client 的核心功能：
 * - 连接到FTP服务器
 * - 用户认证（登录/匿名）
 * - 目录操作（列出/切换/创建/删除）
 * - 文件下载/上传
 * - 断点续传
 * - TLS加密（FTPS）
 * - 主动/被动模式
 */

#include <NeForce/core/system/console.hpp>
#include <NeForce/core/system/environment.hpp>
#include <NeForce/network/ftp/ftp_client.hpp>

using namespace neforce;

int main() {
    println("=== FTP Client Example ===\n");

    // 使用 environment 类读取环境变量，未设置时使用默认值
    auto ftp_host = environment::get("FTP_HOST");
    auto ftp_port_str = environment::get("FTP_PORT");
    auto ftp_user = environment::get("FTP_USER");
    auto ftp_pass = environment::get("FTP_PASS");

    // 默认值：匿名FTP
    if (ftp_host.empty()) {
        ftp_host = "ftp.gnu.org";
        println("FTP_HOST not set, using default: ftp.gnu.org");
    }
    if (ftp_user.empty()) {
        ftp_user = "anonymous";
        println("FTP_USER not set, using default: anonymous");
    }
    if (ftp_pass.empty()) {
        ftp_pass = "guest@";
        println("FTP_PASS not set, using default: guest@");
    }
    println("");

    ports ftp_port(ftp_port_str.empty() ? 21U : uinteger32::parse(ftp_port_str.view()).value());

    try {
        ftp_client ftp;

        // ========== 连接到服务器 ==========
        printfln("Connecting to {}:{}...", ftp_host, static_cast<uint16_t>(ftp_port));
        ftp.connect(ftp_host, ftp_port);
        println("Connected!");

        // ========== 登录 ==========
        printfln("Logging in as {}...", ftp_user);
        ftp.login(ftp_user, ftp_pass);
        println("Login successful!");

        // ========== 当前目录 ==========
        auto current_dir = ftp.pwd();
        printfln("Current directory: {}", current_dir);

        // ========== 列出目录内容 ==========
        println("\n=== Directory Listing ===");
        {
            auto entries = ftp.list();
            for (const auto& entry: entries) {
                printfln("  {}  {:>10}  {}", entry.is_directory ? "d" : "-", entry.size, entry.name);
            }
        }

        // ========== 切换目录 ==========
        // ftp.cwd("pub");
        // printfln("Changed to: {}", ftp.pwd());

        // ========== 下载文件 ==========
        // println("\n=== Download File ===");
        // {
        //     auto data = ftp.download("README");
        //     printfln("Downloaded {} bytes", data.size());
        //     printfln("Content preview: {}", string_view(data.data(), min(data.size(), size_t(200))));
        // }

        // ========== 上传文件 ==========
        // println("\n=== Upload File ===");
        // {
        //     string content = "Hello from NexusForce FTP Client!";
        //     ftp.upload("nexusforce_test.txt", content.data(), content.size());
        //     printfln("Uploaded {} bytes", content.size());
        // }

        // ========== 创建/删除目录 ==========
        // ftp.mkdir("nexusforce_test_dir");
        // println("Created directory");
        // ftp.rmdir("nexusforce_test_dir");
        // println("Removed directory");

        // ========== 文件大小 ==========
        // auto size = ftp.file_size("README");
        // printfln("File size: {} bytes", size);

        // ========== 重命名 ==========
        // ftp.rename("old_name.txt", "new_name.txt");

        // ========== 删除文件 ==========
        // ftp.remove("unwanted_file.txt");

        ftp.disconnect();
        println("\nDisconnected successfully");

    } catch (const exception& e) {
        printfln("FTP error: {}", e.what());
        return 1;
    }

    println("\nAll FTP examples completed");
    return 0;
}
