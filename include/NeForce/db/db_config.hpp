#ifndef NEFORCE_DATABASE_DB_CONFIG_HPP__
#define NEFORCE_DATABASE_DB_CONFIG_HPP__

/**
 * @file db_config.hpp
 * @brief 数据库配置定义
 *
 * 此文件定义了数据库连接配置结构和相关异常类。
 * 支持多种数据库类型（MySQL、SQLite3、Redis、PostgreSQL），
 * 并为每种数据库提供便捷的配置工厂方法。
 */

#include "NeForce/network/util/ports.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Exceptions 异常类集
 * @brief 异常类集
 * @{
 */

/**
 * @struct database_typecast_exception
 * @brief 数据库数据类型转换异常
 */
struct database_typecast_exception final : database_exception {
    explicit database_typecast_exception(const char* info = "Database Type Mismatch.", const char* type = static_type,
                                         const int code = 0) noexcept :
    database_exception(info, type, code) {}

    explicit database_typecast_exception(const exception& e) :
    database_exception(e) {}

    ~database_typecast_exception() override = default;
    static constexpr auto static_type = "database_typecast_exception";
};

/**
 * @struct database_stmt_exception
 * @brief 数据库处理语句操作异常
 */
struct database_stmt_exception final : database_exception {
    explicit database_stmt_exception(const char* info = "Database Statement Operations Error.",
                                     const char* type = static_type, const int code = 0) noexcept :
    database_exception(info, type, code) {}

    explicit database_stmt_exception(const exception& e) :
    database_exception(e) {}

    ~database_stmt_exception() override = default;
    static constexpr auto static_type = "database_prepared_stmt_exception";
};

/** @} */ // Exceptions

/**
 * @defgroup Database 数据库
 * @brief 数据库相关功能
 * @{
 */

/**
 * @enum db_type
 * @brief 数据库类型枚举
 *
 * 定义支持的数据库类型，每种类型对应一个数据库后端。
 * 枚举值仅在对应数据库支持时可用。
 */
enum class db_type : uint8_t {
#ifdef NEFORCE_SUPPORT_MYSQL
    MYSQL = 1, ///< MySQL数据库
#endif
#ifdef NEFORCE_SUPPORT_SQLITE3
    SQLITE3, ///< SQLite3数据库
#endif
#ifdef NEFORCE_SUPPORT_HIREDIS
    REDIS, ///< Redis键值存储
#endif
#ifdef NEFORCE_SUPPORT_POSTGRESQL
    POSTGRESQL ///< PostgreSQL数据库
#endif
};

/**
 * @struct db_config
 * @brief 数据库连接配置结构
 *
 * 存储连接数据库所需的所有参数，包括用户名、密码、主机、端口、数据库名等。
 * 提供各数据库类型的便捷配置工厂方法。
 */
struct NEFORCE_API db_config {
    string username{};         ///< 数据库用户名
    string password{};         ///< 数据库密码
    string database{};         ///< 数据库名
    string host = "127.0.0.1"; ///< 数据库主机地址
    string charset{};          ///< 数据库字符集
    ports port{};              ///< 数据库端口号

#ifdef NEFORCE_SUPPORT_POSTGRESQL
    /**
     * @brief 创建PostgreSQL数据库配置
     * @param db 数据库名
     * @return 使用默认PostgreSQL参数的配置对象
     *
     * 默认参数：
     * - 端口：5432
     * - 字符集：utf8
     * - 用户名：postgres
     */
    static db_config for_postgresql(const string& db = "postgres");
#endif

#ifdef NEFORCE_SUPPORT_MYSQL
    /**
     * @brief 创建MySQL数据库配置
     * @param db 数据库名
     * @return 使用默认MySQL参数的配置对象
     *
     * 默认参数：
     * - 端口：3306
     * - 字符集：utf8mb4
     * - 用户名：root
     */
    static db_config for_mysql(const string& db);
#endif

#ifdef NEFORCE_SUPPORT_SQLITE3
    /**
     * @brief 创建SQLite3数据库配置
     * @param file 数据库文件路径
     * @return 使用默认SQLite3参数的配置对象
     *
     * 对于SQLite，host和port字段被忽略，
     * database字段存储文件路径。
     */
    static db_config for_sqlite(const string& file);
#endif

#ifdef NEFORCE_SUPPORT_HIREDIS
    /**
     * @brief 创建Redis数据库配置
     * @param db 数据库编号（作为字符串，如"0"）
     * @return 使用默认Redis参数的配置对象
     *
     * 默认参数：
     * - 端口：6379
     * - database：数据库编号
     */
    static db_config for_redis(const string& db);
#endif
};

/** @} */ // Database

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_DATABASE_DB_CONFIG_HPP__
