#include <MSTL/database/mysql/mysql_prepared_statement.hpp>
#ifdef MSTL_SUPPORT_MYSQL__
#include <MSTL/database/mysql/mysql_prepared_result.hpp>
MSTL_BEGIN_NAMESPACE__

mysql_prepared_statement::mysql_prepared_statement(_MSTL_MYSQL MYSQL* conn, const string_view sql)
: conn_(conn) {
    if (!conn_) Exception(DatabasePreparedStmtError("Invalid MySQL connection pointer"));
    stmt_ = _MSTL_MYSQL mysql_stmt_init(conn_);
    if (!stmt_) Exception(DatabasePreparedStmtError("mysql_stmt_init failed"));

    if (_MSTL_MYSQL mysql_stmt_prepare(stmt_, sql.data(), sql.size())) {
        const string_view err = _MSTL_MYSQL mysql_stmt_error(stmt_);
        _MSTL_MYSQL mysql_stmt_close(stmt_);
        stmt_ = nullptr;
        Exception(DatabasePreparedStmtError(err.data()));
    }

    param_count_ = _MSTL_MYSQL mysql_stmt_param_count(stmt_);
    bind_params_.resize(param_count_);
    param_buffers_.resize(param_count_);
    for (unsigned int i = 0; i < param_count_; ++i) {
        memory_zero(&bind_params_[i], sizeof(_MSTL_MYSQL MYSQL_BIND));
    }
}

mysql_prepared_statement::~mysql_prepared_statement() {
    if (stmt_) {
        _MSTL_MYSQL mysql_stmt_close(stmt_);
        stmt_ = nullptr;
    }
}

mysql_prepared_statement::mysql_prepared_statement(mysql_prepared_statement&& other) noexcept
        : stmt_(other.stmt_), conn_(other.conn_), param_count_(other.param_count_),
          bind_params_(_MSTL move(other.bind_params_)),
          param_buffers_(_MSTL move(other.param_buffers_)) {
    other.stmt_ = nullptr;
    other.conn_ = nullptr;
    other.param_count_ = 0;
}

mysql_prepared_statement& mysql_prepared_statement::operator =(mysql_prepared_statement&& other) noexcept {
    if (_MSTL addressof(other) == this) return *this;
    if (stmt_) _MSTL_MYSQL mysql_stmt_close(stmt_);
    swap(other);
    return *this;
}

bool mysql_prepared_statement::bind_param(const uint32_t index, const string_view value) {
    try {
        throw_if_stmt_null();
        if (index >= param_count_) return false;

        vector<char>& buffer = param_buffers_[index];
        buffer.assign(value.begin(), value.end());
        buffer.push_back('\0');

        _MSTL_MYSQL MYSQL_BIND& bind = bind_params_[index];
        memory_zero(&bind, sizeof(_MSTL_MYSQL MYSQL_BIND));
        bind.buffer_type = _MSTL_MYSQL MYSQL_TYPE_STRING;
        bind.buffer = buffer.data();
        bind.buffer_length = buffer.size();
        bind.length = nullptr;
        bind.is_null = nullptr;
        return true;
    } catch (...) {
        return false;
    }
}

bool mysql_prepared_statement::bind_param(const uint32_t index, const int32_t value) {
    try {
        throw_if_stmt_null();
        if (index >= param_count_) return false;

        vector<char>& buffer = param_buffers_[index];
        buffer.resize(sizeof(int32_t));
        memory_copy(buffer.data(), &value, sizeof(int32_t));

        _MSTL_MYSQL MYSQL_BIND& bind = bind_params_[index];
        memory_zero(&bind, sizeof(_MSTL_MYSQL MYSQL_BIND));
        bind.buffer_type = _MSTL_MYSQL MYSQL_TYPE_LONG;
        bind.buffer = buffer.data();
        bind.is_unsigned = false;
        return true;
    } catch (...) {
        return false;
    }
}

