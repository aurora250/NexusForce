/**
 * @example logging_basic.cpp
 * @brief 日志系统基础示例
 *
 * 演示日志系统的核心功能：
 * - 多级别日志（TRACE / DEBUG / INFO / WARN / ERROR / FATAL）
 * - 自定义格式模式
 * - 控制台彩色输出（printcln 按级别着色）
 * - 文件输出与轮转（大小轮转 + 日期轮转 + max_files 保留）
 * - 全局上下文信息
 * - 自定义过滤器
 */

#include <NeForce/core/system/console.hpp>
#include <NeForce/logging/file_sink.hpp>
#include <NeForce/logging/logger.hpp>

using namespace neforce;

int main() {
    auto log = logger_registry::instance().root_logger();

    // ========== 控制台输出（彩色） ==========
    auto sink = make_shared<console_sink>();
    sink->set_formatter(make_unique<log_formatter>("[{time}] [{level}] {file}:{line} {func}() - {message}"));
    log->add_sink(sink);

    println("=== 日志系统基础示例 ===\n");

    log->set_level(log_level::TRACE);

    // ========== 多级别日志 ==========
    printcln(color::cyan(), "--- 多级别日志 ---");
    NEFORCE_LOG_TRACE("这是 TRACE 级别的日志");
    NEFORCE_LOG_DEBUG("这是 DEBUG 级别的日志");
    NEFORCE_LOG_INFO("这是 INFO 级别的日志");
    NEFORCE_LOG_WARN("这是 WARN 级别的日志");
    NEFORCE_LOG_ERROR("这是 ERROR 级别的日志");
    NEFORCE_LOG_FATAL("这是 FATAL 级别的日志");

    // ========== 格式化日志 ==========
    printcln(color::cyan(), "\n--- 格式化日志 ---");
    NEFORCE_LOGF_INFO("用户 {} 在 {} 登录成功", "admin", "2024-01-15 10:30:00");
    NEFORCE_LOGF_WARN("连接超时: {}ms, 重试次数: {}", 3000, 3);

    // ========== 级别过滤 ==========
    printcln(color::cyan(), "\n--- 设置最低级别为 WARN ---");
    log->set_level(log_level::WARN);
    NEFORCE_LOG_INFO("这条 INFO 不会被输出");
    NEFORCE_LOG_WARN("这条 WARN 会被输出");
    NEFORCE_LOG_ERROR("这条 ERROR 也会被输出");
    log->set_level(log_level::TRACE);

    // ========== 文件输出 + 轮转 ==========
    printcln(color::cyan(), "\n--- 文件输出 ---");
    auto file = make_shared<file_sink>(path("logs/app.log"), 1024 * 1024, true, 5);
    file->set_formatter(make_unique<log_formatter>("[{time}] [{level}] {file}:{line} - {message}"));
    log->add_sink(file);
    NEFORCE_LOG_INFO("这条日志同时输出到控制台和文件");

    // ========== 全局上下文 ==========
    printcln(color::cyan(), "\n--- 全局上下文 ---");
    log->add_context("app_name", "NexusForceDemo");
    log->add_context("version", "1.0.0");
    auto ctx_fmt = make_unique<log_formatter>("[{time}] [{level}] [{context.app_name}] {message}");
    sink->set_formatter(move(ctx_fmt));
    NEFORCE_LOG_INFO("带上下文的日志输出");

    // ========== 自定义过滤器 ==========
    printcln(color::cyan(), "\n--- 过滤器：只接收 ERROR 及以上 ---");
    auto filter_console = make_shared<console_sink>();
    filter_console->set_formatter(make_unique<log_formatter>("[FILTERED] [{level}] {message}"));
    auto filter_logger = logger_registry::instance().get_logger("filter_demo");
    filter_logger->add_sink(filter_console);
    filter_logger->set_filter([](const log_event& ev) { return ev.level >= log_level::ERROR; });
    NEFORCE_LOGGER_LOG_INFO("filter_demo", "这条 INFO 被过滤了");
    NEFORCE_LOGGER_LOG_ERROR("filter_demo", "这条 ERROR 通过了过滤器");

    console.pause();
    return 0;
}
