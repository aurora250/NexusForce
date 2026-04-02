#ifndef NEFORCE_DATABASE_SQL_BUILDER_HPP__
#define NEFORCE_DATABASE_SQL_BUILDER_HPP__

/**
 * @file sql_builder.hpp
 * @brief SQL语句构建器
 *
 * 此文件提供了SQL语句的流式构建器，支持SELECT、INSERT、UPDATE、DELETE操作。
 * 通过链式调用构建复杂的SQL语句，避免手动拼接SQL字符串的安全风险。
 */

#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/memory/unique_ptr.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup SQL SQL
 * @brief 数据库相关功能
 * @{
 */

#ifdef DELETE
#undef DELETE
#endif

/**
 * @enum sql_operate
 * @brief SQL操作类型枚举
 */
enum class sql_operate {
    SELECT,  ///< SELECT查询
    INSERT,  ///< INSERT插入
    UPDATE,  ///< UPDATE更新
    DELETE   ///< DELETE删除
};

/**
 * @enum sql_join
 * @brief JOIN类型枚举
 */
enum class sql_join {
    INNER,  ///< INNER JOIN
    LEFT,   ///< LEFT JOIN
    RIGHT,  ///< RIGHT JOIN
    FULL    ///< FULL JOIN
};

/**
 * @enum sql_order
 * @brief 排序方向枚举
 */
enum class sql_order {
    ASC,   ///< 升序
    DESC   ///< 降序
};


/**
 * @class sql_builder
 * @brief SQL语句构建器
 *
 * 提供流式API构建SQL语句：
 * - 支持SELECT、INSERT、UPDATE、DELETE操作
 * - 支持JOIN、WHERE、GROUP BY、ORDER BY、LIMIT等子句
 * - 支持聚合函数（COUNT、SUM、AVG、MAX、MIN）
 * - 支持子查询
 * - 自动处理SQL语法
 */
class NEFORCE_API sql_builder {
public:
    /**
     * @struct select_data
     * @brief SELECT语句的数据结构
     */
    struct select_data {
        vector<string> fields;              ///< 查询字段列表
        vector<string> join_clauses;        ///< JOIN子句列表
        vector<string> group_by_fields;     ///< GROUP BY字段列表
        vector<string> having_conditions;   ///< HAVING条件列表
        vector<string> order_by_clauses;    ///< ORDER BY子句列表
        int limit_count = -1;               ///< LIMIT数量
        int offset_count = -1;              ///< OFFSET数量
        bool distinct = false;              ///< 是否DISTINCT
    };

    /**
     * @struct insert_data
     * @brief INSERT语句的数据结构
     */
    struct insert_data {
        vector<string> fields;          ///< 插入字段列表
        vector<string> placeholders;    ///< 值占位符列表
    };

    /**
     * @struct update_data
     * @brief UPDATE语句的数据结构
     */
    struct update_data {
        vector<string> assignments;     ///< 赋值表达式列表
    };

private:
    sql_operate sql_type_ = sql_operate::SELECT;   ///< 当前SQL操作类型
    string table_;                                 ///< 主表名
    string table_alias_;                           ///< 主表别名
    vector<string> where_conditions_;              ///< WHERE条件列表

    unique_ptr<select_data> select_data_;          ///< SELECT专用数据
    unique_ptr<insert_data> insert_data_;          ///< INSERT专用数据
    unique_ptr<update_data> update_data_;          ///< UPDATE专用数据

private:
    select_data* ensure_select_data();
    insert_data* ensure_insert_data();
    update_data* ensure_update_data();

    void clear_data() noexcept;

public:
    /**
     * @brief 默认构造函数
     */
    sql_builder() = default;

    /**
     * @brief 析构函数
     */
    ~sql_builder() = default;

    /**
     * @brief 拷贝构造函数
     * @param other 源对象
     */
    sql_builder(const sql_builder& other);

    /**
     * @brief 拷贝赋值运算符
     * @param other 源对象
     * @return 自身引用
     */
    sql_builder& operator =(const sql_builder& other);

    sql_builder(sql_builder&&) noexcept = default;
    sql_builder& operator =(sql_builder&&) noexcept = default;

    /**
     * @brief 列表设置SELECT字段
     * @param fields 字段列表
     * @return 自身引用
     */
    sql_builder& select(vector<string> fields);

    /**
     * @brief 初始化列表设置SELECT字段
     * @param fields 字段列表
     * @return 自身引用
     */
    sql_builder& select(std::initializer_list<string> fields);

    /**
     * @brief 添加SELECT字段
     * @param field 字段名
     * @return 自身引用
     */
    sql_builder& select(string field);

    /**
     * @brief 选择所有字段（SELECT *）
     * @return 自身引用
     */
    sql_builder& select_all() noexcept;

    /**
     * @brief 添加DISTINCT关键字
     * @return 自身引用
     */
    sql_builder& distinct();

    /**
     * @brief 设置主表
     * @param table 表名
     * @return 自身引用
     */
    sql_builder& from(string table) noexcept;

    /**
     * @brief 设置主表并指定别名
     * @param table 表名
     * @param alias 表别名
     * @return 自身引用
     */
    sql_builder& from(string table, string alias) noexcept;

    /**
     * @brief 添加JOIN子句
     * @param type JOIN类型
     * @param table 连接表名
     * @param on_condition ON条件
     * @return 自身引用
     */
    sql_builder& join(sql_join type, string table, string on_condition);

    /**
     * @brief 添加INNER JOIN子句
     * @param table 连接表名
     * @param on_condition ON条件
     * @return 自身引用
     */
    sql_builder& join(string table, string on_condition);

    /**
     * @brief 添加LEFT JOIN子句
     * @param table 连接表名
     * @param on_condition ON条件
     * @return 自身引用
     */
    sql_builder& left_join(string table, string on_condition);

    /**
     * @brief 添加RIGHT JOIN子句
     * @param table 连接表名
     * @param on_condition ON条件
     * @return 自身引用
     */
    sql_builder& right_join(string table, string on_condition);

    /**
     * @brief 添加INNER JOIN子句
     * @param table 连接表名
     * @param on_condition ON条件
     * @return 自身引用
     */
    sql_builder& inner_join(string table, string on_condition);

    /**
     * @brief 添加FULL JOIN子句
     * @param table 连接表名
     * @param on_condition ON条件
     * @return 自身引用
     */
    sql_builder& full_join(string table, string on_condition);

    /**
     * @brief 添加WHERE条件
     * @param condition 条件表达式
     * @return 自身引用
     */
    sql_builder& where(string condition);

    /**
     * @brief 添加相等条件
     * @param field 字段名
     * @param value 值
     * @return 自身引用
     */
    sql_builder& where_eq(string field, string value);

    /**
     * @brief 添加不等条件
     * @param field 字段名
     * @param value 值
     * @return 自身引用
     */
    sql_builder& where_ne(string field, string value);

    /**
     * @brief 添加大于条件
     * @param field 字段名
     * @param value 值
     * @return 自身引用
     */
    sql_builder& where_gt(string field, string value);

    /**
     * @brief 添加大于等于条件
     * @param field 字段名
     * @param value 值
     * @return 自身引用
     */
    sql_builder& where_ge(string field, string value);

    /**
     * @brief 添加小于条件
     * @param field 字段名
     * @param value 值
     * @return 自身引用
     */
    sql_builder& where_lt(string field, string value);

    /**
     * @brief 添加小于等于条件
     * @param field 字段名
     * @param value 值
     * @return 自身引用
     */
    sql_builder& where_le(string field, string value);

    /**
     * @brief 添加LIKE条件
     * @param field 字段名
     * @param pattern 模式
     * @return 自身引用
     */
    sql_builder& where_like(string field, string pattern);

    /**
     * @brief 添加NOT LIKE条件
     * @param field 字段名
     * @param pattern 模式
     * @return 自身引用
     */
    sql_builder& where_not_like(string field, string pattern);

    /**
     * @brief 添加IN条件
     * @param field 字段名
     * @param values 值列表
     * @return 自身引用
     */
    sql_builder& where_in(string field, vector<string> values);

    /**
     * @brief 添加NOT IN条件
     * @param field 字段名
     * @param values 值列表
     * @return 自身引用
     */
    sql_builder& where_not_in(string field, vector<string> values);

    /**
     * @brief 添加BETWEEN条件
     * @param field 字段名
     * @param start 起始值
     * @param end 结束值
     * @return 自身引用
     */
    sql_builder& where_between(string field, string start, string end);

    /**
     * @brief 添加NOT BETWEEN条件
     * @param field 字段名
     * @param start 起始值
     * @param end 结束值
     * @return 自身引用
     */
    sql_builder& where_not_between(string field, string start, string end);

    /**
     * @brief 添加IS NULL条件
     * @param field 字段名
     * @return 自身引用
     */
    sql_builder& where_is_null(string field);

    /**
     * @brief 添加IS NOT NULL条件
     * @param field 字段名
     * @return 自身引用
     */
    sql_builder& where_is_not_null(string field);

    /**
     * @brief 添加EXISTS条件
     * @param subquery 子查询
     * @return 自身引用
     */
    sql_builder& where_exists(string subquery);

    /**
     * @brief 添加NOT EXISTS条件
     * @param subquery 子查询
     * @return 自身引用
     */
    sql_builder& where_not_exists(string subquery);

    /**
     * @brief 添加OR条件（与上一个条件组合）
     * @param condition 条件表达式
     * @return 自身引用
     *
     * 将当前条件与上一个条件用OR连接。
     */
    sql_builder& or_where(string condition);

    /**
     * @brief 添加GROUP BY字段
     * @param field 字段名
     * @return 自身引用
     */
    sql_builder& group_by(string field);

    /**
     * @brief 添加GROUP BY字段列表
     * @param fields 字段列表
     * @return 自身引用
     */
    sql_builder& group_by(vector<string> fields);

    /**
     * @brief 添加HAVING条件
     * @param condition 条件表达式
     * @return 自身引用
     */
    sql_builder& having(string condition);

    /**
     * @brief 添加ORDER BY子句
     * @param field 字段名
     * @param order 排序方向
     * @return 自身引用
     */
    sql_builder& order_by(string field, sql_order order = sql_order::ASC);

    /**
     * @brief 添加升序排序
     * @param field 字段名
     * @return 自身引用
     */
    sql_builder& order_by_asc(string field);

    /**
     * @brief 添加降序排序
     * @param field 字段名
     * @return 自身引用
     */
    sql_builder& order_by_desc(string field);

    /**
     * @brief 添加LIMIT限制
     * @param count 返回行数
     * @return 自身引用
     */
    sql_builder& limit(int count);

    /**
     * @brief 添加OFFSET偏移
     * @param count 偏移行数
     * @return 自身引用
     */
    sql_builder& offset(int count);

    /**
     * @brief 添加分页
     * @param page_num 页码（从1开始）
     * @param page_size 每页大小
     * @return 自身引用
     */
    sql_builder& page(int page_num, int page_size);

    /**
     * @brief 设置INSERT INTO表名和字段
     * @param table 表名
     * @param fields 字段列表
     * @return 自身引用
     */
    sql_builder& insert_into(string table, vector<string> fields);

    /**
     * @brief 设置INSERT INTO表名
     * @param table 表名
     * @return 自身引用
     */
    sql_builder& insert_into(string table);

    /**
     * @brief 设置VALUES占位符
     * @param values 值占位符列表（如"?"或实际值）
     * @return 自身引用
     */
    sql_builder& values(vector<string> values);

    /**
     * @brief 设置INSERT字段列表
     * @param fields 字段列表
     * @return 自身引用
     */
    sql_builder& columns(vector<string> fields);

    /**
     * @brief 设置UPDATE表名
     * @param table 表名
     * @return 自身引用
     */
    sql_builder& update(string table);

    /**
     * @brief 添加SET赋值
     * @param assignment 赋值表达式（如"field = value"）
     * @return 自身引用
     */
    sql_builder& set(string assignment);

    /**
     * @brief 添加SET赋值（字段=值形式）
     * @param field 字段名
     * @param value 值
     * @return 自身引用
     */
    sql_builder& set(string field, string value);

    /**
     * @brief 添加自增赋值
     * @param field 字段名
     * @param value 增加量，默认为1
     * @return 自身引用
     */
    sql_builder& set_increment(string field, int value = 1);

    /**
     * @brief 添加自减赋值
     * @param field 字段名
     * @param value 减少量，默认为1
     * @return 自身引用
     */
    sql_builder& set_decrement(string field, int value = 1);

    /**
     * @brief 设置DELETE操作
     * @return 自身引用
     */
    sql_builder& remove();

    /**
     * @brief 设置DELETE FROM表名
     * @param table 表名
     * @return 自身引用
     */
    sql_builder& delete_from(string table);

    sql_builder& select_count(string field, string alias);   ///< COUNT聚合
    sql_builder& select_count(string field);                 ///< COUNT聚合（无别名）
    sql_builder& select_count();                             ///< COUNT(*)

    sql_builder& select_sum(string field, string alias);     ///< SUM聚合
    sql_builder& select_sum(string field);                   ///< SUM聚合（无别名）

    sql_builder& select_avg(string field, string alias);     ///< AVG聚合
    sql_builder& select_avg(string field);                   ///< AVG聚合（无别名）

    sql_builder& select_max(string field, string alias);     ///< MAX聚合
    sql_builder& select_max(string field);                   ///< MAX聚合（无别名）

    sql_builder& select_min(string field, string alias);     ///< MIN聚合
    sql_builder& select_min(string field);                   ///< MIN聚合（无别名）

    /**
     * @brief 添加DISTINCT字段
     * @param field 字段名
     * @return 自身引用
     */
    sql_builder& select_distinct(string field);

    /**
     * @brief 添加子查询作为SELECT字段
     * @param subquery 子查询SQL
     * @param alias 别名
     * @return 自身引用
     */
    sql_builder& select_subquery(string subquery, string alias);

    /**
     * @brief 添加子查询作为SELECT字段（无别名）
     * @param subquery 子查询SQL
     * @return 自身引用
     */
    sql_builder& select_subquery(string subquery);

    /**
     * @brief 使用子查询作为FROM表
     * @param subquery 子查询SQL
     * @param alias 别名
     * @return 自身引用
     */
    sql_builder& from_subquery(string subquery, string alias);

    /**
     * @brief 重置构建器状态
     * @return 自身引用
     */
    sql_builder& reset() noexcept;

    /**
     * @brief 获取当前SQL操作类型
     * @return 操作类型
     */
    NEFORCE_NODISCARD sql_operate type() const noexcept { return sql_type_; }

    /**
     * @brief 获取当前表名
     * @return 表名视图
     */
    NEFORCE_NODISCARD string_view table() const noexcept { return table_.view(); }

    /**
     * @brief 检查构建器是否为空
     * @return 未设置表名返回true
     */
    NEFORCE_NODISCARD bool is_empty() const noexcept { return table_.empty(); }

    /**
     * @brief 构建最终的SQL语句
     * @return 完整的SQL字符串
     * @throws value_exception 当SQL结构不完整时抛出
     *
     * 根据当前构建的状态生成完整的SQL语句。
     */
    NEFORCE_NODISCARD string build() const;
};

/** @} */ // SQL

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_DATABASE_SQL_BUILDER_HPP__
