#ifndef NEFORCE_NETWORK_HTTP_HTTP2_PROTOCOL_HPP__
#define NEFORCE_NETWORK_HTTP_HTTP2_PROTOCOL_HPP__

/**
 * @file http2_protocol.hpp
 * @brief HTTP/2 协议帧、HPACK 编解码器与流控制
 *
 * 实现 RFC 7540（HTTP/2）帧格式、RFC 7541（HPACK）头部压缩、
 * 流状态机和连接/流级流量控制。
 */

#include "NeForce/core/container/unordered_map.hpp"
#include "NeForce/core/functional/function.hpp"
#include "NeForce/core/string/string.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

/**
 * @addtogroup HTTP HTTP
 * @{
 */

/// @brief HTTP/2 最大帧负载大小（RFC 7540 §4.1）
NEFORCE_INLINE17 constexpr uint32_t HTTP2_MAX_FRAME_SIZE = 16384;
/// @brief HPACK 默认动态表大小
NEFORCE_INLINE17 constexpr uint32_t HTTP2_DEFAULT_HEADER_TABLE_SIZE = 4096;
/// @brief 默认最大并发流数
NEFORCE_INLINE17 constexpr uint32_t HTTP2_DEFAULT_MAX_CONCURRENT_STREAMS = 100;
/// @brief 默认初始流控窗口
NEFORCE_INLINE17 constexpr uint32_t HTTP2_DEFAULT_INITIAL_WINDOW_SIZE = 65535;
/// @brief 默认最大头部列表大小
NEFORCE_INLINE17 constexpr uint32_t HTTP2_DEFAULT_MAX_HEADER_LIST_SIZE = 0xFFFFFFFF;
/// @brief HPACK 静态表条目数
NEFORCE_INLINE17 constexpr size_t HPACK_STATIC_TABLE_SIZE = 61;
/// @brief HTTP/2 客户端连接前言（RFC 7540 §3.5）
NEFORCE_INLINE17 constexpr string_view HTTP2_CLIENT_PREFACE = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";

class byte_cursor;

/**
 * @brief HTTP/2 帧类型（RFC 7540 §6）
 */
enum class http2_frame_type : uint8_t {
    DATA = 0x0,          ///< 请求/响应体数据
    HEADERS = 0x1,       ///< 头部块
    PRIORITY = 0x2,      ///< 流优先级
    RST_STREAM = 0x3,    ///< 流终止
    SETTINGS = 0x4,      ///< 连接配置参数
    PUSH_PROMISE = 0x5,  ///< 服务端推送承诺
    PING = 0x6,          ///< 连接活性检测
    GOAWAY = 0x7,        ///< 优雅关闭连接
    WINDOW_UPDATE = 0x8, ///< 流控窗口更新
    CONTINUATION = 0x9,  ///< 头部块续帧
};

/**
 * @brief HTTP/2 错误码（RFC 7540 §7）
 */
enum class http2_error : uint32_t {
    NO_ERROR = 0x0,            ///< 无错误
    PROTOCOL_ERROR = 0x1,      ///< 协议错误
    INTERNAL_ERROR = 0x2,      ///< 内部错误
    FLOW_CONTROL_ERROR = 0x3,  ///< 流控违规
    SETTINGS_TIMEOUT = 0x4,    ///< SETTINGS 确认超时
    STREAM_CLOSED = 0x5,       ///< 流已关闭
    FRAME_SIZE_ERROR = 0x6,    ///< 帧大小错误
    REFUSED_STREAM = 0x7,      ///< 流被拒绝
    CANCEL = 0x8,              ///< 流取消
    COMPRESSION_ERROR = 0x9,   ///< 压缩错误
    CONNECT_ERROR = 0xA,       ///< CONNECT 错误
    ENHANCE_YOUR_CALM = 0xB,   ///< 速率过高
    INADEQUATE_SECURITY = 0xC, ///< 安全不足
    HTTP_1_1_REQUIRED = 0xD,   ///< 需要 HTTP/1.1
};

