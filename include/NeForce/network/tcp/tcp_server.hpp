#ifndef NEFORCE_NETWORK_TCP_TCP_SERVER_HPP__
#define NEFORCE_NETWORK_TCP_TCP_SERVER_HPP__

/**
 * @file tcp_server.hpp
 * @brief TCP服务器实现
 *
 * 此文件提供了TCP服务器的完整实现，支持普通TCP和SSL/TLS加密连接。
 * 使用线程池处理客户端连接，支持非阻塞接受和自定义处理器。
 */

#include "NeForce/core/async/shared_mutex.hpp"
#include "NeForce/core/async/thread_pool.hpp"
#include "NeForce/core/system/pipe.hpp"
#include "NeForce/network/ssl/ssl_acceptor.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup TCP
 * @{
 */

/**
 * @class tcp_server_base
 * @brief TCP服务器基类
 *
 * 提供TCP服务器的核心功能，包括连接接受、线程池管理、
 * 客户端处理和异常处理。派生类实现具体的接受器创建。
 */
class NEFORCE_API tcp_server_base {
public:
    using client_handler_t = function<void(unique_ptr<tcp_socket>)>; ///< 客户端处理器类型
    using exception_handler_t = function<void(const exception&)>;    ///< 异常处理器类型

protected:
    unique_ptr<tcp_acceptor> acceptor_; ///< TCP接受器
    ports port_;                        ///< 监听端口
    atomic<bool> running_{false};       ///< 运行标志
    vector<thread> worker_threads_;     ///< 工作线程列表
    thread_pool client_pool_;           ///< 客户端处理线程池

#ifdef NEFORCE_PLATFORM_WINDOWS
    ::WSAEVENT wake_event_{WSA_INVALID_EVENT};
#else
    pipe wake_pipe_;
#endif
    mutable shared_mutex handler_mutex_;

    client_handler_t client_handler_;       ///< 客户端处理器
    exception_handler_t exception_handler_; ///< 异常处理器
    mutex acceptor_mutex_;                  ///< TCP接受器互斥锁

    /**
     * @brief 向 wake_pipe_ 写端写入一个字节，唤醒 accept_loop
     */
    void notify_stop() noexcept;

    /**
     * @brief 接受连接的主循环
     */
    void accept_loop();

    /**
     * @brief 处理单个客户端连接
     * @param client 已建立连接的 socket
     *
     * 调用客户端处理器处理连接。
     * 派生类可重写此方法以自定义处理逻辑。
     */
    virtual void handle_client(unique_ptr<tcp_socket> client) {
        if (client_handler_) {
            client_handler_(move(client));
        }
    }

    /**
     * @brief 创建一个客户端连接
     * @return 客户端socket，无连接返回none
     *
     * 派生类实现具体的连接接受逻辑。
     * 支持阻塞和非阻塞模式。
     */
    virtual optional<unique_ptr<tcp_socket>> accept_one() = 0;

    /**
     * @brief 创建并配置acceptor
     * @param endpoint 监听地址
     * @param backlog 连接队列大小
     *
     * 派生类实现具体的acceptor创建和配置。
     */
    virtual void create_acceptor(const ip_address& endpoint, int backlog) = 0;

public:
    /**
     * @brief 构造函数
     * @param port 监听端口
     * @param worker_count 工作线程数（默认最大）
     * @throws value_exception worker_count为0时抛出
     */
    explicit tcp_server_base(ports port, size_t worker_count = thread_pool::max_thread_threshhold());

    /**
     * @brief 析构函数
     *
     * 自动停止服务器。
     */
    virtual ~tcp_server_base() { stop(); }

    tcp_server_base(const tcp_server_base&) = delete;
    tcp_server_base& operator=(const tcp_server_base&) = delete;

    tcp_server_base(tcp_server_base&& other) noexcept = delete;
    tcp_server_base& operator=(tcp_server_base&& other) noexcept = delete;

    /**
     * @brief 设置客户端处理器
     * @param handler 处理器函数
     * @return 设置成功返回true
     *
     * 必须在服务器启动前设置。
     */
    bool set_client_handler(client_handler_t handler);

    /**
     * @brief 设置异常处理器
     * @param handler 处理器函数
     * @return 设置成功返回true
     */
    bool set_exception_handler(exception_handler_t handler);

    /**
     * @brief 启动服务器
     * @param backlog 连接队列大小（默认SOMAXCONN）
     * @return 启动成功返回true
     *
     * 创建acceptor，开始接受连接。
     * 需要先设置客户端处理器。
     */
    virtual bool start(int backlog = SOMAXCONN) noexcept;

    /**
     * @brief 停止服务器
     *
     * 停止接受新连接，等待现有连接处理完成。
     */
    void stop();

    /**
     * @brief 检查服务器是否运行中
     * @return 运行中返回true
     */
    NEFORCE_NODISCARD bool is_running() const noexcept { return running_; }

    /**
     * @brief 获取监听端口
     * @return 端口号
     */
    NEFORCE_NODISCARD ports port() const noexcept { return port_; }
};

/**
 * @class tcp_server
 * @brief TCP服务器类
 *
 * 实现普通TCP服务器，使用tcp_acceptor接受连接。
 *
 * 主要功能：
 * - TCP服务器启动和停止
 * - SSL/TLS服务器支持
 * - 线程池处理客户端连接
 * - 非阻塞连接接受
 * - 自定义客户端处理器
 * - 异常处理回调
 */
class NEFORCE_API tcp_server final : public tcp_server_base {
private:
    void create_acceptor(const ip_address& endpoint, int backlog) override;
    optional<unique_ptr<tcp_socket>> accept_one() override;

public:
    using tcp_server_base::tcp_server_base;
};

/** @} */ // TCP

/**
 * @addtogroup SSL SSL/TLS
 * @{
 */

/**
 * @class ssl_server
 * @brief SSL/TLS服务器类
 *
 * 实现SSL/TLS加密的TCP服务器，使用ssl_acceptor接受连接。
 *
 * 使用示例：
 * @code
 * // 创建SSL服务器
 * ssl_server server(443, 10);
 *
 * // 加载证书
 * if (!server.load_certificate("server.crt", "server.key")) {
 *     println("Failed to load certificate");
 * }
 *
 * // 设置客户端处理器
 * server.set_client_handler([](unique_ptr<tcp_socket> client) {
 *     // client is polymorphic — ssl_socket methods work via virtual dispatch
 *     client->send_all({"response"});
 * });
 *
 * // 启动服务器
 * server.start();
 * @endcode
 */
class NEFORCE_API ssl_server final : public tcp_server_base {
private:
    ssl_context ssl_ctx_; ///< SSL上下文

private:
    void create_acceptor(const ip_address& endpoint, int backlog) override;
    optional<unique_ptr<tcp_socket>> accept_one() override;

public:
    /**
     * @brief 构造函数
     * @param port 监听端口
     * @param worker_count 工作线程数
     */
    explicit ssl_server(ports port, size_t worker_count = thread_pool::max_thread_threshhold());

    /**
     * @brief 加载证书和私钥
     * @param cert_file 证书文件路径
     * @param key_file 私钥文件路径
     * @return 加载成功返回true
     */
    bool load_certificate(const string& cert_file, const string& key_file);

    /**
     * @brief 设置SSL上下文
     * @param ctx SSL上下文
     * @throws ssl_exception 服务器运行时或上下文无效时抛出
     */
    void set_ssl_context(ssl_context ctx);

    /**
     * @brief 获取SSL上下文
     * @return SSL上下文引用
     */
    NEFORCE_NODISCARD ssl_context& get_ssl_context() noexcept { return ssl_ctx_; }

    /**
     * @brief 获取常量SSL上下文
     * @return 常量SSL上下文引用
     */
    NEFORCE_NODISCARD const ssl_context& get_ssl_context() const noexcept { return ssl_ctx_; }

    /**
     * @brief 启动SSL服务器
     * @param backlog 连接队列大小
     * @return 启动成功返回true
     *
     * 需要先加载证书或设置SSL上下文。
     */
    bool start(int backlog = SOMAXCONN) noexcept override;
};

/** @} */ // SSL/TLS

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_TCP_TCP_SERVER_HPP__
