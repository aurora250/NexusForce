#include <NeForce/core/utility/packages.hpp>
#include <NeForce/db/sql_builder.hpp>
NEFORCE_BEGIN_NAMESPACE__

sql_builder::select_data* sql_builder::ensure_select_data() {
    if (!select_data_) {
        select_data_ = make_unique<select_data>();
    }
    return select_data_.get();
}

sql_builder::insert_data* sql_builder::ensure_insert_data() {
    if (!insert_data_) {
        insert_data_ = make_unique<insert_data>();
    }
    return insert_data_.get();
}

sql_builder::update_data* sql_builder::ensure_update_data() {
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

sql_builder::sql_builder(const sql_builder& other) :
sql_type_(other.sql_type_),
table_(other.table_),
table_alias_(other.table_alias_),
where_conditions_(other.where_conditions_) {
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

sql_builder& sql_builder::operator=(const sql_builder& other) {
    if (addressof(other) == this) {
        return *this;
    }

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

    return *this;
}

sql_builder& sql_builder::select(vector<string> fields) {
    sql_type_ = sql_operate::SELECT;
    ensure_select_data()->fields = move(fields);
    return *this;
}

sql_builder& sql_builder::select(const std::initializer_list<string> fields) {
    sql_type_ = sql_operate::SELECT;
    auto* data = ensure_select_data();
    data->fields.clear();
    data->fields.reserve(fields.size());
    for (const auto& field: fields) {
        data->fields.emplace_back(move(field));
    }
    return *this;
}

sql_builder& sql_builder::select(string field) {
    sql_type_ = sql_operate::SELECT;
    ensure_select_data()->fields.emplace_back(move(field));
    return *this;
}

sql_builder& sql_builder::select_all() noexcept {
    sql_type_ = sql_operate::SELECT;
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
    table_ = move(table);
    return *this;
}

sql_builder& sql_builder::from(string table, string alias) noexcept {
    table_ = move(table);
    table_alias_ = move(alias);
    return *this;
}


sql_builder& sql_builder::join(const sql_join type, string table, string on_condition) {
    string join_str;
    switch (type) {
        case sql_join::INNER:
            join_str = "INNER JOIN ";
            break;
        case sql_join::LEFT:
            join_str = "LEFT JOIN ";
            break;
        case sql_join::RIGHT:
            join_str = "RIGHT JOIN ";
            break;
        case sql_join::FULL:
            join_str = "FULL JOIN ";
            break;
    }
    join_str += move(table) + " ON " + move(on_condition);
    ensure_select_data()->join_clauses.emplace_back(move(join_str));
    return *this;
}

sql_builder& sql_builder::join(string table, string on_condition) {
    return join(sql_join::INNER, move(table), move(on_condition));
}

sql_builder& sql_builder::left_join(string table, string on_condition) {
    return join(sql_join::LEFT, move(table), move(on_condition));
}

sql_builder& sql_builder::right_join(string table, string on_condition) {
    return join(sql_join::RIGHT, move(table), move(on_condition));
}

sql_builder& sql_builder::inner_join(string table, string on_condition) {
    return join(sql_join::INNER, move(table), move(on_condition));
}

sql_builder& sql_builder::full_join(string table, string on_condition) {
    return join(sql_join::FULL, move(table), move(on_condition));
}

sql_builder& sql_builder::where(string condition) {
    where_conditions_.emplace_back(move(condition));
    return *this;
}

sql_builder& sql_builder::where_eq(string field, string value) {
    where_conditions_.emplace_back(move(field) + " = " + move(value));
    return *this;
}

sql_builder& sql_builder::where_ne(string field, string value) {
    where_conditions_.emplace_back(move(field) + " != " + move(value));
    return *this;
}

sql_builder& sql_builder::where_gt(string field, string value) {
    where_conditions_.emplace_back(move(field) + " > " + move(value));
    return *this;
}

sql_builder& sql_builder::where_ge(string field, string value) {
    where_conditions_.emplace_back(move(field) + " >= " + move(value));
    return *this;
}

sql_builder& sql_builder::where_lt(string field, string value) {
    where_conditions_.emplace_back(move(field) + " < " + move(value));
    return *this;
}

sql_builder& sql_builder::where_le(string field, string value) {
    where_conditions_.emplace_back(move(field) + " <= " + move(value));
    return *this;
}

sql_builder& sql_builder::where_like(string field, string pattern) {
    where_conditions_.emplace_back(move(field) + " LIKE " + move(pattern));
    return *this;
}

sql_builder& sql_builder::where_not_like(string field, string pattern) {
    where_conditions_.emplace_back(move(field) + " NOT LIKE " + move(pattern));
    return *this;
}

sql_builder& sql_builder::where_in(string field, vector<string> values) {
    if (values.empty()) {
        return *this;
    }
    string condition = move(field) + " IN (";
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) {
            condition += ", ";
        }
        condition += move(values[i]);
    }
    condition += ")";
    where_conditions_.emplace_back(move(condition));
    return *this;
}

