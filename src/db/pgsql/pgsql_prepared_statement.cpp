#include <NeForce/db/pgsql/pgsql_prepared_statement.hpp>
#ifdef NEFORCE_SUPPORT_POSTGRESQL
#    include <NeForce/core/async/atomic.hpp>
#    include <NeForce/core/utility/packages.hpp>
#    include <NeForce/db/pgsql/pgsql_result.hpp>
NEFORCE_BEGIN_NAMESPACE__

pgsql_prepared_statement::pgsql_prepared_statement(::PGconn* conn, string sql) :
conn_(conn),
sql_(move(sql)) {
    static atomic<uint64_t> stmt_counter{0};
    stmt_name_ = "pstmt_" + to_string(stmt_counter++);

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
            max_param = max(param_num, max_param);
        }
    }
    param_count_ = max_param;

    data_->param_values.resize(param_count_);
    data_->param_ptrs.resize(param_count_, nullptr);
    data_->param_lengths.resize(param_count_, 0);
    data_->param_formats.resize(param_count_, 0);
    ;

    last_error_.clear();
    last_errno_ = 0;

    ::PGresult* result = ::PQprepare(conn_, stmt_name_.data(), sql_.data(), static_cast<int>(param_count_), nullptr);
    if (result == nullptr) {
        set_error("Failed to prepare statement", 1);
        return;
    }

    const ::ExecStatusType status = ::PQresultStatus(result);
    ::PQclear(result);
    if (status != ::PGRES_COMMAND_OK) {
        set_error(::PQerrorMessage(conn_), 2);
    }
}

pgsql_prepared_statement::~pgsql_prepared_statement() {
    if (conn_ == nullptr) {
        return;
    }

    try {
        const string deallocate_sql = "DEALLOCATE " + move(stmt_name_);
        ::PGresult* result = ::PQexec(conn_, deallocate_sql.data());
        if (result != nullptr) {
            ::PQclear(result);
        }
        data_.reset();
        // NOLINTNEXTLINE(bugprone-empty-catch)
    } catch (...) {
        // ignore
    }
}

void pgsql_prepared_statement::set_error(string error, const uint32_t errno_val) noexcept {
    last_error_ = move(error);
    last_errno_ = errno_val;
}

bool pgsql_prepared_statement::bind_param(const uint32_t index, const string_view value) {
    if (index == 0 || index > param_count_) {
        set_error("Parameter index out of range", 3);
        return false;
    }

    const size_t idx = index - 1;
    data_->param_values[idx] = value;
    data_->param_ptrs[idx] = data_->param_values[idx].data();
    data_->param_lengths[idx] = static_cast<int>(data_->param_values[idx].length());
    data_->param_formats[idx] = 0;

    return true;
}

bool pgsql_prepared_statement::bind_param(const uint32_t index, const int32_t value) {
    return bind_param(index, to_string(value));
}

bool pgsql_prepared_statement::bind_param(const uint32_t index, const int64_t value) {
    return bind_param(index, to_string(value));
}

bool pgsql_prepared_statement::bind_param(const uint32_t index, const float64_t value) {
    return bind_param(index, to_string(value));
}

bool pgsql_prepared_statement::bind_param(const uint32_t index, const cbyte_view value) {
    if (index == 0 || index > param_count_) {
        set_error("Parameter index out of range", 3);
        return false;
    }

    if (value.empty()) {
        return bind_param(index, string_view{});
    }

    const size_t idx = index - 1;

    if (param_buffers_.size() <= idx) {
        param_buffers_.resize(idx + 1);
    }

    const auto* begin = reinterpret_cast<const char*>(value.data());
    param_buffers_[idx].assign(begin, begin + value.size());

    data_->param_ptrs[idx] = param_buffers_[idx].data();
    data_->param_lengths[idx] = static_cast<int>(value.size());
    data_->param_formats[idx] = 1;

    return true;
}

bool pgsql_prepared_statement::execute() {
    last_error_.clear();
    last_errno_ = 0;

    ::PGresult* result =
            ::PQexecPrepared(conn_, stmt_name_.data(), static_cast<int>(param_count_), data_->param_ptrs.data(),
                             data_->param_lengths.data(), data_->param_formats.data(), 0);

    if (result == nullptr) {
        set_error("Failed to execute prepared statement", 4);
        return false;
    }

    const ::ExecStatusType status = ::PQresultStatus(result);
    ::PQclear(result);

    if (status != ::PGRES_COMMAND_OK && status != ::PGRES_TUPLES_OK) {
        set_error(::PQerrorMessage(conn_), 5);
        return false;
    }

    return true;
}

unique_ptr<idb_tb_result> pgsql_prepared_statement::execute_query() {
    last_error_.clear();
    last_errno_ = 0;

    ::PGresult* result = ::PQexecPrepared(conn_, stmt_name_.data(), static_cast<int>(param_count_),
                                          data_->param_ptrs.empty() ? nullptr : data_->param_ptrs.data(),
                                          data_->param_lengths.empty() ? nullptr : data_->param_lengths.data(),
                                          data_->param_formats.empty() ? nullptr : data_->param_formats.data(), 0);

    if (result == nullptr) {
        set_error("Failed to execute prepared statement query", 6);
        return nullptr;
    }

    const ::ExecStatusType status = ::PQresultStatus(result);
    if (status != ::PGRES_TUPLES_OK) {
        set_error(::PQerrorMessage(conn_), 7);
        ::PQclear(result);
        return nullptr;
    }

    return make_unique<pgsql_tb_result>(result, true);
}

NEFORCE_END_NAMESPACE__
#endif
