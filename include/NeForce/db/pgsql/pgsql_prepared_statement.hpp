#ifndef NEFORCE_DATABASE_PGSQL_PREPARED_STATEMENT_HPP__
#define NEFORCE_DATABASE_PGSQL_PREPARED_STATEMENT_HPP__
#ifdef NEFORCE_SUPPORT_POSTGRESQL
#    include <libpq-fe.h>
#    include "NeForce/db/db_interface.hpp"
NEFORCE_BEGIN_NAMESPACE__

class NEFORCE_API pgsql_prepared_statement final : public idb_prepared_statement {
private:
    struct pstmt_data {
        vector<string> param_values{};
        vector<const char*> param_ptrs{};
        vector<int> param_lengths{};
        vector<int> param_formats{};
    };

    ::PGconn* conn_ = nullptr;
    string stmt_name_{};
    string sql_{};
    uint32_t param_count_ = 0;
    unique_ptr<pstmt_data> data_ = make_unique<pstmt_data>();
    vector<vector<char>> param_buffers_{};
    string last_error_{};
    uint32_t last_errno_ = 0;

    void set_error(string error, uint32_t errno_val = 0) noexcept;

public:
    pgsql_prepared_statement(PGconn* conn, string sql);
    ~pgsql_prepared_statement() override;

    NEFORCE_NODISCARD uint32_t param_count() const noexcept override { return param_count_; }

    bool bind_param(uint32_t index, const string& value) override { return bind_param(index, value.view()); }
    bool bind_param(uint32_t index, string_view value) override;
    bool bind_param(uint32_t index, const char* value) override { return bind_param(index, string_view(value)); }
    bool bind_param(uint32_t index, int32_t value) override;
    bool bind_param(uint32_t index, int64_t value) override;
    bool bind_param(uint32_t index, float64_t value) override;
    bool bind_param(uint32_t index, const void* data, size_t length) override;

    bool execute() override;
    NEFORCE_NODISCARD unique_ptr<idb_prepared_result> execute_query() override;
    NEFORCE_NODISCARD string_view get_error() const noexcept override { return last_error_.view(); }
    NEFORCE_NODISCARD uint32_t get_errno() const noexcept override { return last_errno_; }
};

NEFORCE_END_NAMESPACE__
#endif
#endif // NEFORCE_DATABASE_PGSQL_PREPARED_STATEMENT_HPP__
