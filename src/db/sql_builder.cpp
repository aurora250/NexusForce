#include <MSTL/core/utilities/packages.hpp>
#include <MSTL/db/sql_builder.hpp>
MSTL_BEGIN_NAMESPACE__

select_data* sql_builder::ensure_select_data() {
    if (!select_data_) {
        select_data_ = make_unique<select_data>();
    }
    return select_data_.get();
}

insert_data* sql_builder::ensure_insert_data() {
    if (!insert_data_) {
        insert_data_ = make_unique<insert_data>();
    }
    return insert_data_.get();
}

update_data* sql_builder::ensure_update_data() {
    if (!update_data_) {
        update_data_ = make_unique<update_data>();
    }
    return update_data_.get();
}

void sql_builder::clear_data() noexcept {
    select_data_.reset();
    insert_data_.reset();
    update_data_.reset();
}

sql_builder::sql_builder(const sql_builder& other)
    : sql_type_(other.sql_type_), table_(other.table_),
    table_alias_(other.table_alias_), where_conditions_(other.where_conditions_) {
    if (other.select_data_) {
        select_data_ = make_unique<select_data>(*other.select_data_);
    }
    if (other.insert_data_) {
        insert_data_ = make_unique<insert_data>(*other.insert_data_);
    }
    if (other.update_data_) {
        update_data_ = make_unique<update_data>(*other.update_data_);
    }
}

sql_builder& sql_builder::operator =(const sql_builder& other) {
    if (this != &other) {
        sql_type_ = other.sql_type_;
        table_ = other.table_;
        table_alias_ = other.table_alias_;
        where_conditions_ = other.where_conditions_;

        if (other.select_data_) {
            select_data_ = make_unique<select_data>(*other.select_data_);
        } else {
            select_data_.reset();
        }

        if (other.insert_data_) {
            insert_data_ = make_unique<insert_data>(*other.insert_data_);
        } else {
            insert_data_.reset();
        }

        if (other.update_data_) {
            update_data_ = make_unique<update_data>(*other.update_data_);
        } else {
            update_data_.reset();
        }
    }
    return *this;
}

sql_builder& sql_builder::select(vector<string> fields) {
    sql_type_ = SQL_OPERATE_TYPE::SELECT;
    ensure_select_data()->fields = _MSTL move(fields);
    return *this;
}

sql_builder& sql_builder::select(const std::initializer_list<string> fields) {
    sql_type_ = SQL_OPERATE_TYPE::SELECT;
    auto* data = ensure_select_data();
    data->fields.clear();
    data->fields.reserve(fields.size());
    for (const auto& field : fields) {
        data->fields.emplace_back(_MSTL move(field));
    }
    return *this;
}

sql_builder& sql_builder::select(string field) {
    sql_type_ = SQL_OPERATE_TYPE::SELECT;
    ensure_select_data()->fields.emplace_back(_MSTL move(field));
    return *this;
}

sql_builder& sql_builder::select_all() noexcept {
    sql_type_ = SQL_OPERATE_TYPE::SELECT;
    if (select_data_) {
        select_data_->fields.clear();
    }
    return *this;
}

sql_builder& sql_builder::distinct() {
    ensure_select_data()->distinct = true;
    return *this;
}

sql_builder& sql_builder::from(string table) noexcept {
    table_ = _MSTL move(table);
    return *this;
}

sql_builder& sql_builder::from(string table, string alias) noexcept {
    table_ = _MSTL move(table);
    table_alias_ = _MSTL move(alias);
    return *this;
}


sql_builder& sql_builder::join(const SQL_JOIN_TYPE type, string table, string on_condition) {
    string join_str;
    switch (type) {
        case SQL_JOIN_TYPE::INNER: join_str = "INNER JOIN "; break;
        case SQL_JOIN_TYPE::LEFT: join_str = "LEFT JOIN "; break;
        case SQL_JOIN_TYPE::RIGHT: join_str = "RIGHT JOIN "; break;
        case SQL_JOIN_TYPE::FULL: join_str = "FULL JOIN "; break;
    }
    join_str += _MSTL move(table) + " ON " + _MSTL move(on_condition);
    ensure_select_data()->join_clauses.emplace_back(_MSTL move(join_str));
    return *this;
}

