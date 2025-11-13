#include <MSTL/db/mysql/mysql_result.hpp>
#ifdef MSTL_SUPPORT_MYSQL__
MSTL_BEGIN_NAMESPACE__

mysql_result::mysql_result(_MSTL_MYSQL MYSQL_RES* result) noexcept
: result_(result), rows_(_MSTL_MYSQL mysql_num_rows(result)), columns_(_MSTL_MYSQL mysql_num_fields(result)) {
    _MSTL_MYSQL MYSQL_FIELD* field;
    while ((field = _MSTL_MYSQL mysql_fetch_field(result))) {
        column_name_->push_back(field->name);
        column_types_->push_back(field->type);
    }
}

bool mysql_result::next() noexcept {
    if (!empty()) {
        cursor_ = _MSTL_MYSQL mysql_fetch_row(result_);
        return cursor_ != nullptr;
    }
    return false;
}

_MSTL string_view mysql_result::get(const size_type n) const noexcept {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    return cursor_[n];
}

bool mysql_result::get_bool(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    if (column_types_->at(n) != _MSTL_MYSQL MYSQL_TYPE_BOOL)
        Exception(DatabaseTypeCastError("database type cast to bool mismatch"));
    return static_cast<bool>(boolean::parse(cursor_[n]));
}

int8_t mysql_result::get_int8(const size_type n) const {
    return static_cast<int8_t>(this->get_int16(n));
}

int16_t mysql_result::get_int16(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    const auto type = column_types_->at(n);
    if (!(type == _MSTL_MYSQL MYSQL_TYPE_SHORT || type == _MSTL_MYSQL MYSQL_TYPE_TINY ||
        type == _MSTL_MYSQL MYSQL_TYPE_BOOL))
        Exception(DatabaseTypeCastError("database type cast to int16 mismatch"));
    return integer16::parse(cursor_[n]);
}

int32_t mysql_result::get_int32(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    const auto type = column_types_->at(n);
    if (!(type == _MSTL_MYSQL MYSQL_TYPE_LONG || type == _MSTL_MYSQL MYSQL_TYPE_INT24 ||
        type == _MSTL_MYSQL MYSQL_TYPE_SHORT || type == _MSTL_MYSQL MYSQL_TYPE_TINY ||
        type == _MSTL_MYSQL MYSQL_TYPE_BOOL))
        Exception(DatabaseTypeCastError("database type cast to int32 mismatch"));
    return integer32::parse(cursor_[n]);
}

int64_t mysql_result::get_int64(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    const auto type = column_types_->at(n);
    if (!(type == _MSTL_MYSQL MYSQL_TYPE_LONGLONG || type == _MSTL_MYSQL MYSQL_TYPE_LONG ||
        type == _MSTL_MYSQL MYSQL_TYPE_INT24 || type == _MSTL_MYSQL MYSQL_TYPE_SHORT ||
        type == _MSTL_MYSQL MYSQL_TYPE_TINY || type == _MSTL_MYSQL MYSQL_TYPE_BOOL))
        Exception(DatabaseTypeCastError("database type cast to int64 mismatch"));
    return integer64::parse(cursor_[n]);
}

float32_t mysql_result::get_float32(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    const auto type = column_types_->at(n);
    if (!(type == _MSTL_MYSQL MYSQL_TYPE_FLOAT || type == _MSTL_MYSQL MYSQL_TYPE_LONG
        || type == _MSTL_MYSQL MYSQL_TYPE_SHORT || type == _MSTL_MYSQL MYSQL_TYPE_TINY))
        Exception(DatabaseTypeCastError("database type cast to float32 mismatch"));
    return float32::parse(cursor_[n]);
}

