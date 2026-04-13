#include <NeForce/db/mysql/mysql_prepared_statement.hpp>
#ifdef NEFORCE_SUPPORT_MYSQL
#    include <NeForce/db/mysql/mysql_prepared_result.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    void throw_if_stmt_null(const ::MYSQL_STMT* ptr_) {
        if (ptr_ != nullptr) {
            return;
        }
        NEFORCE_THROW_EXCEPTION(database_stmt_exception("Prepared statement not initialized"));
    }
} // namespace


mysql_prepared_statement::mysql_prepared_statement(::MYSQL* conn, const string_view sql) :
conn_(conn) {
    if (conn_ == nullptr) {
        NEFORCE_THROW_EXCEPTION(database_stmt_exception("Invalid MySQL connection pointer"));
    }
    stmt_ = ::mysql_stmt_init(conn_);
    if (stmt_ == nullptr) {
        NEFORCE_THROW_EXCEPTION(database_stmt_exception("mysql_stmt_init failed"));
    }

    if (::mysql_stmt_prepare(stmt_, sql.data(), sql.size()) != 0) {
        const string_view err = ::mysql_stmt_error(stmt_);
        ::mysql_stmt_close(stmt_);
        stmt_ = nullptr;
        NEFORCE_THROW_EXCEPTION(database_stmt_exception(err.data()));
    }

    param_count_ = ::mysql_stmt_param_count(stmt_);
    bind_params_.resize(param_count_);
    param_buffers_.resize(param_count_);
    for (unsigned int i = 0; i < param_count_; ++i) {
        memory_zero(&bind_params_[i]);
    }
}

mysql_prepared_statement::~mysql_prepared_statement() {
    if (stmt_ != nullptr) {
        ::mysql_stmt_close(stmt_);
        stmt_ = nullptr;
    }
}

mysql_prepared_statement::mysql_prepared_statement(mysql_prepared_statement&& other) noexcept :
stmt_(other.stmt_),
conn_(other.conn_),
param_count_(other.param_count_),
bind_params_(move(other.bind_params_)),
param_buffers_(move(other.param_buffers_)) {
    other.stmt_ = nullptr;
    other.conn_ = nullptr;
    other.param_count_ = 0;
}

mysql_prepared_statement& mysql_prepared_statement::operator=(mysql_prepared_statement&& other) noexcept {
    if (_NEFORCE addressof(other) == this) {
        return *this;
    }

    if (stmt_ != nullptr) {
        ::mysql_stmt_close(stmt_);
    }
    stmt_ = other.stmt_;
    conn_ = other.conn_;
    param_count_ = other.param_count_;
    bind_params_ = move(other.bind_params_);
    param_buffers_ = move(other.param_buffers_);
    other.stmt_ = nullptr;
    other.conn_ = nullptr;
    other.param_count_ = 0;
    return *this;
}

bool mysql_prepared_statement::bind_param(const uint32_t index, const string_view value) {
    try {
        throw_if_stmt_null(stmt_);
        if (index >= param_count_) {
            return false;
        }

        vector<char>& buffer = param_buffers_[index];
        buffer.assign(value.begin(), value.end());
        buffer.push_back('\0');

        ::MYSQL_BIND& bind = bind_params_[index];
        memory_zero(&bind);
        bind.buffer_type = ::MYSQL_TYPE_STRING;
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
        throw_if_stmt_null(stmt_);
        if (index >= param_count_) {
            return false;
        }

        vector<char>& buffer = param_buffers_[index];
        buffer.resize(sizeof(int32_t));
        memory_copy(buffer.data(), &value, sizeof(int32_t));

        ::MYSQL_BIND& bind = bind_params_[index];
        memory_zero(&bind);
        bind.buffer_type = ::MYSQL_TYPE_LONG;
        bind.buffer = buffer.data();
        bind.is_unsigned = false;
        return true;
    } catch (...) {
        return false;
    }
}

bool mysql_prepared_statement::bind_param(const uint32_t index, const int64_t value) {
    try {
        throw_if_stmt_null(stmt_);
        if (index >= param_count_) {
            return false;
        }

        vector<char>& buffer = param_buffers_[index];
        buffer.resize(sizeof(int64_t));
        memory_copy(buffer.data(), &value, sizeof(int64_t));

        ::MYSQL_BIND& bind = bind_params_[index];
        memory_zero(&bind);
        bind.buffer_type = ::MYSQL_TYPE_LONGLONG;
        bind.buffer = buffer.data();
        bind.is_unsigned = false;
        return true;
    } catch (...) {
        return false;
    }
}

bool mysql_prepared_statement::bind_param(const uint32_t index, const float64_t value) {
    try {
        throw_if_stmt_null(stmt_);
        if (index >= param_count_) {
            return false;
        }

        vector<char>& buffer = param_buffers_[index];
        buffer.resize(sizeof(float64_t));
        memory_copy(buffer.data(), &value, sizeof(float64_t));

        ::MYSQL_BIND& bind = bind_params_[index];
        memory_zero(&bind);
        bind.buffer_type = ::MYSQL_TYPE_DOUBLE;
        bind.buffer = buffer.data();
        return true;
    } catch (...) {
        return false;
    }
}

bool mysql_prepared_statement::bind_param(const uint32_t index, const cbyte_view value) {
    try {
        throw_if_stmt_null(stmt_);
        if (index >= param_count_) {
            return false;
        }

        vector<char>& buffer = param_buffers_[index];
        buffer.resize(value.size());
        memory_copy(buffer.data(), value.data(), value.size());

        ::MYSQL_BIND& bind = bind_params_[index];
        memory_zero(&bind);
        bind.buffer_type = ::MYSQL_TYPE_BLOB;
        bind.buffer = buffer.data();
        bind.buffer_length = value.size();
        return true;
    } catch (...) {
        return false;
    }
}

bool mysql_prepared_statement::execute() {
    throw_if_stmt_null(stmt_);
    if (param_count_ > 0) {
        if (::mysql_stmt_bind_param(stmt_, bind_params_.data())) {
            return false;
        }
    }
    return ::mysql_stmt_execute(stmt_) == 0;
}

unique_ptr<idb_prepared_result> mysql_prepared_statement::execute_query() {
    throw_if_stmt_null(stmt_);
    if (param_count_ > 0) {
        if (::mysql_stmt_bind_param(stmt_, bind_params_.data())) {
            return nullptr;
        }
    }
    if (::mysql_stmt_execute(stmt_) != 0) {
        return nullptr;
    }
    return make_unique<mysql_prepared_result>(stmt_);
}

string_view mysql_prepared_statement::get_error() const noexcept {
    if (stmt_ == nullptr) {
        return "Invalid statement!";
    }
    return ::mysql_stmt_error(stmt_);
}

uint32_t mysql_prepared_statement::get_errno() const noexcept {
    if (stmt_ == nullptr) {
        return 0;
    }
    return ::mysql_stmt_errno(stmt_);
}

NEFORCE_END_NAMESPACE__
#endif
