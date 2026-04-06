#ifndef NEFORCE_DATABASE_PGSQL_RESULT_HPP__
#define NEFORCE_DATABASE_PGSQL_RESULT_HPP__
#ifdef NEFORCE_SUPPORT_POSTGRESQL
#    include <libpq-fe.h>
#    include "NeForce/db/db_interface.hpp"
NEFORCE_BEGIN_NAMESPACE__

class NEFORCE_API pgsql_tb_result final : public idb_tb_result {
private:
    ::PGresult* result_ = nullptr;
    size_type current_row_ = 0;
    size_type row_count_ = 0;
    size_type column_count_ = 0;
    mutable vector<string_view> column_names_;
    bool owns_result_;

public:
    explicit pgsql_tb_result(::PGresult* result, bool owns = true);
    ~pgsql_tb_result() override;

    pgsql_tb_result(const pgsql_tb_result&) = delete;
    pgsql_tb_result& operator=(const pgsql_tb_result&) = delete;

    NEFORCE_NODISCARD bool empty() const noexcept override { return row_count_ == 0; }
    bool is_null(size_type index) const;
    NEFORCE_NODISCARD bool next() noexcept override;

    NEFORCE_NODISCARD size_type row_count() const noexcept override { return row_count_; }
    NEFORCE_NODISCARD size_type column_count() const noexcept override { return column_count_; }
    NEFORCE_NODISCARD const vector<string_view>& column_names() const override;

    NEFORCE_NODISCARD string_view get(size_type index) const override;

    NEFORCE_NODISCARD bool get_bool(size_type index) const override;
    NEFORCE_NODISCARD int16_t get_int16(size_type index) const override;
    NEFORCE_NODISCARD int32_t get_int32(size_type index) const override;
    NEFORCE_NODISCARD int64_t get_int64(size_type index) const override;
    NEFORCE_NODISCARD float32_t get_float32(size_type index) const override;
    NEFORCE_NODISCARD float64_t get_float64(size_type index) const override;
    NEFORCE_NODISCARD decimal_t get_decimal(size_type index) const override;

    NEFORCE_NODISCARD vector<char> get_blob(size_type index) const override;
    NEFORCE_NODISCARD uint64_t get_bit(size_type index) const override;

    NEFORCE_NODISCARD date get_date(size_type index) const override;
    NEFORCE_NODISCARD time get_time(size_type index) const override;
    NEFORCE_NODISCARD datetime get_datetime(size_type index) const override;
    NEFORCE_NODISCARD timestamp get_timestamp(size_type index) const override;
};

NEFORCE_END_NAMESPACE__
#endif
#endif // NEFORCE_DATABASE_PGSQL_RESULT_HPP__
