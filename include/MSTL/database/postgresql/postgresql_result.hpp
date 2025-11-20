#ifndef MSTL_POSTGRESQL_RESULT_HPP__
#define MSTL_POSTGRESQL_RESULT_HPP__
#ifdef MSTL_SUPPORT_POSTGRESQL__
#include "MSTL/core/config/undef_cmacro.hpp"
#include "MSTL/database/db_interface.hpp"
#include "postgresql_config.hpp"
MSTL_BEGIN_NAMESPACE__

class MSTL_API postgresql_tb_result final : public idb_tb_result {
private:
    mutable vector<string_view> column_names_;
    _MSTL_POSTGRESQL PGresult* result_ = nullptr;
    size_type current_row_ = 0;
    size_type row_count_ = 0;
    size_type column_count_ = 0;
    bool owns_result_;

protected:
    void init_column_names() const;
    bool is_null(size_type index) const;

public:
    explicit postgresql_tb_result(_MSTL_POSTGRESQL PGresult* result, bool owns = true) noexcept;
    ~postgresql_tb_result() override;

    postgresql_tb_result(const postgresql_tb_result&) = delete;
    postgresql_tb_result& operator=(const postgresql_tb_result&) = delete;

    MSTL_NODISCARD bool empty() const noexcept override { return row_count_ == 0; }
    MSTL_NODISCARD bool next() noexcept override;

    MSTL_NODISCARD size_type row_count() const noexcept override { return row_count_; }
    MSTL_NODISCARD size_type column_count() const noexcept override { return column_count_; }
    MSTL_NODISCARD const vector<string_view>& column_names() const override;

    MSTL_NODISCARD string_view get(size_type index) const override;
    MSTL_NODISCARD bool get_bool(size_type index) const override;
    MSTL_NODISCARD int8_t get_int8(size_type index) const override;
    MSTL_NODISCARD int16_t get_int16(size_type index) const override;
    MSTL_NODISCARD int32_t get_int32(size_type index) const override;
    MSTL_NODISCARD int64_t get_int64(size_type index) const override;
    MSTL_NODISCARD float32_t get_float32(size_type index) const override;
    MSTL_NODISCARD float64_t get_float64(size_type index) const override;
    MSTL_NODISCARD decimal_t get_decimal(size_type index) const override;
    MSTL_NODISCARD vector<char> get_blob(size_type index) const override;
    MSTL_NODISCARD string get_set(size_type index) const override { return get_string(index); }
    MSTL_NODISCARD uint64_t get_bit(size_type index) const override;
    MSTL_NODISCARD _MSTL date get_date(size_type index) const override;
    MSTL_NODISCARD _MSTL time get_time(size_type index) const override;
    MSTL_NODISCARD _MSTL datetime get_datetime(size_type index) const override;
    MSTL_NODISCARD _MSTL timestamp get_timestamp(size_type index) const override;
    MSTL_NODISCARD string get_string(size_type index) const override { return string{get(index)}; }
    MSTL_NODISCARD string_view get_enum(size_type index) const override { return get(index); }
};

MSTL_END_NAMESPACE__
#endif
#endif // MSTL_POSTGRESQL_RESULT_HPP__
