#ifndef MSTL_DATABASE_SQLITE_PREPARED_STATEMENT_HPP__
#define MSTL_DATABASE_SQLITE_PREPARED_STATEMENT_HPP__
#ifdef MSTL_SUPPORT_SQLITE3__
#include "MSTL/database/db_interface.hpp"
#include <sqlite3.h>
MSTL_BEGIN_NAMESPACE__

class MSTL_API sqlite_prepared_statement final : public idb_prepared_statement {
private:
    ::sqlite3* db_ = nullptr;
    ::sqlite3_stmt* stmt_ = nullptr;

    uint32_t param_count_ = 0;
    vector<vector<char>> param_buffers_;
    bool prepared_ = false;
    mutable string last_error_;

    void clear_bindings() noexcept;
    void reset_statement() noexcept;

public:
    explicit sqlite_prepared_statement(::sqlite3* db, const string& sql);

    sqlite_prepared_statement(const sqlite_prepared_statement&) = delete;
    sqlite_prepared_statement& operator =(const sqlite_prepared_statement&) = delete;

    sqlite_prepared_statement(sqlite_prepared_statement&& other) noexcept;
    sqlite_prepared_statement& operator =(sqlite_prepared_statement&& other) noexcept;

    ~sqlite_prepared_statement() override;

    MSTL_NODISCARD uint32_t param_count() const noexcept override { return param_count_; }

    bool bind_param(uint32_t index, string_view value) override;
    bool bind_param(uint32_t index, const string& value) override { return bind_param(index, value.view()); }
    bool bind_param(uint32_t index, const char* value) override { return bind_param(index, string_view(value)); }
    bool bind_param(uint32_t index, int32_t value) override;
    bool bind_param(uint32_t index, int64_t value) override;
    bool bind_param(uint32_t index, float64_t value) override;
    bool bind_param(uint32_t index, const void* data, size_t length) override;

    bool execute() override;
    unique_ptr<idb_prepared_result> execute_query() override;

    MSTL_NODISCARD string_view get_error() const noexcept override { return last_error_.view(); }
    MSTL_NODISCARD uint32_t get_errno() const noexcept override {
        return db_ ? ::sqlite3_errcode(db_) : 0;
    }
};

MSTL_END_NAMESPACE__
#endif
#endif // MSTL_DATABASE_SQLITE_PREPARED_STATEMENT_HPP__
