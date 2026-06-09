#ifndef NEFORCE_DATABASE_MYSQL_CONNECT_HPP__
#define NEFORCE_DATABASE_MYSQL_CONNECT_HPP__

/**
 * @file mysql_connect.hpp
 * @brief MySQL数据库连接实现
 *
 * 此文件提供了MySQL数据库的连接实现，支持连接管理、查询执行、预处理语句等功能。
 */

#ifdef NEFORCE_SUPPORT_MYSQL
#    include <mysql/mysql.h>
#    include "NeForce/db/sql_connect_base.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Database 数据库
 * @brief 数据库相关功能
 * @{
 */

/**
 * @defgroup MySQL MySQL
 * @brief MySQL数据库相关功能
 * @{
 */

/**
 * @class mysql_connect
 * @brief MySQL数据库连接类
 *
 * 实现idb_tb_connect接口，提供MySQL数据库的连接和操作功能。
 *
 * 主要功能：
 * - MySQL连接建立和关闭
 * - 字符集设置
 * - SQL语句执行
 * - 查询结果获取
 * - 预处理语句支持
 */
struct NEFORCE_API mysql_connect final : sql_connect_base<mysql_connect> {
protected:
    ::MYSQL* link_ = nullptr; ///< MySQL连接句柄
    friend sql_connect_base<mysql_connect>;

private:
    mutable string last_error_;       ///< 最后错误信息
    mutable uint32_t last_errno_ = 0; ///< 最后错误码

public:
    /**
     * @brief 构造函数
     *
     * 初始化MySQL连接句柄。
     */
    mysql_connect() { link_ = ::mysql_init(nullptr); }

    /**
     * @brief 析构函数
     *
     * 关闭数据库连接。
     */
    ~mysql_connect() noexcept override { close(); }

    /**
     * @brief 建立数据库连接
     * @param config 连接配置
     * @return 连接成功返回true，失败返回false
     *
     * 使用配置建立连接，连接后自动设置字符集。
     */
    NEFORCE_NODISCARD bool connect(const db_config& config) override;

    /**
     * @brief 重新连接数据库
     * @param config 连接配置
     * @return 重连成功返回true，失败返回false
     *
     * 关闭当前连接，使用新配置重新连接。
     */
    NEFORCE_NODISCARD bool reconnect(const db_config& config) override;

    /**
     * @brief 关闭数据库连接
     */
    void close() noexcept override;

    /**
     * @brief 设置字符集
     * @param encoding 字符集名称
     * @return 设置成功返回true
     */
    NEFORCE_NODISCARD bool set_character_set(const string& encoding) noexcept override;

    /**
     * @brief 设置MySQL连接选项
     * @param option MySQL选项类型
     * @param str 选项值字符串
     * @return 设置成功返回true
     */
    NEFORCE_NODISCARD bool set_options(::mysql_option option, const string& str) const noexcept;

    /**
     * @brief 获取当前字符集
     * @return 字符集名称
     */
    NEFORCE_NODISCARD string_view get_character_set() const noexcept override;

    /**
     * @brief 获取最后错误信息
     * @return 错误描述字符串
     */
    NEFORCE_NODISCARD string_view get_error() const noexcept override;

    /**
     * @brief 获取最后错误码
     * @return MySQL错误码
     */
    NEFORCE_NODISCARD uint32_t get_errno() const noexcept override;

    /**
     * @brief 执行非查询SQL语句
     * @param sql SQL语句
     * @return 执行成功返回true
     */
    NEFORCE_NODISCARD bool update(const string& sql) const override;

    /**
     * @brief 执行查询SQL语句
     * @param sql SELECT语句
     * @return 查询结果集，失败返回空指针
     */
    NEFORCE_NODISCARD unique_ptr<idb_tb_result> query(const string& sql) const override;

    /**
     * @brief 创建预处理语句
     * @param sql 带占位符的SQL语句
     * @return 预处理语句对象
     * @throws database_stmt_exception 创建失败时抛出
     */
    NEFORCE_NODISCARD unique_ptr<idb_prepared_statement> prepare_statement(const string& sql) const override;

    /**
     * @brief 检查连接是否已建立
     * @return 已连接返回true
     */
    NEFORCE_NODISCARD bool connected() const noexcept override { return link_ != nullptr; }

    /**
     * @brief 检查连接是否有效
     * @return 有效返回true
     */
    NEFORCE_NODISCARD bool is_valid() const noexcept override { return ::mysql_ping(link_) == 0; }

    size_t batch_insert(const string& table, const vector<string>& columns,
                        const vector<vector<string>>& rows) override;

private:
    string begin_sql() { return "START TRANSACTION"; }
    string commit_sql() { return "COMMIT"; }
    string rollback_sql() { return "ROLLBACK"; }
    string table_exists_query(const string& table) const { return "SHOW TABLES LIKE '" + table + "'"; }
};

/**
 * @class mysql_factory
 * @brief MySQL连接工厂类
 *
 * 实现idb_factory接口，用于创建MySQL连接和结果集对象。
 */
class NEFORCE_API mysql_factory final : public idb_factory {
public:
    /**
     * @brief 构造函数
     * @param config 数据库配置
     */
    explicit mysql_factory(db_config config) noexcept :
    idb_factory(move(config)) {}

    mysql_factory(const mysql_factory&) = delete;
    mysql_factory& operator=(const mysql_factory&) = delete;

    mysql_factory(mysql_factory&&) noexcept = default;
    mysql_factory& operator=(mysql_factory&&) noexcept = default;

    ~mysql_factory() noexcept override = default;

    /**
     * @brief 创建MySQL连接对象
     * @return 连接对象指针
     */
    idb_connect* create_connect() override;

    /**
     * @brief 创建MySQL结果集对象
     * @param native_result 原生MYSQL_RES句柄
     * @return 结果集对象指针
     */
    idb_result* create_result(void* native_result) override;
};

/** @} */ // MySQL

/** @} */ // Database

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_SUPPORT_MYSQL
#endif // NEFORCE_DATABASE_MYSQL_CONNECT_HPP__
