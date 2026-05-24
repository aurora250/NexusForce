#ifndef NEFORCE_NETWORK_HTTP_WEBSOCKET_HPP__
#define NEFORCE_NETWORK_HTTP_WEBSOCKET_HPP__

/**
 * @file websocket.hpp
 * @brief WebSocket协议实现
 *
 * 此文件提供了WebSocket协议的完整实现。
 */

#include "NeForce/core/async/atomic.hpp"
#include "NeForce/core/async/condition_variable.hpp"
#include "NeForce/core/async/thread.hpp"
#include "NeForce/core/container/queue.hpp"
#include "NeForce/core/container/unordered_map.hpp"
#include "NeForce/core/functional/function.hpp"
#include "NeForce/core/memory/shared_ptr.hpp"
#include "NeForce/network/http/http_server_message.hpp"
#include "NeForce/network/ssl/ssl_socket.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

/**
 * @addtogroup HTTP HTTP
 * @{
 */

/**
 * @defgroup WebSocket WebSocket
 * @brief WebSocket升级协议实现
 *
 * 支持RFC 6455标准。包括WebSocket服务器、会话管理、帧解析、心跳检测等功能。
 *
 * 主要功能：
 * - WebSocket协议握手和升级
 * - 文本/二进制消息收发
 * - 分片消息处理
 * - 心跳检测（Ping/Pong）
 * - 关闭握手
 * - 多路由支持
 * - 广播消息
 * - 线程安全的队列管理
 * @{
 */

/**
 * @enum websocket_status
 * @brief WebSocket关闭状态码
 *
 * 定义WebSocket协议标准关闭码。
 */
enum class websocket_status : uint16_t {
    NORMAL_CLOSURE = 1000,             ///< 正常关闭
    GOING_AWAY = 1001,                 ///< 端点离开
    PROTOCOL_ERROR = 1002,             ///< 协议错误
    UNSUPPORTED_DATA = 1003,           ///< 不支持的数据类型
    RESERVED = 1004,                   ///< 保留
    NO_STATUS_RCVD = 1005,             ///< 未收到状态码
    ABNORMAL_CLOSURE = 1006,           ///< 异常关闭
    INVALID_FRAME_PAYLOAD_DATA = 1007, ///< 无效帧负载
    POLICY_VIOLATION = 1008,           ///< 策略违规
    MESSAGE_TOO_BIG = 1009,            ///< 消息过大
    MANDATORY_EXT = 1010,              ///< 缺少必要扩展
    INTERNAL_ERROR = 1011,             ///< 内部错误
    SERVICE_RESTART = 1012,            ///< 服务重启
    TRY_AGAIN_LATER = 1013,            ///< 稍后重试
    BAD_GATEWAY = 1014,                ///< 错误的网关
    TLS_HANDSHAKE = 1015               ///< TLS握手失败
};

/**
 * @enum websocket_opcode
 * @brief WebSocket帧操作码
 *
 * 定义WebSocket帧的类型。
 */
enum class websocket_opcode : uint8_t {
    CONTINUATION = 0x0, ///< 延续帧
    TEXT = 0x1,         ///< 文本帧
    BINARY = 0x2,       ///< 二进制帧
    CLOSE = 0x8,        ///< 关闭帧
    PING = 0x9,         ///< Ping帧
    PONG = 0xA          ///< Pong帧
};

#pragma pack(push, 1)
/**
 * @struct websocket_frame_header
 * @brief WebSocket帧头部结构
 *
 * 定义WebSocket帧头部的位域布局。
 *
 * @note 基于 LSB→MSB 分配策略。
 */
struct websocket_frame_header {
    byte_t opcode : 4;      ///< 操作码 (bits 0-3)
    byte_t rsv3 : 1;        ///< 保留位3 (bit 4)
    byte_t rsv2 : 1;        ///< 保留位2 (bit 5)
    byte_t rsv1 : 1;        ///< 保留位1 (bit 6)
    byte_t fin : 1;         ///< 是否最后一帧 (bit 7)
    byte_t payload_len : 7; ///< 负载长度 (bits 0-6)
    byte_t masked : 1;      ///< 是否掩码 (bit 7)
};
#pragma pack(pop)