/**
 * @brief HTTP/2 SETTINGS 参数标识符（RFC 7540 §6.5.2）
 */
enum class http2_settings_id : uint16_t {
    HEADER_TABLE_SIZE = 0x1,      ///< HPACK 动态表最大大小
    ENABLE_PUSH = 0x2,            ///< 是否启用服务端推送
    MAX_CONCURRENT_STREAMS = 0x3, ///< 最大并发流数
    INITIAL_WINDOW_SIZE = 0x4,    ///< 流级初始窗口大小
    MAX_FRAME_SIZE = 0x5,         ///< 最大帧负载大小
    MAX_HEADER_LIST_SIZE = 0x6,   ///< 最大头部列表大小
};


/**
 * @struct http2_frame_header
 * @brief HTTP/2 帧头（RFC 7540 §4.1），9 字节 packed 布局
 *
 * Byte 0-2: Length (24-bit big-endian)
 * Byte 3:   Type
 * Byte 4:   Flags
 * Byte 5-8: R(1) + Stream Identifier (31-bit big-endian)
 */
#pragma pack(push, 1)
struct http2_frame_header {
    uint8_t length_hi = 0;  ///< Length[23:16]
    uint8_t length_mid = 0; ///< Length[15:8]
    uint8_t length_lo = 0;  ///< Length[7:0]
    uint8_t type = 0;       ///< Frame type
    uint8_t flags = 0;      ///< Frame flags
    uint8_t sid_r = 0;      ///< R(1) | StreamID[30:24]
    uint8_t sid_mid = 0;    ///< StreamID[23:16]
    uint8_t sid_lo1 = 0;    ///< StreamID[15:8]
    uint8_t sid_lo0 = 0;    ///< StreamID[7:0]

    NEFORCE_NODISCARD uint32_t get_length() const noexcept {
        return (static_cast<uint32_t>(length_hi) << 16) | (static_cast<uint32_t>(length_mid) << 8) |
               static_cast<uint32_t>(length_lo);
    }

    void set_length(uint32_t len) noexcept {
        length_hi = static_cast<uint8_t>((len >> 16) & 0xFF);
        length_mid = static_cast<uint8_t>((len >> 8) & 0xFF);
        length_lo = static_cast<uint8_t>(len & 0xFF);
    }

    NEFORCE_NODISCARD uint32_t get_stream_id() const noexcept {
        return (static_cast<uint32_t>(sid_r & 0x7F) << 24) | (static_cast<uint32_t>(sid_mid) << 16) |
               (static_cast<uint32_t>(sid_lo1) << 8) | static_cast<uint32_t>(sid_lo0);
    }

    void set_stream_id(uint32_t id) noexcept {
        sid_r = static_cast<uint8_t>((id >> 24) & 0x7F);
        sid_mid = static_cast<uint8_t>((id >> 16) & 0xFF);
        sid_lo1 = static_cast<uint8_t>((id >> 8) & 0xFF);
        sid_lo0 = static_cast<uint8_t>(id & 0xFF);
    }
};
#pragma pack(pop)


NEFORCE_INLINE17 constexpr uint8_t HTTP2_FLAG_END_STREAM = 0x1;
NEFORCE_INLINE17 constexpr uint8_t HTTP2_FLAG_END_HEADERS = 0x4;
NEFORCE_INLINE17 constexpr uint8_t HTTP2_FLAG_PADDED = 0x8;
NEFORCE_INLINE17 constexpr uint8_t HTTP2_FLAG_PRIORITY = 0x20;
NEFORCE_INLINE17 constexpr uint8_t HTTP2_FLAG_ACK = 0x1;

/**
 * @brief SETTINGS 帧中的单个参数条目
 */
struct http2_settings_entry {
    http2_settings_id id; ///< 参数标识符
    uint32_t value;       ///< 参数值
};

/**
 * @brief 流优先级信息（RFC 7540 §5.3）
 */