sql_builder& sql_builder::join(string table, string on_condition) {
    return join(SQL_JOIN_TYPE::INNER, _MSTL move(table), _MSTL move(on_condition));
}

sql_builder& sql_builder::left_join(string table, string on_condition) {
    return join(SQL_JOIN_TYPE::LEFT, _MSTL move(table), _MSTL move(on_condition));
}

sql_builder& sql_builder::right_join(string table, string on_condition) {
    return join(SQL_JOIN_TYPE::RIGHT, _MSTL move(table), _MSTL move(on_condition));
}

sql_builder& sql_builder::inner_join(string table, string on_condition) {
    return join(SQL_JOIN_TYPE::INNER, _MSTL move(table), _MSTL move(on_condition));
}

sql_builder& sql_builder::full_join(string table, string on_condition) {
    return join(SQL_JOIN_TYPE::FULL, _MSTL move(table), _MSTL move(on_condition));
}

sql_builder& sql_builder::where(string condition) {
    where_conditions_.emplace_back(_MSTL move(condition));
    return *this;
}

sql_builder& sql_builder::where_eq(string field, string value) {
    where_conditions_.emplace_back(_MSTL move(field) + " = " + _MSTL move(value));
    return *this;
}

sql_builder& sql_builder::where_ne(string field, string value) {
    where_conditions_.emplace_back(_MSTL move(field) + " != " + _MSTL move(value));
    return *this;
}

sql_builder& sql_builder::where_gt(string field, string value) {
    where_conditions_.emplace_back(_MSTL move(field) + " > " + _MSTL move(value));
    return *this;
}

sql_builder& sql_builder::where_ge(string field, string value) {
    where_conditions_.emplace_back(_MSTL move(field) + " >= " + _MSTL move(value));
    return *this;
}

sql_builder& sql_builder::where_lt(string field, string value) {
    where_conditions_.emplace_back(_MSTL move(field) + " < " + _MSTL move(value));
    return *this;
}

sql_builder& sql_builder::where_le(string field, string value) {
    where_conditions_.emplace_back(_MSTL move(field) + " <= " + _MSTL move(value));
    return *this;
}

sql_builder& sql_builder::where_like(string field, string pattern) {
    where_conditions_.emplace_back(_MSTL move(field) + " LIKE " + _MSTL move(pattern));
    return *this;
}

sql_builder& sql_builder::where_not_like(string field, string pattern) {
    where_conditions_.emplace_back(_MSTL move(field) + " NOT LIKE " + _MSTL move(pattern));
    return *this;
}

sql_builder& sql_builder::where_in(string field, vector<string> values) {
    if (values.empty()) return *this;
    string condition = _MSTL move(field) + " IN (";
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) condition += ", ";
        condition += _MSTL move(values[i]);
    }
    condition += ")";
    where_conditions_.emplace_back(_MSTL move(condition));
    return *this;
}

sql_builder& sql_builder::where_not_in(string field, vector<string> values) {
    if (values.empty()) return *this;
    string condition = _MSTL move(field) + " NOT IN (";
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) condition += ", ";
        condition += _MSTL move(values[i]);
    }
    condition += ")";
    where_conditions_.emplace_back(_MSTL move(condition));
    return *this;
}

sql_builder& sql_builder::where_between(string field, string start, string end) {
    where_conditions_.emplace_back(_MSTL move(field) + " BETWEEN " + _MSTL move(start) + " AND " + _MSTL move(end));
    return *this;
}

sql_builder& sql_builder::where_not_between(string field, string start, string end) {
    where_conditions_.emplace_back(_MSTL move(field) + " NOT BETWEEN " + _MSTL move(start) + " AND " + _MSTL move(end));
    return *this;
}

sql_builder& sql_builder::where_is_null(string field) {
    where_conditions_.emplace_back(_MSTL move(field) + " IS NULL");
    return *this;
}

sql_builder& sql_builder::where_is_not_null(string field) {
    where_conditions_.emplace_back(_MSTL move(field) + " IS NOT NULL");
    return *this;
}

sql_builder& sql_builder::where_exists(string subquery) {
    where_conditions_.emplace_back("EXISTS (" + _MSTL move(subquery) + ")");
    return *this;
}

