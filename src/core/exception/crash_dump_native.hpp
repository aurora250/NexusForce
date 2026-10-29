#ifndef NEFORCE_SRC_CORE_EXCEPTION_CRASH_DUMP_NATIVE_HPP__
#define NEFORCE_SRC_CORE_EXCEPTION_CRASH_DUMP_NATIVE_HPP__

/**
 * @file crash_dump_native.hpp
 * @brief 平台原生转储写入器的内部接口
 *
 * 接口只使用 POD 与静态缓冲区，因此可以在信号处理函数与未处理异常过滤器中调用。
 */

#include "NeForce/core/exception/errc.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_INNER__

/**
 * @brief 转储文件路径缓冲区容量
 */
NEFORCE_INLINE17 constexpr size_t native_dump_path_capacity = 2048;


/**
 * @struct native_dump_request
 * @brief 原生转储的请求参数
 *
 * 字符串指针指向调用方保证在调用期间有效的缓冲区，写入器不会保存它们。
 */
struct native_dump_request {
    const char* directory{nullptr};   ///< 输出目录，绝对路径
    const char* prefix{nullptr};      ///< 文件名前缀，已去除路径分隔符
    bool include_memory{false};       ///< 是否附带内存内容
    uint32_t max_segments{2048};      ///< 内存段数量上限
    uint64_t max_file_bytes{0};       ///< 文件总字节上限，0 表示不限制
    int32_t signal_number{0};         ///< 触发信号，非信号路径为 0
    const void* signal_info{nullptr}; ///< siginfo_t（POSIX）
    const void* context{nullptr};     ///< ucontext_t（POSIX）或 CONTEXT（Windows）
    uint32_t thread_id{0};            ///< 崩溃线程 ID
    int64_t utc_offset_seconds{0};    ///< 本地时间相对 UTC 的偏移（POSIX）
};

/**
 * @struct native_dump_result
 * @brief 原生转储的写入结果
 */
struct native_dump_result {
    bool written{false};                    ///< 是否写出可解析的转储
    bool truncated{false};                  ///< 是否因限制省略了部分内容
    int32_t error{0};                       ///< 失败时的系统错误码
    uint64_t bytes{0};                      ///< 文件字节数
    uint32_t segments{0};                   ///< 写入的内存段数量
    char file[native_dump_path_capacity]{}; ///< 转储文件路径，未写出时为空
};

/**
 * @brief 写出当前进程的原生转储
 * @param request 请求参数
 * @param result 写入结果
 * @return 是否写出可解析的转储
 */
bool write_native_dump(const native_dump_request& request, native_dump_result& result) noexcept;

NEFORCE_END_INNER__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_SRC_CORE_EXCEPTION_CRASH_DUMP_NATIVE_HPP__
