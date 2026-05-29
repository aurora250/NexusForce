/**
 * @example logging_hierarchy.cpp
 * @brief 层级化 Logger 与条件日志示例
 *
 * 演示日志系统的层级化管理功能：
 * - 命名 Logger 层级（"app.module.sub" 点分隔）
 * - 级别继承（子 Logger 自动继承父级）
 * - 级别重载（子 Logger 覆盖父级设置）
 * - 条件日志（LOG_IF / LOG_EVERY_N / LOG_FIRST_N）
 * - 编译期级别过滤（Release 模式自动剥离 TRACE/DEBUG）
 */

#include <NeForce/core/system/console.hpp>
#include <NeForce/logging/logger.hpp>

using namespace neforce;

int main() {
    // ========== 公共控制台 sink ==========
    auto sink = make_shared<console_sink>();
    sink->set_formatter(make_unique<log_formatter>("[{time}] [{level}] {name}<{name}> {message}"));
    // 使用 filepath 显示完整路径以区分不同模块
    (void) sink;

    println("=== 层级化 Logger 示例 ===\n");

    // ========== 构建层级 ==========
    auto root = logger_registry::instance().root_logger();
    root->add_sink(sink);

    auto app = logger_registry::instance().get_logger("app");
    auto network = logger_registry::instance().get_logger("app.network");
    auto db = logger_registry::instance().get_logger("app.db");

    printcln(color::cyan(), "--- Logger 层级结构 ---");
    println("root -> app -> app.network");
    println("              -> app.db");
    printfln("app.parent() = {}", app->parent() ? app->parent()->name() : "(null)");
    printfln("app.network.parent() = {}", network->parent() ? network->parent()->name() : "(null)");

    // ========== 级别继承 ==========
    printcln(color::cyan(), "\n--- 级别继承 ---");
    root->set_level(log_level::WARN);
    printfln("root level = {}", to_string(root->effective_level()));
    printfln("app level  = {} (继承自 root)", to_string(app->effective_level()));
    printfln("app.network level = {} (继承自 root)", to_string(network->effective_level()));

    // 只有 WARN 及以上会输出
    NEFORCE_LOGGER_LOG_INFO("app", "这条 INFO 不输出");
    NEFORCE_LOGGER_LOG_WARN("app", "这条 WARN 输出");

    // ========== 级别重载 ==========
    printcln(color::cyan(), "\n--- 级别重载 ---");
    network->set_level(log_level::TRACE);
    printfln("app.network level (重载后) = {}", to_string(network->effective_level()));
    printfln("app.level (不受影响) = {}", to_string(app->effective_level()));
    NEFORCE_LOGGER_LOG_TRACE("app.network", "app.network 的 TRACE 输出");
    NEFORCE_LOGGER_LOG_TRACE("app", "app 的 TRACE 不输出（仍继承 WARN）");

    // ========== 条件日志 ==========
    printcln(color::cyan(), "\n--- 条件日志 LOG_IF ---");
    for (int i = 0; i < 5; ++i) {
        NEFORCE_LOG_WARN_IF(i % 2 == 0, format("i={} 是偶数，输出警告", i));
    }

    printcln(color::cyan(), "\n--- 频率控制 LOG_EVERY_N ---");
    root->set_level(log_level::INFO);
    for (int i = 0; i < 10; ++i) {
        NEFORCE_LOG_INFO_EVERY_N(3, format("每 3 次输出 1 次: i={}", i));
    }

    printcln(color::cyan(), "\n--- 次数限制 LOG_FIRST_N ---");
    for (int i = 0; i < 20; ++i) {
        NEFORCE_LOG_INFO_FIRST_N(2, format("只输出前 2 次: i={}", i));
    }

    // ========== 使用宏便捷获取 Logger ==========
    printcln(color::cyan(), "\n--- 便捷宏 ---");
    auto my_logger = NEFORCE_GET_LOGGER("my.feature.module");
    my_logger->set_level(log_level::INFO);
    NEFORCE_LOGGER_LOG_INFO("my.feature.module", "通过 NEFORCE_GET_LOGGER 获取并使用");
    NEFORCE_LOGGER_LOGF_INFO("my.feature.module", "格式化: pi={:.2f}", 3.14159);

    console.pause();
    return 0;
}
