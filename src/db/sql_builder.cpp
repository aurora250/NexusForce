#include <NeForce/core/utility/packages.hpp>
#include <NeForce/db/sql_builder.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    string build_window_clause(const vector<string>& partition_by, const vector<string>& order_by) {
        string clause = " OVER (";
        if (!partition_by.empty()) {
            clause += "PARTITION BY ";
            for (size_t i = 0; i < partition_by.size(); ++i) {
                if (i > 0) {
                    clause += ", ";
                }
                clause += partition_by[i];
            }
            if (!order_by.empty()) {
                clause += " ";
            }
        }
        if (!order_by.empty()) {
            clause += "ORDER BY ";
            for (size_t i = 0; i < order_by.size(); ++i) {
                if (i > 0) {
                    clause += ", ";
                }
                clause += order_by[i];
            }
        }
        clause += ")";
        return clause;
    }
} // namespace


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

sql_builder::create_table_data* sql_builder::ensure_create_table_data() {
    if (!create_table_data_) {
        create_table_data_ = make_unique<create_table_data>();
    }
    return create_table_data_.get();
}

sql_builder::create_view_data* sql_builder::ensure_create_view_data() {
    if (!create_view_data_) {
        create_view_data_ = make_unique<create_view_data>();
    }
    return create_view_data_.get();
}

sql_builder::alter_table_data* sql_builder::ensure_alter_table_data() {
    if (!alter_table_data_) {
        alter_table_data_ = make_unique<alter_table_data>();
    }
    return alter_table_data_.get();
}

sql_builder::create_index_data* sql_builder::ensure_create_index_data() {
    if (!create_index_data_) {
        create_index_data_ = make_unique<create_index_data>();
    }
    return create_index_data_.get();
}

void sql_builder::clear_data() noexcept {
    select_data_.reset();
    insert_data_.reset();
    update_data_.reset();
    create_table_data_.reset();
    create_view_data_.reset();
    alter_table_data_.reset();
    create_index_data_.reset();
}

sql_builder::sql_builder(const sql_builder& other) :
sql_type_(other.sql_type_),
dialect_(other.dialect_),
param_seq_(other.param_seq_),
table_(other.table_),
table_alias_(other.table_alias_),
where_conditions_(other.where_conditions_),
set_operations_(other.set_operations_),
cte_entries_(other.cte_entries_),
drop_if_exists_(other.drop_if_exists_) {
    if (other.select_data_) {
        select_data_ = make_unique<select_data>(*other.select_data_);
    }
    if (other.insert_data_) {
        insert_data_ = make_unique<insert_data>(*other.insert_data_);
    }
    if (other.update_data_) {
        update_data_ = make_unique<update_data>(*other.update_data_);
    }
    if (other.create_table_data_) {
        create_table_data_ = make_unique<create_table_data>(*other.create_table_data_);
    }
    if (other.create_view_data_) {
        create_view_data_ = make_unique<create_view_data>(*other.create_view_data_);
    }
    if (other.alter_table_data_) {
        alter_table_data_ = make_unique<alter_table_data>(*other.alter_table_data_);
    }
    if (other.create_index_data_) {
        create_index_data_ = make_unique<create_index_data>(*other.create_index_data_);
    }
}