class websocket_session;


/**
 * @class websocket_server
 * @brief WebSocket服务器类
 *
 * 管理WebSocket路由和会话，处理协议升级请求。
 *
 * 使用示例：
 * @code
 * websocket_server ws_server;
 * ws_server.route("/chat", [](websocket_server::session_ptr session) {
 *     session->set_message_handler([](const string& msg, websocket_opcode opcode) {
 *         println("Received: ", msg);
 *         session->send("Echo: " + msg);
 *     });
 * });
 *
 * // 在HTTP服务器中处理升级请求
 * if (request.method == http_method::GET() && request.header("Upgrade") == "websocket") {
 *     if (ws_server.handle_upgrade(request, move(socket))) {
 *         return; // 连接已升级为WebSocket
 *     }
 * }
 * @endcode
 */
class NEFORCE_API websocket_server {
public:
    using session_ptr = shared_ptr<websocket_session>;   ///< 会话智能指针类型
    using session_handler = function<void(session_ptr)>; ///< 会话处理器类型

private:
    unordered_map<string, session_handler> route_handlers_; ///< 路由处理器映射
    vector<session_ptr> sessions_;                          ///< 所有活动会话
    mutable mutex sessions_mutex_;                          ///< 会话列表互斥锁

public:
    websocket_server() = default;
    ~websocket_server() = default;

    websocket_server(const websocket_server&) = delete;
    websocket_server& operator=(const websocket_server&) = delete;

    websocket_server(websocket_server&&) noexcept = delete;
    websocket_server& operator=(websocket_server&&) noexcept = delete;

    /**
     * @brief 注册WebSocket路由
     * @param path 路由路径
     * @param handler 会话处理器
     */
    void route(const string& path, session_handler handler) { route_handlers_[path] = _NEFORCE move(handler); }

    /**
     * @brief 处理WebSocket升级请求
     * @param request HTTP升级请求
     * @param sock 已建立的TCP/SSL socket
     * @return 成功创建会话返回true，未找到对应路由返回false
     */
    bool handle_upgrade(const http_request& request, unique_ptr<tcp_socket> sock);

    /**
     * @brief 移除会话
     * @param session 要移除的会话
     */
    void remove_session(const session_ptr& session);

    /**
     * @brief 向所有会话广播消息
     * @param data 消息数据
     * @param opcode 操作码（默认TEXT）
     */
    void broadcast(const string& data, websocket_opcode opcode = websocket_opcode::TEXT);

    /**
     * @brief 获取活动会话数量
     * @return 会话数量
     */
    size_t session_count() const noexcept {
        lock<mutex> lk(sessions_mutex_);
        return sessions_.size();
    }
};

/**
 * @class websocket_session
 * @brief WebSocket会话类
 *
 * 表示单个WebSocket连接，负责帧的解析、发送、心跳和生命周期管理。
 *
 * 使用示例：
 * @code
 * auto session = make_shared<websocket_session>(move(sock));
 * session->set_message_handler([](const string& msg, websocket_opcode opcode) {
 *     if (opcode == websocket_opcode::TEXT) {
 *         println("Text: ", msg);
 *     } else {
 *         // 处理二进制消息
 *     }
 * });
 * session->set_close_handler([](websocket_status status, const string& reason) {
 *     println("Closed: ", static_cast<int>(status), " - ", reason);
 * });
 * session->start();
 * @endcode
 */
class NEFORCE_API websocket_session : public enable_shared_from_this<websocket_session> {
public:
    using message_handler = function<void(const string&, websocket_opcode)>; ///< 消息处理器类型
    using close_handler = function<void(websocket_status, const string&)>;   ///< 关闭处理器类型
    using error_handler = function<void(const exception&)>;                  ///< 错误处理器类型

private:
    unique_ptr<tcp_socket> socket_; ///< 底层socket
    websocket_server* server_;      ///< 所属服务器

    atomic<bool> running_{false}; ///< 运行标志
    atomic_flag closed_once_;     ///< 防止重复关闭

    thread read_thread_;      ///< 读线程
    thread write_thread_;     ///< 写线程
    thread heartbeat_thread_; ///< 心跳线程