sql_builder& sql_builder::where_not_in(string field, vector<string> values) {
    if (values.empty()) {
        return *this;
    }
    string condition = move(field) + " NOT IN (";
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) {
            condition += ", ";
        }
        condition += move(values[i]);
    }
    condition += ")";
    where_conditions_.emplace_back(move(condition));
    return *this;
}

sql_builder& sql_builder::where_between(string field, string start, string end) {
    where_conditions_.emplace_back(move(field) + " BETWEEN " + move(start) + " AND " + move(end));
    return *this;
}

sql_builder& sql_builder::where_not_between(string field, string start, string end) {
    where_conditions_.emplace_back(move(field) + " NOT BETWEEN " + move(start) + " AND " + move(end));
    return *this;
}

sql_builder& sql_builder::where_is_null(string field) {
    where_conditions_.emplace_back(move(field) + " IS NULL");
    return *this;
}

sql_builder& sql_builder::where_is_not_null(string field) {
    where_conditions_.emplace_back(move(field) + " IS NOT NULL");
    return *this;
}

sql_builder& sql_builder::where_exists(string subquery) {
    where_conditions_.emplace_back("EXISTS (" + move(subquery) + ")");
    return *this;
}

sql_builder& sql_builder::where_not_exists(string subquery) {
    where_conditions_.emplace_back("NOT EXISTS (" + move(subquery) + ")");
    return *this;
}

sql_builder& sql_builder::or_where(string condition) {
    if (!where_conditions_.empty()) {
        where_conditions_.back() = "(" + move(where_conditions_.back()) + " OR " + move(condition) + ")";
    } else {
        where_conditions_.emplace_back(move(condition));
    }
    return *this;
}

sql_builder& sql_builder::group_by(string field) {
    ensure_select_data()->group_by_fields.emplace_back(move(field));
    return *this;
}

sql_builder& sql_builder::group_by(const vector<string>& fields) {
    auto* data = ensure_select_data();
    for (const auto& field: fields) {
        data->group_by_fields.emplace_back(field);
    }
    return *this;
}

sql_builder& sql_builder::having(string condition) {
    ensure_select_data()->having_conditions.emplace_back(move(condition));
    return *this;
}


sql_builder& sql_builder::order_by(string field, const sql_order order) {
    string order_str = move(field) + (order == sql_order::ASC ? " ASC" : " DESC");
    ensure_select_data()->order_by_clauses.emplace_back(move(order_str));
    return *this;
}

sql_builder& sql_builder::order_by_asc(string field) { return order_by(move(field), sql_order::ASC); }

sql_builder& sql_builder::order_by_desc(string field) { return order_by(move(field), sql_order::DESC); }

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
    sql_type_ = sql_operate::INSERT;
    table_ = move(table);
    auto* data = ensure_insert_data();
    data->fields = move(fields);
    data->placeholders.clear();
    data->placeholders.resize(data->fields.size(), "?");
    return *this;
}

sql_builder& sql_builder::insert_into(string table) {
    sql_type_ = sql_operate::INSERT;
    table_ = move(table);
    ensure_insert_data();
    return *this;
}

sql_builder& sql_builder::values(vector<string> values) {
    ensure_insert_data()->placeholders = move(values);
    return *this;
}

sql_builder& sql_builder::columns(vector<string> fields) {
    auto* data = ensure_insert_data();
    data->fields = move(fields);
    if (data->placeholders.empty()) {
        data->placeholders.resize(data->fields.size(), "?");
    }
    return *this;
}

sql_builder& sql_builder::update(string table) {
    sql_type_ = sql_operate::UPDATE;
    table_ = move(table);
    ensure_update_data();
    return *this;
}

sql_builder& sql_builder::set(string assignment) {
    ensure_update_data()->assignments.emplace_back(move(assignment));
    return *this;
}

