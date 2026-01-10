#include <MSTL/database/sqlite/sqlite_prepared_result.hpp>
#ifdef MSTL_SUPPORT_SQLITE3__
MSTL_BEGIN_NAMESPACE__

sqlite_prepared_result::sqlite_prepared_result(::sqlite3_stmt* statement) noexcept : stmt_(statement) {
    if (stmt_) {
        columns_ = ::sqlite3_column_count(stmt_);
        for (int i = 0; i < columns_; ++i) {
            column_names_->push_back(::sqlite3_column_name(stmt_, i));
            column_types_->push_back(::sqlite3_column_type(stmt_, i));
        }
    }
}

bool sqlite_prepared_result::next() noexcept {
    if (empty()) return false;
    return ::sqlite3_step(stmt_) == SQLITE_ROW && ++cursor_;
}

_MSTL string_view sqlite_prepared_result::get(const size_type n) const noexcept {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    const auto text = reinterpret_cast<const char*>(::sqlite3_column_text(stmt_, n));
    return text ? string_view{text} : string_view{};
}

bool sqlite_prepared_result::get_bool(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    return ::sqlite3_column_int(stmt_, n) != 0;
}

int8_t sqlite_prepared_result::get_int8(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    return static_cast<int8_t>(::sqlite3_column_int(stmt_, n));
}

int16_t sqlite_prepared_result::get_int16(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    return static_cast<int16_t>(::sqlite3_column_int(stmt_, n));
}

int32_t sqlite_prepared_result::get_int32(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    return ::sqlite3_column_int(stmt_, n);
}

int64_t sqlite_prepared_result::get_int64(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    return ::sqlite3_column_int64(stmt_, n);
}

float32_t sqlite_prepared_result::get_float32(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    return static_cast<float32_t>(::sqlite3_column_double(stmt_, n));
}

float64_t sqlite_prepared_result::get_float64(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    return static_cast<float64_t>(::sqlite3_column_double(stmt_, n));
}

decimal_t sqlite_prepared_result::get_decimal(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    return static_cast<decimal_t>(::sqlite3_column_double(stmt_, n));
}

_MSTL vector<char> sqlite_prepared_result::get_blob(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    const string_view view = this->get(n);
    return vector<char>{view.data(), view.data() + view.size()};
}

uint64_t sqlite_prepared_result::get_bit(const size_type n) const noexcept {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    const auto data = get(n);
    uint64_t value = 0;
    for (const char i : data) {
        value = value << 8 | static_cast<byte_t>(i);
    }
    return value;
}

_MSTL datetime sqlite_prepared_result::get_datetime(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    const auto text = reinterpret_cast<const char*>(::sqlite3_column_text(stmt_, n));
    if (text) return _MSTL datetime::parse(text);
    return {};
}

_MSTL timestamp sqlite_prepared_result::get_timestamp(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    return timestamp{static_cast<long>(::sqlite3_column_int64(stmt_, n))};
}

MSTL_END_NAMESPACE__
#endif
