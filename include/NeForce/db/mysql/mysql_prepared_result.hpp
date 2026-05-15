#ifndef NEFORCE_DATABASE_MYSQL_PREPARED_RESULT_HPP__
#define NEFORCE_DATABASE_MYSQL_PREPARED_RESULT_HPP__

/**
 * @file mysql_prepared_result.hpp
 * @brief MySQL预处理语句结果集实现
 *
 * 此文件提供了MySQL预处理语句查询结果集的实现。
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
 * @class mysql_prepared_result
 * @brief MySQL预处理语句结果集类
 *
 * 实现idb_prepared_result接口，提供MySQL预处理语句查询结果的访问功能。
 *
 * 主要功能：
 * - 结果集元数据获取
 * - 逐行遍历结果集
 * - 支持多种数据类型转换
 * - 自动处理数据截断和NULL值
 */
class NEFORCE_API mysql_prepared_result final : public idb_prepared_result {
private:
    ::MYSQL_STMT* stmt_ = nullptr;    ///< MySQL预处理语句句柄
    ::MYSQL_RES* metadata_ = nullptr; ///< 结果集元数据
    uint32_t column_count_ = 0;       ///< 列数
    uint64_t row_count_ = 0;          ///< 行数
    bool has_current_row_ = false;    ///< 是否有当前行

    unique_ptr<vector<string_view>> column_names_ = make_unique<vector<string_view>>();               ///< 列名列表
    unique_ptr<vector<::enum_field_types>> column_types_ = make_unique<vector<::enum_field_types>>(); ///< 列类型列表

    unique_ptr<vector<::MYSQL_BIND>> bind_results_ = make_unique<vector<::MYSQL_BIND>>(); ///< 结果绑定数组
    unique_ptr<vector<vector<char>>> buffers_ = make_unique<vector<vector<char>>>();      ///< 数据缓冲区
    unique_ptr<vector<unsigned long>> lengths_ = make_unique<vector<unsigned long>>();    ///< 数据长度数组
    unique_ptr<vector<bool>> is_null_ = make_unique<vector<bool>>();                      ///< NULL标志数组
    unique_ptr<vector<bool>> is_error_ = make_unique<vector<bool>>();                     ///< 错误标志数组

    void initialize_bindings() const;

public:
    /**
     * @brief 构造函数
     * @param stmt MySQL预处理语句句柄
     * @throws database_stmt_exception 元数据获取失败时抛出
     *
     * 获取结果集元数据，初始化列绑定，存储结果集。
     */
    explicit mysql_prepared_result(::MYSQL_STMT* stmt);

    /**
     * @brief 析构函数
     *
     * 释放结果集元数据。
     */
    ~mysql_prepared_result() override;

    mysql_prepared_result(const mysql_prepared_result&) = delete;
    mysql_prepared_result& operator=(const mysql_prepared_result&) = delete;

    /**
     * @brief 检查结果集是否为空
     * @return 无数据返回true
     */
    NEFORCE_NODISCARD bool empty() const override { return row_count_ == 0; }

    /**
     * @brief 移动到下一行
     * @return 成功移动返回true，到达末尾返回false
     *
     * 获取下一行数据，处理数据截断情况。
     */
    bool next() override;

    /**
     * @brief 获取结果集行数
     * @return 总行数
     */
    NEFORCE_NODISCARD size_type row_count() const override { return row_count_; }

    /**
     * @brief 获取结果集列数
     * @return 总列数
     */
    NEFORCE_NODISCARD size_type column_count() const override { return column_count_; }

    /**
     * @brief 获取列名列表
     * @return 列名视图列表
     */
    NEFORCE_NODISCARD const vector<string_view>& column_names() const override { return *column_names_; }

    /**
     * @brief 获取列类型列表
     * @return MySQL字段类型列表
     */
    NEFORCE_NODISCARD const vector<::enum_field_types>& column_types() const { return *column_types_; }

    /**
     * @brief 获取字符串值
     * @param n 列索引（从0开始）
     * @return 字符串视图
     * @throws database_typecast_exception 类型不匹配时抛出
     */
    NEFORCE_NODISCARD string_view get(size_type n) const override;

    /**
     * @brief 获取布尔值
     * @param n 列索引
     * @return 布尔值
     * @throws database_typecast_exception 类型不匹配时抛出
     */
    NEFORCE_NODISCARD bool get_bool(size_type n) const override;

    /**
     * @brief 获取16位整数值
     * @param n 列索引
     * @return 16位整数
     * @throws database_typecast_exception 类型不匹配时抛出
     */
    NEFORCE_NODISCARD int16_t get_int16(size_type n) const override;

    /**
     * @brief 获取32位整数值
     * @param n 列索引
     * @return 32位整数
     * @throws database_typecast_exception 类型不匹配时抛出
     */
    NEFORCE_NODISCARD int32_t get_int32(size_type n) const override;

    /**
     * @brief 获取64位整数值
     * @param n 列索引
     * @return 64位整数
     * @throws database_typecast_exception 类型不匹配时抛出
     */
    NEFORCE_NODISCARD int64_t get_int64(size_type n) const override;

    /**
     * @brief 获取32位浮点值
     * @param n 列索引
     * @return 32位浮点数
     * @throws database_typecast_exception 类型不匹配时抛出
     */
    NEFORCE_NODISCARD float32_t get_float32(size_type n) const override;

    /**
     * @brief 获取64位浮点值
     * @param n 列索引
     * @return 64位浮点数
     * @throws database_typecast_exception 类型不匹配时抛出
     */
    NEFORCE_NODISCARD float64_t get_float64(size_type n) const override;

    /**
     * @brief 获取十进制值
     * @param n 列索引
     * @return 十进制数对象
     * @throws database_typecast_exception 类型不匹配时抛出
     */
    NEFORCE_NODISCARD decimal_t get_decimal(size_type n) const override;

    /**
     * @brief 获取BLOB二进制数据
     * @param n 列索引
     * @return 字节向量
     * @throws database_typecast_exception 类型不匹配时抛出
     */
    NEFORCE_NODISCARD vector<char> get_blob(size_type n) const override;

    /**
     * @brief 获取BIT位字段值
     * @param n 列索引
     * @return 位字段的整数值
     * @throws database_typecast_exception 类型不匹配时抛出
     */
    NEFORCE_NODISCARD uint64_t get_bit(size_type n) const override;

    /**
     * @brief 获取日期值
     * @param n 列索引
     * @return 日期对象
     * @throws database_typecast_exception 类型不匹配时抛出
     */
    NEFORCE_NODISCARD date get_date(size_type n) const override;

    /**
     * @brief 获取时间值
     * @param n 列索引
     * @return 时间对象
     * @throws database_typecast_exception 类型不匹配时抛出
     */
    NEFORCE_NODISCARD time get_time(size_type n) const override;

    /**
     * @brief 获取日期时间值
     * @param n 列索引
     * @return 日期时间对象
     * @throws database_typecast_exception 类型不匹配时抛出
     */
    NEFORCE_NODISCARD datetime get_datetime(size_type n) const override;

    /**
     * @brief 获取时间戳值
     * @param n 列索引
     * @return 时间戳对象
     * @throws database_typecast_exception 类型不匹配时抛出
     */
    NEFORCE_NODISCARD timestamp get_timestamp(size_type n) const override;
};

/** @} */ // MySQL

/** @} */ // Database

NEFORCE_END_NAMESPACE__
#endif
#endif // NEFORCE_DATABASE_MYSQL_PREPARED_RESULT_HPP__
