#ifndef MSTL_SQLITE_PREPARED_RESULT_HPP__
#define MSTL_SQLITE_PREPARED_RESULT_HPP__
#ifdef MSTL_SUPPORT_SQLITE3__
#include "MSTL/db/db_interface.hpp"
#include "sqlite_config.hpp"
MSTL_BEGIN_NAMESPACE__

struct MSTL_API sqlite_prepared_result final : idb_prepared_result {
private:
    _MSTL_SQLITE sqlite3_stmt* stmt_ = nullptr;
    size_type cursor_ = 0;
    size_type columns_ = 0;
    unique_ptr<vector<string_view>> column_names_ = make_unique<vector<string_view>>();
    unique_ptr<vector<int>> column_types_ = make_unique<vector<int>>();

public:
    sqlite_prepared_result() noexcept = default;
    explicit sqlite_prepared_result(_MSTL_SQLITE sqlite3_stmt* statement) noexcept;

    ~sqlite_prepared_result() override { if (stmt_) _MSTL_SQLITE sqlite3_reset(stmt_); }

    MSTL_NODISCARD MSTL_DEPRECATE_FOR("use COUNT * instead of using this function")
    size_type row_count() const noexcept override { return 0; }
    MSTL_NODISCARD size_type column_count() const noexcept override { return columns_; }

    MSTL_NODISCARD bool empty() const noexcept override { return stmt_ == nullptr; }
    MSTL_NODISCARD const vector<string_view>& column_names() const noexcept override { return *column_names_; }
    MSTL_NODISCARD const vector<int>& column_types() const noexcept { return *column_types_; }
    MSTL_NODISCARD bool next() noexcept override;

    MSTL_NODISCARD _MSTL string_view get(size_type n) const noexcept override;

    MSTL_NODISCARD bool get_bool(size_type n) const override;
    MSTL_NODISCARD int8_t get_int8(size_type n) const override;
    MSTL_NODISCARD int16_t get_int16(size_type n) const override;
    MSTL_NODISCARD int32_t get_int32(size_type n) const override;
    MSTL_NODISCARD int64_t get_int64(size_type n) const override;
    MSTL_NODISCARD float32_t get_float32(size_type n) const override;
    MSTL_NODISCARD float64_t get_float64(size_type n) const override;
    MSTL_NODISCARD decimal_t get_decimal(size_type n) const override;
    MSTL_NODISCARD _MSTL vector<char> get_blob(size_type n) const override;
    MSTL_NODISCARD _MSTL string get_set(size_type n) const override { return get_string(n); }
    MSTL_NODISCARD uint64_t get_bit(size_type n) const noexcept override;
    MSTL_NODISCARD _MSTL date get_date(size_type n) const noexcept override { return get_datetime(n).dates(); }
    MSTL_NODISCARD _MSTL time get_time(size_type n) const noexcept override { return get_datetime(n).times(); }
    MSTL_NODISCARD _MSTL datetime get_datetime(size_type n) const override;
    MSTL_NODISCARD _MSTL timestamp get_timestamp(size_type n) const override;
    MSTL_NODISCARD _MSTL string get_string(size_type n) const override { return string{get(n)}; }
    MSTL_NODISCARD _MSTL string_view get_enum(size_type n) const override { return get(n); }
};

MSTL_END_NAMESPACE__
#endif
#endif // MSTL_SQLITE_PREPARED_RESULT_HPP__
