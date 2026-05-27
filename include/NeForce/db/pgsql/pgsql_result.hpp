#ifndef NEFORCE_DATABASE_PGSQL_RESULT_HPP__
#define NEFORCE_DATABASE_PGSQL_RESULT_HPP__

/**
 * @file pgsql_result.hpp
 * @brief PostgreSQL查询结果集实现
 *
 * 此文件提供了PostgreSQL查询结果集的实现。
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
 * @class pgsql_tb_result
 * @brief PostgreSQL查询结果集类
 *
 * 实现idb_tb_result接口，支持结果集遍历、列名获取和类型安全的数据读取。
 *
 * 主要功能：
 * - 结果集元数据获取
 * - 逐行遍历结果集
 * - 支持多种数据类型转换
 * - BYTEA二进制数据处理
 * - NULL值检测
 */
class NEFORCE_API pgsql_tb_result final : public idb_tb_result {
private:
    ::PGresult* result_ = nullptr;                      ///< PostgreSQL结果集句柄
    size_type current_row_{static_cast<size_type>(-1)}; ///< 当前行索引
    size_type row_count_ = 0;                           ///< 总行数
    size_type column_count_ = 0;                        ///< 总列数
    mutable vector<string_view> column_names_;          ///< 列名列表
    bool owns_result_;                                  ///< 是否拥有结果集所有权

public:
    /**
     * @brief 构造函数
     * @param result PostgreSQL结果集句柄
     * @param owns 是否拥有结果集所有权
     *
     * 获取结果集的行数和列数信息。
     */
    explicit pgsql_tb_result(::PGresult* result, bool owns = true);

    /**
     * @brief 析构函数
     *
     * 如果拥有所有权，释放PGresult资源。
     */
    ~pgsql_tb_result() override;

    pgsql_tb_result(const pgsql_tb_result&) = delete;
    pgsql_tb_result& operator=(const pgsql_tb_result&) = delete;

    /**
     * @brief 检查结果集是否为空
     * @return 无数据返回true
     */
    NEFORCE_NODISCARD bool empty() const noexcept override { return row_count_ == 0; }

    /**
     * @brief 检查当前行的指定列是否为NULL
     * @param index 列索引（从0开始）
     * @return NULL返回true
     * @throws database_exception 列索引越界时抛出
     */
    bool is_null(size_type index) const;

    /**
     * @brief 移动到下一行
     * @return 成功移动返回true，到达末尾返回false
     */
    NEFORCE_NODISCARD bool next() noexcept override;

    /**
     * @brief 获取结果集行数
     * @return 总行数
     */
    NEFORCE_NODISCARD size_type row_count() const noexcept override { return row_count_; }

    /**
     * @brief 获取结果集列数
     * @return 总列数
     */
    NEFORCE_NODISCARD size_type column_count() const noexcept override { return column_count_; }

    /**
     * @brief 获取列名列表
     * @return 列名视图列表
     *
     * 首次调用时从PGresult中提取列名并缓存。
     */
    NEFORCE_NODISCARD const vector<string_view>& column_names() const override;

    NEFORCE_NODISCARD column_meta column_metadata(size_type index) const override;

    /**
     * @brief 获取字符串值
     * @param index 列索引（从0开始）
     * @return 字符串视图，NULL返回空字符串
     */
    NEFORCE_NODISCARD string_view get(size_type index) const override;

    /**
     * @brief 获取布尔值
     * @param index 列索引
     * @return 布尔值
     * @throws database_typecast_exception 解析失败时抛出
     */
    NEFORCE_NODISCARD bool get_bool(size_type index) const override;

    /**
     * @brief 获取16位整数值
     * @param index 列索引
     * @return 16位整数
     * @throws database_typecast_exception 解析失败时抛出
     */
    NEFORCE_NODISCARD int16_t get_int16(size_type index) const override;

    /**
     * @brief 获取32位整数值
     * @param index 列索引
     * @return 32位整数
     * @throws database_typecast_exception 解析失败时抛出
     */
    NEFORCE_NODISCARD int32_t get_int32(size_type index) const override;

    /**
     * @brief 获取64位整数值
     * @param index 列索引
     * @return 64位整数
     * @throws database_typecast_exception 解析失败时抛出
     */
    NEFORCE_NODISCARD int64_t get_int64(size_type index) const override;

    /**
     * @brief 获取32位浮点值
     * @param index 列索引
     * @return 32位浮点数
     * @throws database_typecast_exception 解析失败时抛出
     */
    NEFORCE_NODISCARD float32_t get_float32(size_type index) const override;

    /**
     * @brief 获取64位浮点值
     * @param index 列索引
     * @return 64位浮点数
     * @throws database_typecast_exception 解析失败时抛出
     */
    NEFORCE_NODISCARD float64_t get_float64(size_type index) const override;

    /**
     * @brief 获取十进制值
     * @param index 列索引
     * @return 十进制数对象
     * @throws database_typecast_exception 解析失败时抛出
     */
    NEFORCE_NODISCARD decimal_t get_decimal(size_type index) const override;

    /**
     * @brief 获取BLOB二进制数据
     * @param index 列索引
     * @return 字节向量
     *
     * 自动处理PostgreSQL的BYTEA类型：
     * - 十六进制格式（\\x开头）自动解码
     * - 转义格式自动处理
     */
    NEFORCE_NODISCARD vector<char> get_blob(size_type index) const override;

    /**
     * @brief 获取BIT位字段值
     * @param index 列索引
     * @return 位字段的整数值
     * @throws database_typecast_exception 解析失败时抛出
     */
    NEFORCE_NODISCARD uint64_t get_bit(size_type index) const override;

    /**
     * @brief 获取日期值
     * @param index 列索引
     * @return 日期对象
     * @throws database_typecast_exception 解析失败时抛出
     */
    NEFORCE_NODISCARD date get_date(size_type index) const override;

    /**
     * @brief 获取时间值
     * @param index 列索引
     * @return 时间对象
     * @throws database_typecast_exception 解析失败时抛出
     */
    NEFORCE_NODISCARD time get_time(size_type index) const override;

    /**
     * @brief 获取日期时间值
     * @param index 列索引
     * @return 日期时间对象
     * @throws database_typecast_exception 解析失败时抛出
     */
    NEFORCE_NODISCARD datetime get_datetime(size_type index) const override;

    /**
     * @brief 获取时间戳值
     * @param index 列索引
     * @return 时间戳对象
     * @throws database_typecast_exception 解析失败时抛出
     */
    NEFORCE_NODISCARD timestamp get_timestamp(size_type index) const override;
};

/** @} */ // PostgreSQL

/** @} */ // Database

NEFORCE_END_NAMESPACE__
#endif
#endif // NEFORCE_DATABASE_PGSQL_RESULT_HPP__