struct http2_priority {
    uint32_t stream_dependency; ///< 依赖的流 ID（0 表示根）
    uint8_t weight;             ///< 权重（1-256）
    bool exclusive;             ///< 是否为独占依赖
};

/**
 * @brief DATA 帧（RFC 7540 §6.1）
 */
struct http2_data_frame {
    uint32_t stream_id;      ///< 流 ID
    byte_vector data;        ///< 负载数据
    uint8_t pad_length = 0;  ///< 填充长度
    bool end_stream = false; ///< 是否结束流
};

/**
 * @brief HEADERS 帧（RFC 7540 §6.2）
 */
struct http2_headers_frame {
    uint32_t stream_id;        ///< 流 ID
    byte_vector header_block;  ///< HPACK 编码的头部块
    uint8_t pad_length = 0;    ///< 填充长度
    http2_priority priority{}; ///< 优先级信息
    bool end_stream = false;   ///< 是否结束流
    bool end_headers = false;  ///< 是否结束头部块
    bool has_priority = false; ///< 是否携带优先级
};

/**
 * @brief RST_STREAM 帧（RFC 7540 §6.4）
 */
struct http2_rst_stream_frame {
    uint32_t stream_id;     ///< 流 ID
    http2_error error_code; ///< 错误码
};

/**
 * @brief SETTINGS 帧（RFC 7540 §6.5）
 */
struct http2_settings_frame {
    bool ack = false;                     ///< 是否为确认帧
    vector<http2_settings_entry> entries; ///< 设置参数列表
};

/**
 * @brief PING 帧（RFC 7540 §6.7）
 */
struct http2_ping_frame {
    uint64_t opaque_data; ///< 不透明数据（回显）
    bool ack = false;     ///< 是否为确认帧
};

/**
 * @brief GOAWAY 帧（RFC 7540 §6.8）
 */
struct http2_goaway_frame {
    uint32_t last_stream_id; ///< 最后处理的流 ID
    http2_error error_code;  ///< 错误码
    byte_vector debug_data;  ///< 调试数据
};

/**
 * @brief WINDOW_UPDATE 帧（RFC 7540 §6.9）
 */
struct http2_window_update_frame {
    uint32_t stream_id;             ///< 流 ID（0 表示连接级）
    uint32_t window_size_increment; ///< 窗口增量
};

/**
 * @brief PUSH_PROMISE 帧（RFC 7540 §6.6）
 */
struct http2_push_promise_frame {
    uint32_t stream_id;          ///< 关联的流 ID（父流）
    uint32_t promised_stream_id; ///< 服务器承诺的新流 ID（必须为偶数）
    byte_vector header_block;    ///< 请求头部块
    uint8_t pad_length = 0;      ///< 填充长度
    bool end_headers = false;    ///< 是否结束头部块
};

/**
 * @brief CONTINUATION 帧（RFC 7540 §6.10）
 */
struct http2_continuation_frame {
    uint32_t stream_id;       ///< 流 ID
    byte_vector header_block; ///< HPACK 编码的续帧头部块
    bool end_headers = false; ///< 是否结束头部块
};

/**
 * @brief HPACK 头部字段（名值对）
 */
struct hpack_header_field {
    string name;  ///< 头部名称（小写）
    string value; ///< 头部值
};


/**
 * @class hpack_encoder
 * @brief HPACK 头部编码器（RFC 7541）
 *
 * 将 HTTP/2 头部字段列表编码为 HPACK 格式的字节块，
 * 支持静态表和动态表的索引优化。
 */
class hpack_encoder {
private:
    struct table_entry {
        string name;
        string value;
        size_t name_len;
        size_t value_len;
    };

    /// @brief 动态表最大大小（由 SETTINGS 帧协商）
    uint32_t max_table_size_;
    /// @brief 当前动态表占用大小
    uint32_t current_table_size_ = 0;
    /// @brief 动态表条目列表
    vector<table_entry> dynamic_table_;

