#ifndef NEFORCE_NETWORK_HTTP_HTTP2_CONNECTION_HPP__
#define NEFORCE_NETWORK_HTTP_HTTP2_CONNECTION_HPP__

/**
 * @file http2_connection.hpp
 * @brief HTTP/2 连接管理器
 *
 * 管理单个 HTTP/2 连接的生命周期，包括帧 I/O、HPACK 编解码、
 * 流状态管理、流量控制以及与 http_router 的集成。
 * 使用 event_loop 驱动异步 I/O。
 */

#include "NeForce/core/async/atomic.hpp"
#include "NeForce/core/async/event_loop.hpp"
#include "NeForce/core/memory/weak_ptr.hpp"
#include "NeForce/network/http/http2_protocol.hpp"
#include "NeForce/network/http/http_router.hpp"
#include "NeForce/network/tcp/tcp_socket.hpp"

#ifdef NO_ERROR
#    undef NO_ERROR
#endif

NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

/**
 * @addtogroup HTTP HTTP
 * @{
 */

/**
 * @class http2_connection
 * @brief HTTP/2 连接管理器
 *
 * 管理单个 HTTP/2 连接的生命周期：
 * - Frame I/O（通过 tcp_socket）
 * - HPACK 编码/解码
 * - Stream 状态管理
 * - Flow Control
 * - 与 http_router 集成
 *
 * 使用 event_loop 驱动异步 I/O。
 * 继承 enable_shared_from_this 以支持异步回调生命周期安全。
 */
class http2_connection : public enable_shared_from_this<http2_connection> {
public:
    using stream_handler = function<void(uint32_t stream_id, const vector<hpack_header_field>& headers,
                                         const byte_t* data, size_t data_len, bool end_stream)>;
    using close_handler = function<void(uint32_t error_code)>;

private:
    unique_ptr<tcp_socket> socket_;
    shared_ptr<event_loop> loop_;

    http2_framer framer_;
    http2_settings local_settings_;
    http2_settings remote_settings_;
    hpack_encoder encoder_;
    hpack_decoder decoder_;
    http2_flow_control flow_control_;

    unordered_map<uint32_t, unique_ptr<http2_stream>> streams_;
    uint32_t last_stream_id_ = 0;
    atomic<bool> closed_{false};
    bool preface_received_ = false;

    byte_vector read_buffer_;
    byte_vector write_buffer_;
    mutex write_mutex_;
    mutable recursive_mutex stream_mutex_; ///< 保护 streams_/pending_/stream_priorities_ 等流状态

    uint32_t connection_consumed_ = 0;                   ///< 连接级已消费窗口（用于 WINDOW_UPDATE 批量发送）
    unordered_map<uint32_t, uint32_t> stream_consumed_;  ///< 每流已消费窗口
    unordered_map<uint32_t, uint8_t> stream_priorities_; ///< 每流优先级权重
    uint32_t local_stream_window_ = 65535;               ///< 本地流初始窗口

    stream_handler stream_handler_;
    close_handler close_handler_;
    http_router* router_ = nullptr;

    struct pending_stream {
        vector<hpack_header_field> headers;
        byte_vector header_block_fragment;
        byte_vector data;
        bool end_stream = false;
        bool waiting_continuation = false;
    };

    unordered_map<uint32_t, pending_stream> pending_;

    void route_stream(uint32_t stream_id, const vector<hpack_header_field>& headers, const byte_t* data,
                      size_t data_len, bool end_stream);

    void on_readable(int fd, uint32_t events);
    void on_writable(int fd, uint32_t events);

    void handle_frame(http2_frame_type type, uint8_t flags, uint32_t stream_id, const byte_t* payload, size_t len);

