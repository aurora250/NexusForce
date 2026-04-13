#ifndef NEFORCE_DATABASE_PGSQL_PREPARED_RESULT_HPP__
#define NEFORCE_DATABASE_PGSQL_PREPARED_RESULT_HPP__

/**
 * @file pgsql_prepared_result.hpp
 * @brief PostgreSQL预处理语句结果集实现
 *
 * 此文件提供了PostgreSQL预处理语句结果集的实现。
 */

#ifdef NEFORCE_SUPPORT_POSTGRESQL
#    include "pgsql_result.hpp"
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
 * @class pgsql_prepared_result
 * @brief PostgreSQL预处理语句结果集类
 *
 * 实现idb_prepared_result接口，提供PostgreSQL预处理语句查询结果的访问功能。
 *
 * 主要功能：
 * - 结果集元数据获取
 * - 逐行遍历结果集
 * - 支持多种数据类型转换
 * - BYTEA二进制数据处理
 * - NULL值检测
 *
 * @note 该类与pgsql_tb_result功能完全相同，但类型上区分了普通查询和预处理语句查询，
 *       便于接口设计和类型安全。
 */
class NEFORCE_API pgsql_prepared_result final : public idb_prepared_result {
private:
    pgsql_tb_result impl_; ///< 内部持有的结果集

public:
    /**
     * @brief 构造函数
     * @param result PostgreSQL结果集句柄
     *
     * 创建预处理语句结果集对象，接管PGresult的所有权。
     */
    explicit pgsql_prepared_result(::PGresult* result) noexcept :
    impl_(result, true) {}

    /**
     * @brief 析构函数
     *
     * 自动释放PGresult资源。
     */
    ~pgsql_prepared_result() override = default;

    /**
     * @brief 检查结果集是否为空
     * @return 无数据返回true
     */
    NEFORCE_NODISCARD bool empty() const noexcept override { return impl_.empty(); }

    /**
     * @brief 移动到下一行
     * @return 成功移动返回true，到达末尾返回false
     */
    NEFORCE_NODISCARD bool next() noexcept override { return impl_.next(); }

    /**
     * @brief 获取结果集行数
     * @return 总行数
     */
    NEFORCE_NODISCARD size_type row_count() const noexcept override { return impl_.row_count(); }

    /**
     * @brief 获取结果集列数
     * @return 总列数
     */
    NEFORCE_NODISCARD size_type column_count() const noexcept override { return impl_.column_count(); }

    /**
     * @brief 获取列名列表
     * @return 列名视图列表
     */
    NEFORCE_NODISCARD const vector<string_view>& column_names() const override { return impl_.column_names(); }

    /**
     * @brief 获取字符串值
     * @param index 列索引（从0开始）
     * @return 字符串视图，NULL返回空字符串
     */
    NEFORCE_NODISCARD string_view get(size_type index) const override { return impl_.get(index); }

    /**
     * @brief 获取布尔值
     * @param index 列索引
     * @return 布尔值
     */
    NEFORCE_NODISCARD bool get_bool(size_type index) const override { return impl_.get_bool(index); }

    /**
     * @brief 获取16位整数值
     * @param index 列索引
     * @return 16位整数
     */
    NEFORCE_NODISCARD int16_t get_int16(size_type index) const override { return impl_.get_int16(index); }

    /**
     * @brief 获取32位整数值
     * @param index 列索引
     * @return 32位整数
     */
    NEFORCE_NODISCARD int32_t get_int32(size_type index) const override { return impl_.get_int32(index); }

    /**
     * @brief 获取64位整数值
     * @param index 列索引
     * @return 64位整数
     */
    NEFORCE_NODISCARD int64_t get_int64(size_type index) const override { return impl_.get_int64(index); }

    /**
     * @brief 获取32位浮点值
     * @param index 列索引
     * @return 32位浮点数
     */
    NEFORCE_NODISCARD float32_t get_float32(size_type index) const override { return impl_.get_float32(index); }

    /**
     * @brief 获取64位浮点值
     * @param index 列索引
     * @return 64位浮点数
     */
    NEFORCE_NODISCARD float64_t get_float64(size_type index) const override { return impl_.get_float64(index); }

    /**
     * @brief 获取十进制值
     * @param index 列索引
     * @return 十进制数对象
     */
    NEFORCE_NODISCARD decimal_t get_decimal(size_type index) const override { return impl_.get_decimal(index); }

    /**
     * @brief 获取BLOB二进制数据
     * @param index 列索引
     * @return 字节向量
     *
     * 自动处理PostgreSQL的BYTEA类型解码。
     */
    NEFORCE_NODISCARD vector<char> get_blob(size_type index) const override { return impl_.get_blob(index); }

    /**
     * @brief 获取BIT位字段值
     * @param index 列索引
     * @return 位字段的整数值
     */
    NEFORCE_NODISCARD uint64_t get_bit(size_type index) const override { return impl_.get_bit(index); }

    /**
     * @brief 获取日期值
     * @param index 列索引
     * @return 日期对象
     */
    NEFORCE_NODISCARD _NEFORCE date get_date(size_type index) const override { return impl_.get_date(index); }

    /**
     * @brief 获取时间值
     * @param index 列索引
     * @return 时间对象
     */
    NEFORCE_NODISCARD _NEFORCE time get_time(size_type index) const override { return impl_.get_time(index); }

    /**
     * @brief 获取日期时间值
     * @param index 列索引
     * @return 日期时间对象
     */
    NEFORCE_NODISCARD _NEFORCE datetime get_datetime(size_type index) const override {
        return impl_.get_datetime(index);
    }

    /**
     * @brief 获取时间戳值
     * @param index 列索引
     * @return 时间戳对象
     */
    NEFORCE_NODISCARD _NEFORCE timestamp get_timestamp(size_type index) const override {
        return impl_.get_timestamp(index);
    }
};

/** @} */ // PostgreSQL

/** @} */ // Database

NEFORCE_END_NAMESPACE__
#endif
#endif // NEFORCE_DATABASE_PGSQL_PREPARED_RESULT_HPP__