    void add_to_dynamic_table(const string& name, const string& value);
    void evict_for_size(uint32_t needed);
    NEFORCE_NODISCARD int32_t find_in_tables(const string& name, const string& value) const;
    NEFORCE_NODISCARD int32_t find_name_in_tables(const string& name) const;

public:
    explicit hpack_encoder(uint32_t max_table_size = HTTP2_DEFAULT_HEADER_TABLE_SIZE);

    byte_vector encode(const vector<hpack_header_field>& headers);

    void set_max_table_size(uint32_t size);
    NEFORCE_NODISCARD uint32_t max_table_size() const noexcept { return max_table_size_; }
    NEFORCE_NODISCARD size_t table_size() const noexcept { return dynamic_table_.size(); }
};

/**
 * @class hpack_decoder
 * @brief HPACK 头部解码器（RFC 7541）
 *
 * 将 HPACK 编码的字节块解码为 HTTP/2 头部字段列表。
 * 支持增量解码和回调模式。
 */
class hpack_decoder {
public:
    /// @brief 解码回调：void(const string& name, const string& value)
    using header_callback = function<void(const string& name, const string& value)>;

private:
    struct table_entry {
        string name;
        string value;
        size_t name_len;
        size_t value_len;
    };

    /// @brief 动态表最大大小
    uint32_t max_table_size_;
    /// @brief 当前动态表占用大小
    uint32_t current_table_size_ = 0;
    /// @brief 动态表条目列表
    vector<table_entry> dynamic_table_;

    void add_to_dynamic_table(const string& name, const string& value);
    void evict_for_size(uint32_t needed);

public:
    explicit hpack_decoder(uint32_t max_table_size = HTTP2_DEFAULT_HEADER_TABLE_SIZE);

    vector<hpack_header_field> decode(const byte_t* data, size_t len);
    void set_max_table_size(uint32_t size);

    void decode_incremental(byte_cursor& cur, header_callback cb);
};


/**
 * @class http2_framer
 * @brief HTTP/2 帧编解码器
 *
 * 负责 HTTP/2 帧的编码和字节流解析。
 * 支持不完整帧的缓冲和回调式解码。
 */
class http2_framer {
public:
    using frame_callback =
            function<void(http2_frame_type type, uint8_t flags, uint32_t stream_id, const byte_t* payload, size_t len)>;

private:
    /// @brief 不完整帧的缓冲数据
    byte_vector partial_buffer_;
    /// @brief 当前帧期望的负载长度
    size_t expected_length_ = 0;
    /// @brief 当前正在解析的帧头
    http2_frame_header current_header_{};
    /// @brief 是否正在读取帧头（9 字节）
    bool reading_header_ = true;

public:
    static byte_vector encode_data_frame(const http2_data_frame& frame);
    static byte_vector encode_headers_frame(const http2_headers_frame& frame);
    static byte_vector encode_push_promise_frame(const http2_push_promise_frame& frame);
    static byte_vector encode_rst_stream_frame(const http2_rst_stream_frame& frame);
    static byte_vector encode_settings_frame(const http2_settings_frame& frame);
    static byte_vector encode_ping_frame(const http2_ping_frame& frame);
    static byte_vector encode_goaway_frame(const http2_goaway_frame& frame);
    static byte_vector encode_window_update_frame(const http2_window_update_frame& frame);
    static byte_vector encode_continuation_frame(const http2_continuation_frame& frame);

    void decode_frames(const byte_t* data, size_t len, frame_callback cb);
};


/**
 * @brief HTTP/2 流状态（RFC 7540 §5.1）
 */
enum class http2_stream_state {
    IDLE,
    RESERVED_LOCAL,
    RESERVED_REMOTE,
    OPEN,
    HALF_CLOSED_LOCAL,
    HALF_CLOSED_REMOTE,
    CLOSED,
};

/**
 * @class http2_stream
 * @brief HTTP/2 流状态机
 *
 * 追踪单个 HTTP/2 流的生命周期状态和流控窗口。
 */
