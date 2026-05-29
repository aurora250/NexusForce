/**
 * @example logging_async.cpp
 * @brief 异步日志与溢出策略示例
 *
 * 演示日志系统的异步处理功能：
 * - 线程池异步模式（基于 thread_pool）
 * - 有界队列 + 三种溢出策略（block / discard / overrun_oldest）
 * - 自动刷新间隔
 * - 同步/异步模式切换
 * - 批量日志压力测试
 */

#include <NeForce/core/system/console.hpp>
#include <NeForce/logging/logger.hpp>
#include <NeForce/core/async/thread.hpp>

using namespace neforce;

int main() {
    println("=== 异步日志示例 ===\n");

    auto log = logger_registry::instance().root_logger();
    auto sink = make_shared<console_sink>();
    sink->set_formatter(make_unique<log_formatter>("[{time}] [{level}] {message}"));
    log->add_sink(sink);

    // ========== 同步模式基准 ==========
    printcln(color::cyan(), "--- 同步模式 ---");
    log->disable_async();
    log->set_level(log_level::INFO);

    NEFORCE_LOG_INFO("同步模式：事件立即写入 sink");
    log->flush();

    // ========== 启用异步模式（block 策略） ==========
    printcln(color::cyan(), "\n--- 异步模式（block 策略）---");
    log->enable_async(nullptr, // nullptr = 自动创建专用线程池
                      4096,    // 队列容量 4096
                      overflow_policy::block);
    printfln("异步模式已启用: is_async={}", log->is_async());

    NEFORCE_LOG_INFO("异步模式：事件进入队列，由线程池异步处理");
    log->flush();
    println("flush() 完成后，队列中事件已被处理完毕");

    // ========== 溢出策略对比 ==========
    printcln(color::cyan(), "\n--- 小容量队列 + discard 策略 ---");
    log->disable_async();
    log->enable_async(nullptr, 8, overflow_policy::discard);
    for (int i = 0; i < 50; ++i) {
        NEFORCE_LOGF_INFO("discard 测试 #{:03d}", i);
    }
    log->flush();
    println("队列容量=8，发送50条事件 → 大量被丢弃");

    printcln(color::cyan(), "\n--- 小容量队列 + overrun_oldest 策略 ---");
    log->disable_async();
    log->enable_async(nullptr, 8, overflow_policy::overrun_oldest);
    for (int i = 0; i < 50; ++i) {
        NEFORCE_LOGF_INFO("overrun 测试 #{:03d}", i);
    }
    log->flush();
    println("队列容量=8，发送50条事件 → 保留最新8条");

    // ========== 自动刷新 ==========
    printcln(color::cyan(), "\n--- 自动刷新 ---");
    log->disable_async();
    log->enable_async(nullptr, 4096, overflow_policy::block);
    log->set_auto_flush(2000); // 每 2 秒自动刷新
    println("已设置自动刷新间隔: 2000ms");

    NEFORCE_LOG_INFO("这条日志将在 2 秒内自动刷新到 sink");
    println("等待自动刷新...");
    // 实际应用中不需要手动 sleep，auto_flush 会在后续 log 调用时触发检查

    // ========== 多线程并发写入 ==========
    printcln(color::cyan(), "\n--- 多线程并发写入 ---");
    log->disable_async();
    log->enable_async(nullptr, 8192, overflow_policy::block);
    log->set_auto_flush(0);

    vector<thread> threads;
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([t] {
            for (int i = 0; i < 5; ++i) {
                NEFORCE_LOGF_INFO("线程{} 消息#{}", t, i);
                this_thread::sleep_for(10_ms);
            }
        });
    }
    for (auto& t: threads) {
        t.join();
    }
    log->flush();
    println("4 个线程各发送 5 条消息，全部异步处理完成");

    // ========== 切回同步模式排空队列 ==========
    printcln(color::cyan(), "\n--- 切回同步模式 ---");
    NEFORCE_LOG_INFO("切换前最后一条异步消息");
    log->disable_async();
    println("已切回同步模式，队列已排空");

    // 清理
    log->disable_async();
    console.pause();
    return 0;
}
