#ifndef MSTL_MYSQL_PREPARED_STATEMENT_HPP__
#define MSTL_MYSQL_PREPARED_STATEMENT_HPP__
#ifdef MSTL_SUPPORT_MYSQL__
#include "mysql_prepared_result.hpp"
MSTL_BEGIN_NAMESPACE__

class MSTL_API mysql_prepared_statement {
private:
    _MSTL_MYSQL MYSQL_STMT* stmt_ = nullptr;
    _MSTL_MYSQL MYSQL* conn_ = nullptr;
    uint32_t param_count_ = 0;

    vector<_MSTL_MYSQL MYSQL_BIND> bind_params_;
    vector<vector<char>> param_buffers_;

    MSTL_ALWAYS_INLINE void throw_if_stmt_null() const {
        if (!stmt_) {
            Exception(DatabasePreparedStmtError("Prepared statement not initialized"));
        }
    }

public:
    explicit mysql_prepared_statement(_MSTL_MYSQL MYSQL* conn, string_view sql);

    explicit mysql_prepared_statement(_MSTL_MYSQL MYSQL* conn, const string& sql)
    : mysql_prepared_statement(conn, sql.view()) {}

    explicit mysql_prepared_statement(_MSTL_MYSQL MYSQL* conn, const char* sql)
   : mysql_prepared_statement(conn, string_view{sql}) {}

    mysql_prepared_statement(mysql_prepared_statement&& other) noexcept;
    mysql_prepared_statement& operator =(mysql_prepared_statement&& other) noexcept;

    mysql_prepared_statement(const mysql_prepared_statement&) = delete;
    mysql_prepared_statement& operator =(const mysql_prepared_statement&) = delete;

    ~mysql_prepared_statement();

    MSTL_NODISCARD uint32_t param_count() const noexcept { return param_count_; }

    bool bind_param(uint32_t index, const string& value);
    bool bind_param(uint32_t index, int32_t value);
    bool bind_param(uint32_t index, int64_t value);
    bool bind_param(uint32_t index, float64_t value);
    bool bind_param(uint32_t index, const void* data, size_t length);

    bool execute();
    MSTL_NODISCARD unique_ptr<mysql_prepared_result> execute_query();

    MSTL_NODISCARD string_view get_error() const noexcept {
        if (!stmt_) return "Invalid statement!";
        return _MSTL_MYSQL mysql_stmt_error(stmt_);
    }

    MSTL_NODISCARD uint32_t get_errno() const noexcept {
        if (!stmt_) return 0;
        return _MSTL_MYSQL mysql_stmt_errno(stmt_);
    }

    void swap(mysql_prepared_statement& other) noexcept;
};

MSTL_END_NAMESPACE__
#endif // MSTL_SUPPORT_MYSQL__
#endif // MSTL_MYSQL_PREPARED_STATEMENT_HPP__