sql_builder& sql_builder::where_not_exists(string subquery) {
    where_conditions_.emplace_back("NOT EXISTS (" + _MSTL move(subquery) + ")");
    return *this;
}

sql_builder& sql_builder::or_where(string condition) {
    if (!where_conditions_.empty()) {
        where_conditions_.back() =
            "(" + _MSTL move(where_conditions_.back()) + " OR " + _MSTL move(condition) + ")";
    } else {
        where_conditions_.emplace_back(_MSTL move(condition));
    }
    return *this;
}

sql_builder& sql_builder::group_by(string field) {
    ensure_select_data()->group_by_fields.emplace_back(_MSTL move(field));
    return *this;
}

sql_builder& sql_builder::group_by(vector<string> fields) {
    auto* data = ensure_select_data();
    for (const auto& field : fields) {
        data->group_by_fields.emplace_back(_MSTL move(field));
    }
    return *this;
}

sql_builder& sql_builder::having(string condition) {
    ensure_select_data()->having_conditions.emplace_back(_MSTL move(condition));
    return *this;
}


sql_builder& sql_builder::order_by(string field, const SQL_ORDER_TYPE order) {
    string order_str = _MSTL move(field) + (order == SQL_ORDER_TYPE::ASC ? " ASC" : " DESC");
    ensure_select_data()->order_by_clauses.emplace_back(_MSTL move(order_str));
    return *this;
}

sql_builder& sql_builder::order_by_asc(string field) {
    return order_by(_MSTL move(field), SQL_ORDER_TYPE::ASC);
}

sql_builder& sql_builder::order_by_desc(string field) {
    return order_by(_MSTL move(field), SQL_ORDER_TYPE::DESC);
}

sql_builder& sql_builder::limit(const int count) {
    ensure_select_data()->limit_count = count;
    return *this;
}

sql_builder& sql_builder::offset(const int count) {
    ensure_select_data()->offset_count = count;
    return *this;
}

sql_builder& sql_builder::page(const int page_num, const int page_size) {
    auto* data = ensure_select_data();
    data->limit_count = page_size;
    data->offset_count = (page_num - 1) * page_size;
    return *this;
}

sql_builder& sql_builder::insert_into(string table, vector<string> fields) {
    sql_type_ = SQL_OPERATE_TYPE::INSERT;
    table_ = _MSTL move(table);
    auto* data = ensure_insert_data();
    data->fields = _MSTL move(fields);
    data->placeholders.clear();
    data->placeholders.resize(data->fields.size(), "?");
    return *this;
}

sql_builder& sql_builder::insert_into(string table) {
    sql_type_ = SQL_OPERATE_TYPE::INSERT;
    table_ = _MSTL move(table);
    ensure_insert_data();
    return *this;
}

sql_builder& sql_builder::values(vector<string> values) {
    ensure_insert_data()->placeholders = _MSTL move(values);
    return *this;
}

sql_builder& sql_builder::columns(vector<string> fields) {
    auto* data = ensure_insert_data();
    data->fields = _MSTL move(fields);
    if (data->placeholders.empty()) {
        data->placeholders.resize(data->fields.size(), "?");
    }
    return *this;
}

sql_builder& sql_builder::update(string table) {
    sql_type_ = SQL_OPERATE_TYPE::UPDATE;
    table_ = _MSTL move(table);
    ensure_update_data();
    return *this;
}

sql_builder& sql_builder::set(string assignment) {
    ensure_update_data()->assignments.emplace_back(_MSTL move(assignment));
    return *this;
}

sql_builder& sql_builder::set(string field, string value) {
    ensure_update_data()->assignments.emplace_back(_MSTL move(field) + " = " + _MSTL move(value));
    return *this;
}

sql_builder& sql_builder::set_increment(string field, const int value) {
    ensure_update_data()->assignments.emplace_back(
        field + " = " + _MSTL move(field) + " + " + _MSTL to_string(value));
    return *this;
}

sql_builder& sql_builder::set_decrement(string field, const int value) {
    ensure_update_data()->assignments.emplace_back(
        _MSTL move(field) + " = " + field + " - " + _MSTL to_string(value));
    return *this;
}

