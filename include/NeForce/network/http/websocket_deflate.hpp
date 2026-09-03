#ifndef NEFORCE_NETWORK_HTTP_WEBSOCKET_DEFLATE_HPP__
#define NEFORCE_NETWORK_HTTP_WEBSOCKET_DEFLATE_HPP__

/**
 * @file websocket_deflate.hpp
 * @brief WebSocket permessage-deflate 扩展
 *
 * 实现 WebSocket 消息的 per-message deflate 压缩/解压。
 * 支持窗口比特位协商和上下文接管控制。
 */

#include "NeForce/core/string/string.hpp"
#ifdef NEFORCE_SUPPORT_ZLIB
#    include "NeForce/core/memory/unique_ptr.hpp"
#    include "NeForce/compress/zlib_compress.hpp"
#endif
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

/**
 * @addtogroup HTTP HTTP
 * @{
 */

/**
 * @class websocket_deflate_config
 * @brief permessage-deflate 扩展配置
 *
 * RFC 7692 §7 定义的扩展参数。
 */
struct NEFORCE_API websocket_deflate_config {
    /** @brief 客户端→服务端压缩窗口比特位 (8-15)，默认15 */
    int client_max_window_bits = 15;

    /** @brief 服务端→客户端压缩窗口比特位 (8-15)，默认15 */
    int server_max_window_bits = 15;

    /** @brief 客户端是否禁用上下文接管（逐消息重置压缩器） */
    bool client_no_context_takeover = false;

    /** @brief 服务端是否禁用上下文接管（逐消息重置压缩器） */
    bool server_no_context_takeover = false;

    /** @brief 是否有有效的协商配置 */
    bool active = false;

    /**
     * @brief 从请求的 Sec-WebSocket-Extensions 头部解析并协商
     * @param request_extensions 客户端请求的扩展参数
     * @return 协商后的配置
     */
    static websocket_deflate_config negotiate(string_view request_extensions);

    /**
     * @brief 生成服务端响应头部值
     * @return Sec-WebSocket-Extensions 响应值，未激活返回空
     */
    NEFORCE_NODISCARD string to_response_header() const;
};

#ifdef NEFORCE_SUPPORT_ZLIB

/**
 * @class websocket_deflate
 * @brief per-message deflate 压缩/解压器
 *
 * 每个 WebSocket 会话持有两个实例：
 * - 一个用于发送方向（压缩）
 * - 一个用于接收方向（解压）
 *
 * 支持上下文接管优化（跨消息复用 zlib 流状态）。
 */
class NEFORCE_API websocket_deflate {
private:
    /// @brief true=压缩模式，false=解压模式
    bool compress_mode_;
    /// @brief LZ77 窗口比特位（8-15）
    int window_bits_;
    /// @brief 每条消息后是否重置上下文
    bool no_context_takeover_;
    /// @brief 是否已初始化 zlib 流
    bool initialized_ = false;
    /// @brief 流是否已完成（final 帧之后）
    bool stream_finished_ = false;

    unique_ptr<zlib_compressor::stream_compressor> compressor_;
    unique_ptr<zlib_compressor::stream_decompressor> decompressor_;

    void ensure_initialized();

public:
    /**
     * @brief 构造压缩/解压器
     * @param compress true=压缩模式, false=解压模式
     * @param window_bits 窗口比特位 (8-15)
     * @param no_context_takeover 每消息结束后是否重置上下文
     */
    websocket_deflate(bool compress, int window_bits, bool no_context_takeover);

    ~websocket_deflate() = default;

    websocket_deflate(const websocket_deflate&) = delete;
    websocket_deflate& operator=(const websocket_deflate&) = delete;
    websocket_deflate(websocket_deflate&&) noexcept;
    websocket_deflate& operator=(websocket_deflate&&) noexcept;

    /**
     * @brief 处理消息（压缩或解压）
     * @param data 输入数据
     * @param is_final 是否最终片段
     * @return 处理后的数据
     */
    string process(string_view data, bool is_final);

    /**
     * @brief 重置上下文（新消息开始时调用）
     */
    void reset_context();
};

#else // !NEFORCE_SUPPORT_ZLIB

/**
 * @class websocket_deflate
 * @brief zlib 不可用时的桩实现
 *
 * 当 NEFORCE_SUPPORT_ZLIB 未定义时，permessage-deflate 无法协商。
 * 此类作为占位符，保证代码可编译，实际不会被使用。
 */
class websocket_deflate {
public:
    websocket_deflate(bool, int, bool) {}
    ~websocket_deflate() = default;
    websocket_deflate(const websocket_deflate&) = delete;
    websocket_deflate& operator=(const websocket_deflate&) = delete;
    websocket_deflate(websocket_deflate&&) noexcept {}
    websocket_deflate& operator=(websocket_deflate&&) noexcept { return *this; }

    /// @brief zlib 不可用时原样返回数据
    string process(string_view data, bool) { return {data}; }

    /// @brief zlib 不可用时无操作
    void reset_context() {}
};

#endif // NEFORCE_SUPPORT_ZLIB

/** @} */ // HTTP

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_HTTP_WEBSOCKET_DEFLATE_HPP__
