#ifndef NEFORCE_DATABASE_MYSQL_RESULT_HPP__
#define NEFORCE_DATABASE_MYSQL_RESULT_HPP__

/**
 * @file mysql_result.hpp
 * @brief MySQL查询结果集实现
 *
 * 此文件提供了MySQL查询结果集的实现。
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
 * @struct mysql_result
 * @brief MySQL查询结果集类
 *
 * 实现idb_tb_result接口，支持结果集遍历、列名获取和类型安全的数据读取。
 *
 * 主要功能：
 * - 结果集元数据获取
 * - 逐行遍历结果集
 * - 支持多种数据类型转换
 * - 自动处理NULL值和类型转换
 */
struct NEFORCE_API mysql_result final : idb_tb_result {
private:
    ::MYSQL_RES* result_ = nullptr; ///< MySQL结果集句柄
    ::MYSQL_ROW cursor_ = nullptr;  ///< 当前行指针
    size_type rows_ = 0;            ///< 总行数
    size_type columns_ = 0;         ///< 总列数

    unique_ptr<vector<string_view>> column_name_;         ///< 列名列表
    unique_ptr<vector<::enum_field_types>> column_types_; ///< 列类型列表

public:
    /**
     * @brief 默认构造函数
     *
     * 创建空结果集。
     */
    mysql_result();

    /**
     * @brief 构造函数
     * @param result MySQL结果集句柄
     *
     * 获取结果集元数据。
     */
    explicit mysql_result(::MYSQL_RES* result);

    /**
     * @brief 析构函数
     *
     * 释放结果集资源。
     */
    ~mysql_result() override;

    /**
     * @brief 检查结果集是否为空
     * @return 空结果集返回true
     */
    NEFORCE_NODISCARD bool empty() const noexcept override { return result_ == nullptr; }

    /**
     * @brief 获取结果集行数
     * @return 总行数
     */
    NEFORCE_NODISCARD size_type row_count() const noexcept override { return rows_; }

    /**
     * @brief 获取结果集列数
     * @return 总列数
     */
    NEFORCE_NODISCARD size_type column_count() const noexcept override { return columns_; }

    /**
     * @brief 获取列名列表
     * @return 列名视图列表
     */
    NEFORCE_NODISCARD const vector<string_view>& column_names() const noexcept override { return *column_name_; }

    NEFORCE_NODISCARD column_meta column_metadata(size_type n) const override;

    /**
     * @brief 获取列类型列表
     * @return MySQL字段类型列表
     */
    NEFORCE_NODISCARD const vector<::enum_field_types>& column_types() const noexcept { return *column_types_; }

    /**
     * @brief 移动到下一行
     * @return 成功移动返回true，到达末尾返回false
     */
    NEFORCE_NODISCARD bool next() noexcept override;

    /**
     * @brief 获取字符串值
     * @param n 列索引（从0开始）
     * @return 字符串视图
     */
    NEFORCE_NODISCARD string_view get(size_type n) const noexcept override;

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
#endif // NEFORCE_SUPPORT_MYSQL
#endif // NEFORCE_DATABASE_MYSQL_RESULT_HPP__
