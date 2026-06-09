#ifndef NEFORCE_DATABASE_SQLITE_RESULT_HPP__
#define NEFORCE_DATABASE_SQLITE_RESULT_HPP__

/**
 * @file sqlite_result.hpp
 * @brief SQLite查询结果集实现
 *
 * 此文件提供了SQLite查询结果集的实现。
 */

#ifdef NEFORCE_SUPPORT_SQLITE3
#    ifdef NEFORCE_SUPPORT_SQLCIPHER
#        include <sqlcipher/sqlite3.h>
#    else
#        include <sqlite3.h>
#    endif
#    include "NeForce/db/db_interface.hpp"
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
 * @brief SQLite语句清理方式
 */
enum class sqlite_cleanup_action : uint8_t {
    finalize, ///< 调用sqlite3_finalize释放语句（用于普通查询）
    reset     ///< 调用sqlite3_reset重置语句（用于预处理语句复用）
};

/**
 * @struct sqlite_result
 * @brief SQLite查询结果集类
 *
 * 实现idb_tb_result接口，提供SQLite查询结果的访问功能。
 * 支持结果集遍历、列名获取和类型安全的数据读取。
 *
 * 主要功能：
 * - 结果集元数据获取
 * - 逐行遍历结果集
 * - 支持多种数据类型转换
 * - NULL值处理
 * - 自动语句资源管理（finalize或reset）
 *
 * @note 普通查询使用finalize模式（默认），预处理语句查询使用reset模式以允许语句复用。
 */
struct NEFORCE_API sqlite_result final : idb_tb_result {
private:
    ::sqlite3_stmt* stmt_ = nullptr;                                  ///< SQLite预处理语句句柄
    sqlite_cleanup_action cleanup_ = sqlite_cleanup_action::finalize; ///< 清理方式
    size_type cursor_ = 0;                                            ///< 当前行索引
    size_type columns_ = 0;                                           ///< 总列数

    unique_ptr<vector<string_view>> column_names_; ///< 列名列表
    unique_ptr<vector<int>> column_types_;         ///< 列类型列表

public:
    /**
     * @brief 默认构造函数
     *
     * 创建空结果集。
     */
    sqlite_result();

    /**
     * @brief 构造函数
     * @param statement SQLite预处理语句句柄
     *
     * 获取结果集的列数、列名和列类型信息。
     * 默认使用finalize清理方式。
     */
    explicit sqlite_result(::sqlite3_stmt* statement);

    /**
     * @brief 构造函数（带清理方式）
     * @param statement SQLite预处理语句句柄
     * @param cleanup 清理方式（finalize或reset）
     *
     * 预处理语句查询应使用reset方式，以允许语句复用。
     */
    sqlite_result(::sqlite3_stmt* statement, sqlite_cleanup_action cleanup);

    /**
     * @brief 析构函数
     *
     * 根据cleanup_标志调用sqlite3_finalize或sqlite3_reset。
     */
    ~sqlite_result() override;

    /**
     * @brief 获取结果集行数
     * @return 始终返回0
     * @deprecated SQLite无法高效获取行数，建议使用COUNT(*)
     */
    NEFORCE_NODISCARD NEFORCE_DEPRECATED_FOR("use COUNT(*) instead of using this function") size_type
            row_count() const noexcept override {
        return 0;
    }

    /**
     * @brief 获取结果集列数
     * @return 总列数
     */
    NEFORCE_NODISCARD size_type column_count() const noexcept override { return columns_; }

    /**
     * @brief 检查结果集是否为空
     * @return 空结果集返回true
     */
    NEFORCE_NODISCARD bool empty() const noexcept override { return stmt_ == nullptr; }

    /**
     * @brief 获取列名列表
     * @return 列名视图列表
     */
    NEFORCE_NODISCARD const vector<string_view>& column_names() const noexcept override { return *column_names_; }

    NEFORCE_NODISCARD column_meta column_metadata(size_type n) const override;

    /**
     * @brief 获取列类型列表
     * @return SQLite类型代码列表
     */
    NEFORCE_NODISCARD const vector<int>& column_types() const noexcept { return *column_types_; }

    /**
     * @brief 移动到下一行
     * @return 成功移动返回true，到达末尾返回false
     */
    NEFORCE_NODISCARD bool next() noexcept override;

    /**
     * @brief 获取字符串值
     * @param n 列索引（从0开始）
     * @return 字符串视图，NULL返回空字符串
     */
    NEFORCE_NODISCARD string_view get(size_type n) const noexcept override;

    /**
     * @brief 获取布尔值
     * @param n 列索引
     * @return 布尔值（非0为true）
     */
    NEFORCE_NODISCARD bool get_bool(size_type n) const override;

    /**
     * @brief 获取16位整数值
     * @param n 列索引
     * @return 16位整数
     */
    NEFORCE_NODISCARD int16_t get_int16(size_type n) const override;

    /**
     * @brief 获取32位整数值
     * @param n 列索引
     * @return 32位整数
     */
    NEFORCE_NODISCARD int32_t get_int32(size_type n) const override;

    /**
     * @brief 获取64位整数值
     * @param n 列索引
     * @return 64位整数
     */
    NEFORCE_NODISCARD int64_t get_int64(size_type n) const override;

    /**
     * @brief 获取32位浮点值
     * @param n 列索引
     * @return 32位浮点数
     */
    NEFORCE_NODISCARD float32_t get_float32(size_type n) const override;

    /**
     * @brief 获取64位浮点值
     * @param n 列索引
     * @return 64位浮点数
     */
    NEFORCE_NODISCARD float64_t get_float64(size_type n) const override;

    /**
     * @brief 获取十进制值
     * @param n 列索引
     * @return 十进制数对象
     */
    NEFORCE_NODISCARD decimal_t get_decimal(size_type n) const override;

    /**
     * @brief 获取BLOB二进制数据
     * @param n 列索引
     * @return 字节向量
     */
    NEFORCE_NODISCARD vector<char> get_blob(size_type n) const override;

    /**
     * @brief 获取BIT位字段值
     * @param n 列索引
     * @return 位字段的整数值
     */
    NEFORCE_NODISCARD uint64_t get_bit(size_type n) const noexcept override;

    /**
     * @brief 获取日期值
     * @param n 列索引
     * @return 日期对象
     */
    NEFORCE_NODISCARD date get_date(size_type n) const override { return get_datetime(n).date(); }

    /**
     * @brief 获取时间值
     * @param n 列索引
     * @return 时间对象
     */
    NEFORCE_NODISCARD time get_time(size_type n) const override { return get_datetime(n).time(); }

    /**
     * @brief 获取日期时间值
     * @param n 列索引
     * @return 日期时间对象
     *
     * 解析SQLite的文本格式日期时间
     */
    NEFORCE_NODISCARD datetime get_datetime(size_type n) const override;

    /**
     * @brief 获取时间戳值
     * @param n 列索引
     * @return 时间戳对象
     *
     * 将Unix时间戳整数转换为timestamp对象。
     */
    NEFORCE_NODISCARD timestamp get_timestamp(size_type n) const override;
};

/** @} */ // SQLite3

/** @} */ // Database

NEFORCE_END_NAMESPACE__
#endif
#endif // NEFORCE_DATABASE_SQLITE_RESULT_HPP__
