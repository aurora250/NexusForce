#ifndef NEFORCE_DATABASE_SQLITE_CONNECT_HPP__
#define NEFORCE_DATABASE_SQLITE_CONNECT_HPP__

/**
 * @file sqlite_connect.hpp
 * @brief SQLite数据库连接实现
 *
 * 此文件提供了SQLite数据库的连接实现。
 */

#ifdef NEFORCE_SUPPORT_SQLITE3
#    ifdef NEFORCE_SUPPORT_SQLCIPHER
#        include <sqlcipher/sqlite3.h>

#        include <utility>
#    else
#        include <sqlite3.h>
#    endif
#    include "NeForce/db/sql_connect_base.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Database 数据库
 * @brief 数据库相关功能
 * @{
 */

/**
 * @defgroup SQLite3 SQLite3
 * @brief SQLite3数据库相关功能
 * @{
 */

/**
 * @struct sqlite_connect
 * @brief SQLite数据库连接类
 *
 * 实现idb_tb_connect接口，提供SQLite数据库的连接和操作功能。
 *
 * 主要功能：
 * - SQLite数据库连接建立和关闭
 * - 字符集设置和查询
 * - SQL语句执行
 * - 查询结果获取
 * - 预处理语句支持
 * - 连接健康检查
 *
 * @note SQLite连接是文件级别的，database字段指定数据库文件路径。
 *       支持内存数据库（":memory:"）。
 */
struct NEFORCE_API sqlite_connect final : sql_connect_base<sqlite_connect> {
public:
    /**
     * @enum key_type
     * @brief SQLCipher 加密密钥类型
     *
     * 控制 SQLCipher 如何处理传入的密钥数据。
     *
     * - PBKDF2：口令派生（PBKDF2-HMAC-SHA256，默认）
     * - RAW：原始密钥（32 字节 AES-256 key，不派生）
     */
    enum class key_type : byte_t {
        PBKDF2, ///< 口令派生
        RAW     ///< 原始密钥
    };

protected:
    ::sqlite3* link_ = nullptr; ///< SQLite数据库连接句柄
    friend sql_connect_base<sqlite_connect>;

private:
    mutable string last_error_;       ///< 最后错误信息
    mutable uint32_t last_errno_ = 0; ///< 最后错误码

public:
    /**
     * @brief 默认构造函数
     *
     * 创建内存数据库连接（临时数据库）。
     */
    sqlite_connect() noexcept { ::sqlite3_open(nullptr, &link_); }

    /**
     * @brief 析构函数
     *
     * 自动关闭数据库连接。
     */
    ~sqlite_connect() noexcept override { this->close(); }

    /**
     * @brief 建立数据库连接
     * @param config 连接配置
     * @return 连接成功返回true
     *
     * 打开config.database指定的数据库文件。
     * 如果database为空，创建内存数据库。
     */
    bool connect(const db_config& config) override;

    /**
     * @brief 重新连接数据库
     * @param config 连接配置
     * @return 重连成功返回true
     *
     * 关闭当前连接，使用新配置重新连接。
     */
    NEFORCE_NODISCARD bool reconnect(const db_config& config) override;

#    ifdef NEFORCE_SUPPORT_SQLCIPHER

    /**
     * @brief 建立数据库连接
     * @param config 连接配置
     * @param encryption_key 加密密钥
     * @param type 密钥类型
     * @return 连接成功返回true
     *
     * 打开config.database指定的数据库文件。
     * 如果database为空，创建内存数据库。
     */
    bool connect(const db_config& config, const string& encryption_key, key_type type = key_type::PBKDF2);

    /**
     * @brief 重新连接数据库
     * @param config 连接配置
     * @param encryption_key 加密密钥
     * @param type 密钥类型
     * @return 重连成功返回true
     *
     * 关闭当前连接，使用新配置重新连接。
     */
    NEFORCE_NODISCARD bool reconnect(const db_config& config, const string& encryption_key,
                                     key_type type = key_type::PBKDF2);

    /**
     * @brief 修改已打开数据库的加密密钥
     * @param new_key 新密钥
     * @param type 密钥类型
     * @return 修改成功返回true
     *
     * 允许在不关闭连接的情况下更换加密密钥。如果从未加密数据库调用此方法，将为其启用加密。
     */
    NEFORCE_NODISCARD bool rekey(const string& new_key, key_type type = key_type::PBKDF2);

#    endif

    /**
     * @brief 关闭数据库连接
     */
    void close() noexcept override;

    /**
     * @brief 设置字符集
     * @param encoding 字符集名称
     * @return 设置成功返回true
     */
    NEFORCE_NODISCARD bool set_character_set(const string& encoding) override;

    /**
     * @brief 获取当前字符集
     * @return 字符集名称
     */
    NEFORCE_NODISCARD string_view get_character_set() const override;

    /**
     * @brief 获取最后错误信息
     * @return 错误描述字符串
     */
    NEFORCE_NODISCARD string_view get_error() const override;

    /**
     * @brief 获取最后错误码
     * @return SQLite错误码
     */
    NEFORCE_NODISCARD uint32_t get_errno() const override { return last_errno_; }

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
     */
    NEFORCE_NODISCARD unique_ptr<idb_prepared_statement> prepare_statement(const string& sql) const override;

    /**
     * @brief 检查连接是否已建立
     * @return 已连接返回true
     */
    NEFORCE_NODISCARD bool connected() const override { return link_ != nullptr; }

    /**
     * @brief 检查连接是否有效
     * @return 有效返回true
     */
    NEFORCE_NODISCARD bool is_valid() const override;

    NEFORCE_NODISCARD bool table_exists(const string& table) const override;

    size_t batch_insert(const string& table, const vector<string>& columns,
                        const vector<vector<string>>& rows) override;

private:
    string begin_sql() { return "BEGIN TRANSACTION"; }
    string commit_sql() { return "COMMIT"; }
    string rollback_sql() { return "ROLLBACK"; }
    string table_exists_query(const string& table) const {
        return "SELECT name FROM sqlite_master WHERE type='table' AND name='" + table + "'";
    }
};

/**
 * @class sqlite_factory
 * @brief SQLite连接工厂类
 *
 * 实现idb_factory接口，用于创建SQLite连接和结果集对象。
 */
class NEFORCE_API sqlite_factory final : public idb_factory {
#    ifdef NEFORCE_SUPPORT_SQLCIPHER
    string encryption_key_;                                                ///< 加密密钥
    sqlite_connect::key_type key_type_ = sqlite_connect::key_type::PBKDF2; ///< 密钥类型
#    endif

public:
    /**
     * @brief 构造函数
     * @param config 数据库配置
     */
    explicit sqlite_factory(db_config config) :
    idb_factory(move(config)) {}

#    ifdef NEFORCE_SUPPORT_SQLCIPHER
    /**
     * @brief 构造函数
     * @param config 数据库配置
     * @param encryption_key 加密密钥
     * @param type 密钥类型：PBKDF2=口令派生, RAW=原始密钥字节
     */
    explicit sqlite_factory(db_config config, string encryption_key,
                            sqlite_connect::key_type type = sqlite_connect::key_type::PBKDF2) :
    idb_factory(move(config)),
    encryption_key_(move(encryption_key)),
    key_type_(type) {}
#    endif

    /**
     * @brief 创建SQLite连接对象
     * @return 连接对象指针
     */
    idb_connect* create_connect() override;

    /**
     * @brief 创建SQLite结果集对象
     * @param native_result 原生sqlite3_stmt指针
     * @return 结果集对象指针
     */
    idb_result* create_result(void* native_result) override;
};

/** @} */ // SQLite3

/** @} */ // Database

NEFORCE_END_NAMESPACE__
#endif
#endif // NEFORCE_DATABASE_SQLITE_CONNECT_HPP__