bool mysql_prepared_statement::bind_param(const uint32_t index, const int64_t value) {
    try {
        throw_if_stmt_null();
        if (index >= param_count_) return false;

        vector<char>& buffer = param_buffers_[index];
        buffer.resize(sizeof(int64_t));
        memory_copy(buffer.data(), &value, sizeof(int64_t));

        _MSTL_MYSQL MYSQL_BIND& bind = bind_params_[index];
        memory_zero(&bind, sizeof(_MSTL_MYSQL MYSQL_BIND));
        bind.buffer_type = _MSTL_MYSQL MYSQL_TYPE_LONGLONG;
        bind.buffer = buffer.data();
        bind.is_unsigned = false;
        return true;
    } catch (...) {
        return false;
    }
}

bool mysql_prepared_statement::bind_param(const uint32_t index, const float64_t value) {
    try {
        throw_if_stmt_null();
        if (index >= param_count_) return false;

        vector<char>& buffer = param_buffers_[index];
        buffer.resize(sizeof(float64_t));
        memory_copy(buffer.data(), &value, sizeof(float64_t));

        _MSTL_MYSQL MYSQL_BIND& bind = bind_params_[index];
        memory_zero(&bind, sizeof(_MSTL_MYSQL MYSQL_BIND));
        bind.buffer_type = _MSTL_MYSQL MYSQL_TYPE_DOUBLE;
        bind.buffer = buffer.data();
        return true;
    } catch (...) {
        return false;
    }
}

bool mysql_prepared_statement::bind_param(const uint32_t index, const void* data, const size_t length) {
    try {
        throw_if_stmt_null();
        if (index >= param_count_) return false;

        vector<char>& buffer = param_buffers_[index];
        buffer.resize(length);
        memory_copy(buffer.data(), data, length);

        _MSTL_MYSQL MYSQL_BIND& bind = bind_params_[index];
        memory_zero(&bind, sizeof(_MSTL_MYSQL MYSQL_BIND));
        bind.buffer_type = _MSTL_MYSQL MYSQL_TYPE_BLOB;
        bind.buffer = buffer.data();
        bind.buffer_length = length;
        return true;
    } catch (...) {
        return false;
    }
}

bool mysql_prepared_statement::execute() {
    throw_if_stmt_null();
    if (param_count_ > 0) {
        if (_MSTL_MYSQL mysql_stmt_bind_param(stmt_, bind_params_.data())) {
            return false;
        }
    }
    return mysql_stmt_execute(stmt_) == 0;
}

unique_ptr<idb_prepared_result> mysql_prepared_statement::execute_query() {
    throw_if_stmt_null();
    if (param_count_ > 0) {
        if (_MSTL_MYSQL mysql_stmt_bind_param(stmt_, bind_params_.data()) != 0) {
            return nullptr;
        }
    }
    if (_MSTL_MYSQL mysql_stmt_execute(stmt_) != 0) {
        return nullptr;
    }
    return make_unique<mysql_prepared_result>(stmt_);
}

string_view mysql_prepared_statement::get_error() const noexcept {
    if (!stmt_) return "Invalid statement!";
    return _MSTL_MYSQL mysql_stmt_error(stmt_);
}

uint32_t mysql_prepared_statement::get_errno() const noexcept {
    if (!stmt_) return 0;
    return _MSTL_MYSQL mysql_stmt_errno(stmt_);
}

void mysql_prepared_statement::swap(mysql_prepared_statement& other) noexcept {
    stmt_ = other.stmt_;
    conn_ = other.conn_;
    param_count_ = other.param_count_;
    bind_params_ = _MSTL move(other.bind_params_);
    param_buffers_ = _MSTL move(other.param_buffers_);
    other.stmt_ = nullptr;
    other.conn_ = nullptr;
    other.param_count_ = 0;
}

MSTL_END_NAMESPACE__
#endif
