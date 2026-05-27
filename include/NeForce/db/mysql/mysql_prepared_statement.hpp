#ifndef NEFORCE_DATABASE_MYSQL_PREPARED_STATEMENT_HPP__
#define NEFORCE_DATABASE_MYSQL_PREPARED_STATEMENT_HPP__

/**
 * @file mysql_prepared_statement.hpp
 * @brief MySQL预处理语句实现
 *
 * 此文件提供了MySQL预处理语句的实现。
 */

#ifdef NEFORCE_SUPPORT_MYSQL
#    include <mysql/mysql.h>
#    include "NeForce/db/db_interface.hpp"
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
 * @class mysql_prepared_statement
 * @brief MySQL预处理语句类
 *
 * 实现idb_prepared_statement接口，支持参数绑定、语句执行和结果集获取。
 *
 * 主要功能：
 * - SQL语句预处理
 * - 参数绑定
 * - 语句执行
 * - 查询执行
 * - 错误信息获取
 *
 * @note MySQL使用?作为参数占位符，参数索引从0开始。
 */
class NEFORCE_API mysql_prepared_statement final : public idb_prepared_statement {
private:
    ::MYSQL_STMT* stmt_ = nullptr; ///< MySQL预处理语句句柄
    ::MYSQL* conn_ = nullptr;      ///< MySQL连接句柄
    uint32_t param_count_ = 0;     ///< 参数数量

    vector<::MYSQL_BIND> bind_params_;   ///< 参数绑定数组
    vector<vector<char>> param_buffers_; ///< 参数数据缓冲区

public:
    /**
     * @brief 构造函数
     * @param conn MySQL连接句柄
     * @param sql SQL语句
     * @throws database_stmt_exception 预处理失败时抛出
     *
     * 初始化预处理语句，获取参数数量。
     */
    mysql_prepared_statement(::MYSQL* conn, string_view sql);

    /**
     * @brief 移动构造函数
     * @param other 源对象
     */
    mysql_prepared_statement(mysql_prepared_statement&& other) noexcept;

    /**
     * @brief 移动赋值运算符
     * @param other 源对象
     * @return 自身引用
     */
    mysql_prepared_statement& operator=(mysql_prepared_statement&& other) noexcept;

    mysql_prepared_statement(const mysql_prepared_statement&) = delete;
    mysql_prepared_statement& operator=(const mysql_prepared_statement&) = delete;

    /**
     * @brief 析构函数
     *
     * 关闭预处理语句，释放资源。
     */
    ~mysql_prepared_statement() override;

    /**
     * @brief 获取参数数量
     * @return SQL语句中的?占位符数量
     */
    NEFORCE_NODISCARD uint32_t param_count() const noexcept override { return param_count_; }

    /**
     * @brief 绑定字符串参数
     * @param index 参数索引（从0开始）
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
    bool bind_param(uint32_t index, const char* value) override { return bind_param(index, string_view{value}); }

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
    NEFORCE_NODISCARD unique_ptr<idb_tb_result> execute_query() override;

    /**
     * @brief 获取错误信息
     * @return 错误描述字符串
     */
    NEFORCE_NODISCARD string_view get_error() const noexcept override;

    /**
     * @brief 获取错误码
     * @return MySQL错误码
     */
    NEFORCE_NODISCARD uint32_t get_errno() const noexcept override;
};

/** @} */ // MySQL

/** @} */ // Database

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_SUPPORT_MYSQL
#endif // NEFORCE_DATABASE_MYSQL_PREPARED_STATEMENT_HPP__
