#include <NeForce/db/mysql/mysql_result.hpp>
#ifdef NEFORCE_SUPPORT_MYSQL
NEFORCE_BEGIN_NAMESPACE__

mysql_result::mysql_result() :
column_name_(make_unique<vector<string_view>>()),
column_types_(make_unique<vector<::enum_field_types>>()) {}

mysql_result::mysql_result(::MYSQL_RES* result) :
result_(result),
rows_(::mysql_num_rows(result)),
columns_(::mysql_num_fields(result)),
column_name_(make_unique<vector<string_view>>()),
column_types_(make_unique<vector<::enum_field_types>>()) {
    const ::MYSQL_FIELD* field = nullptr;
    while ((field = ::mysql_fetch_field(result)) != nullptr) {
        column_name_->push_back(field->name);
        column_types_->push_back(field->type);
    }
}

mysql_result::~mysql_result() {
    if (result_ != nullptr) {
        ::mysql_free_result(result_);
    }
}

bool mysql_result::next() noexcept {
    if (!empty()) {
        cursor_ = ::mysql_fetch_row(result_);
        return cursor_ != nullptr;
    }
    return false;
}

string_view mysql_result::get(const size_type n) const noexcept {
    NEFORCE_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    NEFORCE_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    return cursor_[n];
}

bool mysql_result::get_bool(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    NEFORCE_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    if (column_types_->at(n) != ::MYSQL_TYPE_BOOL) {
        NEFORCE_THROW_EXCEPTION(database_typecast_exception("database type cast to bool mismatch"));
    }
    return boolean::parse(cursor_[n]).value();
}

int16_t mysql_result::get_int16(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    NEFORCE_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    const auto type = column_types_->at(n);
    if (type != ::MYSQL_TYPE_SHORT && type != ::MYSQL_TYPE_TINY && type != ::MYSQL_TYPE_BOOL) {
        NEFORCE_THROW_EXCEPTION(database_typecast_exception("database type cast to int16 mismatch"));
    }
    return integer16::parse(cursor_[n]).value();
}

int32_t mysql_result::get_int32(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    NEFORCE_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    const auto type = column_types_->at(n);
    if (type != ::MYSQL_TYPE_LONG && type != ::MYSQL_TYPE_INT24 && type != ::MYSQL_TYPE_SHORT &&
        type != ::MYSQL_TYPE_TINY && type != ::MYSQL_TYPE_BOOL) {
        NEFORCE_THROW_EXCEPTION(database_typecast_exception("database type cast to int32 mismatch"));
    }
    return integer32::parse(cursor_[n]).value();
}

int64_t mysql_result::get_int64(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    NEFORCE_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    const auto type = column_types_->at(n);
    if (type != ::MYSQL_TYPE_LONGLONG && type != ::MYSQL_TYPE_LONG && type != ::MYSQL_TYPE_INT24 &&
        type != ::MYSQL_TYPE_SHORT && type != ::MYSQL_TYPE_TINY && type != ::MYSQL_TYPE_BOOL) {
        NEFORCE_THROW_EXCEPTION(database_typecast_exception("database type cast to int64 mismatch"));
    }
    return integer64::parse(cursor_[n]).value();
}

float32_t mysql_result::get_float32(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    NEFORCE_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    const auto type = column_types_->at(n);
    if (type != ::MYSQL_TYPE_FLOAT && type != ::MYSQL_TYPE_LONG && type != ::MYSQL_TYPE_SHORT &&
        type != ::MYSQL_TYPE_TINY) {
        NEFORCE_THROW_EXCEPTION(database_typecast_exception("database type cast to float32 mismatch"));
    }
    return float32::parse(cursor_[n]).value();
}

float64_t mysql_result::get_float64(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    NEFORCE_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    const auto type = column_types_->at(n);
    if (type != ::MYSQL_TYPE_DOUBLE && type != ::MYSQL_TYPE_FLOAT && type != ::MYSQL_TYPE_LONGLONG &&
        type != ::MYSQL_TYPE_LONG && type != ::MYSQL_TYPE_SHORT && type != ::MYSQL_TYPE_TINY) {
        NEFORCE_THROW_EXCEPTION(database_typecast_exception("database type cast to float64 mismatch"));
    }
    return float64::parse(cursor_[n]).value();
}

decimal_t mysql_result::get_decimal(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    NEFORCE_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    const auto type = column_types_->at(n);
    if (type != ::MYSQL_TYPE_DECIMAL && type != ::MYSQL_TYPE_NEWDECIMAL && type != ::MYSQL_TYPE_DOUBLE &&
        type != ::MYSQL_TYPE_FLOAT && type != ::MYSQL_TYPE_LONGLONG && type != ::MYSQL_TYPE_LONG &&
        type != ::MYSQL_TYPE_SHORT && type != ::MYSQL_TYPE_TINY) {
        NEFORCE_THROW_EXCEPTION(database_typecast_exception("database type cast to decimal mismatch"));
    }
    return decimal::parse(cursor_[n]).value();
}

vector<char> mysql_result::get_blob(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    NEFORCE_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    const auto type = column_types_->at(n);
    if (type != ::MYSQL_TYPE_BLOB && type != ::MYSQL_TYPE_TINY_BLOB && type != ::MYSQL_TYPE_MEDIUM_BLOB &&
        type != ::MYSQL_TYPE_LONG_BLOB) {
        NEFORCE_THROW_EXCEPTION(database_typecast_exception("database type cast to blob mismatch"));
    }
    return {cursor_[n], cursor_[n] + ::mysql_fetch_lengths(result_)[n]};
}

uint64_t mysql_result::get_bit(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    NEFORCE_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    if (column_types_->at(n) != ::MYSQL_TYPE_BIT) {
        NEFORCE_THROW_EXCEPTION(database_typecast_exception("database type cast to BIT mismatch"));
    }
    const unsigned long length = mysql_fetch_lengths(result_)[n];
    const char* data = cursor_[n];

    uint64_t value = 0;
    for (unsigned long i = 0; i < length; ++i) {
        value = (value << 8) | static_cast<byte_t>(data[i]);
    }
    return value;
}

date mysql_result::get_date(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    NEFORCE_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    if (column_types_->at(n) != ::MYSQL_TYPE_DATE) {
        NEFORCE_THROW_EXCEPTION(database_typecast_exception("database type cast to date mismatch"));
    }
    return date::parse(cursor_[n]);
}

time mysql_result::get_time(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    NEFORCE_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    if (column_types_->at(n) != ::MYSQL_TYPE_TIME) {
        NEFORCE_THROW_EXCEPTION(database_typecast_exception("database type cast to time mismatch"));
    }
    return time::parse(cursor_[n]);
}

datetime mysql_result::get_datetime(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    NEFORCE_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    if (column_types_->at(n) != ::MYSQL_TYPE_DATETIME) {
        NEFORCE_THROW_EXCEPTION(database_typecast_exception("database type cast to datetime mismatch"));
    }
    return datetime::parse(cursor_[n]);
}

timestamp mysql_result::get_timestamp(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    NEFORCE_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    if (column_types_->at(n) != ::MYSQL_TYPE_TIMESTAMP) {
        NEFORCE_THROW_EXCEPTION(database_typecast_exception("database type cast to timestamp mismatch"));
    }
    return timestamp(datetime::parse(cursor_[n]));
}

column_meta mysql_result::column_metadata(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    column_meta meta;
    meta.name = (*column_name_)[n];
    meta.type = static_cast<int32_t>((*column_types_)[n]);
    return meta;
}

NEFORCE_END_NAMESPACE__
#endif
