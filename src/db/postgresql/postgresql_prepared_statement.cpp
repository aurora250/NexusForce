#include <MSTL/db/postgresql/postgresql_prepared_statement.hpp>
#ifdef MSTL_SUPPORT_POSTGRESQL__
#include <MSTL/core/utility/packages.hpp>
#include <MSTL/db/postgresql/postgresql_prepared_result.hpp>
#include <atomic>
MSTL_BEGIN_NAMESPACE__

postgresql_prepared_statement::postgresql_prepared_statement(_MSTL_POSTGRESQL PGconn* conn, const string& sql)
: conn_(conn), sql_(sql) {
    static std::atomic<uint64_t> stmt_counter{0};
    stmt_name_ = "pstmt_" + _MSTL to_string(stmt_counter++);

    size_t pos = 0;
    uint32_t max_param = 0;
    while ((pos = sql_.find('$', pos)) != string::npos) {
        ++pos;
        if (pos < sql_.length() && is_digit(sql_[pos])) {
            uint32_t param_num = 0;
            while (pos < sql_.length() && is_digit(sql_[pos])) {
                param_num = param_num * 10 + (sql_[pos] - '0');
                ++pos;
            }
            if (param_num > max_param) {
                max_param = param_num;
            }
        }
    }
    param_count_ = max_param;

    init_params();
    prepare();
}

postgresql_prepared_statement::~postgresql_prepared_statement() {
    if (conn_) {
        const string deallocate_sql = "DEALLOCATE " + _MSTL move(stmt_name_);
        _MSTL_POSTGRESQL PGresult* result = _MSTL_POSTGRESQL PQexec(conn_, deallocate_sql.c_str());
        if (result) {
            _MSTL_POSTGRESQL PQclear(result);
        }
    }
}

void postgresql_prepared_statement::init_params() const {
    data_->param_values.resize(param_count_);
    data_->param_ptrs.resize(param_count_, nullptr);
    data_->param_lengths.resize(param_count_, 0);
    data_->param_formats.resize(param_count_, 0);
}

bool postgresql_prepared_statement::prepare() {
    clear_error();

    _MSTL_POSTGRESQL PGresult* result = _MSTL_POSTGRESQL PQprepare(
        conn_, stmt_name_.c_str(), sql_.c_str(), param_count_, nullptr);

    if (!result) {
        set_error("Failed to prepare statement", 1);
        return false;
    }

    const _MSTL_POSTGRESQL ExecStatusType status = _MSTL_POSTGRESQL PQresultStatus(result);
    _MSTL_POSTGRESQL PQclear(result);

    if (status != _MSTL_POSTGRESQL PGRES_COMMAND_OK) {
        set_error(_MSTL_POSTGRESQL PQerrorMessage(conn_), 2);
        return false;
    }

    return true;
}

void postgresql_prepared_statement::clear_error() noexcept {
    last_error_.clear();
    last_errno_ = 0;
}

void postgresql_prepared_statement::set_error(string error, const uint32_t errno_val) noexcept {
    last_error_ = _MSTL move(error);
    last_errno_ = errno_val;
}

bool postgresql_prepared_statement::bind_param(const uint32_t index, const string_view value) {
    if (index == 0 || index > param_count_) {
        set_error("Parameter index out of range", 3);
        return false;
    }

    const size_t idx = index - 1;
    data_->param_values[idx] = value;
    data_->param_ptrs[idx] = data_->param_values[idx].c_str();
    data_->param_lengths[idx] = static_cast<int>(data_->param_values[idx].length());
    data_->param_formats[idx] = 0;

    return true;
}

bool postgresql_prepared_statement::bind_param(const uint32_t index, const int32_t value) {
    return bind_param(index, _MSTL to_string(value));
}

bool postgresql_prepared_statement::bind_param(const uint32_t index, const int64_t value) {
    return bind_param(index, _MSTL to_string(value));
}

bool postgresql_prepared_statement::bind_param(const uint32_t index, const float64_t value) {
    return bind_param(index, _MSTL to_string(value));
}

bool postgresql_prepared_statement::bind_param(const uint32_t index, const void* data, const size_t length) {
    if (index == 0 || index > param_count_) {
        set_error("Parameter index out of range", 3);
        return false;
    }

    if (!data || length == 0) {
        return bind_param(index, static_cast<const char*>(nullptr));
    }

    const size_t idx = index - 1;

    if (param_buffers_.size() <= idx) {
        param_buffers_.resize(idx + 1);
    }

    param_buffers_[idx].assign(
        static_cast<const char*>(data),
        static_cast<const char*>(data) + length);

    data_->param_ptrs[idx] = param_buffers_[idx].data();
    data_->param_lengths[idx] = static_cast<int>(length);
    data_->param_formats[idx] = 1;

    return true;
}

bool postgresql_prepared_statement::execute() {
    clear_error();

    _MSTL_POSTGRESQL PGresult* result = _MSTL_POSTGRESQL PQexecPrepared(
        conn_, stmt_name_.c_str(), param_count_,
        data_->param_ptrs.data(), data_->param_lengths.data(),
        data_->param_formats.data(), 0);

    if (!result) {
        set_error("Failed to execute prepared statement", 4);
        return false;
    }

    const _MSTL_POSTGRESQL ExecStatusType status = _MSTL_POSTGRESQL PQresultStatus(result);
    _MSTL_POSTGRESQL PQclear(result);

    if (status != _MSTL_POSTGRESQL PGRES_COMMAND_OK && status != _MSTL_POSTGRESQL PGRES_TUPLES_OK) {
        set_error(_MSTL_POSTGRESQL PQerrorMessage(conn_), 5);
        return false;
    }

    return true;
}

unique_ptr<idb_prepared_result> postgresql_prepared_statement::execute_query() {
    clear_error();

    _MSTL_POSTGRESQL PGresult* result = _MSTL_POSTGRESQL PQexecPrepared(
        conn_, stmt_name_.c_str(), param_count_,
        data_->param_ptrs.data(), data_->param_lengths.data(),
        data_->param_formats.data(), 0);

    if (!result) {
        set_error("Failed to execute prepared statement query", 6);
        return nullptr;
    }

    const _MSTL_POSTGRESQL ExecStatusType status = _MSTL_POSTGRESQL PQresultStatus(result);
    if (status != _MSTL_POSTGRESQL PGRES_TUPLES_OK) {
        set_error(_MSTL_POSTGRESQL PQerrorMessage(conn_), 7);
        _MSTL_POSTGRESQL PQclear(result);
        return nullptr;
    }

    return make_unique<postgresql_prepared_result>(result);
}

MSTL_END_NAMESPACE__
#endif
