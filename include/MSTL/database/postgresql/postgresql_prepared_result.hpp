#ifndef MSTL_DATABASE_POSTGRESQL_PREPARED_RESULT_HPP__
#define MSTL_DATABASE_POSTGRESQL_PREPARED_RESULT_HPP__
#ifdef MSTL_SUPPORT_POSTGRESQL__
#include "postgresql_result.hpp"
MSTL_BEGIN_NAMESPACE__

class MSTL_API postgresql_prepared_result final : public idb_prepared_result {
private:
    unique_ptr<postgresql_tb_result> impl_ = nullptr;

public:
    explicit postgresql_prepared_result(_MSTL_POSTGRESQL PGresult* result) noexcept
    : impl_(make_unique<postgresql_tb_result>(result, true)) {}

    ~postgresql_prepared_result() override = default;

    MSTL_NODISCARD bool empty() const noexcept override { return impl_->empty(); }
    MSTL_NODISCARD bool next() noexcept override { return impl_->next(); }

    MSTL_NODISCARD size_type row_count() const noexcept override { return impl_->row_count(); }
    MSTL_NODISCARD size_type column_count() const noexcept override { return impl_->column_count(); }
    MSTL_NODISCARD const vector<string_view>& column_names() const override { return impl_->column_names(); }

    MSTL_NODISCARD string_view get(size_type index) const override { return impl_->get(index); }
    MSTL_NODISCARD bool get_bool(size_type index) const override { return impl_->get_bool(index); }
    MSTL_NODISCARD int8_t get_int8(size_type index) const override { return impl_->get_int8(index); }
    MSTL_NODISCARD int16_t get_int16(size_type index) const override { return impl_->get_int16(index); }
    MSTL_NODISCARD int32_t get_int32(size_type index) const override { return impl_->get_int32(index); }
    MSTL_NODISCARD int64_t get_int64(size_type index) const override { return impl_->get_int64(index); }
    MSTL_NODISCARD float32_t get_float32(size_type index) const override { return impl_->get_float32(index); }
    MSTL_NODISCARD float64_t get_float64(size_type index) const override { return impl_->get_float64(index); }
    MSTL_NODISCARD decimal_t get_decimal(size_type index) const override { return impl_->get_decimal(index); }
    MSTL_NODISCARD vector<char> get_blob(size_type index) const override { return impl_->get_blob(index); }
    MSTL_NODISCARD string get_set(size_type index) const override { return impl_->get_set(index); }
    MSTL_NODISCARD uint64_t get_bit(size_type index) const override { return impl_->get_bit(index); }
    MSTL_NODISCARD _MSTL date get_date(size_type index) const override { return impl_->get_date(index); }
    MSTL_NODISCARD _MSTL time get_time(size_type index) const override { return impl_->get_time(index); }
    MSTL_NODISCARD _MSTL datetime get_datetime(size_type index) const override { return impl_->get_datetime(index); }
    MSTL_NODISCARD _MSTL timestamp get_timestamp(size_type index) const override { return impl_->get_timestamp(index); }
    MSTL_NODISCARD string get_string(size_type index) const override { return impl_->get_string(index); }
    MSTL_NODISCARD string_view get_enum(size_type index) const override { return impl_->get_enum(index); }
};

MSTL_END_NAMESPACE__
#endif
#endif // MSTL_DATABASE_POSTGRESQL_PREPARED_RESULT_HPP__
