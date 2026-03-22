#ifndef NEFORCE_DATABASE_SQL_BUILDER_HPP__
#define NEFORCE_DATABASE_SQL_BUILDER_HPP__
#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/memory/unique_ptr.hpp"
NEFORCE_BEGIN_NAMESPACE__

#ifdef DELETE
#undef DELETE
#endif

enum class SQL_OPERATE_TYPE {
    SELECT, INSERT, UPDATE, DELETE
};

enum class SQL_JOIN_TYPE {
    INNER, LEFT, RIGHT, FULL
};

enum class SQL_ORDER_TYPE {
    ASC, DESC
};


struct select_data {
    vector<string> fields;
    vector<string> join_clauses;
    vector<string> group_by_fields;
    vector<string> having_conditions;
    vector<string> order_by_clauses;
    int limit_count = -1;
    int offset_count = -1;
    bool distinct = false;
};

struct insert_data {
    vector<string> fields;
    vector<string> placeholders;
};

struct update_data {
    vector<string> assignments;
};


class NEFORCE_API sql_builder {
private:
    SQL_OPERATE_TYPE sql_type_ = SQL_OPERATE_TYPE::SELECT;
    string table_;
    string table_alias_;
    vector<string> where_conditions_;

    unique_ptr<select_data> select_data_;
    unique_ptr<insert_data> insert_data_;
    unique_ptr<update_data> update_data_;

private:
    select_data* ensure_select_data();
    insert_data* ensure_insert_data();
    update_data* ensure_update_data();

    void clear_data() noexcept;

public:
    sql_builder() = default;
    ~sql_builder() = default;

    sql_builder(const sql_builder& other);
    sql_builder& operator =(const sql_builder& other);

    sql_builder(sql_builder&&) noexcept = default;
    sql_builder& operator =(sql_builder&&) noexcept = default;

    sql_builder& select(vector<string> fields);
    sql_builder& select(std::initializer_list<string> fields);
    sql_builder& select(string field);
    sql_builder& select_all() noexcept;

    sql_builder& distinct();

    sql_builder& from(string table) noexcept;
    sql_builder& from(string table, string alias) noexcept;

    sql_builder& join(SQL_JOIN_TYPE type, string table, string on_condition);
    sql_builder& join(string table, string on_condition);

    sql_builder& left_join(string table, string on_condition);
    sql_builder& right_join(string table, string on_condition);
    sql_builder& inner_join(string table, string on_condition);
    sql_builder& full_join(string table, string on_condition);

    sql_builder& where(string condition);

    sql_builder& where_eq(string field, string value);
    sql_builder& where_ne(string field, string value);
    sql_builder& where_gt(string field, string value);
    sql_builder& where_ge(string field, string value);
    sql_builder& where_lt(string field, string value);
    sql_builder& where_le(string field, string value);

    sql_builder& where_like(string field, string pattern);
    sql_builder& where_not_like(string field, string pattern);

    sql_builder& where_in(string field, vector<string> values);
    sql_builder& where_not_in(string field, vector<string> values);

    sql_builder& where_between(string field, string start, string end);
    sql_builder& where_not_between(string field, string start, string end);

    sql_builder& where_is_null(string field);
    sql_builder& where_is_not_null(string field);

    sql_builder& where_exists(string subquery);
    sql_builder& where_not_exists(string subquery);

    sql_builder& or_where(string condition);

    sql_builder& group_by(string field);
    sql_builder& group_by(vector<string> fields);

    sql_builder& having(string condition);

    sql_builder& order_by(string field, SQL_ORDER_TYPE order = SQL_ORDER_TYPE::ASC);
    sql_builder& order_by_asc(string field);
    sql_builder& order_by_desc(string field);

    sql_builder& limit(int count);
    sql_builder& offset(int count);
    sql_builder& page(int page_num, int page_size);

    sql_builder& insert_into(string table, vector<string> fields);
    sql_builder& insert_into(string table);

    sql_builder& values(vector<string> values);
    sql_builder& columns(vector<string> fields);

    sql_builder& update(string table);

    sql_builder& set(string assignment);
    sql_builder& set(string field, string value);

    sql_builder& set_increment(string field, int value = 1);
    sql_builder& set_decrement(string field, int value = 1);

    sql_builder& remove();
    sql_builder& delete_from(string table);

    sql_builder& select_count(string field, string alias);
    sql_builder& select_count(string field);
    sql_builder& select_count();

    sql_builder& select_sum(string field, string alias);
    sql_builder& select_sum(string field);

    sql_builder& select_avg(string field, string alias);
    sql_builder& select_avg(string field);

    sql_builder& select_max(string field, string alias);
    sql_builder& select_max(string field);

    sql_builder& select_min(string field, string alias);
    sql_builder& select_min(string field);

    sql_builder& select_distinct(string field);

    sql_builder& select_subquery(string subquery, string alias);
    sql_builder& select_subquery(string subquery);

    sql_builder& from_subquery(string subquery, string alias);

    sql_builder& reset() noexcept;

    NEFORCE_NODISCARD SQL_OPERATE_TYPE get_type() const noexcept { return sql_type_; }
    NEFORCE_NODISCARD string_view get_table() const noexcept { return table_.view(); }
    NEFORCE_NODISCARD bool empty() const noexcept { return table_.empty(); }

    NEFORCE_NODISCARD string build() const;
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_DATABASE_SQL_BUILDER_HPP__
