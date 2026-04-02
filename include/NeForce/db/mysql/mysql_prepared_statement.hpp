#ifndef NEFORCE_DATABASE_MYSQL_PREPARED_STATEMENT_HPP__
#define NEFORCE_DATABASE_MYSQL_PREPARED_STATEMENT_HPP__
#ifdef NEFORCE_SUPPORT_MYSQL
#    include <mysql/mysql.h>
#    include "NeForce/db/db_interface.hpp"
NEFORCE_BEGIN_NAMESPACE__

class NEFORCE_API mysql_prepared_statement final : public idb_prepared_statement {
private:
    ::MYSQL_STMT* stmt_ = nullptr;
    ::MYSQL* conn_ = nullptr;
    uint32_t param_count_ = 0;

    vector<::MYSQL_BIND> bind_params_;
    vector<vector<char>> param_buffers_;

public:
    mysql_prepared_statement(::MYSQL* conn, string_view sql);

    mysql_prepared_statement(mysql_prepared_statement&& other) noexcept;
    mysql_prepared_statement& operator=(mysql_prepared_statement&& other) noexcept;

    mysql_prepared_statement(const mysql_prepared_statement&) = delete;
    mysql_prepared_statement& operator=(const mysql_prepared_statement&) = delete;

    ~mysql_prepared_statement() override;

    NEFORCE_NODISCARD uint32_t param_count() const noexcept override { return param_count_; }

    bool bind_param(uint32_t index, const string& value) override { return bind_param(index, value.view()); }
    bool bind_param(uint32_t index, string_view value) override;
    bool bind_param(uint32_t index, const char* value) override { return bind_param(index, string_view{value}); }
    bool bind_param(uint32_t index, int32_t value) override;
    bool bind_param(uint32_t index, int64_t value) override;
    bool bind_param(uint32_t index, float64_t value) override;
    bool bind_param(uint32_t index, const void* data, size_t length) override;

    bool execute() override;
    NEFORCE_NODISCARD unique_ptr<idb_prepared_result> execute_query() override;

    NEFORCE_NODISCARD string_view get_error() const noexcept override;
    NEFORCE_NODISCARD uint32_t get_errno() const noexcept override;
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_SUPPORT_MYSQL
#endif // NEFORCE_DATABASE_MYSQL_PREPARED_STATEMENT_HPP__
