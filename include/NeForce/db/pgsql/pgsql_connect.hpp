#ifndef NEFORCE_DATABASE_PGSQL_CONNECT_HPP__
#define NEFORCE_DATABASE_PGSQL_CONNECT_HPP__

/**
 * @file pgsql_connect.hpp
 * @brief PostgreSQL数据库连接实现
 *
 * 此文件提供了PostgreSQL数据库的连接实现。
 */

#ifdef NEFORCE_SUPPORT_POSTGRESQL
#    include <libpq-fe.h>
#    include "NeForce/db/sql_connect_base.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Database 数据库
 * @brief 数据库相关功能
 * @{
 */

/**
 * @defgroup PostgreSQL PostgreSQL
 * @brief PostgreSQL数据库相关功能
 * @{
 */

/**
 * @class pgsql_connect
 * @brief PostgreSQL数据库连接类
 *
 * 实现idb_tb_connect接口，提供PostgreSQL数据库的连接和操作功能。
 *
 * 主要功能：
 * - PostgreSQL连接建立和关闭
 * - 字符集设置和查询
 * - SQL语句执行
 * - 查询结果获取
 * - 预处理语句支持
 * - 连接健康检查
 */
class NEFORCE_API pgsql_connect final : public sql_connect_base<pgsql_connect> {
protected:
    ::PGconn* link_ = nullptr; ///< PostgreSQL连接句柄
    friend sql_connect_base<pgsql_connect>;

private:
    mutable string last_error_;       ///< 最后错误信息
    mutable uint32_t last_errno_ = 0; ///< 最后错误码

public:
    /**
     * @brief 默认构造函数
     */
    pgsql_connect() = default;

    /**
     * @brief 析构函数
     *
     * 自动关闭数据库连接。
     */
    ~pgsql_connect() override { close(); }

    pgsql_connect(const pgsql_connect&) = delete;
    pgsql_connect& operator=(const pgsql_connect&) = delete;

    /**
     * @brief 建立数据库连接
     * @param config 连接配置
     * @return 连接成功返回true
     *
     * 根据配置构建连接字符串，建立PostgreSQL连接。
     * 连接成功后刷新存活时间。
     */
    NEFORCE_NODISCARD bool connect(const db_config& config) override;

    /**
     * @brief 重新连接数据库
     * @param config 连接配置
     * @return 重连成功返回true
     *
     * 关闭当前连接，使用新配置重新连接。
     */
    bool reconnect(const db_config& config) override;

    /**
     * @brief 关闭数据库连接
     */
    void close() override;

    /**
     * @brief 设置字符集
     * @param encoding 字符集名称
     * @return 设置成功返回true
     */
    bool set_character_set(const string& encoding) override;

    /**
     * @brief 获取当前字符集
     * @return 字符集名称
     */
    NEFORCE_NODISCARD string_view get_character_set() const override;

    /**
     * @brief 获取最后错误信息
     * @return 错误描述字符串
     */
    NEFORCE_NODISCARD string_view get_error() const override { return last_error_.view(); }

    /**
     * @brief 获取最后错误码
     * @return 错误码
     */
    NEFORCE_NODISCARD uint32_t get_errno() const override { return last_errno_; }

    /**
     * @brief 执行非查询SQL语句
     * @param sql SQL语句
     * @return 执行成功返回true
     */
    bool update(const string& sql) const override;

    /**
     * @brief 执行查询SQL语句
     * @param sql SELECT语句
     * @return 查询结果集，失败返回空指针
     */
    unique_ptr<idb_tb_result> query(const string& sql) const override;

    /**
     * @brief 创建预处理语句
     * @param sql 带占位符的SQL语句（使用$1, $2等）
     * @return 预处理语句对象
     */
    unique_ptr<idb_prepared_statement> prepare_statement(const string& sql) const override;

    /**
     * @brief 检查连接是否已建立
     * @return 已连接返回true
     */
    NEFORCE_NODISCARD bool connected() const override;

    /**
     * @brief 检查连接是否有效
     * @return 有效返回true
     */
    NEFORCE_NODISCARD bool is_valid() const override { return connected(); }

    size_t batch_insert(const string& table, const vector<string>& columns,
                        const vector<vector<string>>& rows) override;

    // PgSQL table_exists 使用 SELECT EXISTS(...) 模式，不同于基类的简单查询
    NEFORCE_NODISCARD bool table_exists(const string& table) const override;

private:
    string begin_sql() { return "BEGIN"; }
    string commit_sql() { return "COMMIT"; }
    string rollback_sql() { return "ROLLBACK"; }
    string table_exists_query(const string& table) const {
        return "SELECT EXISTS (SELECT FROM information_schema.tables WHERE table_name = '" + table + "')";
    }
};

/**
 * @class pgsql_factory
 * @brief PostgreSQL连接工厂类
 *
 * 实现idb_factory接口，用于创建PostgreSQL连接和结果集对象。
 */
class NEFORCE_API pgsql_factory final : public idb_factory {
public:
    /**
     * @brief 构造函数
     * @param config 数据库配置
     */
    explicit pgsql_factory(db_config config) :
    idb_factory(_NEFORCE move(config)) {}

    /**
     * @brief 析构函数
     */
    ~pgsql_factory() override = default;

    /**
     * @brief 创建PostgreSQL连接对象
     * @return 连接对象指针
     */
    idb_connect* create_connect() override;

    /**
     * @brief 创建PostgreSQL结果集对象
     * @param native_result 原生PGresult指针
     * @return 结果集对象指针
     */
    idb_result* create_result(void* native_result) override;
};

/** @} */ // PostgreSQL

/** @} */ // Database

NEFORCE_END_NAMESPACE__
#endif
#endif // NEFORCE_DATABASE_PGSQL_CONNECT_HPP__