sql_builder& sql_builder::operator=(const sql_builder& other) {
    if (addressof(other) == this) {
        return *this;
    }

    sql_type_ = other.sql_type_;
    dialect_ = other.dialect_;
    param_seq_ = other.param_seq_;
    table_ = other.table_;
    table_alias_ = other.table_alias_;
    where_conditions_ = other.where_conditions_;
    set_operations_ = other.set_operations_;
    cte_entries_ = other.cte_entries_;
    drop_if_exists_ = other.drop_if_exists_;

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

    if (other.create_table_data_) {
        create_table_data_ = make_unique<create_table_data>(*other.create_table_data_);
    } else {
        create_table_data_.reset();
    }

    if (other.create_view_data_) {
        create_view_data_ = make_unique<create_view_data>(*other.create_view_data_);
    } else {
        create_view_data_.reset();
    }

    if (other.alter_table_data_) {
        alter_table_data_ = make_unique<alter_table_data>(*other.alter_table_data_);
    } else {
        alter_table_data_.reset();
    }

    if (other.create_index_data_) {
        create_index_data_ = make_unique<create_index_data>(*other.create_index_data_);
    } else {
        create_index_data_.reset();
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

sql_builder& sql_builder::group_by_rollup(vector<string> fields) {
    auto* data = ensure_select_data();
    data->group_by_fields.clear();
    data->cube_fields.clear();
    data->rollup_fields = move(fields);
    return *this;
}

sql_builder& sql_builder::group_by_cube(vector<string> fields) {
    auto* data = ensure_select_data();
    data->group_by_fields.clear();
    data->rollup_fields.clear();
    data->cube_fields = move(fields);
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
    auto* data = ensure_select_data();
    data->limit_count = count;
    data->fetch_count = -1; // LIMIT takes precedence over FETCH FIRST
    return *this;
}

sql_builder& sql_builder::offset(const int count) {
    ensure_select_data()->offset_count = count;
    return *this;
}

sql_builder& sql_builder::fetch_first(const int row_count) {
    auto* data = ensure_select_data();
    data->fetch_count = row_count;
    data->limit_count = -1; // FETCH FIRST takes precedence over LIMIT
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
    data->placeholders.reserve(data->fields.size());
    for (size_t i = 0; i < data->fields.size(); ++i) {
        data->placeholders.push_back(make_placeholder(i + 1));
    }
    param_seq_ = data->fields.size();
    return *this;
}

sql_builder& sql_builder::insert_into(string table) {
    sql_type_ = sql_operate::INSERT;
    table_ = move(table);
    ensure_insert_data();
    return *this;
}

sql_builder& sql_builder::values(vector<string> values) {
    auto* data = ensure_insert_data();
    data->placeholders = move(values);
    data->extra_rows.clear();
    return *this;
}

sql_builder& sql_builder::values(std::initializer_list<string> values) {
    auto* data = ensure_insert_data();
    // convert to vector first to avoid pre-existing SSO boundary bug
    // when directly assigning initializer_list to vector<string> with LONG strings.
    vector<string> tmp(values);
    data->placeholders = move(tmp);
    data->extra_rows.clear();
    return *this;
}

sql_builder& sql_builder::values(vector<vector<string>> value_rows) {
    auto* data = ensure_insert_data();
    data->placeholders.clear();
    data->extra_rows.clear();
    if (value_rows.empty()) {
        return *this;
    }
    data->placeholders = move(value_rows[0]);
    for (size_t i = 1; i < value_rows.size(); ++i) {
        data->extra_rows.push_back(move(value_rows[i]));
    }
    return *this;
}

sql_builder& sql_builder::add_values(vector<string> values) {
    ensure_insert_data()->extra_rows.push_back(move(values));
    return *this;
}

sql_builder& sql_builder::columns(vector<string> fields) {
    auto* data = ensure_insert_data();
    data->fields = move(fields);
    if (data->placeholders.empty()) {
        data->placeholders.reserve(data->fields.size());
        for (size_t i = 0; i < data->fields.size(); ++i) {
            data->placeholders.push_back(make_placeholder(i + 1));
        }
        param_seq_ = data->fields.size();
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

sql_builder& sql_builder::drop_table(string table) {
    sql_type_ = sql_operate::DROP_TABLE;
    table_ = move(table);
    drop_if_exists_ = false;
    return *this;
}

sql_builder& sql_builder::drop_table_if_exists(string table) {
    sql_type_ = sql_operate::DROP_TABLE;
    table_ = move(table);
    drop_if_exists_ = true;
    return *this;
}

sql_builder& sql_builder::with_(string name, string query) {
    cte_entries_.push_back({move(name), move(query), false});
    return *this;
}

sql_builder& sql_builder::with_recursive(string name, string query) {
    cte_entries_.push_back({move(name), move(query), true});
    return *this;
}

sql_builder& sql_builder::cross_join(string table) {
    ensure_select_data()->join_clauses.emplace_back("CROSS JOIN " + move(table));
    return *this;
}

sql_builder& sql_builder::select_row_number(string alias, const vector<string>& partition_by,
                                            const vector<string>& order_by) {
    string expr = "ROW_NUMBER()" + build_window_clause(partition_by, order_by);
    if (!alias.empty()) {
        expr += " AS " + move(alias);
    }
    ensure_select_data()->fields.emplace_back(move(expr));
    return *this;
}

sql_builder& sql_builder::select_rank(string alias, const vector<string>& partition_by,
                                      const vector<string>& order_by) {
    string expr = "RANK()" + build_window_clause(partition_by, order_by);
    if (!alias.empty()) {
        expr += " AS " + move(alias);
    }
    ensure_select_data()->fields.emplace_back(move(expr));
    return *this;
}

sql_builder& sql_builder::select_dense_rank(string alias, const vector<string>& partition_by,
                                            const vector<string>& order_by) {
    string expr = "DENSE_RANK()" + build_window_clause(partition_by, order_by);
    if (!alias.empty()) {
        expr += " AS " + move(alias);
    }
    ensure_select_data()->fields.emplace_back(move(expr));
    return *this;
}

sql_builder& sql_builder::select_ntile(const int buckets, string alias, const vector<string>& partition_by,
                                       const vector<string>& order_by) {
    string expr = "NTILE(" + to_string(buckets) + ")" + build_window_clause(partition_by, order_by);
    if (!alias.empty()) {
        expr += " AS " + move(alias);
    }
    ensure_select_data()->fields.emplace_back(move(expr));
    return *this;
}

sql_builder& sql_builder::select_lead(string field, string alias, const vector<string>& partition_by,
                                      const vector<string>& order_by) {
    string expr = "LEAD(" + move(field) + ")" + build_window_clause(partition_by, order_by);
    if (!alias.empty()) {
        expr += " AS " + move(alias);
    }
    ensure_select_data()->fields.emplace_back(move(expr));
    return *this;
}

sql_builder& sql_builder::select_lag(string field, string alias, const vector<string>& partition_by,
                                     const vector<string>& order_by) {
    string expr = "LAG(" + move(field) + ")" + build_window_clause(partition_by, order_by);
    if (!alias.empty()) {
        expr += " AS " + move(alias);
    }
    ensure_select_data()->fields.emplace_back(move(expr));
    return *this;
}

sql_builder& sql_builder::select_first_value(string field, string alias, const vector<string>& partition_by,
                                             const vector<string>& order_by) {
    string expr = "FIRST_VALUE(" + move(field) + ")" + build_window_clause(partition_by, order_by);
    if (!alias.empty()) {
        expr += " AS " + move(alias);
    }
    ensure_select_data()->fields.emplace_back(move(expr));
    return *this;
}

sql_builder& sql_builder::select_last_value(string field, string alias, const vector<string>& partition_by,
                                            const vector<string>& order_by) {
    string expr = "LAST_VALUE(" + move(field) + ")" + build_window_clause(partition_by, order_by);
    if (!alias.empty()) {
        expr += " AS " + move(alias);
    }
    ensure_select_data()->fields.emplace_back(move(expr));
    return *this;
}

string sql_builder::make_case_simple(string field, vector<pair<string, string>> when_then, string else_val) {
    string result = "CASE " + move(field);
    for (auto& [when_val, then_val]: when_then) {
        result += " WHEN " + move(when_val) + " THEN " + move(then_val);
    }
    if (!else_val.empty()) {
        result += " ELSE " + move(else_val);
    }
    result += " END";
    return result;
}

string sql_builder::make_case_searched(vector<pair<string, string>> when_then, string else_val) {
    string result = "CASE";
    for (auto& [condition, then_val]: when_then) {
        result += " WHEN " + move(condition) + " THEN " + move(then_val);
    }
    if (!else_val.empty()) {
        result += " ELSE " + move(else_val);
    }
    result += " END";
    return result;
}

string sql_builder::make_cast(string expression, string target_type) {
    return "CAST(" + move(expression) + " AS " + move(target_type) + ")";
}

sql_builder& sql_builder::create_table(string table) {
    sql_type_ = sql_operate::CREATE_TABLE;
    table_ = move(table);
    ensure_create_table_data();
    create_table_data_->if_not_exists = false;
    return *this;
}

sql_builder& sql_builder::create_table_if_not_exists(string table) {
    sql_type_ = sql_operate::CREATE_TABLE;
    table_ = move(table);
    ensure_create_table_data();
    create_table_data_->if_not_exists = true;
    return *this;
}

sql_builder& sql_builder::create_temp_table(string table) {
    sql_type_ = sql_operate::CREATE_TABLE;
    table_ = move(table);
    ensure_create_table_data();
    create_table_data_->temporary = true;
    create_table_data_->if_not_exists = false;
    return *this;
}

sql_builder& sql_builder::create_temp_table_if_not_exists(string table) {
    sql_type_ = sql_operate::CREATE_TABLE;
    table_ = move(table);
    ensure_create_table_data();
    create_table_data_->temporary = true;
    create_table_data_->if_not_exists = true;
    return *this;
}

sql_builder& sql_builder::column(string name, string type) {
    ensure_create_table_data()->columns.push_back({move(name), move(type)});
    return *this;
}

sql_builder& sql_builder::column_not_null(string name, string type) {
    column_definition col{move(name), move(type)};
    col.not_null = true;
    ensure_create_table_data()->columns.push_back(move(col));
    return *this;
}

sql_builder& sql_builder::column_primary_key(string name, string type) {
    column_definition col{move(name), move(type)};
    col.primary_key = true;
    ensure_create_table_data()->columns.push_back(move(col));
    return *this;
}

sql_builder& sql_builder::column_unique(string name, string type) {
    column_definition col{move(name), move(type)};
    col.unique_ = true;
    ensure_create_table_data()->columns.push_back(move(col));
    return *this;
}

sql_builder& sql_builder::column_default(string name, string type, string default_val) {
    column_definition col{move(name), move(type)};
    col.default_value = move(default_val);
    ensure_create_table_data()->columns.push_back(move(col));
    return *this;
}

sql_builder& sql_builder::column_check(string name, string type, string check_expr) {
    column_definition col{move(name), move(type)};
    col.check_expr = move(check_expr);
    ensure_create_table_data()->columns.push_back(move(col));
    return *this;
}

sql_builder& sql_builder::column_auto_increment(string name, string type) {
    column_definition col{move(name), move(type)};
    col.auto_increment = true;
    col.primary_key = true;
    ensure_create_table_data()->columns.push_back(move(col));
    return *this;
}

sql_builder& sql_builder::table_primary_key(vector<string> columns) {
    string constraint = "PRIMARY KEY (";
    for (size_t i = 0; i < columns.size(); ++i) {
        if (i > 0) {
            constraint += ", ";
        }
        constraint += move(columns[i]);
    }
    constraint += ")";
    ensure_create_table_data()->table_constraints.push_back(move(constraint));
    return *this;
}

sql_builder& sql_builder::table_foreign_key(string column, string ref_table, string ref_column) {
    string constraint =
            "FOREIGN KEY (" + move(column) + ") REFERENCES " + move(ref_table) + "(" + move(ref_column) + ")";
    ensure_create_table_data()->table_constraints.push_back(move(constraint));
    return *this;
}

sql_builder& sql_builder::table_unique(vector<string> columns) {
    string constraint = "UNIQUE (";
    for (size_t i = 0; i < columns.size(); ++i) {
        if (i > 0) {
            constraint += ", ";
        }
        constraint += move(columns[i]);
    }
    constraint += ")";
    ensure_create_table_data()->table_constraints.push_back(move(constraint));
    return *this;
}

sql_builder& sql_builder::table_check(string expr) {
    ensure_create_table_data()->table_constraints.push_back("CHECK (" + move(expr) + ")");
    return *this;
}

sql_builder& sql_builder::alter_table(string table) {
    sql_type_ = sql_operate::ALTER_TABLE;
    table_ = move(table);
    ensure_alter_table_data();
    return *this;
}

sql_builder& sql_builder::add_column(string name, string type) {
    ensure_alter_table_data()->actions.push_back("ADD COLUMN " + move(name) + " " + move(type));
    return *this;
}

sql_builder& sql_builder::drop_column(string name) {
    ensure_alter_table_data()->actions.push_back("DROP COLUMN " + move(name));
    return *this;
}

sql_builder& sql_builder::alter_column_set_type(string name, string new_type) {
    ensure_alter_table_data()->actions.push_back("ALTER COLUMN " + move(name) + " SET DATA TYPE " + move(new_type));
    return *this;
}

sql_builder& sql_builder::alter_column_set_default(string name, string default_val) {
    ensure_alter_table_data()->actions.push_back("ALTER COLUMN " + move(name) + " SET DEFAULT " + move(default_val));
    return *this;
}

sql_builder& sql_builder::alter_column_drop_default(string name) {
    ensure_alter_table_data()->actions.push_back("ALTER COLUMN " + move(name) + " DROP DEFAULT");
    return *this;
}

sql_builder& sql_builder::alter_column_set_not_null(string name) {
    ensure_alter_table_data()->actions.push_back("ALTER COLUMN " + move(name) + " SET NOT NULL");
    return *this;
}

sql_builder& sql_builder::alter_column_drop_not_null(string name) {
    ensure_alter_table_data()->actions.push_back("ALTER COLUMN " + move(name) + " DROP NOT NULL");
    return *this;
}

sql_builder& sql_builder::rename_column(string old_name, string new_name) {
    ensure_alter_table_data()->actions.push_back("RENAME COLUMN " + move(old_name) + " TO " + move(new_name));
    return *this;
}

sql_builder& sql_builder::add_primary_key(vector<string> columns) {
    string constraint = "ADD PRIMARY KEY (";
    for (size_t i = 0; i < columns.size(); ++i) {
        if (i > 0) {
            constraint += ", ";
        }
        constraint += move(columns[i]);
    }
    constraint += ")";
    ensure_alter_table_data()->actions.push_back(move(constraint));
    return *this;
}

sql_builder& sql_builder::drop_constraint(string constraint_name) {
    ensure_alter_table_data()->actions.push_back("DROP CONSTRAINT " + move(constraint_name));
    return *this;
}

sql_builder& sql_builder::create_view(string view_name, string select_query) {
    sql_type_ = sql_operate::CREATE_VIEW;
    table_ = view_name;
    auto* data = ensure_create_view_data();
    data->view_name = move(view_name);
    data->view_query = move(select_query);
    data->or_replace = false;
    return *this;
}

sql_builder& sql_builder::create_or_replace_view(string view_name, string select_query) {
    sql_type_ = sql_operate::CREATE_VIEW;
    table_ = view_name;
    auto* data = ensure_create_view_data();
    data->view_name = move(view_name);
    data->view_query = move(select_query);
    data->or_replace = true;
    return *this;
}

sql_builder& sql_builder::drop_view(string view_name) {
    sql_type_ = sql_operate::DROP_VIEW;
    table_ = move(view_name);
    return *this;
}

sql_builder& sql_builder::drop_view_if_exists(string view_name) {
    sql_type_ = sql_operate::DROP_VIEW;
    table_ = move(view_name);
    drop_if_exists_ = true;
    return *this;
}

sql_builder& sql_builder::create_index(string index_name, string table, vector<string> columns) {
    sql_type_ = sql_operate::CREATE_INDEX;
    auto* data = ensure_create_index_data();
    data->index_name = move(index_name);
    data->table_name = move(table);
    data->columns = move(columns);
    data->unique = false;
    table_ = data->table_name;
    return *this;
}

sql_builder& sql_builder::create_unique_index(string index_name, string table, vector<string> columns) {
    sql_type_ = sql_operate::CREATE_INDEX;
    auto* data = ensure_create_index_data();
    data->index_name = move(index_name);
    data->table_name = move(table);
    data->columns = move(columns);
    data->unique = true;
    table_ = data->table_name;
    return *this;
}

sql_builder& sql_builder::drop_index(string index_name) {
    sql_type_ = sql_operate::DROP_INDEX;
    table_ = move(index_name);
    drop_if_exists_ = false;
    return *this;
}

sql_builder& sql_builder::drop_index_if_exists(string index_name) {
    sql_type_ = sql_operate::DROP_INDEX;
    table_ = move(index_name);
    drop_if_exists_ = true;
    return *this;
}

sql_builder& sql_builder::truncate(string table) {
    sql_type_ = sql_operate::TRUNCATE;
    table_ = move(table);
    return *this;
}

sql_builder& sql_builder::union_(string query) {
    sql_type_ = sql_operate::SELECT;
    set_operations_.push_back({sql_set_op::UNION, move(query)});
    return *this;
}

sql_builder& sql_builder::union_all(string query) {
    sql_type_ = sql_operate::SELECT;
    set_operations_.push_back({sql_set_op::UNION_ALL, move(query)});
    return *this;
}

sql_builder& sql_builder::intersect(string query) {
    sql_type_ = sql_operate::SELECT;
    set_operations_.push_back({sql_set_op::INTERSECT, move(query)});
    return *this;
}

sql_builder& sql_builder::intersect_all(string query) {
    sql_type_ = sql_operate::SELECT;
    set_operations_.push_back({sql_set_op::INTERSECT_ALL, move(query)});
    return *this;
}

sql_builder& sql_builder::except_(string query) {
    sql_type_ = sql_operate::SELECT;
    set_operations_.push_back({sql_set_op::EXCEPT_, move(query)});
    return *this;
}

sql_builder& sql_builder::except_all(string query) {
    sql_type_ = sql_operate::SELECT;
    set_operations_.push_back({sql_set_op::EXCEPT_ALL, move(query)});
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
    set_operations_.clear();
    cte_entries_.clear();
    drop_if_exists_ = false;
    param_seq_ = 0;
    return *this;
}

string sql_builder::make_placeholder(const size_t index) const {
    switch (dialect_) {
        case sql_dialect::POSTGRESQL:
            return "$" + to_string(index);
        case sql_dialect::ORACLE:
            return ":" + to_string(index);
        default:
            return "?";
    }
}

string sql_builder::build() const {
    if (table_.empty()) {
        NEFORCE_THROW_EXCEPTION(value_exception("Table name is required"));
    }

    string result;
    switch (sql_type_) {
        case sql_operate::SELECT: {
            // CTE prepend
            if (!cte_entries_.empty()) {
                bool has_recursive = false;
                for (const auto& cte: cte_entries_) {
                    if (cte.recursive) {
                        has_recursive = true;
                        break;
                    }
                }
                result += "WITH ";
                if (has_recursive) {
                    result += "RECURSIVE ";
                }
                for (size_t i = 0; i < cte_entries_.size(); ++i) {
                    if (i > 0) {
                        result += ",\n";
                    }
                    result += cte_entries_[i].name + " AS (\n" + cte_entries_[i].query + "\n)";
                }
                result += "\n";
            }
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

            // GROUP BY ROLLUP
            if (select_data_ && !select_data_->rollup_fields.empty()) {
                result += " GROUP BY ROLLUP(";
                for (size_t i = 0; i < select_data_->rollup_fields.size(); ++i) {
                    if (i > 0) {
                        result += ", ";
                    }
                    result += select_data_->rollup_fields[i];
                }
                result += ")";
            }

            // GROUP BY CUBE
            if (select_data_ && !select_data_->cube_fields.empty()) {
                result += " GROUP BY CUBE(";
                for (size_t i = 0; i < select_data_->cube_fields.size(); ++i) {
                    if (i > 0) {
                        result += ", ";
                    }
                    result += select_data_->cube_fields[i];
                }
                result += ")";
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

            // Set operations (UNION / INTERSECT / EXCEPT)
            for (const auto& sop: set_operations_) {
                switch (sop.op) {
                    case sql_set_op::UNION:
                        result += "\nUNION\n";
                        break;
                    case sql_set_op::UNION_ALL:
                        result += "\nUNION ALL\n";
                        break;
                    case sql_set_op::INTERSECT:
                        result += "\nINTERSECT\n";
                        break;
                    case sql_set_op::INTERSECT_ALL:
                        result += "\nINTERSECT ALL\n";
                        break;
                    case sql_set_op::EXCEPT_:
                        result += "\nEXCEPT\n";
                        break;
                    case sql_set_op::EXCEPT_ALL:
                        result += "\nEXCEPT ALL\n";
                        break;
                }
                result += sop.query;
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

            // LIMIT & OFFSET / FETCH FIRST
            if (select_data_) {
                if (select_data_->fetch_count > 0 || dialect_ == sql_dialect::ORACLE) {
                    // ANSI SQL:2008 FETCH FIRST syntax (Oracle 12c+ compatible)
                    const int limit =
                            select_data_->fetch_count > 0 ? select_data_->fetch_count : select_data_->limit_count;
                    if (select_data_->offset_count > 0) {
                        result += " OFFSET " + to_string(select_data_->offset_count) + " ROWS";
                    }
                    if (limit > 0) {
                        result += " FETCH FIRST " + to_string(limit) + " ROWS ONLY";
                    }
                } else {
                    // De-facto LIMIT/OFFSET
                    if (select_data_->limit_count > 0) {
                        result += " LIMIT " + to_string(select_data_->limit_count);
                    }
                    if (select_data_->offset_count > 0) {
                        result += " OFFSET " + to_string(select_data_->offset_count);
                    }
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
            for (const auto& row: insert_data_->extra_rows) {
                result += ", (";
                for (size_t i = 0; i < row.size(); ++i) {
                    if (i > 0) {
                        result += ", ";
                    }
                    result += row[i];
                }
                result += ")";
            }
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
        case sql_operate::CREATE_TABLE: {
            if (!create_table_data_ || create_table_data_->columns.empty()) {
                NEFORCE_THROW_EXCEPTION(value_exception("No columns for CREATE TABLE"));
            }
            result += "CREATE ";
            if (create_table_data_->temporary) {
                result += "TEMP ";
            }
            result += "TABLE ";
            if (create_table_data_->if_not_exists) {
                result += "IF NOT EXISTS ";
            }
            result += table_ + " (\n";
            for (size_t i = 0; i < create_table_data_->columns.size(); ++i) {
                if (i > 0) {
                    result += ",\n";
                }
                const auto& col = create_table_data_->columns[i];
                result += "    " + col.name + " ";

                if (col.auto_increment) {
                    switch (dialect_) {
                        case sql_dialect::POSTGRESQL: {
                            // PostgreSQL: use SERIAL pseudo-type instead of INTEGER AUTO_INCREMENT
                            const auto& t = col.type;
                            if (t == "INTEGER" || t == "INT") {
                                result += "SERIAL";
                            } else if (t == "BIGINT") {
                                result += "BIGSERIAL";
                            } else if (t == "SMALLINT") {
                                result += "SMALLSERIAL";
                            } else {
                                result += col.type; // fallback
                            }
                            break;
                        }
                        case sql_dialect::MSSQL:
                            result += col.type + " IDENTITY(1,1)";
                            break;
                        case sql_dialect::ORACLE:
                            result += col.type + " GENERATED BY DEFAULT AS IDENTITY";
                            break;
                        default:
                            result += col.type + " AUTO_INCREMENT";
                            break;
                    }
                } else {
                    result += col.type;
                }
                if (col.not_null) {
                    result += " NOT NULL";
                }
                if (col.unique_) {
                    result += " UNIQUE";
                }
                if (col.primary_key) {
                    result += " PRIMARY KEY";
                }
                if (!col.default_value.empty()) {
                    result += " DEFAULT " + col.default_value;
                }
                if (!col.check_expr.empty()) {
                    result += " CHECK (" + col.check_expr + ")";
                }
            }
            for (const auto& constraint: create_table_data_->table_constraints) {
                result += ",\n    " + constraint;
            }
            result += "\n)";
            break;
        }
        case sql_operate::CREATE_VIEW: {
            if (!create_view_data_ || create_view_data_->view_query.empty()) {
                NEFORCE_THROW_EXCEPTION(value_exception("No query for CREATE VIEW"));
            }
            result += "CREATE ";
            if (create_view_data_->or_replace) {
                result += "OR REPLACE ";
            }
            result += "VIEW " + create_view_data_->view_name + " AS\n" + create_view_data_->view_query;
            break;
        }
        case sql_operate::DROP_VIEW: {
            result += "DROP VIEW ";
            if (drop_if_exists_) {
                result += "IF EXISTS ";
            }
            result += table_;
            break;
        }
        case sql_operate::ALTER_TABLE: {
            if (!alter_table_data_ || alter_table_data_->actions.empty()) {
                NEFORCE_THROW_EXCEPTION(value_exception("No actions for ALTER TABLE"));
            }
            result += "ALTER TABLE " + table_;
            for (size_t i = 0; i < alter_table_data_->actions.size(); ++i) {
                result += (i == 0 ? " " : ", ") + alter_table_data_->actions[i];
            }
            break;
        }
        case sql_operate::CREATE_INDEX: {
            if (!create_index_data_ || create_index_data_->columns.empty()) {
                NEFORCE_THROW_EXCEPTION(value_exception("No columns for CREATE INDEX"));
            }
            result += "CREATE ";
            if (create_index_data_->unique) {
                result += "UNIQUE ";
            }
            result += "INDEX " + create_index_data_->index_name + " ON " + create_index_data_->table_name + " (";
            for (size_t i = 0; i < create_index_data_->columns.size(); ++i) {
                if (i > 0) {
                    result += ", ";
                }
                result += create_index_data_->columns[i];
            }
            result += ")";
            break;
        }
        case sql_operate::DROP_INDEX: {
            result += "DROP INDEX ";
            if (drop_if_exists_) {
                result += "IF EXISTS ";
            }
            result += table_;
            break;
        }
        case sql_operate::TRUNCATE: {
            result += "TRUNCATE TABLE " + table_;
            break;
        }
        case sql_operate::DROP_TABLE: {
            result += "DROP TABLE ";
            if (drop_if_exists_) {
                result += "IF EXISTS ";
            }
            result += table_;
            break;
        }
        default: {
            NEFORCE_THROW_EXCEPTION(value_exception("Unsupported SQL type or not specified"));
        }
    }

    result += ";";

    return result;
}

NEFORCE_END_NAMESPACE__
