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
 * - CPU信息（型号、频率、核心数、SIMD 能力）
 * - 操作系统版本、系统架构
 * - 系统页面大小、分配粒度等底层信息
 * - NUMA 拓扑与主机名
 *
 * @note 全局保持唯一的系统信息实例。
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
        uint16_t processor_level{0};        ///< 处理器级别（Windows）
        uint16_t processor_revision{0};     ///< 处理器修订版本（Windows）
    };

    /**
     * @struct memory_info
     * @brief 内存信息
     */
    struct memory_info {
        uint64_t total_physical{0};      ///< 物理内存总量（字节）
        uint64_t available_physical{0};  ///< 可用物理内存（字节）
        uint64_t cached_physical{0};     ///< 系统缓存/待机内存（字节）
        uint64_t total_virtual{0};       ///< 提交上限总量（物理内存 + 交换空间，字节）
        uint64_t available_virtual{0};   ///< 可用提交额度（字节）
        uint64_t total_page_file{0};     ///< 页面文件/交换空间总量（字节）
        uint64_t available_page_file{0}; ///< 可用页面文件/交换空间（字节）

        /**
         * @brief 计算物理内存使用率
         * @return 内存使用百分比（0.0-100.0）
         */
        NEFORCE_NODISCARD float64_t physical_memory_usage() const noexcept {
            if (total_physical == 0) {
                return 0.0;
            }
            return 100.0 * (1.0 - static_cast<float64_t>(available_physical) / static_cast<float64_t>(total_physical));
        }

        /**
         * @brief 获取总可用内存
         * @return 可用内存总量（物理+虚拟）
         */
        NEFORCE_NODISCARD size_t available_memory() const noexcept { return available_physical + available_virtual; }
    };

    /**
     * @struct disk_info
     * @brief 磁盘空间信息
     *
     * 描述单个磁盘分区（或挂载点）的容量及使用情况。
     */
    struct disk_info {
        string path;                 ///< 查询路径或挂载点
        string filesystem;           ///< 文件系统类型名，无法确定时为空
        uint64_t total_bytes{0};     ///< 总容量（字节）
        uint64_t free_bytes{0};      ///< 空闲空间（字节，含保留块）
        uint64_t available_bytes{0}; ///< 非特权用户可用空间（字节）
        uint64_t used_bytes{0};      ///< 已用空间（字节，等于总容量减空闲空间）

        /**
         * @brief 磁盘使用率
         * @return 使用百分比（0.0 - 100.0）
         */
        NEFORCE_NODISCARD float64_t usage_percent() const noexcept {
            if (total_bytes == 0) {
                return 0.0;
            }
            return 100.0 * static_cast<float64_t>(used_bytes) / static_cast<float64_t>(total_bytes);
        }
    };

    /**
     * @enum simd_instruction
     * @brief SIMD 指令集特性位
     *
     * 多个特性位可组合，用 simd_info::supports() 测试单个特性。
     */
    enum class simd_instruction : uint32_t {
        sse2 = 1U << 0,      ///< SSE2
        ssse3 = 1U << 1,     ///< SSSE3
        sse4_1 = 1U << 2,    ///< SSE4.1
        sse4_2 = 1U << 3,    ///< SSE4.2
        avx = 1U << 4,       ///< AVX
        avx2 = 1U << 5,      ///< AVX2
        fma = 1U << 6,       ///< FMA3
        avx512f = 1U << 7,   ///< AVX-512
        aes = 1U << 8,       ///< AES-NI 或 ARMv8 AES
        pclmulqdq = 1U << 9, ///< PCLMULQDQ 或 ARMv8 PMULL
        neon = 1U << 10,     ///< ARM NEON / ASIMD
        sve = 1U << 11       ///< ARM SVE
    };

    /**
     * @struct simd_info
     * @brief 运行时 SIMD 能力
     */
    struct simd_info {
        uint32_t features{0}; ///< simd_instruction 位掩码

        /**
         * @brief 测试指令集是否可用
         * @param instruction 指令集特性位
         * @return 可用返回 true
         */
        NEFORCE_NODISCARD bool supports(const simd_instruction instruction) const noexcept {
            return (features & static_cast<uint32_t>(instruction)) != 0;
        }
    };

    /**
     * @struct cpu_info
     * @brief CPU信息
     *
     * 包含处理器的型号、频率、核心数等信息。
     */
    struct cpu_info {
        string vendor;                  ///< 厂商名称
        string brand;                   ///< 型号名称
        uint32_t max_mhz{0};            ///< 最大频率（MHz）
        uint32_t current_mhz{0};        ///< 采集时刻的频率（MHz）
        uint32_t cores{0};              ///< 物理核心数
        uint32_t logical_processors{0}; ///< 逻辑处理器数
        string features;                ///< 支持的指令集特性
        simd_info simd;                 ///< 运行时 SIMD 能力

        /**
         * @struct x86_info
         * @brief x86 专有 CPU 信息
         */
        struct x86_info {
            string family;     ///< 基础 Family
            string model;      ///< 基础 Model
            string stepping;   ///< Stepping
            string ext_family; ///< 扩展 Family
            string ext_model;  ///< 扩展 Model
            string revision;   ///< 修订号
            string core_vid;   ///< 核心电压
            string technology; ///< 制程工艺
            string socket;     ///< 封装/插槽
            string code_name;  ///< 核心代号

            uint32_t model_id{0};      ///< 完整 Model 编号
            uint32_t max_tdp_watts{0}; ///< 最大热设计功耗（W）
            uint32_t bus_speed_mhz{0}; ///< 外部时钟/总线频率（MHz）
        } x86;

        /**
         * @struct arm_info
         * @brief ARM64 专有 CPU 信息
         */
        struct arm_info {
            string architecture; ///< CPU architecture
            string implementer;  ///< CPU implementer
            string part;         ///< CPU part
            string variant;      ///< CPU variant
            string revision;     ///< CPU revision
            string core_name;    ///< 核心名称
        } arm;

        /**
         * @struct riscv_info
         * @brief RISC-V 专有 CPU 信息
         */
        struct riscv_info {
            string isa;       ///< ISA 字符串
            string mmu;       ///< MMU 类型
            string uarch;     ///< 微架构
            string mvendorid; ///< 厂商 ID
            string marchid;   ///< 架构 ID
            string mimpid;    ///< 实现 ID
        } riscv;

        /**
         * @struct loongarch_info
         * @brief LoongArch 专有 CPU 信息
         */
        struct loongarch_info {
            string family;           ///< CPU Family
            string revision;         ///< CPU Revision
            string fpu_revision;     ///< FPU Revision
            string isa;              ///< ISA
            string address_sizes;    ///< 地址位宽
            string machine;          ///< 机器型号
            string system_type;      ///< 系统类型
            string watchpoint;       ///< 硬件观察点能力
            uint32_t tlb_entries{0}; ///< TLB 条目数
        } loongarch;

        uint32_t l1_kib{0};                   ///< L1 单实例容量（KiB）
        uint32_t l1_instances{0};             ///< L1 实例数
        uint32_t l1_data_kib{0};              ///< L1 数据缓存单实例容量（KiB）
        uint32_t l1_data_instances{0};        ///< L1 数据缓存实例数
        uint32_t l1_instruction_kib{0};       ///< L1 指令缓存单实例容量（KiB）
        uint32_t l1_instruction_instances{0}; ///< L1 指令缓存实例数
        uint32_t l2_kib{0};                   ///< L2 单实例容量（KiB）
        uint32_t l2_instances{0};             ///< L2 实例数
        uint32_t l3_kib{0};                   ///< L3 单实例容量（KiB）
        uint32_t l3_instances{0};             ///< L3 实例数
        uint32_t pcore_count{0};              ///< 性能核数量（异构 CPU）
        uint32_t ecore_count{0};              ///< 能效核数量（异构 CPU）

        /**
         * @brief 检查是否支持超线程
         * @return 如果逻辑处理器数大于物理核心数返回true
         */
        NEFORCE_NODISCARD bool hyperthreading() const noexcept { return logical_processors > cores; }
    };

    /**
     * @struct os_version_info
     * @brief 操作系统版本信息
     */
    struct os_version_info {
        uint32_t major{0};       ///< 主版本号
        uint32_t minor{0};       ///< 次版本号
        uint32_t build{0};       ///< 构建号
        uint32_t platform_id{0}; ///< 平台ID（Windows）
        string csd_version;      ///< 补丁版本（Windows）
        string product_name;     ///< 产品名称

        /**
         * @brief 获取版本字符串
         * @return 格式为"major.minor.build"的版本字符串
         */
        NEFORCE_NODISCARD NEFORCE_API string version() const;
    };

    /**
     * @struct hardware_info
     * @brief 机器硬件档案
     *
     * 汇总 BIOS、主板和内存的静态信息。
     */
    struct hardware_info {
        /**
         * @struct bios_section
         * @brief BIOS 信息
         */
        struct bios_section {
            string brand;     ///< BIOS 厂商
            string version;   ///< BIOS 版本
            string date;      ///< 发布日期
            string microcode; ///< CPU 微码版本（x86）
        } bios;

        /**
         * @struct board_section
         * @brief 主板信息
         */
        struct board_section {
            string vendor;      ///< 主板厂商
            string model;       ///< 主板型号
            string bus;         ///< 总线类型
            string chipset;     ///< 芯片组
            string southbridge; ///< 南桥/IO 控制器
        } board;

        /**
         * @struct memory_section
         * @brief 内存信息
         */
        struct memory_section {
            string type;                ///< 内存类型（Linux 需 root）
            string channel;             ///< 通道与模组数量（Linux 需 root）
            uint32_t module_count{0};   ///< 已识别模组数（Linux 需 root）
            uint32_t configured_mts{0}; ///< 实际运行速率（MT/s，Linux 需 root）
            uint32_t rated_mts{0};      ///< 模组标称速率（MT/s，Linux 需 root）
            string timings;             ///< 时序
            string llc_ring;            ///< LLC/环形总线信息
        } memory;
    };

    /**
     * @struct network_interface
     * @brief 网络接口信息
     */
    struct network_interface {
        string name;              ///< 接口名称
        string address;           ///< 首选单播 IP 地址
        string netmask;           ///< 该地址的子网掩码，IPv6 下为空
        string mac;               ///< MAC 地址，无法获取时为空
        string operstate;         ///< 运行状态（"up"/"down"/"unknown" 等）
        uint32_t index{0};        ///< 接口索引
        uint8_t prefix_length{0}; ///< 首选地址的前缀长度
        uint64_t mtu{0};          ///< MTU（字节），无法获取时为 0
        uint64_t speed_bps{0};    ///< 链路速率（bit/s），无法获取时为 0
        uint64_t rx_bytes{0};     ///< 累计接收字节
        uint64_t tx_bytes{0};     ///< 累计发送字节
        uint64_t rx_packets{0};   ///< 累计接收包
        uint64_t tx_packets{0};   ///< 累计发送包
        uint64_t rx_errors{0};    ///< 接收错误数
        uint64_t tx_errors{0};    ///< 发送错误数
        uint64_t rx_dropped{0};   ///< 接收丢包数
        uint64_t tx_dropped{0};   ///< 发送丢包数
        bool is_up{false};        ///< 接口是否启用
        bool is_loopback{false};  ///< 是否为回环接口
    };

    /**
     * @enum architecture
     * @brief 系统架构枚举
     */
    enum class architecture {
        unknown,     ///< 未知架构
        x86,         ///< 32位x86
        x64,         ///< 64位x86_64
        arm,         ///< 32位ARM
        arm64,       ///< 64位ARM
        riscv32,     ///< 32位RISC-V
        riscv64,     ///< 64位RISC-V
        loongarch32, ///< 32位LoongArch
        loongarch64  ///< 64位LoongArch
    };

    /**
     * @struct battery_info
     * @brief 电池与电源状态
     *
     * 多电池时按剩余电量加权平均百分比。
     */
    struct battery_info {
        bool present{false};               ///< 是否存在电池
        bool on_ac{false};                 ///< 是否接入交流电源
        bool charging{false};              ///< 是否正在充电
        float64_t charge_percent{-1.0};    ///< 剩余电量百分比，未知为 -1
        int64_t remaining_seconds{-1};     ///< 预计可用秒数，未知为 -1（充电/满电时通常未知）
        float64_t rate_mw{-1.0};           ///< 充放电功率绝对值（毫瓦），未知为 -1（满电/交流供电时可能未知）
        float64_t capacity_mwh{-1.0};      ///< 当前剩余容量（毫瓦时），未知为 -1
        float64_t full_capacity_mwh{-1.0}; ///< 满电容量（毫瓦时），未知为 -1
        string status;                     ///< "charging"/"discharging"/"full"/"idle"/"unknown"
    };

    /**
     * @struct cpu_breakdown_info
     * @brief CPU 时间构成占比
     *
     * 各字段为 0.0-100.0 的百分比；平台无法提供的状态保持 0。
     */
    struct cpu_breakdown_info {
        float64_t user{0.0};       ///< 用户态
        float64_t nice{0.0};       ///< 低优先级用户态（Linux）
        float64_t system{0.0};     ///< 内核态
        float64_t idle{0.0};       ///< 空闲
        float64_t iowait{0.0};     ///< 等待 I/O（Linux）
        float64_t irq{0.0};        ///< 硬中断
        float64_t softirq{0.0};    ///< 软中断（Windows 为 DPC）
        float64_t steal{0.0};      ///< 虚拟化窃取时间（Linux）
        float64_t guest{0.0};      ///< 客户机时间（Linux）
        float64_t guest_nice{0.0}; ///< 低优先级客户机时间（Linux）
    };

    /**
     * @struct disk_io_info
     * @brief 磁盘 I/O 速率
     */
    struct disk_io_info {
        string name;                         ///< 设备名
        float64_t read_bytes_per_second{0};  ///< 读取速率（字节/秒）
        float64_t write_bytes_per_second{0}; ///< 写入速率（字节/秒）
        float64_t busy_percent{0};           ///< 忙碌百分比（0-100）
        bool aggregate{false};               ///< 是否为整盘而非分区
    };

    /**
     * @struct numa_node_info
     * @brief NUMA 节点信息
     *
     * 描述单个 NUMA 节点的拓扑信息，包括节点编号、
     * 所属逻辑核心列表和核心位图掩码
     */
    struct numa_node_info {
        uint32_t node_id{0};        ///< NUMA 节点编号
        uint32_t core_count{0};     ///< 属于该节点的逻辑核心数
        uint64_t core_mask{0};      ///< 核心位图掩码（64 核以内直接可用）
        vector<uint32_t> core_list; ///< 核心逻辑编号列表
    };

    /**
     * @struct mount_point
     * @brief 挂载点信息
     */
    struct mount_point {
        string source;               ///< 设备或来源
        string target;               ///< 挂载路径
        string filesystem;           ///< 文件系统类型
        string options;              ///< 挂载选项
        uint64_t total_bytes{0};     ///< 总容量（字节）
        uint64_t free_bytes{0};      ///< 空闲容量（字节，含保留块）
        uint64_t available_bytes{0}; ///< 非特权用户可用容量（字节）
    };

