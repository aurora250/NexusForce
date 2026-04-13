#ifndef NEFORCE_DATABASE_SQLITE_PREPARED_STATEMENT_HPP__
#define NEFORCE_DATABASE_SQLITE_PREPARED_STATEMENT_HPP__

/**
 * @file sqlite_prepared_statement.hpp
 * @brief SQLite预处理语句实现
 *
 * 此文件提供了SQLite预处理语句的实现。
 */

#ifdef NEFORCE_SUPPORT_SQLITE3
#    include <sqlite3.h>
#    include "NeForce/db/db_interface.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Database 数据库
 * @brief 数据库相关功能
 * @{
 */

/**
 * @class sqlite_prepared_statement
 * @brief SQLite预处理语句类
 *
 * 实现idb_prepared_statement接口，提供SQLite预处理语句的完整功能。
 * 支持参数绑定、语句执行和结果集获取。
 *
 * 主要功能：
 * - SQL语句预处理（支持?占位符）
 * - 参数绑定
 * - 语句执行
 * - 查询执行
 * - 语句重置和资源管理
 * - 错误信息获取
 *
 * @note SQLite使用?作为参数占位符，参数索引从1开始。
 */
class NEFORCE_API sqlite_prepared_statement final : public idb_prepared_statement {
private:
    ::sqlite3* db_ = nullptr;        ///< SQLite数据库连接句柄
    ::sqlite3_stmt* stmt_ = nullptr; ///< SQLite预处理语句句柄

    uint32_t param_count_ = 0;           ///< 参数数量（?占位符个数）
    vector<vector<char>> param_buffers_; ///< 参数数据缓冲区
    bool prepared_ = false;              ///< 预处理是否成功
    mutable string last_error_;          ///< 最后错误信息

    /**
     * @brief 清除所有绑定
     *
     * 清除参数绑定并清空缓冲区。
     */
    void clear_bindings();

public:
    /**
     * @brief 构造函数
     * @param db SQLite数据库连接句柄
     * @param sql SQL语句（使用?作为占位符）
     *
     * 使用sqlite3_prepare_v2预处理SQL语句，
     * 获取参数数量（?占位符个数）。
     */
    explicit sqlite_prepared_statement(::sqlite3* db, const string& sql);

    sqlite_prepared_statement(const sqlite_prepared_statement&) = delete;
    sqlite_prepared_statement& operator=(const sqlite_prepared_statement&) = delete;

    /**
     * @brief 移动构造函数
     * @param other 源对象
     */
    sqlite_prepared_statement(sqlite_prepared_statement&& other) noexcept;

    /**
     * @brief 移动赋值运算符
     * @param other 源对象
     * @return 自身引用
     */
    sqlite_prepared_statement& operator=(sqlite_prepared_statement&& other) noexcept;

    /**
     * @brief 析构函数
     *
     * 调用sqlite3_finalize释放预处理语句资源。
     */
    ~sqlite_prepared_statement() override;

    /**
     * @brief 获取参数数量
     * @return SQL语句中的?占位符数量
     */
    NEFORCE_NODISCARD uint32_t param_count() const noexcept override { return param_count_; }

    /**
     * @brief 绑定字符串视图参数
     * @param index 参数索引（从1开始）
     * @param value 字符串值
     * @return 绑定成功返回true
     */
    bool bind_param(uint32_t index, string_view value) override;

    /**
     * @brief 绑定字符串参数
     * @param index 参数索引（从1开始）
     * @param value 字符串值
     * @return 绑定成功返回true
     */
    bool bind_param(uint32_t index, const string& value) override { return bind_param(index, value.view()); }

    /**
     * @brief 绑定C字符串参数
     * @param index 参数索引（从1开始）
     * @param value C字符串
     * @return 绑定成功返回true
     */
    bool bind_param(uint32_t index, const char* value) override { return bind_param(index, string_view(value)); }

    /**
     * @brief 绑定32位整数参数
     * @param index 参数索引（从1开始）
     * @param value 整数值
     * @return 绑定成功返回true
     */
    bool bind_param(uint32_t index, int32_t value) override;

    /**
     * @brief 绑定64位整数参数
     * @param index 参数索引（从1开始）
     * @param value 整数值
     * @return 绑定成功返回true
     */
    bool bind_param(uint32_t index, int64_t value) override;

    /**
     * @brief 绑定浮点数参数
     * @param index 参数索引（从1开始）
     * @param value 浮点值
     * @return 绑定成功返回true
     */
    bool bind_param(uint32_t index, float64_t value) override;

    /**
     * @brief 绑定二进制数据参数
     * @param index 参数索引（从1开始）
     * @param value 字节视图
     * @return 绑定成功返回true
     *
     * 适用于BLOB类型的参数绑定。
     */
    bool bind_param(uint32_t index, cbyte_view value) override;

    /**
     * @brief 执行非查询语句
     * @return 执行成功返回true
     *
     * 执行已绑定参数的预处理语句，不返回结果集。
     * 执行后自动重置语句并清除绑定。
     */
    bool execute() override;

    /**
     * @brief 执行查询语句
     * @return 查询结果集，失败返回空指针
     *
     * 执行查询并返回结果集对象。
     * 注意：结果集对象在析构时会重置语句，允许语句被复用。
     */
    unique_ptr<idb_prepared_result> execute_query() override;

    /**
     * @brief 获取错误信息
     * @return 错误描述字符串
     */
    NEFORCE_NODISCARD string_view get_error() const noexcept override { return last_error_.view(); }

    /**
     * @brief 获取错误码
     * @return SQLite错误码
     */
    NEFORCE_NODISCARD uint32_t get_errno() const noexcept override { return db_ ? ::sqlite3_errcode(db_) : 0; }
};

/** @} */ // Database

NEFORCE_END_NAMESPACE__
#endif
#endif // NEFORCE_DATABASE_SQLITE_PREPARED_STATEMENT_HPP__
