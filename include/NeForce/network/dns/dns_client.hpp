#ifndef NEFORCE_NETWORK_DNS_DNS_CLIENT_HPP__
#define NEFORCE_NETWORK_DNS_DNS_CLIENT_HPP__

/**
 * @file dns_client.hpp
 * @brief DNS客户端实现
 *
 * 此文件提供了DNS客户端的功能实现，支持DNS查询、记录解析和缓存管理。
 * 支持UDP和TCP协议，提供同步和异步查询接口。
 */

#include "NeForce/core/async/future.hpp"
#include "NeForce/core/async/promise.hpp"
#include "NeForce/core/async/shared_mutex.hpp"
#include "NeForce/core/async/thread_pool.hpp"
#include "NeForce/core/container/unordered_map.hpp"
#ifndef NEFORCE_PLATFORM_WINDOWS
#    include "NeForce/core/system/pipe.hpp"
#endif
#include "NeForce/core/time/clocks.hpp"
#include "NeForce/core/utility/optional.hpp"
#include "NeForce/network/dns/dns_message.hpp"
#include "NeForce/network/udp_socket.hpp"
#include "NeForce/network/util/ports.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup DNS DNS
 * @{
 */

/**
 * @class dns_client
 * @brief DNS客户端类
 *
 * 提供DNS查询服务的完整实现，支持缓存、超时控制和多种查询类型。
 *
 * 主要功能：
 * - DNS查询（支持A、AAAA、CNAME、MX、TXT等记录类型）
 * - DNS缓存（减少重复查询）
 * - 同步/异步查询
 * - 批量查询
 * - 反向查询（PTR记录）
 * - TCP/UDP协议自动切换（响应截断时自动切换TCP）
 */
class NEFORCE_API dns_client {
public:
    /**
     * @struct config
     * @brief DNS客户端配置
     */
    struct config {
        string server{"8.8.8.8"};   ///< DNS服务器地址
        ports port{ports::DNS};     ///< DNS服务器端口
        milliseconds timeout{5000}; ///< 查询超时时间
    };

private:
    struct pending_entry {
        _NEFORCE promise<dns_query_result> promise;
        byte_vector query_data;
        steady_clock::time_point created_at{steady_clock::now()};
    };

    config config_{};                                                               ///< 客户端配置
    unordered_map<string, pair<dns_query_result, steady_clock::time_point>> cache_; ///< DNS缓存
    mutable shared_mutex cache_mutex_;                                              ///< 缓存互斥锁
    seconds cache_ttl_{300};                                                        ///< 缓存TTL
    bool use_tcp_ = false;                                                          ///< 是否强制使用TCP
    bool recursion_desired_ = true;                                                 ///< 是否期望递归查询（RD）
    uint16_t edns_udp_payload_{edns::DEFAULT_UDP_PAYLOAD};                          ///< EDNS0 UDP载荷大小
    bool dnssec_ok_ = false;                                                        ///< 是否请求DNSSEC（DO）

    udp_socket shared_socket_;                               ///< 共享UDP socket
    unordered_map<uint16_t, pending_entry> pending_queries_; ///< 待处理查询（ID索引）
    mutable mutex pending_mutex_;                            ///< 待处理查询互斥锁
    mutable mutex send_mutex_;                               ///< 发送互斥锁
    thread_pool io_pool_;                                    ///< I/O线程池
#ifndef NEFORCE_PLATFORM_WINDOWS
    pipe wake_pipe_; ///< 唤醒管道
#endif
    atomic<bool> io_running_{false}; ///< I/O循环运行标志

private:
    void ensure_io_started();
    void start_io();
    void stop_io();
    void io_receive_loop();

    void send_query(const byte_vector& query);
    byte_vector send_tcp_query(const byte_vector& query);

    optional<dns_query_result> check_cache(const string& key);
    void update_cache(const string& key, const dns_query_result& result);

public:
    /**
     * @brief 默认构造函数
     *
     * 使用默认配置创建DNS客户端。
     */
    dns_client() :
    dns_client(config()) {}

    /**
     * @brief 构造函数
     * @param cfg 客户端配置
     * @param use_tcp 是否强制使用TCP
     * @throws dns_exception 配置无效时抛出
     */
    explicit dns_client(config cfg, bool use_tcp = false);

    ~dns_client();

    dns_client(const dns_client&) = delete;
    dns_client& operator=(const dns_client&) = delete;
    dns_client(dns_client&&) = delete;
    dns_client& operator=(dns_client&&) = delete;

    /**
     * @brief 设置客户端配置
     * @param cfg 新配置
     */
    void set_config(config cfg) noexcept { config_ = move(cfg); }

    /**
     * @brief 设置超时时间
     * @param timeout 超时时间
     */
    void set_timeout(const milliseconds timeout) noexcept { config_.timeout = timeout; }

    /**
     * @brief 设置是否使用TCP
     * @param use_tcp 是否使用TCP
     */
    void set_use_tcp(const bool use_tcp) noexcept { use_tcp_ = use_tcp; }

    /**
     * @brief 设置缓存TTL
     * @param ttl 缓存生存时间
     */
    void set_cache_ttl(const seconds ttl) noexcept { cache_ttl_ = ttl; }

    /**
     * @brief 设置是否期望递归查询（RD）
     * @param rd 是否期望递归
     */
    void set_recursion_desired(const bool rd) { recursion_desired_ = rd; }

    /**
     * @brief 设置EDNS0 UDP载荷大小
     * @param payload_size UDP载荷大小（字节）
     */
    void set_edns_udp_payload(const uint16_t payload_size) noexcept { edns_udp_payload_ = payload_size; }

    /**
     * @brief 设置是否请求DNSSEC（DO）
     * @param ok 是否启用DNSSEC
     */
    void set_dnssec_ok(const bool ok) noexcept { dnssec_ok_ = ok; }

    /**
     * @brief 清空缓存
     */
    void clear_cache() noexcept { cache_.clear(); }

    /**
     * @brief 执行DNS查询
     * @param domain 域名
     * @param type 记录类型（默认A记录）
     * @param qclass 查询类（默认INTERNET）
     * @return 查询结果
     * @throws dns_exception 查询失败时抛出
     */
    dns_query_result query(string_view domain, dns_record::raw type = dns_record::A,
                           dns_class qclass = dns_class::INTERNET);

    /**
     * @brief 异步DNS查询
     * @param domain 域名
     * @param type 记录类型
     * @param qclass 查询类
     * @return future对象，可等待查询结果
     */
    future<dns_query_result> query_async(const string& domain, dns_record::raw type = dns_record::A,
                                         dns_class qclass = dns_class::INTERNET);

    /**
     * @brief 解析A记录（IPv4地址）
     * @param domain 域名
     * @return IPv4地址列表
     */
    vector<string> resolve_a(string_view domain);

    /**
     * @brief 解析AAAA记录（IPv6地址）
     * @param domain 域名
     * @return IPv6地址列表
     */
    vector<string> resolve_aaaa(string_view domain);

    /**
     * @brief 解析CNAME记录（别名）
     * @param domain 域名
     * @return 别名列表
     */
    vector<string> resolve_cname(string_view domain);

    /**
     * @brief 解析MX记录（邮件交换器）
     * @param domain 域名
     * @return MX记录列表（格式："优先级 域名"）
     */
    vector<string> resolve_mx(string_view domain);

    /**
     * @brief 解析TXT记录（文本记录）
     * @param domain 域名
     * @return 文本记录列表
     */
    vector<string> resolve_txt(string_view domain);

    /**
     * @brief 解析SRV记录（服务定位器）
     * @param domain 域名
     * @return SRV记录列表
     */
    vector<dns_srv_record> resolve_srv(string_view domain);

    /**
     * @brief 解析SOA记录（授权区域起始）
     * @param domain 域名
     * @return SOA记录，失败返回空optional
     */
    optional<dns_soa_record> resolve_soa(string_view domain);

    /**
     * @brief 反向查询（从IP获取域名）
     * @param ip IPv4地址
     * @return 域名，失败返回空字符串
     * @throws dns_exception IP地址无效时抛出
     *
     * 执行PTR记录查询，将IP地址转换为域名。
     * 支持IPv4地址，IPv6支持有限。
     */
    string reverse_query(string_view ip);

    /**
     * @brief 批量查询
     * @param domains 域名列表
     * @param type 记录类型（默认A记录）
     * @return 查询结果列表
     *
     * 并发执行多个DNS查询，提高批量查询效率。
     */
    vector<dns_query_result> batch_query(const vector<string>& domains, dns_record::raw type = dns_record::A);

    /**
     * @brief 构建DNS查询消息
     * @param domain 域名
     * @param type 记录类型
     * @param qclass 查询类
     * @param rd 是否期望递归
     * @param edns_enable 是否启用EDNS0
     * @param dnssec_ok 是否请求DNSSEC
     * @param edns_payload EDNS0 UDP载荷大小
     * @return 编码后的DNS查询消息
     */
    static byte_vector build_query(string_view domain, dns_record::raw type = dns_record::A,
                                   dns_class qclass = dns_class::INTERNET, bool rd = true, bool edns_enable = true,
                                   bool dnssec_ok = false, uint16_t edns_payload = edns::DEFAULT_UDP_PAYLOAD);

    /**
     * @brief 解析DNS响应消息
     * @param response 原始响应数据
     * @param expected_id 期望的查询ID（0表示不校验）
     * @return 解析后的查询结果
     * @throws dns_exception 解析或校验失败时抛出
     */
    static dns_query_result parse_response(const byte_vector& response, uint16_t expected_id = 0);
};

/** @} */ // DNS

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_DNS_DNS_CLIENT_HPP__