    mutex write_mutex_;              ///< 写队列互斥锁
    condition_variable write_cv_;    ///< 写条件变量
    queue<byte_vector> write_queue_; ///< 普通消息队列
    queue<byte_vector> ctrl_queue_;  ///< 控制帧队列（Ping/Pong/Close）

    string fragment_buffer_;                                    ///< 分片缓冲区
    websocket_opcode fragment_opcode_ = websocket_opcode::TEXT; ///< 当前分片类型
    bool in_fragment_ = false;                                  ///< 是否正在接收分片

    atomic<bool> ping_pending_{false}; ///< 是否有待响应的Ping
    atomic<int64_t> last_pong_ms_{0};  ///< 最后收到Pong的时间

    message_handler on_message_; ///< 消息回调
    close_handler on_close_;     ///< 关闭回调
    error_handler on_error_;     ///< 错误回调

    bool queue_frame(byte_vector frame, bool is_control = false);
    void write_loop();

    void read_loop();
    bool read_frame();

    bool dispatch(const websocket_frame_header& hdr, websocket_opcode opcode, string payload);
    void deliver_message(const string& data, websocket_opcode opcode);

    void send_close_frame(websocket_status status, const string& reason);
    void handle_close_frame(string payload);

    void heartbeat_loop();

    void do_stop(websocket_status status, const string& reason);

public:
    /**
     * @brief 构造函数
     * @param sock TCP/SSL socket
     * @param server 所属服务器
     */
    explicit websocket_session(unique_ptr<tcp_socket> sock, websocket_server* server = nullptr);

    /**
     * @brief 析构函数
     */
    ~websocket_session();

    websocket_session(const websocket_session&) = delete;
    websocket_session& operator=(const websocket_session&) = delete;

    /**
     * @brief 启动会话
     *
     * 启动读、写、心跳三个线程。
     */
    void start();

    /**
     * @brief 关闭连接
     * @param status 关闭状态码（默认NORMAL_CLOSURE）
     * @param reason 关闭原因
     */
    void close(websocket_status status = websocket_status::NORMAL_CLOSURE, const string& reason = "");

    /**
     * @brief 停止会话
     */
    void stop();

    /**
     * @brief 发送文本/二进制消息
     * @param data 消息数据
     * @param opcode 操作码（默认TEXT）
     * @return 发送成功返回true
     */
    bool send(const string& data, websocket_opcode opcode = websocket_opcode::TEXT);

    /**
     * @brief 发送二进制消息
     * @param data 消息数据
     * @return 发送成功返回true
     */
    bool send_binary(const string& data) { return send(data, websocket_opcode::BINARY); }

    /**
     * @brief 检查连接是否开启
     * @return 开启返回true
     */
    bool is_open() const noexcept { return running_ && socket_->is_open(); }

    /**
     * @brief 设置消息处理器
     * @param handler 处理函数
     */
    void set_message_handler(message_handler handler) { on_message_ = _NEFORCE move(handler); }

    /**
     * @brief 设置关闭处理器
     * @param handler 处理函数
     */
    void set_close_handler(close_handler handler) { on_close_ = _NEFORCE move(handler); }

    /**
     * @brief 设置错误处理器
     * @param handler 处理函数
     */
    void set_error_handler(error_handler handler) { on_error_ = _NEFORCE move(handler); }

    /**
     * @brief 获取底层socket引用
     * @return socket引用
     */
    tcp_socket& socket() noexcept { return *socket_; }

    /**
     * @brief 获取底层socket常量引用
     * @return socket常量引用
     */
    const tcp_socket& socket() const noexcept { return *socket_; }

    /**
     * @brief 获取SSL socket指针
     * @return SSL socket指针，若非SSL连接返回nullptr
     */
    ssl_socket* ssl_socket_ptr() noexcept { return dynamic_cast<ssl_socket*>(socket_.get()); }

    /**
     * @brief 获取SSL socket常量指针
     * @return SSL socket常量指针，若非SSL连接返回nullptr
     */
    const ssl_socket* ssl_socket_ptr() const noexcept { return dynamic_cast<const ssl_socket*>(socket_.get()); }
};

/** @} */ // WebSocket

/** @} */ // HTTP

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_HTTP_WEBSOCKET_HPP__
