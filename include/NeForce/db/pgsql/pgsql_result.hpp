#ifndef NEFORCE_DATABASE_PGSQL_RESULT_HPP__
#define NEFORCE_DATABASE_PGSQL_RESULT_HPP__
#ifdef NEFORCE_SUPPORT_POSTGRESQL
#include "NeForce/db/db_interface.hpp"
#include <libpq-fe.h>
NEFORCE_BEGIN_NAMESPACE__

class NEFORCE_API pgsql_tb_result final : public idb_tb_result {
private:
    mutable vector<string_view> column_names_;
    ::PGresult* result_ = nullptr;
    size_type current_row_ = 0;
    size_type row_count_ = 0;
    size_type column_count_ = 0;
    bool owns_result_;

protected:
    void init_column_names() const;
    bool is_null(size_type index) const;

public:
    explicit pgsql_tb_result(::PGresult* result, bool owns = true) noexcept;
    ~pgsql_tb_result() override;

    pgsql_tb_result(const pgsql_tb_result&) = delete;
    pgsql_tb_result& operator =(const pgsql_tb_result&) = delete;

    NEFORCE_NODISCARD bool empty() const noexcept override { return row_count_ == 0; }
    NEFORCE_NODISCARD bool next() noexcept override;

    NEFORCE_NODISCARD size_type row_count() const noexcept override { return row_count_; }
    NEFORCE_NODISCARD size_type column_count() const noexcept override { return column_count_; }
    NEFORCE_NODISCARD const vector<string_view>& column_names() const override;

    NEFORCE_NODISCARD string_view get(size_type index) const override;
    NEFORCE_NODISCARD bool get_bool(size_type index) const override;
    NEFORCE_NODISCARD int8_t get_int8(size_type index) const override;
    NEFORCE_NODISCARD int16_t get_int16(size_type index) const override;
    NEFORCE_NODISCARD int32_t get_int32(size_type index) const override;
    NEFORCE_NODISCARD int64_t get_int64(size_type index) const override;
    NEFORCE_NODISCARD float32_t get_float32(size_type index) const override;
    NEFORCE_NODISCARD float64_t get_float64(size_type index) const override;
    NEFORCE_NODISCARD decimal_t get_decimal(size_type index) const override;
    NEFORCE_NODISCARD vector<char> get_blob(size_type index) const override;
    NEFORCE_NODISCARD string get_set(size_type index) const override { return get_string(index); }
    NEFORCE_NODISCARD uint64_t get_bit(size_type index) const override;
    NEFORCE_NODISCARD _NEFORCE date get_date(size_type index) const override;
    NEFORCE_NODISCARD _NEFORCE time get_time(size_type index) const override;
    NEFORCE_NODISCARD _NEFORCE datetime get_datetime(size_type index) const override;
    NEFORCE_NODISCARD _NEFORCE timestamp get_timestamp(size_type index) const override;
    NEFORCE_NODISCARD string get_string(size_type index) const override { return string{get(index)}; }
    NEFORCE_NODISCARD string_view get_enum(size_type index) const override { return get(index); }
};

NEFORCE_END_NAMESPACE__
#endif
#endif // NEFORCE_DATABASE_PGSQL_RESULT_HPP__
