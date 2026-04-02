#include <NeForce/db/sqlite/sqlite_prepared_statement.hpp>
#ifdef NEFORCE_SUPPORT_SQLITE3
#    include <NeForce/db/sqlite/sqlite_prepared_result.hpp>
NEFORCE_BEGIN_NAMESPACE__

void sqlite_prepared_statement::clear_bindings() {
    if (stmt_) {
        ::sqlite3_clear_bindings(stmt_);
    }
    param_buffers_.clear();
    param_buffers_.resize(param_count_);
}

sqlite_prepared_statement::sqlite_prepared_statement(::sqlite3* db, const string& sql) :
db_(db) {
    if (!db_) {
        last_error_ = "Database connection is null";
        return;
    }

    const int rc = ::sqlite3_prepare_v2(db_, sql.data(), static_cast<int>(sql.size()), &stmt_, nullptr);
    if (rc != SQLITE_OK) {
        last_error_ = ::sqlite3_errmsg(db_);
        stmt_ = nullptr;
        prepared_ = false;
        return;
    }

    param_count_ = static_cast<uint32_t>(::sqlite3_bind_parameter_count(stmt_));
    param_buffers_.resize(param_count_);
    prepared_ = true;
}

sqlite_prepared_statement::sqlite_prepared_statement(sqlite_prepared_statement&& other) noexcept :
db_(other.db_),
stmt_(other.stmt_),
param_count_(other.param_count_),
param_buffers_(move(other.param_buffers_)),
prepared_(other.prepared_),
last_error_(_NEFORCE move(other.last_error_)) {
    other.stmt_ = nullptr;
    other.db_ = nullptr;
    other.param_count_ = 0;
    other.prepared_ = false;
    other.param_buffers_.clear();
    other.last_error_.clear();
}

sqlite_prepared_statement& sqlite_prepared_statement::operator=(sqlite_prepared_statement&& other) noexcept {
    if (this != &other) {
        if (stmt_) {
            ::sqlite3_finalize(stmt_);
        }
        db_ = other.db_;
        stmt_ = other.stmt_;
        param_count_ = other.param_count_;
        param_buffers_ = _NEFORCE move(other.param_buffers_);
        last_error_ = _NEFORCE move(other.last_error_);
        prepared_ = other.prepared_;

        other.stmt_ = nullptr;
        other.db_ = nullptr;
        other.param_count_ = 0;
        other.prepared_ = false;
        other.param_buffers_.clear();
        other.last_error_.clear();
    }
    return *this;
}

sqlite_prepared_statement::~sqlite_prepared_statement() {
    if (stmt_) {
        ::sqlite3_finalize(stmt_);
        stmt_ = nullptr;
    }
}

bool sqlite_prepared_statement::bind_param(const uint32_t index, const string_view value) {
    if (!prepared_ || !stmt_ || index == 0 || index > param_count_) {
        last_error_ = "Invalid parameter index or statement not prepared";
        return false;
    }
    const size_t len = value.size();
    if (param_buffers_.size() < index) {
        param_buffers_.resize(index);
    }
    param_buffers_[index - 1].assign(value.data(), value.data() + len);
    const int rc = ::sqlite3_bind_text(stmt_, index, param_buffers_[index - 1].data(), static_cast<int>(len),
                                       SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) {
        last_error_ = ::sqlite3_errmsg(db_);
        return false;
    }
    return true;
}

bool sqlite_prepared_statement::bind_param(const uint32_t index, const int32_t value) {
    if (!prepared_ || !stmt_ || index == 0 || index > param_count_) {
        last_error_ = "Invalid parameter index or statement not prepared";
        return false;
    }
    const int rc = ::sqlite3_bind_int(stmt_, index, value);
    if (rc != SQLITE_OK) {
        last_error_ = ::sqlite3_errmsg(db_);
        return false;
    }
    return true;
}

bool sqlite_prepared_statement::bind_param(const uint32_t index, const int64_t value) {
    if (!prepared_ || !stmt_ || index == 0 || index > param_count_) {
        last_error_ = "Invalid parameter index or statement not prepared";
        return false;
    }
    const int rc = ::sqlite3_bind_int64(stmt_, index, value);
    if (rc != SQLITE_OK) {
        last_error_ = ::sqlite3_errmsg(db_);
        return false;
    }
    return true;
}

bool sqlite_prepared_statement::bind_param(const uint32_t index, const float64_t value) {
    if (!prepared_ || !stmt_ || index == 0 || index > param_count_) {
        last_error_ = "Invalid parameter index or statement not prepared";
        return false;
    }
    const int rc = ::sqlite3_bind_double(stmt_, index, value);
    if (rc != SQLITE_OK) {
        last_error_ = ::sqlite3_errmsg(db_);
        return false;
    }
    return true;
}

bool sqlite_prepared_statement::bind_param(const uint32_t index, const void* data, const size_t length) {
    if (!prepared_ || !stmt_ || index == 0 || index > param_count_ || !data) {
        last_error_ = "Invalid parameter or null pointer for blob";
        return false;
    }
    if (param_buffers_.size() < index) {
        param_buffers_.resize(index);
    }
    param_buffers_[index - 1].resize(length);
    memory_copy(param_buffers_[index - 1].data(), data, length);
    const int rc = ::sqlite3_bind_blob(stmt_, index, param_buffers_[index - 1].data(), static_cast<int>(length),
                                       SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) {
        last_error_ = ::sqlite3_errmsg(db_);
        return false;
    }
    return true;
}

bool sqlite_prepared_statement::execute() {
    if (!prepared_ || !stmt_) {
        last_error_ = "Statement not prepared";
        return false;
    }
    const int rc = ::sqlite3_step(stmt_);
    if (rc != SQLITE_DONE) {
        last_error_ = ::sqlite3_errmsg(db_);
        ::sqlite3_reset(stmt_);
        return false;
    }
    ::sqlite3_reset(stmt_);
    clear_bindings();
    return true;
}

unique_ptr<idb_prepared_result> sqlite_prepared_statement::execute_query() {
    if (!prepared_ || !stmt_) {
        last_error_ = "Statement not prepared";
        return nullptr;
    }
    ::sqlite3_reset(stmt_);
    return make_unique<sqlite_prepared_result>(stmt_);
}

NEFORCE_END_NAMESPACE__
#endif
