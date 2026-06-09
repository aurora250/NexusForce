#ifndef NEFORCE_NETWORK_HTTP_CHUNKED_READER_HPP__
#define NEFORCE_NETWORK_HTTP_CHUNKED_READER_HPP__

/**
 * @file chunked_reader.hpp
 * @brief HTTP 分块传输编码（Chunked Transfer Encoding）读取器
 */

#include "NeForce/core/container/unordered_map.hpp"
#include "NeForce/core/utility/byte_size.hpp"
#include "NeForce/network/tcp/tcp_socket.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

/**
 * @addtogroup HTTP HTTP
 * @{
 */

/**
 * @class chunked_body_reader
 * @brief HTTP Chunked Transfer Encoding 读取器
 *
 * 按RFC 7230第4.1节实现分块传输解码。
 * 从socket逐块读取数据，处理chunk-size、chunk-data、trailer。
 *
 * 使用示例：
 * @code
 * chunked_body_reader reader(socket);
 * byte_vector chunk;
 * while (reader.next_chunk(chunk)) {
 *     process(chunk);
 * }
 * // 读取trailer
 * auto trailers = reader.trailers();
 * @endcode
 */
class chunked_body_reader {
public:
    byte_size max_chunk_size{1_MB};   ///< 单chunk最大大小
    byte_size max_total_size{100_MB}; ///< 总请求体最大大小
    milliseconds read_timeout{30000}; ///< 读取超时

    explicit chunked_body_reader(tcp_socket& socket) :
    socket_(&socket) {}

    /**
     * @brief 读取下一个chunk
     * @param out 输出缓冲区
     * @return 成功读取chunk返回true，到达末尾返回false
     * @throws http_exception 格式错误时抛出
     */
    bool next_chunk(byte_vector& out);

    /**
     * @brief 获取所有trailer头部
     * @return trailer头部映射
     */
    NEFORCE_NODISCARD const unordered_map<string, string>& trailers() const noexcept { return trailers_; }

    /**
     * @brief 获取已读取的总体积
     */
    NEFORCE_NODISCARD byte_size total_read() const noexcept { return total_read_; }

    /**
     * @brief 检查是否已完成所有chunk的读取
     */
    NEFORCE_NODISCARD bool is_complete() const noexcept { return complete_; }

private:
    /// @brief 底层 TCP socket 指针
    tcp_socket* socket_;
    /// @brief 已读取的总字节数
    byte_size total_read_{0};
    /// @brief 是否已完成所有 chunk 读取
    bool complete_{false};
    /// @brief 是否已解析 trailer 头部
    bool trailers_parsed_{false};
    /// @brief trailer 头部键值对
    unordered_map<string, string> trailers_;
    /// @brief 内部读取缓冲区
    byte_vector buffer_;

    bool read_line(string& line);
    void parse_trailers();
};

/** @} */ // HTTP

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_HTTP_CHUNKED_READER_HPP__
