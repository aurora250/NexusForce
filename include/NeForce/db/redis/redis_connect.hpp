#ifndef NEFORCE_DATABASE_REDIS_CONNECT_HPP__
#define NEFORCE_DATABASE_REDIS_CONNECT_HPP__

/**
 * @file redis_connect.hpp
 * @brief Redis数据库连接实现
 *
 * 此文件提供了Redis数据库的连接实现。
 */

#ifdef NEFORCE_SUPPORT_HIREDIS
#    include <hiredis/hiredis.h>
#    include "NeForce/db/db_interface.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Database 数据库
 * @brief 数据库相关功能
 * @{
 */

/**
 * @defgroup Redis Redis
 * @brief Redis数据库相关功能
 * @{
 */

/**
 * @struct redis_connect
 * @brief Redis数据库连接类
 *
 * 实现idb_kv_connect接口，提供Redis数据库的连接和操作功能。
 *
 * 主要功能：
 * - Redis连接建立和关闭
 * - 密码认证和数据库选择
 * - 字符串操作（SET、GET、SETEX、DEL、EXISTS、EXPIRE）
 * - 哈希表操作（HSET、HGET、HGETALL）
 * - 列表操作（LPUSH、RPUSH、LRANGE）
 * - 集合操作（SADD、SMEMBERS）
 * - 连接健康检查（PING）
 */
struct NEFORCE_API redis_connect final : idb_kv_connect {
private:
    ::redisContext* link_ = nullptr; ///< Redis连接上下文
    mutable string last_error_;      ///< 最后错误信息
    mutable uint32_t last_errno_ = 0; ///< 最后错误码

    ::redisReply* execute_command(string_view command, const vector<string_view>& args) const;
    bool authenticate(const string& password) const;
    bool select_database(const string& db_index) const;

public:
    /**
     * @brief 默认构造函数
     */
    redis_connect() = default;

    /**
     * @brief 析构函数
     *
     * 关闭Redis连接。
     */
    ~redis_connect() override { close(); }

    /**
     * @brief 建立Redis连接
     * @param config 连接配置
     * @return 连接成功返回true
     *
     * 建立连接，进行密码认证，选择数据库。
     */
    bool connect(const db_config& config) override;

    /**
     * @brief 重新连接
     * @param config 连接配置
     * @return 重连成功返回true
     */
    bool reconnect(const db_config& config) override;

    /**
     * @brief 关闭Redis连接
     */
    void close() noexcept override;

    /**
     * @brief 设置字符集（Redis不支持）
     * @deprecated Redis不支持设置字符集
     */
    NEFORCE_DEPRECATED_FOR("Redis not support setting character sets")
    bool set_character_set(const string& /*encoding*/) const noexcept override { return false; }

    /**
     * @brief 获取字符集（Redis不支持）
     * @deprecated Redis不支持字符集概念
     */
    NEFORCE_DEPRECATED_FOR("Redis not support setting character sets")
    string_view get_character_set() const noexcept override { return ""; }

    /**
     * @brief 获取最后错误信息
     * @return 错误描述字符串
     */
    string_view get_error() const override;

    /**
     * @brief 获取最后错误码
     * @return hiredis错误码
     */
    uint32_t get_errno() const noexcept override { return last_errno_; }

    /**
     * @brief 执行非查询命令
     * @param sql Redis命令字符串
     * @return 执行成功返回true
     */
    bool update(const string& sql) const override;

    /**
     * @brief 执行查询命令
     * @param sql Redis命令字符串
     * @return 查询结果集
     */
    unique_ptr<idb_kv_result> query(const string& sql) const override;

    /**
     * @brief 检查连接是否已建立
     * @return 已连接返回true
     */
    bool connected() const noexcept override { return link_ != nullptr && link_->err == 0; }

    /**
     * @brief 检查连接是否有效
     * @return 有效返回true
     */
    bool is_valid() const override;

    bool begin() override;
    bool commit() override;
    bool rollback() override;

    NEFORCE_NODISCARD void* native_handle() noexcept override { return link_; }

    bool set(const string& key, const string& value) override;                ///< SET命令
    bool setex(const string& key, const string& value, int seconds) override; ///< SETEX命令（带过期时间）
    unique_ptr<idb_kv_result> get(const string& key) override;                ///< GET命令
    bool del(const string& key) override;                                     ///< DEL命令
    bool exists(const string& key) override;                                  ///< EXISTS命令
    bool expire(const string& key, int seconds) override;                     ///< EXPIRE命令

    bool hset(const string& key, const string& field, const string& value) override; ///< HSET命令
    unique_ptr<idb_kv_result> hget(const string& key, const string& field) override; ///< HGET命令
    unique_ptr<idb_kv_result> hgetall(const string& key) override;                   ///< HGETALL命令

    bool lpush(const string& key, const string& value) override;                       ///< LPUSH命令
    bool rpush(const string& key, const string& value) override;                       ///< RPUSH命令
    unique_ptr<idb_kv_result> lrange(const string& key, int start, int stop) override; ///< LRANGE命令

    bool sadd(const string& key, const string& member) override;    ///< SADD命令
    unique_ptr<idb_kv_result> smembers(const string& key) override; ///< SMEMBERS命令
};

/**
 * @class redis_factory
 * @brief Redis连接工厂类
 *
 * 实现idb_factory接口，用于创建Redis连接和结果集对象。
 */
class NEFORCE_API redis_factory final : public idb_factory {
public:
    /**
     * @brief 构造函数
     * @param config 数据库配置
     */
    explicit redis_factory(db_config config) :
    idb_factory(_NEFORCE move(config)) {}

    /**
     * @brief 创建Redis连接对象
     * @return 连接对象指针
     */
    idb_connect* create_connect() override;

    /**
     * @brief 创建Redis结果集对象
     * @param native_result 原生redisReply指针
     * @return 结果集对象指针
     */
    idb_result* create_result(void* native_result) override;
};

/** @} */ // Redis

/** @} */ // Database

NEFORCE_END_NAMESPACE__
#endif
#endif // NEFORCE_DATABASE_REDIS_CONNECT_HPP__