private:
    system_info system_info_{};                        ///< 系统信息
    cpu_info cpu_info_{};                              ///< CPU信息
    hardware_info hardware_info_{};                    ///< 机器硬件档案
    os_version_info os_version_info_{};                ///< 操作系统信息
    architecture architecture_{architecture::unknown}; ///< 系统架构
    vector<numa_node_info> numa_nodes_;                ///< NUMA 节点信息列表
    string hostname_;                                  ///< 主机名
    atomic<bool> initialized_{false};                  ///< 初始化标志

    /**
     * @brief 私有构造函数
     */
    sysinfo() noexcept;

    /**
     * @brief 析构函数
     */
    ~sysinfo() = default;

    /**
     * @brief 采集机器档案
     */
    void init();

    /**
     * @brief 采集 CPU/BIOS/主板/内存详细档案
     */
    void probe_hardware();

    /**
     * @brief 探测 CPU 与操作系统的 SIMD 能力
     * @return SIMD 能力位图
     */
    NEFORCE_NODISCARD static simd_info probe_simd();

public:
    /**
     * @brief 获取单例实例
     * @return 系统信息实例引用
     */
    static sysinfo& instance() noexcept;

    sysinfo(const sysinfo&) = delete;
    sysinfo& operator=(const sysinfo&) = delete;
    sysinfo(sysinfo&&) = delete;
    sysinfo& operator=(sysinfo&&) = delete;

    /**
     * @brief 刷新机器档案
     * @note 静态度量函数每次调用都取当前值，不受此函数影响
     */
    void refresh() noexcept;

    /**
     * @brief 检查是否已初始化
     * @return 是否已成功初始化
     */
    NEFORCE_NODISCARD bool is_initialized() const noexcept { return initialized_.load(memory_order_acquire); }

    /**
     * @brief 获取系统底层信息
     * @return 系统信息结构引用
     */
    NEFORCE_NODISCARD const system_info& get_system_info() const noexcept { return system_info_; }

    /**
     * @brief 获取CPU信息
     * @return CPU信息结构引用
     */
    NEFORCE_NODISCARD const cpu_info& get_cpu_info() const noexcept { return cpu_info_; }

    /**
     * @brief 获取机器硬件档案
     * @return BIOS、主板和内存详细信息的引用
     */
    NEFORCE_NODISCARD const hardware_info& get_hardware_info() const noexcept { return hardware_info_; }

    /**
     * @brief 获取操作系统版本信息
     * @return 操作系统版本信息结构引用
     */
    NEFORCE_NODISCARD const os_version_info& get_os_version_info() const noexcept { return os_version_info_; }

    /**
     * @brief 获取系统架构
     * @return 架构枚举值
     */
    NEFORCE_NODISCARD architecture get_architecture() const noexcept { return architecture_; }

    /**
     * @brief 获取 NUMA 节点拓扑信息
     * @return NUMA 节点信息列表
     *
     * 在支持 NUMA 的系统中返回所有 NUMA 节点的拓扑信息。
     * 不支持 NUMA 的系统返回空列表。
     */
    NEFORCE_NODISCARD const vector<numa_node_info>& get_numa_info() const noexcept { return numa_nodes_; }

    /**
     * @brief 获取主机名
     * @return 主机名，采集失败时为空字符串
     */
    NEFORCE_NODISCARD const string& get_hostname() const noexcept { return hostname_; }

    /**
     * @brief 获取主机名副本
     * @return 主机名
     */
    NEFORCE_NODISCARD string host_name() const;

    /**
     * @brief 获取操作系统版本副本
     * @return 操作系统版本信息
     */
    NEFORCE_NODISCARD os_version_info os_version() const;

    /**
     * @brief 从CPU型号字符串中解析标称频率
     * @param brand CPU型号字符串
     * @return 标称频率，单位 MHz；型号字符串不含频率信息时返回 0
     */
    NEFORCE_NODISCARD static uint32_t parse_brand_frequency(string_view brand) noexcept;

    /**
     * @brief 获取当前CPU使用率
     * @return CPU使用百分比（0.0-100.0）
     * @note 基于两次采样的差值，首次调用返回 0
     */
    NEFORCE_NODISCARD static float64_t cpu_usage();

    /**
     * @brief 获取每个逻辑处理器的 CPU 使用率
     * @return 与逻辑处理器数量等长的使用率列表（0.0-100.0），无法获取时为空
     * @note 基于两次采样的差值，首次调用返回全 0
     */
    NEFORCE_NODISCARD static vector<float64_t> cpu_usage_per_core();

    /**
     * @brief 获取 CPU 时间构成占比
     * @return 各状态百分比
     */
    NEFORCE_NODISCARD static cpu_breakdown_info cpu_usage_breakdown();

    /**
     * @brief 获取当前内存使用情况
     * @return 内存信息，每次调用实时采样
     */
    NEFORCE_NODISCARD static memory_info memory_status();

    /**
     * @brief 获取指定路径的磁盘空间信息
     * @param path 文件系统路径，为空时取当前工作目录
     * @return 磁盘信息，查询失败时容量字段为 0 且 filesystem 为空
     */
    NEFORCE_NODISCARD static disk_info disk_status(const string& path = {});

    /**
     * @brief 获取当前运行的进程数量
     * @return 进程数量
     */
    NEFORCE_NODISCARD static uint32_t process_count();

    /**
     * @brief 枚举所有网络接口
     * @return 网络接口信息列表
     */
    NEFORCE_NODISCARD static vector<network_interface> network_interfaces();

    /**
     * @brief 枚举本机挂载点
     * @return 挂载点列表，平台不支持或读取失败时返回空列表
     */
    NEFORCE_NODISCARD static vector<mount_point> mount_points();

    /**
     * @brief 获取各磁盘的 I/O 速率
     * @return 磁盘 I/O 列表，按设备名排序
     * @note 基于两次调用的差值，首次调用返回空或全 0；需要读取 /proc/diskstats 或 PDH
     */
    NEFORCE_NODISCARD static vector<disk_io_info> disk_io_rates();

    /**
     * @brief 获取系统运行时间
     * @return 系统启动以来的秒数
     */
    NEFORCE_NODISCARD static uint64_t uptime_seconds();

    /**
     * @brief 获取电池与电源状态
     * @return 电池状态，无电池时 present 为 false
     */
    NEFORCE_NODISCARD static battery_info battery_status();
};

/** @} */ // SystemInfo

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_SYSTEM_SYSINFO_HPP__