    void handle_data_frame(uint8_t flags, uint32_t stream_id, const byte_t* payload, size_t len);
    void handle_headers_frame(uint8_t flags, uint32_t stream_id, const byte_t* payload, size_t len);
    void handle_continuation_frame(uint8_t flags, uint32_t stream_id, const byte_t* payload, size_t len);
    void handle_priority_frame(uint32_t stream_id, const byte_t* payload, size_t len);
    void handle_push_promise_frame(uint32_t stream_id, const byte_t* payload, size_t len);
    void handle_settings_frame(uint8_t flags, const byte_t* payload, size_t len);
    void handle_ping_frame(uint8_t flags, const byte_t* payload, size_t len);
    void handle_goaway_frame(const byte_t* payload, size_t len);
    void handle_rst_stream_frame(uint32_t stream_id, const byte_t* payload);
    void handle_window_update_frame(uint32_t stream_id, const byte_t* payload);
    void send_window_update(uint32_t stream_id, uint32_t increment);

    void write_frame(const byte_vector& frame);
    void flush_writes();
    void close_connection(http2_error error = http2_error::NO_ERROR);

public:
    http2_connection(unique_ptr<tcp_socket> socket, shared_ptr<event_loop> loop);
    ~http2_connection();

    http2_connection(const http2_connection&) = delete;
    http2_connection& operator=(const http2_connection&) = delete;

    /**
     * @brief 启动连接处理
     *
     * 发送初始 SETTINGS 帧并开始读取客户端帧。
     */
    void start();

    /**
     * @brief 发送 HTTP 响应（HEADERS + DATA）
     * @param stream_id 流ID
     * @param headers 响应头部
     * @param body 响应体（可选）
     * @param end_stream 是否结束流
     */
    void send_response(uint32_t stream_id, const vector<hpack_header_field>& headers, const string& body = {},
                       bool end_stream = true);

    /**
     * @brief 发送 PUSH_PROMISE（Server Push）
     * @param stream_id 父流ID
     * @param promised_stream_id 新承诺的流ID（必须为偶数，服务器发起）
     * @param request_headers 承诺的请求头部
     *
     * 向客户端承诺将推送指定资源，客户端可以选择接受或通过 RST_STREAM 拒绝。
     * 必须在发送任何可能触发新资源请求的响应数据之前调用。
     */
    void send_push_promise(uint32_t stream_id, uint32_t promised_stream_id,
                           const vector<hpack_header_field>& request_headers);

    /**
     * @brief 发送 RST_STREAM
     */
    void send_rst_stream(uint32_t stream_id, http2_error error);

    /**
     * @brief 发送 GOAWAY 并关闭连接
     */
    void send_goaway(http2_error error = http2_error::NO_ERROR);

    /**
     * @brief h2c 升级请求注入（RFC 7540 §3.2）
     *
     * 将 HTTP/1.1 Upgrade 前的原始请求作为 HTTP/2 stream 1 注入路由器。
     * 必须在 start() 之后、事件循环启动前调用。
     */
    void handle_upgrade_request(uint32_t stream_id, const vector<hpack_header_field>& headers, const byte_t* data,
                                size_t data_len, bool end_stream);

    /**
     * @brief 设置流处理器（收到完整请求时回调）
     */
    void set_stream_handler(stream_handler handler) { stream_handler_ = move(handler); }

    /**
     * @brief 设置路由器（启用自动请求分发）
     *
     * 设置后，收到 HTTP/2 请求时将自动转换为 http_request，
     * 通过路由器分发，并将响应转回 HTTP/2 帧发送。
     */
    void set_router(http_router* router) { router_ = router; }

    /**
     * @brief 设置关闭回调
     */
    void set_close_handler(close_handler handler) { close_handler_ = move(handler); }

    /**
     * @brief 获取本地设置
     */
    http2_settings& local_settings() noexcept { return local_settings_; }

    /**
     * @brief 获取远程设置
     */
    const http2_settings& remote_settings() const noexcept { return remote_settings_; }

    bool is_closed() const noexcept { return closed_; }
    uint32_t last_stream_id() const noexcept { return last_stream_id_; }

    void stop();
};

/** @} */ // HTTP

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_HTTP_HTTP2_CONNECTION_HPP__
