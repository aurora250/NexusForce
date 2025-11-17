#ifndef MSTL_MYSQL_RESULT_HPP__
#define MSTL_MYSQL_RESULT_HPP__
#ifdef MSTL_SUPPORT_MYSQL__
#include "MSTL/db/db_interface.hpp"
#include "mysql_config.hpp"
#include "MSTL/core/undef_cmacro.hpp"
MSTL_BEGIN_NAMESPACE__

struct MSTL_API mysql_result final : idb_tb_result {
private:
    _MSTL_MYSQL MYSQL_RES* result_ = nullptr;
    size_type rows_ = 0;
    size_type columns_ = 0;
    _MSTL_MYSQL MYSQL_ROW cursor_ = nullptr;

    unique_ptr<vector<string_view>> column_name_ = make_unique<vector<string_view>>();

    unique_ptr<vector<_MSTL_MYSQL enum_field_types>> column_types_ =
        make_unique<vector<_MSTL_MYSQL enum_field_types>>();

public:
    mysql_result() noexcept = default;
    explicit mysql_result(_MSTL_MYSQL MYSQL_RES* result) noexcept;

    ~mysql_result() override { if (result_) _MSTL_MYSQL mysql_free_result(result_); }

    MSTL_NODISCARD bool empty() const noexcept override { return result_ == nullptr; }
    MSTL_NODISCARD size_type row_count() const noexcept override { return rows_; }
    MSTL_NODISCARD size_type column_count() const noexcept override { return columns_; }

    MSTL_NODISCARD const vector<string_view>& column_names() const noexcept override { return *column_name_; }
    MSTL_NODISCARD const vector<_MSTL_MYSQL enum_field_types>& column_types() const noexcept { return *column_types_; }

    MSTL_NODISCARD bool next() noexcept override;

    MSTL_NODISCARD string_view get(size_type n) const noexcept override;

    MSTL_NODISCARD bool get_bool(size_type n) const override;
    MSTL_NODISCARD int8_t get_int8(size_type n) const override;
    MSTL_NODISCARD int16_t get_int16(size_type n) const override;
    MSTL_NODISCARD int32_t get_int32(size_type n) const override;
    MSTL_NODISCARD int64_t get_int64(size_type n) const override;
    MSTL_NODISCARD float32_t get_float32(size_type n) const override;
    MSTL_NODISCARD float64_t get_float64(size_type n) const override;
    MSTL_NODISCARD decimal_t get_decimal(size_type n) const override;
    MSTL_NODISCARD vector<char> get_blob(size_type n) const override;
    MSTL_NODISCARD string get_set(size_type n) const override;
    MSTL_NODISCARD uint64_t get_bit(size_type n) const override;
    MSTL_NODISCARD _MSTL date get_date(size_type n) const override;
    MSTL_NODISCARD _MSTL time get_time(size_type n) const override;
    MSTL_NODISCARD _MSTL datetime get_datetime(size_type n) const override;
    MSTL_NODISCARD _MSTL timestamp get_timestamp(size_type n) const override;
    MSTL_NODISCARD string get_string(size_type n) const noexcept override { return string{get(n)}; }
    MSTL_NODISCARD string_view get_enum(size_type n) const noexcept override { return get(n); }
};

MSTL_END_NAMESPACE__
#endif // MSTL_SUPPORT_MYSQL__
#endif // MSTL_MYSQL_RESULT_HPP__
