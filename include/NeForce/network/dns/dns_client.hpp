#ifndef NEFORCE_NETWORK_DNS_CLIENT_HPP__
#define NEFORCE_NETWORK_DNS_CLIENT_HPP__

/**
 * @file dns_client.hpp
 * @brief DNS客户端实现
 *
 * 此文件提供了DNS客户端的功能实现，支持DNS查询、记录解析和缓存管理。
 * 支持UDP和TCP协议，提供同步和异步查询接口。
 */

#include "NeForce/core/async/future.hpp"
#include "NeForce/core/async/shared_mutex.hpp"
#include "NeForce/core/container/unordered_map.hpp"
#include "NeForce/core/time/clocks.hpp"
#include "NeForce/core/utility/optional.hpp"
#include "NeForce/network/dns/dns_message.hpp"
#include "NeForce/network/util/ports.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Network 网络通信
 * @brief 网络通信相关组件
 * @{
 */

/**
 * @defgroup DNS DNS
 * @brief DNS组件
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
        ports port{ports::dns};     ///< DNS服务器端口
        milliseconds timeout{5000}; ///< 查询超时时间
    };

private:
    config config_{};                                                               ///< 客户端配置
    unordered_map<string, pair<dns_query_result, steady_clock::time_point>> cache_; ///< DNS缓存
    mutable shared_mutex cache_mutex_;                                              ///< 缓存互斥锁
    seconds cache_ttl_{300};                                                        ///< 缓存TTL（秒）
    bool use_tcp_ = false;                                                          ///< 是否强制使用TCP

private:
    byte_vector send_udp_query(const byte_vector& query) const;
    byte_vector send_tcp_query(const byte_vector& query) const;

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

    /**
     * @brief 设置客户端配置
     * @param cfg 新配置
     */
    void set_config(config cfg) { config_ = move(cfg); }

    /**
     * @brief 设置超时时间
     * @param timeout 超时时间
     */
    void set_timeout(const milliseconds timeout) { config_.timeout = timeout; }

    /**
     * @brief 设置是否使用TCP
     * @param use_tcp 是否使用TCP
     */
    void set_use_tcp(const bool use_tcp) { use_tcp_ = use_tcp; }

    /**
     * @brief 设置缓存TTL
     * @param ttl 缓存生存时间
     */
    void set_cache_ttl(const seconds ttl) { cache_ttl_ = ttl; }

    /**
     * @brief 清空缓存
     */
    void clear_cache() { cache_.clear(); }

    /**
     * @brief 执行DNS查询
     * @param domain 域名
     * @param type 记录类型（默认A记录）
     * @param qclass 查询类（默认INTERNET）
     * @return 查询结果
     * @throws dns_exception 查询失败时抛出
     */
    dns_query_result query(string_view domain, dns_record::raw type = dns_record::A,
                           dns_query qclass = dns_query::INTERNET);

    /**
     * @brief 异步DNS查询
     * @param domain 域名
     * @param type 记录类型
     * @param qclass 查询类
     * @return future对象，可等待查询结果
     */
    future<dns_query_result> query_async(const string& domain, dns_record::raw type = dns_record::A,
                                         dns_query qclass = dns_query::INTERNET);

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
};

/** @} */ // DNS

/** @} */ // Network

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_DNS_CLIENT_HPP__