sql_builder& sql_builder::remove() {
    sql_type_ = SQL_OPERATE_TYPE::DELETE;
    return *this;
}

sql_builder& sql_builder::delete_from(string table) {
    sql_type_ = SQL_OPERATE_TYPE::DELETE;
    table_ = _MSTL move(table);
    return *this;
}

sql_builder& sql_builder::select_count(string field, string alias) {
    string expr = "COUNT(" + _MSTL move(field) + ")";
    if (!alias.empty()) expr += " AS " + _MSTL move(alias);
    ensure_select_data()->fields.emplace_back(_MSTL move(expr));
    return *this;
}

sql_builder& sql_builder::select_count(string field) {
    string expr = "COUNT(" + _MSTL move(field) + ")";
    ensure_select_data()->fields.emplace_back(_MSTL move(expr));
    return *this;
}

sql_builder& sql_builder::select_count() {
    ensure_select_data()->fields.emplace_back("COUNT(*)");
    return *this;
}

sql_builder& sql_builder::select_sum(string field, string alias) {
    string expr = "SUM(" + _MSTL move(field) + ")";
    if (!alias.empty()) expr += " AS " + _MSTL move(alias);
    ensure_select_data()->fields.emplace_back(_MSTL move(expr));
    return *this;
}

sql_builder& sql_builder::select_sum(string field) {
    string expr = "SUM(" + _MSTL move(field) + ")";
    ensure_select_data()->fields.emplace_back(_MSTL move(expr));
    return *this;
}

sql_builder& sql_builder::select_avg(string field, string alias) {
    string expr = "AVG(" + _MSTL move(field) + ")";
    if (!alias.empty()) expr += " AS " + _MSTL move(alias);
    ensure_select_data()->fields.emplace_back(_MSTL move(expr));
    return *this;
}

sql_builder& sql_builder::select_avg(string field) {
    string expr = "AVG(" + _MSTL move(field) + ")";
    ensure_select_data()->fields.emplace_back(_MSTL move(expr));
    return *this;
}

sql_builder& sql_builder::select_max(string field, string alias) {
    string expr = "MAX(" + _MSTL move(field) + ")";
    if (!alias.empty()) expr += " AS " + _MSTL move(alias);
    ensure_select_data()->fields.emplace_back(_MSTL move(expr));
    return *this;
}

sql_builder& sql_builder::select_max(string field) {
    string expr = "MAX(" + _MSTL move(field) + ")";
    ensure_select_data()->fields.emplace_back(_MSTL move(expr));
    return *this;
}

sql_builder& sql_builder::select_min(string field, string alias) {
    string expr = "MIN(" + _MSTL move(field) + ")";
    if (!alias.empty()) expr += " AS " + _MSTL move(alias);
    ensure_select_data()->fields.emplace_back(_MSTL move(expr));
    return *this;
}

sql_builder& sql_builder::select_min(string field) {
    string expr = "MIN(" + _MSTL move(field) + ")";
    ensure_select_data()->fields.emplace_back(_MSTL move(expr));
    return *this;
}

sql_builder& sql_builder::select_distinct(string field) {
    ensure_select_data()->distinct = true;
    ensure_select_data()->fields.emplace_back(_MSTL move(field));
    return *this;
}

sql_builder& sql_builder::select_subquery(string subquery, string alias) {
    string expr = "(" + _MSTL move(subquery) + ")";
    if (!alias.empty()) expr += " AS " + _MSTL move(alias);
    ensure_select_data()->fields.emplace_back(_MSTL move(expr));
    return *this;
}

sql_builder& sql_builder::select_subquery(string subquery) {
    string expr = "(" + _MSTL move(subquery) + ")";
    ensure_select_data()->fields.emplace_back(_MSTL move(expr));
    return *this;
}

sql_builder& sql_builder::from_subquery(string subquery, string alias) {
    table_ = "(" + _MSTL move(subquery) + ")";
    table_alias_ = _MSTL move(alias);
    return *this;
}

sql_builder& sql_builder::reset() noexcept {
    clear_data();
    sql_type_ = SQL_OPERATE_TYPE::SELECT;
    table_.clear();
    table_alias_.clear();
    where_conditions_.clear();
    return *this;
}

