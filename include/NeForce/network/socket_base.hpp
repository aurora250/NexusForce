#ifndef NEFORCE_NETWORK_SOCKET_BASE_HPP__
#define NEFORCE_NETWORK_SOCKET_BASE_HPP__

/**
 * @file socket_base.hpp
 * @brief Socket基础类实现
 *
 * 此文件提供了Socket基础类。
 */

#include "NeForce/core/time/duration.hpp"
#include "NeForce/network/util/ip_address.hpp"
#ifdef NEFORCE_PLATFORM_LINUX
#    include <linux/if_ether.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup Exceptions 异常类集
 * @{
 */

/**
 * @struct socket_exception
 * @brief Socket操作异常类
 *
 * Socket操作失败时抛出的异常，封装了系统错误码。
 */
struct NEFORCE_API socket_exception final : network_exception {
    /**
     * @brief 获取最后系统Socket错误码
     * @return 错误码
     */
    static int last_error() noexcept;

    /**
     * @brief 检查错误码是否表示操作会阻塞
     * @param error 错误码
     * @return 是会阻塞返回true
     */
    static bool is_would_block(int error) noexcept;

    explicit socket_exception(const char* info = "Socket Operation Failed.", const char* type = static_type,
                              const int code = last_error()) noexcept :
    network_exception(info, type, code) {}

    explicit socket_exception(const exception& e) :
    network_exception(e) {}

    ~socket_exception() override = default;

    static constexpr auto static_type = "socket_exception";
};

/** @} */ // Exceptions

/**
 * @defgroup SocketBase socket基类
 * @brief 系统socket基类实现
 * @{
 */

/**
 * @class socket_base
 * @brief Socket基础类
 *
 * 提供跨平台的socket基础操作，是所有socket类的基类。
 * 支持移动语义，不支持拷贝。
 *
 * 主要功能：
 * - Socket创建和关闭
 * - 阻塞/非阻塞模式设置
 * - Socket选项配置（地址重用、端口重用、TCP_NODELAY、KeepAlive等）
 * - 收发缓冲区大小设置
 * - 超时设置
 * - 本地/远程端点查询
 * - 绑定和监听
 * - 移动语义支持
 */
class NEFORCE_API socket_base {
public:
    using native_handle_type =
#ifdef NEFORCE_PLATFORM_WINDOWS
            ::UINT_PTR; ///< 平台原生句柄类型
#else
            int; ///< 平台原生句柄类型
#endif

    /**
     * @brief 无效句柄常量
     */
    static constexpr native_handle_type invalid_handle =
#ifdef NEFORCE_PLATFORM_WINDOWS
            numeric_traits<native_handle_type>::max(); ///< 无效句柄
#else
            -1; ///< 无效句柄
#endif

    using family = ip_address::family; ///< 网络地址族类型

    /**
     * @brief socket 类型枚举
     */
    enum class type : int32_t {
        STREAM = SOCK_STREAM,       ///< 流式套接字（TCP）
        DGRAM = SOCK_DGRAM,         ///< 数据报套接字（UDP）
        RAW = SOCK_RAW,             ///< 原始套接字
        RDM = SOCK_RDM,             ///< 可靠数据报套接字
        SEQPACKET = SOCK_SEQPACKET, ///< 有序分组套接字（SCTP）
#ifdef NEFORCE_PLATFORM_LINUX
        CLOEXEC = SOCK_CLOEXEC,   ///< 执行时关闭标志
        NONBLOCK = SOCK_NONBLOCK, ///< 非阻塞标志
        DCCP = SOCK_DCCP,         ///< 数据报拥塞控制协议（DCCP）
#endif
    };

    /**
     * @brief socket 协议枚举
     *
     * 封装了底层协议标识常量，用于指定套接字使用的网络协议类型。
     */
    enum class protocol : int32_t {
        AUTO = 0,            ///< 自动选择协议
        ICMP = IPPROTO_ICMP, ///< Internet 控制报文协议
        TCP = IPPROTO_TCP,   ///< 传输控制协议
        UDP = IPPROTO_UDP,   ///< 用户数据报协议
        SCTP = IPPROTO_SCTP, ///< 流控制传输协议
        RAW = IPPROTO_RAW,   ///< 原始 IP 数据包
        ESP = IPPROTO_ESP,   ///< IPsec 封装安全载荷
        AH = IPPROTO_AH,     ///< IPsec 认证头
#ifdef NEFORCE_PLATFORM_LINUX
        DCCP = IPPROTO_DCCP, ///< 数据报拥塞控制协议

        IP = ETH_P_IP,             ///< IPv4 以太网帧类型
        ARP = ETH_P_ARP,           ///< 地址解析协议帧类型
        IPV6 = ETH_P_IPV6,         ///< IPv6 以太网帧类型
        LAT = ETH_P_LAT,           ///< 局域传输协议
        ETHERCAT = ETH_P_ETHERCAT, ///< EtherCAT 工业以太网协议
        VLAN = ETH_P_8021Q,        ///< IEEE 802.1Q VLAN 标签帧
        LLDP = ETH_P_LLDP,         ///< 链路层发现协议
        MPLS = ETH_P_MPLS_UC       ///< 多协议标签交换单播
#endif
    };

protected:
    native_handle_type fd_ = invalid_handle; ///< Socket句柄
    bool nonblocking_{false};                ///< 非阻塞模式状态跟踪

public:
    /**
     * @brief 默认构造函数
     */
    socket_base();

    /**
     * @brief 从原生句柄构造
     * @param fd 原生socket句柄
     */
    explicit socket_base(const native_handle_type fd) noexcept :
    fd_(fd) {}

    socket_base(const socket_base&) = delete;
    socket_base& operator=(const socket_base&) = delete;

    /**
     * @brief 移动构造函数
     * @param other 源对象
     */
    socket_base(socket_base&& other) noexcept :
    fd_(exchange(other.fd_, invalid_handle)),
    nonblocking_(exchange(other.nonblocking_, false)) {}

    /**
     * @brief 移动赋值运算符
     * @param other 源对象
     * @return 自身引用
     */
    socket_base& operator=(socket_base&& other) noexcept;

    /**
     * @brief 析构函数
     */
    virtual ~socket_base() { socket_base::close(); }

    /**
     * @brief 获取原生句柄
     * @return 原生socket句柄
     */
    NEFORCE_NODISCARD native_handle_type native_handle() const noexcept { return fd_; }

    /**
     * @brief 检查socket是否已打开
     * @return 已打开返回true
     */
    NEFORCE_NODISCARD bool is_open() const noexcept { return fd_ != invalid_handle; }

    /**
     * @brief 布尔转换运算符
     * @return 已打开返回true
     */
    explicit operator bool() const noexcept { return is_open(); }

    /**
     * @brief 打开socket
     * @param f 地址族
     * @param t socket类型
     * @param p 协议
     * @throws socket_exception 创建失败时抛出
     * @throws value_exception 地址族无效时抛出
     */
    void open(family f, type t = type::STREAM, protocol p = protocol::AUTO);

    /**
     * @brief 关闭socket
     * @return 关闭成功返回true
     */
    virtual bool close() noexcept;

    /**
     * @brief 尝试打开socket
     * @param f 地址族
     * @param t socket类型
     * @param p 协议
     * @return 打开成功返回true
     */
    bool try_open(family f, type t = type::STREAM, protocol p = protocol::AUTO) noexcept;

    /**
     * @brief 查询是否为非阻塞模式
     * @return 非阻塞返回true
     */
    NEFORCE_NODISCARD bool is_nonblocking() const noexcept;

    /**
     * @brief 设置非阻塞模式
     * @param enable 是否启用
     * @return 设置成功返回true
     */
    bool set_nonblocking(bool enable) noexcept;

    /**
     * @brief 关闭发送方向
     * @return 成功返回true
     */
    bool shutdown_send() noexcept;

    /**
     * @brief 关闭接收方向
     * @return 成功返回true
     */
    bool shutdown_receive() noexcept;

    /**
     * @brief 关闭双向通信
     * @return 成功返回true
     */
    bool shutdown_both() noexcept;

    /**
     * @brief 设置socket选项
     * @param level 协议层（SOL_SOCKET、IPPROTO_TCP等）
     * @param optname 选项名
     * @param value 选项值指针
     * @param len 值长度
     * @return 设置成功返回true
     */
    bool set_option(int level, int optname, const void* value, ::socklen_t len) noexcept;

    /**
     * @brief 获取socket选项
     * @param level 协议层
     * @param optname 选项名
     * @param optval 输出缓冲区
     * @param optlen 输入输出长度
     * @return 获取成功返回true
     */
    bool get_option(int level, int optname, void* optval, ::socklen_t* optlen) const noexcept;

    /**
     * @brief 查询地址重用
     * @return 已启用返回true，失败返回false
     */
    NEFORCE_NODISCARD bool get_reuse_address() const noexcept;

    /**
     * @brief 设置地址重用
     * @param enable 是否启用
     * @return 设置成功返回true
     */
    bool set_reuse_address(bool enable = true) noexcept;

    /**
     * @brief 查询端口重用
     * @return 已启用返回true，失败或平台不支持返回false
     */
    NEFORCE_NODISCARD bool get_reuse_port() const noexcept;

    /**
     * @brief 设置端口重用
     * @param enable 是否启用
     * @return 设置成功返回true（Windows不支持）
     */
    bool set_reuse_port(bool enable = true) noexcept;

    /**
     * @brief 查询TCP KeepAlive
     * @return 已启用返回true，失败返回false
     */
    NEFORCE_NODISCARD bool get_keep_alive() const noexcept;

    /**
     * @brief 设置TCP KeepAlive
     * @param enable 是否启用
     * @return 设置成功返回true
     */
    bool set_keep_alive(bool enable = true) noexcept;

    /**
     * @brief 查询Nagle算法
     * @return 已禁用Nagle返回true，失败返回false
     */
    NEFORCE_NODISCARD bool get_tcp_nodelay() const noexcept;

    /**
     * @brief 设置禁用Nagle算法
     * @param enable 是否启用
     * @return 设置成功返回true
     */
    bool set_tcp_nodelay(bool enable = true) noexcept;

    /**
     * @brief 查询接收缓冲区大小
     * @return 缓冲区大小（字节），失败返回-1
     */
    NEFORCE_NODISCARD int get_receive_buffer_size() const noexcept;

    /**
     * @brief 设置接收缓冲区大小
     * @param size 缓冲区大小（字节）
     * @return 设置成功返回true
     */
    bool set_receive_buffer_size(int size) noexcept;

    /**
     * @brief 查询发送缓冲区大小
     * @return 缓冲区大小（字节），失败返回-1
     */
    NEFORCE_NODISCARD int get_send_buffer_size() const noexcept;

    /**
     * @brief 设置发送缓冲区大小
     * @param size 缓冲区大小（字节）
     * @return 设置成功返回true
     */
    bool set_send_buffer_size(int size) noexcept;

    /**
     * @brief 获取发送超时时间
     * @return 超时时间，失败返回none
     */
    NEFORCE_NODISCARD optional<milliseconds> get_send_timeout() const noexcept;

    /**
     * @brief 设置发送超时时间
     * @param timeout 超时时间
     * @return 设置成功返回true
     */
    bool set_send_timeout(milliseconds timeout) noexcept;

    /**
     * @brief 获取接收超时时间
     * @return 超时时间，失败返回none
     */
    NEFORCE_NODISCARD optional<milliseconds> get_receive_timeout() const noexcept;

    /**
     * @brief 设置接收超时时间
     * @param timeout 超时时间
     * @return 设置成功返回true
     */
    bool set_receive_timeout(milliseconds timeout) noexcept;

    /**
     * @brief 获取本地端点地址
     * @return 本地IP地址和端口，失败返回none
     */
    NEFORCE_NODISCARD optional<ip_address> local_endpoint() const;

    /**
     * @brief 获取远程端点地址
     * @return 远程IP地址和端口，失败返回none
     */
    NEFORCE_NODISCARD optional<ip_address> remote_endpoint() const;

    /**
     * @brief 绑定socket到本地地址
     * @param endpoint 要绑定的IP地址和端口
     * @throws socket_exception 绑定失败时抛出
     * @throws value_exception socket未打开或端点无效时抛出
     */
    void bind(const ip_address& endpoint);

    /**
     * @brief 开始监听连接（TCP）
     * @param backlog 连接队列大小
     * @throws socket_exception 监听失败时抛出
     * @throws value_exception socket未打开时抛出
     */
    void listen(int backlog);

    /**
     * @brief 释放socket所有权
     * @return 原生socket句柄
     *
     * 释放后当前对象不再管理该socket，调用者负责关闭。
     */
    NEFORCE_NODISCARD native_handle_type release() noexcept { return exchange(fd_, invalid_handle); }
};

/** @} */ // SocketBase

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_SOCKET_BASE_HPP__
