#ifndef NEFORCE_CORE_EXCEPTION_CRASH_DUMP_HPP__
#define NEFORCE_CORE_EXCEPTION_CRASH_DUMP_HPP__

/**
 * @file crash_dump.hpp
 * @brief 跨平台崩溃转储
 */

#include "NeForce/core/exception/errc.hpp"
#include "NeForce/core/file/path.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup CrashDump 崩溃转储
 * @brief 崩溃现场捕获与转储文件写入
 * @{
 */

/**
 * @struct crash_dump
 * @brief 崩溃转储静态类
 *
 * 把未捕获的 C++ 异常、致命信号（Linux）与未处理的结构化异常（Windows）
 * 统一捕获为平台原生的转储文件，并允许在任意时刻主动写出当前进程的转储：
 *
 * - Windows 写出 minidump（`.dmp`），可直接由 WinDbg / Visual Studio 打开；
 * - Linux x86-64 与 AArch64 写出 ELF core（`.core`），可直接由 gdb / lldb 打开；
 * - 其他平台仍会安装处理器，但不写出转储，只把摘要交给回调。
 *
 * 转储写入只使用系统调用与静态缓冲区，不分配堆内存，可以在信号处理函数中执行。
 * 文件名形如 `<前缀>-<YYYYMMDD>-<HHMMSS>-<pid>.<扩展名>`，同一秒内重复崩溃时追加序号以保证唯一。
 */
class NEFORCE_API crash_dump {
public:
    ~crash_dump() = delete;

    /**
     * @enum format
     * @brief 转储文件格式
     */
    enum class format : uint8_t {
        automatic, ///< 由当前平台决定
        minidump,  ///< Windows 小型转储（`.dmp`，MDMP）
        elf_core   ///< ELF core（`.core`，ET_CORE）
    };

    /**
     * @enum reason
     * @brief 触发转储的原因
     */
    enum class reason : uint8_t {
        manual,              ///< 调用 crash_dump::write() 主动写出
        fatal_signal,        ///< 致命信号，例如 SIGSEGV、SIGABRT
        unhandled_exception, ///< 未捕获的 C++ 异常（terminate）
        structured_exception ///< Windows 上未处理的结构化异常
    };

    using crash_format = crash_dump::format;
    using crash_reason = crash_dump::reason;

    /**
     * @struct result
     * @brief 一次转储写入的结果
     */
    struct result {
        bool written{false};                          ///< 是否成功写出可解析的转储
        bool truncated{false};                        ///< 是否因条数或大小限制省略了部分内容
        crash_format format{crash_format::automatic}; ///< 实际使用的格式
        path file;                                    ///< 转储文件路径，未写出时为空
        uint64_t bytes{0};                            ///< 转储文件字节数
        uint32_t segments{0};                         ///< 写入的内存段数量（minidump 恒为 0）
        int32_t error{0};                             ///< 失败时的错误码
    };

    /**
     * @struct report
     * @brief 传给 `callback` 回调的现场摘要
     * @note 只在回调执行期间有效，不可保存
     */
    struct report {
        crash_reason reason{crash_reason::manual}; ///< 触发转储的原因
        bool written{false};                       ///< 是否成功写出转储
        bool truncated{false};                     ///< 转储是否被截断
        uint32_t thread_id{0};                     ///< 触发崩溃的线程 ID
        const char* file{nullptr};                 ///< 转储文件路径，未写出时为空字符串
        const char* message{nullptr};              ///< 单行摘要，含原因、信号/异常、线程与转储信息
        uint64_t bytes{0};                         ///< 转储文件字节数
        uint32_t segments{0};                      ///< 写入的内存段数量
        int32_t error{0};                          ///< 失败时的系统错误码
        int32_t signal_number{0};                  ///< 触发信号（Linux），非信号路径为 0
        uint32_t exception_code{0};                ///< 结构化异常码（Windows），其余为 0
        const void* address{nullptr};              ///< 故障地址，未知时为空
    };

    /**
     * @brief 转储通知回调
     * @note 必须是 noexcept 函数
     * @note 回调在崩溃现场被调用，此时进程状态已不可信，不得长时间阻塞、不得分配大块内存，也不得再次触发崩溃转储
     */
    using callback = void (*)(const crash_dump::report& report, void* user_data) noexcept;

    /**
     * @struct options
     * @brief 崩溃转储配置
     */
    struct options {
        path directory;                   ///< 输出目录，为空表示当前工作目录
        string file_prefix{"nexusforce"}; ///< 文件名前缀，非字母数字、`-`、`_` 的字符会被替换为 `_`

        crash_dump::format format{crash_dump::format::automatic}; ///< 转储格式
        bool include_memory{true};                                ///< 是否附带内存内容；关闭后无法回溯栈

        uint32_t max_segments{2048};                                 ///< 内存段数量上限，超出部分截断
        uint64_t max_file_bytes{2ULL * 1024ULL * 1024ULL * 1024ULL}; ///< 转储文件总字节上限，超出部分截断

        bool install_terminate_handler{true}; ///< 是否接管 terminate
        bool install_signal_handlers{true};   ///< 是否接管致命信号（Linux）
        bool install_exception_filter{true};  ///< 是否接管未处理的结构化异常（Windows）

        bool kernel_core_fallback{true};        ///< 写转储失败时是否重置信号并交回内核 core（Linux）
        crash_dump::callback callback{nullptr}; ///< 转储完成回调
        void* user_data{nullptr};               ///< 传给回调的用户数据
    };


    /**
     * @brief 平台原生的转储格式
     * @return Windows 返回 format::minidump，其余平台返回 format::elf_core
     */
    NEFORCE_NODISCARD NEFORCE_API static crash_dump::format native_format() noexcept;

    /**
     * @brief 当前平台是否内置了转储写入器
     * @return 能否写出与原生格式对应的转储文件
     *
     * Linux 仅在 x86-64 与 AArch64 上内置 ELF core 写入器；其余架构返回 false，
     * 此时仍可以安装处理器、取得崩溃摘要并退回内核 core 策略。
     *
     * 等价于 NEFORCE_SUPPORT_NATIVE_DUMP。
     */
    NEFORCE_NODISCARD NEFORCE_API static bool native_supported() noexcept;

    /**
     * @brief 安装崩溃处理器
     * @param options 转储配置，内部会拷贝所需字段，之后修改 options 不影响已安装的处理器
     * @return 是否至少安装了一个处理器
     *
     * 安装 terminate 处理器与平台原生的崩溃处理器，崩溃时写出转储并调用回调。
     * 重复调用会以新的配置覆盖旧配置并再次返回 true。
     *
     * @note Linux 上崩溃后进程以 `128 + 信号` 退出；Windows 上未处理异常会交回系统默认处理，terminate 路径以退出码 3 结束
     * @warning 应在创建业务线程之前调用；处理器安装期间不要与崩溃并发执行
     */
    NEFORCE_API static bool install_handlers(const crash_dump::options& options) noexcept;

    /**
     * @brief 卸载崩溃处理器
     *
     * 恢复安装前的 terminate 处理器、信号动作与未处理异常过滤器。
     * 未安装时调用无副作用。
     */
    NEFORCE_API static void uninstall_handlers() noexcept;

    /**
     * @brief 查询崩溃处理器是否已安装
     * @return 已安装返回 true
     */
    NEFORCE_NODISCARD NEFORCE_API static bool handlers_installed() noexcept;

    /**
     * @brief 主动写出当前进程的转储
     * @param options 转储配置
     * @return 写入结果
     *
     * 以手动方式（reason::manual）捕获当前线程上下文并写出转储。
     *
     * @note 成功后同样会调用 options.callback，进程继续运行
     * @note 该函数可在业务代码中用于诊断快照；不要求安装处理器
     */
    NEFORCE_NODISCARD NEFORCE_API static crash_dump::result write(const crash_dump::options& options) noexcept;
};


/**
 * @brief 转储格式的文本表示
 * @param format 转储格式
 */
NEFORCE_NODISCARD NEFORCE_API string to_string(crash_dump::format format);

/**
 * @brief 崩溃原因的文本表示
 * @param reason 崩溃原因
 */
NEFORCE_NODISCARD NEFORCE_API string to_string(crash_dump::reason reason);

/** @} */ // CrashDump

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_EXCEPTION_CRASH_DUMP_HPP__
