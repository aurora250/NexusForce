/**
 * @example logging_context.cpp
 * @brief 上下文、MDC 与 Syslog 示例
 *
 * 演示日志系统的高级上下文功能：
 * - Logger 上下文（COW 零拷贝共享）
 * - MDC 线程局部上下文（请求追踪）
 * - 三层上下文合并（全局 + Logger + MDC）
 * - Syslog 输出
 * - 上下文替换与清除
 */

#include <NeForce/core/system/console.hpp>
#include <NeForce/logging/file_sink.hpp>
#include <NeForce/logging/logger.hpp>
#include <NeForce/core/async/thread.hpp>

using namespace neforce;

void handle_request(const string& request_id, const string& user) {
    mdc::put("request_id", request_id);
    mdc::put("user", user);

    NEFORCE_LOG_INFO("开始处理请求");
    NEFORCE_LOG_INFO("查询数据库...");
    NEFORCE_LOG_INFO("请求处理完成");

    mdc::clear();
}

int main() {
    println("=== 上下文与 MDC 示例 ===\n");

    auto log = logger_registry::instance().root_logger();
    log->disable_async();

    // ========== 基础输出 ==========
    auto sink = make_shared<console_sink>();
    sink->set_formatter(make_unique<log_formatter>("[{time}] [{level}] "
                                                   "[req={context.request_id}] "
                                                   "[user={context.user}] "
                                                   "{message}"));
    log->add_sink(sink);

    // ========== Logger 级别上下文（COW） ==========
    printcln(color::cyan(), "--- Logger 上下文 ---");
    log->add_context("service", "api-gateway");
    log->add_context("environment", "production");
    log->set_level(log_level::INFO);
    NEFORCE_LOG_INFO("Logger 上下文已设置");

    // 修改上下文（COW：只在修改时才复制）
    log->add_context("version", "2.0.0");
    NEFORCE_LOG_INFO("上下文在修改时才执行 Copy-On-Write");

    log->remove_context("environment");
    NEFORCE_LOG_INFO("移除 environment 上下文后");

    // ========== MDC 线程局部上下文 ==========
    printcln(color::cyan(), "\n--- MDC 线程局部上下文 ---");

    thread worker1([&] { handle_request("REQ-001", "alice"); });
    thread worker2([&] { handle_request("REQ-002", "bob"); });
    worker1.join();
    worker2.join();

    printcln(color::cyan(), "\n两条请求的 request_id 和 user 自动附加到对应日志中");
    log->flush();

    // ========== 子 Logger 独立上下文 ==========
    printcln(color::cyan(), "\n--- 子 Logger 独立上下文 ---");
    auto db_logger = logger_registry::instance().get_logger("database");
    db_logger->add_sink(sink);
    db_logger->add_context("module", "postgres");
    db_logger->add_context("pool_size", "10");
    NEFORCE_LOGGER_LOG_INFO("database", "数据库模块日志（独立上下文）");

    // ========== Syslog 输出 ==========
    printcln(color::cyan(), "\n--- Syslog 输出 ---");
#ifdef NEFORCE_PLATFORM_LINUX
    auto syslog = make_shared<syslog_sink>("NexusForceApp", syslog_facility::LOG_USER);
    syslog->set_formatter(make_unique<log_formatter>("[{level}] {message}"));
    log->add_sink(syslog);

    NEFORCE_LOG_WARN("这条警告同时写入 syslog");
    println("可以通过 journalctl -f 或 tail -f /var/log/syslog 查看");
#endif

    // ========== 清除上下文 ==========
    printcln(color::cyan(), "\n--- 清除上下文 ---");
    log->clear_context();
    NEFORCE_LOG_INFO("所有 Logger 上下文已清除");

    console.pause();
    return 0;
}