sql_builder& sql_builder::set(string field, string value) {
    ensure_update_data()->assignments.emplace_back(move(field) + " = " + move(value));
    return *this;
}

sql_builder& sql_builder::set_increment(string field, const int value) {
    string field_copy = field;
    ensure_update_data()->assignments.emplace_back(move(field) + " = " + move(field_copy) + " + " + to_string(value));
    return *this;
}

sql_builder& sql_builder::set_decrement(string field, const int value) {
    string field_copy = field;
    ensure_update_data()->assignments.emplace_back(move(field) + " = " + move(field_copy) + " - " + to_string(value));
    return *this;
}

sql_builder& sql_builder::remove() {
    sql_type_ = sql_operate::DELETE;
    return *this;
}

sql_builder& sql_builder::delete_from(string table) {
    sql_type_ = sql_operate::DELETE;
    table_ = move(table);
    return *this;
}

sql_builder& sql_builder::select_count(string field, string alias) {
    string expr = "COUNT(" + move(field) + ")";
    if (!alias.empty()) {
        expr += " AS " + move(alias);
    }
    ensure_select_data()->fields.emplace_back(move(expr));
    return *this;
}

sql_builder& sql_builder::select_count(string field) {
    string expr = "COUNT(" + move(field) + ")";
    ensure_select_data()->fields.emplace_back(move(expr));
    return *this;
}

sql_builder& sql_builder::select_count() {
    ensure_select_data()->fields.emplace_back("COUNT(*)");
    return *this;
}

sql_builder& sql_builder::select_sum(string field, string alias) {
    string expr = "SUM(" + move(field) + ")";
    if (!alias.empty()) {
        expr += " AS " + move(alias);
    }
    ensure_select_data()->fields.emplace_back(move(expr));
    return *this;
}

sql_builder& sql_builder::select_sum(string field) {
    string expr = "SUM(" + move(field) + ")";
    ensure_select_data()->fields.emplace_back(move(expr));
    return *this;
}

sql_builder& sql_builder::select_avg(string field, string alias) {
    string expr = "AVG(" + move(field) + ")";
    if (!alias.empty()) {
        expr += " AS " + move(alias);
    }
    ensure_select_data()->fields.emplace_back(move(expr));
    return *this;
}

sql_builder& sql_builder::select_avg(string field) {
    string expr = "AVG(" + move(field) + ")";
    ensure_select_data()->fields.emplace_back(move(expr));
    return *this;
}

sql_builder& sql_builder::select_max(string field, string alias) {
    string expr = "MAX(" + move(field) + ")";
    if (!alias.empty()) {
        expr += " AS " + move(alias);
    }
    ensure_select_data()->fields.emplace_back(move(expr));
    return *this;
}

sql_builder& sql_builder::select_max(string field) {
    string expr = "MAX(" + move(field) + ")";
    ensure_select_data()->fields.emplace_back(move(expr));
    return *this;
}

sql_builder& sql_builder::select_min(string field, string alias) {
    string expr = "MIN(" + move(field) + ")";
    if (!alias.empty()) {
        expr += " AS " + move(alias);
    }
    ensure_select_data()->fields.emplace_back(move(expr));
    return *this;
}

sql_builder& sql_builder::select_min(string field) {
    string expr = "MIN(" + move(field) + ")";
    ensure_select_data()->fields.emplace_back(move(expr));
    return *this;
}

sql_builder& sql_builder::select_distinct(string field) {
    ensure_select_data()->distinct = true;
    ensure_select_data()->fields.emplace_back(move(field));
    return *this;
}

sql_builder& sql_builder::select_subquery(string subquery, string alias) {
    string expr = "(" + move(subquery) + ")";
    if (!alias.empty()) {
        expr += " AS " + move(alias);
    }
    ensure_select_data()->fields.emplace_back(move(expr));
    return *this;
}

sql_builder& sql_builder::select_subquery(string subquery) {
    string expr = "(" + move(subquery) + ")";
    ensure_select_data()->fields.emplace_back(move(expr));
    return *this;
}

sql_builder& sql_builder::from_subquery(string subquery, string alias) {
    table_ = "(" + move(subquery) + ")";
    table_alias_ = move(alias);
    return *this;
}

sql_builder& sql_builder::reset() noexcept {
    clear_data();
    sql_type_ = sql_operate::SELECT;
    table_.clear();
    table_alias_.clear();
    where_conditions_.clear();
    return *this;
}