string sql_builder::build() const {
    string result;
    switch (sql_type_) {
        case SQL_OPERATE_TYPE::SELECT: {
            result += "SELECT ";
            if (select_data_ && select_data_->distinct) {
                result += "DISTINCT ";
            }

            if (!select_data_ || select_data_->fields.empty()) {
                result += "*";
            } else {
                for (size_t i = 0; i < select_data_->fields.size(); ++i) {
                    if (i > 0) result += ", ";
                    result += select_data_->fields[i];
                }
            }

            result += " FROM " + table_;
            if (!table_alias_.empty()) {
                result += " " + table_alias_;
            }

            // JOIN
            if (select_data_) {
                for (const auto& join : select_data_->join_clauses) {
                    result += " " + join;
                }
            }

            // WHERE
            if (!where_conditions_.empty()) {
                result += " WHERE ";
                for (size_t i = 0; i < where_conditions_.size(); ++i) {
                    if (i > 0) result += " AND ";
                    result += where_conditions_[i];
                }
            }

            // GROUP BY
            if (select_data_ && !select_data_->group_by_fields.empty()) {
                result += " GROUP BY ";
                for (size_t i = 0; i < select_data_->group_by_fields.size(); ++i) {
                    if (i > 0) result += ", ";
                    result += select_data_->group_by_fields[i];
                }
            }

            // HAVING
            if (select_data_ && !select_data_->having_conditions.empty()) {
                result += " HAVING ";
                for (size_t i = 0; i < select_data_->having_conditions.size(); ++i) {
                    if (i > 0) result += " AND ";
                    result += select_data_->having_conditions[i];
                }
            }

            // ORDER BY
            if (select_data_ && !select_data_->order_by_clauses.empty()) {
                result += " ORDER BY ";
                for (size_t i = 0; i < select_data_->order_by_clauses.size(); ++i) {
                    if (i > 0) result += ", ";
                    result += select_data_->order_by_clauses[i];
                }
            }

            // LIMIT & OFFSET
            if (select_data_) {
                if (select_data_->limit_count > 0) {
                    result += " LIMIT " + _MSTL to_string(select_data_->limit_count);
                }
                if (select_data_->offset_count > 0) {
                    result += " OFFSET " + _MSTL to_string(select_data_->offset_count);
                }
            }

            break;
        }
        case SQL_OPERATE_TYPE::INSERT: {
            if (!insert_data_ || insert_data_->fields.empty()) {
                Exception(ValueError("No fields for INSERT"));
            }
            result += "INSERT INTO " + table_ + " (";
            for (size_t i = 0; i < insert_data_->fields.size(); ++i) {
                if (i > 0) result += ", ";
                result += insert_data_->fields[i];
            }
            result += ") VALUES (";
            for (size_t i = 0; i < insert_data_->placeholders.size(); ++i) {
                if (i > 0) result += ", ";
                result += insert_data_->placeholders[i];
            }
            result += ")";
            break;
        }
        case SQL_OPERATE_TYPE::UPDATE: {
            if (!update_data_ || update_data_->assignments.empty()) {
                Exception(ValueError("No assignments for UPDATE"));
            }
            result += "UPDATE " + table_ + " SET ";
            for (size_t i = 0; i < update_data_->assignments.size(); ++i) {
                if (i > 0) result += ", ";
                result += update_data_->assignments[i];
            }
            if (!where_conditions_.empty()) {
                result += " WHERE ";
                for (size_t i = 0; i < where_conditions_.size(); ++i) {
                    if (i > 0) result += " AND ";
                    result += where_conditions_[i];
                }
            }
            break;
        }
        case SQL_OPERATE_TYPE::DELETE: {
            result += "DELETE FROM " + table_;
            if (!where_conditions_.empty()) {
                result += " WHERE ";
                for (size_t i = 0; i < where_conditions_.size(); ++i) {
                    if (i > 0) result += " AND ";
                    result += where_conditions_[i];
                }
            }
            break;
        }
        default: {
            Exception(ValueError("Unsupported SQL type or not specified"));
            break;
        }
    }
    result += ";";
    return result;
}

MSTL_END_NAMESPACE__
