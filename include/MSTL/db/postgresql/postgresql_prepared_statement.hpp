#ifndef MSTL_POSTGRESQL_PREPARED_STATEMENT_HPP__
#define MSTL_POSTGRESQL_PREPARED_STATEMENT_HPP__
#ifdef MSTL_SUPPORT_POSTGRESQL__
#include "MSTL/db/db_interface.hpp"
#include "postgresql_config.hpp"
MSTL_BEGIN_NAMESPACE__

MSTL_BEGIN_INNER__
struct __pgsql_pstmt_data {
    vector<string> param_values;
    vector<const char*> param_ptrs;
    vector<int> param_lengths;
    vector<int> param_formats;
};
MSTL_END_INNER__

class MSTL_API postgresql_prepared_statement final : public idb_prepared_statement {
private:
    _MSTL_POSTGRESQL PGconn* conn_ = nullptr;
    string stmt_name_{};
    string sql_{};
    uint32_t param_count_ = 0;
    unique_ptr<_INNER __pgsql_pstmt_data> data_ = make_unique<_INNER __pgsql_pstmt_data>();
    vector<vector<char>> param_buffers_{};
    string last_error_{};
    uint32_t last_errno_ = 0;

    void init_params() const;
    bool prepare();
    void clear_error() noexcept;
    void set_error(string error, uint32_t errno_val = 0) noexcept;

public:
    postgresql_prepared_statement(PGconn* conn, const string& sql);
    ~postgresql_prepared_statement() override;

    MSTL_NODISCARD uint32_t param_count() const noexcept override { return param_count_; }

    bool bind_param(uint32_t index, const string& value) override { return bind_param(index, value.view()); }
    bool bind_param(uint32_t index, string_view value) override;
    bool bind_param(uint32_t index, const char* value) override { return bind_param(index, string_view(value)); }
    bool bind_param(uint32_t index, int32_t value) override;
    bool bind_param(uint32_t index, int64_t value) override;
    bool bind_param(uint32_t index, float64_t value) override;
    bool bind_param(uint32_t index, const void* data, size_t length) override;

    bool execute() override;
    MSTL_NODISCARD unique_ptr<idb_prepared_result> execute_query() override;
    MSTL_NODISCARD string_view get_error() const noexcept override { return last_error_.view(); }
    MSTL_NODISCARD uint32_t get_errno() const noexcept override { return last_errno_; }
};

MSTL_END_NAMESPACE__
#endif
#endif // MSTL_POSTGRESQL_PREPARED_STATEMENT_HPP__