class http2_stream {
public:
    explicit http2_stream(uint32_t stream_id);

    NEFORCE_NODISCARD uint32_t id() const noexcept { return stream_id_; }
    NEFORCE_NODISCARD http2_stream_state state() const noexcept { return state_; }

    NEFORCE_NODISCARD bool can_send_headers() const;
    NEFORCE_NODISCARD bool can_send_data() const;
    NEFORCE_NODISCARD bool can_receive() const;
    NEFORCE_NODISCARD bool is_closed() const noexcept { return state_ == http2_stream_state::CLOSED; }

    void on_send_headers(bool end_stream);
    void on_send_data(bool end_stream);
    void on_receive_headers(bool end_stream);
    void on_receive_data(bool end_stream);
    void on_send_rst_stream();
    void on_receive_rst_stream();
    void close();

    NEFORCE_NODISCARD uint32_t local_window() const noexcept { return local_window_; }
    NEFORCE_NODISCARD uint32_t remote_window() const noexcept { return remote_window_; }
    void consume_local_window(uint32_t amount);
    void consume_remote_window(uint32_t amount);
    void add_local_window(uint32_t amount);
    void add_remote_window(uint32_t amount);

private:
    /// @brief 流 ID
    uint32_t stream_id_;
    /// @brief 当前流状态
    http2_stream_state state_ = http2_stream_state::IDLE;
    /// @brief 本地流控窗口
    uint32_t local_window_ = HTTP2_DEFAULT_INITIAL_WINDOW_SIZE;
    /// @brief 远端流控窗口
    uint32_t remote_window_ = HTTP2_DEFAULT_INITIAL_WINDOW_SIZE;
};

/**
 * @class http2_flow_control
 * @brief HTTP/2 流量控制器
 *
 * 管理连接级和流级的发送窗口。
 * WINDOW_UPDATE 帧增加窗口，DATA 发送消费窗口。
 */
class http2_flow_control {
public:
    explicit http2_flow_control(uint32_t initial_window = HTTP2_DEFAULT_INITIAL_WINDOW_SIZE);

    void set_initial_window(uint32_t size);

    NEFORCE_NODISCARD bool can_send(uint32_t stream_id, uint32_t amount) const;
    void consume(uint32_t stream_id, uint32_t amount);
    void add_window(uint32_t stream_id, uint32_t amount);
    NEFORCE_NODISCARD uint32_t window(uint32_t stream_id) const;
    NEFORCE_NODISCARD uint32_t connection_window() const noexcept { return connection_window_; }

private:
    /// @brief 新流的初始窗口大小
    uint32_t initial_window_;
    /// @brief 连接级流控窗口
    uint32_t connection_window_;
    /// @brief 流 ID → 流级窗口映射
    unordered_map<uint32_t, uint32_t> stream_windows_;
};


/**
 * @class http2_settings
 * @brief HTTP/2 连接设置管理器
 *
 * 维护本地和远端 SETTINGS 参数值。
 * 支持默认值和远端参数应用时的范围校验。
 */
class http2_settings {
public:
    http2_settings();

    void set(http2_settings_id id, uint32_t value);
    NEFORCE_NODISCARD uint32_t get(http2_settings_id id) const;

    NEFORCE_NODISCARD uint32_t header_table_size() const;
    NEFORCE_NODISCARD bool enable_push() const;
    NEFORCE_NODISCARD uint32_t max_concurrent_streams() const;
    NEFORCE_NODISCARD uint32_t initial_window_size() const;
    NEFORCE_NODISCARD uint32_t max_frame_size() const;
    NEFORCE_NODISCARD uint32_t max_header_list_size() const;

    void apply_remote_settings(const http2_settings_frame& frame);

    static http2_settings defaults() { return {}; }

private:
    /// @brief SETTINGS 参数映射（id → value）
    unordered_map<uint16_t, uint32_t> params_;
};

/** @} */ // HTTP

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_HTTP_HTTP2_PROTOCOL_HPP__
