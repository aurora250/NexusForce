#ifndef NEFORCE_DATABASE_PGSQL_PREPARED_RESULT_HPP__
#define NEFORCE_DATABASE_PGSQL_PREPARED_RESULT_HPP__
#ifdef NEFORCE_SUPPORT_POSTGRESQL
#include "pgsql_result.hpp"
NEFORCE_BEGIN_NAMESPACE__

class NEFORCE_API pgsql_prepared_result final : public idb_prepared_result {
private:
    pgsql_tb_result impl_;

public:
    explicit pgsql_prepared_result(::PGresult* result) noexcept
    : impl_(result, true) {}

    ~pgsql_prepared_result() override = default;

    NEFORCE_NODISCARD bool empty() const noexcept override { return impl_.empty(); }
    NEFORCE_NODISCARD bool next() noexcept override { return impl_.next(); }

    NEFORCE_NODISCARD size_type row_count() const noexcept override { return impl_.row_count(); }
    NEFORCE_NODISCARD size_type column_count() const noexcept override { return impl_.column_count(); }
    NEFORCE_NODISCARD const vector<string_view>& column_names() const override { return impl_.column_names(); }

    NEFORCE_NODISCARD string_view get(size_type index) const override { return impl_.get(index); }

    NEFORCE_NODISCARD bool get_bool(size_type index) const override { return impl_.get_bool(index); }
    NEFORCE_NODISCARD int16_t get_int16(size_type index) const override { return impl_.get_int16(index); }
    NEFORCE_NODISCARD int32_t get_int32(size_type index) const override { return impl_.get_int32(index); }
    NEFORCE_NODISCARD int64_t get_int64(size_type index) const override { return impl_.get_int64(index); }
    NEFORCE_NODISCARD float32_t get_float32(size_type index) const override { return impl_.get_float32(index); }
    NEFORCE_NODISCARD float64_t get_float64(size_type index) const override { return impl_.get_float64(index); }
    NEFORCE_NODISCARD decimal_t get_decimal(size_type index) const override { return impl_.get_decimal(index); }

    NEFORCE_NODISCARD vector<char> get_blob(size_type index) const override { return impl_.get_blob(index); }
    NEFORCE_NODISCARD uint64_t get_bit(size_type index) const override { return impl_.get_bit(index); }

    NEFORCE_NODISCARD _NEFORCE date get_date(size_type index) const override { return impl_.get_date(index); }
    NEFORCE_NODISCARD _NEFORCE time get_time(size_type index) const override { return impl_.get_time(index); }
    NEFORCE_NODISCARD _NEFORCE datetime get_datetime(size_type index) const override { return impl_.get_datetime(index); }
    NEFORCE_NODISCARD _NEFORCE timestamp get_timestamp(size_type index) const override { return impl_.get_timestamp(index); }
};

NEFORCE_END_NAMESPACE__
#endif
#endif // NEFORCE_DATABASE_PGSQL_PREPARED_RESULT_HPP__
