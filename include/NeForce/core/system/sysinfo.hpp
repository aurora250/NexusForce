#ifndef NEFORCE_CORE_SYSTEM_SYSINFO_HPP__
#define NEFORCE_CORE_SYSTEM_SYSINFO_HPP__

/**
 * @file sysinfo.hpp
 * @brief 系统信息查询工具
 *
 * 此文件提供了获取系统硬件和软件信息的工具类。
 * 支持跨平台获取CPU、内存、操作系统版本等信息。
 */

#include "NeForce/core/async/atomic.hpp"
#include "NeForce/core/string/string.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup SystemInfo 系统信息
 * @brief 系统硬件和软件信息查询
 * @{
 */

/**
 * @class sysinfo
 * @brief 系统信息单例类
 *
 * 提供系统信息的查询功能，包括：
 * - CPU信息（型号、频率、核心数）
 * - 内存信息（物理内存、虚拟内存、使用率）
 * - 操作系统版本
 * - 系统架构
 * - 系统页面大小、分配粒度等底层信息
 *
 * 使用单例模式，确保全局只有一个系统信息实例。
 */
class NEFORCE_API sysinfo {
public:
    /**
     * @struct system_info
     * @brief 系统底层信息
     *
     * 包含系统硬件和内存管理相关的底层信息。
     */
    struct system_info {
        uint32_t processor_numbers{0};      ///< 处理器数量
        uint32_t page_size{0};              ///< 内存页大小
        uint32_t allocation_granularity{0}; ///< 内存分配粒度
        uintptr_t min_app_address{0};       ///< 应用程序最小地址
        uintptr_t max_app_address{0};       ///< 应用程序最大地址
        uintptr_t active_processor_mask{0}; ///< 活动处理器掩码
        uint16_t processor_level{0};        ///< 处理器级别
        uint16_t processor_revision{0};     ///< 处理器修订版本
    };

    /**
     * @struct memory_info
     * @brief 内存信息
     *
     * 包含物理内存和虚拟内存的使用情况。
     */
    struct NEFORCE_API memory_info {
        uint64_t total_physical{0};     ///< 物理内存总量
        uint64_t available_physical{0}; ///< 可用物理内存
        uint64_t total_virtual{0};      ///< 虚拟内存总量
        uint64_t available_virtual{0};  ///< 可用虚拟内存
        uint64_t total_page_file{0};    ///< 页面文件总量
        uint64_t available_page_file{0}; ///< 可用页面文件

        /**
         * @brief 计算物理内存使用率
         * @return 内存使用百分比（0.0-100.0）
         */
        NEFORCE_NODISCARD float64_t physical_memory_usage() const noexcept {
            if (total_physical == 0) return 0.0;
            return 100.0 * (1.0 - static_cast<float64_t>(available_physical) / total_physical);
        }

        /**
         * @brief 获取总可用内存
         * @return 可用内存总量（物理+虚拟）
         */
        NEFORCE_NODISCARD size_t available_memory() const noexcept;
    };

    /**
     * @struct CPU_info
     * @brief CPU信息
     *
     * 包含处理器的型号、频率、核心数等信息。
     */
    struct CPU_info {
        string vendor{};          ///< 厂商名称
        string brand{};           ///< 型号名称
        uint32_t max_MHz{0};      ///< 最大频率
        uint32_t current_MHZ{0};  ///< 当前频率
        uint32_t cores{0};        ///< 物理核心数
        uint32_t logical_processors{0}; ///< 逻辑处理器数
        string features{};        ///< 支持的指令集特性

        /**
         * @brief 检查是否支持超线程
         * @return 如果逻辑处理器数大于物理核心数返回true
         */
        NEFORCE_NODISCARD bool hyperthreading() const noexcept {
            return logical_processors > cores;
        }
    };

    /**
     * @struct os_version_info
     * @brief 操作系统版本信息
     */
    struct NEFORCE_API os_version_info {
        uint32_t major{0};      ///< 主版本号
        uint32_t minor{0};      ///< 次版本号
        uint32_t build{0};      ///< 构建号
        uint32_t platform_id{0}; ///< 平台ID
        string csd_version{};   ///< 补丁版本
        string product_name{};  ///< 产品名称

        /**
         * @brief 获取版本字符串
         * @return 格式为"major.minor.build"的版本字符串
         */
        NEFORCE_NODISCARD string version() const;
    };

    /**
     * @enum architecture
     * @brief 系统架构枚举
     */
    enum class architecture {
        UNKNOWN, ///< 未知架构
        X86,     ///< 32位x86
        X64,     ///< 64位x86_64
        ARM,     ///< 32位ARM
        ARM64,   ///< 64位ARM
        IA64     ///< Intel Itanium
    };

private:
    system_info system_info_{};        ///< 系统信息
    memory_info memory_info_{};        ///< 内存信息
    CPU_info cpu_info_{};              ///< CPU信息
    os_version_info os_version_info_{}; ///< 操作系统信息
    architecture architecture_ = architecture::UNKNOWN; ///< 系统架构
    atomic<bool> initialized_{false};   ///< 初始化标志

    /**
     * @brief 私有构造函数
     */
    sysinfo();

    /**
     * @brief 析构函数
     */
    ~sysinfo() = default;

    /**
     * @brief 初始化系统信息
     *
     * 实际执行信息收集操作，可能抛出异常。
     */
    void init();

public:
    /**
     * @brief 获取单例实例
     * @return 系统信息实例引用
     */
    static sysinfo& instance() {
        static sysinfo instance;
        return instance;
    }

    sysinfo(const sysinfo&) = delete;
    sysinfo& operator =(const sysinfo&) = delete;
    sysinfo(sysinfo&&) = delete;
    sysinfo& operator =(sysinfo&&) = delete;

    /**
     * @brief 刷新系统信息
     *
     * 重新收集所有系统信息，更新缓存的数据。
     */
    void refresh();

    /**
     * @brief 获取系统底层信息
     * @return 系统信息结构引用
     */
    const system_info& get_system_info() const noexcept {
        return system_info_;
    }

    /**
     * @brief 获取内存信息
     * @return 内存信息结构引用
     */
    const memory_info& get_memory_info() const noexcept {
        return memory_info_;
    }

    /**
     * @brief 获取CPU信息
     * @return CPU信息结构引用
     */
    const CPU_info& get_CPU_info() const noexcept {
        return cpu_info_;
    }

    /**
     * @brief 获取操作系统版本信息
     * @return 操作系统版本信息结构引用
     */
    const os_version_info& get_os_version_info() const noexcept {
        return os_version_info_;
    }

    /**
     * @brief 获取系统架构
     * @return 架构枚举值
     */
    architecture get_architecture() const noexcept {
        return architecture_;
    }

    /**
     * @brief 检查是否已初始化
     * @return 是否已成功初始化
     */
    NEFORCE_NODISCARD bool is_initialized() const noexcept {
        return initialized_.load(memory_order_acquire);
    }

    /**
     * @brief 格式化字节数为可读字符串
     * @param bytes 字节数
     * @return 格式化的字符串（如 "1.23 MB"）
     */
    static string format_bytes(uint64_t bytes);

    /**
     * @brief 获取当前CPU使用率
     * @return CPU使用百分比（0.0-100.0）
     */
    static float64_t cpu_usage();

    /**
     * @brief 获取当前运行的进程数量
     * @return 进程数量
     */
    static uint32_t process_count();
};

/** @} */ // SystemInfo

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_SYSTEM_SYSINFO_HPP__
