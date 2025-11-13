#ifndef MSTL_MYSQL_PREPARED_STATEMENT_HPP__
#define MSTL_MYSQL_PREPARED_STATEMENT_HPP__
#ifdef MSTL_SUPPORT_MYSQL__
#include "MSTL/db/db_interface.hpp"
#include "mysql_config.hpp"
MSTL_BEGIN_NAMESPACE__

class MSTL_API mysql_prepared_statement final : public idb_prepared_statement {
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

    ~mysql_prepared_statement() override;

    MSTL_NODISCARD uint32_t param_count() const noexcept override { return param_count_; }

    bool bind_param(uint32_t index, const string& value) override { return bind_param(index, value.view()); }
    bool bind_param(uint32_t index, string_view value) override;
    bool bind_param(uint32_t index, const char* value) override { return bind_param(index, string_view{value}); }
    bool bind_param(uint32_t index, int32_t value) override;
    bool bind_param(uint32_t index, int64_t value) override;
    bool bind_param(uint32_t index, float64_t value) override;
    bool bind_param(uint32_t index, const void* data, size_t length) override;

    bool execute() override;
    MSTL_NODISCARD unique_ptr<idb_prepared_result> execute_query() override;

    MSTL_NODISCARD string_view get_error() const noexcept override;
    MSTL_NODISCARD uint32_t get_errno() const noexcept override;

    void swap(mysql_prepared_statement& other) noexcept;
};

MSTL_END_NAMESPACE__
#endif // MSTL_SUPPORT_MYSQL__
#endif // MSTL_MYSQL_PREPARED_STATEMENT_HPP__
