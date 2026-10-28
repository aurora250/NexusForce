/**
 * @example crash_dump.cpp
 * @brief 跨平台崩溃转储示例
 *
 * 演示崩溃转储组件的核心功能：
 * - 查询当前平台的原生转储格式与写入器支持情况
 * - 安装 terminate、致命信号（Linux）与未处理异常（Windows）处理器
 * - 通过回调取得单行崩溃摘要并写入日志
 * - 在运行中主动写出当前进程的转储
 * - 传入 --crash 参数触发一次真实的空指针解引用
 */

#include <NeForce/core/exception/crash_dump.hpp>
#include <NeForce/core/file/filesystem.hpp>
#include <NeForce/core/file/path.hpp>
#include <NeForce/core/system/console.hpp>

using namespace neforce;

namespace {

    void log_report(const crash_dump::report& report, void*) noexcept {
        printcfln(color::red(), "[crash] {}", report.message);
    }

} // namespace

int main(int argc, char** argv) {
    println("=== 崩溃转储示例 ===\n");

    printcln(color::cyan(), "--- 当前平台 ---");
    printfln("原生格式: {}", to_string(crash_dump::native_format()));
    printfln("内置写入器: {}", crash_dump::native_supported() ? "是" : "否（仅记录摘要并交回内核 core 策略）");

    const path directory = path::temp_directory_path() / path("nexusforce_crash_dump_example");
    static_cast<void>(filesystem::create_directories(directory));

    crash_dump::options options;
    options.directory = directory;
    options.file_prefix = "example";
    options.include_memory = false;
    options.callback = log_report;

    printcln(color::cyan(), "\n--- 安装处理器 ---");
    printfln("安装结果: {}", crash_dump::install_handlers(options) ? "成功" : "失败");
    printfln("处理器状态: {}", crash_dump::handlers_installed() ? "已安装" : "未安装");

    printcln(color::cyan(), "\n--- 主动写出转储 ---");
    const auto result = crash_dump::write(options);
    printfln("写出: {} 格式: {} 大小: {} 字节 内存段: {} 截断: {}", result.written ? "成功" : "失败",
             to_string(result.format), result.bytes, result.segments, result.truncated);
    if (result.written) {
        printfln("文件: {}", result.file.str());
    } else {
        printfln("错误码: {}", result.error);
    }

    crash_dump::uninstall_handlers();
    printfln("处理器状态: {}", crash_dump::handlers_installed() ? "已安装" : "未安装");

    if (argc > 1 && string_compare(argv[1], "--crash") == 0) {
        printcln(color::yellow(), "\n--- 触发致命信号（进程将立即终止并写出转储） ---");
        crash_dump::install_handlers(options);
        volatile int* pointer = nullptr;
        *pointer = 1;
    } else {
        println("\n传入 --crash 可以触发一次真实的崩溃转储");
    }
    return result.written ? 0 : 1;
}
