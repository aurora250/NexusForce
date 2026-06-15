#ifndef NEFORCE_NETWORK_HTTP_GRPC_HPP__
#define NEFORCE_NETWORK_HTTP_GRPC_HPP__

/**
 * @file grpc.hpp
 * @brief gRPC 协议支持（基于 HTTP/2）
 */

#include "NeForce/core/functional/function.hpp"
#include "NeForce/core/utility/byte_size.hpp"
#include "NeForce/network/http/http_server_message.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

/**
 * @defgroup gRPC gRPC支持
 * @brief gRPC协议支持（基于HTTP/2）
 *
 * gRPC over HTTP/2 协议栈：
 * - Content-Type: application/grpc[+proto]
 * - 消息帧格式: [压缩标志(1字节)] [长度(4字节BE)] [消息体]
 * - 响应尾部: grpc-status, grpc-message
 *
 * @section standards 遵循的国际标准
 * 本实现严格遵循以下 gRPC 协议规范与相关标准：
 *
 * **gRPC 协议核心：**
 * - **gRPC over HTTP/2**：gRPC 协议规范（基于 HTTP/2 传输）
 *   https://github.com/grpc/grpc/blob/master/doc/PROTOCOL-HTTP2.md
 * - **gRPC Health Checking Protocol**：gRPC 健康检查协议
 *   https://github.com/grpc/grpc/blob/master/doc/health-checking.md
 * - **Protocol Buffers**：结构化数据序列化格式
 *   https://protobuf.dev/
 *
 * **底层 HTTP/2 传输：**
 * - **IETF RFC 7540**：HTTP/2（帧层、流、连接管理）
 *   https://www.rfc-editor.org/rfc/rfc7540.html
 * - **IETF RFC 7541**：HPACK（HTTP/2 头部压缩算法）
 *   https://www.rfc-editor.org/rfc/rfc7541.html
 *
 * @section grpc_status_codes gRPC 状态码说明
 * 根据 gRPC 协议规范，gRPC 状态码用于表示 RPC 调用的结果：
 *
 * | 类别   | 状态码                       | 含义               |
 * |--------|------------------------------|--------------------|
 * | OK     | 0                            | 调用成功           |
 * | 客户端 | 1-9 (CANCELLED → NOT_FOUND) | 请求无效或不符合预期 |
 * | 系统   | 10-15 (ABORTED → DATA_LOSS) | 服务端内部处理失败 |
 * | 认证   | 16 (UNAUTHENTICATED)         | 缺少有效认证凭据   |
 * @{
 */

NEFORCE_INLINE17 constexpr string_view GRPC_CONTENT_TYPE = "application/grpc";
NEFORCE_INLINE17 constexpr string_view GRPC_CONTENT_TYPE_PROTO = "application/grpc+proto";

/**
 * @brief gRPC 状态码
 */
enum class grpc_status : uint8_t {
    OK = 0,
    CANCELLED = 1,
    UNKNOWN = 2,
    INVALID_ARGUMENT = 3,
    DEADLINE_EXCEEDED = 4,
    NOT_FOUND = 5,
    ALREADY_EXISTS = 6,
    PERMISSION_DENIED = 7,
    RESOURCE_EXHAUSTED = 8,
    FAILED_PRECONDITION = 9,
    ABORTED = 10,
    OUT_OF_RANGE = 11,
    UNIMPLEMENTED = 12,
    INTERNAL = 13,
    UNAVAILABLE = 14,
    DATA_LOSS = 15,
    UNAUTHENTICATED = 16,
};

/**
 * @struct grpc_message
 * @brief gRPC 消息帧
 *
 * gRPC消息格式（RFC标准）：
 *   [1 byte: compressed-flag (0=uncompressed, 1=compressed)]
 *   [4 bytes: message-length (big-endian)]
 *   [message-length bytes: payload]
 */
struct grpc_message {
    bool compressed{false};
    byte_vector payload;

    grpc_message() = default;
    grpc_message(byte_vector data, bool comp = false) :
    compressed(comp),
    payload(move(data)) {}
};

/**
 * @class grpc_framer
 * @brief gRPC消息帧编解码器
 *
 * 线程安全的消息帧编码/解码。
 */
class NEFORCE_API grpc_framer {
public:
    /// 最大接收消息大小（默认4MB）
    byte_size max_receive_size{4_MB};

    grpc_framer() = default;

    /**
     * @brief 编码消息为gRPC帧格式的字节块
     */
    static byte_vector encode(const grpc_message& msg);

    /**
     * @brief 编码多个消息（连续写入同一buffer）
     */
    static byte_vector encode_messages(const vector<grpc_message>& messages);

    /**
     * @brief 从字节块解码gRPC帧
     * @param data 输入字节块
     * @param len 输入长度
     * @param out 输出消息列表
     * @return 成功解码的消息数量，-1表示帧格式错误
     *
     * 一次调用可解码多个连续帧。
     */
    int decode(const byte_t* data, size_t len, vector<grpc_message>& out);
};

/**
 * @class grpc_handler
 * @brief gRPC服务处理器基类
 *
 * 基于HTTP请求/响应模型实现gRPC unary调用。
 * 将gRPC帧解码为请求消息，执行处理逻辑，编码响应消息。
 *
 * 使用示例：
 * @code
 * class MyService : public grpc_handler {
 *     grpc_message handle(const grpc_message& request) override {
 *         string response = "Hello " + string(request.payload);
 *         return grpc_message(byte_vector(response.begin(), response.end()));
 *     }
 * };
 *
 * // 在HTTP路由中使用
 * router.post("/my.package.MyService/MyMethod", [&](http_request& req, http_response& res) {
 *     MyService svc;
 *     svc.process_unary(req, res);
 * });
 * @endcode
 */
class NEFORCE_API grpc_handler {
public:
    using unary_handler = function<grpc_message(const grpc_message&)>;
    using stream_handler = function<void(const grpc_message&, function<void(grpc_message)>)>;

    grpc_handler() = default;
    virtual ~grpc_handler() = default;

    /**
     * @brief 处理一元(unary) gRPC调用
     *
     * 解码请求帧 -> 调用 handler -> 编码响应帧
     */
    void process_unary(http_request& request, http_response& response, unary_handler handler);

    /**
     * @brief 发送gRPC错误响应
     */
    static void send_error(http_response& response, grpc_status status, const string& message);

    /**
     * @brief 将gRPC状态映射到HTTP状态码
     */
    static http_status grpc_to_http_status(grpc_status status) noexcept;

private:
    grpc_framer framer_;
};

// TODO: Client streaming support — implement request-streaming handler with backpressure and flow control
// TODO: Server streaming support — implement response-streaming handler with chunked gRPC frames
// TODO: Bidirectional streaming support — full duplex bidi streaming with concurrent read/write channels
// TODO: Protobuf integration — code generation from .proto files, typed request/response wrappers (beyond raw byte_vector)
// TODO: gRPC-Web support — gRPC-Web protocol translation for browser-based gRPC clients (text and binary modes)
// TODO: gRPC interceptor chain — unary/stream interceptor stack for logging, auth, metrics, tracing

/** @} */ // gRPC

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_HTTP_GRPC_HPP__
