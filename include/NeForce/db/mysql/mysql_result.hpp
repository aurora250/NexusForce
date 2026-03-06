#ifndef NEFORCE_DATABASE_MYSQL_RESULT_HPP__
#define NEFORCE_DATABASE_MYSQL_RESULT_HPP__
#ifdef NEFORCE_SUPPORT_MYSQL
#include "NeForce/db/db_interface.hpp"
#ifdef CR_OUT_OF_MEMORY
#undef CR_OUT_OF_MEMORY
#endif
#include <mysql/mysql.h>
NEFORCE_BEGIN_NAMESPACE__

struct NEFORCE_API mysql_result final : idb_tb_result {
private:
    ::MYSQL_RES* result_ = nullptr;
    size_type rows_ = 0;
    size_type columns_ = 0;
    ::MYSQL_ROW cursor_ = nullptr;

    unique_ptr<vector<string_view>> column_name_ = make_unique<vector<string_view>>();

    unique_ptr<vector<::enum_field_types>> column_types_ =
        make_unique<vector<::enum_field_types>>();

public:
    mysql_result() noexcept = default;
    explicit mysql_result(::MYSQL_RES* result) noexcept;

    ~mysql_result() override { if (result_) ::mysql_free_result(result_); }

    NEFORCE_NODISCARD bool empty() const noexcept override { return result_ == nullptr; }
    NEFORCE_NODISCARD size_type row_count() const noexcept override { return rows_; }
    NEFORCE_NODISCARD size_type column_count() const noexcept override { return columns_; }

    NEFORCE_NODISCARD const vector<string_view>& column_names() const noexcept override { return *column_name_; }
    NEFORCE_NODISCARD const vector<::enum_field_types>& column_types() const noexcept { return *column_types_; }

    NEFORCE_NODISCARD bool next() noexcept override;

    NEFORCE_NODISCARD string_view get(size_type n) const noexcept override;

    NEFORCE_NODISCARD bool get_bool(size_type n) const override;
    NEFORCE_NODISCARD int8_t get_int8(size_type n) const override;
    NEFORCE_NODISCARD int16_t get_int16(size_type n) const override;
    NEFORCE_NODISCARD int32_t get_int32(size_type n) const override;
    NEFORCE_NODISCARD int64_t get_int64(size_type n) const override;
    NEFORCE_NODISCARD float32_t get_float32(size_type n) const override;
    NEFORCE_NODISCARD float64_t get_float64(size_type n) const override;
    NEFORCE_NODISCARD decimal_t get_decimal(size_type n) const override;
    NEFORCE_NODISCARD vector<char> get_blob(size_type n) const override;
    NEFORCE_NODISCARD string get_set(size_type n) const override;
    NEFORCE_NODISCARD uint64_t get_bit(size_type n) const override;
    NEFORCE_NODISCARD _NEFORCE date get_date(size_type n) const override;
    NEFORCE_NODISCARD _NEFORCE time get_time(size_type n) const override;
    NEFORCE_NODISCARD _NEFORCE datetime get_datetime(size_type n) const override;
    NEFORCE_NODISCARD _NEFORCE timestamp get_timestamp(size_type n) const override;
    NEFORCE_NODISCARD string get_string(size_type n) const noexcept override { return string{get(n)}; }
    NEFORCE_NODISCARD string_view get_enum(size_type n) const noexcept override { return get(n); }
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_SUPPORT_MYSQL
#endif // NEFORCE_DATABASE_MYSQL_RESULT_HPP__