float64_t mysql_result::get_float64(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    const auto type = column_types_->at(n);
    if (!(type == _MSTL_MYSQL MYSQL_TYPE_DOUBLE || type == _MSTL_MYSQL MYSQL_TYPE_FLOAT ||
        type == _MSTL_MYSQL MYSQL_TYPE_LONGLONG || type == _MSTL_MYSQL MYSQL_TYPE_LONG ||
        type == _MSTL_MYSQL MYSQL_TYPE_SHORT || type == _MSTL_MYSQL MYSQL_TYPE_TINY))
        Exception(DatabaseTypeCastError("database type cast to float64 mismatch"));
    return float64::parse(cursor_[n]);
}

decimal_t mysql_result::get_decimal(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    const auto type = column_types_->at(n);
    if (!(type == _MSTL_MYSQL MYSQL_TYPE_DECIMAL || type == _MSTL_MYSQL MYSQL_TYPE_NEWDECIMAL ||
        type == _MSTL_MYSQL MYSQL_TYPE_DOUBLE || type == _MSTL_MYSQL MYSQL_TYPE_FLOAT ||
        type == _MSTL_MYSQL MYSQL_TYPE_LONGLONG || type == _MSTL_MYSQL MYSQL_TYPE_LONG ||
        type == _MSTL_MYSQL MYSQL_TYPE_SHORT || type == _MSTL_MYSQL MYSQL_TYPE_TINY))
        Exception(DatabaseTypeCastError("database type cast to decimal mismatch"));
    return decimal::parse(cursor_[n]);
}

_MSTL vector<char> mysql_result::get_blob(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    const auto type = column_types_->at(n);
    if (!(type == _MSTL_MYSQL MYSQL_TYPE_BLOB || type == _MSTL_MYSQL MYSQL_TYPE_TINY_BLOB ||
        type == _MSTL_MYSQL MYSQL_TYPE_MEDIUM_BLOB || type == _MSTL_MYSQL MYSQL_TYPE_LONG_BLOB))
        Exception(DatabaseTypeCastError("database type cast to blob mismatch"));
    return {cursor_[n], cursor_[n] + mysql_fetch_lengths(result_)[n]};
}

_MSTL string mysql_result::get_set(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    if (column_types_->at(n) != _MSTL_MYSQL MYSQL_TYPE_SET) {
        Exception(DatabaseTypeCastError("database type cast to SET mismatch"));
    }
    return cursor_[n];
}

uint64_t mysql_result::get_bit(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    if (column_types_->at(n) != _MSTL_MYSQL MYSQL_TYPE_BIT) {
        Exception(DatabaseTypeCastError("database type cast to BIT mismatch"));
    }
    const unsigned long length = mysql_fetch_lengths(result_)[n];
    const char* data = cursor_[n];

    uint64_t value = 0;
    for (unsigned long i = 0; i < length; ++i) {
        value = (value << 8) | static_cast<byte_t>(data[i]);
    }
    return value;
}

_MSTL date mysql_result::get_date(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    if (column_types_->at(n) != _MSTL_MYSQL MYSQL_TYPE_DATE)
        Exception(DatabaseTypeCastError("database type cast to date mismatch"));
    return _MSTL date::parse(cursor_[n]);
}

_MSTL time mysql_result::get_time(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    if (column_types_->at(n) != _MSTL_MYSQL MYSQL_TYPE_DATE)
        Exception(DatabaseTypeCastError("database type cast to time mismatch"));
    return _MSTL time::parse(cursor_[n]);
}

_MSTL datetime mysql_result::get_datetime(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    if (column_types_->at(n) != _MSTL_MYSQL MYSQL_TYPE_DATETIME)
        Exception(DatabaseTypeCastError("database type cast to datetime mismatch"));
    return _MSTL datetime::parse(cursor_[n]);
}

_MSTL timestamp mysql_result::get_timestamp(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    if (column_types_->at(n) != _MSTL_MYSQL MYSQL_TYPE_TIMESTAMP)
        Exception(DatabaseTypeCastError("database type cast to timestamp mismatch"));
    return _MSTL timestamp(_MSTL datetime::parse(cursor_[n]));
}

MSTL_END_NAMESPACE__
#endif
