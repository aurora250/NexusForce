#ifndef NEFORCE_DATABASE_SQLITE_PREPARED_RESULT_HPP__
#define NEFORCE_DATABASE_SQLITE_PREPARED_RESULT_HPP__
#ifdef NEFORCE_SUPPORT_SQLITE3
#    include <sqlite3.h>
#    include "NeForce/db/db_interface.hpp"
NEFORCE_BEGIN_NAMESPACE__

struct NEFORCE_API sqlite_prepared_result final : idb_prepared_result {
private:
    ::sqlite3_stmt* stmt_ = nullptr;
    size_type cursor_ = 0;
    size_type columns_ = 0;
    unique_ptr<vector<string_view>> column_names_ = make_unique<vector<string_view>>();
    unique_ptr<vector<int>> column_types_ = make_unique<vector<int>>();

public:
    sqlite_prepared_result() noexcept = default;
    explicit sqlite_prepared_result(::sqlite3_stmt* statement);

    ~sqlite_prepared_result() override {
        if (stmt_) {
            ::sqlite3_reset(stmt_);
        }
    }

    NEFORCE_NODISCARD NEFORCE_DEPRECATED_FOR("use COUNT * instead of using this function") size_type
            row_count() const noexcept override {
        return 0;
    }
    NEFORCE_NODISCARD size_type column_count() const noexcept override { return columns_; }

    NEFORCE_NODISCARD bool empty() const noexcept override { return stmt_ == nullptr; }
    NEFORCE_NODISCARD const vector<string_view>& column_names() const noexcept override { return *column_names_; }
    NEFORCE_NODISCARD const vector<int>& column_types() const noexcept { return *column_types_; }
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
    NEFORCE_NODISCARD uint64_t get_bit(size_type n) const noexcept override;

    NEFORCE_NODISCARD date get_date(size_type n) const override { return get_datetime(n).date(); }
    NEFORCE_NODISCARD time get_time(size_type n) const override { return get_datetime(n).time(); }
    NEFORCE_NODISCARD datetime get_datetime(size_type n) const override;
    NEFORCE_NODISCARD timestamp get_timestamp(size_type n) const override;
};

NEFORCE_END_NAMESPACE__
#endif
#endif // NEFORCE_DATABASE_SQLITE_PREPARED_RESULT_HPP__