string sql_builder::build() const {
    if (table_.empty()) {
        NEFORCE_THROW_EXCEPTION(value_exception("Table name is required"));
    }

    string result;
    switch (sql_type_) {
        case sql_operate::SELECT: {
            result += "SELECT ";
            if (select_data_ && select_data_->distinct) {
                result += "DISTINCT ";
            }

            if (!select_data_ || select_data_->fields.empty()) {
                result += "*";
            } else {
                for (size_t i = 0; i < select_data_->fields.size(); ++i) {
                    if (i > 0) {
                        result += ", ";
                    }
                    result += select_data_->fields[i];
                }
            }

            result += " FROM " + table_;
            if (!table_alias_.empty()) {
                result += " " + table_alias_;
            }

            // JOIN
            if (select_data_) {
                for (const auto& join: select_data_->join_clauses) {
                    result += " " + join;
                }
            }

            // WHERE
            if (!where_conditions_.empty()) {
                result += " WHERE ";
                for (size_t i = 0; i < where_conditions_.size(); ++i) {
                    if (i > 0) {
                        result += " AND ";
                    }
                    result += where_conditions_[i];
                }
            }

            // GROUP BY
            if (select_data_ && !select_data_->group_by_fields.empty()) {
                result += " GROUP BY ";
                for (size_t i = 0; i < select_data_->group_by_fields.size(); ++i) {
                    if (i > 0) {
                        result += ", ";
                    }
                    result += select_data_->group_by_fields[i];
                }
            }

            // HAVING
            if (select_data_ && !select_data_->having_conditions.empty()) {
                result += " HAVING ";
                for (size_t i = 0; i < select_data_->having_conditions.size(); ++i) {
                    if (i > 0) {
                        result += " AND ";
                    }
                    result += select_data_->having_conditions[i];
                }
            }

            // ORDER BY
            if (select_data_ && !select_data_->order_by_clauses.empty()) {
                result += " ORDER BY ";
                for (size_t i = 0; i < select_data_->order_by_clauses.size(); ++i) {
                    if (i > 0) {
                        result += ", ";
                    }
                    result += select_data_->order_by_clauses[i];
                }
            }

            // LIMIT & OFFSET
            if (select_data_) {
                if (select_data_->limit_count > 0) {
                    result += " LIMIT " + to_string(select_data_->limit_count);
                }
                if (select_data_->offset_count > 0) {
                    result += " OFFSET " + to_string(select_data_->offset_count);
                }
            }

            break;
        }
        case sql_operate::INSERT: {
            if (!insert_data_ || insert_data_->fields.empty()) {
                NEFORCE_THROW_EXCEPTION(value_exception("No fields for INSERT"));
            }
            result += "INSERT INTO " + table_ + " (";
            for (size_t i = 0; i < insert_data_->fields.size(); ++i) {
                if (i > 0) {
                    result += ", ";
                }
                result += insert_data_->fields[i];
            }
            result += ") VALUES (";
            for (size_t i = 0; i < insert_data_->placeholders.size(); ++i) {
                if (i > 0) {
                    result += ", ";
                }
                result += insert_data_->placeholders[i];
            }
            result += ")";
            break;
        }
        case sql_operate::UPDATE: {
            if (!update_data_ || update_data_->assignments.empty()) {
                NEFORCE_THROW_EXCEPTION(value_exception("No assignments for UPDATE"));
            }
            result += "UPDATE " + table_ + " SET ";
            for (size_t i = 0; i < update_data_->assignments.size(); ++i) {
                if (i > 0) {
                    result += ", ";
                }
                result += update_data_->assignments[i];
            }
            if (!where_conditions_.empty()) {
                result += " WHERE ";
                for (size_t i = 0; i < where_conditions_.size(); ++i) {
                    if (i > 0) {
                        result += " AND ";
                    }
                    result += where_conditions_[i];
                }
            }
            break;
        }
        case sql_operate::DELETE: {
            result += "DELETE FROM " + table_;
            if (!where_conditions_.empty()) {
                result += " WHERE ";
                for (size_t i = 0; i < where_conditions_.size(); ++i) {
                    if (i > 0) {
                        result += " AND ";
                    }
                    result += where_conditions_[i];
                }
            }
            break;
        }
        default: {
            NEFORCE_THROW_EXCEPTION(value_exception("Unsupported SQL type or not specified"));
            break;
        }
    }

    result += ";";

    return result;
}

NEFORCE_END_NAMESPACE__
