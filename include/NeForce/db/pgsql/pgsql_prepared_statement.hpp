#ifndef NEFORCE_DATABASE_PGSQL_PREPARED_STATEMENT_HPP__
#define NEFORCE_DATABASE_PGSQL_PREPARED_STATEMENT_HPP__

/**
 * @file pgsql_prepared_statement.hpp
 * @brief PostgreSQL预处理语句实现
 *
 * 此文件提供了PostgreSQL预处理语句的实现。
 */

#ifdef NEFORCE_SUPPORT_POSTGRESQL
#    include <libpq-fe.h>
#    include "NeForce/db/db_interface.hpp"
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
 * @class pgsql_prepared_statement
 * @brief PostgreSQL预处理语句类
 *
 * 实现idb_prepared_statement接口，支持参数绑定、语句执行和结果集获取。
 *
 * 主要功能：
 * - SQL语句预处理（自动解析$1, $2等占位符）
 * - 参数绑定
 * - 语句执行
 * - 查询执行
 * - 自动生成预处理语句名称
 * - 语句生命周期管理
 *
 * @note PostgreSQL使用 $index 作为参数占位符，参数索引从1开始。
 */
class NEFORCE_API pgsql_prepared_statement final : public idb_prepared_statement {
private:
    struct pstmt_data {
        vector<string> param_values{};    ///< 参数值字符串
        vector<const char*> param_ptrs{}; ///< 参数指针数组
        vector<int> param_lengths{};      ///< 参数长度数组
        vector<int> param_formats{};      ///< 参数格式数组（0=文本，1=二进制）
    };

    ::PGconn* conn_ = nullptr;                                ///< PostgreSQL连接句柄
    string stmt_name_{};                                      ///< 预处理语句名称
    string sql_{};                                            ///< 原始SQL语句
    uint32_t param_count_ = 0;                                ///< 参数数量
    unique_ptr<pstmt_data> data_ = make_unique<pstmt_data>(); ///< 执行数据
    vector<vector<char>> param_buffers_{};                    ///< 二进制参数缓冲区
    string last_error_{};                                     ///< 最后错误信息
    uint32_t last_errno_ = 0;                                 ///< 最后错误码

    void set_error(string error, uint32_t errno_val = 0) noexcept;

public:
    /**
     * @brief 构造函数
     * @param conn PostgreSQL连接句柄
     * @param sql SQL语句
     *
     * 解析SQL语句获取参数数量，创建服务器端预处理语句。
     * 自动生成唯一的语句名称。
     */
    pgsql_prepared_statement(PGconn* conn, string sql);

    /**
     * @brief 析构函数
     *
     * 执行DEALLOCATE释放服务器端预处理语句资源。
     */
    ~pgsql_prepared_statement() override;

    /**
     * @brief 获取参数数量
     * @return SQL语句中的占位符数量
     */
    NEFORCE_NODISCARD uint32_t param_count() const noexcept override { return param_count_; }

    /**
     * @brief 绑定字符串参数
     * @param index 参数索引
     * @param value 字符串值
     * @return 绑定成功返回true
     */
    bool bind_param(uint32_t index, const string& value) override { return bind_param(index, value.view()); }

    /**
     * @brief 绑定字符串视图参数
     * @param index 参数索引
     * @param value 字符串视图
     * @return 绑定成功返回true
     */
    bool bind_param(uint32_t index, string_view value) override;

    /**
     * @brief 绑定C字符串参数
     * @param index 参数索引
     * @param value C字符串
     * @return 绑定成功返回true
     */
    bool bind_param(uint32_t index, const char* value) override { return bind_param(index, string_view(value)); }

    /**
     * @brief 绑定32位整数参数
     * @param index 参数索引
     * @param value 整数值
     * @return 绑定成功返回true
     */
    bool bind_param(uint32_t index, int32_t value) override;

    /**
     * @brief 绑定64位整数参数
     * @param index 参数索引
     * @param value 整数值
     * @return 绑定成功返回true
     */
    bool bind_param(uint32_t index, int64_t value) override;

    /**
     * @brief 绑定浮点数参数
     * @param index 参数索引
     * @param value 浮点值
     * @return 绑定成功返回true
     */
    bool bind_param(uint32_t index, float64_t value) override;

    /**
     * @brief 绑定二进制数据参数
     * @param index 参数索引
     * @param value 字节视图
     * @return 绑定成功返回true
     *
     * 二进制参数使用格式1（二进制）传输，适用于BYTEA类型。
     */
    bool bind_param(uint32_t index, cbyte_view value) override;

    /**
     * @brief 执行非查询语句
     * @return 执行成功返回true
     *
     * 执行已绑定参数的预处理语句，不返回结果集。
     */
    bool execute() override;

    /**
     * @brief 执行查询语句
     * @return 查询结果集，失败返回空指针
     *
     * 执行查询并返回结果集对象。
     */
    NEFORCE_NODISCARD unique_ptr<idb_prepared_result> execute_query() override;

    /**
     * @brief 获取错误信息
     * @return 错误描述字符串
     */
    NEFORCE_NODISCARD string_view get_error() const noexcept override { return last_error_.view(); }

    /**
     * @brief 获取错误码
     * @return PostgreSQL错误码
     */
    NEFORCE_NODISCARD uint32_t get_errno() const noexcept override { return last_errno_; }
};

/** @} */ // PostgreSQL

/** @} */ // Database

NEFORCE_END_NAMESPACE__
#endif
#endif // NEFORCE_DATABASE_PGSQL_PREPARED_STATEMENT_HPP__
