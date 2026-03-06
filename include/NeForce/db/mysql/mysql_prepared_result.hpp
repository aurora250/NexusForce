#ifndef NEFORCE_DATABASE_MYSQL_PREPARED_RESULT_HPP__
#define NEFORCE_DATABASE_MYSQL_PREPARED_RESULT_HPP__
#ifdef NEFORCE_SUPPORT_MYSQL
#include "NeForce/db/db_interface.hpp"
#ifdef CR_OUT_OF_MEMORY
#undef CR_OUT_OF_MEMORY
#endif
#include <mysql/mysql.h>
NEFORCE_BEGIN_NAMESPACE__

class NEFORCE_API mysql_prepared_result final : public idb_prepared_result {
private:
    ::MYSQL_STMT* stmt_ = nullptr;
    ::MYSQL_RES* metadata_ = nullptr;
    uint32_t column_count_ = 0;
    uint64_t row_count_ = 0;
    bool has_current_row_ = false;

    unique_ptr<vector<string_view>> column_names_ = make_unique<vector<string_view>>();
    unique_ptr<vector<::enum_field_types>> column_types_ = make_unique<vector<::enum_field_types>>();

    unique_ptr<vector<::MYSQL_BIND>> bind_results_ = make_unique<vector<::MYSQL_BIND>>();
    unique_ptr<vector<vector<char>>> buffers_ = make_unique<vector<vector<char>>>();
    unique_ptr<vector<unsigned long>> lengths_ = make_unique<vector<unsigned long>>();
    unique_ptr<vector<bool>> is_null_ = make_unique<vector<bool>>();
    unique_ptr<vector<bool>> is_error_ = make_unique<vector<bool>>();

    void initialize_bindings() const;
    static size_t get_buffer_size(::enum_field_types type);

public:
    explicit mysql_prepared_result(::MYSQL_STMT* stmt);
    ~mysql_prepared_result() override;

    mysql_prepared_result(const mysql_prepared_result&) = delete;
    mysql_prepared_result& operator =(const mysql_prepared_result&) = delete;

    bool empty() const override { return row_count_ == 0; }
    bool next() override;

    size_type row_count() const override { return row_count_; }
    size_type column_count() const override { return column_count_; }

    const vector<string_view>& column_names() const override { return *column_names_; }
    const vector<::enum_field_types>& column_types() const { return *column_types_; }

    string_view get(size_type n) const override;
    bool get_bool(size_type n) const override;
    int8_t get_int8(size_type n) const override;
    int16_t get_int16(size_type n) const override;
    int32_t get_int32(size_type n) const override;
    int64_t get_int64(size_type n) const override;
    float32_t get_float32(size_type n) const override;
    float64_t get_float64(size_type n) const override;
    decimal_t get_decimal(size_type n) const override;
    vector<char> get_blob(size_type n) const override;
    string get_set(size_type n) const override;
    uint64_t get_bit(size_type n) const override;
    date get_date(size_type n) const override;
    time get_time(size_type n) const override;
    datetime get_datetime(size_type n) const override;
    timestamp get_timestamp(size_type n) const override;
    string get_string(size_type n) const override { return string(get(n)); }
    string_view get_enum(size_type n) const override { return get(n); }
};

NEFORCE_END_NAMESPACE__
#endif
#endif // NEFORCE_DATABASE_MYSQL_PREPARED_RESULT_HPP__
