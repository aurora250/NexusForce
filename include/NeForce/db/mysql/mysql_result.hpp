#ifndef NEFORCE_DATABASE_MYSQL_RESULT_HPP__
#define NEFORCE_DATABASE_MYSQL_RESULT_HPP__
#ifdef NEFORCE_SUPPORT_MYSQL
#    include <mysql/mysql.h>
#    include "NeForce/db/db_interface.hpp"
NEFORCE_BEGIN_NAMESPACE__

struct NEFORCE_API mysql_result final : idb_tb_result {
private:
    ::MYSQL_RES* result_ = nullptr;
    ::MYSQL_ROW cursor_ = nullptr;
    size_type rows_ = 0;
    size_type columns_ = 0;

    unique_ptr<vector<string_view>> column_name_;
    unique_ptr<vector<::enum_field_types>> column_types_;

public:
    mysql_result() noexcept;
    explicit mysql_result(::MYSQL_RES* result) noexcept;
    ~mysql_result() override;

    NEFORCE_NODISCARD bool empty() const noexcept override { return result_ == nullptr; }
    NEFORCE_NODISCARD size_type row_count() const noexcept override { return rows_; }
    NEFORCE_NODISCARD size_type column_count() const noexcept override { return columns_; }

    NEFORCE_NODISCARD const vector<string_view>& column_names() const noexcept override { return *column_name_; }
    NEFORCE_NODISCARD const vector<::enum_field_types>& column_types() const noexcept { return *column_types_; }

    NEFORCE_NODISCARD bool next() noexcept override;

    NEFORCE_NODISCARD string_view get(size_type n) const noexcept override;

    NEFORCE_NODISCARD bool get_bool(size_type n) const override;
    NEFORCE_NODISCARD int16_t get_int16(size_type n) const override;
    NEFORCE_NODISCARD int32_t get_int32(size_type n) const override;
    NEFORCE_NODISCARD int64_t get_int64(size_type n) const override;
    NEFORCE_NODISCARD float32_t get_float32(size_type n) const override;
    NEFORCE_NODISCARD float64_t get_float64(size_type n) const override;
    NEFORCE_NODISCARD decimal_t get_decimal(size_type n) const override;

    NEFORCE_NODISCARD vector<char> get_blob(size_type n) const override;
    NEFORCE_NODISCARD uint64_t get_bit(size_type n) const override;

    NEFORCE_NODISCARD date get_date(size_type n) const override;
    NEFORCE_NODISCARD time get_time(size_type n) const override;
    NEFORCE_NODISCARD datetime get_datetime(size_type n) const override;
    NEFORCE_NODISCARD timestamp get_timestamp(size_type n) const override;
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_SUPPORT_MYSQL
#endif // NEFORCE_DATABASE_MYSQL_RESULT_HPP__
