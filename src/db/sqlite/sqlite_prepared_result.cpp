#include <NeForce/db/sqlite/sqlite_prepared_result.hpp>
#ifdef NEFORCE_SUPPORT_SQLITE3
NEFORCE_BEGIN_NAMESPACE__

sqlite_prepared_result::sqlite_prepared_result(::sqlite3_stmt* statement) :
stmt_(statement) {
    if (stmt_ != nullptr) {
        columns_ = ::sqlite3_column_count(stmt_);
        for (size_type i = 0; i < columns_; ++i) {
            column_names_->push_back(::sqlite3_column_name(stmt_, static_cast<int>(i)));
            column_types_->push_back(::sqlite3_column_type(stmt_, static_cast<int>(i)));
        }
    }
}

bool sqlite_prepared_result::next() noexcept {
    if (empty()) {
        return false;
    }
    return ::sqlite3_step(stmt_) == SQLITE_ROW && (++cursor_) != 0U;
}

string_view sqlite_prepared_result::get(const size_type n) const noexcept {
    NEFORCE_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    NEFORCE_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    const auto* text = reinterpret_cast<const char*>(::sqlite3_column_text(stmt_, static_cast<int>(n)));
    return text != nullptr ? string_view{text} : string_view{};
}

bool sqlite_prepared_result::get_bool(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    NEFORCE_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    return ::sqlite3_column_int(stmt_, static_cast<int>(n)) != 0;
}

int16_t sqlite_prepared_result::get_int16(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    NEFORCE_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    return static_cast<int16_t>(::sqlite3_column_int(stmt_, static_cast<int>(n)));
}

int32_t sqlite_prepared_result::get_int32(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    NEFORCE_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    return ::sqlite3_column_int(stmt_, static_cast<int>(n));
}

int64_t sqlite_prepared_result::get_int64(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    NEFORCE_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    return ::sqlite3_column_int64(stmt_, static_cast<int>(n));
}

float32_t sqlite_prepared_result::get_float32(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    NEFORCE_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    return static_cast<float32_t>(::sqlite3_column_double(stmt_, static_cast<int>(n)));
}

float64_t sqlite_prepared_result::get_float64(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    NEFORCE_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    return static_cast<float64_t>(::sqlite3_column_double(stmt_, static_cast<int>(n)));
}

decimal_t sqlite_prepared_result::get_decimal(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    NEFORCE_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    return static_cast<decimal_t>(::sqlite3_column_double(stmt_, static_cast<int>(n)));
}

vector<char> sqlite_prepared_result::get_blob(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    NEFORCE_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    const string_view view = this->get(n);
    return vector<char>{view.data(), view.data() + view.size()};
}

uint64_t sqlite_prepared_result::get_bit(const size_type n) const noexcept {
    NEFORCE_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    NEFORCE_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    const auto data = get(n);
    uint64_t value = 0;
    for (const char i: data) {
        value = value << 8 | static_cast<byte_t>(i);
    }
    return value;
}

datetime sqlite_prepared_result::get_datetime(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    NEFORCE_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    const auto* text = reinterpret_cast<const char*>(::sqlite3_column_text(stmt_, static_cast<int>(n)));
    if (text != nullptr) {
        return datetime::parse(text);
    }
    return {};
}

timestamp sqlite_prepared_result::get_timestamp(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    NEFORCE_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    return timestamp{static_cast<long>(::sqlite3_column_int64(stmt_, static_cast<int>(n)))};
}

NEFORCE_END_NAMESPACE__
#endif
